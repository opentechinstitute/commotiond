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
#include "list.h"

static co_obj_t *processes = NULL;
static co_obj_t *sockets = NULL;
static co_obj_t *timers = NULL;
static struct epoll_event *events = NULL;
static bool loop_sigchld = false;
static bool loop_exit = false;
static int poll_fd = -1;

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

static co_obj_t *_co_loop_match_socket_i(co_obj_t *list, co_obj_t *sock, void *uri) {
  const co_socket_t *this_sock = (co_socket_t*)sock;
  const char *this_uri = uri;
  if((strcmp(this_sock->uri, this_uri)) == 0) return sock;
  return NULL;
}

static co_obj_t *_co_loop_match_process_i(co_obj_t *list, co_obj_t *proc, void *pid) {
  const co_process_t *this_proc = (co_process_t*)proc;
  const pid_t *this_pid = pid;
  if(this_proc->pid == *this_pid) return proc;
  return NULL;
}

static long _co_loop_tv_diff(const struct timeval *t1, const struct timeval *t2)
{
  return
  (t1->tv_sec - t2->tv_sec) * 1000 +
  (t1->tv_usec - t2->tv_usec) / 1000;
}

static co_obj_t *_co_loop_compare_timer_i(co_obj_t *list, co_obj_t *timer, void *time) {
  const co_timer_t *this_timer = (co_timer_t*)timer;
  const struct timeval *this_time = time;
  if(_co_loop_tv_diff(&this_timer->deadline, this_time) > 0) return timer;
  return NULL;
}

static co_obj_t *_co_loop_match_timer_i(co_obj_t *list, co_obj_t *timer, void *ptr) {
  const co_timer_t *this_timer = (co_timer_t*)timer;
  const void *match_ptr = ptr;
  if(this_timer->ptr == match_ptr) return timer;
  return NULL;
}

static co_obj_t *_co_loop_hangup_socket_i(co_obj_t *list, co_obj_t *sock, void *efd) {
  co_socket_t *this_sock = (co_socket_t*)sock;
  int *this_efd = efd;

  if((this_sock->fd == *this_efd) || (this_sock->rfd == *this_efd)) {
    this_sock->hangup(sock, efd);
  }

  return NULL;
}

static co_obj_t *_co_loop_poll_socket_i(co_obj_t *list, co_obj_t *sock, void *efd) {
  co_socket_t *this_sock = (co_socket_t*)sock;
  int *this_efd = efd;

  if((this_sock->fd == *this_efd) || (this_sock->rfd == *this_efd)) {
    this_sock->poll_cb(sock, efd);
  }

  return NULL;
}

static co_obj_t *_co_loop_poll_process_i(co_obj_t *list, co_obj_t *proc, void *pid) {
  pid_t *this_pid = pid;
  co_process_t *this_proc = (co_process_t*)proc;
  
  if(this_proc->pid == *this_pid) {
    co_loop_remove_process(*this_pid);
    this_proc->destroy(proc);
  }

  return NULL;
}

static co_obj_t *_co_loop_destroy_socket_i(co_obj_t *list, co_obj_t *sock, void *context) {
  if (IS_SOCK(sock))
    ((co_socket_t*)sock)->destroy(sock);
  return NULL;
}

static co_obj_t *_co_loop_destroy_process_i(co_obj_t *list, co_obj_t *proc, void *context) {
  if (IS_PROCESS(proc))
    ((co_process_t*)proc)->destroy(proc);
  return NULL;
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
      co_list_parse(sockets, _co_loop_hangup_socket_i, &events[i].data.fd);
    } else {
      co_list_parse(sockets, _co_loop_poll_socket_i, &events[i].data.fd);
    }
  }

  return;
}

static void _co_loop_poll_processes(void) {
  loop_sigchld = false;
  pid_t pid;
  if((pid = waitpid(-1, NULL, WNOHANG)) <= 0) return;
  co_list_parse(processes, _co_loop_poll_process_i, &pid);
  return;
}

