/* vim: set ts=2 expandtab: */
/**
 *       @file  command.h
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

#ifndef _COMMAND_H
#define _COMMAND_H

#include <stdlib.h>
#define MAX_COMMANDS 32

typedef char *(*co_cmd_handler_t)(void *self, char *argv[], int argc);

typedef struct {
  int mask;
  char *name;
  char *usage;
  char *description;
  co_cmd_handler_t exec;
} co_cmd_t;

void co_cmd_add(char *name, co_cmd_handler_t handler, char *usage, char *description, int mask);

char *co_cmd_exec(char *name, char *argv[], int argc, int mask);

char *co_cmd_usage(char *name, int mask);

char *co_cmd_description(char *name, int mask);

char *cmd_help(void *self, char *argv[], int argc);

char *cmd_list_profiles(void *self, char *argv[], int argc);

char *cmd_up(void *self, char *argv[], int argc);

char *cmd_down(void *self, char *argv[], int argc);

char *cmd_status(void *self, char *argv[], int argc);

char *cmd_state(void *self, char *argv[], int argc);

#endif
