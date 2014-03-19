/**
 *       @file  sas_request.c
 *      @brief  library for fetching a Serval SAS key over the MDP
 *                overlay network
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

#include <stdlib.h>
#include <stdio.h>
#include <getopt.h>
#include <unistd.h>
#include <signal.h>
#include <fcntl.h>
#include <string.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/time.h>

#include "config.h"
#include <serval.h>
#include <serval/conf.h>
#include <serval/rhizome.h>
#include <serval/crypto.h>
#include <serval/str.h>
#include <serval/overlay_address.h>
#include <serval/mdp_client.h>
#include <serval/keyring.h>

#include "sas_request.h"
#include "debug.h"

static const char __hexdigit[16] = {'0','1','2','3','4','5','6','7','8','9','A','B','C','D','E','F'};

static char *__tohex(char *dstHex, const unsigned char *srcBinary, size_t bytes)
{
  char *p;
  for (p = dstHex; bytes--; ++srcBinary) {
    *p++ = __hexdigit[*srcBinary >> 4];
    *p++ = __hexdigit[*srcBinary & 0xf];
  }
  *p = '\0';
  return dstHex;
}

inline static int __hexvalue(char c)
{
  if (c >= '0' && c <= '9') return c - '0';
  if (c >= 'A' && c <= 'F') return c - 'A' + 10;
  if (c >= 'a' && c <= 'f') return c - 'a' + 10;
  return -1;
}

static size_t __fromhex(unsigned char *dstBinary, const char *srcHex, size_t nbinary)
{
  size_t count = 0;
  while (count != nbinary) {
    unsigned char high = __hexvalue(*srcHex++);
    if (high & 0xf0) return -1;
    unsigned char low = __hexvalue(*srcHex++);
    if (low & 0xf0) return -1;
    dstBinary[count++] = (high << 4) + low;
  }
  return count;
}

static int __fromhexstr(unsigned char *dstBinary, const char *srcHex, size_t nbinary)
{
  return (__fromhex(dstBinary, srcHex, nbinary) == nbinary && srcHex[nbinary * 2] == '\0') ? 0 : -1;
}

int keyring_send_sas_request_client(const char *sid_str, 
				     const size_t sid_len,
				     char *sas_buf,
				     const size_t sas_buf_len){
  int sent, found = 0, ret = 0, mdp_sockfd = -1;
  int siglen=SID_SIZE+crypto_sign_edwards25519sha512batch_BYTES;
  sid_t srcsid;
  unsigned char dstsid[SID_SIZE] = {0};
  unsigned char signature[siglen];
  time_ms_t now = gettime_ms();
  
  CHECK_MEM(sas_buf);
  CHECK(sas_buf_len >= 2 * SAS_SIZE + 1, "Insufficient SAS buffer");
  
  CHECK(sid_len == 2 * SID_SIZE && __fromhexstr(dstsid, sid_str, SID_SIZE) == 0,
	"Invalid SID");
  
  CHECK((mdp_sockfd = overlay_mdp_client_socket()) >= 0,"Cannot create MDP socket");
  
  CHECK(overlay_mdp_getmyaddr(mdp_sockfd, 0, &srcsid) == 0, "Could not get local address");
  
  int client_port = 32768 + (random() & 32767);
  CHECK(overlay_mdp_bind(mdp_sockfd, &srcsid, client_port) == 0,
	"Failed to bind to client socket");
  
  /* request mapping (send request auth-crypted). */
  overlay_mdp_frame mdp;
  memset(&mdp,0,sizeof(mdp));  
  
  mdp.packetTypeAndFlags=MDP_TX;
  mdp.out.queue=OQ_MESH_MANAGEMENT;
  memmove(mdp.out.dst.sid.binary,dstsid,SID_SIZE);
  mdp.out.dst.port=MDP_PORT_KEYMAPREQUEST;
  mdp.out.src.port=client_port;
  memmove(mdp.out.src.sid.binary,srcsid.binary,SID_SIZE);
  mdp.out.payload_length=1;
  mdp.out.payload[0]=KEYTYPE_CRYPTOSIGN;
  
  sent = overlay_mdp_send(mdp_sockfd, &mdp, 0,0);
  if (sent) {
    DEBUG("Failed to send SAS resolution request: %d", sent);
    CHECK(mdp.packetTypeAndFlags != MDP_ERROR,"MDP Server error #%d: '%s'",mdp.error.error,mdp.error.message);
  }
  
  time_ms_t timeout = now + 5000;
  
  while(now<timeout) {
    time_ms_t timeout_ms = timeout - gettime_ms();
    int result = overlay_mdp_client_poll(mdp_sockfd, timeout_ms);
    
    if (result>0) {
      int ttl=-1;
      if (overlay_mdp_recv(mdp_sockfd, &mdp, client_port, &ttl)==0) {
	switch(mdp.packetTypeAndFlags&MDP_TYPE_MASK) {
	  case MDP_ERROR:
	    ERROR("overlay_mdp_recv: %s (code %d)", mdp.error.message, mdp.error.error);
	    break;
	  case MDP_TX:
	  {
	    DEBUG("Received SAS mapping response");
	    found = 1;
	    break;
	  }
	  break;
	  default:
	    DEBUG("overlay_mdp_recv: Unexpected MDP frame type 0x%x", mdp.packetTypeAndFlags);
	    break;
	}
	if (found) break;
      }
    }
    now=gettime_ms();
  }
  
  unsigned keytype = mdp.out.payload[0];
  
  CHECK(keytype == KEYTYPE_CRYPTOSIGN,"Ignoring SID:SAS mapping with unsupported key type %u", keytype);
  
  CHECK(mdp.out.payload_length >= 1 + SAS_SIZE,"Truncated key mapping announcement? payload_length: %d", mdp.out.payload_length);
  
  unsigned char *sas_public=&mdp.out.payload[1];
  unsigned char *compactsignature = &mdp.out.payload[1+SAS_SIZE];
  
  /* reconstitute signed SID for verification */
  memmove(&signature[0],&compactsignature[0],64);
  memmove(&signature[64],&mdp.out.src.sid.binary[0],SID_SIZE);
  
  CHECK(__tohex(sas_buf,sas_public,SAS_SIZE),"Failed to convert signing key");
  sas_buf[2*SAS_SIZE] = '\0';
  
  ret = 1;
  
error:
  if (mdp_sockfd != -1)
    overlay_mdp_client_close(mdp_sockfd);
  return ret;
}