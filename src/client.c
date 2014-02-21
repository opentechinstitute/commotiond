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
 * it under the terms of the GNU Affero General Public License as
 * published by the Free Software Foundation, either version 3 of the
 * License, or (at your option) any later version.
 * 
 * Commotion is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU Affero General Public License for more details.
 * 
 * You should have received a copy of the GNU Affero General Public License
 * along with Commotion.  If not, see <http://www.gnu.org/licenses/>.
 *
 * =====================================================================================
 */

#include <stdlib.h>
#include <stdio.h>
#include <getopt.h>
#include <string.h>
#include "config.h"
#include "debug.h"
#include "util.h"
#include "commotion.h"

#define INPUT_MAX 255

/**
 * @brief parses command line inputs and packages into a commotion message
 * @param argv[] commands input by the user
 * @param argc number of commands and flags input
 */

static co_obj_t * 
cli_parse_argv(char *argv[], const int argc)
{
  co_obj_t *request = co_request_create();
  if(argc > 0) 
  {
    for(int i = 0; i < argc; i++)
    {
      CHECK(co_request_append_str(request, argv[i], strlen(argv[i]) + 1), "Failed to add to argument list.");
    }
  }
  return request;
error:
  co_free(request);
  return NULL; 
}

/**
 * @brief checks user input for valid commands
 * @param input user input submitted at the command prompt
 */
static co_obj_t *
cli_parse_string(char *input, const size_t ilen) 
{
	char *saveptr = NULL, *token = NULL;
  co_obj_t *request = co_request_create();
  if(input != NULL && ilen > 0)
  {
	  token = strtok_r(input, " ", &saveptr);
    DEBUG("Token: %s, Length: %d", token, (int)strlen(token));
  }
  while(token != NULL)
  {
    CHECK(co_request_append_str(request, token, strlen(token) + 1), "Failed to add to argument list.");
    DEBUG("Token: %s, Length: %d", token, (int)strlen(token));
    token = strtok_r(NULL, " ", &saveptr);
  }
  return request;

error:
  co_free(request);
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
  int retval = 1;
  int opt = 0;
  int opt_index = 0;
  char *socket_uri = COMMOTION_MANAGESOCK;
  char *method = NULL;
  size_t mlen = 0;
  co_obj_t *request = NULL, *response = NULL;

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

  DEBUG("optind: %d, argc: %d", optind, argc);
  co_init();
  co_obj_t *conn = co_connect(socket_uri, strlen(socket_uri) + 1);
  CHECK(conn != NULL, "Failed to connect to commotiond at %s\n", socket_uri);

  if(optind < argc) 
  {
    method = argv[optind];
    mlen = strlen(argv[optind]) + 1;
    request = cli_parse_argv(argv + optind + 1, argc - optind - 1);
    if(co_call(conn, &response, method, mlen, request)) retval = 0;
    CHECK(response != NULL, "Invalid response");
    co_response_print(response);
  } 
  else 
  {
    printf("Connected to commotiond at %s\n", socket_uri);
    char input[INPUT_MAX];
    char buf[INPUT_MAX];
    memset(input, '\0', sizeof(input));
    int blen = 0;
    while(printf("Co$ "), fgets(input, INPUT_MAX, stdin), !feof(stdin)) 
    {
      strstrip(input, buf, INPUT_MAX);
      blen = strlen(buf);
      if(buf[blen - 1] == '\n') buf[blen - 1] ='\0';
	    method = strtok(buf, " ");
      mlen = strlen(method) + 1;
      if(mlen < 2) method = "help";
      if(blen > mlen)
      {
        request = cli_parse_string(buf + mlen, blen - mlen);
      }
      else
      {
        request = cli_parse_string(NULL, 0);
      }
      if(co_call(conn, &response, method, mlen, request)) retval = 0;
      CHECK(response != NULL, "Invalid response");
      co_free(request);
      co_response_print(response);
      co_free(response);
    }
  }
  co_shutdown();
  return retval;
error:
  co_shutdown();
  return retval;
}

