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
#define MAX_COMMANDS 32

/**
 * @brief pointer to a command's function
 */
typedef char *(*co_cmd_handler_t)(void *self, char *argv[], int argc);

/**
 * @struct co_cmd_t
 * @brief a struct containing all the relevant information for a specific command
 */
typedef struct {
  int mask; /**< permissions mask for the command */
  char *name; /**< command name */
  char *usage; /**< usage syntax */
  char *description; /**< description */
  co_cmd_handler_t exec; /**< pointer to the command's function */
} co_cmd_t;

/**
 * @brief adds a command
 * @param name command name
 * @param handler location of the function used by the command
 * @param usage usage syntax for the command
 * @param description description of the command
 * @param mask permissions mask
 * 
 */
void co_cmd_add(char *name, co_cmd_handler_t handler, char *usage, char *description, int mask);

/**
 * @brief executes a command by running the function linked to in the command struct
 * @param name command name
 * @param argv[] arguments passed to command
 * @param arc number of arguments passed
 * @param mask permissions mask
 */
char *co_cmd_exec(char *name, char *argv[], int argc, int mask);

/**
 * @brief prints command usage format
 * @param name command name
 * @param mask permissions mask
 */
char *co_cmd_usage(char *name, int mask);

/**
 * @brief prints command description (what the command does)
 * @param name command name
 * @param mask permissions mask
 */
char *co_cmd_description(char *name, int mask);

/**
 * @brief prints help text for command
 * @param self pointer to cmd_help struct
 * @param argv[] the name of the command
 * @param argc number of arguments passed
 */
char *cmd_help(void *self, char *argv[], int argc);

/**
 * @brief lists profiles available
 * @param self pointer to cmd_list_profiles struct
 * @param argv[] directory where profiles are stored
 * @param argc number of arguments passed
 */
char *cmd_list_profiles(void *self, char *argv[], int argc);

/**
 * @brief Generates a local ip in the 169.254.0.0 address range (to be used for the "thisnode" alais)
 */
char *cmd_generate_local_ip();

/**
 * @brief Brings up the wireless interface and configures it using the default settings or a specified profile.
 * @details sets node id, ip address, ssid, bssid and channel, and sets ad hoc mode.
 * @param self pointer to cmd_up struct
 * @param argv profile name (if any)
 * @param argc number of arguments passed
 */
char *cmd_up(void *self, char *argv[], int argc);

/**
 * @brief brings down wireless interface
 * @param self pointer to cmd_down struct
 * @param argv[] node id, ip address, ssid, bssid and channel, and sets ad hoc mode
 * @param argc number of arguments
 * @warning if argc is greater than 1, returns command usage
 */
char *cmd_down(void *self, char *argv[], int argc);

/**
 * @brief Displays current profile
 * @param self pointer to cmd_status struct
 * @param argv[] argument passed
 * @param argc number of arguments
 * @warning if argc is greater than 1, returns command usage
 */
char *cmd_status(void *self, char *argv[], int argc);

/**
 * @brief Displays current wireless interface configurations. If using a profile, returns profile settings, else returns default settings.
 * @param self pointer to cmd_state struct
 * @param argv[] arguments passed
 * @param argc number of arguments
 */
char *cmd_state(void *self, char *argv[], int argc);

/**
 * @brief reads node id from profile
 * @param self pointer to cmd_nodeid struct
 * @param argv[] profile name from which to get node id
 * @param argc number of arguments
 */
char *cmd_nodeid(void *self, char *argv[], int argc);

/**
 * @brief sets node id from device's mac address (if no id specified by profile)
 * @param argv[] device's mac address
 * @param argc number of arguments
 */
char *cmd_set_nodeid_from_mac(void *self, char *argv[], int argc);
#endif
