/* vim: set ts=2 expandtab: */
/**
 *       @file  obj.h
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
#ifndef _OBJ_H
#define _OBJ_H
#include <stdlib.h>
#include <stddef.h>
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
#define _fixext1 0xd4
#define _fixext2 0xd5
#define _fixext4 0xd6
#define _fixext8 0xd7
#define _fixext16 0xd8
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
#define _list16 0xdc
#define _list32 0xdd
#define _tree16 0xde
#define _tree32 0xdf

/* Convenience */
#define CO_TYPE(J) (((co_obj_t *)J)->_type)

/* Lists */
#define _LIST_NEXT(J) (((co_obj_t *)J)->_next)
#define _LIST_PREV(J) (((co_obj_t *)J)->_prev)
#define _LIST_LAST(J) (((co_obj_t *)J)->_prev)

/* Type checking */
#define IS_NIL(J) (CO_TYPE(J) == _nil)
#define IS_FIXINT(J) (CO_TYPE(J) < 128)
#define IS_BOOL(J) ((CO_TYPE(J) == _false) || (CO_TYPE(J) == _true))
#define IS_BIN(J) ((CO_TYPE(J) == _bin8) || (CO_TYPE(J) == _bin16) || (CO_TYPE(J) == _bin32))
#define IS_EXT(J) ((CO_TYPE(J) == _ext8) || (CO_TYPE(J) == _ext16) || (CO_TYPE(J) == _ext32))
#define IS_FIXEXT(J) ((CO_TYPE(J) == _fixext1) || (CO_TYPE(J) == _fixext2) || (CO_TYPE(J) == _fixext4) || (CO_TYPE(J) == _fixext8) || (CO_TYPE(J) == _fixext16))
#define IS_FLOAT(J) ((CO_TYPE(J) == _float32) || (CO_TYPE(J) == _float64))
#define IS_UINT(J) ((CO_TYPE(J) == _uint8) || (CO_TYPE(J) == _uint16) || (CO_TYPE(J) == _uint32) || (CO_TYPE(J) == _uint64))
#define IS_INT(J) ((CO_TYPE(J) == _int8) || (CO_TYPE(J) == _int16) || (CO_TYPE(J) == _int32) || (CO_TYPE(J) == _int64))
#define IS_STR(J) ((CO_TYPE(J) == _str8) || (CO_TYPE(J) == _str16) || (CO_TYPE(J) == _str32))
#define IS_LIST(J) ((CO_TYPE(J) == _list16) || (CO_TYPE(J) == _list32))
#define IS_TREE(J) ((CO_TYPE(J) == _tree16) || (CO_TYPE(J) == _tree32))
#define IS_CHAR(J) (IS_BIN(J) || IS_EXT(J) || IS_STR(J))
#define IS_INTEGER(J) (IS_INT(J) || IS_UINT(J) || IS_FIXINT(J))
#define IS_COMPLEX(J) (IS_LIST(J) || IS_TREE(J))

/* Flags */
#define _OBJ_SCHEMA ((1 << 0))
#define _OBJ_EMPTY ((1 << 1))
#define _OBJ_REQUIRED ((1 << 2))

/*-----------------------------------------------------------------------------
 *  Object Declarations
 *-----------------------------------------------------------------------------*/
typedef uint8_t _type_t;

typedef struct co_obj_t co_obj_t;

struct co_obj_t
{
  uint8_t _flags;
  /*  For "list" types. */
  co_obj_t *_prev;
  co_obj_t *_next;
  _type_t _type;
};

typedef co_obj_t *(*co_iter_t)(co_obj_t *data, co_obj_t *current, void *context);


/*-----------------------------------------------------------------------------
 *  Character-array-types Declaration Macros
 *-----------------------------------------------------------------------------*/
#define _DECLARE_CHAR(T, L) typedef struct { co_obj_t _header; uint##L##_t _len; \
  char data[1]; } co_##T##L##_t; int co_##T##L##_alloc(co_obj_t *output, \
  const size_t out_size, const char *input, const size_t in_size, \
  const uint8_t flags ); co_obj_t *co_##T##L##_create(const char *input, \
  const size_t input_size, const uint8_t flags);

_DECLARE_CHAR(bin, 8);
_DECLARE_CHAR(bin, 16);
_DECLARE_CHAR(bin, 32);
_DECLARE_CHAR(str, 8);
_DECLARE_CHAR(str, 16);
_DECLARE_CHAR(str, 32);

/*-----------------------------------------------------------------------------
 *  Extension-types Declaration Macros
 *-----------------------------------------------------------------------------*/
