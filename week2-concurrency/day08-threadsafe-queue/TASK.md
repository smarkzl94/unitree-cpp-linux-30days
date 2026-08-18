# Day 08 · ThreadSafeQueue（2026-08-25）

## 目标
`mutex` + `condition_variable` 实现线程安全队列。

## 必须实现
- `push` / `wait_and_pop` / `try_pop`
- 析构或 `shutdown()` 能唤醒等待线程
- 单生产单消费跑通
