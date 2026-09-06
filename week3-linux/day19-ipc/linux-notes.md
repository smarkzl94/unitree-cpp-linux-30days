# Day19 · Linux 笔记（ipcs / /dev/shm / 看见 IPC）

日期：2026-09-05

两个进程地址空间隔离，要传数据必须走 IPC。今天推荐匿名管道；命令用来「看见对象在哪」。

---

## 要点

- **`ipcs` / `ipcs -m`：System V IPC**（消息队列、信号量、SysV 共享内存）。空表 ≠ 没有 IPC。
- **`ls -l /dev/shm`：POSIX 共享内存文件**（tmpfs）。`df -h /dev/shm` 看这块内存盘还剩多少，不列文件名。
- **匿名 pipe：** `ipcs` 和 `/dev/shm` 都看不见名字。看 `ls -l /proc/<PID>/fd` 里的 `pipe:[...]`。
- **FIFO：** 文件系统里有路径，`ls -l` 类型是 `p`。
- 程序崩溃后 POSIX shm 名字可能还在，调试先 `ls /dev/shm`，确认是自己的再删。跨机器不能用 shm。

常用命令：

- `ipcs` / `ipcs -m`
- `ls -l /dev/shm`；`df -h /dev/shm`
- `ls -l /proc/<PID>/fd`
