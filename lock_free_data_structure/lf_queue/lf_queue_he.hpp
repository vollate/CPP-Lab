#pragma once

#include <atomic>
#include <memory>
#include <optional>

namespace lf_lab {
template <typename DataType> class LFQueueBasic {
  struct Node;

  void _enqueue_impl(Node *const new_tail) {
    while (new_tail) {
      Node *old_tail = tail_.load();
      Node *expected = nullptr;
      if (old_tail->next_.compare_exchange_strong(expected, new_tail)) {
        tail_.compare_exchange_strong(old_tail, new_tail);
        return;
      }
      Node *next_tail = old_tail->next_.load();
      tail_.compare_exchange_strong(old_tail, next_tail);
    }
  }

public:
  LFQueueBasic()
      : hidden_head_(new Node), head_(new Node), tail_(head_.load()) {
    hidden_head_->next_.store(head_.load());
  }

  LFQueueBasic(const LFQueueBasic &) = delete;

  LFQueueBasic &operator=(const LFQueueBasic &) = delete;

  ~LFQueueBasic();

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
      auto result = std::move(new_head->data_);
      new_head->data_ = nullptr;
      // delete old_head;
      return std::move(result);
    }
  }

  void clear() {
    while (dequeue()) {
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

  Node *hidden_head_;
  std::atomic<Node *> head_;
  std::atomic<Node *> tail_;
};

template <typename DataType> LFQueueBasic<DataType>::~LFQueueBasic() {
  // 析构时从 hidden_head_->next_ 开始（跳过 hidden_head 本身）
  Node *current = hidden_head_->next_.load();
  while (current != nullptr) {
    Node *next = current->next_.load();
    delete current;
    current = next;
  }
  // 最后删除 hidden_head
  delete hidden_head_;
}
} // namespace lf_lab
