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
#include "list.h"

co_obj_t *co_fd_create(co_obj_t *parent, int fd) {
  CHECK(IS_SOCK(parent),"Parent is not a socket.");
  co_fd_t *new_fd = h_calloc(1,sizeof(co_fd_t));
  new_fd->_header._type = _ext8;
  new_fd->_exttype = _fd;
  new_fd->_len = sizeof(co_fd_t);
  new_fd->socket = (co_socket_t*)parent;
  new_fd->fd = fd;
  return (co_obj_t*)new_fd;
error:
  return NULL;
}

co_socket_t unix_socket_proto = {
  .init = unix_socket_init,
  .bind = unix_socket_bind,
  .connect = unix_socket_connect
};

co_obj_t *co_socket_create(size_t size, co_socket_t proto) {

  if(!proto.init) proto.init = NULL;
  if(!proto.destroy) proto.destroy = co_socket_destroy;
  if(!proto.hangup) proto.hangup = co_socket_hangup;
  if(!proto.bind) proto.bind = NULL;
  if(!proto.connect) proto.connect = NULL;
  if(!proto.send) proto.send = co_socket_send;
  if(!proto.receive) proto.receive = co_socket_receive;
  if(!proto.setopt) proto.setopt = co_socket_setopt;
  if(!proto.getopt) proto.getopt = co_socket_getopt;
  co_socket_t *new_sock = h_calloc(1,size);
  *new_sock = proto;
  new_sock->_header._type = _ext8;
  new_sock->_exttype = _sock;
  new_sock->_len = size;
  
  if((proto.init != NULL) && (!new_sock->init((co_obj_t*)new_sock))) {
    SENTINEL("Failed to initialize new socket.");
  } else {
    return (co_obj_t*)new_sock;
  }

error:
  new_sock->destroy((co_obj_t*)new_sock);
  return NULL;
}

int co_socket_init(co_obj_t* self) {
  if(self && IS_SOCK(self)) {
    co_socket_t *this = (co_socket_t*)self;
    this->local = h_calloc(1,sizeof(struct sockaddr_storage));
    hattach(this->local,this);
    this->remote = h_calloc(1,sizeof(struct sockaddr_storage));
    hattach(this->remote,this);
    this->fd = (co_fd_t*)co_fd_create((co_obj_t*)this,-1);
    hattach(this->fd,this);
    this->rfd_lst = co_list16_create();
    hattach(this->rfd_lst,this);
    this->fd_registered = false;
    this->listen = false;
    this->events = 0;
    return 1;
  } else return 0;
}

static co_obj_t *_co_socket_close_fd_i(co_obj_t *list, co_obj_t *fd, void *context) {
  if (IS_FD(fd))
    close(((co_fd_t*)fd)->fd);
  return NULL;
}

int co_socket_destroy(co_obj_t* self) {
  if(self && IS_SOCK(self)) {
    co_socket_t *this = (co_socket_t*)self;
    if (this->fd->fd > 0) close(this->fd->fd);
    co_list_parse(this->rfd_lst,_co_socket_close_fd_i,NULL);
    co_obj_free(self);
    return 1;
  } else return 0;
}

int co_socket_hangup(co_obj_t *self, co_obj_t *context) {
  CHECK_MEM(self);
  CHECK(IS_SOCK(self),"Not a socket.");
  CHECK(IS_FD(context),"Not a FD.");
  co_socket_t *this = (co_socket_t*)self;
  co_fd_t *fd = (co_fd_t*)context;
  CHECK(fd->socket == this,"FD does not match socket");
  if (fd == this->fd) {
    CHECK(close(fd->fd) != -1,"Failed to close socket.");
    this->fd_registered = false;
    this->listen = false;
  } else {
    CHECK(co_list_contains(this->rfd_lst,(co_obj_t*)fd),"Socket does not contain FD");
    fd = (co_fd_t*)co_list_delete(this->rfd_lst,(co_obj_t*)fd);
    CHECK(close(fd->fd) != -1,"Failed to close socket.");
    co_obj_free((co_obj_t*)fd);
  }
  return 1;
error:
  ERROR("Failed to hangup any file descriptors for this socket.");
  return 0;
}

