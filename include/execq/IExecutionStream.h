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

#include <memory>

namespace execq
{
    /**
     * @class IExecutionStream
     * @brief High-level interface that provides access to stream-based tasks execution.
     *
     * @discussion ExecutionStream executes tasks simultaneously in the most efficient way
     * on all available Pool threads every time the thread lacks of work.
     *
     * @discussion Stream could be used when number of tasks is unknown.
     * It could be such cases like filesystem traverse: number of files is not determined, but you want to process
     * all of them in the most efficient way.
     */
    class IExecutionStream
    {
    public:
        virtual ~IExecutionStream() = default;
        
        /**
         * @brief Starts execution stream.
         * Each time when thread in pool becomes free, execution stream will be prompted of next task to execute.
         */
        virtual void start() = 0;
        
        /**
         * @brief Stops execution stream.
         * Execution stream will not be prompted of next tasks to execute until 'start' is called.
         * All tasks being executed during stop will normally continue.
         */
        virtual void stop() = 0;
    };
}
