/**
 *       @file  mdp_client.c
 *      @brief  minimal Serval MDP client functionality, used in the
 *                commotion_serval-sas library
 *
 *     @author  Dan Staples (dismantl), danstaples@opentechinstitute.org
 *
 *   @internal
 *     Created  12/18/2013
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
 Copyright (C) 2010-2012 Paul Gardner-Stephen, Serval Project.
 
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

#include <sys/stat.h>
#include <sys/time.h>
#include <inttypes.h>

#include "config.h"
#include <serval.h>
#include <serval/conf.h>
#include <serval/overlay_buffer.h>
#include <serval/overlay_address.h>
#include <serval/overlay_packet.h>
#include <serval/mdp_client.h>

#include "net.h"
#include "socket.h"
#include "strbuf_helpers.h"
#include "debug.h"
#include "mdp_client.h"

#undef FORM_SERVAL_INSTANCE_PATH
#define FORM_SERVAL_INSTANCE_PATH(A,B) ({ strcpy(A,"/var/serval-node/"); strcat(A,B); A; })

time_ms_t __gettime_ms()
{
  struct timeval nowtv;
  // If gettimeofday() fails or returns an invalid value, all else is lost!
    if (gettimeofday(&nowtv, NULL) == -1)
      FATAL_perror("gettimeofday");
    if (nowtv.tv_sec < 0 || nowtv.tv_usec < 0 || nowtv.tv_usec >= 1000000)
      FATALF("gettimeofday returned tv_sec=%ld tv_usec=%ld", nowtv.tv_sec, nowtv.tv_usec);
  return nowtv.tv_sec * 1000LL + nowtv.tv_usec / 1000;
}

static int __urandombytes(unsigned char *buf, unsigned long long len)
{
  static int urandomfd = -1;
  int tries = 0;
  if (urandomfd == -1) {
    for (tries = 0; tries < 4; ++tries) {
      urandomfd = open("/dev/urandom",O_RDONLY);
      if (urandomfd != -1) break;
      sleep(1);
    }
    if (urandomfd == -1) {
      WHY_perror("open(/dev/urandom)");
      return -1;
    }
  }
  tries = 0;
  while (len > 0) {
    int i = (len < 1048576) ? len : 1048576;
    i = read(urandomfd, buf, i);
    if (i == -1) {
      if (++tries > 4) {
	WHY_perror("read(/dev/urandom)");
	if (errno==EBADF) urandomfd=-1;
	return -1;
      }
    } else {
      tries = 0;
      buf += i;
      len -= i;
    }
  }
  return 0;
}

/** Create a new MDP socket and return its descriptor (-1 on error). */
int __overlay_mdp_client_socket(void)
{
  /* Create local per-client socket to MDP server (connection is always local) */
  int mdp_sockfd;
  struct socket_address addr;
  uint32_t random_value;
  if (__urandombytes((unsigned char *)&random_value, sizeof random_value) == -1)
    return WHY("urandombytes() failed");
  if (__make_local_sockaddr(&addr, "mdp.client.%u.%08lx.socket", getpid(), (unsigned long)random_value) == -1)
    return -1;
  if ((mdp_sockfd = __esocket(AF_UNIX, SOCK_DGRAM, 0)) == -1)
    return -1;
  if (__socket_bind(mdp_sockfd, &addr) == -1) {
    close(mdp_sockfd);
    return -1;
  }
  __socket_set_rcvbufsize(mdp_sockfd, 128 * 1024);
  return mdp_sockfd;
}

int __overlay_mdp_client_close(int mdp_sockfd)
{
  /* Tell MDP server to release all our bindings */
  overlay_mdp_frame mdp;
  mdp.packetTypeAndFlags = MDP_GOODBYE;
  __overlay_mdp_send(mdp_sockfd, &mdp, 0, 0);
  
  __socket_unlink_close(mdp_sockfd);
  return 0;
}

