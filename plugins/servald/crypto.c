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

static keyring_file *mdp_keyring;
static unsigned char *mdp_key;
static int mdp_key_len;

static int serval_create_signature(unsigned char *key,
			const unsigned char *msg,
			const size_t msg_len,
			unsigned char *sig_buffer,
			const size_t sig_size) {
  
  int ret = -1;
  unsigned long long sig_length = SIGNATURE_BYTES;
  
  CHECK(sig_size >= msg_len + SIGNATURE_BYTES,"Signature buffer too small");
  
  unsigned char hash[crypto_hash_sha512_BYTES]; 
  
  crypto_hash_sha512(hash, msg, msg_len); // create sha512 hash of message, which will then be signed
  memcpy(sig_buffer,msg,msg_len);
  
  ret = crypto_create_signature(key, hash, crypto_hash_sha512_BYTES, &sig_buffer[msg_len], &sig_length); // create signature of message hash, append it to end of message
  
error:  
  return ret;
}

static int _serval_init(unsigned char *sid,
		       const size_t sid_len,
		       const char *keyring_path,
		       const size_t keyring_len,
		       keyring_file **_keyring,
		       unsigned char **key,
		       int *key_len) {
  int ret = -1;
  
  keyring_identity *new_ident;
  char keyringFile[1024];
  
  if (sid) CHECK(sid_len == SID_SIZE,"Invalid SID");
  
  if (keyring_path == NULL || keyring_len == 0) { 
    FORM_SERVAL_INSTANCE_PATH(keyringFile, "serval.keyring"); // if no keyring specified, use default keyring
  }
  else { // otherwise, use specified keyring (NOTE: if keyring does not exist, it will be created)
    strncpy(keyringFile,keyring_path,keyring_len);
    keyringFile[keyring_len] = '\0';
  }
  
  *_keyring = keyring_open(keyringFile);
  keyring_enter_pin(*_keyring, KEYRING_PIN); // unlocks Serval keyring for using identities (also initializes global default identity my_subscriber)
  
  if (!sid) {
    //create new sid
    int c;
    for(c = 0; c < (*_keyring)->context_count; c++) { // cycle through the keyring contexts until we find one with room for another identity
      new_ident = keyring_create_identity(*_keyring,(*_keyring)->contexts[c], KEYRING_PIN); // create new Serval identity
      if (new_ident)
	break;
    }
    CHECK(new_ident,"failed to create new SID");
    
    CHECK(keyring_commit(*_keyring) == 0,"Failed to save new SID into keyring"); // need to commit keyring or else new identity won't be saved (needs permissions)
    
    sid = new_ident->subscriber->sid;
  }
  
  *key=keyring_find_sas_private(*_keyring, sid, NULL); // get SAS key associated with our SID
  CHECK(*key,"Failed to fetch SAS key");
  
  if (key_len) *key_len = crypto_sign_edwards25519sha512batch_SECRETKEYBYTES;
  
  ret = 0;
error:
  return ret;
}

