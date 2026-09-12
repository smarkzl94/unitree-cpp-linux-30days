#pragma once

#include "sensor_frame.h" // 传感器帧

#include <mutex> // 互斥锁

// 只有一格：控制覆盖写，网络（现在是打印）拷走。
// 发送时不要持这把锁去做 IO。
struct Latest {
    std::mutex mu; // 互斥锁
    SensorFrame frame{}; // 传感器帧    
    bool has{false}; // 是否有帧
};

inline void publish_latest(Latest& slot, const SensorFrame& f) {// 发布最新帧
    std::lock_guard<std::mutex> lock(slot.mu); // 锁定互斥锁
    slot.frame = f; // 将帧赋值给传感器帧
    slot.has = true; // 设置为 true
}

inline bool snapshot_latest(Latest& slot, SensorFrame& out) {// 快照最新帧
    std::lock_guard<std::mutex> lock(slot.mu); // 锁定互斥锁    
    if (!slot.has) { // 如果没有帧          
        return false; // 返回 false 
    }
    out = slot.frame; // 将帧赋值给传感器帧
    return true; // 返回 true
}
