# Day22 · 看监听端口与 TCP Echo（详细预习）

日期：2026-09-08

预习目标：先学会用 `ss -lntp` 确认「谁在听哪个端口」，再写阻塞模型的 TCP echo server/client，本机回环跑通。上机先练命令，再写代码。

---

# 一、Linux：`ss -lntp` 看见监听

进程要对外收连接，内核里会有一个套接字处于 **LISTEN**。`ss`（socket statistics）列出它们。老命令是 `netstat`，很多发行版已经不默认装全套 `net-tools`；优先 `ss`。

## 1. 今天只要会这一条

```bash
ss -lntp
```

字母含义：

| 选项 | 意思 |
|---|---|
| `-l` | 只看 listening（在听，还没谈完的连接不列） |
| `-n` | 数字端口，不把 80 翻译成 http |
| `-t` | TCP |
| `-p` | 显示进程（PID/程序名）；可能需要权限才看得到别人的） |

UDP 用 `-u`：`ss -lnup`。今天 echo 是 TCP。

典型一行：

```text
State    Recv-Q Send-Q  Local Address:Port   Peer Address:Port  Process
LISTEN   0      128     127.0.0.1:7777       0.0.0.0:*          users:(("echo_server",pid=1234,fd=3))
```

认三件事：

1. **State = LISTEN**：在等 `accept`。
2. **Local Address:Port**：绑在哪。`127.0.0.1:7777` 只本机；`0.0.0.0:7777` 所有网卡（含外网，小心）。
3. **Process**：哪个 PID 占着。`bind` 失败报 `Address already in use` 时，来这里查谁没退干净。

## 2. 和「已建立连接」的区别

```bash
ss -tnp                 # 当前 TCP，含 ESTAB
ss -tnp | grep 7777
```

client `connect` 成功后会出现 `ESTAB`，两端各一行（本机回环会看到 127.0.0.1 对 127.0.0.1）。  
`Recv-Q`/`Send-Q` 非 0 且不变：数据堆在内核缓冲，对端不读或本端不写——Day20 排障时有用。

只听不连：`ss -lntp` 有 LISTEN，`ss -tnp` 没有 ESTAB。  
连上：两边都有。

## 3. 回环 `127.0.0.1`

本机自己连自己，不走真实网卡，防火墙干扰少。今天必须先：

```bash
./echo_server 7777          # 终端 A
ss -lntp | grep 7777        # 确认 LISTEN
./echo_client 127.0.0.1 7777  # 终端 B
```

先本机再谈跨机。跨机还要绑定 `0.0.0.0`、防火墙、对端 IP，问题面立刻变大。

`localhost` 有的系统解析成 `::1`（IPv6）。你的 socket 若只开了 IPv4，client 连 `localhost` 会失败。预习和上机都写死 `127.0.0.1`，少掉这一坑。

## 4. `bind` 失败时怎么查

```bash
./echo_server 7777
# bind: Address already in use
ss -lntp | grep 7777
kill -TERM <那个PID>        # Day18：先 TERM
# 还在再用 KILL；或换端口
```

TIME_WAIT 占用端口（你刚关掉的连接还在收尾）也会让立刻重绑失败。开发时 `setsockopt(SO_REUSEADDR)` 可缓解；先知道「不是魔法，是让处于 TIME_WAIT 的本地地址可重用」。

## 5. 易错点（Linux 侧）

1. **没装 `ss`**  
   `sudo apt install iproute2`。`netstat -lntp` 是后备。
2. **`-p` 看不到进程名**  
   权限不够，或看错列。对自己的进程一般可以。
3. **grep 端口时连 ESTAB 一起看晕**  
   先 `-l` 只看听，再去掉 `-l` 看连接。
4. **服务以为在听，`ss` 没有**  
   `bind`/`listen` 失败你没查返回值（C++ 侧必查）。

## Linux 口述（预习时自己答）

1. **`ss -lntp` 每个字母干什么？**  
   听着的、数字端口、TCP、带进程。用来确认绑定地址和谁占着端口。
2. **LISTEN 和 ESTAB 差在哪？**  
   LISTEN 等新连接；ESTAB 已经握手完，可以 `recv`/`send`。
3. **为什么今天用 `127.0.0.1` 不先用局域网 IP？**  
   回环少变量。跨机再加网卡、防火墙、绑定地址。

## Linux 上机（预习不用敲）

server 起来后 `ss -lntp` 截一行进 `linux-notes.md`；client 连上再 `ss -tnp` 看 ESTAB。故意占着端口再启一次，用 `ss` 找到旧 PID。

---

# 二、C++：阻塞 TCP Echo

## 为什么需要

机器人和上位机/云边常用 TCP：可靠、有序、有连接。先搞通**阻塞**模型的 server/client：一次只服务一个连接（或 accept 后 fork/thread，今天最小版可以单连接循环）。粘包留到 Day23；今天把字节流跑通。

上机任务：本机回环 echo——client 发什么，server 原样发回。

## 1. 套接字步骤（把顺序背熟）

Server：

```text
socket() → bind() → listen() → accept() → recv/send 循环 → close
```

