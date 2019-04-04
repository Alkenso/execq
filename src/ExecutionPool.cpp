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

//namespace
//{
//    std::unique_ptr<execq::details::ThreadWorker> CreateWorker(std::shared_ptr<execq::details::IThreadWorkerTaskProvider> provider)
//    {
//        return std::unique_ptr<execq::details::ThreadWorker>(new execq::details::ThreadWorker(provider));
//    }
//}

execq::ExecutionPool::ExecutionPool()
//: m_schedulerThread(&ExecutionPool::schedulerThread, this)
: m_workerPool(std::make_shared<details::ThreadWorkerPool>())
{
//    auto list = std::make_shared<details::TaskProviderList2>();
//
//    const uint32_t defaultThreadCount = 4;
//    const uint32_t hardwareThreadCount = std::thread::hardware_concurrency();
//
//    const uint32_t threadCount = hardwareThreadCount ? hardwareThreadCount : defaultThreadCount;
//    for (uint32_t i = 0; i < threadCount; i++)
//    {
//        m_workers.emplace_back(new execq::details::ThreadWorker(list));
//    }
}

//execq::ExecutionPool::~ExecutionPool()
//{
//    shutdown();
//    for (const auto& worker : m_workers)
//    {
//        worker->shutdown();
//    }
//}

//void execq::ExecutionPool::shutdown()
//{
//    std::lock_guard<std::mutex> lock(m_providersMutex);
//    m_shouldQuit = true;
////    m_providersCondition.notify_all();
//}

// Execution Queue

//void execq::ExecutionPool::registerQueueTaskProvider(details::ITaskProvider& taskProvider)
//{
//    registerTaskProvider(taskProvider);
//}
//
//void execq::ExecutionPool::unregisterQueueTaskProvider(const details::ITaskProvider& taskProvider)
//{
//    unregisterTaskProvider(taskProvider);
//}
//
//void execq::ExecutionPool::queueDidReceiveNewTask()
//{
//    if (startTask())
//    {
//        return;
//    }
//
//    m_additionalWorkers[&taskProvider]->startTask();
//
//    m_additionalWorkers
    
    //  try start on every pool thread
    //  if not started
    //      try start on per-provider thread
    //  endif
//
//    m_providersCondition.notify_one();
//}

//bool execq::ExecutionPool::execute()
//{
//    //  lock
//    //      find next provider with task
//    //  unlock
//    //
//    //  if found
//    //      provider->execute
//    //  endif
//    //
//    //  return found
//
//    std::unique_lock<std::mutex> lock(m_providersMutex);
//    details::ITaskProvider* provider = m_taskProviders.nextProviderWithTask();
//    if (!provider)
//    {
//        return false;
//    }
//
//    lock.unlock();
//
//    provider->execute();
//
//    return true;
//}

//bool execq::ExecutionPool::valid() const
//{
//    return !m_shouldQuit;
//}

// Execution Stream

//std::unique_ptr<execq::IExecutionStream> execq::ExecutionPool::createExecutionStream(std::function<void(const std::atomic_bool& isCanceled)> executee)
//{
//    return nullptr;//std::unique_ptr<details::ExecutionStream>(new details::ExecutionStream(*this, std::move(executee)));
//}
//
//void execq::ExecutionPool::registerStreamTaskProvider(details::ITaskProvider& taskProvider)
//{
//    registerTaskProvider(taskProvider);
//}
//
//void execq::ExecutionPool::unregisterStreamTaskProvider(const details::ITaskProvider& taskProvider)
//{
//    unregisterTaskProvider(taskProvider);
//}

//void execq::ExecutionPool::streamDidStart()
//{
////    m_providersCondition.notify_all();
//}
//
//void execq::ExecutionPool::workerDidFinishTask()
//{
////    m_providersCondition.notify_one();
//}

// Private

//void execq::ExecutionPool::registerTaskProvider(details::ITaskProvider& taskProvider)
//{
//    std::lock_guard<std::mutex> lock(m_providersMutex);
//    
//    m_taskProviders.add(taskProvider);
//    m_additionalWorkers.emplace(&taskProvider, CreateWorker(*this));
//}
//
//void execq::ExecutionPool::unregisterTaskProvider(const details::ITaskProvider& taskProvider)
//{
//    std::lock_guard<std::mutex> lock(m_providersMutex);
//    
//    m_taskProviders.remove(taskProvider);
//    m_additionalWorkers.erase(&taskProvider);
//}
//
///// Traverse thread workers and try to start the task on an of them
//bool execq::ExecutionPool::startTask()
//{
//    for (const auto& worker : m_workers)
//    {
//        if (worker->startTask())
//        {
//            return true;
//        }
//    }
//    
//    return false;
//}
