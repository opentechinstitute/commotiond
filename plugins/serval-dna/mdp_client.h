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

#include <serval.h>

struct __overlay_mdp_scan{
  struct in_addr addr;
};

time_ms_t __gettime_ms();

/* Client-side MDP function */
extern int __mdp_client_socket;
int __overlay_mdp_client_init();
int __overlay_mdp_client_done();
int __overlay_mdp_client_poll(time_ms_t timeout_ms);
int __overlay_mdp_recv(overlay_mdp_frame *mdp, int port, int *ttl);
int __overlay_mdp_send(overlay_mdp_frame *mdp,int flags,int timeout_ms);
int __overlay_mdp_relevant_bytes(overlay_mdp_frame *mdp);

int __overlay_mdp_getmyaddr(unsigned index, sid_t *sid);
int __overlay_mdp_bind(const sid_t *localaddr, int port);

#endif