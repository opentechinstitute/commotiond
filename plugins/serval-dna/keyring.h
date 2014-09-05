/**
 *       @file  keyring.h
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

#ifndef __CO_SERVAL_KEYRING_H
#define __CO_SERVAL_KEYRING_H

#include "crypto.h"

#define KEYRING_PIN NULL

typedef int (*svl_keyring_update)(svl_crypto_ctx *);

int serval_open_keyring(svl_crypto_ctx *ctx, svl_keyring_update update);
void serval_close_keyring(svl_crypto_ctx *ctx);
int serval_reload_keyring(svl_crypto_ctx *ctx);
int serval_keyring_add_identity(svl_crypto_ctx *ctx);
keyring_file *serval_get_keyring_file(svl_crypto_ctx *ctx);
int serval_extract_sas(svl_crypto_ctx *ctx);

#endif