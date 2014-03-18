/**
 *       @file  mdp_client.h
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
 Copyright (C) 2012 Serval Project.
 
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

#ifndef __CO_SERVAL_MDP_CLIENT_H
#define __CO_SERVAL_MDP_CLIENT_H

#include "config.h"
#include <serval.h>
#include <serval/mdp_client.h>

#include "debug.h"

#undef WHY
#define WHY(J)              (ERROR("%s",J), -1)

#undef WHY_perror
#define WHY_perror(J)       (ERROR("%s",J), -1)

#undef WHYF
#define WHYF(F,...)         (ERROR(F, ##__VA_ARGS__), -1)

#undef WHYF_perror
#define WHYF_perror(F,...)  (ERROR(F, ##__VA_ARGS__), -1)

#undef DEBUGF
#define DEBUGF(F,...)        DEBUG(F, ##__VA_ARGS__)

#undef WARNF
#define WARNF(F,...)         WARN(F, ##__VA_ARGS__)

#undef WARNF_perror
#define WARNF_perror(F,...)  WARNF(F ": %s [errno=%d]", ##__VA_ARGS__, strerror(errno), errno)

#undef WARN_perror
#define WARN_perror(X)       WARNF_perror("%s", (X))

#undef FATALF
#define FATALF(F,...)        do { ERROR(F, ##__VA_ARGS__); abort(); exit(-1); } while (1)

#undef FATAL
#define FATAL(J)             FATALF("%s", (J))

#undef FATALF_perror
#define FATALF_perror(F,...) FATALF(F ": %s [errno=%d]", ##__VA_ARGS__, strerror(errno), errno)

#undef FATAL_perror
#define FATAL_perror(J)      FATALF_perror("%s", (J))

#undef INFOF
#define INFOF(F,...)         INFO(F, ##__VA_ARGS__)

struct __overlay_mdp_scan{
  struct in_addr addr;
};

time_ms_t __gettime_ms();

/* Client-side MDP function */
int __overlay_mdp_client_socket(void);
int __overlay_mdp_client_close(int mdp_sockfd);
int __overlay_mdp_client_poll(int mdp_sockfd, time_ms_t timeout_ms);
int __overlay_mdp_getmyaddr(int mpd_sockfd, unsigned index, sid_t *sid);
int __overlay_mdp_bind(int mdp_sockfd, const sid_t *localaddr, mdp_port_t port) ;
int __overlay_mdp_recv(int mdp_sockfd, overlay_mdp_frame *mdp, mdp_port_t port, int *ttl);
int __overlay_mdp_send(int mdp_sockfd, overlay_mdp_frame *mdp, int flags, int timeout_ms);
ssize_t __overlay_mdp_relevant_bytes(overlay_mdp_frame *mdp);

#endif