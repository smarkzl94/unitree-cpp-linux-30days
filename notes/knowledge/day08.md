# Day08 · 线程安全队列

日期：2026-08-25

## 为什么学这个
传感器线程产数据、控制线程消费——最基础的并发模式。条件变量是面试高频。

## 核心知识
1. **`std::mutex`**：互斥；配合 `lock_guard` / `unique_lock` RAII 加锁。
2. **`condition_variable`**：等待某个条件成立；必须与 `unique_lock` 联用。
3. **等待公式**：`while (!ready) cv.wait(lock);` —— 用 **while** 防虚假唤醒。
4. **`notify_one` / `notify_all`**：条件变化后唤醒等待者。
5. **`ThreadSafeQueue` 接口**：`push`、`wait_and_pop`、`try_pop`、`shutdown`。
6. **shutdown**：置标志并 `notify_all`，避免析构后仍有人阻塞。

## 易错点
- 用 `if` 代替 `while` 等条件
- 忘记 notify
- 持锁做重计算导致吞吐崩

## 面试常问
- 为什么必须 while 判断？
- `lock_guard` 和 `unique_lock` 区别？
