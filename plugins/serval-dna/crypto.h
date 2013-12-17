#ifndef __CO_SERVAL_CRYPTO_H
#define __CO_SERVAL_CRYPTO_H

#include <serval.h>

#include "obj.h"

#define KEYRING_PIN NULL
#define BUF_SIZE 1024

int serval_crypto_register(void);

int olsrd_mdp_register(void);

int olsrd_mdp_sign_register(void);

int serval_crypto_handler(co_obj_t *self, co_obj_t **output, co_obj_t *params);

int olsrd_mdp_init(co_obj_t *self, co_obj_t **output, co_obj_t *params);

int olsrd_mdp_sign(co_obj_t *self, co_obj_t **output, co_obj_t *params);

int serval_open_keyring(const char *keyring_path,
			const size_t keyring_len,
			keyring_file *_keyring);

int serval_init_keyring(unsigned char *sid,
		 const size_t sid_len,
		 const char *keyring_path,
		 const size_t keyring_len,
		 keyring_file *_keyring,
		 unsigned char **key,
		 int *key_len);

int cmd_serval_sign(const char *sid_str, 
		    const size_t sid_len,
		    const unsigned char *msg,
		    const size_t msg_len,
		    char *sig_str_buf,
		    const size_t sig_str_size,
		    const char *keyring_path,
		    const size_t keyring_len);

int cmd_serval_verify(const char *sas_key,
		   const size_t sas_key_len,
		   const unsigned char *msg,
		   const size_t msg_len,
		   const char *sig,
		   const size_t sig_len);

int serval_verify_client(const char *sid_str,
		  const size_t sid_len,
		  const unsigned char *msg,
		  const size_t msg_len,
		  const char *sig,
		  const size_t sig_len);



#endif