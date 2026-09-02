# CMakeLists.txt 怎么写

Day15 起每天都会抄这份。命令行（`-S` / `-B` / `--build`）看当天 `linux-notes.md` 或预习；**本文件只讲 `CMakeLists.txt` 本身。**

环境默认：WSL Ubuntu + GCC/Clang，C++17。版本要求按仓库习惯写 `3.16`。

---

## 0. CMake 在干什么

你不直接给 g++ 一长串文件名。你用 `CMakeLists.txt` **声明**：

- 这个工程叫什么、用哪种语言
- 有哪些**目标**（可执行文件 / 库）
- 每个目标用哪些 `.cpp`、去哪找头文件、链哪些库、加哪些编译选项

然后：

```text
cmake -S . -B build          → 读 CMakeLists.txt，在 build/ 生成 Makefile 或 Ninja
cmake --build build        → 真正调用编译器、链接器
```

改 **`.cpp`**：只 `--build`。  
改 **`CMakeLists.txt`**（加文件、改选项、改依赖）：先重新 `-S -B`，再 `--build`。

源码是 `.cpp` / `.hpp` / `CMakeLists.txt`。生成物（`CMakeCache.txt`、`*.o`、可执行文件）只进 `build/`，不要 commit。

---

## 1. 文件从哪开始读

CMake **只认你指定的那一层**里的 `CMakeLists.txt`。

```bash
cd week3-linux/day15-cmake-asan
cmake -S . -B build
```

这里的 `.` 就是「源码根」：必须能看见一份 `CMakeLists.txt`。子目录若还有自己的 `CMakeLists.txt`，要靠顶层 `add_subdirectory(...)` 才会被读到（见第 8 节）。

推荐当天目录长这样：

```text
day15-cmake-asan/
  CMakeLists.txt      ← 配置入口
  include/foo.hpp     ← 声明
  src/foo.cpp         ← 实现
  src/main.cpp        ← 入口
  build/              ← 生成物，gitignore
```

头文件**不要**写进 `add_executable`。编译器靠 `#include` + include 路径找到。

**不要** `#include "foo.cpp"`。

---

## 2. 最小可跑的一份（先背熟）

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

下面按出现顺序讲每一句。

### 2.1 `cmake_minimum_required(VERSION 3.16)`

必须放在**最上面**（在 `project` 之前）。

低于这个版本的 CMake 直接拒绝，避免老版本把新语法猜错。`3.16` 覆盖常见 Ubuntu 自带的 CMake，也覆盖本仓库根目录那份。

### 2.2 `project(day15 LANGUAGES CXX)`

- 第一个参数：工程名（变量 `PROJECT_NAME`），不等于可执行文件名。
- `LANGUAGES CXX`：只启用 C++。今天不编 C、不编 Fortran，写上更干净。

可执行文件名来自后面的 `add_executable(名字 ...)`，不是这里的 `day15`。

### 2.3 `CMAKE_CXX_STANDARD`

```cmake
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
```

所有后面创建的目标默认按 C++17 编（智能指针、`string_view`、`optional` 都够用）。

`REQUIRED ON`：编译器不支持 17 就失败，不会偷偷降到 14。

这两行写在 `project` 之后、`add_executable` 之前。它们是**目录级默认值**，对后面在本文件（以及 `add_subdirectory` 进来的子目录）里创建的目标生效。

### 2.4 `add_executable(名字 源文件…)`

```cmake
add_executable(demo
  src/main.cpp
  src/foo.cpp
)
```

含义：编出一个叫 `demo` 的程序，把列出的 **`.cpp` 全部编译再链接在一起**。对应手写：

```bash
g++ -std=c++17 src/main.cpp src/foo.cpp -Iinclude -o demo
```

注意：

- 路径相对**这份 CMakeLists.txt 所在目录**。
- 只列 `.cpp`（以及极少见的 `.c`）。`.hpp` 不列。
- 每加一个实现文件，就要写进这个列表。漏了会 `undefined reference`。
- 同一个工程可以有多个 `add_executable`，就会编出多个程序。

编完后二进制一般在 `build/demo`（Unix Makefile 生成器）。名字以 target 为准：`add_executable(demo ...)` → 跑 `./build/demo`。

### 2.5 `target_include_directories`

```cmake
target_include_directories(demo PRIVATE include)
```

给 **target `demo`** 加 `-Iinclude`。于是 `src/main.cpp` 里写 `#include "foo.hpp"` 能找到 `include/foo.hpp`。

