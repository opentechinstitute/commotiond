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
#include "extern/jsmn.h"

#define DEFAULT_TOKENS 128

static co_obj_t *_profiles = NULL;
static co_obj_t *_schemas = NULL;
static co_obj_t *_profile_global = NULL;
static co_obj_t *_schemas_global = NULL;

static co_obj_t *
_co_schema_create(co_cb_t cb)
{
  DEBUG("Registering schema callback %p.", cb);
  co_cbptr_t *schema = NULL;
  CHECK_MEM(schema = h_calloc(1, sizeof(co_cbptr_t)));
  schema->_header._type = _ext8;
  schema->_header._ref = 0;
  schema->_header._flags = 0;
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
  DEBUG("Registering schema.");
  CHECK(co_list_append(_schemas, _co_schema_create(cb)), "Failed to register schema.");
  return 1;
error:
  return 0;
}

int
co_schema_register_global(co_cb_t cb)
{
  DEBUG("Registering global schema.");
  CHECK(co_list_append(_schemas_global, _co_schema_create(cb)), "Failed to register schema.");
  return 1;
error:
  return 0;
}

static co_obj_t *
_co_schemas_load_i(co_obj_t *list, co_obj_t *current, void *context) 
{
  if(IS_CBPTR(current) && IS_PROFILE(context) && (((co_cbptr_t *)current)->cb != NULL))
  {
    DEBUG("Running schema callback.");
    ((co_cbptr_t *)current)->cb(((co_profile_t *)context)->data, NULL, NULL);
  }
  return NULL;
}

static int
_co_schemas_load(co_obj_t *profile, co_obj_t *schemas) 
{
  CHECK(IS_PROFILE(profile), "Not a valid search index.");
  CHECK(schemas != NULL, "Schemas not initialized.");
  co_list_parse(schemas, _co_schemas_load_i, profile);
  return 1;
error:
  return 0;
}

void 
co_profiles_shutdown(void) 
{
  if(_profiles != NULL) co_obj_free(_profiles);
  if(_profile_global != NULL) co_obj_free(_profile_global);
  return;
}

static co_obj_t *
_co_profile_create(const char *name, const size_t nlen) 
{
  DEBUG("Creating profile %s", name);
  co_profile_t *profile = h_calloc(1, sizeof(co_profile_t));
  CHECK_MEM(profile->name = co_str8_create(name, nlen, 0));
  hattach(profile->name, profile);
  CHECK_MEM(profile->data = co_tree16_create());
  hattach(profile->data, profile);
  profile->_exttype = _profile;
  profile->_header._type = _ext8;
  profile->_header._ref = 0;
  profile->_header._flags = 0;
  profile->_len = (sizeof(co_obj_t *) * 2);
  return (co_obj_t *)profile;
error:
  DEBUG("Failed to create profile %s.", name);
  return NULL;
}

int
co_profile_remove(const char *name, const size_t nlen)
{
  co_obj_t *n = co_str8_create(name, nlen, 0);
  co_obj_t *prof = co_profile_find(n);
  prof = co_list_delete(_profiles, prof);
  CHECK(prof != NULL, "Failed to remove profile.");

  co_obj_free(n);
  return 1;
error:
  co_obj_free(n);
  return 0;
}

int
co_profile_add(const char *name, const size_t nlen)
{
  co_obj_t *new_profile = _co_profile_create(name, nlen);
  CHECK(_co_schemas_load(new_profile, _schemas), "Failed to initialize profile with schema.");
  CHECK(co_list_append(_profiles, new_profile), "Failed to add profile to list.");

  return 1;
error:
  co_obj_free(new_profile);
  return 0;
}

int 
co_profiles_init(const size_t index_size) 
{
  if(index_size == 16)
  {
    CHECK((_profiles = (co_obj_t *)co_list16_create()) != NULL, "Profile list creation failed.");
  }
  else if(index_size == 32)
  {
    CHECK((_profiles = (co_obj_t *)co_list32_create()) != NULL, "Profile list creation failed.");
  }
  else SENTINEL("Invalid list index size.");

  if(_schemas == NULL)
  {
    CHECK((_schemas = (co_obj_t *)co_list16_create()) != NULL, "Schema list creation failed.");
  }

  if(_schemas_global == NULL)
  {
    CHECK((_schemas_global = (co_obj_t *)co_list16_create()) != NULL, "Global schema list creation failed.");
  }

  if(_profile_global == NULL)
  {
    CHECK((_profile_global = _co_profile_create("global", sizeof("global"))) != NULL, "Global profile creation failed."); 
  }
  return 1;

error:
  co_profiles_shutdown();
  return 0;
}

