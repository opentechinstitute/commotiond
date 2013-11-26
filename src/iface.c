/* vim: set ts=2 expandtab: */
/**
 *       @file  iface.c
 *      @brief  interface handling for the Commotion daemon
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
#include <stdio.h>
#include <stdbool.h>
#include <net/if.h>
#include <arpa/inet.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <sys/ioctl.h>
#include <netinet/in.h>
#include <linux/sockios.h>
#include "extern/wpa_ctrl.h"
#include "extern/list.h"
#include "debug.h"
#include "iface.h"
#include "util.h"
#include "id.h"


static list_t *ifaces = NULL;

/* Run wpa_supplicant */
static char *wpa_control_dir = "/var/run/wpa_supplicant";

/**
 * @brief checks whether interace is wireless
 * @param co_iface available interface
 */
static int _co_iface_is_wireless(const co_iface_t *iface) {
	CHECK((ioctl(iface->fd, SIOCGIWNAME, &iface->ifr) != -1), "No wireless extensions for interface: %s", iface->ifr.ifr_name);
  return 1;
error: 
  return 0;
}

static void _co_iface_wpa_cb(char *msg, size_t len) {
  INFO("wpa_supplicant says: %s\n", msg);
  return;
}

static int _co_iface_wpa_command(const co_iface_t *iface, const char *cmd, char *buf, size_t *len) {
  CHECK(iface->ctrl != NULL, "Interface %s not connected to wpa_supplicant.", iface->ifr.ifr_name);

	CHECK((wpa_ctrl_request(iface->ctrl, cmd, strlen(cmd), buf, len, _co_iface_wpa_cb) >= 0), "Failed to send command %s to wpa_supplicant.", cmd);
	return 1;

error:
  return 0;
}

static int _co_iface_wpa_add_network(co_iface_t *iface) {
  char buf[WPA_REPLY_SIZE];
  size_t len;
  
  CHECK(_co_iface_wpa_command(iface, "ADD_NETWORK", buf, &len), "Failed to add network to wpa_supplicant.");
  iface->wpa_id = atoi(buf);
  DEBUG("Added wpa_supplicant network #%s", buf);
  return 1;
error:
  return 0;
}

static int _co_iface_wpa_remove_network(co_iface_t *iface) {
  char buf[WPA_REPLY_SIZE];
  size_t len;
  
  CHECK(_co_iface_wpa_command(iface, "REMOVE_NETWORK", buf, &len), "Failed to remove network from wpa_supplicant.");
  DEBUG("Removed wpa_supplicant network #%d", iface->wpa_id);
  return 1;
error:
  return 0;
}

static int _co_iface_wpa_disable_network(co_iface_t *iface) {
  char buf[WPA_REPLY_SIZE];
  size_t len;
  
  CHECK(_co_iface_wpa_command(iface, "DISABLE_NETWORK", buf, &len), "Failed to remove network from wpa_supplicant.");
  DEBUG("Disabled wpa_supplicant network #%d", iface->wpa_id);
  return 1;
error:
  return 0;
}

/**
 * @brief sets up secure access point
 * @param *iface name of commotion interface
 * @param *option option to be configured
 * @param *optval configuration value
 */
static int _co_iface_wpa_set(co_iface_t *iface, const char *option, const char *optval) {
	char cmd[256];
	int res;
  char buf[WPA_REPLY_SIZE];
  size_t len;

  if(iface->wpa_id < 0) { CHECK(_co_iface_wpa_add_network(iface), "Could not set option %s", option); }

	res = snprintf(cmd, sizeof(cmd), "SET_NETWORK %d %s %s",
			  iface->wpa_id, option, optval);
	CHECK((res > 0 && (size_t) res <= sizeof(cmd) - 1), "Too long SET_NETWORK command.");
	
  return _co_iface_wpa_command(iface, cmd, buf, &len);

error:
  return 0;
}

static int _co_iface_match_i(const void *iface, const void *iface_name) {
  const co_iface_t *this_iface = iface;
  const char *this_name = iface_name;
  if((strcmp(this_iface->ifr.ifr_name, this_name)) == 0) return 0;
  return -1;
}

