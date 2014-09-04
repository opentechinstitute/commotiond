/* vim: set ts=2 expandtab: */
/**
 *       @file  daemon.c
 *      @brief  commotiond - an embedded C daemon for managing mesh networks.
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
 * Commotion is free software: you can redistribute it and/or modify
 * it under the terms of the GNU Affero General Public License as
 * published by the Free Software Foundation, either version 3 of the
 * License, or (at your option) any later version.
 * 
 * Commotion is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU Affero General Public License for more details.
 * 
 * You should have received a copy of the GNU Affero General Public License
 * along with Commotion.  If not, see <http://www.gnu.org/licenses/>.
 *
 * =====================================================================================
 */

#include <stdlib.h>
#include <stdio.h>
#include <getopt.h>
#include <unistd.h>
#include <signal.h>
#include <fcntl.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <arpa/inet.h>
#include <limits.h>
#include "config.h"
#include "debug.h"
#include "cmd.h"
#include "util.h"
#include "loop.h"
#include "plugin.h"
#include "process.h"
#include "profile.h"
#include "socket.h"
#include "msg.h"
#include "iface.h"
#include "id.h"
#include "obj.h"
#include "list.h"
#include "tree.h"

#define REQUEST_MAX 1024
#define RESPONSE_MAX 1024

extern co_socket_t unix_socket_proto;
static int pid_filehandle;

/* Global daemon variables. */
static char *_config = COMMOTION_CONFIGFILE;
static char *_pid = NULL;
static char *_state = NULL;
static char *_bind = NULL;
static char *_plugins = NULL;
static char *_profiles = NULL;

SCHEMA(default)
{
  SCHEMA_ADD("ssid", "commotionwireless.net"); 
  SCHEMA_ADD("bssid", "02:CA:FF:EE:BA:BE"); 
  SCHEMA_ADD("bssidgen", "true"); 
  SCHEMA_ADD("channel", "5"); 
  SCHEMA_ADD("mode", "adhoc"); 
  SCHEMA_ADD("type", "mesh"); 
  SCHEMA_ADD("dns", "208.67.222.222"); 
  SCHEMA_ADD("domain", "mesh.local"); 
  SCHEMA_ADD("ipgen", "true"); 
  SCHEMA_ADD("ip", "100.64.0.0"); 
  SCHEMA_ADD("netmask", "255.192.0.0"); 
  SCHEMA_ADD("ipgenmask", "255.192.0.0"); 
  SCHEMA_ADD("encryption", "psk2"); 
  SCHEMA_ADD("key", "c0MM0t10n!r0cks"); 
  SCHEMA_ADD("serval", "false"); 
  SCHEMA_ADD("announce", "true"); 
  return 1;
}

SCHEMA(global)
{
  SCHEMA_ADD("bind", COMMOTION_MANAGESOCK); 
  SCHEMA_ADD("state", COMMOTION_STATEDIR); 
  SCHEMA_ADD("pid", COMMOTION_PIDFILE); 
  SCHEMA_ADD("plugins", COMMOTION_PLUGINDIR); 
  SCHEMA_ADD("profiles", COMMOTION_PROFILEDIR); 
  SCHEMA_ADD("id", "0"); 
  return 1;
}

static co_obj_t *
_cmd_help_i(co_obj_t *data, co_obj_t *current, void *context) 
{
  char *cmd_name = NULL;
  size_t cmd_len = 0;
  CHECK((cmd_len = co_obj_data(&cmd_name, ((co_cmd_t *)current)->name)) > 0, "Failed to read command name.");
  DEBUG("Command: %s, Length: %d", cmd_name, (int)cmd_len);
  co_tree_insert_unsafe((co_obj_t *)context, cmd_name, cmd_len, ((co_cmd_t *)current)->usage);
  return NULL;
error:
  return NULL;
}

CMD(help)
{
  *output = co_tree16_create();
  if(params != NULL && co_list_length(params) > 0)
  {
    co_obj_t *cmd = co_list_element(params, 0);
    if(cmd != NULL && IS_STR(cmd))
    {
      char *cstr = NULL;
      size_t clen = co_obj_data(&cstr, cmd);
      if(clen > 0)
      {
        co_tree_insert_unsafe(*output, cstr, clen, co_cmd_desc(cmd));
        return 1;
      }
    }
    else return 0;
  }
  return co_cmd_process(_cmd_help_i, (void *)*output);
}

static co_obj_t *
_cmd_profiles_i(co_obj_t *data, co_obj_t *current, void *context) 
{
  char *name = NULL;
  size_t nlen = co_obj_data(&name, ((co_profile_t *)current)->name);
  co_tree_insert_unsafe((co_obj_t *)context, name, nlen, ((co_profile_t *)current)->name);
  return NULL;
}

