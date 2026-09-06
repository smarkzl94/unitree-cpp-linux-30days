# Day18 · Linux 笔记（kill 发信号）

日期：2026-09-04

`kill` 是给进程贴一张条子（信号），不是只有「立刻杀死」。

---

## 要点

- **`kill PID` 和 `kill -TERM PID` 是同一张条子：SIGTERM（15）。** 没写信号时默认就是 TERM。运维/Docker 停服务也常用这个。
- **Ctrl+C / `kill -INT PID`：SIGINT（2）。** 能捕获，可以打日志、关文件。INT 和 TERM 都要处理。
- **`kill -9` / `kill -KILL`：SIGKILL。** 内核直接结束，不能捕获，handler 不会跑。先 TERM，还在再考虑 9。
- 对着僵尸 `kill -9` 没用（Day17）。

常用命令：

- `kill PID` 或 `kill -TERM PID`：请退
- `kill -INT PID`：和 Ctrl+C 同类
- `kill -9 PID`：立刻死
