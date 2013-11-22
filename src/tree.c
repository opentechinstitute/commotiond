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
co_tree_find(co_obj_t *root, const char *key, size_t klen)
{
  DEBUG("Get from tree: %s", key);
  _treenode_t *n = NULL;
  if(CO_TYPE(root) == _tree16)
  {
    DEBUG("Is a size 16 tree.");
    n = ((co_tree16_t *)root)->root;
  } 
  else if(CO_TYPE(root) == _tree32) 
  {
    DEBUG("Is a size 32 tree.");
    n = ((co_tree32_t *)root)->root;
  }
  else ERROR("Specified object is not a tree.");
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
    return n->value;
  } 
  else 
  {
    return NULL; 
  }
}


static inline _treenode_t *
_co_tree_delete_r(_treenode_t *root, _treenode_t *current, const char *key, const size_t klen, co_obj_t *value)
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
        hattach(current->value, NULL);
        value = current->value;
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
      ((co_tree16_t *)root)->root, key, klen, value);
  } 
  else if(CO_TYPE(root) == _tree32) 
  {
    ((co_tree32_t *)root)->root = _co_tree_delete_r(((co_tree32_t *)root)->root, \
      ((co_tree32_t *)root)->root, key, klen, value);
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
        co_free(current->value);
      }
      if(current->key != NULL) 
      {
        co_free(current->key);
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

static inline void
_co_tree_process_r(co_obj_t *tree, _treenode_t *current, const co_iter_t iter, void *context)
{
  CHECK(IS_TREE(tree), "Recursion target is not a tree.");
  if(current->value != NULL)
  {
    iter(tree, current->value, context);
  }
  _co_tree_process_r(tree, current->low, iter, context); 
  _co_tree_process_r(tree, current->low, iter, context); 
  _co_tree_process_r(tree, current->low, iter, context); 
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


/*
co_obj_t **co_tree_resize_queue(co_obj_t **queue, int q_start, int q_size, int q_max, int new_size)
{
    int i = 0;
    co_obj_t ** new_queue = calloc(sizeof(co_obj_t *),  new_size);
    CHECK(new_queue, "Failed to reallocate queue for traverse");

    for(i = 0; i < q_size; i++) {
        new_queue[i] = queue[(i + q_start) % q_max];
    }

    free(queue);
    return new_queue;

error:
    free(queue);
    return NULL;
}


void co_tree_traverse(co_obj_t *p, co_iter_t cb, void *data)
{
    if (!p) return;

    int q_start = 0;
    int q_size = 0;
    int q_max = MAX_TRAVERSE_SIZE;
    co_obj_t **queue = calloc(sizeof(co_obj_t *), MAX_TRAVERSE_SIZE);
    CHECK(queue, "Failed to malloc queue for traverse");

    queue[q_start] = p;
    q_size++;

    while(q_size > 0) {
        _treenode_t *cur = ((co_tree16_t *)queue[q_start])->root;
        q_start = (q_start + 1) % q_max;
        q_size--;

        // Resize if we must
        if(q_size + 3 > q_max) {
            queue = co_tree_resize_queue(queue, q_start, q_size, q_max, q_max * 2);
            q_start = 0;
            q_max = q_max * 2;
        }

        if(cur->object) cb(p, cur->object, data);

        if(cur->low) queue[(q_start + (q_size++)) % q_max] = cur->low;
        if(cur->equal) queue[(q_start + (q_size++)) % q_max] = cur->equal;
        if(cur->high) queue[(q_start + (q_size++)) % q_max] = cur->high;
    }

    free(queue);
    return;

error:
    if(queue) free(queue);
}
*/

void 
co_tree_destroy(co_obj_t *root)
{
    if(root) {
        h_free(root);
    }
}
