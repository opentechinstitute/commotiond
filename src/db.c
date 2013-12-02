/* vim: set ts=2 expandtab: */
/**
 *       @file  db.c
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
#include <stdbool.h>
#include "debug.h"
#include "util.h"
#include "obj.h"
#include "tree.h"
#include "db.h"

static co_obj_t *_db = NULL;

void 
co_db_destroy(void) 
{
  if(_db != NULL) {
    co_obj_free(_db);
  }
  return;
}

int 
co_db_create(size_t index_size) 
{
  if(index_size == 16)
  {
    CHECK((_db = (co_obj_t *)co_tree16_create()) != NULL, "DB creation failed.");
  }
  else if(index_size == 32)
  {
    CHECK((_db = (co_obj_t *)co_tree32_create()) != NULL, "DB creation failed.");
  }
  else SENTINEL("Invalid DB index size.");

  return 1;

error:
  co_db_destroy();
  return 0;
}

int
co_db_insert(const char *key, const size_t klen, co_obj_t *value)
{
  CHECK_MEM(_db);
  CHECK(co_tree_insert(_db, key, klen, value), "Failed to add value to db.");
  return 1;
error:
  return 0;
}

co_obj_t *
co_db_find(const char *key, const size_t klen)
{
  co_obj_t *obj = NULL;
  CHECK_MEM(_db);
  CHECK((obj = co_tree_find(_db, key, klen)) != NULL, "Failed to find key.");
  return obj;
error:
  return NULL;
}

co_obj_t *
co_db_namespace_find(const char *namespace, const size_t nlen, const char *key, const size_t klen)
{
  _treenode_t *node = NULL;
  CHECK_MEM(_db);
  node = co_tree_root(_db);
  CHECK((node = co_tree_find_node(node, namespace, nlen)) != NULL, "Failed to find namespace.");
  CHECK((node = co_tree_find_node(node, key, klen)) != NULL, "Failed to find key.");
  return co_node_value(node);
error:
  return NULL;
}

int
co_db_set_str(const char *key, const size_t klen, const char *value, const size_t vlen)
{
  CHECK_MEM(_db);
  CHECK(((key != NULL) && (klen > 0)), "Null key or invalid key length.");
  if(vlen != 0)
  {
    CHECK((value != NULL && vlen > 0), "Null value or invalid value length.");
  }

  CHECK(co_tree_set_str(_db, key, klen, value, vlen), "Failed to set string.");
  
  return 1;
error:
  return 0;
}

size_t
co_db_get_str(char *output, const char *key, const size_t klen)
{
  CHECK_MEM(_db);
  CHECK(((key != NULL) && (klen > 0)), "Null key or invalid key length.");

  co_obj_t *str = co_tree_find(_db, key, klen);
  CHECK(str != NULL, "Failed to get object.");
  return co_obj_data(output, str);
error:
  return -1;
}
/*
static int _co_db_import_files_i(const char *path, const char *filename) {
  char key[80];
  char value[80];
  char line[80];
  char path_tmp[PATH_MAX] = {};
  int line_number = 1;
  FILE *db_file = NULL;

  DEBUG("Importing file %s at path %s", filename, path);

  strlcpy(path_tmp, path, PATH_MAX);
  strlcat(path_tmp, "/", PATH_MAX);
  strlcat(path_tmp, filename, PATH_MAX);
  db_file = fopen(path_tmp, "r");
  CHECK(db_file != NULL, "db file %s/%s could not be opened", path, filename);

  co_obj_t *new_profile = (co_obj_t *)co_tree16_create();
  new_profile->name = strdup(filename);
  while(fgets(line, 80, config_file) != NULL) {
    if(strlen(line) > 1) {
      char *key_copy, *value_copy;
      sscanf(line, "%[^=]=%[^\n]", (char *)key, (char *)value);

      key_copy = (char*)calloc(strlen(key)+1, sizeof(char));
      value_copy = (char*)calloc(strlen(value)+1, sizeof(char));
      strcpy(key_copy, key);
      strcpy(value_copy, value);

      DEBUG("Inserting key: %s and value: %s into db DB.", key_copy, value_copy);
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

int co_db_import_files(const char *path) {
  DEBUG("Importing files from %s", path);
  CHECK(process_files(path, _co_db_import_files_i), "Failed to load all profiles.");

  return 1;

error:
  return 0;
}

*/

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

