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

#include "config.h"

#include <ctype.h>
#include <string.h>
#include <time.h>
#include <assert.h>
#include <stdarg.h>
#include <inttypes.h>
#include <sys/wait.h>
#ifdef HAVE_POLL_H
#include <poll.h>
#endif
#ifdef HAVE_NETINET_IN_H
#include <netinet/in.h>
#endif
#ifdef HAVE_ARPA_INET_H
#include <arpa/inet.h>
#endif
#include <sys/uio.h>

#include "socket.h"
#include "strbuf_helpers.h"

static inline strbuf __toprint(strbuf sb, char c)
{
  if (c == '\0')
    __strbuf_puts(sb, "\\0");
  else if (c == '\n')
    __strbuf_puts(sb, "\\n");
  else if (c == '\r')
    __strbuf_puts(sb, "\\r");
  else if (c == '\t')
    __strbuf_puts(sb, "\\t");
  else if (c == '\\')
    __strbuf_puts(sb, "\\\\");
  else if (c >= ' ' && c <= '~')
    __strbuf_putc(sb, c);
  else
    __strbuf_sprintf(sb, "\\x%02x", (unsigned char) c);
  return sb;
}

inline static strbuf __overrun_quote(strbuf sb, char quote, const char *suffix)
{
  if (__strbuf_overrun(sb)) {
    __strbuf_trunc(sb, -strlen(suffix) - (quote ? 1 : 0));
    if (quote)
      __strbuf_putc(sb, quote);
    __strbuf_puts(sb, suffix);
  }
  return sb;
}

static strbuf __strbuf_toprint_quoted_len(strbuf sb, const char quotes[2], const char *buf, size_t len)
{
  if (quotes && quotes[0])
    __strbuf_putc(sb, quotes[0]);
  for (; len && !__strbuf_overrun(sb); ++buf, --len)
    if (quotes && *buf == quotes[1]) {
      __strbuf_putc(sb, '\\');
      __strbuf_putc(sb, *buf);
    } else
      __toprint(sb, *buf);
    if (quotes && quotes[1])
      __strbuf_putc(sb, quotes[1]);
    return __overrun_quote(sb, quotes ? quotes[1] : '\0', "...");
}

static strbuf __strbuf_append_sockaddr(strbuf sb, const struct sockaddr *addr, socklen_t addrlen)
{
  switch (addr->sa_family) {
  case AF_UNIX: {
      __strbuf_puts(sb, "AF_UNIX:");
      size_t len = addrlen > sizeof addr->sa_family ? addrlen - sizeof addr->sa_family : 0;
      if (addr->sa_data[0]) {
	__strbuf_toprint_quoted_len(sb, "\"\"", addr->sa_data, len);
	if (len < 2)
	  __strbuf_sprintf(sb, " (addrlen=%d too short)", (int)addrlen);
	if (len == 0 || addr->sa_data[len - 1] != '\0')
	  __strbuf_sprintf(sb, " (addrlen=%d, no nul terminator)", (int)addrlen);
      } else {
	__strbuf_puts(sb, "abstract ");
	__strbuf_toprint_quoted_len(sb, "\"\"", addr->sa_data, len);
	if (len == 0)
	  __strbuf_sprintf(sb, " (addrlen=%d too short)", (int)addrlen);
      }
    }
    break;
  case AF_INET: {
      const struct sockaddr_in *addr_in = (const struct sockaddr_in *) addr;
      __strbuf_append_sockaddr_in(sb, addr_in);
      if (addrlen != sizeof(struct sockaddr_in))
	__strbuf_sprintf(sb, " (addrlen=%d should be %zd)", (int)addrlen, sizeof(struct sockaddr_in));
    }
    break;
  default: {
      __strbuf_append_socket_domain(sb, addr->sa_family);
      size_t len = addrlen > sizeof addr->sa_family ? addrlen - sizeof addr->sa_family : 0;
      unsigned i;
      for (i = 0; i < len; ++i) {
	__strbuf_putc(sb, i ? ',' : ':');
	__strbuf_sprintf(sb, "%02x", addr->sa_data[i]);
      }
    }
    break;
  }
  return sb;
}

strbuf __strbuf_append_socket_address(strbuf sb, const struct socket_address *addr)
{
  return __strbuf_append_sockaddr(sb, &addr->addr, addr->addrlen);
}

