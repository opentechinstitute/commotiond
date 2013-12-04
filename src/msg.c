/* vim: set ts=2 expandtab: */
/**
 *       @file  msg.c
 *      @brief  a simple message serialization library
 *
 *     @author  Josh King (jheretic), jking@chambana.net
 *
 *   @internal
 *     Created  03/07/2013
 *    Revision  $Id: doxygen.commotion.templates,v 0.1 2013/01/01 09:00:00 jheretic Exp $
 *    Compiler  gcc/g++
 *     Company  The Open Technology Institute
 *   Copyright  Copyright (c) 2013, Josh King
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

#include <stdlib.h>
#include <stddef.h>
#include <stdbool.h>
#include <string.h>
#include <inttypes.h>
#include <arpa/inet.h>
#include "debug.h"
#include "util.h"
#include "msg.h"
#include "obj.h"
#include "list.h"

static uint32_t _id = 0;

static struct 
{
  uint8_t list_type;
  uint8_t list_len;
  uint8_t type_type;
  uint8_t type_value;
  uint8_t id_type;
} _req_header = {
  .list_type = _list16,
  .list_len = 4,
  .type_type = _uint8,
  .type_value = 0,
  .id_type = _uint32
};

size_t
co_request_alloc(char *output, const size_t olen, const co_obj_t *method, co_obj_t *param)
{
  CHECK(((output != NULL) && (method != NULL) && (param != NULL)), "Invalid request components.");
  CHECK(olen > sizeof(_req_header) + sizeof(uint32_t) + sizeof(co_str16_t) + sizeof(co_list16_t), "Output buffer too small.");
  size_t written = 0;
  memmove(output, &_req_header, sizeof(_req_header));
  written += sizeof(_req_header);
  memmove(output + sizeof(_req_header), &_id, sizeof(uint32_t));
  written += sizeof(uint32_t);
  _id++;
  CHECK(IS_STR(method), "Not a valid method name.");
  written += co_obj_raw(output, method);
  CHECK(written < olen, "Output buffer too small.");
  CHECK(IS_LIST(method), "Not a valid parameter list.");
  written += co_list_raw(output, olen - written, param);
  CHECK(written < olen, "Output buffer too small.");

  return written;
error:
  return -1;
  
}

static struct 
{
  uint8_t list_type;
  uint8_t list_len;
  uint8_t type_type;
  uint8_t type_value;
  uint8_t id_type;
} _resp_header = {
  .list_type = _list16,
  .list_len = 4,
  .type_type = _uint8,
  .type_value = 1,
  .id_type = _uint32
};

size_t
co_response_alloc(char *output, const size_t olen, const uint32_t id, const co_obj_t *error, co_obj_t *result)
{
  CHECK(((output != NULL) && (error != NULL) && (result != NULL)), "Invalid request components.");
  CHECK(olen > sizeof(_resp_header) + sizeof(uint32_t) + sizeof(co_str16_t) + sizeof(co_list16_t), "Output buffer too small.");
  size_t written = 0;
  memmove(output, &_resp_header, sizeof(_resp_header));
  written += sizeof(_resp_header);
  memmove(output + sizeof(_resp_header), &id, sizeof(uint32_t));
  written += sizeof(uint32_t);
  CHECK(IS_STR(error), "Not a valid error name.");
  written += co_obj_raw(output, error);
  CHECK(written < olen, "Output buffer too small.");
  CHECK(IS_LIST(error), "Not a valid result list.");
  written += co_list_raw(output, olen - written, result);
  CHECK(written < olen, "Output buffer too small.");

  return written;
error:
  return -1;
  
}
