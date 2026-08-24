# Day02 · 智能指针（详细预习）

日期：2026-08-19

预习目标：先学 Linux 进程观察，再学三种智能指针。上机先练命令，再写代码。

---

# 一、Linux：进程与信号

程序跑起来就是**进程**，系统用 **PID**（进程号）标识它。编译、跑测试，都会出现进程。今天要会：列出进程、盯着看、发信号让它退出。

## 1. `ps`：拍一张进程名单

```bash
ps aux          # 常见详细列表
ps -ef          # 另一套详细格式
ps aux | grep mystring_test   # 找自己的程序（管道后面会系统学）
```

先认三列：**PID**、命令名、谁在跑。上机时对着**自己的**编译/测试进程看，不要只背命令。

`ps` 是快照：敲一下出一屏，不会自己刷新。

## 2. `top`：连续刷新的现场

```bash
top
```

CPU、内存实时变。按 `q` 退出（和 `man` 一样）。  
`htop` 更好看，没有就用 `top`。

和 `ps` 的差别：`ps` 拍照，`top` 看直播。

## 3. `kill`：给进程发信号，不是只能「杀死」

```bash
kill PID          # 默认 SIGTERM(15)：请你退出，进程可以做清理
kill -9 PID       # SIGKILL(9)：内核直接干掉，来不及析构/关文件
```

习惯：先 `kill`，等几秒还在再考虑 `-9`。机器人/服务上乱 `-9` 可能留下半开的设备句柄。

对照后面的 C++：`SIGTERM` 有点像「请析构」；`SIGKILL` 像直接拔电源。

## Linux 口述（预习时自己答）

1. `ps` 和 `top` 差在哪？  
   `ps` 快照；`top` 持续刷新，`q` 退出。
2. `kill` 和 `kill -9` 差在哪？  
   默认 TERM，进程能清理；`-9` 是 KILL，来不及析构。

## Linux 上机（预习不用敲）

对自己的进程练 `ps` / `top` / `kill`，写当天目录 `linux-notes.md`。

---

# 二、C++：智能指针

## 今天要用的基础（看智能指针之前）

### 栈上的变量 vs 堆上的对象

```cpp
int x = 3;              // 栈：离开 {} 自动没了，不用 delete
int* p = new int(3);    // 堆：new 出来的，必须有人 delete，否则泄漏
delete p;
```

栈像课桌抽屉，出教室（出 `{}`）自动清空。堆像租的储物柜，不还钥匙（`delete`）柜子就一直占着。

智能指针就是「租柜合同」：合同对象离开 `{}` 时自动帮你还柜子。

### 作用域 `{ }`

```cpp
{
    auto p = std::make_unique<int>(1);
}   // 走到这里，p 销毁 → 自动 delete 那个 int
```

谁在哪一层大括号里出生，就在那一层结束时死。析构、智能指针释放，都发生在「离开大括号」。

### `auto`

编译器根据右边推断类型：`auto p = std::make_unique<int>(42);` 等价于写出很长的 `std::unique_ptr<int> p = ...`。今天可以当「懒得写类型」用。

### `.` 和 `->`

```cpp
obj.foo();    // obj 是对象本身
ptr->foo();   // ptr 是指针，先找到对象再调 foo
```

`unique_ptr` / `shared_ptr` 用法像指针：`p->open();` 或 `(*p).open();`。

### `#include <memory>` 和 `std::`

智能指针在标准库头文件 `<memory>` 里。名字在 `std` 命名空间：`std::make_unique`。没有 `#include` 或漏了 `std::` 会编不过。

### 模板皮毛 `unique_ptr<T>`

`T` 是「指向什么类型」。`unique_ptr<int>` 管一个 `int`；上机是 `unique_ptr<Device>` 管一个 `Device`。先当填空：尖括号里写你要管的类名。

### 类里的成员

```cpp
class Node {
public:
    std::shared_ptr<Node> peer;  // 成员：每个 Node 对象自带一个 peer
};
```

「A 持有指向 B 的 `shared_ptr`」= A 这个对象里面有一个名叫 `peer` 的成员，类型是 `shared_ptr<B>`。循环引用就是 A.peer 指向 B、B.peer 指向 A。

---

## 为什么需要智能指针

Day01 的 `MyString` 自己 `new[]` / `delete[]`。任何一条路径忘了 `delete`（提前 `return`、抛异常）就会泄漏。

智能指针把「这块内存归谁管」写进类型里：**对象销毁时自动释放**，还是 RAII，只是标准库已经写好了。

上机任务：用它们管一个「模拟设备」，并亲手制造、再修复循环引用。

## 1. `unique_ptr<T>`：独占

