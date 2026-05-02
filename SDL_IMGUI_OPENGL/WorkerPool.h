#pragma once
#include <vector>
#include <queue>
#include <thread>
#include <mutex>
#include <condition_variable>
#include <functional>
#include <atomic>

class WorkerPool {
public:
    static WorkerPool* Get();

    void Init(int numThreads = 2);
    void Shutdown();

    // 提交一个任务到工作线程
    void Enqueue(std::function<void()> job);

    // 阻塞等待所有已提交的任务完成
    void WaitAll();

    // 是否还有未完成的任务
    bool HasWork() const;

private:
    WorkerPool() = default;
    ~WorkerPool() { Shutdown(); }

    std::vector<std::thread> m_Threads;
    std::queue<std::function<void()>> m_Jobs;
    mutable std::mutex m_Mutex;
    std::condition_variable m_CV;               // 通知 worker 有新 job
    std::condition_variable m_DoneCV;           // 通知 WaitAll 任务完成
    std::atomic<int> m_PendingCount{ 0 };       // 未完成的 job 数
    std::atomic<int> m_ActiveCount{ 0 };        // 正在执行的 job 数
    bool m_Stop = false;

    void WorkerLoop();
};