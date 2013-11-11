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
#include "extern/tst.h"
#include "debug.h"
#include "util.h"
#include "plugin.h"

static tst_t* plugins = NULL;

static int _co_plugins_load_i(const char *path, const char *filename) {
  dlerror(); //Clear existing error codes.
  void *plugin = dlopen(path, RTLD_NOW);
  CHECK(plugin != NULL, "Failed to load plugin %s: %s", path, dlerror());

  //TODO: API version checks.
  
  plugins = tst_insert(new_profile->profile, (char *)key_copy, strlen(key_copy), (void *)value_copy);
  return 1; 
error:
  return 0;
}

static void _co_plugins_call_i(list_t *list, lnode_t *lnode, void *context) {
  void *plugin = lnode_get(lnode);
  char *hook_name = context;
  hook_t hook_handle = dlsym(plugin, hook_name); 

  hook_handle();
  return;
}

int co_plugins_load(const char *dir_path) {
  return process_files(dir_path, _co_plugins_load_i);
}

int co_plugins_call(const char *hook) {
  CHECK_MEM(hook);
  list_process(plugins, (void *)hook, _co_plugins_call_i);
  return 1;
error:
  return 0;
}

