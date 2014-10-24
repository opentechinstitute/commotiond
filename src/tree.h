/* vim: set ts=2 expandtab: */
/**
 *       @file  tree.h
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
#ifndef _TREE_H
#define _TREE_H
#include <stdlib.h>
#include <stddef.h>
#include <stdint.h>
#include "debug.h"
#include "extern/halloc.h"

typedef struct _treenode_t _treenode_t;

/**
 * @struct _treenode_t a data structure that holds one node in search tree
 */
struct _treenode_t
{
    char splitchar; 
    _treenode_t *parent;
    _treenode_t *low;
    _treenode_t *equal;
    _treenode_t *high; 
    co_obj_t *key;
    co_obj_t *value;
} __attribute__((packed));

/* Type "tree" declaration macros */
#define _DECLARE_TREE(L) typedef struct __attribute__((packed)) { co_obj_t _header; uint##L##_t _len; \
  _treenode_t *root;} co_tree##L##_t; int co_tree##L##_alloc(\
  co_obj_t *output); co_obj_t *co_tree##L##_create(void);

_DECLARE_TREE(16);
_DECLARE_TREE(32);

/**
 * @brief return key object for given node
 * @param node node to return object for
 */
co_obj_t *co_node_key(_treenode_t *node);

/**
 * @brief return value object for given node
 * @param node node to return object for
 */
co_obj_t *co_node_value(_treenode_t *node);

/**
 * @brief find node in given tree
 * @param root tree in which to search
 * @param key key to search for
 * @param klen length of key
 */
_treenode_t *co_tree_find_node(_treenode_t *root, const char *key, const size_t klen);

/**
 * @brief return root node of tree object
 * @param tree tree object that contains root
 */
_treenode_t *co_tree_root(const co_obj_t *tree);

/**
 * @brief return length (number of key-value pairs) of given tree
 * @param tree tree object
 */
ssize_t co_tree_length(co_obj_t *tree);

/**
 * @brief return value from given tree that corresponds to key
 * @param root tree object
 * @param key key to search for
 * @param klen length of key
 */
co_obj_t *co_tree_find(const co_obj_t *root, const char *key, const size_t klen);

/**
 * @brief delete value from given tree that corresponds to key
 * @param root tree object
 * @param key key to search for
 * @param klen length of key
 */
co_obj_t *co_tree_delete(co_obj_t *root, const char *key, const size_t klen);

/**
 * @brief insert object into given tree and associate with key
 * @param root tree object
 * @param key key to search for
 * @param klen length of key
 * @param value value object to insert
 */
int co_tree_insert(co_obj_t *root, const char *key, const size_t klen, co_obj_t *value);

/**
 * @brief insert object into given tree and associate with key, where value is not tied to tree
 * @param root tree object
 * @param key key to search for
 * @param klen length of key
 * @param value value object to insert
 */
int co_tree_insert_unsafe(co_obj_t *root, const char *key, const size_t klen, co_obj_t *value);

/**
 * @brief insert object into given tree and associate with key (overwrite if it exists)
 * @param root tree object
 * @param key key to search for
 * @param klen length of key
 * @param value value object to insert
 */
int co_tree_insert_force(co_obj_t *root, const char *key, const size_t klen, co_obj_t *value);

/**
 * @brief insert object into given tree and associate with key, where value is not tied to tree (overwrite if it exists)
 * @param root tree object
 * @param key key to search for
 * @param klen length of key
 * @param value value object to insert
 */
int co_tree_insert_unsafe_force(co_obj_t *root, const char *key, const size_t klen, co_obj_t *value);

/**
 * @brief set value contained in an object in the tree with a specified key (if a string)
 * @param root tree object
 * @param key key to search for
 * @param klen length of key
 * @param value value to insert
 * @param vlen length of value
 */
int co_tree_set_str(co_obj_t *root, const char *key, const size_t klen, const char *value, const size_t vlen);

/**
 * @brief set value contained in an object in the tree with a specified key (if an int)
 * @param root tree object
 * @param key key to search for
 * @param klen length of key
 * @param value value to insert
 */
int co_tree_set_int(co_obj_t *root, const char *key, const size_t klen, const signed long value);

/**
 * @brief set value contained in an object in the tree with a specified key (if an unsigned int)
 * @param root tree object
 * @param key key to search for
 * @param klen length of key
 * @param value value to insert
 */
int co_tree_set_uint(co_obj_t *root, const char *key, const size_t klen, const unsigned long value);

/**
 * @brief set value contained in an object in the tree with a specified key (if a float)
 * @param root tree object
 * @param key key to search for
 * @param klen length of key
 * @param value value to insert
 */
int co_tree_set_float(co_obj_t *root, const char *key, const size_t klen, const double value);

/**
 * @brief process tree with given iterator function
 * @param tree tree object to process
 * @param iter iterator function
 * @param context additional arguments to iterator
 */
int co_tree_process(co_obj_t *tree, const co_iter_t iter, void *context);

/**
 * @brief free tree structure
 * @param root tree object to free
 */
void co_tree_destroy(co_obj_t *root);

/**
 * @brief dump raw representation of tree
 * @param output output buffer
 * @param olen length of output buffer
 * @param tree tree to dump
 */
ssize_t co_tree_raw(char *output, const size_t olen, const co_obj_t *tree);

/**
 * @brief import raw representation of tree
 * @param tree target pointer of new, imported tree object
 * @param input input buffer
 * @param ilen length of input buffer
 */
ssize_t co_tree_import(co_obj_t **tree, const char *input, const size_t ilen);

/**
 * @brief print tree with indent
 * @param tree tree object to print
 * @param indent level of indent
 */
void co_tree_print_indent(co_obj_t *tree, int indent);

/**
 * @brief print tree
 * @param tree tree object to print
 */
int co_tree_print(co_obj_t *tree);

/**
 * @brief print raw dump of tree
 * @param tree tree object to print
 */
int co_tree_print_raw(co_obj_t *tree);

co_obj_t *co_tree_next(const co_obj_t *tree, co_obj_t *key);

#endif
