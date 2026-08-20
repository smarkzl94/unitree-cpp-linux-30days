#include "Device.h"

#include <iostream>

Device::Device(std::string name) : name_(std::move(name)), opened_(false) {
    std::cout << "[Device] construct " << name_ << "\n";
}

Device::~Device() {
    // TODO: 若还开着，先 close；再打一行析构日志
    if(opened_){
        close();
    }
    std::cout << "[Device] destruct " << name_ << " Open_Status" << opened_ << "\n";
}

void Device::open() {
    // TODO: opened_ = true；打印 "[Device] open xxx"
    opened_ =  true;
    std::cout << "[Device] open " << name_ << "\n";
}

void Device::close() {
    // TODO: opened_ = false；打印 "[Device] close xxx"
    opened_ = false;
    std::cout << "[Device] close " << name_ << "\n";
}

const std::string& Device::name() const {
    return name_;
}

Partner::~Partner() {
    std::cout << "[Partner] destruct " << name << "\n";
}
