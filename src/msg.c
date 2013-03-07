/* vim: set ts=2 expandtab: */
/**
 *       @file  msg.c
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

#include <stdlib.h>
#include <stddef.h>
#include <stdbool.h>
#include <string.h>
#include <inttypes.h>
#include <arpa/inet.h>
#include "debug.h"
#include "util.h"
#include "msg.h"

co_msg_t *co_msg_create(const char *target, const char *payload) {
  size_t target_size = 0;
  size_t payload_size = 0;
	co_msg_t *message = malloc(sizeof(co_msg_t));
  message->header.size = sizeof(co_msg_header_t);
  DEBUG("HEADER SIZE: %d", message->header.size);
  message->header.type = 0;
  CHECK(((target_size = strstrip(target, message->target, sizeof(message->target))) != -1), "Invalid target size!");
  DEBUG("TARGET SIZE: %d", (int)target_size);
  if(payload != NULL) CHECK(((payload_size = strstrip(payload, message->payload, sizeof(message->payload))) != -1), "Invalid payload size!");
  DEBUG("PAYLOAD SIZE: %d", (int)payload_size);
  message->header.size += target_size + payload_size;
  DEBUG("Created message with size %d, command #%s, type %d, and payload %s", message->header.size, message->target, message->header.type, message->payload);
  return message;

error:
  free(message);
  return NULL;
}

char *co_msg_pack(const co_msg_t *input) {
  DEBUG("Packing message.");
  uint16_t tmp;
  char *output = malloc(input->header.size);
  char *cursor = output;

  tmp = htons(input->header.size);
  memmove(cursor, &tmp, sizeof(input->header.size));
  cursor += sizeof(input->header.size);

  tmp = htons(input->header.type);
  memmove(cursor, &tmp, sizeof(input->header.type));
  cursor += sizeof(input->header.type);

  memmove(cursor, input->target, sizeof(input->target));
  cursor += sizeof(input->target);

  memmove(cursor, input->payload, sizeof(input->payload));
  return output;
}

co_msg_t *co_msg_unpack(const char *input) {
  CHECK_MEM(input);
  uint16_t tmp = 0;
  co_msg_t *output = malloc(sizeof(co_msg_t));
  //char input_tmp[strlen(input) + 1];
  //strcpy(input_tmp, input);
  ///DEBUG("input_tmp: %s", input_tmp);
  const char *cursor = input;

  memmove(&tmp, cursor, sizeof(output->header.size));
  output->header.size = ntohs(tmp);
  DEBUG("output->header.size: %d", output->header.size);
  cursor += sizeof(output->header.size);

  memmove(&tmp, cursor, sizeof(output->header.type));
  output->header.type = ntohs(tmp);
  cursor += sizeof(output->header.type);

  memmove(output->target, cursor, sizeof(output->target));
  cursor += sizeof(output->target);

  memmove(output->payload, cursor, sizeof(output->payload));

  return output;
error:
  return NULL;
}
