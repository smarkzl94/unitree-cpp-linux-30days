#pragma once

#include <chrono> // 时间库
#include <cstdint> // 整数类型

// 一帧遥测。seq 给丢包；stamp 用单调时钟，后面算延迟。
struct SensorFrame {
    std::uint64_t seq;
    std::chrono::steady_clock::time_point stamp;
    float ax;
    float ay;
    float az;
    std::uint32_t flags;  // bit0 = 相邻帧跳变（控制线程标）
};

const std::uint32_t kFlagJump = 1u;
