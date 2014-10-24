/* vim: set ts=2 expandtab: */
/**
 *       @file  test.cpp
 *      @brief  
 *
 *     @author  Josh King (jheretic), jking@chambana.net
 *
 *   @internal
 *      Created  11/17/2013 06:01:11 PM
 *     Compiler  gcc/g++
 * Organization  The Open Technology Institute
 *    Copyright  Copyright (c) 2013, Josh King
 *
 * This file is part of Commotion, Copyright (c) 2013, Josh King 
 * 
 * Commotion is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published 
 * by the Free Software Foundation, either version 3 of the License, 
 * or (at your option) any later version.
 * 
 * Commotion is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 * 
 * You should have received a copy of the GNU General Public License
 * along with Commotion.  If not, see <http://www.gnu.org/licenses/>.
 *
 * =====================================================================================
 */
extern "C" {
#include "../src/obj.h"
#include "../src/tree.h"
}
#include "gtest/gtest.h"

class TreeNextTest : public ::testing::Test
{
  protected:
    co_obj_t *tree16;
    co_obj_t *tree32;
    int strs_len = 40;
    char *strs[40] = {
      "bbbb",
      "bbba",
      "bbbc",
      "bbab",
      "bbaa",
      "bbac",
      "bbcb",
      "bbca",
      "bbcc",
      "abb",
      "aba",
      "abc",
      "aab",
      "aaa",
      "aac",
      "acb",
      "aca",
      "acc",
      "cbb",
      "cba",
      "cbc",
      "cab",
      "caa",
      "cac",
      "ccb",
      "cca",
      "ccc",
      "b",
      "bb",
      "bba",
      "bbb",
      "bbc",
      "a",
      "aa",
      "ab",
      "ac",
      "c",
      "ca",
      "cb",
      "cc"
    };
    
    //tests
    void Test();

    TreeNextTest()
    {
      tree16 = co_tree16_create();
      tree32 = co_tree32_create();
      
      for (int i = 0; i < strs_len; i++) {
	co_obj_t *str16 = co_str8_create(strs[i],strlen(strs[i]) + 1,0),
		 *str32 = co_str8_create(strs[i],strlen(strs[i]) + 1,0);
	co_tree_insert(tree16,strs[i],strlen(strs[i]) + 1,str16);
	co_tree_insert(tree32,strs[i],strlen(strs[i]) + 1,str32);
      }
    }

    virtual void SetUp()
    {
    }

    ~TreeNextTest()
    {
      co_obj_free(tree16);
      co_obj_free(tree32);
    }
};

/**
 * Compare strings alphabetically, used in qsort
 */
static int
cmpstringp(const void *p1, const void *p2)
{
  /* The actual arguments to this function are "pointers to
    *      pointers to char", but strcmp(3) arguments are "pointers
    *      to char", hence the following cast plus dereference */
  return strcmp(* (char * const *) p1, * (char * const *) p2);
}

void TreeNextTest::Test()
{
  co_obj_t *key = NULL;
  int i = 0;
  
  /* Sort test strings into alphabetical order */
  qsort(strs,strs_len,sizeof(char*),cmpstringp);
  
  for (key = co_tree_next(tree16, NULL), i = 0; i < strs_len; key = co_tree_next(tree16, key), i++) {
    if (!key)
      co_tree_print(tree16);
    ASSERT_TRUE(key);
    EXPECT_STREQ(co_obj_data_ptr(key), strs[i]);
    co_obj_t *val = co_tree_find(tree16, strs[i], strlen(strs[i]) + 1);
    ASSERT_TRUE(val);
    EXPECT_STREQ(co_obj_data_ptr(val), strs[i]);
  }
  EXPECT_EQ(i,strs_len);
  
  for (key = co_tree_next(tree32, NULL), i = 0; i < strs_len; key = co_tree_next(tree32, key), i++) {
    if (!key)
      co_tree_print(tree32);
    ASSERT_TRUE(key);
    EXPECT_STREQ(co_obj_data_ptr(key), strs[i]);
    co_obj_t *val = co_tree_find(tree32, strs[i], strlen(strs[i]) + 1);
    ASSERT_TRUE(val);
    EXPECT_STREQ(co_obj_data_ptr(val), strs[i]);
  }
  EXPECT_EQ(i,strs_len);
}

TEST_F(TreeNextTest, TreeNextTest)
{
  Test();
} 