int __overlay_mdp_send(int mdp_sockfd, overlay_mdp_frame *mdp, int flags, int timeout_ms)
{
  if (mdp_sockfd == -1)
    return -1;
  // Minimise frame length to save work and prevent accidental disclosure of memory contents.
  ssize_t len = __overlay_mdp_relevant_bytes(mdp);
  if (len == -1)
    return WHY("MDP frame invalid (could not compute length)");
  /* Construct name of socket to send to. */
  struct socket_address addr;
  if (__make_local_sockaddr(&addr, "mdp.socket") == -1)
    return -1;
  // Send to that socket
  __set_nonblock(mdp_sockfd);
  ssize_t result = sendto(mdp_sockfd, mdp, (size_t)len, 0, &addr.addr, addr.addrlen);
  __set_block(mdp_sockfd);
  if ((size_t)result != (size_t)len) {
    if (result == -1)
      WHYF_perror("sendto(fd=%d,len=%zu,addr=%s)", mdp_sockfd, (size_t)len, __alloca_socket_address(&addr));
    else
      WHYF("sendto() sent %zu bytes of MDP reply (%zu) to %s", (size_t)result, (size_t)len, __alloca_socket_address(&addr)); 
    mdp->packetTypeAndFlags=MDP_ERROR;
    mdp->error.error=1;
    snprintf(mdp->error.message,128,"Error sending frame to MDP server.");
    return -1;
  } else {
    if (!(flags&MDP_AWAITREPLY)) {       
      return 0;
    }
  }
  
  mdp_port_t port=0;
  if ((mdp->packetTypeAndFlags&MDP_TYPE_MASK) == MDP_TX)
    port = mdp->out.src.port;
  
  time_ms_t started = __gettime_ms();
  while(timeout_ms>=0 && __overlay_mdp_client_poll(mdp_sockfd, timeout_ms)>0){
    int ttl=-1;
    if (!__overlay_mdp_recv(mdp_sockfd, mdp, port, &ttl)) {
      /* If all is well, examine result and return error code provided */
      if ((mdp->packetTypeAndFlags&MDP_TYPE_MASK)==MDP_ERROR)
	return mdp->error.error;
      else
	/* Something other than an error has been returned */
	return 0;
    }
    
    // work out how much longer we can wait for a valid response
    time_ms_t now = __gettime_ms();
    timeout_ms -= (now - started);
  }
  
  /* Timeout */
  mdp->packetTypeAndFlags=MDP_ERROR;
  mdp->error.error=1;
  snprintf(mdp->error.message,128,"Timeout waiting for reply to MDP packet (packet was successfully sent).");    
  return -1; /* WHY("Timeout waiting for server response"); */
}

int __overlay_mdp_client_poll(int mdp_sockfd, time_ms_t timeout_ms)
{
  fd_set r;
  FD_ZERO(&r);
  FD_SET(mdp_sockfd, &r);
  if (timeout_ms<0) timeout_ms=0;
  
  struct pollfd fds[]={
    {
      .fd = mdp_sockfd,
      .events = POLLIN|POLLERR,
    }
  };
  return poll(fds, 1, timeout_ms);
}

int __overlay_mdp_recv(int mdp_sockfd, overlay_mdp_frame *mdp, mdp_port_t port, int *ttl)
{
  /* Construct name of socket to receive from. */
  struct socket_address mdp_addr;
  if (__make_local_sockaddr(&mdp_addr, "mdp.socket") == -1)
    return -1;
  
  /* Check if reply available */
  struct socket_address recvaddr;
  recvaddr.addrlen = sizeof recvaddr.store;
  ssize_t len;
  mdp->packetTypeAndFlags = 0;
  __set_nonblock(mdp_sockfd);
  len = __recvwithttl(mdp_sockfd, (unsigned char *)mdp, sizeof(overlay_mdp_frame), ttl, &recvaddr);
  if (len == -1)
    WHYF_perror("recvwithttl(%d,%p,%zu,&%d,%p(%s))",
	  mdp_sockfd, mdp, sizeof(overlay_mdp_frame), *ttl,
	  &recvaddr, __alloca_socket_address(&recvaddr)
	);
  __set_block(mdp_sockfd);
  if (len <= 0)
    return -1; // no packet received

  // If the received address overflowed the buffer, then it cannot have come from the server, whose
  // address must always fit within a struct sockaddr_un.
  if (recvaddr.addrlen > sizeof recvaddr.store)
    return WHY("reply did not come from server: address overrun");

  // Compare the address of the sender with the address of our server, to ensure they are the same.
  // If the comparison fails, then try using realpath(3) on the sender address and compare again.
  if (	__cmp_sockaddr(&recvaddr, &mdp_addr) != 0
      && (   recvaddr.local.sun_family != AF_UNIX
	  || __real_sockaddr(&recvaddr, &recvaddr) <= 0
	  || __cmp_sockaddr(&recvaddr, &mdp_addr) != 0
	 )
  )
    return WHYF("reply did not come from server: %s", __alloca_socket_address(&recvaddr));
  
  // silently drop incoming packets for the wrong port number
  if (port>0 && port != mdp->out.dst.port){
    WARNF("Ignoring packet for port %"PRImdp_port_t,mdp->out.dst.port);
    return -1;
  }

  ssize_t expected_len = __overlay_mdp_relevant_bytes(mdp);
  if (expected_len < 0)
    return WHY("unsupported MDP packet type");
  if ((size_t)len < (size_t)expected_len)
    return WHYF("Expected packet length of %zu, received only %zd bytes", (size_t) expected_len, (size_t) len);
  
  /* Valid packet received */
  return 0;
}

