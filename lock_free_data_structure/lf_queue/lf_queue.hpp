#pragma once

#include <atomic>
#include <memory>

namespace lf_lab {

template <typename DataType> class Queue {
public:
  Queue() : head_(&guard_), tail_(&guard_) {}
  Queue(const Queue &) = delete;
  Queue &operator=(const Queue &) = delete;
  ~Queue() { // TODO
  }
  void enqueue(const DataType &data) {}
  void enqueue(DataType &&data) {}

private:
  struct Node {
    std::shared_ptr<DataType> data;
  };

  Node guard_;
  std::atomic<Node *> head_;
  std::atomic<Node *> tail_;
};

} // namespace lf_lab