CMD(profiles)
{
  *output = co_tree16_create();
  co_profiles_process(_cmd_profiles_i, (void *)*output);
  return 1;
}

CMD(up)
{
  *output = co_tree16_create();
  CHECK(co_list_length(params) == 2, "Incorrect parameters.");
  unsigned char mac[6];
  memset(mac, '\0', sizeof(mac));
  char address[16];
  memset(address, '\0', sizeof(address));
  char *ifname = NULL;
  size_t iflen = co_obj_data(&ifname, co_list_element(params, 0));
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
  co_obj_t *prof = co_profile_find(co_list_element(params, 1));
  CHECK(prof != NULL, "Failed to load profile.");
#ifndef _OPENWRT
  char *ipgen, *ip, *ipgenmask, *ssid, *bssid, *mode, *dns, *domain, *netmask;
  unsigned int chan;
  CHECK(co_profile_get_str(prof, &ip, "ip", sizeof("ip")), "Failed to get 'ip' option.");
  CHECK(co_profile_get_str(prof, &netmask, "netmask", sizeof("netmask")), "Failed to get 'netmask' option.");
  CHECK(co_profile_get_str(prof, &ipgen, "ipgen", sizeof("ipgen")), "Failed to get 'ipgen' option.");
  CHECK(co_profile_get_str(prof, &ipgenmask, "ipgenmask", sizeof("ipgenmask")), "Failed to get 'ipgenmask' option.");
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
  char *profname = NULL;
  co_obj_data(&profname, ((co_profile_t *)prof)->name);
  ((co_iface_t *)iface)->profile = strdup(profname);

  co_tree_insert(*output, ifname, iflen, co_str8_create("up", sizeof("up"), 0));
  return 1;
error:
  co_iface_remove(ifname);
  co_tree_insert(*output, "error", sizeof("error"), co_str8_create("Failed to bring up interface.", sizeof("Failed to bring up interface."), 0));
  return 0;
}

CMD(down)
{
  *output = co_tree16_create();
  char *ifname = NULL;
  size_t iflen = co_obj_data(&ifname, co_list_element(params, 0));
  CHECK(iflen > 0, "Incorrect parameters.");
  CHECK(co_iface_remove(ifname), "Failed to bring down interface %s.", ifname);
  co_tree_insert(*output, ifname, iflen, co_str8_create("down", sizeof("down"), 0));
  return 1;
error:
  co_tree_insert(*output, "error", sizeof("error"), co_str8_create("Failed to bring down interface.", sizeof("Failed to bring down interface."), 0));
  return 0;
}

CMD(status)
{

  *output = co_tree16_create();
  CHECK(co_list_length(params) == 1, "Incorrect parameters.");
  char *ifname = NULL;
  size_t iflen = co_obj_data(&ifname, co_list_element(params, 0));
  CHECK(iflen > 0, "Incorrect parameters.");
  char *profile_name = profile_name = co_iface_profile(ifname); 
  if(profile_name == NULL)
    co_tree_insert(*output, "status", sizeof("status"), co_str8_create("down", sizeof("down"), 0));
  else
    co_tree_insert(*output, "status", sizeof("status"), co_str8_create(profile_name, strlen(profile_name)+1, 0));
  return 1;
error:
  co_tree_insert(*output, "error", sizeof("error"), co_str8_create("Failed to get status.", sizeof("Failed to get status."), 0));
  return 0;
}

