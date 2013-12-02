/* vim: set ts=2 expandtab: */
/**
 *       @file  command.c
 *      @brief  a mechanism for registering and controlling display of commands
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
#include <stdint.h>
#include "debug.h"
#include "obj.h"
#include "db.h"
#include "cmd.h"

co_obj_t *
co_cmd_create(const char *name, const size_t nlen, const char *usage, const size_t ulen, const char *desc, const size_t dlen, co_cb_t handler) 
{
  DEBUG("Creating command %s", name);
  co_cmd_t *cmd = h_calloc(1, sizeof(co_cmd_t));
  cmd->exec = handler;
  CHECK_MEM(cmd->name = co_str8_create(name, nlen, 0));
  hattach(cmd->name, cmd);
  CHECK_MEM(cmd->usage = co_str8_create(usage, ulen, 0));
  hattach(cmd->usage, cmd);
  CHECK_MEM(cmd->desc = co_str8_create(desc, dlen, 0));
  hattach(cmd->desc, cmd);
  cmd->_len = (sizeof(co_obj_t *) * 3);
  cmd->_exttype = _cmd;
  cmd->_header._type = _ext8;
  return (co_obj_t *)cmd;
error:
  DEBUG("Failed to create command %s.", name);
  return NULL;
}

co_obj_t *
co_cmd_exec(co_obj_t *key, co_obj_t *param) 
{
  char *kstr = NULL;
  size_t klen = co_obj_data(kstr, key);
  co_cmd_t *cmd = (co_cmd_t *)co_db_namespace_find("cmd.", 4, kstr, klen);
  
  CHECK((cmd != NULL), "No such command!");
  return cmd->exec((co_obj_t *)cmd, param);
error:
  return NULL;
}

co_obj_t *
co_cmd_usage(co_obj_t *key) 
{
  char *kstr = NULL;
  size_t klen = co_obj_data(kstr, key);
  co_cmd_t *cmd = (co_cmd_t *)co_db_namespace_find("cmd.", 4, kstr, klen);
  
  CHECK((cmd != NULL), "No such command!");
  return cmd->usage;
error:
  return NULL;
}

co_obj_t *
co_cmd_desc(co_obj_t *key) 
{
  char *kstr = NULL;
  size_t klen = co_obj_data(kstr, key);
  co_cmd_t *cmd = (co_cmd_t *)co_db_namespace_find("cmd.", 4, kstr, klen);
  
  CHECK((cmd != NULL), "No such command!");
  return cmd->desc;
error:
  return NULL;
}

