# Day10 · gdb/strace 看死锁与四条件（详细预习）

日期：2026-08-27

预习目标：先学用 `gdb` 看线程栈、用 `strace` 看见 `futex` 等待，再学死锁四条件、交叉加锁，以及用固定顺序 / `scoped_lock` 修复。上机先练观察，再写「故意死锁 → 修好」。

---

# 一、Linux：死锁时看线程与 futex

程序「卡住、CPU 也不高」常常不是死循环，而是在等锁。Linux 里用户态 `std::mutex` 底层多是 **futex**（fast userspace mutex）：没竞争时用户态原子指令搞定；要睡时陷入内核 `FUTEX_WAIT`。

今天只观察**自己写的**死锁程序，学会读栈和系统调用，不把这些当攻击工具。

## 1. `gdb`：挂上进程，列出线程

```bash
gdb ./deadlock_demo
(gdb) run
# 另开终端：程序卡住后再
gdb -p PID
```

常用：

```text
info threads          # 所有线程，* 是当前线程
thread 2              # 切到线程 2
bt                    # backtrace 调用栈
thread apply all bt   # 每个线程都打栈（死锁首选）
```

死锁典型画面：两个线程栈顶都在 `std::mutex::lock` / `__pthread_mutex_lock` / `futex`，各自已经持有对方想要的那把锁。你要对着源码行看：**线程 A 停在锁 2，线程 B 停在锁 1**。

笔记里贴关键几帧即可（函数名 + 你的 `lock` 那一行），不要整屏寄存器。

卡住时不要急着 `kill -9`；先 gdb 看完再停。`-9` 来不及析构，和 Day02 一样。

## 2. `strace`：看见它在等 futex

```bash
strace -p PID                 # 跟现有进程
strace -f -p PID              # -f：子线程也跟
strace -f ./deadlock_demo     # 从启动跟
```

死锁时两条线程反复或停在类似：

```text
futex(0x..., FUTEX_WAIT_PRIVATE, ...)
```

含义：在等某个地址上的锁被释放。对死锁诊断，**确认「在等锁而不是算东西」** 就够。不必背全 futex 参数。

对比：活着的生产者会看到 `write`/`nanosleep` 等；纯 CPU 死循环几乎没有这些 wait。`strace` 会明显拖慢程序，测性能时关掉。

## 3. 和 gdb 怎么配合

1. 复现卡住。
2. `top -H`：CPU 接近 0 → 更像等锁，不像空转。
3. `strace -f -p PID`：看到 `FUTEX_WAIT`。
4. `gdb -p PID` → `thread apply all bt`：哪两把锁、哪两行代码。

这就是岗上排障最小闭环。Day08 的「线程在等」今天能看见等的是 futex。

## Linux 口述（预习时自己答）

1. 死锁时 `gdb` 先打哪条命令？  
   `info threads`，再 `thread apply all bt`，看谁卡在哪把锁。
2. `strace` 里和 mutex 最相关的调用是什么？  
   `futex`，常见 `FUTEX_WAIT`：线程在睡等锁。
3. `-f` 是干什么的？  
   follow：子进程/线程的系统调用也跟踪，多线程必须加。

## Linux 上机（预习不用敲）

故意卡住后用 gdb + strace 记栈和 futex，写入 `notes/day10-deadlock.md`（与 TASK 一致）。

---

# 二、C++：死锁四条件与修复

## 0. 锁的基本知识（补）

锁保护的是**共享数据**，不是「整个函数」。没有共享、或只有一个线程，就不需要锁。

- **临界区**：持锁期间那几行。只放「必须互斥的读写」，不要 sleep、不要调未知回调、不要打海量日志。
- **一把锁护一份不变量**：Day08/09 的 `m_` 护的是 `q_` + `shutdown_`。两把无关的数据可以两把锁；有交叉时就要规定顺序。
- **数据竞争**：两个线程同时碰同一内存，至少一个在写，且没有同步 → 未定义行为。锁是同步手段之一。
- **拿锁的人在等另一把** 才可能死锁；拿锁的人在干活叫占用，别人在门口等叫**争用（contention）**，程序还能往前走，只是变慢。
- 用户态 `std::mutex`：**不可抢占、不可重入**。同一线程再锁同一把 = 自死锁。

守卫复习：`lock_guard` 一把且不中途放；`unique_lock` 才能 `cv.wait`；多把用 `scoped_lock` 或统一顺序。

## 为什么要亲手制造死锁

