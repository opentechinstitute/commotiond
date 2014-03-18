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

#include <limits.h>
#include <stdlib.h>
#include <assert.h>

#include "config.h"
#include <serval.h>
#include <serval/conf.h>
#include <serval/net.h>
#include <serval/socket.h>

#include "socket.h"
#include "strbuf.h"
#include "strbuf_helpers.h"
#include "str.h"
#include "mdp_client.h"

static char *thisinstancepath = NULL;

static const char *__serval_instancepath()
{
  if (thisinstancepath)
    return thisinstancepath;
  const char *instancepath = getenv("SERVALINSTANCE_PATH");
  if (!instancepath)
    instancepath = DEFAULT_INSTANCE_PATH;
  return instancepath;
}

static int __vformf_serval_instance_path(char *buf, size_t bufsiz, const char *fmt, va_list ap)
{
  strbuf b = __strbuf_local(buf, bufsiz);
  __strbuf_va_vprintf(b, fmt, ap);
  if (!__strbuf_overrun(b) && __strbuf_len(b) && buf[0] != '/') {
    __strbuf_reset(b);
    __strbuf_puts(b, __serval_instancepath());
    __strbuf_putc(b, '/');
    __strbuf_va_vprintf(b, fmt, ap);
  }
  if (!__strbuf_overrun(b))
    return 1;
  WHYF("instance path overflow (strlen %lu, sizeof buffer %lu): %s",
       (unsigned long)__strbuf_count(b),
       (unsigned long)bufsiz,
       __alloca_str_toprint(buf));
  return 0;
}

int __make_local_sockaddr(struct socket_address *addr, const char *fmt, ...) {
  bzero(addr, sizeof(*addr));
  addr->local.sun_family = AF_UNIX;
  va_list ap;
  va_start(ap, fmt);
  int r = __vformf_serval_instance_path(addr->local.sun_path, sizeof addr->local.sun_path, fmt, ap);
  va_end(ap);
  if (!r)
    return WHY("socket name overflow");
  addr->addrlen=sizeof addr->local.sun_family + strlen(addr->local.sun_path) + 1;
// TODO perform real path transformation in making the serval instance path
//  if (real_sockaddr(addr, addr) == -1)
//    return -1;

#ifdef USE_ABSTRACT_NAMESPACE
  // For the abstract name we use the absolute path name with the initial '/' replaced by the
  // leading nul.  This ensures that different instances of the Serval daemon have different socket
  // names.
  addr->local.sun_path[0] = '\0'; // mark as Linux abstract socket
  --addr->addrlen; // do not count trailing nul in abstract socket name
#endif // USE_ABSTRACT_NAMESPACE
  return 0;
}

int __esocket(int domain, int type, int protocol) {
  int fd;
  if ((fd = socket(domain, type, protocol)) == -1)
    return WHYF_perror("socket(%s, %s, 0)", __alloca_socket_domain(domain), __alloca_socket_type(type));
  if (config.debug.io || config.debug.verbose_io)
    DEBUGF("socket(%s, %s, 0) -> %d", __alloca_socket_domain(domain), __alloca_socket_type(type), fd);
  return fd;
}

int __socket_bind(int sock, const struct socket_address *addr) {
  assert(addr->addrlen > sizeof addr->addr.sa_family);
  if (addr->addr.sa_family == AF_UNIX && addr->local.sun_path[0] != '\0') {
    assert(addr->local.sun_path[addr->addrlen - sizeof addr->local.sun_family - 1] == '\0');
    if (unlink(addr->local.sun_path) == -1 && errno != ENOENT)
      WARNF_perror("unlink(%s)", __alloca_str_toprint(addr->local.sun_path));
    if (config.debug.io || config.debug.verbose_io)
      DEBUGF("unlink(%s)", __alloca_str_toprint(addr->local.sun_path));
  }
  if (bind(sock, &addr->addr, addr->addrlen) == -1)
    return WHYF_perror("bind(%d,%s,%lu)", sock, __alloca_socket_address(addr), (unsigned long)addr->addrlen);
  if (config.debug.io || config.debug.verbose_io)
    DEBUGF("bind(%d, %s, %lu)", sock, __alloca_socket_address(addr), (unsigned long)addr->addrlen);
  return 0;
}

