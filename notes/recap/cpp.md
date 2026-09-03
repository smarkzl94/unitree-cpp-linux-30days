# C++ 概念速查

按主题归类。左边概念 / API，右边干什么。带 ★ 的常用、面试也容易被问。

一条线：**谁拥有内存 → 容器与调度 → 定长环 → 线程安全队列 → 死锁/atomic → 定频产销与指标。**

---

## 1. 所有权与五件套

| 概念 / API | 作用 |
|------------|------|
| RAII | ★ 资源跟对象走：构造申请，析构释放 |
| 析构 | 释放自己管的资源。`delete[] nullptr` 安全 |
| 拷贝构造 | 对象还不存在，深拷贝一份新内存 |
| 拷贝赋值 | ★ 左边已有资源：先防 `s = s`，再放旧，再深拷贝 |
| 移动构造 | ★ 偷指针，源置 `nullptr`，源析构必须安全 |
| 移动赋值 | 先放自己的，再偷，源置空 |
| Rule of Five | ★ 有裸资源：五个一起写，或全 `= delete` |
| Rule of Zero | 成员已是 `vector` / `unique_ptr` 等，一个都不用写 |
| 浅拷贝 | 只拷指针 → 析构 double-free |
| 深拷贝 | 各自一份内容，互不影响 |

---

## 2. 移动与 `noexcept`

| 概念 / API | 作用 |
|------------|------|
| `std::move(x)` | ★ 把 `x` 当成右值，允许走移动；本身不搬内存 |
| 临时量 / 右值 | 优先走移动 |
| `return` 局部 | 通常移动，或 RVO 省略拷贝 |
| `noexcept` 标在移动上 | ★ 告诉 `vector` 扩容可以移动；没标可能退化成拷贝 |

---

## 3. 智能指针

| 概念 / API | 作用 |
|------------|------|
| `unique_ptr<T>` | ★ 独占。不能拷，只能移 |
| `shared_ptr<T>` | 引用计数。拷贝 +1，减到 0 才 delete |
| `weak_ptr<T>` | ★ 观察不加计数；用前 `lock()`。打破环 |
| `make_unique` / `make_shared` | ★ 优先于手写 `new` |
| 循环引用 | 互 `shared_ptr` 泄漏。一侧改 `weak_ptr` |

`.` 用在对象上，`->` 用在指针 / 智能指针上。

---

## 4. 动态数组：`vector`

| 概念 / API | 作用 |
|------------|------|
| `size()` | ★ 已经有几个元素 |
| `capacity()` | ★ 这块内存还能装几个（≥ size） |
| `push_back` | 往末尾放 |
| `emplace_back` | ★ 在容器里原地构造 |
| `reserve(n)` | ★ 只订容量，少扩容 |
| `resize(n)` | 真改元素个数 |
| 扩容 | 新块 → 搬元素 → 释放旧块；旧迭代器全部失效 |
| `operator[]` | 不查越界 |
| `at(i)` | 越界抛异常 |

---

## 5. 优先级调度：堆 + 表

| 概念 / API | 作用 |
|------------|------|
| `priority_queue` | ★ 只能高效动堆顶，不能按 id 改内部 |
| `unordered_map` 当表 | ★ 权威：在不在、最新 priority / version |
| `update_priority` | 表 `version++`，堆再塞一张新纸 |
| `pop_highest` | 弹出堆顶；version 对不上就丢（延迟删除） |
| 延迟删除 | ★ 堆不能改内部 → 旧纸弹出时再认 |

---

## 6. Lambda 与算法

| 概念 / API | 作用 |
|------------|------|
| `[捕获](参数) { 函数体 }` | ★ lambda |
| `[=]` / `[&]` | 值捕获 / 引用捕获（引用不延长寿命） |
| `std::sort` | 比较器必须严格弱序，**不要写 `<=`** |
| erase-remove | ★ `v.erase(remove_if(...), v.end())` 才真删 |
| `std::function` | 能存任意可调用对象；热路径有代价 |

---

## 7. RingBuffer

| 概念 / API | 作用 |
|------------|------|
| `head_` / `tail_` | 下一个要读（最旧）/ 下一个要写 |
| `size_` | ★ 当前有几个；用来区分空 / 满 |
| `push` | 写到 `tail_`。满了先丢掉 `head_` 上最旧的（遥测） |
| `pop(T& out)` | ★ 返回 bool，帧写进参数；空则 false。不是 `tmp = pop()` |
| 绕圈 | ★ 下标必须 `% cap_` |
| 满时覆盖 vs 阻塞 | ★ 遥测覆盖最旧；任务队列满了 `wait`、不丢 |
| `pop_latest` | 循环 pop 到空，留下最后一帧（控制要最新） |
| 背压 | 后面满了让前面等；无界 `queue` 没有背压，会 OOM |

空满不要只靠 `head_==tail_`。

---

## 8. 队列与条件变量

