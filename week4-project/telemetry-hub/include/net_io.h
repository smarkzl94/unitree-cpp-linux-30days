#pragma once

#include <cerrno>
#include <cstddef>
#include <sys/socket.h>
#include <unistd.h>

// TCP 可能一次只交出一部分字节。读/写满 n 才算成功（Day16/Day23）。
inline bool recv_full(int fd, void* dst, std::size_t n) {
    char* p = static_cast<char*>(dst); // 转换为字符指针
    std::size_t got = 0; // 已接收字节数
    while (got < n) {
        ssize_t r = recv(fd, p + got, n - got, 0); // 接收字节 接收到的字节数
        if (r < 0) {
            if (errno == EINTR) {
                continue; // 继续接收   
            }
            return false;// 返回 false 
        }
        if (r == 0) {
            return false;// 返回 false  
        }
        got += static_cast<std::size_t>(r);
    }
    return true;
}

inline bool send_full(int fd, const void* src, std::size_t n) {// 发送完整  
    const char* p = static_cast<const char*>(src);
    std::size_t put = 0; // 已发送字节数
    while (put < n) {
        ssize_t w = send(fd, p + put, n - put, 0); // 发送字节 发送到的字节数
        if (w < 0) { // 如果发送失败        
            if (errno == EINTR) {
                continue; // 继续发送   
            }
            return false; // 返回 false  
        }
        if (w == 0) {
            return false; // 返回 false  
        }
        put += static_cast<std::size_t>(w); // 已发送字节数增加
    }
    return true; // 返回 true  
}
