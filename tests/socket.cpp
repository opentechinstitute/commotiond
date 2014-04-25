#include <stdlib.h>
#include <stdio.h>
#include <getopt.h>
#include <unistd.h>
#include <signal.h>
#include <fcntl.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <sys/stat.h>
extern "C" {
  #include "config.h"
  #include "debug.h"
  #include "util.h"
  #include "loop.h"
  #include "process.h"
  #include "profile.h"
  #include "socket.h"
  #include "msg.h"
  #include "iface.h"
  #include "id.h"
  #include "list.h"
}
#include "gtest/gtest.h"

extern co_socket_t unix_socket_proto;

class SocketTest : public ::testing::Test
{
protected:
  int ret;
  
  void Create();
  void SendReceive();
  
  co_socket_t *socket1;
  co_socket_t *socket2;
  
  SocketTest()
  {
    socket1 = (co_socket_t*)co_socket_create(sizeof(co_socket_t), unix_socket_proto);
    // socket1->register_cb = co_loop_add_socket;
    socket1->bind((co_obj_t*)socket1, "commotiontest.sock");
    
    socket2 = (co_socket_t*)co_socket_create(sizeof(co_socket_t), unix_socket_proto);
    socket2->connect((co_obj_t*)socket2, "commotiontest.sock");
    // socket1->register_cb = co_loop_add_socket;
    // socket2->bind((co_obj_t*)socket2, "commotiontest.sock");
  }
  
  virtual void SetUp()
  {
  }
  
  ~SocketTest()
  {
    co_socket_destroy((co_obj_t *)socket1);
  }
};

void SocketTest::Create()
{
  ASSERT_TRUE(socket1);
  ASSERT_TRUE(socket2);
}
  
void SocketTest::SendReceive()
{
  char buffer[13];
  int length = 13;
  int received = 0;
  
  char test_message[13] = "test message";
  
  ret = co_socket_send((co_obj_t *)socket2->fd, test_message, sizeof(test_message));
  ASSERT_EQ(sizeof(test_message), ret);

  received = co_socket_receive((co_obj_t *)socket1, (co_obj_t *)socket1->fd, buffer, length);
  
  received = co_socket_receive((co_obj_t *)socket1, (co_obj_t *)co_list_get_first(socket1->rfd_lst), buffer, length);
  
  DEBUG("\n\nSent %d bytes.\n", ret);
  DEBUG("\n\nReceived %d bytes.\n", received);
  
  ASSERT_STREQ(test_message, buffer);
}

TEST_F(SocketTest, Create)
{
  Create();
}

TEST_F(SocketTest, SendReceive)
{
  SendReceive();
}