# Day26 · 压测时看负载与修一个性能点（详细预习）

日期：2026-09-12

预习目标：先学会在负载下用 `top` / `perf stat` 记录资源，再给 Hub 测延迟 P50/P99 和丢包，并**只修 1 个**真实问题。上机先有对照数字，再改代码。

四天地图见 [day24-27-hub.md](day24-27-hub.md)。

---

# 一、Linux：压测时的 `top` 与 `perf stat`

Day24 记下了空载基线。今天把频率拉到目标（100Hz 产、网络 50Hz 或更高），看 CPU、调度、指令数是不是异常。**`strace` 今天默认不要开**（Day16/20：它会让延迟假高）。

## 1. 压测时 `top` 看什么

```bash
top -H -p PID
# 另开终端
ps -o pid,pcpu,pmem,rss,nlwp,cmd -p PID
```

记一张小表（写进 `notes/day26-benchmark.md`）：

| 项 | 基线（Day24） | 压测 |
|---|---|---|
| 进程 %CPU | | |
| 最热线程 | | |
| RSS | | |
| NLWP | | |

判断：

- CPU 随频率线性涨一点：正常。
- 一核打满、频率还上不去：忙等、锁、或日志。
- RSS 持续爬：队列无界或泄漏（ASan Debug 编一份对照）。
- NLWP 变：不该在压测里还创建线程。

`top` 的瞬时值会抖，看 5～10 秒的观感，或用：

```bash
pidstat -t -p PID 1 10          # 有 sysstat 的话
```

没有 `pidstat` 就手记 `top`。

## 2. `perf stat`：有则用，没有就跳过

```bash
perf stat -p PID -- sleep 10
# 或
perf stat ./hub_server 7777     # 会等进程自己退，不适合长服务
```

常见输出：`cycles`、`instructions`、`task-clock`、`context-switches`、`cache-misses`。

预习只要求建立直觉：

- **context-switches 极高**：线程在锁/cv/短睡眠上打转，或频率睡眠方式太碎。
- **instructions 很多但业务帧很少**：格式化日志、多余拷贝、每帧 `new`。
- **task-clock 接近 1000ms × 核** 而帧率不高：忙等。

WSL / 权限可能没有 `perf`。没有就写「不可用」，用 `top` + 程序自己的计时。不要为装内核旁路花掉今天。

```bash
# 有的系统
sudo apt install linux-tools-common linux-tools-$(uname -r)
```

装不上就算了。

## 3. 和程序内指标的分工

| 来源 | 告诉你 |
|---|---|
| 程序日志每秒摘要 | Hz、丢包、P50/P99（你自己算） |
| `top` | 谁热、内存、线程数 |
| `perf stat` | 是不是空转、切线程是不是疯 |
| `ss -tnp` | Send-Q 是否堆着（网络跟不上） |

三者对不上才有故事：「P99 高 + Send-Q 堆 → 发太勤或 client 慢 → 修 latest 覆盖，不修算法。」

## 4. 易错点（Linux 侧）

1. **开着 strace 测 P99**  
   数字作废。
2. **Debug + ASan 当正式压测**  
   ASan 很慢。测速用 `build-rel`（`-DCMAKE_BUILD_TYPE=Release`），正确性另用 Debug+ASan 跑短测。
3. **机器上还在编译、还开着浏览器**  
   笔记写清环境，数字是「这台 WSL 的相对值」，不是论文。

## Linux 口述（预习时自己答）

1. **压测时为什么不用 strace？**  
   每个 syscall 被拦截，延迟和 CPU 都失真。strace 是排障，不是测速。
2. **`top` 和 `perf stat` 怎么配合？**  
   top 告诉哪条线程热；perf 告诉是切线程多还是指令空转。没有 perf 就 top + 自测指标。
3. **Debug+ASan 和 Release 为何要分开？**  
   插桩改变时间和地址。正确性一套，数字一套。

## Linux 上机（预习不用敲）

Release 跑目标频率，记 `top -H` 与（若有）`perf stat` 10 秒。写入 `notes/day26-benchmark.md`，和 Day24 基线对照。

---

# 二、C++：延迟 / 丢包，并修 1 点

## 为什么需要

「感觉挺快」不能上面试。要有：**目标频率有没有到、延迟分布、丢了多少、你改了哪一刀、数字怎么变。** 只修 1 个点——学会测量比乱优化十处重要。