CMD(state)
{
  *output = co_tree16_create();
  CHECK(co_list_length(params) == 2, "Incorrect parameters.");
  char mac[6];
  memset(mac, '\0', sizeof(mac));
  char address[16];
  memset(address, '\0', sizeof(address));
  char *ifname = NULL;
  char *propname = NULL;
  char *ssid = NULL, *chan = NULL, *ipgen = NULL, *ipgenmask = NULL, *type = NULL, *ip = NULL, *bssidgen = NULL;
  co_obj_t *object = NULL;
  int t = 0;
  co_obj_t *iface = co_list_element(params, 0);
  CHECK(IS_STR(iface), "Incorrect parameters.");
  co_obj_t *prop = co_list_element(params, 1);
  CHECK(IS_STR(prop), "Incorrect parameters.");
  size_t proplen = co_obj_data(&propname, prop);
  CHECK(co_obj_data(&ifname, iface), "Incorrect parameters.");
  char *profile_name = NULL; 
  CHECK((profile_name = co_iface_profile(ifname)), "Interface state is inactive."); 
  DEBUG("profile_name: %s", profile_name);
  co_obj_t *p = co_str8_create(profile_name, strlen(profile_name)+1, 0);
  co_obj_t *prof = NULL;
  CHECK((prof = co_profile_find(p)), "Could not load profile."); 
  if(!strcmp(propname, "ip"))
  {
    if(co_profile_get_str(prof, &ipgen, "ipgen", sizeof("ipgen")) > 0)
    {
      if(!strcmp(ipgen, "true"))
      {
        if(co_profile_get_str(prof, &type, "type", sizeof("type")) > 0)
        {
          if(!strcmp(type, "ap") || !strcmp(type, "plug")) t = 1;
        }
        CHECK(co_profile_get_str(prof, &ipgenmask, "ipgenmask", sizeof("ipgenmask")) > 0, "Attempting to generate IP but ipgenmask not set.");
        CHECK(co_profile_get_str(prof, &ip, "ip", sizeof("ip")) > 0, "Attempting to generate IP but ip not set.");
        CHECK(co_generate_ip(ip, ipgenmask, co_id_get(), address, t), "Failed to generate IP.");
        object = co_str8_create(address, sizeof(address), 0);
        CHECK(object != NULL, "Failed to get property.");
        co_tree_insert(*output, propname, proplen, object);
        return 1;
      }
      else
      {
        object = co_profile_get(prof, prop);
      }
    }
    else
    {
      object = co_profile_get(prof, prop);
    }
  }
  else if(!strcmp(propname, "bssid"))
  {
    if(co_profile_get_str(prof, &bssidgen, "bssidgen", sizeof("bssidgen")) > 0)
    {
      if(!strcmp(bssidgen, "true"))
      {
        CHECK(co_profile_get_str(prof, &ssid, "ssid", sizeof("ssid")) > 0, "Attempting to generate BSSID but SSID not set.");
        CHECK(co_profile_get_str(prof, &chan, "channel", sizeof("channel")) > 0, "Attempting to generate BSSID but channel not set.");
        unsigned int channel;
        CHECK(sscanf(chan, "%u", &channel) > 0, "Failed to convert channel.");
        DEBUG("Channel: %u", channel);
        char bssid[BSSID_SIZE];
        memset(bssid, '\0', sizeof(bssid));
        get_bssid(ssid, channel, bssid);
        char bssidstr[BSSID_STR_SIZE];
        memset(bssidstr, '\0', sizeof(bssidstr));
        CHECK(snprintfcat(bssidstr, BSSID_STR_SIZE, "%02x:%02x:%02x:%02x:%02x:%02x", bssid[0] & 0xff, bssid[1] & 0xff, bssid[2] & 0xff, bssid[3] & 0xff, bssid[4] & 0xff, bssid[5] & 0xff) > 0, "Failed to convert BSSID.");
        DEBUG("BSSID: %s", bssidstr);
        object = co_str8_create(bssidstr, strlen(bssidstr) + 1, 0);
        CHECK(object != NULL, "Failed to get property.");
        co_tree_insert(*output, propname, proplen, object);
        return 1;
      }
      else
      {
        object = co_profile_get(prof, prop);
      }
    }
    else
    {
      object = co_profile_get(prof, prop);
    }
  }
  else
  {
    object = co_profile_get(prof, prop);
  }

  CHECK(object != NULL, "Failed to get property.");
  co_tree_insert_unsafe(*output, propname, proplen, object);
  //co_obj_free(p);
  return 1;
error:
  co_tree_insert(*output, "error", sizeof("error"), co_str8_create("Failed to get property.", sizeof("Failed to get property."), 0));
  //co_obj_free(p);
  return 0;
}

