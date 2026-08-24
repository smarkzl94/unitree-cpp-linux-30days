# Day12 · 墙钟/单调时钟与 100Hz 传感器（详细预习）

日期：2026-08-29

预习目标：先分清 Linux 上的**墙钟**和**单调时钟**，再写 100Hz 模拟传感器帧写入 Day06 的 `RingBuffer`。上机先看时间命令，再写定频生产。

---

# 一、Linux：`date` / `timedatectl`，墙钟 vs 单调

「现在几点」和「过了多少毫秒」不是同一类时间。NTP 对时、用户改系统时间，会让墙钟**跳跃**；用来量 10ms 周期会算出负间隔或超大间隔。机器人测频率、测延迟，间隔要用**单调时钟**。

## 1. `date`：墙钟（给人看的时刻）

```bash
date                      # 本地墙钟
date -u                   # UTC
date +%s                  # Unix 秒（1970 起）
date +%s.%N               # 秒.纳秒（GNU date）
```

这是 **CLOCK_REALTIME** 一类：和日历、时区、NTP 绑定。日志里「2026-08-29 10:00:00 来了一帧」用墙钟合理。

## 2. `timedatectl`：系统时区与 NTP 状态

```bash
timedatectl
```

常见字段：Local time、Universal time、Time zone、NTP service / synchronized。WSL 或虚拟机里时间可能和 Windows 同步，墙钟仍可能被调。

预习要建立：系统时间**可以被改**。改完 `date` 立刻变；你的 `system_clock` 时间戳也会变。

## 3. 单调时钟：只往前走，适合测间隔

内核还有 **CLOCK_MONOTONIC**（及类似 CLOCK_MONOTONIC_RAW）：从某开机起点累加，不受 NTP 调时影响（休眠是否计入因种类而异，预习记住「测间隔用单调」）。

```bash
# 若有 python：对照两类时钟（概念演示）
python3 -c "import time; print(time.time()); print(time.monotonic())"
```

C++ 对应：

- `std::chrono::system_clock` ≈ 墙钟，可跳，能转日历。
- `std::chrono::steady_clock` ≈ 单调，适合 `now - last`、sleep 补偿、测实际 Hz。

`clock_gettime(CLOCK_MONOTONIC, &ts)` 是 POSIX 写法，和 `steady_clock` 同一类用途。

## 4. 对照表（背下来）

| 用途 | 用哪个 |
|------|--------|
| 日志「几点几分」 | 墙钟 `date` / `system_clock` |
| 两帧间隔、100Hz 是否达标 | 单调 `steady_clock` |
| 延迟 `now - frame.ts` | **生产时戳和 now 必须同一类时钟** |

若帧上打了墙钟，消费端用单调去减，结果无意义。Day13 统计延迟会踩这个坑。

## Linux 口述（预习时自己答）

1. `date` 和单调时钟差在哪？  
   `date` 是墙钟，可被 NTP/手动修改；单调时钟测间隔，不跟日历跳。
2. `timedatectl` 能看出什么？  
   本地/UTC、时区、是否 NTP 同步——说明墙钟受系统管理。
3. 测 100Hz 实际频率该用哪类时钟？  
   单调时钟，用一段时间的帧数除以单调经过的时间。

## Linux 上机（预习不用敲）

跑 `date`、`timedatectl`，在 `linux-notes.md` 写清墙钟 vs 单调、各用在哪。

---

# 二、C++：100Hz 传感器 → RingBuffer

## 为什么是这一天

感知侧典型链路：**固定频率采样 → 带时间戳的帧 → 有界缓冲**。Hub 项目会复用。缓冲用 Week1 的 `RingBuffer<T>`：固定内存，满了按既定策略覆盖或拒绝。遥测常**覆盖最旧**——控制更在乎最新姿态，不在乎中间每一帧都在。

## 1. 数据帧：时间戳 + 序列号 + payload

