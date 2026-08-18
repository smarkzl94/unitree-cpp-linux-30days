# Day 15 · CMake + ASan + gdb（2026-09-01）

## 目标
建立可复用的构建与调试基线。

## 必须实现
- 多文件 `CMakeLists.txt`
- Debug 下开启 ASan（GCC/Clang；MSVC 则用对应检查器）
- 故意制造一次崩溃，gdb/调试器打出回溯
