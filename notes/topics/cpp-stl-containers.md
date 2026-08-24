# C++ 标准库容器详解

面向 30 天学习的常查手册。每个容器给：底层结构 → 复杂度 → 成员函数 → 迭代器失效 → 易错点 → 例子。

和 Day04 调度器对照：`priority_queue` 是 `heap_`，`unordered_map` 是 `latest_`。

---

## 0. 先建立整体地图

### 0.1 三大家族

| 家族 | 成员 | 头文件 | 底层 | 特点 |
|------|------|--------|------|------|
| 顺序容器 | `vector` `string` `array` `deque` `list` `forward_list` | `<vector>` `<string>` `<array>` `<deque>` `<list>` `<forward_list>` | 数组 / 链表 | 元素按你放的顺序排 |
| 容器适配器 | `stack` `queue` `priority_queue` | `<stack>` `<queue>` `<queue>` | 包在别的容器外面 | 只暴露受限接口，没有迭代器 |
| 有序关联 | `map` `set` `multimap` `multiset` | `<map>` `<set>` | 红黑树 | 按 key 排序，O(log n) |
| 无序关联 | `unordered_map` `unordered_set` `unordered_multi*` | `<unordered_map>` `<unordered_set>` | 哈希表 | 不排序，平均 O(1) |

### 0.2 几乎所有容器都有的成员

| 函数 | 作用 | 复杂度 |
|------|------|--------|
| `size()` | 元素个数 | O(1)（`forward_list` 没有） |
| `empty()` | 是否为空。比 `size()==0` 更通用 | O(1) |
| `clear()` | 清空元素（**不一定**释放内存） | O(n) |
| `begin()` / `end()` | 迭代器区间 `[begin, end)` | O(1) |
| `cbegin()` / `cend()` | 只读迭代器 | O(1) |
| `rbegin()` / `rend()` | 反向迭代器（`forward_list`、无序容器没有） | O(1) |
| `swap(other)` | 和另一个同类型容器交换内部指针 | O(1) |
| `max_size()` | 理论上限，实际很少用 | O(1) |

`stack` / `queue` / `priority_queue` **没有迭代器**，也就没有 `begin()`，不能用范围 for，不能进标准算法。

### 0.3 `end()` 是什么

`end()` 指向 **最后一个元素的后面一格**，不是最后一个元素。它是个"哨兵"，**不能解引用**。

```cpp
for (auto it = v.begin(); it != v.end(); ++it) { /* *it 是元素 */ }
```

`find` 系列找不到时返回 `end()`，这就是 `if (it == m.end())` 的由来。空容器的 `begin() == end()`。

### 0.4 复杂度总览

| 操作 | vector | deque | list | map | unordered_map |
|------|--------|-------|------|-----|---------------|
| 按下标访问 | O(1) | O(1) | 不支持 | 不支持 | 不支持 |
| 按 key 查找 | O(n) | O(n) | O(n) | O(log n) | 平均 O(1)，最坏 O(n) |
| 尾部插入 | 均摊 O(1) | O(1) | O(1) | — | — |
| 头部插入 | O(n) | O(1) | O(1) | — | — |
| 中间插入 | O(n) | O(n) | O(1)（已有迭代器） | O(log n) | 平均 O(1) |
| 删除 | O(n) | O(n) | O(1)（已有迭代器） | O(log n) | 平均 O(1) |

**复杂度不等于快慢**。`vector` 中间插入是 O(n)，但因为内存连续、CPU 预取有效，元素少于几千个时通常仍然打得过 `list` 的 O(1)。先默认 `vector`，测出来慢了再换。

### 0.5 迭代器的五个等级

不同容器给的迭代器能力不同，这决定了哪些算法能用。

| 等级 | 能做什么 | 谁提供 |
|------|----------|--------|
| 输入 / 输出 | 单向走一遍，只读 / 只写 | 流迭代器 |
| 前向 | 单向走，可多遍 | `forward_list`、`unordered_*` |
| 双向 | 还能 `--` | `list`、`map`、`set` |
| 随机访问 | 还能 `it + n`、`it1 - it2`、`<` | `vector`、`deque`、`array`、`string` |

`std::sort` 要求**随机访问**，所以 `list.sort()` 只能用成员版。`std::binary_search` / `lower_bound` 在非随机访问迭代器上虽然能编译，但比较次数还是 O(log n)、走步数退化成 O(n)。

---

## 1. `vector`：动态数组

**底层**：一块连续的堆内存 + `size` + `capacity`。实现上通常就是三根指针（起点、终点、容量末尾）。Day03 的 `DynArray` 就是它的简化版。

**选它的理由**：内存连续 → CPU 缓存友好 → 实际跑得比链表快得多，即使理论复杂度更差。默认就用 `vector`，除非有明确理由换。

### 1.1 构造

```cpp
std::vector<int> a;                      // 空，不分配内存
std::vector<int> b(10);                  // 10 个 0（值初始化）
std::vector<int> c(10, 7);               // 10 个 7
std::vector<int> d{1, 2, 3};             // 初始化列表：3 个元素
std::vector<int> e(d.begin(), d.end());  // 区间拷贝
std::vector<int> f(d);                   // 拷贝构造
std::vector<int> g(std::move(d));        // 移动构造，d 变空壳
```

**圆括号和花括号是两回事**：

```cpp
std::vector<int> b(10);   // 10 个元素，全是 0
std::vector<int> b{10};   // 1 个元素，值是 10
```

初始化列表优先级更高。`vector<std::string> s(3)` 是 3 个空串，`vector<std::string> s{3}` 编译错误（3 不能转成 string）。

### 1.2 访问

| 函数 | 复杂度 | 返回 | 说明 |
|------|--------|------|------|
| `v[i]` | O(1) | `T&` | **不检查越界**，越界是未定义行为（可能不崩，更可怕） |
| `at(i)` | O(1) | `T&` | 检查越界，抛 `std::out_of_range` |
| `front()` | O(1) | `T&` | 第一个元素。**空容器上是未定义行为** |
| `back()` | O(1) | `T&` | 最后一个元素。同上 |
| `data()` | O(1) | `T*` | 底层裸指针，用于和 C 接口对接 |

