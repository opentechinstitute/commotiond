extern "C" {
#include "../src/obj.h"
#include "../src/list.h"
#include "../src/tree.h"
#include "../src/profile.h"
}
#include "gtest/gtest.h"

SCHEMA(default)
{
  SCHEMA_ADD("ssid", "commotionwireless.net"); 
  SCHEMA_ADD("bssid", "02:CA:FF:EE:BA:BE"); 
  SCHEMA_ADD("bssidgen", "true"); 
  SCHEMA_ADD("channel", "5"); 
  SCHEMA_ADD("mode", "adhoc"); 
  SCHEMA_ADD("type", "mesh"); 
  SCHEMA_ADD("dns", "208.67.222.222"); 
  SCHEMA_ADD("domain", "mesh.local"); 
  SCHEMA_ADD("ipgen", "true"); 
  SCHEMA_ADD("ip", "100.64.0.0"); 
  SCHEMA_ADD("netmask", "255.192.0.0"); 
  SCHEMA_ADD("ipgenmask", "255.192.0.0"); 
  SCHEMA_ADD("encryption", "psk2"); 
  SCHEMA_ADD("key", "c0MM0t10n!r0cks"); 
  SCHEMA_ADD("serval", "false"); 
  SCHEMA_ADD("announce", "true"); 
  return 1;
}

class ProfileTest : public ::testing::Test
{
protected:
  // test functions
  void Init();
  void Add();
  void Find();
  void Remove();
  void SetGet();
  
  // variables
  int ret = 0;
  co_obj_t *profile1 = co_str8_create("profile1", 9, 0);
  co_obj_t *profile2 = co_str8_create("profile2", 9, 0);
  co_obj_t *found = NULL;
  
  // constructor
  ProfileTest()
  {
    ret = co_profiles_init(16);
  }
  
  virtual void SetUp()
  {
  }
  
  ~ProfileTest()
  {
  }
};

void ProfileTest::Init()
{
  ASSERT_EQ(1, ret);
}

void ProfileTest::Add()
{
  ret = co_profile_add("profile1", 9);
  ASSERT_EQ(1, ret);
}

void ProfileTest::Find()
{
  ret = co_profile_add("profile1", 9);
  ASSERT_EQ(1, ret);

  ret = co_profile_add("profile2", 9);
  ASSERT_EQ(1, ret);

  found = co_profile_find(profile1);
  ASSERT_TRUE(NULL != found);
  
  found = co_profile_find(profile2);
  ASSERT_TRUE(NULL != found);
}

void ProfileTest::Remove()
{
  ret = co_profile_add("profile1", 9);
  ASSERT_EQ(1, ret);

  ret = co_profile_add("profile2", 9);
  ASSERT_EQ(1, ret);

  ret = co_profile_remove("profile1", 9);
  ASSERT_EQ(1, ret);
  
  // confirm removal
  found = co_profile_find(profile1);
  ASSERT_TRUE(NULL == found);
  
  ret = co_profile_remove("profile2", 9);
  ASSERT_EQ(1, ret);
  
  //confirm removal
  found = co_profile_find(profile2);
  ASSERT_TRUE(NULL == found);
}

void ProfileTest::SetGet()
{
  SCHEMA_REGISTER(default);
    
  ret = co_profile_add("profile1", 9);
  ASSERT_EQ(1, ret);
  
  found = co_profile_find(profile1);
  ASSERT_TRUE(NULL != found);
  
  ret = co_profile_set_str((co_obj_t *)found, "ip", sizeof("ip"), "192.168.1.254", sizeof("192.168.1.254"));
  ASSERT_EQ(1, ret);
  
  char *ip;
  
  ret = co_profile_get_str(found, &ip, "ip", sizeof("ip"));
  ASSERT_STREQ("192.168.1.254", ip);
}

TEST_F(ProfileTest, Init)
{
  Init();
}

TEST_F(ProfileTest, Add)
{
  Add();
}

TEST_F(ProfileTest, Find)
{
  Find();
}

TEST_F(ProfileTest, Remove)
{
  Remove();
}

TEST_F(ProfileTest, SetGet)
{
  SetGet();
}