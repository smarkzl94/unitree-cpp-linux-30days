# Day05 · Lambda、算法与链接（详细预习）

日期：2026-08-22

预习目标：先分清软链接/硬链接，会用 `tree` / `du` 看目录体积；再用 lambda 捕获和 `<algorithm>` 重写调度器里的策略。上机先整理 week1 目录，再改 C++。

---

# 一、Linux：软链接、硬链接、tree / du

源码树会长大。今天要会「看目录长什么样、哪块占空间」，以及两种「快捷方式」——链接。编译产物、头文件复用后面都会碰到链接。

## 1. 硬链接：同一个 inode 的第二个名字

Linux 文件真正的身份是 **inode**（编号 + 元数据 + 数据块）。目录项只是「名字 → inode」。

```bash
ln original.txt hard.txt     # 硬链接：两个名字指向同一 inode
ls -li original.txt hard.txt # -i 看 inode，两行数字应相同
```

- 改其中一个，另一个内容一起变（本来就是同一份数据）
- 删掉一个名字，只要还有硬链接，数据还在
- **不能**对目录做普通硬链接（防止环）；**不能**跨文件系统

`ls -l` 第二列的链接计数，对普通文件就是硬链接个数。

## 2. 软链接（符号链接）：存的是路径字符串

```bash
ln -s original.txt soft.txt
ls -l soft.txt               # 第一位是 l，箭头指向目标
cat soft.txt                 # 跟着路径去读原文件
```

软链接是一个小文件，里面写着「去哪找」。

- 原文件删了，软链接还在，但变成**悬空**（dangling），读会失败
- **可以**链目录，**可以**跨文件系统
- 权限串经常是 `lrwxrwxrwx`：真正管权限的是目标文件

和 Windows 快捷方式更像的是软链接，不是硬链接。

对照：

| | 硬链接 | 软链接 |
|--|--------|--------|
| 命令 | `ln a b` | `ln -s a b` |
| 存什么 | 同一个 inode | 一条路径 |
| 删原文件 | 另一个名字仍可用 | 悬空 |
| 跨分区 | 否 | 可以 |
| 目录 | 一般不行 | 可以 |

预习时自己建一个文件，做硬链和软链，删原文件看两种后果，写进笔记。

## 3. `tree`：把目录画成树

```bash
tree
tree -L 2                    # 只展开两层
tree -d                      # 只看目录
tree week1-cpp-basics
```

没有 `tree` 就 `sudo apt install tree`，或暂时用 `find . -maxdepth 2`。  
今天用来整理 week1：每个 day 目录该有源码、`TASK.md`、`linux-notes.md`，一眼看到缺什么。

## 4. `du`：磁盘用量

```bash
du -h .                      # 每个子目录多大（human：K/M/G）
du -sh .                     # 当前目录合计
du -h --max-depth=1          # 只看下一层
```

`ls -l` 的文件大小是内容字节；`du` 看的是**占了多少磁盘块**（含目录本身）。空目录也不是 0。

编译出的 `a.out`、没删的目标文件会把目录撑大。整理 week1 时用 `du -sh */` 看哪天最肥。

## Linux 口述（预习时自己答）

1. 软链接和硬链接差在哪？删原文件会怎样？  
   硬链是同一 inode，删一个名字数据还在；软链是路径，原文件没了就悬空。
2. `tree` 和 `ls -R` 谁更适合看结构？  
   `tree` 更直观；没有 tree 时用 `find` / `ls -R`。
3. `du -sh` 干什么？  
   看这个目录一共占多少盘。

## Linux 上机（预习不用敲）

在 week1 目录练 `ln` / `ln -s`、`tree`、`du -sh`，整理缺失的 `linux-notes.md`，实验写进当天笔记。

---

# 二、C++：Lambda 与 `<algorithm>`

## 为什么学这个

