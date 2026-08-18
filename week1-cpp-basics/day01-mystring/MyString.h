#pragma once  // 防止头文件被重复包含（同一编译单元里只处理一次）

#include <cstddef>  // size_t

// Day01：手写简化版字符串，练习资源所有权与 Rule of Five
class MyString {
public:
    // 默认构造：空串
    MyString();

    // 从 C 风格字符串构造（允许 nullptr 当空串，也可自行约定）
    explicit MyString(const char* s);

    // ----- Rule of Five -----
    ~MyString();

    MyString(const MyString& other);             // 拷贝构造
    MyString& operator=(const MyString& other);  // 拷贝赋值

    MyString(MyString&& other) noexcept;             // 移动构造
    MyString& operator=(MyString&& other) noexcept;  // 移动赋值

    // ----- 常用接口 -----
    const char* c_str() const;
    std::size_t size() const;
    bool operator==(const MyString& other) const;

private:
    char* data_;         // 堆上字符数组（含 '\0'），或空串时指向 "" / 自己的缓冲
    std::size_t size_;   // 不含 '\0' 的字符个数
};
