# Day07 · C++ 口述笔记

- 移动何时发生：`std::move`、临时量、return 局部（或 RVO）、vector 扩容且移动 noexcept。
- emplace_back vs push_back：原地转发构造 vs 往往先有临时对象。
- 五件套：有裸资源必须写（或 delete）；vector/unique_ptr 成员 → Rule of Zero。
- RingBuffer API：push / pop / size / full / empty；满时覆盖最旧。
