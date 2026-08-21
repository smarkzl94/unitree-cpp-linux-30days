#include "DynArray.h"

#include <iostream>

int main() {
    DynArray a;

    // ----- 1) 连续 push，观察 size / capacity -----
    for (int i = 0; i < 8; ++i) {
        a.push_back(i);
        std::cout << "push " << i
                  << "  size=" << a.size()
                  << "  cap=" << a.capacity() << "\n";
    }

    // ----- 2) 扩容后地址会变（迭代器/引用失效）-----
    {
        DynArray b;
        b.push_back(1);
        b.push_back(2);
        const int* p = b.data();  // 指向当前堆块
        std::cout << "before grow  &b[0]=" << p << "  cap=" << b.capacity() << "\n";

        // TODO: 再 push，直到 capacity 变大
        // TODO: 再打印 b.data()，应和 p 不同
        // 此时不要再用 p（旧块已 delete[]）
        for(int i = 0; i<20;i++){
            b.push_back(i);
            std::cout << "push " << i
                  << "  size=" << b.size()
                  << "  cap=" << b.capacity() << "\n";
        }
        const int* p2 = b.data();
        std::cout << "after grow  &b[0]=" << p2 << "  cap=" << b.capacity() << "\n";
    }

    return 0;
}
