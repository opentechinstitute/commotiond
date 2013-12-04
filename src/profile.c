/* vim: set ts=2 expandtab: */
/**
 *       @file  profile.c
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

#include <stdlib.h>
#include <stddef.h>
#include <stdio.h>
#include <limits.h>
#include "obj.h"
#include "list.h"
#include "tree.h"
#include "debug.h"
#include "util.h"
#include "profile.h"

static co_obj_t *_profiles = NULL;
static co_obj_t *_schemas = NULL;

co_obj_t *
co_schema_create(co_cb_t cb)
{
  co_cbptr_t *schema = NULL;
  CHECK_MEM(schema = h_calloc(1, sizeof(co_cbptr_t)));
  schema->_header._type = _ext8;
  schema->_exttype = _cbptr;
  schema->_len = sizeof(co_cb_t *);
  schema->cb = cb;
  return (co_obj_t *)schema;
error:
  return NULL;
}

int
co_schema_register(co_cb_t cb)
{
  if(_schemas == NULL)
  {
    CHECK((_schemas = (co_obj_t *)co_list16_create()) != NULL, "Schema list creation failed.");
  }
  co_list_append(_schemas, co_schema_create(cb));
  return 1;
error:
  return 0;
}

static co_obj_t *
_co_schemas_load_i(co_obj_t *list, co_obj_t *current, void *context) 
{
  if(IS_CBPTR(current) && IS_PROFILE(context) && ((co_cbptr_t *)current)->cb != NULL)
  {
    ((co_cbptr_t *)current)->cb((co_obj_t *)context, NULL);
  }
  return NULL;
}

int
co_schemas_load(co_obj_t *profile) 
{
  CHECK(IS_TREE(profile), "Not a valid search index.");
  CHECK(_schemas != NULL, "Schemas not initialized.");
  co_list_parse(_schemas, _co_schemas_load_i, profile);
  return 1;
error:
  return 0;
}

void 
co_profiles_destroy(void) 
{
  if(_profiles != NULL) co_obj_free(_profiles);
  return;
}

int 
co_profiles_create(const size_t index_size) 
{
  if(index_size == 16)
  {
    CHECK((_profiles = (co_obj_t *)co_list16_create()) != NULL, "Plugin list creation failed.");
  }
  else if(index_size == 32)
  {
    CHECK((_profiles = (co_obj_t *)co_list32_create()) != NULL, "Plugin list creation failed.");
  }
  else SENTINEL("Invalid list index size.");

  return 1;

error:
  co_profiles_destroy();
  return 0;
}
/*
static int _co_profile_import_files_i(const char *path, const char *filename) {
  char path_tmp[PATH_MAX] = {};
  FILE *config_file = NULL;

  DEBUG("Importing file %s at path %s", filename, path);

  strlcpy(path_tmp, path, PATH_MAX);
  strlcat(path_tmp, "/", PATH_MAX);
  strlcat(path_tmp, filename, PATH_MAX);
  config_file = fopen(path_tmp, "rb");
  CHECK(config_file != NULL, "File %s/%s could not be opened", path, filename);
  fseek(config_file, 0, SEEK_END);
  long fsize = ftell(config_file);
  rewind(config_file);
  char *buffer = h_calloc(1, fsize + 1);
  CHECK(fread(buffer, fsize, 1, config_file) != 0, "Failed to read from file.");
  fclose(config_file);
  
  buffer[fsize] = '\0';

  co_profile_t *new_profile = calloc(1, sizeof(co_profile_t));
  new_profile->name = strdup(filename);
  while(fgets(line, 80, config_file) != NULL) {
    if(strlen(line) > 1) {
      char *key_copy, *value_copy;
      sscanf(line, "%[^=]=%[^\n]", (char *)key, (char *)value);

      key_copy = (char*)calloc(strlen(key)+1, sizeof(char));
      value_copy = (char*)calloc(strlen(value)+1, sizeof(char));
      strcpy(key_copy, key);
      strcpy(value_copy, value);

      DEBUG("Inserting key: %s and value: %s into profile tree.", key_copy, value_copy);
      new_profile->profile = tst_insert(new_profile->profile, (char *)key_copy, strlen(key_copy), (void *)value_copy);
      CHECK(new_profile->profile != NULL, "Could not load line %d of %s.", line_number, path);
      line_number++;
    }
  }

  fclose(config_file);
  list_append(profiles, lnode_create((void *)new_profile));

  return 1;

error:
  if(config_file) fclose(config_file);
  //if(new_profile != NULL) {
  //  tst_destroy(new_profile->profile);
  //  free(new_profile);
  //}
  return 0;
}

int co_profile_import_files(const char *path) {
  DEBUG("Importing files from %s", path);
  CHECK(process_files(path, _co_profile_import_files_i), "Failed to load all profiles.");

  return 1;

error:
  return 0;
}
*/

