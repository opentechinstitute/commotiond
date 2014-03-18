/**
 *       @file  crypto.c
 *      @brief  serval-dna plugin functionality for signing/verifying
 *                using Serval keys
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
 * ============================================================================
 */

#include <stdio.h>
#include <string.h>
#include <assert.h>

#include "config.h"
#include <serval.h>
#include <serval/keyring.h>
#include <serval/overlay_address.h>
#include <serval/mdp_client.h>
#include <serval/crypto.h>
#include <serval/str.h>
#include <serval/overlay_packet.h>
#include <serval/overlay_buffer.h>
#include <serval/dataformats.h>

#include "obj.h"
#include "list.h"
#include "cmd.h"
#include "debug.h"
#include "crypto.h"
#include "tree.h"

#include "serval-dna.h"

/* libserval global for the primary keyring */
extern keyring_file *keyring;

/* 
 * libserval global identifying the subscriber corresponding
 * to the primary identity in the primary keyring
 */
extern struct subscriber *my_subscriber;

/* file path for the serval state/conf/keyring directory */
char *serval_path = NULL;
 
/* used to store error messages to pass in response to client commands */
co_obj_t *err_msg = NULL;

/* Private */

static int
serval_create_signature(svl_crypto_ctx *ctx)
{
  CHECK(ctx && ctx->sas_private[0] && ctx->msg && ctx->msg_len,"Invalid ctx");
  int sig_length = SIGNATURE_BYTES;
  unsigned char hash[crypto_hash_sha512_BYTES]; 
  
  // create sha512 hash of message, which will then be signed
  crypto_hash_sha512(hash, ctx->msg, ctx->msg_len);
  
  // create signature of message hash, append it to end of message
  int sig_ret = crypto_create_signature(ctx->sas_private,
					hash,
					crypto_hash_sha512_BYTES,
					ctx->signature,
					&sig_length);
  if (sig_ret != 0)
    return 0;
  return 1;
  
error:  
  return 0;
}

static int
serval_extract_sas(svl_crypto_ctx *ctx)
{
  CHECK(ctx,"Invalid ctx");
  struct subscriber *sub = find_subscriber(ctx->sid, SID_SIZE, 0);
  CHECK(sub && sub->identity, "Could not find subscriber associated with SID");
  // get SAS key associated with our SID
  struct keypair *pair = keyring_find_sas_private(ctx->keyring_file, sub->identity);
  CHECK(pair, "Failed to fetch SAS keys");
  CHECK(pair->private_key_len == crypto_sign_SECRETKEYBYTES &&
        pair->public_key_len == crypto_sign_PUBLICKEYBYTES, "Invalid SAS keys");
  memcpy(ctx->sas_private, pair->private_key, crypto_sign_SECRETKEYBYTES);
  memcpy(ctx->sas_public, pair->public_key, crypto_sign_PUBLICKEYBYTES);
  return 1;
error:
  return 0;
}

