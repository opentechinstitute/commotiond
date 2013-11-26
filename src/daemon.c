/* vim: set ts=2 expandtab: */
/**
 *       @file  daemon.c
 *      @brief  commotiond - an embedded C daemon for managing mesh networks.
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
#include <stdio.h>
#include <getopt.h>
#include <unistd.h>
#include <signal.h>
#include <fcntl.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <sys/stat.h>
#include "config.h"
#include "debug.h"
#include "command.h"
#include "util.h"
#include "loop.h"
#include "process.h"
#include "profile.h"
#include "socket.h"
#include "msg.h"
#include "olsrd.h"
#include "iface.h"
#include "id.h"

extern co_socket_t unix_socket_proto;
static int pid_filehandle;

int dispatcher_cb(void *self, void *context);

/**
 * @brief sends/receives socket messages
 * @param self pointer to dispatcher socket struct
 * @param context void context pointer required for event loop callback (currently unused)
 */
int dispatcher_cb(void *self, void *context) {
  co_socket_t *sock = self;
  char buffer[1024];
  char *cmdargv[MAX_ARGS];
  int cmdargc;
  memset((void *)buffer, '\0', sizeof(buffer));

  /* Incoming message on socket */
  int received = sock->receive(sock, buffer, sizeof(buffer));
  DEBUG("Received %d bytes.", received);
  if(received == 0) {
    INFO("Received connection.");
    return 1;
  }
  if (received < 0) {
    INFO("Connection recvd() -1");
    sock->hangup(sock, context);
    return 1;
  }
  /* If it's a commotion message type, parse the header, target and payload */
  co_msg_t *msgrcv = co_msg_unpack(buffer);
  if(msgrcv->header.size <= (sizeof(co_msg_header_t) + strlen(msgrcv->target))) {
    DEBUG("Received message with target %s and empty payload.", msgrcv->target);
    cmdargc = 0;
    cmdargv[0] = NULL;
  } else {
    DEBUG("Received target %s and payload %s", msgrcv->target, msgrcv->payload);
    string_to_argv(msgrcv->payload, cmdargv, &cmdargc, MAX_ARGS);
  }
  /*  */
  char *ret = co_cmd_exec(msgrcv->target, cmdargv, cmdargc, 0);
  if(ret) {
    sock->send(sock, ret, strlen(ret));
    free(ret);
  } else {
    sock->send(sock, "No such command.\n", 17);
  }

 // for(int i = 0; i < cmdargc; i++) {
 //   if(cmdargv[i]) free(cmdargv[i]);
 // }
  return 1;
}

 /**
  * @brief starts the daemon
  * @param statedir directory in which to store lock file
  * @param pidfile name of lock file (stores process id)
  * @warning ensure that there is only one copy 
  */
static void daemon_start(char *statedir, char *pidfile) {
  int pid, sid, i;
  char str[10];


 /*
  * Check if parent process id is set
  * 
  * If PPID exists, we are already a daemon
  */
   if (getppid() == 1) {
    return;
  }

  
  
  pid = fork(); /* Fork parent process */

  if (pid < 0) {
    exit(EXIT_FAILURE);
  }

  if (pid > 0) {
    /* Child created correctly */
    INFO("Child process created: %d\n", pid);
    exit(EXIT_SUCCESS); /* exit parent process */
  }

 /* Child continues from here */
 
 /* 
  * Set file permissions to 750 
  * -- Owner may read/write/execute
  * -- Group may read/write
  * -- World has no permissions
  */
  umask(027); 

  /* Get a new process group */
  sid = setsid();

  if (sid < 0) {
    exit(EXIT_FAILURE);
  }

  /* Close all descriptors */
  for (i = getdtablesize(); i >=0; --i) {
    close(i);
  }

  /* Route i/o connections */
  close(STDIN_FILENO);
  close(STDOUT_FILENO);
  close(STDERR_FILENO);

  chdir(statedir); /* Change running directory */

  /*
   * Open lock file
   * Ensure that there is only one copy
   */
  pid_filehandle = open(pidfile, O_RDWR|O_CREAT, 0600);

  if(pid_filehandle == -1) {
  /* Couldn't open lock file */
    ERROR("Could not lock PID lock file %s, exiting", pidfile);
    exit(EXIT_FAILURE);
  }

  /* Get and format PID */
  sprintf(str, "%d\n", getpid());

  /* Write PID to lockfile */
  write(pid_filehandle, str, strlen(str));

}

