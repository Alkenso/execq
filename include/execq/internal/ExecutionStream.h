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

#include "execq/IExecutionStream.h"
#include "execq/internal/ITaskProvider.h"

#include <mutex>
#include <thread>
#include <atomic>
#include <condition_variable>

namespace execq
{
    namespace details
    {
        class IExecutionStreamDelegate
        {
        public:
            virtual ~IExecutionStreamDelegate() = default;

            virtual void registerStreamTaskProvider(details::ITaskProvider& taskProvider) = 0;
            virtual void unregisterStreamTaskProvider(const details::ITaskProvider& taskProvider) = 0;
            virtual void streamDidStart() = 0;
        };
        
        
        class ExecutionStream: public IExecutionStream, private ITaskProvider
        {
        public:
            ExecutionStream(IExecutionStreamDelegate& delegate, std::function<void(const std::atomic_bool& shouldQuit)> executee);
            ~ExecutionStream();
            
        public: // IExecutionStream
            virtual void start() final;
            virtual void stop() final;
            
        private: // ITaskProvider
            virtual Task nextTask() final;
            
        private:
            void streamThreadWorker();
            void waitPendingTasks();
            
        private:
            std::atomic_bool m_shouldQuit { false };
            std::atomic_bool m_started { false };
            
            std::atomic_size_t m_pendingTaskCount { 0 };
            std::mutex m_taskCompleteMutex;
            std::condition_variable m_taskCompleteCondition;
            
            std::mutex m_taskStartMutex;
            std::condition_variable m_taskStartCondition;
            
            std::reference_wrapper<IExecutionStreamDelegate> m_delegate;
            std::function<void(const std::atomic_bool& shouldQuit)> m_executee;
            
            std::future<void> m_thread;
        };
    }
}