/* Used by sas_request */
static int
keyring_send_sas_request_client(struct subscriber *subscriber)
{
  int ret = 0, mdp_sockfd;
  sid_t srcsid;
  time_ms_t now = gettime_ms();
  
  CHECK((mdp_sockfd = overlay_mdp_client_socket()) >= 0,"Cannot create MDP socket");
  
  CHECK_ERR(overlay_mdp_getmyaddr(mdp_sockfd, 0, &srcsid) == 0, "Could not get local address");
  
  if (subscriber->sas_valid)
    return 1;
  
  CHECK_ERR(now >= subscriber->sas_last_request + 100, "Too soon to ask for SAS mapping again");
  
  CHECK_ERR(my_subscriber, "Couldn't request SAS (I don't know who I am)");
  
  DEBUG("Requesting SAS mapping for SID=%s", alloca_tohex(subscriber->sid.binary, SID_SIZE));
  
  int client_port = 32768 + (random() & 32767);
  CHECK_ERR(overlay_mdp_bind(mdp_sockfd, &my_subscriber->sid, client_port) == 0,
	    "Failed to bind to client socket");
  
  /* request mapping (send request auth-crypted). */
  struct internal_mdp_header header;
  bzero(&header, sizeof header);
  
  header.source = my_subscriber;
  header.destination = subscriber;
  header.source_port = MDP_PORT_KEYMAPREQUEST;
  header.destination_port = MDP_PORT_KEYMAPREQUEST;
  header.qos = OQ_MESH_MANAGEMENT;
  
  struct overlay_buffer *payload = ob_new();
  ob_append_byte(payload, KEYTYPE_CRYPTOSIGN);
  
  subscriber->sas_last_request=now;
  ob_flip(payload);
  int sent = overlay_send_frame(&header, payload);
  ob_free(payload);
  if (sent)
    DEBUG("Failed to send SAS resolution request: %d", sent);
  
  time_ms_t timeout = now + 5000;
  overlay_mdp_frame mdp;
  
  while(now < timeout) {
    time_ms_t timeout_ms = timeout - gettime_ms();
    int result = overlay_mdp_client_poll(mdp_sockfd, timeout_ms);
    
    if (result > 0) {
      int ttl = -1;
      if (overlay_mdp_recv(mdp_sockfd, &mdp, client_port, &ttl) == 0) {
	int found = 0;
	switch(mdp.packetTypeAndFlags & MDP_TYPE_MASK) {
	  case MDP_ERROR:
	    ERROR("overlay_mdp_recv: %s (code %d)", mdp.error.message, mdp.error.error);
	    break;
	  case MDP_TX:
	    DEBUG("Received SAS mapping response");
	    found = 1;
	    break;
	  default:
	    DEBUG("overlay_mdp_recv: Unexpected MDP frame type 0x%x", mdp.packetTypeAndFlags);
	    break;
	}
	if (found)
	  break;
      }
    }
    now = gettime_ms();
    if (servalShutdown)
      break;
  }
  
  unsigned keytype = mdp.out.payload[0];
  
  CHECK_ERR(keytype == KEYTYPE_CRYPTOSIGN, "Ignoring SID:SAS mapping with unsupported key type %u", keytype);
  
  CHECK_ERR(mdp.out.payload_length >= 1 + SAS_SIZE,
	    "Truncated key mapping announcement? payload_length: %d",
	    mdp.out.payload_length);
  
  unsigned char *plain = (unsigned char*)calloc(mdp.out.payload_length,sizeof(unsigned char));
  unsigned long long plain_len = 0;
  unsigned char *sas_public = &mdp.out.payload[1];
  unsigned char *compactsignature = &mdp.out.payload[SAS_SIZE + 1];
  
  /* reconstitute signed SID for verification */
  unsigned char signature[SID_SIZE + crypto_sign_edwards25519sha512batch_BYTES];
  memmove(&signature[0], &compactsignature[0], 64);
  memmove(&signature[64], &mdp.out.src.sid.binary[0], SID_SIZE);
  
  int sign_ret = crypto_sign_edwards25519sha512batch_open(plain,
							  &plain_len,
							  signature,
							  SID_SIZE + crypto_sign_edwards25519sha512batch_BYTES,
							  sas_public);
  CHECK_ERR(sign_ret == 0, "SID:SAS mapping verification signature does not verify");
  
  /* These next two tests should never be able to fail, but let's just check anyway. */
  CHECK_ERR(plain_len == SID_SIZE, "SID:SAS mapping signed block is wrong length");
  CHECK_ERR(memcmp(plain, mdp.out.src.sid.binary, SID_SIZE) == 0, "SID:SAS mapping signed block is for wrong SID");
  
  memmove(subscriber->sas_public, sas_public, SAS_SIZE);
  subscriber->sas_valid = 1;
  subscriber->sas_last_request = now;
  ret = 1;
  
error:
  if (plain)
    free(plain);
  overlay_mdp_client_close(mdp_sockfd);
  return ret;
}

/* Public */

svl_crypto_ctx *
svl_crypto_ctx_new(void)
{
  svl_crypto_ctx *ctx = h_calloc(1,sizeof(svl_crypto_ctx));
  CHECK_MEM(ctx);
  return ctx;
error:
  return NULL;
}

void
svl_crypto_ctx_free(svl_crypto_ctx *ctx)
{
  if (!ctx)
    return;
  if (ctx->keyring_file)
    keyring_free(ctx->keyring_file);
  h_free(ctx);
}

void
stowSid(unsigned char *packet, int ofs, const char *sid)
{
  if (strcasecmp(sid,"broadcast") == 0)
    memset(packet + ofs, 0xff, SID_SIZE);
  else 
    fromhex(packet + ofs, sid, SID_SIZE);
}

