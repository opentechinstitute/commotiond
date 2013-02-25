/*! 
 *
 * \file loop.c 
 *
 * \brief a simple object-oriented event loop
 *
 * \author Josh King <jking@chambana.net>
 * 
 * \date
 *
 * \copyright This file is part of Commotion, Copyright(C) 2012-2013 Josh King
 * 
 *            Commotion is free software: you can redistribute it and/or modify
 *            it under the terms of the GNU General Public License as published 
 *            by the Free Software Foundation, either version 3 of the License, 
 *            or (at your option) any later version.
 * 
 *            Commotion is distributed in the hope that it will be useful,
 *            but WITHOUT ANY WARRANTY; without even the implied warranty of
 *            MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *            GNU General Public License for more details.
 * 
 *            You should have received a copy of the GNU General Public License
 *            along with Commotion.  If not, see <http://www.gnu.org/licenses/>.
 * 
 * */

#include <stdlib.h>
#include <stddef.h>
#include <stdbool.h>
#include <unistd.h>
#include <sys/socket.h>
#include <sys/wait.h>
#include <sys/epoll.h>
#include <signal.h>
#include "extern/list.h"
#include "debug.h"
#include "process.h"
#include "socket.h"
#include "loop.h"

static list_t *processes = NULL;
static list_t *sockets = NULL;
static struct epoll_event *events = NULL;
static bool loop_sigchld = false;
static bool loop_exit = false;
static int poll_fd = -1;

//Private functions

