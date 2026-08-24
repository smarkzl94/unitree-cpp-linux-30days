# Day01 · MyString 与目录权限（详细预习）

日期：2026-08-18

预习目标：先会在终端里走动、看权限、查 man；再搞懂 MyString 的所有权、深/浅拷贝、五件套和移动。上机先练命令，再写代码。

---

# 一、Linux：目录、权限与 man

写 C++ 之前，先要会「人在哪、目录里有什么、谁能碰」。相对路径、编译命令、以后的脚本，全靠这个。迷路了先 `pwd`，看不清文件先 `ls -la`，忘了参数就 `man`。

## 1. `pwd`：我现在在哪

```bash
pwd
```

打印 **当前工作目录**（print working directory）。终端里所有相对路径（`g++ MyString.cpp`、`./a.out`、`ls main.cpp`）都相对这个目录，不是相对「资源管理器里打开的文件夹」。

进错目录再编译，会 `No such file or directory`。先 `pwd` 确认，再动手。

## 2. `cd`：换地方

```bash
cd /mnt/d/unitree-cpp-linux-30days   # 绝对路径：从根写起
cd week1-cpp-basics/day01-mystring     # 相对路径：相对 pwd
cd ..                                  # 上一级
cd ~                                   # 家目录
cd -                                   # 回到上一次所在目录（可选会）
```

`cd` 失败常见两种：路径打错；目录没有 **执行权限 `x`**（下一节）。  
`.` 是当前目录，`..` 是父目录。`cd` 不带参数通常等于 `cd ~`。

## 3. `ls` 与 `ls -la`

```bash
ls           # 只列名字
ls -l        # 详细：权限、所有者、大小、时间
ls -a        # 含隐藏（`.` 开头，如 `.git`、`.`、`..`）
ls -la       # 两者合在一起，今天必练
```

**易错**：`-l` 是 **long listing（详细列表）**，不是「长路径」。路径写多长跟 `-l` 无关。

对着自己的 `MyString.cpp` / `TASK.md` 看一列权限，不要只背字母。

## 4. 读懂权限串 `rwx`

`ls -la` 第一列长这样：

```text
drwxr-xr-x  5 user user 4096 Aug 18 10:00 day01-mystring
-rw-r--r--  1 user user  312 Aug 18 10:00 MyString.h
```

拆开：

```text
类型  owner  group  others
 d     rwx    r-x    r-x
 -     rw-    r--    r--
```

- 第一位类型：`-` 普通文件，`d` 目录，`l` 软链接
- 后九位：主人 / 同组 / 其他人，各三位 `rwx`

| 权限 | 对文件 | 对目录 |
|------|--------|--------|
| `r` | 读内容 | 列出里面有什么（`ls`） |
| `w` | 改内容 | 在目录里增删、改名字 |
| `x` | 当程序执行 | **能 `cd` 进去**、能穿过它访问里面的文件 |

目录缺 `x`：就算有 `r`，`cd` 也会 `Permission denied`。文件缺 `x` 就不能 `./a.out`（Day04 给脚本加 `chmod +x` 就是补这个位）。

数字写法（Day04 的 `chmod` 会用）：`r=4`、`w=2`、`x=1`，三组相加。

- `644` = `rw-r--r--`（源码常见）
- `755` = `rwxr-xr-x`（可执行脚本、目录常见）

## 5. `man`：忘了参数就查手册

```bash
man ls
man man
```

- **空格**下翻，**b** 上翻
- **`q` 退出**（不是在手册里敲 `exit` 或 Ctrl+C 乱按）
- `/关键字` 搜索，`n` 下一个匹配，`N` 上一个

手册分节：第 1 节是用户命令。不确定时 `man 1 ls`。预习时至少把 `man ls` 翻到 `-l`、`-a` 那两段。

## Linux 口述（预习时自己答）

1. `ls` 和 `ls -la` 差在哪？`-l` 是什么意思？  
   `ls` 只列名；`-l` 是详细列表（不是长路径）；`-a` 含隐藏文件。
2. 目录的 `x` 和文件的 `x` 差在哪？  
   文件 `x` = 可当程序执行；目录 `x` = 能 `cd` 进去。
3. `man` 怎么退出、怎么搜？  
   `q` 退出；`/关键字` 搜索，`n` 下一个。

## Linux 上机（预习不用敲）

进入 `week1-cpp-basics/day01-mystring/`，练 `pwd` / `cd` / `ls -la` / `man ls`，写当天 `linux-notes.md`。

---

# 二、C++：MyString 与 Rule of Five

## 为什么要手写 MyString

机器人/中间件里大量对象「拥有一块资源」（缓冲、句柄、配置字符串）。不管拷贝和析构，就会泄漏或 **double-free**。面试常问「三件套 / 五件套」。

`MyString` 在堆上自己管一块 `char*`：活着就管，死了就 `delete[]`。这是后面所有权问题的最小模型。Day02 的智能指针，也是把这件事交给类型自动做。

## 1. RAII

**Resource Acquisition Is Initialization**：资源获取即初始化。申请绑在构造，释放绑在析构。离开作用域（含提前 `return`、抛异常）自动清理，比到处手写 `delete` 安全。

智能指针（Day02）是同一种思想，只是标准库已经写好了。

## 2. 对象里有什么

```text
MyString a("hello");
  data_ ──► [h][e][l][l][o][\0]   ← new[] 出来的
  size_ = 5                       ← 不含 '\0'
```

你要保证：构造申请、析构释放、拷贝/移动时想清楚这块内存归谁。

空串两种合法表示（**选一种贯彻到底**）：

- `data_ = nullptr`，`c_str()` 返回静态的 `""`
- 或 `new char[1]`，里面放 `'\0'`

