/**
 *       @file  ubus.h
 *      @brief  
 *
 * Detailed description starts here.
 *
 *     @author  Josh King (jheretic), jking@chambana.net
 *
 *   @internal
 *     Created  02/25/2013
 *    Revision  $Id: doxygen.templates,v 1.3 2010/07/06 09:20:12 mehner Exp $
 *    Compiler  gcc/g++
 *     Company  The Open Technology Institute
 *   Copyright  Copyright (c) 2013, Josh King
 *
 * This source code is released for free distribution under the terms of the
 * GNU General Public License as published by the Free Software Foundation.
 * =====================================================================================
 */

#include <stdlib.h>
#include <libubus.h>
#include "socket.h"

typedef struct {
  socket_t proto;
  struct ubus_context *ctx;
} ubus_socket_t;

int ubus_socket_init(void *self);

int ubus_socket_bind(void *self, const char *endpoint);
