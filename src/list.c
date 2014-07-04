/* vim: set ts=2 expandtab: */
/**
 *       @file  list.c
 *      @brief  Commotion double linked-list implementation
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
#include <stdint.h>
#include "debug.h"
#include "obj.h"
#include "list.h"
#include "tree.h"
#include "extern/halloc.h"

#undef _LIST_NEXT
#undef _LIST_PREV

#define _LIST_NEXT(J) (((_listnode_t *)J)->next)
#define _LIST_PREV(J) (((_listnode_t *)J)->prev)

struct _listnode_t
{
    _listnode_t *prev;
    _listnode_t *next;
    co_obj_t *value;
} __attribute__((packed));

typedef _listnode_t *(*_listiter_t)(co_obj_t *data, _listnode_t *current, void *context);

#define _DEFINE_LIST(L) int co_list##L##_alloc(co_obj_t *output) \
    { \
      output->_type = _list##L; \
      output->_next = NULL; \
      output->_prev = NULL; \
      output->_flags = 0; \
      output->_ref = 0; \
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

static _listnode_t *
_listnode_create(co_obj_t *value, bool safe)
{
  _listnode_t *ret = h_calloc(1, sizeof(_listnode_t));
  _LIST_PREV(ret) = NULL;
  _LIST_NEXT(ret) = NULL;
  if(value != NULL)
  {
    ret->value = value;
    if(safe) hattach(value, ret);
  }
  else
    ret->value = NULL;
  return ret;
}

static _listnode_t * /* Done */
_co_list_get_first_node(const co_obj_t *list)
{
  CHECK_MEM(list);
  _listnode_t *n = NULL;
  if(CO_TYPE(list) == _list16)
  {
    n = ((co_list16_t *)list)->_first;
  }
  else if(CO_TYPE(list) == _list32)
  {
    n = ((co_list32_t *)list)->_first;
  }
  else SENTINEL("Specified object is not a list.");

  return n;
error:
  return NULL;
}

co_obj_t * /* Done */
co_list_get_first(const co_obj_t *list)
{
  return (_co_list_get_first_node(list))->value;
}

static int /* Done */
_co_list_set_first(co_obj_t *list, _listnode_t *node)
{
  CHECK_MEM(list);
  if(CO_TYPE(list) == _list16)
  {
    ((co_list16_t *)list)->_first = node;
  }
  else if(CO_TYPE(list) == _list32)
  {
    ((co_list32_t *)list)->_first = node;
  }
  else SENTINEL("Specified object is not a list.");

  return 1;
error:
  return 0;
}

static _listnode_t * /* Done */
_co_list_get_last_node(const co_obj_t *list)
{
  CHECK_MEM(list);
  _listnode_t *n = NULL;
  if(CO_TYPE(list) == _list16)
  {
    n = ((co_list16_t *)list)->_last;
  }
  else if(CO_TYPE(list) == _list32)
  {
    n = ((co_list32_t *)list)->_last;
  }
  else SENTINEL("Specified object is not a list.");

  return n;
error:
  return NULL;
}

co_obj_t * /* Done */
co_list_get_last(const co_obj_t *list)
{
  return (_co_list_get_last_node(list))->value;
}

static int /* Done */
_co_list_set_last(co_obj_t *list, _listnode_t *node)
{
  CHECK_MEM(list);
  if(CO_TYPE(list) == _list16)
  {
    ((co_list16_t *)list)->_last = node;
  }
  else if(CO_TYPE(list) == _list32)
  {
    ((co_list32_t *)list)->_last = node;
  }
  else SENTINEL("Specified object is not a list.");

  return 1;
error:
  return 0;
}

static size_t  /* Done */
_co_list_change_length(co_obj_t *list, const int delta)
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

static size_t  /* Done */
_co_list_increment(co_obj_t *list)
{
  return _co_list_change_length(list, 1);
}

static size_t /* Done */
_co_list_decrement(co_obj_t *list)
{
  return _co_list_change_length(list, -1);
}

size_t /* Done */
co_list_length(co_obj_t *list)
{
  return _co_list_change_length(list, 0);
}

static _listnode_t * /* Done */
_co_list_parse_node(co_obj_t *root, _listiter_t iter, void *context)
{
  _listnode_t *result = NULL;
  _listnode_t *next = _co_list_get_first_node(root);
  while(next != NULL && result == NULL)
  {
    result = iter(root, next, context);
    next = _LIST_NEXT(next);
  }
  return result;
}

co_obj_t *
co_list_for(co_obj_t *list, void** cookie) {
  _listnode_t *next;

  if (*cookie == NULL) next = _co_list_get_first_node(root);
  else next = (_listnode_t *) *cookie;

  *cookie = (void*) _LIST_NEXT(next);
  return next;
}


