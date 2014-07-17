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

#ifndef _IFACE_H
#define _IFACE_H
#include <stdbool.h>
#include <net/if.h>
#include "id.h"
#include "obj.h"

#define FREQ_LEN 5 //number of characters in 802.11 frequency designator
#define MAC_LEN 6
#define WPA_REPLY_SIZE 2048
#define IFACES_MAX 32
#define SIOCGIWNAME 0x8B01

/**
 * @enum co_iface_status_t indicates whether the interface is UP(1) or DOWN(0)
 */
typedef enum {
  DOWN = 0,
  UP = 1
} co_iface_status_t;

typedef struct co_iface_t co_iface_t;

/**
 * @struct co_iface_t
 * @brief contains the file descriptor, interface status (up or down), profile name, interface frequency struct, wpa control struct, wpa id, and a bool to indicare whether the interface is wireless or not
 */
struct co_iface_t {
  co_obj_t _header;
  uint8_t _exttype;
  uint8_t _len;
  int fd;
  co_iface_status_t status;
  char *profile;
  struct ifreq ifr;
  struct wpa_ctrl *ctrl;
  int wpa_id;
  bool wireless;
} __attribute__((packed));

/**
 * @brief checks for available interfaces
 */
int co_ifaces_create(void);

/**
 * @brief removes all interfaces
 */
void co_ifaces_shutdown(void);

/**
 * @brief removes an interface from the list of available interfaces
 * @param iface_name name of interface to be removed
 */
int co_iface_remove(char *iface_name);

/**
 * @brief adds a new interface to the list of available interfaces, and checks whether it is IPv4 and IPv6 capable, and whether it is wireless
 * @param iface_name name of the interface
 * @param family address family of the interface. Must be AF_INET (IPv4) or AF_INET6 (IPv6), which uses host and port number for addressing
 */
co_obj_t *co_iface_add(const char *iface_name, const int family);

/**
 * @brief connects the commotion interface to wpa supplicant
 * @param iface interface object
 */
int co_iface_wpa_connect(co_obj_t *iface);

/**
 * @brief turns off wpa supplicant
 * @param iface interface object
 */
int co_iface_wpa_disconnect(co_obj_t *iface);

/**
 * @brief 
 * @param iface interface object
 * @param output output of hardware MAC address lookup
 * @param output_size size of MAC address. Must be six bytes
 */
int co_iface_get_mac(co_obj_t *iface, unsigned char *output, int output_size);

/**
 * @brief sets ip address and netmask for commotion interface
 * @param iface interface object
 * @param ip_addr ip address for the socket
 * @param netmask netmask for the socket
 */
int co_iface_set_ip(co_obj_t *iface, const char *ip_addr, const char *netmask);

/**
 * @brief Resets ip address of chosen interface
 * @param iface interface object
 */
int co_iface_unset_ip(co_obj_t *iface);

/**
 * @brief sets SSID from profile or uses default
 * @param iface interface object
 * @param ssid desired SSID
 */
int co_iface_set_ssid(co_obj_t *iface, const char *ssid);

/**
 * @brief sets BSSID from profile or uses default
 * @param iface interface object
 * @param bssid desired BSSID
 */
int co_iface_set_bssid(co_obj_t *iface, const char *bssid);

/**
 * @brief sets wireless frequency (eg. channel) from profile or uses default
 * @param iface interface object
 * @param frequency desired frequency
 */
int co_iface_set_frequency(co_obj_t *iface, const int frequency);

/**
 * @brief specifies the wpa encryption protocol
 * @param iface interface object
 * @param proto desired protocol
 */
int co_iface_set_encryption(co_obj_t *iface, const char *proto);

/**
 * @brief specifies the wpa encryption key
 * @param iface interface object
 * @param key desired key
 */
int co_iface_set_key(co_obj_t *iface, const char *key);

/**
 * @brief specifies the Wi-Fi mode
 * @param iface interface object
 * @param mode desired mode
 */
int co_iface_set_mode(co_obj_t *iface, const char *mode);

/**
 * @brief sets AP_SCAN value
 * @param iface interface object
 * @param value desired value for AP_SCAN
 */
int co_iface_set_apscan(co_obj_t *iface, const int value);

/**
 * @brief enables specified wireless network
 * @param iface interface object
 */
int co_iface_wireless_enable(co_obj_t *iface);

/**
 * @brief disables specified wireless network
 * @param iface interface object
 */
int co_iface_wireless_disable(co_obj_t *iface);

/**
 * @brief sets DNS name server and sets search domain
 * @param dnsserver IP addresss of DNS server
 * @param searchdomain desired search domain
 * @param resolvpath filepath to resolv.conf
 */
int co_set_dns(const char *dnsserver, const char *searchdomain, const char *resolvpath);

//int co_set_dns(const char *dnsservers[], const size_t numservers, const char *searchdomain, const char *resolvpath);

/**
 * @brief generates an ip table for a commotion interface
 * @param base base address
 * @param genmask genmask
 * @param id the node id
 * @param type whether the device is a gateway (1) or not (0)
 */
int co_generate_ip(const char *base, const char *genmask, const nodeid_t id, char *output, int type);

//int co_iface_status(const char *iface_name);

/**
 * @brief sets node configuration profile 
 * @param iface_name name of interface
 */
char *co_iface_profile(char *iface_name);

/**
 * @brief retrieves node configuration profile 
 * @param iface_name name of interface
 */
co_obj_t *co_iface_get(char *iface_name);

#endif