/*
#define _DECLARE_FIXEXT(L) typedef struct { co_obj_t _header; \
  uint8_t type; char data[L]; } co_fixext##L##_t; int co_fixext##L##_alloc(co_obj_t *output, \
  const char *input, const size_t in_size, \
  const uint8_t flags, const uint8_t type ); co_obj_t *co_fixext##L##_create(const char *input, \
  const size_t input_size, const uint8_t flags, const uint8_t type);

_DECLARE_FIXEXT(1);
_DECLARE_FIXEXT(2);
_DECLARE_FIXEXT(4);
_DECLARE_FIXEXT(8);
_DECLARE_FIXEXT(16);

#define _DECLARE_EXT(L) typedef struct { co_obj_t _header; uint##L##_t _len; \
  uint8_t type; char data[1]; } co_ext##L##_t; int co_ext##L##_alloc(co_obj_t *output, \
  const size_t out_size, const char *input, const size_t in_size, \
  const uint8_t flags, const uint8_t type ); co_obj_t *co_ext##L##_create(const char *input, \
  const size_t input_size, const uint8_t flags, const uint8_t type);

_DECLARE_EXT(8);
_DECLARE_EXT(16);
_DECLARE_EXT(32);
*/

/*-----------------------------------------------------------------------------
 *  Integer-types Declaration Macros 
 *-----------------------------------------------------------------------------*/
#define _DECLARE_INTEGER(T, L) typedef struct { co_obj_t _header; T##L##_t data; }\
  co_##T##L##_t; int co_##T##L##_alloc(co_obj_t *output, \
  const T##L##_t input, const uint8_t flags); co_obj_t *co_##T##L##_create(\
  const T##L##_t input, const uint8_t flags);

_DECLARE_INTEGER(int, 8);
_DECLARE_INTEGER(int, 16);
_DECLARE_INTEGER(int, 32);
_DECLARE_INTEGER(int, 64);
_DECLARE_INTEGER(uint, 8);
_DECLARE_INTEGER(uint, 16);
_DECLARE_INTEGER(uint, 32);
_DECLARE_INTEGER(uint, 64);

/*-----------------------------------------------------------------------------
 *  Declarations of other simple types
 *-----------------------------------------------------------------------------*/

/* Type "Nil" declaration */
typedef struct
{
  co_obj_t _header;
} co_nil_t;

int co_nil_alloc(co_obj_t *output, const uint8_t flags);
co_obj_t * co_nil_create(const uint8_t flags);

/* Type "Bool" declaration */
typedef struct
{
  co_obj_t _header;
} co_bool_t;

int co_bool_alloc(co_obj_t *output, const bool input, const uint8_t flags);
co_obj_t * co_bool_create(const bool input, const uint8_t flags);

/* Type "fixint" declaration */
typedef struct
{
  co_obj_t _header;
} co_fixint_t;

int co_fixint_alloc(co_obj_t *output, const int input, const uint8_t flags);
co_obj_t * co_fixint_create(const int input, const uint8_t flags);

/* Type "float32" declaration */
typedef struct
{
  co_obj_t _header;
  float data;
} co_float32_t;

int co_float32_alloc(co_obj_t *output, const float input, const uint8_t flags);
co_obj_t * co_float32_create(const double input, const uint8_t flags);

/* Type "float64" declaration */
typedef struct
{
  co_obj_t _header;
  double data;
} co_float64_t;

int co_float64_alloc(co_obj_t *output, const double input, const uint8_t flags);
co_obj_t * co_float64_create(const double input, const uint8_t flags);


/*-----------------------------------------------------------------------------
 *  Deconstructors
 *-----------------------------------------------------------------------------*/
void co_obj_free(co_obj_t *object);

/*-----------------------------------------------------------------------------
 *  Accessors
 *-----------------------------------------------------------------------------*/
size_t co_obj_raw(void *data, const co_obj_t *object);

size_t co_obj_data(void *data, const co_obj_t *object);

int co_obj_import(co_obj_t *output, const size_t out_size, const char *input, \
    const size_t in_size, const uint8_t flags);

int co_obj_getflags(const co_obj_t *object);

void co_obj_setflags(co_obj_t *object, const int flags);

/*-----------------------------------------------------------------------------
 *  Strings
 *-----------------------------------------------------------------------------*/

int co_str_copy(co_obj_t *dst, const co_obj_t *src, const size_t size);

int co_str_cat(co_obj_t *dst, const co_obj_t *src, const size_t size);
#endif