int co_socket_send(co_obj_t *self, char *outgoing, size_t length) {
  CHECK_MEM(self);
  CHECK(IS_FD(self),"Not a FD.");
  co_fd_t *this = (co_fd_t*)self;
  unsigned int sent = 0;
  unsigned int remaining = length;
  int n;

  while(sent < length) {
    n = send(this->fd, outgoing+sent, remaining, 0);
    if(n < 0) break;
    sent += n;
    remaining -= n;
  }

  DEBUG("Sent %d bytes.", sent);
  return sent;

error:
  return -1;
}

int co_socket_receive(co_obj_t *self, co_obj_t *fd, char *incoming, size_t length) {
  CHECK_MEM(self);
  CHECK(IS_SOCK(self),"Not a socket.");
  co_socket_t *this = (co_socket_t*)self;
  CHECK(((co_fd_t*)fd)->socket == this,"FD does not match socket");
  int received = 0;
  int rfd = 0;
  if(this->listen) {
    DEBUG("Receiving on listening socket.");
    if((co_fd_t*)fd == this->fd) {
      socklen_t size = sizeof(*(this->remote));
      DEBUG("Accepting connection (fd=%d).", this->fd->fd);
      CHECK((rfd = accept(this->fd->fd, (struct sockaddr *) this->remote, &size)) != -1, "Failed to accept connection.");
      DEBUG("Accepted connection (fd=%d).", rfd);
      co_obj_t *new_rfd = co_fd_create((co_obj_t*)this,rfd);
      CHECK(co_list_append(this->rfd_lst,new_rfd),"Failed to append rfd");
      int flags = fcntl(rfd, F_GETFL, 0);
      fcntl(rfd, F_SETFL, flags | O_NONBLOCK); //Set non-blocking.
      if(this->register_cb) this->register_cb((co_obj_t*)this, new_rfd);
      return 0;
    } else {
      rfd = ((co_fd_t*)fd)->fd;
      DEBUG("Setting receiving file descriptor %d.", rfd);
    }
  } else if(this->fd->fd >= 0) {
    DEBUG("Setting listening file descriptor %d.", this->fd->fd);
    rfd = this->fd->fd;
  } else ERROR("No valid file descriptor found in socket!"); 

  DEBUG("Attempting to receive data on FD %d.", rfd);
  CHECK((received = recv(rfd, incoming, length, 0)) >= 0, "Error receiving data from socket.");
  return received;

error:
  return -1;
}

int co_socket_setopt(co_obj_t * self, int level, int option, void *optval, socklen_t optvallen) {
  CHECK(IS_SOCK(self),"Not a socket.");
  co_socket_t *this = (co_socket_t*)self;

  //Check to see if this is a standard socket option, or needs custom handling.
  if(level <= MAX_IPPROTO) {
    CHECK(!setsockopt(this->fd->fd, level, option, optval, optvallen), "Problem setting socket options.");
  } else {
    SENTINEL("No custom socket options defined!");
  }

  return 1;

error:
  return 0;
}

int co_socket_getopt(co_obj_t * self, int level, int option, void *optval, socklen_t optvallen) {
  CHECK(IS_SOCK(self),"Not a socket.");
  co_socket_t *this = (co_socket_t*)self;

  //Check to see if this is a standard socket option, or needs custom handling.
  if(level <= MAX_IPPROTO) {
    CHECK(!getsockopt(this->fd->fd, level, option, optval, &optvallen), "Problem setting socket options.");
  } else {
    SENTINEL("No custom socket options defined!");
  }

  return 1;

error:
  return 0;
}

int unix_socket_init(co_obj_t *self) {
  if(self && IS_SOCK(self)) {
    co_socket_t *this = (co_socket_t*)self;
    this->fd = (co_fd_t*)co_fd_create((co_obj_t*)this,-1);
    hattach(this->fd,this);
    this->rfd_lst = co_list16_create();
    hattach(this->rfd_lst,this);
    this->local = h_calloc(1,sizeof(struct sockaddr_un));
    hattach(this->local,this);
    this->remote = h_calloc(1,sizeof(struct sockaddr_un));
    hattach(this->remote,this);
    this->fd_registered = false;
    this->listen = false;
    this->uri = h_strdup("unix://");
    hattach(this->uri,this);
    this->events = 0;
    return 1;
  } else return 0;
}