Day04 的调度器用比较器决定谁先出队；过滤「过期任务」、排序展示，也可以写成策略。现代 C++ 用 **lambda** 把策略写在调用点旁边，用算法库少手写容易写错的循环。

上机：用 lambda 与 `<algorithm>` 重写 Day04 核心逻辑，行为保持一致，对比行数和可读性。

## 1. Lambda 长什么样

```cpp
auto cmp = [](const Task& a, const Task& b) {
    return a.priority < b.priority;
};
```

完整形态：

```text
[capture](params) -> ret { body }
```

- **capture**：从外面抓哪些变量进 lambda
- **params**：调用时的参数，和普通函数一样
- **`-> ret`**：返回类型，能推断时常省略
- lambda 是一个**匿名函数对象**（编译器生成一个独特的 class，`operator()` 里是你的 body）

可直接传给 `sort`、`priority_queue` 的比较器、`find_if` 的谓词。

## 2. 捕获：值、引用、具名、this

```cpp
int thresh = 10;
std::vector<int> out;

auto by_value = [=](int x) { return x > thresh; };   // 拷一份 thresh
auto by_ref   = [&](int x) { return x > thresh; };   // 用外面那个 thresh
auto named    = [thresh](int x) { return x > thresh; };
auto mixed    = [=, &out] { out.push_back(thresh); };  // thresh 拷贝，out 引用
```

`mixed` 没有参数列表，调用写成 `mixed()`。默认捕获（`=` 或 `&`）必须写在最前，例外跟在后面：`[=, &out]`、`[&, thresh]`。

| 写法 | 含义 |
|------|------|
| `[=]` | 用到的自动变量按值捕获 |
| `[&]` | 用到的按引用捕获 |
| `[x]` / `[&x]` | 只捕获 x |
| `[this]` | 捕获当前对象指针，可在 lambda 里用成员 |
| `[*this]` | C++17：拷一份当前对象（少用但要知道） |

**值捕获**：lambda 里是副本，外面后改不影响（除非 `mutable`）。副本的寿命跟着 lambda 对象。  
**引用捕获**：不延长寿命。lambda 若活过局部变量（存进 `std::function`、当回调挂出去），引用就**悬空**。

岗上回调、滤波策略最容易踩这个：局部 `config` 按 `&` 捕获，函数返回后回调还在跑。

```cpp
std::function<void()> bad() {
    int local = 1;
    return [&] { return local; };  // 悬空
}
```

（`std::function` 是什么见第 4 节。这里先把它当成「能把 lambda 存起来、带出函数」的盒子就行 —— 正因为能带出去，才有机会活过 `local`。）

能值捕获就值捕获；引用只留给「确定 lambda 活得比变量短」的情况（算法调用期间）。

## 3. 谓词与常用算法

**谓词**（predicate）是个术语，意思很朴素：一个返回 `bool` 的可调用对象，用来回答「这个元素算不算数」。算法负责走遍容器，谓词负责判断，两边分工。

标准算法都接收**迭代器区间** `[first, last)`，而不是容器本身。所以基本都写成 `v.begin(), v.end()` —— 这样也能只处理一部分。

```cpp
#include <algorithm>
#include <numeric>
```

### 3.1 `sort`：按你的规则排

```cpp
std::sort(v.begin(), v.end(), [](const Task& a, const Task& b) {
    return a.pri > b.pri;      // 返回 true 表示 a 应该排在 b 前面
});
```

比较器的语义是「a 是否应排在 b 之前」。写 `>` 得到从大到小，写 `<` 得到从小到大。

两条约束：

- **要随机访问迭代器**。`vector` / `deque` / `array` 可以；`list` 不行，用它自己的 `list::sort()`（原因见容器手册里的迭代器等级）。
- **比较器必须是严格弱序**，核心要求是 `cmp(a, a)` 必须为 `false`。写成 `<=` 就违反了，`sort` 内部可能越界访问导致崩溃 —— 这不是理论风险，是真会崩。

