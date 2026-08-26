#pragma once

#include <condition_variable>
#include <cstddef>
#include <mutex>
#include <queue>
#include <utility>

// 有界 MPMC。对照预习：push 满了也要 wait（所以也是 unique_lock）；
// 一把 cv 用 notify_all；满了阻塞不覆盖。不要把完整答案粘过来。

template <typename T>
class BoundedQueue {
public:
    explicit BoundedQueue(std::size_t cap) : cap_(cap) {}

    void push(T x) {
        std::unique_lock<std::mutex> lk(m_);
        while(q_.size() >= cap_ && !shutdown_) {
            cv_.wait(lk);
        }
        if(shutdown_) return;
        q_.push(std::move(x));
        cv_.notify_all();
        // unique_lock
        // while (q_.size() >= cap_ && !shutdown_) cv_.wait(lk);
        // if (shutdown_) return;
        // q_.push(std::move(x));
        // 出锁后（或锁内）notify_all
        // (void)x;
    }

    bool wait_and_pop(T& out) {
        // unique_lock
        // while (q_.empty() && !shutdown_) cv_.wait(lk);
        // if (q_.empty()) return false;
        // out = std::move(q_.front()); q_.pop(); notify_all; return true
        std::unique_lock<std::mutex> lk(m_);

        while(q_.empty() && !shutdown_) {
            cv_.wait(lk);
        }
        if(q_.empty()) return false;
        out = std::move(q_.front());
        q_.pop();
        cv_.notify_all();
        return true;
        // return false;
    }

    void shutdown() {
        // 持锁 shutdown_ = true; notify_all
        std::lock_guard<std::mutex> lc(m_);
        shutdown_ = true;
        cv_.notify_all();
    }

private:
    std::size_t cap_;
    std::queue<T> q_;
    std::mutex m_;
    std::condition_variable cv_;
    bool shutdown_ = false;
};
