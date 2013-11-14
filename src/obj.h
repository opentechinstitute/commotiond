/* vim: set ts=2 expandtab: */
/**
 *       @file  obj.c
 *      @brief  Commotion object model
 *
 *     @author  Josh King (jheretic), jking@chambana.net
 *
 *   @internal
 *      Created  11/07/2013 02:00:15 PM
 *     Compiler  gcc/g++
 * Organization  The Open Technology Institute
 *    Copyright  Copyright (c) 2013, Josh King
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
#include <stdint.h>
#include "debug.h"
#include "extern/halloc.h"

/* Types */
#define _nil 0xc0
#define _false 0xc2
#define _true 0xc3
#define _bin8 0xc4
#define _bin16 0xc5
#define _bin32 0xc6
#define _ext8 0xc7
#define _ext16 0xc8
#define _ext32 0xc9
#define _float32 0xca
#define _float64 0xcb
#define _uint8 0xcc
#define _uint16 0xcd
#define _uint32 0xce
#define _uint64 0xcf
#define _int8 0xd0
#define _int16 0xd1
#define _int32 0xd2
#define _int64 0xd3
#define _str8 0xd9
#define _str16 0xda
#define _str32 0xdb
#define _arr16 0xdc
#define _arr32 0xdd
#define _map16 0xde
#define _map32 0xdf

/* Convenience */
#define CO_TYPE(J) (J->_header._type)

/* Lists */
#define _LIST_NEXT(J) (J->_header._next)
#define _LIST_PREV(J) (J->_header._prev)
#define _LIST_LAST(J) (J->_header._prev)

/* Type checking */
#define IS_NIL(J) (CO_TYPE(J) == _nil)
#define IS_FIXINT(J) (CO_TYPE(J) < 128)
#define IS_BOOL(J) ((CO_TYPE(J) == _false) || (CO_TYPE(J) == _true))
#define IS_BIN(J) ((CO_TYPE(J) == _bin8) || (CO_TYPE(J) == _bin16) || (CO_TYPE(J) == _bin32))
#define IS_EXT(J) ((CO_TYPE(J) == _ext8) || (CO_TYPE(J) == _ext16) || (CO_TYPE(J) == _ext32))
#define IS_FLOAT(J) ((CO_TYPE(J) == _float32) || (CO_TYPE(J) == _float64))
#define IS_UINT(J) ((CO_TYPE(J) == _uint8) || (CO_TYPE(J) == _uint16) || (CO_TYPE(J) == _uint32) || (CO_TYPE(J) == _uint64))
#define IS_INT(J) ((CO_TYPE(J) == _int8) || (CO_TYPE(J) == _int16) || (CO_TYPE(J) == _int32) || (CO_TYPE(J) == _int64))
#define IS_STR(J) ((CO_TYPE(J) == _str8) || (CO_TYPE(J) == _str16) || (CO_TYPE(J) == _str32))
#define IS_LIST(J) ((CO_TYPE(J) == _arr16) || (CO_TYPE(J) == _arr32))
#define IS_MAP(J) ((CO_TYPE(J) == _map16) || (CO_TYPE(J) == _map32))
#define IS_CHAR(J) (IS_BIN(J) || IS_EXT(J) || IS_STR(J))
#define IS_INTEGER(J) (IS_INT(J) || IS_UINT(J) || IS_FIXINT(J))
#define IS_COMPLEX(J) (IS_ARR(J) || IS_MAP(J))

/* Flags */
#define _OBJ_SCHEMA ((1 << 0))
#define _OBJ_EMPTY ((1 << 1))
#define _OBJ_REQUIRED ((1 << 2))

/* Objects */

typedef uint8_t _type_t;

typedef struct
{
  uint8_t _flags;
  /*  For "array" types, which are actually lists. */
  struct co_obj_t *_prev;
  struct co_obj_t *_next;
  _type_t _type;
} co_obj_t;

typedef co_obj_t *(*co_iter_t)(co_obj_t *data, co_obj_t *current, void *context);

/* Type "Nil" declaration */
typedef struct
{
  co_obj_t _header;
} co_nil_t;

int co_nil_alloc(co_nil_t *output);
co_nil_t * co_nil_create(void);

/* Type "Bool" declaration */
typedef struct
{
  co_obj_t _header;
} co_bool_t;

int co_bool_alloc(co_bool_t *output);
co_bool_t * co_bool_create(const bool input);

/* Type "Bin" declaration macros */
#define _DECLARE_CHAR(T, L) typedef struct { co_obj_t _header; uint##L##_t _len; \
  char data[1]; } co_##T##L##_t; int co_##T##L##_alloc(co_##T##L##_t *output, \
  const size_t out_size, const char *input, const size_t in_size ); \
  co_##T##L##_t *co_##T##L##_create(const char *input, const size_t input_size);

_DECLARE_CHAR(bin, 8);

/* Type "Ext" declaration macros */
#define _DEFINE_ext(L) typedef struct { co_obj_t _header; uint##L##_t _len; \
  char data[1]; } co_ext##L##_t; int co_ext##L##_alloc(co_ext##L##_t *output, \
  const size_t out_size, const char *input, const size_t in_size); inline \
  co_ext##L##_t *co_ext##L##_create(const char *input, const size_t input_size);

/* Type "fixint" declaration */
typedef struct
{
  co_obj_t _header;
} co_fixint_t;

int co_fixint_alloc(co_fixint_t *output, const int input);
co_fixint_t * co_fixint_create(const int input);

/* Type "float32" declaration */
typedef struct
{
  co_obj_t _header;
  float data;
} co_float32_t;

int co_float32_alloc(co_float32_t *output, const float input);
co_float32_t * co_float32_create(const double input);

/* Type "float64" declaration */
typedef struct
{
  co_obj_t _header;
  double data;
} co_float64_t;

int co_float64_alloc(co_float64_t *output, const double input);
co_float64_t * co_float64_create(const double input);

/* Type "int" declaration macros */
#define _DECLARE_INTEGER(T, L) typedef struct { co_obj_t _header; T##L##_t data } \
  co_##T##L##_t; int co_##T##L##_alloc(co_##T##L##_t *output, \
  const int##L##_t input); inline co_##T##L##_t *co_##T##L##_create(\
  const int##L##_t input);

/* Type "str" declaration macros */
#define _DEFINE_str(L) typedef struct { co_obj_t _header; uint##L##_t _len; \
  char data[1]; } co_str##L##_t; int co_str##L##_alloc(co_str##L##_t *output, \
  const size_t out_size, const char *input, const size_t in_size );  inline \
  co_str##L##_t *co_str##L##_create(const char *input, const size_t input_size);

/* Type "uint" declaration macros */
#define _DEFINE_uint(L) typedef struct { co_obj_t _header; uint##L##_t data; } \
  co_uint##L##_t; int co_uint##L##_alloc(co_uint##L##_t *output, \
  const uint##L##_t input); inline co_uint##L##_t *co_uint##L##_create(\
  const uint##L##_t input);

void co_free(co_obj_t *object);
