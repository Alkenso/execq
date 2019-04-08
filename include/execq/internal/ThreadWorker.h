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

#include <mutex>
#include <atomic>
#include <thread>
#include <future>
#include <condition_variable>

namespace execq
{
    namespace impl
    {
        using Task = std::packaged_task<void()>;
        class ITaskProvider
        {
        public:
            virtual ~ITaskProvider() = default;
            
            virtual Task nextTask() = 0;
        };
        
        
        class IThreadWorker
        {
        public:
            virtual ~IThreadWorker() = default;
            
            virtual bool notifyWorker() = 0;
        };
        
        
        class IThreadWorkerFactory
        {
        public:
            static std::shared_ptr<const IThreadWorkerFactory> defaultFactory();
            
            virtual ~IThreadWorkerFactory() = default;
            
            virtual std::unique_ptr<impl::IThreadWorker> createWorker(impl::ITaskProvider& provider) const = 0;
        };
    }
}


