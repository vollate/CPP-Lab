#pragma once

#include <atomic>
#include <memory>
#include <optional>
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

  auto dequeue() -> std::optional<std::unique_ptr<DataType>> {
    while (true) {
      Node *old_head = head_.load();
      Node *new_head = old_head->next_.load();
      if (new_head == nullptr) {
        return std::nullopt;
      }
      if (!head_.compare_exchange_strong(old_head, new_head)) {
        continue;
      }
      auto result = std::move(old_head->data_);
      old_head->data_ = nullptr;
      delete old_head;
      return std::move(result);
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
    std::unique_ptr<DataType> data_ = nullptr;
    std::atomic<Node *> next_ = nullptr;

    Node() = default;

    Node(const DataType &data) : data_(std::make_unique<DataType>(data)) {}

    Node(DataType &&data)
        : data_(std::make_unique<DataType>(std::move(data))) {}

    ~Node() = default;
  };

  Node head_guard_;
  std::atomic<Node *> head_;
  std::atomic<Node *> tail_;
};

template <typename DataType> LFQueue<DataType>::~LFQueue() { clear(); }
} // namespace lf_lab
