# Day27 · Linux 笔记

日期：2026-09-13  
环境：暂无 WSL；Hub 代码学完 24–27 后一起写。发布命令有二进制再跑。

---

## `ldd`

- 看可执行文件的**动态** `.so` 依赖和加载路径，不是编译期所有库。
- `not found`：换机器会立刻起不来。README 要写需要 g++ 运行时 / Ubuntu 版本。
- Debug+ASan 的 `ldd` 里会有 `libasan`，不要当发布版。Release 再 `ldd` 一次应没有它。
- 只对自己编的二进制用。

## `strip`

- 删调试符号，体积变小；`gdb` 函数名变差。
- 演示/盘小可以 strip；自己留一份 unstripped 才能 `bt`。
- 不要 strip ASan Debug 包当日常调试。`file` 能看出 stripped 与否。

## `run.sh`

- 以脚本所在目录为根（不要写死 `/home/你的名字`）。
- `set -euo pipefail`；没有二进制就 `cmake -S -B` + `--build`。
- 默认端口、日志级别可覆盖。`exec` 把进程换成 server，Ctrl+C 打到它身上。
- Windows 在 WSL 里跑。`chmod +x run.sh`。
- 后台起服务要记 PID；演示优先前台 `exec`。

## 和 README 的关系

别人只看「依赖 + 编译 + 运行」三节必须能看到 seq 在涨。`ldd` 有 `not found` 不能写「单文件即可」。端口占用：`ss -lntp`，先 TERM 旧进程。
