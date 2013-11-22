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
#include "../src/list.h"
#include "../src/tree.h"
}
#include "gtest/gtest.h"

class TreeTest : public ::testing::Test
{
  protected:
    co_obj_t *Tree16;
    co_obj_t *Tree32;
    void InsertObj();
    void DeleteObj();

    TreeTest()
    {
      Tree16 = co_tree16_create();
      Tree32 = co_tree32_create();
    }

    virtual void SetUp()
    {
    }

    ~TreeTest()
    {
      co_free(Tree16);  
      co_free(Tree32);  
    }
};

void TreeTest::InsertObj()
{
  co_obj_t *TestString1 = co_bin8_create("1TESTVALUE1", 11, 0);
  int ret = co_tree_insert(Tree16, "1TESTKEY1", 10, TestString1);
  ASSERT_EQ(1, ret);
  co_obj_t *ptr = co_tree_find(Tree16, "1TESTKEY1", 10);
  ASSERT_EQ(TestString1, ptr);
  co_obj_t *TestString2 = co_bin8_create("2TESTVALUE2", 12, 0);
  ret = co_tree_insert(Tree16, "2TESTKEY2", 10, TestString2);
  ASSERT_EQ(1, ret);
  ptr = co_tree_find(Tree16, "2TESTKEY2", 10);
  ASSERT_EQ(TestString2, ptr);
  ret = co_tree_insert(Tree32, "1TESTKEY1", 10, TestString1);
  ASSERT_EQ(1, ret);
  ptr = co_tree_find(Tree32, "1TESTKEY1", 10);
  ASSERT_EQ(TestString1, ptr);
  ret = co_tree_insert(Tree32, "2TESTKEY2", 10, TestString2);
  ASSERT_EQ(1, ret);
  ptr = co_tree_find(Tree32, "2TESTKEY2", 10);
  ASSERT_EQ(TestString2, ptr);
}

void TreeTest::DeleteObj()
{
  co_obj_t *TestString1 = co_bin8_create("1TESTVALUE1", 11, 0);
  int ret = co_tree_insert(Tree16, "1TESTKEY1", 9, TestString1);
  ASSERT_EQ(1, ret);
  co_obj_t *ptr = co_tree_delete(Tree16, "1TESTKEY1", 9);
  ASSERT_EQ(TestString1, ptr);
  co_obj_t *TestString2 = co_bin8_create("2TESTVALUE2", 11, 0);
  ret = co_tree_insert(Tree16, "2TESTKEY2", 9, TestString2);
  ASSERT_EQ(1, ret);
  ptr = co_tree_delete(Tree16, "2TESTKEY2", 9);
  ASSERT_EQ(TestString2, ptr);
}
  
TEST_F(TreeTest, TreeInsertTest)
{
  InsertObj();
} 

TEST_F(TreeTest, TreeDeleteTest)
{
  DeleteObj();
} 
