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

namespace execq
{
    namespace details
    {
        Task ReceiveNextTask(ITaskProvider& taskProvider, std::condition_variable& taskCondition, std::mutex& taskMutex, const std::atomic_bool& shouldStop)
        {
            Task task;
            while (true)
            {
                std::unique_lock<std::mutex> lock(taskMutex);
                
                task = taskProvider.nextTask();
                if (task.valid() || shouldStop)
                {
                    break;
                }
                
                taskCondition.wait(lock);
            }
            
            return task;
        }
        
        void WorkerThread(ITaskProvider& taskProvider, std::condition_variable& taskCondition, std::mutex& taskMutex, const std::atomic_bool& shouldStop)
        {
            while (true)
            {
                Task task = ReceiveNextTask(taskProvider, taskCondition, taskMutex, shouldStop);
                if (task.valid())
                {
                    task();
                }
                else if (shouldStop)
                {
                    break;
                }
            }
        }
    }
}
