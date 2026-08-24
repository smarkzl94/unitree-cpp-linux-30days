# Week1 知识点总复盘（Day01–Day07）

一条线：**谁拥有这块内存 → 容器怎么长 → 调度怎么挑急的 → 定长缓冲怎么绕圈。**  
Linux 穿插：会走路、会看进程、会管道、会脚本权限、会链接和磁盘、会 `/proc`、会用脚本冒烟。

---

## Day01 · MyString + 目录/权限/man

**C++**
- 堆上 `char*` 由对象负责 `new[]` / `delete[]`（RAII）。
- 五件套：析构、拷贝构造、拷贝赋值、移动构造、移动赋值。有裸资源就要自己写或 `= delete`，否则编译器浅拷贝 → double-free。
- 拷贝 = 新开一块；赋值先防 `s = s`，再放旧再拷。空串不能 `strcpy` 空指针。
- 移动 = 偷指针，源置 `nullptr`，源析构必须安全。
- `noexcept`：标在移动构造上，告诉 `vector`「扩容时可以移动，不会 throw」；没标可能退化成拷贝。

**Linux**
- `pwd` / `cd` / `ls -la`；目录没有 `x` 不能 `cd`。
- `man`：空格翻页，`q` 退出，`/词` 搜索。

**口述**：深拷贝 vs 浅拷贝；移动何时发生（`std::move`、临时量、`return`、RVO）。

---

## Day02 · 智能指针 + ps/top/kill

**C++**
- `unique_ptr`：独占，不能拷只能移。
- `shared_ptr`：引用计数；两个 `shared_ptr` 不要从同一块裸指针各构造一次。
- `weak_ptr`：观察不加计数，用前 `lock()`；打破 `shared_ptr` 环。
- 优先 `make_unique` / `make_shared`。

**Linux**
- `ps` / `top` 看进程；`kill` 请它退出，`kill -9` 强杀（像拔电源，析构可能跑不到）。

**口述**：循环引用为何泄漏；为何「拷贝 unique_ptr」编不过。

---

## Day03 · DynArray / vector + 管道/grep

**C++**
- `size` = 已有几个；`capacity` = 这块还能装几个。
- 满了再 `push_back`：新块（常 2 倍）→ 搬元素 → 释放旧块 → **迭代器/引用/指针全失效**。
- 预知规模就 `reserve`。`emplace_back` 原地构造；`push_back` 往往先有临时对象。
- `operator[]` 不查越界；`at()` 越界抛异常。

**Linux**
- `|` 把左边 stdout 接到右边 stdin。
- `grep` / `find` / `wc`；`2>` 是 stderr。

**口述**：扩容后引用为何悬空；`emplace_back` vs `push_back`。

---

## Day04 · 优先级调度 + chmod/脚本

**C++**
- `priority_queue` 只能动堆顶，不能按 id 改堆里某个元素。
- 权威在 `unordered_map`：在不在、最新 priority / version。
- 改优先级：表 `version++`，堆再 `push` 一张新纸；`pop` 时 version 对不上就丢（延迟删除）。
- 「在不在」问 map，不问 `heap_.size()`。

**Linux**
- `chmod +x`；`644` / `755`。`chmod` 改 rwx，`chown` 改所有者。
- `./script` 要执行位；`sh script` 把文件当脚本喂给 sh。
- 本机路径用 `D:/...`，没有 `/d/...`。

**口述**：堆不能改内部时怎么办；为何不能拿堆当任务个数。

---

## Day05 · Lambda / algorithm + 软链/du

**C++**
- lambda：`[捕获](参数) { 函数体 }`。值捕获是副本；引用捕获不延长寿命，挂出去会悬空。
- `std::sort` / `remove_if`：比较器必须严格弱序，不要写 `<=`。
- `remove_if` 只把要留的挤到前面，必须再 `erase`（erase-remove）。
- `std::function` 能存任意可调用对象（类型擦除），但可能堆分配、难以内联；热路径优先模板 / 直接 lambda。Day05 调度器用 `std::function` 是为了「构造时换比较器」。

**Linux**
- 硬链：同一 inode 的第二个名字；软链：存路径，目标没了就悬空。
- `tree` / `find`；`du -sh` 看目录占盘。

**口述**：值捕获 vs 引用捕获；`remove_if` 为什么还要 `erase`。

---

## Day06 · RingBuffer + /proc

**C++**
- 定长数组 + `head_`（读）+ `tail_`（写）+ 取模绕圈。
- `w==r` 分不清空满 → 牺牲一格，或另存 `size_`（本周实现用 `size_`）。
- 满时：覆盖最旧（遥测）或拒绝（指令）。本周选覆盖。
- 构造时一次分配，热路径不再 `new`。

**Linux**
- `/proc` 不是普通磁盘目录，读文件等于问内核。
- 内存看 **MemAvailable** 多于 MemFree。
- `nproc` 或 `grep -c "^processor" /proc/cpuinfo`。

**口述**：空满怎么区分；遥测为何用覆盖。

---

## Day07 · 复盘 + 冒烟脚本

- 不写新功能。默写 RingBuffer API：`push` / `pop` / `size` / `full` / `empty`。
- `week1_smoke.sh`：从仓库根编译跑 Day01 + Day06；`set -e` 失败即停；二进制不要提交。
- 本周薄弱点：拷贝赋值忘自赋值；循环里抓住 `vector` 引用后又 `push_back`；lambda `[&]` 捕获局部再存起来。

---

## 面试三问（Day07 必答）

1. **移动何时发生？**  
   `std::move`、右值/临时量、`return` 局部（常移动或 RVO）、`vector` 扩容且移动 `noexcept` 时。

2. **`emplace_back` vs `push_back`？**  
   emplace 把参数转发到容器里原地构造；push 通常先造临时对象再放进去。

3. **五件套何时手写？**  
   类里有裸资源（指针、fd、锁）必须写或全 delete。成员已是 `vector` / `unique_ptr` 则 Rule of Zero。
