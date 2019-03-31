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

#include "execq/IExecutionQueue.h"
#include "execq/internal/ITaskProvider.h"
#include "execq/internal/CancelTokenProvider.h"

#include <queue>
#include <mutex>
#include <thread>
#include <atomic>
#include <condition_variable>

namespace execq
{
    namespace details
    {
        class IExecutionQueueDelegate
        {
        public:
            virtual ~IExecutionQueueDelegate() = default;

            virtual void registerQueueTaskProvider(ITaskProvider& taskProvider) = 0;
            virtual void unregisterQueueTaskProvider(const details::ITaskProvider& taskProvider) = 0;
            virtual void queueDidReceiveNewTask() = 0;
        };
        
        template <typename T, typename R>
        class ExecutionQueue: public IExecutionQueue<R(T)>, private ITaskProvider
        {
        public:
            ExecutionQueue(const bool serial, IExecutionQueueDelegate& delegate, std::function<R(const std::atomic_bool& shouldQuit, T&& object)> executor);
            ~ExecutionQueue();
            
        public: // IExecutionQueue
            virtual void cancel() final;
            
        private: // IExecutionQueue
            virtual void pushImpl(std::unique_ptr<QueuedObject<T, R>> object) final;
            
        private: // ITaskProvider
            virtual Task nextTask() final;
            
        private:
            void execute(T&& object, std::promise<void>& promise, const std::atomic_bool& canceled);
            template <typename Y>
            void execute(T&& object, std::promise<Y>& promise, const std::atomic_bool& canceled);
            
            std::unique_ptr<QueuedObject<T, R>> popObject();
            void waitPendingTasks();
            
        private:
            size_t m_tasksRunningCount = 0;
            
            std::queue<std::unique_ptr<QueuedObject<T, R>>> m_taskQueue;
            std::mutex m_taskQueueMutex;
            std::condition_variable m_taskQueueCondition;
            CancelTokenProvider m_cancelTokenProvider;
            
            const bool m_isSerial;
            std::reference_wrapper<IExecutionQueueDelegate> m_delegate;
            std::function<R(const std::atomic_bool& shouldQuit, T&& object)> m_executor;
        };
    }
}

template <typename T, typename R>
execq::details::ExecutionQueue<T, R>::ExecutionQueue(const bool serial, IExecutionQueueDelegate& delegate,
                                                     std::function<R(const std::atomic_bool& shouldQuit, T&& object)> executor)
: m_isSerial(serial)
, m_delegate(delegate)
, m_executor(std::move(executor))
{
    m_delegate.get().registerQueueTaskProvider(*this);
}

template <typename T, typename R>
execq::details::ExecutionQueue<T, R>::~ExecutionQueue()
{
    m_cancelTokenProvider.cancel();
    waitPendingTasks();
    m_delegate.get().unregisterQueueTaskProvider(*this);
}

// IExecutionQueue

template <typename T, typename R>
void execq::details::ExecutionQueue<T, R>::cancel()
{
    m_cancelTokenProvider.cancelAndRenew();
}

template <typename T, typename R>
void execq::details::ExecutionQueue<T, R>::pushImpl(std::unique_ptr<QueuedObject<T, R>> object)
{
    std::lock_guard<std::mutex> lock(m_taskQueueMutex);
    
    m_taskQueue.push(std::move(object));
    m_delegate.get().queueDidReceiveNewTask();
}

// ITaskProvider

template <typename T, typename R>
execq::details::Task execq::details::ExecutionQueue<T, R>::nextTask()
{
    std::lock_guard<std::mutex> lock(m_taskQueueMutex);
    if (m_isSerial && m_tasksRunningCount > 0)
    {
        return Task();
    }
    
    std::unique_ptr<QueuedObject<T, R>> object = popObject();
    if (!object)
    {
        return Task();
    }
    
    m_tasksRunningCount++;
    
    
    std::shared_ptr<QueuedObject<T, R>> sharedObject = std::move(object);
    CancelToken cancelToken = m_cancelTokenProvider.token();
    return Task([this, sharedObject, cancelToken] {
        execute(std::move(sharedObject->object), sharedObject->promise, *cancelToken);
        
        std::lock_guard<std::mutex> lock(m_taskQueueMutex);
        m_tasksRunningCount--;
        m_taskQueueCondition.notify_one();
        if (m_isSerial && !m_taskQueue.empty())
        {
            m_delegate.get().queueDidReceiveNewTask();
        }
    });
}

// Private

template <typename T, typename R>
void execq::details::ExecutionQueue<T, R>::execute(T&& object, std::promise<void>& promise, const std::atomic_bool& canceled)
{
    m_executor(canceled, std::move(object));
    promise.set_value();
}

template <typename T, typename R>
template <typename Y>
void execq::details::ExecutionQueue<T, R>::execute(T&& object, std::promise<Y>& promise, const std::atomic_bool& canceled)
{
    promise.set_value(m_executor(canceled, std::move(object)));
}

template <typename T, typename R>
std::unique_ptr<execq::details::QueuedObject<T, R>> execq::details::ExecutionQueue<T, R>::popObject()
{
    if (m_taskQueue.empty())
    {
        return nullptr;
    }
    
    std::unique_ptr<QueuedObject<T, R>> object = std::move(m_taskQueue.front());
    m_taskQueue.pop();
    
    return object;
}

template <typename T, typename R>
void execq::details::ExecutionQueue<T, R>::waitPendingTasks()
{
    std::unique_lock<std::mutex> lock(m_taskQueueMutex);
    while (m_tasksRunningCount > 0 || !m_taskQueue.empty())
    {
        m_taskQueueCondition.wait(lock);
    }
}
