#ifndef __CO_SERVAL_DNA_H
#define __CO_SERVAL_DNA_H

#define PATH_MAX 4096

#define CHECK_ERR(A, M, ...) if(!(A)) { \
    ERROR(M, ##__VA_ARGS__); \
    if (err_msg == NULL) \
      err_msg = co_list16_create(); \
    char *msg = NULL; \
    int len = snprintf(NULL, 0, M, ##__VA_ARGS__); \
    msg = calloc(len,sizeof(char)); \
    sprintf(msg, M, ##__VA_ARGS__); \
    if (len < UINT8_MAX) { \
      co_list_append(err_msg, co_str8_create(msg,len+1,0)); \
    } else if (len < UINT16_MAX) { \
      co_list_append(err_msg, co_str16_create(msg,len+1,0)); \
    } else if (len < UINT32_MAX) { \
      co_list_append(err_msg, co_str32_create(msg,len+1,0)); \
    } \
    free(msg); \
    goto error; \
  }

/** until we have nested tree support */
#undef CHECK_ERR
#define CHECK_ERR(A, M, ...) if(!(A)) { \
    ERROR(M, ##__VA_ARGS__); \
    if (err_msg) \
      co_obj_free(err_msg); \
    char *msg = NULL; \
    int len = snprintf(NULL, 0, M, ##__VA_ARGS__); \
    msg = calloc(len,sizeof(char)); \
    sprintf(msg, M, ##__VA_ARGS__); \
    if (len < UINT8_MAX) { \
      err_msg = co_str8_create(msg,len+1,0); \
    } else if (len < UINT16_MAX) { \
      err_msg = co_str16_create(msg,len+1,0); \
    } else if (len < UINT32_MAX) { \
      err_msg = co_str32_create(msg,len+1,0); \
    } \
    free(msg); \
    goto error; \
  }

#define CLEAR_ERR() if (err_msg) { co_obj_free(err_msg); err_msg = NULL; }

#define INS_ERROR() if (err_msg) { CMD_OUTPUT("errors",err_msg); }

int co_plugin_register(co_obj_t *self, co_obj_t **output, co_obj_t *params);
int co_plugin_init(co_obj_t *self, co_obj_t **output, co_obj_t *params);
int co_plugin_name(co_obj_t *self, co_obj_t **output, co_obj_t *params);
int co_plugin_shutdown(co_obj_t *self, co_obj_t **output, co_obj_t *params);
int serval_socket_cb(co_obj_t *self, co_obj_t *context);
int serval_timer_cb(co_obj_t *self, co_obj_t **output, co_obj_t *context);
int serval_schema(co_obj_t *self, co_obj_t **output, co_obj_t *params);
int serval_daemon_handler(co_obj_t *self, co_obj_t **output, co_obj_t *params);

#endif