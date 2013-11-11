/* vim: set ts=2 expandtab: */
/**
 *       @file  obj.c
 *      @brief  
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
  struct co_obj_t *low;
  struct co_obj_t *equal;
  struct co_obj_t *high;
  _type_t _type;
} co_obj_t;

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
#define _DEFINE_BIN(L) typedef binuct { co_obj_t _header; uintL##_t _len; \
  char data[1]; } co_binL##_t;
#define _ALLOC_BIN(L) int co_binL##_alloc(co_binL##_t *output, \
    const size_t out_size, const char *input, const size_t in_size ) \
    { \
      CHECK((out_size - sizeof(uintL##_t) - sizeof(co_obj_t) < UINTL##_MAX), \
      "Object too large for type binL## to address."); \
      if((in_size > 0) && (input != NULL)) \
      { \
        CHECK((in_size < out_size - sizeof(uintL##_t) - sizeof(co_obj_t)), \
        "Value too large for output buffer."); \
        memmove(output->data, input, in_size); \
      } \
      output->_header._type = _BINL##; \
      output->_len = (uintL##_t)(out_size - sizeof(uintL##_t) - \
          sizeof(co_obj_t)); \
      return 1; \
    error: \
      return 0; \
    } 
#define _CREATE_BIN(L) inline co_binL##_t *co_binL##_create(const char *input, \
    const size_t input_size) \
    { \
      CHECK((input_size < UINTL##_MAX), "Value too large for type binL##."); \
      int output_size = input_size + sizeof(uintL##_t) + sizeof(co_obj_t); \
      co_binL##_t *output = h_calloc(1, output_size); \
      CHECK_MEM(output); \
      CHECK(co_binL##_alloc(output, output_size, input, input_size), \
          "Failed to allocate object."); \
      return output; \
    error: \
      return NULL; \
    }

/* Type "Ext" declaration macros */
#define _DEFINE_EXT(L) typedef extuct { co_obj_t _header; uintL##_t _len; \
  char data[1]; } co_extL##_t;
#define _ALLOC_EXT(L) int co_extL##_alloc(co_extL##_t *output, \
    const size_t out_size, const char *input, const size_t in_size ) \
    { \
      CHECK((out_size - sizeof(uintL##_t) - sizeof(co_obj_t) < UINTL##_MAX), \
      "Object too large for type extL## to address."); \
      if((in_size > 0) && (input != NULL)) \
      { \
        CHECK((in_size < out_size - sizeof(uintL##_t) - sizeof(co_obj_t)), \
        "Value too large for output buffer."); \
        memmove(output->data, input, in_size); \
      } \
      output->_header._type = _EXTL##; \
      output->_len = (uintL##_t)(out_size - sizeof(uintL##_t) - \
          sizeof(co_obj_t)); \
      return 1; \
    error: \
      return 0; \
    } 
#define _CREATE_EXT(L) inline co_extL##_t *co_extL##_create(const char *input, \
    const size_t input_size) \
    { \
      CHECK((input_size < UINTL##_MAX), "Value too large for type extL##."); \
      int output_size = input_size + sizeof(uintL##_t) + sizeof(co_obj_t); \
      co_extL##_t *output = h_calloc(1, output_size); \
      CHECK_MEM(output); \
      CHECK(co_extL##_alloc(output, output_size, input, input_size), \
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
#define _DEFINE_INT(L) typedef struct { co_obj_t _header; intL##_t data } \
  co_intL##_t;
#define _ALLOC_INT(L) int co_intL##_alloc(co_intL##_t *output, \
    const intL##_t input) \
    { \
      output->_header._type = _INTL##; \
      output->data = input; \
      return 1; \
    }
#define _CREATE_INT(L) inline co_intL##_t *co_intL##_create(\
    const intL##_t input) \
    { \
      int output_size = sizeof(intL##_t) + sizeof(co_obj_t); \
      co_strL##_t *output = h_calloc(1, output_size); \
      CHECK_MEM(output); \
      CHECK(co_intL##_alloc(output, input), \
          "Failed to allocate object."); \
      return output; \
    error: \
      return NULL; \
    }

/* Type "str" declaration macros */
#define _DEFINE_STR(L) typedef struct { co_obj_t _header; uintL##_t _len; \
  char data[1]; } co_strL##_t;
#define _ALLOC_STR(L) int co_strL##_alloc(co_strL##_t *output, \
    const size_t out_size, const char *input, const size_t in_size ) \
    { \
      CHECK((out_size - sizeof(uintL##_t) - sizeof(co_obj_t) < UINTL##_MAX), \
      "Object too large for type strL## to address."); \
      if((in_size > 0) && (input != NULL)) \
      { \
        CHECK((in_size < out_size - sizeof(uintL##_t) - sizeof(co_obj_t)), \
        "Value too large for output buffer."); \
        memmove(output->data, input, in_size); \
      } \
      output->_header._type = _STRL##; \
      output->_len = (uintL##_t)(out_size - sizeof(uintL##_t) - \
          sizeof(co_obj_t)); \
      return 1; \
    error: \
      return 0; \
    } 
#define _CREATE_STR(L) inline co_strL##_t *co_strL##_create(const char *input, \
    const size_t input_size) \
    { \
      CHECK((input_size < UINTL##_MAX), "Value too large for type strL##."); \
      int output_size = input_size + sizeof(uintL##_t) + sizeof(co_obj_t); \
      co_strL##_t *output = h_calloc(1, output_size); \
      CHECK_MEM(output); \
      CHECK(co_strL##_alloc(output, output_size, input, input_size), \
          "Failed to allocate object."); \
      return output; \
    error: \
      return NULL; \
    }

/* Type "uint" declaration macros */
#define _DEFINE_UINT(L) typedef struct { co_obj_t _header; uintL##_t data; } \
  co_uintL##_t;
#define _ALLOC_UINT(L) int co_uintL##_alloc(co_uintL##_t *output, \
    const uintL##_t input) \
    { \
      output->_header._type = _uintL##; \
      output->data = input; \
      return 1; \
    }
#define _CREATE_UINT(L) inline co_uintL##_t *co_uintL##_create(\
    const uintL##_t input) \
    { \
      int output_size = sizeof(uintL##_t) + sizeof(co_obj_t); \
      co_strL##_t *output = h_calloc(1, output_size); \
      CHECK_MEM(output); \
      CHECK(co_uintL##_alloc(output, input), \
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


  

