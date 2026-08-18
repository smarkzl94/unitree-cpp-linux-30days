# Day02 · 智能指针

日期：2026-08-19

## 为什么学这个
手写 `new/delete` 易漏；现代 C++ 用智能指针表达所有权。设备驱动包装、模块生命周期管理都靠它。

## 核心知识
1. **`unique_ptr<T>`**：独占所有权，不可拷贝，可移动；默认删除器调用 `delete`。
2. **`shared_ptr<T>`**：共享所有权，引用计数；拷贝时 count+1，销毁 -1，到 0 释放。
3. **`weak_ptr<T>`**：不增加引用计数；用于观察或打破环；`lock()` 升级为 `shared_ptr`。
4. **循环引用**：A 持有 `shared_ptr` 到 B，B 持有 `shared_ptr` 到 A → 永远不释放；一侧改 `weak_ptr`。
5. **`make_unique` / `make_shared`**：异常安全更好，优先使用。
6. **自定义删除器**：可管理 `FILE*`、Windows HANDLE、socket 等。

## 易错点
- 用两个 `shared_ptr` 管理同一裸指针（未从同一控制块创建）→ 双重释放
- 从 `shared_ptr` 再取出裸指针长期保存
- 误以为 `weak_ptr` 能保证对象一直活着

## 面试常问
- `unique_ptr` 为何不能拷贝？
- `make_shared` 相对 `shared_ptr(new T)` 的优势？
- 如何用 `weak_ptr` 破环？

## 和宇树岗的关系
传感器模块、通信会话、插件式算法模块的生命周期管理。
