/*
Serval Distributed Numbering Architecture (DNA)
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

#include <unistd.h>
#include <errno.h>
#include <fcntl.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <time.h>

#include <serval.h>
#include <serval/str.h>

#include "net.h"
#include "debug.h"

int __set_nonblock(int fd)
{
  int flags;
  CHECK((flags = fcntl(fd, F_GETFL, NULL)) != -1,"set_nonblock: fcntl(%d,F_GETFL,NULL)", fd);
  CHECK(fcntl(fd, F_SETFL, flags | O_NONBLOCK) != -1,"set_nonblock: fcntl(%d,F_SETFL,0x%x|O_NONBLOCK)", fd, flags);
  return 0;
error:
  return -1;
}

int __set_block(int fd)
{
  int flags;
  CHECK((flags = fcntl(fd, F_GETFL, NULL)) != -1,"set_block: fcntl(%d,F_GETFL,NULL)", fd);
  CHECK(fcntl(fd, F_SETFL, flags & ~O_NONBLOCK) != -1,"set_block: fcntl(%d,F_SETFL,0x%x&~O_NONBLOCK)", fd, flags);
  return 0;
error:
  return -1;
}


ssize_t __recvwithttl(int sock,unsigned char *buffer, size_t bufferlen,int *ttl,
		    struct sockaddr *recvaddr, socklen_t *recvaddrlen)
{
  struct msghdr msg;
  struct iovec iov[1];
  
  iov[0].iov_base=buffer;
  iov[0].iov_len=bufferlen;
  bzero(&msg,sizeof(msg));
  msg.msg_name = recvaddr;
  msg.msg_namelen = *recvaddrlen;
  msg.msg_iov = &iov[0];
  msg.msg_iovlen = 1;
  // setting the following makes the data end up in the wrong place
  //  msg.msg_iov->iov_base=iov_buffer;
  // msg.msg_iov->iov_len=sizeof(iov_buffer);
  
  struct cmsghdr cmsgcmsg[16];
  msg.msg_control = &cmsgcmsg[0];
  msg.msg_controllen = sizeof(struct cmsghdr)*16;
  msg.msg_flags = 0;
  
  ssize_t len = recvmsg(sock,&msg,0);
  if (len == -1 && errno != EAGAIN && errno != EWOULDBLOCK)
    return -1;
  
//   if (0 && config.debug.packetrx) {
//     DEBUGF("recvmsg returned %lld (flags=%d, msg_controllen=%d)", (long long) len, msg.msg_flags, msg.msg_controllen);
//     dump("received data", buffer, len);
//   }
  
  struct cmsghdr *cmsg;
  if (len>0)
  {
    for (cmsg = CMSG_FIRSTHDR(&msg); 
	 cmsg != NULL; 
	 cmsg = CMSG_NXTHDR(&msg,cmsg)) {
      
      if ((cmsg->cmsg_level == IPPROTO_IP) && 
	  ((cmsg->cmsg_type == IP_RECVTTL) ||(cmsg->cmsg_type == IP_TTL))
	  &&(cmsg->cmsg_len) ){
// 	if (config.debug.packetrx)
// 	  DEBUGF("  TTL (%p) data location resolves to %p", ttl,CMSG_DATA(cmsg));
	if (CMSG_DATA(cmsg)) {
	  *ttl = *(unsigned char *) CMSG_DATA(cmsg);
// 	  if (config.debug.packetrx)
// 	    DEBUGF("  TTL of packet is %d", *ttl);
	} 
//       } else {
// 	if (config.debug.packetrx)
// 	  DEBUGF("I didn't expect to see level=%02x, type=%02x",
// 		 cmsg->cmsg_level,cmsg->cmsg_type);
      }	 
    }
  }
  *recvaddrlen=msg.msg_namelen;
  
  return len;
}
