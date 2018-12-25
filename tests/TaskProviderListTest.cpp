//
//  TaskProviderListTest.cpp
//  ExecutionQueueTests
//
//  Created by Alk on 12/25/18.
//  Copyright © 2018 alkenso. All rights reserved.
//

#include "TaskProviderList.h"

#include <gmock/gmock.h>

namespace
{
    class MockTaskProvider: public execq::details::ITaskProvider
    {
    public:
        MOCK_METHOD0(nextTask, execq::details::Task());
    };

    execq::details::Task MakeValidTask()
    {
        return execq::details::Task([] {});
    }

    execq::details::Task MakeInvalidTask()
    {
        return execq::details::Task();
    }
}

TEST(ExecutionPool, TaskProvidersList_NoItems)
{
    execq::details::TaskProviderList providers;
    
    EXPECT_FALSE(providers.nextTask().valid());
}

TEST(ExecutionPool, TaskProvidersList_SindleItem)
{
    execq::details::TaskProviderList providers;
    auto provider = std::make_shared<MockTaskProvider>();
    providers.add(*provider);
    
    
    // return valid tasks
    EXPECT_CALL(*provider, nextTask())
    .WillOnce(::testing::Return(::testing::ByMove(MakeValidTask())))
    .WillOnce(::testing::Return(::testing::ByMove(MakeValidTask())));
    
    EXPECT_TRUE(providers.nextTask().valid());
    EXPECT_TRUE(providers.nextTask().valid());
    
    
    // return invalid tasks
    EXPECT_CALL(*provider, nextTask())
    .WillOnce(::testing::Return(::testing::ByMove(MakeInvalidTask())))
    .WillOnce(::testing::Return(::testing::ByMove(MakeInvalidTask())));
    
    EXPECT_FALSE(providers.nextTask().valid());
    EXPECT_FALSE(providers.nextTask().valid());
}

TEST(ExecutionPool, TaskProvidersList_MultipleProvidersWithValidTasks)
{
    execq::details::TaskProviderList providers;
    
    // Provider #1 has 2 valid tasks
    auto provider1 = std::make_shared<MockTaskProvider>();
    providers.add(*provider1);
    
    ::testing::MockFunction<void()> provider1Callback;
    EXPECT_CALL(*provider1, nextTask())
    .WillOnce(::testing::Return(::testing::ByMove(execq::details::Task(provider1Callback.AsStdFunction()))))
    .WillOnce(::testing::Return(::testing::ByMove(execq::details::Task(provider1Callback.AsStdFunction()))));
    
    EXPECT_CALL(provider1Callback, Call())
    .Times(2);
    
    
    // Provider #2 and #3 each has 1 valid task
    auto provider2 = std::make_shared<MockTaskProvider>();
    providers.add(*provider2);
    
    ::testing::MockFunction<void()> provider2Callback;
    EXPECT_CALL(*provider2, nextTask())
    .WillOnce(::testing::Return(::testing::ByMove(execq::details::Task(provider2Callback.AsStdFunction()))));
    
    EXPECT_CALL(provider2Callback, Call())
    .Times(1);
    
    auto provider3 = std::make_shared<MockTaskProvider>();
    providers.add(*provider3);
    
    ::testing::MockFunction<void()> provider3Callback;
    EXPECT_CALL(*provider3, nextTask())
    .WillOnce(::testing::Return(::testing::ByMove(execq::details::Task(provider3Callback.AsStdFunction()))));
    
    EXPECT_CALL(provider3Callback, Call())
    .Times(1);
    
    
    // Receiving task from the first provider
    auto provider1Task = providers.nextTask();
    ASSERT_TRUE(provider1Task.valid());
    provider1Task();
    
    
    // Receiving task from the second provider
    auto provider2Task = providers.nextTask();
    ASSERT_TRUE(provider2Task.valid());
    provider2Task();
    
    
    // Receiving task from the third provider
    auto provider3Task = providers.nextTask();
    ASSERT_TRUE(provider3Task.valid());
    provider3Task();
    
    
    // Receiving task from the first provider again
    auto provider1Task2 = providers.nextTask();
    ASSERT_TRUE(provider1Task2.valid());
    provider1Task2();
}

