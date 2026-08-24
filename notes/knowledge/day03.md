# Day03 · DynArray 与管道检索（详细预习）

日期：2026-08-20

预习目标：先学重定向和管道，用 `grep` / `find` / `wc` 在代码树里找符号；再搞懂 `vector` 的 size/capacity、扩容和迭代器失效。上机先练命令，再写 DynArray。

---

# 一、Linux：重定向、管道、在代码树里搜

编译器和测试会往屏幕打字。今天要把输出接到文件里，或接到下一个命令。这是以后写脚本、看日志的底子。

程序默认三根「管子」：

- **stdin (0)**：键盘输入
- **stdout (1)**：正常输出
- **stderr (2)**：错误信息（编译报错走这里）

## 1. 重定向：把管子接到文件

```bash
g++ main.cpp -o app          # 成功信息在屏幕，失败也在屏幕
g++ main.cpp -o app 2> err.txt    # 错误写进 err.txt
./app > out.txt              # stdout 写入（覆盖）
./app >> out.txt             # 追加，不覆盖已有内容
./app > out.txt 2>&1         # stdout 和 stderr 都进 out.txt
./app < input.txt            # 从文件读，当键盘
```

`>` 会**清空再写**。手滑 `>` 到重要文件会把内容干掉，追加用 `>>`。

`2>&1` 读法：把 2 接到 1 **当前**所指的地方。顺序有讲究：先 `> out.txt` 再 `2>&1`，两边才进同一个文件。

## 2. 管道 `|`：上一个的输出当下一个的输入

```bash
ps aux | grep mystring
ls -la | less
```

左边 stdout 接到右边 stdin。Day02 的 `ps aux | grep ...` 就是管道。  
管道默认不管 stderr：编译报错仍可能直接打在屏幕上，要一起传就 `2>&1 |`。

和重定向差别：`>` 接到**文件**；`|` 接到**另一个程序**。

## 3. `grep`：在文本里找行

```bash
grep "push_back" *.cpp
grep -n "TODO" MyString.cpp          # -n 带行号
grep -R "emplace_back" ..            # -R 递归目录
grep -R --include='*.cpp' --include='*.h' "size_" .
```

默认打印**整行**。找不到退出码是 1（脚本里要小心）。  
大小写：`-i`。只要文件名：`-l`。

在本仓库里练：从 `week1-cpp-basics` 搜 `c_str`、`use_count`、`TODO`，看自己写过的符号出现在哪。

## 4. `find`：按名字找文件

```bash
find . -name "TASK.md"
find . -name "*.cpp"
find week1-cpp-basics -type f -name "*test*"
```

`grep` 搜**内容**，`find` 搜**路径/文件名**。先 `find` 定位文件，再 `grep` 看里面写了什么，是排障常用组合：

```bash
find . -name "*.cpp" | xargs grep -n "operator="
```

## 5. `wc`：数行、词、字节

```bash
wc -l main.cpp                 # 行数
wc -l *.cpp
find . -name "*.cpp" | wc -l   # 有多少个 cpp 文件（管道数的是「文件名行数」）
grep -R "TODO" . | wc -l       # 还有多少 TODO
```

`-l` 行，`-w` 词，`-c` 字节。预习时对当天目录跑一遍，心里有个数。

## Linux 口述（预习时自己答）

1. `>` 和 `>>`、`2>` 差在哪？  
   `>` 覆盖 stdout；`>>` 追加；`2>` 重定向 stderr。
2. 管道和重定向差在哪？  
   `|` 接到下一个程序；`>` 接到文件。
3. `grep` 和 `find` 各找什么？  
   `grep` 找文件内容；`find` 找文件名/路径。

## Linux 上机（预习不用敲）

在 `week1-cpp-basics/` 下用管道串 `find` / `grep` / `wc` 搜符号，重定向一份结果到当天 `linux-notes.md` 旁的小文件也行。

---

# 二、C++：DynArray 与 `vector` 原理

## 为什么要手写 DynArray

`std::vector` 是用得最多的容器。轨迹点、传感器批量数据、日志行缓冲，岗上都是它。不懂扩容和迭代器失效，热路径上会写出「偶尔崩、ASan 才抓得到」的代码。

DynArray 就是剥开的 `vector`：一块堆数组 + 两个数字（或三个指针）。上机自己扩容一次，比背文档记得住。

## 1. 三指针（概念）模型

逻辑上 `vector` / DynArray 管三件事：

```text
begin          size 末尾        capacity 末尾
  |                |                  |
  v                v                  v
 [e0][e1][e2][e3][  ][  ][  ][  ]
  已用 size=4           空闲
  整块容量 capacity=8
```

