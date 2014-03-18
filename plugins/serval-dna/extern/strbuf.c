/**
 *       @file  strbuf.c
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

#include "strbuf.h"

strbuf __strbuf_init(strbuf sb, char *buffer, ssize_t size)
{
  sb->start = buffer;
  sb->end = size >= 0 ? sb->start + size - 1 : NULL;
  return strbuf_reset(sb);
}

strbuf __strbuf_reset(strbuf sb)
{
  sb->current = sb->start;
  if (sb->start)
    *sb->start = '\0';
  return sb;
}

strbuf __strbuf_puts(strbuf sb, const char *text)
{
  if (sb->start) {
    if (!sb->end) {
      while ((*sb->current = *text)) {
	++sb->current;
	++text;
      }
    } else if (sb->current < sb->end) {
      register size_t n = sb->end - sb->current;
      while (n-- && (*sb->current = *text)) {
	++sb->current;
	++text;
      }
      *sb->current = '\0';
    }
  }
  while (*text++)
    ++sb->current;
  return sb;
}

strbuf __strbuf_putc(strbuf sb, char ch)
{
  if (sb->start && (!sb->end || sb->current < sb->end)) {
    sb->current[0] = ch;
    sb->current[1] = '\0';
  }
  ++sb->current;
  return sb;
}

int __strbuf_sprintf(strbuf sb, const char *fmt, ...)
{
  va_list ap;
  va_start(ap, fmt);
  int n = __strbuf_vsprintf(sb, fmt, ap);
  va_end(ap);
  return n;
}

int __strbuf_vsprintf(strbuf sb, const char *fmt, va_list ap)
{
  int n;
  if (sb->start && !sb->end) {
    n = vsprintf(sb->current, fmt, ap);
  } else if (sb->start && sb->current < sb->end) {
    int space = sb->end - sb->current + 1;
    n = vsnprintf(sb->current, space, fmt, ap);
    if (n >= space)
      *sb->end = '\0';
  } else {
    char tmp[1];
    n = vsnprintf(tmp, sizeof tmp, fmt, ap);
  }
  if (n != -1)
    sb->current += n;
  return n;
}

inline int __strbuf_overrun(const_strbuf sb) {
  return sb->end && sb->current > sb->end;
}
inline char *__strbuf_str(const_strbuf sb) {
  return sb->start;
}
inline size_t __strbuf_count(const_strbuf sb) {
  return sb->current - sb->start;
}
inline size_t __strbuf_len(const_strbuf sb) {
  return __strbuf_end(sb) - sb->start;
}
inline char *__strbuf_end(const_strbuf sb) {
  return sb->end && sb->current > sb->end ? sb->end : sb->current;
}