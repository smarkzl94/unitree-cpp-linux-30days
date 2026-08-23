# Day06 · Linux 笔记（/proc）

日期：2026-08-23  
环境说明：w64devkit，无 `/proc`（概念学习；数字无法采集）

## 今日命令

| 命令 | 作用 |
|------|------|
| `ls /proc` | 列出内核暴露的状态入口（进程目录、cpuinfo、meminfo 等），不是普通磁盘文件 |
| `cat /proc/cpuinfo` | 看 CPU 型号、逻辑核等 |
| `cat /proc/meminfo` | 看内存总量、空闲、可用等（单位常见 kB） |
| `nproc` 或 `grep -c "^processor" /proc/cpuinfo` | 数**整机逻辑 CPU 个数**，不是「当前进程的 CPU 信息」 |
| `grep -E "MemTotal\|MemAvailable\|MemFree" /proc/meminfo` | 筛出物理内存总量 / 估算还能用多少 / 完全没被占用的 |
| `ls -l /proc/self` | `/proc/self` 是软链，指向当前进程的 `/proc/<pid>` |

## `/proc` 是什么

不是普通磁盘目录。内核把**系统状态**伪装成文件；`cat` 等于当场问内核。不只有进程，还有 CPU、内存等。

## 内存该看哪个

优先 **MemAvailable**（把可回收的缓存也算进去）。MemFree 只是完全空着的，看起来会偏少。MemTotal 是物理内存总量，不是磁盘。

## 上机数字

本环境无 `/proc`，未采集。有 WSL/Linux 时补核数、型号、MemTotal / MemAvailable / MemFree。

## 口述自检

1. 不是普通磁盘目录；读文件等于查询内核状态。
2. 看 MemAvailable。
3. `nproc`，或 `grep -c "^processor" /proc/cpuinfo`。
