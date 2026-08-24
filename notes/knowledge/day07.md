# Day07 · Week1 复盘（详细预习）

日期：2026-08-24

预习目标：先写 `week1_smoke.sh`，一键编译跑 Day01 + Day06；再把本周 C++ 串成能口述的一条线。今天不写新功能，上机先跑脚本，再对着笔记讲。

---

# 一、Linux：`week1_smoke.sh` 冒烟脚本

冒烟（smoke test）= 通不通电：不测每个边界，只确认「能编过、能跑起来、关键 assert 没炸」。Week1 收束用脚本把 Day01 的 MyString 和 Day06 的 RingBuffer 串起来，避免「某个目录只能手工敲 g++」。

## 1. 脚本要解决什么

现在每天目录里可能有 `main.cpp` 或测试文件。手工：

```bash
cd week1-cpp-basics/day01-mystring
g++ -std=c++17 -Wall -Wextra MyString.cpp main.cpp -o mystring_test
./mystring_test

cd ../day06-ringbuffer
g++ -std=c++17 -Wall -Wextra ... -o ringbuffer_test
./ringbuffer_test
```

目录一多就漏。脚本应用：**从仓库约定位置出发，编、跑、失败就停、最后打印 OK**。

## 2. 建议骨架（预习看懂，上机再敲）

```bash
#!/usr/bin/env bash
set -euo pipefail

ROOT="$(cd "$(dirname "$0")" && pwd)"
cd "$ROOT"

CXXFLAGS=(-std=c++17 -Wall -Wextra)

g++ "${CXXFLAGS[@]}" -o /tmp/day01_test \
    week1-cpp-basics/day01-mystring/MyString.cpp \
    week1-cpp-basics/day01-mystring/main.cpp
/tmp/day01_test

g++ "${CXXFLAGS[@]}" -o /tmp/day06_test \
    week1-cpp-basics/day06-ringbuffer/*.cpp
/tmp/day06_test

echo "week1 smoke: day01 + day06 OK"
```

要点：

- **shebang** + `chmod +x week1_smoke.sh`（Day04 的权限）
- **`set -e`**：g++ 或测试非 0，脚本立刻失败，不要继续报假成功
- **`cd` 到脚本所在根**：用 `dirname "$0"`，避免你在别的目录 `./week1_smoke.sh` 时相对路径全错
- 输出可执行文件放到 `/tmp` 或当天 `build/`，不要把二进制提交进 git

有 ASan 的环境（Linux g++）建议加一版：

```bash
g++ "${CXXFLAGS[@]}" -fsanitize=address -g -o /tmp/day01_asan ...
/tmp/day01_asan
```

ASan 能抓 Day01 的 double-free、越界。任务要求环境允许就跑 Day01 + Day06。WSL 未装齐时，脚本里可以先普通编译，笔记写明「ASan 未跑」。

## 3. 退出码与怎么判断成功

Linux 约定：进程退出码 **0 = 成功**，非 0 = 失败。`assert` 失败会 abort，非 0。  
脚本末尾 `echo OK` 只有前面全过才会执行（因为 `set -e`）。

手动看：

```bash
./week1_smoke.sh
echo $?          # 0 才算过
```

管道、重定向（Day03）可以用来留日志：

```bash
./week1_smoke.sh > smoke.log 2>&1
```

## 4. 易错点

1. **相对路径相对启动时的 cwd**，不是相对脚本文件——所以要先 `cd` 到 ROOT。
2. **忘记 `chmod +x`**，`./week1_smoke.sh` Permission denied；可用 `bash week1_smoke.sh` 救急。
3. **通配 `*.cpp` 没匹配到** 时，bash 可能把字面量 `*.cpp` 传给 g++。先 `ls` 确认文件名。
4. **只编了 cpp 忘了头文件路径**：本周都是同目录 `#include "MyString.h"`，一般不用 `-I`；文件挪了就要加。

## Linux 口述（预习时自己答）

1. 冒烟脚本和完整单测差在哪？  
   冒烟只保证编过、主路径能跑；完整单测覆盖空/满/自赋值等边界（那些应写在各天 main 里，脚本负责把它们跑起来）。
2. 为什么要 `set -e`？  
   编译失败还继续跑旧二进制，会报假绿。
3. `chmod +x` 之后如何执行？  
   `./week1_smoke.sh`；退出码 `$?` 为 0 表示成功。

## Linux 上机（预习不用敲）

写 `week1_smoke.sh`：自动编译并运行 Day01 + Day06 测试。有 ASan 就加一组。命令和结果记进 `linux-notes.md`。

---

# 二、C++：Week1 口述复盘

本周一句话：**所有权清晰 → 容器懂原理 → 定长缓冲能落地。**  
下面五块必须能不看代码讲完。讲的时候用自己的 MyString / DynArray / 调度器 / RingBuffer 当例子。

