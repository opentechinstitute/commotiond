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
#include <arpa/inet.h>
#include "extern/tst.h"
#include "extern/list.h"
#include "debug.h"
#include "util.h"
#include "profile.h"
#include "iface.h"
#include "command.h"
#include "id.h"

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
  if(!commands) ERROR("Failed to register command %s!", name);
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
  char *ret = NULL;
  CHECK((ret = co_list_profiles()), "No valid profiles to list!");
  return ret;
error:
  ret = strdup("No profiles available.\n");
  return ret;
}

/* Set link local ip */
char *cmd_generate_local_ip() {

  unsigned char mac[6];
  memset(mac, '\0', sizeof(mac));
  
  char *address = malloc(16 * sizeof(char)); 
  memset(address, '\0', sizeof(address));
    
  /* Get node id */
  nodeid_t id = co_id_get();

  /* Generate local ip */
  co_generate_ip ("169.254.0.0", "255.255.0.0", id, address, 0);   
    
  DEBUG("Local address for this node: %s", address);
  
// NOTE: whoever calls this function must then free the heap space for "address"
return address; 
}  

/* Bring up wireless interface and configure it */
char *cmd_up(void *self, char *argv[], int argc) {
  co_cmd_t *this = self;
  unsigned char mac[6];
  memset(mac, '\0', sizeof(mac));
  char address[16];
  memset(address, '\0', sizeof(address));
  char *ret;
  if(argc < 2) {
    return this->usage;
  }
  co_iface_t *iface = co_iface_add(argv[0], AF_INET);
  DEBUG("Bringing up iface %s", argv[0]);
  CHECK(iface != NULL, "Failed to create interface %s.", argv[0]);
  /* Get node id */
  nodeid_t id = co_id_get();
  /* If no node id specified, create id from mac address */
  if(!id.id && co_iface_get_mac(iface, mac, sizeof(mac))) {
    //print_mac(mac);
    co_id_set_from_mac(mac, sizeof(mac));
  }
  /* Load profile */
  co_profile_t *prof = co_profile_find(argv[1]);
  CHECK(prof != NULL, "Failed to load profile %s.", argv[1]);
#ifndef _OPENWRT
  co_profile_dump(prof);

  /* Load interface configurations from profile */
  if(!strcmp("true", co_profile_get_string(prof, "ipgenerate", "true"))) {
    co_generate_ip(co_profile_get_string(prof, "ip", "5.0.0.0"), 
      co_profile_get_string(prof, 
                            "ipgeneratemask",
                            co_profile_get_string(prof, "netmask", "255.0.0")
                           ),
       co_id_get(), 
       address, 
       0);
  }
  DEBUG("Address: %s", address);
  
  /* If no profile use default configuration */
  if(iface->wireless && co_iface_wpa_connect(iface)) {
    co_iface_set_ssid(iface, co_profile_get_string(prof, "ssid", "\"commotionwireless.net\""));
    co_iface_set_bssid(iface, co_profile_get_string(prof, "bssid", "02:CA:FF:EE:BA:BE"));
    co_iface_set_frequency(iface, wifi_freq(co_profile_get_int(prof, "channel", 5)));
    co_iface_set_mode(iface, co_profile_get_string(prof, "mode", "\"adhoc\""));
    co_iface_set_apscan(iface, 0);
    co_iface_wireless_enable(iface);
  }
  
  /* Set DNS configuration */
  co_set_dns(co_profile_get_string(prof, "dns", "8.8.8.8"), co_profile_get_string(prof, "domain", "mesh.local"), "/tmp/resolv.commotion");
  co_iface_set_ip(iface, address, co_profile_get_string(prof, "netmask", "255.0.0.0"));
#endif
  iface->profile = strdup(argv[1]);

  ret = strdup("Interface up.\n");
  return ret;
error:
  free(iface);
  ret = strdup("Interface up failed.\n");
  return ret;
}

/* Bring down the wireless interface */
char *cmd_down(void *self, char *argv[], int argc) {
  co_cmd_t *this = self;
  char *ret = NULL;
  if(argc < 1) {
    return this->usage;
  }

  if(co_iface_remove(argv[0])) {
    ret = strdup("Interface down.\n");
  } else {
    ret = strdup("Failed to bring down interface!\n");
  }
  return ret;
}


char *cmd_status(void *self, char *argv[], int argc) {
  co_cmd_t *this = self;
  char *ret = NULL;
  if(argc < 1) {
    return ret = strdup(this->usage);
  }
  CHECK((ret = co_iface_profile(argv[0])), "Interface status not found.");
  ret = strdup(ret);
  return ret;

error:
  ret = strdup("Status not available.\n");
  return ret;
}

