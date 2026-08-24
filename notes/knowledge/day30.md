# Day30 · Hub 演示故事与现场 Linux 排障（详细预习）

日期：2026-09-16

预习目标：先把「演示时随手用的 Linux 排障」练成肌肉记忆，再把 10 分钟 Hub 演示和一页纸项目故事写熟。上机是彩排，不是加功能。

---

# 一、Linux：演示时现场排障

面试官（或你自己扮的）会说：「连不上」「卡住了」「CPU 好高」。今天 Linux 的产出是：**在 Hub 还在跑的时候，60 秒内用命令定位，而不是立刻改代码。** Day20 清单的实战版。

## 1. 开演前 2 分钟检查（避免演示自己翻车）

```bash
ss -lntp | grep 7777 || true          # 先清残留
./run.sh 7777                         # 或你的 Release 路径
ss -lntp | grep 7777                  # 必须有 LISTEN，记下 PID
ps -o pid,stat,nlwp,rss,cmd -p PID
```

第二终端 client。第三终端留着敲命令——**不要在跑 server 的那个窗口清屏乱翻**。

预想好窗口布局：A 编译/server 日志（每秒摘要），B client seq，C 命令。

## 2. 现场 60 秒脚本（背这张）

```text
连不上
  ss -lntp | grep 端口     → 没有 LISTEN：没起来 / bind 失败 / 已经退了
  有 LISTEN 仍连不上     → client IP 是不是 127.0.0.1；IPv6 localhost
  Address already in use  → ss 找旧 PID，kill -TERM

连上但不动
  ss -tnp | grep 端口      → 有没有 ESTAB；Send-Q/Recv-Q 是否堆
  日志最后一行时间        → 是否还在打 Hz
  strace -p PID -e accept,recv,send   # 短看 5 秒，Ctrl+C，勿一直开

CPU 打满
  top -H -p PID            → 哪条线程；对照是不是日志 Debug、忙等

Ctrl+C 后端口还在
  ss -lntp                 → 没退干净；ps 看还在不在；僵尸看 STAT=Z
```

口述句式：「我先看监听和连接，再看线程热度，最后才 attach strace。」这就是工程岗要的顺序。

## 3. 演示里要主动露一手（不是出了问题才敲）

10 分钟里留 **30～40 秒** 说：

```bash
ss -lntp | grep 7777
top -H -p PID    # 指给观众：这几条是传感/控制/网络，CPU 不高
# 若准备了：ldd build-rel/hub_server | head
```

一句话：「资源在预期里，延迟数字是 Release、没开 strace 测的。」Day26 笔记此时可以指一下。

不要现场第一次跑 `perf`、不要 sudo tcpdump 赌权限。用你彩排过的命令。

## 4. 崩了怎么圆（预习想好，比装没事强）

- ASan 红字：读第一行类型（overflow/UAF），承认 Debug 构建，指分配点。转 Release 演示功能。
- 端口占用：ss + TERM，10 秒恢复。
- client 挂了：Day27 重连，起新 client，server 不用重启——这反而是加分项，彩排一次「故意杀 client」。
- 完全起不来：备份录屏/日志截图，走架构口述。诚实说环境问题。

## 5. 易错点（Linux 侧）

1. **演示用 Debug+ASan+strace 同时开着**  
   卡、慢、数字和 Day26 对不上。彩排就定：演示 Release，出问题再换 Debug。
2. **在讲解时才第一次 `ss`**  
   选项会忘。写一张纸在旁边。
3. **杀错 PID**  
   `ps` 对一下命令行。

## Linux 口述（预习时自己答）

1. **演示连不上你当面试官面怎么做？**  
   ss 看 LISTEN → 看绑定地址 → 看 client 目标 → 占用则 TERM 旧进程。
2. **为什么演示中还要敲 top/ss？**  
   证明你会观察，不是只会跑 happy path；也给「卡了」留对照。
3. **现场排障为什么先命令后改代码？**  
   先分是环境、连接、忙等还是逻辑。改错地方浪费面试时间。

## Linux 上机（预习不用敲）

完整彩排一次：正常跑 + 故意杀 client 重连 + 假装连不上走 60 秒脚本。命令记在纸上或 `linux-notes.md`。见 `week4-project/day30-mock-interview/TASK.md`。

