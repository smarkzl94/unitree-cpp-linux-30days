# Day 09 · 多生产者多消费者（2026-08-26）

## 目标
2 生产 + 2 消费，有界队列，证明不丢任务。

## 文件
- 填：`BoundedQueue.h`
- 测：`main.cpp`（先别改测试，改到两个 `ok` 都打印）

## 必须实现
- 生产 N 个带序号的任务（测试里已拆成两个生产者区间）
- 消费端本地记录，join 后主线程合并校验
- 有界：满则生产者 `wait`，不覆盖
- 结束时所有线程 `join`；`shutdown()` 能唤醒空等的消费者

## 编译（WSL）

```bash
cd week2-concurrency/day09-mpmc
g++ -std=c++17 -Wall -Wextra -pthread main.cpp -o day09
./day09
```

未实现时场景 1 会很快失败（`wait_and_pop` 直接 `false`）或卡住。

Linux：程序跑着时 `nice -n 10 ./day09`、`renice`，NI 抄进 `linux-notes.md` 上机栏。
