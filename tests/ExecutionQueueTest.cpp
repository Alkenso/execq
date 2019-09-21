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
#include "ExecqTestUtil.h"

using namespace execq::test;

TEST(ExecutionPool, ExecutionQueue_SingleTask)
{
    auto pool = execq::CreateExecutionPool();

    ::testing::MockFunction<void(const std::atomic_bool&, std::string&&)> mockExecutor;
    auto queue = execq::CreateConcurrentExecutionQueue(pool, mockExecutor.AsStdFunction());

    EXPECT_CALL(mockExecutor, Call(::testing::_, CompareRvalue("qwe")))
    .WillOnce(::testing::Return());

    queue->push("qwe");
}

TEST(ExecutionPool, ExecutionQueue_SingleTask_WithFuture)
{
    auto pool = execq::CreateExecutionPool();

    ::testing::MockFunction<std::string(const std::atomic_bool&, std::string&&)> mockExecutor;
    auto queue = execq::CreateConcurrentExecutionQueue(pool, mockExecutor.AsStdFunction());

    // sleep for a some time and then return the same object
    EXPECT_CALL(mockExecutor, Call(::testing::_, CompareRvalue("qwe")))
    .WillOnce(::testing::Invoke([] (const std::atomic_bool& shouldQuit, std::string s) {
        WaitForLongTermJob();
        return s;
    }));

    std::future<std::string> result = queue->push("qwe");

    EXPECT_TRUE(result.wait_for(kTimeout) == std::future_status::ready);
}

