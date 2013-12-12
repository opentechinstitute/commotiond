#ifndef SERVAL_CRYPTO_H
#define SERVAL_CRYPTO_H

#include "obj.h"

#define KEYRING_PIN NULL
#define BUF_SIZE 1024

int serval_crypto_register(void);

int olsrd_mdp_register(void);

int olsrd_mdp_sign_register(void);

int serval_crypto_handler(co_obj_t *self, co_obj_t **output, co_obj_t *params);

int olsrd_mdp_init(co_obj_t *self, co_obj_t **output, co_obj_t *params);

int olsrd_mdp_sign(co_obj_t *self, co_obj_t **output, co_obj_t *params);

int _serval_init(unsigned char *sid,
		 const size_t sid_len,
		 const char *keyring_path,
		 const size_t keyring_len,
		 keyring_file **_keyring,
		 unsigned char **key,
		 int *key_len);

#endif