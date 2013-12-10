#ifndef SERVALD_H
#define SERVALD_H

void _init(void);
co_obj_t *_name(co_obj_t *self, co_obj_t *params);
void teardown(void);
int serval_socket_cb(co_obj_t *self, co_obj_t *context);
co_obj_t *serval_timer_cb(co_obj_t *self, co_obj_t *context);

#endif