#include "MyString.h"

#include <cassert>
#include <cstring>
#include <iostream>
#include <utility>  // std::move

int main() {
    // 1) 默认空串
    {
        MyString a;
        assert(a.size() == 0);
        assert(std::strcmp(a.c_str(), "") == 0);
    }

    // 2) 从字面量构造
    {
        MyString a("hello");
        assert(a.size() == 5);
        assert(std::strcmp(a.c_str(), "hello") == 0);
    }

    // 3) 拷贝后两边独立（需要你提供能改内容的途径时再强化；
    //    最小版：拷贝后内容相同，且不是同一块指针）
    {
        MyString a("abc");
        MyString b(a);  // 拷贝构造
        assert(b == a);
        assert(a.c_str() != b.c_str());  // 深拷贝：指针应不同
    }

    // 4) 移动后目标有效、源可安全析构
    {
        MyString a("move-me");
        MyString b(std::move(a));
        assert(std::strcmp(b.c_str(), "move-me") == 0);
        // a 被移走后：允许为空串，或仍可安全调用；关键是析构不崩
    }

    // 5) 自赋值不崩溃
    {
        MyString s("self");
        s = s;
        assert(std::strcmp(s.c_str(), "self") == 0);

        MyString t("self2");
        t = std::move(t);
        assert(t.c_str() != nullptr);  // 至少可安全用 / 析构
    }

    // 6) 一连串赋值不崩、内容符合预期（有条件可加 ASan）
    {
        MyString a("one");
        MyString b("two");
        MyString c("three");
        a = b;  // a -> "two"
        b = c;  // b -> "three"
        c = a;  // c -> "two"
        assert(std::strcmp(a.c_str(), "two") == 0);
        assert(std::strcmp(b.c_str(), "three") == 0);
        assert(std::strcmp(c.c_str(), "two") == 0);
    }

    std::cout << "All MyString tests passed.\n";
    return 0;
}
