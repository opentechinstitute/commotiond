/*! 
 *
 * \file process.h 
 *
 * \brief a simple object-oriented process manager
 *
 *        object model inspired by Zed Shaw
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
} process_state_t;

typedef struct {
  int pid;
  bool registered;
  bool use_watchdog;
  process_state_t state;
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
} process_t;

process_t *process_create(size_t size, process_t proto, const char *name, const char *pid_file, const char *exec_path, const char *run_path);

int process_destroy(void *self);

int process_start(void *self, char *argv[]);

int process_stop(void *self);

int process_restart(void *self);

#endif
