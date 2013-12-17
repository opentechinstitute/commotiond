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

#include <serval.h>
#include <serval/conf.h>
#include <serval/mdp_client.h>
#include <serval/rhizome.h>
#include <serval/crypto.h>

#include "debug.h"
#include "util.h"
#include "socket.h"
#include "obj.h"
#include "crypto.h"

#include "serval-dna.h"

#define SERVAL_MDPSOCK "/var/serval-node/mdp.socket"

typedef enum {
  SERVAL_SIGN = 0,
  SERVAL_VERIFY = 1,
  SERVAL_CRYPTO = 2
} serval_cmd;

#define _DECLARE_SERVAL(F) extern int F(const struct cli_parsed *parsed, void *context);
_DECLARE_SERVAL(commandline_usage);
_DECLARE_SERVAL(app_echo);
_DECLARE_SERVAL(app_log);
_DECLARE_SERVAL(app_dna_lookup);
_DECLARE_SERVAL(app_mdp_ping);
_DECLARE_SERVAL(app_trace);
_DECLARE_SERVAL(app_config_schema);
_DECLARE_SERVAL(app_config_dump);
_DECLARE_SERVAL(app_config_set);
_DECLARE_SERVAL(app_config_get);
_DECLARE_SERVAL(app_rhizome_hash_file);
_DECLARE_SERVAL(app_rhizome_add_file);
_DECLARE_SERVAL(app_slip_test);
_DECLARE_SERVAL(app_rhizome_import_bundle);
_DECLARE_SERVAL(app_rhizome_append_manifest);
_DECLARE_SERVAL(app_rhizome_delete);
_DECLARE_SERVAL(app_rhizome_clean);
_DECLARE_SERVAL(app_rhizome_extract);
_DECLARE_SERVAL(app_rhizome_export_file);
_DECLARE_SERVAL(app_rhizome_list);
_DECLARE_SERVAL(app_keyring_create);
_DECLARE_SERVAL(app_keyring_dump);
_DECLARE_SERVAL(app_keyring_list);
_DECLARE_SERVAL(app_keyring_add);
_DECLARE_SERVAL(app_keyring_set_did);
_DECLARE_SERVAL(app_id_self);
_DECLARE_SERVAL(app_route_print);
_DECLARE_SERVAL(app_network_scan);
_DECLARE_SERVAL(app_count_peers);
_DECLARE_SERVAL(app_dna_lookup);
_DECLARE_SERVAL(app_reverse_lookup);
_DECLARE_SERVAL(app_monitor_cli);
_DECLARE_SERVAL(app_crypt_test);
_DECLARE_SERVAL(app_nonce_test);
#undef _DECLARE_SERVAL