上机任务：冲到目标频率；记录 P50/P99 与丢包；修 1 个性能问题；结果写入 `notes/day26-benchmark.md`。

## 1. 先定义指标（写进代码注释和笔记）

**延迟**：建议 `now_mono - frame.stamp_ns`（传感打戳 → 控制处理完或 client 收到）。两端都用单调时钟。不要用墙钟算间隔（可能跳）。

**P50 / P99**：把一段窗口（如 5 秒）的延迟排序，50%、99% 位置的值。实现：环形存最近 N 个，拷出来 `nth_element`。N=512 够画个分布。

```cpp
std::vector<double> ms = samples;
auto pct = [&](double p) {
    if (ms.empty()) return 0.0;
    size_t k = static_cast<size_t>(p * (ms.size() - 1));
    std::nth_element(ms.begin(), ms.begin() + k, ms.end());
    return ms[k];
};
```

**丢包**：RingBuffer 覆盖次数 + 「latest 被新帧覆盖但从未发送」的次数。TCP 本身不丢；这是**应用层主动丢旧遥测**。面试要主动说，免得被追问「TCP 怎么会丢」。

**频率**：每秒 `frames_in` / `frames_sent` / `frames_recv`。入 100、出 50 是设计如此，不要标成 bug。

## 2. 怎么测（顺序别反）

```text
1. Release 编译，日志 Warn，关 Debug
2. 跑 20～30 秒热身，再取 10 秒稳定窗口
3. 记下 P50/P99、drop、Hz、top/perf
4. 改这一处（假设写在笔记里）
5. 同样窗口再测一遍，对比
```

不要一边改十个地方一边看数字。

## 3. 常见「值得修的 1 点」（选你真测到的）

对照着查，有哪个修哪个：

1. **每帧日志 / `std::endl` / `stringstream`**  
   `top` 里热在写日志。改成每秒摘要。通常 P99 立刻掉。
2. **忙等**  
   `while (!rb.pop()) {}`。改成 `cv.wait` 或 `sleep_until` 下一拍。CPU 从 100% 掉下来。
3. **持锁 send**  
   网络卡顿拖住传感。改 snapshot（Day24 已讲，若你没做就是今天的点）。
4. **每帧 `vector`/`string` 分配**  
   Framer 反复 `erase` 头、每次 `send` 都 `new`。预分配缓冲。
5. **sleep 不准导致堆叠**  
   `sleep(10ms)` 不管已经花的时间，循环越来越挤。改 `sleep_until(next)`。
6. **Send-Q 堆 + 无覆盖**  
   改「只发 latest」，丢包计数上升但 P99 下降——这是正确权衡，写进笔记。

修完用一句话写：**现象 → 假设 → 改动 → 数字变化。** 这就是 Day30 的「一个难点」。

## 4. 笔记模板（直接按这个填）

```text
环境：WSL / 机器 / Release 是否 / 有无 perf
目标：产 100Hz，发 50Hz
改前：P50=  P99=  drop=  CPU=  最热线程=
改动：只写一处
改后：P50=  P99=  drop=  CPU=
结论：为什么变好（或没变、说明假设错了）
```

没变也好：排除了一个假设，仍算今天完成，只要测量是诚实的。

## 5. 易错点

1. **用 Debug+ASan 的数字对外讲「我很快」**  
   分开报。
2. **P99 用平均值冒充**  
   面试官会问分布。平均被长尾骗。
3. **把「50Hz 发送所以入 100 出 50」叫丢包**  
   那是降采样。丢包是缓冲覆盖或未发送就被顶掉。
4. **优化拷贝却不测**  
   没有对照就不算修。
5. **同时改协议、改线程数、改滤波**  
   不知道是谁好的。

## C++ 面试口述（预习时自己答一遍）

1. **你怎么证明 Hub 达到频率？**  
   程序内每秒计数 + 单调时钟；对照 `top` 确认不是空转。给出 P50/P99 和丢包定义。
2. **满缓冲覆盖是性能问题还是设计？**  
   对只要最新的遥测是设计。要讲清丢的是旧帧、内存有界、延迟不堆。
3. **修了哪一点？**  
   用你笔记里的 STAR：现象、改动、数字。没有数字就说还没测完，不要编。

## C++ 上机（预习不用写）

见 `week4-project/day26-benchmark/TASK.md`：

- 冲到目标频率
- 记录延迟 P50/P99 与丢包
- 修 1 个性能问题
- 写入 `notes/day26-benchmark.md`，附 `top`/`perf` 观察
