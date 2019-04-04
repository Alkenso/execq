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
#include "execq/internal/ThreadWorker.h"

#include <queue>
#include <mutex>
#include <thread>
#include <atomic>
#include <condition_variable>

namespace execq
{
    namespace details
    {
        template <typename T, typename R>
        struct QueuedObject
        {
            std::unique_ptr<T> object;
            std::promise<R> promise;
            CancelToken cancelToken;
        };
        
        template <typename T, typename R>
        class ExecutionQueue: public IExecutionQueue<R(T)>, private IThreadWorkerPoolTaskProvider
        {
        public:
            ExecutionQueue(const bool serial, std::shared_ptr<ThreadWorkerPool> workerPool,
                           std::function<R(const std::atomic_bool& isCanceled, T&& object)> executor);
            ~ExecutionQueue();
            
        public: // IExecutionQueue
            virtual void cancel() final;
            
        private: // IExecutionQueue
            virtual std::future<R> pushImpl(std::unique_ptr<T> object) final;
            
        private: // IThreadWorkerPoolTaskProvider
            virtual bool execute() final;
            virtual bool hasTask() const final;
            
        private:
            void execute(T&& object, std::promise<void>& promise, const std::atomic_bool& canceled);
            template <typename Y>
            void execute(T&& object, std::promise<Y>& promise, const std::atomic_bool& canceled);
            
            std::unique_ptr<QueuedObject<T, R>> popObject();
            void waitAllTasks();
            
        private:
            std::atomic_size_t m_tasksRunningCount { 0 };
            
            std::atomic_bool m_hasTask { false };
            std::queue<std::unique_ptr<QueuedObject<T, R>>> m_taskQueue;
            std::mutex m_taskQueueMutex;
            std::condition_variable m_taskQueueCondition;
            CancelTokenProvider m_cancelTokenProvider;
            
            const bool m_isSerial = false;
            std::shared_ptr<ThreadWorkerPool> m_workerPool;
            std::function<R(const std::atomic_bool& isCanceled, T&& object)> m_executor;
            
            
            ThreadWorker m_additionalWorker;
        };
    }
}

template <typename T, typename R>
execq::details::ExecutionQueue<T, R>::ExecutionQueue(const bool serial, std::shared_ptr<ThreadWorkerPool> workerPool,
                                                     std::function<R(const std::atomic_bool& shouldQuit, T&& object)> executor)
: m_isSerial(serial)
, m_workerPool(workerPool)
, m_executor(std::move(executor))
, m_additionalWorker(*this)
{
    m_workerPool->addProvider(*this);
}

template <typename T, typename R>
execq::details::ExecutionQueue<T, R>::~ExecutionQueue()
{
    m_cancelTokenProvider.cancel();
    waitAllTasks();
    m_workerPool->removeProvider(*this);
}

// IExecutionQueue

template <typename T, typename R>
void execq::details::ExecutionQueue<T, R>::cancel()
{
    m_cancelTokenProvider.cancelAndRenew();
}

template <typename T, typename R>
std::future<R> execq::details::ExecutionQueue<T, R>::pushImpl(std::unique_ptr<T> object)
{
    using QueuedObject = details::QueuedObject<T, R>;
    
    std::promise<R> promise;
    std::future<R> future = promise.get_future();
    
    std::unique_ptr<QueuedObject> queuedObject(new QueuedObject { std::move(object), std::move(promise), m_cancelTokenProvider.token() });
    
    {
        std::lock_guard<std::mutex> lock(m_taskQueueMutex);
        m_hasTask = true;
        m_taskQueue.push(std::move(queuedObject));
    }
    
    if (!m_workerPool->startTask())
    {
        m_additionalWorker.startTask();
    }
    
    return future;
}

// ITaskProvider

//template <typename T, typename R>
//execq::details::Task execq::details::ExecutionQueue<T, R>::nextTask()
//{
//    std::lock_guard<std::mutex> lock(m_taskQueueMutex);
//    if (m_isSerial && m_tasksRunningCount > 0)
//    {
//        return Task();
//    }
//
//    std::unique_ptr<QueuedObject<T, R>> object = popObject();
//    if (!object)
//    {
//        return Task();
//    }
//
//    m_tasksRunningCount++;
//
//
//    std::shared_ptr<QueuedObject<T, R>> sharedObject = std::move(object);
//    return Task([this, sharedObject] {
//        execute(std::move(*sharedObject->object), sharedObject->promise, *sharedObject->cancelToken);
//
//        std::lock_guard<std::mutex> lock(m_taskQueueMutex);
//        m_tasksRunningCount--;
//        m_taskQueueCondition.notify_one();
//        if (m_isSerial && !m_taskQueue.empty())
//        {
//            m_delegate.get().queueDidReceiveNewTask();
//        }
//    });
//}


template <typename T, typename R>
bool execq::details::ExecutionQueue<T, R>::execute()
{
    if (!m_hasTask)
    {
        return false;
    }
    
    class RunningTaskCounterGuard
    {
    public:
        RunningTaskCounterGuard(std::atomic_size_t& tasksRunningCount, std::atomic_bool& hasTask, std::condition_variable& taskQueueCondition)
        : m_tasksRunningCount(tasksRunningCount)
        , m_hasTask(hasTask)
        , m_taskQueueCondition(taskQueueCondition)
        {
            m_tasksRunningCount++;
        }
        
        ~RunningTaskCounterGuard()
        {
            m_tasksRunningCount--;
            
            if (!m_hasTask && !m_tasksRunningCount)
            {
                m_taskQueueCondition.notify_one();
            }
        }
    private:
        std::atomic_size_t& m_tasksRunningCount;
        std::atomic_bool& m_hasTask;
        std::condition_variable& m_taskQueueCondition;
    };
    
    RunningTaskCounterGuard counterGuard(m_tasksRunningCount, m_hasTask, m_taskQueueCondition);
    std::unique_ptr<QueuedObject<T, R>> object = popObject();
    if (!object)
    {
        return false;
    }
    
    execute(std::move(*object->object), object->promise, *object->cancelToken);
    
    return true;
}

template <typename T, typename R>
bool execq::details::ExecutionQueue<T, R>::hasTask() const
{
    return m_hasTask;
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
void execq::details::ExecutionQueue<T, R>::waitAllTasks()
{
    std::unique_lock<std::mutex> lock(m_taskQueueMutex);
    while (m_tasksRunningCount > 0 || !m_taskQueue.empty())
    {
        m_taskQueueCondition.wait(lock);
    }
}