### 3.2 `find_if`：找第一个满足条件的

```cpp
auto it = std::find_if(v.begin(), v.end(),
                       [](const Task& t) { return t.id == 7; });
if (it != v.end()) { /* 找到了，*it 是那个元素 */ }
```

找不到返回 `end()`，所以**必须先判**再解引用。这和 `map::find` 是同一套约定。

### 3.3 `remove_if` + `erase`：条件删除

这是最容易写错的一个：

```cpp
// 去掉 pri < 0 的
v.erase(std::remove_if(v.begin(), v.end(),
                       [](const Task& t) { return t.pri < 0; }),
        v.end());
```

`remove_if` **不会**真的删除任何元素，它做不到 —— 算法只拿到迭代器，碰不到容器本身，没法改 `size`。

它实际做的是：把要**保留**的元素依次往前挪，然后返回一个迭代器，指向「保留区的末尾」。这个位置之后的元素是什么，标准不保证（通常是被移动走后的残骸）。

```text
原始:  [5, -1, 8, -3, 2]
                ↓ remove_if(pri < 0)
结果:  [5, 8, 2, ?, ?]
                 ↑ 返回这个位置
```

所以必须再调 `v.erase(那个位置, v.end())` 把尾巴真正切掉。这一套组合叫 **erase-remove 惯用法**。

只调 `remove_if` 不调 `erase`，是最典型的翻车现场：看起来删了，`size()` 却一点没变，尾部还留着重复的旧值。

C++20 起有更简单的写法：`std::erase_if(v, pred)`，一步到位。

### 3.4 `transform` 和 `accumulate`

```cpp
// 把每个元素变换后放进 out
std::transform(in.begin(), in.end(), std::back_inserter(out),
               [](int x) { return x * 2; });

int sum = std::accumulate(v.begin(), v.end(), 0);   // 0 是初始值
```

`back_inserter(out)` 是个**插入迭代器**：给它赋值等于对 `out` 调 `push_back`。不用它的话，`out` 必须提前 `resize` 到足够大，否则就是往越界的位置写。

`accumulate` 的初始值决定结果类型 —— 写 `0` 得到 `int`，元素是 `double` 也会被截断成整数。求和浮点要写 `0.0`。它也能传第四个参数自定义「怎么合并」，不一定是加法。

## 4. `std::function`：能存，但有代价

### 4.1 先看它解决什么问题

**每个 lambda 都是独一无二的类型**，而且这个类型**没有名字**。

```cpp
auto a = [](int x) { return x > 0; };
auto b = [](int x) { return x > 0; };   // 代码一模一样
// 但 a 和 b 的类型不同！编译器给每个 lambda 生成了一个单独的匿名 class
```

写 `auto` 的时候你没感觉，因为编译器替你填了那个说不出口的类型。可一旦需要**把类型写出来**，就卡住了：

```cpp
class Scheduler {
    ??? cmp_;              // 成员变量怎么声明类型？
};

void f(??? pred);          // 函数参数怎么写？

std::vector<???> handlers; // 想存一堆回调怎么办？
```

`auto` 只能用在「定义时立刻初始化」的地方，上面这三种情况都用不了。

`std::function` 就是来填这个洞的：它是一个**能装下任何可调用东西的盒子**，只要签名对得上。

```cpp
std::function<bool(const Task&)> pred = [](const Task& t) { return t.pri > 0; };
//            ^^^^^^^^^^^^^^^^^^
//            签名：接收一个 const Task&，返回 bool
```

签名一致就能装，不管里面是什么：

```cpp
bool by_pri(const Task& t) { return t.pri > 0; }        // 普通函数
struct ByPri { bool operator()(const Task& t) const; }; // 函数对象

std::function<bool(const Task&)> f;
f = [](const Task& t) { return t.pri > 0; };   // lambda
f = by_pri;                                     // 函数指针
f = ByPri{};                                    // 函数对象
// 同一个变量，先后装了三种完全不同的类型
```

