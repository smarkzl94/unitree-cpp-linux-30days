# Linux 每日任务清单（与 C++ 同天进行）

完整双轨日历见根目录 `PLAN.md`。每天在对应 `dayXX-*` 目录写 `linux-notes.md`。

## Week 1
| Day | Linux 必做 |
|-----|------------|
| 01 | `pwd` `ls -la` `man`；权限 rwx；`linux-notes.md` |
| 02 | `ps`/`top`/`kill` 观察自己的进程 |
| 03 | 管道与重定向；`grep`/`find` 搜代码 |
| 04 | `build.sh` + `chmod +x` |
| 05 | 软链/`du`；整理目录 |
| 06 | `/proc/cpuinfo` `/proc/meminfo` |
| 07 | `week1_smoke.sh` 编译跑 day01+day06 |

## Week 2
| Day | Linux 必做 |
|-----|------------|
| 08 | `top -H` / `ps -eLf` 看线程 |
| 09 | `ulimit -a`；`nice` 概念 |
| 10 | `gdb` + `strace` 查死锁 |
| 11 | `lscpu`；假共享笔记 |
| 12 | 墙钟 vs 单调时钟 |
| 13 | 日志重定向 + `tail -f` |
| 14 | CMake 收束；`ldd` |

## Week 3
| Day | Linux 必做 |
|-----|------------|
| 15 | out-of-source `cmake -S -B` |
| 16 | `strace` 跟踪 mycp |
| 17 | 僵尸进程实验 |
| 18 | `kill -INT/-TERM/-KILL` |
| 19 | 管道或 shm 观察命令 |
| 20 | ps/top/strace 排障清单 |
| 21 | 统一工程 + 口述复盘 |

## Week 4
| Day | Linux 必做 |
|-----|------------|
| 22 | `ss -lntp` |
| 23 | `tcpdump` 本机（可选） |
| 24 | Hub 运行时 top/ss |
| 25 | 日志轮转/落盘策略 |
| 26 | 压测时系统观察 |
| 27 | `run.sh` + `ldd` |
| 28 | Linux 面试口述一组 |
| 29 | Linux 弱项最小 Demo |
| 30 | 演示时现场用 Linux 工具 |

建议环境：**WSL2 Ubuntu**。
