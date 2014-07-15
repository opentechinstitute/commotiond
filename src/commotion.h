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
 * it under the terms of the GNU Affero General Public License as
 * published by the Free Software Foundation, either version 3 of the
 * License, or (at your option) any later version.
 *
 * Commotion is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU Affero General Public License for more details.
 *
 * You should have received a copy of the GNU Affero General Public License
 * along with Commotion.  If not, see <http://www.gnu.org/licenses/>.
 *
 * =====================================================================================
 */

#ifndef _COMMOTION_H
#define _COMMOTION_H

#include <stdbool.h>

#ifndef CO_OBJ_INTERNAL
typedef void co_obj_t;
#endif

/**
 * @brief initializes API
 */
extern int co_init(void);

/**
 * @brief shuts down the API
 */
extern int co_shutdown(void);

/**
 * @brief creates a connection to Commotion daemon at the given URI
 * @param uri URI string
 * @param ulen length of URI string
 */
extern co_obj_t *co_connect(const char *uri, const size_t ulen);

/**
 * @brief closes connection to Commotion daemon
 * @param connection context object for active connection
 */
extern int co_disconnect(co_obj_t *connection);

/**
 * @brief create an API request
 */
extern co_obj_t *co_request_create(void);

/**
 * @brief appends object to request
 * @param request request object to append to
 * @param object object to append
 */
extern int co_request_append(co_obj_t *request, co_obj_t *object);

/**
 * @brief appends string to request
 * @param request request object to append to
 * @param s string to append
 * @param slen length of string to append
 */
extern int co_request_append_str(co_obj_t *request, const char *s, const size_t slen);

/**
 * @brief appends byte array to request
 * @param request request object to append to
 * @param s array to append
 * @param slen length of array to append
 */
extern int co_request_append_bin(co_obj_t *request, const char *s, const size_t slen);

/**
 * @brief appends int to request
 * @param request request object to append to
 * @param i integer to append
 */
extern int co_request_append_int(co_obj_t *request, const int i);

/**
 * @brief appends unsigned int to request
 * @param request request object to append to
 * @param i integer to append
 */
extern int co_request_append_uint(co_obj_t *request, const unsigned int i);

/**
 * @brief sense procedure call to daemon
 * @param connection context object for connection
 * @param response pointer to response buffer
 * @param method method name
 * @param mlen length of method name
 * @param request request object to send
 */
extern int co_call(co_obj_t *connection, co_obj_t **response, const char *method, const size_t mlen, co_obj_t *request);

/**
 * @brief retrieve object from response
 * @param response pointer to response object
 * @param key identifier for response element to retrieve
 * @param klen length of key name
 */
extern co_obj_t *co_response_get(co_obj_t *response, const char *key, const size_t klen);

/**
 * @brief retrieve string from response
 * @param response pointer to response object
 * @param output pointer to output buffer
 * @param key identifier for response element to retrieve
 * @param klen length of key name
 */
extern size_t co_response_get_str(co_obj_t *response, char **output, const char *key, const size_t klen);

/**
 * @brief retrieve byte array from response
 * @param response pointer to response object
 * @param output pointer to output buffer
 * @param key identifier for response element to retrieve
 * @param klen length of key name
 */
extern size_t co_response_get_bin(co_obj_t *response, char **output, const char *key, const size_t klen);

/**
 * @brief retrieve unsigned int from response
 * @param response pointer to response object
 * @param output pointer to output buffer
 * @param key identifier for response element to retrieve
 * @param klen length of key name
 */
extern int co_response_get_uint(co_obj_t *response, unsigned long *output, const char *key, const size_t klen);

/**
 * @brief retrieve signed int from response
 * @param response pointer to response object
 * @param output pointer to output buffer
 * @param key identifier for response element to retrieve
 * @param klen length of key name
 */
extern int co_response_get_int(co_obj_t *response, signed long *output, const char *key, const size_t klen);

/**
 * @brief retrieve bool from response
 * @param response pointer to response object
 * @param output pointer to output buffer
 * @param key identifier for response element to retrieve
 * @param klen length of key name
 */
extern int co_response_get_bool(co_obj_t *response, bool *output, const char *key, const size_t klen);

/**
 * @brief print response object
 * @param response pointer to response object
 */
extern int co_response_print(co_obj_t *response);

/**
 * @brief free API objects
 * @param object object to free
 */
extern void co_free(co_obj_t *object);

#endif