int
serval_open_keyring(svl_crypto_ctx *ctx)
{
  CHECK_ERR(ctx,"Invalid ctx");
  if (ctx->keyring_len == 0) {
    // if no keyring specified, use default keyring
    CHECK(serval_path,"Default Serval path not initialized");
    char keyring_path_str[PATH_MAX] = {0};
    strcpy(keyring_path_str, serval_path);
    if (serval_path[strlen(serval_path) - 1] != '/')
      strcat(keyring_path_str, "/");
    strcat(keyring_path_str, "serval.keyring");
    // Fetching SAS keys requires setting the SERVALINSTANCE_PATH environment variable
    CHECK_ERR(setenv("SERVALINSTANCE_PATH", serval_path, 1) == 0,
	      "Failed to set SERVALINSTANCE_PATH env variable");
    ctx->keyring_len = strlen(keyring_path_str);
    ctx->keyring_path = h_malloc(ctx->keyring_len + 1);
    strcpy(ctx->keyring_path,keyring_path_str);
    hattach(ctx->keyring_path,ctx);
  } else {
    // otherwise, use specified keyring (NOTE: if keyring does not exist, it will be created)
    CHECK(ctx->keyring_len < PATH_MAX, "Keyring length too long");
  }
  
  ctx->keyring_file = keyring_open(ctx->keyring_path, 1);
  CHECK_ERR(ctx->keyring_file, "Failed to open specified keyring file");

  if (keyring_enter_pin(ctx->keyring_file, KEYRING_PIN) <= 0) {
    // put initial identity in if we don't have any visible
    CHECK_ERR(keyring_seed(ctx->keyring_file) == 0, "Failed to seed keyring");
  }
  
  return 1;
error:
  return 0;
}

int
serval_init_keyring(svl_crypto_ctx *ctx)
{
  keyring_identity *new_ident;
  
  CHECK_ERR(ctx,"Invalid ctx");
  
  CHECK_ERR(serval_open_keyring(ctx), "Failed to open keyring");
  
  if (!ctx->sid[0]) { //create new sid
    // cycle through the keyring contexts until we find one with room for another identity
    for(int c = 0; c < (ctx->keyring_file)->context_count; c++) {
      // create new Serval identity
      new_ident = keyring_create_identity(ctx->keyring_file,
					  (ctx->keyring_file)->contexts[c],
					  KEYRING_PIN);
      if (new_ident)
	break;
    }
    CHECK_ERR(new_ident, "Failed to create new SID");
    
    // need to commit keyring or else new identity won't be saved (needs permissions)
    CHECK_ERR(keyring_commit(ctx->keyring_file) == 0, "Failed to save new SID into keyring");
    
    memcpy(ctx->sid,new_ident->subscriber->sid.binary,SID_SIZE);
  }
  
  CHECK(serval_extract_sas(ctx), "Failed to fetch SAS keys");
  
  return 1;
error:
  return 0;
}

int
cmd_serval_sign(svl_crypto_ctx *ctx)
{
  CHECK_ERR(ctx,"Invalid ctx");
  
  /* If a non-default keyring path is given, and/or no SID is set,
   * we need to initialize the keyring */
  if (ctx->keyring_path || !ctx->sid[0]) {
    CHECK_ERR(serval_init_keyring(ctx),
	      "Failed to initialize Serval keyring");
  } else {
    // or just use the default keyring
    ctx->keyring_file = keyring; // serval global
    CHECK_ERR(serval_extract_sas(ctx),
	      "Failed to fetch SAS keys");
  }
  
  CHECK_ERR(serval_create_signature(ctx),"Failed to create signature");
  
  return 1;
error:
  return 0;
}

int
cmd_serval_verify(svl_crypto_ctx *ctx)
{
  int verdict = 0;
  
  CHECK_ERR(ctx && ctx->msg && ctx->signature[0] && ctx->sas_public[0],"Invalid ctx");
  
  DEBUG("Message to verify:\n%s", ctx->msg);
  
  unsigned char hash[crypto_hash_sha512_BYTES];
  crypto_hash_sha512(hash, ctx->msg, ctx->msg_len);
  
  int sig_ret = crypto_verify_signature(ctx->sas_public,
					hash,
					crypto_hash_sha512_BYTES,
					ctx->signature,
					SIGNATURE_BYTES);
  if (sig_ret == 0)
    verdict = 1;  // successfully verified
    
error:
  return verdict;
}

/* Used by serval-client */
int
serval_verify_client(svl_crypto_ctx *ctx)
{
  CHECK(ctx && ctx->sid[0] && ctx->msg && ctx->signature[0] && ctx->keyring_path,
	"Invalid ctx");
  
  CHECK(serval_init_keyring(ctx), "Failed to initialize Serval keyring");
      
  struct subscriber *sub = find_subscriber(ctx->sid, SID_SIZE, 1); // get Serval identity described by given SID
  
  CHECK(sub, "Failed to fetch Serval subscriber");
  
  CHECK(keyring_send_sas_request_client(sub), "SAS request failed");
  
  CHECK(sub->sas_valid, "Could not validate the signing key!");
  CHECK(sub->sas_public[0], "Could not validate the signing key!");
  
  memcpy(ctx->sas_public,sub->sas_public,crypto_sign_PUBLICKEYBYTES);
  
  return cmd_serval_verify(ctx);
error:
  return 0;
}

