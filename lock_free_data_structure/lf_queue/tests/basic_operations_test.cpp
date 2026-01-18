#include "lf_queue.hpp"

#include <gtest/gtest.h>

TEST(BasicOperations, SingleEnqueueDequeue) {
  lf_lab::LFQueue<int> queue;

  queue.enqueue(42);

  int result;
  ASSERT_TRUE(queue.dequeue(result));
  EXPECT_EQ(result, 42);
  EXPECT_TRUE(queue.empty());
}

TEST(BasicOperations, MultipleEnqueueDequeueSequential) {
  lf_lab::LFQueue<int> queue;

  const int num_elements = 10;
  for (int i = 0; i < num_elements; ++i) {
    queue.enqueue(i);
  }

  EXPECT_FALSE(queue.empty());

  int result;
  for (int i = 0; i < num_elements; ++i) {
    ASSERT_TRUE(queue.dequeue(result));
    EXPECT_EQ(result, i);
  }

  EXPECT_TRUE(queue.empty());
}

TEST(BasicOperations, EmptyQueueDequeue) {
  lf_lab::LFQueue<int> queue;

  int result;
  ASSERT_FALSE(queue.dequeue(result));
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

  int result;
  ASSERT_TRUE(queue.dequeue(result));
  EXPECT_EQ(result, 42);
  EXPECT_TRUE(queue.empty());
}

TEST(BasicOperations, EmptyStateCheck) {
  lf_lab::LFQueue<int> queue;

  EXPECT_TRUE(queue.empty());

  queue.enqueue(1);
  EXPECT_FALSE(queue.empty());

  queue.enqueue(2);
  EXPECT_FALSE(queue.empty());

  int result;
  queue.dequeue(result);
  EXPECT_FALSE(queue.empty());

  queue.dequeue(result);
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

  std::string result1, result2;
  ASSERT_TRUE(queue.dequeue(result1));
  ASSERT_TRUE(queue.dequeue(result2));

  EXPECT_EQ(result1, "hello");
  EXPECT_EQ(result2, "world");
  EXPECT_TRUE(queue.empty());
}
