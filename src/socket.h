/* vim: set ts=2 expandtab: */
/**
 *       @file  socket.h
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

#ifndef _SOCKET_H
#define _SOCKET_H

#include <stdlib.h>
#include <stddef.h>
#include <stdbool.h>

#define MAX_IPPROTO 255
#define MAX_CONNECTIONS 32

/*!
 * \struct socket
 * \
 *
 * */
typedef struct {
  char *uri;
  int fd; //socket file descriptor
  int rfd; //accept socket file descriptors
  bool fd_registered;
  bool rfd_registered;
  struct sockaddr* local;
  struct sockaddr* remote;
  bool listen;
  int (*init)(void *self);
  int (*destroy)(void *self);
  int (*hangup)(void *self, void *context);
  int (*bind)(void *self, const char *endpoint);
  int (*connect)(void *self, const char *endpoint);
  int (*send)(void *self, char *outgoing, size_t length);
  int (*receive)(void *self, char *incoming, size_t length);
  int (*setopt)(void *self, int level, int option, void *optval, socklen_t optvallen);
  int (*getopt)(void *self, int level, int option, void *optval, socklen_t optvallen);
  int (*poll_cb)(void *self, void *context);
  int (*register_cb)(void *self, void *context);
} co_socket_t;

co_socket_t *co_socket_create(size_t size, co_socket_t proto);

int co_socket_init(void *self);

int co_socket_destroy(void *self);

int co_socket_hangup(void *self, void *context); 

int co_socket_send(void *self, char *outgoing, size_t length);

int co_socket_receive(void * self, char *incoming, size_t length);

int co_socket_setopt(void * self, int level, int option, void *optval, socklen_t optvallen);

int co_socket_getopt(void * self, int level, int option, void *optval, socklen_t optvallen);

typedef struct {
  co_socket_t proto;
  char *path;
} unix_socket_t;

int unix_socket_init(void *self);

int unix_socket_bind(void *self, const char *endpoint);

int unix_socket_connect(void *self, const char *endpoint);


#endif