static void print_usage() {
  printf(
          "The Commotion management daemon.\n"
          "http://commotionwireless.net\n\n"
          "Usage: commotiond [options]\n"
          "\n"
          "Options:\n"
          " -b, --bind <uri>      Specify management socket.\n"
          " -f, --profiles <dir>  Specify profile directory.\n"
          " -i, --id <nodeid>     Specify unique id number for this node.\n"
          " -n, --nodaemonize     Do not fork into the background.\n"
          " -p, --pid <file>      Specify pid file.\n"
          " -s, --statedir <dir>  Specify instance directory.\n"
          " -h, --help            Print this usage message.\n"
  );
}
/**
 * @brief Creates sockets for event loop, daemon and dispatcher. Starts/stops event loop.
 * 
 */
int main(int argc, char *argv[]) {
  int opt = 0;
  int opt_index = 0;
  int daemonize = 1;
  uint32_t newid = 0;
  char *pidfile = COMMOTION_PIDFILE;
  char *statedir = COMMOTION_STATEDIR;
  char *socket_uri = COMMOTION_MANAGESOCK;
  //char *plugindir = COMMOTION_PLUGINDIR;
  char *profiledir = COMMOTION_PROFILEDIR;

  static const char *opt_string = "b:d:f:i:np:s:h";

  static struct option long_opts[] = {
    {"bind", required_argument, NULL, 'b'},
    {"plugins", required_argument, NULL, 'd'},
    {"profiles", required_argument, NULL, 'f'},
    {"nodeid", required_argument, NULL, 'i'},
    {"nodaemon", no_argument, NULL, 'n'},
    {"pid", required_argument, NULL, 'p'},
    {"statedir", required_argument, NULL, 's'},
    {"help", no_argument, NULL, 'h'}
  };
  

 /* Parse command line arguments */
  opt = getopt_long(argc, argv, opt_string, long_opts, &opt_index);

  while(opt != -1) {
    switch(opt) {
      case 'b':
        socket_uri = optarg;
        break;
      case 'd':
        //plugindir = optarg;
        break;
      case 'f':
        profiledir = optarg;
        break;
      case 'i':
        newid = strtoul(optarg,NULL,10);
        break;
      case 'n':
        daemonize = 0;
        break;
      case 'p':
        pidfile = optarg;
        break;
      case 's':
        statedir = optarg;
        break;
      case 'h':
      default:
        print_usage();
        return 0;
        break;
    }
    opt = getopt_long(argc, argv, opt_string, long_opts, &opt_index);
  }

  /* If the daemon is needed, start the daemon */
  if(daemonize) daemon_start((char *)statedir, (char *)pidfile); /* Input state directory and lockfile with process id */
  co_id_set_from_int(newid);
  nodeid_t id = co_id_get();
  DEBUG("Node ID: %d", (int) id.id);
  co_loop_create(); /* Start event loop */
  co_ifaces_create(); /* Configure interfaces */
  co_profiles_create(); /* Set up profiles */
  co_profile_import_files(profiledir); /* Import profiles from profiles directory */
  
  /* Add standard commands */
  co_cmd_add("help", cmd_help, "help <none>\n", "Print list of commands and usage information.\n", 0);
  co_cmd_add("list_profiles", cmd_list_profiles, "list_profiles <none>\n", "Print list of available profiles.\n", 0);
  co_cmd_add("up", cmd_up, "up <interface> <profile>\n", "Apply profile to interface.\n", 0);
  co_cmd_add("down", cmd_down, "down <interface>\n", "Bring specified interface down.\n", 0);
  co_cmd_add("status", cmd_status, "status <interface>\n", "Report profile of connected interface.\n", 0);
  co_cmd_add("state", cmd_state, "state <interface> <property>\n", "Report properties of connected interface.\n", 0);
  co_cmd_add("nodeid", cmd_nodeid, "nodeid <none>\n", "Print unique ID for this node\n", 0);
  co_cmd_add("nodeidset", cmd_set_nodeid_from_mac, "nodeid <mac>\n", "Use mac address to generate identifier for this node.\n", 0);
  co_cmd_add("localipset", cmd_generate_local_ip, "localip <none>\n", "Generate the local ip address for this node\n", 0);
  //plugins_create();
  //plugins_load_all(plugindir);
  
  /* Set up sockets */
  co_socket_t *socket = NEW(co_socket, unix_socket);
  socket->poll_cb = dispatcher_cb;
  socket->register_cb = co_loop_add_socket;
  socket->bind(socket, socket_uri);
  co_loop_start();
  co_loop_destroy();

  return 0;
}
