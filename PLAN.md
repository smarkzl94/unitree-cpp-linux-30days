# 30 天学习日历（双轨：每天 C++ + Linux）

周期：2026-08-18 ~ 2026-09-16  
每天约 **3 小时** = **C++ ~100 分钟** + **Linux ~70 分钟** + 收尾 10 分钟  

原则：两条线每天都有「可运行/可演示」小产出；Week4 在 Telemetry Hub 里汇合。  
Linux 练习优先在 **WSL2 Ubuntu**（Week1 命令行也可先在 Git Bash/WSL）。

---

## Week 1（08-18 ~ 08-24）· 现代 C++ + Linux 基础操作

| 日期 | 目录 | C++（~100min） | Linux（~70min） |
|------|------|----------------|-----------------|
| 08-18 | day01 | `MyString` 拷贝/移动/析构 | 目录/权限/`ls`/`cd`/`pwd`/`man`；用终端进入项目目录 |
| 08-19 | day02 | 智能指针设备管理器 | `ps`/`top`/`htop`/`kill`；观察自己的编译进程 |
| 08-20 | day03 | `DynArray` | 重定向与管道；`grep`/`find`/`wc` 在代码树里搜符号 |
| 08-21 | day04 | 优先级调度器 | `chmod`/`chown` 概念；写 `build.sh` 一键 g++ 编译当天程序 |
| 08-22 | day05 | lambda 重写调度器 | 软链接/硬链接；`tree`/`du`；整理 week1 目录 |
| 08-23 | day06 | `RingBuffer<T>` | `/proc` 入门：`cat /proc/cpuinfo` 等；记录本机环境 |
| 08-24 | day07 | Week1 C++ 复盘 | 写 `week1_smoke.sh`：自动编译跑 day01+day06 测试 |

## Week 2（08-25 ~ 08-31）· 并发 C++ + Linux 进程/调试观察

| 日期 | 目录 | C++（~100min） | Linux（~70min） |
|------|------|----------------|-----------------|
| 08-25 | day08 | `ThreadSafeQueue` | `ps -eLf` / `top -H`：线程视角；对照你的多线程程序 |
| 08-26 | day09 | MPMC 不丢任务 | `nice`/`renice`/`ulimit`；有界资源直觉 |
| 08-27 | day10 | 死锁制造与修复 | **`gdb` + `strace`**：死锁时看线程栈与 futex |
| 08-28 | day11 | `atomic` | `lscpu`、缓存行概念笔记；为何乱共享会慢 |
| 08-29 | day12 | 100Hz 传感器 → RingBuffer | `date`/`timedatectl`；墙钟 vs 单调时钟笔记 |
| 08-30 | day13 | 消费端延迟/丢包 | 日志重定向到文件；`tail -f` 观察运行 |
| 08-31 | day14 | Week2 C++ 复盘 | CMake 最小工程收束 week2；`ldd` 看依赖 |

## Week 3（09-01 ~ 09-07）· 系统编程（C++ 写 Linux API）+ 命令加深

> 这周 C++ 与 Linux 绑得更紧：用 C++ 调用 POSIX，同时命令行会排障。

| 日期 | 目录 | C++（~100min） | Linux（~70min） |
|------|------|----------------|-----------------|
| 09-01 | day15 | CMake 多文件 + ASan 跑通 | 目录规范 `build/`；`cmake -S -B`；对比 Makefile 思路 |
| 09-02 | day16 | C++ 实现 `mycat`/`mycp`（POSIX I/O） | `strace -e open,read,write` 跟踪自己的 mycp |
| 09-03 | day17 | C++：`fork`/`exec`/`wait` 拉起工具 | 僵尸进程实验；`ps` 看 Z 状态再 `wait` |
| 09-04 | day18 | C++：信号 + `atomic` 标志优雅退出 | `kill -TERM`/`-INT`；对比 SIGKILL |
| 09-05 | day19 | C++：管道或 shm 传传感器帧 | `ipcs`/`ls -l /dev/shm`（若用 shm） |
| 09-06 | day20 | C++：给 Hub 加简单日志模块 | `strace`/`top`/`ps` 系统化排查笔记 |
| 09-07 | day21 | 把 week1–2 收进统一 CMake | 口述+笔记：进程/线程/僵尸/信号安全 |

## Week 4（09-08 ~ 09-16）· 网络与 Hub（双轨融合）

| 日期 | 目录 | C++（~100min） | Linux（~70min） |
|------|------|----------------|-----------------|
| 09-08 | day22 | TCP echo server/client | `ss -lntp`/`netstat`；看监听端口 |
| 09-09 | day23 | 定长头协议 + 粘包处理 | `tcpdump -i lo port …` 抓本机包（可选） |
| 09-10 | day24 | Hub：传感 + RingBuffer + TCP | 用 `top`/`ss` 边跑边观察资源 |
| 09-11 | day25 | Hub：滤波/异常检测 + 日志开关 | `journalctl` 概念 or 文件日志轮转小脚本 |
| 09-12 | day26 | 压测延迟/丢包并修 1 点 | 压测时用 `top`/`perf stat`（有则用）记录 |
| 09-13 | day27 | README + 简单重连 | 发布：`strip`/`ldd`/一键 `run.sh` |
| 09-14 | day28 | 面试口述（C++ 题） | 面试口述（Linux 题）各准备一组 |
| 09-15 | day29 | C++ 弱项最小 Demo | Linux 弱项最小 Demo |
| 09-16 | day30 | Hub 演示 + 项目故事 | 演示时现场用 Linux 工具展示排障一手 |

---

## 每天时间盒（建议）

1. **0:00–1:40** C++ 必交产出  
2. **1:40–2:50** Linux 必交产出（命令练习写进当天目录 `linux-notes.md`）  
3. **2:50–3:00** 勾选 `PROGRESS.md`，两句话收获  

## 积压规则

- 当天两边都要有「最小完成」；若超时，C++ 保核心测试，Linux 保笔记命令清单  
- 最多积压 1 天  
- 仍说「开始今天学习」→ 我按双轨给你**写代码/练习需求**