这个「把具体类型藏起来，只保留『能这样调用』这个能力」的技术，叫**类型擦除**（type erasure）。和基类指针指向不同派生类是同一个思路：调用方只认接口，不认具体是谁。

### 4.2 代价从哪来

**代价一：可能堆分配。**

`std::function` 对象本身大小是固定的（通常 32 字节）。lambda 捕获的东西如果小，能塞进这块内置空间；捕获得多了塞不下，就得 `new` 一块堆内存存。

```cpp
int a, b;
std::function<void()> f1 = [a, b] { ... };        // 小，通常不分配

BigConfig cfg;  // 几百字节
std::function<void()> f2 = [cfg] { ... };         // 装不下，堆分配
```

**代价二：不能内联。**

这条影响更大。直接用 lambda 时，编译器**看得见**函数体，可以把调用整个展开掉：

```cpp
std::sort(v.begin(), v.end(), [](auto& a, auto& b) { return a.pri > b.pri; });
// 比较器被内联成几条比较指令，没有函数调用开销
```

换成 `std::function`，编译器只知道「要通过一个指针去调用某个东西」，具体是谁要运行时才确定，于是每次调用都是一次**间接跳转**，展开不了。

`sort` 会调用比较器 O(n log n) 次。10000 个元素就是十几万次。每次多一次间接调用 + 无法内联，差距能到几倍。

### 4.3 那不用它写什么

原来那句「能写成模板 / `decltype(lambda)` 就别先上 `std::function`」说的是这两种替代写法：

**写法 A：模板参数**（标准算法库的做法）

```cpp
template <typename Cmp>
void sort_tasks(std::vector<Task>& v, Cmp cmp) {
    std::sort(v.begin(), v.end(), cmp);
}

sort_tasks(v, [](const Task& a, const Task& b) { return a.pri > b.pri; });
```

`Cmp` 在编译期就被推导成那个具体的匿名类型，编译器看得见函数体，能内联。`std::sort` 的第三个参数就是这么设计的。

**写法 B：`decltype` 把类型「拿」出来**

类型没名字，但可以让编译器指着某个变量说「就是它的类型」：

```cpp
auto cmp = [](const HeapItem& a, const HeapItem& b) {
    return a.priority < b.priority;
};

std::priority_queue<HeapItem, std::vector<HeapItem>, decltype(cmp)> pq(cmp);
//                                                   ^^^^^^^^^^^^^
//                                                   「cmp 那个变量的类型」
```

`decltype(cmp)` 读作「cmp 的类型」。这样模板参数拿到的是真实类型，同样能内联。注意还要把 `cmp` 对象本身传给构造函数 —— 类型只是模板参数，对象得另给。

### 4.4 什么时候该用它

不是说 `std::function` 不好，是要知道在换什么。

| 场景 | 选什么 | 理由 |
|------|--------|------|
| 算法的比较器 / 谓词 | 直接传 lambda | 模板参数，能内联 |
| 热路径（控制循环里每帧调用） | 模板参数 | 间接调用累积起来很贵 |
| 存成成员变量、能运行时替换 | `std::function` | 类型写不出来，且要能换 |
| 存进容器（一堆回调） | `std::function` | 容器要求元素类型统一 |
| 跨模块的接口参数 | `std::function` | 不想让头文件变成模板 |

Day05 的 `Scheduler` 用的是 `std::function`：

```cpp
using Cmp = std::function<bool(const HeapItem&, const HeapItem&)>;
explicit Scheduler(Cmp cmp = default_less);
```

这是**有意的取舍**：要支持「构造时传入任意比较器、默认给一个」，比较器类型就不能写死在模板参数里，否则每种比较器都会生成一个不同类型的 `Scheduler`。调度器每次 `pop` 才比较几次，不在热路径上，这点开销换来的接口灵活性是划算的。

