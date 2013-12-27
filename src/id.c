/* vim: set ts=2 expandtab: */
/**
 *       @file  id.c
 *      @brief  simple interface for managing node id
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

#include <stdint.h>
#include <arpa/inet.h>
#include "debug.h"
#include "id.h"

static nodeid_t nodeid = {0};

void co_id_set_from_mac(const unsigned char *mac, int mac_size) {
  nodeid.id = 0;
  CHECK(mac_size == 6, "MAC size is not six.");
  DEBUG("Received MAC Address : %02x:%02x:%02x:%02x:%02x:%02x\n",
                mac[0], mac[1], mac[2], mac[3], mac[4], mac[5]);
  /* Load last four bytes of MAC address into nodeid struct*/
  for(int i = 0; i < 4; i++) {
    nodeid.bytes[i] = mac[i + 2];
  }
error:
  return;
}

void co_id_set_from_int(const uint32_t n) {
  nodeid.id = htonl(n);
  return;
}

nodeid_t co_id_get(void) {
  return nodeid;
}
