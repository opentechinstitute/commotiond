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

#ifndef _PROFILE_H
#define _PROFILE_H

#include <stdlib.h>
#include <stddef.h>
#include "obj.h"

typedef struct co_cbptr_t co_cbptr_t;

/**
 * @struct co_profile_t a linked list for profile structs
 * @param name name of the struct
 * @param profile pointer to a profile struct
 */
struct co_cbptr_t {
  co_obj_t _header;
  uint8_t _exttype;
  uint8_t _len;
  co_cb_t cb;
};

typedef struct co_profile_t co_profile_t;

/**
 * @struct co_profile_t a linked list for profile structs
 * @param name name of the struct
 * @param profile pointer to a profile struct
 */
struct co_profile_t {
  co_obj_t _header;
  uint8_t _exttype;
  uint8_t _len;
  co_obj_t *name; /**< command name */
  co_obj_t *data;
};

co_obj_t *co_schema_create(co_cb_t cb);
int co_schema_register(co_cb_t cb);
int co_schemas_load(co_obj_t *profile);
void co_profiles_destroy(void);
int co_profiles_create(const size_t index_size);

/**
 * @brief creates a list of available profiles
 */
int co_profiles_create(const size_t index_size);

/**
 * @brief removes the list of available profiles
 */
void co_profiles_destroy(void);

/**
 * @brief imports available profiles from profiles directory
 * @param path file path to the profiels directory
 */
int co_profile_import_files(const char *path);

//int profile_export_file(tst_t *profile, const char *path);

//int profile_export(tst_t *profile, const char *path, const int length);

/**
 * @brief sets a specified profile according to configuration file
 * @param profile profile struct
 * @param key key in profile
 * @param value key's value
 */
int co_profile_set(co_profile_t *profile, const char *key, const char *value);

/**
 * @brief returns the key value (if an int) from the profile. If no value set, returns the default value
 * @param profile profile struct
 * @param key key in profile
 * @param def default key value
 */
signed long co_profile_get_int(co_profile_t *profile, const char *key, const size_t klen);

/**
 * @brief returns the key value (if a string) from the profile. If no value set, returns the default value
 * @param profile profile struct
 * @param key key in profile
 * @param def default key value
 */
char *co_profile_get_string(co_profile_t *profile, const char *key, char *def);

/**
 * @brief returns list of available profiles
 */
char *co_list_profiles(void);

/**
 * @brief searches the profile list for a specified profile
 * @param name profile name (search key)
 */
co_profile_t *co_profile_find(co_obj_t *name);

/**
 * @brief dumps profile data 
 * @param profile profile struct
 */
void co_profile_dump(co_profile_t *profile);

int co_profile_set_str(co_profile_t *profile, const char *key, const size_t klen, const char *value, const size_t vlen);
size_t co_profile_get_str(co_profile_t *profile, char *output, const char *key, const size_t klen);
int co_profile_set_int(co_profile_t *profile, const char *key, const size_t klen, const signed long value);
signed long co_profile_get_int(co_profile_t *profile, const char *key, const size_t klen);
int co_profile_set_uint(co_profile_t *profile, const char *key, const size_t klen, const unsigned long value);
unsigned long co_profile_get_uint(co_profile_t *profile, const char *key, const size_t klen);
int co_profile_set_float(co_profile_t *profile, const char *key, const size_t klen, const double value);
double co_profile_get_float(co_profile_t *profile, const char *key, const size_t klen);
#endif