int co_ifaces_create(void) {
  CHECK((ifaces = list_create(IFACES_MAX)) != NULL, "Interface loader creation failed, clearing lists.");
  return 1;

error:
  list_destroy(ifaces);
  free(ifaces);
  return 0;
}

int co_iface_remove(const char *iface_name) {
  lnode_t *node;
  CHECK((node = list_find(ifaces, iface_name, _co_iface_match_i)) != NULL, "Failed to delete interface %s!", iface_name);
  node = list_delete(ifaces, node);
  co_iface_t *iface = lnode_get(node);
  co_iface_unset_ip(iface);
  co_iface_wireless_disable(iface);
  co_iface_wpa_disconnect(iface);
  free(iface->profile);
  free(iface);
  return 1;

error:
  return 0;
}

co_iface_t *co_iface_add(const char *iface_name, const int family) {
  co_iface_t *iface = malloc(sizeof(co_iface_t));
  memset(iface, '\0', sizeof(co_iface_t));
  iface->fd = socket(PF_INET, SOCK_DGRAM, IPPROTO_IP);
  strlcpy(iface->ifr.ifr_name, iface_name, IFNAMSIZ);
  
 /* Check whether interface is IPv4 or IPv6 */
  if((family & AF_INET) == AF_INET) {
    iface->ifr.ifr_addr.sa_family = AF_INET; 
  } else if((family & AF_INET6) == AF_INET6) {
    iface->ifr.ifr_addr.sa_family = AF_INET6; 
  } else {
    ERROR("Invalid address family!");
    free(iface);
    return NULL;
  }

  /* Check whether interface is wireless or not */
  if(_co_iface_is_wireless(iface)) iface->wireless = true;
  iface->wpa_id = -1;
    
  list_append(ifaces, lnode_create((void *)iface));
  return iface; 
}

int co_iface_get_mac(co_iface_t *iface, unsigned char *output, int output_size) {
  co_iface_t *maciface = NULL;
  CHECK(output_size == 6, "output_size does not equal six");

  maciface = malloc(sizeof(co_iface_t));
  memset(maciface, '\0', sizeof(co_iface_t));
  memmove(maciface, iface, sizeof(co_iface_t));
  if (0 == ioctl(iface->fd, SIOCGIFHWADDR, &maciface->ifr)) {
    DEBUG("Received MAC Address : %02x:%02x:%02x:%02x:%02x:%02x\n",
                maciface->ifr.ifr_hwaddr.sa_data[0],maciface->ifr.ifr_hwaddr.sa_data[1],maciface->ifr.ifr_hwaddr.sa_data[2]
                ,maciface->ifr.ifr_hwaddr.sa_data[3],maciface->ifr.ifr_hwaddr.sa_data[4],maciface->ifr.ifr_hwaddr.sa_data[5]);
    memmove(output, maciface->ifr.ifr_addr.sa_data, output_size);
    free(maciface);
    return 1;
  }
  free(maciface);
error:
  return 0;
}


int co_iface_set_ip(co_iface_t *iface, const char *ip_addr, const char *netmask) {
  CHECK_MEM(iface); 
	struct sockaddr_in *addr = (struct sockaddr_in *)&iface->ifr.ifr_addr;

  DEBUG("Setting address %s and netmask %s.", ip_addr, netmask);
 
	// Convert IP from numbers and dots to binary notation
  inet_pton(AF_INET, ip_addr, &addr->sin_addr);
	CHECK((ioctl(iface->fd, SIOCSIFADDR, &iface->ifr) == 0), "Failed to set IP address for interface: %s", iface->ifr.ifr_name);

  inet_pton(AF_INET, netmask, &addr->sin_addr);
	CHECK((ioctl(iface->fd, SIOCSIFNETMASK, &iface->ifr) == 0), "Failed to set IP address for interface: %s", iface->ifr.ifr_name);

  //Get and set interface flags.
  CHECK((ioctl(iface->fd, SIOCGIFFLAGS, &iface->ifr) == 0), "Interface shutdown: %s", iface->ifr.ifr_name);
	iface->ifr.ifr_flags |= IFF_UP;
	iface->ifr.ifr_flags |= IFF_RUNNING;
	CHECK((ioctl(iface->fd, SIOCSIFFLAGS, &iface->ifr) == 0), "Interface up failed: %s", iface->ifr.ifr_name);
 
  DEBUG("Addressing for interface %s is done!", iface->ifr.ifr_name);
	return 1;

error:
  return 0;
}