CMD(nodeid)
{
  *output = co_tree16_create();
  char *ret = NULL;
  co_obj_t *out = co_str8_create(NULL, 11, 0);
  nodeid_t id;
  size_t plen = co_list_length(params);
  if(plen == 0)
  {
    co_obj_data(&ret, out);
    id = co_id_get();
    snprintf(ret, 11, "%u", ntohl(id.id));
    INFO("Node ID: %u", ntohl(id.id));
    co_tree_insert(*output, "id", sizeof("id"), out);
    return 1;
  }
  else if(plen == 1)
  {
    int n = 0;
    out = co_list_element(params, 0);
    co_obj_data(&ret, out);
    CHECK(sscanf(ret, "%d", &n) > 0, "Failed to read new nodeid."); 
    co_id_set_from_int(n);
    id = co_id_get();
    snprintf(ret, 11, "%u", ntohl(id.id));
    INFO("Node ID: %u", ntohl(id.id));
    co_tree_insert(*output, "id", sizeof("id"), out);
    return 1;
  }
  else if(plen == 2)
  {
    co_obj_t *macobj = co_str8_create("mac", sizeof("mac"), 0);
    if(!co_str_cmp(co_list_element(params, 0), macobj))
    {
      unsigned char mac[6];
      char *macstr = NULL;
      co_obj_t *macaddr = co_list_element(params, 1);
      CHECK(IS_STR(macaddr), "Incorrect parameters.");
      co_obj_data(&macstr, macaddr);
      mac_string_to_bytes(macstr, mac);
      co_id_set_from_mac(mac, sizeof(mac));
      co_obj_data(&ret, out);
      id = co_id_get();
      snprintf(ret, 11, "%u", ntohl(id.id));
      INFO("Node ID: %u", ntohl(id.id));
      co_tree_insert(*output, "id", sizeof("id"), out);
      co_obj_free(macobj);
      return 1;
    }
    else
    {
      co_tree_insert(*output, "error", sizeof("error"), co_str8_create("Incorrect nodeid parameters.", sizeof("Incorrect nodeid parameters."), 0));
      co_obj_free(macobj);
      return 0;
    }

  }
  else
  {
    co_tree_insert(*output, "error", sizeof("error"), co_str8_create("Incorrect nodeid parameters.", sizeof("Incorrect nodeid parameters."), 0));
    return 0;
  }
error:
  co_tree_insert(*output, "error", sizeof("error"), co_str8_create("Incorrect nodeid parameters.", sizeof("Incorrect nodeid parameters."), 0));
  return 0;

}

CMD(genip)
{
  *output = co_tree16_create();
  size_t plen = co_list_length(params);
  CHECK(plen <= 3, "Incorrect parameters.");

  int type = 0;

  char *subnet = NULL;
  co_obj_data(&subnet, co_list_element(params, 0));

  char *netmask = NULL;
  co_obj_data(&netmask, co_list_element(params, 1));

  char address[16]; 
  memset(address, '\0', sizeof(address));
    
  /* Get node id */
  nodeid_t id = co_id_get();

  if(plen == 3)
  {
    co_obj_t *gwobj = co_str8_create("gw", sizeof("gw"), 0);
    if(co_str_cmp(co_list_element(params, 2), gwobj) == 0) 
    {
      DEBUG("Gateway-type address.");
      type = 1;
    }
    co_obj_free(gwobj);
  }

  /* Generate local ip */
  CHECK(co_generate_ip (subnet, netmask, id, address, type), "IP generation unsuccessful.");   
    
  DEBUG("Local address for this node: %s", address);
  
  // NOTE: whoever calls this function must then free the heap space for "address"
  co_tree_insert(*output, "address", sizeof("address"), co_str8_create(address, sizeof(address), 0));
  return 1;
error:
  co_tree_insert(*output, "error", sizeof("error"), co_str8_create("Incorrect genip parameters.", sizeof("Incorrect genip parameters."), 0));
  return 0;
}

CMD(genbssid)
{
  *output = co_tree16_create();
  size_t plen = co_list_length(params);
  CHECK(plen == 2, "Incorrect parameters.");

  char *ssid = NULL;
  co_obj_data(&ssid, co_list_element(params, 0));

  char *chan = NULL;
  co_obj_data(&chan, co_list_element(params, 1));
  unsigned int channel = 0;
  CHECK(sscanf(chan, "%u", &channel) > 0, "Failed to convert channel.");
  char bssid[BSSID_SIZE];
  memset(bssid, '\0', sizeof(bssid));
  get_bssid(ssid, channel, bssid);
  DEBUG("ssid: %s, channel: %u, bssid: %s", ssid, channel, bssid);
  char bssidstr[BSSID_STR_SIZE];
  memset(bssidstr, '\0', sizeof(bssidstr));
  CHECK(snprintfcat(bssidstr, BSSID_STR_SIZE, "%02x:%02x:%02x:%02x:%02x:%02x", bssid[0] & 0xff, bssid[1] & 0xff, \
        bssid[2] & 0xff, bssid[3] & 0xff, bssid[4] & 0xff, bssid[5] & 0xff) > 0, "Failed to convert BSSID.");
  DEBUG("BSSID: %s", bssidstr);
  co_obj_t *object = co_str8_create(bssidstr, strlen(bssidstr) + 1, 0);

  // NOTE: whoever calls this function must then free the heap space for "address"
  co_tree_insert(*output, "bssid", sizeof("bssid"), object);
  return 1;
error:
  co_tree_insert(*output, "error", sizeof("error"), co_str8_create("Incorrect genbssid parameters.", \
        sizeof("Incorrect genbssid parameters."), 0));
  return 0;
}

