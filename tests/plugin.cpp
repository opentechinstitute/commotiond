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
  const char *_plugindir = DEMO_PLUGINDIR;
  
  
  co_obj_t *_plugins; // NOTE: this is just a placeholder. I have to add co_list_plugins to plugins.c which will create it's own list, which I will use here
  
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
   ret = co_plugins_load(_plugindir);
   
   plugin1 = co_list_parse(_plugins, (co_iter_t)(co_context_i), NULL);
   DEBUG("\n\nHEY THERE!\n\n");
   DEBUG("\n\nObject: %s\n\n", plugin1);
   
   co_obj_t *plugins2 = co_plugins_list();
   DEBUG("\n\nObject: %s\n\n", plugins2);
   
   
   // confirm that this is the right object (the demo plugin)
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