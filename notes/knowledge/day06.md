# Day06 · RingBuffer 与 /proc（详细预习）

日期：2026-08-23

预习目标：先用 `/proc` 看本机 CPU/内存；再搞懂环形缓冲的空/满判定，以及满时覆盖还是拒绝。上机先记环境，再写 `RingBuffer<T>`（后续项目会复用）。

---

# 一、Linux：`/proc` 入门

`/proc` 不是磁盘上的普通目录，是内核把**正在运行的系统状态**伪装成文件。`cat` 这些文件等于在问内核「现在硬件和进程怎样」。嵌入式板、工控机排障经常先看这里。

```bash
ls /proc
cat /proc/cpuinfo
cat /proc/meminfo
```

文件大小看起来常常是 0，但 `cat` 有内容：因为是现问现生成的。

## 1. `/proc/cpuinfo`：这台机器的 CPU 是谁

```bash
cat /proc/cpuinfo
```

重点看（x86 上常见这些键；ARM 板字段名可能不同）：

- **processor**：逻辑 CPU 编号。出现几次 ≈ 几个逻辑核（含超线程）
- **model name**：型号
- **cpu cores** / **siblings**：物理核 vs 逻辑核
- **flags**：指令集特征（`sse`、`avx`……）；后面看原子、内存序时会再碰到缓存行概念

数逻辑核：

```bash
grep -c "^processor" /proc/cpuinfo
nproc                          # 更短的写法
```

预习记下来：几核、什么型号。Week2 多线程时对照：你的程序是不是真的在用多个核。

## 2. `/proc/meminfo`：内存还剩多少

```bash
cat /proc/meminfo
```

先认这几个（单位默认 kB）：

- **MemTotal**：物理内存总量
- **MemFree**：完全没被用的（这个数字往往看起来很「少」，正常）
- **MemAvailable**：估算「再开新程序大概还能用多少」——**看这个比看 MemFree 有用**
- **Buffers / Cached**：缓存，内核可以收回给应用
- **SwapTotal / SwapFree**：交换分区

```bash
grep -E "MemTotal|MemAvailable|MemFree" /proc/meminfo
```

机器人板上内存紧，RingBuffer 用**定长**就是为了不在热路径上 `new`。对照：你的缓冲容量 × 元素大小，相对 MemAvailable 是否过大。

## 3. 顺带认识 `/proc/self`

```bash
ls -l /proc/self          # 指向当前进程的 /proc/<pid>
cat /proc/self/status     # 自己的进程信息（Name、VmRSS 等）
```

每个进程在 `/proc/<PID>/` 下有一份。Day02 用 `ps` 找 PID，今天知道详细材料在 `/proc` 里。预习能 `cat` cpuinfo/meminfo 并抄几个数字进笔记即可。

WSL 里这些文件也有，数字是虚拟机的，不是 Windows 任务管理器那份，笔记里写明环境。

## Linux 口述（预习时自己答）

1. `/proc` 是普通磁盘目录吗？  
   不是。内核提供的虚拟文件系统，读文件等于查询系统状态。
2. 看内存该看 MemFree 还是 MemAvailable？  
   优先 MemAvailable，它把可回收缓存算进去了。
3. 怎么数逻辑 CPU 个数？  
   `grep -c "^processor" /proc/cpuinfo` 或 `nproc`。

## Linux 上机（预习不用敲）

`cat /proc/cpuinfo`、`/proc/meminfo`，把核数、MemTotal、MemAvailable 记进当天 `linux-notes.md`，注明 WSL 还是真机。

---

# 二、C++：RingBuffer（环形缓冲）

## 为什么要环形缓冲

传感器高频数据、音视频、电机反馈：生产者一直来，消费者有时慢半拍。用 `vector` 一直 `push_back` 会无限涨；用 `queue` 一直分配节点。实时系统更想要：

- **固定内存**，构造时一次分配
- 下标转一圈就回来（取模），没有「搬所有元素」
- 满了时策略清晰：覆盖最旧，或拒绝写入

后续 Telemetry Hub 会复用这个结构。接口要干净，策略要在注释里写死。

## 1. 定长数组 + 读写游标

```text
下标:  0  1  2  3  4  5  6  7     capacity = 8
       [ ][A][B][C][ ][ ][ ][ ]
           ^        ^
           r        w
```

