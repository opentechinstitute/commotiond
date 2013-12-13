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
#include "debug.h"
#include "util.h"
#include "profile.h"
#include "iface.h"
#include "cmd.h"
#include "id.h"
#include "obj.h"
#include "list.h"
#include "tree.h"
#include "core.h"


static co_obj_t *
_cmd_help_i(co_obj_t *data, co_obj_t *current, void *context) 
{
  char *name = NULL;
  size_t nlen = co_obj_data(&name, ((co_cmd_t *)current)->name);
  co_tree_insert((co_obj_t *)context, name, nlen, ((co_cmd_t *)current)->usage);
  return NULL;
}

int
cmd_help(co_obj_t *self, co_obj_t **output, co_obj_t *params) 
{
  *output = co_tree16_create();
  return co_cmd_process(_cmd_help_i, (void *)*output);
}

static co_obj_t *
_cmd_list_profiles_i(co_obj_t *data, co_obj_t *current, void *context) 
{
  char *name = NULL;
  size_t nlen = co_obj_data(&name, ((co_cmd_t *)current)->name);
  co_tree_insert((co_obj_t *)context, name, nlen, ((co_cmd_t *)current)->name);
  return NULL;
}

int
cmd_list_profiles(co_obj_t *self, co_obj_t **output, co_obj_t *params) 
{
  *output = co_tree16_create();
  co_profiles_process(_cmd_list_profiles_i, (void *)*output);
  return 1;
}

/* Set link local ip */
int
cmd_generate_local_ip(co_obj_t *self, co_obj_t **output, co_obj_t *params) 
{
  unsigned char mac[6];
  memset(mac, '\0', sizeof(mac));
  
  char address[16]; 
  memset(address, '\0', sizeof(address));
    
  /* Get node id */
  nodeid_t id = co_id_get();

  /* Generate local ip */
  co_generate_ip ("169.254.0.0", "255.255.0.0", id, address, 0);   
    
  DEBUG("Local address for this node: %s", address);
  
  // NOTE: whoever calls this function must then free the heap space for "address"
  *output = co_str8_create(address, sizeof(address), 0);
  return 1;
}  

/* Bring up wireless interface and configure it */
int
cmd_up(co_obj_t *self, co_obj_t **output, co_obj_t *params) 
{
  unsigned char mac[6];
  memset(mac, '\0', sizeof(mac));
  char address[16];
  memset(address, '\0', sizeof(address));
  char *ifname = NULL;
  co_obj_data(&ifname, co_list_element(params, 0));
  co_obj_t *iface = co_iface_add(ifname, AF_INET);
  DEBUG("Bringing up iface %s", ifname);
  CHECK(iface != NULL, "Failed to create interface %s.", ifname);
  /* Get node id */
  nodeid_t id = co_id_get();
  /* If no node id specified, create id from mac address */
  if(!id.id && co_iface_get_mac(iface, mac, sizeof(mac))) {
    //print_mac(mac);
    co_id_set_from_mac(mac, sizeof(mac));
  }
  /* Load profile */
  co_obj_t *prof = co_profile_find(co_list_element(params, 0));
  CHECK(prof != NULL, "Failed to load profile.");
#ifndef _OPENWRT
  co_profile_dump(prof);
  char *ipgen, *ip, *ipgenmask, *ssid, *bssid, *mode, *dns, *domain, *netmask;
  unsigned int chan;
  CHECK(co_profile_get_str(prof, &ip, "ip", sizeof("ip")), "Failed to get 'ip' option.");
  CHECK(co_profile_get_str(prof, &netmask, "netmask", sizeof("netmask")), "Failed to get 'netmask' option.");
  CHECK(co_profile_get_str(prof, &ipgen, "ipgenerate", sizeof("ipgenerate")), "Failed to get 'ipgenerate' option.");
  CHECK(co_profile_get_str(prof, &ipgenmask, "ipgeneratemask", sizeof("ipgeneratemask")), "Failed to get 'ipgeneratemask' option.");
  CHECK(co_profile_get_str(prof, &ssid, "ssid", sizeof("ssid")), "Failed to get 'ssid' option.");
  CHECK(co_profile_get_str(prof, &bssid, "bssid", sizeof("bssid")), "Failed to get 'bssid' option.");
  CHECK(co_profile_get_str(prof, &mode, "mode", sizeof("mode")), "Failed to get 'mode' option.");
  CHECK(co_profile_get_str(prof, &dns, "dns", sizeof("dns")), "Failed to get 'dns' option.");
  CHECK(co_profile_get_str(prof, &domain, "domain", sizeof("domain")), "Failed to get 'domain' option.");
  chan = co_profile_get_uint(prof, "channel", sizeof("channel"));
  CHECK(chan >= 0, "Failed to get 'channel' option.");
  /* Load interface configurations from profile */
  if(!strcmp("true", ipgen )) {
    co_generate_ip(ip, ipgenmask, co_id_get(), address, 0);
  }
  DEBUG("Address: %s", address);
  
  /* If no profile use default configuration */
  if(((co_iface_t *)iface)->wireless && co_iface_wpa_connect(iface)) {
    co_iface_set_ssid(iface, ssid);
    co_iface_set_bssid(iface, bssid);
    co_iface_set_frequency(iface, chan);
    co_iface_set_mode(iface, mode);
    co_iface_set_apscan(iface, 0);
    co_iface_wireless_enable(iface);
  }
  
  /* Set DNS configuration */
  co_set_dns(dns, domain, "/tmp/resolv.commotion");
  co_iface_set_ip(iface, address, netmask);
#endif
  ((co_iface_t *)iface)->profile = strdup(ifname);

  *output = co_str8_create("Interface up.", sizeof("Interface up."), 0);
  return 1;
error:
  co_iface_remove(ifname);
  *output = co_str8_create("Interface up failed.", sizeof("Interface up failed."), 0);
  return 0;
}