#define KEYRING_PIN_OPTIONS ,"[--keyring-pin=<pin>]","[--entry-pin=<pin>]..."
static struct cli_schema command_line_options[]={
  {commandline_usage,{"help|-h|--help","...",NULL},CLIFLAG_PERMISSIVE_CONFIG,
   "Display command usage."},
  {app_echo,{"echo","[-e]","[--]","...",NULL},CLIFLAG_PERMISSIVE_CONFIG,
   "Output the supplied string."},
  {app_log,{"log","error|warn|hint|info|debug","<message>",NULL},CLIFLAG_PERMISSIVE_CONFIG,
   "Log the supplied message at given level."},
//   {app_server_start,{"start","[foreground|exec <path>]",NULL}, 0,
//    "Start daemon with instance path from SERVALINSTANCE_PATH environment variable."},
//   {app_server_stop,{"stop",NULL},CLIFLAG_PERMISSIVE_CONFIG,
//    "Stop a running daemon with instance path from SERVALINSTANCE_PATH environment variable."},
//   {app_server_status,{"status",NULL},CLIFLAG_PERMISSIVE_CONFIG,
//    "Display information about running daemon."},
  {app_mdp_ping,{"mdp","ping","[--interval=<ms>]","[--timeout=<seconds>]","<SID>|broadcast","[<count>]",NULL}, 0,
   "Attempts to ping specified node via Mesh Datagram Protocol (MDP)."},
  {app_trace,{"mdp","trace","<SID>",NULL}, 0,
   "Trace through the network to the specified node via MDP."},
  {app_config_schema,{"config","schema",NULL},CLIFLAG_PERMISSIVE_CONFIG,
   "Display configuration schema."},
  {app_config_dump,{"config","dump","[--full]",NULL},CLIFLAG_PERMISSIVE_CONFIG,
   "Dump configuration settings."},
  {app_config_set,{"config","set","<variable>","<value>","...",NULL},CLIFLAG_PERMISSIVE_CONFIG,
   "Set and del specified configuration variables."},
  {app_config_set,{"config","del","<variable>","...",NULL},CLIFLAG_PERMISSIVE_CONFIG,
   "Del and set specified configuration variables."},
  {app_config_get,{"config","get","[<variable>]",NULL},CLIFLAG_PERMISSIVE_CONFIG,
   "Get specified configuration variable."},
//   {app_vomp_console,{"console",NULL}, 0,
//     "Test phone call life-cycle from the console"},
  {app_rhizome_append_manifest, {"rhizome", "append", "manifest", "<filepath>", "<manifestpath>", NULL}, 0,
    "Append a manifest to the end of the file it belongs to."},
  {app_rhizome_hash_file,{"rhizome","hash","file","<filepath>",NULL}, 0,
   "Compute the Rhizome hash of a file"},
  {app_rhizome_add_file,{"rhizome","add","file" KEYRING_PIN_OPTIONS,"<author_sid>","<filepath>","[<manifestpath>]","[<bsk>]",NULL}, 0,
   "Add a file to Rhizome and optionally write its manifest to the given path"},
  {app_rhizome_import_bundle,{"rhizome","import","bundle","<filepath>","<manifestpath>",NULL}, 0,
   "Import a payload/manifest pair into Rhizome"},
  {app_rhizome_list,{"rhizome","list" KEYRING_PIN_OPTIONS,
	"[<service>]","[<name>]","[<sender_sid>]","[<recipient_sid>]","[<offset>]","[<limit>]",NULL}, 0,
	"List all manifests and files in Rhizome"},
  {app_rhizome_extract,{"rhizome","export","bundle" KEYRING_PIN_OPTIONS,
	"<manifestid>","[<manifestpath>]","[<filepath>]",NULL}, 0,
	"Export a manifest and payload file to the given paths, without decrypting."},
  {app_rhizome_extract,{"rhizome","export","manifest" KEYRING_PIN_OPTIONS,
	"<manifestid>","[<manifestpath>]",NULL}, 0,
	"Export a manifest from Rhizome and write it to the given path"},
  {app_rhizome_export_file,{"rhizome","export","file","<fileid>","[<filepath>]",NULL}, 0,
	"Export a file from Rhizome and write it to the given path without attempting decryption"},
  {app_rhizome_extract,{"rhizome","extract","bundle" KEYRING_PIN_OPTIONS,
	"<manifestid>","[<manifestpath>]","[<filepath>]","[<bsk>]",NULL}, 0,
	"Extract and decrypt a manifest and file to the given paths."},
  {app_rhizome_extract,{"rhizome","extract","file" KEYRING_PIN_OPTIONS,
	"<manifestid>","[<filepath>]","[<bsk>]",NULL}, 0,
        "Extract and decrypt a file from Rhizome and write it to the given path"},
  {app_rhizome_delete,{"rhizome","delete","manifest|payload|bundle","<manifestid>",NULL}, 0,
	"Remove the manifest, or payload, or both for the given Bundle ID from the Rhizome store"},
  {app_rhizome_delete,{"rhizome","delete","|file","<fileid>",NULL}, 0,
	"Remove the file with the given hash from the Rhizome store"},
  {app_rhizome_direct_sync,{"rhizome","direct","sync","[<url>]",NULL}, 0,
	"Synchronise with the specified Rhizome Direct server. Return when done."},
  {app_rhizome_direct_sync,{"rhizome","direct","push","[<url>]",NULL}, 0,
	"Deliver all new content to the specified Rhizome Direct server. Return when done."},
  {app_rhizome_direct_sync,{"rhizome","direct","pull","[<url>]",NULL}, 0,
	"Fetch all new content from the specified Rhizome Direct server. Return when done."},
  {app_rhizome_clean,{"rhizome","clean","[verify]",NULL}, 0,
	"Remove stale and orphaned content from the Rhizome store"},
  {app_keyring_create,{"keyring","create",NULL}, 0,
   "Create a new keyring file."},
  {app_keyring_dump,{"keyring","dump" KEYRING_PIN_OPTIONS,"[--secret]","[<file>]",NULL}, 0,
   "Dump all keyring identities that can be accessed using the specified PINs"},
  {app_keyring_list,{"keyring","list" KEYRING_PIN_OPTIONS,NULL}, 0,
   "List identities that can be accessed using the supplied PINs"},
  {app_keyring_add,{"keyring","add" KEYRING_PIN_OPTIONS,"[<pin>]",NULL}, 0,
   "Create a new identity in the keyring protected by the supplied PIN (empty PIN if not given)"},
  {app_keyring_set_did,{"keyring", "set","did" KEYRING_PIN_OPTIONS,"<sid>","<did>","<name>",NULL}, 0,
   "Set the DID for the specified SID (must supply PIN to unlock the SID record in the keyring)"},
  {app_id_self,{"id","self|peers|allpeers",NULL}, 0,
   "Return identity(s) as URIs of own node, or of known routable peers, or all known peers"},
  {app_route_print, {"route","print",NULL}, 0,
  "Print the routing table"},
  {app_network_scan, {"scan","[<address>]",NULL}, 0,
    "Scan the network for serval peers. If no argument is supplied, all local addresses will be scanned."},
  {app_count_peers,{"peer","count",NULL}, 0,
    "Return a count of routable peers on the network"},
  {app_dna_lookup,{"dna","lookup","<did>","[<timeout>]",NULL}, 0,
   "Lookup the subscribers (SID) with the supplied telephone number (DID)."},
  {app_reverse_lookup, {"reverse", "lookup", "<sid>", "[<timeout>]", NULL}, 0,
    "Lookup the phone number (DID) and name of a given subscriber (SID)"},
//   {app_monitor_cli,{"monitor",NULL}, 0,
//    "Interactive servald monitor interface."},
  {app_crypt_test,{"test","crypt",NULL}, 0,
   "Run cryptography speed test"},
  {app_nonce_test,{"test","nonce",NULL}, 0,
   "Run nonce generation test"},
//   {app_slip_test,{"test","slip","[--seed=<N>]","[--duration=<seconds>|--iterations=<N>]",NULL}, 0,
//    "Run serial encapsulation test"},
// #ifdef HAVE_VOIPTEST
//   {app_pa_phone,{"phone",NULL}, 0,
//    "Run phone test application"},
// #endif
  {commandline_usage,{"sign","[<SID>]","<message>","[--keyring=<keyring_path>]",NULL},0,
   "Sign any arbitrary text using a Serval ID (if none is given, an identity is created)"},
  {commandline_usage,{"verify","<SID>","<signature>","<message>",NULL},0,
   "Verify a signed message using the given Serval ID"},
  {NULL,{NULL}}
};

