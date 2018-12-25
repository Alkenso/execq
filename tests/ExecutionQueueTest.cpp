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
    const std::chrono::milliseconds kTimeout = 5 * kLongTermJob;
    
    void WaitForLongTermJob()
    {
        std::this_thread::sleep_for(kLongTermJob);
    }
    
    MATCHER_P(CompareWithAtomic, value, "")
    {
        return value == arg;
    }
}

TEST(ExecutionPool, ExecutionQueue_SingleTask)
{
    execq::ExecutionPool pool;
    
    ::testing::MockFunction<void(const std::atomic_bool&, std::string)> mockExecutor;
    auto queue = pool.createExecutionQueue<std::string>(mockExecutor.AsStdFunction());
    
    EXPECT_CALL(mockExecutor, Call(CompareWithAtomic(false), "qwe"))
    .WillOnce(::testing::Return());
    
    queue->push("qwe");
    
    
    // wait some time to be sure that object has been delivered to corresponding thread.
    WaitForLongTermJob();
}

TEST(ExecutionPool, ExecutionQueue_MultipleTasks)
{
    execq::ExecutionPool pool;
    
    ::testing::MockFunction<void(const std::atomic_bool&, uint32_t)> mockExecutor;
    auto queue = pool.createExecutionQueue<uint32_t>(mockExecutor.AsStdFunction());
    
    const int count = 100;
    EXPECT_CALL(mockExecutor, Call(CompareWithAtomic(false), ::testing::_))
    .Times(count).WillRepeatedly(::testing::Return());
    
    for (int i = 0; i < count; i++)
    {
        queue->push(arc4random());
    }
    
    
    // wait some time to be sure that objects have been delivered to corresponding threads and processed.
    WaitForLongTermJob();
}

TEST(ExecutionPool, ExecutionQueue_TaskExecutionWhenQueueDestroyed)
{
    execq::ExecutionPool pool;
    
    ::testing::MockFunction<void(const std::atomic_bool&, std::string)> mockExecutor;
    std::promise<std::pair<bool, std::string>> isExecutedPromise;
    auto isExecuted = isExecutedPromise.get_future();
    auto queue = pool.createExecutionQueue<std::string>([&isExecutedPromise] (const std::atomic_bool& shouldQuit, std::string object) {
        // wait for double time comparing to time waiting before reset
        WaitForLongTermJob();
        WaitForLongTermJob();
        isExecutedPromise.set_value(std::make_pair(shouldQuit.load(), object));
    });
    queue->push("qwe");
    
    
    // wait for enough time to push object into processing.
    WaitForLongTermJob();
    queue.reset();
    
    ASSERT_EQ(isExecuted.wait_for(kTimeout), std::future_status::ready);
    
    std::pair<bool, std::string> executeState = isExecuted.get();
    EXPECT_EQ(executeState.first, true);
    EXPECT_EQ(executeState.second, "qwe");
}

TEST(ExecutionPool, ExecutionQueue_Delegate)
{
    class MockExecutionQueueDelegate: public execq::details::IExecutionQueueDelegate
    {
    public:
        MOCK_METHOD1(registerQueueTaskProvider, void(execq::details::ITaskProvider& taskProvider));
        MOCK_METHOD1(unregisterQueueTaskProvider, void(const execq::details::ITaskProvider& taskProvider));
        MOCK_METHOD0(queueDidReceiveNewTask, void());
    };
    
    MockExecutionQueueDelegate delegate;
    
    
    //  Queue must call 'register' method when created and 'unregister' method when destroyed.
    EXPECT_CALL(delegate, registerQueueTaskProvider(::testing::_))
    .WillOnce(::testing::Return());
    
    EXPECT_CALL(delegate, unregisterQueueTaskProvider(::testing::_))
    .WillOnce(::testing::Return());
    
    
    ::testing::MockFunction<void(const std::atomic_bool&, std::string)> mockExecutor;
    execq::details::ExecutionQueue<std::string> queue(delegate, mockExecutor.AsStdFunction());
    
    EXPECT_CALL(delegate, queueDidReceiveNewTask())
    .WillOnce(::testing::Return());
    
    EXPECT_CALL(mockExecutor, Call(CompareWithAtomic(false), "qwe"))
    .WillOnce(::testing::Return());
    
    queue.push("qwe");
    
    
    // wait some time to be sure that object has been delivered to corresponding thread.
    WaitForLongTermJob();
}