static int serval_sign(const char *sid_str, 
	 const size_t sid_len,
	 const unsigned char *msg,
	 const size_t msg_len,
	 char *sig_str_buf,
	 const size_t sig_str_size,
	 const char *keyring_path,
	 const size_t keyring_len) {
  
  int ret = -1;
  unsigned char signed_msg[msg_len + SIGNATURE_BYTES];
  keyring_file *_keyring = NULL;
  unsigned char *key = NULL;
  unsigned char *packedSid = NULL;
  
  CHECK(sig_str_size >= 2*SIGNATURE_BYTES + 1,"Signature buffer too small");
  
  if (sid_str) {
    CHECK(sid_len == 2*SID_SIZE,"Invalid SID");
    stowSid(packedSid,0,sid_str);
  }
  
  CHECK(_serval_init(packedSid,
                     packedSid ? SID_SIZE : 0,
		     keyring_path,
		     keyring_len,
		     &_keyring,
		     &key,
		     NULL) == 0, "Failed to initialize Serval keyring");
  
  ret = serval_create_signature(key, msg, msg_len, signed_msg, SIGNATURE_BYTES + msg_len);
  
  if (ret == 0) { //success
	strncpy(sig_str_buf,alloca_tohex(signed_msg + msg_len,SIGNATURE_BYTES),2*SIGNATURE_BYTES);
	sig_str_buf[2*SIGNATURE_BYTES] = '\0';
  }

error:
  if (_keyring) keyring_free(_keyring);
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

static int serval_verify(const char *sid,
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
  /** name: serval-crypto
   * param[1] - param[4]: (co_str?_t)
   */
  
  const char name[] = "serval-crypto",
  usage[] = "serval-crypto (Serval Crypto) 3.0\n"
  "Usage: serval-crypto sign|verify [<SID>] [<SIGNATURE>] <MESSAGE> [--keyring=<KEYRING_PATH>]\n\n"
  "Commands:\n\n"
  "        sign                    Sign a message with a Serval key\n"
  "        verify                  Verify a signed message with a Serval key\n\n"
  "Options:\n\n"
  "  [<SID>]                       Serval ID (SID) used to sign or verify. If a SID is not provided\n"
  "                                     when signing a message, a new SID is created.\n"
  "  [<SIGNATURE>]                 Signature of message to verify\n"
  "  <MESSAGE>                     Message to sign or verify (not including signature)\n"
  "  --keyring=[<KEYRING_PATH>]    Path to Serval keyring to use\n\n",
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

int olsrd_mdp_register(void) {
  /**
   * name: olsrd-mdp
   * param[1] <required>: SID (co_bin8_t)
   * param[2] : keyring path (co_str8_t)
   */
  const char name[] = "olsrd-mdp";
  
  mdp_keyring = NULL;
  mdp_key = NULL;
  mdp_key_len = 0;
  
  CHECK(co_cmd_register(name,strlen(name),NULL,0,NULL,0,olsrd_mdp_init),"Failed to register command");
  
  return 0;
error:
  return -1;
}

int olsrd_mdp_sign_register(void) {
  /**
   * name: mdp-sign
   * param[1] <required>: data (co_bin?_t)
   */
  
  const char name[] = "mdp-sign";
  
  CHECK(co_cmd_register(name,strlen(name),NULL,0,NULL,0,olsrd_mdp_sign),"Failed to register command");
  
  return 0;
error:
  return -1;
}

int serval_crypto_handler(co_obj_t *self, co_obj_t **output, co_obj_t *params) {
  int list_len = co_list_length(params), keypath = 0;
  
  CHECK(IS_LIST(params) && list_len >= 2,"Invalid params");
  
  if (!strncmp("--keyring=",co_obj_data_ptr(_LIST_LAST(params)),10)) {
    keypath = 1;
    --list_len;
  }
  
  if (co_str_cmp_str(co_list_element(params,0),"sign") == 0) {
    
    CHECK(list_len == 2 || list_len == 3,"Invalid arguments");
    char sig_buf[2*SIGNATURE_BYTES + 1] = {0};
    if (list_len == 3) {
      CHECK(serval_sign(_LIST_ELEMENT(params,1),
			co_str_len(co_list_element(params,1)),
			(unsigned char*)_LIST_ELEMENT(params,2),
			co_str_len(co_list_element(params,2)),
			sig_buf,
			2*SIGNATURE_BYTES + 1,
			keypath ? _LIST_ELEMENT(params,3) + 10 : NULL, // strlen("--length=") == 10
			keypath ? co_str_len(co_list_element(params,3)) - 10 : 0) == 0,"Failed to create signature");
    } else if (list_len == 2) {
      CHECK(serval_sign(NULL,
			0,
			(unsigned char*)_LIST_ELEMENT(params,1),
			co_str_len(co_list_element(params,1)),
			sig_buf,
			2*SIGNATURE_BYTES + 1,
			keypath ? _LIST_ELEMENT(params,2) + 10 : NULL, // strlen("--length=") == 10
			keypath ? co_str_len(co_list_element(params,2)) - 10 : 0) == 0,"Failed to create signature");
    }
    *output = co_str8_create(sig_buf,2*SIGNATURE_BYTES+1,0);
    
  } else if (co_str_cmp_str(co_list_element(params,0),"verify") == 0) {
    
    CHECK(list_len == 4,"Invalid arguments");
    int verdict = -1;
    CHECK(verdict = serval_verify(_LIST_ELEMENT(params,1),
				  co_str_len(co_list_element(params,1)),
				  (unsigned char*)_LIST_ELEMENT(params,3),
				  co_str_len(co_list_element(params,3)),
				  _LIST_ELEMENT(params,2),
				  co_str_len(co_list_element(params,2)),
				  keypath ? _LIST_ELEMENT(params,4) + 10 : NULL, // strlen("--length=") == 10
				  keypath ? co_str_len(co_list_element(params,4)) - 10 : 0) >= 0, "Error during signature verification");
    if (verdict == 0) {
      char msg[] = "Message verified!";
      *output = co_str8_create(msg,strlen(msg),0);
    } else if (verdict == 1) {
      char msg[] = "Message NOT verified!";
      *output = co_str8_create(msg,strlen(msg),0);
    }
    
  }
  
  return 0;
error:
  return -1;
}

int olsrd_mdp_init(co_obj_t *self, co_obj_t **output, co_obj_t *params) {
  int list_len = co_list_length(params);
  char *keyring_path = NULL;
  int keyring_len = 0;
  
  CHECK(IS_LIST(params) && list_len >= 2,"Invalid params");
  
  if (list_len == 3) {
    CHECK(!strncmp("--keyring=",co_obj_data_ptr(co_list_element(params,2)),10),"Invalid keyring");
    keyring_len = co_obj_data(&keyring_path,co_list_element(params,2));
  }
  
  CHECK(_serval_init(_LIST_ELEMENT(params,1),
		     co_str_len(co_list_element(params,1)),
		     keyring_path,
		     keyring_len,
		     &mdp_keyring,
		     &mdp_key,
		     &mdp_key_len) == 0, "Failed to initialize Serval keyring");
  
  // TODO: prepare response object
  
  return 0;
error:
  return -1;
}

int olsrd_mdp_sign(co_obj_t *self, co_obj_t **output, co_obj_t *params) {
  int list_len = co_list_length(params), msg_len = 0, ret = -1, sig_buf_len;
  unsigned char *msg = NULL, *sig_buf = NULL;
  
  CHECK(mdp_keyring && mdp_key && mdp_key_len,"Haven't run olsrd_mdp_init");
  CHECK(IS_LIST(params) && list_len == 2,"Invalid params");
  
  msg_len = co_obj_data(&msg,co_list_element(params,1));
  sig_buf_len = SIGNATURE_BYTES + msg_len;
  sig_buf = calloc(sig_buf_len,sizeof(unsigned char));
  
  CHECK(serval_create_signature(mdp_key,
                     msg,
		     msg_len,
		     sig_buf,
		     sig_buf_len) == 0,"Failed to sign OLSRd packet");
  
  *output = co_bin8_create(sig_buf,sig_buf_len,0);
  
  ret = 0;
error:
  if (sig_buf) free(sig_buf);
  return ret;
}