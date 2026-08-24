# Day15 · CMake 源外构建与 ASan（详细预习）

日期：2026-09-01

预习目标：先学 Linux 上 `cmake -S -B` 源外构建和 `build/` 目录规范，再学多文件 CMake 与 AddressSanitizer。上机先练命令，再写工程。

---

# 一、Linux：源外构建（out-of-source）

程序不是「一个 `.cpp` 敲 `g++` 就完」。工程一大，就要把**源码**和**生成物**分开：`.cpp` / `CMakeLists.txt` 是源；`*.o`、可执行文件、`CMakeCache.txt` 是生成物。生成物进 `build/`（或别的空目录），源码树保持干净，这叫**源外构建**。

对比：源内构建会把一堆中间文件洒在仓库里，`git status` 全是垃圾，清一次还容易误删源码。

## 1. 为什么是 CMake，不是永远手写 g++

Day04 的 `build.sh` 能编一个小程序。多文件、多目录、要 Debug/Release、要开 ASan 时，手写命令会炸：

```bash
# 还能忍
g++ -std=c++17 -g main.cpp foo.cpp -Iinclude -o app

# 文件再多、还要库、还要换编译选项——脚本会变成一锅粥
```

CMake 做两件事：

1. 你用 `CMakeLists.txt` **声明**「有哪些目标、依赖谁、要什么选项」
2. CMake **生成**底层构建文件（Linux 上常见是 Makefile 或 Ninja），再真正编译

你平时敲的是：

```bash
cmake -S . -B build          # 配置：读 CMakeLists.txt，在 build/ 里生成构建系统
cmake --build build          # 编译：进 build/ 真正 g++ / 链接
```

`-S` = source（源码根，里面有 `CMakeLists.txt`）。  
`-B` = build（生成目录，可以不存在，CMake 会建）。

## 2. 目录规范：仓库里永远有一个 `build/`

推荐长这样：

```text
day15-cmake-asan/
  CMakeLists.txt
  include/foo.hpp
  src/foo.cpp
  src/main.cpp
  build/            ← 生成物，不要 commit（.gitignore）
```

第一次配置 + 编译：

```bash
cd week3-linux/day15-cmake-asan
cmake -S . -B build
cmake --build build
./build/demo          # 可执行文件名以你 add_executable 为准
```

改了 `.cpp` 再编，多数情况只需：

```bash
cmake --build build
```

改了 `CMakeLists.txt`（加文件、改选项）才需要重新 `cmake -S . -B build`。拿不准就两个都跑，无害。

清干净重来（源外所以安全）：

```bash
rm -rf build
cmake -S . -B build && cmake --build build
```

**不要** `rm -rf` 源码目录。只删 `build/`。

## 3. 和 Makefile 思路怎么对应（建立直觉）

手写 Makefile 时你在想：哪个 `.o` 依赖哪个 `.cpp`，`app` 依赖哪些 `.o`。  
CMake 把这些藏起来：`add_executable(demo src/main.cpp src/foo.cpp)` 就表示「这两个源编完链成 `demo`」。

| 你想做的事 | 手写 | CMake |
|---|---|---|
| 指定源码根 / 生成目录 | 自己约定 `OBJDIR=` | `-S` / `-B` |
| 加一个 .cpp | 改规则、改依赖 | 写进 `add_executable` / `add_library` |
| Debug 带 `-g` | 自己拼 `CFLAGS` | `-DCMAKE_BUILD_TYPE=Debug` |
| 只重编改过的文件 | make 的时间戳 | `cmake --build` 同样增量 |

今天只要会：**配置一次，以后增量编；生成物全在 `build/`。**

## 4. Debug / Release 是配置时定的

```bash
cmake -S . -B build -DCMAKE_BUILD_TYPE=Debug
cmake --build build

cmake -S . -B build-rel -DCMAKE_BUILD_TYPE=Release
cmake --build build-rel
```

