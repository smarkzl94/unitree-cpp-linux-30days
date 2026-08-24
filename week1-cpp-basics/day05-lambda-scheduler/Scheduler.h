#pragma once

#include <functional>
#include <queue>
#include <string>
#include <unordered_map>
#include <vector>

struct Task {
    int id = 0;
    int priority = 0;
    std::string payload;
};

struct HeapItem {
    int priority = 0;
    int version = 0;
    int id = 0;
};

// 默认：priority 大的先出（和 Day04 一样）
inline bool default_less(const HeapItem& a, const HeapItem& b) {
    if (a.priority != b.priority) return a.priority < b.priority;
    return a.version < b.version;
}

class Scheduler {
public:
    using Cmp = std::function<bool(const HeapItem&, const HeapItem&)>;

    // 不传比较器 = 大顶堆；传入 lambda 可改成小顶等
    explicit Scheduler(Cmp cmp = default_less);

    void add(int id, int priority, std::string payload);
    bool contains(int id) const;
    void update_priority(int id, int new_priority);
    bool pop_highest(Task& out);

    // 从 map 拷一份「当前仍有效」的任务，用算法排序或过滤（不要手写选择排序）
    std::vector<Task> snapshot_sorted() const;

private:
    struct Meta {
        int priority = 0;
        int version = 0;
        std::string payload;
    };

    Cmp cmp_;
    std::priority_queue<HeapItem, std::vector<HeapItem>, Cmp> heap_;
    std::unordered_map<int, Meta> latest_;
};