co_obj_t * /* Done */
co_list_parse(co_obj_t *list, co_iter_t iter, void *context)
{
  co_obj_t *result = NULL;
  CHECK_MEM(list);
  CHECK(IS_LIST(list), "Not a list object.");
  //co_obj_t *next = _LIST_NEXT(list);
  _listnode_t *next = _co_list_get_first_node(list);
  while(next != NULL && result == NULL)
  {
    result = iter(list, next->value, context);
    next = _LIST_NEXT(next);
  }
  return result;
error:
  return result;
}

static _listnode_t * /* Done */
_co_list_contains_i(co_obj_t *data, _listnode_t *current, void *context)
{
  if((co_obj_t *)context == current->value) return current;
  return NULL;
}

int /* Done */
co_list_contains(co_obj_t *list, co_obj_t *item)
{
  _listnode_t * result = _co_list_parse_node(list, _co_list_contains_i, (void *)item);
  if(result != NULL) return 1;
  return 0;
}

static _listnode_t * /* Done */
_co_list_find_node(co_obj_t *list, co_obj_t *item)
{
  return _co_list_parse_node(list, _co_list_contains_i, (void *)item);
}


int /* Done */
co_list_insert_before(co_obj_t *list, co_obj_t *new_obj, co_obj_t *this_obj)
{
  CHECK(IS_LIST(list), "Not a list object.");
  _listnode_t *this_node = _co_list_find_node(list, this_obj);
  CHECK(this_node != NULL, "Unable to find existing node in \
specified list.");
  CHECK(!co_list_contains(list, new_obj), "New node already in specified \
list.");
  _listnode_t *adjacent = _LIST_PREV(this_node);
  _listnode_t *new_node = _listnode_create(new_obj, true);
  hattach(new_node, list);
  if(adjacent == NULL)
  {
    /* First in list. */
    _co_list_set_first(list, new_node);
  }
  else
  {
    _LIST_NEXT(adjacent) = new_node;
    _LIST_PREV(new_node) = adjacent;
  }
  _LIST_NEXT(new_node) = this_node;
  _LIST_PREV(this_node) = new_node;
  _co_list_increment(list);
  return 1;
error:
  return 0;
}

int /* Done */
co_list_insert_before_unsafe(co_obj_t *list, co_obj_t *new_obj, co_obj_t *this_obj)
{
  CHECK(IS_LIST(list), "Not a list object.");
  _listnode_t *this_node = _co_list_find_node(list, this_obj);
  CHECK(this_node != NULL, "Unable to find existing node in \
specified list.");
  CHECK(!co_list_contains(list, new_obj), "New node already in specified \
list.");
  _listnode_t *adjacent = _LIST_PREV(this_node);
  _listnode_t *new_node = _listnode_create(new_obj, false);
  hattach(new_node, list);
  if(adjacent == NULL)
  {
    /* First in list. */
    _co_list_set_first(list, new_node);
  }
  else
  {
    _LIST_NEXT(adjacent) = new_node;
    _LIST_PREV(new_node) = adjacent;
  }
  _LIST_NEXT(new_node) = this_node;
  _LIST_PREV(this_node) = new_node;
  _co_list_increment(list);
  return 1;
error:
  return 0;
}

int /* Done */
co_list_insert_after(co_obj_t *list, co_obj_t *new_obj, co_obj_t *this_obj)
{
  CHECK(IS_LIST(list), "Not a list object.");
  _listnode_t *this_node = _co_list_find_node(list, this_obj);
  CHECK(this_node != NULL, "Unable to find existing node in \
specified list.");
  CHECK(!co_list_contains(list, new_obj), "New node already in specified \
list.");
  _listnode_t *adjacent = _LIST_NEXT(this_node);
  _listnode_t *new_node = _listnode_create(new_obj, true);
  hattach(new_node, list);
  if(adjacent == NULL)
  {
    /* Last in list. */
    _co_list_set_last(list, new_node);
  }
  else
  {
    _LIST_PREV(adjacent) = new_node;
    _LIST_NEXT(new_node) = adjacent;
  }
  _LIST_PREV(new_node) = this_node;
  _LIST_NEXT(this_node) = new_node;
  _co_list_increment(list);
  return 1;
error:
  return 0;
}

