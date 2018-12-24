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

#include "ITaskProvider.h"

#include <list>
#include <mutex>

namespace execq
{
    namespace details
    {
        class TaskProviderList: public ITaskProvider
        {
        public:
            void add(ITaskProvider& taskProvider);
            void remove(const ITaskProvider& taskProvider);
            
        public: // ITaskProvider
            virtual Task nextTask() final;
            
        private:
            using TaskProviders_mt = std::list<ITaskProvider*>;
            TaskProviders_mt m_taskProviders;
            TaskProviders_mt::iterator m_currentTaskProviderIt;
        };
    }
}

void execq::details::TaskProviderList::add(ITaskProvider& taskProvider)
{
    m_taskProviders.push_back(&taskProvider);
    m_currentTaskProviderIt = m_taskProviders.begin();
}

void execq::details::TaskProviderList::remove(const ITaskProvider& taskProvider)
{
    const auto it = std::find(m_taskProviders.begin(), m_taskProviders.end(), &taskProvider);
    if (it != m_taskProviders.end())
    {
        m_taskProviders.erase(it);
        m_currentTaskProviderIt = m_taskProviders.begin();
    }
}

execq::details::Task execq::details::TaskProviderList::nextTask()
{
    execq::details::Task task;
    
    if (m_taskProviders.empty())
    {
        return task;
    }
    
    const size_t taskProvidersCount = m_taskProviders.size();
    const auto listEndIt = m_taskProviders.end();
    
    for (size_t i = 0; i < taskProvidersCount; i++)
    {
        if (m_currentTaskProviderIt == listEndIt)
        {
            m_currentTaskProviderIt = m_taskProviders.begin();
        }
        
        task = (*m_currentTaskProviderIt)->nextTask();
        
        m_currentTaskProviderIt++;
        
        if (task.valid())
        {
            break;
        }
    }
    
    return task;
}
