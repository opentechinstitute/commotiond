/* vim: set ts=2 expandtab: */
/**
 *       @file  client.c
 *      @brief  commotion - a client to the embedded C daemon
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
#include "msg.h"
#include "debug.h"
#include "util.h"
#include "socket.h"

extern co_socket_t unix_socket_proto;

/**
 * @brief parses command line inputs and packages into a commotion message
 * @param argv[] commands input by the user
 * @param argc number of commands and flags input
 */
static co_msg_t *cli_parse_argv(char *argv[], const int argc) {
  CHECK(argv != NULL, "No input.");
  char payload[MSG_MAX_PAYLOAD];
  memset(payload, '\0', sizeof(payload));
  co_msg_t *message;
  if(argc > 0) {
    char **args = argv + 1;
    CHECK(argv_to_string(args, argc, payload, MSG_MAX_PAYLOAD), "Failed to parse argv.");
    message = co_msg_create(argv[0], payload);
  } else {
    message = co_msg_create(argv[0], NULL);
  }
  CHECK(message != NULL, "Invalid message.");
  return message;

error:
  free(message);
  return NULL;
}

/**
 * @brief checks user input for valid commands
 * @param input user input submitted at the command prompt
 */
static co_msg_t *cli_parse_string(const char *input) {
  CHECK(input != NULL, "No input.");
	char *saveptr = NULL;
  int inputsize = strlen(input);
  char *input_tmp = NULL;
  co_msg_t *message = NULL;
  if(inputsize > 2) {
    input_tmp = malloc(inputsize);
    strstrip(input, input_tmp, inputsize);
    if(input_tmp[inputsize - 1] == '\n') input_tmp[inputsize - 1] ='\0';
	  char *command = strtok_r(input_tmp, " ", &saveptr);
    if(strlen(command) < 2) command = "help";
    char *payload = strchr(input, ' ');
    message = co_msg_create(command, payload);
  } else {
    message = co_msg_create("help", NULL);
  }
  CHECK(message != NULL, "Invalid message.");
  free(input_tmp);
  return message;

error:
  free(input_tmp);
  free(message);
  return NULL;
}

/**
 * @brief prints Commotion management shell usage
 * @details allows user to specify management socket
 */
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

  co_socket_t *socket = NEW(co_socket, unix_socket);

  CHECK((socket->connect(socket, socket_uri)), "Failed to connect to commotiond at %s\n", socket_uri);
  DEBUG("opt_index: %d argc: %d", optind, argc);
  int received = 0;
  char str[1024];
  memset(str, '\0', sizeof(str));
  if(optind < argc) {
    co_msg_t *message = cli_parse_argv(argv + optind, argc - optind - 1);
    char *msgstr = co_msg_pack(message);
    CHECK(socket->send(socket, msgstr, sizeof(co_msg_t)) != -1, "Send error!");
    if((received = socket->receive(socket, str, sizeof(str))) > 0) {
      str[received] = '\0';
      printf("%s", str);
    }
  } else {
    printf("Connected to commotiond at %s\n", socket_uri);
    while(printf("Co$ "), fgets(str, 100, stdin), !feof(stdin)) {
      //if(str[strlen(str) - 1] == '\n') {
      //  str[strlen(str) - 1] = '\0';
      //}
      char *msgstr = co_msg_pack(cli_parse_string(str));
      CHECK(socket->send(socket, msgstr, sizeof(co_msg_t)) != -1, "Send error!");
      if((received = socket->receive(socket, str, sizeof(str))) > 0) {
        str[received] = '\0';
        printf("%s", str);
      }
    }
  }

  socket->destroy(socket);
  return 0;
error:
  socket->destroy(socket);
  return 1;
}

