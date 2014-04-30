extern "C" {
  #include "plugin.h"
  #include "obj.h"
  #include "list.h"
}
#include "gtest/gtest.h"

#define DEMO_PLUGINDIR "./demolibs"

  
// iterator function
co_obj_t *co_context_i(co_obj_t *data, co_obj_t *iter, void *context);
  
class PluginTest : public ::testing::Test
{
protected:
  // variables
  int ret;
  co_obj_t *plugin1;
  co_obj_t *_plugins;
  
  // NOTE _plugins is a commotion object that becomes the head of the plugins list

  // functions
  void Load();
  void Start();
  void Check();
  
  // iterator function
//   co_obj_t *co_context_i(co_obj_t *data, co_obj_t *iter, void *context);
  
  PluginTest()
  {
    co_plugins_init(16);
  }
  
  ~PluginTest()
  {
  }
};

co_obj_t *co_context_i(co_obj_t *data, co_obj_t *iter, void *context)
{
  return data;
}

void PluginTest::Load()
{
  const char *_plugindir = DEMO_PLUGINDIR;
  ret = co_plugins_load(_plugindir);
  ASSERT_EQ(1, ret);
}

void PluginTest::Start()
{
  ret = co_plugins_start();
  ASSERT_EQ(1, ret);
}

void PluginTest::Check()
{
   plugin1 = co_list_parse(_plugins, (co_iter_t)(co_context_i), NULL);
   DEBUG("\n\n HEY THERE!\n\n");
}

TEST_F(PluginTest, Load)
{
  Load();
}

TEST_F(PluginTest, Start)
{
  Start();
}

TEST_F(PluginTest, Check)
{
  Check();
}