---

# 二、C++：10 分钟演示 + 一页纸故事

## 为什么需要

代码已经在 Day24–27。今天要的是**能讲、能跑、能回答「你解决了什么」**。文件：`notes/project-story.md`。自问自答 15 分钟用 Day28 提纲。

## 1. 十分钟结构（掐表）

| 时间 | 做什么 | 不要做什么 |
|---|---|---|
| 0:00–0:30 | 一句话目标 | 从 Day01 讲起 |
| 0:30–2:30 | 架构图：三线程 + RingBuffer + latest + TCP 定长头 | 念源文件列表 |
| 2:30–5:30 | 现场跑：seq 涨、每秒 Hz、指 ss/top | 现改代码 |
| 5:30–8:30 | 一个难点与修复（Day26 那一刀） | 罗列十个小改 |
| 8:30–10:00 | 还差什么 / 如何扩到多客户端或 ROS2 | 贬低自己的项目 |

一句话目标模板：

「这是一个本机遥测中枢：100Hz 模拟传感器，有界环形缓冲，控制线程做轻处理，TCP 把**最新**状态推给客户端；支持 Ctrl+C 清理和断线重连。」

架构就画 Day27 README 那张 ASCII。嘴上点名：满了覆盖、发送不持锁、协议 length 上限、handler 只设 atomic。

现场跑：Release + `run.sh`。指出 client 的 `seq` 和日志摘要里的 drop/P99（有就说，没有就说「笔记里有一次测量」）。

难点：只用 Day26 的 STAR。没有数字就讲一个正确性 bug（粘包、重连清空缓冲），不要编 P99。

扩展（挑 2 个说，表示你知道边界）：

- 多 client：`accept` 后线程池或 epoll（Day28 概念）
- 跨机：`0.0.0.0` + 防火墙 + 字节序
- ROS2：这层相当于一个 topic 的精简版，真实机器人用 DDS
- 真传感器：替换 sensor_thread 的填充函数，后面不用动

## 2. 项目故事一页纸（STAR）

写到 `notes/project-story.md`，半页到一页，能照着讲。

- **Situation**：要练一条可演示的真机式遥测链路（固定频率、有界内存、可观察、可退出），而不是散落的练习。
- **Task**：三线程接通；TCP 定长头；只要最新；能测延迟/丢包；Ctrl+C 与断线。
- **Action**：RingBuffer 满覆盖；latest 槽；Framer；日志分级；Day26 只修一点（写具体）；`run.sh`/`ldd`。
- **Result**：给出你的 Hz、P50/P99、drop、是否 ASan 干净；你学到的一句工程结论（例如「测比猜有用」或「持锁 send 会把产线拖死」）。

加三行「我不会装的」：没做 epoll、没做跨机、滤波很朴素。比假装做了强。

## 3. 自问自答 15 分钟

用 Day28 五题压缩版，每题约 2 分钟，最后留 5 分钟给 Hub 追问：

- 为什么可以丢中间帧？
- 粘包怎么拆？
- handler 为什么不 close socket？
- 若 P99 突然变高你现场怎么查？（接第一节 Linux）

弱项 Demo（Day29）当备用例子，不要当主演示。

## 4. 易错点

1. **十分钟讲构建系统和所有 Day**  
   面试官要的是一条链 + 一个深度点。
2. **现场加功能**  
   彩排冻结代码。
3. **故事里全是「学习了」没有「数字/现象」**  
   STAR 的 R 要具体。
4. **贬低作业**  
   可以说边界，不要说「只是玩具没什么」。它就是你的作品。

## C++ 面试口述（预习时自己答一遍）

1. **30 秒项目**  
   用上面那句目标。
2. **一个技术决策**  
   只要最新 / 覆盖旧帧：保内存和延迟，应用层丢旧遥测，不是 TCP 不可靠。
3. **一个你修过的问题**  
   Day26 或粘包/重连，带现象和结果。

## C++ 上机（预习不用写）

见 `week4-project/day30-mock-interview/TASK.md`：

1. 演示 Telemetry Hub 10 分钟（含 Linux 一眼 ss/top）
2. 自问自答 15 分钟（Day28）
3. 完成 `notes/project-story.md`：问题 → 方案 → 结果
