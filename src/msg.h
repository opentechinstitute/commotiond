/*! 
 *
 * \file msg.h 
 *
 * \brief a simple message packing library
 *
 * \author Josh King <jking@chambana.net>
 * 
 * \date
 *
 * \copyright This file is part of Commotion, Copyright(C) 2012-2013 Josh King
 * 
 *            Commotion is free software: you can redistribute it and/or modify
 *            it under the terms of the GNU General Public License as published 
 *            by the Free Software Foundation, either version 3 of the License, 
 *            or (at your option) any later version.
 * 
 *            Commotion is distributed in the hope that it will be useful,
 *            but WITHOUT ANY WARRANTY; without even the implied warranty of
 *            MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *            GNU General Public License for more details.
 * 
 *            You should have received a copy of the GNU General Public License
 *            along with Commotion.  If not, see <http://www.gnu.org/licenses/>.
 * 
 * */

#ifndef _MSG_H
#define _MSG_H

#include <stdlib.h>
#include <stddef.h>
#include <inttypes.h>

#define CMD_HELP 0
#define CMD_LIST_PROFILES 1
#define CMD_LOAD_PROFILE 2

extern const char *commands[];
extern const size_t command_num;

#define MSG_TYPE_STRING 0
#define MSG_TYPE_INT 1
#define MSG_TARGET_SIZE 32
#define MSG_PAYLOAD_SIZE 220

typedef struct {
  uint16_t size;
  uint16_t type;
  char target[MSG_TARGET_SIZE];;
  char payload[MSG_PAYLOAD_SIZE];
} msg_t;

msg_t *msg_create(const char *target, const char *payload);
char *msg_pack(const msg_t *input);
msg_t *msg_unpack(const char *input);

#endif
