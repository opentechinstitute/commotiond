/* vim: set ts=2 expandtab: */
/**
 *       @file  plugin.h
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

#ifndef _PLUGIN_H
#define _PLUGIN_H

#include <stdlib.h>

#define MAX_PLUGINS 32

typedef int (*hook_t)(void);

typedef struct {
  const char *name;
  const char *description;
  const char *filename;
  void *handle;
} co_plugin_t;

int co_plugins_create(void);

int co_plugins_load(const char *dir_path);

int co_plugins_call(const char *hook);

#endif
