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

#include <gmock/gmock.h>

#include "ExecutionPool.h"
#include "ExecutionStream.h"

namespace
{
    const std::chrono::milliseconds kLongTermJob { 100 };
    const std::chrono::milliseconds kTimeout { 500 };
    
    void WaitForLongTermJob()
    {
        std::this_thread::sleep_for(kLongTermJob);
    }
    
    MATCHER_P(CompareWithAtomic, value, "")
    {
        return value == arg;
    }
    
    MATCHER_P(SaveArgAddress, value, "")
    {
        *value = &arg;
        return true;
    }
    
    class MockExecutionQueueDelegate: public execq::details::IExecutionQueueDelegate
    {
    public:
        MOCK_METHOD1(registerQueueTaskProvider, void(execq::details::ITaskProvider& taskProvider));
        MOCK_METHOD1(unregisterQueueTaskProvider, void(const execq::details::ITaskProvider& taskProvider));
        MOCK_METHOD0(queueDidReceiveNewTask, void());
    };
}

TEST(ExecutionPool, ExecutionQueue_SingleTask)
{
    execq::ExecutionPool pool;
    
    ::testing::MockFunction<void(const std::atomic_bool&, std::string)> mockExecutor;
    auto queue = pool.createConcurrentExecutionQueue(mockExecutor.AsStdFunction());
    
    EXPECT_CALL(mockExecutor, Call(::testing::_, "qwe"))
    .WillOnce(::testing::Return());
    
    queue->push("qwe");
}

TEST(ExecutionPool, ExecutionQueue_SingleTask_WithFuture)
{
    execq::ExecutionPool pool;

    ::testing::MockFunction<std::string(const std::atomic_bool&, std::string)> mockExecutor;
    auto queue = pool.createConcurrentExecutionQueue(mockExecutor.AsStdFunction());

    // sleep for a some time and then return the same object
    EXPECT_CALL(mockExecutor, Call(::testing::_, "qwe"))
    .WillOnce(::testing::Invoke([] (const std::atomic_bool& shouldQuit, std::string s) {
        WaitForLongTermJob();
        return s;
    }));
    
    std::future<std::string> result = queue->push("qwe");
    
    EXPECT_TRUE(result.wait_for(kTimeout) == std::future_status::ready);
}

TEST(ExecutionPool, ExecutionQueue_MultipleTasks)
{
    execq::ExecutionPool pool;
    
    ::testing::MockFunction<void(const std::atomic_bool&, uint32_t)> mockExecutor;
    auto queue = pool.createConcurrentExecutionQueue(mockExecutor.AsStdFunction());
    
    const size_t count = 1000;
    EXPECT_CALL(mockExecutor, Call(::testing::_, ::testing::_))
    .Times(count).WillRepeatedly(::testing::Return());
    
    for (size_t i = 0; i < count; i++)
    {
        queue->push(arc4random());
    }
}

TEST(ExecutionPool, ExecutionQueue_TaskExecutionWhenQueueDestroyed)
{
    execq::ExecutionPool pool;
    
    ::testing::MockFunction<void(const std::atomic_bool&, std::string)> mockExecutor;
    std::promise<std::pair<bool, std::string>> isExecutedPromise;
    auto isExecuted = isExecutedPromise.get_future();
    auto queue = pool.createConcurrentExecutionQueue<std::string, void>([&isExecutedPromise] (const std::atomic_bool& shouldQuit, std::string object) {
        // wait for double time comparing to time waiting before reset
        WaitForLongTermJob();
        WaitForLongTermJob();
        isExecutedPromise.set_value(std::make_pair(shouldQuit.load(), object));
    });
    queue->push("qwe");
    
    // delete queue
    queue.reset();
    
    ASSERT_TRUE(isExecuted.valid());
    
    std::pair<bool, std::string> executeState = isExecuted.get();
    EXPECT_EQ(executeState.first, true);
    EXPECT_EQ(executeState.second, "qwe");
}

TEST(ExecutionPool, ExecutionQueue_Delegate)
{
    MockExecutionQueueDelegate delegate;
    
    //  Queue must call 'register' method when created and 'unregister' method when destroyed.
    execq::details::ITaskProvider* registeredProvider = nullptr;
    EXPECT_CALL(delegate, registerQueueTaskProvider(SaveArgAddress(&registeredProvider)))
    .WillOnce(::testing::Return());
    
    ::testing::MockFunction<void(const std::atomic_bool&, std::string)> mockExecutor;
    execq::details::ExecutionQueue<std::string, void> queue(false, delegate, mockExecutor.AsStdFunction());
    
    ASSERT_NE(registeredProvider, nullptr);
    
    EXPECT_CALL(delegate, queueDidReceiveNewTask())
    .WillOnce(::testing::Return());
    
    EXPECT_CALL(mockExecutor, Call(CompareWithAtomic(false), "qwe"))
    .WillOnce(::testing::Return());
    
    queue.push("qwe");
    
    execq::details::Task task = registeredProvider->nextTask();
    ASSERT_TRUE(task.valid());
    task();
    
    EXPECT_CALL(delegate, unregisterQueueTaskProvider(::testing::_))
    .WillOnce(::testing::Return());
}

TEST(ExecutionPool, ExecutionQueue_Serial)
{
    MockExecutionQueueDelegate delegate;
    
    //  Queue must call 'register' method when created and 'unregister' method when destroyed.
    execq::details::ITaskProvider* serialProvider = nullptr;
    EXPECT_CALL(delegate, registerQueueTaskProvider(SaveArgAddress(&serialProvider)))
    .WillOnce(::testing::Return());
    
    ::testing::MockFunction<void(const std::atomic_bool&, std::string)> mockExecutor;
    execq::details::ExecutionQueue<std::string, void> queue(true, delegate, mockExecutor.AsStdFunction());
    
    ASSERT_NE(serialProvider, nullptr);
    
    // setup other mock calls for test correctness
    EXPECT_CALL(delegate, queueDidReceiveNewTask())
    .WillRepeatedly(::testing::Return());
    
    EXPECT_CALL(delegate, unregisterQueueTaskProvider(::testing::_))
    .WillOnce(::testing::Return());
    
    // push few object into the queue
    queue.push("qwe");
    queue.push("qwe");
    
    // Serial queue should provide next valid task ONLY by one.
    // If any task is in progress (i.e. popped out of the queue), 'nextTask' have to return invalid task.
    execq::details::Task task1 = serialProvider->nextTask();
    ASSERT_TRUE(task1.valid());
    
    // ensure that attempts to get next task fails.
    ASSERT_FALSE(serialProvider->nextTask().valid());
    
    // run current task. The next task should be available right after call
    task1();
    
    execq::details::Task task2 = serialProvider->nextTask();
    EXPECT_TRUE(task2.valid());
    task2();
}
