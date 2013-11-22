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
#ifndef _LIST_H
#define _LIST_H
#include <stdlib.h>
#include <stddef.h>
#include <stdint.h>
#include "debug.h"
#include "extern/halloc.h"

/* Type "list" declaration macros */
#define _DECLARE_LIST(L) typedef struct { co_obj_t _header; uint##L##_t _len; } \
  co_list##L##_t; int co_list##L##_alloc(co_obj_t *output); co_obj_t *\
  co_list##L##_create(void);

_DECLARE_LIST(16);
_DECLARE_LIST(32);

size_t co_list_change_length(co_obj_t *list, const int delta);

size_t co_list_length(co_obj_t *list);

size_t co_list_increment(co_obj_t *list);

size_t co_list_decrement(co_obj_t *list);

co_obj_t * co_list_parse(co_obj_t *list, co_iter_t iter, void *context);

int co_list_contains(co_obj_t *list, co_obj_t *item);

int co_list_insert_before(co_obj_t *list, co_obj_t *new_obj, co_obj_t *this_obj);

int co_list_insert_after(co_obj_t *list, co_obj_t *new_obj, co_obj_t *this_obj);

int co_list_prepend(co_obj_t *list, co_obj_t *new_obj);

int co_list_append(co_obj_t *list, co_obj_t *new_obj);

co_obj_t *co_list_delete(co_obj_t *list, co_obj_t *item);

#endif
