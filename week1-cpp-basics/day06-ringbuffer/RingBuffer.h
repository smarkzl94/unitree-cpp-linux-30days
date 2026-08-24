#pragma once

#include <cstddef>
#include <utility>
#include <vector>

// 定长环形缓冲。满时策略：覆盖最旧（遥测常用；丢旧帧，留下最新）。
// 空满判定：另存 size_（能装满 cap_ 个槽）。空：size_==0；满：size_==cap_。
template <typename T>
class RingBuffer {
public:
    explicit RingBuffer(std::size_t cap)
        : buf_(cap), cap_(cap), head_(0), tail_(0), size_(0) {}

    bool empty() const { return size_ == 0; }
    bool full() const { return size_ == cap_; }
    std::size_t size() const { return size_; }
    std::size_t capacity() const { return cap_; }

    // 写入。满了先丢掉 head_ 上那份最旧的，再写。
    void push(T x) {
        if(full()) {
            head_= (head_ + 1) % cap_;
            size_--;
        }
        buf_[tail_] = std::move(x);
        tail_ = (tail_ + 1) % cap_;
        size_++;
        // TODO: 如果 full()：head_ 往前走一格（记得 % cap_），size_--
        // TODO: buf_[tail_] = std::move(x);
        // TODO: tail_ 往前走一格（% cap_），size_++
    }

    // 读出最旧的一份。空则返回 false，不改 out。
    bool pop(T& out) {
        if(empty()) {
            return false;
        }
        out = std::move(buf_[head_]);
        head_ = (head_ + 1) % cap_;
        size_--;
        return true;
        // TODO: empty() 则 return false
        // TODO: out = std::move(buf_[head_]);
        // TODO: head_ 往前走一格，size_--，return true
    }

private:
    std::vector<T> buf_;   // 固定 cap_ 个槽，构造时一次分配
    std::size_t cap_;
    std::size_t head_;     // 下一个要读的下标
    std::size_t tail_;     // 下一个要写的下标
    std::size_t size_;     // 当前有几个元素（用来区分空/满）
};
