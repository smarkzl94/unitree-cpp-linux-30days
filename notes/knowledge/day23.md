# Day23 · 本机抓包与定长头粘包（详细预习）

日期：2026-09-09

预习目标：先（可选）用 `tcpdump` 在回环上看自己的 TCP 字节，再学「定长长度头 + payload」把字节流还原成一帧帧。上机命令可选，C++ 必做。

---

# 一、Linux：`tcpdump -i lo`（可选）

Day22 的 `ss` 告诉你「有没有在听、有没有连上」。`tcpdump` 告诉你「线上实际飞过哪些字节」。今天可选：能抓到自己的 echo/定长头包更好，抓不到别卡死——重点在 C++ 拆包。

需要权限（常见 `sudo`），WSL 有的版本回环抓包受限。失败就记一笔「环境不支持」，改用 `strace -e sendto,recvfrom` 看用户态进出。

## 1. 最小命令

```bash
sudo tcpdump -i lo port 7777 -nn
sudo tcpdump -i lo port 7777 -nn -X          # 十六进制 + ASCII
sudo tcpdump -i lo port 7777 -nn -c 20       # 抓 20 个包就停
```

- `-i lo`：回环网卡。本机 `127.0.0.1` 对 `127.0.0.1` 走这里，不是 `eth0`。
- `port 7777`：过滤，否则桌面环境会刷一屏别的。
- `-nn`：IP、端口都保持数字，不反查域名。
- `-X`：看到 payload。定长头协议时，你应能认出前 4 字节长度。

先开 tcpdump，再跑 client。顺序反了，可能错过握手。

## 2. 你能认出什么

三次握手：`S`（SYN）、`S.`（SYN-ACK）、`.`（ACK）。然后才是带数据的包（`P` 或长度非 0）。

`-X` 里若你发的是「4 字节长度 + Hello」：

```text
0x0000:  0000 0005 4865 6c6c 6f         .....Hello
```

`0000 0005` 是大端的 5——这就是 `htonl(5)`。若你写成主机序小端，这里会是 `0500 0000`，对端 `ntohl` 会得到天文数字。抓包就是为了**看见字节序**，不是为了背 TCP 标志。

抓不到时的后备：

```bash
strace -e trace=network -s 200 -p PID
```

`send`/`recv` 的缓冲内容也能印证长度头。

## 3. 易错点（Linux 侧）

1. **`-i eth0` 抓回环**  
   本机对本机要 `-i lo`。
2. **没 sudo 就说 tcpdump 坏了**  
   权限问题。不要为可选工具浪费一小时。
3. **过滤写成 `host 7777`**  
   端口用 `port 7777`。

## Linux 口述（预习时自己答）

1. **`ss` 和 `tcpdump` 差在哪？**  
   `ss` 看套接字状态和谁占用端口；`tcpdump` 看线上字节。排障先 `ss`，怀疑协议/字节序再抓包。
2. **为什么回环要 `-i lo`？**  
   127.0.0.1 的流量走回环接口，不出现在物理网卡上。
3. **可选的含义？**  
   环境不支持就跳过，用 strace 看 send/recv；C++ 拆包仍必须做。

## Linux 上机（预习不用敲）

有权限就 `tcpdump -i lo port … -nn -c 20` 抓一次，笔记里贴一行长度头。没有就写明原因，改 strace。

---

# 二、C++：定长头协议与粘包

## 为什么需要

TCP 是字节流（Day22）：一次 `recv` 可能是半帧、一帧、一帧半、三帧黏在一起。应用必须自己定「什么叫一条消息」。机器人遥测、上位机指令都是同一问题。

上机任务：帧格式 `uint32 length` + payload；客户端约 50Hz 发结构体；服务端正确拆包。

## 1. 粘包 / 拆包分别是什么

```text
发送端两次 send：[AAAA] [BBBB]
接收端可能 recv 到：
  [AAAA] [BBBB]          碰巧对齐
  [AA] 然后 [AABB] [BB]  拆开了（拆包）
  [AAAABBBB]             黏在一起（粘包）
  [AAAABB] 然后 [BB]     又黏又拆
```

根因：TCP 不保留 `send` 的边界；Nagle、窗口、调度都会合并或切开。**这是正常行为，不是 bug。** 当 bug 的是：你假设对齐。

字符串协议用 `\n` 当边界也行，但二进制中间可以有 `0x0a`，不能靠字符扫。二进制用**长度头**最干净。

## 2. 定长头：先收齐 4 字节，再收齐 body

约定（今天用这个，写进代码注释）：

```text
| uint32_t length (网络字节序) | payload（length 字节） |
length = payload 的字节数，不含这 4 字节自己
```

```cpp
std::uint32_t nlen = 0;
if (!recv_full(fd, &nlen, 4)) return false;
std::uint32_t len = ntohl(nlen);
if (len == 0 || len > kMaxPayload) return false;   // 上限！
std::vector<char> body(len);
if (!recv_full(fd, body.data(), len)) return false;
```

`recv_full` 就是 Day16 的短读循环，读满 `n` 字节才返回：

