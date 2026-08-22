#pragma once

#include <queue>
#include <string>
#include <unordered_map>

struct Task {
    int id = 0;
    int priority = 0;
    std::string payload;
};

// 堆里的一条记录（同一 id 更新后，旧记录仍可能躺在堆里）
struct HeapItem {
    int priority = 0;
    int version = 0;
    int id = 0;
};

// 大顶堆：priority 大的在 top。默认 priority_queue 用 operator<
inline bool operator<(const HeapItem& a, const HeapItem& b) {
    if (a.priority != b.priority) return a.priority < b.priority;
    return a.version < b.version;
}

class Scheduler {
public:
    // 新任务。id 已存在则不必处理（或直接 return）
    void add(int id, int priority, std::string payload);

    bool contains(int id) const;

    // 延迟删除：改 map 里的最新值，再 push 一条新 HeapItem；旧的留在堆里
    void update_priority(int id, int new_priority);

    // 循环看堆顶：version 对不上就 pop 丢掉，直到有效或堆空
    // 有有效任务：填 out 并从 map 删掉，return true
    bool pop_highest(Task& out);

private:
    struct Meta {
        int priority = 0;
        int version = 0;
        std::string payload;
    };

    std::priority_queue<HeapItem> heap_;
    std::unordered_map<int, Meta> latest_;  // 权威数据：在不在、最新优先级
};
