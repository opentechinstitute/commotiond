/**
 * Plan of Attack
 * * identify functions to test
 * * plan out test fixtures and structure
 * * set up test class
 * 
 */

/**
 * Functions to test:
 * int co_profiles_create(const size_t index_size); +
 * int co_profiles_init(const size_t index_size); +
 * co_obj_t *co_profile_find(co_obj_t *name); + 
 * int co_profile_add(const char *name, const size_t nlen); +
 * int co_profile_remove(const char *name, const size_t nlen);
 * co_obj_t *co_profile_get(co_obj_t *profile, const co_obj_t *key);
 * size_t co_profile_get_str(co_obj_t *profile, char **output, const char *key, const size_t klen);
 * int co_profile_set_str(co_obj_t *profile, const char *key, const size_t klen, const char *value, const size_t vlen);
 * 
 * 
 */

/**
 * TEST Create
 * 
 * create
 * init
 * add
 * find
 * 
 * 
 * TEST Modify
 * 
 * create add, etc
 * set string, int, etc.
 * 
 * 
 * TEST Retrieve
 * 
 * create, add, etc.
 * get string, int, etc
 * 
 * 
 * TEST Remove
 * 
 * remove
 */

extern "C" {
#include "../src/obj.h"
#include "../src/list.h"
#include "../src/tree.h"
#include "../src/profile.h"
}
#include "gtest/gtest.h"


class ProfileTest : public ::testing::Test
{
protected:
  // test functions
  void Init();
  void Add();
  void Retrieve();
  void Remove();
  
  // variables
  int ret;
  co_obj_t *profile1 = co_str8_create("profile1", 9, 0);
  co_obj_t *profile2 = co_str8_create("profile2", 9, 0);
  
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

void ProfileTest::Retrieve()
{
  ret = co_profile_add("profile1", 9);
  ASSERT_EQ(1, ret);

  ret = co_profile_add("profile2", 9);
  ASSERT_EQ(1, ret);

  co_obj_t *found = co_profile_find(profile1);
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
  co_obj_t *found = co_profile_find(profile1);
  ASSERT_TRUE(NULL == found);
  
  ret = co_profile_remove("profile2", 9);
  ASSERT_EQ(1, ret);
  
  found = co_profile_find(profile2);
  ASSERT_TRUE(NULL == found);
  
  
}

TEST_F(ProfileTest, Init)
{
  Init();
}

TEST_F(ProfileTest, Add)
{
  Add();
}

TEST_F(ProfileTest, Retrieve)
{
  Retrieve();
}

TEST_F(ProfileTest, Remove)
{
  Remove();
}