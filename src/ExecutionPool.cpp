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

#include "ExecutionPool.h"

execq::ExecutionPool::ExecutionPool()
{
    const uint32_t defaultThreadCount = 4;
    const uint32_t hardwareThreadCount = std::thread::hardware_concurrency();
    
    const uint32_t threadCount = hardwareThreadCount ? hardwareThreadCount : defaultThreadCount;
    for (uint32_t i = 0; i < threadCount; i++)
    {
        m_threads.emplace_back(std::async(std::launch::async, std::bind(&ExecutionPool::workerThread, this)));
    }
}

execq::ExecutionPool::~ExecutionPool()
{
    shutdown();
}

void execq::ExecutionPool::shutdown()
{
    std::lock_guard<std::mutex> lock(m_providersMutex);
    m_shouldQuit = true;
    m_providersCondition.notify_all();
}

// Execution Queue

void execq::ExecutionPool::registerQueueTaskProvider(details::ITaskProvider& taskProvider)
{
    registerTaskProvider(taskProvider);
}

void execq::ExecutionPool::unregisterQueueTaskProvider(const details::ITaskProvider& taskProvider)
{
    unregisterTaskProvider(taskProvider);
}

void execq::ExecutionPool::queueDidReceiveNewTask()
{
    m_providersCondition.notify_one();
}

// Execution Stream

std::unique_ptr<execq::IExecutionStream> execq::ExecutionPool::createExecutionStream(std::function<void(const std::atomic_bool& shouldQuit)> executee)
{
    return std::unique_ptr<details::ExecutionStream>(new details::ExecutionStream(*this, std::move(executee)));
}

void execq::ExecutionPool::registerStreamTaskProvider(details::ITaskProvider& taskProvider)
{
    registerTaskProvider(taskProvider);
}

void execq::ExecutionPool::unregisterStreamTaskProvider(const details::ITaskProvider& taskProvider)
{
    unregisterTaskProvider(taskProvider);
}

void execq::ExecutionPool::streamDidStart()
{
    m_providersCondition.notify_all();
}

// Private

void execq::ExecutionPool::registerTaskProvider(details::ITaskProvider& taskProvider)
{
    std::lock_guard<std::mutex> lock(m_providersMutex);
    
    m_taskProviders.add(taskProvider);
}

void execq::ExecutionPool::unregisterTaskProvider(const details::ITaskProvider& taskProvider)
{
    std::lock_guard<std::mutex> lock(m_providersMutex);
    
    m_taskProviders.remove(taskProvider);
}

void execq::ExecutionPool::workerThread()
{
    WorkerThread(m_taskProviders, m_providersCondition, m_providersMutex, m_shouldQuit);
}
