#include "debug.h"
#include "plugin.h"
#include "list.h"
#include "profile.h"
#include "cmd.h"

#include "demo.h"

int echo_handler(co_obj_t *self, co_obj_t **output, co_obj_t *params)
{
  CHECK(IS_LIST(params) && co_list_length(params) == 1,"Invalid parameters");

  char *data;
  co_obj_t *input;
  co_obj_t *echo;
  size_t size; 
  
  // get list element
  input = co_list_element(params, 0);
  
  // copy input object data
  size = co_obj_data(&data, input);
  
  // create new object with same string
  echo = co_str8_create(data, strlen(data) + 1, 0);
  
  // echo string
  CMD_OUTPUT("result", echo);
  
  return 1;
error:
  return 0;
}

int demo_register(void)
{
  const char name[] = "echo", 
  usage[] = "echo [string to echo]",
  desc[] = "Echo a string.";
  
  CHECK(co_cmd_register(name, sizeof(name), usage, sizeof(usage), desc,sizeof(desc), echo_handler), "Failed to register commands");
  
  return 1;
error:
  return 0;
}

int co_plugin_register(co_obj_t *self, co_obj_t **output, co_obj_t *params) {
  DEBUG("Loading demo schema.");
  SCHEMA_GLOBAL(demo); // adds to global commotion.conf file
  SCHEMA_REGISTER(demo); // adds to profile schema
  return 1;
}

int co_plugin_name(co_obj_t *self, co_obj_t **output, co_obj_t *params) 
{
  const char name[] = "demo";
  CHECK((*output = co_str8_create(name,strlen(name)+1,0)),"Failed to create plugin name");
  return 1;
error:
  return 0;
}

int co_plugin_init(co_obj_t *self, co_obj_t **output, co_obj_t *params)
{
  int ret = 0;
  ret = demo_register();
  return ret;
}