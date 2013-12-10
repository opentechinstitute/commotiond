#ifndef SERVAL_CRYPTO_H
#define SERVAL_CRYPTO_H

#include "obj.h"

#define KEYRING_PIN NULL
#define BUF_SIZE 1024

int serval_verify(const char *sid,
		  const size_t sid_len,
		  const unsigned char *msg,
		  const size_t msg_len,
		  const char *sig,
		  const size_t sig_len,
		  const char *keyringName,
		  const size_t keyring_len);

int serval_sign(const char *sid, 
		const size_t sid_len,
		const unsigned char *msg,
		const size_t msg_len,
		char *sig_buffer,
		const size_t sig_size,
		const char *keyringName,
		const size_t keyring_len);

int serval_crypto_register(void);

co_obj_t *serval_crypto_handler(co_obj_t *self, co_obj_t *params);

#endif