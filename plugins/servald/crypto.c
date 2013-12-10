#include <stdio.h>
#include <string.h>
#include <assert.h>
#include <serval.h>
#include <serval/overlay_address.h>
#include <serval/mdp_client.h>
#include <serval/crypto.h>
#include <serval/str.h>

#include "obj.h"
#include "list.h"
#include "cmd.h"
#include "debug.h"
#include "crypto.h"

// extern keyring_file *keyring; // keyring is global Serval variable

int serval_sign(const char *sid, 
	 const size_t sid_len,
	 const unsigned char *msg,
	 const size_t msg_len,
	 char *sig_buffer,
	 const size_t sig_size,
	 const char *keyringName,
	 const size_t keyring_len) {
  
  int ret = -1;
  unsigned long long sig_length = SIGNATURE_BYTES;
  unsigned char signed_msg[msg_len + sig_length];
  keyring_file *keyring = NULL;
  keyring_identity *new_ident;
  char keyringFile[1024];
  
  CHECK(msg_len,"No message");
  
  if (sid) CHECK(sid_len == 2*SID_SIZE,"Invalid SID length");
  
  if (keyringName == NULL || keyring_len == 0) { 
    FORM_SERVAL_INSTANCE_PATH(keyringFile, "serval.keyring"); // if no keyring specified, use default keyring
  }
  else { // otherwise, use specified keyring (NOTE: if keyring does not exist, it will be created)
    strncpy(keyringFile,keyringName,keyring_len);
    keyringFile[keyring_len] = '\0';
  }
  
  keyring = keyring_open(keyringFile);
  keyring_enter_pin(keyring, KEYRING_PIN); // unlocks Serval keyring for using identities (also initializes global default identity my_subscriber)
  
  if (!sid) {
    //create new sid
    int c;
    for(c=0;c<keyring->context_count;c++) { // cycle through the keyring contexts until we find one with room for another identity
      new_ident = keyring_create_identity(keyring,keyring->contexts[c], KEYRING_PIN); // create new Serval identity
      if (new_ident)
	break;
    }
    CHECK(new_ident,"failed to create new SID");

    CHECK(keyring_commit(keyring) == 0,"Failed to save new SID into keyring"); // need to commit keyring or else new identity won't be saved (needs permissions)

    sid = alloca_tohex_sid(new_ident->subscriber->sid); // convert SID from binary to hex
  } else {
    CHECK(str_is_subscriber_id(sid),"Invalid SID");
  }
  
  unsigned char packedSid[SID_SIZE];
  stowSid(packedSid,0,sid);
  
  unsigned char *key=keyring_find_sas_private(keyring, packedSid, NULL); // get SAS key associated with our SID
  CHECK(key,"Failed to fetch SAS key");
  
  unsigned char hash[crypto_hash_sha512_BYTES]; 
  
  crypto_hash_sha512(hash, msg, msg_len); // create sha512 hash of message, which will then be signed
  memcpy(signed_msg,msg,msg_len);
  
  ret = crypto_create_signature(key, hash, crypto_hash_sha512_BYTES, &signed_msg[msg_len], &sig_length); // create signature of message hash, append it to end of message
  
  if (ret == 0) { //success
    if (sig_size > 0) {
      if (sig_size >= 2*sig_length + 1) {
        strncpy(sig_buffer,alloca_tohex(signed_msg + msg_len,sig_length),2*sig_length);
        sig_buffer[2*sig_length] = '\0';
      } else
	ERROR("Insufficient signature buffer size");
    }
  }

error:
  if (keyring) keyring_free(keyring);
  return ret;
}

