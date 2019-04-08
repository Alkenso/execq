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

#include "execq/internal/TaskProviderList.h"

#include <atomic>
#include <memory>
#include <vector>

namespace execq
{
    class IExecutionPool
    {
    public:
        virtual ~IExecutionPool() = default;
        
        virtual void addProvider(impl::ITaskProvider& provider) = 0;
        virtual void removeProvider(impl::ITaskProvider& provider) = 0;
        
        virtual bool notifyOneWorker() = 0;
        virtual void notifyAllWorkers() = 0;
    };
    
    namespace impl
    {
        class ExecutionPool: public IExecutionPool
        {
        public:
            ExecutionPool(const uint32_t threadCount, const IThreadWorkerFactory& workerFactory);
            
            virtual void addProvider(ITaskProvider& provider) final;
            virtual void removeProvider(ITaskProvider& provider) final;
            
            virtual bool notifyOneWorker() final;
            virtual void notifyAllWorkers() final;
            
        private:
            std::atomic_bool m_valid { true };
            TaskProviderList m_providerGroup;
            
            std::vector<std::unique_ptr<IThreadWorker>> m_workers;
        };
        
        
        namespace details
        {
            bool NotifyWorkers(const std::vector<std::unique_ptr<IThreadWorker>>& workers, const bool single);
        }
    }
}


