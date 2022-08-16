#include <condition_variable>
#include <deque>
#include <mutex>
#include <optional>
#include <thread>
#include <vector>

#include <evhttp.h>

#include "task.h"
#include "threadpool.h"

// BlockingQueue

template <class T>
bool BlockingQueue<T>::Push(T value) {
    auto lock = std::lock_guard(mutex_);

    if (isStopped_) {
        return false;
    }
    buf_.push_back(std::move(value));
    notEmpty_.notify_one();
    return true;
}

template <class T>
std::optional<T> BlockingQueue<T>::Pop() {
    auto lock = std::unique_lock(mutex_);

    notEmpty_.wait(lock, [this] {
        return isStopped_ || !buf_.empty();
    });
    if (isStopped_ && buf_.empty()) {
        return std::nullopt;
    }

    T res = std::move(buf_.front());
    buf_.pop_front();
    return res;
}

template <class T>
void BlockingQueue<T>::Close() {
    auto lock = std::lock_guard(mutex_);
    isStopped_ = true;
    notEmpty_.notify_all();
}

// ThreadPool

ThreadPool::ThreadPool(size_t threadCnt) {
    workers_.reserve(threadCnt);
    for (int i = 0; i < threadCnt; i++) {
        workers_.emplace_back([this] {
            Run();
        });
    }
}

void ThreadPool::Close() {
    q_.Close();
    for (auto& worker : workers_) {
        if (worker.joinable()) {
            worker.join();
        }
    }
}

bool ThreadPool::PushTask(CbTask task) {
    return q_.Push(std::move(task));
}

void ThreadPool::Run() {
    while (auto task = q_.Pop()) {
        (*task)();
    }
}