static void _co_loop_process_timers(struct timeval *now) {
  co_timer_t *timer = NULL;
  
  while (co_list_length(timers)) {
    timer = (co_timer_t*)_LIST_FIRST(timers);
    if (_co_loop_tv_diff(&timer->deadline,now) > 0)
      break;
    if (co_loop_remove_timer((co_obj_t*)timer,NULL) == 0) {
      ERROR("Failed to process timer %ld.%06ld",timer->deadline.tv_sec,timer->deadline.tv_usec);
    }
    // call the timer's callback function:
    timer->timer_cb((co_obj_t*)timer,NULL);
  }
}

/** fill timeval tv with the current time */
static void _co_loop_gettime(struct timeval *tv)
{
  struct timespec ts;
  
  clock_gettime(CLOCK_REALTIME, &ts);
  tv->tv_sec = ts.tv_sec;
  tv->tv_usec = ts.tv_nsec / 1000;
}

static int _co_loop_get_next_deadline(struct timeval *now) {
  co_timer_t *timer;
  
  if (co_list_length(timers) == 0)
    return 0;
  
  timer = (co_timer_t*)_LIST_FIRST(timers);
  return _co_loop_tv_diff(&timer->deadline,now);
}

//Public functions

int co_loop_create(void) {
  if (poll_fd >= 0) {
    WARN("Event loop already created.");
		return 0;
  }

  CHECK((poll_fd = epoll_create1(EPOLL_CLOEXEC)) != -1, "Failed to create epoll event.");
	
  processes = co_list16_create();
  sockets = co_list16_create();
  timers = co_list16_create();
  events = calloc(LOOP_MAXEVENT, sizeof(struct epoll_event));
  loop_exit = false;
  return 1;

error:
  ERROR("Event loop creation failed, clearing lists.");
  co_obj_free(processes);
  co_obj_free(sockets);
  co_obj_free(timers);
  free(events);
  return 0;
}