```cpp
struct SensorFrame {
    std::chrono::steady_clock::time_point ts;  // 与测间隔同一时钟
    uint64_t seq;
    float acc[3];   // 模拟 IMU；关节角同理
};
```

- **seq**：后面 Day13 用间隙算丢包。
- **ts**：打在**生产瞬间**，不要等写入成功再随便补一个墙钟。
- payload 今天模拟即可，不必接真 IMU。

## 2. 定频 100Hz：周期 T = 10ms

100 次/秒 ⇒ 周期 10ms。软实时做法：循环里干活，然后睡到下一拍。

```cpp
using clock = std::chrono::steady_clock;
auto next = clock::now();
uint64_t seq = 0;
while (running) {
    SensorFrame f;
    f.ts = clock::now();
    f.seq = seq++;
    // 填 payload
    buffer.push(f);          // 满则覆盖或丢弃，与 Day06 策略一致

    next += 10ms;
    std::this_thread::sleep_until(next);  // 比 sleep_for(10ms) 更能抗「本拍超时」
}
```

`sleep_for(10ms)` 每次从「睡之前」再算 10ms，若本拍工作花了 2ms，实际周期约 12ms，**误差累积**。`sleep_until` 对齐时间轴，工作超时会少睡或不睡（可能赶下一拍，仍可能丢拍——要测量）。

`sleep` 只能近似：调度、定时器分辨率、WSL 都会抖。真实系统用定时器/时钟源；今天允许近似，但必须**测实际频率**，不要假设「写了 10ms 就是 100Hz」。

## 3. 写入 RingBuffer：满时策略写死

沿用 Day06：

- **覆盖最旧**：适合最新状态（遥测/控制输入）。
- **拒绝写入**：适合不能丢的指令（更像 Day09 有界阻塞队列）。

传感器 → 控制，通常覆盖。覆盖次数就是一种「丢包」，Day13 会统计。生产过快、消费过慢时，有界+覆盖避免无限内存，也可能饿死「中间帧」——这是策略，不是 bug；**策略要打印出来**。

单产单消时 RingBuffer 仍要注意：多线程下游标要原子或加锁。今天若复用无锁环形，约定单写；否则 mutex 护住，正确优先。

## 4. 测量实际频率

用单调时钟：

```cpp
auto t0 = clock::now();
uint64_t n0 = seq;
// ... 跑 2～5 秒 ...
auto t1 = clock::now();
double hz = (seq - n0) / std::chrono::duration<double>(t1 - t0).count();
std::cout << "actual_hz=" << hz << '\n';
```

测试太短（几百毫秒）统计不稳。墙钟在这段里若发生 NTP 跳变，用 `system_clock` 去除法会炸。

软实时 vs 硬实时（面试概念）：软实时「超时不好、系统还活着」（今天这类）；硬实时「超时即故障」，要专门内核/调度，不是 `sleep_until` 能保证的。

## 5. 易错点（看懂再上机）

1. 只用 `sleep_for`，不管累计误差。
2. 帧时间戳用墙钟，间隔用单调，混减。
3. 满策略说不清，和 Day09「不丢阻塞」搞混。
4. 生产过快无界队列把控制线程/内存打死。
5. 打印实际 Hz 却用墙钟做分母。

## C++ 面试口述（预习时自己答一遍）

1. **如何测量实际采样率？**  
   单调时钟起止，帧数差除以秒。不要用会跳变的墙钟做间隔。

2. **软实时和硬实时差别？**  
   软：超时降级仍运行。硬：必须在时限内，超时不可接受。`sleep` 近似属于软实时。

3. **为什么遥测 RingBuffer 常用覆盖？**  
   控制要最新状态；中间帧可丢。内存固定，背压清晰。

## C++ 上机（预习不用写）

见 `week2-concurrency/day12-sensor-producer/TASK.md`：

- 固定周期生产（sleep 近似）
- 带时间戳的帧
- 写入 RingBuffer，打印实际频率
