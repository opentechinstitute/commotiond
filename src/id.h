/* vim: set ts=2 expandtab: */
/**
 *       @file  id.h
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
#ifndef _ID_H
#define _ID_H

#include <stdint.h>

/**
 * @struct nodeid_t struct containing node id and last four bytes of MAC address
 */
typedef union {
  uint32_t id;
  uint8_t bytes[4];
} nodeid_t;

/**
 * @brief sets node id using last four bytes of device's MAC address
 * @param mac the MAC address of the device
 * @param mac_size the size of the MAC address
 */
void co_id_set_from_mac(const unsigned char *mac, int mac_size);

/**
 * @brief Converts node id from host byte order to network byte order
 * @param n the node id (4 bytes)
 */
void co_id_set_from_int(const uint32_t n);

/**
 * @brief Returns nodeid
 */
nodeid_t co_id_get(void);

#endif
