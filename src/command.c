/*! 
 *
 * \file command.c 
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
#include "extern/tst.h"
#include "debug.h"
#include "util.h"
#include "command.h"

static tst_t *commands = NULL;

/*  void commands_destroy(void) {
  int i = 0;
  if(commands->element_size > 0) {
    for(i = 0; i < commands->max; i++) {
      if(commands->contents[i] != NULL) {
        free(commands->contents[i]->usage);
        free(commands->contents[i]->description);
        free(commands->contents[i]);
      }
    }
  }
  darray_destroy(commands);
  return;
}
*/


void command_add(char *name, command_handler_t handler, char *usage, char *description, int mask) {
  command_t *new_cmd = malloc(sizeof(command_t));
  new_cmd->exec = handler;
  new_cmd->mask = mask;
  new_cmd->name = strdup(name);
  new_cmd->usage = strdup(usage);
  new_cmd->description = strdup(description);
  commands = tst_insert(commands, (char *)name, strlen(name), (void *)new_cmd);
  return;
}

char *command_exec(char *name, char *args, int mask) {
  command_t *cmd = tst_search(commands, name, strlen(name));
  CHECK((cmd != NULL), "No such command!");
  CHECK((cmd->mask & mask) == mask, "Permissions denied for command %s from this access type.", name);
  return cmd->exec((void *)cmd, args);
error:
  return NULL;
}

char *command_usage(char *name, int mask) {
  command_t *cmd = tst_search(commands, name, strlen(name));
  CHECK((cmd != NULL), "No such command!");
  CHECK((cmd->mask & mask) == mask, "Permissions denied for command %s from this access type.", name);
  return cmd->usage;
error:
  return 0;
}

char *command_description(char *name, int mask) {
  command_t *cmd = tst_search(commands, name, strlen(name));
  CHECK((cmd != NULL), "No such command!");
  CHECK((cmd->mask & mask) == mask, "Permissions denied for command %s from this access type.", name);
  return cmd->description;
error:
  return 0;
}

