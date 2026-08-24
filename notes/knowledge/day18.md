# Day18 · kill 信号与优雅退出（详细预习）

日期：2026-09-04（建议 WSL）

预习目标：先在 Linux 上对同一进程分别发 `-TERM`/`-INT` 和 `-KILL`，看谁能清理；再学 C++ 里 `sigaction` + `atomic` 标志做优雅退出。上机先练命令，再写代码。

---

# 一、Linux：`kill` 是发信号，不是只有「杀死」

Day02 已经用过 `kill` / `kill -9`。今天要分清三种你天天会碰到的信号，并亲手对比：**进程有机会清理** vs **内核直接拔电源**。

信号是内核往进程上贴的一张条子：「请停」「请退出」「非法内存」。进程可以注册处理函数（handler），但有的信号不能注册。

## 1. 三个信号，先记行为

| 你敲的 | 信号 | 默认动作 | 能捕获吗 |
|---|---|---|---|
| `Ctrl+C` 或 `kill -INT PID` | SIGINT (2) | 退出 | 能 |
| `kill PID` 或 `kill -TERM PID` | SIGTERM (15) | 退出 | 能 |
| `kill -9 PID` 或 `kill -KILL PID` | SIGKILL (9) | 立刻死 | **不能** |

```bash
kill -TERM PID      # 请你退出，可以刷日志、join 线程、关 socket
kill -INT  PID      # 和终端 Ctrl+C 同类
kill -KILL PID      # 内核撕掉进程，来不及析构
# 等价写法
kill -15 PID
kill -2  PID
kill -9  PID
```

习惯：**先 TERM，等几秒还在再考虑 KILL。** 机器人/服务上乱 `-9` 可能留下半开的设备句柄、没写完的录包、没释放的共享内存。

`kill` 的名字容易误导：默认 15，是「请求终止」，不是 9。

## 2. 对照实验（今天 Linux 上机的核心）

准备一个会「收到信号后打印并睡一会儿再退」的程序（就是你的 C++ demo）。另开一个终端：

```bash
ps aux | grep your_demo
kill -TERM <PID>          # 应看到程序打印 “got SIGTERM”，然后清理退出
```

再跑一次，这次：

```bash
kill -INT <PID>           # 应同样走优雅路径（如果你注册了 SIGINT）
```

再跑一次：

```bash
kill -KILL <PID>
# 程序的 handler 不会跑；终端里没有 “got SIGKILL”
# ps 里进程直接没了
```

把三次现象写进 `linux-notes.md`：**TERM/INT 能看见清理日志；KILL 不能。**

进程已经不在时：

```bash
kill -TERM 999999
# kill: (999999): No such process
```

没权限杀别人的进程会 `Operation not permitted`。

## 3. 和 Day02 / Day17 串起来

- Day02：`ps` 拍照，`top` 直播，`kill` 发信号。
- Day17：子进程被信号打死，父的 `waitpid` 走 `WIFSIGNALED`，`WTERMSIG` 就是 9 或 15。
- 今天：你自己决定「收到 2 和 15 时做什么」；9 你做不了什么。

```bash
# 父 wait 到被 KILL 的子时，状态不是「exit code=0」
# 而是 “killed by signal 9”
```

## 4. 为什么服务脚本都是先 TERM 再 KILL

`systemd`、Docker 停容器：先发 SIGTERM，等 `TimeoutStopSec`，还在才 SIGKILL。给你时间：

- 刷磁盘上的日志
- 告诉客户端「我要下线」
- join 工作线程，避免用已经析构的对象
- 关掉 fd，让对端 `recv` 到 0

KILL 之后这些都不会发生。所以「线上杀进程」默认不该是 `-9`。

## 5. 易错点（Linux 侧）

1. **只试了 Ctrl+C，没试 `kill -TERM`**  
   部署时运维/守护进程发的是 TERM。只处理 INT，容器停的时候你的清理不会跑。
2. **对僵尸 `kill -9`**  
   Day17：僵尸已死。找父进程 wait。
3. **PID 看错，杀了别人的 bash**  
   `ps` 对一下命令行再下手。杀自己的实验进程。

## Linux 口述（预习时自己答）

1. **`kill` 和 `kill -9` 差在哪？**  
   默认 TERM，进程能捕获并清理；`-9` 是 KILL，不能捕获，内核直接结束，析构/关文件都来不及。
2. **Ctrl+C 是哪个信号？运维杀服务常用哪个？**  
   Ctrl+C → SIGINT。`kill` / systemd 停服务 → SIGTERM。两者都该处理。
3. **SIGKILL 能抓住吗？**  
   不能。没有 handler 能跑。

## Linux 上机（预习不用敲）

对自己的优雅退出 demo 分别 `kill -TERM`、`kill -INT`、`kill -KILL`，对照日志和 `ps`。写入 `linux-notes.md`。

---

# 二、C++：信号 + `atomic` 优雅退出

## 为什么需要

Ctrl+C 要停干净：刷日志、停线程、关 socket。真机部署很看重。错误做法是在 handler 里 `printf`、拿锁、`join` 线程——既不安全，也容易死锁。正确做法：**handler 只改一个原子标志，真正的清理放回主线程。**

上机任务：捕获 SIGINT / SIGTERM；停线程、刷日志、再退出；Ctrl+C 不留僵尸、不崩。