static int print_usage(serval_cmd cmd) {
  printf("Serval client\n");
  printf("Usage:\n");
  if (cmd == SERVAL_SIGN || cmd == SERVAL_CRYPTO) {
    printf(" sign [<SID>] <message> [--keyring=<keyring_path>]\n");
    printf("   Sign any arbitrary text using a Serval ID (if none is given, an identity is created)\n");
  } 
  if (cmd == SERVAL_VERIFY || cmd == SERVAL_CRYPTO) {
    printf(" verify <SID> <signature> <message>\n");
    printf("   Verify a signed message using the given Serval ID\n");
  }
  return 1;
}

int main(int argc, char *argv[]) {
  int ret = 1, opt = 0, opt_index = 0, keypath = 0;

  static const char *opt_string = "h";

  static struct option long_opts[] = {
    {"help", no_argument, NULL, 'h'}
  };

  opt = getopt_long(argc, argv, opt_string, long_opts, &opt_index);

  while(opt != -1) {
    switch(opt) {
      case 'h':
      default:
        print_usage(SERVAL_CRYPTO);
        return 0;
        break;
    }
    opt = getopt_long(argc, argv, opt_string, long_opts, &opt_index);
  }

  // Run the Serval command
  
  if (!strcmp(argv[optind],"sign")) {
    if (argc - optind <= 2 || argc - optind >= 4) {
      print_usage(SERVAL_SIGN);
      goto error;
    }
    char sig_buf[2*SIGNATURE_BYTES + 1] = {0};
    if (!strncmp("--keyring=",argv[argc-1],10)) {
      keypath = 1;
      argc--;
    }
    char keyring_path[PATH_MAX] = {0};
    FORM_SERVAL_INSTANCE_PATH(keyring_path, "serval.keyring");
    if (argc - optind == 3) {
      CHECK(cmd_serval_sign(argv[optind+1],
			  strlen(argv[optind+1]),
			  (unsigned char*)argv[optind+2],
			  strlen(argv[optind+2]),
			  sig_buf,
			  2*SIGNATURE_BYTES + 1,
			  keypath ? argv[optind+3] + 10 : keyring_path, // strlen("--length=") == 10
			  keypath ? strlen(argv[optind+3] + 10) : strlen(keyring_path)),"Failed to create signature");
    } else if (argc - optind == 2) {
      CHECK(cmd_serval_sign(NULL,
			0,
			(unsigned char*)argv[optind+1],
			strlen(argv[optind+1]),
			sig_buf,
			2*SIGNATURE_BYTES + 1,
			keypath ? argv[optind+2] + 10 : keyring_path, // strlen("--length=") == 10
			keypath ? strlen(argv[optind+2] + 10) : strlen(keyring_path)),"Failed to create signature");
    }
    printf("%s\n",sig_buf);
    
  } else if (!strcmp(argv[optind],"verify"))  {
    if (argc - optind != 4) {
      print_usage(SERVAL_VERIFY);
      goto error;
    }
    int verdict = serval_verify_client(argv[optind+1],
				strlen(argv[optind+1]),
				(unsigned char*)argv[optind+3],
				strlen(argv[optind+3]),
				argv[optind+2],
				strlen(argv[optind+2]));
    if (verdict == 1)
      printf("Message verified!\n");
    else
      printf("Message NOT verified!\n");
    
  } else {  // Serval built-in commands
    struct cli_parsed parsed;
    int result = cli_parse(argc - optind, (const char*const*)(argv + optind), command_line_options, &parsed);
    switch (result) {
      case 0:
        // Do not run the command if the configuration does not load ok.
        if (((parsed.commands[parsed.cmdi].flags & CLIFLAG_PERMISSIVE_CONFIG) ? cf_reload_permissive() : cf_reload()) != -1) {
	  if (cli_invoke(&parsed, NULL) == 0) {
	    ret = 0;
	  }
        }
        break;
      case 1:
      case 2:
        // Load configuration so that log messages can get out.
        cf_reload_permissive();
        //       NOWHENCE(HINTF("Run \"%s help\" for more information.", argv0 ? argv0 : "servald"));
        break;
      default:
        // Load configuration so that log error messages can get out.
        cf_reload_permissive();
        break;
    }
  }
  
  /* clean up after ourselves */
  overlay_mdp_client_done();
  rhizome_close_db();
  
error:
  return ret;
}
