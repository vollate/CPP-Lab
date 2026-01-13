#pragma once

#include <atomic>
#include <memory>

namespace lf_lab {

template <typename DataType> class ListQueue {
private:
  struct Node;

public:
  ListQueue() : head_(&head_guard_), tail_(&head_guard_) {
    head_guard_.next_.store(&head_guard_);
  }
  ListQueue(const ListQueue &) = delete;
  ListQueue &operator=(const ListQueue &) = delete;
  ~ListQueue() { // TODO
  }

  void enqueue(DataType &&data) {
    Node *new_tail = new Node(std::forward(data));
    do {
      Node *expected = nullptr;
      Node *old_tail = tail_.load();
      if (old_tail->next_.compare_exchange_strong(expected, new_tail)) {
        tail_.store(new_tail);
        return;
      } else {
        tail_.compare_exchange_strong(old_tail, expected);
      }
    } while (true);
  }

  auto dequeue() -> DataType * {
    Node *to_pop = nullptr;
    do {
      to_pop = head_->next_.load();
      if (to_pop == head_) { // queue empty
        return nullptr;
      }
      Node *second_node_ = to_pop->next_.load();
      if (head_->next_.compare_exchange_strong(to_pop, second_node_)) {
        return to_pop;
      }
    } while (true);
  }

private:
  struct Node {
    DataType *data_ = nullptr;
    std::atomic<Node *> next_ = nullptr;

    Node() = default;
    Node(DataType &&data) : data_(std::forward(data)) {}
    ~Node() {}
  };

  Node head_guard_;
  Node *head_;
  std::atomic<Node *> tail_;
};

} // namespace lf_lab
