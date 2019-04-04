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

#include "TaskProviderList.h"
//
//void execq::details::TaskProviderList::add(ITaskProvider& taskProvider)
//{
//    std::lock_guard<std::mutex> lock(m_mutex);
//    m_taskProviders.push_back(&taskProvider);
//    m_currentTaskProviderIt = m_taskProviders.begin();
//}
//
//void execq::details::TaskProviderList::remove(const ITaskProvider& taskProvider)
//{
//    std::lock_guard<std::mutex> lock(m_mutex);
//    const auto it = std::find(m_taskProviders.begin(), m_taskProviders.end(), &taskProvider);
//    if (it != m_taskProviders.end())
//    {
//        m_taskProviders.erase(it);
//        m_currentTaskProviderIt = m_taskProviders.begin();
//    }
//}
//
//std::unique_ptr<execq::details::StoredTask> execq::details::TaskProviderList::nextTask()
//{
//    const size_t taskProvidersCount = m_taskProviders.size();
//    const auto listEndIt = m_taskProviders.end();
//
//    for (size_t i = 0; i < taskProvidersCount; i++)
//    {
//        if (m_currentTaskProviderIt == listEndIt)
//        {
//            m_currentTaskProviderIt = m_taskProviders.begin();
//        }
//
//        ITaskProvider* provider = *m_currentTaskProviderIt;
//        Task task = provider->nextTask();
//
//        m_currentTaskProviderIt++;
//
//        if (task.valid())
//        {
//            return std::unique_ptr<StoredTask>(new StoredTask { std::move(task), *provider });
//        }
//    }
//
//    return nullptr;
//}
//
//execq::details::ITaskProvider* execq::details::TaskProviderList::nextProviderWithTask()
//{
//    std::lock_guard<std::mutex> lock(m_mutex);
//
//    const size_t taskProvidersCount = m_taskProviders.size();
//    const auto listEndIt = m_taskProviders.end();
//
//    for (size_t i = 0; i < taskProvidersCount; i++)
//    {
//        if (m_currentTaskProviderIt == listEndIt)
//        {
//            m_currentTaskProviderIt = m_taskProviders.begin();
//        }
//
//        ITaskProvider* provider = *(m_currentTaskProviderIt++);
//        if (provider->hasTask())
//        {
//            return provider;
//        }
//    }
//
//    return nullptr;
//}
//
//
//
//void execq::details::TaskProviderList2::add(std::shared_ptr<ITaskProvider> taskProvider)
//{
//    std::lock_guard<std::mutex> lock(m_mutex);
//    m_taskProviders.push_back(&taskProvider);
//    m_currentTaskProviderIt = m_taskProviders.begin();
//}
//
//void execq::details::TaskProviderList2::remove(std::shared_ptr<ITaskProvider> taskProvider)
//{
//    std::lock_guard<std::mutex> lock(m_mutex);
//    const auto it = std::find(m_taskProviders.begin(), m_taskProviders.end(), &taskProvider);
//    if (it != m_taskProviders.end())
//    {
//        m_taskProviders.erase(it);
//        m_currentTaskProviderIt = m_taskProviders.begin();
//    }
//}
//
//std::shared_ptr<execq::details::ITaskProvider> execq::details::TaskProviderList2::nextProviderWithTask()
//{
//    std::lock_guard<std::mutex> lock(m_mutex);
//
//    const size_t taskProvidersCount = m_taskProviders.size();
//    const auto listEndIt = m_taskProviders.end();
//
//    for (size_t i = 0; i < taskProvidersCount; i++)
//    {
//        if (m_currentTaskProviderIt == listEndIt)
//        {
//            m_currentTaskProviderIt = m_taskProviders.begin();
//        }
//
//        auto provider = *(m_currentTaskProviderIt++);
//        if (provider->hasTask())
//        {
//            return provider;
//        }
//    }
//
//    return nullptr;
//}
//
//void execq::details::TaskProviderList2::invalidate()
//{
//    m_valid = false;
//}
//
//bool execq::details::TaskProviderList2::execute()
//{
//    const std::shared_ptr<ITaskProvider>& provider = nextProviderWithTask();
//    if (provider)
//    {
//        return provider->execute();
//    }
//
//    return false;
//}
//
//bool execq::details::TaskProviderList2::valid() const
//{
//    return m_valid;
//}
