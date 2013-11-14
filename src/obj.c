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


/* Type "Nil" declaration */
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
#define _CREATE_BIN(L) co_bin##L##_t *co_bin##L##_create(const char *input, \
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
_ALLOC_BIN(16);
_ALLOC_BIN(32);
_CREATE_BIN(8);
_CREATE_BIN(16);
_CREATE_BIN(32);

/* Type "Ext" declaration macros */
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
#define _CREATE_EXT(L) co_ext##L##_t *co_ext##L##_create(const char *input, \
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

_ALLOC_EXT(8);
_ALLOC_EXT(16);
_ALLOC_EXT(32);
_CREATE_EXT(8);
_CREATE_EXT(16);
_CREATE_EXT(32);

/* Type "fixint" declaration */
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
#define _ALLOC_INT(L) int co_int##L##_alloc(co_int##L##_t *output, \
    const int##L##_t input) \
    { \
      output->_header._type = _INT##L; \
      output->data = input; \
      return 1; \
    }
#define _CREATE_INT(L) co_int##L##_t *co_int##L##_create(\
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
_ALLOC_INT(8);
_ALLOC_INT(16);
_ALLOC_INT(32);
_ALLOC_INT(64);
_CREATE_INT(8);
_CREATE_INT(16);
_CREATE_INT(32);
_CREATE_INT(64);

/* Type "str" declaration macros */
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
#define _CREATE_STR(L) co_str##L##_t *co_str##L##_create(const char *input, \
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

_ALLOC_STR(8);
_ALLOC_STR(16);
_ALLOC_STR(32);
_CREATE_STR(8);
_CREATE_STR(16);
_CREATE_STR(32);

/* Type "uint" declaration macros */
#define _ALLOC_UINT(L) int co_uint##L##_alloc(co_uint##L##_t *output, \
    const uint##L##_t input) \
    { \
      output->_header._type = _UINT##L; \
      output->data = input; \
      return 1; \
    }
#define _CREATE_UINT(L) co_uint##L##_t *co_uint##L##_create(\
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
_ALLOC_UINT(8);
_ALLOC_UINT(16);
_ALLOC_UINT(32);
_ALLOC_UINT(64);
_CREATE_UINT(8);
_CREATE_UINT(16);
_CREATE_UINT(32);
_CREATE_UINT(64);

void
co_free(co_obj_t *object)
{
  h_free(object);
  return;
}