热点循环里用 `[]`，边界不确定的地方用 `at()`。`at()` 的检查开销很小，但它会阻止某些向量化优化。

`data()` 在空 `vector` 上可能返回 `nullptr`，也可能返回一个不可解引用的合法指针 —— 别假设它非空，先判 `empty()`。

### 1.3 增删

| 函数 | 复杂度 | 返回 | 说明 |
|------|--------|------|------|
| `push_back(x)` | 均摊 O(1) | `void` | 尾部追加，拷贝或移动 x |
| `emplace_back(args...)` | 均摊 O(1) | `T&`（C++17 起） | 尾部**原地构造**，省一次临时对象 |
| `pop_back()` | O(1) | `void` | 删最后一个，**不返回值**。空容器上是未定义行为 |
| `insert(pos, x)` | O(n) | 指向新元素的迭代器 | 在迭代器位置**之前**插入 |
| `insert(pos, n, x)` | O(n) | 迭代器 | 插入 n 个 x |
| `insert(pos, first, last)` | O(n) | 迭代器 | 插入一个区间 |
| `emplace(pos, args...)` | O(n) | 迭代器 | 原地构造版的 insert |
| `erase(pos)` | O(n) | **被删元素之后**的迭代器 | 删一个 |
| `erase(first, last)` | O(n) | 迭代器 | 删一段，比逐个删快得多 |
| `assign(n, x)` | O(n) | `void` | 整体替换成 n 个 x |
| `clear()` | O(n) | `void` | 清空元素，`capacity` **不变** |

`insert` / `erase` 的 O(n) 来自"搬家"：后面所有元素都要往前/往后挪一格。在**尾部**操作没有搬家，所以是 O(1)。

### 1.4 `push_back` vs `emplace_back`

```cpp
struct Task { Task(int id, std::string name); };
std::vector<Task> v;

v.push_back(Task(1, "a"));   // 1) 构造临时 Task  2) 移动进容器  3) 析构临时对象
v.emplace_back(1, "a");      // 直接在容器内存上构造，只有 1 步
```

`emplace_back` 把参数**完美转发**给 `T` 的构造函数。对 `int` 这种平凡类型两者没区别；对含有堆分配成员的类型（`string`、`vector`）能省一次移动。

但 `emplace_back` 有个坑：它会调用 **explicit** 构造函数，绕过你可能想要的隐式转换检查。

```cpp
std::vector<std::unique_ptr<int>> v;
v.push_back(new int(1));      // 编译错误（unique_ptr 构造是 explicit）—— 这是好事
v.emplace_back(new int(1));   // 编译通过，但如果扩容时抛异常就泄漏
```

安全写法是 `v.push_back(std::make_unique<int>(1))`。

### 1.5 容量

| 函数 | 复杂度 | 说明 |
|------|--------|------|
| `size()` | O(1) | 当前**元素个数** |
| `capacity()` | O(1) | 当前**能装下**几个而不重新分配 |
| `reserve(n)` | O(n) | 只订容量，`size` 不变，**不构造元素** |
| `resize(n)` | O(n) | 真的改成 n 个元素：多了删，少了补默认值 |
| `resize(n, x)` | O(n) | 少了的补 x |
| `shrink_to_fit()` | O(n) | 请求把容量缩到 `size`，**不保证**执行 |

`reserve` 和 `resize` 最容易混：

```cpp
std::vector<int> v;
v.reserve(10);
v.size();       // 0 —— 还没有元素，v[0] 是未定义行为
v.capacity();   // >= 10

std::vector<int> w;
w.resize(10);
w.size();       // 10 —— 真有 10 个 0，w[0] 合法
```

**已知要塞多少个就先 `reserve`**：省掉多次扩容和搬家，也顺带避免迭代器失效。

`reserve` 只增不减：`reserve(5)` 在容量已是 100 时什么都不做。

### 1.6 扩容机制

`size == capacity` 时再 `push_back`：

1. 申请更大的块（GCC/Clang 通常 2 倍，MSVC 1.5 倍）
2. 把旧元素**移动**（有 `noexcept` 移动构造时）或**拷贝**过去
3. 析构旧元素，释放旧块

单次 O(n)，但因为容量成倍增长，均摊到每次 `push_back` 仍是 **O(1)**（1+2+4+…+n ≈ 2n）。

这就是 Day01 里 `noexcept` 移动构造值钱的地方：扩容中途如果抛异常，`vector` 必须保证原状态不被破坏。移动是破坏性的，没法回滚；拷贝可以。所以**移动构造没标 `noexcept`，`vector` 扩容时就退化成拷贝**。

```cpp
MyString(MyString&& other) noexcept { /* ... */ }   // 这个 noexcept 直接决定扩容性能
```

### 1.7 迭代器失效

| 操作 | 失效范围 |
|------|----------|
| 扩容（`push_back` 触发） | **全部**迭代器、引用、指针 |
| `push_back` 未扩容 | 只有 `end()` 失效，其余仍有效 |
| `insert` | 插入点及之后全部；若扩容则全部 |
| `erase` | 删除点及之后全部 |
| `reserve` / `resize` 导致重新分配 | 全部 |
| `clear()` | 全部（元素被析构了） |
| 只读操作（`size`、`[]`、遍历） | 不失效 |

```cpp
std::vector<int> v{1, 2};
int& r = v[0];
v.push_back(3);
v.push_back(4);   // 很可能扩容
// r 现在可能悬空，不要再用
```

**判断口诀**：只要元素可能被搬到新内存，指向老内存的一切（迭代器、引用、指针）全部作废。

### 1.8 易错点

1. **循环里 `push_back` 还拿着旧迭代器** —— 扩容后悬空。改用下标，或先 `reserve` 够。

2. **边遍历边 `erase`**：

```cpp
// 错：erase 之后 it 失效，++it 是未定义行为
for (auto it = v.begin(); it != v.end(); ++it)
    if (*it == 3) v.erase(it);

// 对：erase 返回下一个位置
for (auto it = v.begin(); it != v.end(); )
    if (*it == 3) it = v.erase(it);
    else ++it;

// 更好：erase-remove 惯用法，只搬一次家
v.erase(std::remove(v.begin(), v.end(), 3), v.end());

// C++20 起最简单
std::erase(v, 3);
```

