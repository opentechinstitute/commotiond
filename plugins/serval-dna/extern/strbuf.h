/**
 *       @file  strbuf.h
 *      @brief  minimal Serval MDP client functionality, used in the
 *                commotion_serval-sas library
 *
 *     @author  Dan Staples (dismantl), danstaples@opentechinstitute.org
 *
 *   @internal
 *     Created  3/17/2014
 *    Compiler  gcc/g++
 *     Company  The Open Technology Institute
 *   Copyright  Copyright (c) 2013, Dan Staples
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

/*
Serval string buffer helper functions.
Copyright (C) 2012 Serval Project Inc.

This program is free software; you can redistribute it and/or
modify it under the terms of the GNU General Public License
as published by the Free Software Foundation; either version 2
of the License, or (at your option) any later version.

This program is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with this program; if not, write to the Free Software
Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA.
*/

#ifndef __CO_SERVAL_STRBUF_H
#define __CO_SERVAL_STRBUF_H

#include <sys/types.h>
#include <stdint.h> // for SIZE_MAX on Debian/Unbuntu/...
#include <limits.h> // for SIZE_MAX on Android
#include <stdio.h>
#include <string.h>
#include <stdarg.h>
#include <alloca.h>
#include <assert.h>

#include "config.h"
#include "serval/strbuf.h"

#define __strbuf_local(buf,len) __strbuf_init(alloca(SIZEOF_STRBUF), (buf), (len))

strbuf __strbuf_reset(strbuf sb);
strbuf __strbuf_init(strbuf sb, char *buffer, ssize_t size);
strbuf __strbuf_puts(strbuf sb, const char *text);
strbuf __strbuf_putc(strbuf sb, char ch);
int __strbuf_sprintf(strbuf sb, const char *fmt, ...) __attribute__((format(printf, 2, 3)));
strbuf __strbuf_trunc(strbuf sb, int offset);

int __strbuf_overrun(const_strbuf sb);
char *__strbuf_str(const_strbuf sb);
size_t __strbuf_count(const_strbuf sb);

#endif