# Day07 · Week1 复盘要点

日期：2026-08-24

## 本周一条线
**所有权清晰 → 容器懂原理 → 定长缓冲能落地。**

## 必须能口述
1. Rule of Five 各干什么
2. `unique_ptr` / `shared_ptr` / `weak_ptr` 使用边界
3. `vector` 扩容与迭代器失效
4. RingBuffer 空满与满时策略
5. 移动语义何时发生（返回值、`std::move`、右值）

## 自检清单
- [ ] Day01 无泄漏（ASan 或逻辑计数）
- [ ] Day06 单测 ≥5 个
- [ ] 能默写 RingBuffer public API

## 常见薄弱点
说得清概念但写时漏自赋值；RingBuffer 满策略说不清「为什么选覆盖」。
