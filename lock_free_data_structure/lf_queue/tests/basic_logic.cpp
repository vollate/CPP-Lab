#include "lf_queue.hpp"

#include <gtest/gtest.h>

TEST(BasicLogic, EnqueueAndDequeue) {
  lf_lab::ListQueue<int> a;
  a.enqueue(1);
  ASSERT_EQ(*a.dequeue(), 1);
}