int unix_socket_bind(co_obj_t *self, const char *endpoint) {
  DEBUG("Binding unix_socket %s.", endpoint);
  CHECK(IS_SOCK(self),"Not a socket.");
  unix_socket_t *this = (unix_socket_t*)self;
  struct sockaddr_un *address = (struct sockaddr_un *)this->_(local);
  CHECK_MEM(address);
  address->sun_family = AF_UNIX;
  CHECK((sizeof(endpoint) <= sizeof(address->sun_path)), "Endpoint %s is too large for socket path.", endpoint);
  strcpy(address->sun_path, endpoint);
  unlink(address->sun_path);

  //Initialize socket.
  CHECK((this->_(fd)->fd = socket(AF_UNIX, SOCK_STREAM, 0)) != -1, "Failed to grab Unix socket file descriptor.");

  //Set some default socket options.
	const int optval = 1;
	setsockopt(this->_(fd)->fd, SOL_SOCKET, SO_REUSEADDR, &optval, sizeof(optval));
  //int flags = fcntl(this->_(fd)->fd, F_GETFL, 0);
  //fcntl(this->_(fd)->fd, F_SETFL, flags | O_NONBLOCK); //Set non-blocking.
  //Set default timeout
  struct timeval timeout;
  timeout.tv_sec = 5;
  timeout.tv_usec = 0;
	setsockopt(this->_(fd)->fd, SOL_SOCKET, SO_RCVTIMEO, (char *)&timeout, sizeof(timeout));
	setsockopt(this->_(fd)->fd, SOL_SOCKET, SO_SNDTIMEO, (char *)&timeout, sizeof(timeout));

  //Bind socket to file descriptor.
  socklen_t size = offsetof(struct sockaddr_un, sun_path) + sizeof(address->sun_path);
	CHECK(!bind(this->_(fd)->fd, (struct sockaddr *) address, size), "Failed to bind Unix socket %s.", address->sun_path);
  CHECK(!listen(this->_(fd)->fd, SOMAXCONN), "Failed to listen on Unix socket %s.", address->sun_path);
  this->_(listen) = true;
  if(this->_(register_cb)) this->_(register_cb)((co_obj_t*)this, (co_obj_t*)this->_(fd));
  return 1;

error:
	close(this->_(fd)->fd);
  return 0;
}

int unix_socket_connect(co_obj_t *self, const char *endpoint) {
  DEBUG("Connecting unix_socket %s.", endpoint);
  CHECK(IS_SOCK(self),"Not a socket.");
  unix_socket_t *this = (unix_socket_t*)self;
  struct sockaddr_un address = { .sun_family = AF_UNIX };
  FILE *f = NULL;

  CHECK((sizeof(endpoint) <= sizeof(address.sun_path)), "Endpoint %s is too large for socket path.", endpoint);
  CHECK(!(f = fopen(endpoint, "r")), "Invalid socket file path.");
  strcpy(address.sun_path, endpoint);

  //Initialize socket.
  CHECK((this->_(fd)->fd = socket(AF_UNIX, SOCK_STREAM, 0)) != -1, "Failed to grab Unix socket file descriptor.");

  //Set some default socket options.
	const int optval = 1;
	setsockopt(this->_(fd)->fd, SOL_SOCKET, SO_REUSEADDR, &optval, sizeof(optval));
  //Set default timeout
  struct timeval timeout;
  timeout.tv_sec = 5;
  timeout.tv_usec = 0;
	setsockopt(this->_(fd)->fd, SOL_SOCKET, SO_RCVTIMEO, (char *)&timeout, sizeof(timeout));
	setsockopt(this->_(fd)->fd, SOL_SOCKET, SO_SNDTIMEO, (char *)&timeout, sizeof(timeout));
  //int flags = fcntl(this->_(fd)->fd, F_GETFL, 0);
  //fcntl(this->_(fd)->fd, F_SETFL, flags | O_NONBLOCK); //Set non-blocking.

  //Bind socket to file descriptor.
	CHECK(!connect(this->_(fd)->fd, (struct sockaddr *) &address, sizeof(address)) || errno == EINPROGRESS, "Failed to bind Unix socket.");
  return 1;

error:
	close(this->_(fd)->fd);
  return 0;
}

