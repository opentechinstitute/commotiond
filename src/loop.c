/* vim: set ts=2 expandtab: */
/**
 *       @file  loop.c
 *      @brief  a simple object-oriented event loop
 *
 *     @author  Josh King (jheretic), jking@chambana.net
 *
 *   @internal
 *     Created  03/07/2013
 *    Revision  $Id: doxygen.commotion.templates,v 0.1 2013/01/01 09:00:00 jheretic Exp $
 *    Compiler  gcc/g++
 *     Company  The Open Technology Institute
 *   Copyright  Copyright (c) 2013, Josh King
 *
 * This file is part of Commotion, Copyright (c) 2013, Josh King 
 * 
 * Commotion is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published 
 * by the Free Software Foundation, either version 3 of the License, 
 * or (at your option) any later version.
 * 
 * Commotion is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 * 
 * You should have received a copy of the GNU General Public License
 * along with Commotion.  If not, see <http://www.gnu.org/licenses/>.
 *
 * =====================================================================================
 */

#include <stdlib.h>
#include <stddef.h>
#include <stdbool.h>
#include <unistd.h>
#include <sys/socket.h>
#include <sys/wait.h>
#include <sys/epoll.h>
#include <signal.h>
#include <string.h>
#include <time.h>
#include "extern/list.h"
#include "debug.h"
#include "process.h"
#include "socket.h"
#include "loop.h"

static list_t *processes = NULL;
static list_t *sockets = NULL;
static list_t *timers = NULL;
static struct epoll_event *events = NULL;
static bool loop_sigchld = false;
static bool loop_exit = false;
static int poll_fd = -1;

co_timer_t co_timer_proto = {
  .pending = false,
  .timer_cb = NULL,
  .deadline = {0}
};

//Private functions

static void _co_loop_handle_signals(int sig) {
  switch(sig) {
    case SIGHUP:
      DEBUG("Received SIGHUP signal.");
      break;
    case SIGCHLD:
      DEBUG("Received SIGCHLD.");
      loop_sigchld = true;
      break;
    case SIGINT:
    case SIGTERM:
      DEBUG("Loop exiting.");
      loop_exit = true;
      break;
    default:
      WARN("Unhandled signal %s", strsignal(sig));
      break;
  }
}

static void _co_loop_setup_signals(void) {
  struct sigaction new_sigaction;
  sigset_t new_sigset;
  
  //Set signal mask - signals we want to block
  sigemptyset(&new_sigset);
  sigaddset(&new_sigset, SIGTSTP); //ignore TTY stop signals
  sigaddset(&new_sigset, SIGTTOU); //ignore TTY background writes
  sigaddset(&new_sigset, SIGTTIN); //ignore TTY background reads
  sigprocmask(SIG_BLOCK, &new_sigset, NULL); //block the above signals

  //Set up signal handler
  new_sigaction.sa_handler = _co_loop_handle_signals;
  sigemptyset(&new_sigaction.sa_mask);
  new_sigaction.sa_flags = 0;

  //Signals to handle:
  sigaction(SIGCHLD, &new_sigaction, NULL); //catch child signal
  sigaction(SIGHUP, &new_sigaction, NULL); //catch hangup signal
  sigaction(SIGTERM, &new_sigaction, NULL); //catch term signal
  sigaction(SIGINT, &new_sigaction, NULL); //catch interrupt signal
}

static int _co_loop_match_socket_i(const void *sock, const void *uri) {
  const co_socket_t *this_sock = sock;
  const char *this_uri = uri;
  if((strcmp(this_sock->uri, this_uri)) == 0) return 0;
  return -1;
}

static int _co_loop_match_process_i(const void *proc, const void *pid) {
  const co_process_t *this_proc = proc;
  const pid_t *this_pid = pid;
  if(this_proc->pid == *this_pid) return 0;
  return -1;
}

static int _co_loop_tv_diff(const struct timeval *t1, const struct timeval *t2)
{
  return
  (t1->tv_sec - t2->tv_sec) * 1000 +
  (t1->tv_usec - t2->tv_usec) / 1000;
}

