/* vim: set ts=2 expandtab: */
/**
 *       @file  process.h
 *      @brief  a simple object-oriented process manager
 *              object model inspired by Zed Shaw
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

#ifndef _PROCESS_H
#define _PROCESS_H

#include <stdlib.h>
#include <stdbool.h>
#include "util.h"

typedef enum {
  STOPPING = 0,
  STOPPED =1,
  STARTING = 2,
  STARTED = 3,
  FAILED = -1
} co_process_state_t;

typedef struct {
  int pid;
  bool registered;
  bool use_watchdog;
  co_process_state_t state;
  char *name;
  char *pid_file;
  char *exec_path;
  char *run_path;
  int input;
  int output;
  int (*init)(void *self);
  int (*destroy)(void *self);
  int (*start)(void *self, char *argv[]);
  int (*stop)(void *self);
  int (*restart)(void *self);
} co_process_t;

co_process_t *co_process_create(size_t size, co_process_t proto, const char *name, const char *pid_file, const char *exec_path, const char *run_path);

int co_process_destroy(void *self);

int co_process_start(void *self, char *argv[]);

int co_process_stop(void *self);

int co_process_restart(void *self);

#endif
