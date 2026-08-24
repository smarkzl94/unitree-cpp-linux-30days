# Day 01 · MyString（2026-08-18）

预计：3 小时（C++ ~100min + Linux ~70min）

## 目标

**C++**：手写简化版 `MyString`（拷贝 / 移动 / 析构）。  
**Linux**：会在终端进项目目录、看权限、用 `man`。

## 建议文件

```
day01-mystring/
  MyString.h
  MyString.cpp
  main.cpp
```

## 必须实现

- 默认构造、`const char*` 构造
- 拷贝构造、拷贝赋值
- 移动构造、移动赋值（移动后源对象安全可析构）
- 析构
- `c_str()` / `size()` / `operator==`

## 自测用例（写在 main 里 assert）

1. 默认空串
2. 从字面量构造
3. 拷贝后两边内容独立（改一边不影响另一边）
4. 移动后目标有效、源可安全析构
5. 自赋值 `s = s` 不崩溃
6. 一连串赋值不泄漏（可用 ASan 或自己计数）

## 复盘口述（结束前 20 分钟）

1. 深拷贝和浅拷贝差在哪？
2. 移动构造什么时候会被调用？`return` 局部对象时会发生什么？

## Linux 块（~70min）必做

1. 在 WSL/终端进入本目录；练习 `pwd` `ls -la` `man ls`
2. 弄清文件权限 rwx 含义
3. 写本目录 `linux-notes.md`：5 个命令 + 权限理解

## 完成后

1. 勾选 `../../PROGRESS.md` 里的 day01（C++ + Linux 都完成才勾）
2. 笔记：`notes/daily/2026-08-18.md`（或喊我同步）