int __socket_set_rcvbufsize(int sock, unsigned buffer_size) {
  if (setsockopt(sock, SOL_SOCKET, SO_RCVBUF, &buffer_size, sizeof buffer_size) == -1) {
    WARNF_perror("setsockopt(%d,SOL_SOCKET,SO_RCVBUF,&%u,%u)", sock, buffer_size, (unsigned)sizeof buffer_size);
    return -1;
  }
  if (config.debug.io || config.debug.verbose_io)
    DEBUGF("setsockopt(%d, SOL_SOCKET, SO_RCVBUF, &%u, %u)", sock, buffer_size, (unsigned)sizeof buffer_size);
  return 0;
}

int __socket_unlink_close(int sock) {
  // get the socket name and unlink it from the filesystem if not abstract
  struct socket_address addr;
  addr.addrlen = sizeof addr.store;
  if (getsockname(sock, &addr.addr, &addr.addrlen))
    WHYF_perror("getsockname(%d)", sock);
  else if (addr.addr.sa_family==AF_UNIX 
    && addr.addrlen > sizeof addr.local.sun_family 
    && addr.addrlen <= sizeof addr.local 
    && addr.local.sun_path[0] != '\0') {
    if (unlink(addr.local.sun_path) == -1)
      WARNF_perror("unlink(%s)", __alloca_str_toprint(addr.local.sun_path));
  }
  close(sock);
  return 0;
}

int __real_sockaddr(const struct socket_address *src_addr, struct socket_address *dst_addr)
{
  assert(src_addr->addrlen > sizeof src_addr->local.sun_family);
  size_t src_path_len = src_addr->addrlen - sizeof src_addr->local.sun_family;
  if (	 src_addr->addrlen >= sizeof src_addr->local.sun_family + 1
      && src_addr->local.sun_family == AF_UNIX
      && src_addr->local.sun_path[0] != '\0'
      && src_addr->local.sun_path[src_path_len - 1] == '\0'
  ) {
    char real_path[PATH_MAX];
    size_t real_path_len;
    if (realpath(src_addr->local.sun_path, real_path) == NULL)
      return WHYF_perror("realpath(%s)", __alloca_str_toprint(src_addr->local.sun_path));
    else if ((real_path_len = strlen(real_path) + 1) > sizeof dst_addr->local.sun_path)
      return WHYF("sockaddr overrun: realpath(%s) returned %s", 
	  __alloca_str_toprint(src_addr->local.sun_path), __alloca_str_toprint(real_path));
    else if (   real_path_len != src_path_len
	     || memcmp(real_path, src_addr->local.sun_path, src_path_len) != 0
    ) {
      memcpy(dst_addr->local.sun_path, real_path, real_path_len);
      dst_addr->addrlen = real_path_len + sizeof dst_addr->local.sun_family;
      return 1;
    }
  }
  if (dst_addr != src_addr){
    memcpy(&dst_addr->addr, &src_addr->addr, src_addr->addrlen);
    dst_addr->addrlen = src_addr->addrlen;
  }
  return 0;
}