static jsmntok_t *_co_json_string_tokenize(const char *js)
{
  jsmn_parser parser;
  jsmn_init(&parser);

  unsigned int t = DEFAULT_TOKENS;
  jsmntok_t *tokens = h_calloc(t, sizeof(jsmntok_t));
  CHECK_MEM(tokens);

  int ret = jsmn_parse(&parser, js, tokens, t);

  while (ret == JSMN_ERROR_NOMEM)
  {
      t = t * 2 + 1;
      tokens = h_realloc(tokens, sizeof(jsmntok_t) * t);
      CHECK_MEM(tokens);
      ret = jsmn_parse(&parser, js, tokens, t);
  }

  CHECK(ret != JSMN_ERROR_INVAL, "Invalid JSON.");
  CHECK(ret != JSMN_ERROR_PART, "Incomplete JSON.");

  return tokens;
error:
  if(tokens != NULL) h_free(tokens);
  return NULL;
}

static char *_co_json_token_stringify(char *json, const jsmntok_t *token)
{
  json[token->end] = '\0';
  return json + token->start;
}

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
  jsmntok_t *tokens = _co_json_string_tokenize(buffer);

  typedef enum { START, KEY, VALUE, STOP } parse_state;
  parse_state state = START;

  size_t object_tokens = 0;
  co_obj_t *new_profile = _co_profile_create(filename, strlen(filename) + 1);
  CHECK(_co_schemas_load(new_profile, _schemas), "Failed to initialize profile with schema.");
  char *key = NULL;
  size_t klen = 0;

  for (size_t i = 0, j = 1; j > 0; i++, j--)
  {
    jsmntok_t *t = &tokens[i];

    // Should never reach uninitialized tokens
    CHECK(t->start != -1 && t->end != -1, "Tokens uninitialized.");

    if (t->type == JSMN_ARRAY || t->type == JSMN_OBJECT)
      j += t->size;

    switch (state)
    {
      case START:
        CHECK(t->type == JSMN_OBJECT, "Invalid root element.");

        state = KEY;
        object_tokens = t->size;

        if (object_tokens == 0)
          state = STOP;

        CHECK(object_tokens % 2 == 0, "Object must have even number of children.");
        break;

      case KEY:
        object_tokens--;

        CHECK(t->type == JSMN_STRING, "Keys must be strings.");
        state = VALUE;
        key = _co_json_token_stringify(buffer, t);
        klen = t->end - t->start;

        break;

      case VALUE:
        CHECK(t->type == JSMN_STRING, "Values must be strings.");

        if(key != NULL && klen > 0)
        {
          if(!co_profile_set_str(new_profile, key, klen + 1, _co_json_token_stringify(buffer, t), t->end - t->start + 1))
          {
            INFO("Value not in schema.");
          }
          
        }

        key = NULL;
        klen = 0;
        object_tokens--;
        state = KEY;

        if (object_tokens == 0)
          state = STOP;

        break;

      case STOP:
        // Just consume the tokens
        break;

      default:
        SENTINEL("Invalid state %u", state);
    }
  }

  co_list_append(_profiles, new_profile);

  return 1;

error:
  if(config_file != NULL) fclose(config_file);
  if(new_profile != NULL) co_obj_free(new_profile);
  return 0;
}

int co_profile_import_files(const char *path) {
  DEBUG("Importing files from %s", path);
  CHECK(process_files(path, _co_profile_import_files_i), "Failed to load all profiles.");

  return 1;

error:
  return 0;
}

