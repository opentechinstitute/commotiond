/* vim: set ts=2 expandtab: */
/**
 *       @file  commotion.c
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
#include <stdlib.h>
#include <stdbool.h>
#include <sys/socket.h>
#include "debug.h"
#include "obj.h"
#include "list.h"
#include "tree.h"
#include "socket.h"
#include "util.h"
#include "msg.h"
#include "commotion.h"

#define REQUEST_MAX 4096
#define RESPONSE_MAX 4096

static co_obj_t *_pool = NULL;
static co_obj_t *_sockets = NULL;

extern co_socket_t unix_socket_proto;

int
co_init(void)
{
  CHECK(_pool == NULL, "Commotion API already initialized.");
  _pool = h_calloc(1, sizeof(_pool));
  _sockets = co_list16_create();
  hattach(_sockets, _pool);
  DEBUG("Commotion API initialized.");
  return 1;
error:
  return 0; 
}

static inline co_obj_t *
_co_shutdown_sockets_i(co_obj_t *data, co_obj_t *current, void *context)
{
  if(current != NULL && IS_SOCK(current)) ((co_socket_t*)current)->destroy(current);  
  return NULL;
}

int
co_shutdown(void)
{
  CHECK_MEM(_sockets);
  CHECK(IS_LIST(_sockets), "API not properly initialized.");
  co_list_parse(_sockets, _co_shutdown_sockets_i, NULL);
  co_obj_free(_pool);
  return 1;
error:
  return 0;
}

co_obj_t *
co_connect(const char *uri, const size_t ulen)
{
  CHECK_MEM(_sockets);
  CHECK(uri != NULL && ulen > 0, "Invalid URI.");
  co_obj_t *socket = NEW(co_socket, unix_socket);
  hattach(socket, _pool);
  CHECK((((co_socket_t*)socket)->connect(socket, uri)), "Failed to connect to commotiond at %s\n", uri);
  co_list_append(_sockets, socket);
  return socket;
error:
  co_obj_free(socket);
  return NULL;
}

int
co_disconnect(co_obj_t *connection)
{
  CHECK_MEM(connection);
  CHECK_MEM(_sockets);
  CHECK(IS_SOCK(connection), "Specified object is not a Commotion socket.");
  
  co_list_delete(_sockets, connection);
  ((co_socket_t*)connection)->destroy(connection);
  return 1;
error:
  return 0;
}

co_obj_t *
co_request_create(void)
{
  return co_list16_create(); 
}

int
co_request_append(co_obj_t *request, co_obj_t *object)
{
  CHECK_MEM(request);
  CHECK_MEM(object);
  CHECK(IS_LIST(request), "Not a valid request.");
  return co_list_append(request, object);
error:
  return 0;
}

int
co_request_append_str(co_obj_t *request, const char *s, const size_t slen)
{
  CHECK_MEM(request);
  CHECK_MEM(s);
  CHECK(IS_LIST(request), "Not a valid request.");
  CHECK(slen < UINT32_MAX, "String is too large.");
  if(slen > UINT16_MAX) return co_list_append(request, co_str32_create(s, slen, 0));
  if(slen > UINT8_MAX) return co_list_append(request, co_str16_create(s, slen, 0));
  return co_list_append(request, co_str8_create(s, slen, 0));
error:
  return 0;
}

int
co_request_append_bin(co_obj_t *request, const char *s, const size_t slen)
{
  CHECK_MEM(request);
  CHECK_MEM(s);
  CHECK(IS_LIST(request), "Not a valid request.");
  CHECK(slen < UINT32_MAX, "Binary is too large.");
  if(slen > UINT16_MAX) return co_list_append(request, co_bin32_create(s, slen, 0));
  if(slen > UINT8_MAX) return co_list_append(request, co_bin16_create(s, slen, 0));
  return co_list_append(request, co_bin8_create(s, slen, 0));
  error:
  return 0;
}

int
co_request_append_int(co_obj_t *request, const int i)
{
  CHECK_MEM(request);
  CHECK(IS_LIST(request), "Not a valid request.");
  CHECK(i > INT64_MAX || i < INT64_MIN, "Integer out of bounds.");
  if(i > INT32_MAX || i < INT32_MIN) return co_list_append(request, co_int64_create(i, 0));
  if(i > INT16_MAX || i < INT16_MIN) return co_list_append(request, co_int32_create(i, 0));
  if(i > INT8_MAX || i < INT8_MIN) return co_list_append(request, co_int16_create(i, 0));
  return co_list_append(request, co_int8_create(i, 0));
error:
  return 0;
}

int
co_request_append_uint(co_obj_t *request, const unsigned int i)
{
  CHECK_MEM(request);
  CHECK(IS_LIST(request), "Not a valid request.");
  CHECK(i > UINT64_MAX, "Integer out of bounds.");
  if(i > UINT32_MAX) return co_list_append(request, co_uint64_create(i, 0));
  if(i > UINT16_MAX) return co_list_append(request, co_uint32_create(i, 0));
  if(i > UINT8_MAX) return co_list_append(request, co_uint16_create(i, 0));
  return co_list_append(request, co_uint8_create(i, 0));
error:
  return 0;
}

int
co_call(co_obj_t *connection, co_obj_t **response, const char *method, const size_t mlen, co_obj_t *request)
{
  CHECK(method != NULL && mlen > 0 && mlen < UINT8_MAX, "Invalid method name.");
  CHECK(connection != NULL && IS_SOCK(connection), "Invalid connection.");
  co_obj_t *params = NULL, *rlist = NULL, *rtree = NULL;
  int retval = 0;
  size_t reqlen = 0, resplen = 0;
  char req[REQUEST_MAX];
  char resp[RESPONSE_MAX];
  if(request != NULL)
  {
    CHECK(IS_LIST(request), "Not a valid request.");
    params = request;
  }
  else
  {
    params = co_list16_create();
  }
  co_obj_t *m = co_str8_create(method, mlen, 0);
  reqlen = co_request_alloc(req, sizeof(req), m, params);
  CHECK(((co_socket_t*)connection)->send((co_obj_t*)((co_socket_t*)connection)->fd, req, reqlen) != -1, "Send error!");
  if((resplen = ((co_socket_t*)connection)->receive(connection, (co_obj_t*)((co_socket_t*)connection)->fd, resp, sizeof(resp))) > 0) 
  {
    CHECK(co_list_import(&rlist, resp, resplen) > 0, "Failed to parse response.");
    rtree = co_list_element(rlist, 3);
    if(!IS_NIL(rtree))
    {
      retval = 1;
    }
    else
    {
      rtree = co_list_element(rlist, 2);
      retval = 0;
    }
    if(rtree != NULL && IS_TREE(rtree)) 
    {
      *response = rtree;
      hattach(*response, _pool);
    }
    else SENTINEL("Invalid response.");
  }
  else SENTINEL("Failed to receive data.");

  co_obj_free(m);
  if(params != request) co_obj_free(params);
  return retval;
  
error:
  co_obj_free(m);
  if(params != request) co_obj_free(params);
  return retval;
}

co_obj_t *
co_response_get(co_obj_t *response, const char *key, const size_t klen)
{
  CHECK(response != NULL && IS_TREE(response), "Invalid response object.");
  CHECK(key != NULL && klen > 0, "Invalid key.");
  return co_tree_find(response, key, klen);
error:
  return NULL;
}

size_t
co_response_get_str(co_obj_t *response, char **output, const char *key, const size_t klen) 
{
  co_obj_t *obj = co_response_get(response, key, klen);
  CHECK(obj != NULL, "Response value %s does not exist.", key);
  CHECK(IS_STR(obj), "Value %s is not a string.", key);
  return co_obj_data(output, obj);
error:
  return -1;
}

size_t
co_response_get_bin(co_obj_t *response, char **output, const char *key, const size_t klen) 
{
  co_obj_t *obj = co_response_get(response, key, klen);
  CHECK(obj != NULL, "Response value %s does not exist.", key);
  CHECK(IS_BIN(obj), "Value %s is not a binary.", key);
  return co_obj_data(output, obj);
  error:
  return -1;
}

int
co_response_get_uint(co_obj_t *response, unsigned long *output, const char *key, const size_t klen) 
{
  co_obj_t *obj = co_response_get(response, key, klen);
  CHECK(obj != NULL, "Response value %s does not exist.", key);
  switch(CO_TYPE(obj))
  {
    case _uint8:
      *output = (unsigned long)(((co_uint8_t *)obj)->data);
      break;
    case _uint16:
      *output = (unsigned long)(((co_uint16_t *)obj)->data);
      break;
    case _uint32:
      *output = (unsigned long)(((co_uint32_t *)obj)->data);
      break;
    case _uint64:
      *output = (unsigned long)(((co_uint64_t *)obj)->data);
      break;
    default:
      SENTINEL("Not an unsigned integer.");
      break;
  }
  return 1;

error:
  return 0;
}

int
co_response_get_int(co_obj_t *response, signed long *output, const char *key, const size_t klen) 
{
  co_obj_t *obj = co_response_get(response, key, klen);
  CHECK(obj != NULL, "Response value %s does not exist.", key);
  switch(CO_TYPE(obj))
  {
    case _int8:
      *output = (unsigned long)(((co_int8_t *)obj)->data);
      break;
    case _int16:
      *output = (unsigned long)(((co_int16_t *)obj)->data);
      break;
    case _int32:
      *output = (unsigned long)(((co_int32_t *)obj)->data);
      break;
    case _int64:
      *output = (unsigned long)(((co_int64_t *)obj)->data);
      break;
    default:
      SENTINEL("Not an unsigned integer.");
      break;
  }
  return 1;

error:
  return 0;
}

int
co_response_get_bool(co_obj_t *response, bool *output, const char *key, const size_t klen) {
  co_obj_t *obj = co_response_get(response, key, klen);
  CHECK(obj != NULL, "Response value %s does not exist.", key);
  switch(CO_TYPE(obj))
  {
    case _false:
      *output = false;
      break;
    case true:
      *output = true;
      break;
    default:
      SENTINEL("Not a boolean.");
      break;
  }
  return 1;
  
  error:
  return 0;
}

int
co_response_print(co_obj_t *response)
{
  CHECK(response != NULL && IS_TREE(response), "Invalid response object.");
  return co_tree_print(response);
error:
  return 0;
}

void
co_free(co_obj_t *object)
{
  co_obj_free(object);
  return;
}