TEST(ExecutionPool, TaskProvidersList_MultipleProvidersInvalidTasks)
{
    execq::details::TaskProviderList providers;
    
    // Providers have no valid tasks
    auto provider1 = std::make_shared<MockTaskProvider>();
    providers.add(*provider1);
    
    EXPECT_CALL(*provider1, nextTask())
    .WillOnce(::testing::Return(::testing::ByMove(execq::details::Task(MakeInvalidTask()))));
    
    auto provider2 = std::make_shared<MockTaskProvider>();
    providers.add(*provider2);
    
    EXPECT_CALL(*provider2, nextTask())
    .WillOnce(::testing::Return(::testing::ByMove(execq::details::Task(MakeInvalidTask()))));
    
    auto provider3 = std::make_shared<MockTaskProvider>();
    providers.add(*provider3);
    
    EXPECT_CALL(*provider3, nextTask())
    .WillOnce(::testing::Return(::testing::ByMove(execq::details::Task(MakeInvalidTask()))));
    
    
    // Providers have no valid tasks, returning invalid task
    EXPECT_FALSE(providers.nextTask().valid());
}

TEST(ExecutionPool, TaskProvidersList_MultipleProviders_Valid_Invalid_Tasks)
{
    execq::details::TaskProviderList providers;
    
    // Provider #1 has 1 valid task
    auto provider1 = std::make_shared<MockTaskProvider>();
    providers.add(*provider1);
    
    ::testing::MockFunction<void()> provider1Callback;
    EXPECT_CALL(*provider1, nextTask())
    .WillOnce(::testing::Return(::testing::ByMove(execq::details::Task(provider1Callback.AsStdFunction()))));
    
    EXPECT_CALL(provider1Callback, Call())
    .Times(1);
    
    
    // Provider #2 has no valid tasks
    auto provider2 = std::make_shared<MockTaskProvider>();
    providers.add(*provider2);
    
    EXPECT_CALL(*provider2, nextTask())
    .WillOnce(::testing::Return(::testing::ByMove(MakeInvalidTask())));
    
    
    // Provider #3 has 1 valid task
    auto provider3 = std::make_shared<MockTaskProvider>();
    providers.add(*provider3);
    
    ::testing::MockFunction<void()> provider3Callback;
    EXPECT_CALL(*provider3, nextTask())
    .WillOnce(::testing::Return(::testing::ByMove(execq::details::Task(provider3Callback.AsStdFunction()))));
    
    EXPECT_CALL(provider3Callback, Call())
    .Times(1);
    
    
    // Receiving task from the first provider
    auto provider1Task = providers.nextTask();
    ASSERT_TRUE(provider1Task.valid());
    provider1Task();
    
    
    // Skipping task from the second provider (because it has invalid task)
    // and
    // Receiving task from the third provider
    auto provider3Task = providers.nextTask();
    ASSERT_TRUE(provider3Task.valid());
    provider3Task();
}

TEST(ExecutionPool, TaskProvidersList_Add_Remove)
{
    execq::details::TaskProviderList providers;
    
    // No providers - no valid tasks
    EXPECT_FALSE(providers.nextTask().valid());
    
    // Add providers
    auto provider1 = std::make_shared<MockTaskProvider>();
    providers.add(*provider1);
    
    auto provider2 = std::make_shared<MockTaskProvider>();
    providers.add(*provider2);
    
    
    // Providers don't have valid tasks, so they both are checked
    EXPECT_CALL(*provider1, nextTask())
    .WillOnce(::testing::Return(::testing::ByMove(execq::details::Task(MakeInvalidTask()))));
    
    EXPECT_CALL(*provider2, nextTask())
    .WillOnce(::testing::Return(::testing::ByMove(execq::details::Task(MakeInvalidTask()))));
    
    EXPECT_FALSE(providers.nextTask().valid());
    
    
    // Provider 1 is removed, so check only provider #2
    providers.remove(*provider1);
    
    EXPECT_CALL(*provider2, nextTask())
    .WillOnce(::testing::Return(::testing::ByMove(execq::details::Task(MakeInvalidTask()))));
    
    EXPECT_FALSE(providers.nextTask().valid());
    
    
    // Provider 2 is removed, providers list is empty
    providers.remove(*provider2);
    
    EXPECT_FALSE(providers.nextTask().valid());
}