int __cmp_sockaddr(const struct socket_address *addrA, const struct socket_address *addrB)
{
  // Two zero-length sockaddrs are equal.
  if (addrA->addrlen == 0 && addrB->addrlen == 0)
    return 0;
  // If either sockaddr is truncated, then we compare the bytes we have.
  if (addrA->addrlen < sizeof addrA->addr.sa_family || addrB->addrlen < sizeof addrB->addr.sa_family) {
    int c = memcmp(addrA, addrB, addrA->addrlen < addrB->addrlen ? addrA->addrlen : addrB->addrlen);
    if (c == 0)
      c = addrA->addrlen < addrB->addrlen ? -1 : addrA->addrlen > addrB->addrlen ? 1 : 0;
    return c;
  }
  // Order first by address family.
  if (addrA->addr.sa_family < addrB->addr.sa_family)
    return -1;
  if (addrA->addr.sa_family > addrB->addr.sa_family)
    return 1;
  // Both addresses are in the same family...
  switch (addrA->addr.sa_family) {
  case AF_INET: {
      if (addrA->inet.sin_addr.s_addr < addrB->inet.sin_addr.s_addr)
	return -1;
      if (addrA->inet.sin_addr.s_addr > addrB->inet.sin_addr.s_addr)
	return 1;
      if (addrA->inet.sin_port < addrB->inet.sin_port)
	return -1;
      if (addrA->inet.sin_port > addrB->inet.sin_port)
	return 1;
      return 0;
    }break;
  case AF_UNIX: {
      unsigned pathlenA = addrA->addrlen - sizeof (addrA->local.sun_family);
      unsigned pathlenB = addrB->addrlen - sizeof (addrB->local.sun_family);
      int c;
      if (   pathlenA > 1 && pathlenB > 1
	  && addrA->local.sun_path[0] == '\0'
	  && addrB->local.sun_path[0] == '\0'
      ) {
	// Both abstract sockets - just compare names, nul bytes are not terminators.
	c = memcmp(&addrA->local.sun_path[1],
		   &addrB->local.sun_path[1],
		   (pathlenA < pathlenB ? pathlenA : pathlenB) - 1);
      } else {
	// Either or both are named local file sockets.  If the file names are identical up to the
	// first nul, then the addresses are equal.  This collates abstract socket names, whose first
	// character is a nul, ahead of all non-empty file socket names.
	c = strncmp(addrA->local.sun_path,
		    addrB->local.sun_path,
		    (pathlenA < pathlenB ? pathlenA : pathlenB));
      }
      if (c == 0)
	c = pathlenA < pathlenB ? -1 : pathlenA > pathlenB ? 1 : 0;
      return c;
    }
    break;
  }
  // Fall back to comparing raw data bytes.
  int c = memcmp(addrA->addr.sa_data, addrB->addr.sa_data, 
      (addrA->addrlen < addrB->addrlen ? addrA->addrlen : addrB->addrlen) - sizeof addrA->addr.sa_family);
  if (c == 0)
    c = addrA->addrlen < addrB->addrlen ? -1 : addrA->addrlen > addrB->addrlen ? 1 : 0;
  
  return c;
}

ssize_t __recvwithttl(int sock, unsigned char *buffer, size_t bufferlen, int *ttl, struct socket_address *recvaddr) {
  struct msghdr msg;
  struct iovec iov[1];
  struct cmsghdr cmsgcmsg[16];
  iov[0].iov_base=buffer;
  iov[0].iov_len=bufferlen;
  bzero(&msg,sizeof(msg));
  msg.msg_name = &recvaddr->store;
  msg.msg_namelen = recvaddr->addrlen;
  msg.msg_iov = &iov[0];
  msg.msg_iovlen = 1;
  msg.msg_control = cmsgcmsg;
  msg.msg_controllen = sizeof cmsgcmsg;
  msg.msg_flags = 0;
  
  ssize_t len = recvmsg(sock,&msg,0);
  if (len == -1 && errno != EAGAIN && errno != EWOULDBLOCK)
    return WHYF_perror("recvmsg(%d,%p,0)", sock, &msg);
  
  if (len > 0) {
    struct cmsghdr *cmsg;
    for (cmsg = CMSG_FIRSTHDR(&msg); cmsg != NULL; cmsg = CMSG_NXTHDR(&msg, cmsg)) {
      if (   cmsg->cmsg_level == IPPROTO_IP
	  && ((cmsg->cmsg_type == IP_RECVTTL) || (cmsg->cmsg_type == IP_TTL))
	  && cmsg->cmsg_len
      ) {
	if (config.debug.packetrx)
	  DEBUGF("  TTL (%p) data location resolves to %p", ttl,CMSG_DATA(cmsg));
	if (CMSG_DATA(cmsg)) {
	  *ttl = *(unsigned char *) CMSG_DATA(cmsg);
	  if (config.debug.packetrx)
	    DEBUGF("  TTL of packet is %d", *ttl);
	} 
      } else {
	if (config.debug.packetrx)
	  DEBUGF("I didn't expect to see level=%02x, type=%02x",
		 cmsg->cmsg_level,cmsg->cmsg_type);
      }	 
    }
  }
  recvaddr->addrlen = msg.msg_namelen;
  
  return len;
}