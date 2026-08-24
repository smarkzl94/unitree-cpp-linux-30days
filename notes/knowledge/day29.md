# Day29 · 弱项最小 Demo（详细预习）

日期：2026-09-15

预习目标：先用 Linux 侧一个最小实验把最虚的命令钉死，再在 C++ 侧用 50～100 行 Demo 挖透最弱的一块（或两块）。上机写的是**新的小程序**，不是继续堆 Hub。

---

# 一、Linux：弱项最小实验

回看 Week1–3 的 `linux-notes.md` 和 Day28 录音：哪条命令你不敢当着人敲，或讲到一半要翻稿，那就是今天的 Linux 弱项。只选 **1 个**（最多 2 个），做成「能复现的最小现场」。

## 1. 怎么选

优先顺序：

1. 口述卡壳的（僵尸、TERM vs KILL、ss 列含义、短读、strace 过滤）
2. 做过但环境失败、你没补的（tcpdump 权限、perf 没有、ipcs 为空）
3. 会敲不会解释的（`cmake -S -B`、`ldd` `not found`）

不要选「从没学过的新工具」充数。今天是补洞，不是开新坑。

## 2. 每个候选做成什么样（预习时对号入座）

下面是现成实验说明书。**只做你弱的那条**，做完按「现象 → 错误做法 → 正确做法 → 一句话结论」写进当天 notes。

**A. 僵尸**

```bash
# 最小：用昨天的 fork demo 或 10 行 C
# 子 _exit(0)，父 sleep 20，不 wait
ps -o pid,ppid,stat,cmd -p 子PID
# 看到 Z
# 再跑 wait 版，Z 消失
```

结论：Z 已死待收，kill -9 无效。

**B. TERM vs KILL**

同一优雅退出程序，三次 kill。笔记必须有「TERM 有清理日志、KILL 没有」。

**C. ss 端口占用**

起 echo，`ss -lntp` 贴一行；再起一次看 `Address already in use`；用 ss 找到 PID，TERM 掉。

**D. strace 短读**

`mycp` 大文件，`strace -e read,write` 看最后一次 n < 缓冲。再故意把缓冲改 1，`strace -c` 次数爆炸（做完改回去）。

**E. POSIX shm 残留**

`ls /dev/shm`，跑 shm demo，再崩一次（`kill -9`），看对象还在。`rm` 或 `shm_unlink`。结论：崩不等于卸名字。

**F. 源外构建脏目录**

在玩具目录故意 `cmake .` 一次，看多出哪些文件；删掉。再 `-S . -B build` 对照。

**G. 轮转 inode**

程序一直 `append` 写 `t.log`，另一个终端 `mv t.log t.log.1`，看程序是否还在写旧文件、`ls` 新 `t.log` 在不在。

## 3. 记录格式（和 C++ Demo 同一套）

```text
弱项：ss 看 LISTEN
现象：bind 失败我只会换端口
错误：不知道谁占着
正确：ss -lntp | grep 端口 → kill -TERM PID
结论：先看监听表再杀自己的进程
```

## 4. 易错点（Linux 侧）

1. **选了 5 个各做 2 分钟**  
   一个都钉不透。最多两个。
2. **只抄命令不写「错误做法」**  
   面试问的是你怎么错的。
3. **用 Hub 当实验体改到半残**  
   Linux 弱项用旧小程序或 10 行 C，别拆产线。

## Linux 口述（预习时自己答）

用你选的那条，按上面四段讲 90 秒。讲完能不看稿复述结论句。

## Linux 上机（预习不用敲）

做 1 个最小实验，笔记四段式。脚本或命令清单放当天目录。见 `week4-project/day29-weakspots/TASK.md`。

---

# 二、C++：50～100 行挖透一个点

## 为什么需要

Hub 已经很大，弱项藏在里面更难看见。最小 Demo：**一个 main、一个断言或一行打印、能 g++ 直接编**。目标是「现象可复现，修法可指着代码说」。

上机：选最弱的 **2 块**（TASK 要求），各一个 Demo，放 `week4-project/day29-weakspots/`。

## 1. 怎么选弱项

翻 Week1–3 notes 和 Day28 提纲：讲不清、做过但不敢演示、ASan 报过又蒙混的。常见候选（预习先读懂，上机只实现你的两个）：

## 2. 候选 Demo 规格（每个都按这个写）

统一开头注释：

```cpp
// 现象：
// 错误做法：
// 正确做法：
// 一句话结论：
```

**A. 移动赋值自赋值**

```cpp
MyString a("hi");
a = std::move(a);   // 先释放再偷自己 → 空或崩
```

错误：没判断 `this == &other` 就 `delete[]` 再拷。正确：自赋值直接 return，或先偷到局部再换。结论：移动赋值也要保证源可析构。

**B. 条件变量用 `if` 等**

```cpp
if (!ready) cv.wait(lk);   // 虚假唤醒或通知丢失会错
```

正确：`while (!ready) cv.wait(lk);`。Demo 里用一次故意的 `notify` + 谓词仍假，或注释写清虚假唤醒是内核允许的。结论：等待公式是 while 不是 if。

**C. 粘包状态机**

一个函数 `feed`，先 `feed` 半个长度头，再 `feed` 剩余 + 两帧黏在一起，回调次数必须是 2。错误做法：每次 feed 当一帧。结论：缓冲 + 循环切帧。

**D. fork 后谁 exec**

打印「child pid=0 才 exec」；父打印 wait 退出码。错误：父子都 exec 或都不判断。结论：`pid==0` 子路，失败 `_exit`。

**E. RingBuffer 空满**

容量 N 放 N 个，再 push 走你声明的策略（覆盖或拒绝），`size`/`empty`/`full` 断言。错误：`head==tail` 既当空又当满。结论：浪费一格或另存 `count`。

**F. 两个 shared_ptr 同一裸指针**

ASan 下双重释放。正确：`make_shared` 或从同一个 shared 拷。结论：一个控制块。

**G. handler 里拿锁（对照，不要当正确代码发布）**

主线程持锁 sleep，发 SIGINT，handler 再拿同一把锁 → 可能挂死。旁边放「只改 atomic」的正确版。结论：handler 只设标志。

## 3. 怎么写才算「挖透」

```text
1. 先让错误版跑出你描述的现象（崩、挂、断言失败、打印错次数）
2. 再改对，同一组输入通过
3. 不要上 CMake 巨工程；g++ -std=c++17 -g -fsanitize=address 一行即可
4. 对着 Day28 提纲，用这个 Demo 当例子再讲一遍
```

两个 Demo 主题不要太近（不要两个都是智能指针）。建议：一个所有权/容器，一个并发或系统（fork/信号/粘包）。

## 4. 易错点

1. **把 Hub 剪一坨过来当 Demo**  
   依赖一堆，弱项看不清。
2. **只写正确版**  
   没有现象，面试无法「演示错误」。
3. **修法靠注释，代码仍错**  
   断言必须绿。
4. **超过 150 行还在加功能**  
   砍。

## C++ 面试口述（预习时自己答一遍）

对每个 Demo 讲 90 秒：我过去错在哪，屏幕上会看见什么，改哪几行，结论句。然后接回 Day28 对应大题。

## C++ 上机（预习不用写）

见 `week4-project/day29-weakspots/TASK.md`：

- 最弱 2 块，各一个最小可运行 Demo，放本目录
- 四段式注释 + 当天 notes
- 用 Demo 把 Day28 相关题再讲一遍
