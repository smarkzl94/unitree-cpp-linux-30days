# Day11 · Linux 笔记（lscpu / 缓存行 / 伪共享）

日期：2026-08-28

Day06 的 `/proc/cpuinfo` 是细表；`lscpu` 是同一套信息的摘要。

---

## 要点

- 线程跑在 CPU 核上；核与核通过缓存交换数据。缓存按固定大小的块走，这块叫 **cache line（缓存行）**。x86 上常见 **64 字节**（`getconf LEVEL1_DCACHE_LINESIZE`）。
- `lscpu` 先看：**CPU(s)** 逻辑核（超线程会大于物理核）、**Socket / Core(s) per socket**、**Thread(s) per core**、**L1d / L1i / L2 / L3**（越小越近、越快）。
- 硬件同步的最小单位是缓存行，不是一个 C++ 变量。核多 ≠ 线性加速。
- **真共享**：两核改同一个变量，慢有理由。**伪共享**：变量不同、只是落在同一行，一个核改邻居，整行在另一核上常被作废——慢得冤枉。缓解：padding / `alignas(64)`，或各线程本地计数最后再汇总。

常用命令：`lscpu`、`lscpu | grep -i cache`、`getconf LEVEL1_DCACHE_LINESIZE`。
