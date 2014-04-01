/**
 *       @file  olsr_mdp.h
 *      @brief  command handlers for mdp plugin for olsrd
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

#include "config.h"
#include "serval.h"
#include <serval/os.h>
#include <serval/dataformats.h>

#include "obj.h"
#include "list.h"
#include "cmd.h"
#include "tree.h"

#include "serval-dna.h"
#include "crypto.h"
#include "keyring.h"
#include "olsr_mdp.h"

extern co_obj_t *err_msg;

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
  
  CHECK(serval_open_keyring(ctx, NULL), "Failed to initialize Serval keyring");
  
  // TODO serval_extract_sas requires use of serval-dna global keyring, which we can't use here. need another way to extract sas
  CHECK(serval_extract_sas(ctx), "Failed to fetch SAS keys");
  
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
