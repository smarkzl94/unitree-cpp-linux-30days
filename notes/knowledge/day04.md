# Day04 · 优先级调度与脚本权限（详细预习）

日期：2026-08-21

预习目标：先学会 `chmod` / `chown`，并看懂「写 `build.sh` + 加执行权限」；再学 `priority_queue`、延迟删除和 `unordered_map`。上机先练命令和脚本，再写调度器。

---

# 一、Linux：chmod / chown 与可执行脚本

Day01 会**读**权限串。今天要会**改**权限，并写一个一键编译的 `build.sh`。机器人板上大量工具是脚本，没有 `x` 就跑不起来。

## 1. `chmod`：改权限

```bash
chmod 644 build.sh       # rw-r--r--  源码/普通文件
chmod 755 build.sh       # rwxr-xr-x  主人可执行
chmod +x build.sh        # 给「该有 x 的人」加上执行位（最常用）
chmod u+x build.sh       # 只给 owner 加 x
chmod go-w file.txt      # 去掉 group/others 的写
```

数字：`r=4`、`w=2`、`x=1`。三组（owner/group/others）各自相加。

- `644` = 6(`rw-`) 4(`r--`) 4(`r--`)
- `755` = 7(`rwx`) 5(`r-x`) 5(`r-x`)
- `700` = 只有主人能读写执行

符号写法：`u` 主人、`g` 组、`o` 其他人、`a` 全部；`+` 加、`-` 减、`=` 设成。

**目录**也用 `chmod`。目录 `755` 很常见：别人能 `cd` 和 `ls`，不能在里面乱删你的文件（缺 `w`）。

## 2. `chown`：改所有者和组

```bash
ls -l build.sh           # 先看第三、四列：user  group
chown user:group file    # 一般要 root
sudo chown $USER:$USER build.sh
```

预习阶段把概念搞清即可：文件「归谁」和「权限位」是两件事。你不是 owner、也没有写权限，就改不了内容。WSL 里从 Windows 盘拷过来的文件，有时 owner 很怪，编译没问题但 `chmod` 表现和原生 Linux 不完全一样——记在笔记里。

没有 sudo 改不了别人的文件，这是正常的。

## 3. 写 `build.sh` 并 `chmod +x`

脚本就是一串命令的文本文件。第一行 shebang 告诉系统用谁来跑：

```bash
#!/usr/bin/env bash
set -euo pipefail          # 出错就停，避免编译失败还继续跑

g++ -std=c++17 -Wall -Wextra -o scheduler \
    Scheduler.cpp main.cpp

echo "build ok: ./scheduler"
```

然后：

```bash
chmod +x build.sh
./build.sh                 # 当前目录下当程序跑
```

没有 `+x` 时：`./build.sh` 会 `Permission denied`。仍可 `bash build.sh`（把文件当参数交给 bash），但习惯是加执行位，用 `./`。

路径：脚本里的相对路径相对**你启动时的当前目录**，不一定相对脚本所在目录。所以要么先 `cd` 到当天目录再 `./build.sh`，要么脚本里自己 `cd "$(dirname "$0")"`。

`set -e`：任一命令失败（非 0 退出）就退出。编译挂了不会假装成功。

## Linux 口述（预习时自己答）

1. `chmod +x` 干什么？不加能不能跑？  
   给执行位。不加则 `./script` 失败，但仍可用 `bash script`。
2. `644` 和 `755` 各长什么样？  
   `644` = `rw-r--r--`；`755` = `rwxr-xr-x`。
3. `chmod` 和 `chown` 差在哪？  
   `chmod` 改 rwx；`chown` 改所有者和组。

## Linux 上机（预习不用敲）

在 `week1-cpp-basics/day04-scheduler/` 写 `build.sh`，一键 `g++` 编当天程序，`chmod +x` 后 `./build.sh`。权限变化写进 `linux-notes.md`。

---

# 二、C++：优先级调度器

## 为什么学这个

任务/事件按优先级处理在机器人软件里常见：告警 > 控制 > 日志。你需要「永远先拿当前最紧急的那个」，还要能按 id 改优先级、查在不在。

标准库没有「可改堆内任意元素优先级」的现成堆。今天用 `priority_queue` + `unordered_map` + **延迟删除** 拼出来。

## 1. `priority_queue`：只能高效拿堆顶

