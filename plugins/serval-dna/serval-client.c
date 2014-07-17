/**
 *       @file  serval-client.c
 *      @brief  client for running serval-dna's built-in commands and
 *                signing and verification using Serval keys
 *
 *     @author  Dan Staples (dismantl), danstaples@opentechinstitute.org
 *
 *   @internal
 *     Created  12/18/2013
 *    Compiler  gcc/g++
 *     Company  The Open Technology Institute
 *   Copyright  Copyright (c) 2013, Dan Staples
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
#include <unistd.h>
#include <signal.h>
#include <fcntl.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <sys/stat.h>

#include "serval-config.h"
#include <serval.h>
#include <serval/conf.h>
#include <serval/mdp_client.h>
#include <serval/rhizome.h>
#include <serval/crypto.h>
#include <serval/dataformats.h>
#include <serval/overlay_address.h>

#include "config.h" // COMMOTION_MANAGESOCK
#include "debug.h"
#include "util.h"
#include "socket.h"
#include "obj.h"
#include "tree.h"
#include "commotion.h"

#include "serval-config.h"
#include "crypto.h"
#include "keyring.h"

char bind_uri[PATH_MAX] = {0};

typedef enum {
  SERVAL_SIGN = 0,
  SERVAL_VERIFY = 1,
  SERVAL_CRYPTO = 2
} serval_client_cmd;

#define _DECLARE_SERVAL(F) extern int F(const struct cli_parsed *parsed, struct cli_context *context);
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
  {commandline_usage,{"sign","[<SID>]","<message>","[--bind=<commotiond socket>]","[--keyring=<keyring_path>]",NULL},0,
   "Sign any arbitrary text using a Serval ID (if none is given, an identity is created)"},
  {commandline_usage,{"verify","<SID>","<signature>","<message>",NULL},0,
   "Verify a signed message using the given Serval ID"},
  {NULL,{NULL}}
};

static int print_usage(serval_client_cmd cmd) {
  printf("Serval client\n");
  printf("Usage:\n");
  if (cmd == SERVAL_SIGN || cmd == SERVAL_CRYPTO) {
    printf(" sign [<SID>] <message> [--bind=<commotiond socket>] [--keyring=<keyring_path>]\n");
    printf("   Sign any arbitrary text using a Serval ID (if none is given, an identity is created)\n");
  } 
  if (cmd == SERVAL_VERIFY || cmd == SERVAL_CRYPTO) {
    printf(" verify <SID> <signature> <message> [-k <keyring_path>]\n");
    printf("   Verify a signed message using the given Serval ID\n");
  }
  return 1;
}

static int
keyring_send_sas_request_client(struct subscriber *subscriber)
{
  int ret = 0, mdp_sockfd = -1;
  sid_t srcsid;
  unsigned char *plain = NULL;
  time_ms_t now = gettime_ms();
  
  CHECK((mdp_sockfd = overlay_mdp_client_socket()) >= 0,"Cannot create MDP socket");
  
  CHECK(overlay_mdp_getmyaddr(mdp_sockfd, 0, &srcsid) == 0, "Could not get local address");
  
  if (subscriber->sas_valid)
    return 1;
  
  CHECK(now >= subscriber->sas_last_request + 100, "Too soon to ask for SAS mapping again");
  
  CHECK(my_subscriber, "Couldn't request SAS (I don't know who I am)");
  
  DEBUG("Requesting SAS mapping for SID=%s", alloca_tohex(subscriber->sid.binary, SID_SIZE));
  
  int client_port = 32768 + (random() & 32767);
  CHECK(overlay_mdp_bind(mdp_sockfd, &srcsid, client_port) == 0,
	"Failed to bind to client socket");
  
  /* request mapping (send request auth-crypted). */
  overlay_mdp_frame mdp;
  memset(&mdp,0,sizeof(mdp));  
  
  mdp.packetTypeAndFlags=MDP_TX;
  mdp.out.queue=OQ_MESH_MANAGEMENT;
  memmove(mdp.out.dst.sid.binary,subscriber->sid.binary,SID_SIZE);
  mdp.out.dst.port=MDP_PORT_KEYMAPREQUEST;
  mdp.out.src.port=client_port;
  memmove(mdp.out.src.sid.binary,srcsid.binary,SID_SIZE);
  mdp.out.payload_length=1;
  mdp.out.payload[0]=KEYTYPE_CRYPTOSIGN;
  
  int sent = overlay_mdp_send(mdp_sockfd, &mdp, 0,0);
  if (sent) {
    DEBUG("Failed to send SAS resolution request: %d", sent);
    CHECK(mdp.packetTypeAndFlags != MDP_ERROR,"MDP Server error #%d: '%s'",mdp.error.error,mdp.error.message);
  }
  
  time_ms_t timeout = now + 5000;
  
  while(now < timeout) {
    time_ms_t timeout_ms = timeout - gettime_ms();
    int result = overlay_mdp_client_poll(mdp_sockfd, timeout_ms);
    
    if (result > 0) {
      int ttl = -1;
      if (overlay_mdp_recv(mdp_sockfd, &mdp, client_port, &ttl) == 0) {
	int found = 0;
	switch(mdp.packetTypeAndFlags & MDP_TYPE_MASK) {
	  case MDP_ERROR:
	    ERROR("overlay_mdp_recv: %s (code %d)", mdp.error.message, mdp.error.error);
	    break;
	  case MDP_TX:
	    DEBUG("Received SAS mapping response");
	    found = 1;
	    break;
	  default:
	    DEBUG("overlay_mdp_recv: Unexpected MDP frame type 0x%x", mdp.packetTypeAndFlags);
	    break;
	}
	if (found)
	  break;
      }
    }
    now = gettime_ms();
    if (servalShutdown)
      break;
  }
  
  unsigned keytype = mdp.out.payload[0];
  
  CHECK(keytype == KEYTYPE_CRYPTOSIGN, "Ignoring SID:SAS mapping with unsupported key type %u", keytype);
  
  CHECK(mdp.out.payload_length >= 1 + SAS_SIZE,
	    "Truncated key mapping announcement? payload_length: %d",
	    mdp.out.payload_length);
  
  plain = (unsigned char*)calloc(mdp.out.payload_length,sizeof(unsigned char));
  CHECK_MEM(plain);
  unsigned long long plain_len = 0;
  unsigned char *sas_public = &mdp.out.payload[1];
  unsigned char *compactsignature = &mdp.out.payload[SAS_SIZE + 1];
  
  /* reconstitute signed SID for verification */
  unsigned char signature[SID_SIZE + crypto_sign_edwards25519sha512batch_BYTES];
  memmove(&signature[0], &compactsignature[0], 64);
  memmove(&signature[64], &mdp.out.src.sid.binary[0], SID_SIZE);
  
  int sign_ret = crypto_sign_edwards25519sha512batch_open(plain,
							  &plain_len,
							  signature,
							  SID_SIZE + crypto_sign_edwards25519sha512batch_BYTES,
							  sas_public);
  CHECK(sign_ret == 0, "SID:SAS mapping verification signature does not verify");
  
  /* These next two tests should never be able to fail, but let's just check anyway. */
  CHECK(plain_len == SID_SIZE, "SID:SAS mapping signed block is wrong length");
  CHECK(memcmp(plain, mdp.out.src.sid.binary, SID_SIZE) == 0, "SID:SAS mapping signed block is for wrong SID");
  
  memmove(subscriber->sas_public, sas_public, SAS_SIZE);
  subscriber->sas_valid = 1;
  subscriber->sas_last_request = now;
  ret = 1;
  
