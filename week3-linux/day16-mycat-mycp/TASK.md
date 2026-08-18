# Day 16 · mycat / mycp（2026-09-02）

Windows 上可用 MinGW 或 WSL。优先系统调用风格（POSIX：`open/read/write`；Win：`CreateFile/ReadFile` 亦可，但建议 WSL 练 POSIX）。

## 必须实现
- `mycat <file>`
- `mycp <src> <dst>`，大文件拷正确
