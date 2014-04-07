/**
 *       @file  keyring.c
 *      @brief  functions for managing serval-dna keyrings
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

#include "serval-config.h"
#include <serval.h>
#include <serval/os.h>
#include <serval/overlay_address.h>

#include "obj.h"
#include "list.h"

#include "serval-dna.h"
#include "crypto.h"
#include "keyring.h"

typedef struct svl_keyring {
  keyring_file *keyring_file;
  char *keyring_path;
  size_t refs;
  svl_keyring_update update;
  svl_keyring *_next;
} svl_keyring;

extern char *serval_path;

svl_keyring *keyring_list = NULL;

/* Private */

static svl_keyring *
_svl_keyring_new(void)
{
  svl_keyring *keyring = h_calloc(1,sizeof(svl_keyring));
  CHECK_MEM(keyring);
  keyring->refs = 1;
error:
  return keyring;
}

static void
_svl_keyring_free(svl_keyring *keyring)
{
  if (keyring)
    h_free(keyring);
}

static svl_keyring *
_svl_keyring_find(const char *keyring_path)
{
  svl_keyring *keyring = keyring_list;
  for (; keyring; keyring = keyring->_next) {
    if (strcasecmp(keyring->keyring_path, keyring_path) == 0)
      break;
  }
  return keyring;
}

static int
_svl_keyring_add(svl_keyring **list, svl_keyring *keyring)
{
  CHECK(keyring, "Invalid keyring");
  keyring->_next = *list;
  *list = keyring;
  
  return 1;
error:
  return 0;
}

static int
_svl_keyring_open(svl_keyring *keyring)
{
  CHECK(keyring, "Invalid keyring");
  
  keyring->keyring_file = keyring_open(keyring->keyring_path, 1);
  CHECK(keyring->keyring_file, "Failed to open specified keyring file");
  
  if (keyring_enter_pin(keyring->keyring_file, KEYRING_PIN) <= 0) {
    // put initial identity in if we don't have any visible
    CHECK(keyring_seed(keyring->keyring_file) == 0, "Failed to seed keyring");
  }
  
  return 1;
error:
  return 0;
}

/* Public */

void
serval_close_keyring(svl_crypto_ctx *ctx)
{
  if (ctx && ctx->keyring) {
    ctx->keyring->refs--;
    if (ctx->keyring->refs <= 0) {
      keyring_free(ctx->keyring->keyring_file);
      h_free(ctx->keyring);
    }
    ctx->keyring = NULL;
  }
}

int
serval_open_keyring(svl_crypto_ctx *ctx, svl_keyring_update update)
{
  svl_keyring *kr = NULL;
  
  CHECK(ctx,"Invalid ctx");
  
  if (ctx->keyring_len == 0) {
    // if no keyring specified, use default keyring
    CHECK(serval_path,"Default Serval path not initialized");
    char keyring_path_str[PATH_MAX] = {0};
    strcpy(keyring_path_str, serval_path);
    if (serval_path[strlen(serval_path) - 1] != '/')
      strcat(keyring_path_str, "/");
    strcat(keyring_path_str, "serval.keyring");
    // Fetching SAS keys requires setting the SERVALINSTANCE_PATH environment variable
    CHECK(setenv("SERVALINSTANCE_PATH", serval_path, 1) == 0,
	      "Failed to set SERVALINSTANCE_PATH env variable");
    ctx->keyring_len = strlen(keyring_path_str);
    ctx->keyring_path = h_malloc(ctx->keyring_len + 1);
    strcpy(ctx->keyring_path,keyring_path_str);
    hattach(ctx->keyring_path,ctx);
  } else {
    // otherwise, use specified keyring (NOTE: if keyring does not exist, it will be created)
    CHECK(ctx->keyring_len > 0 && ctx->keyring_len < PATH_MAX, "Invalid keyring length");
  }
  
  if ((kr = _svl_keyring_find(ctx->keyring_path))) {
    ctx->keyring = kr;
    kr->refs++;
  } else {
    // create new keyring and open it
    kr = _svl_keyring_new();
    CHECK_MEM(kr);
    kr->update = (update) ? update : serval_reload_keyring;
    kr->keyring_path = h_strdup(ctx->keyring_path);
    hattach(kr->keyring_path, kr);
    
    CHECK(_svl_keyring_open(kr), "Failed to open keyring");
    
    ctx->keyring = kr;
    
    // add to master keyring list
    CHECK(_svl_keyring_add(&keyring_list, kr), "Failed to add new keyring to list");
  }
  
  return 1;
error:
  if (kr)
    _svl_keyring_free(kr);
  return 0;
}

