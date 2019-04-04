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

#include <list>
#include <mutex>
//
//namespace execq
//{
//    namespace details
//    {
//        struct StoredTask
//        {
//            Task task;
//            ITaskProvider& associatedProvider;
//        };
//
//        class TaskProviderList
//        {
//        public:
//            void add(ITaskProvider& taskProvider);
//            void remove(const ITaskProvider& taskProvider);
//
//            std::unique_ptr<StoredTask> nextTask();
//            ITaskProvider* nextProviderWithTask();
//
//        private:
//            using TaskProviders_lt = std::list<ITaskProvider*>;
//            TaskProviders_lt m_taskProviders;
//            TaskProviders_lt::iterator m_currentTaskProviderIt;
//            std::mutex m_mutex;
//        };
//
//
//        class TaskProviderList2
//        {
//        public:
//            void add(std::shared_ptr<ITaskProvider> taskProvider);
//            void remove(std::shared_ptr<ITaskProvider> taskProvider);
//
//            void invalidate();
//
//            virtual bool execute() final;
//            virtual bool valid() const final;
//
//        private:
//            std::shared_ptr<ITaskProvider> nextProviderWithTask();
//
//        private:
//            using TaskProviders_lt = std::list<std::shared_ptr<ITaskProvider>>;
//            TaskProviders_lt m_taskProviders;
//            TaskProviders_lt::iterator m_currentTaskProviderIt;
//            std::mutex m_mutex;
//            std::atomic_bool m_valid { true };
//        };
//    }
//}
