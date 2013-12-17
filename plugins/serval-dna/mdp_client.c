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

#include <serval.h>
#include <serval/conf.h>
#include <serval/str.h>
#include <serval/strbuf.h>
#include <serval/overlay_buffer.h>
#include <serval/overlay_address.h>
#include <serval/overlay_packet.h>

#include "mdp_client.h"
#include "net.h"
#include "debug.h"

// #include "serval/log.h"

#undef WHY
#define WHY(J) (ERROR("%s",J), -1)

#undef WHY_perror
#define WHY_perror(J) (ERROR("%s",J), -1)

#undef WHYF
#define WHYF(F,...) (ERROR(F, ##__VA_ARGS__), -1)

#undef DEBUGF
#define DEBUGF(F,...) (DEBUG(F, ##__VA_ARGS__), -1)

#undef WARNF
#define WARNF(F,...) (WARN(F, ##__VA_ARGS__), -1)

#undef FATALF
#define FATALF(F,...)       do { ERROR(F, ##__VA_ARGS__); abort(); exit(-1); } while (1)

#undef WARN_perror
#define WARN_perror(X)      WARNF("%s", (X))

#undef INFOF
#define INFOF(F,...)        INFO(F, ##__VA_ARGS__)

#undef FORM_SERVAL_INSTANCE_PATH
#define FORM_SERVAL_INSTANCE_PATH(A,B) ({ strcpy(A,"/var/serval-node/"); strcat(A,B); A; })

int __mdp_client_socket=-1;

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

