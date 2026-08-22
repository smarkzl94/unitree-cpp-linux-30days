#include "Scheduler.h"

void Scheduler::add(int id, int priority, std::string payload) {
    // count(id)：表里有没有这个钥匙；有则非 0，当 true 用
    if (latest_.count(id)) return;  // 已经登记过，不再加一份
    // [] 下标：没有就新建一格。右边 {} 按 Meta 的三个字段填：priority, version, payload
    latest_[id] = {priority, 1, payload};
    // push：往堆顶结构里塞一张纸（priority, version, id），旧纸不会被改掉
    heap_.push({priority, 1, id});
}

bool Scheduler::contains(int id) const {
    // 只问表。堆里可能还有作废的旧纸，不能用 heap_.size()
    if (latest_.count(id)) return true;
    return false;
}

void Scheduler::update_priority(int id, int new_priority) {
    if (!latest_.count(id)) return;  // 表里没有这个 id，没法改
    latest_[id].version++;           // 表：版本 1→2。latest_[id] 是对象，用 .
    latest_[id].priority = new_priority;
    // 堆再塞一张新纸，version 抄表里加完后的值，不要再 +1
    heap_.push({new_priority, latest_[id].version, id});
}

bool Scheduler::pop_highest(Task& out) {
    // 堆空了就没有纸条可拿
    while (!heap_.empty()) {
        auto top = heap_.top();  // 看最上面那张（还没拿走）
        heap_.pop();             // 拿走最上面
        // find：按 id 去表里找。找到返回「指向那一格」的迭代器 it
        auto it = latest_.find(top.id);
        // end() 表示没找到（任务已被弹出/删除）
        // it 像指针，所以用 -> 。map 每一格是 {钥匙, 值}：
        //   it->first  = id
        //   it->second = Meta（priority / version / payload）
        if (it == latest_.end() || it->second.version != top.version) {
            continue;  // 脏数据：扔掉这张纸，看堆里下一张
        }
        // 对上了：把结果填进调用者的 Task out
        out = {top.id, top.priority, it->second.payload};
        latest_.erase(it);  // 从表里删掉，之后 contains 就是 false
        return true;
    }
    return false;  // 堆里全是脏数据，或本来就是空的
}
