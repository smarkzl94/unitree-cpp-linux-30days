# Day16 · strace 看系统调用与 POSIX 文件 I/O（详细预习）

日期：2026-09-02

预习目标：先学用 `strace -e open,read,write` 盯住自己的拷贝程序，再学 POSIX `open/read/write/close` 写 `mycat`/`mycp`。上机先练命令，再写代码。

---

# 一、Linux：用 strace 看见「程序在跟内核说什么」

你在 C++ 里写 `read(fd, buf, n)`，真正干活的是内核：从文件取出字节，拷进你的用户态缓冲。用户态 ↔ 内核的这一跳叫**系统调用**（syscall）。

`strace` 把每次系统调用的名字、参数、返回值打出来。今天只盯拷文件最相关的三个：`open`、`read`、`write`。

## 1. 最小用法

先有一个会读写文件的程序（上机是你的 `mycp`；预习可以先想 `cp`）：

```bash
strace -e open,read,write ./mycp src.bin dst.bin
```

`-e` 是过滤器：只显示这些调用。不加 `-e` 会刷屏（`mmap`、`brk`、`close`、动态库一堆）。

常见还会用：

```bash
strace -e open,read,write,close,openat ./mycp a b
strace -c ./mycp a b          # 汇总：每种调用多少次、花多少时间
strace -p PID                 # 贴到已经在跑的进程（Day20 排障用）
```

现代 glibc 打开文件常常走 `openat` 而不是 `open`。若 `-e open` 什么都没有，加上 `openat`。

## 2. 怎么读一行 strace

典型输出（示意）：

```text
openat(AT_FDCWD, "src.bin", O_RDONLY) = 3
openat(AT_FDCWD, "dst.bin", O_WRONLY|O_CREAT|O_TRUNC, 0666) = 4
read(3, "...."..., 65536) = 65536
write(4, "...."..., 65536) = 65536
read(3, "...."..., 65536) = 1234
write(4, "...."..., 1234) = 1234
read(3, "", 65536) = 0
```

怎么读：

- `= 3`、`= 4`：返回**文件描述符**（fd）。0/1/2 是 stdin/stdout/stderr，所以你的文件从 3 开始很正常。
- `read` 的第三个参数是你请求的长度；等号右边是**实际读到的字节**。
- `read = 0`：到文件末尾（EOF）。`mycat`/`mycp` 就该停。
- `write` 的返回值也是实际写出的字节——**可以小于请求**，这就是短写。

失败会长这样：

```text
openat(AT_FDCWD, "nope", O_RDONLY) = -1 ENOENT (No such file or directory)
```

返回 `-1`，后面是 `errno` 的名字。C++ 里就是 `if (fd < 0) perror(...)`。

## 3. 你要从 strace 里看出的三件事

1. **打开了谁、模式对不对**  
   源文件必须 `O_RDONLY`；目标必须带 `O_CREAT`，通常还要 `O_TRUNC`（否则旧文件尾巴可能残留）。
2. **每次读写多大**  
   若每次 `read(fd, ..., 1)`，拷 64MB 就是几千万次系统调用，会极慢。缓冲常用 4K～64K。
3. **有没有短读短写**  
   最后一次 `read` 几乎一定小于缓冲；管道上 `write` 也可能一次写不完。代码必须按返回值循环。

对照：`cp` 或你的 `mycp` 拷同一个大文件，`strace -c` 看 `read`/`write` 次数。缓冲从 1 改成 64K，次数应掉几个数量级。

## 4. 和 gdb 的分工（先建立，Day20 再系统化）

| 工具 | 看见什么 |
|---|---|
| `strace` | 和内核的对话：卡在哪个 syscall、参数、errno |
| `gdb` | 用户态逻辑：变量、调用栈、哪一行 C++ |

文件拷错、权限失败、卡在读设备——先 `strace`。算错长度、缓冲逻辑错——再 gdb。

## 5. 易错点（Linux 侧）

1. **strace 本身有开销**  
   每个 syscall 都要被跟踪，测「极限吞吐」时别开着 strace。今天是为了看懂，不是为了跑分。
2. **过滤写太窄**  
   只过滤 `open` 会漏掉 `openat`。先 `-e open,openat,read,write,close`。
3. **把动态库的 open 当成你的业务**  
   启动时会 `open` `libc.so`、`/etc/ld.so.cache`。认路径：只有你传入的文件名才是 `mycp` 的业务。

## Linux 口述（预习时自己答）

1. **`strace -e open,read,write` 能帮你看到什么？**  
   程序打开了哪些路径、每次请求读写多少、内核实际返回多少、失败时的 errno。
2. **`read` 返回 0 和返回 -1 差在哪？**  
   0 是 EOF，正常结束；-1 是出错，看 errno（如 `EINTR`、`EIO`）。
3. **为什么拷大文件不能每次读写 1 字节？**  
   每次都是一次系统调用（用户态/内核切换）。次数爆炸，CPU 浪费在进进出出，不是在搬数据。

## Linux 上机（预习不用敲）

对你的 `mycp`（没有就先对 `cp`）跑 `strace -e open,openat,read,write`。记下：源/目标 fd、缓冲大小、最后一次短读。再 `strace -c` 看次数。写入当天 `linux-notes.md`。

---

# 二、C++：POSIX `mycat` / `mycp`

## 为什么需要

日志、标定、录包都要可靠读写。C++ 的 `fstream` 底下最终还是这些系统调用；今天自己写一遍，才能理解短读、返回值、缓冲。机器人岗常问「拷文件怎么又对又快」。

上机任务：`mycat <file>` 把文件打到标准输出；`mycp <src> <dst>` 正确拷大文件。优先 WSL 上的 POSIX。

