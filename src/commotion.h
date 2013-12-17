/* vim: set ts=2 expandtab: */
/**
 *       @file  commotion.h
 *      @brief  Client API for the Commotion Daemon
 *
 *     @author  Josh King (jheretic), jking@chambana.net
 *
 *   @internal
 *      Created  12/16/2013 12:00:25 PM
 *     Compiler  gcc/g++
 * Organization  The Open Technology Institute
 *    Copyright  Copyright (c) 2013, Josh King
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

#ifndef _CONNECT_H
#define _CONNECT_H

#ifndef CO_OBJ_INTERNAL
typedef void co_obj_t; 
#endif

int co_init(void);

int co_shutdown(void);

co_obj_t *co_connect(const char *uri, const size_t ulen);

int co_disconnect(co_obj_t *connection);

co_obj_t *co_request_create(void);

int co_request_append(co_obj_t *request, co_obj_t *object);

int co_request_append_str(co_obj_t *request, const char *s, const size_t slen);

int co_request_append_int(co_obj_t *request, const int i);

int co_request_append_uint(co_obj_t *request, const unsigned int i);

int co_call(co_obj_t *connection, co_obj_t **response, const char *method, const size_t mlen, co_obj_t *request);

co_obj_t *co_response_get(co_obj_t *response, const char *key, const size_t klen);

size_t co_response_get_str(co_obj_t *response, char **output, const char *key, const size_t klen); 

int co_response_get_uint(co_obj_t *response, unsigned long *output, const char *key, const size_t klen);

int co_response_get_int(co_obj_t *response, signed long *output, const char *key, const size_t klen); 

int co_response_print(co_obj_t *response);

void co_free(co_obj_t *object);
#endif
