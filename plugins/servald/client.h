#ifndef SERVAL_CLIENT_H
#define SERVAL_CLIENT_H

#include "obj.h"

int register_commands(void);
co_obj_t *serval_cmd_handler(co_obj_t *self, co_obj_t *params);

#endif