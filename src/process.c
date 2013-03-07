/* vim: set ts=2 expandtab: */
/**
 *       @file  process.c
 *      @brief  a simple object-oriented process manager
 *
 *     @author  Josh King (jheretic), jking@chambana.net
 *              object model inspired by Zed Shaw
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

#include <stdlib.h>
#include <stddef.h>
#include <stdbool.h>
#include <unistd.h>
#include <sys/types.h>
#include <signal.h>
#include "debug.h"
#include "process.h"
#include "util.h"

co_process_t *co_process_create(const size_t size, co_process_t proto, const char *name, const char *pid_file, const char *exec_path, const char *run_path) {
  if(!proto.init) proto.init = NULL;
  if(!proto.destroy) proto.destroy = co_process_destroy;
  if(!proto.start) proto.start = co_process_start;
  if(!proto.stop) proto.stop = co_process_stop;

  co_process_t *new_proc = malloc(size);
  *new_proc = proto;

  CHECK_MEM(new_proc);
  
  new_proc->name = strdup(name); 
  new_proc->pid_file = strdup(pid_file); 
  new_proc->exec_path = strdup(exec_path); 
  new_proc->run_path = strdup(run_path); 

  if(!new_proc->init(new_proc)) {
    SENTINEL("Failed to initialize new process.");
  } else {
    return new_proc;
  }

error:
  new_proc->destroy(new_proc);
  return NULL;
}

int co_process_destroy(void *self) {
  CHECK_MEM(self);
  co_process_t *this = self;

  free(this->name);
  free(this->pid_file);
  free(this->exec_path);
  free(this->run_path);

  free(self);

  return 1;

error:
  return 0;
}

int co_process_start(void *self, char *argv[]) {
  CHECK_MEM(self);
  co_process_t *this = self;
  char *exec = NULL;
  CHECK(((exec = this->exec_path) != NULL), "Invalid exec path!");
	int pid = 0;
	int local_stdin_pipe[2], local_stdout_pipe[2];

	if(pipe(local_stdin_pipe)) {
    ERROR("Could not initialize stdin pipe.");
    return 0;
  }
	if(pipe(local_stdout_pipe)) {
    ERROR("Could not initialize stdout pipe.");
    return 0;
  }
	
  if (!(pid = fork())) {
		dup2(local_stdin_pipe[0], 0);
		dup2(local_stdout_pipe[1], 1);

		close(local_stdin_pipe[0]);
		close(local_stdin_pipe[1]);
		close(local_stdout_pipe[0]);
		close(local_stdout_pipe[1]);

		CHECK((execvp(exec, argv) != -1), "Failed to exec!");
	}
	INFO("fork()ed: %d\n", pid);
	this->pid = pid;

	this->input = local_stdin_pipe[1];
	this->output = local_stdout_pipe[0];

	close(local_stdin_pipe[0]);
	close(local_stdout_pipe[1]);

  return 1;

error:
  exit(1);
  return 0;
}

int co_process_stop(void *self) {
  CHECK_MEM(self);
  co_process_t *this = self;
  CHECK(!(kill(this->pid, SIGKILL)), "Failed to kill co_process_t %d.", this->pid);

  return 1;

error:
  return 0;
}

int co_process_restart(void *self) {
  CHECK_MEM(self);
  co_process_t *this = self;
  if(this->stop(this)) this->start(this, NULL);

  return 1;

error:
  return 0;
}
