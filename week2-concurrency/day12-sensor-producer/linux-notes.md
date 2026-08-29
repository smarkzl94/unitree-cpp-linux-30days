# Day12 · Linux 笔记（date / timedatectl，墙钟 vs 单调）

日期：2026-08-29

「现在几点」和「过了多少毫秒」不是同一类时间。

---

## 要点

- **墙钟**：墙上那种「几点几分」。跟日历、时区绑在一起；NTP 或手动改系统时间会让它跳。给人看、写日志用。`date`、`date -u`、`date +%s` 都是墙钟。C++ 里接近 `system_clock`。
- **`timedatectl`**：看本地时间、UTC、时区、NTP。说明墙钟归系统管，会被改。
- **单调时钟**：只往前走，不跟日历跳。测两帧间隔、100Hz 是否达标用它。终端里没有像 `date` 那样常用的命令，在程序里读：C++ `steady_clock`，或 `clock_gettime(CLOCK_MONOTONIC)`。
- 算延迟时，帧上的时间和 `now` 必须同一类。墙钟和单调不能混减。

常用命令：

- `date`：显示本地墙钟
- `date -u`：显示 UTC
- `date +%s`：Unix 秒（从 1970-01-01 起）
- `timedatectl`：系统时间/时区/NTP 状态