CMD(set)
{
  *output = co_tree16_create();
  size_t plen = co_list_length(params);
  CHECK(plen == 3, "Incorrect parameters.");
  co_obj_t *prof = NULL;

  if(!co_str_cmp_str(co_list_element(params, 0), "global"))
  {
    prof = co_profile_global();
  }
  else
  {
    prof = co_profile_find(co_list_element(params, 0));
  }

  if(prof == NULL)
  {
    co_tree_insert(*output, "error", sizeof("error"), co_str8_create("Profile not found.", sizeof("Profile not found."), 0));
    return 0;
  }

  char *kstr = NULL;
  size_t klen = co_obj_data(&kstr, co_list_element(params, 1));
  CHECK(klen > 0, "Invalid key.");

  char *vstr = NULL;
  size_t vlen = co_obj_data(&vstr, co_list_element(params, 2));
  CHECK(vlen > 0, "Invalid value.");

  CHECK(co_profile_set_str(prof, kstr, klen, vstr, vlen), "Failed to set key %s to value %s.", kstr, vstr);
  co_tree_insert(*output, kstr, klen, co_list_element(params, 2));
  return 1;

error:
  co_tree_insert(*output, "error", sizeof("error"), co_str8_create("Incorrect profile set parameters.", sizeof("Incorrect profile set parameters."), 0));
  return 0;
}

CMD(get)
{
  *output = co_tree16_create();
  size_t plen = co_list_length(params);
  CHECK((plen == 2), "Incorrect parameters.");
  co_obj_t *prof = NULL;

  if(!co_str_cmp_str(co_list_element(params, 0), "global"))
  {
    prof = co_profile_global();
  }
  else
  {
    prof = co_profile_find(co_list_element(params, 0));
  }

  if(prof == NULL)
  {
    co_tree_insert(*output, "error", sizeof("error"), co_str8_create("Profile not found.", sizeof("Profile not found."), 0));
    return 0;
  }

  char *kstr = NULL;
  size_t klen = co_obj_data(&kstr, co_list_element(params, 1));
  CHECK(klen > 0, "Invalid key.");
  co_obj_t *kobj = co_str8_create(kstr, klen, 0);

  co_obj_t *value = co_profile_get(prof, kobj);
  CHECK(value != NULL, "Invalid value.");
  co_tree_insert_unsafe(*output, kstr, klen, value);
  return 1;

error:
  co_tree_insert(*output, "error", sizeof("error"), co_str8_create("Incorrect profile get parameters.", sizeof("Incorrect profile get parameters."), 0));
  return 0;
}

CMD(save)
{
  *output = co_tree16_create();
  size_t plen = co_list_length(params);
  CHECK(plen <= 2, "Incorrect parameters.");
  co_obj_t *prof = NULL;
  char *pstr = NULL;
  size_t proflen = 0;
  char path_tmp[PATH_MAX] = {};

  if(!co_str_cmp_str(co_list_element(params, 0), "global"))
  {
    CHECK(plen == 1, "Incorrect parameters.");
    prof = co_profile_global();
    if(prof == NULL)
    {
      co_tree_insert(*output, "error", sizeof("error"), co_str8_create("Profile not found.", sizeof("Profile not found."), 0));
      return 0;
    }

    if(plen == 2)
    {
      proflen = co_obj_data(&pstr, co_list_element(params, 1));
    }
    else
    {
      proflen = co_obj_data(&pstr, ((co_profile_t *)prof)->name);
    }
    CHECK(proflen > 0, "Failed to get profile name.");

    CHECK(co_profile_export_file(prof, _config), "Failed to export file.");
  }
  else
  {
    CHECK(plen <= 2, "Incorrect parameters.");
    prof = co_profile_find(co_list_element(params, 0));
    if(prof == NULL)
    {
      co_tree_insert(*output, "error", sizeof("error"), co_str8_create("Profile not found.", sizeof("Profile not found."), 0));
      return 0;
    }

    if(plen == 2)
    {
      proflen = co_obj_data(&pstr, co_list_element(params, 1));
    }
    else
    {
      proflen = co_obj_data(&pstr, ((co_profile_t *)prof)->name);
    }
    CHECK(proflen > 0, "Failed to get profile name.");

    strlcpy(path_tmp, _profiles, PATH_MAX);
    strlcat(path_tmp, "/", PATH_MAX);
    strlcat(path_tmp, pstr, PATH_MAX);
    CHECK(co_profile_export_file(prof, path_tmp), "Failed to export file.");
  }

  co_tree_insert(*output, pstr, proflen, co_str8_create("Saved.", sizeof("Saved."), 0));
  return 1;

error:
  co_tree_insert(*output, "error", sizeof("error"), co_str8_create("Error attempting to save profile.", sizeof("Error attempting to save profile."), 0));
  return 0;
}

