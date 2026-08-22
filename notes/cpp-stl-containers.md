# C++ 标准库容器详解

面向 30 天学习的常查手册。每个容器给：底层结构 → 复杂度 → 成员函数 → 迭代器失效 → 易错点 → 例子。

和 Day04 调度器对照：`priority_queue` 是 `heap_`，`unordered_map` 是 `latest_`。

---

## 0. 先建立整体地图

### 0.1 三大家族

| 家族 | 成员 | 底层 | 特点 |
|------|------|------|------|
| 顺序容器 | `vector` `string` `array` `deque` `list` `forward_list` | 数组 / 链表 | 元素按你放的顺序排 |
| 容器适配器 | `stack` `queue` `priority_queue` | 包在别的容器外面 | 只暴露受限接口 |
| 关联容器 | `map` `set` `multimap` `multiset` | 红黑树 | 按 key 排序 |
| 无序关联 | `unordered_map` `unordered_set` `unordered_multi*` | 哈希表 | 不排序，平均更快 |

### 0.2 几乎所有容器都有的成员

| 函数 | 作用 |
|------|------|
| `size()` | 元素个数 |
| `empty()` | 是否为空。比 `size()==0` 更通用（`forward_list` 没有 `size`） |
| `clear()` | 清空元素（**不一定**释放内存） |
| `begin()` / `end()` | 迭代器区间 `[begin, end)` |
| `cbegin()` / `cend()` | 只读迭代器 |
| `rbegin()` / `rend()` | 反向迭代器（`forward_list`、无序容器没有） |
| `swap(other)` | 和另一个同类型容器交换，通常 O(1) |
| `max_size()` | 理论上限，实际很少用 |

`stack` / `queue` / `priority_queue` **没有迭代器**，也就没有 `begin()`。

### 0.3 `end()` 是什么

`end()` 指向 **最后一个元素的后面一格**，不是最后一个元素。所以：

```cpp
for (auto it = v.begin(); it != v.end(); ++it) { /* *it 是元素 */ }
```

`find` 找不到时返回 `end()`，这就是 `if (it == m.end())` 的由来。

### 0.4 复杂度总览

| 操作 | vector | deque | list | map | unordered_map |
|------|--------|-------|------|-----|---------------|
| 按下标访问 | O(1) | O(1) | 不支持 | 不支持 | 不支持 |
| 按 key 查找 | O(n) | O(n) | O(n) | O(log n) | 平均 O(1)，最坏 O(n) |
| 尾部插入 | 均摊 O(1) | O(1) | O(1) | — | — |
| 头部插入 | O(n) | O(1) | O(1) | — | — |
| 中间插入 | O(n) | O(n) | O(1)（已有迭代器） | O(log n) | 平均 O(1) |
| 删除 | O(n) | O(n) | O(1)（已有迭代器） | O(log n) | 平均 O(1) |

---

## 1. `vector`：动态数组

**底层**：一块连续的堆内存 + `size` + `capacity`。Day03 的 `DynArray` 就是它的简化版。

**选它的理由**：内存连续 → CPU 缓存友好 → 实际跑得比链表快得多，即使理论复杂度更差。默认就用 `vector`，除非有明确理由换。

### 成员函数

**构造**

```cpp
std::vector<int> a;                 // 空
std::vector<int> b(10);             // 10 个 0
std::vector<int> c(10, 7);          // 10 个 7
std::vector<int> d{1, 2, 3};        // 初始化列表
std::vector<int> e(d.begin(), d.end());  // 区间拷贝
```

注意 `vector<int> b(10)` 和 `vector<int> b{10}` 完全不同：前者是 10 个 0，后者是 1 个元素 10。

**访问**

| 函数 | 说明 |
|------|------|
| `v[i]` | 不检查越界，越界是未定义行为（可能不崩，更可怕） |
| `at(i)` | 检查越界，抛 `std::out_of_range` |
| `front()` | 第一个元素。空容器上调用是未定义行为 |
| `back()` | 最后一个元素。同上 |
| `data()` | 返回底层 `T*`，用于和 C 接口对接 |

**改大小 / 容量**

