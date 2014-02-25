/* vim: set ts=2 expandtab: */
/**
 *       @file  list.h
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
#ifndef _LIST_H
#define _LIST_H
#include <stdlib.h>
#include <stddef.h>
#include <stdint.h>
#include "debug.h"
#include "extern/halloc.h"

typedef struct _listnode_t _listnode_t;

/* Type "list" declaration macros */
#define _DECLARE_LIST(L) typedef struct __attribute__((packed)) \
  { co_obj_t _header; uint##L##_t _len; _listnode_t *_first; _listnode_t *_last; } co_list##L##_t; \
  int co_list##L##_alloc(co_obj_t *output); co_obj_t *\
  co_list##L##_create(void);

_DECLARE_LIST(16);
_DECLARE_LIST(32);

/**
 * @brief return length (number of objects) of given list
 * @param list list object
 */
size_t co_list_length(co_obj_t *list);

/**
 * @brief return first element of given list
 * @param list list object
 */
co_obj_t * co_list_get_first(const co_obj_t *list);

/**
 * @brief return last element of given list
 * @param list list object
 */
co_obj_t * co_list_get_last(const co_obj_t *list);

/**
 * @brief process list with given iterator function
 * @param list list object to process
 * @param iter iterator function
 * @param context additional arguments to iterator
 */
co_obj_t * co_list_parse(co_obj_t *list, co_iter_t iter, void *context);

/**
 * @brief determine if list contains specified item
 * @param list list object to process
 * @param item item to search for
 */
int co_list_contains(co_obj_t *list, co_obj_t *item);

/**
 * @brief insert new item in list before specified item
 * @param list list object to process
 * @param new_obj item to insert
 * @param this_obj item to insert before
 */
int co_list_insert_before(co_obj_t *list, co_obj_t *new_obj, \
    co_obj_t *this_obj);

/**
 * @brief insert new item in list before specified item without the list managing the item's memory
 * @param list list object to process
 * @param new_obj item to insert
 * @param this_obj item to insert before
 */
int co_list_insert_before_unsafe(co_obj_t *list, co_obj_t *new_obj, \
    co_obj_t *this_obj);

/**
 * @brief insert new item in list after specified item
 * @param list list object to process
 * @param new_obj item to insert
 * @param this_obj item to insert after
 */
int co_list_insert_after(co_obj_t *list, co_obj_t *new_obj, co_obj_t *this_obj);

/**
 * @brief insert new item in list after specified item without the list managing the item's memory
 * @param list list object to process
 * @param new_obj item to insert
 * @param this_obj item to insert after
 */
int co_list_insert_after_unsafe(co_obj_t *list, co_obj_t *new_obj, co_obj_t *this_obj);

/**
 * @brief insert new item at beginning of list
 * @param list list object to process
 * @param new_obj item to insert
 */
int co_list_prepend(co_obj_t *list, co_obj_t *new_obj);

/**
 * @brief insert new item at beginning of list without the list managing the item's memory
 * @param list list object to process
 * @param new_obj item to insert
 */
int co_list_prepend_unsafe(co_obj_t *list, co_obj_t *new_obj);

/**
 * @brief insert new item at end of list
 * @param list list object to process
 * @param new_obj item to insert
 */
int co_list_append(co_obj_t *list, co_obj_t *new_obj);

/**
 * @brief insert new item at end of list without the list managing the item's memory
 * @param list list object to process
 * @param new_obj item to insert
 */
int co_list_append_unsafe(co_obj_t *list, co_obj_t *new_obj);

/**
 * @brief delete specified item from list
 * @param list list object to process
 * @param item item to delete
 */
co_obj_t *co_list_delete(co_obj_t *list, co_obj_t *item);

/**
 * @brief return item at specified position in list
 * @param list list object to process
 * @param index number of item to return
 */
co_obj_t *co_list_element(co_obj_t *list, const unsigned int index);

/**
 * @brief dump raw representation of list
 * @param output output buffer 
 * @param olen length of output buffer 
 * @param list list object to process
 */
size_t co_list_raw(char *output, const size_t olen, const co_obj_t *list);

/**
 * @brief import list from raw representation
 * @param list target pointer to new list object
 * @param input input buffer 
 * @param ilen length of input buffer 
 */
size_t co_list_import(co_obj_t **list, const char *input, const size_t ilen);

/**
 * @brief print list with indent
 * @param list list  object to print
 * @param indent level of indent
 */
void co_list_print_indent(co_obj_t *list, int indent);

/**
 * @brief print list
 * @param list list object to print
 */
int co_list_print(co_obj_t *list);

#endif
