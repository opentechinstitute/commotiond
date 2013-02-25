/*! 
 *
 * \file client.c 
 *
 * \brief commotion - a client to the embedded C daemon for managing mesh 
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
#include "msg.h"
#include "debug.h"
#include "util.h"
#include "socket.h"

extern socket_t unix_socket_proto;

static msg_t *cli_parse_string(const char *input) {
  CHECK(input != NULL, "No input.");
	char *saveptr = NULL;
  DEBUG("Checking for tokens.");
  char *input_tmp = strdup(input);
	char *command = strtok_r(input_tmp, " ", &saveptr);
  if(strlen(command) < 2) command = "help";
  char *payload = strchr(input, ' ');
  DEBUG("payload: %s", payload);
  msg_t *message = msg_create(command, payload);
  CHECK(message != NULL, "Invalid message.");
  free(input_tmp);
  return message;

error:
  free(input_tmp);
  free(message);
  return NULL;
}

static void print_usage() {
  printf(
          "The Commotion management shell.\n"
          "http://commotionwireless.net\n\n"
          "Usage: commotion [options]\n"
          "\n"
          "Options:\n"
          " -b, --bind <uri>    Specify management socket.\n"
          " -h, --help          Print this usage message.\n"
  );
}

int main(int argc, char *argv[]) {
  int opt = 0;
  int opt_index = 0;
  char *socket_uri = COMMOTION_MANAGESOCK;

  static const char *opt_string = "b:h";

  static struct option long_opts[] = {
    {"bind", required_argument, NULL, 'b'},
    {"help", no_argument, NULL, 'h'}
  };

  opt = getopt_long(argc, argv, opt_string, long_opts, &opt_index);

  while(opt != -1) {
    switch(opt) {
      case 'b':
        socket_uri = optarg;
        break;
      case 'h':
      default:
        print_usage();
        return 0;
        break;
    }
    opt = getopt_long(argc, argv, opt_string, long_opts, &opt_index);
  }


  socket_t *socket = NEW(socket, unix_socket);
  if(socket->connect(socket, socket_uri)) printf("Connected to commotiond at %s\n", socket_uri);
  char str[100];
  int received = 0;
  while(printf("Co$ "), fgets(str, 100, stdin), !feof(stdin)) {
    char *msgstr = msg_pack(cli_parse_string(str));
    CHECK(socket->send(socket, msgstr, sizeof(msg_t)) != -1, "Send error!");
    if((received = socket->receive(socket, str, sizeof(str))) > 0) {
      str[received + 1] = '\0';
      printf("%s", str);
    }
  }

  return 0;
error:
  socket->destroy(socket);
  return 1;
}