- **begin**：堆上数组的起点
- **size**：已经放了几个元素（`size()`）
- **capacity**：这块内存能装几个（`capacity()`）

`size() <= capacity()` 永远成立。`size == capacity` 时再 `push_back`，就要扩容。

对应接口（上机要自己写）：`push_back` / `size` / `capacity` / `operator[]`。

## 2. 扩容时发生了什么

```text
旧块（满了）:  [a][b][c][d]     size=4, cap=4
新块（常 2 倍）: [a][b][c][d][ ][ ][ ][ ]   把旧元素移动/拷贝过来
然后 delete[] 旧块
```

步骤：

1. 分配更大块（常见 2 倍；实现可自己定，但必须增长）
2. 把旧元素 **move**（有 `noexcept` 移动时）或 copy 到新块
3. 析构旧元素，释放旧块
4. 把新元素放进去，`size + 1`

单次扩容是 O(n)，但不是每次 `push_back` 都扩。均摊下来每次插入仍近似 **O(1)**：1+2+4+…+n ≈ 2n。

Day01 的 `noexcept` 移动在这里值钱：`vector` 扩容时优先移动，少一次深拷贝。

## 3. 迭代器 / 引用 / 指针失效

扩容后**旧内存已经释放**。原先拿到的迭代器、引用、指针全指向垃圾：

```cpp
std::vector<int> v = {1, 2};
int& r = v[0];
auto it = v.begin();
v.push_back(3);
v.push_back(4);   // 很可能已经扩容
// r、it 此时可能悬空 —— 不要再用
```

不是只有扩容：在中间 `insert` / `erase` 也会让后面的迭代器失效。规则可记：

- **扩容**：所有迭代器、引用、指针失效
- **未扩容的 `push_back`**：迭代器一般还有效，但别依赖「这次没扩」——下次就可能扩
- 循环里 `push_back` 还拿着旧 `it` 往前走 → 经典踩坑

上机要求：扩容前后打印元素的**地址**，写进笔记，亲眼看地址变了。

## 4. `reserve`：预知大小就先订座位

```cpp
std::vector<Point> pts;
pts.reserve(1000);      // capacity >= 1000，size 仍是 0
for (int i = 0; i < 1000; ++i)
    pts.push_back(read_point());
```

`reserve(n)` 保证接下来至少 n 次 `push_back` 不必再分配（从空开始算）。实时控制、传感器批量入队，岗上常 `reserve`，避免控制周期里来一次大分配。

`resize(n)` 不同：它真的构造 n 个元素，`size` 变成 n。只要容量、不要元素，用 `reserve`。

## 5. `emplace_back` vs `push_back`

```cpp
v.push_back(MyString("hi"));     // 先构造临时量，再拷贝/移动进 vector
v.emplace_back("hi");            // 在 vector 尾部原地构造，少一次临时对象
```

`emplace_back` 把构造参数**转发**到元素的构造函数。对有堆资源的类型（`MyString`、`unique_ptr`）更明显。简单 `int` 两者几乎一样。

口述时不要说「emplace 一定更快」：类型平凡时优化会把差距吃掉。原则是：**能原地构造就 emplace**，语义更直接。

## 6. 易错点（看懂再上机）

1. **循环中 `push_back` 后继续用旧迭代器**  
   扩容后是悬空。需要下标循环，或先 `reserve` 够。
2. **`vector<bool>`**  
   它不是真正的 `bool` 数组，是比特压缩，`operator[]` 返回代理对象。不要当普通 `vector<T>` 想。
3. **频繁在头部插入**  
   每次 O(n) 搬元素。头插该用 `deque` 或别的结构。
4. **`capacity` 不会随 `clear` 缩小**  
   `clear` 只把 `size` 变 0，内存还在。要还给系统用 `shrink_to_fit`（不保证立刻缩）。
5. **自己写 DynArray 忘了析构里 `delete[]`**  
   和 Day01 一样，对象拥有那块数组。

## C++ 面试口述（预习时自己答一遍）

1. **`size` 和 `capacity` 区别？**  
   `size` 是已有元素个数；`capacity` 是这块内存能装几个。`size <= capacity`。
2. **扩容后引用为什么悬空？**  
   旧块被释放，引用还指着旧地址。
3. **如何减少扩容次数？**  
   预知规模就 `reserve`；避免在热循环里一次次从 0 涨上去。
4. **`emplace_back` 和 `push_back`？**  
   emplace 原地构造；push 往往先有临时对象再放入。

## C++ 上机（预习不用写）

见 `week1-cpp-basics/day03-dynarray/TASK.md`：

- `push_back` / `size` / `capacity` / `operator[]`，满时扩容（例如 2 倍）
- 扩容后旧迭代器失效写进笔记（打印地址最清楚）