- **r (read)**：下一个要读的位置
- **w (write)**：下一个要写的位置
- 逻辑下标对 `capacity` 取模：`i % n`。为了避开 `%` 的开销，容量常用 2 的幂，用位运算 `i & (n-1)`；上机不必强制 2 的幂，`%` 写清楚即可。

```cpp
buf[w] = item;
w = (w + 1) % n;
```

读同理。不要每次 `push` 再 `new`。

## 2. 空 / 满怎么区分（必须选一种）

`w == r` 既可以是空，也可以是「写了一整圈刚好追上读」。所以要额外约定。

### 办法 A：牺牲一个空位

```text
空： w == r
满： (w + 1) % n == r     // 再写一步就会和 r 重合
```

容量 n 的槽，最多存 n−1 个元素。实现简单，不用另存 size，单产单消时也好做无锁。

### 办法 B：另存 `size_`（或写计数）

```text
空： size_ == 0
满： size_ == n
```

能用满全部 n 个槽。每次 push/pop 改 `size_`。多线程时 `size_` 和游标要一起考虑同步（今天先单线程）。

两种都对。上机**选一种贯彻**，`empty()` / `full()` / `size()` 必须和它一致。不要空用 A、满用 B 混着写。

## 3. 满时策略：覆盖 vs 拒绝（必须写清）

| 策略 | 满时 push 做什么 | 适合 |
|------|------------------|------|
| **覆盖最旧（overwrite）** | 丢掉 `r` 上那份，写入新的，`r` 也向前 | 遥测、IMU、最新状态更重要 |
| **拒绝写入（reject）** | 不写，返回 false / 抛错 / 断言 | 不能丢的指令、关键事件 |

机器人遥测常用覆盖：控制周期更关心「现在姿态」，丢几帧旧的可接受。指令队列常用拒绝或阻塞：丢一条「急停」不能接受。

```cpp
bool push(const T& x) {
    if (full()) {
        // 覆盖：pop 掉最旧再写；或直接覆盖 r 并 r++
        // 拒绝：return false;
    }
    // 写入 w，w 前进
}
```

注释里写死选了哪一种。测试要覆盖：空、满、刚满再 push、绕圈多次。

## 4. 和 `queue` / `vector` 比

- `queue`：不固定上限（除非自己包一层），节点可能多次分配
- `vector` 当队列：头弹是 O(n) 搬移（除非只用下标当环形，那就是 RingBuffer）
- RingBuffer：内存上限已知，实时友好；代价是满了必须有策略

单线程先写对。无锁环形缓冲通常要求**单写单读** + 原子游标；多写必须加锁。假共享（读写游标挤同一缓存行）了解即可，Week2 再展开。

## 5. 易错点（看懂再上机）

1. **空满判断写反**  
   `(w+1)%n==r` 当空，或 `w==r` 当满却没牺牲空位 → 覆盖未读数据或死循环。先画 4 格纸上走两圈。
2. **`size()` 和满策略不一致**  
   覆盖式 pop 掉最旧时 size 不应超过 cap；拒绝式 full 后 size 不变。
3. **对移动-only 的 `T`（如 `unique_ptr`）**  
   `push` 要有 `T&&` 重载；槽位里用 `std::move`。
4. **取模忘了**  
   `w++` 不 `% n` 会写出界。
5. **多线程当单线程用**  
   今天测试单线程。两个线程同时改 `w` 是数据竞争，不是「偶尔丢一个」。

## C++ 面试口述（预习时自己答一遍）

1. **为什么遥测常用覆盖式 RingBuffer？**  
   最新状态更重要，定长内存，丢旧帧可接受；分配次数为零。
2. **空和满如何区分？**  
   牺牲一个空位，或另存 size/计数。`w==r` 单独不够。
3. **和 `queue` 比优劣？**  
   环形：定长、无反复分配、满策略明确。`queue`：实现快、默认无上限、可能一直涨内存。

## C++ 上机（预习不用写）

见 `week1-cpp-basics/day06-ringbuffer/TASK.md`：

- `push` / `pop` / `size` / `full` / `empty`
- 满时策略写清（丢最旧或拒绝），至少 5 个单测
- 完成后可把头文件放到 `common/include/` 供后面用
