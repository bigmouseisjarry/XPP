#include "WorkerPool.h"

WorkerPool* WorkerPool::Get()
{
    static WorkerPool instance;
    return &instance;
}

void WorkerPool::Init(int numThreads)
{
    for (int i = 0; i < numThreads; ++i) {
        m_Threads.emplace_back(&WorkerPool::WorkerLoop, this);
    }
}

void WorkerPool::Shutdown()
{
    {
        std::lock_guard lock(m_Mutex);
        m_Stop = true;
    }
    m_CV.notify_all();
    for (auto& t : m_Threads) {
        if (t.joinable()) t.join();
    }
    m_Threads.clear();
}

void WorkerPool::Enqueue(std::function<void()> job)
{
    {
        std::lock_guard lock(m_Mutex);
        m_Jobs.push(std::move(job));
        m_PendingCount.fetch_add(1);
    }
    m_CV.notify_one();
}

void WorkerPool::WaitAll()
{
    std::unique_lock lock(m_Mutex);
    m_DoneCV.wait(lock, [this] {
        return m_PendingCount.load() == 0;
        });
}

bool WorkerPool::HasWork() const
{
    return m_PendingCount.load() > 0;
}

void WorkerPool::WorkerLoop()
{
    while (true) {
        std::function<void()> job;
        {
            std::unique_lock lock(m_Mutex);
            m_CV.wait(lock, [this] { return !m_Jobs.empty() || m_Stop; });
            if (m_Stop && m_Jobs.empty()) return;
            job = std::move(m_Jobs.front());
            m_Jobs.pop();
        }
        m_ActiveCount.fetch_add(1);
        job();
        m_ActiveCount.fetch_sub(1);
        if (m_PendingCount.fetch_sub(1) == 1) 
        {
            // 最后一个任务完成，通知 WaitAll
            m_DoneCV.notify_all();
        }
    }
}
