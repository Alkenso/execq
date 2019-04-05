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

#include "execq/internal/ThreadWorker.h"

#include <mutex>
#include <list>

namespace execq
{
    namespace impl
    {
        class ITaskProvider: public ITaskExecutor
        {
        public:
            virtual bool hasTask() const = 0;
        };
        
        
        class TaskProviderGroup: public ITaskExecutor
        {
        public: // IThreadWorkerTaskProvider
            virtual bool execute() final;
            
        public:
            void addProvider(ITaskProvider& provider);
            void removeProvider(ITaskProvider& provider);
            
        private:
            ITaskProvider* nextProviderWithTask();
            
        private:
            using TaskProviders_lt = std::list<ITaskProvider*>;
            TaskProviders_lt m_taskProviders;
            TaskProviders_lt::iterator m_currentTaskProviderIt;
            std::mutex m_mutex;
        };
    }
}


