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
    
    void WaitForLongTermJob()
    {
        std::this_thread::sleep_for(kLongTermJob);
    }
    
    MATCHER_P(CompareWithAtomic, value, "")
    {
        return value == arg;
    }
}

TEST(ExecutionPool, ExecutionStream_UsualRun)
{
    execq::ExecutionPool pool;
    
    ::testing::MockFunction<void(const std::atomic_bool&)> mockExecutor;
    auto stream = pool.createExecutionStream(mockExecutor.AsStdFunction());
    
    auto executedTaskCount = std::make_shared<std::atomic_size_t>(0);
    auto canceledTaskCount = std::make_shared<std::atomic_size_t>(0);
    EXPECT_CALL(mockExecutor, Call(::testing::_))
    .WillRepeatedly(::testing::Invoke([executedTaskCount, canceledTaskCount] (const std::atomic_bool& shouldQuit) {
        if (shouldQuit)
        {
            (*canceledTaskCount)++;
        }
        else
        {
            (*executedTaskCount)++;
            WaitForLongTermJob();
        }
    }));
    
    stream->start();
    
    // wait some time to be sure that object has been delivered to corresponding thread.
    WaitForLongTermJob();
    
    // At least 2 tasks must be executed (1 in pool threads + 1 in stream thread)
    EXPECT_GE(executedTaskCount->load(), 2);
    EXPECT_EQ(canceledTaskCount->load(), 0);
}

TEST(ExecutionPool, ExecutionStream_Delegate)
{
    class MockExecutionStreamDelegate: public execq::details::IExecutionStreamDelegate
    {
    public:
        MOCK_METHOD1(registerStreamTaskProvider, void(execq::details::ITaskProvider& taskProvider));
        MOCK_METHOD1(unregisterStreamTaskProvider, void(const execq::details::ITaskProvider& taskProvider));
        MOCK_METHOD0(streamDidStart, void());
    };
    
    MockExecutionStreamDelegate delegate;
    
    
    //  Queue must call 'register' method when created and 'unregister' method when destroyed.
    EXPECT_CALL(delegate, registerStreamTaskProvider(::testing::_))
    .WillOnce(::testing::Return());
    
    EXPECT_CALL(delegate, unregisterStreamTaskProvider(::testing::_))
    .WillOnce(::testing::Return());
    
    // Stream is created in 'stopped' state.
    ::testing::MockFunction<void(const std::atomic_bool&)> mockExecutor;
    execq::details::ExecutionStream stream(delegate, mockExecutor.AsStdFunction());
    
    EXPECT_CALL(mockExecutor, Call(::testing::_))
    .WillRepeatedly(::testing::Invoke([] (const std::atomic_bool&) {
        WaitForLongTermJob();
    }));
    
    EXPECT_CALL(delegate, streamDidStart())
    .WillOnce(::testing::Return());
    
    stream.start();
    
    
    // check if 'streamDidStart' is called after 'stop'
    stream.stop();
    
    EXPECT_CALL(delegate, streamDidStart())
    .WillOnce(::testing::Return());
    
    stream.start();
}
