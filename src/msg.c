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
#include "tree.h"

static uint32_t _id = 0;

static struct 
{
  uint8_t list_type;
  uint16_t list_len;
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
  
  CHECK(((output != NULL) && (method != NULL)), "Invalid request components.");
  CHECK(olen > sizeof(_req_header) + sizeof(uint32_t) + sizeof(co_str16_t) + sizeof(co_list16_t), "Output buffer too small.");
  size_t written = 0;
  char *cursor = NULL;
  size_t s = 0;

  /* Pack request header */
  memmove(output + written, &_req_header.list_type, sizeof(_req_header.list_type));
  written += sizeof(_req_header.list_type);

  memmove(output + written, &_req_header.list_len, sizeof(_req_header.list_len));
  written += sizeof(_req_header.list_len);

  memmove(output + written, &_req_header.type_type, sizeof(_req_header.type_type));
  written += sizeof(_req_header.type_type);
  
  memmove(output + written, &_req_header.type_value, sizeof(_req_header.type_value));
  written += sizeof(_req_header.type_value);
  
  memmove(output + written, &_req_header.id_type, sizeof(_req_header.id_type));
  written += sizeof(_req_header.id_type);

  /* Pack request ID */
  memmove(output + written, &_id, sizeof(uint32_t));
  written += sizeof(uint32_t);
  _id++;

  /* Pack method call */
  CHECK(IS_STR(method), "Not a valid method name.");
  char *buffer = NULL;
  size_t buffer_write = co_obj_raw(&buffer, method);
  CHECK(buffer_write >= 0, "Failed to pack object.");
  memmove(output + written, buffer, buffer_write);
  written += buffer_write;

  /* Pack parameters */
  CHECK(written < olen, "Output buffer too small.");
  if(param != NULL)
  {
    if(IS_LIST(param))
      written += co_list_raw(output + written, olen - written, param);
    else 
    {
      s = co_obj_raw(&cursor, param);
      written += s;
      memmove(output + written, cursor, s);
    }
  }
  CHECK(written >= 0, "Failed to pack object.");
  DEBUG("Request bytes written: %d", (int)written);
  CHECK(written < olen, "Output buffer too small.");

  return written;
error:
  return -1;
  
}

static struct 
{
  uint8_t list_type;
  uint16_t list_len;
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
  CHECK(((output != NULL) && (error != NULL) && (result != NULL)), "Invalid response components.");
  CHECK(olen > sizeof(_resp_header) + sizeof(uint32_t) + sizeof(co_str16_t) + sizeof(co_list16_t), "Output buffer too small.");
  size_t written = 0;
  char *cursor = NULL;
  size_t s = 0;

  /* Pack response header */
  memmove(output + written, &_resp_header.list_type, sizeof(_resp_header.list_type));
  written += sizeof(_resp_header.list_type);

  memmove(output + written, &_resp_header.list_len, sizeof(_resp_header.list_len));
  written += sizeof(_resp_header.list_len);

  memmove(output + written, &_resp_header.type_type, sizeof(_resp_header.type_type));
  written += sizeof(_resp_header.type_type);
  
  memmove(output + written, &_resp_header.type_value, sizeof(_resp_header.type_value));
  written += sizeof(_resp_header.type_value);
  
  memmove(output + written, &_resp_header.id_type, sizeof(_resp_header.id_type));
  written += sizeof(_resp_header.id_type);

  /* Pack response ID */
  memmove(output + written, &id, sizeof(uint32_t));
  written += sizeof(uint32_t);

  /* Pack error code */
  //CHECK(IS_STR(error) || IS_NIL(error), "Not a valid error name.");
  if(error != NULL)
  {
    if(IS_LIST(error))
      written += co_list_raw(output + written, olen - written, error);
    else if(IS_TREE(error))
    {
      written += co_tree_raw(output + written, olen - written, error);
    }
    else
    {
      s = co_obj_raw(&cursor, error);
      memmove(output + written, cursor, s);
      written += s;
    }
  }

  /* Pack method result */
  CHECK(written < olen, "Output buffer too small.");
  if(result != NULL)
  {
    if(IS_LIST(result))
      written += co_list_raw(output + written, olen - written, result);
    else if(IS_TREE(result))
    {
      written += co_tree_raw(output + written, olen - written, result);
    }
    else
    {
      s = co_obj_raw(&cursor, result);
      memmove(output + written, cursor, s);
      written += s;
    }
  }

  DEBUG("Response bytes written: %d", (int)written);
  CHECK(written < olen, "Output buffer too small.");

  return written;
error:
  return -1;
}
