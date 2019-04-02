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

#include "execq/internal/ITaskProvider.h"

#include <atomic>
#include <thread>
#include <condition_variable>

namespace execq
{
    namespace details
    {
        class IThreadWorkerDelegate
        {
        public:
            virtual ~IThreadWorkerDelegate() = default;
            
            virtual void workerDidFinishTask() = 0;
        };
        
        class ThreadWorker
        {
        public:
            explicit ThreadWorker(IThreadWorkerDelegate& delegate);
            ~ThreadWorker();
            
            bool startTask(details::Task&& task);
            
        private:
            void shutdown();
            Task waitTask(bool& shouldQuit);
            void threadMain();
            
        private:
            bool m_shouldQuit = false;
            std::atomic_bool m_busy { false };
            std::condition_variable m_condition;
            std::mutex m_mutex;
            std::thread m_thread;
            
            details::Task m_task;
            
            IThreadWorkerDelegate& m_delegate;
        };
    }
}