`std::remove` **不删元素**，它把要保留的往前挪，返回"新逻辑末尾"。只调 `remove` 不调 `erase`，`size()` 不会变，尾部残留旧值。

3. **`clear()` 不释放内存**：`capacity` 不变。要真还给系统：

```cpp
std::vector<int>().swap(v);   // C++98 惯用法，最可靠
v.shrink_to_fit();            // C++11，但标准允许实现忽略它
```

4. **`vector<bool>` 不是普通容器**：它是位压缩的特化，`operator[]` 返回**代理对象**而不是 `bool&`，`&v[0]` 拿不到 `bool*`，`auto x = v[0]` 拿到的是代理而非值。要真数组用 `vector<char>`；要定长位集用 `std::bitset`。

5. **自引用插入**：`v.push_back(v[0])` 在扩容时，`v[0]` 的引用会在拷贝前失效。标准要求实现处理好这种情况，但自定义容器（比如 Day03 的 `DynArray`）不会自动帮你处理。

---

## 2. `string`：字符专用的动态数组

本质是 `std::basic_string<char>`，行为和 `vector<char>` 很像，另加一堆字符串操作。容量语义（`size` / `capacity` / `reserve` / 扩容）和 `vector` 完全一致。

### 2.1 常用成员

| 函数 | 复杂度 | 说明 |
|------|--------|------|
| `c_str()` | O(1) | 返回 `const char*`，保证以 `\0` 结尾 |
| `data()` | O(1) | C++11 起和 `c_str()` 等价（也保证 `\0`） |
| `size()` / `length()` | O(1) | 完全一样，两个名字。**不含**结尾的 `\0` |
| `substr(pos, len)` | O(len) | 取子串，返回**新** string（有分配开销） |
| `find(s, pos)` | O(n·m) | 找不到返回 `npos` |
| `rfind(s)` | O(n·m) | 从后往前找 |
| `find_first_of(chars)` | O(n·m) | 找**任一**字符首次出现 |
| `replace(pos, len, s)` | O(n) | 替换一段 |
| `append(s)` / `operator+=` | 均摊 O(m) | 追加 |
| `insert(pos, s)` / `erase(pos, len)` | O(n) | 中间插删 |
| `compare(s)` | O(n) | 返回负 / 0 / 正 |
| `starts_with` / `ends_with` | O(m) | C++20 |
| `std::stoi` / `std::to_string` | — | 字符串与数字互转（**非成员**函数） |

### 2.2 `npos` 陷阱

```cpp
if (s.find("abc") != std::string::npos) { /* 找到了 */ }   // 对
if (s.find("abc") >= 0) { /* 恒真！ */ }                    // 错
```

`npos` 是 `static const size_type(-1)`，即最大的 `size_t`。它**不是** -1，也不是负数 —— 无符号数没有负数，`>= 0` 永远成立。

### 2.3 小字符串优化（SSO）

短字符串（libstdc++ 约 15 字符以内）直接存在 `string` 对象内部，不上堆。所以短 `string` 的拷贝很便宜，不必为它到处用 `const string&`。

但这也意味着 `string` 对象本身比你想的大（通常 32 字节），别在超大数组里无脑塞 `string`。

### 2.4 拼接与 `string_view`

```cpp
s = a + b + c;        // 可能产生 2 个临时对象
s = a; s += b; s += c;   // 更省，只在需要时扩容

// 只读、不拥有的字符串视图，零拷贝
void log(std::string_view msg);   // 传 const char* 和 string 都不分配
log("hello");
log(s);
```

`string_view` **不拥有**数据。别让它活过底层字符串：`std::string_view sv = get_string();` 是悬空的（临时对象已析构）。

---

## 3. `array`：定长数组的壳

**底层**：就是一个 C 数组，外面套了层标准容器接口。没有任何额外开销 —— `sizeof(std::array<int,5>) == sizeof(int[5]) == 20`。

**选它的理由**：长度编译期已知时，它比 `vector` 更好 —— 不用堆分配、不用析构、缓存友好到极致。同时又补齐了 C 数组缺的一切。

### 3.1 构造与初始化

```cpp
std::array<int, 5> a{1, 2, 3, 4, 5};
std::array<int, 5> b{1, 2};      // 剩下 3 个补 0
std::array<int, 5> c{};          // 全部 0
std::array<int, 5> d;            // 未初始化！内容是垃圾值
```

**`c{}` 和 `d` 的区别是真的会咬人**。`array` 是聚合类型，没有构造函数，不写 `{}` 就完全不初始化。这点和 `vector<int> v(5)`（保证 5 个 0）正好相反。养成写 `{}` 的习惯。

长度是**模板参数**，编译期定死。`std::array<int,5>` 和 `std::array<int,6>` 是两个毫不相干的类型，不能互相赋值。

C++17 起可以推导：

```cpp
std::array a{1, 2, 3};   // 推导成 std::array<int, 3>
```

### 3.2 成员函数

| 函数 | 复杂度 | 说明 |
|------|--------|------|
| `a[i]` | O(1) | 不查越界 |
| `at(i)` | O(1) | 查越界，抛 `out_of_range` |
| `front()` / `back()` | O(1) | 首尾元素 |
| `data()` | O(1) | 底层指针，永远非空（长度 0 时除外） |
| `size()` | O(1) | **编译期常量**，可用于 `static_assert` |
| `empty()` | O(1) | 编译期就知道结果 |
| `fill(x)` | O(n) | 全部填成 x |
| `swap(other)` | **O(n)** | 逐个交换 |

**没有** `push_back` / `insert` / `erase` / `resize` / `capacity` —— 长度不可变，这些概念都不存在。

`size()` 是编译期常量这点很有用：

```cpp
std::array<int, 5> a{};
static_assert(a.size() == 5);           // 编译期就能检查
int raw[a.size()];                       // 能当数组维度用
```

### 3.3 比 C 数组好在哪

```cpp
void f(int a[5]);              // 骗人：参数实际是 int*，sizeof(a) 是指针大小
void g(std::array<int,5> a);   // 真的是 5 个元素（按值拷贝）
```

C 数组传参会**退化**成指针，函数里丢失长度信息，这是无数越界 bug 的源头。`array` 不退化。

