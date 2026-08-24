# Day21 · Week3 复盘：进程口述与统一 CMake（详细预习）

日期：2026-09-07

预习目标：先把本周 Linux 主线（进程/线程/僵尸/信号安全）口述到能闭卷讲 2 分钟，再把 week1–2 收进一份能复用的 CMake。上机先自己答口述，再改工程。

---

# 一、Linux：进程 / 线程 / 僵尸 / 信号（本周一条线）

Week3 命令侧不是零散工具，而是一条排障链：

```text
能源外构建（Day15）
  → 能看见 syscall（Day16 strace）
  → 能解释进程从生到死（Day17 fork/wait，Z 状态）
  → 能文明地停它（Day18 TERM/INT vs KILL）
  → 能看见进程间对象（Day19 ipcs /dev/shm）
  → 卡了会按清单查（Day20 ps/top/strace）
```

今天不学新命令。把四个最容易混的概念钉死。预习时**对着下面四问自己讲出声**，讲不利索的回去翻对应日。

## 1. 进程 vs 线程

**进程**是资源账户：自己的地址空间、自己的 fd 表、自己的 PID。`fork` 出另一个账户（COW 页）。一个进程崩了，默认不直接把另一个进程的内存撕掉。

**线程**是同一账户里的多条执行流：共享堆、全局量、fd。一个线程 `delete` 了别人还在用的对象，全进程一起炸。`ps -eLf` / `top -H` 看到的 LWP 就是线程。

对照本周作业：

- Day17 的父子：两个进程，`mycat` 在子的地址空间里跑。
- Day08/Day12 的产销：多个线程，RingBuffer 必须同步。
- Day19 的 pipe/shm：因为是**进程**隔离，才需要 IPC；线程之间直接共享内存，用 mutex/atomic 即可。

口述一句：**「要隔离、要独立崩溃边界，用进程；要共享、要轻，用线程。IPC 是给进程用的，锁是给线程用的。」**

## 2. 僵尸：Z 不是卡死

子进程已经退出，内核保留退出状态，等父 `wait`/`waitpid`。`ps` 的 STAT 第一字符是 `Z`，常带 `<defunct>`。

- `kill -9` 杀不掉僵尸（已经死了）。
- 找 **PPID**，让父 wait；或让父退出，PID 1 代收。
- 长期服务必须自己回收：`waitpid` 或 `SIGCHLD` + 循环 `WNOHANG`。

对照实验（Day17）：故意不 wait 看见 Z，再 wait 消失。今天复述时要把「看见过」说出来。

孤儿 ≠ 僵尸：父先死、子还活，子被 init 收养，STAT 不是 Z。

## 3. 信号：能捕获 vs 不能；handler 里干什么

```text
SIGINT  (Ctrl+C / kill -INT)   能捕获 → 优雅退出
SIGTERM (kill / kill -TERM)    能捕获 → 部署停服务走这条
SIGKILL (kill -9)              不能捕获 → 内核直接撕掉
SIGPIPE                        写已关闭的管道/socket，默认打死进程
SIGCHLD                        子状态变，常用来 wait
```

Day18 对照：TERM/INT 能打印清理日志；KILL 没有任何用户态机会。

**异步信号安全**：handler 可能打断持锁或 `malloc`。只允许改 `std::atomic<bool>`（或 `volatile sig_atomic_t`）。`printf`、拿 mutex、`join`、打 Day20 那种日志，都算犯规。清理回主线程：唤醒 → join → flush → close → wait 子进程。

`sigaction` 优于 `signal`。INT 和 TERM **都要**注册。

## 4. 和 strace / ps 怎么对答案

口述排障 30 秒版（Day20 清单压缩）：

```text
ps 看 STAT
  Z → 回收问题
  D → 磁盘/设备
  S + CPU 低 → strace 看卡在 read/futex/poll
  R + CPU 满 → gdb 看是不是死循环
kill 先 TERM 后 KILL
```

## Linux 口述（预习时自己答）

下面五题要能不看稿讲满约 1 分钟。答案已经散在上面，这里再收成「标准短答」。写进 notes。

1. **进程 vs 线程**  
   进程隔离地址空间和 fd；线程共享。崩溃、泄漏、锁的边界不同。`fork` 出进程，`std::thread` 出线程。

2. **僵尸进程**  
   子死父不 wait，进程表占槽。`ps` 为 Z。避免：父 waitpid。不能 kill 掉僵尸。

3. **信号 handler 为什么只设标志**  
   异步打断不安全点。只写 atomic；复杂清理在主线程。SIGKILL 无法处理。

4. **短读短写（顺便复盘 Day16）**  
   `read`/`write` 返回值才是实际长度。必须循环。`strace` 能看见最后一次短读。

5. **进程卡住了怎么查**  
   ps → top/top -H → strace -p → 必要时 gdb。先状态后 syscall，再用户态。

## Linux 上机（预习不用敲）

不要求新实验。把上面 5 题写进当天 notes，对着终端用旧程序各演示一次（Z、TERM vs KILL、strace 卡点）更好。见 `week3-linux/day21-review/TASK.md`。

---

# 二、C++：把 week1–2 收进统一 CMake

## 为什么需要

