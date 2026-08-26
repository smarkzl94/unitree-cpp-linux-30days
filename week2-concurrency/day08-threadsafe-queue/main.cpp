#include "ThreadSafeQueue.h"

#include <cassert>
#include <chrono>
#include <iostream>
#include <thread>
#include <vector>

int main() {
    // --- 场景 1：单生产单消费，1..N 不丢 ---
    {
        ThreadSafeQueue<int> q;
        constexpr int N = 100;
        std::vector<int> got;
        got.reserve(N);

        std::thread producer([&] {
            for (int i = 1; i <= N; ++i) {
                q.push(i);
            }
        });

        std::thread consumer([&] {
            int x = 0;
            while (got.size() < static_cast<size_t>(N)) {
                if (q.wait_and_pop(x)) {
                    got.push_back(x);
                }
            }
        });

        producer.join();
        consumer.join();
        q.shutdown();

        assert(got.size() == static_cast<size_t>(N));
        for (int i = 0; i < N; ++i) {
            assert(got[static_cast<size_t>(i)] == i + 1);
        }
        std::cout << "ok: spsc 1.." << N << "\n";
    }

    // --- 场景 2：消费者已经堵在 wait_and_pop，shutdown 必须能醒 ---
    {
        ThreadSafeQueue<int> q;
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

    // --- 场景 3：try_pop 空队列立刻失败 ---
    {
        ThreadSafeQueue<int> q;
        int x = 42;
        assert(!q.try_pop(x));
        q.push(7);
        assert(q.try_pop(x) && x == 7);
        assert(!q.try_pop(x));
        std::cout << "ok: try_pop\n";
    }

    std::cout << "all passed\n";
    return 0;
}
