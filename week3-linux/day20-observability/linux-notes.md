# Day20 · Linux 笔记（卡住了怎么查）

日期：2026-09-06

不要先改代码。先问：还在吗？忙还是睡？卡在哪个系统调用？

---

## 要点

- **顺序：`ps` → `top` / `top -H` → `strace -p` → 仍像逻辑再 gdb。**
- **`ps` 拍照：** PID、STAT、命令行。`R` 可跑；`S` 在睡；`D` 等磁盘；`Z` 僵尸（找父 wait）。
- **`top` 直播：** CPU 打满像在算或死循环；CPU 很低却卡住，多半堵在 syscall。
- **`strace -p PID`：** 听已经在跑的进程和内核说什么。常见 `nanosleep`、`read` 不返回、`futex`（等锁/cv）。
- **排障可以开 strace；测速度要关。** 它会拖慢程序。
- strace 看系统对话；gdb 看你的行号和变量。

常用命令：

- `ps aux | grep 程序`
- `ps -o pid,ppid,stat,cmd -p PID`
- `top -H -p PID`
- `strace -p PID`（`-f` 跟线程）
