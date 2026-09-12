# 知识盲区与错题（个人）

按「你问过、混过、还没动手」收。不是全课目录。速查仍看 [recap/linux.md](../recap/linux.md)、[recap/cpp.md](../recap/cpp.md)。CMake 语法不用背，见 [cmake-lists.md](./cmake-lists.md)。

---

## 一、最大缺口：概念有了，程序几乎没写

Week3（Day15–20）知识卡多为「进行中」。下面这些**面试能说、手上没跑过**，优先补 WSL 后各写一个最小程序：

| 课 | 缺的上机 |
|----|----------|
| Day15 | 多文件 CMake + Debug ASan 故意越界 |
| Day16 | `mycat` / `mycp`，`strace` 看短读 |
| Day17 | `fork`/`exec`/`wait`；故意不 wait 看 Z |
| Day18 | `sigaction` + `atomic`；TERM/INT vs `-9` |
| Day19 | `pipe` 传一帧并校验 |
| Day20 | 对着自己的程序走 ps → top → strace |
| Day10 | 有 `deadlock_bad`，修好的 `scoped_lock` 对照还没有 |
| Day14 | Week2 CMake/`ldd`、口述填空未完成 |

没有 Ubuntu 时：Windows 的 `ps` **不是** Linux 那套；`fork`/`kill -TERM` 都要在 WSL 里练。

---

## 二、你容易混的（错题本）

### 1. `kill PID` 和 `kill -TERM`

**同一张条子：SIGTERM（15）。** 省略信号名时默认就是 TERM。  
**不同的是 `kill -9`（SIGKILL）**：不能捕获，handler 不会跑。  
Ctrl+C = INT（2），也能捕获。部署停服务常用 TERM，所以 INT 和 TERM 都要处理。

### 2. gdb vs strace

| | 看见什么 |
|--|----------|
| **strace** | 程序 ↔ 内核（open/read/futex） |
| **gdb** | 你的行号、变量、`bt` |

文件拷错、卡在系统调用 → strace。算错、segfault 看局部变量 → gdb。

### 3. POSIX 不是一个库名

Unix 风格**接口约定**。`fork`/`open` 是内核能力，C 库给你同名函数。Windows 原生没有这套。

头文件别混：`fork` 在 `<unistd.h>`；`waitpid` 在 `<sys/wait.h>`；**`perror` 在 `<cstdio>`**；`pid_t` 在 `<sys/types.h>`（常被 `unistd.h` 带进来）。

### 4. fd

进程里的**整数门票**（0/1/2 是标准输入输出错误）。每个进程一张表。`fork` 时拷贝一份，之后各关各的。

### 5. `fork` 返回两次

子拿到 `0`，父拿到子的 PID（`pid_t`）。不判断 `pid` 会父子各跑一遍后半段。`exec` **不新建进程**，是把当前进程换成新程序。

### 6. 僵尸 vs 杀父

Z = 子已死，父没 `wait`。**`kill -9` 僵尸没用。**  
`kill -9` **父**之后 Z 常被 PID 1 收掉，那是托底，不算程序写对。长期服务必须父自己 `waitpid`。

### 7. ASan「编译」

编译器往代码里插内存检查（越界、UAF）。**编译和链接都要** `-fsanitize=address`。只在 Debug 开。

### 8. 看见 IPC（Day19）

| 你用的 | 看哪 |
|--------|------|
| System V shm | `ipcs -m` |
| POSIX shm | `ls -l /dev/shm`（`df -h` 是看容量，不是列文件） |
| 匿名 pipe | `/proc/PID/fd` 里的 `pipe:[…]`；ipcs 里没有 |

`ipcs` 为空 ≠ 没建成。跨机器不能 shm。`htole`/`htobe` 是跨机器字节序，本机 pipe 传结构体可先不管。

### 9. CMake 只要会验收

`-S` 配置、`--build` 编译；改 cpp 只 `--build`；生成物进 `build/`。Lists 正文交给手册/AI，你检查有没有漏 `.cpp`、ASan 有没有两边加。

### 10. 信号 handler

只改 `std::atomic<bool>`。不要 `printf`、加锁、`join`、打日志。清理回主线程。普通 `bool` 可能被优化成只读一次。

---

## 三、Week1–2 里仍要能口述的（旧错）

这些课上写过代码，别只留 Week3：

- `volatile` **不能**替代 `atomic`
- `cv.wait` 必须 **`while` + `unique_lock`**，不能 `if` / `lock_guard`
- 测 Hz、延迟用 **单调时钟**，不要用 `date`/墙钟做分母
- `>` 只重定向 stdout；`>f 2>&1` 顺序不能反
- `ps -eLf` 中间是大写 **L**（线程）；小写 l 是 long

---

## 四、可以先不啃（别再平行铺）

- CMake `PUBLIC`/`INTERFACE` 细抠、顶层 `add_subdirectory` 全挂绿
- System V `shmget` 整套 API、POSIX shm 同步细节
- `SA_RESTART`、`SIGCHLD`+`WNOHANG` 循环（知道有这回事即可）
- 字节序 `htole`/`htobe`（到 TCP 再学）

---

## 五、建议补洞顺序（有 WSL 之后）

1. Day17：最小 `fork` + 不 wait 看 Z + `waitpid`  
2. Day18：`atomic` 标志 + `kill` / `kill -9` 对照  
3. Day16：缓冲循环 `mycp` + `strace` 一眼短读  
4. Day15：源外 CMake + 一次 ASan 越界报告  
5. Day19：pipe 一帧  

口述每天对着第二节那 10 条过一遍，比重新翻预习页有效。
