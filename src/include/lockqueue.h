#pragma once
#include <queue>
#include <thread>
#include <mutex>              // pthread_mutex_t
#include <condition_variable> // pthread_condition_t：线程通信的条件变量

// 日志队列--异步
template <typename T>
class LockQueue
{
public:
    // 多个worker线程都会写日志queue
    void Push(const T &data)
    {
        // 类似于智能指针的lock_guard能够自动的加锁和释放锁
        std::lock_guard<std::mutex> lock(m_mutex);
        m_queue.push(data); 
        m_condvariable.notify_one();
    }

    // 只有一个线程读日志queue，写日志文件
    T Pop()
    {
        std::unique_lock<std::mutex> lock(m_mutex);
        // 防止线程的虚假唤醒
        while (m_queue.empty())
        {
            // 日志队列为空，线程进入wait状态,并释放锁
            m_condvariable.wait(lock);
        }

        T data = m_queue.front();
        m_queue.pop();
        return data;
    }

private:
    std::queue<T> m_queue;
    std::mutex m_mutex;
    std::condition_variable m_condvariable;
};
