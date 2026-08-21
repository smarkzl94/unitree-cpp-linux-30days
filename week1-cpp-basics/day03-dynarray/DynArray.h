#pragma once

#include <cstddef>

// 手写缩小版 vector<int>：一块堆数组 + size + capacity
class DynArray {
public:
    DynArray();
    ~DynArray();

    // 先禁止拷贝，避免漏写深拷贝导致 double-free
    DynArray(const DynArray&) = delete;
    DynArray& operator=(const DynArray&) = delete;

    void push_back(int value);
    std::size_t size() const;
    std::size_t capacity() const;
    int& operator[](std::size_t i);
    const int& operator[](std::size_t i) const;

    // 扩容演示用：第 0 个元素在堆上的地址
    const int* data() const;

private:
    void grow();  // 容量不够时：new 更大块 → 拷旧元素 → delete[] 旧块

    int* data_;
    std::size_t size_;
    std::size_t cap_;
};
