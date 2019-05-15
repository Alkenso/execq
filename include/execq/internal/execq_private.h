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

#pragma once

#include "execq/internal/ExecutionQueue.h"

namespace execq
{
    namespace details
    {
        template <typename R>
        void ExecuteQueueTask(const std::atomic_bool& isCanceled, QueueTask<R>&& task)
        {
            if (task.valid())
            {
                task(isCanceled);
            }
        }
    }
}

template <typename T, typename R>
std::unique_ptr<execq::IExecutionQueue<R(T)>> execq::CreateConcurrentExecutionQueue(std::shared_ptr<IExecutionPool> executionPool,
                                                                                    std::function<R(const std::atomic_bool& isCanceled, T&& object)> executor)
{
    return std::unique_ptr<impl::ExecutionQueue<T, R>>(new impl::ExecutionQueue<T, R>(false,
                                                                                      executionPool,
                                                                                      *impl::IThreadWorkerFactory::defaultFactory(),
                                                                                      std::move(executor)));
}

template <typename T, typename R>
std::unique_ptr<execq::IExecutionQueue<R(T)>> execq::CreateSerialExecutionQueue(std::shared_ptr<IExecutionPool> executionPool,
                                                                                std::function<R(const std::atomic_bool& isCanceled, T&& object)> executor)
{
    return std::unique_ptr<impl::ExecutionQueue<T, R>>(new impl::ExecutionQueue<T, R>(false,
                                                                                      executionPool,
                                                                                      *impl::IThreadWorkerFactory::defaultFactory(),
                                                                                      std::move(executor)));
}

template <typename T, typename R>
std::unique_ptr<execq::IExecutionQueue<R(T)>> execq::CreateSerialExecutionQueue(std::function<R(const std::atomic_bool& isCanceled, T&& object)> executor)
{
    return std::unique_ptr<impl::ExecutionQueue<T, R>>(new impl::ExecutionQueue<T, R>(true,
                                                                                      nullptr,
                                                                                      *impl::IThreadWorkerFactory::defaultFactory(),
                                                                                      std::move(executor)));
}

template <typename R>
std::unique_ptr<execq::IExecutionQueue<void(execq::QueueTask<R>)>> execq::CreateConcurrentTaskExecutionQueue(std::shared_ptr<IExecutionPool> executionPool)
{
    return CreateConcurrentExecutionQueue<QueueTask<R>, void>(executionPool, &details::ExecuteQueueTask<R>);
}

template <typename R>
std::unique_ptr<execq::IExecutionQueue<void(execq::QueueTask<R>)>> execq::CreateSerialTaskExecutionQueue(std::shared_ptr<IExecutionPool> executionPool)
{
    return CreateSerialExecutionQueue<QueueTask<R>, void>(executionPool, &details::ExecuteQueueTask<R>);
}

template <typename R>
std::unique_ptr<execq::IExecutionQueue<void(execq::QueueTask<R>)>> execq::CreateSerialTaskExecutionQueue()
{
    return CreateSerialExecutionQueue<QueueTask<R>, void>(&details::ExecuteQueueTask<R>);
}
