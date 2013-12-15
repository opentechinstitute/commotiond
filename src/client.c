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
#include "obj.h"
#include "list.h"
#include "tree.h"

#define REQUEST_MAX 4096
#define RESPONSE_MAX 4096
#define INPUT_MAX 255

extern co_socket_t unix_socket_proto;

/**
 * @brief parses command line inputs and packages into a commotion message
 * @param argv[] commands input by the user
 * @param argc number of commands and flags input
 */

static size_t 
cli_parse_argv(char *output, const size_t olen, char *argv[], const int argc)
{
  CHECK(((argv != NULL) && (argc > 0)), "No input.");
  co_obj_t *params = co_list16_create();
  if(argc > 1) 
  {
    for(int i = 1; i < argc; i++)
    {
      CHECK(co_list_append(params, co_str8_create(argv[i], strlen(argv[i]) + 1, 0)), "Failed to add to argument list.");
    }
    DEBUG("Parameter list size: %d", (int)co_list_length(params));
  }
  co_obj_t *method = co_str8_create(argv[0], strlen(argv[0]) + 1, 0);
  size_t retval = co_request_alloc(output, olen, method, params);
  DEBUG("Request: %s", output);
  co_obj_free(params);
  co_obj_free(method);
  return retval;
error:
  co_obj_free(params);
  return -1; 
}

/**
 * @brief checks user input for valid commands
 * @param input user input submitted at the command prompt
 */
static size_t
cli_parse_string(char *output, const size_t olen, const char *input, const size_t ilen) 
{
  CHECK(((input != NULL) && (ilen > 0)), "No input.");
	char *saveptr = NULL;
  char *buf = NULL;
  size_t blen = ilen; 
  co_obj_t *method = NULL;
  co_obj_t *params = NULL;
  if(ilen > 2) /* Make sure it's not just, say, a null byte and a newline. */
  {
    params = co_list16_create();
    buf = h_calloc(1, ilen);
    strstrip(input, buf, blen);
    if(buf[blen - 1] == '\n') buf[blen - 1] ='\0';
	  char *token = strtok_r(buf, " ", &saveptr);
    DEBUG("Token: %s, Length: %d", token, (int)strlen(token));
    if(strlen(token) < 2) token = "help";
    method = co_str8_create(token, strlen(token) + 1, 0);
    token = strtok_r(NULL, " ", &saveptr);
    while(token != NULL)
    {
      CHECK(co_list_append(params, co_str8_create(token, strlen(token) + 1, 0)), "Failed to add to argument list.");
      DEBUG("Token: %s, Length: %d", token, (int)strlen(token));
      token = strtok_r(NULL, " ", &saveptr);
    }
  } 
  else 
  {
    SENTINEL("Message too short.");
  }
  size_t retval = co_request_alloc(output, olen, method, params);
  co_obj_free(method);
  co_obj_free(params);
  if(buf != NULL) h_free(buf);
  return retval;

error:
  co_obj_free(method);
  co_obj_free(params);
  if(buf != NULL) h_free(buf);
  return -1;
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
  int retval = 0;
  int opt = 0;
  int opt_index = 0;
  char *socket_uri = COMMOTION_MANAGESOCK;
  co_obj_t *rlist = NULL, *rtree = NULL;

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

  co_obj_t *socket = NEW(co_socket, unix_socket);

  CHECK((((co_socket_t*)socket)->connect(socket, socket_uri)), "Failed to connect to commotiond at %s\n", socket_uri);
  DEBUG("opt_index: %d argc: %d", optind, argc);
  char request[REQUEST_MAX];
  memset(request, '\0', sizeof(request));
  size_t reqlen = 0;
  char response[RESPONSE_MAX];
  memset(response, '\0', sizeof(response));
  size_t resplen = 0;
  if(optind < argc) 
  {
    reqlen = cli_parse_argv(request, REQUEST_MAX, argv + optind, argc - optind);
    CHECK(((co_socket_t*)socket)->send(socket, request, reqlen) != -1, "Send error!");
    if((resplen = ((co_socket_t*)socket)->receive(socket, response, sizeof(response))) > 0) 
    {
      CHECK(co_list_import(&rlist, response, resplen) > 0, "Failed to parse response.");
      rtree = co_list_element(rlist, 3);
      if(!IS_NIL(rtree))
      {
        if(rtree != NULL && IS_TREE(rtree)) co_tree_print(rtree);
        retval = 0;
      }
      else
      {
        rtree = co_list_element(rlist, 2);
        if(rtree != NULL && IS_TREE(rtree)) co_tree_print(rtree);
        retval = 1;
      }
    }
  } 
  else 
  {
    printf("Connected to commotiond at %s\n", socket_uri);
    char input[INPUT_MAX];
    memset(input, '\0', sizeof(input));
    while(printf("Co$ "), fgets(input, 100, stdin), !feof(stdin)) 
    {
      reqlen = cli_parse_string(request, REQUEST_MAX, input, strlen(input));
      CHECK(((co_socket_t*)socket)->send(socket, request, reqlen) != -1, "Send error!");
      if((resplen = ((co_socket_t*)socket)->receive(socket, response, sizeof(response))) > 0) 
      {
        CHECK(co_list_import(&rlist, response, resplen) > 0, "Failed to parse response.");
        rtree = co_list_element(rlist, 3);
        if(!IS_NIL(rtree))
        {
          if(rtree != NULL && IS_TREE(rtree)) co_tree_print(rtree);
          retval = 0;
        }
        else
        {
          rtree = co_list_element(rlist, 2);
          printf("Error: ");
          if(rtree != NULL && IS_TREE(rtree)) co_tree_print(rtree);
          retval = 1;
        }
      }
    }
  }

  co_obj_free(rtree);
  co_obj_free(rlist);
  ((co_socket_t*)socket)->destroy(socket);
  return retval;
error:
  co_obj_free(rtree);
  co_obj_free(rlist);
  ((co_socket_t*)socket)->destroy(socket);
  return retval;
}

