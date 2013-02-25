/*! 
 *
 * \file msg.c 
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

#include <stdlib.h>
#include <stddef.h>
#include <stdbool.h>
#include <string.h>
#include <inttypes.h>
#include <arpa/inet.h>
#include "debug.h"
#include "util.h"
#include "msg.h"

const char *commands[] = { "HELP", "LIST", "LOAD" };
const size_t command_num = 3;


msg_t *msg_create(const char *target, const char *payload) {
  size_t target_size = -1;
  size_t payload_size = -1;
	msg_t *message = malloc(sizeof(msg_t));
  message->type = 0;
  target_size = strlen(target);
  CHECK((target_size < MSG_TARGET_SIZE) && (target_size != -1), "Invalid target size!");
  if(payload == NULL) {
    payload_size = 0;
  } else payload_size = strlen(payload);
  CHECK((payload_size < MSG_PAYLOAD_SIZE) && (payload_size != -1), "Invalid payload size!");
  memmove(message->target, target, target_size);
  memmove(message->payload, payload, payload_size);
  message->size = payload_size;
  DEBUG("Created message with size %d, command #%s, type #%d, and payload %s", message->size, message->target, message->type, message->payload);
  return message;

error:
  free(message);
  return NULL;
}

char *msg_pack(const msg_t *input) {
  DEBUG("Packing message.");
  uint16_t tmp;
  char *output = malloc(sizeof(msg_t));
  char *cursor = output;

  tmp = htons(input->size);
  memmove(cursor, &tmp, sizeof(input->size));
  cursor += sizeof(input->size);

  tmp = htons(input->type);
  memmove(cursor, &tmp, sizeof(input->type));
  cursor += sizeof(input->type);

  memmove(cursor, input->target, sizeof(input->target));
  cursor += sizeof(input->target);

  memmove(cursor, input->payload, sizeof(input->payload));
  return output;
}

msg_t *msg_unpack(const char *input) {
  CHECK_MEM(input);
  uint16_t tmp = 0;
  msg_t *output = malloc(sizeof(msg_t));
  //char input_tmp[strlen(input) + 1];
  //strcpy(input_tmp, input);
  ///DEBUG("input_tmp: %s", input_tmp);
  const char *cursor = input;

  memmove(&tmp, cursor, sizeof(output->size));
  output->size = ntohs(tmp);
  DEBUG("output->size: %d", output->size);
  cursor += sizeof(output->size);

  memmove(&tmp, cursor, sizeof(output->type));
  output->type = ntohs(tmp);
  cursor += sizeof(output->type);

  memmove(output->target, cursor, sizeof(output->target));
  cursor += sizeof(output->target);

  memmove(output->payload, cursor, sizeof(output->payload));

  return output;
error:
  return NULL;
}