| 概念 / API | 作用 |
|------------|------|
| `mutex` + `lock_guard` | ★ 护共享数据；不 `wait` 时用 |
| `unique_lock` | ★ 能中途放锁，给 `cv.wait` 用 |
| `cv.wait(lk)` | 放锁睡觉；醒来已重新持锁 |
| `while (!谓词) wait` | ★ 防虚假唤醒、防别人先拿走数据；不能 `if` |
| `notify_all` | 空等和满等都可能要醒时用 |
| `shutdown` | ★ 置标志 + `notify_all`，再 `join`，再析构 |
| MPMC | 多产多消；正确性仍可一把 mutex；校验本地记、join 后再合并 |

---

## 9. 锁与死锁

| 概念 / API | 作用 |
|------------|------|
| 死锁四条件 | ★ 互斥、占有且等待、不可抢占、循环等待 |
| AB-BA | 交叉加锁 |
| 统一顺序 / `scoped_lock` | ★ 打破循环等待 |
| 争用 vs 死锁 | 争用还能结束；死锁成环结束不了 |
| `lock_guard` | 管一把；**不能单独解决两把锁交叉** |

---

## 10. atomic 与伪共享

| 概念 / API | 作用 |
|------------|------|
| 普通 `++` | 读-加-写三步，多线程会丢更新 |
| `std::atomic` | ★ 这一次加减完整；计数、开关 |
| `fetch_add` / `load` / `store` | 原子加、读、写 |
| `volatile` | ★ 只约束编译器；**不能**替代 atomic |
| 何时仍用 mutex | ★ 多字段不变量、空/满要 `wait` |
| 伪共享 | 无关变量挤同一缓存行（常 64B） |

---

## 11. 时钟、延迟、丢包

| 概念 / API | 作用 |
|------------|------|
| `system_clock` | 墙钟，可跳；给人看几点 |
| `steady_clock` | ★ 单调；测间隔、Hz、延迟、`sleep_until` |
| `sleep_until` | ★ 睡到某一时刻；少攒误差 |
| `sleep_for` | 从「现在」再睡一段 |
| 延迟 | ★ `now - frame.ts`，同一时钟；`size()` 不是延迟 |
| `gap_drop` | seq 跳号：我没见到的号 |
| `overwrite` | 环满主动丢掉的；和 gap 分开报 |
| 每秒一行日志 | 少 IO；记得更新 `last_print` |

---

## 口述要点

1. 深拷贝 vs 浅拷贝；`unique_ptr` 为何不能拷。  
2. CV 为什么必须 `while`；`wait` 配 `unique_lock`。  
3. 死锁四条件；AB-BA 用顺序或 `scoped_lock`。  
4. 环满覆盖 vs 任务队列阻塞；无界没有背压。  
5. 何时 atomic、何时 mutex；`volatile` 不能替代。  
6. 延迟和丢包：同一单调时钟相减；seq 间隙和 overwrite 分列。
7. 僵尸：子死父不 wait；fork 复制、exec 替换；perror 不在 unistd。

---

## 易错清单

| 现象 | 原因 / 正确做法 |
|------|-----------------|
| 析构 double-free | 浅拷贝。有裸指针必须深拷或 `= delete` |
| `vector` 扩容后引用脏了 | 预知规模 `reserve` |
| 拷贝 `unique_ptr` 编不过 | 要 `std::move` |
| `shared_ptr` 环泄漏 | 一侧 `weak_ptr` |
| `std::sort` 随机崩 | 比较器不要写 `<=` |
| `remove_if` 后元素还在 | 忘了 `erase` |
| CV 用 `if` wait | 空队列也会 pop；改 `while` |
| `wait` 配 `lock_guard` | 必须 `unique_lock` |
| 忘记 `notify_all` / `shutdown` | 消费者永睡 |
| `volatile` 当退出标志 | 用 `atomic` |
| 帧用墙钟、间隔用单调 | 混减无意义 |
| `size()` 当延迟 | 延迟是时间差 |
| 不更新 `last_print` | 每拍都打日志 |
| 对着僵尸 `kill -9` | 找父进程 `waitpid` |
| `fork` 后不看 `pid` | 父子会各跑一遍后半段 |
| `perror` 当 unistd | 它在 `<cstdio>` |

---

## 12. POSIX：进程与文件

| 概念 / API | 作用 |
|------------|------|
| POSIX | Unix 风格系统调用约定，不是一种语言 |
| fd | ★ 进程里的整数门票；每进程一张表；`fork` 时拷贝 |
| `open` / `read` / `write` / `close` | 文件；短读写按返回值循环 |
| `fork` | ★ 系统调用（`<unistd.h>`）；一次返回两次：子 0，父得子 PID |
| `pid_t` | 装 PID 的整数；定义在 `<sys/types.h>` |
| `exec*` | ★ 不新建进程，换成新程序；失败才返回 |
| `waitpid` | ★ 父领退出状态；不领就是僵尸 |
| `_exit` | 子进程 exec 失败用它，少跑 atexit |
| `perror` | ★ 在 `<cstdio>`，把 `errno` 打成句子 |

进程隔离地址空间和 fd；线程共享。`fork` 不是 `std::thread`。