int co_profile_import_global(const char *path) {
  FILE *config_file = NULL;

  if(_profile_global == NULL)
  {
    _profile_global = _co_profile_create("global", sizeof("global"));
  }
  _co_schemas_load(_profile_global, _schemas_global);

  DEBUG("Importing file at path %s", path);

  config_file = fopen(path, "rb");
  CHECK(config_file != NULL, "File %s could not be opened", path);
  fseek(config_file, 0, SEEK_END);
  long fsize = ftell(config_file);
  rewind(config_file);
  char *buffer = h_calloc(1, fsize + 1);
  CHECK(fread(buffer, fsize, 1, config_file) != 0, "Failed to read from file.");
  fclose(config_file);
  
  buffer[fsize] = '\0';
  jsmntok_t *tokens = _co_json_string_tokenize(buffer);

  typedef enum { START, KEY, VALUE, STOP } parse_state;
  parse_state state = START;

  size_t object_tokens = 0;
  char *key = NULL;
  size_t klen = 0;

  for (size_t i = 0, j = 1; j > 0; i++, j--)
  {
    jsmntok_t *t = &tokens[i];

    // Should never reach uninitialized tokens
    CHECK(t->start != -1 && t->end != -1, "Tokens uninitialized.");

    if (t->type == JSMN_ARRAY || t->type == JSMN_OBJECT)
      j += t->size;

    switch (state)
    {
      case START:
        CHECK(t->type == JSMN_OBJECT, "Invalid root element.");

        state = KEY;
        object_tokens = t->size;

        if (object_tokens == 0)
          state = STOP;

        CHECK(object_tokens % 2 == 0, "Object must have even number of children.");
        break;

      case KEY:
        object_tokens--;

        CHECK(t->type == JSMN_STRING, "Keys must be strings.");
        state = VALUE;
        key = _co_json_token_stringify(buffer, t);
        klen = t->end - t->start;

        break;

      case VALUE:
        CHECK(t->type == JSMN_STRING, "Values must be strings.");

        if(key != NULL && klen > 0)
        {
          if(!co_profile_set_str(_profile_global, key, klen + 1, _co_json_token_stringify(buffer, t), t->end - t->start + 1))
          {
            INFO("Value not in schema.");
          }
          
        }

        key = NULL;
        klen = 0;
        object_tokens--;
        state = KEY;

        if (object_tokens == 0)
          state = STOP;

        break;

      case STOP:
        // Just consume the tokens
        break;

      default:
        SENTINEL("Invalid state %u", state);
    }
  }

  return 1;

error:
  if(config_file != NULL) fclose(config_file);
  return 0;
}

co_obj_t *
co_profile_get(co_obj_t *profile, const co_obj_t *key) 
{
  CHECK(IS_PROFILE(profile),"Not a profile.");
  CHECK_MEM(((co_profile_t*)profile)->data);
  CHECK_MEM(key);
  char *kstr = NULL;
  size_t klen = co_obj_data(&kstr, key);
  co_obj_t *obj = NULL;
  CHECK((obj = co_tree_find(((co_profile_t*)profile)->data, kstr, klen)) != NULL, "Failed to find key %s.", kstr);
  return obj;

error:
  return NULL;
}

int 
co_profile_set_str(co_obj_t *profile, const char *key, const size_t klen, const char *value, const size_t vlen) 
{
    CHECK(IS_PROFILE(profile),"Not a profile.");
    CHECK(co_tree_set_str(((co_profile_t*)profile)->data, key, klen, value, vlen), 
            "No corresponding key %s in schema, can't set %s:%s",
            key, key, value);
    return 1;

error:
    return 0;
}

size_t
co_profile_get_str(co_obj_t *profile, char **output, const char *key, const size_t klen) 
{
  CHECK(IS_PROFILE(profile),"Not a profile.");
  CHECK_MEM(((co_profile_t*)profile)->data);
  CHECK_MEM(key);
  co_obj_t *obj = NULL;
  CHECK((obj = co_tree_find(((co_profile_t*)profile)->data, key, klen)) != NULL, "Failed to find key %s.", key);
  CHECK(IS_STR(obj), "Object is not a string.");
  return co_obj_data((char **)output, obj);

error:
  return -1;
}

int 
co_profile_set_int(co_obj_t *profile, const char *key, const size_t klen, const signed long value) 
{
  CHECK(IS_PROFILE(profile),"Not a profile.");
  CHECK(co_tree_set_int(((co_profile_t*)profile)->data, key, klen, value), 
            "No corresponding key %s in schema, can't set %s:%ld",
            key, key, value);
    return 1;

error:
    return 0;
}

signed long
co_profile_get_int(co_obj_t *profile, const char *key, const size_t klen) 
{
  CHECK(IS_PROFILE(profile),"Not a profile.");
  CHECK_MEM(((co_profile_t*)profile)->data);
  CHECK_MEM(key);
  co_obj_t *obj = NULL;
  CHECK((obj = co_tree_find(((co_profile_t*)profile)->data, key, klen)) != NULL, "Failed to find key %s.", key);
  CHECK(IS_INT(obj), "Object is not a signed integer.");
  signed long *output;
  CHECK(co_obj_data((char **)&output, obj) >= 0, "Failed to read data from %s.", key);
  return *output;

error:
  return -1;
}

