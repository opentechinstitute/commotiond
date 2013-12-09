/* vim: set ts=2 expandtab: */
/**
 *       @file  olsrd.h
 *      @brief  OLSRd configuration and process management
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
#include "process.h"
#include "util.h"
#include "obj.h"

#define OLSR_HNA4 (1 << 0)
#define OLSR_HNA6 (1 << 1)
#define OLSR_IFACE_MESH (1 << 0)
#define OLSR_IFACE_ETHER (1 << 1)

/**
 * @struct co_olsrd_process_t
 * @brief contains the OLSRd process protocol
 */
typedef struct {
  co_process_t proto;
} co_olsrd_process_t;

/**
 * @struct co_olsrd_conf_item_t
 * @brief contains the key and value for configuring OLSRd for Commotion
 */
typedef struct {
  char *key;
  char *value;
} co_olsrd_conf_item_t;

/**
 * @struct co_olsrd_conf_plugin_t
 * @brief contains the number of configuration items for OLSRd
 */
typedef struct {
  char *name; 
  co_olsrd_conf_item_t **attr;
  int num_attr;
} co_olsrd_conf_plugin_t;

/**
 * @struct co_olsrd_conf_iface_t
 * @brief contains interace configuration settings for OLSR, including interface name, mode and IPv4 broadcast message
 */
typedef struct {
  char *ifname;
  int mode;
  char *Ipv4Broadcast;
} co_olsrd_conf_iface_t;

/**
 * @struct co_olsrd_conf_hna_t
 * @brief contains host network address settings, including family, address and netmask
 */
typedef struct {
  int family;
  char *address;
  char *netmask;
} co_olsrd_conf_hna_t;

/**
 * @brief adds an interface to OLSRd
 * @param name name of the interface to be added
 * @param mode network mode for interface
 * @param Ipv4Broadcast IPv4 broadcast address (node address, using netmask as last octet)
 */
int co_olsrd_add_iface(const char* name, int mode, const char *Ipv4Broadcast);

/**
 * @brief removes an interface from OLSRd
 * @param name name of the interface to be removed
 * @param mode network mode for interface
 * @param Ipv4Broadcast IPv4 broadcast address (node address, using netmask as last octet)
 */
int co_olsrd_remove_iface(char* name, int mode, char *Ipv4Broadcast);

/**
 * @brief adds a host network address to OLSRd
 * @param family the address family
 * @param address the network address for the interface
 * @param netmask the netmask for the interface
 */
int co_olsrd_add_hna(const int family, const char *address, const char *netmask);

/**
 * @brief remves a host network address from OLSRd
 * @param family the address family
 * @param address the network address for the interface
 * @param netmask the netmask for the interface
 */
int co_olsrd_remove_hna(int family, char *address, char *netmask);

/**
 * @brief prints OLSR configuration info from file (currently unimplemented) 
 * @param filename the configuration file
 */
int co_olsrd_print_conf(const char *filename);

/**
 * @brief initiates the OLSR daemon when a new process gets created (currently unimplemented)
 * @param self process to be called
 */
int co_olsrd_init(co_obj_t *self);
