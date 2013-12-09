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
#include "obj.h"

/**
 * @enum co_process_state_t defines the state of the process
 */
typedef enum {
  _STOPPING = 0,
  _STOPPED =1,
  _STARTING = 2,
  _STARTED = 3,
  _FAILED = -1
} co_process_state_t;

/**
 * @struct co_process_t process struct, including process id, run path and state information
 */
typedef struct {
  co_obj_t _header;
  uint8_t _exttype;
  uint8_t _len;
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
  int (*init)(co_obj_t *self);
  int (*destroy)(co_obj_t *self);
  int (*start)(co_obj_t *self, char *argv[]);
  int (*stop)(co_obj_t *self);
  int (*restart)(co_obj_t *self);
} co_process_t;

/**
 * @brief creates a new commotion process
 * @param size size of the process
 * @param co_process_t protocol
 * @param name name of the process
 * @param pid_file the lockfile where the process id is stored
 * @param exec_path the execution path
 * @param run_path the run path
 * @return co_process_t to be registered with the daemon
 */
co_obj_t *co_process_create(size_t size, co_process_t proto, const char *name, const char *pid_file, const char *exec_path, const char *run_path);

/**
 * @brief removes a process from commotiond
 * @param self pointer to the process' struct
 */
int co_process_destroy(co_obj_t *self);

/**
 * @brief starts a selected process
 * @param self pointer to the process' struct
 * @param argv[] execution path for the process
 */
int co_process_start(co_obj_t *self, char *argv[]);

/**
 * @brief stops a running process
 * @param self pointer to the process' struct
 */
int co_process_stop(co_obj_t *self);

/**
 * @brief restarts a process
 * @param self pointer to the process' struct
 */
int co_process_restart(co_obj_t *self);

#endif
