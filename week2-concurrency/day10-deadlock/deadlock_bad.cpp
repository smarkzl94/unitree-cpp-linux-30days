#include <chrono>
#include <mutex>
#include <thread>

// 仅供 Day10 Linux 观察：稳定 AB-BA 死锁。修好是后面 C++ 的事。

std::mutex m1;
std::mutex m2;

int main() {
    std::thread a([] {
        std::lock_guard<std::mutex> lk1(m1);
        std::this_thread::sleep_for(std::chrono::milliseconds(50));
        std::lock_guard<std::mutex> lk2(m2);
    });
    std::thread b([] {
        std::lock_guard<std::mutex> lk2(m2);
        std::this_thread::sleep_for(std::chrono::milliseconds(50));
        std::lock_guard<std::mutex> lk1(m1);
    });
    a.join();
    b.join();
    return 0;
}
