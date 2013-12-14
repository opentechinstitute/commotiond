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
#include "cmd.h"
#include "util.h"
#include "loop.h"
#include "plugin.h"
#include "process.h"
#include "profile.h"
#include "socket.h"
#include "msg.h"
#include "olsrd.h"
#include "iface.h"
#include "id.h"
#include "obj.h"
#include "list.h"
#include "tree.h"
#include "core.h"

#define REQUEST_MAX 1024
#define RESPONSE_MAX 1024

extern co_socket_t unix_socket_proto;
static int pid_filehandle;

SCHEMA(default)
{
  SCHEMA_ADD("ssid", "commotionwireless.net"); 
  SCHEMA_ADD("bssid", "02:CA:FF:EE:BA:BE"); 
  SCHEMA_ADD("bssidgen", "true"); 
  SCHEMA_ADD("channel", "5"); 
  SCHEMA_ADD("mode", "adhoc"); 
  SCHEMA_ADD("type", "mesh"); 
  SCHEMA_ADD("dns", "208.67.222.222"); 
  SCHEMA_ADD("domain", "mesh.local"); 
  SCHEMA_ADD("ipgen", "true"); 
  SCHEMA_ADD("ip", "100.64.0.0"); 
  SCHEMA_ADD("netmask", "255.192.0.0"); 
  SCHEMA_ADD("ipgenmask", "255.192.0.0"); 
  SCHEMA_ADD("wpa", "true"); 
  SCHEMA_ADD("wpakey", "c0MM0t10n!r0cks"); 
  SCHEMA_ADD("servald", "false"); 
  SCHEMA_ADD("servalsid", ""); 
  SCHEMA_ADD("announce", "true"); 
  return 1;
}

SCHEMA(global)
{
  SCHEMA_ADD("bind", COMMOTION_MANAGESOCK); 
  SCHEMA_ADD("state", COMMOTION_STATEDIR); 
  SCHEMA_ADD("pid", COMMOTION_PIDFILE); 
  SCHEMA_ADD("plugins", COMMOTION_PLUGINDIR); 
  SCHEMA_ADD("profiles", COMMOTION_PROFILEDIR); 
  SCHEMA_ADD("id", "0"); 
  return 1;
}

static co_obj_t *
_cmd_help_i(co_obj_t *data, co_obj_t *current, void *context) 
{
  co_tree_insert((co_obj_t *)context, "command", sizeof("command"), ((co_cmd_t *)current)->name);
  co_tree_insert((co_obj_t *)context, "usage", sizeof("usage"), ((co_cmd_t *)current)->usage);
  co_tree_insert((co_obj_t *)context, "description", sizeof("description"), ((co_cmd_t *)current)->desc);
  return NULL;
}

CMD(help)
{
  *output = co_tree16_create();
  return co_cmd_process(_cmd_help_i, (void *)*output);
}

int dispatcher_cb(co_obj_t *self, co_obj_t *context);

/**
 * @brief sends/receives socket messages
 * @param self pointer to dispatcher socket struct
 * @param context void context pointer required for event loop callback (currently unused)
 */
