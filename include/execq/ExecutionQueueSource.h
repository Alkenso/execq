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

#include "internal/ExecutionQueue.h"
#include "internal/TaskProviderList.h"
#include "internal/TaskProviderUtils.h"

#include <vector>

namespace execq
{
    /**
     * @class ExecutionQueueSource
     * @brief ThreadPool-like object that provides concurrent task execution. Tasks are provided by 'ExecutionQueue' instances.
     */
    class ExecutionQueueSource: private details::IExecutionQueueDelegate
    {
    public:
        /**
         * @brief Creates pool that works as factory for execution queues.
         * Usually you should create single instance of ExecutionQueueSource with multiple IExecutionQueue's to achive best performance.
         */
        ExecutionQueueSource();
        ~ExecutionQueueSource();
        
        /**
         * @brief Creates execution queue with specific processing function.
         * @discussion All objects pushed into the queue will be processed on either one of pool threads or on the queue-specific thread.
         */
        template <typename T>
        std::unique_ptr<IExecutionQueue<T>> createExecutionQueue(std::function<void(const std::atomic_bool& shouldStop, T object)> executor);
        
    private: // details::IExecutionQueueDelegate
        virtual void registerTaskProvider(details::ITaskProvider& taskProvider) final;
        virtual void unregisterTaskProvider(const details::ITaskProvider& taskProvider) final;
        virtual void taskProviderDidReceiveNewTask() final;
        
    private:
        void workerThread();
        void shutdown();
        
    private:
        std::atomic_bool m_shouldStop { false };
        
        details::TaskProviderList m_taskProviders;
        std::mutex m_providersMutex;
        std::condition_variable m_providersCondition;
        
        std::vector<std::future<void>> m_threads;
    };
}

execq::ExecutionQueueSource::ExecutionQueueSource()
{
    const uint32_t defaultThreadCount = 4;
    const uint32_t hardwareThreadCount = std::thread::hardware_concurrency();
    
    const uint32_t threadCount = hardwareThreadCount ? hardwareThreadCount : defaultThreadCount;
    for (uint32_t i = 0; i < threadCount; i++)
    {
        m_threads.emplace_back(std::async(std::launch::async, std::bind(&ExecutionQueueSource::workerThread, this)));
    }
}

execq::ExecutionQueueSource::~ExecutionQueueSource()
{
    shutdown();
}

void execq::ExecutionQueueSource::shutdown()
{
    std::lock_guard<std::mutex> lock(m_providersMutex);
    m_shouldStop = true;
    m_providersCondition.notify_all();
}

template <typename T>
std::unique_ptr<execq::IExecutionQueue<T>> execq::ExecutionQueueSource::createExecutionQueue(std::function<void(const std::atomic_bool& shouldStop, T object)> executor)
{
    return std::unique_ptr<details::ExecutionQueue<T>>(new details::ExecutionQueue<T>(*this, std::move(executor)));
}

void execq::ExecutionQueueSource::registerTaskProvider(details::ITaskProvider& taskProvider)
{
    std::lock_guard<std::mutex> lock(m_providersMutex);
    
    m_taskProviders.add(taskProvider);
}

void execq::ExecutionQueueSource::unregisterTaskProvider(const details::ITaskProvider& taskProvider)
{
    std::lock_guard<std::mutex> lock(m_providersMutex);
    
    m_taskProviders.remove(taskProvider);
}

void execq::ExecutionQueueSource::taskProviderDidReceiveNewTask()
{
    m_providersCondition.notify_one();
}

void execq::ExecutionQueueSource::workerThread()
{
    WorkerThread(m_taskProviders, m_providersCondition, m_providersMutex, m_shouldStop);
}
