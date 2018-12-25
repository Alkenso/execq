//
//  TaskProviderUtils.cpp
//  CoolThreadPool
//
//  Created by Alk on 12/25/18.
//  Copyright © 2018 alkenso. All rights reserved.
//

#include "TaskProviderUtils.h"

namespace
{
    execq::details::Task ReceiveNextTask(execq::details::ITaskProvider& taskProvider, std::condition_variable& taskCondition,
                                         std::mutex& taskMutex, const std::atomic_bool& shouldQuit)
    {
        execq::details::Task task;
        while (true)
        {
            std::unique_lock<std::mutex> lock(taskMutex);
            
            task = taskProvider.nextTask();
            if (task.valid() || shouldQuit)
            {
                break;
            }
            
            taskCondition.wait(lock);
        }
        
        return task;
    }
}

void execq::details::WorkerThread(ITaskProvider& taskProvider, std::condition_variable& taskCondition, std::mutex& taskMutex, const std::atomic_bool& shouldQuit)
{
    while (true)
    {
        Task task = ReceiveNextTask(taskProvider, taskCondition, taskMutex, shouldQuit);
        if (task.valid())
        {
            task();
        }
        else if (shouldQuit)
        {
            break;
        }
    }
}

execq::details::Task execq::details::PopTaskFromQueue(std::queue<Task>& queue)
{
    if (queue.empty())
    {
        return Task();
    }
    
    Task task = std::move(queue.front());
    queue.pop();
    
    return task;
}
