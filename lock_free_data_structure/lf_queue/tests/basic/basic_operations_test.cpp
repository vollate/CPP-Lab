#include "lf_queue_basic.hpp"

#include <gtest/gtest.h>

TEST(BasicOperations, SingleEnqueueDequeue) {
  lf_lab::LFQueue<int> queue;

  queue.enqueue(42);

  auto result = queue.dequeue();
  ASSERT_TRUE(result.has_value());
  EXPECT_EQ(**result, 42);
  EXPECT_TRUE(queue.empty());
}

TEST(BasicOperations, MultipleEnqueueDequeueSequential) {
  lf_lab::LFQueue<int> queue;

  constexpr int num_elements = 10;
  for (int i = 0; i < num_elements; ++i) {
    queue.enqueue(i);
  }

  EXPECT_FALSE(queue.empty());

  for (int i = 0; i < num_elements; ++i) {
    auto result = queue.dequeue();
    ASSERT_TRUE(result.has_value());
    EXPECT_EQ(*result->get(), i);
  }

  EXPECT_TRUE(queue.empty());
}

TEST(BasicOperations, EmptyQueueDequeue) {
  lf_lab::LFQueue<int> queue;

  auto result = queue.dequeue();
  ASSERT_FALSE(result.has_value());
  EXPECT_TRUE(queue.empty());
}

TEST(BasicOperations, EnqueueAfterClear) {
  lf_lab::LFQueue<int> queue;

  queue.enqueue(1);
  queue.enqueue(2);
  queue.enqueue(3);

  queue.clear();
  EXPECT_TRUE(queue.empty());

  queue.enqueue(42);

  auto result = queue.dequeue();
  ASSERT_TRUE(result.has_value());
  EXPECT_EQ(**result, 42);
  EXPECT_TRUE(queue.empty());
}

TEST(BasicOperations, EmptyStateCheck) {
  lf_lab::LFQueue<int> queue;

  EXPECT_TRUE(queue.empty());

  queue.enqueue(1);
  EXPECT_FALSE(queue.empty());

  queue.enqueue(2);
  EXPECT_FALSE(queue.empty());

  queue.dequeue();
  EXPECT_FALSE(queue.empty());

  queue.dequeue();
  EXPECT_TRUE(queue.empty());
}

TEST(BasicOperations, MoveSemanticsBasic) {
  lf_lab::LFQueue<std::string> queue;

  std::string str1 = "hello";
  std::string str2 = "world";

  queue.enqueue(std::move(str1));
  queue.enqueue(std::move(str2));

  EXPECT_TRUE(str1.empty());
  EXPECT_TRUE(str2.empty());

  auto result1 = queue.dequeue();
  auto result2 = queue.dequeue();
  ASSERT_TRUE(result1.has_value());
  ASSERT_TRUE(result2.has_value());

  EXPECT_EQ(**result1, "hello");
  EXPECT_EQ(**result2, "world");
  EXPECT_TRUE(queue.empty());
}
