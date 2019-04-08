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

execq::impl::ExecutionPool::ExecutionPool(const uint32_t threadCount, const IThreadWorkerFactory& workerFactory)
{
    for (uint32_t i = 0; i < threadCount; i++)
    {
        m_workers.emplace_back(workerFactory.createWorker(m_providerGroup));
    }
}

void execq::impl::ExecutionPool::addProvider(ITaskProvider& provider)
{
    m_providerGroup.addProvider(provider);
}

void execq::impl::ExecutionPool::removeProvider(ITaskProvider& provider)
{
    m_providerGroup.removeProvider(provider);
}

bool execq::impl::ExecutionPool::notifyOneWorker()
{
    return details::NotifyWorkers(m_workers, true);
}

void execq::impl::ExecutionPool::notifyAllWorkers()
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
