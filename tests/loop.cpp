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

bool timer1_success = false;
bool timer2_success = false;
bool timer3_success = false;

int timer1_cb(co_obj_t *self, co_obj_t **output, co_obj_t *params);
int timer2_cb(co_obj_t *self, co_obj_t **output, co_obj_t *params);
int timer3_cb(co_obj_t *self, co_obj_t **output, co_obj_t *params);
int loop_stop_cb(co_obj_t *self, co_obj_t **output, co_obj_t *params);

class LoopTest : public ::testing::Test
{
  protected:
    int ret;
    
    // context pointers
    co_obj_t *context1;
    co_obj_t *context2;
    co_obj_t *context3;
    co_obj_t *context4;
    co_obj_t *stop;
    
    // time values
    struct timeval timeval1;
    struct timeval timeval2;
    struct timeval timeval3;
    struct timeval tv_stop;
    
    
    // tests
    void Timer();
    void Socket();
    
    LoopTest()
    {
      ret = co_loop_create();
      // CHECK((poll_fd = epoll_create1(EPOLL_CLOEXEC)) != -1, "Failed to create epoll event.");
    }
    
  virtual void SetUp()
  {
  }
  
  ~LoopTest()
  {
    ret = co_loop_destroy();
  }
};

void LoopTest::Timer()
{
  // intialize timer1
  co_obj_t *timer1 = co_timer_create(timeval1, timer1_cb, context1);
  ret = co_loop_set_timer(timer1, 1, context1);
  ASSERT_EQ(1, ret);
  
  // initialize timer2
  co_obj_t *timer2 = co_timer_create(timeval2, timer2_cb, context2);
  ret = co_loop_set_timer(timer2, 1, context1);
  ASSERT_EQ(1, ret);
  
  // initialize timer3
  co_obj_t *timer3 = co_timer_create(timeval3, timer3_cb, context3);
  ret = co_loop_set_timer(timer3, 1, context1);
  ASSERT_EQ(1, ret);
  
  // stops the loop (after 100 ms)
  co_obj_t *loop_stop = co_timer_create(tv_stop, loop_stop_cb, stop);
  ret = co_loop_set_timer(loop_stop, 100, stop);
  ASSERT_EQ(1, ret);
  
  co_loop_start();
  
  EXPECT_TRUE(timer1_success);
  EXPECT_TRUE(timer2_success);
  EXPECT_TRUE(timer3_success);
}

void LoopTest::Socket()
{
  // NOTE: co_socket_init is not used right now
  // NOTE: co_socket_destroy causes a crash
  
  // initialize socket and register it with the event loop
  co_socket_t *socket1 = (co_socket_t*)co_socket_create(sizeof(co_socket_t), unix_socket_proto);
  socket1->register_cb = co_loop_add_socket;
  socket1->bind((co_obj_t*)socket1, "commotiontest.sock");
  
//   ret = co_loop_add_socket((co_obj_t *)socket1, NULL);
//   ASSERT_EQ(1, ret);
  
  //ret = co_socket_destroy((co_obj_t *)socket1);
  //ASSERT_EQ(1, ret);
  
//   ret = co_loop_remove_socket((co_obj_t *)socket1, NULL);
//   ASSERT_EQ(1, ret);
}

TEST_F(LoopTest, Timer)
{
  Timer();
}

TEST_F(LoopTest, Socket)
{
  Socket();
}
  
  
// callback functions
  int timer1_cb(co_obj_t *self, co_obj_t **output, co_obj_t *params)
{
  timer1_success = true;
  return 1;
}

int timer2_cb(co_obj_t *self, co_obj_t **output, co_obj_t *params)
{
  timer2_success = true;
  return 1;
}

int timer3_cb(co_obj_t *self, co_obj_t **output, co_obj_t *params)
{
  timer3_success = true;
  return 1;
}

// loop stop function (currently unused)
int loop_stop_cb(co_obj_t *self, co_obj_t **output, co_obj_t *params)
{
  co_loop_stop(); // breaks loop without returning
  return 1;
}