error:
  if (plain)
    free(plain);
  if (mdp_sockfd != -1)
    overlay_mdp_client_close(mdp_sockfd);
  return ret;
}

static int
serval_sign_client(svl_crypto_ctx *ctx)
{
  int ret = 0;
  co_obj_t *co_conn = NULL, *co_req = NULL, *co_resp = NULL;
  
  CHECK(ctx && ctx->msg && ctx->keyring_path && ctx->keyring_len, "Invalid ctx");

  co_conn = co_connect(bind_uri,strlen(bind_uri)+1);
  CHECK(co_conn, "Failed to connect to commotiond management socket");
  
  CHECK_MEM((co_req = co_request_create()));
  CHECK(co_request_append_str(co_req,"sign",sizeof("sign")),"Failed to append to request");
  if (!iszero(ctx->sid,SID_SIZE))
    CHECK(co_request_append_str(co_req,
				alloca_tohex(ctx->sid, SID_SIZE),
				2 * SID_SIZE + 1),
	  "Failed to append to request");
  CHECK(co_request_append_str(co_req,(char*)ctx->msg,ctx->msg_len + 1),"Failed to append to request");
  char keyring_path[PATH_MAX] = {0};
  strcpy(keyring_path, "keyring=");
  strncat(keyring_path,ctx->keyring_path,ctx->keyring_len);
  CHECK(co_request_append_str(co_req, keyring_path, strlen(keyring_path) + 1),"Failed to append to request");
  
  CHECK(co_call(co_conn,&co_resp,"serval-crypto",sizeof("serval-crypto"),co_req),
	"Failed to dispatch signing request");
  
  co_tree_print(co_resp);
  
  ret = 1;
error:
  if (co_req)
    co_free(co_req);
  if (co_resp)
    co_free(co_resp);
  if (co_conn)
    co_disconnect(co_conn);
  return ret;
}

static int
serval_verify_client(svl_crypto_ctx *ctx)
{
  CHECK(ctx
	&& !iszero(ctx->sid,SID_SIZE)
	&& ctx->msg
	&& !iszero(ctx->signature,SIGNATURE_BYTES)
	&& ctx->keyring,
	"Invalid ctx");
  
  keyring = serval_get_keyring_file(ctx);
      
  struct subscriber *sub = find_subscriber(ctx->sid, SID_SIZE, 1); // get Serval identity described by given SID
  
  CHECK(sub, "Failed to fetch Serval subscriber");
  
  CHECK(keyring_send_sas_request_client(sub), "SAS request failed");
  
  CHECK(sub->sas_valid, "Could not validate the signing key!");
  CHECK(!iszero(sub->sas_public,crypto_sign_PUBLICKEYBYTES), "Could not validate the signing key!");
  
  memcpy(ctx->sas_public,sub->sas_public,crypto_sign_PUBLICKEYBYTES);
  
  return cmd_serval_verify(ctx);
error:
  return 0;
}

