/* vim: set ts=2 expandtab: */
/**
 *       @file  profile.h
 *      @brief  a key-value store for loading configuration information
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

#ifndef _PROFILE_H
#define _PROFILE_H

#include <stdlib.h>
#include <stddef.h>
#include "extern/tst.h"

#define PROFILES_MAX 128

typedef struct {
  char *name;
  tst_t *profile;
} co_profile_t;

int co_profiles_create(void);

int co_profiles_destroy(void);

int co_profile_import_files(const char *path);

//int profile_export_file(tst_t *profile, const char *path);

//int profile_export(tst_t *profile, const char *path, const int length);

int co_profile_set(co_profile_t *profile, const char *key, const char *value);

int co_profile_get_int(co_profile_t *profile, const char *key, const int def);

char *co_profile_get_string(co_profile_t *profile, const char *key, char *def);

char *co_list_profiles(void);

co_profile_t *co_profile_find(const char *name);

void co_profile_dump(co_profile_t *profile);
#endif
