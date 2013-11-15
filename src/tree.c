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

struct _treenode_t
{
    char splitchar; 
    _treenode_t *low;
    _treenode_t *equal;
    _treenode_t *high; 
    co_obj_t *object;
};

/*
enum {
    MAX_TRAVERSE_SIZE = 128
};
*/

co_obj_t *co_tree_search(co_obj_t *root, const char *s, size_t len)
{
  _treenode_t *p = NULL;
  if(CO_TYPE(root) == _tree16)
  {
    p = ((co_tree16_t *)root)->root;
  } 
  else if(CO_TYPE(root) == _tree32) 
  {
    p = ((co_tree32_t *)root)->root;
  }
  else ERROR("Specified object is not a tree.");
  size_t i = 0;

  while(i < len && p) 
  {
    if (s[i] < p->splitchar) 
    {
      p = p->low; 
    } 
    else if (s[i] == p->splitchar) 
    {
      i++;
      if(i < len) p = p->equal; 
    } 
    else 
    {
      p = p->high; 
    }
  }

  if(p) 
  {
    return p->object;
  } 
  else 
  {
    return NULL; 
  }
}


static inline _treenode_t *co_tree_insert_r(_treenode_t *root, _treenode_t *p, const char *s, const size_t len, co_obj_t *value)
{
    if (p == NULL) { 
        p = (_treenode_t *) h_calloc(sizeof(_treenode_t), 1);

        if(root == NULL) {
            root = p;
        } else {
            hattach(p, root);
        }

        p->splitchar = *s; 
    }

    if (*s < p->splitchar) {
        p->low = co_tree_insert_r(root, p->low, s, len, value); 
    } else if (*s == p->splitchar) { 
        if (len > 1) {
            // not done yet, keep going but one less
            p->equal = co_tree_insert_r(root, p->equal, s+1, len - 1, value);
        } else {
            CHECK(p->object != NULL, "Duplicate insert into tree.");
            p->object = value;
        }
    } else {
        p->high = co_tree_insert_r(root, p->high, s, len, value);
    }

    return p; 
error:
    return NULL;
}

int co_tree_insert(co_obj_t *root, const char *s, const size_t len, co_obj_t *value)
{
  _treenode_t *p = NULL;
  if(CO_TYPE(root) == _tree16)
  {
    p = ((co_tree16_t *)root)->root;
  } 
  else if(CO_TYPE(root) == _tree32) 
  {
    p = ((co_tree32_t *)root)->root;
  }
  p = co_tree_insert_r(p, p, s, len, value);
  if(p != NULL)
  {
    value->_key = (co_obj_t *)co_str8_create(s, len);
    hattach(value->_key, p);
    return 1;
  }
  else return 0;
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


void co_tree_destroy(co_obj_t *root)
{
    if(root) {
        h_free(root);
    }
}
