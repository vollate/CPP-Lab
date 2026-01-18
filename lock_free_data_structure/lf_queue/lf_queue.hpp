#pragma once

#include <atomic>
#include <tuple>

namespace lf_lab {

template <typename DataType> class LFQueue {
  struct Node;

private:
  void _enqueue_impl(Node *const new_tail) {
    while (true) {
      Node *old_tail = tail_.load();
      Node *expected = nullptr;
      if (old_tail->next_.compare_exchange_strong(expected, new_tail)) {
        Node *cur_tail = tail_.load();
        std::ignore = tail_.compare_exchange_strong(cur_tail, new_tail);
        return;
      } else {
        Node *cur_tail = tail_.load();
        Node *next_tail = cur_tail->next_.load();
        std::ignore = tail_.compare_exchange_strong(cur_tail, next_tail);
      }
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
      if (empty()) {
        return false;
      }
      Node *to_pop = head_->next_;
      if (head_->next_ == tail_) {
        tail_.store(head_);
        head_->next_.load() = nullptr;
      } else {
        head_->next_ = to_pop->next_;
      }
      result = std::move(*to_pop->data_);
      delete to_pop;
      return true;
    }
  }

  void clear() {
    while (head_->next_) {
      Node *to_pop = head_->next_;
      head_->next_ = to_pop->next_;
      delete to_pop;
    }
  }

  auto empty() -> bool const { return head_->next_.load() == nullptr; }

private:
  struct Node {
    DataType *data_ = nullptr;
    std::atomic<Node *> next_ = nullptr;

    Node() = default;
    Node(const DataType &data) : data_(new DataType(data)) {}
    Node(DataType &&data)
        : data_(new DataType(static_cast<DataType &&>(data))) {}
    ~Node() { delete data_; }
  };

  Node head_guard_;
  Node *head_;
  std::atomic<Node *> tail_;
};

template <typename DataType> LFQueue<DataType>::~LFQueue() { clear(); }

} // namespace lf_lab
