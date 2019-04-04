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

#include "execq/internal/ExecutionStream.h"
#include "execq/internal/ExecutionQueue.h"
#include "execq/internal/TaskProviderList.h"
#include "execq/internal/ThreadWorker.h"

#include <map>
#include <list>
#include <vector>
#include <memory>

namespace execq
{
    /**
     * @class ExecutionPool
     * @brief ThreadPool-like object that provides concurrent task execution.
     * Tasks are provided by 'IExecutionQueue' ot 'IExecutionStream' instances.
     */
    class ExecutionPool
    {
    public:
        /**
         * @brief Creates pool that works as factory for execution queues and streams.
         * Usually you should create single instance of ExecutionPool with multiple IExecutionQueue/IExecutionStream to achive best performance.
         */
        ExecutionPool();
//        ~ExecutionPool();
        
        /**
         * @brief Creates execution queue with specific processing function.
         * @discussion All objects pushed into the queue will be processed on either one of pool threads or on the queue-specific thread.
         * @discussion Tasks in the queue run concurrently on available threads.
         */
        template <typename T, typename R>
        std::unique_ptr<IExecutionQueue<R(T)>> createConcurrentExecutionQueue(std::function<R(const std::atomic_bool& isCanceled, T&& object)> executor);
        
        /**
         * @brief Creates execution queue with specific processing function.
         * @discussion All objects pushed into the queue will be processed on either one of pool threads or on the queue-specific thread.
         * @discussion Tasks in the queue run in serial (one-after-one) order.
         */
        template <typename T, typename R>
        std::unique_ptr<IExecutionQueue<R(T)>> createSerialExecutionQueue(std::function<R(const std::atomic_bool& isCanceled, T&& object)> executor);
        
        /**
         * @brief Creates execution stream with specific executee function. Stream is stopped by default.
         * @discussion When stream started, 'executee' function will be called each time when ExecutionPool have free thread.
         */
        std::unique_ptr<IExecutionStream> createExecutionStream(std::function<void(const std::atomic_bool& isCanceled)> executee);
        
//    private: // details::IExecutionQueueDelegate
//        virtual void registerQueueTaskProvider(details::ITaskProvider& taskProvider) final;
//        virtual void unregisterQueueTaskProvider(const details::ITaskProvider& taskProvider) final;
//        virtual void queueDidReceiveNewTask() final;
        
//    private: // details::IExecutionStreamDelegate
//        virtual void registerStreamTaskProvider(details::ITaskProvider& taskProvider) final;
//        virtual void unregisterStreamTaskProvider(const details::ITaskProvider& taskProvider) final;
//        virtual void streamDidStart() final;
        
    private: // details::IThreadWorkerDelegate
//        bool execute() final;
//        virtual bool valid() const final;
//        virtual void workerDidFinishTask() final;
        
    private:
//        void registerTaskProvider(details::ITaskProvider& taskProvider);
//        void unregisterTaskProvider(const details::ITaskProvider& taskProvider);
//        void shutdown();
        
//        bool startTask();
        
    private:
        std::shared_ptr<details::ThreadWorkerPool> m_workerPool;
        
//        std::atomic_bool m_shouldQuit { false };
        
//        details::TaskProviderList m_taskProviders;
//        std::mutex m_providersMutex;
//        std::condition_variable m_providersCondition;
        
//        std::vector<std::unique_ptr<details::ThreadWorker>> m_workers;
//        std::map<const details::ITaskProvider*, std::shared_ptr<details::ThreadWorker>> m_additionalWorkers;
//        std::thread m_schedulerThread;
    };
}

template <typename T, typename R>
std::unique_ptr<execq::IExecutionQueue<R(T)>> execq::ExecutionPool::createConcurrentExecutionQueue(std::function<R(const std::atomic_bool& isCanceled, T&& object)> executor)
{
    return std::unique_ptr<details::ExecutionQueue<T, R>>(new details::ExecutionQueue<T, R>(false, m_workerPool, std::move(executor)));
}

template <typename T, typename R>
std::unique_ptr<execq::IExecutionQueue<R(T)>> execq::ExecutionPool::createSerialExecutionQueue(std::function<R(const std::atomic_bool& isCanceled, T&& object)> executor)
{
    return std::unique_ptr<details::ExecutionQueue<T, R>>(new details::ExecutionQueue<T, R>(true, m_workerPool, std::move(executor)));
}