| 函数 | 说明 |
|------|------|
| `push_back(x)` | 尾部追加，均摊 O(1) |
| `emplace_back(args...)` | 尾部**原地构造**，少一次临时对象 |
| `pop_back()` | 删最后一个，不返回值 |
| `insert(pos, x)` | 在迭代器位置插入，O(n) |
| `erase(pos)` / `erase(first, last)` | 删除，返回被删元素之后的迭代器 |
| `resize(n)` | 真的改成 n 个元素（多了删、少了补默认值） |
| `reserve(n)` | 只订容量，`size` 不变 |
| `capacity()` | 当前容量 |
| `shrink_to_fit()` | 请求把容量缩到 size，**不保证**执行 |
| `assign(n, x)` | 整体替换内容 |

### 扩容机制

`size == capacity` 时再 `push_back`：

1. 申请更大的块（GCC/Clang 通常 2 倍，MSVC 1.5 倍）
2. 把旧元素移动（有 `noexcept` 移动构造时）或拷贝过去
3. 析构旧元素，释放旧块

单次 O(n)，但因为容量翻倍，均摊到每次 `push_back` 仍是 **O(1)**（1+2+4+…+n ≈ 2n）。

这就是 Day01 里 `noexcept` 移动构造值钱的地方：没有 `noexcept`，`vector` 扩容时为了异常安全会退化成**拷贝**。

### 迭代器失效

| 操作 | 失效范围 |
|------|----------|
| 扩容（`push_back` 触发） | **全部**迭代器、引用、指针 |
| `push_back` 未扩容 | `end()` 失效，其余仍有效 |
| `insert` | 插入点及之后全部；若扩容则全部 |
| `erase` | 删除点及之后全部 |
| `reserve` / `resize` 导致重新分配 | 全部 |
| 只读操作 | 不失效 |

```cpp
std::vector<int> v{1, 2};
int& r = v[0];
v.push_back(3);
v.push_back(4);   // 很可能扩容
// r 现在可能悬空，不要再用
```

### 易错点

1. **循环里 `push_back` 还拿着旧迭代器**——扩容后悬空。改用下标，或先 `reserve` 够。
2. **边遍历边 `erase`**：

```cpp
// 错：erase 之后 it 失效，++it 是未定义行为
for (auto it = v.begin(); it != v.end(); ++it)
    if (*it == 3) v.erase(it);

// 对：erase 返回下一个位置
for (auto it = v.begin(); it != v.end(); )
    if (*it == 3) it = v.erase(it);
    else ++it;

// 更好：erase-remove 惯用法
v.erase(std::remove(v.begin(), v.end(), 3), v.end());
```

3. **`clear()` 不释放内存**：`capacity` 不变。要真还给系统：`std::vector<int>().swap(v)` 或 `shrink_to_fit()`。
4. **`vector<bool>` 不是普通容器**：它是位压缩的特化，`operator[]` 返回代理对象，`&v[0]` 拿不到 `bool*`。要真数组用 `vector<char>` 或 `std::bitset`。

---

## 2. `string`：字符专用的动态数组

本质是 `basic_string<char>`，行为和 `vector<char>` 很像，另加一堆字符串操作。

| 函数 | 说明 |
|------|------|
| `c_str()` | 返回 `const char*`，保证以 `\0` 结尾 |
| `data()` | C++11 起和 `c_str()` 基本等价 |
| `size()` / `length()` | 完全一样，两个名字 |
| `substr(pos, len)` | 取子串，返回**新** string |
| `find(s)` / `rfind(s)` | 找不到返回 `std::string::npos`，**不是** -1 也不是 0 |
| `replace(pos, len, s)` | 替换 |
| `append(s)` / `operator+=` | 追加 |
| `compare(s)` | 比较，返回负 / 0 / 正 |
| `starts_with` / `ends_with` | C++20 |
| `std::stoi` / `std::to_string` | 字符串与数字互转（非成员函数） |

**易错点**

```cpp
if (s.find("abc") != std::string::npos) { /* 找到了 */ }  // 对
if (s.find("abc") >= 0) { /* 恒真！npos 是很大的无符号数 */ }  // 错
```

小字符串优化（SSO）：短字符串直接存在对象内部，不上堆。所以短 `string` 拷贝很便宜。

---

## 3. `array`：定长数组的壳

```cpp
std::array<int, 5> a{1, 2, 3, 4, 5};
```

