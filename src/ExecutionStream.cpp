//
//  ExecutionStream.cpp
//  CoolThreadPool
//
//  Created by Alk on 12/25/18.
//  Copyright © 2018 alkenso. All rights reserved.
//

#include "ExecutionStream.h"

execq::details::ExecutionStream::ExecutionStream(IExecutionStreamDelegate& delegate, std::function<void(const std::atomic_bool& shouldQuit)> executee)
: m_delegate(delegate)
, m_executee(std::move(executee))
, m_thread(std::async(std::launch::async, std::bind(&ExecutionStream::streamThreadWorker, this)))
{
    m_delegate.get().registerStreamTaskProvider(*this);
}

execq::details::ExecutionStream::~ExecutionStream()
{
    stop();
    m_shouldQuit = true;
    m_delegate.get().unregisterStreamTaskProvider(*this);
    m_taskStartCondition.notify_all();
    waitPendingTasks();
}

// IExecutionStream

void execq::details::ExecutionStream::start()
{
    m_started = true;
    m_delegate.get().streamDidStart();
    m_taskStartCondition.notify_one();
}

void execq::details::ExecutionStream::stop()
{
    m_started = false;
}

// ITaskProvider

execq::details::Task execq::details::ExecutionStream::nextTask()
{
    if (!m_started)
    {
        return Task();
    }
    
    m_pendingTaskCount++;
    
    return Task([this] () {
        m_executee(m_shouldQuit);
        
        m_pendingTaskCount--;
        m_taskCompleteCondition.notify_one();
    });
}

// Private

void execq::details::ExecutionStream::streamThreadWorker()
{
    WorkerThread(*this, m_taskStartCondition, m_taskStartMutex, m_shouldQuit);
}

void execq::details::ExecutionStream::waitPendingTasks()
{
    std::unique_lock<std::mutex> lock(m_taskCompleteMutex);
    while (m_pendingTaskCount > 0)
    {
        m_taskCompleteCondition.wait(lock);
    }
}
