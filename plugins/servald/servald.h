#ifndef SERVALD_H
#define SERVALD_H

int _init(co_obj_t *self, co_obj_t **output, co_obj_t *params);
int _name(co_obj_t *self, co_obj_t **output, co_obj_t *params);
void teardown(void);
int serval_socket_cb(co_obj_t *self, co_obj_t *context);
int serval_timer_cb(co_obj_t *self, co_obj_t **output, co_obj_t *context);

#endif