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
#include <stropts.h>
#include <net/if.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <linux/sockios.h>
#include "extern/wpa_ctrl.h"
#include "debug.h"
#include "iface.h"
#include "util.h"

static char *wpa_control_dir = "/var/run/wpa_supplicant";

static int _co_iface_is_wireless(const co_iface_t *iface) {
	CHECK((ioctl(iface->fd, SIOCSIFADDR, iface->ifr) != -1), "No wireless extensions for interface: %s", iface->ifr.ifr_name);
  return 1;
error: 
  return 0;
}

static void _co_iface_wpa_cb(char *msg, size_t len) {
  INFO("wpa_supplicant says: %s\n", msg);
  return;
}

static int _co_iface_wpa_command(const co_iface_t *iface, const char *cmd) {
	char buffer[2048];
	size_t length;

  CHECK(iface->ctrl != NULL, "Interface %s not connected to wpa_supplicant.", iface->ifr.ifr_name);

	length = sizeof(buffer) - 1;
	CHECK((wpa_ctrl_request(iface->ctrl, cmd, strlen(cmd), buffer, &length, _co_iface_wpa_cb) >= 0), "Failed to send command %s to wpa_supplicant.", cmd);
	return 1;

error:
  return 0;
}

static int _co_iface_wpa_add_network(co_iface_t *iface) {
  CHECK((iface->wpa_id = _co_iface_wpa_command(iface, "ADD_NETWORK")), "Failed to add network to wpa_supplicant.");
  return 1;
error:
  return 0;
}

static int _co_iface_wpa_set(co_iface_t *iface, const char *option, const char *optval) {
	char cmd[256];
	int res;

  if(iface->wpa_id < 0) { CHECK(_co_iface_wpa_add_network(iface), "Could not set option %s", option); }

	res = snprintf(cmd, sizeof(cmd), "SET_NETWORK %d %s %s",
			  iface->wpa_id, option, optval);
	CHECK((res > 0 && (size_t) res <= sizeof(cmd) - 1), "Too long SET_NETWORK command.");
	
  return _co_iface_wpa_command(iface, cmd);

error:
  return 0;
}

co_iface_t *co_iface_create(const char *iface_name, const int family) {
  co_iface_t *iface = malloc(sizeof(co_iface_t));
  iface->fd = socket(PF_INET, SOCK_DGRAM, IPPROTO_IP);
  strlcpy(iface->ifr.ifr_name, iface_name, IFNAMSIZ);
  
  if((family & AF_INET) == AF_INET) {
    iface->ifr.ifr_addr.sa_family = AF_INET; 
  } else if((family & AF_INET6) == AF_INET6) {
    iface->ifr.ifr_addr.sa_family = AF_INET6; 
  } else {
    ERROR("Invalid address family!");
    free(iface);
    return NULL;
  }

  if(_co_iface_is_wireless(iface)) iface->wireless = true;
  iface->wpa_id = -1;
    
  return iface; 
}

int co_iface_get_mac(co_iface_t *iface, char output[6]) {
  if (0 == ioctl(iface->fd, SIOCGIFHWADDR, iface->ifr)) {
    memmove(output, iface->ifr.ifr_addr.sa_data, sizeof(output));
    return 1;
  }
  return 0;
}


int co_iface_set_ip(co_iface_t *iface, const char *ip_addr, const char *netmask) {
  CHECK_MEM(iface); 
	struct sockaddr_in *addr = (struct sockaddr_in *)&iface->ifr.ifr_addr;

  DEBUG("Setting address %s and netmask %s.", ip_addr, netmask);
 
	/* Read interface flags */
	CHECK((ioctl(iface->fd, SIOCGIFFLAGS, iface->ifr) < 0), "Interface shutdown: %s", iface->ifr.ifr_name);
 
	// If interface is down, bring it up
	if (!(iface->ifr.ifr_flags & IFF_UP)) {
		fprintf(stdout, "Device is currently down..setting up.-- %u\n",iface->ifr.ifr_flags);
		iface->ifr.ifr_flags |= IFF_UP;
		CHECK((ioctl(iface->fd, SIOCSIFFLAGS, iface->ifr) < 0), "Interface up failed: %s", iface->ifr.ifr_name);
    DEBUG("Interface is up.");
	}
 
	// Convert IP from numbers and dots to binary notation
  inet_pton(AF_INET, ip_addr, &addr->sin_addr);
	CHECK((ioctl(iface->fd, SIOCSIFADDR, iface->ifr) < 0), "Failed to set IP address for interface: %s", iface->ifr.ifr_name);

  inet_pton(AF_INET, netmask, &addr->sin_addr);
	CHECK((ioctl(iface->fd, SIOCSIFNETMASK, iface->ifr) < 0), "Failed to set IP address for interface: %s", iface->ifr.ifr_name);
 
  DEBUG("Addressing for interface %s is done!", iface->ifr.ifr_name);
	return 1;

error:
  return 0;
}

int co_iface_wpa_connect(co_iface_t *iface) {
	char *filename;
	size_t length;

  //CHECK(iface->wireless, "Not a wireless interface: %s", iface->ifr.ifr_name);

	length = strlen(wpa_control_dir) + strlen(iface->ifr.ifr_name) + 2;
	filename = malloc(length);
	CHECK_MEM(filename);
	snprintf(filename, length, "%s/%s", wpa_control_dir, iface->ifr.ifr_name);

	iface->ctrl = wpa_ctrl_open(filename);
	free(filename);
	return 1;

error:
  if(filename) free(filename);
  return 0;
}


int co_iface_set_ssid(co_iface_t *iface, const char *ssid) {
  return _co_iface_wpa_set(iface, "ssid", ssid); 
}

int co_iface_set_bssid(co_iface_t *iface, const char *bssid) {
	char cmd[256], *pos, *end;
	int ret;

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

	return _co_iface_wpa_command(iface, cmd);
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

int co_generate_ip(const char *ip, const char *netmask, const char mac[MAC_LEN], char *output) {
  uint32_t subnet = 0;
  uint32_t addr = 0;
  struct in_addr ipaddr;
  struct in_addr netaddr;
  struct in_addr maskaddr;
  CHECK(inet_aton(ip, &ipaddr) != 0, "Invalid ip address %s", ip); 
  CHECK(inet_aton(netmask, &maskaddr) != 0, "Invalid netmask address %s", netmask); 

  subnet = maskaddr.s_addr;
  /*
   * Turn the IP address into a 
   * network address.
   */
  netaddr.s_addr = (ipaddr.s_addr & subnet);

  /*
   * get the matching octet from 
   * the mac and and then move it
   * left to the proper spot.
   */
  for (int i = 0; i < 4; i++)
    addr |= ((mac[i]&0xff)%0xfe) << (i*8);

  /*
   * mask out the parts of address
   * that overlap with the subnet 
   * mask
   */
  addr &= ~subnet;

  /*
   * add back the user-supplied 
   * network number.
   */
  netaddr.s_addr = (netaddr.s_addr|addr);
  strcpy(output, inet_ntoa(netaddr));

  return 1;
error:
  return 0;
}
