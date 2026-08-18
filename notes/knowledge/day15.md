# Day15 · CMake、ASan、gdb

日期：2026-09-01

## 为什么学这个
不会构建与调试，算法写出来也难落地。工程岗面试会看你怎么查崩溃。

## 核心知识
1. **CMake 最小结构**：`cmake_minimum_required`、`project`、`add_executable`、`target_include_directories`。
2. **多文件**：库/`add_library` + 链接；Debug/Release。
3. **AddressSanitizer（ASan）**：检测堆越界、use-after-free、泄漏（视平台）；GCC/Clang：`-fsanitize=address -g`。
4. **gdb 基本功**：`break`、`run`、`bt`、`info locals`、`print`；多线程 `info threads`。
5. **调试信息**：`-g`；优化开太高可能难跟。

## Windows 提示
优先 **WSL + g++/clang**；MSVC 有自带 sanitizer/调试器，命令不同但思路相同。

## 面试常问
- 遇到 segfault 你怎么查？
- ASan 能抓到什么、抓不到什么？
