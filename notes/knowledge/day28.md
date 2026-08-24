# Day28 · Linux / C++ 面试口述（详细预习）

日期：2026-09-14

预习目标：先把 Linux 题讲到 2～3 分钟/题（有命令、有现象），再把 C++ 题讲到同样密度（有例子、能回到 Hub）。上机是对着镜子或录音讲，不是写新功能。答案写进 `notes/interview-answers.md`。

---

# 一、Linux：面试口述题（各准备一组）

下面每题按「一句话结论 → 怎么工作 → 命令或对照 → 和 Hub/本课的例子 → 易错」。预习时**出声讲**，讲不满两分钟就补例子，超过三分钟就收。

## 1. 进程卡住了你怎么查？

**结论**：先身份和状态，再 syscall，再用户态。不要先改代码。

流水线：`ps` 看 PID/STAT/NLWP/RSS → `top`/`top -H` 看谁热 → `STAT` 分诊（R/S/D/Z）→ `strace -p` 看卡在 `read`/`futex`/`poll` 还是疯狂失败 → 仍像逻辑/锁再用 `gdb`，`info threads`。

Hub 例子：client 不更新时先 `ss -tnp` 有没有 ESTAB，再 strace server 是堵在 `accept` 还是 `send`。

易错：开着 strace 测性能；只看 CPU 忽略 D/Z。

## 2. `ps` 和 `top` 差在哪？Z 是什么？

**结论**：`ps` 快照，`top` 直播。Z 是僵尸：子已死，父未 `wait`，占 PID 槽。

命令：`ps -o pid,ppid,stat,cmd`；Z 找 PPID。`kill -9` 杀不掉僵尸。长期服务必须 `waitpid`。

对照：D 是还活着、常等磁盘，也杀不醒。孤儿是父死子活，不是 Z。

Day17 实验：故意不 wait 看见 Z，wait 后消失。

## 3. `kill` / `kill -TERM` / `kill -9` / Ctrl+C

**结论**：`kill` 默认 SIGTERM(15)，进程能清理；Ctrl+C 是 SIGINT(2)；`-9` 是 SIGKILL，不能捕获，内核直接撕掉。

习惯：先 TERM，等几秒再 KILL。systemd 停服务也是这个顺序。机器人乱 -9 可能留半开设备、没刷的日志。

`ss` 上看：TERM 后 LISTEN 应消失；KILL 也会消失但没用户态清理。

## 4. 源外构建：`cmake -S . -B build`

**结论**：源码和生成物分开。`-S` 源，`-B` 生成目录。改 cpp 只需 `cmake --build`；改 CMakeLists 才重新配置。

`rm -rf build` 安全重来。`ldd` 看依赖；发布用 Release，`strip` 掉符号并留 unstripped。`run.sh` 用脚本相对路径。

## 5. `ss -lntp` 和抓包

**结论**：`ss -lntp` 看谁在听（l 听、n 数字、t TCP、p 进程）。LISTEN vs ESTAB。回环先 `127.0.0.1`。

`tcpdump -i lo port …` 看字节和长度头字节序，可选。排障先 ss，怀疑协议再抓包。`bind` 失败先 ss 找占用。

## 6. 短读、strace、文件拷贝

**结论**：`read`/`write` 返回值才是实际长度。`strace -e open,read,write` 能看见最后一次短读和缓冲大小。逐字节拷会 syscall 爆炸。

`read==0` 是 EOF，不是失败。管道/TCP 同理。

## 7. IPC 怎么看见、怎么选

**结论**：System V 用 `ipcs`；POSIX shm 看 `ls /dev/shm`；匿名管道看 `/proc/PID/fd`。同机高频大块用 shm（要同步）；父子简单用 pipe；跨机必须网络。

残留的 `/dev/shm` 对象会坑下次运行。

## 8. 日志在哪、为什么轮转

**结论**：开发写文件 + `tail -f`；部署 stdout 可进 journald，`journalctl -u`。文件必须轮转，否则磁盘满。`mv` 不改变已打开 inode，进程要 reopen。

压测关 Debug。handler 里不打日志。

## Linux 口述（预习时自己答）

把上面 1、2、3、5、6 各讲一遍，录音。加分题：4、7、8。每题结尾加半句「我在 Hub/mycp/僵尸实验里见过……」。

## Linux 上机（预习不用敲）

不写新程序。对着 `notes/interview-answers.md` 写 Linux 组短答（每题 8～15 行，含命令）。用旧进程现场演示一两个（ss、Z、TERM vs KILL）。见 `week4-project/day28-interview-drill/TASK.md`。

