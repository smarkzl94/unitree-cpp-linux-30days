# Day08 · 线程视角与线程安全队列（详细预习）

日期：2026-08-25

预习目标：先学 Linux 怎么看**线程**（不是只看进程），再学 `mutex` + 条件变量实现 `ThreadSafeQueue`。上机先练命令，再写代码。

---

# 一、Linux：进程里的线程

Day02 用 `ps` / `top` 看的是**进程**。今天同一个 PID 里可以有多条**线程**（LWP，轻量级进程）。C++ 里 `std::thread` 跑起来，内核里就是这些 LWP。上机要对着**自己的**多线程程序看，不要只背命令。

## 1. `ps -eLf`：带线程的名单

```bash
ps -eLf
ps -eLf | grep your_program   # 找自己的程序（可能多行，每行一条线程）
```

先认这几列：

- **PID**：进程号。同一进程的多条线程，PID 相同。
- **LWP**：这条线程自己的 id（light-weight process）。
- **NLWP**：该进程一共有多少条线程。
- **CMD**：命令名。

`ps -eLf` 仍是**快照**：敲一下出一屏，不会自己刷新。和 Day02 的 `ps aux` 一样，只是多了线程维度。

主线程的 LWP 常常等于 PID；你再 `std::thread` 几个工人，就会多出几行、NLWP 变大。

## 2. `top -H`：按线程刷新

```bash
top -H
top -H -p $(pgrep -n your_program)   # 只盯某一个进程的线程
```

`-H`：**H**reads，每一行是一条线程，不是一个进程。CPU 百分比是这条线程在吃。按 `q` 退出。

对照：

| 命令 | 粒度 | 刷新 |
|------|------|------|
| `ps aux` / `ps -ef` | 进程 | 快照 |
| `ps -eLf` | 线程 | 快照 |
| `top` | 进程 | 直播 |
| `top -H` | 线程 | 直播 |

## 3. 和后面 C++ 对照

你会写一个生产者线程 `push`、一个消费者线程 `wait_and_pop`。Linux 上应至少看到：主线程 + 生产 + 消费（具体条数看你有没有 join 完、有没有额外线程）。

线程卡住时，`top -H` 里那条可能 CPU 接近 0（在等锁/条件变量），不是「没在跑」，而是在等。Day10 会用 gdb/strace 把「等」看清楚。

## Linux 口述（预习时自己答）

1. `ps aux` 和 `ps -eLf` 差在哪？  
   前者按进程；后者列出每条 LWP，同一 PID 可多行，看 NLWP/LWP。
2. `top` 和 `top -H` 差在哪？  
   默认按进程；`-H` 按线程，CPU 算到每条线程上。

## Linux 上机（预习不用敲）

对自己的多线程程序练 `ps -eLf` / `top -H`，对照线程数，写当天目录 `linux-notes.md`。

---

# 二、C++：今天会用到的基础（先读再写队列）

正课是 mutex / 条件变量。下面这些是写 `ThreadSafeQueue` **绕不开的语法和对象模型**，看懂再往下。

## 0.1 进程 vs 线程（和上午 Linux 对上）

一个**进程**有自己的地址空间、一个 PID。进程里可以有多条**线程**，共享同一块内存（全局变量、堆上的队列）。Linux 里线程叫 LWP；C++ 的 `std::thread` 跑起来就是多出来的 LWP。

共享内存 = 能方便传数据，也 = 两个人同时改同一个 `std::queue` 会炸。所以后面才要锁。

## 0.2 `std::thread`：一构造就开跑，必须 `join`

```cpp
#include <thread>
#include <iostream>

void worker() { std::cout << "hi\n"; }

int main() {
    std::thread t(worker);  // 到这里子线程已经在跑
    t.join();               // 主线程等到 t 结束；不 join 也不 detach → 析构时 terminate
}
```

- 传函数名、或 **lambda**：`std::thread t([&] { q.wait_and_pop(x); });`
- **`[&]`**：按引用抓住外面的 `q`。队列活在 `main` 的栈上，线程必须在 `q` 销毁前 `join` 完，否则悬空。
- 今天不要 `detach`：人跑了你管不着，shutdown 也对不上。

