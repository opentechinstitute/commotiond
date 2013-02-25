/*! 
 *
 * \file util.c 
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

#include <stdlib.h>
#include <dirent.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/types.h>
#include <sys/stat.h>
#include "debug.h"
#include "util.h"



size_t strlcpy(char *dest, const char *src, size_t size)
{
  DEBUG("Strlcpy.");
	const size_t length = strlen(src);
	if (size != 0) {
		memmove(dest, src, (length > size - 1) ? size - 1 : length);
		dest[size - 1] = '\0';
	}
	return length;
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
  
  while((dir_entry = readdir(dir_iter)) != NULL) {
    if(!strcmp(dir_entry->d_name, ".")) break;
    if(!strcmp(dir_entry->d_name, "..")) break;
    if(!(*loader)(dir_entry->d_name)) break;
  }

  if(dir_iter) closedir(dir_iter);
  return 1;

error:
  return 0;
}

