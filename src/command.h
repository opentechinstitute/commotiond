/*! 
 *
 * \file command.h 
 *
 * \brief a mechanism for registering and controlling display of commands 
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
#define MAX_COMMANDS 32

typedef char *(*command_handler_t)(void *self, char *args);

typedef struct {
  int mask;
  char *name;
  char *usage;
  char *description;
  command_handler_t exec;
} command_t;

void command_add(const char *name, command_handler_t handler, const char *usage, const char *description, const int mask);

char *command_exec(const char *name, char *args, const int mask);

char *command_usage(const char *name, const int mask);

char *command_description(const char *name, const int mask);

