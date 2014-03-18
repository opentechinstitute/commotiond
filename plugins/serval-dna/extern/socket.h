/**
 *       @file  socket.h
 *      @brief  minimal Serval MDP client functionality, used in the
 *                commotion_serval-sas library
 *
 *     @author  Dan Staples (dismantl), danstaples@opentechinstitute.org
 *
 *   @internal
 *     Created  3/17/2014
 *    Compiler  gcc/g++
 *     Company  The Open Technology Institute
 *   Copyright  Copyright (c) 2013, Dan Staples
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

/* 
Serval DNA header file for socket operations
Copyright (C) 2012-2013 Serval Project Inc.

This program is free software; you can redistribute it and/or
modify it under the terms of the GNU General Public License
as published by the Free Software Foundation; either version 2
of the License, or (at your option) any later version.

This program is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with this program; if not, write to the Free Software
Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA.
*/

#ifndef __CO_SERVAL_SOCKET_H
#define __CO_SERVAL_SOCKET_H

#include "config.h"
#include <serval/socket.h>

#ifdef WIN32
#   include "win32/win32.h"
#else
#   include <sys/un.h>
#   ifdef HAVE_SYS_SOCKET_H
#     include <sys/socket.h>
#   endif
#   ifdef HAVE_NETINET_IN_H
#     include <netinet/in.h>
#   endif
#endif

int __make_local_sockaddr(struct socket_address *addr, const char *fmt, ...)
    __attribute__((format(printf, 2, 3)));
int __esocket(int domain, int type, int protocol);
int __socket_bind(int sock, const struct socket_address *addr);
int __socket_set_rcvbufsize(int sock, unsigned buffer_size);
int __socket_unlink_close(int sock);

int __real_sockaddr(const struct socket_address *src_addr, struct socket_address *dst_addr);
int __cmp_sockaddr(const struct socket_address *addrA, const struct socket_address *addrB);

ssize_t __recvwithttl(int sock, unsigned char *buffer, size_t bufferlen, int *ttl, struct socket_address *recvaddr);

#endif