链接：`g++ -std=c++17 -pthread ...`

## 0.3 `.` 和 `->`

| 你手里是 | 用 | 例子 |
|----------|----|------|
| 对象、引用 | `.` | `q.push(1);` `lk.lock();` |
| 指针、`unique_ptr` | `->` | `p->push(1);` |

今天队列、锁、条件变量都是**成员对象**（`q_`、`m_`、`cv_`），一律 `.`。

## 0.4 引用参数 `T& out`

```cpp
bool wait_and_pop(T& out) {
    out = std::move(q_.front());  // 写回调用者的变量
    return true;                  // 另用 bool 表示成功/失败
}
```

`T&` 不是拷贝：函数里改 `out` 就是改外面那个变量。失败时（shutdown 且空）返回 `false`，不要去 `pop` 空队列。

## 0.5 模板皮毛 `ThreadSafeQueue<T>`

和 Day06 `RingBuffer<T>` 一样：`T` 是占位，「什么类型的队列」。实现放**头文件**（模板要在用到的地方能看见完整定义）。

```cpp
ThreadSafeQueue<int> q;   // T = int
```

成员写成 `std::queue<T> q_;` 即可。

## 0.6 `std::move` 和 `std::queue`

`std::queue`：`push` / `front` / `pop`（`pop` 只删不返回元素，所以先 `front` 再 `pop`）。

大对象不要白拷贝：`q_.push(std::move(x));`、`out = std::move(q_.front());`。`int` 搬不搬差不多；养成习惯，后面传感器帧会变大。

## 0.7 RAII：作用域结束 = 析构 = 解锁

Day02 的 `unique_ptr`：离开作用域自动 `delete`。锁也是同一招：

```cpp
{
    std::lock_guard<std::mutex> lk(m_);  // 构造：加锁
    q_.push(x);
}  // 析构：解锁。中途 return / 抛异常也会走这里
```

不要手写 `m.lock()` / `m.unlock()`。`cv.wait` 必须中途放锁，所以要能「暂时解锁」的 **`unique_lock`**，不能用死拿着的 `lock_guard`。细节见下一节。

## 0.8 类里的成员和下划线

```cpp
class ThreadSafeQueue {
    std::queue<T> q_;
    std::mutex m_;
    std::condition_variable cv_;
    bool shutdown_ = false;
};
```

末尾 `_` 只是习惯：这是对象自己的数据。`public` 里写 `push` 等接口；上面这些放 `private`，外面不许直接摸队列。

建议头文件：`<queue>` `<mutex>` `<condition_variable>` `<utility>`（`std::move`）。

---

# 三、C++：ThreadSafeQueue

## 为什么需要线程安全队列

机器人里典型模式：**传感器线程产数据、控制线程消费**。两边同时碰同一个 `std::queue` 就是数据竞争——未定义行为，不是「偶尔错一下」。

保护方式：一把 `mutex` 护住队列；队列空时消费者不要空转死循环，用 **条件变量** 睡过去，有数据再醒。这是面试高频，也是后面 MPMC、Hub 的底座。

## 1. `mutex` 与 RAII 锁

**互斥**：同一时刻只有一个线程能持有这把锁。锁里的代码叫临界区，队列的读写必须在临界区里。

```cpp
#include <mutex>

std::mutex m;
{
    std::lock_guard<std::mutex> lk(m);  // 构造加锁，析构解锁
    // 改共享数据
}  // 离开作用域自动解锁，异常也能解开
```

- `lock_guard`：简单 RAII，**不能**中途解锁，也不能交给 `condition_variable::wait`。
- `unique_lock`：可以 `unlock`/`lock`，`cv.wait` **必须**配它。

不要手写 `m.lock()` / `m.unlock()`：漏一条路径（`return`、异常）就会永远锁死。

## 2. 条件变量：等「某个条件成立」

```cpp
std::condition_variable cv;
std::unique_lock<std::mutex> lk(m);
while (!ready) {
    cv.wait(lk);  // 原子地：解锁并睡眠；被唤醒后重新加锁再返回
}
```

