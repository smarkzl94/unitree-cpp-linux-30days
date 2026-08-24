# Day07 · Linux 笔记（week1_smoke.sh）

日期：2026-08-24  
环境：Windows + w64devkit；路径用 `D:/...`

## 今日命令

| 命令 | 作用 |
|------|------|
| `set -euo pipefail` | 失败立刻停；未定义变量报错 |
| `dirname "$0"` | 脚本自己所在目录 |
| `chmod +x week1_smoke.sh` | 加执行位 |
| `./week1_smoke.sh` | 跑冒烟 |
| `echo $?` | 上一命令退出码，0 成功 |

## 上机记录

- 脚本路径：`week1-cpp-basics/day07-review/week1_smoke.sh`
- Day01：`MyString.cpp` + `main.cpp`
- Day06：只需 `main.cpp`（`RingBuffer.h` 被 include）
- ASan：本环境未跑（笔记）

## 口述自检

1. 冒烟 vs 完整单测：冒烟只保证能编能跑；边界在各天 main 里。
2. 为什么 `set -e`：编失败还跑旧二进制会假绿。
3. Permission denied：`chmod +x` 或 `bash week1_smoke.sh`。
