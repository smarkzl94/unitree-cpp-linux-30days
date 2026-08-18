# Day 02 · 智能指针（2026-08-19）

## 目标
写一个「设备句柄管理器」，用 `unique_ptr` / `shared_ptr` / `weak_ptr`，并演示解除循环引用。

## 建议文件
`Device.h` `Device.cpp` `main.cpp`

## 必须实现
- `unique_ptr` 独占一个模拟设备（open/close 打日志）
- 两个对象互相持有 `shared_ptr` 造成泄漏，再用 `weak_ptr` 修好
- 打印 use_count 变化

## 复盘
1. `unique_ptr` 为什么不能拷贝？
2. 循环引用为什么会导致泄漏？
