/* vim: set ts=2 expandtab: */
/**
 *       @file  msg.h
 *      @brief  a simple message serialization library
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

#ifndef _MSG_H
#define _MSG_H

#include <stdlib.h>
#include <stddef.h>
#include <inttypes.h>
#include "obj.h"

size_t co_request_alloc(char *output, const size_t olen, const co_obj_t *method, co_obj_t *param);
size_t co_response_alloc(char *output, const size_t olen, const uint32_t id, const co_obj_t *error, co_obj_t *result);

#endif
