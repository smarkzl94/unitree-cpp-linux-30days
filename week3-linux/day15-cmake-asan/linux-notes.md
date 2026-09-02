# Day15 · Linux 笔记（源外构建）

日期：2026-09-01

源码（`.cpp` / `CMakeLists.txt`）和生成物（`*.o`、可执行文件、`CMakeCache.txt`）分开。生成物只进 `build/`。

---

## 要点

- **`cmake -S . -B build` 是配置。** `-S` 是源码根（有 `CMakeLists.txt` 的那一层），`-B` 是生成目录。读 Lists，在 `build/` 里生成 Makefile/Ninja，不是编译。
- **`cmake --build build` 才是编译链接。** 真正调 g++。只改 `.cpp` → 再 `--build`。改了 `CMakeLists.txt` → 先重新 `-S -B`，再 `--build`。
- **必须源外。** 源内 `cmake .` 会把 `CMakeCache.txt`、Makefile 洒进仓库。清干净只删 `build/`，不要 `rm -rf` 源码目录。`build/` 已在 `.gitignore`，不要 commit（cache 里有本机绝对路径）。
- **`add_executable(demo a.cpp b.cpp)`** 相当于 Makefile 里：`demo` 由这两个 `.cpp` 编出来的 `.o` 链成。Debug 用 `-DCMAKE_BUILD_TYPE=Debug`（对应手写 `-g`）。

常用命令：

- `cmake -S . -B build -DCMAKE_BUILD_TYPE=Debug`：配置（调试/ASan 要带 Debug）
- `cmake --build build`：编译
- `./build/demo`：运行（名字以 target 为准）
- `rm -rf build`：清生成物后重来
