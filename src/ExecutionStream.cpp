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

#include "ExecutionStream.h"

execq::details::ExecutionStream::ExecutionStream(IExecutionStreamDelegate& delegate, std::function<void(const std::atomic_bool& isCanceled)> executee)
: m_delegate(delegate)
, m_executee(std::move(executee))
{
    m_delegate.get().registerStreamTaskProvider(*this);
}

execq::details::ExecutionStream::~ExecutionStream()
{
    stop();
    m_shouldQuit = true;
    m_taskStartCondition.notify_all();
    waitPendingTasks();
    m_delegate.get().unregisterStreamTaskProvider(*this);
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
    
    std::lock_guard<std::mutex> lock(m_taskCompleteMutex);
    m_tasksRunningCount++;
    
    return Task([this] () {
        m_executee(m_shouldQuit);
        
        std::lock_guard<std::mutex> lock(m_taskCompleteMutex);
        m_tasksRunningCount--;
        m_taskCompleteCondition.notify_one();
    });
}

// Private

void execq::details::ExecutionStream::waitPendingTasks()
{
    std::unique_lock<std::mutex> lock(m_taskCompleteMutex);
    while (m_tasksRunningCount > 0)
    {
        m_taskCompleteCondition.wait(lock);
    }
}
