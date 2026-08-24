# Day20 · 排障清单与 Hub 简单日志（详细预习）

日期：2026-09-06

预习目标：先把 `ps` / `top` / `strace` 收成一套可重复的排查清单，再给后面的 Telemetry Hub 写一个能开关、能落地的简单日志模块。上机先练命令，再写代码。

---

# 一、Linux：进程卡住了，按这张清单走

线上慢或卡死，不要先改代码。先回答三个问题：**进程还在吗？它在什么状态？它卡在哪个系统调用？** `ps`/`top`/`strace` 分工不同，凑成一条流水线。

## 1. 总清单（背下来，上机对着自己的程序走一遍）

```text
1. ps：还在不在？PID、PPID、STAT、CPU、RSS、命令行
2. top / top -H：CPU 是不是打满？是算力还是在睡？线程各占多少
3. 认 STAT：R / S / D / Z（Day17 已见 Z）
4. strace -p PID：现在堵在哪个 syscall
5. 仍像逻辑错：再 gdb attach（Day10/Day15）
```

工具边界：

| 工具 | 像什么 | 适合问 |
|---|---|---|
| `ps` | 拍照 | 有谁、状态、谁是父 |
| `top` | 直播 | CPU/内存是否在涨、哪个进程热 |
| `strace` | 窃听内核对话 | 卡在 `read`/`futex`/`poll` 还是疯狂失败 |
| `gdb` | 看用户态 | 哪一行、锁在谁手里、变量是啥 |

## 2. `ps`：先拍清楚身份

```bash
ps aux | grep your_app
ps -o pid,ppid,stat,pcpu,pmem,rss,nlwp,wchan,cmd -p PID
ps -eLf | grep your_app          # 线程：LWP 列，NLWP 是线程数
```

认这些：

- **STAT**：`R` 跑或等 CPU；`S` 可中断睡（等 I/O、条件变量、sleep）；`D` 不可中断（常等磁盘，kill 也难）；`Z` 僵尸。
- **RSS**：常驻内存。只涨不跌要想泄漏（再靠 ASan/堆剖析）。
- **NLWP**：线程数。Hub 预期 3～4 个工作线程，变成 300 就不对。
- **WCHAN**：睡在哪个内核等待点（如 `futex_wait`、`pipe_read`）。有的内核会显示。

`ps` 是快照。怀疑它抖，连拍几次或改用 `top`。

## 3. `top`：看是忙还是闲

```bash
top
top -H -p PID          # 只看这个进程的线程
# 按 H 切换线程模式；q 退出
```

读法：

- **CPU 接近 100% × 线程数**：在算，或死循环。再看是不是某个线程独大。
- **CPU 很低、进程还「卡住」**：多半堵在 syscall（锁、`recv`、磁盘）。这时 `strace` 比 `top` 有用。
- **内存持续爬**：泄漏或缓存没上限（无界队列）。

`htop` 更好认，没有就 `top`。测性能时 `top` 本身也吃一点 CPU，但排障可以忽略。

## 4. `strace`：贴上去看卡在哪

Day16 用 `strace` 跟自己的 `mycp` 启动全过程。今天跟**已经在跑**的进程：

```bash
strace -p PID
strace -p PID -e trace=network,file,desc
strace -p PID -T -f              # -T 每个调用耗时；-f 跟子进程/线程
strace -c -p PID                 # 先汇总一会儿，Ctrl+C 看谁最耗时
```

典型画面：

```text
read(3,  <卡住，一直不返回>          # 等数据 / 对端不关
futex(0x..., FUTEX_WAIT, ...)     # 等锁或条件变量（Day10 死锁也是它）
poll([{fd=5, events=POLLIN}], ...)# 等 socket 可读
write(4, "...", 4096) = -1 EAGAIN # 非阻塞写，缓冲满
```

读 strace 的三个问题：

1. **卡在哪一次调用不回来？** 那就是阻塞点。
2. **是否疯狂重复失败？** 例如反复 `ENOENT`、`ECONNREFUSED`——逻辑在空转。
3. **是否小 I/O 爆炸？** 每次 `write` 几个字节（Day16 缓冲太小）。

**不要**在正式压测时一直开着 `strace`：每个 syscall 都被拦截，延迟会假高。排障开，测速关。

## 5. 一张决策表（口述就按这个讲）

```text
STAT=Z           → 父没 wait（Day17），别 kill 僵尸
STAT=D           → 磁盘/设备，先 iostat/dmesg，别只看 CPU
CPU 打满         → 死循环或真在算；gdb 看调用栈
CPU 很低 + 卡住  → strace 看 syscall；futex → 想锁/cv
线程数暴涨       → 创建线程没 join，或每连接一个线程没上限
RSS 暴涨         → 无界队列 / 泄漏；对照日志有没有「只产不消」
```

## 6. 易错点（Linux 侧）

1. **只看 CPU，忽略 D/Z**  
   卡死不一定热。
2. **strace 当 profiler**  
   它改变时间。`perf` / 计时日志才是测速（Day26）。
3. **`grep` 到的是自己的 grep**  
   `ps aux | grep foo` 会多一行。认命令列，或 `pgrep -a foo`。
4. **attach 前没权限**  
   `strace -p` 对别人的进程要权限；对自己的实验进程即可。

## Linux 口述（预习时自己答）