判断标准就一句：**这个可调用对象会被调用多少次？** 几次到几百次，随便用；每帧几万次，去掉它。

## 5. 和 Day04 调度器怎么接

Day04 的调度器已经能跑了，今天不是推倒重来，是把里面**写死的策略**抽出来变成可替换的参数。有两处可以动。

### 5.1 比较器：从写死的 `operator<` 变成构造时传入

Day04 是这么定优先级的：

```cpp
// Day04：规则焊死在类型上，全局唯一
inline bool operator<(const HeapItem& a, const HeapItem& b) {
    return a.priority < b.priority;
}
std::priority_queue<HeapItem> heap_;
```

问题是这条规则**改不了**。想要一个小顶堆的调度器？想要「优先级相同时先到先服务」？只能再写一个类型。

Day05 改成构造时传进来：

```cpp
using Cmp = std::function<bool(const HeapItem&, const HeapItem&)>;
explicit Scheduler(Cmp cmp = default_less);   // 不传就用默认的大顶
```

于是同一个 `Scheduler` 类能装不同策略：

```cpp
Scheduler s1;                              // 默认：优先级大的先出

Scheduler s2([](const HeapItem& a, const HeapItem& b) {
    return a.priority > b.priority;        // 反过来：小顶
});

Scheduler s3([](const HeapItem& a, const HeapItem& b) {
    if (a.priority != b.priority) return a.priority < b.priority;
    return a.version > b.version;          // 同优先级时，先登记的先出
});
```

注意第三个例子里比较器的方向：`priority_queue` 的比较器返回 `true` 表示「a 的优先级**低于** b」，也就是 a 排在后面。想让 version 小的先出队，就要在 version 大的时候返回 `true`。这个方向极容易写反，写完拿两个任务试一下。

### 5.2 快照：用算法代替手写循环

Day04 里「在不在」只能问 `latest_` 这个 map，堆里有过期副本。所以要看「当前还有哪些任务」，得从 map 里捞。

`snapshot_sorted()` 就是干这个的：把 map 里每一项拷进 `vector`，然后排序。手写选择排序也能做，但这正是算法库存在的意义：

```cpp
std::vector<Task> Scheduler::snapshot_sorted() const {
    std::vector<Task> v;
    v.reserve(latest_.size());              // 已知个数，先订容量

    for (const auto& kv : latest_) {        // kv.first 是 id，kv.second 是 Meta
        v.push_back({kv.first, kv.second.priority, kv.second.payload});
    }

    std::sort(v.begin(), v.end(), [](const Task& a, const Task& b) {
        return a.priority > b.priority;     // 从大到小
    });
    return v;
}
```

这里有几个点值得留意：

- `latest_` 是 `unordered_map`，**遍历顺序不确定**。所以必须排序，否则每次运行输出可能不一样。
- 返回的是一份**拷贝**，不是引用。算法在这份拷贝上折腾，动不到调度器的真实状态 —— 这是安全的关键。
- `reserve` 不是必需的，但既然已经知道要放几个，顺手省掉几次扩容。

如果想练 `remove_if`，可以再加个带过滤的版本：

```cpp
// 只要优先级达到门槛的
auto v = snapshot_sorted();
int thresh = 30;
v.erase(std::remove_if(v.begin(), v.end(),
                       [thresh](const Task& t) { return t.priority < thresh; }),
        v.end());
```

这里 `thresh` 必须捕获才能在 lambda 里用 —— lambda 看不见外面的局部变量，除非你把它抓进来。

### 5.3 行为不能变

改完之后，`main.cpp` 的输出应该和 Day04 一致：延迟删除的逻辑一个字都没动，权威数据仍在 map，堆里仍然可能有脏数据。今天只换了「策略怎么表达」，没换「调度器怎么工作」。

跑完对比一下两天的代码行数，写进 `notes.md`。重点不是行数少了多少，是**排序规则现在写在调用点、一眼能看见**，而不是藏在某个全局 `operator<` 里。

