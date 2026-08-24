#include "RingBuffer.h"

#include <cassert>
#include <iostream>
#include <string>

int main() {
    // 1) 空
    RingBuffer<int> a(4);
    assert(a.empty());
    assert(a.size() == 0);
    int x = 0;
    assert(!a.pop(x));

    // 2) 未满时 push / pop 顺序（FIFO）
    a.push(10);
    a.push(20);
    a.push(30);
    assert(a.size() == 3);
    assert(!a.full());
    assert(a.pop(x) && x == 10);
    assert(a.pop(x) && x == 20);
    assert(a.pop(x) && x == 30);
    assert(a.empty());

    // 3) 刚好满
    RingBuffer<int> b(3);
    b.push(1);
    b.push(2);
    b.push(3);
    assert(b.full());
    assert(b.size() == 3);

    // 4) 满后再 push：覆盖最旧。1 被丢掉，剩下 2,3,4
    b.push(4);
    assert(b.full());
    assert(b.pop(x) && x == 2);
    assert(b.pop(x) && x == 3);
    assert(b.pop(x) && x == 4);
    assert(b.empty());

    // 5) 绕圈多次（下标要 % cap，不能写出界）
    RingBuffer<std::string> c(2);
    c.push("a");
    c.push("b");
    c.push("c");  // 覆盖 a，剩下 b,c
    c.push("d");  // 覆盖 b，剩下 c,d
    std::string s;
    assert(c.pop(s) && s == "c");
    assert(c.pop(s) && s == "d");
    assert(c.empty());

    std::cout << "day06 ringbuffer: 5 tests ok\n";
    return 0;
}
