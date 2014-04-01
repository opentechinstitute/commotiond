/**
 *       @file  crypto.h
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
 * =====================================================================================
 */

#ifndef __CO_SERVAL_CRYPTO_H
#define __CO_SERVAL_CRYPTO_H

#include <stdlib.h>
#include <stdint.h>

#include "config.h"
#include <serval.h>
#include <serval/keyring.h>
#include <serval/crypto.h>

#include "obj.h"

#define BUF_SIZE 1024
#define crypto_sign_PUBLICKEYBYTES crypto_sign_edwards25519sha512batch_PUBLICKEYBYTES
#define crypto_sign_SECRETKEYBYTES crypto_sign_edwards25519sha512batch_SECRETKEYBYTES
#define SIGNATURE_BYTES crypto_sign_edwards25519sha512batch_BYTES

typedef struct svl_keyring svl_keyring;

typedef struct svl_crypto_ctx {
  char *keyring_path;
  size_t keyring_len;
  svl_keyring *keyring;
  unsigned char sid[SID_SIZE]; // SID public key
  unsigned char sas_public[crypto_sign_PUBLICKEYBYTES]; // SAS public key
  unsigned char sas_private[crypto_sign_SECRETKEYBYTES]; // SAS private key
  unsigned char signature[SIGNATURE_BYTES];
  unsigned char *msg; // byte array used to create signature
  size_t msg_len;
} svl_crypto_ctx;

svl_crypto_ctx *svl_crypto_ctx_new(void);
void svl_crypto_ctx_free(svl_crypto_ctx *ctx);

void stowSid(unsigned char *packet, int ofs, const char *sid);

int serval_crypto_handler(co_obj_t *self, co_obj_t **output, co_obj_t *params);
int cmd_serval_sign(svl_crypto_ctx *ctx);
int cmd_serval_verify(svl_crypto_ctx *ctx);
int serval_create_signature(svl_crypto_ctx *ctx);

#endif