#include <stdio.h>

void _init(void);
co_obj_t *_name(co_obj_t *self, co_obj_t *params);
void teardown(void);
int serval_socket_cb(void *self, void *context);
int serval_timer_cb(void *self, void *context);