CMD(new)
{
  *output = co_tree16_create();
  size_t plen = co_list_length(params);
  CHECK(plen == 1, "Incorrect parameters.");

  co_obj_t *prof = co_profile_find(co_list_element(params, 0));
  if(prof != NULL)
  {
    co_tree_insert(*output, "error", sizeof("error"), co_str8_create("Profile already exists.", sizeof("Profile already exists."), 0));
    return 0;
  }

  char *kstr = NULL;
  size_t klen = co_obj_data(&kstr, co_list_element(params, 0));
  CHECK(klen > 0, "Invalid key.");
  CHECK(co_profile_add(kstr, klen), "Failed to add profile.");

  co_tree_insert(*output, kstr, klen, co_str8_create("Created.", sizeof("Created."), 0));
  return 1;
error:
  co_tree_insert(*output, "error", sizeof("error"), co_str8_create("Error creating profile.", sizeof("Error creating profile."), 0));
  return 0;
}

CMD(delete)
{
  *output = co_tree16_create();
  size_t plen = co_list_length(params);
  CHECK(plen == 1, "Incorrect parameters.");

  co_obj_t *prof = co_profile_find(co_list_element(params, 0));
  if(prof == NULL)
  {
    co_tree_insert(*output, "error", sizeof("error"), co_str8_create("Profile doesn't exist.", sizeof("Profile doesn't exist."), 0));
    return 0;
  }

  char *kstr = NULL;
  size_t klen = co_obj_data(&kstr, co_list_element(params, 0));
  CHECK(klen > 0, "Invalid key.");
  CHECK(co_profile_remove(kstr, klen), "Failed to add profile.");

  char path_tmp[PATH_MAX] = {};
  strlcpy(path_tmp, _profiles, PATH_MAX);
  strlcat(path_tmp, "/", PATH_MAX);
  strlcat(path_tmp, kstr, PATH_MAX);
  remove(path_tmp);

  co_tree_insert(*output, kstr, klen, co_str8_create("Deleted.", sizeof("Deleted."), 0));
  return 1;
error:
  co_tree_insert(*output, "error", sizeof("error"), co_str8_create("Error creating profile.", sizeof("Error creating profile."), 0));
  return 0;
}

int dispatcher_cb(co_obj_t *self, co_obj_t *context);

/**
 * @brief sends/receives socket messages
 * @param self pointer to dispatcher socket struct
 * @param fd file descriptor object of connected socket
 */
int dispatcher_cb(co_obj_t *self, co_obj_t *fd) {
  CHECK(IS_SOCK(self),"Not a socket.");
  co_socket_t *sock = (co_socket_t*)self;
  char reqbuf[REQUEST_MAX];
  memset(reqbuf, '\0', sizeof(reqbuf));
  ssize_t reqlen = 0;
  char respbuf[RESPONSE_MAX];
  memset(respbuf, '\0', sizeof(respbuf));
  size_t resplen = 0;
  co_obj_t *request = NULL, *response = NULL;
  uint8_t *type = NULL;
  uint32_t *id = NULL;
  int ret = 0;
  co_obj_t *nil = co_nil_create(0);
  CHECK_MEM(nil);

  /* Incoming message on socket */
  reqlen = sock->receive((co_obj_t*)sock, fd, reqbuf, sizeof(reqbuf));
  DEBUG("Received %d bytes.", (int)reqlen);
  if(reqlen == 0) {
    INFO("Received connection.");
    co_obj_free(nil);
    return 1;
  }
  if (reqlen < 0) {
    INFO("Connection recvd() -1");
    sock->hangup((co_obj_t*)sock, fd);
    co_obj_free(nil);
    return 1;
  }
  /* If it's a commotion message type, parse the header, target and payload */
  CHECK(co_list_import(&request, reqbuf, reqlen) > 0, "Failed to import request.");
  co_obj_data((char **)&type, co_list_element(request, 0)); 
  CHECK(*type == 0, "Not a valid request.");
  CHECK(co_obj_data((char **)&id, co_list_element(request, 1)) == sizeof(uint32_t), "Not a valid request ID.");
  if(co_cmd_exec(co_list_element(request, 2), &response, co_list_element(request, 3)))
  {
    resplen = co_response_alloc(respbuf, sizeof(respbuf), *id, nil, response);
    sock->send(fd, respbuf, resplen);
  }
  else
  {
    if(response == NULL)
    {
      response = co_tree16_create();
      co_tree_insert(response, "error", sizeof("error"), co_str8_create("Incorrect command.", sizeof("Incorrect command."), 0));
    }
    resplen = co_response_alloc(respbuf, sizeof(respbuf), *id, response, nil);
    sock->send(fd, respbuf, resplen);
  }

  ret = 1;
error:
  if (nil) co_obj_free(nil);
  if (request) co_obj_free(request);
  if (response) co_obj_free(response);
  return ret;
}

 /**
  * @brief starts the daemon
  * @param statedir directory in which to store lock file
  * @param pidfile name of lock file (stores process id)
  * @warning ensure that there is only one copy 
  */
