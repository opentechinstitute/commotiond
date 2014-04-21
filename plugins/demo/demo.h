SCHEMA(demo) {
  SCHEMA_ADD("demo","enabled");
  return 1;
}

int echo_handler(co_obj_t *self, co_obj_t **output, co_obj_t *params);

int demo_register(void);

int co_plugin_register(co_obj_t *self, co_obj_t **output, co_obj_t *params);

int co_plugin_name(co_obj_t *self, co_obj_t **output, co_obj_t *params);

int co_plugin_init(co_obj_t *self, co_obj_t **output, co_obj_t *params);