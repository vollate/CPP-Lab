#include <algorithm>
#include <cstdio>
#include <gtest/gtest.h>

namespace basic_lab {

template <typename DataType> class UnsafeQueue {
  struct Node;

private:
  void _enqueue_impl(Node *const new_tail) {
    tail_->next_ = new_tail;
    tail_ = new_tail;
  }

public:
  UnsafeQueue() : head_(&head_guard_), tail_(&head_guard_) {}
  UnsafeQueue(const UnsafeQueue &) = delete;
  UnsafeQueue &operator=(const UnsafeQueue &) = delete;
  ~UnsafeQueue();

  void enqueue(const DataType &data) {
    Node *new_tail = new Node(data);
    _enqueue_impl(new_tail);
  }

  void enqueue(DataType &&data) {
    Node *new_tail = new Node(std::move(data));
    _enqueue_impl(new_tail);
  }

  auto dequeue(DataType &result) -> bool {
    if (empty()) {
      return false;
    }
    Node *to_pop = head_->next_;
    result = std::move(*to_pop->data_);
    if (head_->next_ == tail_) {
      tail_ = head_;
      head_->next_ = nullptr;
    } else {
      head_->next_ = to_pop->next_;
    }
    delete to_pop;
    return true;
  }

  void clear() {
    while (head_->next_) {
      Node *to_pop = head_->next_;
      head_->next_ = to_pop->next_;
      delete to_pop;
    }
  }

  auto empty() -> bool const { return head_->next_ == nullptr; }

private:
  struct Node {
    DataType *data_ = nullptr;
    Node *next_ = nullptr;

    Node() = default;
    Node(const DataType &data) : data_(new DataType(data)) {}
    Node(DataType &&data)
        : data_(new DataType(static_cast<DataType &&>(data))) {}
    ~Node() { delete data_; }
  };

  Node head_guard_;
  Node *head_;
  Node *tail_;
};

template <typename DataType> UnsafeQueue<DataType>::~UnsafeQueue() { clear(); }

} // namespace basic_lab

TEST(UnsafeQueue, BasicOperations) {
  basic_lab::UnsafeQueue<int> queue;

  queue.enqueue(1);
  queue.enqueue(2);
  queue.enqueue(3);

  ASSERT_EQ(queue.empty(), false);

  printf("Initial queue size test:\n");
  printf("Expected 3 elements\n");

  int value;
  while (queue.dequeue(value)) {
    printf("Dequeued: %d\n", value);
  }

  printf("Queue is now empty: %s\n", queue.empty() ? "true" : "false");
}
