# Day25 · Linux 笔记

日期：2026-09-11  
环境：暂无 WSL；命令有环境再补。概念走文件轮转。

---

## journal vs 自己的文件

- `journalctl` 查的是 systemd journal（常在 `/var/log/journal`），不是 `open("hub.log")` 那个路径。
- 按单元 / 优先级 / 时间过滤，磁盘有上限，旧记录会被丢掉。
- 开发：自己写文件 + 轮转。部署：stdout 交给 journald，`journalctl -u myhub -f`。
- 前台跑 Hub 不必做成 `.service`。没有 systemd 不硬磕 `journalctl`。

## 轮转

- 100Hz Debug 一天能到 GB，磁盘满了比程序 bug 更先停机。按大小或按天切，只留最近 N 份。看体积用 `ls -lh`（`-l` 长列表，`-h` 人读单位）。
- `mv hub.log hub.log.1` 只改目录名，进程里旧 fd 仍写原来的 inode。新 `hub.log` 不会自动接上。要 `close` + 再 `open`，或停进程再切。
- 正规：`kill -USR1` → handler 只置 `g_reopen` → 主线程 `log_reopen()`。最小版：重启才换文件。
- `tail -f` 跟着旧 inode；`tail -F` 按名字跟到新文件。
- `kill -9` 不保证最后几行进文件（用户态缓冲没 flush）。优雅退出才刷。

## 和 Hub 的关系

- 日志路径要和轮转脚本是同一个文件。
- 压测用 `--log=warn`，不要每帧 Debug，否则 `ls -lh` 和 CPU 一起炸。
