// 第 2 步客户端：连上后按定长头拆包，打印 seq。Ctrl+C 退出。

#include "protocol.h"

#include <arpa/inet.h> // 包含 inet_pton 函数
#include <atomic> // 包含 atomic 类型
#include <csignal> // 包含 signal 函数
#include <cstdint> // 包含 uint32_t 类型
#include <cstdio> // 包含 perror 函数   
#include <cstdlib> // 包含 atoi 函数
#include <cstring> // 包含 memcpy 函数
#include <iostream> // 包含 cout 函数
#include <netinet/in.h> // 包含 sockaddr_in 结构体
#include <sys/socket.h> // 包含 socket 函数
#include <unistd.h> // 包含 close 函数

namespace {

std::atomic<bool> g_running{true}; // 运行状态

void on_stop(int) { // 停止信号处理函数 设置运行状态为 false        
    g_running.store(false);
}

int connect_to(const char* host, int port) { // 连接到服务器
    int fd = socket(AF_INET, SOCK_STREAM, 0); // 创建 socket
    if (fd < 0) {
        perror("socket");
        return -1; // 返回 -1 表示失败
    }
    sockaddr_in addr{};
    addr.sin_family = AF_INET; // 设置地址族
    addr.sin_port = htons(static_cast<std::uint16_t>(port)); // 设置端口号
    if (inet_pton(AF_INET, host, &addr.sin_addr) != 1) {
        std::cerr << "bad host\n"; // 输出错误信息
        close(fd); // 关闭 socket
        return -1; // 返回 -1 表示失败
    }
    if (connect(fd, reinterpret_cast<sockaddr*>(&addr), sizeof(addr)) < 0) { // 连接到服务器
        perror("connect"); // 输出错误信息
        close(fd); // 关闭 socket
        return -1; // 返回 -1 表示失败
    }
    return fd; // 返回 socket 描述符
}

}  // namespace

int main(int argc, char** argv) {//主函数 连接到服务器 并接收帧
    std::signal(SIGINT, on_stop);
    std::signal(SIGTERM, on_stop);

    const char* host = "127.0.0.1";
    int port = 7777;
    if (argc >= 2) {
        host = argv[1];
    }
    if (argc >= 3) {
        port = std::atoi(argv[2]);
    }

    int fd = connect_to(host, port); // 连接到服务器
    if (fd < 0) {
        return 1; // 返回 1 表示失败
    }

    std::cout << "connected " << host << ":" << port << "\n"; // 输出连接信息
    while (g_running.load()) { // 运行状态为 true
        SensorFrame f;
        if (!recv_frame(fd, &f)) { // 接收帧
            std::cout << "peer closed\n"; // 输出错误信息
            break; // 退出循环
        }
        std::cout << "seq=" << f.seq << " ax=" << f.ax << " flags=" << f.flags << "\n";
    }
    close(fd); // 关闭 socket
    return 0;
}