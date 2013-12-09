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
#include "obj.h"

#define MAX_IPPROTO 255
#define MAX_CONNECTIONS 32

/**
 * @struct co_socket_t contains file path and state information for socket
 */
typedef struct {
  co_obj_t _header;
  uint8_t _exttype;
  uint8_t _len;
  char *uri;
  int fd; //socket file descriptor
  int rfd; //accept socket file descriptors
  bool fd_registered;
  bool rfd_registered;
  struct sockaddr* local;
  struct sockaddr* remote;
  bool listen;
  int (*init)(co_obj_t *self);
  int (*destroy)(co_obj_t *self);
  int (*hangup)(co_obj_t *self, co_obj_t *context);
  int (*bind)(co_obj_t *self, const char *endpoint);
  int (*connect)(co_obj_t *self, const char *endpoint);
  int (*send)(co_obj_t *self, char *outgoing, size_t length);
  int (*receive)(co_obj_t *self, char *incoming, size_t length);
  int (*setopt)(co_obj_t *self, int level, int option, void *optval, socklen_t optvallen);
  int (*getopt)(co_obj_t *self, int level, int option, void *optval, socklen_t optvallen);
  int (*poll_cb)(co_obj_t *self, co_obj_t *context);
  int (*register_cb)(co_obj_t *self, co_obj_t *context);
} co_socket_t;

/**
 * @brief creates a socket from specified values or initializes defaults
 * @param size size of socket struct
 * @param proto socket protocol
 */
co_obj_t *co_socket_create(size_t size, co_socket_t proto);

/**
 * @brief creates a socket from specified values or initializes defaults
 * @param size size of socket struct
 * @param proto socket protocol
 */
int co_socket_init(co_obj_t *self);

/**
 * @brief closes a socket and removes it from memory
 * @param self socket name
 */
int co_socket_destroy(co_obj_t *self);

/**
 * @brief closes a socket and changes its state information
 * @param self socket name
 * @param context co_obj_t context pointer (currently unused)
 */
int co_socket_hangup(co_obj_t *self, co_obj_t *context); 

/**
 * @brief sends a message on a specified socket
 * @param self socket name
 * @param outgoing message to be sent
 * @param length length of message
 */
int co_socket_send(co_obj_t *self, char *outgoing, size_t length);

/**
 * @brief receives a message on the listening socket
 * @param self socket name
 * @param incoming message received
 * @param length length of message
 */
int co_socket_receive(co_obj_t * self, char *incoming, size_t length);

/**
 * @brief sets custom socket options, if specified by user
 * @param self socket name
 * @param level the networking level to be customized
 * @param option the option to be changed
 * @param optval the value for the new option
 * @param optvallen the length of the value for the new option
 */
int co_socket_setopt(co_obj_t * self, int level, int option, void *optval, socklen_t optvallen);

/**
 * @brief gets custom socket options specified from the user
 * @param self socket name
 * @param level the networking level to be customized
 * @param option the option to be changed
 * @param optval the value for the new option
 * @param optvallen the length of the value for the new option
 */
int co_socket_getopt(co_obj_t * self, int level, int option, void *optval, socklen_t optvallen);

/**
 * @struct unix_socket_t struct for unix sockets. Contains protocol and file path to socket library
 */
typedef struct {
  co_socket_t proto;
  char *path;
} unix_socket_t;

/**
 * @brief initializes a unix socket
 * @param self socket name
 */
int unix_socket_init(co_obj_t *self);

/**
 * @brief binds a unix socket to a specified endpoint
 * @param self socket name
 * @param endpoint specified endpoint for socket (file path)
 */
int unix_socket_bind(co_obj_t *self, const char *endpoint);

/**
 * @brief connects a socket to specified endpoint
 * @param self socket name
 * @param endpoint specified endpoint for socket (file path)
 */
int unix_socket_connect(co_obj_t *self, const char *endpoint);


#endif