```cpp
#include <queue>
std::priority_queue<int> pq;     // 默认大顶堆：top 是最大的
pq.push(3);
pq.push(10);
pq.push(7);
pq.top();    // 10
pq.pop();    // 去掉 10
```

底层通常是 `vector` + heap 算法（`push_heap` / `pop_heap`）。入堆、出堆 **O(log n)**，看堆顶 **O(1)**。

它**不能**：

- 按 id 找到堆里那个元素再改优先级（没有这种接口，硬改会破坏堆序）
- 遍历「第 k 大」以外的任意元素还保持高效

比较器决定大小顶：

```cpp
std::priority_queue<int, std::vector<int>, std::greater<int>> min_pq;  // 小顶
```

自定义类型要提供 `operator<` 或传入 Compare。lambda 当比较器时，类型往往要写成 `decltype(cmp)`，上机对照编译器报错改即可。

## 2. 延迟删除（lazy delete）：堆不能改内部，就让旧的失效

需求：任务 id=7 原来优先级 10，现在改成 100。堆里那条旧记录改不了。

做法：堆里允许脏数据；真正弹出时再问「这还是最新的吗？」

```text
push(id=7, pri=10, ver=1)
update(id=7, pri=100)  →  map[7] 改成 100 / ver=2
                         再 push 一条 (7, 100, ver=2) 进堆
                         旧的 (7, 10, ver=1) 还躺在堆里

pop 时：看堆顶 id 的版本是不是 map 里那份
  不是 → 扔掉，继续 pop（延迟删除）
  是   → 这才是有效任务
```

要点：

- `unordered_map<id, 最新信息>`：查「在不在」、拿最新优先级，平均 O(1)
- 堆里可能有同一 id 的多个条目，只有最新版本算数
- `pop` 必须循环跳过过期项，不能拿了堆顶就信

改优先级、删除任务都可以走这套：删除 = 在 map 里标记作废 / 擦掉，堆里的残骸以后 pop 到再丢。

## 3. `unordered_map`：平均 O(1) 的索引

```cpp
#include <unordered_map>
std::unordered_map<int, int> best;   // id -> 当前优先级
best[7] = 100;
if (best.count(7)) { /* 还在 */ }
best.erase(7);
```

哈希表：用哈希函数把 key 映射到桶。平均查找 O(1)，最坏（全挤一个桶）O(n)。key 必须可哈希；自定义类型要提供哈希和 `==`。

和 `std::map`（红黑树、有序、O(log n)）怎么选：

- 只要按 id 查、不需要排序遍历 → `unordered_map`
- 需要有序、范围查询 → `map`

调度器的「按 id 查最新优先级」正是哈希表场景。

**不要**把「会变的对象」当 key 却在插入后改它的哈希字段：改完就找不回去了。id 用 `int` / `string` 这种稳定值。

## 4. 调度器最小模型

上机接口大致是：

- 添加任务 `(id, priority, payload)`
- 取出当前最高优先级（注意先清脏项）
- 按 id 改优先级（写 map + 再 push 一条）
- 按 id 查询是否存在（只问 map，不问堆）

「是否存在」一定查 map：堆里可能还有已经过期的副本。

## 5. 易错点（看懂再上机）

1. **直接改 `priority_queue` 内部**  
   没有合法接口。改底层 `c` 会破坏堆性质。
2. **同一 id 多次更新却不跳过脏项**  
   堆顶可能是旧的低优先级，调度会错。
3. **用堆的 size 当「还有几个任务」**  
   堆里含过期项。有效个数应以 map 为准。
4. **比较器写反**  
   大顶/小顶搞反，最高优先级永远出不来。先写几个数字 assert `top`。
5. **key 不稳定**  
   见上一节。id 用整数即可。

## C++ 面试口述（预习时自己答一遍）

1. **堆能 O(log n) 改任意元素吗？不能的话怎么做？**  
   标准 `priority_queue` 不能。延迟删除：map 存最新，堆里留脏数据，弹出时丢弃过期项。
2. **`map` vs `unordered_map` 如何选？**  
   要有序/范围 → `map`；只要按 key 查 → 哈希表平均更快。
3. **为什么「在不在」不能问堆？**  
   堆里可能有过期副本；权威数据在 map。

## C++ 上机（预习不用写）

见 `week1-cpp-basics/day04-scheduler/TASK.md`：

- 添加 / 取最高优先 / 按 id 改优先级（允许延迟删除）/ 按 id 查询
- 用 `build.sh` 编过再跑
