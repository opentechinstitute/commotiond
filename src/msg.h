/* vim: set ts=2 expandtab: */
/**
 *       @file  msg.h
 *      @brief  a simple message serialization library
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

#ifndef _MSG_H
#define _MSG_H

#include <stdlib.h>
#include <stddef.h>
#include <inttypes.h>

#define CMD_HELP 0
#define CMD_LIST_PROFILES 1
#define CMD_LOAD_PROFILE 2

#define MSG_TYPE_STRING 0
#define MSG_TYPE_INT 1
#define MSG_TARGET_SIZE 32
#define MSG_MAX_PAYLOAD 512
#define MSG_PAYLOAD_DELIM ':'


/**
 * @struct co_msg_header_t
 * @brief contains the header info (size and type) of a commotion message
 */
typedef struct {
  uint16_t size;
  uint8_t type;
} co_msg_header_t;


/**
 * @struct co_msg_t 
 * @brief contains the target (destination) and payload (contents) of a commotion message
 */
typedef struct {
  co_msg_header_t header;
  char target[MSG_TARGET_SIZE];
  char payload[MSG_MAX_PAYLOAD];
} co_msg_t;

/**
 * @brief packages a message in a commotion-readable format (header, target and payload) and checks for valid inputs
 * @param target the intended socket for the message
 * @param payload the contents of the message (eg. commands, configuration information, etc.)
 */
co_msg_t *co_msg_create(const char *target, const char *payload);

/**
 * @brief loads commotion message into message struct
 * @param input input from user to be packaged as a commotion message
 */
char *co_msg_pack(const co_msg_t *input);

/**
 * @brief unpacks commage message from message struct
 * @param input commotion message struct to be sent to output
 */
co_msg_t *co_msg_unpack(const char *input);

#endif