int co_iface_unset_ip(co_iface_t *iface) {
  CHECK_MEM(iface); 
  //Get and set interface flags.
  CHECK((ioctl(iface->fd, SIOCGIFFLAGS, &iface->ifr) == 0), "Interface shutdown: %s", iface->ifr.ifr_name);
	iface->ifr.ifr_flags &= ~IFF_UP;
	iface->ifr.ifr_flags &= ~IFF_RUNNING;
	CHECK((ioctl(iface->fd, SIOCSIFFLAGS, &iface->ifr) == 0), "Interface up failed: %s", iface->ifr.ifr_name);
  return 1;
error:
  return 0;
}

int co_iface_wpa_disconnect(co_iface_t *iface) {
  if(iface->ctrl) {
    wpa_ctrl_detach(iface->ctrl);
    wpa_ctrl_close(iface->ctrl);
    return 1;
  } else return 0;
}

int co_iface_wpa_connect(co_iface_t *iface) {
	char *filename;
	size_t length;

  CHECK(iface->wireless, "Not a wireless interface: %s", iface->ifr.ifr_name);

	length = strlen(wpa_control_dir) + strlen(iface->ifr.ifr_name) + 2;
	filename = malloc(length);
	CHECK_MEM(filename);
  memset(filename, '\0', length);
	snprintf(filename, length, "%s/%s", wpa_control_dir, iface->ifr.ifr_name);
  DEBUG("WPA control file: %s", filename);

	CHECK((iface->ctrl = wpa_ctrl_open(filename)), "Failed to connect to wpa_supplicant.");
	free(filename);
	return 1;

error:
  free(filename);
  return 0;
}


int co_iface_set_ssid(co_iface_t *iface, const char *ssid) {
  return _co_iface_wpa_set(iface, "ssid", ssid); 
}

int co_iface_set_bssid(co_iface_t *iface, const char *bssid) {
	char cmd[256], *pos, *end;
	int ret;
  char buf[WPA_REPLY_SIZE];
  size_t len;

	end = cmd + sizeof(cmd);
	pos = cmd;
	ret = snprintf(pos, end - pos, "BSSID");
	if (ret < 0 || ret >= end - pos) {
		ERROR("Too long BSSID command.");
		return 0;
	}
	pos += ret;
	ret = snprintf(pos, end - pos, " %d %s", iface->wpa_id, bssid);
	if (ret < 0 || ret >= end - pos) {
		ERROR("Too long BSSID command.");
		return 0;
	}
	pos += ret;

	return _co_iface_wpa_command(iface, cmd, buf, &len);
}

int co_iface_set_frequency(co_iface_t *iface, const int frequency) {
  char freq[FREQ_LEN]; 
  snprintf(freq, FREQ_LEN, "%d", frequency);
  return _co_iface_wpa_set(iface, "frequency", freq); 
}

int co_iface_set_encryption(co_iface_t *iface, const char *proto) {
  return _co_iface_wpa_set(iface, "proto", proto); 
}

int co_iface_set_key(co_iface_t *iface, const char *key) {
  return _co_iface_wpa_set(iface, "psk", key); 
}

int co_iface_set_mode(co_iface_t *iface, const char *mode) {
  return _co_iface_wpa_set(iface, "mode", mode); 
}

int co_iface_set_apscan(co_iface_t *iface, const int value) {
  char cmd[256];
  char buf[WPA_REPLY_SIZE];
  size_t len;

  snprintf(cmd, sizeof(cmd), "AP_SCAN %d", value);
	cmd[sizeof(cmd) - 1] = '\0';

	return _co_iface_wpa_command(iface, cmd, buf, &len);
}

