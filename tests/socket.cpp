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
  
  void Send();
  
  co_socket_t *socket1;
  
  SocketTest()
  {
    socket1 = (co_socket_t*)co_socket_create(sizeof(co_socket_t), unix_socket_proto);
    // socket1->register_cb = co_loop_add_socket;
    socket1->bind((co_obj_t*)socket1, "commotiontest2.sock");
  }
  
  virtual void SetUp()
  {
  }
  
  ~SocketTest()
  {
    co_socket_destroy((co_obj_t *)socket1);
  }
  

};
  
void SocketTest::Send()
{
  
  ASSERT_EQ(1, 1);
}

TEST_F(SocketTest, Send)
{
  Send();
}