int 
co_profile_set_str(co_profile_t *profile, const char *key, const size_t klen, const char *value, const size_t vlen) 
{
    CHECK(co_tree_set_str(profile->data, key, klen, value, vlen), 
            "No corresponding key %s in schema, can't set %s:%s",
            key, key, value);
    return 1;

error:
    return 0;
}

size_t
co_profile_get_str(co_profile_t *profile, char *output, const char *key, const size_t klen) 
{
  CHECK_MEM(profile->data);
  CHECK_MEM(key);
  co_obj_t *obj = NULL;
  CHECK((obj = co_tree_find(profile->data, key, klen)) != NULL, "Failed to find key %s.", key);
  CHECK(IS_STR(obj), "Object is not a string.");
  return co_obj_data(output, obj);

error:
  return -1;
}

int 
co_profile_set_int(co_profile_t *profile, const char *key, const size_t klen, const signed long value) 
{
    CHECK(co_tree_set_int(profile->data, key, klen, value), 
            "No corresponding key %s in schema, can't set %s:%ld",
            key, key, value);
    return 1;

error:
    return 0;
}

signed long
co_profile_get_int(co_profile_t *profile, const char *key, const size_t klen) 
{
  CHECK_MEM(profile->data);
  CHECK_MEM(key);
  co_obj_t *obj = NULL;
  CHECK((obj = co_tree_find(profile->data, key, klen)) != NULL, "Failed to find key %s.", key);
  CHECK(IS_INT(obj), "Object is not a signed integer.");
  signed long output;
  CHECK(co_obj_data(&output, obj) >= 0, "Failed to read data from %s.", key);
  return output;

error:
  return -1;
}

int 
co_profile_set_uint(co_profile_t *profile, const char *key, const size_t klen, const unsigned long value) 
{
    CHECK(co_tree_set_uint(profile->data, key, klen, value), 
            "No corresponding key %s in schema, can't set %s:%lu",
            key, key, value);
    return 1;

error:
    return 0;
}

unsigned long
co_profile_get_uint(co_profile_t *profile, const char *key, const size_t klen) 
{
  CHECK_MEM(profile->data);
  CHECK_MEM(key);
  co_obj_t *obj = NULL;
  CHECK((obj = co_tree_find(profile->data, key, klen)) != NULL, "Failed to find key %s.", key);
  CHECK(IS_UINT(obj), "Object is not an unsigned integer.");
  unsigned long output;
  CHECK(co_obj_data(&output, obj) >= 0, "Failed to read data from %s.", key);
  return output;

error:
  return -1;
}

int 
co_profile_set_float(co_profile_t *profile, const char *key, const size_t klen, const double value) 
{
    CHECK(co_tree_set_float(profile->data, key, klen, value), 
            "No corresponding key %s in schema, can't set %s:%lf",
            key, key, value);
    return 1;

error:
    return 0;
}

double
co_profile_get_float(co_profile_t *profile, const char *key, const size_t klen) 
{
  CHECK_MEM(profile->data);
  CHECK_MEM(key);
  co_obj_t *obj = NULL;
  CHECK((obj = co_tree_find(profile->data, key, klen)) != NULL, "Failed to find key %s.", key);
  CHECK(IS_FLOAT(obj), "Object is not a floating point value.");
  double output;
  CHECK(co_obj_data(&output, obj) >= 0, "Failed to read data from %s.", key);
  return output;

error:
  return -1;
}
/*
static void _co_list_profiles_i(list_t *list, lnode_t *lnode, void *context) {
  co_profile_t *profile = lnode_get(lnode);
  char *data = context;
  strcat((char *)data, profile->name);
  return;
}

char *co_list_profiles(void) {
  char *ret = calloc(1024, sizeof(char));
  list_process(profiles, (void *)ret, _co_list_profiles_i);
  if(strlen(ret) > 0) {
    return ret;
  } else return NULL;
}
*/

static co_obj_t *
_co_profile_find_i(co_obj_t *list, co_obj_t *current, void *context) 
{
  if(!co_str_cmp(((co_profile_t *)current)->name, ((co_profile_t *)context)->name)) return current;
  return NULL;
}

co_profile_t *
co_profile_find(co_obj_t *name) 
{
  CHECK(IS_STR(name), "Not a valid search index.");
  co_obj_t *result = NULL;
  CHECK((result = co_list_parse(_profiles, _co_profile_find_i, name)) != NULL, "Failed to find profile.");
  return (co_profile_t *)result;
error:
  return NULL;
}

//int profile_export_file(tst_t *profile, const char *path) {
//  FILE *config_file = NULL;
//  struct bstrList *config_item = NULL;
//  tst_t *config_tree = NULL;
//
//  config_file = fopen(path, "a");
//  CHECK(config_file != NULL, "Config file %s could not be opened", path);
//
//  fprintf(config_file, "%s", string);
//
//  fclose (config_file); 
//  return 1;
//
//error:
//  return 0;
//}