int dispatcher_cb(co_obj_t *self, co_obj_t *context) {
  CHECK(IS_SOCK(self),"Not a socket.");
  co_socket_t *sock = (co_socket_t*)self;
  char reqbuf[REQUEST_MAX];
  memset(reqbuf, '\0', sizeof(reqbuf));
  size_t reqlen = 0;
  char respbuf[RESPONSE_MAX];
  memset(respbuf, '\0', sizeof(respbuf));
  size_t resplen = 0;
  co_obj_t *request = NULL;
  uint8_t *type = NULL;
  uint32_t *id = NULL;
  co_obj_t *nil = co_nil_create(0);

  /* Incoming message on socket */
  reqlen = sock->receive((co_obj_t*)sock, reqbuf, sizeof(reqbuf));
  DEBUG("Received %d bytes.", (int)reqlen);
  if(reqlen == 0) {
    INFO("Received connection.");
    return 1;
  }
  if (reqlen < 0) {
    INFO("Connection recvd() -1");
    sock->hangup((co_obj_t*)sock, context);
    return 1;
  }
  /* If it's a commotion message type, parse the header, target and payload */
  CHECK(co_list_import(&request, reqbuf, reqlen) > 0, "Failed to import request.");
  co_obj_data((char **)&type, co_list_element(request, 0)); 
  CHECK(*type == 0, "Not a valid request.");
  CHECK(co_obj_data((char **)&id, co_list_element(request, 1)) == sizeof(uint32_t), "Not a valid request ID.");
  co_obj_t *ret = NULL;
  if(co_cmd_exec(co_list_element(request, 2), &ret, co_list_element(request, 3)))
  {
    resplen = co_response_alloc(respbuf, sizeof(respbuf), *id, nil, ret);
    sock->send((co_obj_t*)sock, respbuf, resplen);
  }
  else
  {
    resplen = co_response_alloc(respbuf, sizeof(respbuf), *id, ret, nil);
    sock->send((co_obj_t*)sock, respbuf, resplen);
  }

  co_obj_free(nil);
  co_obj_free(request);
  co_obj_free(ret);
  return 1;
error:
  co_obj_free(nil);
  co_obj_free(request);
  co_obj_free(ret);
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
          "https://commotionwireless.net\n\n"
          "Usage: commotiond [options]\n"
          "\n"
          "Options:\n"
          " -c, --config <file>   Specify global config file.\n"
          " -b, --bind <uri>      Specify management socket.\n"
          " -d, --plugins <dir>   Specify plugin directory.\n"
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
  char *config = COMMOTION_CONFIGFILE;
  char *pid = NULL;
  char *state = NULL;
  char *bind = NULL;
  char *plugins = NULL;
  char *profiles = NULL;

  static const char *opt_string = "c:b:d:f:i:np:s:h";

  static struct option long_opts[] = {
    {"config", required_argument, NULL, 'c'},
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
      case 'c':
        config = optarg;
        break;
      case 'b':
        bind = optarg;
        break;
      case 'd':
        plugins = optarg;
        break;
      case 'f':
        profiles = optarg;
        break;
      case 'i':
        newid = strtoul(optarg,NULL,10);
        break;
      case 'n':
        daemonize = 0;
        break;
      case 'p':
        pid = optarg;
        break;
      case 's':
        state = optarg;
        break;
      case 'h':
      default:
        print_usage();
        return 0;
        break;
    }
    opt = getopt_long(argc, argv, opt_string, long_opts, &opt_index);
  }

  co_id_set_from_int(newid);
  nodeid_t id = co_id_get();
  DEBUG("Node ID: %d", (int) id.id);

  co_profiles_init(16); /* Set up profiles */
  SCHEMA_GLOBAL(global);
  if(!co_profile_import_global(config)) WARN("Failed to load global configuration file %s!", config);
  co_obj_t *settings = co_profile_global();
  
  if(pid == NULL)
  {
    if(settings != NULL)
      co_profile_get_str(settings, &pid, "pid", sizeof("pid"));
    else
      pid = COMMOTION_PIDFILE;
  }
  DEBUG("PID file: %s", pid);

  if(bind == NULL)
  {
    if(settings != NULL)
      co_profile_get_str(settings, &bind, "bind", sizeof("bind"));
    else
      bind = COMMOTION_MANAGESOCK;
  }
  DEBUG("Client socket: %s", bind);
  
  if(state == NULL)
  {
    if(settings != NULL)
      co_profile_get_str(settings, &state, "state", sizeof("state"));
    else
      state = COMMOTION_STATEDIR;
  }
  DEBUG("State directory: %s", state);
  
  if(plugins == NULL)
  {
    if(settings != NULL)
      co_profile_get_str(settings, &plugins, "plugins", sizeof("plugins"));
    else
      plugins = COMMOTION_PLUGINDIR;
  }
  DEBUG("Plugins directory: %s", plugins);

  if(profiles == NULL)
  {
    if(settings != NULL)
      co_profile_get_str(settings, &profiles, "profiles", sizeof("profiles"));
    else
      profiles = COMMOTION_PROFILEDIR;
  }
  DEBUG("Profiles directory: %s", profiles);

  //co_profile_delete_global();
  co_plugins_init(16);
  co_cmds_init(16);
  co_loop_create(); /* Start event loop */
  co_ifaces_create(); /* Configure interfaces */
  co_plugins_load(plugins); /* Load plugins and register plugin profile schemas */
  co_profile_import_global(config);
  SCHEMA_REGISTER(default); /* Register default schema */
  co_profile_import_files(profiles); /* Import profiles from profiles directory */
  CMD_REGISTER(help, "help <none>", "Print list of commands and usage information.");
  
  /* Add standard commands */
  /*
  co_cmd_add("help", cmd_help, "help <none>\n", "Print list of commands and usage information.\n", 0);
  co_cmd_add("list_profiles", cmd_list_profiles, "list_profiles <none>\n", "Print list of available profiles.\n", 0);
  co_cmd_add("up", cmd_up, "up <interface> <profile>\n", "Apply profile to interface.\n", 0);
  co_cmd_add("down", cmd_down, "down <interface>\n", "Bring specified interface down.\n", 0);
  co_cmd_add("status", cmd_status, "status <interface>\n", "Report profile of connected interface.\n", 0);
  co_cmd_add("state", cmd_state, "state <interface> <property>\n", "Report properties of connected interface.\n", 0);
  co_cmd_add("nodeid", cmd_nodeid, "nodeid <none>\n", "Print unique ID for this node\n", 0);
  co_cmd_add("nodeidset", cmd_set_nodeid_from_mac, "nodeid <mac>\n", "Use mac address to generate identifier for this node.\n", 0);
  co_cmd_add("localipset", cmd_generate_local_ip, "localipset <none>\n", "Generate the local ip address for this node\n", 0);
  */
  //plugins_create();
  //plugins_load_all(plugindir);
  
  /* Set up sockets */
  co_socket_t *socket = (co_socket_t*)NEW(co_socket, unix_socket);
  socket->poll_cb = dispatcher_cb;
  socket->register_cb = co_loop_add_socket;
  socket->bind((co_obj_t*)socket, bind);
  co_plugins_start();

  /* If the daemon is needed, start the daemon */
  if(daemonize) daemon_start((char *)state, (char *)pid); /* Input state directory and lockfile with process id */

  co_loop_start();
  co_loop_destroy();
  co_cmds_shutdown();
  co_profiles_shutdown();
  co_plugins_shutdown();

  return 0;
}
