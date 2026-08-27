# Day10 · Linux 笔记（gdb / strace / futex）· 理论为主

日期：2026-08-27  
环境：概念 + 笔记（上机可选）

---

## 今日命令

| 命令 | 作用 |
|------|------|
| `gdb ./程序` | 用调试器**启动**程序来调 |
| `gdb -p <PID>` | 把 gdb **挂到已经卡住的进程**上 |
| `(gdb) info threads` | 列出这个进程里的**线程**（`*` 是当前线程） |
| `(gdb) bt` | backtrace：当前这条线程的调用栈 |
| `(gdb) thread apply all bt` | 对**所有线程**各打一份 bt（死锁首选） |
| `(gdb) thread N` | 切到编号 N 的线程；再 `bt` 只看这一条 |
| `strace -p <PID>` | 打印该进程正在做的**系统调用** |
| `strace -f -p <PID>` | 同上，并 follow **子进程/线程**（多线程必加） |
| `top -H -p <PID>` | 按**线程**直播（H ≈ Threads）；死锁时 CPU 常接近 0 |

gdb 的 `thread`、`top -H` 看的都是**线程**，不是进程。进程是容器（一个 PID），线程是里面的执行流。

---

## 小考

### 1. 程序卡住、CPU 接近 0，更像死循环还是在等？和 Day08 的 wait 怎么对上？

更像在等，不是空转死循环。Day08 的 `cv.wait`、今天的 `mutex::lock` 在 `top -H` 里都可能 CPU 很低。要用 strace/gdb 区分：队列空等 vs 交叉等锁（死锁）。

### 2. 用户态 `std::mutex` 睡下去时，内核里常见在等什么？（一个词）

**futex**（fast userspace mutex）。

### 3. 死锁时 gdb 建议先打哪两条？各自看到什么？

- 第一条：`info threads` — 有哪些线程，当前停在哪条
- 第二条：`thread apply all bt` — 每人一份栈，看谁卡在哪把锁

### 4. `bt` 是什么意思？死锁时栈顶常见停在哪类函数？

backtrace（回溯）。栈顶常见 `std::mutex::lock` / `pthread_mutex_lock` / futex 等待，不是在算业务代码。

### 5. `strace` 里和 mutex 最相关的调用？`FUTEX_WAIT` 表示什么？

**futex**。`FUTEX_WAIT`：睡在锁对应的地址上，等别人释放再被叫醒。看见它 = 在等锁，不是在算。

### 6. `strace -f` 的 `-f` 是干什么的？多线程为啥要加？

**follow**：子进程/线程的系统调用也跟踪。不加往往只看到主线程，工人在等锁会漏掉。

### 7. gdb 和 strace 怎么配合（三步顺序）？

1. `top -H`：CPU 低 → 像在等  
2. `strace -f`：`FUTEX_WAIT` → 确认在等锁  
3. `gdb -p PID` → `thread apply all bt` → 哪两把锁、源码哪两行  

这是检测；工程上以预防（加锁顺序 / `scoped_lock`）为主。

### 8. 卡住时为什么先不要 `kill -9`？

`-9` 是 SIGKILL，内核直接干掉，来不及析构、现场也没了。先 gdb 看完，再用普通结束（gdb `quit` 或 `kill` 不带 `-9`）。

---

## 口述自检

1. 死锁：`info threads` 再 `thread apply all bt`
2. futex / FUTEX_WAIT = 睡等锁
3. `-f` = follow 线程/子进程
4. 预防为主，gdb/strace 是复现之后的检测

---

## 上机（可选）

未在 WSL 实挂进程；理论已收。观察用程序：`deadlock_bad.cpp`。