## 6. 易错点（看懂再上机）

### 6.1 引用捕获了局部变量，而 lambda 活得更久

```cpp
std::function<int()> make() {
    int local = 42;
    return [&] { return local; };   // local 在函数返回时就没了
}
auto f = make();
f();   // 读的是已经销毁的栈内存
```

**现象**：不一定崩。多半是返回一个垃圾数，或者时对时错 —— 那块栈内存还没被别人覆盖时看着是对的。这类 bug 最难查。

**怎么修**：改成值捕获 `[=]` 或 `[local]`。引用捕获只留给「lambda 一定活得比变量短」的场景，典型就是当场传给算法用完就扔。

### 6.2 一边遍历一边改容器结构

```cpp
// 错：erase 之后 it 已失效，++it 是未定义行为
for (auto it = v.begin(); it != v.end(); ++it)
    if (t.pri < 0) v.erase(it);
```

**现象**：漏删、重复删、或直接崩。`vector` 上 `erase` 会让删除点之后的迭代器全部失效。

**怎么修**：要么用 `erase` 的返回值接着走（`it = v.erase(it)`，且这一轮不要 `++it`），要么直接上 erase-remove 惯用法，一次搬完家。

### 6.3 比较器写成 `<=`

```cpp
std::sort(v.begin(), v.end(), [](const Task& a, const Task& b) {
    return a.pri <= b.pri;   // 错
});
```

**现象**：运行时崩溃，且往往在数据量大了才出现（小数组走的是插入排序分支，不越界）。

**为什么**：`sort` 依赖「严格弱序」，其中一条是 `cmp(a, a)` 必须为 `false`。写 `<=` 时相等元素两边都返回 `true`，内部的分区循环找不到停止条件，指针就走出数组了。

**怎么修**：比较器只写 `<` 或 `>`。需要多级排序时，前几级不等才比较、相等则继续比下一级 —— 就是 5.1 里第三个例子的写法。

### 6.4 热路径上滥用 `std::function`

**现象**：不报错，就是慢。而且慢得不显眼，profile 里看到的是「到处都有一点点开销」。

**怎么修**：见 4.3 的两种替代写法。判断依据还是那句话 —— 这东西一秒被调用多少次。

### 6.5 `[=]` 悄悄拷贝了大对象

```cpp
std::vector<char> buf(10 * 1024 * 1024);   // 10 MB
auto f = [=] { return buf.size(); };        // 整整拷了 10 MB
```

**现象**：内存翻倍、构造 lambda 时莫名卡顿。`[=]` 长得像「按值捕获，很轻」，但它拷的是**真正的对象**，不是指针。

**怎么修**：大对象具名引用捕获 `[&buf]`，同时自己确认 lambda 活不过 `buf`。两者都要满足时，考虑捕获一个 `shared_ptr`。

顺带一提，`[=]` 和 `[&]` 这种「默认全抓」的写法在 C++20 里捕获 `this` 已被弃用，也普遍被认为不如显式列出捕获的变量清楚。养成写 `[thresh]`、`[&out]` 的习惯，读代码的人一眼知道这个 lambda 碰了什么。

## C++ 面试口述（预习时自己答一遍）

1. **值捕获和引用捕获区别？**  
   值是副本，跟 lambda 走；引用是别名，不延长寿命，lambda 活过变量就悬空。
2. **`remove_if` 为什么还要 `erase`？**  
   它只把保留元素挤到前面，不改 size。真正删尾巴靠 erase。
3. **为什么有时宁可手写循环？**  
   逻辑有多步状态、要提前 break、或算法组合更难读时；热路径要避开 `std::function` 时。

## C++ 上机（预习不用写）

见 `week1-cpp-basics/day05-lambda-scheduler/TASK.md`：

- 可自定义比较器
- 用 STL 算法完成过滤或排序至少一类
- 对比两天代码行数与可读性，写进 notes
