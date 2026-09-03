# Day17 · Linux 笔记（僵尸进程与 ps 状态）

日期：2026-09-03

子退出后内核还留着退出码，等父用 `wait`/`waitpid` 来领。不领就是僵尸（Z）：几乎不占内存，但占一个 PID。

这些命令是 Linux 的 `ps`。Windows PowerShell 里的 `ps` 不是同一套。

---

## 要点

- **`ps aux` 看 STAT 第一字母。** `R` 可跑；`S` 在睡（还活着）；`Z` 已退出、父还没 wait。`Z` 不是卡死还在跑。
- **只看一个进程：** `ps -o pid,ppid,stat,cmd -p <PID>`。`PID` 换成数字。PPID 是爸爸，该 `wait` 的是爸爸。
- **收尸只能爸爸在代码里 `waitpid`。** 你在另一个终端敲 `wait` 管不到别人的孩子。
- **`kill -9` 僵尸没用。** `kill -9` 父进程后，Z 常被 PID 1 收养并收掉，那是系统托底，不算程序写对。
- **僵尸 vs 孤儿：** 僵尸 = 子已死、父还活着但不收。孤儿 = 父先死、子还活着，过继给 PID 1。

常用命令：

- `ps aux`
- `ps -o pid,ppid,stat,cmd -p <PID>`
- `kill -9 <父PID>`：演示用；长期服务必须父自己 wait
