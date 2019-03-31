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

#include "ThreadWorker.h"

execq::details::ThreadWorker::ThreadWorker(IThreadWorkerDelegate& delegate)
: m_delegate(delegate)
{}

execq::details::ThreadWorker::~ThreadWorker()
{
    shutdown();
    if (m_thread.joinable())
    {
        m_thread.join();
    }
}

bool execq::details::ThreadWorker::startTask(details::Task&& task)
{
    const bool alreadyBusy = m_busy.exchange(true);
    if (alreadyBusy)
    {
        return false;
    }
    
    if (!m_thread.joinable())
    {
        m_thread = std::thread(&ThreadWorker::threadMain, this);
    }
    
    std::lock_guard<std::mutex> lock(m_mutex);
    m_task = std::move(task);
    m_condition.notify_one();
    
    return true;
}

void execq::details::ThreadWorker::shutdown()
{
    std::lock_guard<std::mutex> lock(m_mutex);
    m_shouldQuit = true;
    m_condition.notify_one();
}

execq::details::Task execq::details::ThreadWorker::waitTask(bool& shouldQuit)
{
    while (true)
    {
        std::unique_lock<std::mutex> lock(m_mutex);
        if (m_task.valid())
        {
            Task task = std::move(m_task);
            m_task = Task();
            
            return task;
        }
        else if (m_shouldQuit)
        {
            shouldQuit = true;
            break;
        }
        
        m_condition.wait(lock);
    }
    
    return Task();
}

void execq::details::ThreadWorker::threadMain()
{
    while (true)
    {
        bool shouldQuit = false;
        Task task = waitTask(shouldQuit);
        if (task.valid())
        {
            task();
            
            m_busy = false;
            m_delegate.workerDidFinishTask();
        }
        else if (shouldQuit)
        {
            break;
        }
    }
}

