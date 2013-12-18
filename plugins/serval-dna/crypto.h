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

#include <serval.h>

#include "obj.h"

#define KEYRING_PIN NULL
#define BUF_SIZE 1024

int serval_crypto_register(void);

int olsrd_mdp_register(void);

int olsrd_mdp_sign_register(void);

int serval_crypto_handler(co_obj_t *self, co_obj_t **output, co_obj_t *params);

int olsrd_mdp_init(co_obj_t *self, co_obj_t **output, co_obj_t *params);

int olsrd_mdp_sign(co_obj_t *self, co_obj_t **output, co_obj_t *params);

int serval_open_keyring(const char *keyring_path,
			const size_t keyring_len,
			keyring_file **_keyring);

int serval_init_keyring(unsigned char *sid,
		 const size_t sid_len,
		 const char *keyring_path,
		 const size_t keyring_len,
		 keyring_file **_keyring,
		 unsigned char **key,
		 int *key_len);

int cmd_serval_sign(const char *sid_str, 
		    const size_t sid_len,
		    const unsigned char *msg,
		    const size_t msg_len,
		    char *sig_str_buf,
		    const size_t sig_str_size,
		    const char *keyring_path,
		    const size_t keyring_len);

int cmd_serval_verify(const char *sas_key,
		   const size_t sas_key_len,
		   const unsigned char *msg,
		   const size_t msg_len,
		   const char *sig,
		   const size_t sig_len);

int serval_verify_client(const char *sid_str,
		  const size_t sid_len,
		  const unsigned char *msg,
		  const size_t msg_len,
		  const char *sig,
		  const size_t sig_len,
		  const char *keyring_path,
		  const size_t keyring_len);

#endif