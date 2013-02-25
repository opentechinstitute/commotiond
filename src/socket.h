/*! 
 *
 * \file socket.h 
 *
 * \brief a simple object-oriented socket wrapper
 *
 *        object model inspired by Zed Shaw
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
} socket_t;

socket_t *socket_create(size_t size, socket_t proto);

int socket_init(void *self);

int socket_destroy(void *self);

int socket_hangup(void *self, void *context); 

int socket_send(void *self, char *outgoing, size_t length);

int socket_receive(void * self, char *incoming, size_t length);

int socket_setopt(void * self, int level, int option, void *optval, socklen_t optvallen);

int socket_getopt(void * self, int level, int option, void *optval, socklen_t optvallen);

typedef struct {
  socket_t proto;
  char *path;
} unix_socket_t;

int unix_socket_init(void *self);

int unix_socket_bind(void *self, const char *endpoint);

int unix_socket_connect(void *self, const char *endpoint);


#endif
