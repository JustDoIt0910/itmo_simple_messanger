//
// Created by just do it on 2023/10/23.
//

#ifndef NP_AS1_BUFFER_H
#define NP_AS1_BUFFER_H
#include <algorithm>
#include <vector>

class Buffer {
 public:
  Buffer(int initial_size = 0) : read_ptr_(0), write_ptr_(0) {
    data_.reserve(initial_size);
  }

  void append(const void* data, int len) {
    int size = data_.size();
    if (size - write_ptr_ < len) {
      data_.resize(size + len);
    }
    std::copy(static_cast<const char*>(data),
              static_cast<const char*>(data) + len, &data_[write_ptr_]);
    write_ptr_ += len;
  }

  char* peek() { return &data_[read_ptr_]; }

  void retrieve(int len) {
    read_ptr_ += len;
    if (read_ptr_ >= write_ptr_) {
      read_ptr_ = write_ptr_ = 0;
    }
  }

  int readable() const { return write_ptr_ - read_ptr_; }

 private:
  std::vector<char> data_;
  int read_ptr_;
  int write_ptr_;
};

#endif  // NP_AS1_BUFFER_H