int
serval_keyring_add_identity(svl_crypto_ctx *ctx)
{
  keyring_identity *new_ident = NULL;
  CHECK(ctx && ctx->keyring,"Invalid ctx");
  
  // cycle through the keyring contexts until we find one with room for another identity
  for(int i = 0; i < (ctx->keyring->keyring_file)->context_count; i++) {
    // create new Serval identity
    new_ident = keyring_create_identity(ctx->keyring->keyring_file,
				        ctx->keyring->keyring_file->contexts[i],
					KEYRING_PIN);
    if (new_ident)
      break;
  }
  
  CHECK(new_ident, "Failed to create new SID");
  
  // Extract SAS keys
  keypair *kp = keyring_find_sas_private(ctx->keyring->keyring_file, new_ident);
  CHECK(kp, "Failed to get SAS keys from new identity");
  memcpy(ctx->sas_private, kp->private_key, crypto_sign_SECRETKEYBYTES);
  memcpy(ctx->sas_public, kp->public_key, crypto_sign_PUBLICKEYBYTES);
  
  // Extract SID
  sid_t *sid = NULL;
  keyring_identity_extract(new_ident, (const sid_t**)&sid, NULL, NULL);
  memcpy(ctx->sid,sid->binary,SID_SIZE);
  
  // need to commit keyring or else new identity won't be saved (needs permissions)
  CHECK(keyring_commit(ctx->keyring->keyring_file) == 0, "Failed to save new SID into keyring");
  
  CHECK(ctx->keyring->update(ctx), "Failed to update keyring");
  
  return 1;
error:
  return 0;
}

keyring_file *
serval_get_keyring_file(svl_crypto_ctx *ctx)
{
  keyring_file *krf = NULL;
  if (ctx && ctx->keyring)
    krf = ctx->keyring->keyring_file;
  return krf;
}

/** Requires use of serval-dna global keyring */
int
serval_extract_sas(svl_crypto_ctx *ctx)
{
  CHECK(ctx,"Invalid ctx");
  
  unsigned int cn=0, in=0, kp=0;
  int ret = keyring_find_sid(ctx->keyring->keyring_file, &cn, &in, &kp, (sid_t*)ctx->sid);
  CHECK(ret != 0, "Failed to find identity in keyring associated with SID");
  
  // get SAS key associated with our SID
  kp = keyring_identity_find_keytype(ctx->keyring->keyring_file, cn, in, KEYTYPE_CRYPTOSIGN);
  CHECK(kp != -1, "Failed to extract SAS from keyring associated with SID");
  
  struct keypair *pair = ctx->keyring->keyring_file->contexts[cn]->identities[in]->keypairs[kp];
  CHECK(pair, "Failed to fetch SAS keys");
  CHECK(pair->private_key_len == crypto_sign_SECRETKEYBYTES
	&& pair->public_key_len == crypto_sign_PUBLICKEYBYTES,
	"Invalid SAS keys");
  
  memcpy(ctx->sas_private, pair->private_key, crypto_sign_SECRETKEYBYTES);
  memcpy(ctx->sas_public, pair->public_key, crypto_sign_PUBLICKEYBYTES);
  
  return 1;
error:
  return 0;
}

int
serval_reload_keyring(svl_crypto_ctx *ctx)
{
  CHECK(ctx && ctx->keyring && ctx->keyring->keyring_file, "Invalid ctx");
  
  keyring_free(ctx->keyring->keyring_file);
  ctx->keyring->keyring_file = NULL;
  CHECK(_svl_keyring_open(ctx->keyring), "Failed to open keyring");
  
  return 1;
error:
  return 0;
}