长度是模板参数，**编译期定死**，不能增删。元素在栈上（如果对象在栈上）。

| 函数 | 说明 |
|------|------|
| `a[i]` / `at(i)` | 访问 |
| `size()` | 编译期常量 |
| `fill(x)` | 全部填成 x |
| `data()` | 底层指针 |
| `front()` / `back()` | 首尾 |

比 C 数组好在：知道自己的 `size()`、可以拷贝赋值、能进标准算法。

---

## 4. `deque`：双端队列

**底层**：一组固定大小的块 + 一张块索引表。所以**不是**一整块连续内存。

| 函数 | 说明 |
|------|------|
| `push_front(x)` / `pop_front()` | 头部，O(1) |
| `push_back(x)` / `pop_back()` | 尾部，O(1) |
| `d[i]` / `at(i)` | 下标访问，O(1)（比 vector 稍慢，要过索引表） |

**没有** `capacity()` / `reserve()`。

**失效规则和 vector 不同**：在头尾插入会让**迭代器**失效，但**引用和指针仍然有效**（元素没被搬家）。中间插入则全失效。

什么时候用：需要频繁在**头部**插删。否则优先 `vector`。

---

## 5. `list` / `forward_list`：链表

`list` 是双向链表，`forward_list` 是单向。

| 函数 | 说明 |
|------|------|
| `push_front` / `push_back` | O(1)（`forward_list` 只有 front） |
| `insert(pos, x)` | 已有迭代器时 O(1) |
| `erase(pos)` | 同上 |
| `splice(pos, other)` | **接管**另一个链表的节点，O(1)，不拷贝元素 |
| `remove(value)` / `remove_if(pred)` | 按值 / 条件删 |
| `sort()` | 成员版（`std::sort` 需要随机访问，链表用不了） |
| `merge(other)` | 合并两个有序链表 |
| `unique()` | 去掉**相邻**重复元素 |
| `reverse()` | 反转 |

**最大优点**：插入删除不会让**其他**迭代器失效（只有被删的那个失效）。

**最大缺点**：节点分散在堆上，缓存不友好；没有下标访问。实际项目里比想象中少用。

---

## 6. `stack`：栈（适配器）

默认底层是 `deque`。后进先出。

| 函数 | 说明 |
|------|------|
| `push(x)` / `emplace(args...)` | 压栈 |
| `pop()` | 弹栈，**不返回元素** |
| `top()` | 看栈顶（可修改） |
| `size()` / `empty()` | — |

```cpp
// 正确用法：先看再弹
auto x = s.top();
s.pop();
```

`pop()` 不返回值是有意设计：返回值需要拷贝，拷贝可能抛异常，那样元素就既没返回也已被删掉了。

---

## 7. `queue`：队列（适配器）

默认底层 `deque`。先进先出。

| 函数 | 说明 |
|------|------|
| `push(x)` / `emplace(...)` | 尾部入队 |
| `pop()` | 头部出队，不返回 |
| `front()` | 看队头 |
| `back()` | 看队尾 |

---

## 8. `priority_queue`：优先队列 / 堆

**底层**：默认 `vector` + 堆算法（`push_heap` / `pop_heap`）。**默认是大顶堆**：`top()` 是最大的。

| 函数 | 复杂度 |
|------|--------|
| `push(x)` | O(log n) |
| `emplace(...)` | O(log n) |
| `top()` | O(1)，返回 `const&`，**不能改** |
| `pop()` | O(log n)，不返回元素 |
| `size()` / `empty()` | O(1) |

**它做不到的事**：没有迭代器，没有 `find`，不能按 key 定位并修改内部元素，不能遍历。

### 改比较器

```cpp
// 小顶堆
std::priority_queue<int, std::vector<int>, std::greater<int>> minheap;

// 自定义类型：给 operator< （Day04 用的就是这个）
inline bool operator<(const HeapItem& a, const HeapItem& b) {
    return a.priority < b.priority;   // 返回 true 表示 a 排在 b 后面
}

// lambda 当比较器
auto cmp = [](const Task& a, const Task& b) { return a.pri < b.pri; };
std::priority_queue<Task, std::vector<Task>, decltype(cmp)> pq(cmp);
```

比较器的语义要记牢：`cmp(a, b) == true` 表示 **a 的优先级低于 b**，所以默认的 `less` 得到的是大顶堆。写反了最紧急的任务永远出不来。