static int _co_loop_compare_timer_i(const void *timer, const void *time) {
  const co_timer_t *this_timer = timer;
  const struct timeval *this_time = time;
  if(_co_loop_tv_diff(&this_timer->deadline, this_time) > 0) return 0;
  return -1;
}

static int _co_loop_match_timer_i(const void *timer, const void *time) {
  const co_timer_t *this_timer = timer;
  const struct timeval *this_time = time;
  if(_co_loop_tv_diff(&this_timer->deadline, this_time) == 0) return 0;
  return -1;
}

static void _co_loop_hangup_socket_i(list_t *list, lnode_t *lnode, void *context) {
  co_socket_t *sock = lnode_get(lnode);
  int *efd = context;

  if((sock->fd == *efd) || (sock->rfd == *efd)) {
    sock->hangup(sock, context);
  }

  return;
}

static void _co_loop_poll_socket_i(list_t *list, lnode_t *lnode, void *context) {
  co_socket_t *sock = lnode_get(lnode);
  int *efd = context;

  if((sock->fd == *efd) || (sock->rfd == *efd)) {
    sock->poll_cb(sock, context);
  }

  return;
}

static void _co_loop_poll_process_i(list_t *list, lnode_t *lnode, void *context) {
  pid_t *pid = context;
  co_process_t *proc = lnode_get(lnode);
  
  if(proc->pid == *pid) {
    co_loop_remove_process(*pid);
    proc->destroy(proc);
  }

  return;
}

static void _co_loop_destroy_socket_i(list_t *list, lnode_t *lnode, void *context) {
  co_socket_t *sock = lnode_get(lnode);
  sock->destroy(sock);
  return;
}

static void _co_loop_destroy_process_i(list_t *list, lnode_t *lnode, void *context) {
  co_process_t *proc = lnode_get(lnode);
  proc->destroy(proc);
  return;
}

static void _co_loop_poll_sockets(int deadline) {
  if (deadline < 0) deadline = 0;
  int n = epoll_wait(poll_fd, events, LOOP_MAXEVENT, deadline < LOOP_TIMEOUT ? deadline : LOOP_TIMEOUT);
  
  for(int i = 0; i < n; i++) {
    if((events[i].events & EPOLLERR) || 
      (!(events[i].events & EPOLLIN))) {
      WARN("EPOLL Error!");
	    close (events[i].data.fd);
	    continue;
    } else if(events[i].events & EPOLLHUP) {
      DEBUG("Hanging up socket.");
      list_process(sockets, (void *)&events[i].data.fd, _co_loop_hangup_socket_i);
	  } else {
      list_process(sockets, (void *)&events[i].data.fd, _co_loop_poll_socket_i);
    }
  }

  return;
}

static void _co_loop_poll_processes(void) {
  loop_sigchld = false;
  pid_t pid;
  if((pid = waitpid(-1, NULL, WNOHANG)) <= 0) return;
  list_process(processes, (void *)&pid, _co_loop_poll_process_i);
  return;
}

static void _co_loop_process_timers(struct timeval *now) {
  co_timer_t *timer = NULL;
  lnode_t *node = NULL;
  
  while (!list_isempty(timers)) {
    node = list_first(timers);
    timer = lnode_get(node);
    if (_co_loop_tv_diff(&timer->deadline,now) > 0)
      break;
    if (co_loop_remove_timer(timer,(void*)NULL) == 0) {
      ERROR("Failed to process timer %ld.%06ld",timer->deadline.tv_sec,timer->deadline.tv_usec);
    }
    // call the timer's callback function:
    timer->timer_cb(timer,(void*)NULL);
  }
}

/** fill timeval tv with the current time */
static void _co_loop_gettime(struct timeval *tv)
{
  struct timespec ts;
  
  clock_gettime(CLOCK_MONOTONIC, &ts);
  tv->tv_sec = ts.tv_sec;
  tv->tv_usec = ts.tv_nsec / 1000;
}

