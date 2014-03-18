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
    void FirstLast();
    co_obj_t *TestString1;
    co_obj_t *TestString2;
    co_obj_t *TestString3;
    co_obj_t *TestString4;
    co_obj_t *TestString5;
    co_obj_t *TestString6;
    
    int ret;
    co_obj_t *ptr;
    
    ListTest()
    {
      List16 = co_list16_create();
      List32 = co_list32_create();
      
      TestString1 = co_bin8_create("1TESTVALUE1", 12, 0);
      TestString2 = co_bin8_create("2TESTVALUE2", 12, 0);
      TestString3 = co_bin8_create("3TESTVALUE3", 12, 0);
      TestString4 = co_bin8_create("4TESTVALUE4", 12, 0);
      TestString5 = co_bin8_create("5TESTVALUE5", 12, 0);
      TestString6 = co_bin8_create("6TESTVALUE6", 12, 0);
      
      ret = 0;
      ptr = NULL;
    }

    virtual void SetUp()
    {
    }

    ~ListTest()
    {
      co_obj_free(List16);  
      co_obj_free(List32);  
    }
};

void ListTest::InsertObj()
{
  ret = co_list_append(List16, TestString1);
  ASSERT_EQ(1, ret);
  
  ret = co_list_contains(List16, TestString1);
  ASSERT_EQ(1, ret);
  
  ret = co_list_append(List16, TestString2);
  ASSERT_EQ(1, ret);
  
  ret = co_list_contains(List16, TestString2);
  ASSERT_EQ(1, ret);
  
  // insert before
  ret = co_list_insert_before(List16, TestString3, TestString1);
  ASSERT_EQ(1, ret);
  
  ptr = co_list_get_first(List16);
  ASSERT_EQ(ptr, TestString3);
  
  // insert after
  ret = co_list_insert_after(List16, TestString4, TestString1);
  ASSERT_EQ(1, ret);
  
  ret = co_list_contains(List16, TestString4);
  ASSERT_EQ(1, ret);
  
  // append
  ret = co_list_append(List16, TestString5);
  ASSERT_EQ(1, ret);
  
  ptr = co_list_get_last(List16);
  ASSERT_EQ(ptr, TestString5);
  
  // prepend
  ret = co_list_prepend(List16, TestString6);
  ASSERT_EQ(1, ret);
  
  ptr = co_list_get_first(List16);
  ASSERT_EQ(ptr, TestString6);
  
  
  // repeat for List32
  ret = co_list_append(List32, TestString1);
  ASSERT_EQ(1, ret);
  
  ret = co_list_contains(List32, TestString1);
  ASSERT_EQ(1, ret);
  
  ret = co_list_append(List32, TestString2);
  ASSERT_EQ(1, ret);
  
  ret = co_list_contains(List32, TestString2);
  ASSERT_EQ(1, ret);
  
  // insert before
  ret = co_list_insert_before(List32, TestString3, TestString1);
  ASSERT_EQ(1, ret);
  
  ptr = co_list_get_first(List32);
  ASSERT_EQ(ptr, TestString3);
  
  // insert after
  ret = co_list_insert_after(List32, TestString4, TestString1);
  ASSERT_EQ(1, ret);
  
  ret = co_list_contains(List32, TestString4);
  ASSERT_EQ(1, ret);
  
  // append
  ret = co_list_append(List32, TestString5);
  ASSERT_EQ(1, ret);
  
  ptr = co_list_get_last(List32);
  ASSERT_EQ(ptr, TestString5);
  
  // prepend
  ret = co_list_prepend(List32, TestString6);
  ASSERT_EQ(1, ret);
  
  ptr = co_list_get_first(List32);
  ASSERT_EQ(ptr, TestString6);
}

void ListTest::DeleteObj()
{
  ret = co_list_append(List16, TestString1);
  ASSERT_EQ(1, ret);
  
  co_obj_t *ptr = co_list_delete(List16, TestString1);
  ASSERT_EQ(TestString1, ptr);
  
  ret = co_list_append(List16, TestString2);
  ASSERT_EQ(1, ret);
  
  ptr = co_list_delete(List16, TestString2);
  ASSERT_EQ(TestString2, ptr);
  
  // confirm deletions
  ret = co_list_contains(List16, TestString1);
  ASSERT_EQ(0, ret);
  
  ret = co_list_contains(List16, TestString1);
  ASSERT_EQ(0, ret);
  
  
  // repeat for List32
  ret = co_list_append(List32, TestString1);
  ASSERT_EQ(1, ret);
  
  ptr = co_list_delete(List32, TestString1);
  ASSERT_EQ(TestString1, ptr);
  
  ret = co_list_append(List32, TestString2);
  ASSERT_EQ(1, ret);
  
  ptr = co_list_delete(List32, TestString2);
  ASSERT_EQ(TestString2, ptr);
  
  // confirm deletions
  ret = co_list_contains(List32, TestString1);
  ASSERT_EQ(0, ret);
  
  ret = co_list_contains(List32, TestString1);
  ASSERT_EQ(0, ret);
}

void ListTest::FirstLast()
{
  ret = co_list_append(List16, TestString1);
  ASSERT_EQ(1, ret);
  
  ret = co_list_append(List16, TestString2);
  ASSERT_EQ(1, ret);
  
  ret = co_list_append(List16, TestString3);
  ASSERT_EQ(1, ret);

  // get first node
  ptr = co_list_get_first(List16);
  ASSERT_EQ(TestString1, ptr);
  
  // get last node
  ptr = co_list_get_last(List16);
  ASSERT_EQ(TestString3, ptr);
  
  // get specific nodes
  ptr = co_list_element(List16, 1);
  ASSERT_EQ(TestString2, ptr);
  
  ptr = co_list_element(List16, 2);
  ASSERT_EQ(TestString3, ptr);
  
  
  // repeat for List32
  ret = co_list_append(List32, TestString1);
  ASSERT_EQ(1, ret);
  
  ret = co_list_append(List32, TestString2);
  ASSERT_EQ(1, ret);
  
  ret = co_list_append(List32, TestString3);
  ASSERT_EQ(1, ret);

  // get first node
  ptr = co_list_get_first(List32);
  ASSERT_EQ(TestString1, ptr);
  
  // get last node
  ptr = co_list_get_last(List32);
  ASSERT_EQ(TestString3, ptr);
  
  // get specific nodes
  ptr = co_list_element(List32, 1);
  ASSERT_EQ(TestString2, ptr);
  
  ptr = co_list_element(List32, 2);
  ASSERT_EQ(TestString3, ptr);
}
  
  

  
TEST_F(ListTest, ListInsertTest)
{
  InsertObj();
} 

TEST_F(ListTest, ListDeleteTest)
{
  DeleteObj();
}

TEST_F(ListTest, FirstLast)
{
  FirstLast();
}