可以开两个生成目录，互不影响。调试、ASan 用 Debug；看速度用 Release。  
**注意**：`CMAKE_BUILD_TYPE` 主要对单配置生成器（Unix Makefiles）有意义。配完再改类型，最好清 `build/` 或换目录。

## 5. 易错点（Linux 侧）

1. **在源码目录里直接 `cmake .`**  
   会把 `CMakeCache.txt`、`Makefile` 洒进仓库。养成肌肉记忆：永远 `-B build`。
2. **把 `build/` 提交进 git**  
   `.gitignore` 写上 `build/`。别人机器的绝对路径会写进 cache，拉下来必炸。
3. **编译失败却去改错目录**  
   可执行文件在 `build/` 下，源码在外面。报错行号对着源码改，不要在 `build/` 里改生成文件。
4. **Windows / MSVC**  
   本课命令按 WSL Ubuntu + GCC/Clang。若在 PowerShell 里跑原生 VS，生成器不同，但「源外、`-S`/`-B`」思想一样。

## Linux 口述（预习时自己答）

1. **`cmake -S . -B build` 和 `cmake --build build` 各干什么？**  
   前者配置：读 `CMakeLists.txt`，在 `build/` 生成 Makefile/Ninja。后者编译：真正调用编译器链接。
2. **为什么必须源外构建？**  
   生成物和源码分开，仓库干净，删 `build/` 就能重来，不会误删源码。
3. **改了一个 `.cpp` 要不要重新 cmake？**  
   不用，`cmake --build build` 即可。改 `CMakeLists.txt` 才要重新配置。

## Linux 上机（预习不用敲）

在当天目录用 `-S`/`-B` 配出 `build/`，编一次，再 `rm -rf build` 重来一次。对比「源内 `cmake .`」有多脏（做完立刻清掉）。写入 `linux-notes.md`。

---

# 二、C++：多文件 CMake + ASan

## 为什么需要

不会构建与调试，算法写出来也难落地。工程岗面试会看你怎么查崩溃：先复现、再 sanitizer、再 gdb。今天把「多文件工程 + Debug 开 ASan」做成以后每天都能抄的基线。

上机任务：一个可复用的多文件 CMake，Debug 下开 ASan；故意造一次崩溃，用 gdb 打出回溯。

## 1. 最小 `CMakeLists.txt` 在说什么

```cmake
cmake_minimum_required(VERSION 3.16)
project(day15 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

add_executable(demo
  src/main.cpp
  src/foo.cpp
)
target_include_directories(demo PRIVATE include)
```

逐行：

- `cmake_minimum_required`：低于这个版本直接拒绝，避免老 CMake 乱猜语法。
- `project`：工程名；`LANGUAGES CXX` 表示只启用 C++。
- `CMAKE_CXX_STANDARD 17`：所有目标默认 C++17（智能指针、`string_view` 都够用）。
- `add_executable`：列出**所有要编进这个程序的 .cpp**。头文件不用写在这里（编译器靠 `#include` + include 路径找到）。
- `target_include_directories(... PRIVATE include)`：只有 `demo` 编译时加 `-Iinclude`。`PRIVATE` = 不向外传递。以后做库再谈 `PUBLIC`。

头文件放 `include/`，实现放 `src/`，`main` 只负责串起来。这是后面 week3/week4 每个目录的默认长相。

## 2. 多文件：库 + 可执行文件

文件一多，不要全塞进一个 `add_executable`。抽库：

```cmake
add_library(foo_lib STATIC
  src/foo.cpp
)
target_include_directories(foo_lib PUBLIC include)

add_executable(demo src/main.cpp)
target_link_libraries(demo PRIVATE foo_lib)
```

- `STATIC`：打成 `libfoo_lib.a`，链接时打进可执行文件（简单，今天够用）。
- 库的 include 用 `PUBLIC`：谁链接这个库，谁自动带上 `include/`。
- `demo` 只写 `main.cpp`，实现细节在库里。

对照：Day01 的 `MyString.cpp` + `MyString.hpp` 以后就可以变成一个 `add_library`。

