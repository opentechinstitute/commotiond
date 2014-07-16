/* vim: set ts=2 expandtab: */
/**
 *       @file  cmd.h
 *      @brief  a mechanism for registering and controlling display of commands
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
 * it under the terms of the GNU Affero General Public License as
 * published by the Free Software Foundation, either version 3 of the
 * License, or (at your option) any later version.
 * 
 * Commotion is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU Affero General Public License for more details.
 * 
 * You should have received a copy of the GNU Affero General Public License
 * along with Commotion.  If not, see <http://www.gnu.org/licenses/>.
 *
 * =====================================================================================
 */

#ifndef _CMD_H
#define _CMD_H

#include <stdlib.h>
#include <stddef.h>
#include <stdint.h>
#include "obj.h"

#define CMD(N) static int cmd_##N(co_obj_t *self, co_obj_t **output, co_obj_t *params)

#define CMD_REGISTER(N, U, D) co_cmd_register(#N, sizeof(#N), U, sizeof(U), D, sizeof(D), cmd_##N )

#define CMD_OUTPUT(K, V) if(*output == NULL) *output = co_tree16_create(); co_tree_insert(*output, K, sizeof(K), V)

#define HOOK(N) static int hook_##N##(co_obj_t *self, co_obj_t **output, co_obj_t *params)

#define HOOK_REGISTER(N, C) co_cmd_hook_str(N, sizeof(N), hook_##N)

typedef struct co_cmd_t co_cmd_t;

void co_cmds_shutdown(void);

int co_cmds_init(const size_t index_size);

/**
 * @struct co_cmd_t
 * @brief a struct containing all the relevant information for a specific command
 */
struct co_cmd_t {
  co_obj_t _header;
  uint8_t _exttype;
  uint8_t _len;
  co_cb_t exec; /**< pointer to the command's function */
  co_obj_t *name; /**< command name */
  co_obj_t *usage; /**< usage syntax */
  co_obj_t *desc; /**< description */
  co_obj_t *hooks;
} __attribute__((packed));

int co_cmd_register(const char *name, const size_t nlen, const char *usage, const size_t ulen, const char *desc, const size_t dlen, co_cb_t handler); 

/**
 * @brief executes a command by running the function linked to in the command struct
 * @param key the name of the command
 * @param output return object of command
 */
int co_cmd_exec(co_obj_t *key, co_obj_t **output, co_obj_t *param);

/**
 * @brief returns command usage format
 * @param key command name
 */
co_obj_t *co_cmd_usage(co_obj_t *key); 

/**
 * @brief returns command description (what the command does)
 * @param key the name of the command
 */
co_obj_t *co_cmd_desc(co_obj_t *key);

/**
 * @brief hooks callback function into a command
 * @param key the name of the command
 * @param cb reference to callback function
 */
int co_cmd_hook(const co_obj_t *key, co_obj_t *cb);

/**
 * @brief hooks callback function into a command
 * @param key the name of the command
 * @param klen length of key string
 * @param cb reference to callback function
 */
int co_cmd_hook_str(const char *key, const size_t klen, co_obj_t *cb);

/**
 * @brief process all registered commands with given iterator
 * @param iter iterator function reference
 * @param context other parameters passed to iterator
 */
int co_cmd_process(co_iter_t iter, void *context);


/**
 * @brief get the command object
 * @param kstr the name
 * @param klen the lenght of the name
 */
co_obj_t *co_cmd_get(const char *kstr, size_t klen);

#endif
