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
#include "extern/tst.h"
#include "extern/list.h"
#include "debug.h"
#include "util.h"
#include "profile.h"
#include "iface.h"
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


void co_cmd_add(char *name, co_cmd_handler_t handler, char *usage, char *description, int mask) {
  DEBUG("Registering command %s", name);
  co_cmd_t *new_cmd = malloc(sizeof(co_cmd_t));
  new_cmd->exec = handler;
  new_cmd->mask = mask;
  new_cmd->name = strdup(name);
  new_cmd->usage = strdup(usage);
  new_cmd->description = strdup(description);
  commands = tst_insert(commands, (char *)name, strlen(name), (void *)new_cmd);
  return;
}

char *co_cmd_exec(char *name, char *argv[], int argc, int mask) {
  DEBUG("Command name: %s", name);
  co_cmd_t *cmd = tst_search(commands, name, strlen(name));
  CHECK((cmd != NULL), "No such command!");
  CHECK((cmd->mask & mask) == mask, "Permissions denied for command %s from this access type.", name);
  DEBUG("Executing command %s", name);
  return cmd->exec((void *)cmd, argv, argc);
error:
  return NULL;
}

char *co_cmd_usage(char *name, int mask) {
  co_cmd_t *cmd = tst_search(commands, name, strlen(name));
  CHECK((cmd != NULL), "No such command!");
  CHECK((cmd->mask & mask) == mask, "Permissions denied for command %s from this access type.", name);
  return cmd->usage;
error:
  return 0;
}

char *co_cmd_description(char *name, int mask) {
  co_cmd_t *cmd = tst_search(commands, name, strlen(name));
  CHECK((cmd != NULL), "No such command!");
  CHECK((cmd->mask & mask) == mask, "Permissions denied for command %s from this access type.", name);
  return cmd->description;
error:
  return 0;
}

static void _cmd_help_i(void *value, void *data) {
  co_cmd_t *cvalue = value;
  snprintfcat((char *)data, 2048, "Command: %s Usage: %s", cvalue->name, cvalue->usage);
  return;
}

char *cmd_help(void *self, char *argv[], int argc) {
  char *ret = malloc(2048);
  memset(ret, '\0', sizeof(2048));
  tst_traverse(commands, _cmd_help_i, (void *)ret);
  return ret;
}

char *cmd_list_profiles(void *self, char *argv[], int argc) {
  return co_list_profiles();
}

char *cmd_up(void *self, char *argv[], int argc) {
  co_cmd_t *this = self;
  char mac[6];
  char address[16];
  char *ret = strdup("Interface up!\n");
  if(argc < 2) {
    return this->usage;
  }
  co_iface_t *iface = co_iface_create(argv[0], AF_INET);
  CHECK(iface != NULL, "Failed to create interface %s.", argv[0]);
  co_profile_t *prof = co_profile_find(argv[1]);
  co_profile_dump(prof);
  if(!strcmp("true", co_profile_get_string(prof, "ipgenerate", "true"))) {
    co_iface_get_mac(iface, mac);
    co_generate_ip(co_profile_get_string(prof, "ip", "5.0.0.0"), co_profile_get_string(prof, "ip", "255.0.0.0"), mac, address);
  }
  DEBUG("Address: %s", address);
  if(iface->wireless) {
    co_iface_wpa_connect(iface);
    co_iface_set_ssid(iface, co_profile_get_string(prof, "ssid", "\"commotionwireless.net\""));
    co_iface_set_bssid(iface, co_profile_get_string(prof, "bssid", "02:CA:FF:EE:BA:BE"));
    //co_iface_set_frequency(iface, co_profile_get_string(prof, "freq", 2447));
    co_iface_set_mode(iface, co_profile_get_string(prof, "mode", "\"adhoc\""));
    co_set_dns(co_profile_get_string(prof, "dns", "8.8.8.8"), co_profile_get_string(prof, "domain", "mesh.local"), "/tmp/resolv.commotion");
    co_iface_wireless_apscan(iface, 0);
    co_iface_wireless_enable(iface);
  }

  co_iface_set_ip(iface, address, co_profile_get_string(prof, "ip", "255.0.0.0"));

  return ret;
error:
  free(iface);
  return ret;
}

char *cmd_down(void *self, char *argv[], int argc) {
  co_cmd_t *this = self;
  char *ret = strdup("Interface down!\n");
  if(argc < 2) {
    return this->usage;
  }
  co_iface_t *iface = co_iface_create(argv[0], AF_INET);
  CHECK(iface != NULL, "Failed to create interface %s.", argv[0]);
  co_iface_unset_ip(iface);
  return ret;
error:
  return ret;
}

char *cmd_status(void *self, char *argv[], int argc) {
  co_cmd_t *this = self;
  char *ret = strdup("Interface down!\n");
  if(argc < 1) {
    return this->usage;
  }
  return ret;
  
}
