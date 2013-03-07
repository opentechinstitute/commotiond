/* vim: set ts=2 expandtab: */
/**
 *       @file  util.c
 *      @brief  utility functions for the Commotion daemon
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

#include <stdlib.h>
#include <dirent.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <ctype.h>
#include <stdarg.h>
#include "debug.h"
#include "util.h"



size_t strlcpy(char *dest, const char *src, const size_t size)
{
	const size_t length = strlen(src);
	if (size != 0) {
		memmove(dest, src, (length > size - 1) ? size - 1 : length);
		dest[size - 1] = '\0';
	}
	return length;
}

size_t strlcat(char *dst, const char *src, const size_t size)
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

size_t snprintfcat(char *str, size_t size, const char *format, ...) {
  size_t result;
  va_list args;
  size_t len = strnlen(str, size);

  va_start(args, format);
  result = vsnprintf(str + len, size - len, format, args);
  va_end(args);

  return result + len;
}
/*
int strstrip(char *s) {
  size_t len = strlen(s);
  char *ptr = s;

  while(isspace(ptr[len-1])) ptr[--len] = '\0';
  while(*ptr && isspace(*ptr)) {
    ++ptr; 
    --len;
  }

  memmove(s, ptr, len + 1);
  return len;
}
*/

size_t strstrip(const char *s, char *out, size_t outlen) {
  size_t len = strlen(s);
  const char *ptr = s;
  if(len == 0) return 0;

  while(*ptr && isspace(*ptr)) {
    ++ptr; 
    --len;
  }

  if(len <= outlen) {
    strlcpy(out, ptr, len + 1);
    return len;
  } else return -1;
}

int compare_version(const char *aver, const char *bver) {
  char *a, *b, *anext, *bnext;
  long int avalue, bvalue;

  // Grab pointer to first number in each version string.
  a = strpbrk(aver, "0123456789");
  b = strpbrk(bver, "0123456789");

  while(a && b) {
    // Convert characters to number.
    avalue = strtol(a, &anext, 10);
    bvalue = strtol(b, &bnext, 10);

    if(avalue < bvalue) return -1;
    if(avalue > bvalue) return 1;

    a = strpbrk(anext, "0123456789");
    b = strpbrk(bnext, "0123456789");
  }

  // If valid characters remain in 1 string, assume a point release.
  if(b) return -1;
  if(a) return 1;

  // Versions are identical.
  return 0;
}

int process_files(const char *dir_path, file_iter loader) {
  size_t path_size = strlen(dir_path);
  CHECK((path_size > 0) && (path_size <= PATH_MAX), "Invalid path length!");
  DIR *dir_iter = NULL;
  dir_iter = opendir(dir_path);
  struct dirent *dir_entry = NULL;
  DEBUG("Processing files in directory %s", dir_path);
  
  while((dir_entry = readdir(dir_iter)) != NULL) {
    DEBUG("Checking file %s", dir_entry->d_name);
    if(!strcmp(dir_entry->d_name, ".")) continue;
    if(!strcmp(dir_entry->d_name, "..")) continue;
    if(!(*loader)(dir_path, dir_entry->d_name)) break;
  }

  if(dir_iter) closedir(dir_iter);
  return 1;

error:
  return 0;
}

int string_to_argv(const char *input, char **argv, int *argc, const size_t max) {
  int count = 0;
	char *saveptr = NULL;
	char *token = NULL;
  char *input_tmp = strdup(input);
	CHECK_MEM(token = strtok_r(input_tmp, " ", &saveptr));
  while(token && count < max) {
    argv[count++] = token;
	  token = strtok_r(NULL, " ", &saveptr);
  }
  *argc = count;
  return 1; 
error:
  free(input_tmp);
  return 0;
}

int argv_to_string(char **argv, const int argc, char *output, const size_t max) {
  int i;
  for(i = 0; i < argc; i++) {
    strlcat(output, argv[i], max);
    strlcat(output, " ", max);
  }
  if(i < argc) {
    ERROR("Failed to concatenate all of argv.");
    return 0;
  } 
  return 1; 
}
