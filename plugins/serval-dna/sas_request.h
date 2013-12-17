#include <serval.h>

#ifndef __CO_SERVAL_SAS_REQUEST_H
#define __CO_SERVAL_SAS_REQUEST_H

#define SID_SIZE 32
#define SAS_SIZE 32

int keyring_send_sas_request_client(const char *sid_str, 
				    const size_t sid_len,
				    char *sas_buf,
				    const size_t sas_buf_len);

#endif