static int keyring_send_sas_request_client(struct subscriber *subscriber){
  int sent, client_port, found = 0, ret = -1;
  int siglen=SID_SIZE+crypto_sign_edwards25519sha512batch_BYTES;
  unsigned char *srcsid[SID_SIZE] = {0}, *plain = NULL;
  unsigned char signature[siglen];
  time_ms_t now = gettime_ms();
  
  CHECK(overlay_mdp_getmyaddr(0,(sid_t *)srcsid) == 0,"Could not get local address");

  if (subscriber->sas_valid)
    return 0;
  
  if (now < subscriber->sas_last_request + 100){
    DEBUG("Too soon to ask for SAS mapping again");
    return 0;
  }
  
  CHECK(my_subscriber,"couldn't request SAS (I don't know who I am)");
  
  DEBUG("Requesting SAS mapping for SID=%s", alloca_tohex_sid(subscriber->sid));
  
  CHECK(overlay_mdp_bind((sid_t *)my_subscriber->sid,(client_port=32768+(random()&32767))) == 0,"Failed to bind to client socket");

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
  
  sent = overlay_mdp_send(&mdp, 0,0);
  if (sent) {
    DEBUG("Failed to send SAS resolution request: %d", sent);
    CHECK(mdp.packetTypeAndFlags != MDP_ERROR,"MDP Server error #%d: '%s'",mdp.error.error,mdp.error.message);
  }
  
  time_ms_t timeout = now + 5000;

  while(now<timeout) {
    time_ms_t timeout_ms = timeout - gettime_ms();
    int result = overlay_mdp_client_poll(timeout_ms);
    
    if (result>0) {
      int ttl=-1;
      if (overlay_mdp_recv(&mdp, client_port, &ttl)==0) {
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
    if (servalShutdown)
      break;
  }

  unsigned keytype = mdp.out.payload[0];
  
  CHECK(keytype == KEYTYPE_CRYPTOSIGN,"Ignoring SID:SAS mapping with unsupported key type %u", keytype);
  
  CHECK(mdp.out.payload_length >= 1 + SAS_SIZE,"Truncated key mapping announcement? payload_length: %d", mdp.out.payload_length);
  
  plain = (unsigned char*)calloc(mdp.out.payload_length,sizeof(unsigned char));
  unsigned long long plain_len=0;
  unsigned char *sas_public=&mdp.out.payload[1];
  unsigned char *compactsignature = &mdp.out.payload[1+SAS_SIZE];
  
  /* reconstitute signed SID for verification */
  memmove(&signature[0],&compactsignature[0],64);
  memmove(&signature[64],&mdp.out.src.sid[0],SID_SIZE);
  
  int r=crypto_sign_edwards25519sha512batch_open(plain,&plain_len,
						 signature,siglen,
						 sas_public);
  CHECK(r == 0,"SID:SAS mapping verification signature does not verify");

  /* These next two tests should never be able to fail, but let's just check anyway. */
  CHECK(plain_len == SID_SIZE,"SID:SAS mapping signed block is wrong length");
  CHECK(memcmp(plain, mdp.out.src.sid, SID_SIZE) == 0,"SID:SAS mapping signed block is for wrong SID");
  
  memmove(subscriber->sas_public, sas_public, SAS_SIZE);
  subscriber->sas_valid=1;
  subscriber->sas_last_request=now;
  ret = 0;
  
error:
  if (plain) free(plain);
  return ret;
}

int serval_verify(const char *sid,
	   const size_t sid_len,
	   const unsigned char *msg,
	   const size_t msg_len,
	   const char *sig,
	   const size_t sig_len,
	   const char *keyringName,
	   const size_t keyring_len) {
  
  char *abs_path = NULL;
  int verdict = -1;
  unsigned char combined_msg[msg_len + SIGNATURE_BYTES];
  keyring_file *keyring = NULL;
  
  CHECK(sid_len == 2*SID_SIZE,"Invalid SID length");
  CHECK(sig_len == 2*SIGNATURE_BYTES,"Invalid signature length");
  
  unsigned char bin_sig[SIGNATURE_BYTES];
  // convert signature from hex to binary
  CHECK(fromhexstr(bin_sig,sig,SIGNATURE_BYTES) == 0,"Invalid signature");

  CHECK(str_is_subscriber_id(sid) != 0,"Invalid SID");
  
  memcpy(combined_msg,msg,msg_len);
  memcpy(combined_msg + msg_len,bin_sig,SIGNATURE_BYTES); // append signature to end of message
  int combined_msg_length = msg_len + SIGNATURE_BYTES;
  
  char keyringFile[1024];
  
  if (keyringName == NULL || keyring_len == 0) { 
    FORM_SERVAL_INSTANCE_PATH(keyringFile, "serval.keyring"); // if no keyring specified, use default keyring
  }
  else { // otherwise, use specified keyring (NOTE: if keyring does not exist, it will be created)
    strncpy(keyringFile,keyringName,keyring_len);
    keyringFile[keyring_len] = '\0';
    // Fetching SAS keys requires setting the SERVALINSTANCE_PATH environment variable
    CHECK((abs_path = realpath(keyringFile,NULL)) != NULL,"Error deriving absolute path from given keyring file");
    *strrchr(abs_path,'/') = '\0';
    CHECK(setenv("SERVALINSTANCE_PATH",abs_path,1) == 0,"Failed to set SERVALINSTANCE_PATH env variable");
  }
  
  keyring = keyring_open(keyringFile);
  CHECK(keyring,"Failed to open Serval keyring");

  int num_identities = keyring_enter_pin(keyring, KEYRING_PIN); // unlocks Serval keyring for using identities (also initializes global default identity my_subscriber)
  CHECK(num_identities > 0,"Failed to unlock any Serval identities");
  
  unsigned char packedSid[SID_SIZE];
  stowSid(packedSid,0,sid);
  
  struct subscriber *src_sub = find_subscriber(packedSid, SID_SIZE, 1); // get Serval identity described by given SID
  
  CHECK(src_sub,"Failed to fetch Serval subscriber");
  
  CHECK(keyring_send_sas_request_client(src_sub) == 0,"SAS request failed");
  
  CHECK(src_sub->sas_valid,"Could not validate the signing key!");
  
  DEBUG("Message to verify:\n%s",msg);
  
  verdict = crypto_verify_message(src_sub, combined_msg, &combined_msg_length);
  
error:
  if (abs_path) free(abs_path);
  if (keyring) keyring_free(keyring);
   
  return verdict;
}

int serval_crypto_register(void) {
  const char name[] = "serval-crypto",
  usage[] = "serval-crypto (Serval Crypto) 3.0\n"
  "Usage: serval-crypto sign|verify [<SID>] [<SIGNATURE>] <MESSAGE>\n\n"
  "Commands:\n\n"
  "        sign                    Sign a message with a Serval key\n"
  "        verify                  Verify a signed message with a Serval key\n\n"
  "Options:\n\n"
  "  [<SID>]                       Serval ID (SID) used to sign or verify. If a SID is not provided\n"
  "                                     when signing a message, a new SID is created.\n"
  "  [<SIGNATURE>]                 Signature of message to verify\n"
  "  <MESSAGE>                     Message to sign or verify (not including signature)\n\n",
  desc[] =   "Serval-crypto utilizes Serval's crypto API to:\n"
  "      * Sign any arbitrary text using a Serval key. If no Serval key ID (SID) is given,\n"
  "             a new key will be created on the default Serval keyring.\n"
  "      * Verify any arbitrary text, a signature, and a Serval key ID (SID), and will\n"
  "             determine if the signature is valid.\n\n";
  
  CHECK(co_cmd_register(name,strlen(name),usage,strlen(usage),desc,strlen(desc),serval_crypto_handler),"Failed to register commands");
  
  return 0;
error:
  return -1;
}

co_obj_t *serval_crypto_handler(co_obj_t *self, co_obj_t *params) {
  co_obj_t *ret = NULL;
  int list_len = co_list_length(params);
  
  CHECK(IS_LIST(params),"Invalid params");
  CHECK(list_len >= 2 && list_len <= 4,"Invalid number of arguments");
  
  if (co_str_cmp_str(co_list_element(params,0),"sign") == 0) {
    CHECK(list_len == 2 || list_len == 3,"Invalid arguments");
    char sig_buf[2*SIGNATURE_BYTES + 1] = {0};
    if (list_len == 3) {
      CHECK(serval_sign(_LIST_ELEMENT(params,1),
			strlen(_LIST_ELEMENT(params,1)),
			_LIST_ELEMENT(params,2),
			strlen(_LIST_ELEMENT(params,2)),
			sig_buf,
			2*SIGNATURE_BYTES + 1,
			NULL,
			0) == 0,"Failed to create signature");
    } else {
      CHECK(serval_sign(NULL,
			0,
			_LIST_ELEMENT(params,1),
			strlen(_LIST_ELEMENT(params,1)),
			sig_buf,
			2*SIGNATURE_BYTES + 1,
			NULL,
			0) == 0,"Failed to create signature");
    }
    ret = co_str8_create(sig_buf,2*SIGNATURE_BYTES+1,0);
  } else if (co_str_cmp_str(co_list_element(params,0),"verify") == 0) {
    CHECK(list_len == 4,"Invalid arguments");
    int verdict = -1;
    CHECK(verdict = serval_verify(_LIST_ELEMENT(params,1),
				  strlen(_LIST_ELEMENT(params,1)),
				  _LIST_ELEMENT(params,2),
				  strlen(_LIST_ELEMENT(params,2)),
				  _LIST_ELEMENT(params,2),
				  strlen(_LIST_ELEMENT(params,2)),
				  NULL,
				  0) >= 0, "Error during signature verification");
    if (verdict == 0) {
      char msg[] = "Message verified!";
      ret = co_str8_create(msg,strlen(msg),0);
    } else if (verdict == 1) {
      char msg[] = "Message NOT verified!";
      ret = co_str8_create(msg,strlen(msg),0);
    }
  }
  
error:
  return ret;
}
