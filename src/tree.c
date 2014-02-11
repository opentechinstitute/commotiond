/* vim: set ts=2 expandtab: */
/**
 *       @file  tree.c
 *      @brief  String-indexed search tree for Commotion object system.
 *
 *     @author  Josh King (jheretic), jking@chambana.net
 *
 *   @internal
 *      Created  11/14/2013 10:43:57 PM
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
#include "util.h"
#include "extern/halloc.h"

#define _DEFINE_TREE(L) int co_tree##L##_alloc(co_obj_t *output) \
    { \
      output->_type = _tree##L; \
      output->_next = NULL; \
      output->_prev = NULL; \
      output->_ref = 0; \
      output->_flags = 0; \
      ((co_tree##L##_t *)output)->_len = 0; \
      ((co_tree##L##_t *)output)->root = NULL; \
      return 1; \
    } \
  co_obj_t *co_tree##L##_create(void) \
    { \
      co_obj_t *output = h_calloc(1, sizeof(co_tree##L##_t)); \
      CHECK_MEM(output); \
      CHECK(co_tree##L##_alloc(output), \
          "Failed to allocate object."); \
      return output; \
    error: \
      return NULL; \
    }

_DEFINE_TREE(16);
_DEFINE_TREE(32);


_treenode_t *
co_tree_root(const co_obj_t *tree)
{
  CHECK_MEM(tree);
  _treenode_t *n = NULL;
  if(CO_TYPE(tree) == _tree16)
  {
    n = ((co_tree16_t *)tree)->root;
  } 
  else if(CO_TYPE(tree) == _tree32) 
  {
    n = ((co_tree32_t *)tree)->root;
  }
  else SENTINEL("Specified object is not a tree.");

  return n;
error:
  return NULL;
}

co_obj_t *
co_node_key(_treenode_t *node)
{
  if(node != NULL)
    return node->key;
  else
    return NULL;
}

co_obj_t *
co_node_value(_treenode_t *node)
{
  if(node != NULL)
    return node->value;
  else
    return NULL;
}

static size_t 
_co_tree_change_length(co_obj_t *tree, const int delta)
{ 
  if(CO_TYPE(tree) == _tree16)
  {
    ((co_tree16_t *)tree)->_len += delta;
    return (size_t)(((co_tree16_t *)tree)->_len);
  } else if(CO_TYPE(tree) == _tree32) {
    ((co_tree32_t *)tree)->_len += delta;
    return (size_t)(((co_tree32_t *)tree)->_len);
  }
  ERROR("Not a tree object.");
  return -1;
}

size_t 
co_tree_length(co_obj_t *tree)
{ 
  return _co_tree_change_length(tree, 0);
}

static size_t 
_co_tree_increment(co_obj_t *tree)
{ 
  return _co_tree_change_length(tree, 1);
}

static size_t 
_co_tree_decrement(co_obj_t *tree)
{ 
  return _co_tree_change_length(tree, -1);
}

_treenode_t *
co_tree_find_node(_treenode_t *root, const char *key, const size_t klen)
{
  _treenode_t *n = root;
  size_t i = 0;

  while(i < klen && n) 
  {
    if (key[i] < n->splitchar) 
    {
      n = n->low; 
    } 
    else if (key[i] == n->splitchar) 
    {
      i++;
      if(i < klen) n = n->equal; 
    } 
    else 
    {
      n = n->high; 
    }
  }

  if(n) 
  {
    return n;
  } 
  else 
  {
    return NULL; 
  }
}

co_obj_t *
co_tree_find(const co_obj_t *root, const char *key, const size_t klen)
{
  DEBUG("Get from tree: %s", key);
  _treenode_t *n = NULL;
  CHECK((n = co_tree_root(root)) != NULL, "Specified object is not a tree.");
  n = co_tree_find_node(n, key, klen);

  CHECK(n != NULL, "Unable to find value.");
  return n->value;
error:
  return NULL; 
}

static inline _treenode_t *
_co_tree_delete_r(_treenode_t *root, _treenode_t *current, const char *key, const size_t klen, co_obj_t **value)
{
  DEBUG("Tree current: %s", key);
  if (*key < current->splitchar) 
  {
    current->low = _co_tree_delete_r(root, current->low, key, klen, value); 
  } 
  else if (*key == current->splitchar) 
  { 
    if (klen > 1) 
    {
      // not done yet, keep going but one less
      current->equal = _co_tree_delete_r(root, current->equal, key+1, klen - 1, value);
    } 
    else 
    {
      if(current->value != NULL)
      {
        DEBUG("Found current value.");
        hattach(current->value, NULL);
        current->value->_ref--;
        *value = current->value;
        current->value = NULL;
      }
    }
  } 
  else 
  {
    current->high = _co_tree_delete_r(root, current->high, key, klen, value);
  }

  if(current->low == NULL && current->high == NULL && current->equal == NULL && current->value == NULL)
  {
    h_free(current);
    return NULL;
  }
  else return current; 
}

co_obj_t *
co_tree_delete(co_obj_t *root, const char *key, const size_t klen)
{
  co_obj_t *value = NULL;
  if(CO_TYPE(root) == _tree16)
  {
    ((co_tree16_t *)root)->root = _co_tree_delete_r(((co_tree16_t *)root)->root, \
      ((co_tree16_t *)root)->root, key, klen, &value);
  } 
  else if(CO_TYPE(root) == _tree32) 
  {
    ((co_tree32_t *)root)->root = _co_tree_delete_r(((co_tree32_t *)root)->root, \
      ((co_tree32_t *)root)->root, key, klen, &value);
  }

  CHECK(value != NULL, "Failed to delete value.");
  CHECK(_co_tree_decrement(root), "Failed to decrement value.");
  return value;
error:
  return NULL;
}

static inline _treenode_t *
_co_tree_insert_r(_treenode_t *root, _treenode_t *current, const char *orig_key, const size_t orig_klen,  const char *key, const size_t klen, co_obj_t *value)
{
  if (current == NULL) 
  { 
    current = (_treenode_t *) h_calloc(1, sizeof(_treenode_t));
    if(root == NULL) 
    {
      root = current;
    } 
    else 
    {
      hattach(current, root);
    }
    current->splitchar = *key; 
  }

  if (*key < current->splitchar) 
  {
    current->low = _co_tree_insert_r(root, current->low, orig_key, orig_klen, key, klen, value); 
  } 
  else if (*key == current->splitchar) 
  { 
    if (klen > 1) 
    {
      // not done yet, keep going but one less
      current->equal = _co_tree_insert_r(root, current->equal, orig_key, orig_klen, key+1, klen - 1, value);
    } 
    else 
    {
      if(current->value != NULL)
      {
        co_obj_free(current->value);
      }
      if(current->key != NULL) 
      {
        co_obj_free(current->key);
      }
      current->value = value;
      current->key = co_str8_create(orig_key, orig_klen, 0);
      hattach(current->key, current);
      hattach(current->value, current);
      current->key->_ref++;
      current->value->_ref++;
    }
  } 
  else 
  {
    current->high = _co_tree_insert_r(root, current->high, orig_key, orig_klen, key, klen, value);
  }

  return current; 
}

static int 
_co_tree_insert(co_obj_t *root, const char *key, const size_t klen, co_obj_t *value)
{
  _treenode_t *n = NULL;
  if(CO_TYPE(root) == _tree16)
  {
    ((co_tree16_t *)root)->root = _co_tree_insert_r(((co_tree16_t *)root)->root, \
      ((co_tree16_t *)root)->root, key, klen, key, klen, value);
    n = ((co_tree16_t *)root)->root;
    hattach(n, root);
  } 
  else if(CO_TYPE(root) == _tree32) 
  {
    ((co_tree32_t *)root)->root = _co_tree_insert_r(((co_tree32_t *)root)->root, \
      ((co_tree32_t *)root)->root, key, klen, key, klen, value);
    n = ((co_tree32_t *)root)->root;
    hattach(n, root);
  }

  CHECK(n != NULL, "Failed to insert value.");
  CHECK(_co_tree_increment(root), "Failed to increment value.");
  return 1;
error:
  return 0;
}

int
co_tree_insert(co_obj_t *root, const char *key, const size_t klen, co_obj_t *value)
{
  _treenode_t *n = co_tree_find_node(co_tree_root(root), key, klen);
  CHECK(n == NULL, "Key exists.");
  return _co_tree_insert(root, key, klen, value);
error:
  return 0;
}

int
co_tree_insert_force(co_obj_t *root, const char *key, const size_t klen, co_obj_t *value)
{
  return _co_tree_insert(root, key, klen, value);
}

static int
_co_node_set_str(_treenode_t *n, const char *value, const size_t vlen)
{
  CHECK(n != NULL, "Invalid node supplied.");
  CHECK(n->value != NULL, "Invalid node supplied.");
  switch(CO_TYPE(n->value))
  {
    case _str8:
      CHECK(vlen <= UINT8_MAX, "Value too large for type str8.");
      if(vlen != (((co_str8_t *)(n->value))->_len))
      {
        n->value = h_realloc(n->value, (size_t)(vlen + sizeof(co_str8_t) - 1));
      }
      CHECK_MEM(memmove((((co_str8_t *)(n->value))->data), value, vlen));
      (((co_str8_t *)(n->value))->_len) = (uint8_t)vlen;
      break;
    case _str16:
      CHECK(vlen <= UINT16_MAX, "Value too large for type str16.");
      if(vlen != (((co_str16_t *)(n->value))->_len))
      {
        n->value = h_realloc(n->value, (size_t)(vlen + sizeof(co_str16_t) - 1));
      }
      CHECK_MEM(memmove((((co_str16_t *)(n->value))->data), value, vlen));
      (((co_str16_t *)(n->value))->_len) = (uint16_t)vlen;
      break;
    case _str32:
      CHECK(vlen <= UINT32_MAX, "Value too large for type str32.");
      if(vlen != (((co_str32_t *)(n->value))->_len))
      {
        n->value = h_realloc(n->value, (size_t)(vlen + sizeof(co_str32_t) - 1));
      }
      CHECK_MEM(memmove((((co_str32_t *)(n->value))->data), value, vlen));
      (((co_str32_t *)(n->value))->_len) = (uint32_t)vlen;
      break;
    default:
      SENTINEL("Specified object is not a string.");
      break;
  }

  return 1;
error:
  return 0;
}

int
co_tree_set_str(co_obj_t *root, const char *key, const size_t klen, const char *value, const size_t vlen)
{
  DEBUG("Get from tree: %s", key);
  _treenode_t *n = NULL;
  CHECK((n = co_tree_root(root)) != NULL, "Specified object is not a tree.");
  n = co_tree_find_node(n, key, klen);
  CHECK(n != NULL, "Failed to find key in tree.");
  CHECK(_co_node_set_str(n, value, vlen), "Unable to set string for node.");
  return 1;
error:
  return 0;
}

static int
_co_node_set_int(_treenode_t *n, const signed long value)
{
  CHECK(n != NULL, "Invalid node supplied.");
  switch(CO_TYPE(n->value))
  {
    case _int8:
      (((co_int8_t *)(n->value))->data) = value;
      break;
    case _int16:
      (((co_int16_t *)(n->value))->data) = value;
      break;
    case _int32:
      (((co_int32_t *)(n->value))->data) = value;
      break;
    case _int64:
      (((co_int64_t *)(n->value))->data) = value;
      break;
    default:
      SENTINEL("Specified object is not a signed integer.");
      break;
  }
  return 1;
error:
  return 0;
}

int
co_tree_set_int(co_obj_t *root, const char *key, const size_t klen, const signed long value)
{
  DEBUG("Get from tree: %s", key);
  _treenode_t *n = NULL;
  CHECK((n = co_tree_root(root)) != NULL, "Specified object is not a tree.");
  n = co_tree_find_node(n, key, klen);
  CHECK(n != NULL, "Failed to find key in tree.");
  CHECK(_co_node_set_int(n, value), "Unable to set integer for node.");
  return 1;
error:
  return 0;
}

static int
_co_node_set_uint(_treenode_t *n, const unsigned long value)
{
  CHECK(n != NULL, "Invalid node supplied.");
  switch(CO_TYPE(n->value))
  {
    case _uint8:
      (((co_uint8_t *)(n->value))->data) = value;
      break;
    case _uint16:
      (((co_uint16_t *)(n->value))->data) = value;
      break;
    case _uint32:
      (((co_uint32_t *)(n->value))->data) = value;
      break;
    case _uint64:
      (((co_uint64_t *)(n->value))->data) = value;
      break;
    default:
      SENTINEL("Specified object is not a unsigned integer.");
      break;
  }
  return 1;
error:
  return 0;
}

int
co_tree_set_uint(co_obj_t *root, const char *key, const size_t klen, const unsigned long value)
{
  DEBUG("Get from tree: %s", key);
  _treenode_t *n = NULL;
  CHECK((n = co_tree_root(root)) != NULL, "Specified object is not a tree.");
  n = co_tree_find_node(n, key, klen);
  CHECK(n != NULL, "Failed to find key in tree.");
  CHECK(_co_node_set_uint(n, value), "Unable to set unsigned integer for node.");

  return 1;
error:
  return 0;
}

static int
_co_node_set_float(_treenode_t *n, const double value)
{
  CHECK(n != NULL, "Invalid node supplied.");
  if(CO_TYPE(n->value) == _float32)
  {
    (((co_float32_t *)(n->value))->data) = value;
  } 
  else if(CO_TYPE(n->value) == _float64) 
  {
    (((co_float64_t *)(n->value))->data) = value;
  }
  else ERROR("Specified object is not a floating-point value.");

  return 1;
error:
  return 0;
}

int
co_tree_set_float(co_obj_t *root, const char *key, const size_t klen, const double value)
{
  DEBUG("Get from tree: %s", key);
  _treenode_t *n = NULL;
  CHECK((n = co_tree_root(root)) != NULL, "Specified object is not a tree.");
  n = co_tree_find_node(n, key, klen);
  CHECK(n != NULL, "Failed to find key in tree.");
  CHECK(_co_node_set_float(n, value), "Unable to set float for node.");

  return 1;
error:
  return 0;
}

static inline void
_co_tree_process_r(co_obj_t *tree, _treenode_t *current, const co_iter_t iter, void *context)
{
  CHECK(IS_TREE(tree), "Recursion target is not a tree.");
  if(current != NULL)
  {
    if(current->value != NULL) iter(tree, current->value, context);
    _co_tree_process_r(tree, current->low, iter, context); 
    _co_tree_process_r(tree, current->equal, iter, context); 
    _co_tree_process_r(tree, current->high, iter, context); 
  }
  return; 
error:
  return;
}

int 
co_tree_process(co_obj_t *tree, const co_iter_t iter, void *context)
{
  switch(CO_TYPE(tree))
  {
    case _tree16:
      _co_tree_process_r(tree, ((co_tree16_t *)tree)->root, iter, context);
      break;
    case _tree32:
      _co_tree_process_r(tree, ((co_tree32_t *)tree)->root, iter, context);
      break;
    default:
      SENTINEL("Object is not a tree.");
      break;
  }
  return 1;
error:
  return 0;
}

void 
co_tree_destroy(co_obj_t *root)
{
    if(root) {
        h_free(root);
    }
}

static inline void
_co_tree_raw_r(char **output, const size_t *olen, size_t *written, _treenode_t *current)
{
  if(current == NULL) return;
  size_t klen = 0, vlen = 0; 
  char *kbuf = NULL, *vbuf = NULL;
  if(current->value != NULL)
  {
    CHECK((klen = co_obj_raw(&kbuf, current->key)) > 0, "Failed to read key.");
    memmove(*output, kbuf, klen);
    *output += klen;
    *written += klen;
    if(IS_TREE(current->value))
    {
      vlen = co_tree_raw(*output, *olen - *written, current->value);
      CHECK(vlen > 0, "Failed to dump tree value.");
    }
    else if(IS_LIST(current->value))
    {
      vlen = co_list_raw(*output, *olen, current->value);
      CHECK(vlen > 0, "Failed to dump tree value.");
    }
    else
    {
      CHECK((vlen = co_obj_raw(&vbuf, current->value)) > 0, "Failed to read value.");
      CHECK((klen + vlen) < ((*olen) - (*written)), "Data too large for buffer.");
      DEBUG("Dumping value %s of size %d with key %s of size %d.", vbuf, (int)vlen, kbuf, (int)klen);
      memmove(*output, vbuf, vlen);
    }
    *written += vlen;
    *output += vlen;
  }
  _co_tree_raw_r(output, olen, written, current->low); 
  _co_tree_raw_r(output, olen, written, current->equal); 
  _co_tree_raw_r(output, olen, written, current->high); 
  return; 
error:
  return;
}

size_t
co_tree_raw(char *output, const size_t olen, const co_obj_t *tree)
{
  char *out = output;
  size_t written = 0;
  switch(CO_TYPE(tree))
  {
    case _tree16:
      memmove(out, &(tree->_type), sizeof(tree->_type));
      out += sizeof(tree->_type);
      written += sizeof(tree->_type);
      memmove(out, &(((co_tree16_t *)tree)->_len), sizeof(((co_tree16_t *)tree)->_len));
      out += sizeof(((co_tree16_t *)tree)->_len);
      written += sizeof(((co_tree16_t *)tree)->_len);
      break;
    case _tree32:
      memmove(out, &(tree->_type), sizeof(tree->_type));
      out += sizeof(tree->_type);
      written += sizeof(tree->_type);
      memmove(out, &(((co_tree32_t *)tree)->_len), sizeof(((co_tree32_t *)tree)->_len));
      out += sizeof(((co_tree32_t *)tree)->_len);
      written += sizeof(((co_tree32_t *)tree)->_len);
      break;
    default:
      SENTINEL("Not a tree object.");
      break;
  }
  
  _co_tree_raw_r(&out, &olen, &written, co_tree_root(tree));
  DEBUG("Tree bytes written: %d", (int)written);
  return written;
error:
  return -1;
}


size_t
co_tree_import(co_obj_t **tree, const char *input, const size_t ilen)
{
  size_t length = 0, olen = 0, read = 0, klen = 0;
  char *kstr = NULL;
  int i = 0;
  co_obj_t *obj = NULL;
  const char *cursor = input;
  switch((uint8_t)input[0])
  {
    case _tree16:
      length = *((uint16_t *)(input + 1));
      *tree = co_tree16_create();
      cursor += sizeof(uint16_t) + 1;
      read = sizeof(uint16_t) + 1;
      break;
    case _tree32:
      length = (uint32_t)(*(uint32_t*)(input + 1));
      *tree = co_tree32_create();
      cursor += sizeof(uint32_t) + 1;
      read = sizeof(uint32_t) + 1;
      break;
    default:
      SENTINEL("Not a tree.");
      break;
  }
  while(i < length && read <= ilen)
  {
    DEBUG("Importing tuple:");
    if((uint8_t)cursor[0] == _str8)
    {
      DEBUG("Reading key...");
      cursor += 1;
      read += 1;
      klen = (uint8_t)cursor[0];
      kstr = (char *)&cursor[1];
      cursor += klen + 1;
      read += klen + 1;

      DEBUG("Reading value...");
      switch((uint8_t)cursor[0])
      {
        case _list16:
        case _list32:
          olen = co_list_import(&obj, cursor, ilen - read);
          break;
        case _tree16:
        case _tree32:
          olen = co_tree_import(&obj, cursor, ilen - read);
          break;
        default:
          olen = co_obj_import(&obj, cursor, ilen - read, 0);
          break;
      }
      CHECK(olen > 0, "Failed to import object.");
      cursor +=olen;
      read += olen;

      DEBUG("Inserting value into tree with key.");
      CHECK(co_tree_insert(*tree, kstr, klen, obj), "Failed to insert object.");
      i++;
    }
  }
  return read;
error:
  if(obj != NULL) co_obj_free(obj);
  return -1;
}

static inline void
_co_tree_print_r(co_obj_t *tree, _treenode_t *current, int *count)
{
  CHECK(IS_TREE(tree), "Recursion target is not a tree.");
  if(current == NULL) return;
  char *key = NULL;
  char *value = NULL;
  if(co_node_value(current) != NULL)
  {
    (*count)++;
    co_obj_data(&key, co_node_key(current));
    co_obj_data(&value, co_node_value(current));
    //CHECK(IS_STR(key) && IS_STR(value), "Incorrect types for profile.");
    if(*count < co_tree_length(tree))
    {
      printf(" \"%s\": \"%s\", ", key, value);
    }
    else
    {
      printf(" \"%s\": \"%s\" ", key, value);
    }
  }
  _co_tree_print_r(tree, current->low, count); 
  _co_tree_print_r(tree, current->equal, count); 
  _co_tree_print_r(tree, current->high, count); 
  return; 
error:
  return;
}

int 
co_tree_print(co_obj_t *tree)
{
  CHECK_MEM(tree);
  int count = 0;

  printf("{");
  _co_tree_print_r(tree, co_tree_root(tree), &count);
  printf("}\n");

  return 1;
error:
  return 0;
}

static inline void
_co_tree_print_raw_r(co_obj_t *tree, _treenode_t *current)
{
  CHECK(IS_TREE(tree), "Recursion target is not a tree.");
  if(current == NULL) return;
  size_t klen = 0, vlen = 0; 
  char *kbuf = NULL, *vbuf = NULL;
  if(current->value != NULL)
  {
    CHECK((klen = co_obj_raw(&kbuf, current->key)) > 0, "Failed to read key.");
    CHECK((vlen = co_obj_raw(&vbuf, current->value)) > 0, "Failed to read value.");
    printf("KEY:\n");
    hexdump((void *)kbuf, klen);
    printf("VAL:\n");
    hexdump((void *)vbuf, vlen);
  }
  _co_tree_print_raw_r(tree, current->low); 
  _co_tree_print_raw_r(tree, current->equal); 
  _co_tree_print_raw_r(tree, current->high); 
  return; 
error:
  return;
}

int
co_tree_print_raw(co_obj_t *tree)
{
  CHECK_MEM(tree);
  _co_tree_print_raw_r(tree, co_tree_root(tree));
  return 1;
error:
  return 0;
}
