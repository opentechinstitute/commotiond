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


typedef struct {
  co_obj_t _header;
  uint8_t _exttype;
  uint8_t _len;
  co_obj_t *name; /**< command name */
  co_obj_t *filename;
  co_cb_t shutdown;
  void *handle;
} co_plugin_t;

int co_plugins_shutdown(void);

int co_plugins_init(size_t index_size);

int co_plugins_load(const char *dir_path);

#endif
