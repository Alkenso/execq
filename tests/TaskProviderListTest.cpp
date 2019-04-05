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

#include "TaskProviderList.h"

#include <gmock/gmock.h>

using namespace ::testing;

namespace
{
    class MockTaskProvider: public execq::impl::ITaskProvider
    {
    public:
        MOCK_METHOD0(nextTask, execq::impl::Task());
    };
    
    execq::impl::Task MakeValidTask()
    {
        return execq::impl::Task([] {});
    }
    
    execq::impl::Task MakeInvalidTask()
    {
        return execq::impl::Task();
    }
}

TEST(ExecutionPool, TaskProviderList_NoItems)
{
    execq::impl::TaskProviderList providers;
    
    EXPECT_FALSE(providers.nextTask().valid());
}

TEST(ExecutionPool, TaskProviderList_SindleItem)
{
    execq::impl::TaskProviderList providers;
    
    MockTaskProvider provider;
    providers.addProvider(provider);
    
    
    EXPECT_CALL(provider, nextTask())
    .WillOnce([] { return MakeValidTask(); })
    .WillOnce([] { return MakeInvalidTask(); });
    
    ASSERT_TRUE(providers.nextTask().valid());
    EXPECT_FALSE(providers.nextTask().valid());
}

TEST(ExecutionPool, TaskProviderList_MultipleItems)
{
    execq::impl::TaskProviderList providers;
    
    // fill group with providers
    MockTaskProvider provider1;
    providers.addProvider(provider1);
    
    MockTaskProvider provider2;
    providers.addProvider(provider2);
    
    MockTaskProvider provider3;
    providers.addProvider(provider3);
    
    // Provider #1 and #3 have one valid task. Provider #2 hasn't valid tasks
    EXPECT_CALL(provider1, nextTask())
    .WillOnce([] { return MakeValidTask(); })
    .WillRepeatedly([] { return MakeInvalidTask(); });
    
    EXPECT_CALL(provider3, nextTask())
    .WillOnce([] { return MakeValidTask(); })
    .WillRepeatedly([] { return MakeInvalidTask(); });
    
    EXPECT_CALL(provider2, nextTask())
    .WillRepeatedly([] { return MakeInvalidTask(); });
    
    
    // Checking & Executing task from the #1 provider
    ASSERT_TRUE(providers.nextTask().valid());
    
    // Checking #2 provider. Checking & Executing task from the #3 provider
    ASSERT_TRUE(providers.nextTask().valid());
    
    // Next times all providers will be checked, but not executed because of lack of tasks
    EXPECT_FALSE(providers.nextTask().valid());
}

TEST(ExecutionPool, TaskProviderList_Add_Remove)
{
    execq::impl::TaskProviderList providers;
    
    // fill group with providers
    MockTaskProvider provider1;
    providers.addProvider(provider1);
    
    MockTaskProvider provider2;
    providers.addProvider(provider2);
    
    // Then remove first provider, so it shouldn't be called in any way
    providers.removeProvider(provider1);
    
    // Make an expectations
    EXPECT_CALL(provider1, nextTask())
    .Times(0);
    
    EXPECT_CALL(provider2, nextTask())
    .WillOnce([] { return MakeValidTask(); })
    .WillOnce([] { return MakeValidTask(); })
    .WillOnce([] { return MakeInvalidTask(); });
    
    
    // Check it
    EXPECT_TRUE(providers.nextTask().valid());
    EXPECT_TRUE(providers.nextTask().valid());
    EXPECT_FALSE(providers.nextTask().valid());
}
