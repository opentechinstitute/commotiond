#include <stdlib.h>
#include <stdio.h>

#include "serval.h"
#include "conf.h"
#include "mdp_client.h"
#include "rhizome.h"
#include "crypto.h"

#include "client.h"
#include "obj.h"
#include "list.h"
#include "cmd.h"
#include "tree.h"
#include "debug.h"

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
  {NULL,{NULL}}
};

struct arguments {
  int argc;
  int i;
  char **argv;
};

static co_obj_t *_serialize_params_i(co_obj_t *list, co_obj_t *param, void *args) {
  if(!IS_STR(param)) return NULL;
  struct arguments *these_args = (struct arguments*)args;
  CHECK(these_args->i < these_args->argc,"Out of bounds");
  CHECK(co_obj_data(&these_args->argv[these_args->i], param) == sizeof(co_str8_t),"Error fetching param");
  these_args->i++;
error:
  return NULL;
}

int serval_handler(co_obj_t *self, co_obj_t **output, co_obj_t *params) {
  struct cli_parsed parsed;
  char *retbuf = NULL;
  unsigned long len = 0;
  int ret = 0;
  
  // parse params into args, argc
  if (!IS_LIST(params)) {
    ERROR("Invalid params");
    return 0;
  }
  int argc = co_list_length(params);
  char *argv[argc];
  struct arguments args = {
    .argc = argc,
    .i = 0,
    .argv = argv
  };
  co_list_parse(params, _serialize_params_i, &args);
  
  // Capture stdout to send back to client
  int stdout_pipe[2], old_stdout = dup(1);
  fflush(stdout);
  pipe(stdout_pipe);
  fcntl(stdout_pipe[0], F_SETFL, O_NONBLOCK);
  dup2(stdout_pipe[1], STDOUT_FILENO);   /* redirect stdout to the pipe */
  close(stdout_pipe[1]);
    
  // Run the Serval command
    
  int result = cli_parse(args.argc, (const char*const*)args.argv, command_line_options, &parsed);
  switch (result) {
    case 0:
      // Do not run the command if the configuration does not load ok.
      if (((parsed.commands[parsed.cmdi].flags & CLIFLAG_PERMISSIVE_CONFIG) ? cf_reload_permissive() : cf_reload()) != -1) {
        if (cli_invoke(&parsed, NULL) == 0)
	  ret = 1;
      } else
        ret = -1;
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
  
  /* clean up after ourselves */
  overlay_mdp_client_done();
  rhizome_close_db();

  // Capture and send response back to client
  fflush(stdout);
  char readbuf[BUF_SIZE] = {0};
  int n = 0;
  while ((n = read(stdout_pipe[0], readbuf, BUF_SIZE-1)) > 0) {
    retbuf = realloc(retbuf,len + n + 1);
    strncpy(retbuf + len,readbuf,n);
    len += n;
    retbuf[len] = '\0';
  }
    
  dup2(old_stdout, STDOUT_FILENO);  /* reconnect stdout for testing */

  if (len < UINT8_MAX) {
    CHECK(co_tree_insert(*output,"result",6,co_str8_create(retbuf,len+1,0)),"Failed to set return value");
  } else if (len < UINT16_MAX) {
    CHECK(co_tree_insert(*output,"result",6,co_str16_create(retbuf,len+1,0)),"Failed to set return value");
  } else if (len < UINT32_MAX) {
    CHECK(co_tree_insert(*output,"result",6,co_str32_create(retbuf,len+1,0)),"Failed to set return value");
  }
  
  if (ret == -1) 
    ret = 0;
  else
    ret = 1;
error:
  if (retbuf) free(retbuf);
  return ret;
}

int serval_register(void) {
  const char name[] = "serval",
               usage[] = "run \"help\" for usage information",
	       desc[] = "Serval DNA";
  CHECK(co_cmd_register(name,strlen(name),usage,strlen(usage),desc,strlen(desc),serval_handler),"Failed to register commands");
  
  return 1;
error:
  return 0;
}
