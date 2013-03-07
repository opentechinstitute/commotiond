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
#include "extern/tst.h"
#include "debug.h"
#include "util.h"
#include "profile.h"

static list_t *profiles = NULL;

int co_profiles_create(void) {
  CHECK((profiles = list_create(PROFILES_MAX)) != NULL, "Profile loader creation failed, clearing lists.");
  return 1;

error:
  list_destroy(profiles);
  free(profiles);
  return 0;
}

int co_profiles_destroy(void) {
  if(profiles != NULL) {
    //list_process(profiles, NULL, _co_loop_destroy_profile_i);
    list_destroy_nodes(profiles);
    list_destroy(profiles);
  }
  return 1;
}

static int _co_profile_find_i(const void *prof, const void *name) {
  const co_profile_t *this_profile = prof;
  const char *this_name = name;
  if((strcmp(this_profile->name, this_name)) == 0) return 0;
  return -1;
}

static int _co_profile_import_files_i(const char *path, const char *filename) {
  char key[80];
  char value[80];
  char line[80];
  char path_tmp[PATH_MAX];
  int line_number = 1;
  //int strings = 0;
  FILE *config_file = NULL;
  tst_t *config_tree = NULL;

  DEBUG("Importing file %s at path %s", filename, path);

  strlcpy(path_tmp, path, PATH_MAX);
  strlcat(path_tmp, filename, PATH_MAX);
  config_file = fopen(path_tmp, "r");
  CHECK(config_file != NULL, "Config file %s could not be opened", path);

  //while((strings = fscanf(config_file, "%[^=]= %[^\n]", (char *)key, (char *)value)) != EOF) {
  while(fgets(line, 80, config_file) != NULL) {
    if(strlen(line) > 1) {
      sscanf(line, "%[^=]=%[^\n]", (char *)key, (char *)value);
      DEBUG("Inserting key: %s and value: %s into profile tree.", key, value);
      config_tree = tst_insert(config_tree, (char *)key, strlen(key), (void *)value);
      CHECK(config_tree != NULL, "Could not load line %d of %s.", line_number, path);
      line_number++;
    }
  }

  fclose(config_file);
  co_profile_t *new_profile = malloc(sizeof(co_profile_t));
  new_profile->name = strdup(filename);
  new_profile->profile = config_tree;
  list_append(profiles, lnode_create((void *)new_profile));

  return 1;

error:
  if(config_file) fclose(config_file);
  if(config_tree) tst_destroy(config_tree);
  return 0;
}

int co_profile_import_files(const char *path) {
  DEBUG("Importing files from %s", path);
  CHECK(process_files(path, _co_profile_import_files_i), "Failed to load all profiles.");

  return 1;

error:
  return 0;
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

int co_profile_set(co_profile_t *profile, const char *key, const char *value) {
    char *value_str = NULL;
    CHECK(!tst_search(profile->profile, key, strlen(key)), 
            "Setting key %s already exists, can't add %s:%s",
            key, key, value);

    value_str = strdup(value);
    profile->profile = tst_insert(profile->profile, key,
            strlen(key), value_str);

    free(value_str);
    return 1;

error:
    free(value_str);
    return 0;
}

int co_profile_get_int(co_profile_t *profile, const char *key, const int def) {
  char *value = tst_search(profile->profile, key, strlen(key));

  if(value) {
    return atoi((const char *)value);
  } else {
    return def;
  }
}

char *co_profile_get_string(co_profile_t *profile, const char *key, char *def) {
  CHECK_MEM(profile);
  char *value = tst_search(profile->profile, key, strlen(key));

  return value == NULL ? def : value;

error:
  return NULL;
}

static void _co_list_profiles_i(list_t *list, lnode_t *lnode, void *context) {
  co_profile_t *profile = lnode_get(lnode);
  char *data = context;
  snprintf((char *)data, 1024, "%s\n", profile->name);
  return;
}

char *co_list_profiles(void) {
  char *ret = malloc(1024);
  list_process(profiles, (void *)ret, _co_list_profiles_i);
  return ret;
}

co_profile_t *co_profile_find(const char *name) {
  lnode_t *node;
  CHECK((node = list_find(profiles, name, _co_profile_find_i)) != NULL, "Failed to find profile %s!", name);
  return lnode_get(node);
error:
  return NULL;
}