**含义**：同一时刻只有一个主人。不能拷贝，只能移动（把所有权交出去）。

```cpp
#include <memory>

auto p = std::make_unique<int>(42);  // 推荐
// std::unique_ptr<int> q = p;       // 编译失败：不能拷贝
std::unique_ptr<int> q = std::move(p);  // OK：p 不再拥有
// 此时 p 为空，q 独占那块 int
```

- 析构时自动 `delete`（数组用 `unique_ptr<T[]>`，对应 `delete[]`）
- `p.get()`：借出裸指针，**所有权还在 p**
- `p.reset()`：释放当前对象（或换成新的）
- `p.release()`：放弃所有权，返回裸指针——你必须自己 `delete`，一般少用

**为何不能拷贝？** 拷贝会出现两个主人，析构两次 → 和 Day01 浅拷贝同一类事故。

和宇树岗：一个串口、一块 DMA 缓冲，同一时刻只该有一个模块独占。

## 2. `shared_ptr<T>`：共享 + 引用计数

**含义**：可以有多个主人。内部有一个**控制块**，记着「现在几个人还握着」。

- 拷贝：计数 +1
- 某个 `shared_ptr` 销毁：计数 −1
- 减到 **0**：才真正 `delete` 对象

```cpp
auto a = std::make_shared<int>(1);
auto b = a;          // 拷贝，use_count() == 2
std::cout << a.use_count();  // 2
b.reset();           // 只剩 a，计数变 1
// a 离开作用域 → 计数 0 → 释放
```

`use_count()` 上机要打印，用来「看见」共享关系。

### `make_shared` vs `shared_ptr(new T)`

```cpp
std::shared_ptr<T> p(new T());     // 两次分配：对象 + 控制块
auto p = std::make_shared<T>();    // 通常一次分配；异常更安全
```

优先 `make_shared` / `make_unique`。

## 3. `weak_ptr<T>`：观察，不加计数

`weak_ptr` **不**让对象多活一会儿。它只是说：「我认识那个对象，但它可能已经没了。」

```cpp
std::weak_ptr<int> w = a;   // 计数不变
if (auto sp = w.lock()) {
    // lock 成功：对象还在，sp 是临时的 shared_ptr，这段里对象保证活着
} else {
    // 对象已经销毁
}
```

- `w.expired()`：已经死了则为 true
- **不要以为**有了 `weak_ptr` 对象就一直在

用途：观察、缓存、**打破循环引用**。

## 4. 循环引用（今天必会）

```text
A.peer ──shared_ptr──► B
B.peer ──shared_ptr──► A
```

A 的计数至少有 1（B 握着），B 的计数至少有 1（A 握着）。外面的指针都丢了，计数也到不了 0 → **泄漏**。

修法：环上**一侧**改成 `weak_ptr`（谁「观察」谁，谁用 weak）。

```text
A.peer ──shared_ptr──► B
B.peer ──weak_ptr───► A     // B 不把 A 的计数 +1
```

上机：先写出互持 `shared_ptr` 的泄漏版，打印 `use_count`；再改一侧为 `weak_ptr`，看计数能降到 0。

## 5. 自定义删除器（知道即可）

`unique_ptr` / `shared_ptr` 不一定 `delete`。可以在销毁时 `fclose`、关 socket。今天设备的 `close` 日志，可以想成「删除器在析构时被调用」。

## 6. 易错点（看懂再上机）

1. **两个 `shared_ptr` 分别从同一裸指针构造**
   ```cpp
   T* raw = new T;
   std::shared_ptr<T> a(raw);
   std::shared_ptr<T> b(raw);  // 两个控制块，会 delete 两次
   ```
   必须从同一个 `shared_ptr` 拷贝，或用 `make_shared`。

2. **长期保存 `.get()` 出来的裸指针**  
   智能指针释放后，裸指针成悬空。

3. **`weak_ptr` 当保险柜**  
   它不延长寿命；用之前必须 `lock()`。

## C++ 面试口述（预习时自己答一遍）

1. **`unique_ptr` 为何不能拷贝？**  
   独占。拷贝 = 两个主人 = 双重释放。只能 `move`。

2. **循环引用为何泄漏？怎么破？**  
   互相 `shared_ptr`，计数互锁到不了 0。一侧改 `weak_ptr`。

3. **`make_shared` 比 `shared_ptr(new T)` 好在哪？**  
   常一次分配对象+控制块；异常时也不容易只漏一半。

## C++ 上机（预习不用写）

见 `week1-cpp-basics/day02-smart-ptr/TASK.md`：

- `unique_ptr` 独占模拟设备（open/close 打日志）
- 两对象互 `shared_ptr` 泄漏，再 `weak_ptr` 修好
- 打印 `use_count`
