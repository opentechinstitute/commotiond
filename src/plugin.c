/* vim: set ts=2 expandtab: */
/**
 *       @file  plugin.c
 *      @brief  The commotiond plugin loader.
 *
 *     @author  Josh King (jheretic), jking@chambana.net
 *
 *   @internal
 *      Created  11/04/2013 10:47:37 AM
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

#include <stdlib.h>
#include <stdio.h>
#include <dirent.h>
#include <dlfcn.h>
#include "extern/list.h"
#include "debug.h"
#include "util.h"
#include "plugin.h"

static list_t* plugins = NULL;

int plugins_create(void) {
  CHECK_MEM(plugins = list_create(MAX_PLUGINS));
  return 1;
error:
  return 0;
}

static int plugins_load_i(const char *path) {
  void *plugin = dlopen(path, RTLD_NOW);
  CHECK(plugin != NULL, "Failed to load plugin %s: %s", path, dlerror());

  //TODO: API version checks.
  
  list_append(plugins, lnode_create(plugin));
  return 1; 
error:
  return 0;
}

static void plugins_call_i(list_t *list, lnode_t *lnode, void *context) {
  void *plugin = lnode_get(lnode);
  char *hook_name = context;
  hook_t hook_handle = dlsym(plugin, hook_name); 

  hook_handle();
  return;
}

int plugins_load_all(const char *dir_path) {
  return process_files(dir_path, plugins_load_i);
}

int plugins_call(const char *hook) {
  CHECK_MEM(hook);
  list_process(plugins, (void *)hook, plugins_call_i);
  return 1;
error:
  return 0;
}

