/*! 
 *
 * \file profile.c 
 *
 * \brief a key-value store for loading configuration information
 *
 * \author Josh King <jking@chambana.net>
 * 
 * \date
 *
 * \copyright This file is part of Commotion, Copyright(C) 2012-2013 Josh King
 * 
 *            Commotion is free software: you can redistribute it and/or modify
 *            it under the terms of the GNU General Public License as published 
 *            by the Free Software Foundation, either version 3 of the License, 
 *            or (at your option) any later version.
 * 
 *            Commotion is distributed in the hope that it will be useful,
 *            but WITHOUT ANY WARRANTY; without even the implied warranty of
 *            MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *            GNU General Public License for more details.
 * 
 *            You should have received a copy of the GNU General Public License
 *            along with Commotion.  If not, see <http://www.gnu.org/licenses/>.
 * 
 * */

#include <stdlib.h>
#include <stddef.h>
#include <stdio.h>
#include "extern/tst.h"
#include "debug.h"
#include "util.h"
#include "profile.h"

static list_t *profiles = NULL;

int profile_import_files_i(const char *path) {
  char key[80];
  char value[80];
  int line_number = 1;
  int strings = 0;
  FILE *config_file = NULL;
  tst_t *config_tree = NULL;

  config_file = fopen(path, "r");
  CHECK(config_file != NULL, "Config file %s could not be opened", path);

  while((strings = fscanf(config_file, "%s %s", (char *)key, (char *)value)) != EOF) {
    CHECK(strings == 2, "Line %d of %s is not valid syntax.", line_number, path);

    config_tree = tst_insert(config_tree, (char *)key, strlen(key), (void *)value);
    CHECK(config_tree != NULL, "Could not load line %d of %s.", line_number, path);

    line_number++;
  }

  fclose(config_file);
  list_append(profiles, lnode_create((void *)config_tree));

  return 1;

error:
  if(config_file) fclose(config_file);
  if(config_tree) tst_destroy(config_tree);
  return 0;
}

int profile_import_files(const char *path) {
  CHECK(process_files(path, profile_import_files_i), "Failed to load all profiles.");

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

int profile_set(tst_t *profile, const char *key, const char *value) {
    char *value_str = NULL;
    CHECK(!tst_search(profile, key, strlen(key)), 
            "Setting key %s already exists, can't add %s:%s",
            key, key, value);

    value_str = strdup(value);
    profile = tst_insert(profile, key,
            strlen(key), value_str);

    free(value_str);
    return 1;

error:
    free(value_str);
    return 0;
}

int profile_get_int(tst_t *profile, const char *key, const int def) {
  char *value = tst_search(profile, key, strlen(key));

  if(value) {
    return atoi((const char *)value);
  } else {
    return def;
  }
}

char *profile_get_string(tst_t *profile, const char *key, char *def) {
  CHECK_MEM(profile);
  char *value = tst_search(profile, key, strlen(key));

  return value == NULL ? def : value;

error:
  return NULL;
}