TEST(ExecutionPool, ExecutionQueue_MultipleTasks)
{
    auto pool = execq::CreateExecutionPool();

    ::testing::MockFunction<void(const std::atomic_bool&, uint32_t&&)> mockExecutor;
    auto queue = execq::CreateConcurrentExecutionQueue(pool, mockExecutor.AsStdFunction());

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
    auto pool = execq::CreateExecutionPool();

    std::promise<std::pair<bool, std::string>> isExecutedPromise;
    auto isExecuted = isExecutedPromise.get_future();
    auto queue = execq::CreateConcurrentExecutionQueue<void, std::string>(pool, [&isExecutedPromise] (const std::atomic_bool& shouldQuit,
                                                                                                      std::string&& object) {
        // wait for double time comparing to time waiting before reset
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

TEST(ExecutionPool, ExecutionQueue_ExecutionPool_Concurrent)
{
    auto executionPool = std::make_shared<MockExecutionPool>();
    MockThreadWorkerFactory workerFactory {};
    
    //  Queue must 'register' itself in ExecutionPool when created
    execq::impl::ITaskProvider* registeredProvider = nullptr;
    EXPECT_CALL(*executionPool, addProvider(SaveArgAddress(&registeredProvider)))
    .WillOnce(::testing::Return());

    
    // Queue also creates additional single thread worker for its own needs
    std::unique_ptr<MockThreadWorker> additionalWorkerPtr(new MockThreadWorker{});
    MockThreadWorker& additionalWorker = *additionalWorkerPtr;
    EXPECT_CALL(workerFactory, createWorker(::testing::_))
    .WillOnce(::testing::Return(::testing::ByMove(std::move(additionalWorkerPtr))));
    
    
    // Create queue with mock execution function
    ::testing::MockFunction<void(const std::atomic_bool&, std::string&&)> mockExecutor;
    execq::impl::ExecutionQueue<void, std::string> queue(false, executionPool, workerFactory, mockExecutor.AsStdFunction());
    ASSERT_NE(registeredProvider, nullptr);
    
    
    // When new object comes to the queue, it notifies workers
    EXPECT_CALL(*executionPool, notifyOneWorker())
    .WillOnce(::testing::Return(true));
    queue.push("qwe");
    
    
    // If all workers of the pool are busy, trigger additional (own) worker
    EXPECT_CALL(*executionPool, notifyOneWorker())
    .WillOnce(::testing::Return(false));
    EXPECT_CALL(additionalWorker, notifyWorker())
    .WillOnce(::testing::Return(true));
    queue.push("asd");
    
    
    // Test that executors method is called in the proper moment
    execq::impl::Task task = registeredProvider->nextTask();
    ASSERT_TRUE(task.valid());
    EXPECT_CALL(mockExecutor, Call(CompareWithAtomic(false), CompareRvalue("qwe")))
    .WillOnce(::testing::Return());
    task();

    task = registeredProvider->nextTask();
    ASSERT_TRUE(task.valid());
    EXPECT_CALL(mockExecutor, Call(CompareWithAtomic(false), CompareRvalue("asd")))
    .WillOnce(::testing::Return());
    task();
    
    
    // No tasks - no execution
    EXPECT_FALSE(registeredProvider->nextTask().valid());
    
    
    //  Queue must 'unregister' itself in ExecutionPool when destroyed
    EXPECT_CALL(*executionPool, removeProvider(::testing::_))
    .WillOnce(::testing::Return());
}

TEST(ExecutionPool, ExecutionQueue_ExecutionPool_Serial)
{
    auto executionPool = std::make_shared<MockExecutionPool>();
    MockThreadWorkerFactory workerFactory {};
    
    //  Queue must 'register' itself in ExecutionPool when created
    execq::impl::ITaskProvider* registeredProvider = nullptr;
    EXPECT_CALL(*executionPool, addProvider(SaveArgAddress(&registeredProvider)))
    .WillOnce(::testing::Return());
    
    // Queue also creates additional single thread worker for its own needs
    std::unique_ptr<MockThreadWorker> additionalWorkerPtr(new MockThreadWorker{});
    MockThreadWorker& additionalWorker = *additionalWorkerPtr;
    EXPECT_CALL(workerFactory, createWorker(::testing::_))
    .WillOnce(::testing::Return(::testing::ByMove(std::move(additionalWorkerPtr))));
    
    
    // Create queue with mock execution function
    ::testing::MockFunction<void(const std::atomic_bool&, std::string&&)> mockExecutor;
    execq::impl::ExecutionQueue<void, std::string> queue(true, executionPool, workerFactory, mockExecutor.AsStdFunction());
    ASSERT_NE(registeredProvider, nullptr);
    
    
    // When new object comes to the queue, it notifies workers
    EXPECT_CALL(*executionPool, notifyOneWorker())
    .WillOnce(::testing::Return(true));
    queue.push("qwe");
    
    
    // The serial queue already has a task for execution, no reason to notify workers
    EXPECT_CALL(*executionPool, notifyOneWorker())
    .Times(0);
    EXPECT_CALL(additionalWorker, notifyWorker())
    .Times(0);
    queue.push("asd");
    
    
    // Test that executors method is called in the proper moment
    execq::impl::Task task = registeredProvider->nextTask();
    ASSERT_TRUE(task.valid());
    EXPECT_CALL(mockExecutor, Call(CompareWithAtomic(false), CompareRvalue("qwe")))
    .WillOnce(::testing::Return());
    
    // For serial queue, if the operation completes and there is any task to execute, it will notify workers to handle it
    EXPECT_CALL(*executionPool, notifyOneWorker())
    .WillOnce(::testing::Return(true));
    task();
    
    
    task = registeredProvider->nextTask();
    ASSERT_TRUE(task.valid());
    EXPECT_CALL(mockExecutor, Call(CompareWithAtomic(false), CompareRvalue("asd")))
    .WillOnce(::testing::Return());
    task();
    
    
    // No tasks - no execution
    EXPECT_FALSE(registeredProvider->nextTask().valid());
    
    
    //  Queue must 'unregister' itself in ExecutionPool when destroyed
    EXPECT_CALL(*executionPool, removeProvider(::testing::_))
    .WillOnce(::testing::Return());
}

TEST(ExecutionPool, ExecutionQueue_Cancelability)
{
    auto executionPool = std::make_shared<MockExecutionPool>();
    MockThreadWorkerFactory workerFactory {};
    
    // Assume worker pool always has free workers
    EXPECT_CALL(*executionPool, notifyOneWorker())
    .WillRepeatedly(::testing::Return(true));
    
    
    //  Queue must 'register' itself in ExecutionPool when created
    execq::impl::ITaskProvider* registeredProvider = nullptr;
    EXPECT_CALL(*executionPool, addProvider(SaveArgAddress(&registeredProvider)))
    .WillOnce(::testing::Return());
    
    
    // Queue also creates additional single thread worker for its own needs
    std::unique_ptr<MockThreadWorker> additionalWorkerPtr(new MockThreadWorker{});
    EXPECT_CALL(workerFactory, createWorker(::testing::_))
    .WillOnce(::testing::Return(::testing::ByMove(std::move(additionalWorkerPtr))));
    
    
    // Create queue with mock execution function
    ::testing::MockFunction<void(const std::atomic_bool&, std::string&&)> mockExecutor;
    execq::impl::ExecutionQueue<void, std::string> queue(false, executionPool, workerFactory, mockExecutor.AsStdFunction());
    ASSERT_NE(registeredProvider, nullptr);
    
    
    // When only objects pushed before 'cancel' call are really canceled
    queue.push("qwe");
    queue.cancel();
    queue.push("asd");
    
    
    EXPECT_CALL(mockExecutor, Call(CompareWithAtomic(true), CompareRvalue("qwe")))
    .WillOnce(::testing::Return());
    
    EXPECT_CALL(mockExecutor, Call(CompareWithAtomic(false), CompareRvalue("asd")))
    .WillOnce(::testing::Return());
    
    
    execq::impl::Task task = registeredProvider->nextTask();
    ASSERT_TRUE(task.valid());
    task();
    
    task = registeredProvider->nextTask();
    ASSERT_TRUE(task.valid());
    task();
    
    
    //  Queue must 'unregister' itself in ExecutionPool when destroyed
    EXPECT_CALL(*executionPool, removeProvider(::testing::_))
    .WillOnce(::testing::Return());
}