int co_loop_destroy(void) {
  close(poll_fd);
  free(events);
  poll_fd = -1;
  if(processes != NULL) {
    co_list_parse(processes, _co_loop_destroy_process_i, NULL);
    co_obj_free(processes);
  }
  if(sockets != NULL) {
    co_list_parse(sockets, _co_loop_destroy_socket_i, NULL);
    co_obj_free(sockets);
  }
  if(timers != NULL) {
    co_obj_free(timers);
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

int co_loop_add_process(co_obj_t *proc) {
  CHECK(IS_PROCESS(proc),"Not a process.");
  CHECK(co_list_append(processes,proc),"Failed to add process %d",((co_process_t*)proc)->pid);
  ((co_process_t*)proc)->registered = true; 
  return 1;
error:
  return 0;
}

int co_loop_remove_process(pid_t pid) {
  co_obj_t *proc = NULL;
  CHECK((proc = co_list_parse(processes, _co_loop_match_process_i, &pid)), "Failed to delete process %d!", pid);
  proc = co_list_delete(processes, proc);
  ((co_process_t*)proc)->registered = false; 
  return 1;

error:
  return 0;
}

int co_loop_add_socket(co_obj_t *new_sock, co_obj_t *context) {
  DEBUG("Adding socket to event loop.");
  CHECK(IS_SOCK(new_sock),"Not a socket.");
  co_socket_t *sock = (co_socket_t*)new_sock;
  co_obj_t *node = NULL;
  struct epoll_event event;

  memset(&event, 0, sizeof(struct epoll_event));
  event.events = EPOLLIN;

  if((node = co_list_parse(sockets, _co_loop_match_socket_i, sock->uri))) {
    CHECK((node == (co_obj_t*)sock), "Different socket with URI %s already registered.", sock->uri);
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
      co_list_append(sockets, (co_obj_t*)sock);
      return 1;
  } else {
      co_loop_remove_socket((co_obj_t*)sock, NULL);
      SENTINEL("Unknown error registering socket %s.", sock->uri);
  }

error:
  return 0;
}

int co_loop_remove_socket(co_obj_t *old_sock, co_obj_t *context) {
  co_socket_t *sock = (co_socket_t*)old_sock;
  co_obj_t *node = NULL;
  CHECK((node = co_list_parse(sockets, _co_loop_match_socket_i, sock->uri)), "Failed to delete socket %s!", sock->uri);
  co_list_delete(sockets, node);
  sock->fd_registered = false; 
  sock->rfd_registered = false; 
  epoll_ctl(poll_fd, EPOLL_CTL_DEL, sock->fd, NULL);
  epoll_ctl(poll_fd, EPOLL_CTL_DEL, sock->rfd, NULL);
  return 1;

error:
  return 0;
}

int co_loop_add_timer(co_obj_t *new_timer, co_obj_t *context) {
  co_timer_t *timer = (co_timer_t*)new_timer;
  co_obj_t *node = NULL;
  struct timeval now;
  
  if (timer->pending)
    return 0;
  
  _co_loop_gettime(&now);
  CHECK(timer->timer_cb,"No callback function associated with timer");
  CHECK(_co_loop_tv_diff(&timer->deadline,&now) > 0,"Invalid timer deadline");
    
  CHECK(co_list_parse(timers,_co_loop_match_timer_i,timer->ptr) == NULL,"Timer already scheduled");
  
  // insert into list in chronological order
  if((node = co_list_parse(timers, _co_loop_compare_timer_i, &timer->deadline))) {
    CHECK(co_list_insert_before(timers,(co_obj_t*)timer,node),"Failed to insert timer.");
  } else {
    CHECK(co_list_append(timers,(co_obj_t*)timer),"Failed to insert timer.");
  }
  
  DEBUG("Successfully added timer %ld.%06ld",timer->deadline.tv_sec,timer->deadline.tv_usec);
  
  timer->pending = true;
  return 1;
  
error:
  return 0;
}

int co_loop_remove_timer(co_obj_t *old_timer, co_obj_t *context) {
  co_timer_t *timer = (co_timer_t*)old_timer;
  co_obj_t *node = NULL;
  
  if (!timer->pending)
    return 0;
  
  // remove from list
  CHECK((node = co_list_parse(timers, _co_loop_match_timer_i, timer->ptr)),
	"Failed to delete timer %ld.%06ld %p",timer->deadline.tv_sec,timer->deadline.tv_usec,timer->ptr);
  co_list_delete(timers,node);
  
  timer->pending = false;
  return 1;

error:
  return 0;
}

int co_loop_set_timer(co_obj_t *old_timer, long msecs, co_obj_t *context) {
  co_timer_t *timer = (co_timer_t*)old_timer;
  struct timeval *deadline = &timer->deadline;
  
  if (timer->pending)
    co_loop_remove_timer((co_obj_t*)timer,context);
  
  _co_loop_gettime(&timer->deadline);
  
  deadline->tv_sec += msecs / 1000;
  deadline->tv_usec += (msecs % 1000) * 1000;
  
  if (deadline->tv_usec > 1000000) {
    deadline->tv_sec++;
    deadline->tv_usec %= 1000000;
  }
  
  return co_loop_add_timer((co_obj_t*)timer,context);
}

co_obj_t *co_timer_create(struct timeval deadline, co_cb_t timer_cb, void *ptr) {
  co_timer_t *new_timer = h_calloc(1,sizeof(co_timer_t));
  
  new_timer->_header._type = _ext8;
  new_timer->_exttype = _co_timer;
  new_timer->_len = sizeof(co_timer_t);
  new_timer->pending = false;
  new_timer->deadline = (deadline.tv_sec == 0 && deadline.tv_usec == 0) ? deadline : (struct timeval){0};
  new_timer->timer_cb = timer_cb ? timer_cb : NULL;
  new_timer->ptr = ptr ? ptr : (void*)new_timer;
  
  return (co_obj_t*)new_timer;
}
