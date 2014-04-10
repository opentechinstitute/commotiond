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
  char *method = "help";
  size_t mlen = strlen(method) + 1;
  co_obj_t *m = co_str8_create(method, mlen, 0);
  size_t reqlen = 0;
  size_t importlen = 0;
  co_obj_t *param = co_list16_create();
  co_obj_t *request = NULL;
  uint8_t *type = NULL;
  uint32_t *id = NULL;
  
  // functions
  void Request();
  
  MessageTest()
  {
  }
  
  ~MessageTest()
  {
  }
};


void MessageTest::Request()
{
  // pack message
  reqlen = co_request_alloc(req, REQUEST_MAX, m, param);
  ASSERT_EQ(20, reqlen);
  
  DEBUG("\n\nNumber of bytes packed: %d\n\n", reqlen);
  
  // unpack message
  importlen = co_list_import(&request, req, reqlen);
  
  DEBUG("\n\nNumber of byes unpacked: %d\n\n", importlen);
  
  // check message contents
  co_obj_data((char **)&type, co_list_element(request, 0));
  ASSERT_EQ(0, *type);
  
  ASSERT_EQ(sizeof(uint32_t), co_obj_data((char **)&id, co_list_element(request, 1)));
}

TEST_F (MessageTest, Request)
{
  Request();
}