其他优势：

- 知道自己的 `size()`，不用另传一个长度参数
- 可以整体拷贝赋值（`a = b`），C 数组不行
- 有 `begin()` / `end()`，能直接进标准算法和范围 for
- 能当函数返回值（C 数组不能返回）

### 3.4 易错点

1. **`swap` 和赋值都是 O(n)**。`vector` 的 `swap` 是 O(1)，因为只要换堆指针；`array` 的元素就嵌在对象里，没有指针可偷，只能逐个搬。别在循环里随手 `swap` 两个大 `array`。

2. **大 `array` 放栈上会爆栈**。`std::array<int, 1000000>` 是 4 MB，典型线程栈只有 1–8 MB。要么改成 `static`，要么改用 `vector`。

3. **忘了 `{}`**（见 3.1）。

4. **想按长度做重载会失败**：长度是模板参数的一部分，`f(std::array<int,5>)` 收不了 `std::array<int,6>`。要通吃就把长度也做成模板参数：

```cpp
template <std::size_t N>
void f(const std::array<int, N>& a);
```

---

## 4. `deque`：双端队列

**底层**：一组固定大小的块（chunk）+ 一张记录这些块地址的索引表。所以**不是**一整块连续内存。

```text
索引表:  [ptr0][ptr1][ptr2][ptr3]
            ↓     ↓     ↓     ↓
块:      [..xx][xxxx][xxxx][xx..]
           ↑                  ↑
         头部还有空位      尾部还有空位
```

**关键推论**：`&d[0] + 1` 不一定是 `&d[1]`（跨块时就断了），所以**不能当 C 数组用**，也没有 `data()`。

**为什么两端都是 O(1)**：头部插入时，如果当前头块还有空位就直接放；满了就新分配一个块，把地址记进索引表**前端**。已有元素一个都不用动 —— 这正是 `vector` 做不到的（`vector` 头部插入要把所有元素往后挪）。

### 4.1 成员函数

| 函数 | 复杂度 | 说明 |
|------|--------|------|
| `push_front(x)` / `emplace_front(...)` | O(1) | 头部追加 |
| `pop_front()` | O(1) | 删头部，不返回 |
| `push_back(x)` / `emplace_back(...)` | O(1) | 尾部追加 |
| `pop_back()` | O(1) | 删尾部，不返回 |
| `d[i]` / `at(i)` | O(1) | 下标访问 |
| `front()` / `back()` | O(1) | 首尾元素 |
| `insert` / `erase`（中间） | O(n) | 向较近的一端搬 |
| `resize(n)` | O(n) | 改元素个数 |
| `shrink_to_fit()` | O(n) | 归还空闲块，非强制 |

**没有** `capacity()` / `reserve()` / `data()`。前两个不存在是因为 `deque` 没有"一整块容量"的概念，它按需增删块。

### 4.2 迭代器失效：和 vector 很不一样

| 操作 | 迭代器 | 引用 / 指针 |
|------|--------|-------------|
| 头尾插入 | **全部失效** | **仍然有效** |
| 头尾删除 | 只有被删的失效 | 只有被删的失效 |
| 中间插删 | 全部失效 | 全部失效 |

这是整个标准库里最反直觉的一条：**插入时迭代器失效了，引用却还活着**。

原因在于两者存的东西不同。已有元素确实没搬家，所以指向元素的引用和指针依然正确；但 `deque` 的迭代器内部要记住"当前在哪个块、块内第几个、索引表在哪"，索引表一重新分配，这些记录就全乱了。

```cpp
std::deque<int> d{1, 2, 3};
int& r = d[0];
auto it = d.begin();

d.push_front(0);

r;      // 仍然有效，还是那个 1
*it;    // 未定义行为，迭代器已失效
```

### 4.3 什么时候用

**唯一的强理由：需要频繁在头部插删。** `vector` 的 `push_front` 是 O(n)，`deque` 是 O(1)。

它也是 `stack` / `queue` 的默认底层容器 —— 正因为两端操作都便宜。

其余情况优先 `vector`：

| 对比项 | `vector` | `deque` |
|--------|----------|---------|
| 内存连续 | 是 | 否 |
| 下标访问 | 一次寻址 | 两次（先查索引表） |
| 头部插入 | O(n) | O(1) |
| 能给 C 接口 | `data()` | 不行 |
| 预留容量 | `reserve()` | 没有 |

### 4.4 易错点

1. **以为它内存连续**：`memcpy(&d[0], ...)` 或把 `&d[0]` 传给 C 函数，跨块时就读错数据了。要连续内存只能用 `vector`。

2. **插入后继续用旧迭代器**：见 4.2，头尾插入会让**所有**迭代器失效，哪怕元素没搬家。

3. **为了"两端都快"而无脑选它**：如果实际只在尾部操作，`vector` 更快 —— 下标访问少一次间接寻址，内存也更紧凑。

4. **空 `deque` 上调 `front()` / `pop_front()`**：和 `vector` 一样是未定义行为，先判 `empty()`。

---

## 5. `list` / `forward_list`：链表

**底层**：每个元素单独 `new` 一个节点，节点之间用指针串起来。`list` 是双向链表（每节点两个指针），`forward_list` 是单向（一个指针，更省内存）。

```text
list:          nullptr ← [prev|1|next] ⇄ [prev|2|next] ⇄ [prev|3|next] → nullptr
forward_list:            [1|next] → [2|next] → [3|next] → nullptr
```

节点散落在堆上各处，**彼此不相邻**。这一条既是它全部优点的来源，也是全部缺点的来源。

### 5.1 成员函数

| 函数 | 复杂度 | 说明 |
|------|--------|------|
| `push_front(x)` | O(1) | 头部插入 |
| `push_back(x)` | O(1) | 尾部插入（`forward_list` **没有**） |
| `insert(pos, x)` | O(1) | **前提是已经有 pos 迭代器** |
| `erase(pos)` | O(1) | 同上，返回下一个迭代器 |
| `splice(pos, other)` | O(1) | **接管**另一个链表的节点 |
| `remove(value)` | O(n) | 按值删，**真的删掉** |
| `remove_if(pred)` | O(n) | 按条件删，真的删掉 |
| `sort()` | O(n log n) | 成员版 |
| `merge(other)` | O(n) | 合并两个**已排序**链表 |
| `unique()` | O(n) | 去掉**相邻**重复（要先 sort） |
| `reverse()` | O(n) | 反转 |
| `size()` | O(1) | `forward_list` **没有** |

