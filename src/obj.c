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
#include "obj.h"
#include "extern/halloc.h"

/* Types */
#define _NIL 0xc0
#define _FALSE 0xc2
#define _TRUE 0xc3
#define _BIN8 0xc4
#define _BIN16 0xc5
#define _BIN32 0xc6
#define _EXT8 0xc7
#define _EXT16 0xc8
#define _EXT32 0xc9
#define _FLOAT32 0xca
#define _FLOAT64 0xcb
#define _UINT8 0xcc
#define _UINT16 0xcd
#define _UINT32 0xce
#define _UINT64 0xcf
#define _INT8 0xd0
#define _INT16 0xd1
#define _INT32 0xd2
#define _INT64 0xd3
#define _STR8 0xd9
#define _STR16 0xda
#define _STR32 0xdb
#define _ARR16 0xdc
#define _ARR32 0xdd
#define _MAP16 0xde
#define _MAP32 0xdf

/* Convenience */
#define CO_TYPE(J) (J->_header._type)

/* Lists */
#define _LIST_NEXT(J) (J->_header._next)
#define _LIST_PREV(J) (J->_header._prev)
#define _LIST_LAST(J) (J->_header._prev)

/* Type checking */
#define IS_NIL(J) (CO_TYPE(J) == _NIL)
#define IS_FIXINT(J) (CO_TYPE(J) < 128)
#define IS_BOOL(J) ((CO_TYPE(J) == _FALSE) || (CO_TYPE(J) == _TRUE))
#define IS_BIN(J) ((CO_TYPE(J) == _BIN8) || (CO_TYPE(J) == _BIN16) || (CO_TYPE(J) == _BIN32))
#define IS_EXT(J) ((CO_TYPE(J) == _EXT8) || (CO_TYPE(J) == _EXT16) || (CO_TYPE(J) == _EXT32))
#define IS_FLOAT(J) ((CO_TYPE(J) == _FLOAT32) || (CO_TYPE(J) == _FLOAT64))
#define IS_UINT(J) ((CO_TYPE(J) == _UINT8) || (CO_TYPE(J) == _UINT16) || (CO_TYPE(J) == _UINT32) || (CO_TYPE(J) == _UINT64))
#define IS_INT(J) ((CO_TYPE(J) == _INT8) || (CO_TYPE(J) == _INT16) || (CO_TYPE(J) == _INT32) || (CO_TYPE(J) == _INT64))
#define IS_STR(J) ((CO_TYPE(J) == _STR8) || (CO_TYPE(J) == _STR16) || (CO_TYPE(J) == _STR32))
#define IS_LIST(J) ((CO_TYPE(J) == _ARR16) || (CO_TYPE(J) == _ARR32))
#define IS_MAP(J) ((CO_TYPE(J) == _MAP16) || (CO_TYPE(J) == _MAP32))
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

int 
co_nil_alloc(co_nil_t *output)
{
  output->_header._type = _NIL;
  return 1;
} 

co_nil_t * 
co_nil_create(void)
{
  co_nil_t *output = h_calloc(1, sizeof(co_nil_t));
  co_nil_alloc(output);
  return output;
} 

/* Type "Bool" declaration */
typedef struct
{
  co_obj_t _header;
} co_bool_t;

int 
co_bool_alloc(co_bool_t *output)
{
  output->_header._type = _FALSE;
  return 1;
} 

co_bool_t * 
co_bool_create(const bool input)
{
  co_bool_t *output = h_calloc(1, sizeof(co_bool_t));
  co_bool_alloc(output);
  if(input) output->_header._type = _TRUE;
  return output;
} 