`PRIVATE` 的意思见第 4 节：只给 `demo` 自己用，不传给别人。

少用（今天不要用）全局的：

```cmake
include_directories(include)   # 全目录所有目标都带上，难追查
```

现代写法：**选项挂在具体 target 上**。

---

## 3. 路径：相对谁？

| 你写的路径 | 相对谁 |
|------------|--------|
| `src/main.cpp` | 当前这份 `CMakeLists.txt` 所在目录 |
| `include`（给 include_directories） | 同上，最后变成 `-I该目录/include` |

需要「无论从哪配置都对」时，用 CMake 变量：

```cmake
target_include_directories(demo PRIVATE
  ${CMAKE_CURRENT_SOURCE_DIR}/include
)
```

| 变量 | 是什么 |
|------|--------|
| `CMAKE_CURRENT_SOURCE_DIR` | **当前正在处理的**那份 CMakeLists 所在源码目录 |
| `CMAKE_SOURCE_DIR` | 最顶层 `-S` 那个源码根 |
| `CMAKE_BINARY_DIR` | 最顶层 `-B`，一般是 `build/` |
| `CMAKE_CURRENT_BINARY_DIR` | 当前子目录对应的生成目录 |

单目录小工程写 `include` 就够。子目录、顶层工程再用 `${CMAKE_CURRENT_SOURCE_DIR}`。

---

## 4. PRIVATE / PUBLIC / INTERFACE

这三个词出现在 `target_include_directories`、`target_link_libraries`、`target_compile_options` 上，意思一样：**这个属性传不传给链接我的人**。

把 target 想成积木：

- **PRIVATE**：只给这块积木自己。别人链我，不会自动带上。
- **PUBLIC**：自己用，并且链我的人也用。
- **INTERFACE**：自己没有要编的 `.cpp`（或自己不需要），但链我的人要用。头文件库常用。

### 4.1 可执行文件几乎全是 PRIVATE

`demo` 不会被别人当库来链，所以：

```cmake
target_include_directories(demo PRIVATE include)
target_link_libraries(demo PRIVATE foo_lib Threads::Threads)
target_compile_options(demo PRIVATE -Wall -Wextra)
```

### 4.2 库的头文件常用 PUBLIC

```cmake
add_library(foo_lib STATIC src/foo.cpp)
target_include_directories(foo_lib PUBLIC include)

add_executable(demo src/main.cpp)
target_link_libraries(demo PRIVATE foo_lib)
```

`foo.hpp` 在 `include/`。`foo.cpp` 编译需要它（库自己），`main.cpp` 也 `#include "foo.hpp"`（用库的人）。所以 include 用 **PUBLIC**：链上 `foo_lib` 的人自动获得 `-Iinclude`，`demo` 不必再写一遍。

若头文件只给 `.cpp` 内部用、不对外（很少见），才用 PRIVATE。

### 4.3 口诀

| 场景 | 可见性 |
|------|--------|
| 可执行文件上的选项 / 头文件 / 库 | `PRIVATE` |
| 库的**对外头文件** | `PUBLIC` |
| 纯头文件库（没有 `.cpp`） | `INTERFACE` 库 + `INTERFACE` 路径 |

---

## 5. 抽库：别把所有 `.cpp` 塞进一个 executable

文件一多，`main` 只负责串起来，实现放进库。

```cmake
add_library(foo_lib STATIC
  src/foo.cpp
)
target_include_directories(foo_lib PUBLIC include)

add_executable(demo src/main.cpp)
target_link_libraries(demo PRIVATE foo_lib)
```

### 5.1 `STATIC` / `SHARED` / `INTERFACE`

| 类型 | 产物 | 今天怎么选 |
|------|--------|------------|
| `STATIC` | `libfoo_lib.a`，链接时打进可执行文件 | 默认用这个，简单 |
| `SHARED` | `.so`，运行时再加载 | 本课先不用 |
| `INTERFACE` | 没有要编的源，只传递头文件/选项 | header-only，如只有 `RingBuffer.hpp` |

```cmake
add_library(ringbuffer INTERFACE)
target_include_directories(ringbuffer INTERFACE
  ${CMAKE_CURRENT_SOURCE_DIR}/include
)
# 使用方：
# target_link_libraries(demo PRIVATE ringbuffer)
```

`INTERFACE` 库没有 `.cpp`，所以没有 `STATIC`。链接它 = 把 include 路径（以及你挂在上面的选项）传给对方。

