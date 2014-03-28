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


/*
 * POA
 * * List socket functions that need testing @
 * * Come up with general order of testing @ 
 * * Determine set up variables and functions
 * * Determine tear down process
 * 
 * 
 * Functions to test:
 * co_obj_t *co_socket_create(size_t size, co_socket_t proto);
 * int co_socket_send(co_obj_t *self, char *outgoing, size_t length);
 * int co_socket_receive(co_obj_t * self, co_obj_t *fd, char *incoming, size_t length);
 * int co_socket_hangup(co_obj_t *self, co_obj_t *context); 
 * int co_socket_destroy(co_obj_t *self);
 * 
 * 
 */


class SocketTest : public ::testing::Test
{
protected:
  int ret;
  
  #define MAX_MESSAGE 100
  
  char message1[MAX_MESSAGE];
  
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
  
void SocketTest::SendReceive()
{
  char buffer[14];
  int length = 14;
  int received = 0;
  
  char *test_message = "test message";
  
  ret = co_socket_send((co_obj_t *)socket2->fd, "test message", sizeof("test message"));
  ASSERT_EQ(sizeof("test message"), ret);

  received = co_socket_receive((co_obj_t *)socket1, (co_obj_t *)socket1->fd, buffer, length);
  
  received = co_socket_receive((co_obj_t *)socket1, (co_obj_t *)co_list_get_first(socket1->rfd_lst), buffer, length);
  
  DEBUG("\n\nSent %d bytes.\n", ret);
  DEBUG("\n\nReceived %d bytes.\n", received);
  
  ASSERT_STREQ(test_message, buffer);
  
  co_socket_hangup
}

TEST_F(SocketTest, SendReceive)
{
  SendReceive();
}