static int _co_loop_get_next_deadline(struct timeval *now) {
  co_timer_t *timer;
  lnode_t *node;
  
  if (list_isempty(timers))
    return 0;
  
  node = list_first(timers);
  timer = lnode_get(node);
  return _co_loop_tv_diff(&timer->deadline,now);
}

//Public functions

int co_loop_create(void) {
  if (poll_fd >= 0) {
    WARN("Event loop already created.");
		return 0;
  }

  CHECK((poll_fd = epoll_create1(EPOLL_CLOEXEC)) != -1, "Failed to create epoll event.");
	
  processes = list_create(LOOP_MAXPROC);
  sockets = list_create(LOOP_MAXSOCK);
  timers = list_create(LOOP_MAXTIMER);
  events = calloc(LOOP_MAXEVENT, sizeof(struct epoll_event));
  loop_exit = false;
  return 1;

error:
  ERROR("Event loop creation failed, clearing lists.");
  list_destroy(processes);
  list_destroy_nodes(sockets);
  list_destroy(sockets);
  list_destroy_nodes(timers);
  list_destroy(timers);
  free(events);
  return 0;
}

int co_loop_destroy(void) {
  close(poll_fd);
  free(events);
  poll_fd = -1;
  if(processes != NULL) {
    list_process(processes, NULL, _co_loop_destroy_process_i);
    list_destroy_nodes(processes);
    list_destroy(processes);
  }
  if(sockets != NULL) {
    list_process(sockets, NULL, _co_loop_destroy_socket_i);
    list_destroy_nodes(sockets);
    list_destroy(sockets);
  }
  if(timers != NULL) {
    list_destroy_nodes(timers);
    list_destroy(timers);
  }
  return 1;
}

void co_loop_start(void) {
  struct timeval now;
  _co_loop_setup_signals();
  //Main event loop.
  while(!loop_exit) {
    _co_loop_gettime(&now);
    _co_loop_process_timers(&now);
    
    _co_loop_gettime(&now);
    _co_loop_poll_sockets(_co_loop_get_next_deadline(&now));
    //sleep(1);
		if (loop_exit) break;
		if (loop_sigchld) _co_loop_poll_processes();
  }
  return;
}

void co_loop_stop(void) {
  loop_exit = true;
  return;
}

int co_loop_add_process(co_process_t *proc) {
  list_append(processes, lnode_create((void *)proc));
  proc->registered = true; 
  return 1;
}

int co_loop_remove_process(pid_t pid) {
  lnode_t *node;
  CHECK((node = list_find(processes, &pid, _co_loop_match_process_i)) != NULL, "Failed to delete process %d!", pid);
  node = list_delete(processes, node);
  co_process_t *proc = lnode_get(node);
  proc->registered = false; 
  return 1;

error:
  return 0;
}

int co_loop_add_socket(void *new_sock, void *context) {
  DEBUG("Adding socket to event loop.");
  co_socket_t *sock = new_sock;
  lnode_t *node;
  struct epoll_event event;

  memset(&event, 0, sizeof(struct epoll_event));
  event.events = EPOLLIN;

  if((node = list_find(sockets, sock->uri, _co_loop_match_socket_i))) {
    CHECK((lnode_get(node) == sock), "Different socket with URI %s already registered.", sock->uri);
    if((sock->listen) && (sock->rfd > 0) && (!sock->rfd_registered)) {
      DEBUG("Adding RFD %d to epoll.", sock->rfd);
      event.data.fd = sock->rfd;
      CHECK((epoll_ctl(poll_fd, EPOLL_CTL_ADD, sock->rfd, &event)) != -1, "Failed to add receive FD epoll event.");
      sock->rfd_registered = true; 
    } else {
      SENTINEL("Socket %s already registered.", sock->uri);
    }
  } else if((sock->listen) && (sock->fd > 0) && !sock->fd_registered) {
      DEBUG("Adding FD %d to epoll.", sock->fd);
      event.data.fd = sock->fd;
      CHECK((epoll_ctl(poll_fd, EPOLL_CTL_ADD, sock->fd, &event)) != -1, "Failed to add listen FD epoll event.");
      sock->fd_registered = true; 
      list_append(sockets, lnode_create((void *)sock));
      return 1;
  } else {
      co_loop_remove_socket(sock, NULL);
      SENTINEL("Unknown error registering socket %s.", sock->uri);
  }

error:
  return 0;
}

