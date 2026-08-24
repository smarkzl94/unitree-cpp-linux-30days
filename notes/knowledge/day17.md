# Day17 · 僵尸进程与 fork/exec/wait（详细预习）

日期：2026-09-03（建议 WSL）

预习目标：先在 Linux 上亲手造出一个 Z 状态再 `wait` 收掉，再学 C++ 里 `fork`/`exec`/`wait` 拉起工具。上机先练命令，再写代码。

---

# 一、Linux：看见僵尸（Z），再回收

进程退出后，内核还留着一张「死亡证明」：退出码、一些统计。父进程必须来领（`wait`/`waitpid`）。不领，子进程就变成**僵尸**（zombie）：几乎不占内存，但占一个 PID 槽。槽用尽，系统不能再 `fork`。

`ps` 里状态列是 `Z`，命令名旁边常有 `<defunct>`。

## 1. 先认进程状态（后面 Day20 还要用）

```bash
ps aux          # STAT 列
ps -o pid,ppid,stat,cmd -p PID
```

常见字母：

| STAT | 含义 |
|---|---|
| `R` | 可运行（正在跑或等 CPU） |
| `S` | 可中断睡眠（等 I/O、等信号） |
| `D` | 不可中断睡眠（常等磁盘，`kill` 也叫不醒） |
| `Z` | 僵尸：已死，父还没 wait |
| `T` | 停止（被 SIGSTOP/`Ctrl+Z`） |

`Z` **不是**「卡死还在跑」。它已经退出了，只是尸体还在进程表里。

## 2. 造一个僵尸（理解用，做完立刻收）

思路：父进程 `fork` 出子进程，子马上退出，父去睡觉、不 `wait`。这段时间 `ps` 能看到 `Z`。

预习先看现象；上机用你的 C++ 或下面这个最小壳：

```bash
# 示意：子进程立刻退出，父 sleep 30 秒不 wait
# 上机用自己的程序更清楚；不要在生产环境留僵尸
ps aux | grep Z
# 或
ps -eo pid,ppid,stat,cmd | awk '$3 ~ /Z/'
```

认两列：**PID**（僵尸自己）和 **PPID**（父）。要回收，必须让**父**调用 `wait`，或让父退出（僵尸被 `init`/PID 1 收养并自动收）。

对僵尸发 `kill` 通常没用：它已经死了，没有代码去处理信号。`kill -9` 也杀不掉僵尸。正确动作是修父进程。

## 3. 父 wait 之后 Z 消失

父里调用 `wait`/`waitpid` 后：

```bash
ps -o pid,stat,cmd -p 那个僵尸PID
# 应该：No such process
```

对照实验（上机按这个顺序写进笔记）：

1. 启动「子死、父睡」的程序，记下子 PID。
2. `ps` 看到 `Z`。
3. 让父执行 wait（或你写好的下半段），再 `ps`，Z 没了。
4. 再开一版：父立刻 wait，全程看不到 Z（或只存在极短，肉眼抓不到）。

## 4. 和「孤儿进程」别混

- **僵尸**：子已死，父还活着但不收。
- **孤儿**：父先死，子还活着。内核把子过继给 PID 1，子继续跑，不是 Z。

父退出时，它留下的僵尸会被 PID 1 收掉。所以「杀掉父进程，僵尸消失」看起来像修好了，根因仍是父没 wait。长期跑的服务（机器人节点）父不会退，必须自己 wait。

## 5. 易错点（Linux 侧）

1. **对着 Z 狂 `kill -9`**  
   无效。去找 PPID，修父进程。
2. **`ps` 里 `STAT` 是 `Zs` 之类**  
   第一字符是 `Z` 就是僵尸；后面字母是额外标志，先认第一个。
3. **多线程程序里 `fork`**  
   子进程只复制「当前那条线程」，其他线程消失，锁可能卡死。今天单线程实验；面试知道「多线程里乱 fork 危险」。

## Linux 口述（预习时自己答）

1. **僵尸进程是什么？如何避免？**  
   子已退出，父没 `wait`，进程表留着退出状态。避免：父 `wait`/`waitpid`；或设 `SIGCHLD` 处理并回收；不能靠 `kill` 僵尸。
2. **`ps` 的 Z 和 D 差在哪？**  
   `Z` 已死待收；`D` 还活着，卡在不可中断睡眠（常是磁盘），杀不死。
3. **为什么杀父进程僵尸会消失？**  
   父死后僵尸被 PID 1 收养并 wait。这是系统托底，不是你的程序写对了。

## Linux 上机（预习不用敲）

用当天程序造出 Z：`ps` 记下 PID/PPID/STAT，再让父 wait，确认 Z 消失。写进 `linux-notes.md`。建议全程 WSL。

---

# 二、C++：fork / exec / wait

## 为什么需要

Linux 上「再跑一个程序」的标准三步：`fork` 复制自己 → 子进程 `exec` 换成新程序 → 父 `wait` 回收。机器人里常有独立工具进程（标定、录包、看门狗拉起节点）。今天用 C++ 拉起 Day16 的 `mycat`（或 `/bin/ls`）。

## 1. 进程 vs 线程（必须先钉死）

