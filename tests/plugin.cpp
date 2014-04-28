/**
 * POA
 * * Mimic commotiond functionality
 * *** Identify what pieces I need (eg. event loop, socket, etc.)
 * *** Get those up and running
 * 
 * * Copy in sample demo functions
 * 
 * * Check that sample functions execute correctly
 * 
 * 
 * make demo plugin directory in unit tests
 * --> set it to build
 * --> pass that in to co_plugins_load
 * 
 * 
 * daemon functions:
 * co_plugins_init(16);
 * co_plugins_load(_plugins)
 * co_plugins_start();
 * 
 * 
 */

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
  int ret = 0;

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