# C++ 标准库容器速查

和 Day04 调度器一起看：`priority_queue` 是 `heap_`，`unordered_map` 是 `latest_`。  
几乎所有容器都有：`size()`、`empty()`、`clear()`。  
`stack` / `queue` / `priority_queue` 没有迭代器；其余常用 `begin()` / `end()`。

---

## 一共有哪些盒子

**顺序容器**

| 类型 | 干什么 |
|------|--------|
| `vector` | 动态数组。用得最多。Day03 DynArray 是它的简化版 |
| `string` | 专门装字符的动态数组 |
| `array` | 长度编译期就定死的数组 |
| `deque` | 两头都能高效插删 |
| `list` | 链表。中间插删方便，不能按下标乱跳 |

**适配器（包在别的容器外面）**

| 类型 | 干什么 |
|------|--------|
| `stack` | 栈，后进先出 |
| `queue` | 普通队列，先进先出 |
| `priority_queue` | 优先队列（堆） |

**有序关联（红黑树，按钥匙排序）**

| 类型 | 干什么 |
|------|--------|
| `map` | 钥匙 → 值，钥匙有序 |
| `set` | 只有钥匙，有序、不重复 |
| `multimap` / `multiset` | 允许同一把钥匙出现多次 |

**无序关联（哈希表，不排序）**

| 类型 | 干什么 |
|------|--------|
| `unordered_map` | 钥匙 → 值 |
| `unordered_set` | 只有钥匙，查「在不在」很快 |
| `unordered_multimap` / `unordered_multiset` | 允许重复钥匙 |

岗上最常碰：`vector`、`string`、`unordered_map`、`map`、`priority_queue`、`queue`。

选法三句：要下标、尾部增减 → `vector`；按 id 查、不用排序 → `unordered_map`；要有序或范围 → `map`/`set`；永远拿最大/最小 → `priority_queue`。

---

## 各盒子常用函数

### `vector` / `string`

| 函数 | 作用 |
|------|------|
| `push_back(x)` | 尾部追加 |
| `pop_back()` | 删最后一个 |
| `v[i]` / `at(i)` | 下标；`at` 越界会抛异常 |
| `front()` / `back()` | 第一个 / 最后一个 |
| `reserve(n)` | 预留容量（`vector`） |
| `resize(n)` | 真改成 n 个元素 |
| `capacity()` | 这块内存能装几个（`vector`） |
| `data()` | 底层指针 |
| `c_str()` | 只 `string`：C 风格字符串 |

`string` 还有 `+`、`substr`、`find`。

### `array`

`[i]`、`size()`、`fill(x)`、`data()`。没有 `push_back`。

### `deque`

有下标、`push_back` / `pop_back`，另外：

| 函数 | 作用 |
|------|------|
| `push_front(x)` | 头部插入 |
| `pop_front()` | 删第一个 |

### `list`

`push_front` / `push_back` / `insert` / `erase` / `remove`。没有可靠的 `v[i]`。

### `stack`

| 函数 | 作用 |
|------|------|
| `push(x)` | 压入 |
| `pop()` | 去掉栈顶（不返回元素） |
| `top()` | 看栈顶 |

### `queue`

| 函数 | 作用 |
|------|------|
| `push(x)` | 入队（尾） |
| `pop()` | 出队（头），不返回元素 |
| `front()` | 看队头 |
| `back()` | 看队尾 |

### `priority_queue`（`heap_`）

| 函数 | 作用 |
|------|------|
| `push(x)` | 塞进去 |
| `top()` | 看当前最大（或最小，看比较器） |
| `pop()` | 扔掉当前最大 |

没有 `[i]`，没有 `find`，不能改中间某个元素。

### `map` / `unordered_map`（`latest_` 是后者）

| 函数 | 作用 |
|------|------|
| `m[k] = v` | 写入；没有这把钥匙会新建 |
| `at(k)` | 取；没有这把钥匙抛异常 |
| `count(k)` | 有没有（0 或 1） |
| `find(k)` | 找到迭代器；没有则 `end()` |
| `insert` / `emplace` | 插入 |
| `erase(k)` 或 `erase(it)` | 删 |

迭代器：`it->first` 钥匙，`it->second` 值。  
`map` 按钥匙排序；`unordered_map` 不排序、平均查找更快。

### `set` / `unordered_set`

`insert`、`erase`、`count`、`find`、`size`。没有 `second`，迭代器解出来就是钥匙。

`multi*` 系列：同一把钥匙可以有多份，`count` 可能大于 1。一般不用 `[]`。

---

## 调度器对照

```text
heap_.push(x)      heap_.top()      heap_.pop()      heap_.empty()
latest_.count(id)  latest_[id]      latest_.find(id) latest_.erase(it)
```

`pop()` / `erase()` 都是删掉，不把元素 return 给你；要先 `top()` / `it->second` 看一眼再删。