static void daemon_start(char *statedir, char *pidfile) {
  int pid, sid, i;
  char str[10];


 /*
  * Check if parent process id is set
  * 
  * If PPID exists, we are already a daemon
  */
   if (getppid() == 1) {
    return;
  }

  
  
  pid = fork(); /* Fork parent process */

  if (pid < 0) {
    exit(EXIT_FAILURE);
  }

  if (pid > 0) {
    /* Child created correctly */
    INFO("Child process created: %d\n", pid);
    exit(EXIT_SUCCESS); /* exit parent process */
  }

 /* Child continues from here */
 
 /* 
  * Set file permissions to 750 
  * -- Owner may read/write/execute
  * -- Group may read/write
  * -- World has no permissions
  */
  umask(027); 

  /* Get a new process group */
  sid = setsid();

  if (sid < 0) {
    exit(EXIT_FAILURE);
  }

  /* Close all descriptors */
  for (i = getdtablesize(); i >=0; --i) {
    close(i);
  }

  /* Route i/o connections */
  close(STDIN_FILENO);
  close(STDOUT_FILENO);
  close(STDERR_FILENO);

  chdir(statedir); /* Change running directory */

  /*
   * Open lock file
   * Ensure that there is only one copy
   */
  pid_filehandle = open(pidfile, O_RDWR|O_CREAT, 0600);

  if(pid_filehandle == -1) {
  /* Couldn't open lock file */
    ERROR("Could not lock PID lock file %s, exiting", pidfile);
    exit(EXIT_FAILURE);
  }

  /* Get and format PID */
  sprintf(str, "%d\n", getpid());

  /* Write PID to lockfile */
  write(pid_filehandle, str, strlen(str));

}

static void print_usage() {
  printf(
          "The Commotion management daemon.\n"
          "https://commotionwireless.net\n\n"
          "Usage: commotiond [options]\n"
          "\n"
          "Options:\n"
          " -c, --config <file>   Specify global config file.\n"
          " -b, --bind <uri>      Specify management socket.\n"
          " -d, --plugins <dir>   Specify plugin directory.\n"
          " -f, --profiles <dir>  Specify profile directory.\n"
          " -i, --id <nodeid>     Specify unique id number for this node.\n"
          " -n, --nodaemonize     Do not fork into the background.\n"
          " -p, --pid <file>      Specify pid file.\n"
          " -s, --statedir <dir>  Specify instance directory.\n"
          " -h, --help            Print this usage message.\n"
  );
}


/**
 * @brief Creates sockets for event loop, daemon and dispatcher. Starts/stops event loop.
 * 
 */
