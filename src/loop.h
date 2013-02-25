/*! 
 *
 * \file loop.h 
 *
 * \brief a simple object-oriented event loop
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

#ifndef _LOOP_H
#define _LOOP_H

#include <stdlib.h>
#include "extern/list.h"
#include "process.h"
#include "socket.h"

#define LOOP_MAXPROC 20
#define LOOP_MAXSOCK 20
#define LOOP_MAXEVENT 64


//Public functions

int loop_create(void);

int loop_destroy(void);

void loop_start(void);

void loop_stop(void);

int loop_add_process(process_t *proc);

int loop_remove_process(pid_t pid);

int loop_add_socket(void *new_sock, void *context);

int loop_remove_socket(void *old_sock, void *context);

#endif
