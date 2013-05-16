/* vim: set ts=2 expandtab: */
/**
 *       @file  socket.c
 *      @brief  a simple object-oriented socket wrapper
 *              object model inspired by Zed Shaw
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
#include <string.h>
#include <stdio.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/socket.h>
#include <sys/un.h>
#include "debug.h"
#include "socket.h"
#include "util.h"

co_socket_t unix_socket_proto = {
  .init = unix_socket_init,
  .bind = unix_socket_bind,
  .connect = unix_socket_connect
};

co_socket_t *co_socket_create(size_t size, co_socket_t proto) {

  if(!proto.init) proto.init = NULL;
  if(!proto.destroy) proto.destroy = co_socket_destroy;
  if(!proto.hangup) proto.hangup = co_socket_hangup;
  if(!proto.bind) proto.bind = NULL;
  if(!proto.connect) proto.connect = NULL;
  if(!proto.send) proto.send = co_socket_send;
  if(!proto.receive) proto.receive = co_socket_receive;
  if(!proto.setopt) proto.setopt = co_socket_setopt;
  if(!proto.getopt) proto.getopt = co_socket_getopt;
  co_socket_t *new_sock = malloc(size);
  *new_sock = proto;
  
  if((proto.init != NULL) && (!new_sock->init(new_sock))) {
    SENTINEL("Failed to initialize new socket.");
  } else {
    return new_sock;
  }

error:
  new_sock->destroy(new_sock);
  return NULL;
}

int co_socket_init(void *self) {
  if(self) {
    co_socket_t *this = self;
    this->local = malloc(sizeof(struct sockaddr_storage));
    this->remote = malloc(sizeof(struct sockaddr_storage));
    this->fd = -1;
    this->rfd = -1;
    this->fd_registered = false;
    this->rfd_registered = false;
    this->listen = false;
    return 1;
  } else return 0;
}

int co_socket_destroy(void *self) {
  if(self) {
    co_socket_t *this = self;
    close(this->fd);
    close(this->rfd);
    free(this->local);
    free(this->remote);
    free(this);
    return 1;
  } else return 0;
}

int co_socket_hangup(void *self, void *context) {
  CHECK_MEM(self);
  co_socket_t *this = self;
  if((this->listen) && (this->rfd >= 0)) {
    CHECK((close(this->rfd) != -1), "Failed to close socket.");
    this->rfd = -1;
    this->rfd_registered = false;
    return 1;
  } else if (this->fd >= 0) {
    CHECK((close(this->fd) != -1), "Failed to close socket.");
    this->fd = -1;
    this->fd_registered = false;
    return 1;
  }

error:
  ERROR("Failed to hangup any file descriptors for this socket.");
  return 0;
}


int co_socket_send(void *self, char *outgoing, size_t length) {
  CHECK_MEM(self);
  co_socket_t *this = self;
  unsigned int sent = 0;
  unsigned int remaining = length;
  int n, sfd;
  if(this->listen && (this->rfd >= 0)) {
    sfd = this->rfd;
  } else sfd = this->fd;

  while(sent < length) {
    n = send(sfd, outgoing+sent, remaining, 0);
    if(n < 0) break;
    sent += n;
    remaining -= n;
  }

  DEBUG("Sent %d bytes.", sent);
  return sent;

error:
  return -1;
}

int co_socket_receive(void *self, char *incoming, size_t length) {
  CHECK_MEM(self);
  co_socket_t *this = self;
  int received = 0;
  int rfd = 0;
  if(this->listen) {
    DEBUG("Receiving on listening socket.");
    if(this->rfd < 0) {
      socklen_t size = sizeof(*(this->remote));
      DEBUG("Accepting connection (fd=%d).", this->fd);
      CHECK((rfd = accept(this->fd, (struct sockaddr *) this->remote, &size)) != -1, "Failed to accept connection.");
      DEBUG("Accepted connection (fd=%d).", rfd);
      this->rfd = rfd;
      int flags = fcntl(this->rfd, F_GETFL, 0);
      fcntl(this->rfd, F_SETFL, flags | O_NONBLOCK); //Set non-blocking.
      if(this->register_cb) this->register_cb(this, NULL);
      return 0;
    } else {
      DEBUG("Setting receiving file descriptor %d.", this->rfd);
      rfd = this->rfd;
    }
  } else if(this->fd >= 0) {
    DEBUG("Setting listening file descriptor %d.", this->fd);
    rfd = this->fd;
  } else ERROR("No valid file descriptor found in socket!"); 

  DEBUG("Attempting to receive data on FD %d.", rfd);
  CHECK((received = recv(rfd, incoming, length, 0)) >= 0, "Error receiving data from socket.");
  return received;

error:
  return -1;
}

int co_socket_setopt(void * self, int level, int option, void *optval, socklen_t optvallen) {
  co_socket_t *this = self;

  //Check to see if this is a standard socket option, or needs custom handling.
  if(level <= MAX_IPPROTO) {
    CHECK(!setsockopt(this->fd, level, option, optval, optvallen), "Problem setting socket options.");
  } else {
    SENTINEL("No custom socket options defined!");
  }

  return 1;

error:
  return 0;
}

int co_socket_getopt(void * self, int level, int option, void *optval, socklen_t optvallen) {
  co_socket_t *this = self;

  //Check to see if this is a standard socket option, or needs custom handling.
  if(level <= MAX_IPPROTO) {
    CHECK(!getsockopt(this->fd, level, option, optval, &optvallen), "Problem setting socket options.");
  } else {
    SENTINEL("No custom socket options defined!");
  }

  return 1;

error:
  return 0;
}

int unix_socket_init(void *self) {
  if(self) {
    co_socket_t *this = self;
    this->fd = -1;
    this->rfd = -1;
    this->local = malloc(sizeof(struct sockaddr_un));
    this->remote = malloc(sizeof(struct sockaddr_un));
    this->fd_registered = false;
    this->rfd_registered = false;
    this->listen = false;
    this->uri = strdup("unix://");
    return 1;
  } else return 0;
}

int unix_socket_bind(void *self, const char *endpoint) {
  DEBUG("Binding unix_socket %s.", endpoint);
  unix_socket_t *this = self;
  struct sockaddr_un *address = (struct sockaddr_un *)this->_(local);
  CHECK_MEM(address);
  address->sun_family = AF_UNIX;
  CHECK((sizeof(endpoint) <= sizeof(address->sun_path)), "Endpoint %s is too large for socket path.", endpoint);
  strcpy(address->sun_path, endpoint);
  unlink(address->sun_path);

  //Initialize socket.
  CHECK((this->_(fd) = socket(AF_UNIX, SOCK_STREAM, 0)) != -1, "Failed to grab Unix socket file descriptor.");

  //Set some default socket options.
	const int optval = 1;
	setsockopt(this->_(fd), SOL_SOCKET, SO_REUSEADDR, &optval, sizeof(optval));
  //int flags = fcntl(this->_(fd), F_GETFL, 0);
  //fcntl(this->_(fd), F_SETFL, flags | O_NONBLOCK); //Set non-blocking.

  //Bind socket to file descriptor.
  socklen_t size = offsetof(struct sockaddr_un, sun_path) + sizeof(address->sun_path);
	CHECK(!bind(this->_(fd), (struct sockaddr *) address, size), "Failed to bind Unix socket %s.", address->sun_path);
  CHECK(!listen(this->_(fd), SOMAXCONN), "Failed to listen on Unix socket %s.", address->sun_path);
  this->_(listen) = true;
  if(this->_(register_cb)) this->_(register_cb)(this, NULL);
  return 1;

error:
	close(this->_(fd));
  return 0;
}

int unix_socket_connect(void *self, const char *endpoint) {
  DEBUG("Connecting unix_socket %s.", endpoint);
  unix_socket_t *this = self;
  struct sockaddr_un address = { .sun_family = AF_UNIX };
  FILE *f = NULL;

  CHECK((sizeof(endpoint) <= sizeof(address.sun_path)), "Endpoint %s is too large for socket path.", endpoint);
  CHECK(!(f = fopen(endpoint, "r")), "Invalid socket file path.");
  strcpy(address.sun_path, endpoint);

  //Initialize socket.
  CHECK((this->_(fd) = socket(AF_UNIX, SOCK_STREAM, 0)) != -1, "Failed to grab Unix socket file descriptor.");

  //Set some default socket options.
	const int optval = 1;
	setsockopt(this->_(fd), SOL_SOCKET, SO_REUSEADDR, &optval, sizeof(optval));
  //int flags = fcntl(this->_(fd), F_GETFL, 0);
  //fcntl(this->_(fd), F_SETFL, flags | O_NONBLOCK); //Set non-blocking.

  //Bind socket to file descriptor.
	CHECK(!connect(this->_(fd), (struct sockaddr *) &address, sizeof(address)) || errno == EINPROGRESS, "Failed to bind Unix socket.");
  return 1;

error:
	close(this->_(fd));
  return 0;
}

