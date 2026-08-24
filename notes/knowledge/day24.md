# Day24 · top/ss 观察与 Hub 接通产线（详细预习）

日期：2026-09-10

预习目标：先学会边跑程序边用 `top`/`ss` 看 CPU、线程和连接，再把传感器线程 + RingBuffer + TCP 接到 Hub，让客户端看到不断更新的最新状态。上机先练命令，再接线。

四天地图见 [day24-27-hub.md](day24-27-hub.md)。

---

# 一、Linux：边跑边看 `top` / `ss`

Hub 一跑起来就有多线程 + 一个监听端口。命令不是收尾才用，而是**接线时的眼睛**：线程是不是都在、有没有在听、client 连上没有、CPU 是不是被日志打满。

## 1. 启动后立刻做的三件事

```bash
./hub_server 7777                 # 终端 A
ss -lntp | grep 7777              # 必须有 LISTEN（Day22）
top -H -p $(pgrep -n hub_server)  # 线程视角（Day08/Day20）
```

`pgrep -n` 取最新那个匹配 PID，避免 grep 到旧进程。认不清就 `ps aux | grep hub`。

`top -H -p PID` 里你应能大致数出：主线程、传感、控制、网络（名字取决于你 `pthread_setname_np` 或日志里的 tid）。少一条：对应线程没起来或立刻退了。多出几十条：误循环 `thread` 没 join。

## 2. client 连上之后看 ESTAB

```bash
./hub_client 127.0.0.1 7777       # 终端 B
ss -tnp | grep 7777
```

应看到本机到本机的 `ESTAB`。没有：client 连错端口，或 server 只 `bind` 了却没 `listen`/`accept`。  
`Recv-Q` 持续很大：client 不读，或你的 server 只发不看背压——内核缓冲堆着。这正好逼你采用「只要最新」而不是无限 send。

## 3. `top` 里正常长什么样（建立基准）

100Hz 产 + 轻处理 + 本机 TCP，Idle 机器上 **CPU 应很低**（几个百分点到十几）。若某个线程 100%：

- 忙等（`while(empty) {}` 没有 wait/sleep）
- 日志每帧刷
- 锁竞争打转

先记下「接通时的 CPU、线程数、RSS」，Day26 压测才有对照。写入 `linux-notes.md`。

```bash
ps -o pid,stat,pcpu,rss,nlwp,cmd -p PID
```

NLWP 应接近你设计的线程数 + 运行时库可能多出来的 1～2 个。

## 4. 和 Day20 清单的关系

今天不全面排障，只把清单用在 Hub 上：`ps` 身份 → `top -H` 线程热度 → `ss` 端口/连接。卡死再 `strace -p`。优雅退出后 `ss` 里 LISTEN 应消失，没有残留 Z（`ps` 看子进程，若你有的话）。

## 5. 易错点（Linux 侧）

1. **先写完所有代码再第一次看 `ss`**  
   bind 错了你可能以为「协议没通」。每加一步就看一眼。
2. **`top` 不带 `-H`**  
   只能看见进程合计，分不清是传感死循环还是网络在空转。
3. **client 没连就分析 Send-Q**  
   没有 ESTAB 时先修连接。

## Linux 口述（预习时自己答）

1. **Hub 跑起来你先看哪三个东西？**  
   `ss -lntp` 是否 LISTEN；`top -H` 线程是否都在、CPU 是否异常；`ps` 的 NLWP/RSS 当基线。
2. **`ss` 里 Recv-Q 堆着说明什么？**  
   这一端内核已收到字节，用户态没 `recv` 及时抽走（或反过来看对端 Send-Q）。
3. **为什么本机 100Hz 不该打满一核？**  
   每 10ms 才干一点点活。打满通常是忙等或日志，不是算滤波。

## Linux 上机（预习不用敲）

接通后把 `ss -lntp`、`ss -tnp`、`top -H`、`ps` 各记一行。Ctrl+C 后再看 LISTEN 是否消失。

---

# 二、C++：传感 + RingBuffer + TCP 发最新

## 为什么需要

Week2 有 100Hz 传感器和 RingBuffer，Week4 有 TCP 和定长头。今天第一次把它们焊成一条可演示的链：客户端能看到状态在更新。滤波、压测、README 是后三天的事——今天**最小接通**。

上机任务：接到 `telemetry-hub/`；最小目标是客户端不断显示最新状态。

## 1. 三条线程，各干一件事

