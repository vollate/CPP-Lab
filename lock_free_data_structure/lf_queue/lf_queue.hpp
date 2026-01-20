#pragma once

#include <atomic>
#include <tuple>

namespace lf_lab {
template <typename DataType> class LFQueue {
  struct Node;

  void _enqueue_impl(Node *const new_tail) {
    while (true) {
      Node *old_tail = tail_.load();
      Node *expected = nullptr;
      if (old_tail->next_.compare_exchange_strong(expected, new_tail)) {
        Node *cur_tail = tail_.load();
        std::ignore = tail_.compare_exchange_strong(cur_tail, new_tail);
        return;
      }
      Node *cur_tail = tail_.load();
      Node *next_tail = cur_tail->next_.load();
      std::ignore = tail_.compare_exchange_strong(cur_tail, next_tail);
    }
  }

public:
  LFQueue() : head_(&head_guard_), tail_(&head_guard_) {}

  LFQueue(const LFQueue &) = delete;

  LFQueue &operator=(const LFQueue &) = delete;

  ~LFQueue();

  void enqueue(const DataType &data) {
    Node *new_tail = new Node(data);
    _enqueue_impl(new_tail);
  }

  void enqueue(DataType &&data) {
    Node *new_tail = new Node(std::move(data));
    _enqueue_impl(new_tail);
  }

  auto dequeue(DataType &result) -> bool {
    while (true) {
      Node *old_head = head_.load();
      Node *new_head = old_head->next_.load();
      if (new_head == nullptr) {
        return false;
      }
      if (!head_.compare_exchange_strong(old_head, new_head)) {
        continue;
      }
      result = std::move(*new_head->data_);
      delete new_head;
      return true;
    }
  }

  void clear() {
    while (true) {
      Node *old_head = head_.load();
      Node *new_head = old_head->next_.load();
      if (new_head == nullptr) {
        return;
      }
      if (head_.compare_exchange_strong(old_head, new_head)) {
        delete old_head;
      }
    }
  }

  [[nodiscard]] auto empty() const -> bool {
    Node *head = head_.load();
    return head->next_.load() == nullptr;
  }

private:
  struct Node {
    DataType *data_ = nullptr;
    std::atomic<Node *> next_ = nullptr;

    Node() = default;

    Node(const DataType &data) : data_(new DataType(data)) {}

    Node(DataType &&data) : data_(new DataType(std::move(data))) {}

    ~Node() { delete data_; }
  };

  Node head_guard_;
  std::atomic<Node *> head_;
  std::atomic<Node *> tail_;
};

template <typename DataType> LFQueue<DataType>::~LFQueue() { clear(); }
} // namespace lf_lab
