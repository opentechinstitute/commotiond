/*! 
 *
 * \file main.c 
 *
 * \brief commotiond - an embedded C daemon and library for managing mesh 
 *        network profiles
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

extern socket_t unix_socket_proto;
static int pid_filehandle;

int dispatcher_cb(void *self, void *context);

int dispatcher_cb(void *self, void *context) {
  socket_t *sock = self;
  char buffer[256];
  memset((void *)buffer, '\0', sizeof(buffer));
  DEBUG("Handling socket callback.");

  sock->receive(sock, buffer, sizeof(buffer));
  msg_t *msgrcv = msg_unpack(buffer);
  DEBUG("Received target %s and payload %s", msgrcv->target, msgrcv->payload);
  char *ret = command_exec(msgrcv->target, msgrcv->payload, 0);
  if(ret != NULL) {
    sock->send(sock, ret, strlen(ret));
  } else {
    sock->send(sock, "No such command.\n", 17);
  }

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

  static const char *opt_string = "b:d:np:r:h";

  static struct option long_opts[] = {
    {"bind", required_argument, NULL, 'b'},
    {"plugins", required_argument, NULL, 'd'},
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
  loop_create();
  //plugins_create();
  //plugins_load_all(plugindir);
  socket_t *socket = NEW(socket, unix_socket);
  socket->poll_cb = dispatcher_cb;
  socket->register_cb = loop_add_socket;
  socket->bind(socket, socket_uri);
  loop_start();
  loop_destroy();

  return 0;
}
