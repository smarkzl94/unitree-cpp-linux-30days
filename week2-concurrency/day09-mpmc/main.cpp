#include "BoundedQueue.h"

#include <cassert>
#include <chrono>
#include <iostream>
#include <set>
#include <thread>
#include <vector>

int main() {
    // --- 场景 1：2 产 + 2 消，有界，0..N-1 不丢不重 ---
    {
        constexpr int N = 200;
        constexpr std::size_t cap = 8;  // 远小于 N，才会走到「满、生产者 wait」
        BoundedQueue<int> q(cap);

        std::vector<int> local[2];
        std::set<int> seen;

        std::thread p0([&] {
            for (int i = 0; i < N / 2; ++i) {
                q.push(i);
            }
        });
        std::thread p1([&] {
            for (int i = N / 2; i < N; ++i) {
                q.push(i);
            }
        });

        auto consume = [&](int idx) {
            int x = 0;
            while (q.wait_and_pop(x)) {
                local[idx].push_back(x);
            }
        };
        std::thread c0([&] { consume(0); });
        std::thread c1([&] { consume(1); });

        p0.join();
        p1.join();
        q.shutdown();
        c0.join();
        c1.join();

        // join 之后才合并，不必再锁
        for (int k = 0; k < 2; ++k) {
            for (int x : local[k]) {
                seen.insert(x);
            }
        }

        assert(seen.size() == static_cast<std::size_t>(N));
        for (int i = 0; i < N; ++i) {
            assert(seen.count(i) == 1);
        }
        std::cout << "ok: mpmc 0.." << (N - 1) << " cap=" << cap << "\n";
    }

    // --- 场景 2：空队列上 wait 时 shutdown 能醒 ---
    {
        BoundedQueue<int> q(4);
        bool popped = true;
        std::thread consumer([&] {
            int x = 0;
            popped = q.wait_and_pop(x);
        });
        std::this_thread::sleep_for(std::chrono::milliseconds(200));
        q.shutdown();
        consumer.join();
        assert(!popped);
        std::cout << "ok: shutdown wakes waiter\n";
    }

    std::cout << "all passed\n";
    return 0;
}
