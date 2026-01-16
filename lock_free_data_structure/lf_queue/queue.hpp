#pragma once

#include <atomic>
#include <memory>
#include <utility>

namespace basic_lab {

template <typename DataType> class ListQueue {
  struct Node;

public:
  ListQueue() : head_(&head_guard_), tail_(&head_guard_) {
    head_guard_.next_.store(&head_guard_);
  }
  ListQueue(const ListQueue &) = delete;
  ListQueue &operator=(const ListQueue &) = delete;
  ~ListQueue() { // TODO
  }

  void enqueue(const DataType &data) {
    Node *new_tail = new Node(data);
    enqueue_impl(new_tail);
  }

  void enqueue(DataType &&data) {
    Node *new_tail = new Node(std::move(data));
    enqueue_impl(new_tail);
  }

  void enqueue_impl(Node *new_tail) {
  }

  auto dequeue() -> DataType * {
    Node *to_pop = nullptr;

  }

private:
  struct Node {
    DataType *data_ = nullptr;
    Node *next_ = nullptr;

    Node() = default;
    Node(DataType &&data) : data_(new DataType(std::move(data))) {}
    ~Node() {}
  };

  Node head_guard_;
  Node *head_;
  Node *tail_;
};

} // namespace lf_lab
