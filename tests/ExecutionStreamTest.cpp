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

#include "execq.h"
#include "ExecutionPool.h"
#include "ExecutionStream.h"
#include "ExecqTestUtil.h"

using namespace execq::test;

TEST(ExecutionPool, ExecutionStream_UsualRun)
{
    auto pool = execq::CreateExecutionPool();
    
    ::testing::MockFunction<void(const std::atomic_bool&)> mockExecutor;
    auto stream = execq::CreateExecutionStream(pool, mockExecutor.AsStdFunction());
    
    // counters must be shared_ptrs to be not destroyed at the end of test scope.
    auto executedTaskCount = std::make_shared<std::atomic_size_t>(0);
    auto canceledTaskCount = std::make_shared<std::atomic_size_t>(0);
    EXPECT_CALL(mockExecutor, Call(::testing::_))
    .WillRepeatedly(::testing::Invoke([executedTaskCount, canceledTaskCount] (const std::atomic_bool& isCanceled) {
        if (isCanceled)
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
    
    // At least few tasks must be executed
    EXPECT_GE(executedTaskCount->load(), 0);
    EXPECT_EQ(canceledTaskCount->load(), 0);
}

TEST(ExecutionPool, ExecutionStream_WorkerPool)
{
    auto executionPool = std::make_shared<execq::test::MockExecutionPool>();
    MockThreadWorkerFactory workerFactory {};
    
    //  Stream must 'register' itself in ExecutionPool when created
    execq::impl::ITaskProvider* registeredProvider = nullptr;
    EXPECT_CALL(*executionPool, addProvider(SaveArgAddress(&registeredProvider)))
    .WillOnce(::testing::Return());
    
    
    // Strean also creates additional single thread worker for its own needs
    std::unique_ptr<MockThreadWorker> additionalWorkerPtr(new MockThreadWorker{});
    MockThreadWorker& additionalWorker = *additionalWorkerPtr;
    EXPECT_CALL(workerFactory, createWorker(::testing::_))
    .WillOnce(::testing::Return(::testing::ByMove(std::move(additionalWorkerPtr))));
    
    
    // Create stream with mock execution function
    ::testing::MockFunction<void(const std::atomic_bool&)> mockExecutor;
    execq::impl::ExecutionStream stream(executionPool, workerFactory, mockExecutor.AsStdFunction());
    ASSERT_NE(registeredProvider, nullptr);
    
    
    // When steram starts, it notifies all workers
    EXPECT_CALL(*executionPool, notifyAllWorkers())
    .WillOnce(::testing::Return());
    EXPECT_CALL(additionalWorker, notifyWorker())
    .WillOnce(::testing::Return(true));
    
    stream.start();
    
    
    // While stream is started, it always produce valid tasks
    execq::impl::Task task = registeredProvider->nextTask();
    EXPECT_TRUE(task.valid());
    EXPECT_CALL(mockExecutor, Call(::testing::_))
    .WillOnce(::testing::Return());
    task();
    
    
    stream.stop();
    
    // When stopped, tasks are invalid
    EXPECT_FALSE(registeredProvider->nextTask().valid());
    
    
    //  Stream must 'unregister' itself in ExecutionPool when destroyed
    EXPECT_CALL(*executionPool, removeProvider(::testing::_))
    .WillOnce(::testing::Return());
}
