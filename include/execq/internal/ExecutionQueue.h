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
#include "execq/internal/CancelTokenProvider.h"
#include "execq/internal/ExecutionPool.h"

#include <queue>

namespace execq
{
    namespace impl
    {
        template <typename R, typename T>
        struct QueuedObject
        {
            std::unique_ptr<T> object;
            std::promise<R> promise;
            CancelToken cancelToken;
        };
        
        template <typename R, typename T>
        class ExecutionQueue: public IExecutionQueue<R(T)>, private ITaskProvider
        {
        public:
            ExecutionQueue(const bool serial, std::shared_ptr<IExecutionPool> executionPool,
                           const IThreadWorkerFactory& workerFactory,
                           std::function<R(const std::atomic_bool& isCanceled, T&& object)> executor);
            ~ExecutionQueue();
            
        public: // IExecutionQueue
            virtual void cancel() final;
            
        private: // IExecutionQueue
            virtual std::future<R> pushImpl(std::unique_ptr<T> object) final;
            
        private: // IThreadWorkerPoolTaskProvider
            virtual Task nextTask() final;
            
        private:
            void execute(T&& object, std::promise<void>& promise, const std::atomic_bool& canceled);
            template <typename Y>
            void execute(T&& object, std::promise<Y>& promise, const std::atomic_bool& canceled);
            
            void pushObject(std::unique_ptr<QueuedObject<R, T>> object, bool& alreadyHasTask);
            std::unique_ptr<QueuedObject<R, T>> popObject();
            
            void notifyWorkers();
            bool hasTask();
            void waitAllTasks();
            
        private:
            std::atomic_size_t m_taskRunningCount { 0 };
            
            std::atomic_bool m_hasTask { false };
            std::queue<std::unique_ptr<QueuedObject<R, T>>> m_taskQueue;
            std::mutex m_taskQueueMutex;
            std::condition_variable m_taskQueueCondition;
            
            CancelTokenProvider m_cancelTokenProvider;
            
            const bool m_isSerial = false;
            const std::shared_ptr<IExecutionPool> m_executionPool;
            const std::function<R(const std::atomic_bool& isCanceled, T&& object)> m_executor;
            
            const std::unique_ptr<IThreadWorker> m_additionalWorker;
        };
    }
}

template <typename R, typename T>
execq::impl::ExecutionQueue<R, T>::ExecutionQueue(const bool serial, std::shared_ptr<IExecutionPool> executionPool,
                                                  const IThreadWorkerFactory& workerFactory,
                                                  std::function<R(const std::atomic_bool& shouldQuit, T&& object)> executor)
: m_isSerial(serial)
, m_executionPool(executionPool)
, m_executor(std::move(executor))
, m_additionalWorker(workerFactory.createWorker(*this))
{
    if (m_executionPool)
    {
        m_executionPool->addProvider(*this);
    }
}

template <typename R, typename T>
execq::impl::ExecutionQueue<R, T>::~ExecutionQueue()
{
    m_cancelTokenProvider.cancel();
    waitAllTasks();
    if (m_executionPool)
    {
        m_executionPool->removeProvider(*this);
    }
}

// IExecutionQueue

template <typename R, typename T>
std::future<R> execq::impl::ExecutionQueue<R, T>::pushImpl(std::unique_ptr<T> object)
{
    using QueuedObject = QueuedObject<R, T>;
    
    std::promise<R> promise;
    std::future<R> future = promise.get_future();
    
    std::unique_ptr<QueuedObject> queuedObject(new QueuedObject { std::move(object), std::move(promise), m_cancelTokenProvider.token() });
    
    bool alreadyHasTask = false;
    pushObject(std::move(queuedObject), alreadyHasTask);
    
    const bool shouldNotify = !m_isSerial || !alreadyHasTask;
    if (shouldNotify)
    {
        notifyWorkers();
    }
    
    return future;
}

template <typename R, typename T>
void execq::impl::ExecutionQueue<R, T>::cancel()
{
    m_cancelTokenProvider.cancelAndRenew();
}

// IThreadWorkerPoolTaskProvider

template <typename R, typename T>
execq::impl::Task execq::impl::ExecutionQueue<R, T>::nextTask()
{
    if (!hasTask())
    {
        return Task();
    }
    
    m_taskRunningCount++;
    return Task([&] {
        std::unique_ptr<QueuedObject<R, T>> object = popObject();
        if (object)
        {
            execute(std::move(*object->object), object->promise, *object->cancelToken);
        }
        
        if (--m_taskRunningCount > 0)
        {
            return;
        }
        
        if (!m_hasTask)
        {
            m_taskQueueCondition.notify_all();
        }
        else if (m_isSerial) // if there are more tasks and queue is serial, notify workers
        {
            notifyWorkers();
        }
    });
}

// Private

template <typename R, typename T>
void execq::impl::ExecutionQueue<R, T>::execute(T&& object, std::promise<void>& promise, const std::atomic_bool& canceled)
{
    m_executor(canceled, std::move(object));
    promise.set_value();
}

template <typename R, typename T>
template <typename Y>
void execq::impl::ExecutionQueue<R, T>::execute(T&& object, std::promise<Y>& promise, const std::atomic_bool& canceled)
{
    promise.set_value(m_executor(canceled, std::move(object)));
}

template <typename R, typename T>
void execq::impl::ExecutionQueue<R, T>::pushObject(std::unique_ptr<QueuedObject<R, T>> object, bool& alreadyHasTask)
{
    std::lock_guard<std::mutex> lock(m_taskQueueMutex);
    
    alreadyHasTask = m_hasTask;
    m_hasTask = true;
    m_taskQueue.push(std::move(object));
}

template <typename R, typename T>
std::unique_ptr<execq::impl::QueuedObject<R, T>> execq::impl::ExecutionQueue<R, T>::popObject()
{
    std::lock_guard<std::mutex> lock(m_taskQueueMutex);
    if (m_taskQueue.empty())
    {
        return nullptr;
    }
    
    std::unique_ptr<QueuedObject<R, T>> object = std::move(m_taskQueue.front());
    m_taskQueue.pop();
    
    m_hasTask = !m_taskQueue.empty();
    
    return object;
}

template <typename R, typename T>
bool execq::impl::ExecutionQueue<R, T>::hasTask()
{
    if (!m_hasTask)
    {
        return false;
    }
    
    if (!m_isSerial)
    {
        return true;
    }
    
    return !m_taskRunningCount;
}

template <typename R, typename T>
void execq::impl::ExecutionQueue<R, T>::notifyWorkers()
{
    if (!m_executionPool || !m_executionPool->notifyOneWorker())
    {
        m_additionalWorker->notifyWorker();
    }
}

template <typename R, typename T>
void execq::impl::ExecutionQueue<R, T>::waitAllTasks()
{
    std::unique_lock<std::mutex> lock(m_taskQueueMutex);
    while (m_taskRunningCount > 0 || !m_taskQueue.empty())
    {
        m_taskQueueCondition.wait(lock);
    }
}
