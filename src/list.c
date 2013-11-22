/* vim: set ts=2 expandtab: */
/**
 *       @file  list.c
 *      @brief  
 *
 *     @author  Josh King (jheretic), jking@chambana.net
 *
 *   @internal
 *      Created  11/14/2013 11:22:20 AM
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
#include <stddef.h>
#include <stdbool.h>
#include <stdint.h>
#include "debug.h"
#include "obj.h"
#include "list.h"
#include "extern/halloc.h"

#define _DEFINE_LIST(L) int co_list##L##_alloc(co_obj_t *output) \
    { \
      output->_type = _list##L; \
      output->_next = NULL; \
      output->_prev = NULL; \
      ((co_list##L##_t *)output)->_len = 0; \
      return 1; \
    } \
  co_obj_t *co_list##L##_create(void) \
    { \
      co_obj_t *output = h_calloc(1, sizeof(co_list##L##_t)); \
      CHECK_MEM(output); \
      CHECK(co_list##L##_alloc(output), \
          "Failed to allocate object."); \
      return output; \
    error: \
      return NULL; \
    }

_DEFINE_LIST(16);
_DEFINE_LIST(32);

size_t 
co_list_change_length(co_obj_t *list, const int delta)
{ 
  if(CO_TYPE(list) == _list16)
  {
    ((co_list16_t *)list)->_len += delta;
    return (size_t)(((co_list16_t *)list)->_len);
  } else if(CO_TYPE(list) == _list32) {
    ((co_list32_t *)list)->_len += delta;
    return (size_t)(((co_list32_t *)list)->_len);
  }
  ERROR("Not a list object.");
  return -1;
}

size_t 
co_list_length(co_obj_t *list)
{ 
  return co_list_change_length(list, 0);
}

size_t 
co_list_increment(co_obj_t *list)
{ 
  return co_list_change_length(list, 1);
}

size_t 
co_list_decrement(co_obj_t *list)
{ 
  return co_list_change_length(list, -1);
}

co_obj_t * 
co_list_parse(co_obj_t *list, co_iter_t iter, void *context)
{
  co_obj_t *result = NULL;
  CHECK(IS_LIST(list), "Not a list object.");
  co_obj_t *next = _LIST_NEXT(list);
  while(next != NULL && result == NULL)
  {
    result = iter(list, next, context);
    next = _LIST_NEXT(next);
  }
  return result;
error:
  return result;
}

static co_obj_t *
_co_list_contains_i(co_obj_t *data, co_obj_t *current, void *context)
{
  if((co_obj_t *)context == current) return current;
  return NULL;
}

int
co_list_contains(co_obj_t *list, co_obj_t *item)
{
  co_obj_t * result = co_list_parse(list, _co_list_contains_i, (void *)item);
  if(result != NULL) return 1;
  return 0;
}

int
co_list_insert_before(co_obj_t *list, co_obj_t *new_obj, co_obj_t *this_obj)
{
  CHECK(IS_LIST(list), "Not a list object.");
  CHECK(!co_list_contains(list, new_obj), "New node already in specified list.");
  CHECK(co_list_contains(list, this_obj), "Unable to find existing node in \
      specified list.");
  co_obj_t *adjacent = _LIST_PREV(this_obj);
  _LIST_NEXT(adjacent) = new_obj;
  _LIST_PREV(this_obj) = new_obj;
  _LIST_NEXT(new_obj) = this_obj;
  _LIST_PREV(new_obj) = adjacent;
  hattach(new_obj, list);
  co_list_increment(list);
  return 1;
error:
  return 0;
}

int
co_list_insert_after(co_obj_t *list, co_obj_t *new_obj, co_obj_t *this_obj)
{
  CHECK(IS_LIST(list), "Not a list object.");
  CHECK(!co_list_contains(list, new_obj), "New node already in specified list.");
  CHECK(co_list_contains(list, this_obj), "Unable to find existing node in \
      specified list.");
  co_obj_t *adjacent = _LIST_NEXT(this_obj);
  _LIST_PREV(adjacent) = new_obj;
  _LIST_NEXT(this_obj) = new_obj;
  _LIST_PREV(new_obj) = this_obj;
  _LIST_NEXT(new_obj) = adjacent;
  hattach(new_obj, list);
  co_list_increment(list);
  return 1;
error:
  return 0;
}

int
co_list_prepend(co_obj_t *list, co_obj_t *new_obj)
{
  return co_list_insert_after(list, new_obj, list);
}

int
co_list_append(co_obj_t *list, co_obj_t *new_obj)
{
  return co_list_insert_before(list, new_obj, list);
}

static co_obj_t *
_co_list_delete_i(co_obj_t *data, co_obj_t *current, void *context)
{
  if((co_obj_t *)context == current) 
  {
    if(_LIST_PREV(current) != NULL) _LIST_NEXT(_LIST_PREV(current)) = \
      _LIST_NEXT(current);
    if(_LIST_NEXT(current) != NULL) _LIST_PREV(_LIST_NEXT(current)) = \
      _LIST_PREV(current);
    hattach(current, NULL);
    co_list_decrement(data);
    return current;
  }
  return NULL;
}

co_obj_t *
co_list_delete(co_obj_t *list, co_obj_t *item)
{
  return co_list_parse(list, _co_list_delete_i, (void *)item);
}