### 延迟删除（Day04 的核心技巧）

因为不能改内部元素，改优先级的标准做法：

1. 权威数据放 `unordered_map`，附带一个 `version`
2. 改优先级 → map 里 `version++` 并更新值 → 往堆里 **再 push 一条**新记录
3. 旧记录留在堆里不管
4. `pop` 时循环检查：堆顶的 `version` 是否等于 map 里的 `version`；不等就丢掉继续弹

要点：**「在不在」只能问 map**，因为堆里有过期副本，`heap_.size()` 会偏大。

---

## 9. `map` / `set`：有序关联容器

**底层**：红黑树（自平衡二叉搜索树）。所有操作 O(log n)，元素按 key **有序**。

`map` 存 `pair<const Key, Value>`，`set` 只存 key。

| 函数 | 说明 |
|------|------|
| `m[k]` | 取值；**key 不存在会插入默认值** |
| `at(k)` | 取值；不存在抛 `out_of_range` |
| `find(k)` | 返回迭代器，没有则 `end()` |
| `count(k)` | `map`/`set` 里只能是 0 或 1 |
| `contains(k)` | C++20，语义最清楚 |
| `insert({k, v})` | 已存在则不覆盖，返回 `pair<iterator, bool>` |
| `insert_or_assign(k, v)` | C++17，存在就覆盖 |
| `emplace(k, v)` | 原地构造 |
| `try_emplace(k, args...)` | C++17，已存在则不构造 value |
| `erase(k)` / `erase(it)` | 删除 |
| `lower_bound(k)` | 第一个 **>= k** 的位置 |
| `upper_bound(k)` | 第一个 **> k** 的位置 |
| `equal_range(k)` | 上面两个打包 |

### 迭代器怎么用

```cpp
auto it = m.find(id);
if (it == m.end()) { /* 没有 */ }
it->first;    // key（const，不能改）
it->second;   // value（可以改）

for (const auto& [k, v] : m) { /* C++17 结构化绑定 */ }
```

`it` 用 `->` 是因为它**像指针**：解引用才拿到那一行 `pair`。而 `m[k]` 直接拿到的是 value 本身，所以用 `.`。

### `map` 的独门本事：范围查询

```cpp
// 找出所有 key 在 [10, 20) 的元素
for (auto it = m.lower_bound(10); it != m.upper_bound(20); ++it) ...
```

哈希表做不到这个 —— 这是选 `map` 而不是 `unordered_map` 的主要理由。

### 易错点

```cpp
if (m[k] == 0) { }   // 危险：k 不存在时会插入一个 0 进去，map 变大了
if (m.count(k)) { }  // 安全
```

在 `const map&` 上不能用 `operator[]`，正因为它会插入。

### 迭代器失效

`map` / `set` 的插入**不会**让任何迭代器失效；删除只让被删的那个失效。这点比 `vector` 友好得多。

---

## 10. `unordered_map` / `unordered_set`：哈希表

**底层**：桶数组 + 每个桶挂一条链（拉链法）。

平均 O(1)，最坏 O(n)（所有 key 挤进同一个桶）。不排序，遍历顺序不确定且可能在 rehash 后改变。

接口和 `map` 大体相同，但：

- **没有** `lower_bound` / `upper_bound`（没有序，就没有范围查询）
- 多了哈希相关的：

| 函数 | 说明 |
|------|------|
| `bucket_count()` | 桶个数 |
| `load_factor()` | 元素数 / 桶数 |
| `max_load_factor(f)` | 超过这个比例就 rehash |
| `rehash(n)` | 重建成至少 n 个桶 |
| `reserve(n)` | 预留能装 n 个元素的空间，避免多次 rehash |

### 自定义类型当 key

需要两样东西：哈希函数 + 相等比较。

```cpp
struct Point { int x, y; };

struct PointHash {
    std::size_t operator()(const Point& p) const {
        return std::hash<int>{}(p.x) ^ (std::hash<int>{}(p.y) << 1);
    }
};
struct PointEq {
    bool operator()(const Point& a, const Point& b) const {
        return a.x == b.x && a.y == b.y;
    }
};

std::unordered_map<Point, int, PointHash, PointEq> m;
```

