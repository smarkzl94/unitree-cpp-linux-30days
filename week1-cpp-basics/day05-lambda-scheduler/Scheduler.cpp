#include "Scheduler.h"

#include <algorithm>  // std::sort 在这里

Scheduler::Scheduler(Cmp cmp)
    // std::move：把 cmp 的内容挪给成员 cmp_，避免再拷一份
    // heap_(cmp_)：用这份比较器造堆；不传参时 cmp 默认是 default_less（大顶）
    : cmp_(std::move(cmp)), heap_(cmp_) {}

void Scheduler::add(int id, int priority, std::string payload) {
    if (latest_.count(id)) return;  // 表里已有这个 id，不再加第二份
    // Meta 三个字段按声明顺序：priority, version, payload。version 从 1 起
    latest_[id] = {priority, 1, payload};
    // HeapItem 三个字段：priority, version, id。和表里的 version 必须对上
    heap_.push({priority, 1, id});
}

bool Scheduler::contains(int id) const {
    // 只问表。堆里可能还有作废旧纸，heap_.size() 会偏大
    return latest_.count(id) != 0;
}

void Scheduler::update_priority(int id, int new_priority) {
    if (!latest_.count(id)) return;  // 没登记过，没法改
    latest_[id].version++;           // 表里版本 1→2。latest_[id] 是对象，用 .
    latest_[id].priority = new_priority;
    // 堆再塞一张新纸。version 抄表里加完后的值，不要再 +1
    heap_.push({new_priority, latest_[id].version, id});
}

bool Scheduler::pop_highest(Task& out) {
    while (!heap_.empty()) {
        auto top = heap_.top();  // 看最上面那张（还没拿走）
        heap_.pop();             // 拿走最上面
        auto it = latest_.find(top.id);  // 按 id 去表里找
        // end() = 没找到（已被弹出）。it 像指针，用 ->
        // it->second 是 Meta；version 对不上就是脏数据
        if (it == latest_.end() || it->second.version != top.version) {
            continue;  // 扔掉这张纸，看堆里下一张
        }
        // 对上了：填进调用者的 Task out（id, priority, payload）
        out = {top.id, top.priority, it->second.payload};
        latest_.erase(it);  // 从表里删掉，之后 contains 就是 false
        return true;
    }
    return false;  // 堆空了，或全是脏数据
}

std::vector<Task> Scheduler::snapshot_sorted() const {
    std::vector<Task> v;
    v.reserve(latest_.size());  // 已知要放几个，先订容量，少几次扩容
    // kv 是表的一格：kv.first = id，kv.second = Meta
    for (const auto& kv : latest_) {
        v.push_back({kv.first, kv.second.priority, kv.second.payload});
    }
    // 第三个参数是 lambda：返回 true 表示 a 应排在 b 前面
    std::sort(v.begin(), v.end(), [](const Task& a, const Task& b) {
        return a.priority > b.priority;  // 从大到小；别写成 >=
    });
    return v;  // 返回的是拷贝，动不到调度器内部
}
