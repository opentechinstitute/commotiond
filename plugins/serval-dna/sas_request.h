/**
 *       @file  sas_request.h
 *      @brief  library for fetching a Serval SAS key over the MDP
 *                overlay network
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

#ifndef __CO_SERVAL_SAS_REQUEST_H
#define __CO_SERVAL_SAS_REQUEST_H

#define SID_SIZE 32
#define SAS_SIZE 32

int keyring_send_sas_request_client(const char *sid_str, 
				    const size_t sid_len,
				    char *sas_buf,
				    const size_t sas_buf_len);

#endif