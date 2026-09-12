#pragma once

#include "sensor_frame.h"   // 传感器帧
#include <cstddef> // 大小类型
#include <mutex> // 互斥锁
#include <vector> // 向量

// 有界环：满了覆盖最旧（遥测要「当前」，不要无限排队）。
// 传感 / 控制两个线程会碰到同一份，所以锁住 head/tail/size。
class FrameRing {
public:
    explicit FrameRing(std::size_t cap) // 构造函数
        : buf_(cap), cap_(cap), head_(0), tail_(0), size_(0), overwrite_(0) {}

    // 压入帧
    void push(SensorFrame x) {
        std::lock_guard<std::mutex> lock(mu_);
        // 如果缓冲区满了，先删除第一个，再在"末尾"插入一个新的帧
        if (size_ == cap_) { // 如果缓冲区满了
            head_ = (head_ + 1) % cap_; // 头指针向后移动一个位置
            size_--; // 大小减一
            overwrite_++; // 覆盖次数加一
        }
        buf_[tail_] = x; // 将帧压入缓冲区
        tail_ = (tail_ + 1) % cap_; // 尾指针向后移动一个位置
        size_++; // 大小加一
    }

    // 拿走最旧的一帧。空则 false。
    bool pop(SensorFrame& out) {
        std::lock_guard<std::mutex> lock(mu_);
        if (size_ == 0) { // 如果缓冲区为空
            return false; // 返回 false
        }
        out = buf_[head_]; // 将帧从缓冲区中取出
        head_ = (head_ + 1) % cap_; // 头指针向后移动一个位置
        size_--; // 大小减一
        return true; // 返回 true   
    }

    // 一把锁倒空，留下最后一帧 = 环里最新。控制要「现在」，不要啃积压。
    bool pop_latest(SensorFrame& out) {
        std::lock_guard<std::mutex> lock(mu_);
        if (size_ == 0) { // 如果缓冲区为空
            return false; // 返回 false
        }
        // 写法 A：一格格倒空，最后留在 out 里的就是最新。
        while (size_ > 0) {
            out = buf_[head_]; // 将帧从缓冲区中取出
            head_ = (head_ + 1) % cap_; // 头指针向后移动一个位置
            size_--; // 大小减一
        }
        // 写法 B（等价、O(1)）：最新在「最旧再数 size_-1 格」，不是 head_+size_
        //（那一格是 tail_，下一格要写的空位）。绕圈必须 % cap_。
        // out = buf_[(head_ + size_ - 1) % cap_];
        // head_ = (head_ + size_) % cap_;
        // size_ = 0;
        return true; // 返回 true
    }

    // 返回覆盖次数
    std::uint64_t overwrite_count() { // 返回覆盖次数
        std::lock_guard<std::mutex> lock(mu_);
        return overwrite_; // 返回覆盖次数
    }

private:
    std::mutex mu_; // 互斥锁
    std::vector<SensorFrame> buf_; // 缓冲区
    std::size_t cap_; // 容量
    std::size_t head_; // 头指针
    std::size_t tail_; // 尾指针
    std::size_t size_; // 大小
    std::uint64_t overwrite_; // 覆盖次数
};