int main(int argc, char *argv[]) {
  int opt = 0;
  int opt_index = 0;
  int daemonize = 1;
  uint32_t newid = 0;

  static const char *opt_string = "c:b:d:f:i:np:s:h";

  static struct option long_opts[] = {
    {"config", required_argument, NULL, 'c'},
    {"bind", required_argument, NULL, 'b'},
    {"plugins", required_argument, NULL, 'd'},
    {"profiles", required_argument, NULL, 'f'},
    {"nodeid", required_argument, NULL, 'i'},
    {"nodaemon", no_argument, NULL, 'n'},
    {"pid", required_argument, NULL, 'p'},
    {"statedir", required_argument, NULL, 's'},
    {"help", no_argument, NULL, 'h'}
  };
  

 /* Parse command line arguments */
  opt = getopt_long(argc, argv, opt_string, long_opts, &opt_index);

  while(opt != -1) {
    switch(opt) {
      case 'c':
        _config = optarg;
        break;
      case 'b':
        _bind = optarg;
        break;
      case 'd':
        _plugins = optarg;
        break;
      case 'f':
        _profiles = optarg;
        break;
      case 'i':
        newid = strtoul(optarg,NULL,10);
        break;
      case 'n':
        daemonize = 0;
        break;
      case 'p':
        _pid = optarg;
        break;
      case 's':
        _state = optarg;
        break;
      case 'h':
      default:
        print_usage();
        return 0;
        break;
    }
    opt = getopt_long(argc, argv, opt_string, long_opts, &opt_index);
  }

  co_id_set_from_int(newid);

  co_profiles_init(16); /* Set up profiles */
  SCHEMA_GLOBAL(global);
  if(!co_profile_import_global(_config)) WARN("Failed to load global configuration file %s!", _config);
  co_obj_t *settings = co_profile_global();
  
  if(_pid == NULL)
  {
    if(settings != NULL)
      co_profile_get_str(settings, &_pid, "pid", sizeof("pid"));
    else
      _pid = COMMOTION_PIDFILE;
  }
  DEBUG("PID file: %s", _pid);

  if(_bind == NULL)
  {
    if(settings != NULL)
      co_profile_get_str(settings, &_bind, "bind", sizeof("bind"));
    else
      _bind = COMMOTION_MANAGESOCK;
  }
  DEBUG("Client socket: %s", _bind);
  
  if(_state == NULL)
  {
    if(settings != NULL)
      co_profile_get_str(settings, &_state, "state", sizeof("state"));
    else
      _state = COMMOTION_STATEDIR;
  }
  DEBUG("State directory: %s", _state);
  
  if(_plugins == NULL)
  {
    if(settings != NULL)
      co_profile_get_str(settings, &_plugins, "plugins", sizeof("plugins"));
    else
      _plugins = COMMOTION_PLUGINDIR;
  }
  DEBUG("Plugins directory: %s", _plugins);

  if(_profiles == NULL)
  {
    if(settings != NULL)
      co_profile_get_str(settings, &_profiles, "profiles", sizeof("profiles"));
    else
      _profiles = COMMOTION_PROFILEDIR;
  }
  DEBUG("Profiles directory: %s", _profiles);

  /* If the daemon is needed, start the daemon */
  if(daemonize) daemon_start((char *)_state, (char *)_pid); /* Input state directory and lockfile with process id */

  //co_profile_delete_global();
  co_plugins_init(16);
  co_cmds_init(16);
  co_loop_create(); /* Start event loop */
  co_ifaces_create(); /* Configure interfaces */
  co_plugins_load(_plugins); /* Load plugins and register plugin profile schemas */
  co_profile_import_global(_config);
  SCHEMA_REGISTER(default); /* Register default schema */
  co_profile_import_files(_profiles); /* Import profiles from profiles directory */

  /* Register commands */
  CMD_REGISTER(help, "help <none>", "Print list of commands and usage information.");
  CMD_REGISTER(profiles, "profiles <none>", "Print list of available profiles.");
  CMD_REGISTER(up, "up <interface> <profile>", "Apply a configuration profile to an interface.");
  CMD_REGISTER(down, "down <interface>", "Deconfigure a configured interface.");
  CMD_REGISTER(status, "status <interface>", "Show configured profile for interface.");
  CMD_REGISTER(state, "state <interface> <property>", "Show configured property for interface.");
  CMD_REGISTER(nodeid, "nodeid [<nodeid>] [mac <mac address>]", "Get or set node ID number.");
  CMD_REGISTER(genip, "genip <subnet> <netmask> [gw]", "Generate IP address.");
  CMD_REGISTER(genbssid, "genbssid <ssid> <channel>", "Generate a BSSID.");
  CMD_REGISTER(get, "get <profile> <key>", "Get value from profile.");
  CMD_REGISTER(set, "set <profile> <key> <value>", "Set value to profile.");
  CMD_REGISTER(save, "save <profile> [<filename>]", "Save profile to a file in the profiles directory.");
  CMD_REGISTER(new, "new <profile>", "Create a new profile.");
  CMD_REGISTER(delete, "delete <profile>", "Delete a profile.");
  
  /* Set up sockets */
  co_socket_t *socket = (co_socket_t*)NEW(co_socket, unix_socket);
  socket->poll_cb = dispatcher_cb;
  socket->register_cb = co_loop_add_socket;
  socket->bind((co_obj_t*)socket, _bind);
  co_plugins_start();

  co_loop_start();
  co_loop_destroy();
  co_cmds_shutdown();
  co_profiles_shutdown();
  co_plugins_shutdown();
  co_ifaces_shutdown();

  return 0;
}