Day15 已经会 `-S -B` 和多文件。week1、week2 还散落在各目录、各 `g++`/`build.sh` 里。后面 Hub 要链 RingBuffer、队列、传感器帧，没有统一构建会每天重新发明。今天的产出是：**一份顶层或分层 CMake，Debug 能开 ASan，能分别编/跑已有测试。**

不是重写算法，是把已有 `.cpp` 挂进目标。

## 1. 推荐的目录与目标切法

```text
（示意，按你仓库实际名字对齐）
CMakeLists.txt                 # 顶层：cmake_minimum_required、project、add_subdirectory
common/                        # 以后 Hub 也用
  include/RingBuffer.hpp
week1-cpp-basics/
  day01-mystring/  → 库或可执行 + 测试
  ...
week2-concurrency/
  day08-... /
week3-linux/
  day15-cmake-asan/            # 已有 CMake，可被顶层 include
```

顶层最小：

```cmake
cmake_minimum_required(VERSION 3.16)
project(unitree_30days LANGUAGES CXX)
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

option(ENABLE_ASAN "AddressSanitizer in Debug" ON)

function(days_apply_asan tgt)
  if(ENABLE_ASAN AND CMAKE_BUILD_TYPE STREQUAL "Debug")
    target_compile_options(${tgt} PRIVATE -fsanitize=address -fno-omit-frame-pointer -g)
    target_link_options(${tgt} PRIVATE -fsanitize=address)
  endif()
endfunction()

add_subdirectory(week1-cpp-basics/day01-mystring)
add_subdirectory(week1-cpp-basics/day06-ringbuffer)
add_subdirectory(week2-concurrency/day08-threadsafe-queue)
# ……按你实际有测试的目录往下加
```

某个子目录：

```cmake
add_executable(day06_rb_test test_ringbuffer.cpp)
target_include_directories(day06_rb_test PRIVATE ${CMAKE_CURRENT_SOURCE_DIR})
days_apply_asan(day06_rb_test)
```

头文件库（RingBuffer 是 header-only 也行）：

```cmake
add_library(ringbuffer INTERFACE)
target_include_directories(ringbuffer INTERFACE ${CMAKE_CURRENT_SOURCE_DIR}/include)
```

原则：

- **一个可演示程序 / 一组测试 = 一个 target。** 不要一个巨型 `all.cpp`。
- **可复用的放库或 INTERFACE。** Hub 以后 `target_link_libraries(hub PRIVATE ringbuffer)`。
- **生成物仍只在 `build/`。**  
  `cmake -S . -B build -DCMAKE_BUILD_TYPE=Debug`  
  `cmake --build build -t day06_rb_test`

编不过的旧目录：先挂能跑的，坏的标 TODO，不要为了「全绿」改算法语义。今天目标是构建骨架。

## 2. 和 Day15 对照：你应该已经会的

- `cmake -S . -B build` 配置，`--build` 编译。
- Debug + ASan；Release 另开 `build-rel`。
- `target_include_directories` / `target_link_libraries`，少用全局 `include_directories`。
- 改 `.cpp` 只 `--build`；改 `CMakeLists.txt` 再配置。

今天新增加的只是 **subdirectory + 复用 ASan 函数**，避免每个目录复制粘贴 10 行 sanitizer。

## 3. 自检（构建侧）

```bash
cmake -S . -B build -DCMAKE_BUILD_TYPE=Debug
cmake --build build
# 至少跑通：day01 测试、day06 RingBuffer、day08 或 day12 产销（有哪个挂哪个）
```

ASan 若报旧作业的泄漏：记下，Day29 弱项补洞可以修。今天能编过、能跑是第一优先级。

## 4. 易错点

1. **在每个 day 目录再来一套互相冲突的 `project()` 版本**  
   子目录可以 `project`，但 C++ 标准、ASan 最好顶层统一。
2. **把 `build/` 提交上去**  
   继续 gitignore。
3. **一个 target 把 week1+week2 所有 .cpp 丢进去**  
   符号冲突、`main` 多个。一目标一 `main`。
4. **Windows MSVC 和 GCC 的 ASan 选项混写**  
   用 `if(CMAKE_CXX_COMPILER_ID MATCHES "Clang|GNU")` 包起来。
5. **为了 CMake 去改 RingBuffer 的满策略语义**  
   构建归构建，算法归算法。

## C++ 面试口述（预习时自己答一遍）

把「本周一条线」收成 30 秒：

**「这周把 C++ 接到 Linux API 上：CMake 源外构建和 ASan 查内存；POSIX 文件短读写；fork/exec/wait 和僵尸；信号 handler 只设 atomic，TERM/INT 优雅退出，KILL 不能抓；管道或 shm 传帧；卡了用 ps/top/strace。构建上把 week1–2 收进统一 CMake，给 Hub 复用。」**

再各用一句话复述：进程 vs 线程、僵尸、handler 只设标志、短读写、strace 看见什么。讲不满 5 句就回去翻 Day16–20。

## C++ 上机（预习不用写）

见 `week3-linux/day21-review/TASK.md`：

- 把 week1–2（能跑的部分）收进统一 CMake
- 口述写入 notes：进程 vs 线程、僵尸、信号异步安全
- 自检：CMake 可复用；Ctrl+C 优雅退出 demo 仍跑通；至少一种 IPC 还在
