/* vim: set ts=2 expandtab: */
/**
 *       @file  iface.h
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

#ifndef _IFACE_H
#define _IFACE_H
#include <stdbool.h>
#include "net/if.h"

#define FREQ_LEN 5 //number of characters in 802.11 frequency designator
#define MAC_LEN 6

typedef struct {
  int fd;
  struct ifreq ifr;
  struct wpa_ctrl *ctrl;
  int wpa_id;
  bool wireless;
} co_iface_t;

co_iface_t *co_iface_create(const char *iface_name, const int family);

int co_iface_wpa_connect(co_iface_t *iface);

int co_iface_get_mac(co_iface_t *iface, char output[6]);

int co_iface_set_ip(co_iface_t *iface, const char *ip_addr, const char *netmask);

int co_iface_set_ssid(co_iface_t *iface, const char *ssid);

int co_iface_set_bssid(co_iface_t *iface, const char *bssid);

int co_iface_set_frequency(co_iface_t *iface, const int frequency);

int co_iface_set_encryption(co_iface_t *iface, const char *proto);

int co_iface_set_key(co_iface_t *iface, const char *key);

int co_set_dns(const char *dnsservers[], const size_t numservers, const char *searchdomain, const char *resolvpath);

int co_generate_ip(const char *ip, const char *netmask, const char mac[MAC_LEN], char *output);

#endif