int 
co_profile_set_uint(co_obj_t *profile, const char *key, const size_t klen, const unsigned long value) 
{
    CHECK(IS_PROFILE(profile),"Not a profile.");
    CHECK(co_tree_set_uint(((co_profile_t*)profile)->data, key, klen, value), 
            "No corresponding key %s in schema, can't set %s:%lu",
            key, key, value);
    return 1;

error:
    return 0;
}

unsigned long
co_profile_get_uint(co_obj_t *profile, const char *key, const size_t klen) 
{
  CHECK(IS_PROFILE(profile),"Not a profile.");
  CHECK_MEM(((co_profile_t*)profile)->data);
  CHECK_MEM(key);
  co_obj_t *obj = NULL;
  CHECK((obj = co_tree_find(((co_profile_t*)profile)->data, key, klen)) != NULL, "Failed to find key %s.", key);
  CHECK(IS_UINT(obj), "Object is not an unsigned integer.");
  unsigned long *output;
  CHECK(co_obj_data((char **)&output, obj) >= 0, "Failed to read data from %s.", key);
  return *output;

error:
  return -1;
}

int 
co_profile_set_float(co_obj_t *profile, const char *key, const size_t klen, const double value) 
{
    CHECK(IS_PROFILE(profile),"Not a profile.");
    CHECK(co_tree_set_float(((co_profile_t*)profile)->data, key, klen, value), 
            "No corresponding key %s in schema, can't set %s:%lf",
            key, key, value);
    return 1;

error:
    return 0;
}

double
co_profile_get_float(co_obj_t *profile, const char *key, const size_t klen) 
{
  CHECK(IS_PROFILE(profile),"Not a profile.");
  CHECK_MEM(((co_profile_t*)profile)->data);
  CHECK_MEM(key);
  co_obj_t *obj = NULL;
  CHECK((obj = co_tree_find(((co_profile_t*)profile)->data, key, klen)) != NULL, "Failed to find key %s.", key);
  CHECK(IS_FLOAT(obj), "Object is not a floating point value.");
  double *output;
  CHECK(co_obj_data((char **)&output, obj) >= 0, "Failed to read data from %s.", key);
  return *output;

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
  if (IS_PROFILE(current))
    if(!co_str_cmp(((co_profile_t *)current)->name, context)) return current;
  return NULL;
}

co_obj_t *
co_profile_find(co_obj_t *name) 
{
  CHECK(IS_STR(name), "Not a valid search index.");
  co_obj_t *result = NULL;
  CHECK((result = co_list_parse(_profiles, _co_profile_find_i, name)) != NULL, "Failed to find profile.");
  return result;
error:
  return NULL;
}

void
co_profile_delete_global(void) 
{
  co_obj_free(_profile_global);
  return;
}

co_obj_t *
co_profile_global(void) 
{
  return _profile_global;
}

static inline void
_co_profile_export_file_r(co_obj_t *tree, _treenode_t *current, int *count, FILE *config_file)
{
  if(current == NULL) return;
  CHECK(IS_TREE(tree), "Recursion target is not a tree.");
  char *key = NULL;
  char *value = NULL;
  if(co_node_value(current) != NULL)
  {
    (*count)++;
    co_obj_data(&key, co_node_key(current));
    co_obj_data(&value, co_node_value(current));
    //CHECK(IS_STR(key) && IS_STR(value), "Incorrect types for profile.");
    if(*count < co_tree_length(tree))
    {
      fprintf(config_file, "  \"%s\": \"%s\",\n", key, value);
    }
    else
    {
      fprintf(config_file, "  \"%s\": \"%s\"\n", key, value);
    }
  }
  _co_profile_export_file_r(tree, current->low, count, config_file); 
  _co_profile_export_file_r(tree, current->equal, count, config_file); 
  _co_profile_export_file_r(tree, current->high, count, config_file); 
  return; 
error:
  return;
}

int 
co_profile_export_file(co_obj_t *profile, const char *path)
{
  CHECK(IS_PROFILE(profile),"Not a profile.");
  int count = 0;
  FILE *config_file = fopen(path, "wb");
  CHECK(config_file != NULL, "Config file %s could not be opened", path);

  fprintf(config_file, "{\n");

  _co_profile_export_file_r(((co_profile_t*)profile)->data, co_tree_root(((co_profile_t*)profile)->data), &count, config_file);

  fprintf(config_file, "}");
  fclose (config_file); 
  return 1;
error:
  return 0;
}

co_obj_t *
co_profiles_process(co_iter_t iter, void *context)
{
  return co_list_parse(_profiles, iter, context);
}