## 1. 异步信号安全：handler 几乎什么都不能做

信号可能在**任意指令之间**到达，包括你正拿着 mutex、正执行 `malloc` 的时候。handler 里再拿同一把锁 → 自己等自己 → 死锁。handler 里再 `printf`/`malloc` → 也可能和被打断的调用缠在一起。

POSIX 保证在 handler 里安全的函数很少（`write` 到已经打开的 fd、`_exit`、改 `volatile sig_atomic_t` 等）。C++ 里最稳妥的工程约定：

```cpp
#include <atomic>
#include <csignal>

std::atomic<bool> g_running{true};

void on_signal(int /*sig*/) {
    g_running.store(false, std::memory_order_relaxed);
}
```

只写一个 `atomic<bool>`。不打印、不加锁、不 `join`、不 `delete`、不碰复杂容器。

主循环：

```cpp
while (g_running.load()) {
    // 干活；阻塞等待时要想好「怎么被唤醒」
}
// 这里才：notify_all、join 线程、flush 日志、close fd
```

对照内核：SIGTERM 像「请析构」；handler 只是把「开始析构」这个开关拨下来。

## 2. 用 `sigaction`，不要用老式 `signal`

```cpp
#include <signal.h>

struct sigaction sa{};
sa.sa_handler = on_signal;
sigemptyset(&sa.sa_mask);
sa.sa_flags = 0;              // 需要可中断的阻塞调用时，不要乱加 SA_RESTART
sigaction(SIGINT,  &sa, nullptr);
sigaction(SIGTERM, &sa, nullptr);
```

- `signal()` 在不同 Unix 上语义不一致（有的调用一次就恢复默认）。
- `sigaction` 能控制：进入 handler 时屏蔽哪些信号、被打断的 syscall 是否自动重启。
- **SIGINT 和 SIGTERM 都要注册。** 只注册一个，另一种杀法就不优雅。

不要注册 SIGKILL：内核不让，`sigaction` 会失败。

`SA_RESTART`：若加上，很多阻塞 syscall 被信号打断后会自己重试，你的 `read` 看不到 `EINTR`，循环可能停不下来。优雅退出时常**不加**，让阻塞调用返回 `EINTR`，循环检查 `g_running`。Day16 写过 `EINTR` 重试——退出路径上应改成：`EINTR` 且 `!g_running` 则停。

## 3. 完整退出流程（和条件变量、线程一起）

```text
handler:     g_running = false
主线程醒过来（EINTR / 超时 / notify）
    → 对队列 shutdown + cv.notify_all()     // Day08
    → join 所有工作线程
    → flush 日志、close socket/fd
    → return 0
```

关键：线程可能正堵在 `cv.wait` 或 `recv` 上。只改标志、不唤醒，它们永远等。所以：

- 条件变量：`shutdown()` 里置位并 `notify_all`（Day08 已练）。
- 阻塞 `read`/`recv`：依赖 EINTR，或关掉 fd 让调用返回，或用超时。
- 不要在 handler 里 `notify`（锁不安全）。主线程或专门的「看门」路径来 notify。

```cpp
// 示意：工作线程
while (g_running.load()) {
    std::unique_lock<std::mutex> lk(m);
    cv.wait_for(lk, 100ms, [] {
        return !g_running.load() || !queue.empty();
    });
    if (!g_running.load()) break;
    // 取任务……
}
```

`wait_for` 带超时，最坏 100ms 能看到标志；更干净是 `shutdown` 里 `notify_all`。

## 4. 和 Day17 的子进程一起想

若你 fork 过子进程：优雅退出时父要 `waitpid` 收尸，否则 Ctrl+C 后可能留 Z。也可以在停自己之前先给子发 SIGTERM，再 wait。不要只退父、不管子。

## 5. 易错点（看懂再上机）

1. **handler 里 `printf` / 拿锁 / `join`**  
   可能死锁或再入 libc。只设原子标志。
2. **只处理 SIGINT 不处理 SIGTERM**  
   本机 Ctrl+C 好看，部署一 `kill` 就不清理。
3. **退出后线程还在碰已毁对象**  
   必须先 join 再析构队列/缓冲。析构顺序：停产 → 抽干或丢弃 → join → 再毁共享结构。
4. **`g_running` 用普通 `bool`**  
   编译器可能把 `while (g_running)` 优化成只读一次。用 `std::atomic<bool>`。
5. **以为能捕获 SIGKILL**  
   不能。KILL 的对照实验就是为了建立这个直觉。

## C++ 面试口述（预习时自己答一遍）

1. **为什么 handler 里只设标志？**  
   信号异步到达，可能打断持锁或 `malloc`。handler 里再拿锁/打印会死锁或损坏堆。只写 `atomic`，复杂清理回主线程。
2. **SIGKILL 能抓住吗？**  
   不能。内核直接结束进程。优雅退出只能覆盖 INT/TERM。
3. **优雅退出要做哪些事？**  
   置 `running=false` → 唤醒所有阻塞 → join 线程 → 刷日志、关 fd → 父 wait 子进程 → 再退出。

## C++ 上机（预习不用写）

见 `week3-linux/day18-signals/TASK.md`：

- 捕获 SIGINT / SIGTERM
- 停线程、刷日志、再退出
- Ctrl+C 不留僵尸、不崩
- 用第一节的三种 `kill` 验收
