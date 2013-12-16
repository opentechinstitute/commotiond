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
#include "tree.h"

#include "serval-dna.h"

#define CHECK_ERR(A, M, ...) if(!(A)) { \
    ERROR(M, ##__VA_ARGS__); \
    if (err_msg == NULL) \
      err_msg = co_list16_create(); \
    char *msg = NULL; \
    int len = snprintf(NULL, 0, M, ##__VA_ARGS__); \
    msg = calloc(len,sizeof(char)); \
    sprintf(msg, M, ##__VA_ARGS__); \
    if (len < UINT8_MAX) { \
      co_list_append(err_msg, co_str8_create(msg,len,0)); \
    } else if (len < UINT16_MAX) { \
      co_list_append(err_msg, co_str8_create(msg,len,0)); \
    } else if (len < UINT32_MAX) { \
      co_list_append(err_msg, co_str8_create(msg,len,0)); \
    } \
    free(msg); \
    goto error; \
  }

#define CLEAR_ERR() if (err_msg) { co_obj_free(err_msg); err_msg = NULL; }

#define INS_ERROR() if (err_msg) { co_tree_insert(*output, "errors", 6, err_msg); }

extern struct subscriber *my_subscriber;
extern keyring_file *keyring;

char *serval_path = NULL;

keyring_file *mdp_keyring = NULL;
unsigned char *mdp_key = NULL;
int mdp_key_len = 0;
static co_obj_t *err_msg = NULL;

static int serval_create_signature(unsigned char *key,
			const unsigned char *msg,
			const size_t msg_len,
			unsigned char *sig_buffer,
			const size_t sig_size) {
  
  unsigned long long sig_length = SIGNATURE_BYTES;
  
  CHECK(sig_size >= msg_len + SIGNATURE_BYTES,"Signature buffer too small");
  
  unsigned char hash[crypto_hash_sha512_BYTES]; 
  
  crypto_hash_sha512(hash, msg, msg_len); // create sha512 hash of message, which will then be signed
  memcpy(sig_buffer,msg,msg_len);
  
  if (crypto_create_signature(key, hash, crypto_hash_sha512_BYTES, &sig_buffer[msg_len], &sig_length) != 0) // create signature of message hash, append it to end of message
    return 0;
  return 1;
  
error:  
  return 0;
}

static int serval_extract_sas(unsigned char **key, int *key_len, keyring_file *_keyring, unsigned char *_sid) {
    *key=keyring_find_sas_private(_keyring, _sid, NULL); // get SAS key associated with our SID
    CHECK_ERR(*key,"Failed to fetch SAS key");
    if (key_len) *key_len = crypto_sign_edwards25519sha512batch_SECRETKEYBYTES;
    return 1;
error:
    return 0;
}

int serval_init_keyring(unsigned char *sid,
		       const size_t sid_len,
		       const char *keyring_path,
		       const size_t keyring_len,
		       keyring_file **_keyring,
		       unsigned char **key,
		       int *key_len) {
  keyring_identity *new_ident;
  char keyringFile[PATH_MAX] = {0};
  unsigned char *_sid = sid;
  char *abs_path = NULL;
  int ret = 0;
  
  if (sid) CHECK(sid_len == SID_SIZE,"Invalid SID");
  
  if (keyring_path == NULL || keyring_len == 0) { // if no keyring specified, use default keyring
    strcpy(keyringFile,serval_path);
    if (serval_path[strlen(serval_path) - 1] != '/')
      strcat(keyringFile,"/");
    strcat(keyringFile,"serval.keyring");
  }
  else { // otherwise, use specified keyring (NOTE: if keyring does not exist, it will be created)
    CHECK(keyring_len < PATH_MAX,"Keyring length too long");
    strncpy(keyringFile,keyring_path,keyring_len);
    keyringFile[keyring_len] = '\0';
    
    // Fetching SAS keys requires setting the SERVALINSTANCE_PATH environment variable
    CHECK_ERR((abs_path = realpath(keyringFile,NULL)) != NULL,"Error deriving absolute path from given keyring file: %s",keyringFile);
    *strrchr(abs_path,'/') = '\0';
    CHECK_ERR(setenv("SERVALINSTANCE_PATH",abs_path,1) == 0,"Failed to set SERVALINSTANCE_PATH env variable");
  }
  
  CHECK_ERR((*_keyring = keyring_open(keyringFile)),"Failed to open specified keyring file");
  CHECK_ERR(keyring_enter_pin(*_keyring, KEYRING_PIN) > 0,"No valid Serval keys found with NULL PIN"); // unlocks Serval keyring for using identities (also initializes global default identity my_subscriber)
  
  if (!sid) {
    //create new sid
    int c;
    for(c = 0; c < (*_keyring)->context_count; c++) { // cycle through the keyring contexts until we find one with room for another identity
      new_ident = keyring_create_identity(*_keyring,(*_keyring)->contexts[c], KEYRING_PIN); // create new Serval identity
      if (new_ident)
	break;
    }
    CHECK_ERR(new_ident,"failed to create new SID");
    
    CHECK_ERR(keyring_commit(*_keyring) == 0,"Failed to save new SID into keyring"); // need to commit keyring or else new identity won't be saved (needs permissions)
    
    _sid = new_ident->subscriber->sid;
  }
  
  if (key)
    serval_extract_sas(key,key_len, *_keyring, _sid);
  
  ret = 1;
error:
  if (abs_path) free(abs_path);
  return ret;
}

int cmd_serval_sign(const char *sid_str, 
	 const size_t sid_len,
	 const unsigned char *msg,
	 const size_t msg_len,
	 char *sig_str_buf,
	 const size_t sig_str_size,
	 const char *keyring_path,
	 const size_t keyring_len) {
  
  int ret = 0;
  unsigned char signed_msg[msg_len + SIGNATURE_BYTES];
  keyring_file *_keyring = NULL;
  unsigned char *key = NULL;
  unsigned char packedSid[SID_SIZE] = {0};
  
  CHECK(sig_str_size >= 2*SIGNATURE_BYTES + 1,"Signature buffer too small");
  
  if (sid_str) {
    CHECK_ERR(sid_len == 2*SID_SIZE && str_is_subscriber_id(sid_str) == 1,"Invalid SID");
    stowSid(packedSid,0,sid_str);
  }
  
  if (keyring_path) {
    CHECK_ERR(serval_init_keyring(sid_str ? packedSid : NULL,
                     sid_str ? SID_SIZE : 0,
		     keyring_path,
		     keyring_len,
		     &_keyring,
		     &key,
		     NULL), "Failed to initialize Serval keyring");
  } else {
    CHECK_ERR(serval_extract_sas(&key,NULL,keyring,packedSid),"Failed to fetch SAS key");
  }
  
  CHECK_ERR(serval_create_signature(key, msg, msg_len, signed_msg, SIGNATURE_BYTES + msg_len),"Failed to create signature");
  
  strncpy(sig_str_buf,alloca_tohex(signed_msg + msg_len,SIGNATURE_BYTES),2*SIGNATURE_BYTES);
  sig_str_buf[2*SIGNATURE_BYTES] = '\0';

  ret = 1;
error:
  if (_keyring) keyring_free(_keyring);
  return ret;
}

static int keyring_send_sas_request_client(struct subscriber *subscriber){
  int sent, client_port, found = 0, ret = 0;
  int siglen=SID_SIZE+crypto_sign_edwards25519sha512batch_BYTES;
  unsigned char *srcsid[SID_SIZE] = {0}, *plain = NULL;
  unsigned char signature[siglen];
  time_ms_t now = gettime_ms();
  
  CHECK_ERR(overlay_mdp_getmyaddr(0,(sid_t *)srcsid) == 0,"Could not get local address");

  if (subscriber->sas_valid)
    return 1;
  
  CHECK_ERR(now >= subscriber->sas_last_request + 100,"Too soon to ask for SAS mapping again");
  
  CHECK_ERR(my_subscriber,"couldn't request SAS (I don't know who I am)");
  
  DEBUG("Requesting SAS mapping for SID=%s", alloca_tohex_sid(subscriber->sid));
  
  CHECK_ERR(overlay_mdp_bind((sid_t *)my_subscriber->sid,(client_port=32768+(random()&32767))) == 0,"Failed to bind to client socket");

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
    CHECK_ERR(mdp.packetTypeAndFlags != MDP_ERROR,"MDP Server error #%d: '%s'",mdp.error.error,mdp.error.message);
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
  
  CHECK_ERR(keytype == KEYTYPE_CRYPTOSIGN,"Ignoring SID:SAS mapping with unsupported key type %u", keytype);
  
  CHECK_ERR(mdp.out.payload_length >= 1 + SAS_SIZE,"Truncated key mapping announcement? payload_length: %d", mdp.out.payload_length);
  
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
  CHECK_ERR(r == 0,"SID:SAS mapping verification signature does not verify");

  /* These next two tests should never be able to fail, but let's just check anyway. */
  CHECK_ERR(plain_len == SID_SIZE,"SID:SAS mapping signed block is wrong length");
  CHECK_ERR(memcmp(plain, mdp.out.src.sid, SID_SIZE) == 0,"SID:SAS mapping signed block is for wrong SID");
  
  memmove(subscriber->sas_public, sas_public, SAS_SIZE);
  subscriber->sas_valid=1;
  subscriber->sas_last_request=now;
  ret = 1;
  
error:
  if (plain) free(plain);
  return ret;
}

int cmd_serval_verify(const char *sas_key,
		   const size_t sas_key_len,
		   const unsigned char *msg,
		   const size_t msg_len,
		   const char *sig,
		   const size_t sig_len) {
  int verdict = 0;
  
  unsigned char bin_sig[SIGNATURE_BYTES];
  unsigned char bin_sas[SAS_SIZE];
  
  CHECK_ERR(sig_len == 2*SIGNATURE_BYTES,"Invalid signature");
  CHECK_ERR(sas_key_len == 2*SAS_SIZE,"Invalid SAS key");
  
  // convert signature from hex to binary
  CHECK_ERR(fromhexstr(bin_sig,sig,SIGNATURE_BYTES) == 0,"Invalid signature");
  CHECK_ERR(fromhexstr(bin_sas,sas_key,SAS_SIZE) == 0,"Invalid SAS key");
  
  DEBUG("Message to verify:\n%s",msg);
  
  unsigned char hash[crypto_hash_sha512_BYTES];
  crypto_hash_sha512(hash,msg,msg_len);
  
  if (crypto_verify_signature(bin_sas, hash, crypto_hash_sha512_BYTES,
    &bin_sig[0], SIGNATURE_BYTES) == 0)
    verdict = 1;  // successfully verified
    
error:
  return verdict;
}

int serval_verify_client(const char *sid_str,
	   const size_t sid_len,
	   const unsigned char *msg,
	   const size_t msg_len,
	   const char *sig,
	   const size_t sig_len
 	   /*const char *keyring_path,
 	   const size_t keyring_len*/) {
  
  int verdict = 0;
  char sas_str[2*SAS_SIZE+1] = {0};
  unsigned char packedSid[SID_SIZE] = {0};
  char keyring_path[PATH_MAX] = {0};
  
  CHECK(sid_len == 2*SID_SIZE,"Invalid SID length");
  CHECK(sig_len == 2*SIGNATURE_BYTES,"Invalid signature length");
  
  CHECK(str_is_subscriber_id(sid_str) != 0,"Invalid SID");
  stowSid(packedSid,0,sid_str);
  
  FORM_SERVAL_INSTANCE_PATH(keyring_path, "serval.keyring");
  CHECK(serval_init_keyring(packedSid,
			 SID_SIZE,
			 keyring_path,
			 strlen(keyring_path),
			 &keyring,
			 NULL,
			 NULL), "Failed to initialize Serval keyring");
      
  struct subscriber *sub = find_subscriber(packedSid, SID_SIZE, 1); // get Serval identity described by given SID
  
  CHECK(sub,"Failed to fetch Serval subscriber");
  
  CHECK(keyring_send_sas_request_client(sub),"SAS request failed");
  
  CHECK(sub->sas_valid,"Could not validate the signing key!");
  CHECK(sub->sas_public[0],"Could not validate the signing key!");
  CHECK(tohex(sas_str,sub->sas_public,SAS_SIZE),"Failed to convert signing key");
  
  verdict = cmd_serval_verify(sas_str,2*SAS_SIZE,
			   msg,msg_len,sig,sig_len);
  
error:
  return verdict;
}

int serval_crypto_register(void) {
  /** name: serval-crypto
   * param[1] - param[4]: (co_str?_t)
   */
  
  const char name[] = "serval-crypto",
  usage[] = "serval-crypto sign [<SID>] <MESSAGE> [--keyring=<KEYRING_PATH>]\n"
            "serval-crypto verify <SAS> <SIGNATURE> <MESSAGE>\n",
  desc[] =   "Serval-crypto utilizes Serval's crypto API to:\n"
  "      * Sign any arbitrary text using a Serval key. If no Serval key ID (SID) is given,\n"
  "             a new key will be created on the default Serval keyring.\n"
  "      * Verify any arbitrary text, a signature, and a Serval key ID (SID), and will\n"
  "             determine if the signature is valid.\n\n\0";
  
  CHECK(co_cmd_register(name,sizeof(name),usage,sizeof(usage),desc,sizeof(desc),serval_crypto_handler),"Failed to register commands");
  
  return 1;
error:
  return 0;
}

int olsrd_mdp_register(void) {
  /**
   * name: olsrd-mdp
   * param[1] <required>: <SID> (co_bin8_t)
   * param[2]: --keyring=<keyring_path> (co_str8_t)
   */
  const char name[] = "olsrd-mdp";
  
  CHECK(co_cmd_register(name,sizeof(name),"",1,"",1,olsrd_mdp_init),"Failed to register command");
  
  return 1;
error:
  return 0;
}

int olsrd_mdp_sign_register(void) {
  /**
   * name: mdp-sign
   * param[1] <required>: data (co_bin?_t)
   */
  
  const char name[] = "mdp-sign";
  
  CHECK(co_cmd_register(name,sizeof(name),"",1,"",1,olsrd_mdp_sign),"Failed to register command");
  
  return 1;
error:
  return 0;
}

int serval_crypto_handler(co_obj_t *self, co_obj_t **output, co_obj_t *params) {
  CLEAR_ERR();
  
  int list_len = co_list_length(params), keypath = 0;
  
  CHECK_ERR(IS_LIST(params) && list_len >= 2,"Invalid params");
  
  if (!strncmp("--keyring=",co_obj_data_ptr(co_list_get_last(params)),10)) {
    keypath = 1;
    --list_len;
  }
  
  if (co_str_cmp_str(co_list_element(params,0),"sign") == 0) {
    
    CHECK_ERR(list_len == 2 || list_len == 3,"Invalid arguments");
    char sig_buf[2*SIGNATURE_BYTES + 1] = {0};
    if (list_len == 3) {
      CHECK_ERR(cmd_serval_sign(_LIST_ELEMENT(params,1),
			co_str_len(co_list_element(params,1)) - 1,
			(unsigned char*)_LIST_ELEMENT(params,2),
			co_str_len(co_list_element(params,2)) - 1,
			sig_buf,
			2*SIGNATURE_BYTES + 1,
			keypath ? _LIST_ELEMENT(params,3) + 10 : NULL, // strlen("--length=") == 10
			keypath ? co_str_len(co_list_element(params,3)) - 11 : 0),"Failed to create signature");
    } else if (list_len == 2) {
      CHECK_ERR(cmd_serval_sign(NULL,
			0,
			(unsigned char*)_LIST_ELEMENT(params,1),
			co_str_len(co_list_element(params,1)) - 1,
			sig_buf,
			2*SIGNATURE_BYTES + 1,
			keypath ? _LIST_ELEMENT(params,2) + 10 : NULL, // strlen("--length=") == 10
			keypath ? co_str_len(co_list_element(params,2)) - 11 : 0),"Failed to create signature");
    }
    CMD_OUTPUT("result",co_str8_create(sig_buf,2*SIGNATURE_BYTES+1,0));
    
  } else if (co_str_cmp_str(co_list_element(params,0),"verify") == 0) {
    
    CHECK_ERR(!keypath,"Keyring option not available for verification");
    CHECK_ERR(list_len == 4,"Invalid arguments");
    int verdict = cmd_serval_verify(_LIST_ELEMENT(params,1),
				  co_str_len(co_list_element(params,1)) - 1,
				  (unsigned char*)_LIST_ELEMENT(params,3),
				  co_str_len(co_list_element(params,3)) - 1,
				  _LIST_ELEMENT(params,2),
				  co_str_len(co_list_element(params,2)));
// 				  keypath ? _LIST_ELEMENT(params,4) + 10 : NULL, // strlen("--length=") == 10
// 				  keypath ? co_str_len(co_list_element(params,4)) - 10 : 0);
    if (verdict == 1) {
      CMD_OUTPUT("result",co_str8_create("Message verified!",sizeof("Message verified!"),0));
    } else if (verdict == 0) {
      CMD_OUTPUT("result",co_str8_create("Message NOT verified!",sizeof("Message NOT verified!"),0));
    }
    
  }
  
  return 1;
error:
  INS_ERROR();
  return 0;
}

int olsrd_mdp_init(co_obj_t *self, co_obj_t **output, co_obj_t *params) {
  CLEAR_ERR();
  
  int list_len = co_list_length(params);
  char *keyring_path = NULL;
  int keyring_len = 0;
  
  CHECK_ERR(IS_LIST(params) && list_len >= 2,"Invalid params");
  
  if (list_len == 3) {
    CHECK_ERR(!strncmp("--keyring=",co_obj_data_ptr(co_list_element(params,2)),10),"Invalid keyring");
    keyring_len = co_obj_data(&keyring_path,co_list_element(params,2));
  }
  
  CHECK_ERR(serval_init_keyring((unsigned char*)_LIST_ELEMENT(params,1),
		     co_str_len(co_list_element(params,1)),
		     keyring_path,
		     keyring_len,
		     &mdp_keyring,
		     &mdp_key,
		     &mdp_key_len), "Failed to initialize Serval keyring");
  
  CMD_OUTPUT("success",co_bool_create(true,0));
  
  return 1;
error:
  INS_ERROR();
  return 0;
}

int olsrd_mdp_sign(co_obj_t *self, co_obj_t **output, co_obj_t *params) {
  CLEAR_ERR();
  
  /** this is not meant to be used by mere humans, so skipping some error checking */
  
//   int list_len = co_list_length(params);
  int msg_len = 0, ret = 0, sig_buf_len;
  unsigned char *msg = NULL, *sig_buf = NULL;
  
  CHECK(mdp_keyring && mdp_key && mdp_key_len,"Haven't run olsrd_mdp_init");
//   CHECK(IS_LIST(params) && list_len == 2,"Invalid params");
  
  msg_len = co_obj_data((char**)&msg,co_list_element(params,1));
  sig_buf_len = SIGNATURE_BYTES + msg_len + 1;
  sig_buf = calloc(sig_buf_len,sizeof(unsigned char));
  
  CHECK(serval_create_signature(mdp_key,
                     msg,
		     msg_len,
		     sig_buf,
		     sig_buf_len),"Failed to sign OLSRd packet");
  
  CMD_OUTPUT("sig",co_bin8_create((char*)sig_buf,sig_buf_len,0));
  
  ret = 1;
error:
  if (sig_buf) free(sig_buf);
  return ret;
}