```text
sensor_thread     100Hz 产 SensorFrame → RingBuffer::push（满则覆盖最旧）
control_thread    pop 做（今天可空）处理 → 写入 latest_ 槽
net_thread        accept/recv 拆包可选；定时把 latest_ 按 Day23 帧发出
main              注册信号，join，close
```

也可以 net 和 control 合并，但「产 / 处理 / 发」职责要能在 README 里画出三块。面试就画这张图。

`SensorFrame` 沿用 Day19/23：`seq`、`stamp_ns`、几个 float。时间戳用单调时钟，方便 Day26 算延迟。

## 2. 只要最新：有界内存，网络慢就丢旧的

错误做法：无界 `queue`，sender 跟不上就无限堆积，RSS 爬升，延迟变成「排队年龄」。

正确做法（二选一，推荐 B 给发送侧）：

```text
A. RingBuffer 满时覆盖最旧（Day06 策略之一）
B. 另做 atomic/mutex 保护的 latest_：控制线程只覆盖写，发送线程只读拷走
```

遥测的业务含义是「当前姿态」，不是「历史每一帧都必须到达」。TCP 保证到了的字节可靠，但**应用层可以决定只发当前 latest**。中间帧没发出去，算丢包，记计数，不要堵死产线。

```cpp
struct Latest {
    std::mutex mu;
    SensorFrame frame{};
    bool has{false};
};

void publish(Latest& s, const SensorFrame& f) {
    std::lock_guard<std::mutex> lk(s.mu);
    s.frame = f;
    s.has = true;
}

bool snapshot(Latest& s, SensorFrame& out) {
    std::lock_guard<std::mutex> lk(s.mu);
    if (!s.has) return false;
    out = s.frame;
    return true;
}
```

发送循环：`snapshot` → 定长头 `send_full` → 按目标频率睡（如 50Hz）。若 snapshot 的 `seq` 和上次一样，可以不发（省带宽）或仍发（心跳）。选一种写进注释。

RingBuffer 给控制线程「尽量处理每一帧」做滤波；latest 给网络「只保证眼前这个」。两级不要揉成一个无界队列。

## 3. 协议与退出：复用，不重写轮子

- 线上格式：Day23 的 `uint32 length` + payload，`htonl`，长度上限。
- 客户端：Framer 或 `recv_full`，打印 `seq` / 字段，人眼能看出在涨。
- 退出：Day18 的 `g_running`；对 RingBuffer/`cv` `notify_all`；`shutdown(sock, SHUT_RDWR)` 让阻塞 `accept`/`recv` 醒来；join；flush 日志。
- 构建：挂进 Day21 的 CMake，Debug + ASan。

## 4. 最小验收

```bash
cmake -S . -B build -DCMAKE_BUILD_TYPE=Debug && cmake --build build
./build/hub_server 7777
./build/hub_client 127.0.0.1 7777
# client 屏幕上 seq 持续增加，字段在变
# ss / top 如第一节
# Ctrl+C server：双方干净退出，无 Z，无 ASan 报
```

还不用：滤波算法、P99、重连、漂亮 README。能演示「数据在流」即可。

## 5. 易错点（看懂再上机）

1. **无界队列当背压**  
   网络一慢，内存和延迟一起炸。
2. **发送线程持锁做 `send`**  
   先 snapshot 拷到栈上，解锁后再发送。否则传感/控制会被网络阻塞。
3. **每帧 LOG_INFO**  
   `top` 里日志线程会热。用 Day20 模块，默认 Warn/Info 摘要。
4. **`accept` 放在传感线程**  
   没人连时 100Hz 也卡着。网络单独听。
5. **忘记长度上限和短写**  
   Day16/23 的债今天会在长跑里出现。

## C++ 面试口述（预习时自己答一遍）

1. **为什么遥测可以丢中间帧？**  
   要的是当前状态。积压旧帧既占内存又加大延迟。有界缓冲 + 覆盖，是背压策略，不是偷懒。
2. **三线程怎么切？**  
   产频率稳定、处理不堵产、网络不持业务锁。用 RingBuffer 和 latest 槽连接。
3. **接通的验收是什么？**  
   客户端 seq 涨；`ss` 有 LISTEN/ESTAB；Ctrl+C 干净；`top` 不无故打满核。

## C++ 上机（预习不用写）

见 `week4-project/day24-hub-integrate-1/TASK.md`：

- 传感器线程 + RingBuffer + TCP 发最新
- 代码放 `telemetry-hub/`
- 边跑边做第一节的 `top`/`ss` 记录
