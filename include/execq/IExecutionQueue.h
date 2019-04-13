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
#include <future>

namespace execq
{
    template <typename Unused>
    class IExecutionQueue;
    
    /**
     * @class IExecutionQueue
     * @brief High-level interface that provides access to queue-based tasks execution.
     * @templatefield T Type of the object to be processed on the queue.
     * @templatefield R Type of the result of object processing. Can be 'void'.
     */
    template <typename T, typename R>
    class IExecutionQueue <R(T)>
    {
    public:
        virtual ~IExecutionQueue() = default;
        
        /**
         * @brief Pushes-by-copy an object to be processed on the queue.
         * @discussion You can freely ignore return value: it would not block in future's destructor.
         * @return Future object to obtain result when the task is done.
         */
        std::future<R> push(const T& object);
        
        /**
         * @brief Pushes-by-move an object to be processed on the queue.
         * @discussion You can freely ignore return value: it would not block in future's destructor.
         * @return Future object to obtain result when the task is done.
         */
        std::future<R> push(T&& object);
        
        /**
         * @brief Emplaces an object to be processed on the queue.
         * @discussion You can freely ignore return value: it would not block in future's destructor.
         * @return Future object to obtain result when the task is done.
         */
        template <typename... Args>
        std::future<R> emplace(Args&&... args);
        
        /**
         * @brief Makrs all tasks as canceled.
         * @discussion Be aware that new tasks added after 'cancel' call will not be marked as 'canceled'.
         */
        virtual void cancel() = 0;
        
    private:
        virtual std::future<R> pushImpl(std::unique_ptr<T> object) = 0;
    };
}

template <typename T, typename R>
std::future<R> execq::IExecutionQueue<R(T)>::push(const T& object)
{
    return pushImpl(std::unique_ptr<T>(new T { object }));
}

template <typename T, typename R>
std::future<R> execq::IExecutionQueue<R(T)>::push(T&& object)
{
    return pushImpl(std::unique_ptr<T>(new T { std::move(object) }));
}

template <typename T, typename R>
template <typename... Args>
std::future<R> execq::IExecutionQueue<R(T)>::emplace(Args&&... args)
{
    return pushImpl(std::unique_ptr<T>(new T { std::forward<Args>(args)... }));
}
