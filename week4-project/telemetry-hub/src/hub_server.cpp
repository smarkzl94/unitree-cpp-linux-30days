// 第 2 步：产 + 处理 + TCP 发 latest。打印换成网络线程。
// Ctrl+C：handler 只改 g_running；main 里 shutdown 监听套接字，再 join。

#include "frame_ring.h"
#include "latest.h"
#include "log.h"
#include "protocol.h"
#include "sensor_frame.h"

#include <arpa/inet.h>
#include <atomic>
#include <cerrno>
#include <chrono>
#include <cmath>
#include <csignal>
#include <cstdint>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <netinet/in.h>
#include <sys/socket.h>
#include <thread>
#include <unistd.h>

namespace {

const std::size_t kRingCap = 64;
const int kSensorPeriodMs = 10;
const int kControlPeriodMs = 20;
const int kNetPeriodMs = 20;
const int kDefaultPort = 7777;
const float kJumpThresh = 0.5f;

std::atomic<bool> g_running{true};
std::atomic<std::uint64_t> g_hz_in{0};
std::atomic<std::uint64_t> g_hz_out{0};
std::atomic<std::uint64_t> g_anom{0};

void on_stop(int) {
    g_running.store(false);
}

void fill_frame(SensorFrame* f, std::uint64_t seq) {
    f->seq = seq;
    f->stamp = std::chrono::steady_clock::now();
    f->ax = static_cast<float>(seq % 100) * 0.01f;
    f->ay = 0.2f;
    f->az = 9.8f;
    f->flags = 0;
}

void run_sensor(FrameRing* ring) {// 传感器线程 填充帧 并推入 ring
    std::uint64_t seq = 0;// 序列号
    std::chrono::steady_clock::time_point next = std::chrono::steady_clock::now();// 下一个时间点
    while (g_running.load()) {// 运行状态为 true
        SensorFrame f;// 传感器帧
        fill_frame(&f, seq);// 填充帧
        seq++;// 序列号增加
        ring->push(f);
        g_hz_in.fetch_add(1);
        next += std::chrono::milliseconds(kSensorPeriodMs);// 下一个时间点增加
        std::this_thread::sleep_until(next);// 睡眠直到下一个时间点
    }
}
void mark_jump(SensorFrame* f, const SensorFrame* prev, bool have_prev,
               std::chrono::steady_clock::time_point* last_warn) {
    f->flags = 0;
    if (!have_prev) {
        return;
    }
    float dax = std::fabs(f->ax - prev->ax);
    float day = std::fabs(f->ay - prev->ay);
    float daz = std::fabs(f->az - prev->az);
    if (dax <= kJumpThresh && day <= kJumpThresh && daz <= kJumpThresh) {
        return;
    }
    f->flags = kFlagJump;
    g_anom.fetch_add(1);
    std::chrono::steady_clock::time_point now = std::chrono::steady_clock::now();
    if (now - *last_warn >= std::chrono::seconds(1)) {
        hub_log(kLogWarn, "jump seq=%llu dax=%.3f",
                static_cast<unsigned long long>(f->seq), static_cast<double>(dax));
        *last_warn = now;
    }
}

void run_control(FrameRing* ring, Latest* latest) {
    std::chrono::steady_clock::time_point next = std::chrono::steady_clock::now();
    SensorFrame prev{};
    bool have_prev = false;
    std::chrono::steady_clock::time_point last_warn =
        std::chrono::steady_clock::now() - std::chrono::seconds(1);
    while (g_running.load()) {
        SensorFrame f;
        if (ring->pop_latest(f)) {
            mark_jump(&f, &prev, have_prev, &last_warn);
            publish_latest(*latest, f);
            prev = f;
            have_prev = true;
        }
        next += std::chrono::milliseconds(kControlPeriodMs);
        std::this_thread::sleep_until(next);
    }
}

int make_listen(int port) {// 创建监听套接字
    int fd = socket(AF_INET, SOCK_STREAM, 0); // 创建 socket
    if (fd < 0) { // 如果创建失败
        perror("socket"); // 输出错误信息
        return -1; // 返回 -1 表示失败
    }
    int yes = 1; // 设置选项
    setsockopt(fd, SOL_SOCKET, SO_REUSEADDR, &yes, sizeof(yes)); // 设置选项
    sockaddr_in addr{}; // 定义地址
    addr.sin_family = AF_INET; // 设置地址族
    addr.sin_addr.s_addr = htonl(INADDR_ANY); // 设置地址
    addr.sin_port = htons(static_cast<std::uint16_t>(port)); // 设置端口号
    if (bind(fd, reinterpret_cast<sockaddr*>(&addr), sizeof(addr)) < 0) {//
        perror("bind"); // 输出错误信息
        close(fd); // 关闭 socket
        return -1; // 返回 -1 表示失败  
    }
    if (listen(fd, 4) < 0) {// 监听
        perror("listen"); // 输出错误信息
        close(fd); // 关闭 socket
        return -1; // 返回 -1 表示失败  
    }
    return fd;
}

void serve_client(int cfd, Latest* latest) {// 服务客户端 发送帧
    std::uint64_t last_seq = 0;
    bool have = false;
    std::chrono::steady_clock::time_point next = std::chrono::steady_clock::now();
    while (g_running.load()) {// 运行状态为 true
        SensorFrame f;
        if (snapshot_latest(*latest, f)) {
            if (!have || f.seq != last_seq) {// 如果帧序列号不等于上次帧序列号
                if (!send_frame(cfd, f)) {
                    hub_log(kLogWarn, "client send failed, will accept again");
                    return;
                }
                g_hz_out.fetch_add(1);
                hub_log(kLogDebug, "sent seq=%llu", static_cast<unsigned long long>(f.seq));
                last_seq = f.seq;
                have = true; // 设置已发送
            }
        }
        next += std::chrono::milliseconds(kNetPeriodMs); // 下一个时间点增加
        std::this_thread::sleep_until(next); // 睡眠直到下一个时间点
    }
}

void run_net(int listen_fd, Latest* latest) {// 网络线程 接受客户端连接 并发送帧
    while (g_running.load()) {
        int cfd = accept(listen_fd, nullptr, nullptr);
        if (cfd < 0) {
            if (!g_running.load()) {// 如果运行状态为 false
                break; // 退出循环
            }
            if (errno == EINTR) {// 如果中断
                continue; // 继续
            }
            perror("accept");
            hub_log(kLogError, "accept failed");
            break; // 退出循环
        }
        serve_client(cfd, latest); // 服务客户端
        close(cfd); // 关闭连接
    }
}

}  // namespace

