#include "Scheduler.h"

#include <iostream>

int main() {
    Scheduler s;  // 默认大顶
    s.add(1, 10, "log");
    s.add(2, 50, "alarm");
    s.add(3, 20, "ctrl");
    s.update_priority(3, 80);

    std::cout << "snapshot:";
    for (const auto& t : s.snapshot_sorted()) {
        std::cout << " " << t.id << "(" << t.priority << ")";
    }
    std::cout << "\n";
    // 期望：3(80) 2(50) 1(10)

    Task t;
    while (s.pop_highest(t)) {
        std::cout << "pop id=" << t.id << " pri=" << t.priority << "\n";
    }
    return 0;
}
