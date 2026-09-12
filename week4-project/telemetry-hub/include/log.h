#pragma once

#include <atomic>// 包含 atomic 类型
#include <cstdarg>// 包含 cstdarg 类型
#include <cstdio>// 包含 cstdio 类型
#include <cstring>// 包含 cstring 类型

// 数字越大越啰嗦。默认 Info：启动/每秒摘要看得到，每帧 Debug 看不到。
enum LogLevel {
    kLogError = 0,
    kLogWarn = 1,
    kLogInfo = 2,
    kLogDebug = 3
};

inline std::atomic<int>& log_level_cell() {
    static std::atomic<int> level{kLogInfo};// 静态 atomic 类型 level 初始化为 kLogInfo
    return level;
}

inline void log_set_level(LogLevel level) {
    log_level_cell().store(static_cast<int>(level));// 存储 level 到 log_level_cell
}

inline LogLevel log_parse_level(const char* s) {// 解析 level 字符串
    if (s == nullptr) {
        return kLogInfo;
    }
    if (std::strcmp(s, "error") == 0) {// 如果 s 为 error
        return kLogError;
    }
    if (std::strcmp(s, "warn") == 0) {// 如果 s 为 warn 
        return kLogWarn;
    }
    if (std::strcmp(s, "info") == 0) {// 如果 s 为 info
        return kLogInfo;
    }
    if (std::strcmp(s, "debug") == 0) {// 如果 s 为 debug
        return kLogDebug;
    }
    return kLogInfo;
}

inline void hub_log(LogLevel level, const char* fmt, ...) {
    if (static_cast<int>(level) > log_level_cell().load()) {// 如果 level 大于 log_level_cell 的负载
        return;
    }
    const char* tag = "INFO";
    if (level == kLogError) {
        tag = "ERROR";
    } else if (level == kLogWarn) {
        tag = "WARN";
    } else if (level == kLogDebug) {
        tag = "DEBUG";
    }
    std::fprintf(stderr, "%s ", tag);
    va_list ap;
    va_start(ap, fmt);
    std::vfprintf(stderr, fmt, ap);
    va_end(ap);
    std::fputc('\n', stderr);
}
