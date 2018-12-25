//
//  TaskProviderList.cpp
//  CoolThreadPool
//
//  Created by Alk on 12/25/18.
//  Copyright © 2018 alkenso. All rights reserved.
//

#include "TaskProviderList.h"

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