## 1. 四个系统调用

```cpp
#include <fcntl.h>      // open, O_RDONLY, O_CREAT ...
#include <unistd.h>     // read, write, close
#include <cerrno>
#include <cstring>
#include <cstdio>       // perror

int fd = open(path, O_RDONLY);
if (fd < 0) {
    perror("open");     // 打印 errno 对应的话
    return 1;
}
// ... read/write ...
close(fd);
```

| 调用 | 作用 | 成功 | 失败 |
|---|---|---|---|
| `open` | 打开/创建，返回 fd | fd ≥ 0 | -1 |
| `read(fd, buf, n)` | 最多读 n 字节 | 实际字节；0 = EOF | -1 |
| `write(fd, buf, n)` | 最多写 n 字节 | 实际字节 | -1 |
| `close(fd)` | 还回 fd | 0 | -1 |

fd 是进程里的整数门票。关掉再对这个数字 `read` 是未定义的（可能复用到别的文件）。

常见 `open` 标志：

```cpp
open(src, O_RDONLY);
open(dst, O_WRONLY | O_CREAT | O_TRUNC, 0644);
```

- `O_CREAT`：不存在就建；必须给第三个参数（权限，如 `0644`）。
- `O_TRUNC`：存在就清空。没有它，源文件更短时，目标会留下旧尾巴——拷「错」却不一定报错。
- `0644`：主人读写，其他人读。具体以 umask 再裁。

## 2. 短读短写：必须按返回值循环

内核**不保证**一次 `read`/`write` 完成你要的长度。管道、信号、磁盘、网络都可能少给。

错误写法：

```cpp
char buf[4096];
read(fd, buf, sizeof(buf));           // 不管返回值
write(1, buf, sizeof(buf));           // 可能写了未初始化的尾巴
```

正确骨架（`mycat`：读文件写到 stdout=1）：

```cpp
char buf[64 * 1024];
for (;;) {
    ssize_t n = read(in, buf, sizeof(buf));
    if (n < 0) {
        if (errno == EINTR) continue; // 被信号打断，重试
        perror("read");
        break;
    }
    if (n == 0) break;                // EOF

    ssize_t off = 0;
    while (off < n) {
        ssize_t w = write(out, buf + off, n - off);
        if (w < 0) {
            if (errno == EINTR) continue;
            perror("write");
            return;
        }
        off += w;                     // 短写：接着写剩下的
    }
}
```

`mycp` 只是 `in`/`out` 都是你 `open` 出来的 fd，循环一样。

`ssize_t` 是带符号的尺寸类型：成功 ≥ 0，失败 -1。不要用 `int` 去接再和 `sizeof` 混比而不想清楚。

## 3. 缓冲 I/O vs 系统调用

| | `read`/`write` | `fread`/`fwrite` / `fstream` |
|---|---|---|
| 每次调用 | 进内核 | 先填用户态缓冲，满了（或 flush）才进内核 |
| 崩溃时 | 已 write 的在；没 write 的无 | 缓冲里可能丢 |
| 小次数大量数据 | 自己选 64K 就很好 | 标准库已经在帮你攒 |

`mycat`/`mycp` 今天走系统调用，是为了看见短读和 fd。以后写日志可以用 `fstream`，但要知道：`endl` 会 flush；崩溃前没 flush 的行会丢。Day20 的日志模块会回到这点。

## 4. `mycat` 与 `mycp` 的行为要对齐直觉

```bash
./mycat notes.md              # 等价于 cat：内容去 stdout
./mycp big.bin /tmp/big.bin
cmp big.bin /tmp/big.bin      # 一个字节都不能差
```

- 参数不够：打印用法，返回非 0。
- 源打不开、目标建不了：`perror`，非 0。
- 不要把目标打开成 `O_RDONLY`。
- 大文件（几十 MB）也要正确——所以必须循环，不能 `new` 整个文件进内存（能做，但不是今天要练的，也撑不住更大文件）。

## 5. 易错点（看懂再上机）

1. **忽略短读**  
   最后一块、管道、网络几乎必短。按 `n` 写，不要按 `sizeof(buf)` 写。
2. **不检查返回值**  
   磁盘满时 `write` 失败；继续装没发生，目标是半截文件。
3. **`EINTR`**  
   信号到来时，未完成的 `read`/`write` 可能返回 -1/`EINTR`。循环里重试。今天了解并写上，Day18 信号会真正碰到。
4. **当字符串处理二进制**  
   中间可以有 `'\0'`。用长度，不要 `strlen`。
5. **忘关 fd**  
   短程序能跑，长跑会耗尽 fd（`ulimit -n`）。RAII 包一个 `unique_fd` 更好；今天至少所有路径 `close`。

## C++ 面试口述（预习时自己答一遍）

1. **为什么 `write` 一次写不完？**  
   POSIX 允许短写：管道缓冲满、信号、设备只接受一部分。必须用返回值推进偏移，循环直到请求写完或出错。
2. **拷文件如何又对又快？**  
   对：检查每个返回值，处理短读写、`EINTR`、EOF；目标用 `O_TRUNC`。快：用几 K 到几十 K 的缓冲循环，减少 syscall 次数；不要逐字节。还要快可以谈 `sendfile`/`copy_file_range`（知道即可）。
3. **`read` 返回 0 是不是失败？**  
   不是。0 是 EOF。失败是 -1。

## C++ 上机（预习不用写）

见 `week3-linux/day16-mycat-mycp/TASK.md`：

- `mycat <file>`
- `mycp <src> <dst>`，大文件 `cmp` 通过
- 用第一节的 `strace` 对照你的缓冲大小和短读
