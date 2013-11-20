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


/*-----------------------------------------------------------------------------
 *   Constructors of character-array-types
 *-----------------------------------------------------------------------------*/
#define _DEFINE_CHAR(T, L) int co_##T##L##_alloc(co_##T##L##_t *output, \
    const size_t out_size, const char *input, const size_t in_size ) \
    { \
      CHECK((out_size - sizeof(uint##L##_t) - sizeof(co_obj_t) < UINT##L##_MAX), \
      "Object too large for type ##T##L## to address."); \
      if((in_size > 0) && (input != NULL)) \
      { \
        DEBUG("in_size: %d", in_size); \
        DEBUG("out_size: %d", out_size); \
        DEBUG("sizeof(uint##L##_t): %d", sizeof(uint##L##_t)); \
        DEBUG("sizeof(co_obj_t): %d", sizeof(co_obj_t)); \
        CHECK((in_size <= (out_size - sizeof(uint##L##_t) - sizeof(co_obj_t))), \
        "Value too large for output buffer."); \
        memmove(output->data, input, in_size); \
      } \
      output->_header._type = _##T##L; \
      output->_len = (uint##L##_t)(out_size - sizeof(uint##L##_t) - \
          sizeof(co_obj_t)); \
      return 1; \
    error: \
      return 0; \
    } \
    co_##T##L##_t *co_##T##L##_create(const char *input, \
    const size_t input_size) \
    { \
      CHECK((input_size < UINT##L##_MAX), "Value too large for type ##T##L##."); \
      int output_size = input_size + sizeof(uint##L##_t) + sizeof(co_obj_t); \
      co_##T##L##_t *output = h_calloc(1, output_size); \
      CHECK_MEM(output); \
      CHECK(co_##T##L##_alloc(output, output_size, input, input_size), \
          "Failed to allocate object."); \
      return output; \
    error: \
      return NULL; \
    }

_DEFINE_CHAR(bin, 8);
_DEFINE_CHAR(bin, 16);
_DEFINE_CHAR(bin, 32);
_DEFINE_CHAR(ext, 8);
_DEFINE_CHAR(ext, 16);
_DEFINE_CHAR(ext, 32);
_DEFINE_CHAR(str, 8);
_DEFINE_CHAR(str, 16);
_DEFINE_CHAR(str, 32);

/*-----------------------------------------------------------------------------
 *   Constructors of integer types
 *-----------------------------------------------------------------------------*/
#define _DEFINE_INTEGER(T, L) int co_##T##L##_alloc(co_##T##L##_t *output, \
    const T##L##_t input) \
    { \
      output->_header._type = _##T##L; \
      output->data = input; \
      return 1; \
    } \
  co_##T##L##_t *co_##T##L##_create(\
    const T##L##_t input) \
    { \
      int output_size = sizeof(T##L##_t) + sizeof(co_obj_t); \
      co_##T##L##_t *output = h_calloc(1, output_size); \
      CHECK_MEM(output); \
      CHECK(co_##T##L##_alloc(output, input), \
          "Failed to allocate object."); \
      return output; \
    error: \
      return NULL; \
    }

_DEFINE_INTEGER(int, 8);
_DEFINE_INTEGER(int, 16);
_DEFINE_INTEGER(int, 32);
_DEFINE_INTEGER(int, 64);
_DEFINE_INTEGER(uint, 8);
_DEFINE_INTEGER(uint, 16);
_DEFINE_INTEGER(uint, 32);
_DEFINE_INTEGER(uint, 64);

/* Type "Nil" constructors */
int 
co_nil_alloc(co_nil_t *output)
{
  output->_header._type = _nil;
  return 1;
} 

co_nil_t * 
co_nil_create(void)
{
  co_nil_t *output = h_calloc(1, sizeof(co_nil_t));
  co_nil_alloc(output);
  return output;
} 

/* Type "Bool" constructors */
int 
co_bool_alloc(co_bool_t *output)
{
  output->_header._type = _false;
  return 1;
} 

co_bool_t * 
co_bool_create(const bool input)
{
  co_bool_t *output = h_calloc(1, sizeof(co_bool_t));
  co_bool_alloc(output);
  if(input) output->_header._type = _true;
  return output;
} 


/* Type "fixint" constructors */
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

/* Type "float32" constructors */
int 
co_float32_alloc(co_float32_t *output, const float input)
{
  output->_header._type = _float32;
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

/* Type "float64" constructors */
int 
co_float64_alloc(co_float64_t *output, const double input)
{
  output->_header._type = _float64;
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

/* Deconstructor definition */
void
co_free(co_obj_t *object)
{
  h_free(object);
  return;
}


  

/*-----------------------------------------------------------------------------
 *   String operations
 *-----------------------------------------------------------------------------*/

size_t
co_data(char *data, const co_obj_t *object)
{
  switch(CO_TYPE(object))
  {
    case _str8:
      data = (char *)(((co_str8_t *)object)->data);
      return (size_t)(((co_str8_t *)object)->_len);
      break;
    case _str16:
      data = (char *)(((co_str16_t *)object)->data);
      return (size_t)(((co_str16_t *)object)->_len);
      break;
    case _str32:
      data = (char *)(((co_str32_t *)object)->data);
      return (size_t)(((co_str32_t *)object)->_len);
      break;
    case _bin8:
      data = (char *)(((co_bin8_t *)object)->data);
      return (size_t)(((co_bin8_t *)object)->_len);
      break;
    case _bin16:
      data = (char *)(((co_bin16_t *)object)->data);
      return (size_t)(((co_bin16_t *)object)->_len);
      break;
    case _bin32:
      data = (char *)(((co_bin32_t *)object)->data);
      return (size_t)(((co_bin32_t *)object)->_len);
      break;
    case _ext8:
      data = (char *)(((co_ext8_t *)object)->data);
      return (size_t)(((co_ext8_t *)object)->_len);
      break;
    case _ext16:
      data = (char *)(((co_ext16_t *)object)->data);
      return (size_t)(((co_ext16_t *)object)->_len);
      break;
    case _ext32:
      data = (char *)(((co_ext32_t *)object)->data);
      return (size_t)(((co_ext32_t *)object)->_len);
      break;
    default:
      WARN("Not a character object.");
      return -1;
  }
}

int 
co_strcpy(co_obj_t *dst, const co_obj_t *src, const size_t size)
{
  char *src_data, *dst_data; 
  size_t length = 0;
  CHECK(((length = co_data(src_data, src)) >= 0), "Not a string object.");
  if(length >= UINT16_MAX)
  {

  }
  else if(length >= UINT8_MAX)
  {

  }
  else CHECK();
	if (size != 0) {
		memmove(dst, src, (length > size - 1) ? size - 1 : length);
		dst_data[size - 1] = '\0';
	}
	return length;
error:
  return 0;
}

int 
co_strcat(co_obj_t *dst, const co_obj_t *src)
{
  size_t used, length, copy;

  used = strlen(dst);
  length = strlen(src);
  if (size > 0 && used < size - 1) {
    copy = (length >= size - used) ? size - used - 1 : length;
    memcpy(dst + used, src, copy);
    dst[used + copy] = '\0';
  }
  return used + length;
}

int 
co_strprint(char *str, size_t size, const char *format, ...) {
  size_t result;
  va_list args;
  size_t len = strnlen(str, size);

  va_start(args, format);
  result = vsnprintf(str + len, size - len, format, args);
  va_end(args);

  return result + len;
}