int main(int argc, char** argv) {
    std::signal(SIGINT, on_stop);
    std::signal(SIGTERM, on_stop);
    std::signal(SIGPIPE, SIG_IGN);

    int port = kDefaultPort;
    for (int i = 1; i < argc; i++) {
        if (std::strncmp(argv[i], "--log=", 6) == 0) {
            log_set_level(log_parse_level(argv[i] + 6));
        } else {
            port = std::atoi(argv[i]);
        }
    }

    int listen_fd = make_listen(port);
    if (listen_fd < 0) {
        hub_log(kLogError, "listen failed on port %d", port);
        return 1;
    }

    FrameRing ring(kRingCap);
    Latest latest;

    std::thread sensor(run_sensor, &ring);
    std::thread control(run_control, &ring, &latest);
    std::thread net(run_net, listen_fd, &latest);

    hub_log(kLogInfo, "listen %d  (ss -lntp | grep %d)  --log=warn|info|debug", port, port);

    std::chrono::steady_clock::time_point last = std::chrono::steady_clock::now();
    while (g_running.load()) {
        std::chrono::steady_clock::time_point now = std::chrono::steady_clock::now();
        if (now - last >= std::chrono::seconds(1)) {
            std::uint64_t hin = g_hz_in.exchange(0);
            std::uint64_t hout = g_hz_out.exchange(0);
            std::uint64_t nanom = g_anom.exchange(0);
            hub_log(kLogInfo, "hz_in=%llu hz_out=%llu drop=%llu anom=%llu",
                    static_cast<unsigned long long>(hin),
                    static_cast<unsigned long long>(hout),
                    static_cast<unsigned long long>(ring.overwrite_count()),
                    static_cast<unsigned long long>(nanom));
            last = now;
        }
        std::this_thread::sleep_for(std::chrono::milliseconds(100));
    }

    shutdown(listen_fd, SHUT_RDWR);
    sensor.join();
    control.join();
    net.join();
    close(listen_fd);
    hub_log(kLogInfo, "shutdown");
    return 0;
}