**没有** `operator[]` / `at()` —— 想要第 k 个只能从头走 k 步，标准库不提供这种伪装成 O(1) 的接口。

`forward_list` 还有一组 `_after` 版本（`insert_after` / `erase_after`），因为单向链表拿到一个节点无法回头找前驱，只能操作"它后面那个"。

### 5.2 `insert` 的 O(1) 有前提

这是最容易被误读的复杂度。

```cpp
// O(1)：已经有迭代器了
l.insert(it, 42);

// O(n)：先要找到位置
auto it = std::find(l.begin(), l.end(), target);
l.insert(it, 42);
```

链表插入本身只是改几个指针，确实 O(1)。但**走到那个位置**是 O(n)。只有在遍历过程中顺手插删、或长期持有迭代器时，才真正吃到这个优势。

拿它和 `vector` 比"插入快"往往比错了：`vector` 的 O(n) 是连续内存的 `memmove`，快得离谱；`list` 的 O(n) 查找是每步一次指针跳转 + 缓存未命中。

### 5.3 独门本事：`splice`

把另一个链表的节点整体**接管**过来，只改几个指针，元素一个都不动、一次都不拷贝。

```cpp
std::list<int> a{1, 2, 3}, b{4, 5};

a.splice(a.end(), b);          // 整个 b 接到 a 尾部；b 变空。O(1)
// a = {1,2,3,4,5}, b = {}

std::list<int> c{7, 8, 9};
a.splice(a.begin(), c, c.begin());   // 只搬 c 的第一个元素
```

这是 `vector` 完全做不到的：合并两个 `vector` 必须逐个拷贝或移动。做 LRU 缓存这类"把某节点挪到队首"的操作时，`list::splice` 是 O(1)，无可替代。

### 5.4 成员版算法 vs 标准算法

`list` 自带 `sort` / `remove` / `unique` / `reverse`，和 `<algorithm>` 里的同名货**不是一回事**：

| | 成员版 | 标准算法版 |
|--|--------|-----------|
| `sort` | `l.sort()`，重排指针 | `std::sort` **用不了**（需要随机访问迭代器） |
| `remove` | `l.remove(v)`，真的删除、size 变小 | `std::remove` 只搬不删，要配 `erase` |
| `unique` | `l.unique()`，真的删除 | `std::unique` 只搬不删 |
| `reverse` | `l.reverse()`，改指针 | `std::reverse` 交换元素值 |

**能用成员版就用成员版**：它改指针而不是搬元素，更快，而且语义更符合直觉（真的删掉了）。

### 5.5 迭代器失效：最大的优点

**插入不失效任何迭代器；删除只失效被删的那个。** 其他所有迭代器、引用、指针全部继续有效。

```cpp
std::list<Task> l;
auto it = l.insert(l.end(), task);   // 记下这个位置

l.push_front(other);
l.push_back(another);
// it 依然有效，还指向 task

l.erase(it);   // 只有 it 失效
```

因为节点从不搬家。需要**长期持有元素句柄**时，这是选 `list` 的真正理由 —— 比"插入 O(1)"重要得多。

### 5.6 代价与易错点

1. **缓存极不友好**。每个节点单独 `new`，地址随机，遍历时几乎每步都是缓存未命中。遍历 100 万个 `int`，`vector` 能比 `list` 快一个数量级 —— 理论复杂度一样，实测差距巨大。

2. **内存开销大**。存一个 `int`（4 字节）要搭上两个指针（16 字节）加分配器元数据，实际占用可能是 `vector` 的 5–10 倍。

3. **`forward_list` 没有 `size()`**。这是故意的：维护计数会让 `splice` 从 O(1) 退化成 O(n)。要个数只能 `std::distance(l.begin(), l.end())`，O(n)。

4. **`unique()` 只去相邻重复**。不先排序的话，`{1,2,1}` 去不掉任何东西。

5. **误以为"中间插入多就该用链表"**。先问自己：真的会长期持有迭代器吗？还是只是听说"数组插入很慢"？**实际项目里 `list` 比想象中少用得多**，默认答案仍然是 `vector`。

---

## 6. `stack`：栈（适配器）

### 6.1 什么叫"适配器"

`stack` 不是从头实现的容器，它是**包在别的容器外面的一层壳**，把底层丰富的接口**削减**成只剩栈该有的几个。

```cpp
template <class T, class Container = std::deque<T>>
class stack {
protected:
    Container c;      // 真正存数据的是它
public:
    void push(const T& x) { c.push_back(x); }
    void pop()            { c.pop_back(); }
    T&   top()            { return c.back(); }
    // ...
};
```

所以适配器的意义是**限制**而不是增强：拿到一个 `stack`，你在类型上就被保证不会有人从中间insert，也不会有人乱序遍历。

代价是**没有迭代器** —— 不能范围 for，不能进标准算法，不能打印内容而不破坏它。

### 6.2 成员函数

| 函数 | 复杂度 | 说明 |
|------|--------|------|
| `push(x)` | 同底层 | 压栈 |
| `emplace(args...)` | 同底层 | 原地构造 |
| `pop()` | O(1) | 弹栈，**不返回元素** |
| `top()` | O(1) | 看栈顶，返回 `T&`，**可修改** |
| `size()` / `empty()` | O(1) | — |

后进先出（LIFO）。

```cpp
// 正确用法：先看再弹
if (!s.empty()) {
    auto x = s.top();
    s.pop();
}
```

### 6.3 `pop()` 为什么不返回元素

这是个经典设计问题，值得想明白 —— `priority_queue` 和 `queue` 同理。

假设 `pop()` 返回元素，它必须这样实现：

1. 把栈顶元素**拷贝**到返回值
2. 从底层容器删掉它

如果第 1 步的拷贝构造函数**抛异常**呢？元素已经准备返回但没送到，而第 2 步还没执行 —— 或者更糟，某些实现里已经删了。异常传出去后，这个元素**既没交给调用者，又可能已经从栈里消失**，数据凭空蒸发，无法恢复。

