// Day11：同一套 ++，对比 裸 int / mutex / atomic
// 单个计数、开关用 atomic；队列（多字段、要 wait）仍用 mutex。

#include <atomic>
#include <cassert>
#include <chrono>
#include <iostream>
#include <mutex>
#include <thread>

const int kPerThread = 1000000;  // 每个线程 ++ 的次数
const int kExpect = 4 * 1000000; // 4 个线程全加完，正确结果应是这个

// 场景 0：无保护。n 用指针，因为要改 main 里的那一份。
void add_bare(volatile int* n) {
    for (int k = 0; k < kPerThread; ++k) {
        ++(*n);  // 读、加、写三步，多线程会丢更新
    }
}

// 场景 A：加锁后再 ++。
void add_locked(int* n, std::mutex* m) {
    for (int k = 0; k < kPerThread; ++k) {
        std::lock_guard<std::mutex> lock(*m);  // 进来加锁，出这个 {} 自动解锁
        ++(*n);
    }
}

// 场景 B：原子 +1，不用 mutex。
void add_atomic(std::atomic<int>* n) {
    for (int k = 0; k < kPerThread; ++k) {
        n->fetch_add(1);  // 这一次 +1 是完整的。写成 ++(*n) 也行
    }
}

int main() {
    // ---------- 场景 0：裸 ++（常常是错的）----------
    {
        // volatile：让编译器别把循环优化没。它仍然不是 atomic。
        volatile int n = 0;

        std::chrono::steady_clock::time_point t0 = std::chrono::steady_clock::now();

        std::thread t1(add_bare, &n);  // 把函数名和参数传给线程，线程里会调用 add_bare(&n)
        std::thread t2(add_bare, &n);
        std::thread t3(add_bare, &n);
        std::thread t4(add_bare, &n);
        t1.join();  // 等这条线程结束。不 join 程序可能提前退出
        t2.join();
        t3.join();
        t4.join();

        std::chrono::steady_clock::time_point end = std::chrono::steady_clock::now();
        long long ms = std::chrono::duration_cast<std::chrono::milliseconds>(end - t0).count();

        std::cout << "bare int:     n=" << n << " expect=" << kExpect;
        if (n == kExpect) {
            std::cout << "  OK (lucky)";
        } else {
            std::cout << "  WRONG (lost updates)";
        }
        std::cout << "  " << ms << " ms\n";
    }

    // ---------- 场景 A：mutex + 普通 int ----------
    {
        int n = 0;
        std::mutex m;

        std::chrono::steady_clock::time_point t0 = std::chrono::steady_clock::now();

        std::thread t1(add_locked, &n, &m);
        std::thread t2(add_locked, &n, &m);
        std::thread t3(add_locked, &n, &m);
        std::thread t4(add_locked, &n, &m);
        t1.join();
        t2.join();
        t3.join();
        t4.join();

        std::chrono::steady_clock::time_point end = std::chrono::steady_clock::now();
        long long ms = std::chrono::duration_cast<std::chrono::milliseconds>(end - t0).count();

        assert(n == kExpect);
        std::cout << "mutex + int:  n=" << n << " expect=" << kExpect
                  << "  OK  " << ms << " ms\n";
    }

    // ---------- 场景 B：atomic，不加锁 ----------
    {
        std::atomic<int> n(0);  // 原子 int，初值 0

        std::chrono::steady_clock::time_point t0 = std::chrono::steady_clock::now();

        std::thread t1(add_atomic, &n);
        std::thread t2(add_atomic, &n);
        std::thread t3(add_atomic, &n);
        std::thread t4(add_atomic, &n);
        t1.join();
        t2.join();
        t3.join();
        t4.join();

        std::chrono::steady_clock::time_point end = std::chrono::steady_clock::now();
        long long ms = std::chrono::duration_cast<std::chrono::milliseconds>(end - t0).count();

        assert(n.load() == kExpect);  // load()：读出当前值
        std::cout << "atomic:       n=" << n.load() << " expect=" << kExpect
                  << "  OK  " << ms << " ms\n";
    }

    std::cout << "all passed\n";
    return 0;
}