char *cmd_state(void *self, char *argv[], int argc) {
  co_cmd_t *this = self;
  char *ret = NULL;
  char mac[6];
  memset(mac, '\0', sizeof(mac));
  char address[16];
  memset(address, '\0', sizeof(address));
  if(argc < 2) {
    return ret = strdup(this->usage);
  }
  char *profile_name = NULL; 
  CHECK((profile_name = co_iface_profile(argv[0])), "Interface state is inactive."); 
  DEBUG("profile_name: %s", profile_name);
  co_profile_t *prof = NULL;
  CHECK((prof = co_profile_find(profile_name)), "Could not load profile."); 
  if(!strcmp(argv[1], "ssid")) {
    ret = co_profile_get_string(prof, "ssid", "commotionwireless.net");
  } else if(!strcmp(argv[1], "bssid")) {
    ret = co_profile_get_string(prof, "bssid", "02:CA:FF:EE:BA:BE");
  } else if(!strcmp(argv[1], "channel")) {
    ret = co_profile_get_string(prof, "channel", "5");
  } else if(!strcmp(argv[1], "type")) {
    ret = co_profile_get_string(prof, "type", "mesh");
  } else if(!strcmp(argv[1], "dns")) {
    ret = co_profile_get_string(prof, "dns", "8.8.8.8");
  } else if(!strcmp(argv[1], "domain")) {
    ret = co_profile_get_string(prof, "domain", "mesh.local");
  } else if(!strcmp(argv[1], "ipgenerate")) {
    ret = co_profile_get_string(prof, "ipgenerate", "true");
  } else if(!strcmp(argv[1], "mode")) {
    ret = co_profile_get_string(prof, "mode", "adhoc");
  } else if(!strcmp(argv[1], "netmask")) {
    ret = co_profile_get_string(prof, "netmask", "255.0.0.0");
  } else if(!strcmp(argv[1], "wpa")) {
    ret = co_profile_get_string(prof, "wpa", "false");
  } else if(!strcmp(argv[1], "wpakey")) {
    ret = co_profile_get_string(prof, "wpakey", "c0MM0t10n!r0cks");
  } else if(!strcmp(argv[1], "servald")) {
    ret = co_profile_get_string(prof, "servald", "false");
  } else if(!strcmp(argv[1], "servaldsid")) {
    ret = co_profile_get_string(prof, "servaldsid", "");
  } else if(!strcmp(argv[1], "announce")) {
    ret = co_profile_get_string(prof, "announce", "true");
  } else if(!strcmp(argv[1], "ip")) {
    if(!strcmp(co_profile_get_string(prof, "ipgenerate", "true"), "true")) {
      if(!strcmp(co_profile_get_string(prof, "type", "mesh"), "mesh")) {
        co_generate_ip(co_profile_get_string(prof, "ip", "5.0.0.0"), 
                       co_profile_get_string(prof, 
                                             "ipgeneratemask",
                                             co_profile_get_string(prof, 
                                                                   "netmask", 
                                                                   "255.0.0")),
                       co_id_get(), 
                       address, 
                       0);
      } else {
        co_generate_ip(co_profile_get_string(prof, "ip", "5.0.0.0"), 
                       co_profile_get_string(prof, 
                                             "ipgeneratemask",
                                             co_profile_get_string(prof, 
                                                                   "netmask", 
                                                                   "255.0.0")),
                       co_id_get(), 
                       address, 
                       1);
      }
      ret = address;
    } else {
      ret = co_profile_get_string(prof, "ip", "5.0.0.0");
    }
  } else ret = NULL;

  if(ret) {
    return ret = strdup(ret);
  } else return ret = strdup("Failed to get variable state.\n");
error:
  return ret = strdup("Failed to get interface or profile.\n");
}

char *cmd_nodeid(void *self, char *argv[], int argc) {
  char *ret = malloc(11);
  memset(ret, '\0', 11);
  nodeid_t id = co_id_get();
  snprintf(ret, 11, "%u", ntohl(id.id));
  INFO("Node ID: %u", ntohl(id.id));
  return ret;
}

char *cmd_set_nodeid_from_mac(void *self, char *argv[], int argc) {
  co_cmd_t *this = self;
  unsigned char mac[6];
  if(argc < 1) {
    return this->usage;
  }
  mac_string_to_bytes(argv[0], mac);
  co_id_set_from_mac(mac, sizeof(mac));

  return strdup("Set nodeid.");
}