拆成 `top()` + `pop()` 就没这个问题：`top()` 只返回引用不拷贝，拷贝发生在你自己的赋值语句里（失败了栈没动）；`pop()` 只删不返回（不会抛）。两步各自都是**异常安全**的。

### 6.4 换底层容器

```cpp
std::stack<int>                        s1;  // 默认 deque
std::stack<int, std::vector<int>>      s2;  // 用 vector
std::stack<int, std::list<int>>        s3;  // 用 list
```

底层只需要提供 `back()` / `push_back()` / `pop_back()`，所以这三个都行。

`vector` 版通常更快（内存连续），代价是扩容时要搬家。默认选 `deque` 是因为它两端操作稳定 O(1)、不会一次性大搬家。

### 6.5 易错点

1. **空栈上 `top()` / `pop()`** —— 未定义行为，不会自动抛异常。永远先判 `empty()`。
2. **想遍历栈** —— 做不到。真要看内容，要么自己另存一份，要么换 `vector` 手动当栈用（`push_back` / `back` / `pop_back`）。
3. **以为 `pop()` 能拿到值** —— 最常见的手误，`auto x = s.pop();` 直接编译错误。

---

## 7. `queue`：队列（适配器）

同样是适配器，默认底层 `deque`。先进先出（FIFO）。

### 7.1 成员函数

| 函数 | 复杂度 | 说明 |
|------|--------|------|
| `push(x)` / `emplace(...)` | O(1) | 尾部入队 |
| `pop()` | O(1) | **头部**出队，不返回 |
| `front()` | O(1) | 看队头（下一个要出的） |
| `back()` | O(1) | 看队尾（最后进来的） |
| `size()` / `empty()` | O(1) | — |

注意方向：`push` 在尾，`pop` 在头。两个操作在容器的两端，这正是 FIFO 的含义。

```cpp
std::queue<Task> q;
q.push(a);
q.push(b);
q.front();   // a，最先进来的
q.back();    // b，最后进来的
q.pop();     // 删掉 a
```

### 7.2 底层不能用 `vector`

```cpp
std::queue<int, std::vector<int>> q;   // 编译错误
```

底层需要 `front()` / `back()` / `push_back()` / **`pop_front()`**，而 `vector` 没有 `pop_front()` —— 因为那是 O(n)，标准库不提供这种性能陷阱。

能用的是 `deque`（默认）和 `list`。

### 7.3 易错点

1. **空队列上 `front()` / `pop()`** —— 未定义行为，先判 `empty()`。
2. **`front()` 和 `back()` 记反** —— `front` 是"队头"，是**最老**的、下一个要出的那个。
3. **想做优先级队列** —— 那是 `priority_queue`，见第 8 节。`queue` 严格按到达顺序。
4. **多线程里直接用它** —— 标准容器都不是线程安全的。生产者-消费者要自己加锁和条件变量（这是 Week2 的内容）。

---

## 8. `priority_queue`：优先队列 / 堆

**底层**：默认 `vector` + 堆算法（`push_heap` / `pop_heap`）。**默认是大顶堆**：`top()` 是最大的。

| 函数 | 复杂度 | 说明 |
|------|--------|------|
| `push(x)` | O(log n) | 上浮 |
| `emplace(...)` | O(log n) | 原地构造 |
| `top()` | O(1) | 返回 `const&`，**不能改** |
| `pop()` | O(log n) | 删堆顶，**不返回元素** |
| `size()` / `empty()` | O(1) | — |

**它做不到的事**：没有迭代器，没有 `find`，不能按 key 定位并修改内部元素，不能遍历。这几条限制直接决定了 Day04 要用延迟删除。

`top()` 返回 `const&` 是必须的：你要是能改堆顶，堆序就被破坏了。

### 8.1 改比较器

```cpp
// 小顶堆
std::priority_queue<int, std::vector<int>, std::greater<int>> minheap;

// 自定义类型：给 operator<（Day04 用的就是这个）
inline bool operator<(const HeapItem& a, const HeapItem& b) {
    return a.priority < b.priority;   // 返回 true 表示 a 排在 b 后面
}

// lambda 当比较器：注意要把类型和对象都传进去
auto cmp = [](const Task& a, const Task& b) { return a.pri < b.pri; };
std::priority_queue<Task, std::vector<Task>, decltype(cmp)> pq(cmp);
```

比较器的语义要记牢：**`cmp(a, b) == true` 表示 a 的优先级低于 b**（a 排在后面）。所以默认的 `std::less` 得到的是**大顶堆**。写反了最紧急的任务永远出不来。

比较器必须是**严格弱序**：`cmp(a, a)` 必须是 `false`。写成 `<=` 会让堆算法越界访问。

### 8.2 延迟删除（Day04 的核心技巧）

因为不能改内部元素，改优先级的标准做法：

1. 权威数据放 `unordered_map`，附带一个 `version`
2. 改优先级 → map 里 `version++` 并更新值 → 往堆里**再 push 一条**新记录
3. 旧记录留在堆里不管
4. `pop` 时循环检查：堆顶的 `version` 是否等于 map 里的 `version`；不等就丢掉继续弹

```cpp
bool pop_highest(Task& out) {
    while (!heap_.empty()) {
        auto top = heap_.top();
        heap_.pop();
        auto it = latest_.find(top.id);
        if (it == latest_.end() || it->second.version != top.version)
            continue;               // 脏数据，扔掉
        out = {top.id, top.priority, it->second.payload};
        latest_.erase(it);
        return true;
    }
    return false;
}
```

要点：**「在不在」只能问 map**，因为堆里有过期副本，`heap_.size()` 会偏大。

代价：堆会膨胀。更新极其频繁时，可以在脏数据比例过高时重建堆。

---

## 9. `map` / `set`：有序关联容器

**底层**：红黑树（自平衡二叉搜索树）。所有操作 O(log n)，元素按 key **有序**。

`map` 存 `std::pair<const Key, Value>` —— 注意 key 是 `const`，拿到迭代器也改不了 key。`set` 只存 key，且**元素本身就是 const**。

### 9.1 成员函数

