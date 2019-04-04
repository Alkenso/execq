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

#include "ThreadWorkerPool.h"

namespace
{
    bool NotifyWorkers(const std::vector<std::unique_ptr<execq::details::IThreadWorker>>& workers, const bool single)
    {
        for (const auto& worker : workers)
        {
            if (worker->notifyWorker() && single)
            {
                return true;
            }
        }
        
        return false;
    }
}

execq::details::ThreadWorkerPool::ThreadWorkerPool()
{
    const uint32_t defaultThreadCount = 4;
    const uint32_t hardwareThreadCount = std::thread::hardware_concurrency();
    
    const uint32_t threadCount = hardwareThreadCount ? hardwareThreadCount : defaultThreadCount;
    for (uint32_t i = 0; i < threadCount; i++)
    {
        m_workers.emplace_back(createNewWorker(*this));
    }
}

std::unique_ptr<execq::details::IThreadWorker> execq::details::ThreadWorkerPool::createNewWorker(IThreadWorkerTaskProvider& provider) const
{
    return std::unique_ptr<ThreadWorker>(new ThreadWorker(provider));
}

void execq::details::ThreadWorkerPool::addProvider(IThreadWorkerPoolTaskProvider& provider)
{
    std::lock_guard<std::mutex> lock(m_mutex);
    m_taskProviders.push_back(&provider);
    m_currentTaskProviderIt = m_taskProviders.begin();
}

void execq::details::ThreadWorkerPool::removeProvider(IThreadWorkerPoolTaskProvider& provider)
{
    std::lock_guard<std::mutex> lock(m_mutex);
    const auto it = std::find(m_taskProviders.begin(), m_taskProviders.end(), &provider);
    if (it != m_taskProviders.end())
    {
        m_taskProviders.erase(it);
        m_currentTaskProviderIt = m_taskProviders.begin();
    }
}

execq::details::IThreadWorkerPoolTaskProvider* execq::details::ThreadWorkerPool::nextProviderWithTask()
{
    std::lock_guard<std::mutex> lock(m_mutex);
    
    const size_t taskProvidersCount = m_taskProviders.size();
    const auto listEndIt = m_taskProviders.end();
    
    for (size_t i = 0; i < taskProvidersCount; i++)
    {
        if (m_currentTaskProviderIt == listEndIt)
        {
            m_currentTaskProviderIt = m_taskProviders.begin();
        }
        
        IThreadWorkerPoolTaskProvider* provider = *(m_currentTaskProviderIt++);
        if (provider->hasTask())
        {
            return provider;
        }
    }
    
    return nullptr;
}

bool execq::details::ThreadWorkerPool::notifyOneWorker()
{
    return NotifyWorkers(m_workers, true);
}

void execq::details::ThreadWorkerPool::notifyAllWorkers()
{
    NotifyWorkers(m_workers, false);
}

bool execq::details::ThreadWorkerPool::execute()
{
    IThreadWorkerPoolTaskProvider *const provider = nextProviderWithTask();
    if (provider)
    {
        return provider->execute();
    }
    
    return false;
}
