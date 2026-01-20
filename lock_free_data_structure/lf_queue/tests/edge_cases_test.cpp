#include "lf_queue.hpp"

#include <atomic>
#include <gtest/gtest.h>
#include <random>
#include <thread>
#include <vector>

TEST(EdgeCases, StressTest1MillionOps) {
  lf_lab::LFQueue<int> queue;
  std::atomic<int> enqueue_count(0);
  std::atomic<int> dequeue_count(0);
  std::vector<int> dequeued_values;
  std::mutex mtx;

  const int total_operations = 1000000;
  const int num_threads = 4;

  std::vector<std::thread> threads;

  for (int i = 0; i < num_threads; ++i) {
    threads.emplace_back([&, thread_id = i]() {
      std::mt19937 rng(thread_id);
      std::uniform_int_distribution<int> dist(0, 9);

      for (int j = 0; j < total_operations / num_threads; ++j) {
        int operation = dist(rng);
        if (operation < 7) {
          queue.enqueue(thread_id * 1000000 + j);
          enqueue_count.fetch_add(1, std::memory_order_relaxed);
        } else {
          int value;
          if (queue.dequeue(value)) {
            std::lock_guard<std::mutex> lock(mtx);
            dequeued_values.push_back(value);
            dequeue_count.fetch_add(1, std::memory_order_relaxed);
          }
        }
      }
    });
  }

  for (auto &t : threads) {
    t.join();
  }

  int value;
  while (queue.dequeue(value)) {
    std::lock_guard<std::mutex> lock(mtx);
    dequeued_values.push_back(value);
    dequeue_count.fetch_add(1, std::memory_order_relaxed);
  }

  EXPECT_EQ(enqueue_count.load(), dequeue_count.load());
  EXPECT_EQ(dequeued_values.size(), static_cast<size_t>(dequeue_count.load()));
}

TEST(EdgeCases, RapidEmptyNonEmpty) {
  lf_lab::LFQueue<int> queue;

  for (int i = 0; i < 10000; ++i) {
    queue.enqueue(i);
    EXPECT_FALSE(queue.empty());

    int value;
    ASSERT_TRUE(queue.dequeue(value));
    EXPECT_EQ(value, i);
    EXPECT_TRUE(queue.empty());
  }

  EXPECT_TRUE(queue.empty());
}

TEST(EdgeCases, AlternatingPattern) {
  lf_lab::LFQueue<int> queue;
  std::vector<int> dequeued_values;

  const int iterations = 10000;

  for (int i = 0; i < iterations; ++i) {
    queue.enqueue(i);
    queue.enqueue(i + 1);

    int value1, value2;
    ASSERT_TRUE(queue.dequeue(value1));
    ASSERT_TRUE(queue.dequeue(value2));
    dequeued_values.push_back(value1);
    dequeued_values.push_back(value2);
  }

  EXPECT_EQ(dequeued_values.size(), static_cast<size_t>(iterations * 2));

  for (size_t i = 0; i < dequeued_values.size(); i += 2) {
    EXPECT_EQ(dequeued_values[i], static_cast<int>(i / 2));
    EXPECT_EQ(dequeued_values[i + 1], static_cast<int>(i / 2) + 1);
  }

  EXPECT_TRUE(queue.empty());
}

TEST(EdgeCases, SingleElementRepeated) {
  lf_lab::LFQueue<int> queue;

  for (int i = 0; i < 1000; ++i) {
    queue.enqueue(42);
    int value;
    ASSERT_TRUE(queue.dequeue(value));
    EXPECT_EQ(value, 42);
  }

  EXPECT_TRUE(queue.empty());
}

TEST(EdgeCases, ClearWhileConcurrent) {
  lf_lab::LFQueue<int> queue;
  std::atomic<bool> should_clear(false);
  std::atomic<int> enqueue_count(0);
  std::atomic<int> dequeue_count(0);

  const int num_threads = 4;
  const int ops_per_thread = 10000;

  std::vector<std::thread> threads;

  for (int i = 0; i < num_threads; ++i) {
    threads.emplace_back([&, thread_id = i]() {
      for (int j = 0; j < ops_per_thread; ++j) {
        if (should_clear.load(std::memory_order_relaxed)) {
          break;
        }
        queue.enqueue(thread_id * ops_per_thread + j);
        enqueue_count.fetch_add(1, std::memory_order_relaxed);

        int value;
        if (queue.dequeue(value)) {
          dequeue_count.fetch_add(1, std::memory_order_relaxed);
        }
      }
    });
  }

  std::thread clear_thread([&]() {
    std::this_thread::sleep_for(std::chrono::milliseconds(100));
    should_clear.store(true, std::memory_order_release);
    queue.clear();
  });

  for (auto &t : threads) {
    t.join();
  }

  clear_thread.join();

  EXPECT_TRUE(queue.empty());

  int value;
  EXPECT_FALSE(queue.dequeue(value));
}

TEST(EdgeCases, MemoryPressure) {
  lf_lab::LFQueue<std::vector<int>> queue;
  std::atomic<int> enqueue_count(0);
  std::atomic<int> dequeue_count(0);

  const int num_threads = 4;
  const int ops_per_thread = 5000;
  const int vector_size = 1000;

  std::vector<std::thread> threads;

  for (int i = 0; i < num_threads; ++i) {
    threads.emplace_back([&, thread_id = i]() {
      std::vector<int> local_vec;
      local_vec.resize(vector_size);

      for (int j = 0; j < ops_per_thread; ++j) {
        std::vector<int> new_vec(vector_size, thread_id * ops_per_thread + j);
        queue.enqueue(std::move(new_vec));
        enqueue_count.fetch_add(1, std::memory_order_relaxed);

        std::vector<int> value;
        if (queue.dequeue(value)) {
          dequeue_count.fetch_add(1, std::memory_order_relaxed);
        }
      }
    });
  }

  for (auto &t : threads) {
    t.join();
  }

  while (true) {
    std::vector<int> value;
    if (!queue.dequeue(value)) {
      break;
    }
    dequeue_count.fetch_add(1, std::memory_order_relaxed);
  }

  EXPECT_EQ(enqueue_count.load(), dequeue_count.load());
  EXPECT_TRUE(queue.empty());
}