int
serval_crypto_register(void)
{
  /** name: serval-crypto
   * param[0] - param[3]: (co_str?_t)
   */
  
  const char name[] = "serval-crypto",
  usage[] = "serval-crypto sign [<SID>] <MESSAGE> [--keyring=<KEYRING_PATH>]\n"
            "serval-crypto verify <SAS> <SIGNATURE> <MESSAGE>",
  desc[] =  "Serval-crypto utilizes Serval's crypto API to:\n"
	    "      * Sign any arbitrary text using a Serval key. If no Serval key ID (SID) is given,\n"
	    "             a new key will be created on the default Serval keyring.\n"
	    "      * Verify any arbitrary text, a signature, and a Serval signing key (SAS), and will\n"
	    "             determine if the signature is valid.";
  
  int reg_ret = co_cmd_register(name,
				sizeof(name),
				usage,
				sizeof(usage),
				desc,
				sizeof(desc),
				serval_crypto_handler);
  CHECK(reg_ret, "Failed to register commands");
  
  return 1;
error:
  return 0;
}

int
olsrd_mdp_register(void)
{
  /**
   * name: mdp-init
   * param[0] <required>: <keyring_path> (co_str16_t)
   * param[1] <required>: <SID> (co_str8_t)
   */
  const char name[] = "mdp-init";
  
  CHECK(co_cmd_register(name, sizeof(name), "", 1, "", 1, olsrd_mdp_init), "Failed to register command");
  
  return 1;
error:
  return 0;
}

int
olsrd_mdp_sign_register(void)
{
  /**
   * name: mdp-sign
   * param[0] <required>: key (co_bin8_t)
   * param[1] <required>: data (co_bin?_t)
   */
  
  const char name[] = "mdp-sign";
  
  CHECK(co_cmd_register(name, sizeof(name), "", 1, "", 1, olsrd_mdp_sign), "Failed to register command");
  
  return 1;
error:
  return 0;
}

int
serval_crypto_handler(co_obj_t *self, co_obj_t **output, co_obj_t *params)
{
  CLEAR_ERR();
  
  svl_crypto_ctx *ctx = NULL;
  int list_len = co_list_length(params);
  int keypath = 0;
  
  CHECK_ERR(IS_LIST(params) && list_len >= 2, "Invalid params");
  
  if (!strncmp("--keyring=", co_obj_data_ptr(co_list_get_last(params)), 10)) {
    keypath = 1;
    --list_len;
  }
  
  ctx = svl_crypto_ctx_new();
  
  if (co_str_cmp_str(co_list_element(params, 0), "sign") == 0) {
    
    CHECK_ERR(list_len == 2 || list_len == 3, "Invalid arguments");
    
    if (list_len == 3) {
      char *sid_str = _LIST_ELEMENT(params, 1);
      size_t sid_len = co_str_len(co_list_element(params, 1)) - 1;
      CHECK_ERR(sid_len == (2 * SID_SIZE) && str_is_subscriber_id(sid_str) == 1,
		"Invalid SID");
      stowSid(ctx->sid, 0, sid_str);
      ctx->msg = (unsigned char*)_LIST_ELEMENT(params, 2);
      ctx->msg_len = co_str_len(co_list_element(params, 2)) - 1;
      if (keypath) {
	ctx->keyring_path = _LIST_ELEMENT(params, 3) + 10;
	ctx->keyring_len = co_str_len(co_list_element(params, 3)) - 11;
	CHECK_ERR(ctx->keyring_len < PATH_MAX,"Keyring path too long");
      }
    
    } else if (list_len == 2) {
      
      ctx->msg = (unsigned char*)_LIST_ELEMENT(params, 1);
      ctx->msg_len = co_str_len(co_list_element(params, 1)) - 1;
      if (keypath) {
	ctx->keyring_path = _LIST_ELEMENT(params, 2) + 10;
	ctx->keyring_len = co_str_len(co_list_element(params, 2)) - 11;
	CHECK_ERR(ctx->keyring_len < PATH_MAX,"Keyring path too long");
      }

    }
    CHECK_ERR(cmd_serval_sign(ctx), "Failed to create signature");
    
    // convert ctx->signature, ctx->sas_public, and ctx->sid to hex: 
    char sid_str[(2 * SID_SIZE) + 1] = {0};
    strncpy(sid_str, alloca_tohex(ctx->sid, SID_SIZE), 2 * SID_SIZE);
    char sas_str[(2 * crypto_sign_PUBLICKEYBYTES) + 1] = {0};
    strncpy(sas_str, alloca_tohex(ctx->sas_public, crypto_sign_PUBLICKEYBYTES), 2 * crypto_sign_PUBLICKEYBYTES);
    char sig_str[(2 * SIGNATURE_BYTES) + 1] = {0};
    strncpy(sig_str, alloca_tohex(ctx->signature, SIGNATURE_BYTES), 2 * SIGNATURE_BYTES);
    CMD_OUTPUT("SID", co_str8_create(sid_str, (2 * SID_SIZE) + 1, 0));
    CMD_OUTPUT("SAS", co_str8_create(sas_str, (2 * crypto_sign_PUBLICKEYBYTES) + 1, 0));
    CMD_OUTPUT("signature", co_str8_create(sig_str, (2 * SIGNATURE_BYTES) + 1, 0));
    
  } else if (co_str_cmp_str(co_list_element(params, 0), "verify") == 0) {
    
    CHECK_ERR(!keypath, "Keyring option not available for verification");
    CHECK_ERR(list_len == 4, "Invalid arguments");
    // convert SAS and signature from hex to bin
    CHECK_ERR(fromhexstr(ctx->signature, _LIST_ELEMENT(params, 2), SIGNATURE_BYTES) == 0, "Invalid signature");
    CHECK_ERR(fromhexstr(ctx->sas_public, _LIST_ELEMENT(params, 1), crypto_sign_PUBLICKEYBYTES) == 0, "Invalid SAS key");
    ctx->msg = (unsigned char*)_LIST_ELEMENT(params, 3);
    ctx->msg_len = co_str_len(co_list_element(params, 3)) - 1;

    int verdict = cmd_serval_verify(ctx);
    if (verdict == 1) {
      DEBUG("signature verified");
      CMD_OUTPUT("result", co_bool_create(true, 0));  // successfully verified
      CMD_OUTPUT("verified",co_str8_create("true",sizeof("true"),0));
    } else if (verdict == 0) {
      DEBUG("signature NOT verified");
      CMD_OUTPUT("result", co_bool_create(false, 0));
      CMD_OUTPUT("verified",co_str8_create("false",sizeof("false"),0));
    }
    
  }
  
  return 1;
error:
  INS_ERROR();
  if (ctx)
    svl_crypto_ctx_free(ctx);
  return 0;
}

