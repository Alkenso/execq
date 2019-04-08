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
#include "ExecutionStream.h"

namespace
{
    uint32_t GetOptimalThreadCount()
    {
        
        const uint32_t defaultThreadCount = 4;
        const uint32_t hardwareThreadCount = std::thread::hardware_concurrency();
        
        return hardwareThreadCount ? hardwareThreadCount : defaultThreadCount;
    }
    
    std::shared_ptr<execq::IExecutionPool> CreateDefaultExecutionPool(const uint32_t threadCount)
    {
        return std::make_shared<execq::impl::ExecutionPool>(threadCount, *execq::impl::IThreadWorkerFactory::defaultFactory());
    }
}

std::shared_ptr<execq::IExecutionPool> execq::CreateExecutionPool()
{
    return CreateDefaultExecutionPool(GetOptimalThreadCount());
}

std::shared_ptr<execq::IExecutionPool> execq::CreateExecutionPool(const uint32_t threadCount)
{
    if (!threadCount)
    {
        throw std::runtime_error("Failed to create IExecutionPool: thread count could not be zero.");
    }
    else if (threadCount == 1)
    {
        throw std::runtime_error("Failed to create IExecutionPool: for single-thread execution use pool-independent serial queue.");
    }
    
    return CreateDefaultExecutionPool(threadCount);
}

std::unique_ptr<execq::IExecutionStream> execq::CreateExecutionStream(std::shared_ptr<IExecutionPool> executionPool,
                                                                      std::function<void(const std::atomic_bool& isCanceled)> executee)
{
    return std::unique_ptr<impl::ExecutionStream>(new impl::ExecutionStream(executionPool,
                                                                            *impl::IThreadWorkerFactory::defaultFactory(),
                                                                            std::move(executee)));
}