Client：

```text
socket() → connect() → send/recv 循环 → close
```

```cpp
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <unistd.h>

int fd = socket(AF_INET, SOCK_STREAM, 0);
if (fd < 0) { perror("socket"); return 1; }

sockaddr_in addr{};
addr.sin_family = AF_INET;
addr.sin_port = htons(7777);                 // 主机序 → 网络序
inet_pton(AF_INET, "127.0.0.1", &addr.sin_addr);

// server:
bind(fd, reinterpret_cast<sockaddr*>(&addr), sizeof(addr));
listen(fd, 16);                              // backlog
int cfd = accept(fd, nullptr, nullptr);      // 阻塞，直到有人 connect

// client:
connect(fd, reinterpret_cast<sockaddr*>(&addr), sizeof(addr));
```

- `AF_INET` + `SOCK_STREAM` = IPv4 TCP。
- `htons` / `htonl`：端口和 IPv4 地址在网上用大端。本机 x86 是小端，忘了转换会绑到「奇怪端口」。
- `listen` 的第二个参数是排队长度，不是「最多服务多少客户」。
- `accept` 返回**新** fd，代表这条连接。原来的 listening fd 继续听。echo 最小版：处理完 `cfd` 再 `accept` 下一个。
- 每个返回值都要查。`bind` 失败看 `ss`。

## 2. TCP 是字节流，没有「消息」

以太网/IP/TCP 保证的是：**字节按序到达，不丢（除非连接断）**。不保证：

- 一次 `send(100)` 对端一次 `recv` 就拿到 100。
- 一次 `recv` 刚好是你应用眼里的「一条」。
- 两次 `send` 不会被黏在一次 `recv` 里。

所以 echo 今天的正确写法是：**读到多少就写回多少**，不要假设「一行」或「一个结构体」。

```cpp
char buf[4096];
for (;;) {
    ssize_t n = recv(cfd, buf, sizeof(buf), 0);
    if (n < 0) {
        if (errno == EINTR) continue;
        perror("recv");
        break;
    }
    if (n == 0) break;          // 对端 close，读到 EOF
    // 短写循环把 buf[0,n) 全部 send 出去（同 Day16 write）
}
close(cfd);
```

`recv == 0`：对端关了写端。不是失败。你也 `close`，连接结束。

`send` 和 `write` 一样可能短写，循环直到 `n` 字节发完。

## 3. Echo 的验收

```bash
# 终端 A
./echo_server 7777
# 终端 B
./echo_client 127.0.0.1 7777
# 输入 hello → 应原样回来
# 也可以:  printf 'hello' | nc 127.0.0.1 7777
```

- 多发几段，确认都回来。
- client Ctrl+C（Day18）：server 那一端 `recv==0` 或 `EINTR`，不要崩，回到 `accept` 或退出。
- `ss -lntp` 在 A 启动后必须能看到端口。

最小 client 也可以是：`send` 一句固定话，`recv` 打出来，对上就算通。

## 4. TCP vs UDP（面试必问，今天先记结论）

| | TCP | UDP |
|---|---|---|
| 连接 | 有，先握手 | 无 |
| 可靠有序 | 是 | 否，可丢可乱 |
| 消息边界 | **无**（字节流） | 有数据报边界 |
| 典型用途 | 指令、遥测要可靠 | 视频、状态允许丢 |

机器人状态上报常用 TCP：丢一条「当前姿态」可能还能靠下一帧补（Hub 只要最新），但控制指令丢了或乱序更麻烦；而且调试时「一定能连上、一定能回显」更简单。UDP 以后再说。

粘包：正因为无边界，Day23 要自己定「长度头 + payload」。

## 5. 易错点（看懂再上机）

1. **当「每次 recv 就是一条完整消息」**  
   今天 echo 按字节回显可以；明天传结构体就会错。心理上现在就要接受字节流。
2. **忘记处理部分 send**  
   内核缓冲满时 `send` 返回正数但小于请求。
3. **`bind` 失败不看 errno、不跑 `ss`**  
   端口占用是最常见原因。
4. **忘记 `htons`**  
   端口错乱。
5. **listening fd 和 conn fd 搞混**  
   `recv` 必须用 `accept` 返回的 fd。
6. **`localhost` vs IPv6**  
   写 `127.0.0.1`。

## C++ 面试口述（预习时自己答一遍）

1. **TCP 和 UDP 区别？机器人状态上报为何常用 TCP？**  
   TCP 有连接、可靠有序、无消息边界；UDP 相反。状态/指令要可靠、调试要简单，先 TCP。允许丢的高频视频才更常 UDP。
2. **什么是粘包？**  
   字节流没有消息边界，一次 recv 可能多条或半条。要自己定帧（Day23）。
3. **`recv` 返回 0 是什么意思？**  
   对端关闭了这一方向。不是错误码。

## C++ 上机（预习不用写）

见 `week4-project/day22-tcp-echo/TASK.md`：

- 阻塞 echo server + client
- 本机 `127.0.0.1` 跑通
- 用 `ss -lntp` 确认监听
