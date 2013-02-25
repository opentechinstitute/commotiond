/*! 
 *
 * \file profile.h 
 *
 * \brief a key-value store for loading configuration information
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

#ifndef _PROFILE_H
#define _PROFILE_H

#include <stdlib.h>
#include <stddef.h>
#include "extern/tst.h"

int profile_import_files_i(const char *path);

int profile_import_files(const char *path);

int profile_export_file(tst_t *profile, const char *path);

//int profile_export(tst_t *profile, const char *path, const int length);

int profile_set(tst_t *profile, const char *key, const char *value);

int profile_get_int(tst_t *profile, const char *key, const int def);

char *profile_get_string(tst_t *profile, const char *key, char *def);

#endif
