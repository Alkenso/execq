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

#include "internal/ExecutionStream.h"
#include "internal/ExecutionQueue.h"
#include "internal/TaskProviderList.h"
#include "internal/TaskProviderUtils.h"

#include <vector>

namespace execq
{
    /**
     * @class ExecutionPool
     * @brief ThreadPool-like object that provides concurrent task execution. Tasks are provided by 'ExecutionQueue' instances.
     */
    class ExecutionPool: private details::IExecutionQueueDelegate, private details::IExecutionStreamDelegate
    {
    public:
        /**
         * @brief Creates pool that works as factory for execution queues and streams.
         * Usually you should create single instance of ExecutionPool with multiple IExecutionQueue/IExecutionStream to achive best performance.
         */
        ExecutionPool();
        ~ExecutionPool();
        
        /**
         * @brief Creates execution queue with specific processing function.
         * @discussion All objects pushed into the queue will be processed on either one of pool threads or on the queue-specific thread.
         */
        template <typename T>
        std::unique_ptr<IExecutionQueue<T>> createExecutionQueue(std::function<void(const std::atomic_bool& shouldQuit, T object)> executor);
        
        /**
         * @brief Creates execution stream with specific executee function. Stream is stopped by default.
         * @discussion When stream started, 'executee' function will be called each time when ExecutionSource have free thread.
         */
        std::unique_ptr<IExecutionStream> createExecutionStream(std::function<void(const std::atomic_bool& shouldQuit)> executee);
        
    private: // details::IExecutionQueueDelegate
        virtual void registerQueueTaskProvider(details::ITaskProvider& taskProvider) final;
        virtual void unregisterQueueTaskProvider(const details::ITaskProvider& taskProvider) final;
        virtual void queueDidReceiveNewTask() final;
        
    private: // details::IExecutionStreamDelegate
        virtual void registerStreamTaskProvider(details::ITaskProvider& taskProvider) final;
        virtual void unregisterStreamTaskProvider(const details::ITaskProvider& taskProvider) final;
        virtual void streamDidStart() final;
        
    private:
        void registerTaskProvider(details::ITaskProvider& taskProvider);
        void unregisterTaskProvider(const details::ITaskProvider& taskProvider);
        void workerThread();
        void shutdown();
        
    private:
        std::atomic_bool m_shouldQuit { false };
        
        details::TaskProviderList m_taskProviders;
        std::mutex m_providersMutex;
        std::condition_variable m_providersCondition;
        
        std::vector<std::future<void>> m_threads;
    };
}

template <typename T>
std::unique_ptr<execq::IExecutionQueue<T>> execq::ExecutionPool::createExecutionQueue(std::function<void(const std::atomic_bool& shouldQuit, T object)> executor)
{
    return std::unique_ptr<details::ExecutionQueue<T>>(new details::ExecutionQueue<T>(*this, std::move(executor)));
}
