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
#include "execq/internal/ThreadWorkerPool.h"

#include <queue>

namespace execq
{
    namespace impl
    {
        template <typename T, typename R>
        struct QueuedObject
        {
            std::unique_ptr<T> object;
            std::promise<R> promise;
            CancelToken cancelToken;
        };
        
        template <typename T, typename R>
        class ExecutionQueue: public IExecutionQueue<R(T)>, private ITaskProvider
        {
        public:
            ExecutionQueue(const bool serial, std::shared_ptr<IThreadWorkerPool> workerPool,
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
            
            void pushObject(std::unique_ptr<QueuedObject<T, R>> object, bool& alreadyHasTask);
            std::unique_ptr<QueuedObject<T, R>> popObject();
            
            void notifyWorkers();
            bool hasTask();
            void waitAllTasks();
            
        private:
            std::atomic_size_t m_taskRunningCount { 0 };
            
            std::atomic_bool m_hasTask { false };
            std::queue<std::unique_ptr<QueuedObject<T, R>>> m_taskQueue;
            std::mutex m_taskQueueMutex;
            std::condition_variable m_taskQueueCondition;
            
            CancelTokenProvider m_cancelTokenProvider;
            
            const bool m_isSerial = false;
            const std::shared_ptr<IThreadWorkerPool> m_workerPool;
            const std::function<R(const std::atomic_bool& isCanceled, T&& object)> m_executor;
            
            const std::unique_ptr<IThreadWorker> m_additionalWorker;
        };
    }
}

template <typename T, typename R>
execq::impl::ExecutionQueue<T, R>::ExecutionQueue(const bool serial, std::shared_ptr<IThreadWorkerPool> workerPool,
                                                     std::function<R(const std::atomic_bool& shouldQuit, T&& object)> executor)
: m_isSerial(serial)
, m_workerPool(workerPool)
, m_executor(std::move(executor))
, m_additionalWorker(workerPool->createNewWorker(*this))
{
    m_workerPool->addProvider(*this);
}

template <typename T, typename R>
execq::impl::ExecutionQueue<T, R>::~ExecutionQueue()
{
    m_cancelTokenProvider.cancel();
    waitAllTasks();
    m_workerPool->removeProvider(*this);
}

// IExecutionQueue

template <typename T, typename R>
std::future<R> execq::impl::ExecutionQueue<T, R>::pushImpl(std::unique_ptr<T> object)
{
    using QueuedObject = QueuedObject<T, R>;
    
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

template <typename T, typename R>
void execq::impl::ExecutionQueue<T, R>::cancel()
{
    m_cancelTokenProvider.cancelAndRenew();
}

// IThreadWorkerPoolTaskProvider

template <typename T, typename R>
execq::impl::Task execq::impl::ExecutionQueue<T, R>::nextTask()
{
    if (!hasTask())
    {
        return Task();
    }
    
    m_taskRunningCount++;
    return Task([&] {
        std::unique_ptr<QueuedObject<T, R>> object = popObject();
        if (object)
        {
            execute(std::move(*object->object), object->promise, *object->cancelToken);
        }
        
        m_taskRunningCount--;
        if (m_taskRunningCount > 0)
        {
            return;
        }
        
        if (m_isSerial && m_hasTask)
        {
            notifyWorkers();
        }
        else
        {
            m_taskQueueCondition.notify_all();
        }
    });
}

// Private

template <typename T, typename R>
void execq::impl::ExecutionQueue<T, R>::execute(T&& object, std::promise<void>& promise, const std::atomic_bool& canceled)
{
    m_executor(canceled, std::move(object));
    promise.set_value();
}

template <typename T, typename R>
template <typename Y>
void execq::impl::ExecutionQueue<T, R>::execute(T&& object, std::promise<Y>& promise, const std::atomic_bool& canceled)
{
    promise.set_value(m_executor(canceled, std::move(object)));
}

template <typename T, typename R>
void execq::impl::ExecutionQueue<T, R>::pushObject(std::unique_ptr<QueuedObject<T, R>> object, bool& alreadyHasTask)
{
    std::lock_guard<std::mutex> lock(m_taskQueueMutex);
    
    alreadyHasTask = m_hasTask;
    m_hasTask = true;
    m_taskQueue.push(std::move(object));
}

template <typename T, typename R>
std::unique_ptr<execq::impl::QueuedObject<T, R>> execq::impl::ExecutionQueue<T, R>::popObject()
{
    std::lock_guard<std::mutex> lock(m_taskQueueMutex);
    if (m_taskQueue.empty())
    {
        return nullptr;
    }
    
    std::unique_ptr<QueuedObject<T, R>> object = std::move(m_taskQueue.front());
    m_taskQueue.pop();
    
    m_hasTask = !m_taskQueue.empty();
    
    return object;
}

template <typename T, typename R>
bool execq::impl::ExecutionQueue<T, R>::hasTask()
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

template <typename T, typename R>
void execq::impl::ExecutionQueue<T, R>::notifyWorkers()
{
    if (!m_workerPool->notifyOneWorker())
    {
        m_additionalWorker->notifyWorker();
    }
}

template <typename T, typename R>
void execq::impl::ExecutionQueue<T, R>::waitAllTasks()
{
    std::unique_lock<std::mutex> lock(m_taskQueueMutex);
    while (m_taskRunningCount > 0 || !m_taskQueue.empty())
    {
        m_taskQueueCondition.wait(lock);
    }
}
