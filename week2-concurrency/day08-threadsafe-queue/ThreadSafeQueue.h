#pragma once

#include <condition_variable>
#include <mutex>
#include <queue>
#include <utility>

// 自己实现四个接口。对照预习：while 等条件、lock_guard vs unique_lock、
// shutdown 要 notify_all。不要把完整答案从别处粘过来。

template <typename T> class ThreadSafeQueue {
public:
  void push(T x) {
    {
        std::lock_guard<std::mutex> lc(m_);
        if (shutdown_)  return;
      // 1. 持锁；若已 shutdown 可直接 return（策略写清即可）
      q_.push(std::move(x));
      // 2. q_.push(std::move(x))
    }
    cv_.notify_one();
    // 3. 解锁后再 notify_one（也可锁内 notify）
  }

  // 阻塞到有元素或 shutdown。成功取出返回 true；shutdown 且空返回 false。
  bool wait_and_pop(T &out) {

    std::unique_lock<std::mutex> lk(m_); // 必须 unique_lock，才能 wait
    while (q_.empty() && !shutdown_) {
      cv_.wait(lk);
    }

    if (q_.empty())
      return false; // shutdown 且没数据
    out = std::move(q_.front());
    q_.pop();
    return true;
  }

  // 空则立刻 false，不睡。
  bool try_pop(T &out) {
    std::lock_guard<std::mutex> lc(m_);
    if (q_.empty())
      return false;
    out = std::move(q_.front());
    q_.pop();
    return true;
  }

  void shutdown() {
    {
      std::lock_guard<std::mutex> lc(m_);
      shutdown_ = true;
    }
    cv_.notify_all();

    // 持锁置 shutdown_ = true，然后 notify_all
  }

private:
  std::queue<T> q_;
  std::mutex m_;
  std::condition_variable cv_;
  bool shutdown_ = false;
};
