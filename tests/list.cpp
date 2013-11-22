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

class ListTest : public ::testing::Test
{
  protected:
    co_obj_t *List16;
    co_obj_t *List32;
    void InsertObj();
    void DeleteObj();

    ListTest()
    {
      List16 = co_list16_create();
      List32 = co_list32_create();
    }

    virtual void SetUp()
    {
    }

    ~ListTest()
    {
      co_free(List16);  
      co_free(List32);  
    }
};

void ListTest::InsertObj()
{
  co_obj_t *TestString1 = co_bin8_create("1TESTVALUE1", 11, 0);
  int ret = co_list_append(List16, TestString1);
  ASSERT_EQ(1, ret);
  ret = co_list_contains(List16, TestString1);
  ASSERT_EQ(1, ret);
  co_obj_t *TestString2 = co_bin8_create("2TESTVALUE2", 12, 0);
  ret = co_list_append(List16, TestString2);
  ASSERT_EQ(1, ret);
  ret = co_list_contains(List16, TestString2);
  ASSERT_EQ(1, ret);
  ret = co_list_append(List32, TestString1);
  ASSERT_EQ(1, ret);
  ret = co_list_contains(List32, TestString1);
  ASSERT_EQ(1, ret);
  ret = co_list_append(List32, TestString2);
  ASSERT_EQ(1, ret);
  ret = co_list_contains(List32, TestString2);
  ASSERT_EQ(1, ret);
}

void ListTest::DeleteObj()
{
  co_obj_t *TestString1 = co_bin8_create("1TESTVALUE1", 11, 0);
  int ret = co_list_append(List16, TestString1);
  ASSERT_EQ(1, ret);
  co_obj_t *ptr = co_list_delete(List16, TestString1);
  ASSERT_EQ(TestString1, ptr);
  co_obj_t *TestString2 = co_bin8_create("2TESTVALUE2", 11, 0);
  ret = co_list_append(List16, TestString2);
  ASSERT_EQ(1, ret);
  ptr = co_list_delete(List16, TestString2);
  ASSERT_EQ(TestString2, ptr);
}
  
TEST_F(ListTest, ListInsertTest)
{
  InsertObj();
} 

TEST_F(ListTest, ListDeleteTest)
{
  DeleteObj();
} 
