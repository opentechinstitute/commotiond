#ifndef SERVALD_H
#define SERVALD_H

#define PATH_MAX 4096

int co_plugin_register(co_obj_t *self, co_obj_t **output, co_obj_t *params);
int co_plugin_init(co_obj_t *self, co_obj_t **output, co_obj_t *params);
int co_plugin_name(co_obj_t *self, co_obj_t **output, co_obj_t *params);
int co_plugin_shutdown(co_obj_t *self, co_obj_t **output, co_obj_t *params);
int serval_socket_cb(co_obj_t *self, co_obj_t *context);
int serval_timer_cb(co_obj_t *self, co_obj_t **output, co_obj_t *context);
int serval_schema(co_obj_t *self, co_obj_t **output, co_obj_t *params);

#endif