上机用第一种：默认构造 `nullptr` + `size_=0`。两种混用也行，但比较和 `strcpy` 必须先统一成「有效 C 串」。

## 3. 浅拷贝 vs 深拷贝（最重要）

编译器默认生成的拷贝往往是**按位抄指针**：

```text
a.data_ ──► [hello\0]
b.data_ ──┘     两个人指向同一块
```

后果：改 A 等于改 B；`a`、`b` 析构各 `delete[]` 一次 → **double-free** 崩。这就是浅拷贝。

**深拷贝**：给 B **重新 `new[]`**，把字符抄过去。两块独立内存，互不影响。

`main` 里 `assert(a.c_str() != b.c_str())` 查的是**指针地址不同**，不是字符串内容不同。内容应该相同，地址必须不同。

## 4. Rule of Five

自己管裸资源时，这五个通常一起写（或明确 `= delete`）：

| 成员 | 何时调用 | 做什么 |
|------|----------|--------|
| 析构 `~MyString()` | 离开作用域 | `delete[] data_`（`delete[] nullptr` 安全） |
| 拷贝构造 | `MyString b(a);` | 深拷贝 |
| 拷贝赋值 | `b = a;`（两边都已存在） | 防自赋值 → 放旧 → 深拷贝 |
| 移动构造 | `MyString b(std::move(a));` | 偷指针，源置空 |
| 移动赋值 | `b = std::move(a);` | 放自己的 → 偷 → 源置空 |

C++98 讲三件套（析构 + 拷贝构造 + 拷贝赋值）；C++11 加上移动变成五件套。漏写其中一个，编译器可能按「浅拷贝」补上，事故从这里来。

`= default` 表示「我确认用编译器默认生成」；`= delete` 表示「禁止」。有裸指针时默认生成几乎一定是错的，所以要自己写或删掉。

### 拷贝赋值为什么多一步

拷贝构造：对象还不存在，只要新建一份。  
拷贝赋值：左边 **已经有**旧内存：

```cpp
MyString& MyString::operator=(const MyString& other) {
    if (this == &other) return *this;   // s = s
    delete[] data_;
    if (other.data_ == nullptr) {       // 空源不能 strcpy
        data_ = nullptr;
        size_ = 0;
    } else {
        size_ = other.size_;
        data_ = new char[size_ + 1];
        strcpy(data_, other.data_);
    }
    return *this;
}
```

不做自赋值检查：先删自己再从自己读 → 读已释放内存。  
`other.data_ == nullptr` 时 `strcpy` 是未定义行为。

### 移动：偷指针，不是再拷一份

大块内存再深拷贝很贵。源马上作废时，**移交指针**：

```text
移动后：b 指向原来 a 的那块；a.data_ = nullptr；a.size_ = 0
```

源必须可安全析构（析构遇到 `nullptr` 是空操作）。`noexcept` 告诉容器：搬迁时优先走移动——后面 `vector` 扩容就靠这个。

何时走移动：`std::move`、右值/临时量；`return` 局部对象常被移动或 **RVO/NRVO** 直接省略拷贝。

```cpp
MyString make() {
    MyString s("ok");
    return s;  // 通常移动或直接构造到调用方
}
```

局部 `return` 不必再写 `return std::move(s)`，有时反而妨碍 NRVO。

## 5. 接口约定

- `c_str()`：借出 C 串（`const char*`，以 `'\0'` 结尾），**不交出所有权**。调用方不能 `delete[]` 它。
- 空串也必须返回有效指针：`return data_ ? data_ : "";`。对 `nullptr` 做 `strcmp` / 打印会崩。
- `size()`：字符数，不含 `'\0'`。
- `operator==`：比**内容**。`MyString()` 和 `MyString("")` 都应相等；用 `strcmp(c_str(), other.c_str())` 最省事，两种空表示也能对上。

## 6. 易错点（Day01 已经踩过的）

1. **`data_ = '\0'`**：`data_` 是指针，`'\0'` 当整数 0，这是赋成空指针，不是往缓冲区写空字符。写字符应 `data_[0] = '\0'`。
2. **两个空表示**：`nullptr` 与 `""` 都是空串。比较必须走 `c_str()`，不能只比指针是否相等。
3. **拷贝赋值对空源**：`other.data_ == nullptr` 时不能 `strcpy`。
4. **循环下标**：`size_` 是 `std::size_t`，用 `int i` 会符号比较告警；和下标对齐用 `std::size_t`。
5. **拷贝构造漏 `'\0'`**：只抄 `size_` 个字符后必须 `data_[size_] = '\0'`，否则不是合法 C 串。
6. **移动后忘置空**：源析构还会 `delete[]` 同一块 → 又是 double-free。

## C++ 面试口述（预习时自己答一遍）

1. **深拷贝 vs 浅拷贝？**  
   浅：只拷指针，共享一块堆，析构 double-free。深：各自 `new[]` 一份内容。
2. **何时移动？`return` 局部呢？**  
   `std::move`、右值。`return` 局部通常移动或被 RVO/NRVO 优化掉。
3. **空串 `c_str()` 为什么不能返回 `nullptr`？**  
   调用方会当 C 串用（`strcmp`、打印）。空串也要是有效的 `""`。
4. **为何要 `= default` / `= delete`？**  
   明确「编译器生成」或「禁止」，避免漏写其中一个特殊成员。

## C++ 上机（预习不用写）

见 `week1-cpp-basics/day01-mystring/TASK.md`：

- 默认 / `const char*` 构造、五件套、`c_str()` / `size()` / `operator==`
- assert：空串、深拷贝独立、移动后源可析构、自赋值不崩
