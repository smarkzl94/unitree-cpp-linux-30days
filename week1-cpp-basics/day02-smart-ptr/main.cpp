#include "Device.h"

#include <iostream>
#include <memory>

int main() {
    // ----- 1) unique_ptr 独占设备 -----
    {
        // TODO: make_unique<Device>("uart0")
        // TODO: p->open();
        // 离开这个 {} 时 unique_ptr 销毁，应看到析构/close 日志
        // 不要写：auto q = p;  （拷贝编不过）
        // 可以试：auto q = std::move(p);
        auto p = std::make_unique<Device>("uart0");
        p->open();
        // auto q= p; 是错误的
        auto q = std::move(p);
        q->open();
    }

    std::cout << "---- unique_ptr scope ended ----\n";

    // ----- 2) 循环引用泄漏：两边都用 strong -----
    {
        auto a = std::make_shared<Partner>();
        auto b = std::make_shared<Partner>();
        a->name = "A";
        b->name = "B";

        // TODO: a->strong = b;  b->strong = a;
        a->strong = b;
        b->strong = a;
        std::cout << "leak  a.use_count=" << a.use_count()
                  << "  b.use_count=" << b.use_count() << "\n";
        // 期望：外面还有 a、b 时计数 >= 2；离开 {} 后 Partner 析构日志可能不出现（泄漏）
    }
    std::cout << "---- leak scope ended (if no ~Partner, leaked) ----\n";

    // ----- 3) 一侧改 weak，打破环 -----
    {
        auto a = std::make_shared<Partner>();
        auto b = std::make_shared<Partner>();
        a->name = "A";
        b->name = "B";

        // TODO: a->strong = b;
        // TODO: b->weak = a;   // 不加 A 的计数
        a->strong = b;
        b->weak = a;
        std::cout << "fixed a.use_count=" << a.use_count()
                  << "  b.use_count=" << b.use_count() << "\n";
        // 期望：离开 {} 后两个 Partner 都能释放
        // 使用 weak 前：if (auto p = b->weak.lock()) { ... }
    }
    std::cout << "---- fixed scope ended ----\n";

    return 0;
}
