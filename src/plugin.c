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
#include "obj.h"
#include "cmd.h"
#include "list.h"
#include "db.h"
#include "debug.h"
#include "util.h"
#include "plugin.h"

static co_obj_t* _plugins = NULL;


static co_obj_t *
_co_plugins_destroy_i(co_obj_t *data, co_obj_t *current, void *context)
{
  if(((co_plugin_t *)current)->shutdown != NULL)
  {
    ((co_plugin_t *)current)->shutdown(current, NULL);
  }
  dlclose(((co_plugin_t *)current)->handle);
  co_list_decrement(data);
  return NULL;
}

int
co_plugins_destroy(void)
{
  co_list_parse(_plugins, _co_plugins_destroy_i, NULL);
  CHECK(co_list_length(_plugins) == 0, "Failed to shutdown all plugins.");
  co_obj_free(_plugins);
  return 1;
error:
  co_obj_free(_plugins);
  return 0;
}

int 
co_plugins_create(size_t index_size)
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

  CHECK(co_db_insert("plugins", 7, _plugins), "Failed to add plugin list to DB.");

  return 1;

error:
  co_plugins_destroy();
  return 0;
}

static int _co_plugins_load_i(const char *path, const char *filename) {
  dlerror(); //Clear existing error codes.
  void *handle = dlopen(path, RTLD_NOW);
  CHECK(handle != NULL, "Failed to load plugin %s: %s", path, dlerror());
  co_cb_t _name = dlsym(handle, "_name");
  CHECK(_name != NULL, "Failed to name plugin %s: %s", path, dlerror());
  co_cb_t _init = dlsym(handle, "_init");
  CHECK(_init != NULL, "Failed to init plugin %s: %s", path, dlerror());
  co_cb_t _shutdown = dlsym(handle, "_shutdown");

  //TODO: API version checks.
  co_plugin_t *plugin = h_calloc(1, sizeof(co_plugin_t));
  plugin->handle = handle;
  
  CHECK_MEM(plugin->name = _name(NULL, NULL));
  hattach(plugin->name, plugin);
  CHECK_MEM(plugin->filename = co_str8_create(filename, strlen(filename), 0));
  hattach(plugin->filename, plugin);
  if(_shutdown != NULL) plugin->shutdown = _shutdown;
  plugin->_len = (sizeof(co_obj_t *) * 4);
  plugin->_exttype = _plug;
  plugin->_header._type = _ext8;
  _init((co_obj_t *)plugin, NULL);
  
  co_list_append(_plugins, (co_obj_t *)plugin);
  return 1; 
error:
  if(handle != NULL) dlclose(handle);
  if(plugin != NULL) co_obj_free((co_obj_t *)plugin);
  return 0;
}

int co_plugins_load(const char *dir_path) {
  return process_files(dir_path, _co_plugins_load_i);
}
