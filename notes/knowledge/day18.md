# Day18 · 信号与优雅退出

日期：2026-09-04（建议 WSL）

## 为什么学这个
Ctrl+C 要停干净：刷日志、停线程、关 socket。真机部署很看重。

## 核心知识
1. **信号**：`SIGINT`（Ctrl+C）、`SIGTERM`（杀进程常用）、`SIGKILL`（不可捕获）。
2. **`sigaction`** 优于老式 `signal`（更可控）。
3. **异步信号安全**：handler 里只做极少事（设 `atomic` 标志）；复杂清理放主线程。
4. **优雅退出流程**：handler 置 `g_running=false` → 唤醒阻塞 → join 线程 → 关 fd → 退出。
5. **和条件变量配合**：退出时 `notify_all`。

## 易错点
- 在 handler 里 `printf`/拿锁（可能死锁）
- 只处理 SIGINT 不处理 SIGTERM
- 退出后仍有线程访问已毁对象

## 面试常问
- 为什么 handler 里只设标志？
- SIGKILL 能抓住吗？
