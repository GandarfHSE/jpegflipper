#include <iostream>
#include <condition_variable>
#include <deque>
#include <mutex>
#include <optional>
#include <thread>
#include <vector>

#include <evhttp.h>

#include "task.cpp"

template <class T>
class BlockingQueue {
public:
    bool push(T value) {
        auto lock = std::lock_guard(mutex_);

        if (isStopped_) {
            return false;
        }
        buf_.push_back(std::move(value));
        notEmpty_.notify_one();
        return true;
    }

    std::optional<T> pop() {
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

    void close() {
        auto lock = std::lock_guard(mutex_);
        isStopped_ = true;
        notEmpty_.notify_all();
    }

private:
    std::mutex mutex_;
    std::condition_variable notEmpty_;
    bool isStopped_ = false;
    std::deque<T> buf_;
};

class ThreadPool {
public:
    ThreadPool(size_t threadCnt = 4) {
        workers_.reserve(threadCnt);
        for (int i = 0; i < threadCnt; i++) {
            workers_.emplace_back([this] {
                Run();
            });
        }
    }

    void Close() {
        q_.close();
        for (auto& worker : workers_) {
            if (worker.joinable()) {
                worker.join();
            }
        }
    }

    ~ThreadPool() {
        Close();
    }

    bool pushTask(CbTask task) {
        return q_.push(std::move(task));
    }

private:
    void Run() {
        while (auto task = q_.pop()) {
            (*task)();
        }
    }

    BlockingQueue<CbTask> q_;
    std::vector<std::thread> workers_;
};