- **进程**：独立地址空间、独立 fd 表（`fork` 时拷贝一份）。互相看不见对方的栈和堆（除非 IPC）。
- **线程**：同一进程里共享地址空间和 fd。一个线程 `close(fd)`，别的线程也没了这个 fd。

`fork` 出来的是新**进程**，不是新线程。Day08 的 `std::thread` 是线程。两者别混着说。

## 2. `fork`：调用一次，返回两次

```cpp
#include <unistd.h>
#include <sys/wait.h>
#include <cstdio>

pid_t pid = fork();
if (pid < 0) {
    perror("fork");
    return 1;
}
if (pid == 0) {
    // 子进程：fork 的返回值是 0
    // 这里准备 exec，或做一点事后 _exit
} else {
    // 父进程：返回值是子进程的 PID（>0）
    int status = 0;
    waitpid(pid, &status, 0);
}
```

内存画面（逻辑上）：

```text
fork 之前：  只有 P，地址空间一份
fork 之后：  P 和 C 各有一份地址空间的「视图」
             内核用写时复制（COW）：没人改的页先共享
             谁先写入，谁拿到自己的拷贝
```

所以 fork 看起来「拷了整个进程」，实际很快——直到你开始写大块内存。

**谁先跑？** 不确定。调度器说了算。不能写「fork 后父一定先 wait 再让子跑」这种假设；要靠 `wait` 同步「子已经结束」，而不是靠运气。

## 3. `exec*`：换成另一个程序，PID 不变

`exec` 不创建新进程。它用新程序的代码和数据**覆盖**当前进程映像。成功则后面的 C++ 一行都不会执行。

```cpp
if (pid == 0) {
    execl("./mycat", "mycat", "notes.md", static_cast<char*>(nullptr));
    perror("execl");   // 只有失败才到这里
    _exit(127);
}
```

- 第一个参数：路径。
- 后面：`argv[0]`、`argv[1]`、…、**必须以空指针结尾**。
- 失败才返回。子进程里失败用 `_exit`，不要 `exit`（少跑 atexit/冲父进程拷来的缓冲）。

常用亲戚：

| 函数 | 路径 | 参数 |
|---|---|---|
| `execl` | 你给路径 | 列表，空指针结尾 |
| `execv` | 你给路径 | `char* argv[]` |
| `execlp` / `execvp` | 在 `PATH` 里找 | 同左 |

拉起自己编的 `mycat` 用带路径的 `execl`/`execv`，别依赖 `PATH`。

## 4. `wait` / `waitpid`：领死亡证明

```cpp
int status = 0;
pid_t w = waitpid(pid, &status, 0);   // 阻塞到这个子结束
if (w < 0) {
    perror("waitpid");
}
if (WIFEXITED(status)) {
    std::printf("exit code=%d\n", WEXITSTATUS(status));
} else if (WIFSIGNALED(status)) {
    std::printf("killed by signal %d\n", WTERMSIG(status));
}
```

- `wait(NULL)`：收任意一个子，不够精确。
- `waitpid(pid, &status, 0)`：收指定子。今天用这个。
- `WNOHANG`：不阻塞，没有子退出就返回 0。循环里收多个子时会用。

不调用它们 → 第一节的 `Z`。

## 5. 典型模式（今天要写熟）

```text
父: fork
    ├─ 子: exec(mycat / ls / 你的工具)
    │      失败则 _exit
    └─ 父: waitpid → 打印退出码
```

这就是 shell 跑一条命令的简化版。Day18 会在这之上加信号；Day19 会在 fork 后、exec 前接管道。

## 6. 易错点（看懂再上机）

1. **fork 之后父子都跑同一套复杂逻辑，不判断 `pid`**  
   会各干一份，文件写两次、端口绑两次。第一件事就是 `if (pid == 0)` 分岔。
2. **忘记 wait**  
   演示用的短父进程一退，僵尸被 init 收，你「看不出问题」。长期服务会堆 Z。上机按第一节故意不 wait 一次。
3. **`exec` 失败后还 `return` 从 `main` 走**  
   子进程会把父的后半段再跑一遍。失败必须 `_exit`。
4. **多线程里 fork**  
   只复制当前线程，持锁的其他线程没了，锁可能永远锁着。今天单线程；生产代码用 `posix_spawn` 或只在单线程阶段 fork。
5. **`execl` 忘了结尾 `nullptr`**  
   未定义行为，参数吃飞。

## C++ 面试口述（预习时自己答一遍）

1. **僵尸进程是什么？如何避免？**  
   子死父不 wait，PID 表占着。父 `waitpid`；服务程序对 `SIGCHLD` 循环 `waitpid(..., WNOHANG)`。
2. **fork 之后父子谁先跑？**  
   不确定。要同步用 wait/管道/信号，不要睡一会碰运气。
3. **fork 和 exec 各干什么？**  
   fork 出新进程（拷贝）；exec 在**现有**进程里换成新程序。合在一起才是「拉起另一个工具」。
4. **进程和线程差在哪？**  
   进程隔离地址空间；线程共享。崩溃、泄漏的边界不同。

## C++ 上机（预习不用写）

见 `week3-linux/day17-fork-exec/TASK.md`：

- 父 fork，子 exec（例如 day16 的 `mycat`）
- 父 wait，打印退出码
- notes 里写清僵尸是什么，并做一次「先 Z 再 wait」对照
