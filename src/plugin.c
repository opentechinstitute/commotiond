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
#include <stdio.h>
#include <dirent.h>
#include <dlfcn.h>
#include "obj.h"
#include "cmd.h"
#include "list.h"
#include "debug.h"
#include "util.h"
#include "plugin.h"

static co_obj_t* _plugins = NULL;


static co_obj_t *
_co_plugins_shutdown_i(co_obj_t *data, co_obj_t *current, void *context)
{
  if(((co_plugin_t *)current)->shutdown != NULL)
  {
    ((co_plugin_t *)current)->shutdown(current, NULL, NULL);
  }
  if(((co_plugin_t *)current)->handle != NULL)
  {
    dlclose(((co_plugin_t *)current)->handle);
  }
  co_list_delete(_plugins,current);
  co_obj_free(current);
  return NULL;
}

int
co_plugins_shutdown(void)
{
  co_list_parse(_plugins, _co_plugins_shutdown_i, NULL);
  CHECK(co_list_length(_plugins) == 0, "Failed to shutdown all plugins.");
  co_obj_free(_plugins);
  return 1;
error:
  co_obj_free(_plugins);
  return 0;
}

static co_obj_t *
_co_plugins_start_i(co_obj_t *data, co_obj_t *current, void *context)
{
  if (IS_PLUG(current))
    CHECK(((co_plugin_t *)current)->init(current, NULL, NULL),"Failed to start plugin %s",co_obj_data_ptr(((co_plugin_t *)current)->name));
  return NULL;
error:
  return current;
}

int
co_plugins_start(void)
{
  CHECK(co_list_parse(_plugins, _co_plugins_start_i, NULL) == NULL,"Failed to start all plugins");
  return 1;
error:
  return 0;
}

int 
co_plugins_init(size_t index_size)
{
  if(index_size == 16)
  {
    CHECK((_plugins = (co_obj_t *)co_list16_create()) != NULL, "Plugin list creation failed.");
  }
  else if(index_size == 32)
  {
    CHECK((_plugins = (co_obj_t *)co_list32_create()) != NULL, "Plugin list creation failed.");
  }
  else SENTINEL("Invalid list index size.");

  return 1;

error:
  co_plugins_shutdown();
  return 0;
}

static int _co_plugins_load_i(const char *path, const char *filename) {
  char path_tmp[PATH_MAX] = {};
  strlcpy(path_tmp, path, PATH_MAX);
  strlcat(path_tmp, "/", PATH_MAX);
  strlcat(path_tmp, filename, PATH_MAX);
  dlerror(); //Clear existing error codes.
  void *handle = dlopen(path_tmp, RTLD_GLOBAL|RTLD_NOW);
  CHECK(handle != NULL, "Failed to load plugin %s: %s", path_tmp, dlerror());
  co_cb_t co_plugin_name = dlsym(handle, "co_plugin_name");
  CHECK(co_plugin_name != NULL, "Failed to name plugin %s: %s", path_tmp, dlerror());
  co_cb_t co_plugin_register = dlsym(handle, "co_plugin_register");
  co_cb_t co_plugin_init = dlsym(handle, "co_plugin_init");
  CHECK(co_plugin_init != NULL, "Failed to init plugin %s: %s", path_tmp, dlerror());
  co_cb_t co_plugin_shutdown = dlsym(handle, "co_plugin_shutdown");

  //TODO: API version checks.
  co_plugin_t *plugin = h_calloc(1, sizeof(co_plugin_t));
  plugin->handle = handle;
  
  CHECK(co_plugin_name(NULL, &(plugin->name), NULL), "Failed to set plugin name.");
  hattach(plugin->name, plugin);
  CHECK_MEM(plugin->filename = co_str8_create(filename, strlen(filename)+1, 0));
  hattach(plugin->filename, plugin);
  if(co_plugin_shutdown != NULL) plugin->shutdown = co_plugin_shutdown;
  plugin->init = co_plugin_init;
  plugin->_len = (sizeof(co_plugin_t));
  plugin->_exttype = _plug;
  plugin->_header._type = _ext8;
  plugin->_header._ref = 0;
  plugin->_header._flags = 0;
  if(co_plugin_register != NULL) co_plugin_register((co_obj_t *)plugin, NULL, NULL);
  
  co_list_append(_plugins, (co_obj_t *)plugin);
  return 1; 
error:
  if(handle != NULL) dlclose(handle);
  if(plugin != NULL) co_obj_free((co_obj_t *)plugin);
  return 1;
}

int co_plugins_load(const char *dir_path) {
  return process_files(dir_path, _co_plugins_load_i);
}
