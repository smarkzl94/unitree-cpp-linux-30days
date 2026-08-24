# Day25 · 日志去哪了与 Hub 滤波（详细预习）

日期：2026-09-11

预习目标：先搞清 Linux 上日志是进 `journalctl` 还是自己的文件、怎么轮转，再在控制线程加简单滤波或异常检测，并把日志做成可开关。上机先练命令/脚本，再改 Hub。

四天地图见 [day24-27-hub.md](day24-27-hub.md)。

---

# 一、Linux：`journalctl` 概念与文件日志轮转

Day20 的日志模块会把行写到 stderr 或文件。真机上还有一层：**谁负责收这些行、文件涨到多大、旧的去哪**。今天二选一做熟：理解 systemd 的 journal，或给自己的 `hub.log` 写一个轮转小脚本。

## 1. journald 是什么（概念，不一定要把 Hub 装成服务）

很多发行版里，守护进程的 stdout/stderr 被 systemd 收走，进二进制日志，用 `journalctl` 查：

```bash
journalctl -u ssh --since "1 hour ago"     # 某个服务
journalctl -f                               # 类似 tail -f
journalctl -p err -b                        # 本启动以来的 error
journalctl --disk-usage
```

认三点：

- **按服务单元 / 优先级 / 时间过滤**，比满盘 `grep` 一个巨大 `.log` 更适合长期跑的节点。
- 日志在 journal 的存储里（常是 `/var/log/journal`），不是你 `open` 的那个路径。
- 空间有上限，旧的会被丢掉——和「自己文件无限追加」不同。

今天 Hub 多半还是前台跑。你**不必**写成 `.service`。但面试要能说：「部署时 stdout 交给 journald，排障用 `journalctl -u myhub -f`；开发时自己写文件 + 轮转。」

若你愿意试（可选）：

```bash
# 仅理解：当前 shell 不是一个 unit，journalctl -f 看不到你的 printf
# 真要进 journal，需要 systemd-cat 或做成 service
echo hello | systemd-cat -t hub-demo
journalctl -t hub-demo -n 5
```

环境没有 systemd（部分 WSL）就跳过，改做文件轮转。

## 2. 文件日志轮转：为什么必须做

```bash
./hub_server 7777 2>> hub.log     # 或模块直接写 hub.log
ls -lh hub.log
```

100Hz Debug 日志，一天能到 GB。磁盘满了比程序 bug 更先让机器人停。轮转 = **按大小或按天换文件，旧的压缩/删除**。

最小脚本（上机可以写进当天目录 `rotate.sh`）：

```bash
#!/usr/bin/env bash
set -euo pipefail
LOG=${1:-hub.log}
MAX_BYTES=${2:-1048576}    # 1MB，实验用；真机常 10~50MB
if [[ ! -f "$LOG" ]]; then exit 0; fi
sz=$(wc -c < "$LOG")
if (( sz < MAX_BYTES )); then exit 0; fi
ts=$(date +%Y%m%d-%H%M%S)
mv "$LOG" "${LOG}.${ts}"
# 程序若还拿着旧 fd，它会继续写已 mv 走的文件——见下文
gzip -f "${LOG}.${ts}" || true
ls -1t "${LOG}."* 2>/dev/null | tail -n +6 | xargs -r rm -f
```

cron 或循环里每分钟跑一次即可。保留最近 N 份，防止 `/var` 被吃光。

## 3. 正在写的文件被 `mv` 走了会怎样

Linux 认的是 inode。`mv hub.log hub.log.1` 之后，进程里旧的 fd 仍指向原来那坨数据，**新的 `hub.log` 不会自动出现**。所以：

- 开发版：轮转时给进程发 SIGUSR1，handler 只设标志，主线程 `close` + 再 `open`（Day18 原则：handler 不 open）。
- 或日志模块每次 `open`/`append`/`close`（慢，今天可接受）。
- 或学 syslog：关文件再开。

预习把「mv 不等于进程换文件」写进脑子。上机最小版：停进程再轮转，或接受「下次启动才换新文件」并在笔记里写明。

`tail -f hub.log` 观察（Day13）：轮转后 `tail -F`（大写）会跟新路径，`-f` 可能停在旧 inode。

## 4. 易错点（Linux 侧）

1. **无限追加、从不看 `ls -lh`**  
   今天就看一眼，设个很小的阈值做实验。
2. **在 WSL 没有 systemd 时死磕 journalctl**  
   改文件轮转。概念会说即可。
3. **`kill -9` 之后指望最后几行一定在文件里**  
   用户态缓冲没 flush（Day16/20）。优雅退出才刷。

## Linux 口述（预习时自己答）

1. **`journalctl` 解决什么问题？**  
   集中收服务日志，按时间/优先级/单元过滤，并限制磁盘。前台作业通常仍写自己的文件。
2. **为什么日志要轮转？**  
   防止磁盘被刷满。按大小或按天切分，保留有限份。
3. **`mv` 正在写的 log，进程会怎样？**  
   继续写旧 inode。需要重开文件或发信号让程序 reopen。

