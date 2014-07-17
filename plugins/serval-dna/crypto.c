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

#include "serval-config.h"
#include <serval.h>
#include <serval/crypto.h>
#include <serval/overlay_address.h>
#include <serval/str.h>
#include <serval/dataformats.h>

#include "obj.h"
#include "list.h"
#include "cmd.h"
#include "debug.h"
#include "tree.h"

#include "serval-dna.h"
#include "keyring.h"
#include "crypto.h"

/* libserval global for the primary keyring */
extern keyring_file *keyring;

/* file path for the serval state/conf/keyring directory */
char *serval_path = NULL;

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
  if (ctx->keyring_file && ctx->keyring_file != keyring
    serval_close_keyring(ctx);
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

inline int
iszero(unsigned char *buf, size_t len) {
  for (int i = 0; i < len; i++) {
    if (buf[i])
      return 0;
  }
  return 1;
}

int
cmd_serval_sign(svl_crypto_ctx *ctx)
{
  CHECK(ctx && ctx->keyring && ctx->msg && ctx->msg_len,"Invalid ctx");
  
  /* If no SID is set, we need to add an identity */
  if (iszero(ctx->sid, SID_SIZE))
    CHECK(serval_keyring_add_identity(ctx), "Failed to add Serval identity");
  
  if (iszero(ctx->sas_public,crypto_sign_PUBLICKEYBYTES) || iszero(ctx->sas_private,crypto_sign_SECRETKEYBYTES))
    CHECK(serval_extract_sas(ctx), "Failed to fetch SAS keys");
  
  CHECK(serval_create_signature(ctx),"Failed to create signature");
  
  return 1;
error:
  return 0;
}

int
cmd_serval_verify(svl_crypto_ctx *ctx)
{
  int verdict = 0;
  
  CHECK(ctx
	    && ctx->msg
	    && !iszero(ctx->signature,SIGNATURE_BYTES)
	    && !iszero(ctx->sas_public,crypto_sign_PUBLICKEYBYTES),
	    "Invalid ctx");
  
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

int
serval_crypto_handler(co_obj_t *self, co_obj_t **output, co_obj_t *params)
{
  CLEAR_ERR();
  
  co_obj_t *out = NULL;
  svl_crypto_ctx *ctx = NULL;
  int list_len = co_list_length(params);
  int keypath = 0;
  
  CHECK(IS_LIST(params) && list_len >= 2, "Invalid params");
  
  if (!strncmp("keyring=", co_obj_data_ptr(co_list_get_last(params)), strlen("keyring="))) {
    keypath = 1;
    --list_len;
  }
  
  ctx = svl_crypto_ctx_new();
  CHECK_MEM(ctx);
  
  if (co_str_cmp_str(co_list_element(params, 0), "sign") == 0) {
    
    CHECK(list_len == 2 || list_len == 3, "Invalid arguments");
    
    if (list_len == 3) { // SID specified
      
      char *sid_str = _LIST_ELEMENT(params, 1);
      size_t sid_len = co_str_len(co_list_element(params, 1)) - 1;
      CHECK(sid_len == (2 * SID_SIZE) && str_is_subscriber_id(sid_str) == 1,
		"Invalid SID");
      stowSid(ctx->sid, 0, sid_str);
      ctx->msg = (unsigned char*)_LIST_ELEMENT(params, 2);
      ctx->msg_len = co_str_len(co_list_element(params, 2)) - 1;
      if (keypath) {
	ctx->keyring_path = _LIST_ELEMENT(params, 3) + strlen("keyring=");
	ctx->keyring_len = co_str_len(co_list_element(params, 3)) - (strlen("keyring=") + 1);
	CHECK(ctx->keyring_len < PATH_MAX,"Keyring path too long");
	// TODO make sure keyring path is an existing file
      }
      // if ctx->keyring_path is not set, opens default keyring
      CHECK(serval_open_keyring(ctx, NULL), "Failed to open keyring");
    
    } else if (list_len == 2) { // no SID specified, one will be created
      
      ctx->msg = (unsigned char*)_LIST_ELEMENT(params, 1);
      ctx->msg_len = co_str_len(co_list_element(params, 1)) - 1;
      if (keypath) {
	ctx->keyring_path = _LIST_ELEMENT(params, 2) + strlen("keyring=");
	ctx->keyring_len = co_str_len(co_list_element(params, 2)) - (strlen("keyring=") + 1);
	CHECK(ctx->keyring_len < PATH_MAX,"Keyring path too long");
	// TODO make sure keyring path is an existing file
      }
      // if ctx->keyring_path is not set, opens default keyring
      CHECK(serval_open_keyring(ctx, NULL), "Failed to open keyring");

    }
    CHECK(cmd_serval_sign(ctx), "Failed to create signature");
    
    // convert ctx->signature, ctx->sas_public, and ctx->sid to hex: 
    char sid_str[(2 * SID_SIZE) + 1] = {0};
    strncpy(sid_str, alloca_tohex(ctx->sid, SID_SIZE), 2 * SID_SIZE);
    char sas_str[(2 * crypto_sign_PUBLICKEYBYTES) + 1] = {0};
    strncpy(sas_str, alloca_tohex(ctx->sas_public, crypto_sign_PUBLICKEYBYTES), 2 * crypto_sign_PUBLICKEYBYTES);
    char sig_str[(2 * SIGNATURE_BYTES) + 1] = {0};
    strncpy(sig_str, alloca_tohex(ctx->signature, SIGNATURE_BYTES), 2 * SIGNATURE_BYTES);
    out = co_str8_create(sid_str, (2 * SID_SIZE) + 1, 0);
    CHECK_MEM(out);
    CMD_OUTPUT("SID", out);
    out = co_str8_create(sas_str, (2 * crypto_sign_PUBLICKEYBYTES) + 1, 0);
    CHECK_MEM(out);
    CMD_OUTPUT("SAS", out);
    out = co_str8_create(sig_str, (2 * SIGNATURE_BYTES) + 1, 0);
    CHECK_MEM(out);
    CMD_OUTPUT("signature", out);
    
  } else if (co_str_cmp_str(co_list_element(params, 0), "verify") == 0) {
    
    CHECK(!keypath, "Keyring option not available for verification");
    CHECK(list_len == 4, "Invalid arguments");
    // convert SAS and signature from hex to bin
    CHECK(fromhexstr(ctx->signature, _LIST_ELEMENT(params, 2), SIGNATURE_BYTES) == 0, "Invalid signature");
    CHECK(fromhexstr(ctx->sas_public, _LIST_ELEMENT(params, 1), crypto_sign_PUBLICKEYBYTES) == 0, "Invalid SAS key");
    ctx->msg = (unsigned char*)_LIST_ELEMENT(params, 3);
    ctx->msg_len = co_str_len(co_list_element(params, 3)) - 1;

    int verdict = cmd_serval_verify(ctx);
    if (verdict == 1) {
      DEBUG("signature verified");
      out = co_bool_create(true, 0);
      CHECK_MEM(out);
      CMD_OUTPUT("result", out);  // successfully verified
      out = co_str8_create("true",sizeof("true"),0);
      CHECK_MEM(out);
      CMD_OUTPUT("verified", out);
    } else if (verdict == 0) {
      DEBUG("signature NOT verified");
      out = co_bool_create(false, 0);
      CHECK_MEM(out);
      CMD_OUTPUT("result", out);
      out = co_str8_create("false",sizeof("false"),0);
      CHECK_MEM(out);
      CMD_OUTPUT("verified", out);
    }
    
  }
  
error:
  INS_ERROR();
  if (ctx)
    svl_crypto_ctx_free(ctx);
  return 1;
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
}

int
serval_create_signature(svl_crypto_ctx *ctx)
{
  CHECK(ctx
	&& !iszero(ctx->sas_private,crypto_sign_SECRETKEYBYTES)
	&& ctx->msg
	&& ctx->msg_len,
	"Invalid ctx");
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
