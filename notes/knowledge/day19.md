# Day19 · 共享内存观察与 IPC 传传感器帧（详细预习）

日期：2026-09-05（建议 WSL）

预习目标：先学会用 `ipcs` 和 `ls /dev/shm` 看见本机的 IPC 对象，再学用管道或共享内存传一帧传感器数据。上机先练命令，再写一种做透。

---

# 一、Linux：`ipcs` 与 `/dev/shm`

进程地址空间隔离（Day17）。要让两个进程看见同一份数据，必须走**进程间通信**（IPC）。Linux 上你能「用命令看见」的，主要是 System V 的消息队列/信号量/共享内存，以及 POSIX 共享内存（通常挂在 `/dev/shm`）。

今天命令的任务不是背全手册，而是：**创建/使用之后，能在系统里把对象找出来；用完知道去哪确认它还在不在。**

## 1. `ipcs`：列出 System V IPC

```bash
ipcs              # 三张表：消息队列、共享内存、信号量
ipcs -m           # 只看共享内存
ipcs -s           # 信号量
ipcs -q           # 消息队列
ipcs -m -p        # 共享内存谁创建、谁最后碰
```

共享内存表里认这几列：

- **key / shmid**：标识。程序里 `shmget` 用 key，内核回一个 id。
- **owner**：谁建的。
- **bytes / nattch**：多大、现在几个进程 attach 着。
- **status**：有的实现会标 dest（已标记删除，等最后一人 detach）。

若你的 C++ 走 POSIX `shm_open` 而不是 `shmget`，`ipcs -m` **可能是空的**。这不代表没共享内存，只是对象不在 System V 那张表里。下一步看 `/dev/shm`。

## 2. `/dev/shm`：POSIX 共享内存的常见落地

```bash
ls -l /dev/shm
df -h /dev/shm          # 其实是 tmpfs，掉电/重启就没
```

`shm_open("/sensor_frames", ...)` 在很多发行版上就是 `/dev/shm/sensor_frames`（名字前面的 `/` 会被规范掉）。

```bash
ls -l /dev/shm/sensor_frames
stat /dev/shm/sensor_frames
```

权限、大小、属主都能看。程序崩溃却没 `shm_unlink`，文件会留在这——下次 `shm_open` 可能接到一份**旧数据**。调试时先 `ls /dev/shm`，该删的删。

```bash
# 确认是你的实验对象再删，不要清别人的
rm /dev/shm/sensor_frames
```

对照：管道（匿名 `pipe`）在 `/dev/shm` 和 `ipcs` 里都**看不见**一张「管道表」。匿名管道随进程，两端 fd 关完就没了。所以：

| 你用的 IPC | 用什么命令看见 |
|---|---|
| System V shm | `ipcs -m` |
| POSIX shm | `ls -l /dev/shm` |
| 匿名 pipe | `ls -l /proc/PID/fd` 看到 `pipe:[inode]` |
| 命名管道 FIFO | `ls -l` 那个路径，类型是 `p` |

## 3. 从 `/proc` 看「这个进程打开了啥」

```bash
ls -l /proc/$PID/fd
# 3 -> /dev/shm/sensor_frames
# 4 -> pipe:[12345]
```

一边跑你的收发程序，一边对 PID 列 fd。这比只背 `ipcs` 更贴「我的进程」。

## 4. 易错点（Linux 侧）

1. **`ipcs` 是空的就以为 shm 没建起来**  
   先确认你用的是 SysV 还是 POSIX，再选命令。
2. **实验残留**  
   `/dev/shm` 里同名对象会坑下一次运行。写进笔记：启动前 `ls`，结束 `unlink`。
3. **`/dev/shm` 空间用满**  
   它是内存盘，默认常是物理内存的一半量级。巨帧不卸会把机器拖慢。今天帧很小，知道即可。

## Linux 口述（预习时自己答）

1. **`ipcs` 和 `ls /dev/shm` 各看什么？**  
   `ipcs` 看 System V IPC；`/dev/shm` 看 POSIX 共享内存（tmpfs 文件）。匿名管道两边都看不到「名字」，要看 `/proc/PID/fd`。
2. **为什么跨机器不能用 shm？**  
   共享内存是同一台机器的物理页。另一台机器没有这些页，必须走网络（Day22）。
3. **程序崩了，共享内存还在吗？**  
   POSIX 名对象常还在 `/dev/shm`，直到 `shm_unlink` 或重启。所以要自己收尾。

## Linux 上机（预习不用敲）

跑你的 IPC demo，按你选的实现对照：`ipcs -m` 和/或 `ls -l /dev/shm`，再 `ls -l /proc/PID/fd`。用完确认对象消失或你主动删掉。写入 `linux-notes.md`。

---

# 二、C++：管道或共享内存传传感器帧

## 为什么需要

多进程架构（感知进程 / 控制进程）需要传数据。同机高频大块用共享内存；简单父子用管道。今天**二选一做透**：把一个「传感器帧」结构体送过去，接收端校验字段完整。不要两种都写一半。

## 1. 先定一帧长什么样

```cpp
#include <cstdint>

struct SensorFrame {
    std::uint64_t seq;
    std::uint64_t stamp_ns;
    float ax, ay, az;
    std::uint32_t crc;      // 简单校验：比如对前面字节做异或或求和
};
```

