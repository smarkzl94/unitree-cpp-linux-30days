# Day 06 · RingBuffer（2026-08-23）

## 目标
实现定长环形缓冲 `RingBuffer<T>`，后续项目会复用。

## 必须实现
- `push` / `pop` / `size` / `full` / `empty`
- 满时策略写清：丢最旧 或 拒绝写入（选一种并注释）
- 至少 5 个单测

完成后可把头文件复制到 `common/include/` 供后面使用。
