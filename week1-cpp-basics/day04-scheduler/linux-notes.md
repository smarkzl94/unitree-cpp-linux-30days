# Day04 · Linux 笔记（chmod / chown / build.sh）

日期：2026-08-21  
环境说明：w64devkit 的 sh；路径用 `D:/...`

## 今日命令

| 命令 | 作用 |
|------|------|
| `ls -l` | 看权限（最左一列） |
| `chmod +x hello.sh` | 给执行位 |
| `chmod 644` / `chmod 755` | 按数字设权限（见下一节） |
| `./hello.sh` | 当程序启动，需要 `x` |
| `sh ./hello.sh` | 交给 sh 解释，不需要 `x` |
| `chown` | 改所有者和组 |

## `chmod` vs `chown`

- `chmod`：改 rwx
- `chown`：改所有者和组

## `644` 和 `755`

`r=4`，`w=2`，`x=1`。

- `644` = `rw-r--r--`
- `755` = `rwxr-xr-x`

## 上机记录

练习文件 `hello.sh`。先 `cd D:/unitree-cpp-linux-30days/week1-cpp-basics/day04-scheduler`。  
`sh hello.sh` 找不到时，多半还在 `~`，改用 `sh ./hello.sh`。

## 口述自检

1. `chmod +x` 加执行位。不加时 `./` 不能跑，`sh ./脚本` 还能跑。
2. `644`=`rw-r--r--`；`755`=`rwxr-xr-x`。
3. `chmod` 改权限；`chown` 改所有者/组。
4. `./build.sh` 相对的是**当前工作目录**，不是脚本所在目录。