// send a request to servald deamon to add a port binding
int __overlay_mdp_bind(int mdp_sockfd, const sid_t *localaddr, mdp_port_t port) 
{
  overlay_mdp_frame mdp;
  mdp.packetTypeAndFlags=MDP_BIND|MDP_FORCE;
  mdp.bind.sid = *localaddr;
  mdp.bind.port=port;
  int result=__overlay_mdp_send(mdp_sockfd, &mdp,MDP_AWAITREPLY,5000);
  if (result) {
    if (mdp.packetTypeAndFlags==MDP_ERROR)
      WHYF("Could not bind to MDP port %"PRImdp_port_t": error=%d, message='%s'",
	   port,mdp.error.error,mdp.error.message);
      else
	WHYF("Could not bind to MDP port %"PRImdp_port_t" (no reason given)",port);
      return -1;
  }
  return 0;
}

int __overlay_mdp_getmyaddr(int mdp_sockfd, unsigned index, sid_t *sidp)
{
  overlay_mdp_frame a;
  memset(&a, 0, sizeof(a));
  
  a.packetTypeAndFlags=MDP_GETADDRS;
  a.addrlist.mode = MDP_ADDRLIST_MODE_SELF;
  a.addrlist.first_sid=index;
  a.addrlist.last_sid=OVERLAY_MDP_ADDRLIST_MAX_SID_COUNT;
  a.addrlist.frame_sid_count=MDP_MAX_SID_REQUEST;
  int result=__overlay_mdp_send(mdp_sockfd,&a,MDP_AWAITREPLY,5000);
  if (result) {
    if (a.packetTypeAndFlags == MDP_ERROR)
      DEBUGF("MDP Server error #%d: '%s'", a.error.error, a.error.message);
    return WHY("Failed to get local address list");
  }
  if ((a.packetTypeAndFlags&MDP_TYPE_MASK)!=MDP_ADDRLIST)
    return WHY("MDP Server returned something other than an address list");
  if (0) DEBUGF("local addr 0 = %s",alloca_tohex_sid_t(a.addrlist.sids[0]));
  *sidp = a.addrlist.sids[0];
  return 0;
}

ssize_t __overlay_mdp_relevant_bytes(overlay_mdp_frame *mdp) 
{
  size_t len;
  switch(mdp->packetTypeAndFlags&MDP_TYPE_MASK)
  {
    case MDP_ROUTING_TABLE:
    case MDP_GOODBYE:
      /* no arguments for saying goodbye */
      len=&mdp->raw[0]-(char *)mdp;
      break;
    case MDP_ADDRLIST: 
      len = mdp->addrlist.sids[mdp->addrlist.frame_sid_count].binary - (unsigned char *)mdp;
      break;
    case MDP_GETADDRS: 
      len = mdp->addrlist.sids[0].binary - (unsigned char *)mdp;
      break;
    case MDP_TX: 
      len=(&mdp->out.payload[0]-(unsigned char *)mdp) + mdp->out.payload_length; 
      break;
    case MDP_BIND:
      // make sure that the compiler has actually given these two structures the same address
      // I've seen gcc 4.8.1 on x64 fail to give elements the same address once
      assert((void *)mdp->raw == (void *)&mdp->bind);
      len=(&mdp->raw[0] - (char *)mdp) + sizeof(struct mdp_sockaddr);
      break;
    case MDP_SCAN:
      len=(&mdp->raw[0] - (char *)mdp) + sizeof(struct overlay_mdp_scan);
      break;
    case MDP_ERROR: 
      /* This formulation is used so that we don't copy any bytes after the
       *      end of the string, to avoid information leaks */
      len=(&mdp->error.message[0]-(char *)mdp) + strlen(mdp->error.message)+1;      
      if (mdp->error.error) INFOF("mdp return/error code: %d:%s",mdp->error.error,mdp->error.message);
      break;
    default:
      return WHY("Illegal MDP frame type.");
  }
  return len;
}
