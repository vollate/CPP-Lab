#include "unsafe_queue.hpp"
#include <gtest/gtest.h>

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