| 函数 | 复杂度 | 说明 |
|------|--------|------|
| `m[k]` | O(log n) | 取值；**key 不存在会插入默认值**。`const map` 上不可用 |
| `at(k)` | O(log n) | 取值；不存在抛 `out_of_range` |
| `find(k)` | O(log n) | 返回迭代器，没有则 `end()` |
| `count(k)` | O(log n) | `map`/`set` 里只能是 0 或 1 |
| `contains(k)` | O(log n) | C++20，语义最清楚 |
| `insert({k, v})` | O(log n) | 已存在则**不覆盖**，返回 `pair<iterator, bool>` |
| `insert_or_assign(k, v)` | O(log n) | C++17，存在就覆盖 |
| `emplace(k, v)` | O(log n) | 原地构造 |
| `try_emplace(k, args...)` | O(log n) | C++17，已存在则**不构造** value |
| `erase(k)` | O(log n) | 按 key 删，返回删掉的个数 |
| `erase(it)` | 均摊 O(1) | 按迭代器删，返回下一个迭代器 |
| `lower_bound(k)` | O(log n) | 第一个 **>= k** 的位置 |
| `upper_bound(k)` | O(log n) | 第一个 **> k** 的位置 |
| `equal_range(k)` | O(log n) | 上面两个打包成 pair |

`insert` 的返回值：

```cpp
auto [it, inserted] = m.insert({k, v});
if (!inserted) { /* k 已存在，it 指向原有的那个，v 被丢弃 */ }
```

`try_emplace` 解决了 `emplace` 的一个浪费：`emplace(k, expensive())` 即使 key 已存在，也可能已经把 `expensive()` 构造出来了。`try_emplace` 保证先查再构造。

### 9.2 迭代器怎么用

```cpp
auto it = m.find(id);
if (it == m.end()) { /* 没有 */ }
it->first;    // key（const，不能改）
it->second;   // value（可以改）

for (const auto& [k, v] : m) { /* C++17 结构化绑定 */ }
```

`it` 用 `->` 是因为它**像指针**：解引用才拿到那一行 `pair`。而 `m[k]` 直接拿到的是 value 本身，所以用 `.`。

这就是 Day04 里 `it->second.version` 的来历：`it->second` 是 `Meta` 对象，再 `.version` 取字段。

### 9.3 独门本事：有序遍历和范围查询

```cpp
// 天然按 key 从小到大遍历
for (const auto& [k, v] : m) ...

// 找出所有 key 在 [10, 20) 的元素
for (auto it = m.lower_bound(10); it != m.lower_bound(20); ++it) ...
```

哈希表做不到这两件事 —— 这是选 `map` 而不是 `unordered_map` 的主要理由。

`lower_bound` / `upper_bound` 的区别只在"k 正好存在"时：前者指向 k 自己，后者指向 k 的下一个。

### 9.4 易错点

```cpp
if (m[k] == 0) { }   // 危险：k 不存在时会插入一个 0 进去，map 悄悄变大
if (m.count(k)) { }  // 安全
```

在 `const map&` 上不能用 `operator[]`，正因为它会插入 —— 编译器帮你挡住了这个错误。

计数惯用法 `++m[word]` 是**故意**利用这个特性的（不存在时自动建 0 再加 1），这时它是对的。

### 9.5 迭代器失效

`map` / `set` 的插入**不会**让任何迭代器失效；删除只让被删的那个失效。这点比 `vector` 友好得多，因为节点在堆上各自独立，从不搬家。

```cpp
for (auto it = m.begin(); it != m.end(); ) {
    if (bad(it->second)) it = m.erase(it);   // C++11 起 erase 返回下一个
    else ++it;
}
```

---

## 10. `unordered_map` / `unordered_set`：哈希表

**底层**：桶数组 + 每个桶挂一条链（拉链法）。

平均 O(1)，最坏 O(n)（所有 key 挤进同一个桶）。不排序，**遍历顺序不确定**且可能在 rehash 后改变。

接口和 `map` 大体相同，但：

- **没有** `lower_bound` / `upper_bound`（没有序，就没有范围查询）
- 多了哈希相关的：

| 函数 | 说明 |
|------|------|
| `bucket_count()` | 桶个数 |
| `load_factor()` | 元素数 / 桶数 |
| `max_load_factor(f)` | 超过这个比例就自动 rehash（默认 1.0） |
| `rehash(n)` | 重建成至少 n 个桶 |
| `reserve(n)` | 预留能装 n 个元素的空间，避免多次 rehash |

### 10.1 自定义类型当 key

需要两样东西：**哈希函数** + **相等比较**。

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

`map` 只需要 `operator<`，要求低得多。key 类型复杂时这是选 `map` 的理由。

上面这个 `^` 组合哈希质量一般（`Point{1,2}` 和 `Point{2,1}` 容易撞）。生产代码里更常见的是仿 boost 的写法：

```cpp
seed ^= std::hash<int>{}(v) + 0x9e3779b9 + (seed << 6) + (seed >> 2);
```

### 10.2 迭代器失效

**rehash 会让所有迭代器失效**（元素被重新分配到新桶），但**引用和指针仍然有效**（节点本身没被销毁，只是换了条链）。

插入只在触发 rehash 时才失效迭代器；删除只失效被删的那个。

### 10.3 易错点

1. **依赖遍历顺序** —— 顺序不保证，换编译器、换标准库版本、甚至插入顺序不同都可能变。需要确定顺序就排序输出或改用 `map`。
2. **key 插入后修改其哈希相关字段** —— 从此再也找不回来。这就是 Day04 用不可变的 `int id` 当 key 的原因。
3. **大量插入前忘了 `reserve`** —— 多次 rehash 很贵，每次都要重新算所有 key 的桶位。
4. **以为它总比 `map` 快** —— 元素少（几十个）时，红黑树的常数更小；哈希还要算哈希值。

---

## 11. `map` vs `unordered_map` 怎么选

| 需求 | 选谁 |
|------|------|
| 只按 key 查、增、删 | `unordered_map`（平均更快） |
| 需要按 key 有序遍历 | `map` |
| 需要范围查询 / `lower_bound` | `map` |
| key 不好写哈希函数 | `map`（只要 `operator<`） |
| 要求稳定的最坏复杂度（实时系统） | `map`（保证 O(log n)） |
| 元素很少（几十个以内） | `map`，甚至 `vector<pair>` 线性查找可能更快 |

