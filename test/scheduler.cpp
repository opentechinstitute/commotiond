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
  #include "command.h"
  #include "util.h"
  #include "loop.h"
  #include "process.h"
  #include "profile.h"
  #include "socket.h"
  #include "msg.h"
  #include "olsrd.h"
  #include "iface.h"
  #include "id.h"
}
#include "gtest/gtest.h"

extern co_timer_t co_timer_proto;

static bool executed;
static bool executed2;
static bool executed3;
static co_timer_t *timer_reset;
static co_timer_t *timer_cancel;
static int timer_callback(void *a, void *b);
static int timer_callback2(void *a, void *b);
static int timer_callback4(void *a, void *b);
static int timer_callback5(void *a, void *b);
static int timer_callback6(void *a, void *b);

class SchedulerTests : public ::testing::Test {
protected:
  SchedulerTests() {
    executed = false;
    executed2 = false;
    executed3 = false;
    co_loop_create();
  }
  virtual ~SchedulerTests() {
    co_loop_destroy();
  }
};

static int timer_callback(void *a, void *b) {
  printf("callback\n");
  executed = true;
  EXPECT_FALSE(executed2);
  return 1;
}

static int timer_callback2(void *a, void *b) {
  printf("callback2\n");
  EXPECT_TRUE(executed);
  executed2 = true;
  co_loop_stop();
  return 1;
}

static int timer_callback4(void *a, void *b) {
  printf("callback4\n");
  co_loop_remove_timer(timer_cancel,(void*)NULL);
  co_loop_set_timer(timer_reset,999,(void*)NULL);
  executed = true;
  return 1;
}

static int timer_callback5(void *a, void *b) {
  printf("callback5\n");
  executed2 = true;
  return 1;
}

static int timer_callback6(void *a, void *b) {
  printf("callback6\n");
  executed3 = true;
  co_loop_stop();
  return 1;
}

TEST_F(SchedulerTests, Scheduler) {
  co_timer_t *timer1 = NEW(co_timer,co_timer);
  co_timer_t *timer2 = NEW(co_timer,co_timer);
  timer1->timer_cb = timer_callback;
  timer2->timer_cb = timer_callback2;
  co_loop_set_timer(timer1,1000,(void*)NULL);
  co_loop_set_timer(timer2,1000,(void*)NULL);
  co_loop_start();
  EXPECT_TRUE(executed);
  EXPECT_TRUE(executed2);
  free(timer1);
  free(timer2);
}

TEST_F(SchedulerTests, Scheduler2) {
  struct timespec ts;
  co_timer_t *timer1 = NEW(co_timer,co_timer);
  co_timer_t *timer2 = NEW(co_timer,co_timer);
  timer1->timer_cb = timer_callback;
  timer2->timer_cb = timer_callback2;
  clock_gettime(CLOCK_REALTIME, &ts);
  timer1->deadline = {ts.tv_sec + 6,1000};
  co_loop_add_timer(timer1,NULL);
  co_loop_set_timer(timer2,7000,(void*)NULL);
  co_loop_start();
  EXPECT_TRUE(executed);
  EXPECT_TRUE(executed2);
  free(timer1);
  free(timer2);
}

TEST_F(SchedulerTests, Scheduler3) {
  co_timer_t *timer1 = NEW(co_timer,co_timer);
  timer_cancel = NEW(co_timer,co_timer);
  timer_reset = NEW(co_timer,co_timer);
  timer1->timer_cb = timer_callback4;
  timer_cancel->timer_cb = timer_callback5;
  timer_reset->timer_cb = timer_callback6;
  co_loop_set_timer(timer1,1000,(void*)NULL);
  co_loop_set_timer(timer_cancel,2000,(void*)NULL);
  co_loop_set_timer(timer_reset,3000,(void*)NULL);
  co_loop_start();
  EXPECT_TRUE(executed);
  EXPECT_FALSE(executed2);
  EXPECT_TRUE(executed3);
  free(timer1);
  free(timer_cancel);
  free(timer_reset);
}

TEST_F(SchedulerTests, Scheduler4) {
  co_timer_t *timer1 = NEW(co_timer,co_timer);
  timer1->timer_cb = timer_callback6;
  co_loop_set_timer(timer1,1000,(void*)NULL);
  EXPECT_FALSE(co_loop_add_timer(timer1,(void*)NULL));
  co_loop_start();
  EXPECT_TRUE(executed3);
  free(timer1);
}

int main(int argc, char **argv) {
  ::testing::InitGoogleTest(&argc, argv);
  return RUN_ALL_TESTS();
}
