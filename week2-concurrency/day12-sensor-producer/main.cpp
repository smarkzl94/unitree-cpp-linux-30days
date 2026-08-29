// Day12：约 100Hz 往 RingBuffer 写模拟传感器帧；满了覆盖最旧。
// 时间戳、停表、sleep_until、测频率都用单调时钟 steady_clock。

#include "../../week1-cpp-basics/day06-ringbuffer/RingBuffer.h"  // Day06 的环：满了覆盖最旧

#include <chrono>    // 时钟、毫秒、sleep_until
#include <cstdint>   // std::uint64_t：序号、帧计数
#include <iostream>  // 打印
#include <thread>    // this_thread::sleep_until

const int kCapacity = 16;   // 环最多同时装 16 帧（容量，不是「已经写了多少」）
const int kPeriodMs = 10;   // 目标 100Hz → 每拍 10 毫秒
const int kRunMs = 3000;    // 大约跑 3000 毫秒再停

struct SensorFrame {  // 一帧数据
    std::chrono::steady_clock::time_point ts;  // 生产这一瞬间（单调时刻）
    std::uint64_t seq;                        // 序号，给后面看丢包用
    float acc[3];                            // 假加速度，不是真 IMU
};

// f 是指针，所以用 -> 改 main 里那一份帧
void fill_frame(SensorFrame* f, std::uint64_t seq) {
    f->ts = std::chrono::steady_clock::now();  // 打时间戳：现在
    f->seq = seq;                               // 记下这一帧的序号
    f->acc[0] = 0.1f;                           // 假数据；f 表示 float
    f->acc[1] = 0.2f;
    f->acc[2] = 0.3f;
}

int main() {
    RingBuffer<SensorFrame> buf(kCapacity);  // 类名<元素类型> 变量(容量)

    std::cout << "policy: overwrite oldest when full\n";  // 打印策略，避免和 Day09 阻塞搞混

    std::chrono::steady_clock::time_point t0 = std::chrono::steady_clock::now();  // 起点时刻
    std::chrono::steady_clock::time_point next = t0;  // 下一拍该醒的时刻，先等于起点
    std::uint64_t seq = 0;        // 下一帧的序号
    std::uint64_t produced = 0;  // 一共成功生产了多少帧

    while (true) {  // 自己 break，所以条件写成 true
        std::chrono::steady_clock::time_point now = std::chrono::steady_clock::now();  // 这一圈的「现在」

        // now - t0 是一段时间；milliseconds(kRunMs) 标明 3000 是毫秒，才能比
        if (now - t0 >= std::chrono::milliseconds(kRunMs)) {
            break;  // 够大约 3 秒，离开循环
        }

        SensorFrame f;           // 栈上一帧，循环结束就销毁
        fill_frame(&f, seq);     // &f：把这份帧的地址传进去填
        seq++;                    // 下一帧序号 +1
        buf.push(f);            // 写入环；满了先丢最旧的再写
        produced++;               // 计数 +1

        next += std::chrono::milliseconds(kPeriodMs);  // 时间轴上的下一拍 = 再加 10ms
        std::this_thread::sleep_until(next);            // 让本线程睡到 next，不是再睡固定 10ms
    }

    std::chrono::steady_clock::time_point t1 = std::chrono::steady_clock::now();  // 结束时刻

    // duration<double>：间隔用「秒、带小数」，不要砍成整秒再整除
    double seconds =
        std::chrono::duration_cast<std::chrono::duration<double>>(t1 - t0).count();
    double actual_hz = static_cast<double>(produced) / seconds;  // 帧数 / 秒数 = 实际频率

    std::cout << "produced: " << produced
              << ", seconds: " << seconds
              << ", actual_hz: " << actual_hz
              << ", buf.size(): " << buf.size() << std::endl;  // size 应是 16，环满不涨
    std::cout << "expected_hz: " << 1000.0 / kPeriodMs << std::endl;  // 1000ms/10ms = 100
    return 0;  // 正常结束
}