int /* Done */
co_list_insert_after_unsafe(co_obj_t *list, co_obj_t *new_obj, co_obj_t *this_obj)
{
  CHECK(IS_LIST(list), "Not a list object.");
  _listnode_t *this_node = _co_list_find_node(list, this_obj);
  CHECK(this_node != NULL, "Unable to find existing node in \
specified list.");
  CHECK(!co_list_contains(list, new_obj), "New node already in specified \
list.");
  _listnode_t *adjacent = _LIST_NEXT(this_node);
  _listnode_t *new_node = _listnode_create(new_obj, false);
  hattach(new_node, list);
  if(adjacent == NULL)
  {
    /* Last in list. */
    _co_list_set_last(list, new_node);
  }
  else
  {
    _LIST_PREV(adjacent) = new_node;
    _LIST_NEXT(new_node) = adjacent;
  }
  _LIST_PREV(new_node) = this_node;
  _LIST_NEXT(this_node) = new_node;
  _co_list_increment(list);
  return 1;
error:
  return 0;
}

int /* Done */
co_list_prepend(co_obj_t *list, co_obj_t *new_obj)
{
  if(co_list_length(list) == 0)
  {
    /* First item in list. */
    _listnode_t *new_node = _listnode_create(new_obj, true);
    _co_list_set_first(list, new_node);
    _co_list_set_last(list, new_node);
    hattach(new_node, list);
    _co_list_increment(list);
    return 1;
  }
  return co_list_insert_before(list, new_obj, (_co_list_get_first_node(list))->value);
}

int /* Done */
co_list_prepend_unsafe(co_obj_t *list, co_obj_t *new_obj)
{
  if(co_list_length(list) == 0)
  {
    /* First item in list. */
    _listnode_t *new_node = _listnode_create(new_obj, false);
    _co_list_set_first(list, new_node);
    _co_list_set_last(list, new_node);
    hattach(new_node, list);
    _co_list_increment(list);
    return 1;
  }
  return co_list_insert_before(list, new_obj, (_co_list_get_first_node(list))->value);
}

int /* Done */
co_list_append(co_obj_t *list, co_obj_t *new_obj)
{
  if(co_list_length(list) == 0)
  {
    /* First item in list. */
    _listnode_t *new_node = _listnode_create(new_obj, true);
    _co_list_set_first(list, new_node);
    _co_list_set_last(list, new_node);
    hattach(new_node, list);
    _co_list_increment(list);
    return 1;
  }
  return co_list_insert_after(list, new_obj, (_co_list_get_last_node(list))->value);
}

int /* Done */
co_list_append_unsafe(co_obj_t *list, co_obj_t *new_obj)
{
  if(co_list_length(list) == 0)
  {
    /* First item in list. */
    _listnode_t *new_node = _listnode_create(new_obj, false);
    _co_list_set_first(list, new_node);
    _co_list_set_last(list, new_node);
    hattach(new_node, list);
    _co_list_increment(list);
    return 1;
  }
  return co_list_insert_after(list, new_obj, (_co_list_get_last_node(list))->value);
}

co_obj_t * /* Done */
co_list_delete(co_obj_t *list, co_obj_t *item)
{
  co_obj_t *ret = NULL;
  _listnode_t *current = _co_list_find_node(list, item);
  if(current != NULL)
  {
    if(_LIST_PREV(current) != NULL)
      _LIST_NEXT(_LIST_PREV(current)) = _LIST_NEXT(current);
    else
      _co_list_set_first(list, _LIST_NEXT(current));

    if(_LIST_NEXT(current) != NULL)
      _LIST_PREV(_LIST_NEXT(current)) = _LIST_PREV(current);
    else
      _co_list_set_last(list, _LIST_PREV(current));

    ret = current->value;
    hattach(current->value, NULL);
    h_free(current);
    _co_list_decrement(list);
    return ret;
  }
  return NULL;
}

co_obj_t * /* Done. */
co_list_element(co_obj_t *list, const unsigned int index)
{
  CHECK(IS_LIST(list), "Not a list object.");
  _listnode_t *next = _co_list_get_first_node(list);
  unsigned int i = 0;
  if(next != NULL)
  {
    for(i = 0; i < index; i++)
    {
      next = _LIST_NEXT(next);
      if(next == NULL) break;
    }
  }
  CHECK(i == index, "List index not found.");
  return next->value;
error:
  return NULL;
}

