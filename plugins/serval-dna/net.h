/*
 * Copyright (C) 2012 Serval Project Inc.
 * 
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * as published by the Free Software Foundation; either version 2
 * of the License, or (at your option) any later version.
 * 
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 * 
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA.
 */

#ifndef __CO_SERVAL_NET_H
#define __CO_SERVAL_NET_H

int __set_block(int fd);
int __set_nonblock(int fd);
ssize_t __recvwithttl(int sock,unsigned char *buffer, size_t bufferlen,int *ttl,
		    struct sockaddr *recvaddr, socklen_t *recvaddrlen);


#endif // __NET_H
