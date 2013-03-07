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

extern co_socket_t unix_socket_proto;
static int pid_filehandle;

int dispatcher_cb(void *self, void *context);

int dispatcher_cb(void *self, void *context) {
  co_socket_t *sock = self;
  char buffer[1024];
  char *cmdargv[MAX_ARGS];
  int cmdargc;
  memset((void *)buffer, '\0', sizeof(buffer));

  int received = sock->receive(sock, buffer, sizeof(buffer));
  DEBUG("Received %d bytes.", received);
  if(received <= 0) {
    INFO("Received connection.");
    return 1;
  }
  co_msg_t *msgrcv = co_msg_unpack(buffer);
  if(msgrcv->header.size <= (sizeof(co_msg_header_t) + strlen(msgrcv->target))) {
    DEBUG("Received message with target %s and empty payload.", msgrcv->target);
    cmdargc = 0;
    cmdargv[0] = NULL;
  } else {
    DEBUG("Received target %s and payload %s", msgrcv->target, msgrcv->payload);
    string_to_argv(msgrcv->payload, cmdargv, &cmdargc, MAX_ARGS);
  }
  char *ret = co_cmd_exec(msgrcv->target, cmdargv, cmdargc, 0);
  if(ret != NULL) {
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

static void daemon_start(char *rundir, char *pidfile) {
  int pid, sid, i;
  char str[10];

  //Check if parent process id is set
  if (getppid() == 1) {
    //PPID exists, therefore we are already a daemon
    return;
  }

  //FORK!
  pid = fork();

  if (pid < 0) {
    //Could not fork
    exit(EXIT_FAILURE);
  }

  if (pid > 0) {
    //Child created correctly, so exit the parent process
    INFO("Child process created: %d\n", pid);
    exit(EXIT_SUCCESS);
  }

  //Child continues from here:
  
  umask(027); //Set file permissions 750

  //Get a new process group
  sid = setsid();

  if (sid < 0) {
    exit(EXIT_FAILURE);
  }

  //Close all descriptors
  for (i = getdtablesize(); i >=0; --i) {
    close(i);
  }

  //Route i/o connections
  close(STDIN_FILENO);
  close(STDOUT_FILENO);
  close(STDERR_FILENO);

  chdir(rundir); //change running directory

  pid_filehandle = open(pidfile, O_RDWR|O_CREAT, 0600); //Ensure only one copy

  if(pid_filehandle == -1) {
    //Couldn't open lock file
    ERROR("Could not lock PID lock file %s, exiting", pidfile);
    exit(EXIT_FAILURE);
  }

  //Get and format PID
  sprintf(str, "%d\n", getpid());

  //Write PID to lockfile
  write(pid_filehandle, str, strlen(str));

}

static void print_usage() {
  printf(
          "The Commotion management daemon.\n"
          "http://commotionwireless.net\n\n"
          "Usage: commotiond [options]\n"
          "\n"
          "Options:\n"
          " -b, --bind <uri>    Specify management socket.\n"
          " -n, --nodaemonize   Do not fork into the background.\n"
          " -p, --pid <file>    Specify pid file.\n"
          " -r, --rundir <dir>  Specify instance directory.\n"
          " -h, --help          Print this usage message.\n"
  );
}

int main(int argc, char *argv[]) {
  int opt = 0;
  int opt_index = 0;
  int daemonize = 1;
  char *pidfile = COMMOTION_PIDFILE;
  char *rundir = COMMOTION_RUNDIR;
  char *socket_uri = COMMOTION_MANAGESOCK;
  //char *plugindir = COMMOTION_PLUGINDIR;
  char *profiledir = COMMOTION_PROFILEDIR;

  static const char *opt_string = "b:d:f:np:r:h";

  static struct option long_opts[] = {
    {"bind", required_argument, NULL, 'b'},
    {"plugins", required_argument, NULL, 'd'},
    {"profiles", required_argument, NULL, 'f'},
    {"nodaemon", no_argument, NULL, 'n'},
    {"pid", required_argument, NULL, 'p'},
    {"rundir", required_argument, NULL, 'r'},
    {"help", no_argument, NULL, 'h'}
  };

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
      case 'n':
        daemonize = 0;
        break;
      case 'p':
        pidfile = optarg;
        break;
      case 'r':
        rundir = optarg;
        break;
      case 'h':
      default:
        print_usage();
        return 0;
        break;
    }
    opt = getopt_long(argc, argv, opt_string, long_opts, &opt_index);
  }


  if(daemonize) daemon_start((char *)rundir, (char *)pidfile);
  co_loop_create();
  co_profiles_create();
  co_profile_import_files(profiledir);
  co_cmd_add("help", cmd_help, "help <none>\n", "Print list of commands and usage information.\n", 0);
  co_cmd_add("list_profiles", cmd_list_profiles, "list_profiles <none>\n", "Print list of available profiles.\n", 0);
  co_cmd_add("up", cmd_up, "up <interface> <profile>\n", "Apply profile to interface.\n", 0);
  //plugins_create();
  //plugins_load_all(plugindir);
  co_socket_t *socket = NEW(co_socket, unix_socket);
  socket->poll_cb = dispatcher_cb;
  socket->register_cb = co_loop_add_socket;
  socket->bind(socket, socket_uri);
  co_loop_start();
  co_loop_destroy();

  return 0;
}