## Linux 上机（预习不用敲）

二选一：`systemd-cat` + `journalctl -t` 看几行；或写 `rotate.sh` 把超 1MB 的 `hub.log` 切走。笔记写清你选哪条、进程是否 reopen。

---

# 二、C++：滤波 / 异常检测 + 日志开关

## 为什么需要

Day24 的控制线程几乎是透传。真机上这一层会做轻处理：去抖、限幅、标异常。同时日志必须能关，否则 Day26 数字全假。今天把「处理」和「可观测开关」补上，代码仍合并进 `telemetry-hub/`。

## 1. 处理要轻，不要在这里做重型算法

控制线程从 RingBuffer `pop`，对**每一帧**做 O(1) 或 O(窗口) 的小计算，再 `publish(latest_)`。

选一个做透（不要三个都做一半）：

**A. 滑动平均滤波（最稳）**

```cpp
struct Avg3 {
    float q[3]{};
    int n = 0, i = 0;
    float push(float x) {
        if (n < 3) q[n++] = x;
        else { q[i] = x; i = (i + 1) % 3; }
        float s = 0;
        for (int k = 0; k < n; ++k) s += q[k];
        return s / n;
    }
};
```

对 `ax,ay,az` 各一个。输出帧带上滤波后的值和原始值（或只发滤波后，注释写明）。

**B. 限幅 + 异常计数**

```cpp
if (std::fabs(f.ax) > kLimit) {
    ++anom;
    f.ax = std::clamp(f.ax, -kLimit, kLimit);
    LOG_WARN("clamp ax seq=%llu", (unsigned long long)f.seq);
}
```

Warn 要限流：同一类异常每秒最多一行，否则又变成每帧日志。

**C. 简单跳变检测**

相邻帧差超过阈值则标 `flag` 位，latest 里带出去。客户端打印 flag。

不管哪种：处理失败不要停产线。坏帧丢弃或标标志，`seq` 仍前进。

## 2. 日志开关：编译期 + 运行期

```cpp
// 运行期：命令行 --log=warn  或环境变量 HUB_LOG=debug
log_set_level(parse_level(arg));

// 热点里
LOG_DEBUG("seq=%llu", seq);   // 默认级别下这行函数开头就 return
```

建议：

| 级别 | 默认 | 内容 |
|---|---|---|
| Error | 开 | open/bind/协议错误 |
| Warn | 开 | 丢帧、限幅、重连 |
| Info | 开 | 启动、每秒一行：Hz、丢包累计、异常累计 |
| Debug | 关 | 每帧 seq |

**每秒一行摘要**比每帧 Info 有用：

```cpp
// control 或独立 tick
if (now - last >= 1s) {
    LOG_INFO("hz_in=%d hz_out=%d drop=%llu anom=%llu",
             frames_in, frames_out, drop, anom);
    frames_in = frames_out = 0;
    last = now;
}
```

`drop`：RingBuffer 覆盖次数 + latest 被覆盖而未发送的次数（定义写清，Day26 沿用）。

命令行示例：`./hub_server 7777 --log=info`。解析失败就维持默认，不要崩。

## 3. 和 Linux 轮转怎么配合

模块提供 `log_reopen()`：关 fd，按原路径再 `open(O_APPEND)`。主循环看到 `g_reopen` 再调。轮转脚本 `mv` 之后发 `kill -USR1 PID`（可选加分）。没有 USR1 就在笔记写「重启才换文件」。

优雅退出：`LOG_INFO("shutdown")` + flush。不要在 handler 里打。

## 4. 验收

- 开滤波或检测：client 仍更新；你能说出处理规则。
- `--log=warn` 时屏幕安静，只在异常出字；`--log=debug` 才能看到每帧。
- 每秒一行 Hz/丢包/异常。
- 第一节的轮转或 journal 演示能对上你的文件路径。

## 5. 易错点

1. **滤波窗口里分配堆、打日志**  
   控制线程被拖慢，RingBuffer 更容易满，丢包是你自己造成的。
2. **异常日志不限流**  
   传感器一直超限 → 和每帧 Debug 一样糟。
3. **开关只改宏、不能运行时关**  
   压测还要重编译。至少运行时 level。
4. **处理线程里 `send`**  
   网络仍只碰 latest。今天别把职责搅回去。

## C++ 面试口述（预习时自己答一遍）

1. **控制线程为什么只做轻处理？**  
   它决定 RingBuffer 能否跟上 100Hz。重活应另线程或降频，否则丢包和延迟一起坏。
2. **日志怎么既有用又不破坏性能？**  
   分级 + 默认关 Debug + 每秒摘要 + 异常限流；压测时 Warn。
3. **文件轮转和进程的关系？**  
   `mv` 不改已打开 fd。要 reopen 或接受重启生效。

## C++ 上机（预习不用写）

见 `week4-project/day25-hub-integrate-2/TASK.md`：

- 控制线程：滤波或异常检测
- 日志可开关，合并进 `telemetry-hub/`
- Linux：journal 概念或轮转脚本