### 5.2 `target_link_libraries`

```cmake
target_link_libraries(demo PRIVATE foo_lib)
```

含义：链接 `demo` 时把 `foo_lib` 链进去，并按 4 节规则**继承** `foo_lib` 的 PUBLIC/INTERFACE 属性。

可以一次链多个：

```cmake
target_link_libraries(demo PRIVATE foo_lib Threads::Threads)
```

顺序一般：自己的库在前，系统库在后。现代 CMake 用 target 名（`foo_lib`、`Threads::Threads`），少写裸的 `-lpthread`。

---

## 6. 线程库：`Threads::Threads`

多线程程序必须链 pthread。不要猜 `-lpthread` 在不在。

```cmake
find_package(Threads REQUIRED)
target_link_libraries(demo PRIVATE Threads::Threads)
```

- `find_package(Threads REQUIRED)`：本机找不到线程库，**配置阶段就失败**，不要拖到链接报错。
- `Threads::Threads` 是 **pthread / 线程库**，不是「进程」。对应手写 `g++ ... -pthread`。

`ldd ./build/demo` 用来看编出来的 ELF 运行时依赖哪些 `.so`。只在 WSL 里对 Linux 二进制用，不要对 Windows `.exe` 用。

---

## 7. Debug / Release 和 ASan

### 7.1 构建类型是配置时定的

```bash
cmake -S . -B build     -DCMAKE_BUILD_TYPE=Debug
cmake -S . -B build-rel -DCMAKE_BUILD_TYPE=Release
```

可以两个生成目录并存。调试、ASan 用 Debug；看速度用 Release。

`CMAKE_BUILD_TYPE` 主要对 **Unix Makefiles** 这类「单配置生成器」有意义。配完再改类型：清掉 `build/` 或换一个 `-B` 目录，不要只改一句话指望缓存自己变对。

CMakeLists 里读它：

```cmake
if(CMAKE_BUILD_TYPE STREQUAL "Debug")
  # ...
endif()
```

没传 `-DCMAKE_BUILD_TYPE=...` 时，这个字符串经常是空的，`if` 进不去——ASan 就不会开。Debug 一定要在命令行写上。

### 7.2 AddressSanitizer（GCC/Clang）

ASan 抓：堆越界、use-after-free、部分泄漏、部分栈越界。  
不抓：纯逻辑错、数据竞争（TSan）、多数未初始化（MSan）。

**编译和链接都必须**加 `-fsanitize=address`。只加一边会链接失败或运行对不上。

```cmake
if(CMAKE_BUILD_TYPE STREQUAL "Debug")
  target_compile_options(demo PRIVATE
    -fsanitize=address
    -fno-omit-frame-pointer
    -g
  )
  target_link_options(demo PRIVATE
    -fsanitize=address
  )
endif()
```

| 选项 | 为什么 |
|------|--------|
| `-fsanitize=address` | 插桩 |
| `-g` | 调试符号，报告才有文件名+行号 |
| `-fno-omit-frame-pointer` | 栈帧完整，`bt` 更好读 |

有多个 target 时，**每个参与的翻译单元 + 最终链接**都要同一套 sanitizer。用 ASan 编的库不要和非 ASan 的可执行文件混链。

Release 不要带 ASan：又慢，地址也变了，不能当正式测速。

MSVC 不是这套 flag，本课按 WSL GCC/Clang。

### 7.3 警告（建议顺手加上）

```cmake
target_compile_options(demo PRIVATE -Wall -Wextra)
```

和 ASan 一样挂在 target 上，不要去改全局 `CMAKE_CXX_FLAGS`（难维护、子目录会互相污染）。

---

## 8. 多目录：`add_subdirectory`

仓库根上已经有一份总 `CMakeLists.txt`，用 `add_subdirectory` 把 week1 某些天挂进去。Day21 会系统收束。写法是：

**顶层：**

```cmake
cmake_minimum_required(VERSION 3.16)
project(unitree_cpp_linux_30days LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

add_subdirectory(week3-linux/day15-cmake-asan)
```

**子目录** `week3-linux/day15-cmake-asan/CMakeLists.txt`：

```cmake
add_executable(demo src/main.cpp src/foo.cpp)
target_include_directories(demo PRIVATE include)
```

子目录文件里**不要再写**一遍 `cmake_minimum_required` / `project`（已经在顶层做过）。子目录只负责自己的 target。

