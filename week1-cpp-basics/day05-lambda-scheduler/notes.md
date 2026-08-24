# Day05 · 与 Day04 对比

- Day04 调度器大约 47 行（`Scheduler.cpp`）；今天大约 50 行。核心增删逻辑没变，多出来的主要是 `snapshot_sorted`。
- 可读性：比较器可以在构造时用 lambda 换大顶/小顶，不必再写一个 `operator<`。快照用 `std::sort` + lambda，一眼能看出「按 priority 从大到小」，比手写选择排序短、也不容易写错边界。