strbuf __strbuf_append_in_addr(strbuf sb, const struct in_addr *addr)
{
  __strbuf_sprintf(sb, "%u.%u.%u.%u",
		 ((unsigned char *) &addr->s_addr)[0],
		 ((unsigned char *) &addr->s_addr)[1],
		 ((unsigned char *) &addr->s_addr)[2],
		 ((unsigned char *) &addr->s_addr)[3]);
  return sb;
}

strbuf __strbuf_append_sockaddr_in(strbuf sb, const struct sockaddr_in *addr)
{
  assert(addr->sin_family == AF_INET);
  __strbuf_puts(sb, "AF_INET:");
  __strbuf_append_in_addr(sb, &addr->sin_addr);
  __strbuf_sprintf(sb, ":%u", ntohs(addr->sin_port));
  return sb;
}

strbuf __strbuf_append_socket_domain(strbuf sb, int domain)
{
  const char *fam = NULL;
  switch (domain) {
  case AF_UNSPEC:    fam = "AF_UNSPEC"; break;
  case AF_UNIX:	     fam = "AF_UNIX"; break;
  case AF_INET:	     fam = "AF_INET"; break;
#if 0
  // These values are not used in Serval, yet.
  case AF_BLUETOOTH: fam = "AF_BLUETOOTH"; break;
  case AF_INET6:     fam = "AF_INET6"; break;
  // These values will probably never be used in Serval.
  case AF_AX25:	     fam = "AF_AX25"; break;
  case AF_IPX:	     fam = "AF_IPX"; break;
  case AF_APPLETALK: fam = "AF_APPLETALK"; break;
  case AF_NETROM:    fam = "AF_NETROM"; break;
  case AF_BRIDGE:    fam = "AF_BRIDGE"; break;
  case AF_ATMPVC:    fam = "AF_ATMPVC"; break;
  case AF_X25:	     fam = "AF_X25"; break;
  case AF_ROSE:	     fam = "AF_ROSE"; break;
  case AF_DECnet:    fam = "AF_DECnet"; break;
  case AF_NETBEUI:   fam = "AF_NETBEUI"; break;
  case AF_SECURITY:  fam = "AF_SECURITY"; break;
  case AF_KEY:	     fam = "AF_KEY"; break;
  case AF_NETLINK:   fam = "AF_NETLINK"; break;
  case AF_PACKET:    fam = "AF_PACKET"; break;
  case AF_ASH:	     fam = "AF_ASH"; break;
  case AF_ECONET:    fam = "AF_ECONET"; break;
  case AF_ATMSVC:    fam = "AF_ATMSVC"; break;
  case AF_SNA:	     fam = "AF_SNA"; break;
  case AF_IRDA:	     fam = "AF_IRDA"; break;
  case AF_PPPOX:     fam = "AF_PPPOX"; break;
  case AF_WANPIPE:   fam = "AF_WANPIPE"; break;
  case AF_LLC:	     fam = "AF_LLC"; break;
  case AF_TIPC:	     fam = "AF_TIPC"; break;
#endif
  }
  if (fam)
    __strbuf_puts(sb, fam);
  else
    __strbuf_sprintf(sb, "[%d]", domain);
  return sb;
}

strbuf __strbuf_append_socket_type(strbuf sb, int type)
{
  const char *typ = NULL;
  switch (type) {
  case SOCK_STREAM:	typ = "SOCK_STREAM"; break;
  case SOCK_DGRAM:	typ = "SOCK_DGRAM"; break;
#ifdef SOCK_RAW
  case SOCK_RAW:	typ = "SOCK_RAW"; break;
#endif
#ifdef SOCK_RDM
  case SOCK_RDM:	typ = "SOCK_RDM"; break;
#endif
#ifdef SOCK_SEQPACKET
  case SOCK_SEQPACKET:	typ = "SOCK_SEQPACKET"; break;
#endif
#ifdef SOCK_PACKET
  case SOCK_PACKET:	typ = "SOCK_PACKET"; break;
#endif
  }
  if (typ)
    __strbuf_puts(sb, typ);
  else
    __strbuf_sprintf(sb, "[%d]", type);
  return sb;
}

strbuf __strbuf_toprint_quoted(strbuf sb, const char quotes[2], const char *str)
{
  if (quotes && quotes[0])
    __strbuf_putc(sb, quotes[0]);
  for (; *str && !__strbuf_overrun(sb); ++str)
    if (quotes && *str == quotes[1]) {
      __strbuf_putc(sb, '\\');
      __strbuf_putc(sb, *str);
    } else
      __toprint(sb, *str);
    if (quotes && quotes[1])
      __strbuf_putc(sb, quotes[1]);
    return __overrun_quote(sb, quotes ? quotes[1] : '\0', "...");
}