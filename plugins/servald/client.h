#ifndef SERVAL_CLIENT_H
#define SERVAL_CLIENT_H

#include "obj.h"

#define BUF_SIZE 1024

int serval_register(void);
co_obj_t *serval_handler(co_obj_t *self, co_obj_t *params);

#endif