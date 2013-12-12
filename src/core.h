/* vim: set ts=2 expandtab: */
/**
 *       @file  command.h
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

#ifndef _COMMAND_H
#define _COMMAND_H

#include <stdlib.h>

#define OUTPUT_MAX 1024


/**
 * @brief prints help text for command
 * @param self pointer to cmd_help struct
 * @param argv[] the name of the command
 * @param argc number of arguments passed
 */
int cmd_help(co_obj_t *self, co_obj_t **output, co_obj_t *params); 

/**
 * @brief lists profiles available
 * @param self pointer to cmd_list_profiles struct
 * @param argv[] directory where profiles are stored
 * @param argc number of arguments passed
 */
int cmd_list_profiles(co_obj_t *self, co_obj_t **output, co_obj_t *params);

/**
 * @brief Generates a local ip in the 169.254.0.0 address range (to be used for the "thisnode" alias)
 */
int cmd_generate_local_ip(co_obj_t *self, co_obj_t **output, co_obj_t *params); 

/**
 * @brief Brings up the wireless interface and configures it using the default settings or a specified profile.
 * @details sets node id, ip address, ssid, bssid and channel, and sets ad hoc mode.
 * @param self pointer to cmd_up struct
 * @param argv profile name (if any)
 * @param argc number of arguments passed
 */
int cmd_up(co_obj_t *self, co_obj_t **output, co_obj_t *params);

/**
 * @brief brings down wireless interface
 * @param self pointer to cmd_down struct
 * @param argv[] node id, ip address, ssid, bssid and channel, and sets ad hoc mode
 * @param argc number of arguments
 * @warning if argc is greater than 1, returns command usage
 */
int cmd_down(co_obj_t *self, co_obj_t **output, co_obj_t *params);

/**
 * @brief Displays current profile
 * @param self pointer to cmd_status struct
 * @param argv[] argument passed
 * @param argc number of arguments
 * @warning if argc is greater than 1, returns command usage
 */
int cmd_status(co_obj_t *self, co_obj_t **output, co_obj_t *params);

/**
 * @brief Displays current wireless interface configurations. If using a profile, returns profile settings, else returns default settings.
 * @param self pointer to cmd_state struct
 * @param argv[] arguments passed
 * @param argc number of arguments
 */
int cmd_state(co_obj_t *self, co_obj_t **output, co_obj_t *params);

/**
 * @brief reads node id from profile
 * @param self pointer to cmd_nodeid struct
 * @param argv[] profile name from which to get node id
 * @param argc number of arguments
 */
int cmd_nodeid(co_obj_t *self, co_obj_t **output, co_obj_t *params);

/**
 * @brief sets node id from device's mac address (if no id specified by profile)
 * @param argv[] device's mac address
 * @param argc number of arguments
 */
int cmd_set_nodeid_from_mac(co_obj_t *self, co_obj_t **output, co_obj_t *params);
#endif
