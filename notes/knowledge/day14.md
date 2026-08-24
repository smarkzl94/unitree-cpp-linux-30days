# Day14 · CMake/ldd 与 Week2 并发复盘（详细预习）

日期：2026-08-31

预习目标：先用最小 **CMake** 收束本周程序并用 `ldd` 看动态库依赖，再把 Week2 一条线串成可口述的讲义（队列、MPMC、死锁、atomic、产销与指标）。上机先练构建命令，再对照清单自检。

---

# 一、Linux：CMake 最小工程与 `ldd`

Week1 多用 `g++ main.cpp -o a.out`。文件一多（队列、测试、传感器、消费者），命令会变成一长串且易漏 `-pthread`。CMake 描述「有哪些源文件、要链什么」，生成实际编译命令。`ldd` 则回答：**这个可执行文件运行时要哪些 .so**——缺 `libstdc++`、没链 pthread 时，问题会出现在这里而不是源码逻辑。

## 1. 最小 `CMakeLists.txt`

```cmake
cmake_minimum_required(VERSION 3.16)
project(week2 LANGUAGES CXX)
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

add_executable(queue_demo
    day08-threadsafe-queue/main.cpp
)
find_package(Threads REQUIRED)
target_link_libraries(queue_demo PRIVATE Threads::Threads)
```

多文件就再 `add_executable` 几个，或后面 Week3 再拆库。今天目标是：**能配置、能编译、能跑一个本周 demo**，不是学完 CMake 全书。

## 2. 配置与编译（源码树外构建）

```bash
cmake -S . -B build           # 读 CMakeLists，生成到 build/
cmake --build build           # 真正调用编译器
./build/queue_demo            # 可执行文件在构建目录（名以你的 target 为准）
```

`-S` 源码目录，`-B` 构建目录。不要在源码目录堆 `.o`，和后面「目录规范 `build/`」一致。

改源码后一般只需再 `cmake --build build`。改了 `CMakeLists.txt` 才需要重新 `cmake -S . -B build`。

对比手写：

```bash
g++ -std=c++17 -pthread main.cpp -o queue_demo
```

CMake 把 `-pthread`、包含路径、多文件列表收进一份描述，CI 和别人的机器才能复现。Week3 Day15 会加 ASan、多文件规范；今天先跑通最小闭环。

## 3. `ldd`：看动态依赖

```bash
ldd ./build/queue_demo
```

每一行：库名 => 实际路径。常见会看到 `libstdc++.so`、`libm.so`、`libc.so`、`libgcc_s.so`，以及 **`libpthread.so`**（或 libc 里已含 pthread，视发行版）。

```text
linux-vdso.so.1 (0x...)
libstdc++.so.6 => /lib/x86_64-linux-gnu/libstdc++.so.6
libpthread.so.0 => ...          # 有的系统合并进 libc
libc.so.6 => ...
```

`not found` 就是运行时缺库：编译过了、别的机器跑不了，或交叉编译了。静态链接另说；预习默认动态链接。

`ldd` 只用于**你自己构建的二进制**。它会加载依赖信息；不要拿去扫来路不明的文件当习惯。

没有 `ldd` 时（纯 Windows 主机）在 WSL 里对 ELF 做。Git Bash 下的 `.exe` 不是同一套。

## 4. 和本周程序的关系

多线程 demo 必须链上 Threads。CMake 里 `Threads::Threads` 比猜 `-lpthread` 更可移植。`ldd` 用来确认「不是忘了链线程库」。跑 `top -H` 之前，先保证二进制能起来。

## Linux 口述（预习时自己答）

1. `cmake -S . -B build` 和 `cmake --build build` 各做什么？  
   前者配置生成构建系统；后者编译链接。
2. 为什么不要只靠超长 `g++` 命令？  
   多文件、pthread、别人复现会漏；CMake 是可重复的工程描述。
3. `ldd` 看什么？缺库长什么样？  
   运行时 .so 列表；`not found` 表示动态依赖找不到。

## Linux 上机（预习不用敲）

给 week2 某 demo 写最小 CMake，`ldd` 结果记入 `linux-notes.md`。

---

# 二、C++：Week2 一条线复盘

## 本周在干什么

**线程安全队列 → 证明 MPMC 不丢 → 会死锁会修 → atomic 边界与伪共享 → 100Hz 入缓冲 → 消费端延迟/丢包可量化。**

