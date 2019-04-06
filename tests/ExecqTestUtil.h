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

#include "ThreadWorkerPool.h"

#include <gmock/gmock.h>

namespace execq
{
    namespace test
    {
        class MockThreadWorkerPool: public execq::impl::IThreadWorkerPool
        {
        public:
            MOCK_CONST_METHOD1(createNewWorker, std::unique_ptr<execq::impl::IThreadWorker>(execq::impl::ITaskProvider& provider));
            
            MOCK_METHOD1(addProvider, void(execq::impl::ITaskProvider& provider));
            MOCK_METHOD1(removeProvider, void(execq::impl::ITaskProvider& provider));
            
            MOCK_METHOD0(notifyOneWorker, bool());
            MOCK_METHOD0(notifyAllWorkers, void());
        };
        
        class MockThreadWorker: public execq::impl::IThreadWorker
        {
        public:
            MOCK_METHOD0(notifyWorker, bool());
        };
        
        static const std::chrono::milliseconds kLongTermJob { 100 };
        static const std::chrono::milliseconds kTimeout { 500 };
        
        inline void WaitForLongTermJob()
        {
            std::this_thread::sleep_for(kLongTermJob);
        }
        
        MATCHER_P(CompareRvalue, value, "")
        {
            return value == arg;
        }
        
        MATCHER_P(CompareWithAtomic, value, "")
        {
            return value == arg;
        }
        
        MATCHER_P(SaveArgAddress, value, "")
        {
            *value = &arg;
            return true;
        }
    }
}