static int serval_cmd(int argc, char *argv[]) {
  struct cli_parsed parsed;
  int result = cli_parse(argc, (const char*const*)argv, command_line_options, &parsed);
  switch (result) {
    case 0:
      // Do not run the command if the configuration does not load ok.
      if (((parsed.commands[parsed.cmdi].flags & CLIFLAG_PERMISSIVE_CONFIG) ? cf_reload_permissive() : cf_reload()) != -1) {
	if (cli_invoke(&parsed, NULL) == 0) {
	  return 0;
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
  return 1;
}

int main(int argc, char *argv[]) {
  int ret = 1;
  char *instance_path = NULL;
  svl_crypto_ctx *ctx = svl_crypto_ctx_new();
  CHECK_MEM(ctx);
  
  CHECK(co_init(), "Failed to initialize");
  
  // Run the Serval command
  
  // TODO only open keyring for sign, or make verify a commotion client request
  if (!strncmp("--keyring=",argv[argc-1],10)) {
    CHECK(strlen(argv[argc-1] + 10) < PATH_MAX,"keyring path too long");
    ctx->keyring_path = argv[argc-1] + 10;
    argc--;
  } else {
    // add default path to ctx->keyring_path
    ctx->keyring_path = h_malloc(strlen(DEFAULT_SERVAL_PATH) + strlen("/serval.keyring"));
    CHECK_MEM(ctx->keyring_path);
    hattach(ctx->keyring_path,ctx);
    strcpy(ctx->keyring_path,DEFAULT_SERVAL_PATH);
    strcat(ctx->keyring_path,"/serval.keyring");
  }
  ctx->keyring_len = strlen(ctx->keyring_path);
  CHECK(serval_open_keyring(ctx, NULL),
	"Failed to open Serval keyring");
  
  argc--; argv++;
  if (!argc) {
    char *help = "help";
    ret = serval_cmd(1, &help);
  } else if (!strcmp(argv[0],"sign")) {
    // check if bind option given
    if (!strncmp("--bind=",argv[argc-1],7)) {
      CHECK(strlen(argv[argc-1] + 7) < PATH_MAX,"bind URI path too long");
      strcpy(bind_uri,argv[argc-1] + 7);
      argc--;
    } else {
      strcpy(bind_uri,COMMOTION_MANAGESOCK);
    }
    
    if (argc < 2 || argc > 3) {
      print_usage(SERVAL_SIGN);
      goto error;
    }

    if (argc == 3) {
      char *sid_str = argv[1];
      CHECK(strlen(sid_str) == (2 * SID_SIZE) && str_is_subscriber_id(sid_str) == 1,
		"Invalid SID");
      stowSid(ctx->sid, 0, sid_str);
      ctx->msg = (unsigned char*)argv[2];
      ctx->msg_len = strlen(argv[2]);
    } else if (argc == 2) {
      ctx->msg = (unsigned char*)argv[1];
      ctx->msg_len = strlen(argv[1]);
    }
    
    CHECK(serval_sign_client(ctx), "Failed to create signature");
    
  } else if (!strcmp(argv[0],"verify"))  {
    if (argc != 4) {
      print_usage(SERVAL_VERIFY);
      goto error;
    }
    
    // Set SERVALINSTANCE_PATH environment variable
    char *last_slash = strrchr(ctx->keyring_path,(int)'/');
    instance_path = calloc(last_slash - ctx->keyring_path + 1,sizeof(char));
    CHECK_MEM(instance_path);
    strncpy(instance_path,ctx->keyring_path,last_slash - ctx->keyring_path);
    CHECK(setenv("SERVALINSTANCE_PATH", instance_path, 1) == 0,
	      "Failed to set SERVALINSTANCE_PATH env variable");
    
    char *sid_str = argv[1];
    CHECK(strlen(sid_str) == (2 * SID_SIZE) && str_is_subscriber_id(sid_str) == 1,
	      "Invalid SID");
    stowSid(ctx->sid, 0, sid_str);
    CHECK(fromhexstr(ctx->signature, argv[2], SIGNATURE_BYTES) == 0, "Invalid signature");
    ctx->msg = (unsigned char*)argv[3];
    ctx->msg_len = strlen(argv[3]);
    int verdict = serval_verify_client(ctx);
    if (verdict == 1)
      printf("Message verified!\n");
    else
      printf("Message NOT verified!\n");
    
  } else {  // Serval built-in commands
    ret = serval_cmd(argc, argv);
  }
  
  /* clean up after ourselves */
  rhizome_close_db();
  
  ret = 0;
  
error:
  co_shutdown();
  
  if (instance_path)
    free(instance_path);
  
  if (ctx)
    svl_crypto_ctx_free(ctx);
  
  return ret;
}
