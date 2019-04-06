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

execq::impl::ThreadWorkerPool::ThreadWorkerPool()
{
    const uint32_t defaultThreadCount = 4;
    const uint32_t hardwareThreadCount = std::thread::hardware_concurrency();
    
    const uint32_t threadCount = hardwareThreadCount ? hardwareThreadCount : defaultThreadCount;
    for (uint32_t i = 0; i < threadCount; i++)
    {
        m_workers.emplace_back(createNewWorker(m_providerGroup));
    }
}

std::unique_ptr<execq::impl::IThreadWorker> execq::impl::ThreadWorkerPool::createNewWorker(ITaskProvider& provider) const
{
    return std::unique_ptr<ThreadWorker>(new ThreadWorker(provider));
}

void execq::impl::ThreadWorkerPool::addProvider(ITaskProvider& provider)
{
    m_providerGroup.addProvider(provider);
}

void execq::impl::ThreadWorkerPool::removeProvider(ITaskProvider& provider)
{
    m_providerGroup.removeProvider(provider);
}

bool execq::impl::ThreadWorkerPool::notifyOneWorker()
{
    return details::NotifyWorkers(m_workers, true);
}

void execq::impl::ThreadWorkerPool::notifyAllWorkers()
{
    details::NotifyWorkers(m_workers, false);
}

// Details

bool execq::impl::details::NotifyWorkers(const std::vector<std::unique_ptr<IThreadWorker>>& workers, const bool single)
{
    bool notified = false;
    for (const auto& worker : workers)
    {
        notified |= worker->notifyWorker();
        if (notified && single)
        {
            return true;
        }
    }
    
    return notified;
}
