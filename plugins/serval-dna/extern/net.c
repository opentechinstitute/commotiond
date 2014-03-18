/**
 *       @file  net.c
 *      @brief  minimal Serval MDP client functionality, used in the
 *                commotion_serval-sas library
 *
 *     @author  Dan Staples (dismantl), danstaples@opentechinstitute.org
 *
 *   @internal
 *     Created  12/18/2013
 *    Compiler  gcc/g++
 *     Company  The Open Technology Institute
 *   Copyright  Copyright (c) 2013, Dan Staples
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

/*
Serval Distributed Numbering Architecture (DNA)
Copyright (C) 2012 Serval Project Inc.

This program is free software; you can redistribute it and/or
modify it under the terms of the GNU General Public License
as published by the Free Software Foundation; either version 2
of the License, or (at your option) any later version.

This program is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with this program; if not, write to the Free Software
Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA.
*/

#include <unistd.h>
#include <errno.h>
#include <fcntl.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <time.h>

#include "config.h"
#include <serval.h>
#include <serval/str.h>

#include "net.h"
#include "debug.h"

int __set_nonblock(int fd)
{
  int flags;
  CHECK((flags = fcntl(fd, F_GETFL, NULL)) != -1,"set_nonblock: fcntl(%d,F_GETFL,NULL)", fd);
  CHECK(fcntl(fd, F_SETFL, flags | O_NONBLOCK) != -1,"set_nonblock: fcntl(%d,F_SETFL,0x%x|O_NONBLOCK)", fd, flags);
  return 0;
error:
  return -1;
}

int __set_block(int fd)
{
  int flags;
  CHECK((flags = fcntl(fd, F_GETFL, NULL)) != -1,"set_block: fcntl(%d,F_GETFL,NULL)", fd);
  CHECK(fcntl(fd, F_SETFL, flags & ~O_NONBLOCK) != -1,"set_block: fcntl(%d,F_SETFL,0x%x&~O_NONBLOCK)", fd, flags);
  return 0;
error:
  return -1;
}
