#pragma once
#include <condition_variable>
#include <deque>
#include <mutex>
#include <optional>
#include <thread>
#include <vector>

#include <evhttp.h>

#include "task.h"

template <class T>
class BlockingQueue {
public:
    bool Push(T value);

    std::optional<T> Pop();

    void Close();

private:
    std::mutex mutex_;
    std::condition_variable notEmpty_;
    bool isStopped_ = false;
    std::deque<T> buf_;
};

class ThreadPool {
public:
    ThreadPool(size_t threadCnt = 4);

    void Close();

    ~ThreadPool() {
        Close();
    }

    bool PushTask(CbTask task);

private:
    void Run();

    BlockingQueue<CbTask> q_;
    std::vector<std::thread> workers_;
};
