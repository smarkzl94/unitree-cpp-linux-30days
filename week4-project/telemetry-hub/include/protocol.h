#pragma once

#include "net_io.h"
#include "sensor_frame.h"

#include <arpa/inet.h>
#include <cstdint>
#include <cstring>

// 线上：4 字节长度（网络字节序）+ payload。
// payload 24 字节：seq(8) + ax/ay/az(各 4) + flags(4)。本机字节序。
const std::uint32_t kPayloadBytes = 24;
const std::uint32_t kMaxPayload = 4096;

// 编码 payload
inline bool encode_payload(const SensorFrame& f, char* buf) { // 编码 payload
    std::memcpy(buf + 0, &f.seq, 8); // 复制序列号
    std::memcpy(buf + 8, &f.ax, 4); // 复制 x 轴加速度
    std::memcpy(buf + 12, &f.ay, 4); // 复制 y 轴加速度
    std::memcpy(buf + 16, &f.az, 4);
    std::memcpy(buf + 20, &f.flags, 4);
    return true;
}

inline bool decode_payload(const char* buf, std::uint32_t len, SensorFrame* out) { // 解码 payload
    if (len != kPayloadBytes) { // 如果长度不等于 payload 字节数
        return false; // 返回 false 
    }
    std::memcpy(&out->seq, buf + 0, 8); // 复制序列号
    std::memcpy(&out->ax, buf + 8, 4); // 复制 x 轴加速度
    std::memcpy(&out->ay, buf + 12, 4); // 复制 y 轴加速度
    std::memcpy(&out->az, buf + 16, 4);
    std::memcpy(&out->flags, buf + 20, 4);
    return true;
}

inline bool send_frame(int fd, const SensorFrame& f) {
    char body[24];
    encode_payload(f, body);
    std::uint32_t nlen = htonl(kPayloadBytes); // htonl 转换为网络字节序
    if (!send_full(fd, &nlen, 4)) { // 发送长度
        return false;
    }
    return send_full(fd, body, kPayloadBytes); // 发送 payload
}

inline bool recv_frame(int fd, SensorFrame* out) {
    std::uint32_t nlen = 0;
    if (!recv_full(fd, &nlen, 4)) { // 接收长度
        return false; // 返回 false 
    }
    std::uint32_t len = ntohl(nlen); // ntohl 转换为本地字节序
    if (len == 0 || len > kMaxPayload) { // 如果长度为 0 或大于最大 payload 长度    
        return false;
    }
    char body[4096];// 定义 payload 缓冲区
    if (!recv_full(fd, body, len)) {
        return false; // 返回 false 
    }
    return decode_payload(body, len, out); // 解码 payload
}
