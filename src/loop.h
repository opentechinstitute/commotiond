/* vim: set ts=2 expandtab: */
/**
 *       @file  loop.h
 *      @brief  a simple object-oriented event loop
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

#ifndef _LOOP_H
#define _LOOP_H

#include <stdlib.h>
#include "process.h"
#include "socket.h"

#define LOOP_MAXPROC 20
#define LOOP_MAXSOCK 20
#define LOOP_MAXEVENT 64
#define LOOP_TIMEOUT 5
#define LOOP_MAXTIMER 20

typedef struct {
  co_obj_t _header;
  uint8_t _exttype;
  uint8_t _len;
  bool pending;
  struct timeval deadline;
  co_cb_t timer_cb;
  void *ptr;
} co_timer_t;

//Public functions

/**
 * @brief sets up the event loop
 */
int co_loop_create(void);

/**
 * @brief closes the event loop
 */
int co_loop_destroy(void);

/**
 * @brief starts the event loop, listening for messages on sockets via _co_loop_poll_sockets
 */
void co_loop_start(void);

/**
 * @brief stops the event loop
 */
void co_loop_stop(void);

/**
 * @brief adds a process to the event loop (for it to listen for)
 * @param proc the process to be added
 */
int co_loop_add_process(co_obj_t *proc);

/**
 * @brief removes a process from the event loop
 * @param pid the id of the process to be removed
 */
int co_loop_remove_process(pid_t pid);

/**
 * @brief adds a new socket to the event loop (for it to listen on)
 * @param new_sock the new socket to be added
 * @param context a co_obj_t context pointer (currently unused)
 */
int co_loop_add_socket(co_obj_t *new_sock, co_obj_t *context);

/**
 * @brief removes a socket from the event loop
 * @param old_sock the socket to be removed
 * @param context a co_obj_t context pointer (currently unused)
 */
int co_loop_remove_socket(co_obj_t *old_sock, co_obj_t *context);

/**
 * @brief schedules a new timer with the event loop
 * @param timer the timer to schedule
 * @param context a co_obj_t context pointer (currently unused)
 */
int co_loop_add_timer(co_obj_t *new_timer, co_obj_t *context);

/**
 * @brief removes a timer from the event loop
 * @param old_timer the timer to remove from list
 * @param context a co_obj_t context pointer (currently unused)
 */
int co_loop_remove_timer(co_obj_t *old_timer, co_obj_t *context);

/**
 * @brief sets timer to expire in msecs from now
 * @param timer the timer to set
 * @param msecs number of milliseconds
 * @param context a co_obj_t context pointer (currently unused)
 */
int co_loop_set_timer(co_obj_t *timer, long msecs, co_obj_t *context);

/**
 * @brief malloc and initialize a timer
 * @param size timer struct size
 * @param proto timer protocol
 */
co_obj_t *co_timer_create(struct timeval deadline, co_cb_t timer_cb, void *ptr);

#endif
