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

#include "IExecutionQueue.h"
#include "TaskProviderUtils.h"

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
        
        
        template <typename T>
        class ExecutionQueue: public IExecutionQueue<T>, private ITaskProvider
        {
        public:
            ExecutionQueue(IExecutionQueueDelegate& delegate, std::function<void(const std::atomic_bool& shouldQuit, T object)> executor);
            ~ExecutionQueue();
            
        private: // IExecutionQueue
            virtual void pushImpl(std::unique_ptr<T> object) final;
            
        private: // ITaskProvider
            virtual Task nextTask() final;
            
        private:
            void queueThreadWorker();
            void waitPendingTasks();
            
        private:
            std::atomic_bool m_shouldQuit { false };
            
            std::atomic_size_t m_pendingTaskCount { 0 };
            std::mutex m_taskCompleteMutex;
            std::condition_variable m_taskCompleteCondition;
            
            std::queue<Task> m_taskQueue;
            std::mutex m_taskQueueMutex;
            std::condition_variable m_taskQueueCondition;
            
            std::reference_wrapper<IExecutionQueueDelegate> m_delegate;
            std::function<void(const std::atomic_bool& shouldQuit, T object)> m_executor;
            
            std::future<void> m_thread;
        };
    }
}

template <typename T>
execq::details::ExecutionQueue<T>::ExecutionQueue(IExecutionQueueDelegate& delegate, std::function<void(const std::atomic_bool& shouldQuit, T object)> executor)
: m_delegate(delegate)
, m_executor(std::move(executor))
, m_thread(std::async(std::launch::async, std::bind(&ExecutionQueue::queueThreadWorker, this)))
{
    m_delegate.get().registerQueueTaskProvider(*this);
}

template <typename T>
execq::details::ExecutionQueue<T>::~ExecutionQueue()
{
    m_shouldQuit = true;
    m_delegate.get().unregisterQueueTaskProvider(*this);
    m_taskQueueCondition.notify_all();
    waitPendingTasks();
}

// IExecutionQueue

template <typename T>
void execq::details::ExecutionQueue<T>::pushImpl(std::unique_ptr<T> object)
{
    m_pendingTaskCount++;
    
    std::shared_ptr<T> sharedObject = std::move(object);
    m_taskQueue.push(Task([this, sharedObject] () {
        m_executor(m_shouldQuit, std::move(*sharedObject));
        
        m_pendingTaskCount--;
        m_taskCompleteCondition.notify_one();
    }));
    
    m_delegate.get().queueDidReceiveNewTask();
    m_taskQueueCondition.notify_one();
}

// ITaskProvider

template <typename T>
execq::details::Task execq::details::ExecutionQueue<T>::nextTask()
{
    std::lock_guard<std::mutex> lock(m_taskQueueMutex);
    return PopTaskFromQueue(m_taskQueue);
}

// Private

template <typename T>
void execq::details::ExecutionQueue<T>::queueThreadWorker()
{
    class NonblockingTaskProvider: public ITaskProvider
    {
    public:
        explicit NonblockingTaskProvider(std::queue<Task>& taskQueue) : m_taskQueueRef(taskQueue) {}
        virtual Task nextTask() final { return PopTaskFromQueue(m_taskQueueRef); }
        
    private:
        std::queue<Task>& m_taskQueueRef;
    };
    
    NonblockingTaskProvider taskProvider(m_taskQueue);
    WorkerThread(taskProvider, m_taskQueueCondition, m_taskQueueMutex, m_shouldQuit);
}

template <typename T>
void execq::details::ExecutionQueue<T>::waitPendingTasks()
{
    std::unique_lock<std::mutex> lock(m_taskCompleteMutex);
    while (m_pendingTaskCount > 0)
    {
        m_taskCompleteCondition.wait(lock);
    }
}
