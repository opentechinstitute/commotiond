/* vim: set ts=2 expandtab: */
/**
 *       @file  db.h
 *      @brief  a key-value store for loading configuration information
 *
 *     @author  Josh King (jheretic), jking@chambana.net
 *
 *   @internal
 *     Created  03/07/2013
 *    Revision  $Id: doxygen.commotion.templates,v 0.1 2013/01/01 09:00:00 jheretic Exp $
 *    Compiler  gcc/g++
 *     Company  The Open Technology Institute
 *   Copyright  Copyright (c) 2013, Josh King
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

#ifndef _DB_H
#define _DB_H

#include <stdlib.h>
#include <stddef.h>
#include "obj.h"

void co_db_destroy(void);
int co_db_create(size_t index_size);
int co_db_insert(const char *key, const size_t klen, co_obj_t *value);
co_obj_t *co_db_namespace_find(const char *namespace, const size_t nlen, const char *key, const size_t klen);
co_obj_t *co_db_find(const char *key, const size_t klen);
int co_db_set_str(const char *key, const size_t klen, const char *value, const size_t vlen);
size_t co_db_get_str(char *output, const char *key, const size_t klen);
#endif
