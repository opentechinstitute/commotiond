/**
 * POA
 * 
 * OKAY, so "method" is actually just the name of the command you want to execute.
 * 
 * For example, I could send "help" as the method and in theory get back the
 * standard text from the help command
 * 
 * So what request does is pack a message buffer with the relevant information
 * for calling a function or command
 * This buffer is then unpacked on the other side
 * 
 * Likewise, response will send a response
 * 
 * Unless I want to fiddle around with setting up sockets and the like,
 * the best option seems to be packing a message buffer, importing it
 * as an object, then checking its members (it will be a list with 
 * a tree attached to it(for the params)) and seeing if they are 
 * correct.
 * --> check the list elements with this function:
 * co_obj_t *co_list_element(co_obj_t *list, const unsigned int index);
 * 
 * 
 * I could test it by packing up the buffer
 * 
 * co_object_import the buffer as a commotion object
 * --> this will be a list with certain elements with certain values
 * --> at the end will be a tree with the parameters I issued to the method
 * 
 * 
 * look up messagepack rpc to see how the messages are formatted
 * 
 * 
 * /**
 * @brief allocate request
 * @param output buffer for output
 * @param olen output buffer length
 * @param method name of method
 * @param param parameters to method
 * size_t co_request_alloc(char *output, const size_t olen, const co_obj_t *method, co_obj_t *param);
 * 
 */

extern "C" {
#include "../src/obj.h"
#include "../src/list.h"
#include "../src/tree.h"
#include "../src/msg.h"
#include "../src/debug.h"
}
#include "gtest/gtest.h"


static uint32_t _id = 0;

static struct 
{
  uint8_t list_type;
  uint16_t list_len;
  uint8_t type_type;
  uint8_t type_value;
  uint8_t id_type;
} _req_header = {
  .list_type = _list16,
  .list_len = 4,
  .type_type = _uint8,
  .type_value = 0,
  .id_type = _uint32
};

#define REQUEST_MAX 4096
#define RESPONSE_MAX 4096

class MessageTest : public ::testing::Test
{
protected:
  // variables
  int ret = 0;
  size_t len = 0;
  char req[REQUEST_MAX];
  char resp[RESPONSE_MAX];

  
  // functions
  void Request();
  
//   void Response();
  
  MessageTest()
  {
  }
  
  ~MessageTest()
  {
  }
};


void MessageTest::Request()
{
    
  char *method = "help";
  size_t mlen = strlen(method) + 1;
  co_obj_t *m = co_str8_create(method, mlen, 0);
  size_t reqlen = 0;
  size_t importlen = 0;
  co_obj_t *param = co_list16_create();
  
  reqlen = co_request_alloc(req, REQUEST_MAX, m, param);
  ASSERT_EQ(20, reqlen);
  
  DEBUG("\n\nThe reqlen value is: %d\n\n", reqlen);
  
  co_obj_t *request = NULL;
      
  uint8_t *type = NULL;
  uint32_t *id = NULL;
  
  importlen = co_list_import(&request, req, reqlen);
  
  DEBUG("\n\nThe importlen value is: %d\n\n", importlen);
  
  co_obj_data((char **)&type, co_list_element(request, 0));
  ASSERT_EQ(0, *type);
  
  ASSERT_EQ(sizeof(uint32_t), co_obj_data((char **)&id, co_list_element(request, 1)));
  
  
  
  return; 
  
error:
  DEBUG("\n\nSomething went wrong.\n\n");
  return;
  /*
  importlen = co_list_import(imported, req, reqlen); 
  ASSERT_EQ(importlen, reqlen);
  */
  
  // now convert the buffer into an object (by importing it?)
  // import it
  // check the elements of the list are correct
  // add parameters
  // do it again and see if that works
}

TEST_F (MessageTest, Request)
{
  Request();
}
  
/*
TEST_F (MessageTest, Response)
{
  Response();
}

*/

