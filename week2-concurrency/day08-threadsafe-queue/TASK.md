# Day 08 · ThreadSafeQueue（2026-08-25）

## 目标
`mutex` + `condition_variable` 实现线程安全队列。

## 文件
- 填：`ThreadSafeQueue.h`
- 测：`main.cpp`（先别改测试，改到三个 `ok` 都打印）

## 必须实现
- `push` / `wait_and_pop` / `try_pop`
- 析构或 `shutdown()` 能唤醒等待线程
- 单生产单消费跑通

## 编译（WSL）

```bash
cd week2-concurrency/day08-threadsafe-queue
g++ -std=c++17 -Wall -Wextra -pthread main.cpp -o day08
./day08
```

未实现时场景 1 会卡住（消费者永远 `wait`）。先把 `wait_and_pop` + `push` + `shutdown` 写对。

Linux：程序跑着时另开终端 `ps -eLf | grep day08`、`top -H -p <PID>`，摘要补进 `linux-notes.md` 上机记录。
