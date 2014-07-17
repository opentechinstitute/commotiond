/* vim: set ts=2 expandtab: */
/**
 *       @file  profile.h
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

#ifndef _PROFILE_H
#define _PROFILE_H

#include <stdlib.h>
#include <stddef.h>
#include "obj.h"

#define SCHEMA(N) static int schema_##N(co_obj_t *self, co_obj_t **output, co_obj_t *params)

#define SCHEMA_ADD(K, V) ({ \
  co_obj_t *val = co_str8_create(V, sizeof(V), 0); \
  if (!co_tree_insert(self, K, sizeof(K), val)) \
    co_obj_free(val); \
  })

#define SCHEMA_REGISTER(N) co_schema_register(schema_##N)

#define SCHEMA_GLOBAL(N) co_schema_register_global(schema_##N)

typedef struct co_cbptr_t co_cbptr_t;

/**
 * @struct co_cbptr_t a data structure wrapping a callback pointer
 */
struct co_cbptr_t {
  co_obj_t _header;
  uint8_t _exttype;
  uint8_t _len;
  co_cb_t cb;
} __attribute__((packed));

typedef struct co_profile_t co_profile_t;

/**
 * @struct co_profile_t a linked list for profile structs
 */
struct co_profile_t {
  co_obj_t _header;
  uint8_t _exttype;
  uint8_t _len;
  co_obj_t *name; /**< command name */
  co_obj_t *data;
} __attribute__((packed));

/**
 * @brief register a callback for populating a profile schema
 * @param cb schema callback
 */
int co_schema_register(co_cb_t cb);

/**
 * @brief register a callback for populating global profile
 * @param cb schema callback
 */
int co_schema_register_global(co_cb_t cb);

/**
 * @brief load schemas for specified profile
 * @param profile profile to load schemas for
 */
int co_schemas_load(co_obj_t *profile);

/**
 * @brief create global profiles list
 * @param index_size size of index for profiles list (16 or 32 bits)
 */
int co_profiles_create(const size_t index_size);

/**
 * @brief creates a list of available profiles
 */
int co_profiles_init(const size_t index_size);

/**
 * @brief removes the list of available profiles
 */
void co_profiles_shutdown(void);

/**
 * @brief imports available profiles from profiles directory
 * @param path file path to the profiles directory
 */
int co_profile_import_files(const char *path);

/**
 * @brief imports global profile
 * @param path file path to the global profile
 */
int co_profile_import_global(const char *path);

/**
 * @brief searches the profile list for a specified profile
 * @param name profile name (search key)
 */
co_obj_t *co_profile_find(co_obj_t *name);

/**
 * @brief returns the global profile
 */
co_obj_t *co_profile_global(void); 

/**
 * @brief deletes the global profile
 */
void co_profile_delete_global(void); 

/**
 * @brief adds a new profile
 * @param name profile name
 * @param nlen length of profile name
 */
int co_profile_add(const char *name, const size_t nlen);

/**
 * @brief removes profile with given name
 * @param name profile name
 * @param nlen length of profile name
 */
int co_profile_remove(const char *name, const size_t nlen);

/**
 * @brief dumps profile data 
 * @param profile profile struct
 */
void co_profile_dump(co_obj_t *profile);

/**
 * @brief returns the key value (if an int) from the profile. If no value set, returns the default value
 * @param profile profile struct
 * @param key key in profile
 * @param klen key length
 */
co_obj_t *co_profile_get(co_obj_t *profile, const co_obj_t *key);

/**
 * @brief sets a specified profile value (if a string)
 * @param profile profile struct
 * @param key key in profile
 * @param klen key length
 * @param value key's value
 * @param vlen value length
 */
int co_profile_set_str(co_obj_t *profile, const char *key, const size_t klen, const char *value, const size_t vlen);

/**
 * @brief returns the key value (if a string) from the profile. If no value set, returns the default value
 * @param profile profile struct
 * @param output output buffer
 * @param key key in profile
 * @param klen key length
 */
size_t co_profile_get_str(co_obj_t *profile, char **output, const char *key, const size_t klen);

/**
 * @brief sets a specified profile value (if an int)
 * @param profile profile struct
 * @param key key in profile
 * @param klen key length
 * @param value key's value
 */
int co_profile_set_int(co_obj_t *profile, const char *key, const size_t klen, const signed long value);

/**
 * @brief returns the key value (if an int) from the profile. If no value set, returns the default value
 * @param profile profile struct
 * @param key key in profile
 * @param klen key length
 */
signed long co_profile_get_int(co_obj_t *profile, const char *key, const size_t klen);

/**
 * @brief sets a specified profile value (if an unsigned int)
 * @param profile profile struct
 * @param key key in profile
 * @param klen key length
 * @param value key's value
 */
int co_profile_set_uint(co_obj_t *profile, const char *key, const size_t klen, const unsigned long value);

/**
 * @brief returns the key value (if an unsigned int) from the profile. If no value set, returns the default value
 * @param profile profile struct
 * @param key key in profile
 * @param klen key length
 */
unsigned long co_profile_get_uint(co_obj_t *profile, const char *key, const size_t klen);

/**
 * @brief sets a specified profile value (if a float)
 * @param profile profile struct
 * @param key key in profile
 * @param klen key length
 * @param value key's value
 */
int co_profile_set_float(co_obj_t *profile, const char *key, const size_t klen, const double value);

/**
 * @brief returns the key value (if a float) from the profile. If no value set, returns the default value
 * @param profile profile struct
 * @param key key in profile
 * @param klen key length
 */
double co_profile_get_float(co_obj_t *profile, const char *key, const size_t klen);

/**
 * @brief exports a profile in memory to a file
 * @param profile profile struct
 * @param path export path
 */
int co_profile_export_file(co_obj_t *profile, const char *path);

/**
 * @brief processes all loaded profiles with specified iterator function
 * @param iter iterator function callback
 * @param context additional parameters for iterator
 */
co_obj_t *co_profiles_process(co_iter_t iter, void *context);
#endif
