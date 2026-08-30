// Day13：生产约 100Hz 灌环，控制约 50Hz 取最新；统计延迟与丢包。
//
// 为啥两线程：生产 100Hz 灌环，控制 50Hz 拿「现在」该用的数据。
// 为啥 pop 到空：Day06 的 pop 拿最旧的；倒空才得到最新。
// 为啥加锁：两个人改同一个环的 head/tail/size。
// 为啥两个丢包数：seq 间隙 = 我没见到的号；overwrite = 环满主动丢掉的。

#include "../../week1-cpp-basics/day06-ringbuffer/RingBuffer.h"

#include <chrono>
#include <cstdint>
#include <iostream>
#include <mutex>
#include <thread>
#include <vector>

const int kCapacity = 16;
const int kProdPeriodMs = 10;  // 约 100Hz
const int kCtrlPeriodMs = 20;  // 约 50Hz
const int kRunMs = 3000;

struct SensorFrame {
    std::chrono::steady_clock::time_point ts;
    std::uint64_t seq;
    float acc[3];
};

void fill_frame(SensorFrame* f, std::uint64_t seq) {
    f->ts = std::chrono::steady_clock::now();
    f->seq = seq;
    f->acc[0] = 0.1f;
    f->acc[1] = 0.2f;
    f->acc[2] = 0.3f;
}

// 倒空环，留下最后一次成功 pop 的帧。环空则 false。
bool pop_latest(RingBuffer<SensorFrame>* buf, std::mutex* m, SensorFrame* out) {
    bool got = false;
    SensorFrame tmp;
    std::lock_guard<std::mutex> lock(*m);
    // RingBuffer::pop 返回 bool、把帧写进 tmp；一直取到空，最后一份就是最新
    while (buf->pop(tmp)) {
        *out = tmp;
        got = true;
    }
    return got;
}

void producer(RingBuffer<SensorFrame>* buf, std::mutex* m, std::uint64_t* overwrite,
              std::chrono::steady_clock::time_point t0) {
    std::chrono::steady_clock::time_point next = std::chrono::steady_clock::now();
    std::uint64_t seq = 0;
    while (true) {
        std::chrono::steady_clock::time_point now = std::chrono::steady_clock::now();
        // producer 超时，退出
        if (now - t0 >= std::chrono::milliseconds(kRunMs)) {
            break;
        }
        SensorFrame f;
        //  生产一个帧
        fill_frame(&f, seq);
        seq++;
        {
            std::lock_guard<std::mutex> lock(*m);
            if (buf->size() >= kCapacity) {
                *overwrite += 1;
            }
            buf->push(f);
        }
        next += std::chrono::milliseconds(kProdPeriodMs);
        std::this_thread::sleep_until(next);
    }
}

void consumer(RingBuffer<SensorFrame>* buf, std::mutex* m, std::uint64_t* overwrite,
              std::chrono::steady_clock::time_point t0) {
    std::chrono::steady_clock::time_point next = std::chrono::steady_clock::now();
    std::uint64_t last_seq = 0;       // 上一帧的 seq；用来和这次比间隙
    bool have_last = false;           // 还没有成功消费过；第一帧不算间隙
    std::uint64_t gap_drop = 0;       // seq 跳号：我没见到的帧数合计
    std::uint64_t consumed = 0;       // 成功取到并处理了多少帧
    long long latency_sum_us = 0;     // 每次延迟(微秒)的总和；除以 consumed 得均值
    std::vector<long long> samples;   // 每次延迟存一份，以后可算 P99

    std::chrono::steady_clock::time_point last_print = t0;  // 上次打日志的时刻，大约每秒一行

    while (true) {
        std::chrono::steady_clock::time_point now = std::chrono::steady_clock::now();
        // consumer 超时退出：now-t0 是过了多久，和 kRunMs(3000ms) 比，到点 break
        // 与 producer 共用起点 t0；两条线程各自看钟，不是等对方喊停
        if (now - t0 >= std::chrono::milliseconds(kRunMs)) {
            break;
        }

        SensorFrame f;
        bool ok = pop_latest(buf, m, &f);
        if (ok) {
            // 延迟 = 现在 - 帧上的时间戳，两边同一个 steady_clock
            long long latency =
                std::chrono::duration_cast<std::chrono::microseconds>(now - f.ts).count();
            latency_sum_us += latency;
            consumed++;
            samples.push_back(latency);

            // seq 跳号：上次 10、这次 13，中间 11/12 没见到 → +2
            if (have_last && f.seq > last_seq + 1) {
                gap_drop += (f.seq - last_seq - 1);
            }
            last_seq = f.seq;
            have_last = true;
        }

        if (now - last_print >= std::chrono::seconds(1)) {
            std::uint64_t ow = 0;
            {
                std::lock_guard<std::mutex> lock(*m);
                ow = *overwrite;  // 和 producer 同一把锁，避免两线程同时读写
            }
            std::cout << "consumed=" << consumed
                      << " mean_us=" << (consumed > 0 ? latency_sum_us / consumed : 0)
                      << " gap_drop=" << gap_drop << " overwrite=" << ow << std::endl;
            last_print = now;  // 记下这次打印时刻，否则条件一直成立、每拍都打
        }

        next += std::chrono::milliseconds(kCtrlPeriodMs);
        std::this_thread::sleep_until(next);
    }

    std::uint64_t ow = 0;
    {
        std::lock_guard<std::mutex> lock(*m);
        ow = *overwrite;
    }
    std::cout << "final consumed=" << consumed << " gap_drop=" << gap_drop
              << " overwrite=" << ow << std::endl;
}

int main() {
    RingBuffer<SensorFrame> buf(kCapacity);
    std::mutex m;
    std::uint64_t overwrite = 0;

    std::cout << "policy: overwrite oldest; control wants latest only\n";

    std::chrono::steady_clock::time_point t0 = std::chrono::steady_clock::now();

    std::thread prod(producer, &buf, &m, &overwrite, t0);
    std::thread ctrl(consumer, &buf, &m, &overwrite, t0);
    prod.join();
    ctrl.join();
    return 0;
}