```cpp
bool recv_full(int fd, void* dst, size_t n) {
    auto* p = static_cast<char*>(dst);
    size_t got = 0;
    while (got < n) {
        ssize_t r = recv(fd, p + got, n - got, 0);
        if (r < 0) {
            if (errno == EINTR) continue;
            return false;
        }
        if (r == 0) return false;   // 对端关了，帧不完整
        got += static_cast<size_t>(r);
    }
    return true;
}
```

发送对称：`uint32_t nlen = htonl(len); send_full(fd, &nlen, 4); send_full(fd, payload, len);`

**为什么 length 必须有上限？** 对端（或损坏数据）说 `length = 4e9`，你 `vector(4e9)` 会内存爆炸或抛异常。上限按业务：今天一个结构体，4KB 足够；Hub 一帧传感器也通常远小于 1MB。超了就关连接，不要信。

## 3. 连接上挂一个 `recv_buffer`（状态机）

`recv_full` 阻塞到齐，适合今天的单线程阻塞 server。更接近真机的是：**每次 `recv` 多少吃多少，拼进缓冲，够一帧就切走。**

```text
状态 A：缓冲里不足 4 字节 → 继续收
状态 B：已有长度，但 body 不够 → 继续收
状态 C：够一帧 → 交给业务，从缓冲头删掉这帧，回到 A（可能一次 recv 里还有下一帧）
```

```cpp
class Framer {
    std::string buf_;          // 或 vector<uint8_t>
    static constexpr uint32_t kMax = 4096;
public:
    // 把这次 recv 到的数据追加进来；完整帧通过 cb 吐出
    bool feed(const char* data, size_t n,
              const std::function<void(const char*, uint32_t)>& cb) {
        buf_.append(data, n);
        for (;;) {
            if (buf_.size() < 4) return true;
            uint32_t len;
            memcpy(&len, buf_.data(), 4);
            len = ntohl(len);
            if (len == 0 || len > kMax) return false;
            if (buf_.size() < 4 + len) return true;
            cb(buf_.data() + 4, len);
            buf_.erase(0, 4 + len);
        }
    }
};
```

要点：

- **一次 `feed` 里用 for 循环**，把黏在一起的多帧全拆完。
- 半帧留在 `buf_` 里等下次。
- 长度非法：拆连接。不要试图「跳过找下一帧」（二进制没有同步字的话，跳了更乱；有 magic 是进阶）。

今天可以先 `recv_full` 阻塞版，理解后再改 Framer。Hub（Day24）建议 Framer，因为后面要和「只要最新」一起跑。

## 4. 客户端 50Hz 发结构体

```cpp
struct Telemetry {
    std::uint64_t seq;
    std::uint64_t stamp_ns;
    float x, y, z;
};
// 发送：length = sizeof(Telemetry)，payload 就是这坨字节
```

50Hz：每 20ms 一帧。用单调时钟睡到下一拍（Day12），不要 `sleep(20ms)` 不管已经花掉的时间——会漂移。

服务端拆出 payload 后 `memcpy` 到 `Telemetry`（同机同编译器，同 Day19 的风险说明）。校验 `seq` 连续或允许跳（UDP 才常跳；TCP 不应丢，跳了说明你拆包错或发送端故意）。

验收：故意让发送端**一次 send 两帧**（先拼到一个缓冲再 send），服务端仍应回调两次。这是粘包测试。再故意 `send` 只发 2 字节头，睡一会再发剩下的——拆包测试。

## 5. 字节序

```cpp
uint32_t host = 5;
uint32_t net  = htonl(host);   // 线上永远大端
uint32_t back = ntohl(net);    // 回来
```

x86 本机对本机，你若两端都不转换，碰巧能通，跨一台大端机器或以后用 Wireshark 看会懵。**长度头必须 htonl/ntohl。** payload 里的 `uint64`/`float` 今天可先本机原生；面试要说「跨机还要约定」。

## 6. 易错点（看懂再上机）

1. **当字符串用 `'\0'` 判断二进制结束**  
   结构体里到处是 0。用 length。
2. **长度字段不校验上限**  
   内存炸弹，也是简单的协议漏洞。
3. **`recv_full` 用 `if` 只读一次**  
   半帧必现。
4. **`length` 含不含头**  
   两边必须同一约定。推荐不含。写进注释。
5. **`erase` 头部在超长 `string` 上很贵**  
   今天帧小无所谓；以后用偏移 + 定期 compact。
6. **50Hz 用墙钟 sleep 不补偿**  
   频率会偏。用单调时钟对齐下一拍。

## C++ 面试口述（预习时自己答一遍）

1. **如何设计简单可靠的应用层帧？**  
   定长头（网络序 length）+ payload；收满头再收满 body；非法长度拆连接。需要可扩展再加 magic、version。
2. **长度字段为什么要有上限？**  
   防止对端谎报超大 length 导致分配爆炸。上限按业务最大值加余量。
3. **粘包是 TCP 的 bug 吗？**  
   不是。字节流无边界。应用层自己定帧。

## C++ 上机（预习不用写）

见 `week4-project/day23-framed-protocol/TASK.md`：

- 帧格式 `uint32 length` + payload
- 客户端 50Hz 发送结构体
- 服务端处理粘包/拆包；最好加「一次发两帧」和「切开长度头」的自测
