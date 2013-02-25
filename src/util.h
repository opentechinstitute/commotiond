/*! 
 *
 * \file util.h 
 *
 * \brief utility functions for the Commotion daemon
 *
 * \author Josh King <jking@chambana.net>
 * 
 * \date
 *
 * \copyright This file is part of Commotion, Copyright(C) 2012-2013 Josh King
 * 
 *            Commotion is free software: you can redistribute it and/or modify
 *            it under the terms of the GNU General Public License as published 
 *            by the Free Software Foundation, either version 3 of the License, 
 *            or (at your option) any later version.
 * 
 *            Commotion is distributed in the hope that it will be useful,
 *            but WITHOUT ANY WARRANTY; without even the implied warranty of
 *            MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *            GNU General Public License for more details.
 * 
 *            You should have received a copy of the GNU General Public License
 *            along with Commotion.  If not, see <http://www.gnu.org/licenses/>.
 * 
 * */

#ifndef _UTIL_H
#define _UTIL_H
#include <stdlib.h>

#define NEW(O, T) O##_create(sizeof(T##_t), T##_proto)
#define _(N) proto.N

#define LOWERCASE(N) for ( ; *N; ++N) *N = tolower(*N);
#define UPPERCASE(N) for ( ; *N; ++N) *N = toupper(*N);

size_t strlcpy(char *dest, const char *src, size_t size);

int compare_version(const char * aver, const char *bver);

typedef int (*file_iter)(const char *path);

int process_files(const char *dir_path, file_iter loader);

#endif
