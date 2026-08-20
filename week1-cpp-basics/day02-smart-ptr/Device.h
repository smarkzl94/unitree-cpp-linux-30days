#pragma once

#include <memory>
#include <string>

// 模拟设备：构造/析构、open/close 打日志，方便看见生命周期
class Device {
public:
    explicit Device(std::string name);
    ~Device();

    void open();
    void close();
    const std::string& name() const;

private:
    std::string name_;
    bool opened_;
};

// 循环引用演示：两个 Partner 互相指
struct Partner {
    std::string name;
    std::shared_ptr<Partner> strong;  // 泄漏版用这个互指
    std::weak_ptr<Partner> weak;      // 修好版：一侧改用这个
    ~Partner();  // 打日志：泄漏时离开 {} 可能看不到这行
};