size_t
co_list_raw(char *output, const size_t olen, const co_obj_t *list)
{
  size_t written = 0, read = 0;
  char *in = NULL;
  char *out = output;
  switch(CO_TYPE(list))
  {
    case _list16:
      memmove(out, &(list->_type), sizeof(uint8_t));
      out += sizeof(uint8_t);
      memmove(out, &(((co_list16_t *)list)->_len), sizeof(uint16_t));
      out += sizeof(uint16_t);
      written = sizeof(uint8_t) + sizeof(uint16_t);
      break;
    case _list32:
      memmove(out, &(list->_type), sizeof(uint8_t));
      out += sizeof(uint8_t);
      memmove(out, &(((co_list32_t *)list)->_len), sizeof(uint32_t));
      out += sizeof(uint32_t);
      written = sizeof(uint8_t) + sizeof(uint32_t);
      break;
    default:
      SENTINEL("Not a list object.");
      break;
  }
  _listnode_t *next = _co_list_get_first_node(list);
  while(next != NULL && next->value != NULL && written <= olen)
  {
    if((CO_TYPE(next->value) == _list16) || (CO_TYPE(next->value) == _list32))
    {
        read = co_list_raw(out, olen - written, next->value);
        CHECK(read >= 0, "Failed to dump object.");
        CHECK(read + written < olen, "Data too large for buffer.");
    }
    else if ((CO_TYPE(next->value) == _tree16) || (CO_TYPE(next->value) == _tree32))
    {
        read = co_tree_raw(out, olen - written, next->value);
        CHECK(read >= 0, "Failed to dump object.");
        CHECK(read + written < olen, "Data too large for buffer.");
    }
    else
    {
        read = co_obj_raw(&in, next->value);
        CHECK(read >= 0, "Failed to dump object.");
        CHECK(read + written < olen, "Data too large for buffer.");
        memmove(out, in, read);
    }
    DEBUG("List in: %s, read: %d", in, (int)read);
    written += read;
    out += read;
    next = _LIST_NEXT(next);
  }
  return written;
error:
  return -1;

}

size_t
co_list_import(co_obj_t **list, const char *input, const size_t ilen)
{
  size_t length = 0, olen = 0, read = 0;
  co_obj_t *obj = NULL;
  const char *cursor = input;
  switch((uint8_t)input[0])
  {
    case _list16:
      length = *((uint16_t *)(input + 1));
      *list = co_list16_create();
      cursor += sizeof(uint16_t) + 1;
      read = sizeof(uint16_t) + 1;
      break;
    case _list32:
      length = *((uint32_t *)(input + 1));
      *list = co_list32_create();
      cursor += sizeof(uint32_t) + 1;
      read = sizeof(uint32_t) + 1;
      break;
    default:
      SENTINEL("Not a list.");
      break;
  }
  for(int i = 0; (i < length && read <= ilen); i++)
  {
    if(((uint8_t)cursor[0] == _list16) || ((uint8_t)cursor[0] == _list32))
    {
      olen = co_list_import(&obj, cursor, ilen - read);
    }
    else if(((uint8_t)cursor[0] == _tree16) || ((uint8_t)cursor[0] == _tree32))
    {
      olen = co_tree_import(&obj, cursor, ilen - read);
    }
    else olen = co_obj_import(&obj, cursor, ilen - read, 0);
    CHECK(olen > 0, "Failed to import object.");
    cursor +=olen;
    read += olen;
    CHECK(co_list_append(*list, obj), "Failed to add object to list.");
  }
  return read;
error:
  return -1;
}

static co_obj_t *
_co_list_print_i(co_obj_t *list, co_obj_t *current, void *_indent)
{
  if (current == list) return NULL;

  char *val = NULL;
  int indent = *(int*)_indent;

  if ((CO_TYPE(current) == _tree16) || (CO_TYPE(current) == _tree32))
  {
    co_tree_print_indent(current, indent);
  }
  else if ((CO_TYPE(current) == _list16) || (CO_TYPE(current) == _list32))
  {
    co_list_print_indent(current, indent);
  }
  else
  {
    for (int i = 0; i < indent; i++) printf("  ");
    co_obj_data(&val,current);
    switch(CO_TYPE(current))
    {
      case _float32:
      case _float64:
	printf("%f",*(float*)val);
	break;
      case _uint8:
      case _int8:
	printf("%d",*(int8_t*)val);
	break;
      case _uint16:
      case _int16:
	printf("%d",*(int16_t*)val);
	break;
      case _uint32:
      case _int32:
	printf("%ld",*(long*)val);
	break;
      case _uint64:
      case _int64:
	printf("%lld",*(long long*)val);
	break;
      case _str8:
      case _str16:
      case _str32:
	printf("\"%s\"",val);
	break;
      case _bin8:
      case _bin16:
      case _bin32:
	printf("%x",*(unsigned int*)val);
    }
  }
  if (co_list_get_last(list) != current)
    printf(",");
  printf("\n");

  return NULL;
}

void
co_list_print_indent(co_obj_t *list, int indent)
{
  for (int i = 0; i < indent; i++) printf("  ");
  printf("[\n");
  indent++;
  co_list_parse(list, _co_list_print_i, &indent);
  indent--;
  for (int i = 0; i < indent; i++) printf("  ");
  printf("]");
  if (!indent) printf("\n");
}

int
co_list_print(co_obj_t *list)
{
  CHECK_MEM(list);

  co_list_print_indent(list,0);

  return 1;
error:
  return 0;
}