## 3. AddressSanitizer：让错误立刻炸在眼前

ASan（AddressSanitizer）在编译和运行时插桩，专门抓：

- 堆越界（`buf[n]` 写成 `buf[n+1]`）
- use-after-free（`delete` 之后还用）
- 部分泄漏（进程退出时还占着的堆，视平台/选项）
- 部分栈越界

**抓不到**：逻辑算错、数据竞争（那是 TSan）、未初始化读（那是 MSan，Linux Clang 更常见）。

GCC/Clang（WSL）：

```cmake
# 只在 Debug 开，Release 测速不要带 ASan（又慢又改地址）
if(CMAKE_BUILD_TYPE STREQUAL "Debug")
  target_compile_options(demo PRIVATE -fsanitize=address -fno-omit-frame-pointer -g)
  target_link_options(demo PRIVATE -fsanitize=address)
endif()
```

必须**编译和链接都加** `-fsanitize=address`。只加一边会链接失败或运行对不上。

`-g`：带调试符号，ASan 才能打印文件名+行号。  
`-fno-omit-frame-pointer`：栈帧完整，回溯更好读。

跑起来：

```bash
cmake -S . -B build -DCMAKE_BUILD_TYPE=Debug
cmake --build build
./build/demo
```

越界时终端会红字：`heap-buffer-overflow`、栈、分配点、释放点。**先读 ASan 报告，再开 gdb。** 很多「神秘崩溃」ASan 比 gdb 先告诉你真相。

故意造一个（上机用，理解后删掉）：

```cpp
int* p = new int[2];
p[2] = 1;          // 越界：合法下标 0,1
delete[] p;
```

## 4. gdb 基本功（ASan 之后仍要会）

ASan 告诉你「哪类内存错」；gdb 告诉你「当时局部变量是什么、怎么走到这的」。

```bash
gdb ./build/demo
(gdb) break foo.cpp:12
(gdb) run
(gdb) bt              # backtrace：谁调用谁
(gdb) info locals
(gdb) print x
(gdb) info threads    # 多线程时看每个线程栈
(gdb) thread 2
(gdb) bt
```

segfault 没开 ASan 时：直接 `gdb --args ./build/demo`，崩了 `bt`。  
优化开太高（`-O2`）变量可能被优化没了，跟不动——所以调试用 Debug。

和 Day10 的死锁调试是同一套：`info threads` + 每个线程 `bt`。

## 5. 易错点（看懂再上机）

1. **ASan 只加在某一个 .cpp**  
   所有参与的翻译单元 + 最终链接都要同一套 sanitizer，否则漏报或链接错。
2. **用 ASan 编的库，和非 ASan 的可执行文件混链**  
   符号对不上。今天一个工程里统一开。
3. **看到崩溃先猜逻辑，不看报告**  
   ASan 已经写了分配点、释放点、出错点。按报告走。
4. **两个 `shared_ptr` 从同一裸指针构造**（复习 Day02）  
   ASan 能抓双重释放；根因仍是所有权。

## C++ 面试口述（预习时自己答一遍）

1. **遇到 segfault 你怎么查？**  
   先用 Debug + `-g` 复现；能开 ASan 就开，看是越界、UAF 还是泄漏。再 gdb：`bt` 看调用栈，`info locals` / `print` 看值。多线程再 `info threads`。不要一上来就猜。
2. **ASan 能抓到什么、抓不到什么？**  
   能：堆/部分栈越界、use-after-free、部分泄漏。不能：纯逻辑错、数据竞争（TSan）、多数未初始化（MSan）。
3. **为什么 Debug 才开 ASan？**  
   插桩让程序变慢、地址变化，不适合当正式测速；Release 要看真实性能。

## C++ 上机（预习不用写）

见 `week3-linux/day15-cmake-asan/TASK.md`：

- 多文件 `CMakeLists.txt`（可执行文件 + include）
- Debug 下 GCC/Clang 开 ASan（MSVC 则用对应检查器）
- 故意制造一次崩溃，gdb 打出回溯
