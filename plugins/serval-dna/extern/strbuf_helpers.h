/**
 *       @file  strbuf_helpers.h
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
Serval string buffer helper functions.
Copyright (C) 2012 Serval Project Inc.

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

#ifndef __CO_SERVAL_STRBUF_HELPERS_H
#define __CO_SERVAL_STRBUF_HELPERS_H

#include "config.h"

// For socklen_t
#ifdef WIN32
#  include "win32/win32.h"
#else
#  ifdef HAVE_SYS_SOCKET_H
#    include <sys/socket.h>
#  endif
#endif

#include "strbuf.h"

// #define alloca_tohex_sid_t(sid)         alloca_tohex((sid).binary, sizeof (*(sid_t*)0).binary)

struct socket_address;
strbuf __strbuf_append_socket_address(strbuf sb, const struct socket_address *addr);
#define __alloca_socket_address(addr)    __strbuf_str(__strbuf_append_socket_address(strbuf_alloca(200), (addr)))

struct in_addr;
strbuf __strbuf_append_in_addr(strbuf sb, const struct in_addr *addr);
#define __alloca_in_addr(addr)    __strbuf_str(__strbuf_append_in_addr(strbuf_alloca(16), (const struct in_addr *)(addr)))

struct sockaddr_in;
strbuf __strbuf_append_sockaddr_in(strbuf sb, const struct sockaddr_in *addr);
#define __alloca_sockaddr_in(addr)    __strbuf_str(__strbuf_append_sockaddr_in(strbuf_alloca(45), (const struct sockaddr_in *)(addr)))

strbuf __strbuf_append_socket_domain(strbuf sb, int domain);
#define __alloca_socket_domain(domain)    __strbuf_str(__strbuf_append_socket_domain(strbuf_alloca(15), domain))

strbuf __strbuf_append_socket_type(strbuf sb, int type);
#define __alloca_socket_type(type)    __strbuf_str(__strbuf_append_socket_type(strbuf_alloca(15), type))

strbuf __strbuf_toprint_quoted(strbuf sb, const char quotes[2], const char *str);

#endif