int co_loop_remove_socket(void *old_sock, void *context) {
  co_socket_t *sock = old_sock;
  lnode_t *node;
  CHECK((node = list_find(sockets, sock->uri, _co_loop_match_socket_i)) != NULL, "Failed to delete socket %s!", sock->uri);
  list_delete(sockets, node);
  sock->fd_registered = false; 
  sock->rfd_registered = false; 
  epoll_ctl(poll_fd, EPOLL_CTL_DEL, sock->fd, NULL);
  epoll_ctl(poll_fd, EPOLL_CTL_DEL, sock->rfd, NULL);
  return 1;

error:
  return 0;
}

int co_loop_add_timer(void *new_timer, void *context) {
  co_timer_t *timer = new_timer;
  lnode_t *node = NULL;
  struct timeval now;
  struct timeval *deadline = &timer->deadline;
  
  if (timer->pending)
    return 0;
  
  _co_loop_gettime(&now);
  CHECK(timer->timer_cb && _co_loop_tv_diff(&timer->deadline,&now) > 0,"Invalid timer");
  
  // each timer should have a unique timeval so we can identify them later for removal
  while ((node = list_find(timers, &timer->deadline, _co_loop_match_timer_i))) {
    deadline->tv_usec += 1000;
    if (deadline->tv_usec > 1000000) {
      deadline->tv_sec++;
      deadline->tv_usec %= 1000000;
    }
  }
  
  // insert into list in chronological order
  if((node = list_find(timers, &timer->deadline, _co_loop_compare_timer_i)))
    list_ins_before(timers, lnode_create((void *)timer), node);
  else
    list_append(timers, lnode_create((void *)timer));
  
  DEBUG("Successfully added timer %ld.%06ld",timer->deadline.tv_sec,timer->deadline.tv_usec);
  
  timer->pending = true;
  return 1;
  
error:
  return 0;
}

int co_loop_remove_timer(void *old_timer, void* context) {
  co_timer_t *timer = old_timer;
  lnode_t *node = NULL;
  
  if (!timer->pending)
    return 0;
  
  // remove from list
  CHECK((node = list_find(timers, &timer->deadline, _co_loop_match_timer_i)),
	"Failed to delete timer %ld.%06ld",timer->deadline.tv_sec,timer->deadline.tv_usec);
  list_delete(timers,node);
  
  timer->pending = false;
  return 1;

error:
  return 0;
}

int co_loop_set_timer(void *_timer, int msecs, void *context) {
  co_timer_t *timer = _timer;
  struct timeval *deadline = &timer->deadline;
  
  if (timer->pending)
    co_loop_remove_timer(timer,context);
  
  _co_loop_gettime(&timer->deadline);
  
  deadline->tv_sec += msecs / 1000;
  deadline->tv_usec += (msecs % 1000) * 1000;
  
  if (deadline->tv_usec > 1000000) {
    deadline->tv_sec++;
    deadline->tv_usec %= 1000000;
  }
  
  return co_loop_add_timer(timer,context);
}

co_timer_t *co_timer_create(size_t size, co_timer_t proto) {
  if (!proto.timer_cb) proto.timer_cb = NULL;
  if (proto.deadline.tv_sec == 0 && proto.deadline.tv_usec == 0) proto.deadline = (struct timeval){0};
  proto.pending = false;
  
  co_timer_t *new_timer = malloc(size);
  *new_timer = proto;
  return new_timer;
}