int co_iface_wireless_enable(co_iface_t *iface) {
  char cmd[256];
  char buf[WPA_REPLY_SIZE];
  size_t len;

  snprintf(cmd, sizeof(cmd), "ENABLE_NETWORK %d", iface->wpa_id);
	cmd[sizeof(cmd) - 1] = '\0';

	return _co_iface_wpa_command(iface, cmd, buf, &len);
}

int co_iface_wireless_disable(co_iface_t *iface) {
  CHECK(_co_iface_wpa_disable_network(iface), "Failed to disable network %s", iface->ifr.ifr_name);
  CHECK(_co_iface_wpa_remove_network(iface), "Failed to remove network %s", iface->ifr.ifr_name);
	return 1;
error:
  return 0;
}

/*
int co_set_dns(const char *dnsservers[], const size_t numservers, const char *searchdomain, const char *resolvpath) {
  FILE *fp = fopen(resolvpath, "w+");
  if(fp != NULL) {
    if(searchdomain != NULL) fprintf(fp, "search %s\n", searchdomain); 
    for(int i = 0; i < numservers; i++) {
      fprintf(fp, "nameserver %s\n", dnsservers[i]);
    }
    fclose(fp);
    return 1;
  } else ERROR("Could not open file: %s", resolvpath);
  return 0;
}
*/


int co_set_dns(const char *dnsserver, const char *searchdomain, const char *resolvpath) {
  FILE *fp = fopen(resolvpath, "w+");
  if(fp != NULL) {
    if(searchdomain != NULL) fprintf(fp, "search %s\n", searchdomain); 
    fprintf(fp, "nameserver %s\n", dnsserver);
    fclose(fp);
    return 1;
  } else ERROR("Could not open file: %s", resolvpath);
  return 0;
}

int co_generate_ip(const char *base, const char *genmask, const nodeid_t id, char *output, int type) {
  nodeid_t addr;
  addr.id = 0;
  struct in_addr baseaddr;
  struct in_addr generatedaddr;
  struct in_addr genmaskaddr;
  CHECK(inet_aton(base, &baseaddr) != 0, "Invalid base ip address %s", base); 
  CHECK(inet_aton(genmask, &genmaskaddr) != 0, "Invalid genmask address %s", genmask); 

  /*
   * Turn the IP address into a 
   * network address.
   */
  generatedaddr.s_addr = (baseaddr.s_addr & genmaskaddr.s_addr);

  /*
   * get the matching octet from 
   * the mac and and then move it
   * left to the proper spot.
   */
  for (int i = 0; i < 4; i++)
    addr.bytes[i] = (id.bytes[i]&0xff)%0xfe;

  /* 
   * if address is of a gateway
   * type, then set the last byte 
   * to '1'
   * */
  if(type) {
    /* 
     * shift us over by one 
     * to ensure that we are
     * getting maximum entropy 
     * from the mac address.
     */
    addr.bytes[1] = addr.bytes[2];
    addr.bytes[2] = addr.bytes[3];
    addr.bytes[3] = 1;
  }

  /*
   * mask out the parts of address
   * that overlap with the genmask 
   */
  addr.id &= ~(genmaskaddr.s_addr);
  /*
   * add back the user-supplied 
   * base ip address.
   */
  generatedaddr.s_addr = (generatedaddr.s_addr|addr.id);

  strcpy(output, inet_ntoa(generatedaddr));
  return 1;
error:
  return 0;
}

char *co_iface_profile(const char *iface_name) {
  lnode_t *node;
  CHECK((node = list_find(ifaces, iface_name, _co_iface_match_i)) != NULL, "Failed to get interface %s profile!", iface_name);
  co_iface_t *iface = lnode_get(node);
  return iface->profile;
error:
  return NULL;
}

co_iface_t *co_iface_get(const char *iface_name) {
  lnode_t *node;
  CHECK((node = list_find(ifaces, iface_name, _co_iface_match_i)) != NULL, "Failed to get interface %s profile!", iface_name);
  return lnode_get(node);
error:
  return NULL;
}