/* Bring down the wireless interface */
int
cmd_down(co_obj_t *self, co_obj_t **output, co_obj_t *params) 
{
  char *ifname = NULL;
  CHECK(co_obj_data(&ifname, co_list_element(params, 0)), "Incorrect parameters.");
  CHECK(co_iface_remove(ifname), "Failed to bring down interface %s.", ifname);
  *output = co_str8_create("Interface down.", sizeof("Interface down."), 0);  
  return 1;
error:
  *output = co_str8_create("Failed to bring down interface.", sizeof("Failed to bring down interface."), 0);  
  return 0;
}


int
cmd_status(co_obj_t *self, co_obj_t **output, co_obj_t *params) 
{
  co_obj_t *iface = co_list_element(params, 0);
  char *ifname = NULL;
  CHECK(co_obj_data(&ifname, iface), "Incorrect parameters.");
  char *profile_name = NULL; 
  CHECK((profile_name = co_iface_profile(ifname)), "Interface state is inactive."); 
  *output = co_str8_create(profile_name, strlen(profile_name), 0);
  return 1;
error:
  *output = co_str8_create("No profile found.", sizeof("No profile found."), 0);
  return 0;
}

int
cmd_state(co_obj_t *self, co_obj_t **output, co_obj_t *params) 
{
  char mac[6];
  memset(mac, '\0', sizeof(mac));
  char address[16];
  memset(address, '\0', sizeof(address));
  char *ifname = NULL;
  co_obj_t *iface = co_list_element(params, 0);
  CHECK(IS_STR(iface), "Incorrect parameters.");
  co_obj_t *prop = co_list_element(params, 1);
  CHECK(IS_STR(prop), "Incorrect parameters.");
  CHECK(co_obj_data(&ifname, iface), "Incorrect parameters.");
  char *profile_name = NULL; 
  CHECK((profile_name = co_iface_profile(ifname)), "Interface state is inactive."); 
  DEBUG("profile_name: %s", profile_name);
  co_obj_t *p = co_str8_create(profile_name, sizeof(profile_name), 0);
  co_obj_t *prof = NULL;
  CHECK((prof = co_profile_find(p)), "Could not load profile."); 
  *output = co_profile_get(prof, prop);
  CHECK(*output != NULL, "Failed to get property.");
  co_obj_free(p);
  return 1;
error:
  co_obj_free(p);
  return 0;
}

int
cmd_nodeid(co_obj_t *self, co_obj_t **output, co_obj_t *params) 
{
  char *ret = NULL;
  *output = co_str8_create(NULL, 11, 0);
  co_obj_data(&ret, *output);
  nodeid_t id = co_id_get();
  snprintf(ret, 11, "%u", ntohl(id.id));
  INFO("Node ID: %u", ntohl(id.id));
  return 1;
}

int
cmd_set_nodeid_from_mac(co_obj_t *self, co_obj_t **output, co_obj_t *params) 
{
  unsigned char mac[6];
  char *macstr = NULL;
  co_obj_t *macaddr = co_list_element(params, 0);
  CHECK(IS_STR(macaddr), "Incorrect parameters.");
  co_obj_data(&macstr, macaddr);
  mac_string_to_bytes(macstr, mac);
  co_id_set_from_mac(mac, sizeof(mac));

  return 1;
error:
  return 0;
}
