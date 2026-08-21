#include "DynArray.h"

#include <stdexcept>

DynArray::DynArray() : data_(nullptr), size_(0), cap_(0) {}

DynArray::~DynArray() {
    // TODO: delete[] data_; 然后 data_ = nullptr;
    delete[] data_;
    data_ = nullptr;
    size_ = 0;
    cap_ = 0;
}

void DynArray::grow() {
    // TODO: 新容量：cap_==0 则变成 1，否则 *2
    // TODO: int* neu = new int[new_cap];
    // TODO: 把 data_[0 .. size_) 拷到 neu
    // TODO: delete[] data_; data_ = neu; cap_ = new_cap;
    int new_cap = cap_ == 0 ? 1 : cap_ * 2;
    int *neu = new int[new_cap];
    for (int i = 0; i < size_; i++) {
        neu[i] = data_[i];
    }
    delete[] data_;
    data_ = neu;
    cap_ = new_cap;
}

void DynArray::push_back(int value) {
    // TODO: 若 size_ == cap_，先 grow()
    // TODO: data_[size_] = value; ++size_;
    if (size_ == cap_) {
        grow();
    }
    data_[size_] = value;
    size_++;
}

std::size_t DynArray::size() const {
    return size_;  // 已放几个
}

std::size_t DynArray::capacity() const {
    return cap_;  // 这块内存能装几个
}

int& DynArray::operator[](std::size_t i) {
    if (i >= size_) throw std::out_of_range("DynArray::operator[]");
    return data_[i];
}

const int& DynArray::operator[](std::size_t i) const {
    if (i >= size_) throw std::out_of_range("DynArray::operator[]");
    return data_[i];
}

const int* DynArray::data() const {
    return data_;
}
