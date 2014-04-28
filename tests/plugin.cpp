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
  #include "plugin.h"
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

#define DEMO_PLUGINDIR "./demolibs"
  
class PluginTest : public ::testing::Test
{
protected:
  // variables
  int ret;

  // functions
  
  void Load();
  void Start();
  
  PluginTest()
  {
    co_plugins_init(16);
    // create plugins list
  }
  
  ~PluginTest()
  {
  }
};

void PluginTest::Load()
{
  const char *_plugindir = DEMO_PLUGINDIR;
  ret = co_plugins_load(_plugindir);
  ASSERT_EQ(1, ret);
}
/*
void PluginTest::Start()
{
  ret = co_plugins_start();
  
}
*/
TEST_F(PluginTest, Load)
{
  Load();
}
/*
TEST_F(PluginTest, Start)
{
  Start();
}
*/