下面五条必须能不看笔记讲完，并且能指到你写过的代码。

## 1. 条件变量 + `while`（Day08）

`cv.wait(lock)` 会放锁睡觉；醒来时已重新持锁。必须：

```cpp
while (!pred) cv.wait(lk);
```

原因：虚假唤醒；多消费者时数据被别人拿走。`if` 会在空队列上 `pop`。

`lock_guard` 不能配 `wait`；用 `unique_lock`。`shutdown`：置标志 + `notify_all` + `join` 再析构。忘记 notify，消费者永睡；忘记 shutdown，析构时仍有人 wait。

## 2. 有界缓冲满时策略（Day09 / Day06）

必须说清**哪一种**，不要混：

| 场景 | 满了怎么办 |
|------|------------|
| 任务/指令 MPMC | **阻塞生产者**，不丢，用序号集合证明 0..N-1 齐全 |
| 遥测 RingBuffer | **覆盖最旧**，控制要最新 |

无界 `queue` 没有背压，生产者快就会吃光内存。有界把压力传回去，和 `ulimit` 同一句话。校验用的 `set` 不能多线程裸写。生命周期：工人还在用时不能先拆队列。

## 3. 死锁四条件与加锁顺序（Day10）

四条件：互斥、占有且等待、不可抢占、循环等待。经典 AB-BA。修：

- 全局固定顺序（先 m1 后 m2）；
- 或 `std::scoped_lock(m1, m2)`。

Linux：`thread apply all bt` 看谁握哪把锁；`strace -f` 见 `FUTEX_WAIT`。预防为主，gdb 是复现后的镜子。

## 4. 何时 atomic、何时 mutex（Day11）

单个计数/标志：`std::atomic`，默认 `seq_cst` 即可。`volatile` 不是原子。多字段不变量、空/满等待：仍 mutex + CV。无锁不是更快的保证。

伪共享：两个无关 `atomic` 挤在同一缓存行（常 64B，`lscpu`/`getconf`），核间来回作废缓存行。真共享改同一变量；伪共享只是邻居。

## 5. 延迟与丢包怎么统计（Day12–13）

生产：100Hz，`sleep_until` 减累计误差，帧上 `seq` + **单调时钟**时间戳，写入 RingBuffer，打印实际 Hz。

消费：频率可不同，常取最新。  

- 延迟 = `now - frame.ts`（同一时钟），报均值和 P99 概念。  
- 丢包 = seq 间隙 **和** overwrite 分开。队列长度 ≠ 延迟。  
- 日志每秒一行，重定向 + `tail -f`；每帧 flush 会污染测量。

墙钟（`date` / `system_clock`）给人看时刻；间隔和延迟用 `steady_clock`。

## 6. 自检清单（对照你的仓库）

- [ ] Day08 队列能 `shutdown` 不挂死；`ps -eLf` / `top -H` 能对上线程数
- [ ] Day09 有界、join、序号证明不丢不重
- [ ] Day10 有 gdb 栈笔记 + 已用顺序或 `scoped_lock` 修复
- [ ] Day11 说得出何时不要无锁、何为伪共享
- [ ] Day13 能打出延迟/丢包数字；日志在文件里能 `tail -f`
- [ ] Day14 最小 CMake 能编过，`ldd` 看过依赖

## C++ 面试口述（预习时自己答一遍）

1. **条件变量为什么必须 `while`？**  
   虚假唤醒；以及谓词在醒来时可能已再次为假。

2. **死锁四条件？怎么修交叉锁？**  
   互斥、占有且等待、不可抢占、循环等待。统一加锁顺序或 `scoped_lock`。

3. **RingBuffer 满了你怎么处理？和任务队列有何不同？**  
   遥测覆盖最旧；任务队列有界阻塞不丢。都要写进接口约定。

4. **延迟和丢包各怎么测？**  
   同一单调时钟相减；seq 间隙与覆盖次数分列；看均值和尾部。

5. **何时用 atomic、何时用 mutex？**  
   单变量计数/标志可用 atomic；多字段不变量和等待用 mutex（+CV）。

## C++ 上机（预习不用写）

见 `week2-concurrency/day14-review/TASK.md`：

- 口述写入 `notes/day14.md`：四条件、CV 的 while、RingBuffer 满策略
- CMake 收束 + `ldd`
- 按上面清单勾选弱项，缺哪天补哪天的最小 demo