/* Type "Bin" declaration macros */
#define _ALLOC_BIN(L) int co_bin##L##_alloc(co_bin##L##_t *output, \
    const size_t out_size, const char *input, const size_t in_size ) \
    { \
      CHECK((out_size - sizeof(uint##L##_t) - sizeof(co_obj_t) < UINT##L##_MAX), \
      "Object too large for type bin##L## to address."); \
      if((in_size > 0) && (input != NULL)) \
      { \
        CHECK((in_size < out_size - sizeof(uint##L##_t) - sizeof(co_obj_t)), \
        "Value too large for output buffer."); \
        memmove(output->data, input, in_size); \
      } \
      output->_header._type = _BIN##L; \
      output->_len = (uint##L##_t)(out_size - sizeof(uint##L##_t) - \
          sizeof(co_obj_t)); \
      return 1; \
    error: \
      return 0; \
    } 
#define _CREATE_BIN(L) inline co_bin##L##_t *co_bin##L##_create(const char *input, \
    const size_t input_size) \
    { \
      CHECK((input_size < UINT##L##_MAX), "Value too large for type bin##L##."); \
      int output_size = input_size + sizeof(uint##L##_t) + sizeof(co_obj_t); \
      co_bin##L##_t *output = h_calloc(1, output_size); \
      CHECK_MEM(output); \
      CHECK(co_bin##L##_alloc(output, output_size, input, input_size), \
          "Failed to allocate object."); \
      return output; \
    error: \
      return NULL; \
    }

_ALLOC_BIN(8);
_CREATE_BIN(8);

/* Type "Ext" declaration macros */
#define _DEFINE_EXT(L) typedef extuct { co_obj_t _header; uint##L##_t _len; \
  char data[1]; } co_ext##L##_t;
#define _ALLOC_EXT(L) int co_ext##L##_alloc(co_ext##L##_t *output, \
    const size_t out_size, const char *input, const size_t in_size ) \
    { \
      CHECK((out_size - sizeof(uint##L##_t) - sizeof(co_obj_t) < UINT##L##_MAX), \
      "Object too large for type ext##L## to address."); \
      if((in_size > 0) && (input != NULL)) \
      { \
        CHECK((in_size < out_size - sizeof(uint##L##_t) - sizeof(co_obj_t)), \
        "Value too large for output buffer."); \
        memmove(output->data, input, in_size); \
      } \
      output->_header._type = _EXT##L; \
      output->_len = (uint##L##_t)(out_size - sizeof(uint##L##_t) - \
          sizeof(co_obj_t)); \
      return 1; \
    error: \
      return 0; \
    } 
#define _CREATE_EXT(L) inline co_ext##L##_t *co_ext##L##_create(const char *input, \
    const size_t input_size) \
    { \
      CHECK((input_size < UINT##L##_MAX), "Value too large for type ext##L##."); \
      int output_size = input_size + sizeof(uint##L##_t) + sizeof(co_obj_t); \
      co_ext##L##_t *output = h_calloc(1, output_size); \
      CHECK_MEM(output); \
      CHECK(co_ext##L##_alloc(output, output_size, input, input_size), \
          "Failed to allocate object."); \
      return output; \
    error: \
      return NULL; \
    }

/* Type "fixint" declaration */
typedef struct
{
  co_obj_t _header;
} co_fixint_t;

int 
co_fixint_alloc(co_fixint_t *output, const int input)
{
  CHECK(input < 128, "Value too large for a fixint.");
  output->_header._type = (uint8_t)input;
  return 1;
error:
  return 0;
} 

co_fixint_t * 
co_fixint_create(const int input)
{
  co_fixint_t *output = h_calloc(1, sizeof(co_fixint_t));
  CHECK(co_fixint_alloc(output, input), "Failed to allocate object.");
  return output;
error:
  return NULL;
} 

/* Type "float32" declaration */
typedef struct
{
  co_obj_t _header;
  float data;
} co_float32_t;

int 
co_float32_alloc(co_float32_t *output, const float input)
{
  output->_header._type = _FLOAT32;
  output->data = input;
  return 1;
} 

co_float32_t * 
co_float32_create(const double input)
{
  co_float32_t *output = h_calloc(1, sizeof(co_float32_t));
  co_float32_alloc(output, input);
  return output;
} 

/* Type "float64" declaration */
typedef struct
{
  co_obj_t _header;
  double data;
} co_float64_t;

int 
co_float64_alloc(co_float64_t *output, const double input)
{
  output->_header._type = _FLOAT64;
  output->data = input;
  return 1;
} 

co_float64_t * 
co_float64_create(const double input)
{
  co_float64_t *output = h_calloc(1, sizeof(co_float64_t));
  co_float64_alloc(output, input);
  return output;
} 

/* Type "int" declaration macros */
#define _DEFINE_INT(L) typedef struct { co_obj_t _header; int##L##_t data } \
  co_int##L##_t;
#define _ALLOC_INT(L) int co_int##L##_alloc(co_int##L##_t *output, \
    const int##L##_t input) \
    { \
      output->_header._type = _INT##L; \
      output->data = input; \
      return 1; \
    }
#define _CREATE_INT(L) inline co_int##L##_t *co_int##L##_create(\
    const int##L##_t input) \
    { \
      int output_size = sizeof(int##L##_t) + sizeof(co_obj_t); \
      co_str##L##_t *output = h_calloc(1, output_size); \
      CHECK_MEM(output); \
      CHECK(co_int##L##_alloc(output, input), \
          "Failed to allocate object."); \
      return output; \
    error: \
      return NULL; \
    }

/* Type "str" declaration macros */
#define _DEFINE_STR(L) typedef struct { co_obj_t _header; uint##L##_t _len; \
  char data[1]; } co_str##L##_t;
#define _ALLOC_STR(L) int co_str##L##_alloc(co_str##L##_t *output, \
    const size_t out_size, const char *input, const size_t in_size ) \
    { \
      CHECK((out_size - sizeof(uint##L##_t) - sizeof(co_obj_t) < UINT##L##_MAX), \
      "Object too large for type str##L## to address."); \
      if((in_size > 0) && (input != NULL)) \
      { \
        CHECK((in_size < out_size - sizeof(uint##L##_t) - sizeof(co_obj_t)), \
        "Value too large for output buffer."); \
        memmove(output->data, input, in_size); \
      } \
      output->_header._type = _STR##L; \
      output->_len = (uint##L##_t)(out_size - sizeof(uint##L##_t) - \
          sizeof(co_obj_t)); \
      return 1; \
    error: \
      return 0; \
    } 
#define _CREATE_STR(L) inline co_str##L##_t *co_str##L##_create(const char *input, \
    const size_t input_size) \
    { \
      CHECK((input_size < UINT##L##_MAX), "Value too large for type str##L##."); \
      int output_size = input_size + sizeof(uint##L##_t) + sizeof(co_obj_t); \
      co_str##L##_t *output = h_calloc(1, output_size); \
      CHECK_MEM(output); \
      CHECK(co_str##L##_alloc(output, output_size, input, input_size), \
          "Failed to allocate object."); \
      return output; \
    error: \
      return NULL; \
    }

/* Type "uint" declaration macros */
#define _DEFINE_UINT(L) typedef struct { co_obj_t _header; uint##L##_t data; } \
  co_uint##L##_t;
#define _ALLOC_UINT(L) int co_uint##L##_alloc(co_uint##L##_t *output, \
    const uint##L##_t input) \
    { \
      output->_header._type = _UINT##L; \
      output->data = input; \
      return 1; \
    }
#define _CREATE_UINT(L) inline co_uint##L##_t *co_uint##L##_create(\
    const uint##L##_t input) \
    { \
      int output_size = sizeof(uint##L##_t) + sizeof(co_obj_t); \
      co_str##L##_t *output = h_calloc(1, output_size); \
      CHECK_MEM(output); \
      CHECK(co_uint##L##_alloc(output, input), \
          "Failed to allocate object."); \
      return output; \
    error: \
      return NULL; \
    }

inline void
co_free(co_obj_t *object)
{
  h_free(object);
  return;
}

