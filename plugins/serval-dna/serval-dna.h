/**
 *       @file  serval-dna.h
 *      @brief  re-implementation of the serval-dna daemon as a
 *                commotiond plugin
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

#ifndef __CO_SERVAL_DNA_H
#define __CO_SERVAL_DNA_H

#include <stdarg.h>

#include "list.h"

#ifdef SVL_DAEMON

extern int collect_errors;
extern co_obj_t *err_msg;

#undef CHECK
#define CHECK(A, M, ...) if(!(A)) { \
    ERROR(M, ##__VA_ARGS__); \
    if (collect_errors) { \
      if (err_msg == NULL) \
	err_msg = co_list16_create(); \
      char *msg = NULL; \
      int len = snprintf(NULL, 0, M, ##__VA_ARGS__); \
      msg = calloc(len,sizeof(char)); \
      sprintf(msg, M, ##__VA_ARGS__); \
      if (len < UINT8_MAX) { \
	co_list_append(err_msg, co_str8_create(msg,len+1,0)); \
      } else if (len < UINT16_MAX) { \
	co_list_append(err_msg, co_str16_create(msg,len+1,0)); \
      } else if (len < UINT32_MAX) { \
	co_list_append(err_msg, co_str32_create(msg,len+1,0)); \
      } \
      free(msg); \
    } \
    errno=0; goto error; \
  }

#define CLEAR_ERR() if (err_msg) { co_obj_free(err_msg); err_msg = NULL; } collect_errors = 1;
#define INS_ERROR() if (err_msg) { CMD_OUTPUT("errors",err_msg); } collect_errors = 0;

#else

#define CLEAR_ERR()
#define INS_ERROR()

#endif

int co_plugin_register(co_obj_t *self, co_obj_t **output, co_obj_t *params);
int co_plugin_init(co_obj_t *self, co_obj_t **output, co_obj_t *params);
int co_plugin_name(co_obj_t *self, co_obj_t **output, co_obj_t *params);
int co_plugin_shutdown(co_obj_t *self, co_obj_t **output, co_obj_t *params);
int serval_socket_cb(co_obj_t *self, co_obj_t *context);
int serval_timer_cb(co_obj_t *self, co_obj_t **output, co_obj_t *context);
int serval_schema(co_obj_t *self, co_obj_t **output, co_obj_t *params);
int serval_daemon_handler(co_obj_t *self, co_obj_t **output, co_obj_t *params);

#endif