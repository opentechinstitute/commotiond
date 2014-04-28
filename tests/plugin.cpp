extern "C" {
  #include "plugin.h"
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