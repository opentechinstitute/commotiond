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
#ifndef _TREE_H
#define _TREE_H
#include <stdlib.h>
#include <stddef.h>
#include <stdint.h>
#include "debug.h"
#include "extern/halloc.h"

typedef struct _treenode_t _treenode_t;

/* Type "tree" declaration macros */
#define _DECLARE_TREE(L) typedef struct { co_obj_t _header; uint##L##_t _len; \
  _treenode_t *root;} co_tree##L##_t; int co_tree##L##_alloc(\
  co_tree##L##_t *output); co_tree##L##_t *co_tree##L##_create(void);

_DECLARE_TREE(16);
_DECLARE_TREE(32);







#endif
