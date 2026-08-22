#include "Scheduler.h"

#include <iostream>

int main() {
    Scheduler s;
    s.add(1, 10, "log");
    s.add(2, 50, "alarm");
    s.add(3, 20, "ctrl");

    std::cout << "contains 2=" << s.contains(2) << "\n";

    s.update_priority(3, 80);  // 3 变成最高，堆里还留着旧的 20

    Task t;
    while (s.pop_highest(t)) {
        std::cout << "pop id=" << t.id
                  << " pri=" << t.priority
                  << " " << t.payload << "\n";
    }
    // 期望顺序：3(80) → 2(50) → 1(10)
    // 不应先弹出 3 的旧优先级 20

    std::cout << "contains 2 after pop=" << s.contains(2) << "\n";
    return 0;
}