int __overlay_mdp_send(overlay_mdp_frame *mdp,int flags,int timeout_ms)
{
  int len=4;
  
  if (__mdp_client_socket==-1) 
    if (__overlay_mdp_client_init() != 0)
      return -1;
  
  /* Minimise frame length to save work and prevent accidental disclosure of
   memory contents. */
  len=__overlay_mdp_relevant_bytes(mdp);
  if (len<0) return WHY("MDP frame invalid (could not compute length)");
  
  /* Construct name of socket to send to. */
  struct sockaddr_un name;
  name.sun_family = AF_UNIX;
  if (!FORM_SERVAL_INSTANCE_PATH(name.sun_path, "mdp.socket"))
    return -1;
  DEBUG("%s",name.sun_path);
  
  __set_nonblock(__mdp_client_socket);
  int result=sendto(__mdp_client_socket, mdp, len, 0,
		    (struct sockaddr *)&name, sizeof(struct sockaddr_un));
  __set_block(__mdp_client_socket);
  if (result<0) {
    mdp->packetTypeAndFlags=MDP_ERROR;
    mdp->error.error=1;
    snprintf(mdp->error.message,128,"Error sending frame to MDP server.");
    return WHY_perror("sendto(f)");
  } else {
    if (!(flags&MDP_AWAITREPLY)) {       
      return 0;
    }
  }
  
  int port=0;
  if ((mdp->packetTypeAndFlags&MDP_TYPE_MASK) == MDP_TX)
      port = mdp->out.src.port;
      
  time_ms_t started = __gettime_ms();
  while(timeout_ms>=0 && __overlay_mdp_client_poll(timeout_ms)>0){
    int ttl=-1;
    if (!__overlay_mdp_recv(mdp, port, &ttl)) {
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

char __overlay_mdp_client_socket_path[1024];
int __overlay_mdp_client_socket_path_len=-1;

int __overlay_mdp_client_init()
{
  if (__mdp_client_socket==-1) {
    /* Open socket to MDP server (thus connection is always local) */
    if (0) WHY("Use of abstract name space socket for Linux not implemented");
    
    __mdp_client_socket = socket(AF_UNIX, SOCK_DGRAM, 0);
    if (__mdp_client_socket < 0) {
      WHY_perror("socket");
      return WHY("Could not open socket to MDP server");
    }
    
    /* We must bind to a temporary file name */
    struct sockaddr_un name;
    unsigned int random_value;
    if (__urandombytes((unsigned char *)&random_value,sizeof(int)))
      return WHY("__urandombytes() failed");
    name.sun_family = AF_UNIX;
    if (__overlay_mdp_client_socket_path_len==-1) {
      char fmt[1024];
      if (!FORM_SERVAL_INSTANCE_PATH(fmt, "mdp-client-%d-%08x.socket"))
	return WHY("Could not form MDP client socket name");
      DEBUG("%s",fmt);
      snprintf(__overlay_mdp_client_socket_path,1024,fmt,getpid(),random_value);
      __overlay_mdp_client_socket_path_len=strlen(__overlay_mdp_client_socket_path)+1;
//       if(config.debug.io) DEBUGF("MDP client socket name='%s'",__overlay_mdp_client_socket_path);
    }
    if (__overlay_mdp_client_socket_path_len > sizeof(name.sun_path) - 1)
      FATALF("MDP socket path too long (%d > %ld)", __overlay_mdp_client_socket_path_len, sizeof(name.sun_path) - 1);
    
    bcopy(__overlay_mdp_client_socket_path,name.sun_path,
	  __overlay_mdp_client_socket_path_len);
    unlink(name.sun_path);
    int len = 1 + strlen(name.sun_path) + sizeof(name.sun_family) + 1;
    int r=bind(__mdp_client_socket, (struct sockaddr *)&name, len);
    if (r) {
      WHY_perror("bind");
      return WHY("Could not bind MDP client socket to file name");
    }
    chmod(name.sun_path,S_IRUSR|S_IWUSR|S_IXUSR|S_IRGRP|S_IWGRP|S_IXGRP|S_IROTH|S_IWOTH|S_IXOTH);
    int send_buffer_size=128*1024;
    if (setsockopt(__mdp_client_socket, SOL_SOCKET, SO_RCVBUF, 
		   &send_buffer_size, sizeof(send_buffer_size)) == -1)
      WARN_perror("setsockopt");
  }
  
  return 0;
}

int __overlay_mdp_client_done()
{
  if (__mdp_client_socket!=-1) {
    /* Tell MDP server to release all our bindings */
    overlay_mdp_frame mdp;
    mdp.packetTypeAndFlags=MDP_GOODBYE;
    __overlay_mdp_send(&mdp,0,0);
  }
  
  if (__overlay_mdp_client_socket_path_len>-1)
    unlink(__overlay_mdp_client_socket_path);
  if (__mdp_client_socket!=-1)
    close(__mdp_client_socket);
  __mdp_client_socket=-1;
  return 0;
}

int __overlay_mdp_client_poll(time_ms_t timeout_ms)
{
  fd_set r;
  int ret;
  FD_ZERO(&r);
  FD_SET(__mdp_client_socket,&r);
  if (timeout_ms<0) timeout_ms=0;
  
  struct timeval tv;
  
  if (timeout_ms>=0) {
    tv.tv_sec=timeout_ms/1000;
    tv.tv_usec=(timeout_ms%1000)*1000;
    ret=select(__mdp_client_socket+1,&r,NULL,&r,&tv);
  }
  else
    ret=select(__mdp_client_socket+1,&r,NULL,&r,NULL);
  return ret;
}

int __overlay_mdp_recv(overlay_mdp_frame *mdp, int port, int *ttl) 
{
  char mdp_socket_name[101];
  unsigned char recvaddrbuffer[1024];
  struct sockaddr *recvaddr=(struct sockaddr *)recvaddrbuffer;
  unsigned int recvaddrlen=sizeof(recvaddrbuffer);
  struct sockaddr_un *recvaddr_un;
  
  if (!FORM_SERVAL_INSTANCE_PATH(mdp_socket_name, "mdp.socket"))
    return WHY("Could not find mdp socket");
  mdp->packetTypeAndFlags=0;
  
  /* Check if reply available */
  __set_nonblock(__mdp_client_socket);
  ssize_t len = __recvwithttl(__mdp_client_socket,(unsigned char *)mdp, sizeof(overlay_mdp_frame),ttl,recvaddr,&recvaddrlen);
  __set_block(__mdp_client_socket);
  
  recvaddr_un=(struct sockaddr_un *)recvaddr;
  /* Null terminate received address so that the stat() call below can succeed */
  if (recvaddrlen<1024) recvaddrbuffer[recvaddrlen]=0;
  if (len>0) {
    /* Make sure recvaddr matches who we sent it to */
    if (strncmp(mdp_socket_name, recvaddr_un->sun_path, sizeof(recvaddr_un->sun_path))) {
      /* Okay, reply was PROBABLY not from the server, but on OSX if the path
       has a symlink in it, it is resolved in the reply path, but might not
       be in the request path (mdp_socket_name), thus we need to stat() and
       compare inode numbers etc */
      struct stat sb1,sb2;
      if (stat(mdp_socket_name,&sb1)) return WHY("stat(mdp_socket_name) failed, so could not verify that reply came from MDP server");
      if (stat(recvaddr_un->sun_path,&sb2)) return WHY("stat(ra->sun_path) failed, so could not verify that reply came from MDP server");
      if ((sb1.st_ino!=sb2.st_ino)||(sb1.st_dev!=sb2.st_dev))
	return WHY("Reply did not come from server");
    }
    
    // silently drop incoming packets for the wrong port number
    if (port>0 && port != mdp->in.dst.port){
      WARNF("Ignoring packet for port %d",mdp->in.dst.port);
      return -1;
    }
    
    int expected_len = __overlay_mdp_relevant_bytes(mdp);
    
    if (len < expected_len){
      return WHYF("Expected packet length of %d, received only %lld bytes", expected_len, (long long) len);
    }
    
    /* Valid packet received */
    return 0;
  } else 
  /* no packet received */
    return -1;
  
}

// send a request to servald deamon to add a port binding
int __overlay_mdp_bind(const sid_t *localaddr, int port) 
{
  overlay_mdp_frame mdp;
  mdp.packetTypeAndFlags=MDP_BIND|MDP_FORCE;
  bcopy(localaddr->binary, mdp.bind.sid, SID_SIZE);
  mdp.bind.port=port;
  int result=__overlay_mdp_send(&mdp,MDP_AWAITREPLY,5000);
  if (result) {
    if (mdp.packetTypeAndFlags==MDP_ERROR)
      WHYF("Could not bind to MDP port %d: error=%d, message='%s'",
	   port,mdp.error.error,mdp.error.message);
    else
      WHYF("Could not bind to MDP port %d (no reason given)",port);
    return -1;
  }
  return 0;
}

int __overlay_mdp_getmyaddr(unsigned index, sid_t *sid)
{
  overlay_mdp_frame a;
  memset(&a, 0, sizeof(a));
  
  a.packetTypeAndFlags=MDP_GETADDRS;
  a.addrlist.mode = MDP_ADDRLIST_MODE_SELF;
  a.addrlist.first_sid=index;
  a.addrlist.last_sid=OVERLAY_MDP_ADDRLIST_MAX_SID_COUNT;
  a.addrlist.frame_sid_count=MDP_MAX_SID_REQUEST;
  int result=__overlay_mdp_send(&a,MDP_AWAITREPLY,5000);
  if (result) {
    if (a.packetTypeAndFlags == MDP_ERROR)
      DEBUGF("MDP Server error #%d: '%s'", a.error.error, a.error.message);
    return WHY("Failed to get local address list");
  }
  if ((a.packetTypeAndFlags&MDP_TYPE_MASK)!=MDP_ADDRLIST)
    return WHY("MDP Server returned something other than an address list");
  if (0) DEBUGF("local addr 0 = %s",alloca_tohex_sid(a.addrlist.sids[0]));
  bcopy(&a.addrlist.sids[0][0], sid->binary, sizeof sid->binary);
  return 0;
}

int __overlay_mdp_relevant_bytes(overlay_mdp_frame *mdp) 
{
  int len;
  switch(mdp->packetTypeAndFlags&MDP_TYPE_MASK)
  {
    case MDP_ROUTING_TABLE:
    case MDP_GOODBYE:
      /* no arguments for saying goodbye */
      len=&mdp->raw[0]-(char *)mdp;
      break;
    case MDP_ADDRLIST: 
      len=(&mdp->addrlist.sids[0][0]-(unsigned char *)mdp) + mdp->addrlist.frame_sid_count*SID_SIZE;
      break;
    case MDP_GETADDRS: 
      len=&mdp->addrlist.sids[0][0]-(unsigned char *)mdp;
      break;
    case MDP_TX: 
      len=(&mdp->out.payload[0]-(unsigned char *)mdp) + mdp->out.payload_length; 
      break;
    case MDP_BIND:
      len=(&mdp->raw[0] - (char *)mdp) + sizeof(sockaddr_mdp);
      break;
    case MDP_SCAN:
      len=(&mdp->raw[0] - (char *)mdp) + sizeof(struct __overlay_mdp_scan);
      break;
    case MDP_ERROR: 
      /* This formulation is used so that we don't copy any bytes after the
       end of the string, to avoid information leaks */
      len=(&mdp->error.message[0]-(char *)mdp) + strlen(mdp->error.message)+1;      
      if (mdp->error.error) INFOF("mdp return/error code: %d:%s",mdp->error.error,mdp->error.message);
      break;
    default:
      return WHY("Illegal MDP frame type.");
  }
  return len;
}