## 1. Rule of Five 各干什么

自己管裸资源（`new[]` 的 `char*`、自己的堆数组）时，五个要一起写或 `= delete`：

- **析构**：释放。`delete[] nullptr` 安全。
- **拷贝构造**：对象还不存在，深拷贝一份。
- **拷贝赋值**：左边已有资源；先防自赋值，再放旧，再深拷贝。空源不能 `strcpy`。
- **移动构造**：偷指针，源置空，源可析构。
- **移动赋值**：放自己的，再偷，源置空。

漏一个，编译器可能浅拷贝补上 → double-free。  
Day01 踩过的：`data_ = '\0'` 是空指针不是写字符；空串 `c_str()` 必须返回 `""`；`MyString()` 与 `MyString("")` 比内容要走 `c_str()`。

**何时必须自己写？** 类里有需要成对申请/释放的东西（裸指针、文件句柄、锁）。成员已经是 `vector` / `unique_ptr` 时，往往 Rule of Zero：一个都不用写。

## 2. 移动何时发生

- 显式 `std::move`
- 右值 / 临时量（`MyString("x")` 当参数）
- `return` 局部：通常移动或 **RVO/NRVO** 直接省略
- `vector` 扩容：元素有 `noexcept` 移动时优先 move

对照 Linux：`kill` 是「请你析构」；`kill -9` 像拔电源。移动是「移交所有权」，不是再克隆一份。

## 3. 三种智能指针边界（Day02）

- **`unique_ptr`**：独占，不能拷贝只能移动。拷贝 = 两个主人 = 双重释放。
- **`shared_ptr`**：引用计数，拷贝 +1，减到 0 才 delete。从同一裸指针构造两个 `shared_ptr` 会两个控制块。
- **`weak_ptr`**：观察不加计数；用之前 `lock()`。循环引用：互 `shared_ptr` 计数到不了 0，一侧改 `weak_ptr`。

`make_shared` / `make_unique` 优先于手写 `new`。

## 4. `vector` 扩容与迭代器失效（Day03）

- `size`：已有几个；`capacity`：这块能装几个。
- 满了再 `push_back`：新块（常 2 倍）→ 搬元素 → 释放旧块。
- **旧内存释放 ⇒ 迭代器/引用/指针全部失效。** 循环里 `push_back` 还用旧 `it` 是经典坑。
- 预知规模 → `reserve`。`emplace_back` 原地构造；`push_back` 往往先有临时对象。

## 5. 调度器：堆 + 延迟删除（Day04–05）

`priority_queue` 只能高效拿堆顶，不能改堆内任意元素。  
做法：`unordered_map` 存 id 的最新优先级/版本；更新时再 push 一条；`pop` 时丢掉过期项。  
「在不在」问 map，不问堆。比较器用 lambda；过滤/排序用算法时记得 erase-remove，捕获别悬空。

## 6. RingBuffer 空满与满时策略（Day06）

- `w==r` 单独分不清空还是满 → 牺牲一格，或另存 `size`。
- 满时：**覆盖最旧**（遥测）或 **拒绝**（指令）。必须选一种写进注释和测试。
- 定长、取模、后续 Hub 复用。至少 5 个单测：空、满、绕圈、满后再 push。

能默写 public API：`push` / `pop` / `size` / `full` / `empty`（以及容量怎么传入）。

## 7. 本周常见薄弱点（对着自己的代码问）

- 说得清深拷贝，写拷贝赋值时漏自赋值或空 `data_`。
- 会说失效，但循环里仍保存 `vector` 的引用。
- RingBuffer 能画图，说不清「为什么遥测选覆盖而不是拒绝」。
- lambda `[&]` 捕获局部再存起来。

## C++ 面试口述（预习时自己答一遍）

1. **五件套各干什么？何时必须自己写？**  
   析构释放；拷贝深拷；赋值先自赋值再换资源；移动偷指针源置空。有裸资源时必须写（或全 delete）。成员已是 RAII 则 Rule of Zero。
2. **`emplace_back` vs `push_back`？**  
   emplace 把参数转发到原地构造；push 通常先临时再放入。
3. **循环引用为何泄漏？**  
   互 `shared_ptr` 计数互锁。一侧 `weak_ptr`。
4. **扩容后引用为什么悬空？怎么少扩容？**  
   旧块释放。`reserve`。
5. **RingBuffer 空满？满了怎么办？**  
   空位或 size 计数。覆盖或拒绝，按数据能不能丢来选。

## C++ 上机（预习不用写）

见 `week1-cpp-basics/day07-review/TASK.md`：

- 不看代码默写 RingBuffer public API
- 口述并写到笔记：移动何时发生、emplace vs push、五件套何时手写
- ASan（若环境允许）跑 Day01 和 Day06；与 `week1_smoke.sh` 一起完成