单独练某一天时，也可以只在当天目录 `-S .`，当天那份 CMakeLists 就要自己带 `cmake_minimum_required` 和 `project`。两种用法都合法：

- **当天目录当根**：文件要完整（第 2 节那份）。
- **仓库根当根**：当天文件只写 target，由顶层 `add_subdirectory`。

Day15 上机按「当天当根」写完整一份即可。

只编某一个 target：

```bash
cmake --build build -t demo
```

---

## 9. 今天可以直接抄的完整模板（Day15）

把这段放进 `week3-linux/day15-cmake-asan/CMakeLists.txt`，再按第 1 节放 `include/`、`src/`。

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

if(CMAKE_BUILD_TYPE STREQUAL "Debug")
  target_compile_options(demo PRIVATE
    -fsanitize=address
    -fno-omit-frame-pointer
    -g
    -Wall
    -Wextra
  )
  target_link_options(demo PRIVATE -fsanitize=address)
endif()
```

配置 + 编译 + 跑：

```bash
cmake -S . -B build -DCMAKE_BUILD_TYPE=Debug
cmake --build build
./build/demo
```

以后要抽库，把 `src/foo.cpp` 换成第 5 节的 `add_library` + `target_link_libraries`。  
以后要多线程，加上第 6 节两行。

---

## 10. 易错清单

1. **在源码目录 `cmake .`**  
   生成物洒进仓库。永远 `-B build`。

2. **头文件写进 `add_executable`**  
   没用。漏的是 `.cpp`。

3. **`#include "foo.cpp"`**  
   会重复定义。只要 `.hpp`。

4. **漏列 `.cpp`**  
   链接 `undefined reference to ...`。把新实现加进 `add_executable` 或 `add_library`。

5. **ASan 只加在 compile、忘了 link**  
   `target_link_options` 也要 `-fsanitize=address`。

6. **没传 `CMAKE_BUILD_TYPE=Debug`，ASan 的 `if` 不进**  
   配置命令里写上。

7. **改了 CMakeLists 只 `--build`**  
   选项/文件列表可能还是旧的。重新 `cmake -S . -B build`。

8. **`file(GLOB ... *.cpp)` 自动收源文件**  
   新文件有时不会触发重新配置。本课**手写源文件列表**。

9. **全局 `include_directories` / `link_libraries` / `add_definitions`**  
   改成 `target_*`。

10. **Windows 上对 `.exe` 跑 `ldd`**  
    `ldd` 看的是 ELF。到 WSL 里对 `./build/demo` 跑。

---

## 11. 和手写 g++ / Makefile 对照

| 你想做的事 | 手写 | CMakeLists |
|------------|------|------------|
| 指定源码根 / 生成目录 | 自己约定目录 | 命令行 `-S` / `-B`（不是写在 Lists 里） |
| 列出要编的 .cpp | `g++ a.cpp b.cpp` | `add_executable` / `add_library` |
| `-Iinclude` | `-Iinclude` | `target_include_directories(... PRIVATE include)` |
| `-pthread` | `-pthread` | `find_package(Threads)` + `Threads::Threads` |
| `-g` / 优化 | 自己拼 `CFLAGS` | `-DCMAKE_BUILD_TYPE=Debug` 或 `Release` |
| ASan | 编译和链接都加 flag | `target_compile_options` + `target_link_options` |
| 抽静态库 | `ar` + 再链 `.a` | `add_library(... STATIC)` + `target_link_libraries` |
| 只重编改过的文件 | make 时间戳 | `cmake --build` 同样增量 |

`add_executable(demo a.cpp b.cpp)` 在想的就是 Makefile 里：「`demo` 依赖 `a.o` 和 `b.o`，每个 `.o` 来自对应 `.cpp`」。CMake 把依赖图藏起来了。

---

## 12. 口述（写 Lists 时能答出来）

1. **`add_executable` 里为什么不写头文件？**  
   头文件不是翻译单元；编译器靠 `#include` 和 `-I` 找到。漏列的是实现 `.cpp`。

2. **`PRIVATE` 和 `PUBLIC` 差在哪？**  
   PRIVATE 只给这个 target；PUBLIC 自己用也传给链接自己的人。可执行文件用 PRIVATE；库的对外头文件用 PUBLIC。

3. **ASan 为什么 compile 和 link 都要写？**  
   插桩在编译，运行时库在链接。只加一边对不上。

4. **`Threads::Threads` 是什么？**  
   线程库（pthread），不是进程。
