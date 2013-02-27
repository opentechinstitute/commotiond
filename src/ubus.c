/**
 *       @file  ubus.c
 *      @brief  
 *
 * Detailed description starts here.
 *
 *     @author  Josh King (jheretic), jking@chambana.net
 *
 *   @internal
 *     Created  02/25/2013
 *    Revision  $Id: doxygen.templates,v 1.3 2010/07/06 09:20:12 mehner Exp $
 *    Compiler  gcc/g++
 *     Company  The Open Technology Institute
 *   Copyright  Copyright (c) 2013, Josh King
 *
 * This source code is released for free distribution under the terms of the
 * GNU General Public License as published by the Free Software Foundation.
 * =====================================================================================
 */

#include <stdlib.h>
#include <stdbool.h>
#include <libubox/blobmsg_json.h>
#include <libubus.h>
#include "socket.h"
#include "command.h"
#include "debug.h"
#include "ubus.h"
#include "util.h"

static struct blob_buf b;


enum {
	CO_ID,
	CO_PAYLOAD,
	__CO_MAX
};

static const struct blobmsg_policy commotion_policy[] = {
	[CO_ID] = { .name = "id", .type = BLOBMSG_TYPE_INT32 },
	[CO_PAYLOAD] = { .name = "payload", .type = BLOBMSG_TYPE_STRING }
};

static int co_ubus_handler(struct ubus_context *ctx, struct ubus_object *obj,
		      struct ubus_request_data *req, const char *method,
		      struct blob_attr *msg)
{
	struct blob_attr *tb[__CO_MAX];
  char *payload = NULL;
  char *reply = NULL;

	blobmsg_parse(commotion_policy, __CO_MAX, tb, blob_data(msg), blob_len(msg));
	if (!tb[CO_ID]) return UBUS_STATUS_INVALID_ARGUMENT;
  if (tb[CO_PAYLOAD]) payload = blobmsg_data(tb[CO_PAYLOAD]);

  reply = command_exec(method, payload, 0); 

  blob_buf_init(&b, 0);
	blobmsg_add_string(&b, "payload", reply);

  req = malloc(sizeof(struct ubus_request_data));
   
  return ubus_send_reply(ctx, req, b.head);
}

static const struct ubus_method commotion_methods[] = {
	UBUS_METHOD("echo", co_ubus_handler, commotion_policy),
};

static struct ubus_object_type commotion_object_type =
	UBUS_OBJECT_TYPE("commotion", commotion_methods);

static struct ubus_object commotion_object = {
	.name = "commotion",
	.type = &commotion_object_type,
	.methods = commotion_methods,
	.n_methods = ARRAY_SIZE(commotion_methods),
};

int ubus_socket_init(void *self) {
  DEBUG("Initializing ubus_socket struct.");
  if(self) {
    socket_t *this = self;
    this->fd = -1;
    this->rfd = -1;
    this->local = NULL;
    this->remote = NULL;
    this->fd_registered = false;
    this->rfd_registered = false;
    this->listen = false;
    this->uri = strdup("ubus://");
    return 1;
  } else return 0;
}

int ubus_socket_bind(void *self, const char *endpoint) {
  ubus_socket_t *this = self;  
  CHECK((this->ctx = ubus_connect(endpoint)) != NULL, "Failed to get ubus socket context.");
  CHECK(!ubus_add_object(this->ctx, &commotion_object), "Failed to add object to ubus.");
  this->_(fd) = this->ctx->sock.fd;
  this->_(listen) = true;
  if(this->_(register_cb)) this->_(register_cb)(this, NULL);
  return 1;
error:
  free(this->ctx);
  return 0;
}

socket_t ubus_socket_proto = {
  .init = ubus_socket_init,
  .bind = ubus_socket_bind
};