`map` 只需要 `operator<`，要求更低。

### 迭代器失效

**rehash 会让所有迭代器失效**（元素被重新分配到新桶），但**引用和指针仍然有效**（节点本身没被销毁）。

### 易错点

1. 依赖遍历顺序 —— 顺序不保证，换编译器/换版本可能就变了。
2. key 插入后修改其哈希相关字段 —— 从此再也找不回来。id 用 `int` / `string` 这种不变的值。
3. 大量插入前忘了 `reserve` —— 多次 rehash 很贵。

---

## 11. `map` vs `unordered_map` 怎么选

| 需求 | 选谁 |
|------|------|
| 只按 key 查、增、删 | `unordered_map`（平均更快） |
| 需要按 key 有序遍历 | `map` |
| 需要范围查询 / `lower_bound` | `map` |
| key 不好写哈希函数 | `map`（只要 `operator<`） |
| 要求稳定的最坏复杂度 | `map`（保证 O(log n)） |
| 元素很少（几十个以内） | `map` 甚至 `vector<pair>` 线性查找可能更快 |

Day04 调度器按 id 查、不需要排序 → `unordered_map`。

---

## 12. `multimap` / `multiset` / `unordered_multi*`

允许**同一个 key 出现多次**。

- `count(k)` 可能大于 1
- **没有** `operator[]`（一个 key 对多个值，下标没意义）
- 查全部同 key 元素用 `equal_range(k)`

```cpp
auto [first, last] = mm.equal_range(k);
for (auto it = first; it != last; ++it) { /* it->second */ }
```

---

## 13. 三个容易混的语义

### `pop()` / `erase()` 不返回元素

`stack::pop`、`queue::pop`、`priority_queue::pop` 都只删不返回。要先 `top()` / `front()` 拿到值再 `pop()`。

### `clear()` 不一定还内存

`vector::clear()` 之后 `size()==0`，但 `capacity()` 不变。

### `[]` 会创建元素

`map` / `unordered_map` 的 `operator[]` 在 key 不存在时会**插入**默认构造的值。只是查询请用 `count` / `find` / `contains`。

`vector` 的 `[]` 则相反：越界不会自动扩，是未定义行为。

---

## 14. 迭代器失效速查表

| 容器 | 插入 | 删除 |
|------|------|------|
| `vector` | 扩容则全失效；否则插入点之后失效 | 删除点之后失效 |
| `deque` | 头尾插：迭代器失效，引用有效；中间插：全失效 | 头尾删：只失效被删的；中间删：全失效 |
| `list` / `forward_list` | 不失效 | 只失效被删的 |
| `map` / `set` | 不失效 | 只失效被删的 |
| `unordered_*` | rehash 时迭代器全失效，引用有效 | 只失效被删的 |

**记忆线索**：只要元素可能被**搬家**（连续内存重新分配、rehash），迭代器就危险；节点式容器（list、map）元素不动，所以安全。

---

## 15. 选择流程图（文字版）

```text
要按 key 查吗？
├─ 是 → 需要有序 / 范围查询吗？
│        ├─ 是 → map / set
│        └─ 否 → unordered_map / unordered_set
└─ 否 → 只关心最大或最小的那个吗？
         ├─ 是 → priority_queue
         └─ 否 → 只在两端进出吗？
                  ├─ 只一端 → stack 或 queue
                  ├─ 两端都要 → deque
                  └─ 否 → 中间频繁插删且已有迭代器？
                           ├─ 是 → list
                           └─ 否 → vector（默认答案）
```

---

## 16. Day04 调度器对照

```cpp
std::priority_queue<HeapItem> heap_;          // 谁最紧急
std::unordered_map<int, Meta> latest_;        // 权威：在不在、最新版本
```

```text
heap_.push(x)       塞一条记录
heap_.top()         看最紧急的（不删）
heap_.pop()         扔掉最紧急的（不返回）
heap_.empty()       还有没有

latest_.count(id)   在不在（0 或 1）
latest_[id]         取那份 Meta，可直接改字段
latest_.find(id)    拿迭代器，配合 end() 判断
latest_.erase(it)   删掉这一行
it->second.version  迭代器用 ->，第二个成员是 value
```

核心约束：堆里可能有脏数据，**任何「状态」问题都问 map，不问堆**。
