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
#include "tree.h"
#include "extern/halloc.h"

#define _DEFINE_TREE(L) int co_tree##L##_alloc(co_obj_t *output) \
    { \
      output->_type = _tree##L; \
      output->_next = NULL; \
      output->_prev = NULL; \
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

struct _treenode_t
{
    char splitchar; 
    _treenode_t *low;
    _treenode_t *equal;
    _treenode_t *high; 
    co_obj_t *key;
    co_obj_t *value;
};

co_obj_t *
co_node_value(_treenode_t *node)
{
  return node->value;
}

_treenode_t *
co_tree_root(const co_obj_t *tree)
{
  _treenode_t *n = NULL;
  if(CO_TYPE(tree) == _tree16)
  {
    DEBUG("Is a size 16 tree.");
    n = ((co_tree16_t *)tree)->root;
  } 
  else if(CO_TYPE(tree) == _tree32) 
  {
    DEBUG("Is a size 32 tree.");
    n = ((co_tree32_t *)tree)->root;
  }
  else SENTINEL("Specified object is not a tree.");

  return n;
error:
  return NULL;
}

_treenode_t *
co_tree_find_node(_treenode_t *root, const char *key, const size_t klen)
{
  _treenode_t *n = root;
  size_t i = 0;

  while(i < klen && n) 
  {
    DEBUG("Tree node: %c", key[i]);
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

  DEBUG("Tree current: %s", key);
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
    }
  } 
  else 
  {
    current->high = _co_tree_insert_r(root, current->high, orig_key, orig_klen, key, klen, value);
  }

  return current; 
}

int 
co_tree_insert(co_obj_t *root, const char *key, const size_t klen, co_obj_t *value)
{
  _treenode_t *n = NULL;
  if(CO_TYPE(root) == _tree16)
  {
    ((co_tree16_t *)root)->root = _co_tree_insert_r(((co_tree16_t *)root)->root, \
      ((co_tree16_t *)root)->root, key, klen, key, klen, value);
    n = ((co_tree16_t *)root)->root;
  } 
  else if(CO_TYPE(root) == _tree32) 
  {
    ((co_tree32_t *)root)->root = _co_tree_insert_r(((co_tree32_t *)root)->root, \
      ((co_tree32_t *)root)->root, key, klen, key, klen, value);
    n = ((co_tree32_t *)root)->root;
  }

  CHECK(n != NULL, "Failed to insert value.");
  return 1;
error:
  return 0;
}

int
co_node_set_str(_treenode_t *n, const char *value, const size_t vlen)
{
  CHECK(n != NULL, "Invalid node supplied.");
  switch(CO_TYPE(n->value) == _str8)
  {
    case _str8:
      DEBUG("Is a size 8 string.");
      CHECK(vlen <= UINT8_MAX, "Value too large for type str8.");
      if(vlen != (((co_str8_t *)(n->value))->_len))
      {
        n->value = h_realloc(n->value, (size_t)(vlen + sizeof(co_str8_t) - 1));
      }
      CHECK_MEM(memmove((((co_str8_t *)(n->value))->data), value, vlen));
      (((co_str8_t *)(n->value))->_len) = (uint8_t)vlen;
      break;
    case _str16:
      DEBUG("Is a size 16 string.");
      CHECK(vlen <= UINT16_MAX, "Value too large for type str16.");
      if(vlen != (((co_str16_t *)(n->value))->_len))
      {
        n->value = h_realloc(n->value, (size_t)(vlen + sizeof(co_str16_t) - 1));
      }
      CHECK_MEM(memmove((((co_str16_t *)(n->value))->data), value, vlen));
      (((co_str16_t *)(n->value))->_len) = (uint16_t)vlen;
      break;
    case _str32:
      DEBUG("Is a size 32 string.");
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
  CHECK(co_node_set_str(n, value, vlen), "Unable to set string for node.");
  return 1;
error:
  return 0;
}

int
co_node_set_int(_treenode_t *n, const signed long value)
{
  CHECK(n != NULL, "Invalid node supplied.");
  switch(CO_TYPE(n->value))
  {
    case _int8:
      DEBUG("Is a size 8 signed int.");
      (((co_int8_t *)(n->value))->data) = value;
      break;
    case _int16:
      DEBUG("Is a size 16 signed int.");
      (((co_int16_t *)(n->value))->data) = value;
      break;
    case _int32:
      DEBUG("Is a size 32 signed int.");
      (((co_int32_t *)(n->value))->data) = value;
      break;
    case _int64:
      DEBUG("Is a size 64 signed int.");
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
  CHECK(co_node_set_int(n, value), "Unable to set integer for node.");
  return 1;
error:
  return 0;
}

int
co_node_set_uint(_treenode_t *n, const unsigned long value)
{
  CHECK(n != NULL, "Invalid node supplied.");
  switch(CO_TYPE(n->value))
  {
    case _uint8:
      DEBUG("Is a size 8 unsigned uint.");
      (((co_uint8_t *)(n->value))->data) = value;
      break;
    case _uint16:
      DEBUG("Is a size 16 unsigned uint.");
      (((co_uint16_t *)(n->value))->data) = value;
      break;
    case _uint32:
      DEBUG("Is a size 32 unsigned uint.");
      (((co_uint32_t *)(n->value))->data) = value;
      break;
    case _uint64:
      DEBUG("Is a size 64 unsigned uint.");
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
  CHECK(co_node_set_uint(n, value), "Unable to set unsigned integer for node.");

  return 1;
error:
  return 0;
}

int
co_node_set_float(_treenode_t *n, const double value)
{
  CHECK(n != NULL, "Invalid node supplied.");
  if(CO_TYPE(n->value) == _float32)
  {
    DEBUG("Is a size 32 float.");
    (((co_float32_t *)(n->value))->data) = value;
  } 
  else if(CO_TYPE(n->value) == _float64) 
  {
    DEBUG("Is a size 64 float.");
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
  CHECK(co_node_set_float(n, value), "Unable to set float for node.");

  return 1;
error:
  return 0;
}

static inline void
_co_tree_process_r(co_obj_t *tree, _treenode_t *current, const co_iter_t iter, void *context)
{
  CHECK(IS_TREE(tree), "Recursion target is not a tree.");
  if(current->value != NULL)
  {
    iter(tree, current->value, context);
  }
  _co_tree_process_r(tree, current->low, iter, context); 
  _co_tree_process_r(tree, current->equal, iter, context); 
  _co_tree_process_r(tree, current->high, iter, context); 
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
