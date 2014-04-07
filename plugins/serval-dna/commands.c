/**
 *       @file  commands.c
 *      @brief  command registration
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

#include "serval-config.h"
#include <serval.h>

#include "cmd.h"

#include "serval-dna.h"
#include "crypto.h"
#include "olsr_mdp.h"
#include "commands.h"

/* used to store error messages to pass in response to client commands */
co_obj_t *err_msg = NULL;
int collect_errors = 0;

int
serval_daemon_register(void)
{
  /**
   * name: serval-daemon
   * param[0] <required>: start|stop|reload (co_str8_t)
   */
  
  const char name[] = "serval-daemon",
//   usage[] = "serval-daemon start|stop|reload",
//   desc[] =  "Start or stop the Serval daemon, or reload the keyring file";
  usage[] = "serval-daemon reload",
  desc[] =  "Reload the keyring file";
  
  CHECK(co_cmd_register(name,sizeof(name),usage,sizeof(usage),desc,sizeof(desc),serval_daemon_handler),"Failed to register commands");
  
  return 1;
error:
  return 0;
}

int
serval_crypto_register(void)
{
  /** name: serval-crypto
   * param[0] - param[3]: (co_str?_t)
   */
  
  const char name[] = "serval-crypto",
  usage[] = "serval-crypto sign [<SID>] <MESSAGE> [--keyring=<KEYRING_PATH>]\n"
            "serval-crypto verify <SAS> <SIGNATURE> <MESSAGE>",
  desc[] =  "Serval-crypto utilizes Serval's crypto API to:\n"
	    "      * Sign any arbitrary text using a Serval key. If no Serval key ID (SID) is given,\n"
	    "             a new key will be created on the default Serval keyring.\n"
	    "      * Verify any arbitrary text, a signature, and a Serval signing key (SAS), and will\n"
	    "             determine if the signature is valid.";
  
  int reg_ret = co_cmd_register(name,
				sizeof(name),
				usage,
				sizeof(usage),
				desc,
				sizeof(desc),
				serval_crypto_handler);
  CHECK(reg_ret, "Failed to register commands");
  
  return 1;
error:
  return 0;
}

int
olsrd_mdp_register(void)
{
  /**
   * name: mdp-init
   * param[0] <required>: <keyring_path> (co_str16_t)
   * param[1] <required>: <SID> (co_str8_t)
   */
  CHECK(co_cmd_register("mdp-init", sizeof("mdp-init"), "", 1, "", 1, olsrd_mdp_init), "Failed to register mdp-init command");
  
  /**
   * name: mdp-sign
   * param[0] <required>: key (co_bin8_t)
   * param[1] <required>: data (co_bin?_t)
   */
  CHECK(co_cmd_register("mdp-sign", sizeof("mdp-sign"), "", 1, "", 1, olsrd_mdp_sign), "Failed to register mdp-sign command");
  
  return 1;
error:
  return 0;
}