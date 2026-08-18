#include "MyString.h"

#include <cstring>  // strlen, strcpy, strcmp — 实现时按需使用

// TODO: 默认构造 — 空串（size_=0，data_ 怎么放要想清楚）
MyString::MyString() {
    // ...
    data_ = nullptr;
    size_ = 0;
}

// TODO: 从 const char* 构造 — new[] 一份，拷贝内容，记得留 '\0'
MyString::MyString(const char* s) {
    // ...
    if(s== nullptr){
        data_ = nullptr;
        size_ = 0;
        return;
    }
    size_ = strlen(s);
    data_ = new char[size_ + 1];
    strcpy(data_, s);
}

// TODO: 析构 — delete[] data_；注意空指针安全
MyString::~MyString() {
    // ...
    size_ = 0;
    delete[] data_;
}

// TODO: 拷贝构造 — 深拷贝：新 new[] 一块，内容与 other 相同
MyString::MyString(const MyString& other) {
    // ...
    size_ = other.size_;
    data_ = new char[size_ + 1];

    for (int i = 0; i < size_; i++) {
        data_[i] = other.data_[i];
    }
    data_[size_] = '\0';
}

// TODO: 拷贝赋值 — 防自赋值 → 释放旧资源 → 深拷贝
MyString& MyString::operator=(const MyString& other) {
    // ...
    if (this == &other) {
        return *this;
    }
    delete[] data_;
    if (other.data_ == nullptr) {
        data_ = nullptr;
        size_ = 0;
    } else {
        size_ = other.size_;
        data_ = new char[size_ + 1];
        strcpy(data_, other.data_);
    }
    return *this;
}

// TODO: 移动构造 — 偷 other.data_ / size_，然后把 other 置成可安全析构状态
MyString::MyString(MyString&& other) noexcept {
    // ...
    data_ = other.data_;
    size_ = other.size_;
    other.data_ = nullptr;
    other.size_ = 0;
}

// TODO: 移动赋值 — 防自赋值 → 释放自己的 → 再偷 → 置空源
MyString& MyString::operator=(MyString&& other) noexcept {
    // ...
    if (this == &other) {
        return *this;
    }
    delete[] data_;
    data_ = other.data_;
    size_ = other.size_;
    other.data_ = nullptr;
    other.size_ = 0;
    return *this;
}

// TODO
const char* MyString::c_str() const {
    return data_? data_ : "";  // 换成真正的 data_（空串也要返回有效 C 串）
}

// TODO
std::size_t MyString::size() const {
    return size_;
}

// 样例：先比长度，再比内容（strcmp 相等返回 0）
bool MyString::operator==(const MyString& other) const {
    return std::strcmp(c_str(), other.c_str()) == 0;
}
