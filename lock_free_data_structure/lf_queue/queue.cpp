#include <algorithm>
#include <cstdio>

namespace basic_lab {

template <typename DataType> class ListQueue {
  struct Node;

public:
  ListQueue() : head_(&head_guard_), tail_(&head_guard_) {}
  ListQueue(const ListQueue &) = delete;
  ListQueue &operator=(const ListQueue &) = delete;
  ~ListQueue();

  void enqueue(const DataType &data) {
    Node *new_tail = new Node(data);
    tail_->next_ = new_tail;
    tail_ = new_tail;
  }

  void enqueue(DataType &&data) {
    Node *new_tail = new Node(std::move(data));
    tail_->next_ = new_tail;
    tail_ = new_tail;
  }

  bool dequeue(DataType &result) {
    if (empty()) {
      return false;
    }
    Node *to_pop = head_->next_;
    result = std::move(*to_pop->data_);
    if (head_->next_ == tail_) {
      tail_ = head_;
    } else {
      head_->next_ = to_pop->next_;
    }
    delete to_pop;
    return true;
  }

  bool empty() const { return head_ == tail_; }

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

template <typename DataType> ListQueue<DataType>::~ListQueue() {
  while (!empty()) {
    Node *to_delete = head_->next_;
    if (head_->next_ == tail_) {
      tail_ = head_;
    } else {
      head_->next_ = to_delete->next_;
    }
    delete to_delete;
  }
}

} // namespace basic_lab

int main() {
  basic_lab::ListQueue<int> queue;

  queue.enqueue(1);
  queue.enqueue(2);
  queue.enqueue(3);

  printf("Initial queue size test:\n");
  printf("Expected 3 elements\n");

  int value;
  while (queue.dequeue(value)) {
    printf("Dequeued: %d\n", value);
  }

  printf("Queue is now empty: %s\n", queue.empty() ? "true" : "false");

  return 0;
}