多锁系统（日志锁 + 队列锁 + 状态锁）很容易 **A 先 1 后 2，B 先 2 后 1**。会修的前提是会稳定复现、会用昨天的队列直觉（锁是互斥的、不能抢）。工程上以**预防**为主：约定顺序，而不是上线后再「检测」。

## 1. 死锁四个必要条件（缺一不可）

1. **互斥**：资源一次只能一人持有（mutex 天生满足）。
2. **占有且等待**：已经拿着锁 1，还想拿锁 2。
3. **不可抢占**：不能把别人手里的 mutex 夺走（用户态 mutex 就是这样）。
4. **循环等待**：A 等 B 的锁，B 等 A 的锁（可以更长的环）。

打破任意一条就不会死锁。实践中最可控的是打破第 4 条：**全局统一加锁顺序**。

## 2. 经典交叉：AB-BA

```cpp
std::mutex m1, m2;

void tA() {
    std::lock_guard<std::mutex> a(m1);
    std::this_thread::sleep_for(10ms);  // 加大窗口，稳定复现
    std::lock_guard<std::mutex> b(m2);  // 等 m2
}

void tB() {
    std::lock_guard<std::mutex> b(m2);
    std::this_thread::sleep_for(10ms);
    std::lock_guard<std::mutex> a(m1);  // 等 m1
}
```

A 持 m1 等 m2，B 持 m2 等 m1 → 环。上机先让这个**必现**，再用 gdb 拍栈，最后再修。不要修完才发现「偶发复现不了」。

单把锁不会用这种方式死锁；死锁至少两把（或同一把锁重复锁且非递归——那是自死锁，也要认识：`std::mutex` 不可重入）。

## 3. 修复一：固定加锁顺序

给每把锁一个编号，**任何线程都按编号从小到大加**。

```cpp
void both() {
    std::lock_guard<std::mutex> a(m1);  // 永远先 m1
    std::lock_guard<std::mutex> b(m2);  // 再 m2
}
```

A、B 都先 m1 再 m2：可能有人等 m1，但不会成环。把「顺序」写进注释/文档，后人加第三把锁时继续遵守。

若需要的锁集合运行时才知道：先排序 mutex 地址或 id，再按序 `lock`。

## 4. 修复二：`std::scoped_lock`（C++17）

```cpp
#include <mutex>
std::scoped_lock lk(m1, m2);  // 一次锁多把，内部避死锁算法
```

它会对多把锁使用类似 `std::lock` 的协议（试锁+回退），避免 AB-BA。作用域结束一起解锁。

注意：

- 仍不要在持锁时再去锁「没交给这次 `scoped_lock` 的第四把」却顺序相反。
- `lock_guard` 只能管一把；多把用 `scoped_lock` 或手动保证顺序。
- `std::lock(m1, m2)` 只加锁，要自己配 `adopt_lock` 的 `lock_guard`，不如直接 `scoped_lock` 干净。

## 5. 其它预防（知道即可）

- **少锁**：能一把护一个结构就不要拆两把再交叉。
- **锁层级**：规定「先队列锁后日志锁」，禁止反向。
- **不要持锁调未知回调**：回调里可能再锁，顺序失控。
- **检测**（超时、死锁检测器）是网，不是设计。

## 6. 易错点（看懂再上机）

1. 修完只跑一次幸福路径，没有压力/重复跑。
2. 用 `sleep` 复现后，修完把 sleep 删掉就以为好了——应用顺序/scoped_lock 保证，不靠运气。
3. `recursive_mutex` 当万金油：能掩盖设计问题，仍可能多锁成环。
4. 忘记文档化锁顺序，Week4 Hub 里再引入第三把锁。

## C++ 面试口述（预习时自己答一遍）

1. **死锁四条件？举一个你修过的例子。**  
   互斥、占有且等待、不可抢占、循环等待。例子：两线程对 m1/m2 交叉加锁；改为统一先 m1 后 m2 或 `scoped_lock(m1,m2)`。

2. **`scoped_lock` 解决什么问题？**  
   一次锁多把 mutex，用避死锁算法，避免手动 AB-BA。

3. **预防和检测哪个为主？**  
   工程上预防（顺序、少锁、scoped_lock）。gdb/strace 是复现后的检测手段。

## C++ 上机（预习不用写）

见 `week2-concurrency/day10-deadlock/TASK.md`：

- 两锁交叉，稳定复现
- gdb 看线程栈，笔记贴关键回溯
- 固定顺序或 `scoped_lock` 修复
- `strace -f` 对照 futex
