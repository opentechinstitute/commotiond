/* vim: set ts=2 expandtab: */
/**
 *       @file  util.h
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

#ifndef _UTIL_H
#define _UTIL_H
#include <stdlib.h>

#define NEW(O, T) O##_create(sizeof(T##_t), T##_proto)
#define _(N) proto.N

#define LOWERCASE(N) for ( ; *N; ++N) *N = tolower(*N);
#define UPPERCASE(N) for ( ; *N; ++N) *N = toupper(*N);

#define MAX_ARGS 16

size_t strlcat(char *dst, const char *src, const size_t size);

size_t strlcpy(char *dest, const char *src, const size_t size);

size_t snprintfcat(char *str, size_t size, const char *format, ...);

size_t strstrip(const char *s, char *out, const size_t outlen);

int compare_version(const char * aver, const char *bver);

typedef int (*file_iter)(const char* path, const char *filename);

int process_files(const char *dir_path, file_iter loader);

int string_to_argv(const char *input, char **argv, int *argc, const size_t max);

int argv_to_string(char **argv, const int argc, char *output, const size_t max);

void mac_string_to_bytes(char *macstr, unsigned char mac[6]);

void print_mac(unsigned char mac[6]);

int wifi_freq(const int channel);

int wifi_chan(const int frequency);
#endif