Day04 调度器按 id 查、不需要排序 → `unordered_map`。

---

## 12. `multimap` / `multiset` / `unordered_multi*`

允许**同一个 key 出现多次**。底层结构和对应的单值版本完全一样（红黑树 / 哈希表），只是去掉了"key 唯一"这条约束。

典型用途：一个学生对应多门成绩、一个事件类型对应多个回调、一个时间戳对应多条日志。

### 12.1 和单值版本的接口差异

| | `map` | `multimap` |
|--|-------|-----------|
| `operator[]` | 有 | **没有** |
| `at(k)` | 有 | **没有** |
| `insert` 返回 | `pair<iterator, bool>` | 只有 `iterator` |
| `insert` 会失败吗 | 已存在则不插入 | **永远成功** |
| `count(k)` | 0 或 1 | 任意个 |
| `find(k)` | 那一个 | **某一个**，不保证是哪个 |
| `erase(k)` | 删 0 或 1 个 | 删**全部**匹配的 |

`operator[]` 和 `at()` 消失是必然的：一个 key 对多个 value，`m[k]` 该返回哪一个？没有合理答案，索性不提供。

`insert` 不再返回 `bool`，因为插入永远成功，没有"已存在所以放弃"这回事。

### 12.2 取出同一个 key 的全部元素

`find(k)` 只给你其中某一个，要拿全部得用 `equal_range`：

```cpp
auto [first, last] = mm.equal_range(k);
for (auto it = first; it != last; ++it) {
    // it->first 是 k（每个都一样）
    // it->second 是其中一个 value
}
```

`equal_range` 返回一对迭代器，等价于 `{lower_bound(k), upper_bound(k)}`。key 不存在时两者相等，循环自然一次都不执行 —— 不用额外判断。

也可以配合 `count`：

```cpp
std::cout << k << " 有 " << mm.count(k) << " 条记录\n";
```

但 `count` 是 O(log n + 个数)，只为判断"有没有"的话，`mm.find(k) != mm.end()` 更省。

### 12.3 删除要当心

```cpp
mm.erase(k);        // 删掉所有 key == k 的，返回删除个数
mm.erase(it);       // 只删迭代器指的那一个，返回下一个迭代器
```

这是最容易出事的地方：**想删一条，结果按 key 删光了一片**。只删特定一条必须先定位到迭代器。

```cpp
// 删掉 key 为 k、value 为 v 的那一条
auto [first, last] = mm.equal_range(k);
for (auto it = first; it != last; ++it) {
    if (it->second == v) { mm.erase(it); break; }
}
```

### 12.4 顺序保证

`multimap` / `multiset` 里同 key 的元素**保持插入顺序**（C++11 起标准保证）。所以 `equal_range` 遍历出来的顺序就是当初插入的顺序，可以依赖。

`unordered_multimap` 的同 key 元素会挨在一起（同一个桶里），但**跨 key 的整体顺序不保证**，rehash 后还可能变。

### 12.5 什么时候改用 `map<K, vector<V>>`

很多场景下这个替代方案更好用：

```cpp
std::map<int, std::vector<Score>> scores;
scores[id].push_back(s);          // 直接追加
for (const auto& s : scores[id])  // 直接遍历，不用 equal_range
```

| | `multimap<K,V>` | `map<K, vector<V>>` |
|--|-----------------|---------------------|
| 取某 key 全部值 | `equal_range` + 循环 | 直接拿到 `vector` |
| 语法直观度 | 差 | 好 |
| 值在内存里 | 分散在树节点 | 连续（vector 内） |
| 整体按 key 有序遍历 | 天然 | 天然 |
| 单条删除 | 需先定位迭代器 | 在 vector 里删，O(n) |

**多数情况下选后者**。`multimap` 的优势场景比较窄：需要把所有 `(key, value)` 对当成一个扁平的有序序列整体遍历时。

---

## 13. 几个容易混的语义

### `pop()` / `erase()` 不返回元素

`stack::pop`、`queue::pop`、`priority_queue::pop`、`vector::pop_back` 都只删不返回。要先 `top()` / `front()` / `back()` 拿到值再 `pop()`。原因见 §6。

例外：`map::erase(it)` 和 `vector::erase(it)` 返回的是**下一个迭代器**，不是被删的元素。

### `clear()` 不一定还内存

`vector::clear()` 之后 `size()==0`，但 `capacity()` 不变。节点式容器（`list` / `map`）的 `clear()` 则真的把节点都 `delete` 了。

### `[]` 会创建元素（关联容器）

`map` / `unordered_map` 的 `operator[]` 在 key 不存在时会**插入**默认构造的值。只是查询请用 `count` / `find` / `contains`。

`vector` 的 `[]` 则相反：越界不会自动扩，是未定义行为。同一个符号，两种容器里语义完全相反。

### `size()` 是元素数，`capacity()` 是容量

只有 `vector` / `string` 有 `capacity()`。`deque` / `list` / 关联容器都没有。

---

## 14. 迭代器失效速查表

| 容器 | 插入 | 删除 |
|------|------|------|
| `vector` / `string` | 扩容则**全失效**；否则插入点之后失效 | 删除点之后失效 |
| `deque` | 头尾插：迭代器失效、**引用有效**；中间插：全失效 | 头尾删：只失效被删的；中间删：全失效 |
| `list` / `forward_list` | 不失效 | 只失效被删的 |
| `map` / `set` | 不失效 | 只失效被删的 |
| `unordered_*` | rehash 时迭代器全失效、**引用有效**；否则不失效 | 只失效被删的 |
| `array` | 不适用（不能增删） | 不适用 |

**记忆线索**：只要元素可能被**搬家**（连续内存重新分配、rehash），迭代器就危险；节点式容器（`list`、`map`）元素从不搬家，所以安全。引用比迭代器更耐活 —— `deque` 和 `unordered_*` 只是改了"索引结构"，元素本身没动。

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

**大部分情况下答案就是 `vector`**。它内存连续、缓存友好、没有额外指针开销。换别的容器要有具体理由。

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