static void loop_handle_signals(int sig) {
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

static void loop_setup_signals(void) {
  struct sigaction new_sigaction;
  sigset_t new_sigset;
  
  //Set signal mask - signals we want to block
  sigemptyset(&new_sigset);
  sigaddset(&new_sigset, SIGTSTP); //ignore TTY stop signals
  sigaddset(&new_sigset, SIGTTOU); //ignore TTY background writes
  sigaddset(&new_sigset, SIGTTIN); //ignore TTY background reads
  sigprocmask(SIG_BLOCK, &new_sigset, NULL); //block the above signals

  //Set up signal handler
  new_sigaction.sa_handler = loop_handle_signals;
  sigemptyset(&new_sigaction.sa_mask);
  new_sigaction.sa_flags = 0;

  //Signals to handle:
  sigaction(SIGCHLD, &new_sigaction, NULL); //catch child signal
  sigaction(SIGHUP, &new_sigaction, NULL); //catch hangup signal
  sigaction(SIGTERM, &new_sigaction, NULL); //catch term signal
  sigaction(SIGINT, &new_sigaction, NULL); //catch interrupt signal
}

static int loop_match_socket_i(const void *sock, const void *uri) {
  const socket_t *this_sock = sock;
  const char *this_uri = uri;
  DEBUG("Matching socket %s to uri %s", this_sock->uri, this_uri);
  if((strcmp(this_sock->uri, this_uri)) == 0) return 0;
  return -1;
}

static int loop_match_process_i(const void *proc, const void *pid) {
  const process_t *this_proc = proc;
  const pid_t *this_pid = pid;
  if(this_proc->pid == *this_pid) return 0;
  return -1;
}

static void loop_hangup_socket_i(list_t *list, lnode_t *lnode, void *context) {
  socket_t *sock = lnode_get(lnode);
  int *efd = context;
  DEBUG("Processing socket.");

  if((sock->fd == *efd) || (sock->rfd == *efd)) {
    sock->hangup(sock, context);
  }

  return;
}

static void loop_poll_socket_i(list_t *list, lnode_t *lnode, void *context) {
  socket_t *sock = lnode_get(lnode);
  int *efd = context;
  DEBUG("Processing socket.");

  if((sock->fd == *efd) || (sock->rfd == *efd)) {
    sock->poll_cb(sock, context);
  }

  return;
}

static void loop_poll_process_i(list_t *list, lnode_t *lnode, void *context) {
  pid_t *pid = context;
  process_t *proc = lnode_get(lnode);
  
  if(proc->pid == *pid) {
    loop_remove_process(*pid);
    proc->destroy(proc);
  }

  return;
}

static void loop_destroy_socket_i(list_t *list, lnode_t *lnode, void *context) {
  socket_t *sock = lnode_get(lnode);
  sock->destroy(sock);
  return;
}

static void loop_destroy_process_i(list_t *list, lnode_t *lnode, void *context) {
  process_t *proc = lnode_get(lnode);
  proc->destroy(proc);
  return;
}

static void loop_poll_sockets(void) {
  int n = epoll_wait(poll_fd, events, LOOP_MAXEVENT, 0);
  //DEBUG("Polling sockets: received %d epoll events.", n);
  
  for(int i = 0; i < n; i++) {
    if((events[i].events & EPOLLERR) || 
      (!(events[i].events & EPOLLIN))) {
      WARN("EPOLL Error!");
	    close (events[i].data.fd);
	    continue;
    } else if(events[i].events & EPOLLHUP) {
      DEBUG("Hanging up socket.");
      list_process(sockets, (void *)&events[i].data.fd, loop_hangup_socket_i);
	  } else {
      list_process(sockets, (void *)&events[i].data.fd, loop_poll_socket_i);
    }
  }

  return;
}

static void loop_poll_processes(void) {
  DEBUG("Polling processes.");
  loop_sigchld = false;
  pid_t pid;
  if((pid = waitpid(-1, NULL, WNOHANG)) <= 0) return;
  list_process(processes, (void *)&pid, loop_poll_process_i);
  return;
}


//Public functions

int loop_create(void) {
  if (poll_fd >= 0) {
    DEBUG("Event loop already created.");
		return 0;
  }

  CHECK((poll_fd = epoll_create1(EPOLL_CLOEXEC)) != -1, "Failed to create epoll event.");
	
  processes = list_create(LOOP_MAXPROC);
  sockets = list_create(LOOP_MAXSOCK);
  events = calloc(LOOP_MAXEVENT, sizeof(struct epoll_event));
  return 1;

error:
  DEBUG("Event loop creation failed, clearing lists.");
  list_destroy(processes);
  list_destroy_nodes(sockets);
  list_destroy(sockets);
  free(events);
  return 0;
}

int loop_destroy(void) {
  close(poll_fd);
  free(events);
  poll_fd = -1;
  if(processes != NULL) {
    list_process(processes, NULL, loop_destroy_process_i);
    list_destroy_nodes(processes);
    list_destroy(processes);
  }
  if(sockets != NULL) {
    list_process(sockets, NULL, loop_destroy_socket_i);
    list_destroy_nodes(sockets);
    list_destroy(sockets);
  }
  return 1;
}

void loop_start(void) {
  loop_setup_signals();
  //Main event loop.
  while(!loop_exit) {
    loop_poll_sockets();
    sleep(1);
		if (loop_exit) break;
		if (loop_sigchld) loop_poll_processes();
  }
  return;
}

void loop_stop(void) {
  loop_exit = true;
  return;
}

int loop_add_process(process_t *proc) {
  list_append(processes, lnode_create((void *)proc));
  proc->registered = true; 
  return 1;
}

int loop_remove_process(pid_t pid) {
  lnode_t *node;
  CHECK((node = list_find(processes, &pid, loop_match_process_i)) != NULL, "Failed to delete process %d!", pid);
  node = list_delete(processes, node);
  process_t *proc = lnode_get(node);
  proc->registered = false; 
  return 1;

error:
  return 0;
}

int loop_add_socket(void *new_sock, void *context) {
  DEBUG("Adding socket to event loop.");
  socket_t *sock = new_sock;
  lnode_t *node;
  struct epoll_event event;
  event.events = EPOLLIN | EPOLLET;
  if((node = list_find(sockets, sock->uri, loop_match_socket_i))) {
    DEBUG("Searching registered sockets.");
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
      loop_remove_socket(sock, NULL);
      SENTINEL("Unknown error registering socket %s.", sock->uri);
  }

error:
  return 0;
}

int loop_remove_socket(void *old_sock, void *context) {
  socket_t *sock = old_sock;
  lnode_t *node;
  CHECK((node = list_find(sockets, &sock->uri, loop_match_socket_i)) != NULL, "Failed to delete socket %s!", sock->uri);
  list_delete(sockets, node);
  sock->fd_registered = false; 
  sock->rfd_registered = false; 
  epoll_ctl(poll_fd, EPOLL_CTL_DEL, sock->fd, NULL);
  epoll_ctl(poll_fd, EPOLL_CTL_DEL, sock->rfd, NULL);
  return 1;

error:
  return 0;
}
