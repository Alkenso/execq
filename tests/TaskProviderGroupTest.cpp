/*
 * MIT License
 *
 * Copyright (c) 2018 Alkenso (Vladimir Vashurkin)
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to deal
 * in the Software without restriction, including without limitation the rights
 * to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 * copies of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in all
 * copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
 * SOFTWARE.
 */

#include "TaskProviderGroup.h"

#include <gmock/gmock.h>

namespace
{
    class MockTaskProvider: public execq::impl::ITaskProvider
    {
    public:
        MOCK_CONST_METHOD0(hasTask, bool());
        MOCK_METHOD0(execute, bool());
    };
}

TEST(ExecutionPool, TaskProviderGroup_NoItems)
{
    execq::impl::TaskProviderGroup providerGroup;
    
    EXPECT_FALSE(providerGroup.execute());
}

TEST(ExecutionPool, TaskProviderGroup_SindleItem)
{
    execq::impl::TaskProviderGroup providerGroup;
    
    MockTaskProvider provider;
    providerGroup.addProvider(provider);
    
    
    EXPECT_CALL(provider, hasTask())
    .WillOnce(::testing::Return(true))
    .WillOnce(::testing::Return(false));
    
    EXPECT_CALL(provider, execute())
    .WillOnce(::testing::Return(true));
    
    EXPECT_TRUE(providerGroup.execute());
    EXPECT_FALSE(providerGroup.execute());
}

TEST(ExecutionPool, TaskProviderGroup_MultipleItems)
{
    execq::impl::TaskProviderGroup providerGroup;
    
    // fill group with providers
    MockTaskProvider provider1;
    providerGroup.addProvider(provider1);
    
    MockTaskProvider provider2;
    providerGroup.addProvider(provider2);
    
    MockTaskProvider provider3;
    providerGroup.addProvider(provider3);
    
    // Provider #1 and #3 one valid task. Provider #2 hasn't valid tasks
    EXPECT_CALL(provider1, hasTask())
    .WillOnce(::testing::Return(true))
    .WillRepeatedly(::testing::Return(false));
    EXPECT_CALL(provider1, execute())
    .WillOnce(::testing::Return(true));
    
    EXPECT_CALL(provider3, hasTask())
    .WillOnce(::testing::Return(true))
    .WillRepeatedly(::testing::Return(false));
    EXPECT_CALL(provider3, execute())
    .WillOnce(::testing::Return(true));
    
    EXPECT_CALL(provider2, hasTask())
    .WillRepeatedly(::testing::Return(false));
    EXPECT_CALL(provider2, execute())
    .Times(0);
    
    
    // Checking & Executing task from the #1 provider
    EXPECT_TRUE(providerGroup.execute());
    
    // Checking #2 provider. Checking & Executing task from the #3 provider
    EXPECT_TRUE(providerGroup.execute());
    
    // Next times all providers will be checked, but not executed because of lack of tasks
    EXPECT_FALSE(providerGroup.execute());
    EXPECT_FALSE(providerGroup.execute());
    EXPECT_FALSE(providerGroup.execute());
}

TEST(ExecutionPool, TaskProviderGroup_Add_Remove)
{
    execq::impl::TaskProviderGroup providerGroup;
    
    // fill group with providers
    MockTaskProvider provider1;
    providerGroup.addProvider(provider1);
    
    MockTaskProvider provider2;
    providerGroup.addProvider(provider2);
    
    // Then remove first provider, so it shouldn't be called in any way
    providerGroup.removeProvider(provider1);
    
    // Make an expectations
    EXPECT_CALL(provider1, hasTask())
    .Times(0);
    EXPECT_CALL(provider1, execute())
    .Times(0);
    
    EXPECT_CALL(provider2, hasTask())
    .WillOnce(::testing::Return(true))
    .WillOnce(::testing::Return(true))
    .WillOnce(::testing::Return(false));
    EXPECT_CALL(provider2, execute())
    .WillOnce(::testing::Return(true))
    .WillOnce(::testing::Return(true));
    
    
    // Check it
    EXPECT_TRUE(providerGroup.execute());
    EXPECT_TRUE(providerGroup.execute());
    EXPECT_FALSE(providerGroup.execute());
}