int
olsrd_mdp_init(co_obj_t *self, co_obj_t **output, co_obj_t *params)
{
  svl_crypto_ctx *ctx = NULL;
  CHECK(IS_LIST(params) && co_list_length(params) == 2, "Invalid params");
  
  size_t sid_len = co_str_len(co_list_element(params, 1));
  char *sid_str = _LIST_ELEMENT(params, 1);
  
  CHECK(sid_len == (2 * SID_SIZE) + 1 && str_is_subscriber_id(sid_str) == 1, "Invalid SID");
  
  ctx = svl_crypto_ctx_new();
  
  stowSid(ctx->sid, 0, sid_str);
  
  ctx->keyring_path = _LIST_ELEMENT(params, 0);
  ctx->keyring_len = co_str_len(co_list_element(params, 0)) - 1;
  CHECK_ERR(ctx->keyring_len < PATH_MAX,"Keyring path too long");
  
  CHECK(serval_init_keyring(ctx), "Failed to initialize Serval keyring");
  
  CMD_OUTPUT("key", co_bin8_create((char*)ctx->sas_private, crypto_sign_SECRETKEYBYTES, 0));
  
  return 1;
error:
  if (ctx)
    svl_crypto_ctx_free(ctx);
  return 0;
}

int
olsrd_mdp_sign(co_obj_t *self, co_obj_t **output, co_obj_t *params)
{
  int ret = 0;
  svl_crypto_ctx *ctx = svl_crypto_ctx_new();
  
  /** skipping some error checking for performance reasons */
  
//   CHECK(IS_LIST(params) && co_list_length(params) == 2, "Invalid params");
  
  ctx->msg_len = co_obj_data((char**)&ctx->msg, co_list_element(params, 1));
  
  memcpy(ctx->sas_private,_LIST_ELEMENT(params, 0),crypto_sign_SECRETKEYBYTES);
  
  CHECK(serval_create_signature(ctx), "Failed to sign OLSRd packet");
  
  CMD_OUTPUT("sig", co_bin8_create((char*)ctx->signature, SIGNATURE_BYTES, 0));
  
  ret = 1;
error:
  svl_crypto_ctx_free(ctx);
  return ret;
}