1. **进程卡住了你怎么查？**  
   `ps` 看在不在和 STAT → `top`/`top -H` 看 CPU 和线程 → `strace -p` 看堵在哪个 syscall → 像锁/逻辑再 gdb。
2. **Z 状态意味着什么？**  
   已死，父未 wait。找 PPID 修回收，不是 `kill -9`。
3. **strace 和 gdb 怎么分工？**  
   strace 看系统交互；gdb 看用户态变量和栈。

## Linux 上机（预习不用敲）

对已有程序（mycp、fork demo、信号 demo 或传感器产销）各走一遍清单：`ps`、`top`、`strace`。笔记写清：看到了哪些系统调用、瓶颈可能在哪。见 `week3-linux/day20-observability/TASK.md`。

---

# 二、C++：给 Hub 加简单日志模块

## 为什么需要

Hub（Week4）会有传感器线程、控制线程、网络线程。出了问题你要知道：**谁、在什么时刻、做了什么、失败码是什么**。`std::cout` 满天飞会乱、不能关、崩溃时可能丢缓冲。今天做一个小模块，后面几天直接用。

上机任务：简单日志模块——能分级、能开关、能写到文件或 stderr；别在热点里无节制刷。

## 1. 最小接口就够

```cpp
enum class LogLevel { Error, Warn, Info, Debug };

void log_set_level(LogLevel min);     // 低于此级别不输出
void log_set_file(const char* path);  // nullptr 表示 stderr
void log_write(LogLevel lv, const char* fmt, ...);
```

或 C++ 风：

```cpp
#define LOG_INFO(...)  log_write(LogLevel::Info, __VA_ARGS__)
#define LOG_ERROR(...) log_write(LogLevel::Error, __VA_ARGS__)
```

约定：

- **Error**：必须看（open 失败、校验失败、断开）。
- **Warn**：还能跑但不对（丢帧、短写重试很多次）。
- **Info**：启动、退出、连接建立、频率摘要（每秒一行，不要每帧）。
- **Debug**：每帧 seq——默认关。100Hz 打日志会比业务还慢。

开关：运行时 `log_set_level(LogLevel::Warn)`，或环境变量 `HUB_LOG=info`。Day25 会再做「可开关」。

## 2. 实现要点（和 Day16/18 对上）

- **一行一条**，带时间、级别、线程名或 tid。时间用墙钟方便人对表；测延迟仍用单调时钟（Day12）。
- **写文件用 `open/write` 或 `fstream`**。要可靠就定期 `flush`/`fsync`（慢）；要快就满缓冲再刷。崩溃可能丢最后几 KB——笔记里写清楚你选哪种。
- **线程安全**：多线程 `<<` 到同一个 stream 会撕行。一把 `mutex` 护住「拼一行 + 写出」。不要在持业务锁时再打日志（先解锁，或只用无锁缓冲）——避免和 Day10 死锁同类问题。
- **信号安全**：Day18 的 handler **不要**调用这个日志。handler 只改 `atomic`。退出路径回到主线程再 `LOG_INFO("bye")`。
- **RAII**：模块析构或 `atexit` 里 flush + close。优雅退出流程里显式 flush。

```cpp
void log_write(LogLevel lv, const char* fmt, ...) {
    if (lv > g_min.load()) return;
    char line[512];
    // 拼时间、级别、正文（注意截断）
    std::lock_guard<std::mutex> lk(g_mu);
    // write(fd, line, len)；检查短写
}
```

固定栈上缓冲，避免日志路径再 `new`（失败时还能说话）。

## 3. 和排障命令怎么配合

日志告诉你「业务认为自己在干什么」；`strace` 告诉你「内核看见什么」。两者对不上就是线索：

- 日志说「已写出」，strace 没有对应 `write` → 还在用户态缓冲，没 flush。
- 日志停了，strace 卡在 `futex` → 死锁或没 notify。
- 日志疯狂刷屏，`top` 里 CPU 在日志线程 → 把级别关掉再测。

所以日志模块本身要能**关**：测速、压测（Day26）默认 Warn。

## 4. 易错点（看懂再上机）

1. **每帧 Info**  
   100Hz × 多线程会把磁盘和锁打满，延迟数字全假。
2. **无锁的 `std::cout`**  
   行交错，看起来像逻辑错。
3. **日志里拿业务锁**  
   和 Day10 一样能死锁。
4. **handler 里打日志**  
   Day18：异步信号不安全。
5. **路径写死绝对路径还不处理打开失败**  
   `open` 失败要回退到 stderr，并设一个「日志已废」标志，避免每次再失败。

## C++ 面试口述（预习时自己答一遍）

1. **进程卡住了你怎么查？（工程版）**  
   先 ps/top/strace 定位状态和 syscall；再看日志最后一行时间戳对不对得上；最后 gdb。三件套，不是只猜代码。
2. **为什么不能在热点路径狂打日志？**  
   锁、格式化、write syscall 都贵；还会改变时序，把竞态「修没」或造出新的。
3. **日志和 strace 看到 write 不一致说明什么？**  
   用户态还没 flush，或写到了别的 fd。先分清缓冲 I/O 和系统调用（Day16）。

## C++ 上机（预习不用写）

见 `week3-linux/day20-observability/TASK.md`：

- 简单日志模块（分级 + 能关 + 写文件或 stderr）
- 对已有程序做一次 ps/top/strace，笔记里写系统调用和可能瓶颈
- 模块放到以后 Hub 能 `#include` 的位置（如 `common/` 或当天目录，Week4 再搬）
