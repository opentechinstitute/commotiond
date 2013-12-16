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

#include <serval.h>
#include <serval/conf.h>
#include <serval/rhizome.h>
#include <serval/crypto.h>
#include <serval/str.h>
#include <serval/overlay_address.h>

#include "sas_request.h"
#include "debug.h"
#include "mdp_client.h"

int keyring_send_sas_request_client(struct subscriber *subscriber){
  int sent, client_port, found = 0, ret = 0;
  int siglen=SID_SIZE+crypto_sign_edwards25519sha512batch_BYTES;
  unsigned char *srcsid[SID_SIZE] = {0};
  unsigned char signature[siglen];
  time_ms_t now = __gettime_ms();
  
  CHECK(__overlay_mdp_getmyaddr(0,(sid_t *)srcsid) == 0,"Could not get local address");
  
  if (subscriber->sas_valid)
    return 1;
  
  CHECK(now >= subscriber->sas_last_request + 100,"Too soon to ask for SAS mapping again");
  
  CHECK(__overlay_mdp_bind((sid_t *)srcsid,(client_port=32768+(random()&32767))) == 0,"Failed to bind to client socket");
  
  /* request mapping (send request auth-crypted). */
  overlay_mdp_frame mdp;
  memset(&mdp,0,sizeof(mdp));  
  
  mdp.packetTypeAndFlags=MDP_TX;
  mdp.out.queue=OQ_MESH_MANAGEMENT;
  memmove(mdp.out.dst.sid,subscriber->sid,SID_SIZE);
  mdp.out.dst.port=MDP_PORT_KEYMAPREQUEST;
  mdp.out.src.port=client_port;
  memmove(mdp.out.src.sid,srcsid,SID_SIZE);
  mdp.out.payload_length=1;
  mdp.out.payload[0]=KEYTYPE_CRYPTOSIGN;
  
  sent = __overlay_mdp_send(&mdp, 0,0);
  if (sent) {
    DEBUG("Failed to send SAS resolution request: %d", sent);
    CHECK(mdp.packetTypeAndFlags != MDP_ERROR,"MDP Server error #%d: '%s'",mdp.error.error,mdp.error.message);
  }
  
  time_ms_t timeout = now + 5000;
  
  while(now<timeout) {
    time_ms_t timeout_ms = timeout - __gettime_ms();
    int result = __overlay_mdp_client_poll(timeout_ms);
    
    if (result>0) {
      int ttl=-1;
      if (__overlay_mdp_recv(&mdp, client_port, &ttl)==0) {
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
    now=__gettime_ms();
  }
  
  unsigned keytype = mdp.out.payload[0];
  
  CHECK(keytype == KEYTYPE_CRYPTOSIGN,"Ignoring SID:SAS mapping with unsupported key type %u", keytype);
  
  CHECK(mdp.out.payload_length >= 1 + SAS_SIZE,"Truncated key mapping announcement? payload_length: %d", mdp.out.payload_length);
  
  unsigned char *sas_public=&mdp.out.payload[1];
  unsigned char *compactsignature = &mdp.out.payload[1+SAS_SIZE];
  
  /* reconstitute signed SID for verification */
  memmove(&signature[0],&compactsignature[0],64);
  memmove(&signature[64],&mdp.out.src.sid[0],SID_SIZE);
  
  memmove(subscriber->sas_public, sas_public, SAS_SIZE);
  subscriber->sas_valid=1;
  subscriber->sas_last_request=now;
  ret = 1;
  
error:
  return ret;
}