同机、同一编译器、同一 `#pragma`/对齐设置，可以直接当字节拷。面试必须能说出风险：

- **对齐/填充**：编译器可能在字段间插 padding，另一边若布局不同就错位。
- **字节序**：跨机器必须 `htole`/`htobe`。今天本机可以先不管，但要会说。
- **当字符串**：中间有 `0` 字节，`strlen` 会截断。必须按 `sizeof` 或自带长度传。

校验：接收端重算 `crc`，对不上就报错，不要默默用坏帧。

## 2. 管道：血缘进程、字节流、简单

```cpp
int fds[2];
if (pipe(fds) < 0) { perror("pipe"); return 1; }
// fds[0] 读端，fds[1] 写端

pid_t pid = fork();
if (pid == 0) {
    close(fds[1]);                    // 子只读
    SensorFrame f{};
    // 必须循环读满 sizeof(f)，短读（Day16）
    close(fds[0]);
    _exit(0);
}
close(fds[0]);                        // 父只写
// write 循环写满
close(fds[1]);                        // 写端关完，读端 read 到 0
waitpid(pid, nullptr, 0);
```

要点：

- **匿名 pipe 只能亲缘进程**（fork 出来的）。无关进程要用 FIFO（`mkfifo`）或网络。
- 管道是**字节流**：一次 `write` 一个结构体，对端可能分两次 `read` 到。必须「读满 N 字节」循环（和 Day23 粘包是同一类问题）。
- 所有写端都 `close` 后，读返回 0。读端先退，再写会 `SIGPIPE`/`EPIPE`。要处理：忽略 SIGPIPE 并检查 `write` 返回值，或保证退出顺序。
- 管道有容量（常 64KB 量级）。写满且没人读，`write` 阻塞。

适合：父子、帧不大、实现要短。不适合：无关进程、超大帧还想零拷贝。

## 3. 共享内存：最快，必须自己同步

POSIX 骨架：

```cpp
#include <fcntl.h>
#include <sys/mman.h>
#include <sys/stat.h>
#include <unistd.h>

int fd = shm_open("/sensor_frames", O_CREAT | O_RDWR, 0600);
ftruncate(fd, sizeof(SharedSlot));
void* p = mmap(nullptr, sizeof(SharedSlot),
               PROT_READ | PROT_WRITE, MAP_SHARED, fd, 0);
```

`MAP_SHARED`：你的写入对其他 attach 的进程可见。`MAP_PRIVATE` 是写时复制，对方看不见你的改，今天不要用错。

**没有同步的 shm 会撕裂**：一边写 `seq` 和 `ax` 写到一半，另一边读到新 seq + 旧 ax。必须二选一（或组合）：

- 双缓冲 + 原子序号：写完再 `seq.fetch_add(1)`，读到连续两次相同 seq 才信。
- 信号量 / 进程共享 mutex（`PTHREAD_PROCESS_SHARED`）。
- 用管道只传「有新帧」的通知，数据在 shm。

最小可用：一个 slot + `atomic<uint64_t> seq`，写端：填 payload → 写 seq；读端：读 seq → 拷 payload → 再读 seq，两次相等才用。

用完：

```cpp
munmap(p, sizeof(SharedSlot));
close(fd);
shm_unlink("/sensor_frames");   // 谁创建谁负责卸名字；过早 unlink 不影响已 mmap 的人
```

## 4. 怎么选（面试就按这个说）

```text
同机、父子、实现要简单、帧小     → pipe
同机、高频、大块、要避免拷贝     → shm + 同步
跨机器 / 跨主机                 → TCP（Day22），不能 shm
只要最新、允许丢中间帧           → shm 覆盖写（Hub 也是这思想）
```

今天选一种，把「发一帧、收一帧、校验过」跑通，再用第一节命令把对象看见。

## 5. 易错点（看懂再上机）

1. **当字符串处理二进制中间的 `\0`**  
   用长度循环，或 `sizeof(SensorFrame)`。
2. **共享内存无同步**  
   偶发错字段，难复现。必须有 seq 或锁。
3. **一端退出另一端不处理 `EPIPE`**  
   写管道被 SIGPIPE 打死。`signal(SIGPIPE, SIG_IGN)` 然后检查 `write == -1 && errno == EPIPE`。
4. **管道当「一次读写 = 一帧」**  
   短读。写 `read_full(fd, &f, sizeof f)`。
5. **两种都写一半**  
   任务是一种做透。

## C++ 面试口述（预习时自己答一遍）

1. **管道和共享内存怎么选？**  
   父子、小数据、要背压阻塞 → 管道。同机高频大块、只要最新 → shm，自己做同步。跨机必须网络。
2. **为什么跨机器要用网络协议而不是 shm？**  
   shm 共享的是本机物理内存。另一台 CPU 看不见这些页。
3. **传结构体有什么风险？**  
   对齐填充、字节序、版本字段变化。同编译器本机可先直接拷，生产要约定序列化。

## C++ 上机（预习不用写）

见 `week3-linux/day19-ipc/TASK.md`：

- 管道或 shm 二选一做透
- 传传感器帧，接收端校验字段完整
- 用 `ipcs` 或 `/dev/shm` 对照