工作过程：

1. 必须先持有 `unique_lock`。
2. `wait` 把锁放开，让别人能 `push`、改 `ready`。
3. 别人改完条件后 `notify_one` / `notify_all`。
4. 本线程醒过来时**已经重新拿到锁**，再检查条件。

`notify_one`：叫醒一个等待者。`notify_all`：全叫醒。`shutdown` 时必须 `notify_all`，否则有人永远睡。

**先改共享状态，再 notify**（都在持锁时改状态最干净）。只 notify、忘了改标志，醒了还是不满足，会再睡回去。

## 3. 必须用 `while`，不能用 `if`

虚假唤醒：没人 notify，内核也可能把 `wait` 返回。POSIX 允许。  
另一个原因：MPMC 时你被叫醒，但数据已经被别的消费者拿走了。

所以公式永远是：

```cpp
while (!pred) cv.wait(lk);
```

`if (!pred) cv.wait(lk);` 是经典坑：醒一次就当条件真了，队列空还 `pop` → 未定义。

`wait(lk, pred)` 等价于上面的 while，内部也是循环判断。

## 4. 接口：`push` / `wait_and_pop` / `try_pop` / `shutdown`

```cpp
class ThreadSafeQueue {
public:
    void push(T x) {
        {
            std::lock_guard<std::mutex> lk(m_);
            if (shutdown_) return;  // 或抛错，策略写清
            q_.push(std::move(x));
        }
        cv_.notify_one();  // 可以锁外 notify，避免被叫醒的人立刻和你抢锁
    }

    bool wait_and_pop(T& out) {
        std::unique_lock<std::mutex> lk(m_);
        while (q_.empty() && !shutdown_) cv_.wait(lk);
        if (q_.empty()) return false;  // 被 shutdown 唤醒且没数据
        out = std::move(q_.front());
        q_.pop();
        return true;
    }

    bool try_pop(T& out);  // 空则立即 false，不睡
    void shutdown();       // 置标志 + notify_all
};
```

- `wait_and_pop`：阻塞到有元素或 shutdown。
- `try_pop`：非阻塞，控制循环「这一拍没有就用旧状态」时会用到。
- 临界区里不要做重计算、打海量日志：锁持太久，吞吐崩。

## 5. `shutdown`：让等待线程能退出

只靠析构不够：若消费者卡在 `cv.wait`，析构条件变量/队列是未定义。正确收尾：

1. 置 `shutdown_ = true`（持锁）。
2. `notify_all`。
3. 等待线程醒了：`while` 看到 shutdown 且队列空 → 返回失败，不要再等。
4. 主线程 `join` 所有工人，再让队列析构。

上机要亲手验证：有人堵在 `wait_and_pop` 时调 `shutdown`，程序能退出、不挂死。

## 6. 易错点（看懂再上机）

1. **`if` 代替 `while`**：虚假唤醒或多消费者抢空。
2. **忘记 notify**：队列有数据，消费者仍永睡。
3. **`lock_guard` 配 `cv.wait`**：编译都过不了，必须 `unique_lock`。
4. **析构时还有人 wait**：先 shutdown + join。
5. **持锁做重活**：锁是为了护队列，不是护整个控制算法。

## C++ 面试口述（预习时自己答一遍）

1. **为什么必须 `while` 判断，不能 `if`？**  
   虚假唤醒；以及多消费者时醒了数据已被拿走。醒后必须再检查谓词。

2. **`lock_guard` 和 `unique_lock` 差在哪？**  
   都是 RAII。`wait` 要中途放锁，只能 `unique_lock`。简单临界区用 `lock_guard` 即可。

3. **`shutdown` 为什么要 `notify_all`？**  
   所有可能堵在 `wait` 的线程都要醒，看到标志后退出，否则 `join` 永远等。

## C++ 上机（预习不用写）

见 `week2-concurrency/day08-threadsafe-queue/TASK.md`：

- `push` / `wait_and_pop` / `try_pop`
- `shutdown()` 能唤醒等待线程
- 单生产单消费跑通
- Linux 侧用 `ps -eLf` / `top -H` 对照线程数