---

# 二、C++：面试口述题（各准备一组）

同样 2～3 分钟。能画图就画。**加分：每题勾一句 Telemetry Hub。**

## 1. 智能指针

三种：`unique_ptr` 独占，不能拷只能移，析构释放；`shared_ptr` 控制块引用计数，到 0 才删；`weak_ptr` 观察不加计数，用前 `lock()`。

循环引用：互 `shared_ptr` 计数互锁 → 泄漏。一侧改 `weak_ptr`。

不要两个 `shared_ptr` 从同一裸指针构造（两个控制块，双重释放）。优先 `make_unique`/`make_shared`。`.get()` 出的裸指针不要长期存。

Hub：设备 fd、socket 可用 `unique_ptr` + 自定义删除器 `close`；latest 槽不要用共享所有权搅线程。

## 2. 移动语义

右值把资源偷走，源置空（指针 nullptr、大小 0）。比深拷快：不新分配、不拷字节。`std::move` 只是转成右值，不移动的话什么都没发生。

移动后源必须可析构、可赋值。自赋值 `a = std::move(a)` 要防。返回局部可以 NRVO/移动，不要 `return std::move(local)` 干扰优化（知道即可）。

Hub：`SensorFrame` 小，拷贝即可；RingBuffer 里大块才值得想移动。讲 Day01 `MyString` 更合适。

## 3. 死锁

四条件：互斥、占有且等待、不可抢占、循环等待。破：统一加锁顺序、`std::scoped_lock` 一次锁两把、缩小临界区、避免持锁调别人（尤其日志、send）。

`gdb`：`info threads`，每个 `bt`，看谁拿着哪把 mutex 等哪把。`strace` 见 `futex`。

Hub：snapshot 拷到栈上再 send，就是避免「持 latest 锁走网络」。Day10 应有自己的制造/修复故事，讲那个。

## 4. epoll 概念（即使项目是阻塞 socket）

阻塞：一个线程卡在一个 `recv`/`accept`，并发等于线程数。连接一多，线程爆。

IO 多路复用：一个线程盯很多 fd，谁就绪处理谁。`select`/`poll` 每次提交整表；`epoll` 内核里记一份，适合高连接。

水平触发（LT）：还能读就会通知，不容易漏。边缘触发（ET）：只在状态变化通知，必须读到 EAGAIN，难写但少唤醒。了解即可。

Hub 为什么还能阻塞：本机一个 client、演示清晰。口述：「要上多客户端再改 epoll 或每连接一线程（有上限）。」不要假装已经写了 epoll。

## 5. 进程与信号

进程 vs 线程：地址空间隔离 vs 共享。`fork`+`exec`+`wait` 拉起工具。僵尸：子死父不 wait。

信号：INT/TERM 可捕获，KILL 不能。handler 只设 `atomic`，清理回主线程（唤醒、join、flush、close、wait 子）。`sigaction` 优于 `signal`。两种终止信号都要注册。

Hub：Ctrl+C 停三线程，不留 Z，LISTEN 消失。

## 6. 加分组（能讲则讲，插入上面各题结尾）

- **短读短写 / 粘包**：TCP 无边界；定长头 + `recv_full`；length 上限防爆炸。
- **只要最新 / 满缓冲覆盖**：有界内存，网络慢丢旧遥测，P99 不堆。这是设计。
- **ASan**：抓越界、UAF、部分泄漏；不抓数据竞争。segfault：ASan → gdb `bt`。
- **条件变量**：`while (!pred) cv.wait`，防虚假唤醒；`shutdown` 必须 `notify_all`。

## 7. 易错（口述时别踩）

1. **背定义没有例子**  
   每题至少一个「我写过 / 我看见过」。
2. **把覆盖丢帧说成 TCP 丢包**  
   主动说清应用层策略。
3. **声称捕获了 SIGKILL**  
   直接零分。
4. **epoll 讲成你实现了**  
   概念题，诚实。

## C++ 面试口述（预习时自己答一遍）

TASK 要求的五题：智能指针、移动语义、死锁、epoll 概念、进程与信号。每题讲透，写到 `notes/interview-answers.md`。加分段单独一小节「Hub：满缓冲覆盖、只要最新」。

## C++ 上机（预习不用写）

见 `week4-project/day28-interview-drill/TASK.md`：

- 五题 + Linux 组写进 `notes/interview-answers.md`
- 每题 2～3 分钟出声讲一遍
- 不要新功能，除非讲的时候发现自己完全讲不清，记到 Day29 弱项
