#include "lf_queue.hpp"

#include <atomic>
#include <gtest/gtest.h>
#include <thread>
#include <vector>

TEST(MemoryTests, LeakAfterClear) {
  lf_lab::LFQueue<int> queue;

  for (int i = 0; i < 10000; ++i) {
    queue.enqueue(i);
  }

  EXPECT_FALSE(queue.empty());

  queue.clear();

  EXPECT_TRUE(queue.empty());

  int value;
  EXPECT_FALSE(queue.dequeue(value));
}

TEST(MemoryTests, LeakAfterDestructor) {
  {
    lf_lab::LFQueue<int> queue;

    for (int i = 0; i < 10000; ++i) {
      queue.enqueue(i);
    }
  }

  lf_lab::LFQueue<int> new_queue;

  for (int i = 0; i < 10000; ++i) {
    new_queue.enqueue(i);
  }

  int value;
  for (int i = 0; i < 10000; ++i) {
    ASSERT_TRUE(new_queue.dequeue(value));
    EXPECT_EQ(value, i);
  }

  EXPECT_TRUE(new_queue.empty());
}

TEST(MemoryTests, NodeAllocationTracking) {
  static std::atomic<int> alloc_count(0);
  static std::atomic<int> dealloc_count(0);

  struct TrackedInt {
    int value_;

    TrackedInt() : value_(0) {
      alloc_count.fetch_add(1, std::memory_order_relaxed);
    }
    explicit TrackedInt(int value) : value_(value) {
      alloc_count.fetch_add(1, std::memory_order_relaxed);
    }
    TrackedInt(const TrackedInt &other) : value_(other.value_) {
      alloc_count.fetch_add(1, std::memory_order_relaxed);
    }
    ~TrackedInt() { dealloc_count.fetch_add(1, std::memory_order_relaxed); }
  };

  alloc_count.store(0, std::memory_order_relaxed);
  dealloc_count.store(0, std::memory_order_relaxed);

  {
    lf_lab::LFQueue<TrackedInt> queue;

    for (int i = 0; i < 1000; ++i) {
      queue.enqueue(TrackedInt(i));
    }

    TrackedInt value;
    for (int i = 0; i < 1000; ++i) {
      queue.dequeue(value);
    }

    int initial_allocs = alloc_count.load(std::memory_order_relaxed);
    int initial_deallocs = dealloc_count.load(std::memory_order_relaxed);
  }

  int final_allocs = alloc_count.load(std::memory_order_relaxed);
  int final_deallocs = dealloc_count.load(std::memory_order_relaxed);

  EXPECT_EQ(final_allocs, final_deallocs)
      << "Memory leak detected: " << (final_allocs - final_deallocs)
      << " objects not freed";
}

TEST(MemoryTests, ConcurrentMemoryAllocation) {
  static std::atomic<int> alloc_count(0);
  static std::atomic<int> dealloc_count(0);

  struct TrackedInt {
    int value_;

    TrackedInt() : value_(0) {
      alloc_count.fetch_add(1, std::memory_order_relaxed);
    }
    explicit TrackedInt(int value) : value_(value) {
      alloc_count.fetch_add(1, std::memory_order_relaxed);
    }
    TrackedInt(const TrackedInt &other) : value_(other.value_) {
      alloc_count.fetch_add(1, std::memory_order_relaxed);
    }
    ~TrackedInt() { dealloc_count.fetch_add(1, std::memory_order_relaxed); }
  };

  alloc_count.store(0, std::memory_order_relaxed);
  dealloc_count.store(0, std::memory_order_relaxed);

  lf_lab::LFQueue<TrackedInt> queue;
  const int num_threads = 8;
  const int ops_per_thread = 1000;

  std::vector<std::thread> threads;

  for (int i = 0; i < num_threads; ++i) {
    threads.emplace_back([&queue, i, ops_per_thread]() {
      for (int j = 0; j < ops_per_thread; ++j) {
        queue.enqueue(TrackedInt(i * ops_per_thread + j));
      }
    });
  }

  for (auto &t : threads) {
    t.join();
  }
  threads.clear();

  for (int i = 0; i < num_threads; ++i) {
    threads.emplace_back([&queue, ops_per_thread]() {
      TrackedInt value;
      for (int j = 0; j < ops_per_thread; ++j) {
        queue.dequeue(value);
      }
    });
  }

  for (auto &t : threads) {
    t.join();
  }

  queue.clear();

  int final_allocs = alloc_count.load(std::memory_order_relaxed);
  int final_deallocs = dealloc_count.load(std::memory_order_relaxed);

  EXPECT_EQ(final_allocs, final_deallocs)
      << "Memory leak detected in concurrent test: "
      << (final_allocs - final_deallocs) << " objects not freed";
}

TEST(MemoryTests, MemoryOrdering) {
  lf_lab::LFQueue<std::atomic<int>> queue;
  const int num_threads = 4;
  const int ops_per_thread = 1000;

  std::vector<std::thread> threads;

  for (int i = 0; i < num_threads; ++i) {
    threads.emplace_back([&queue, i, ops_per_thread]() {
      for (int j = 0; j < ops_per_thread; ++j) {
        std::atomic_int value(i * ops_per_thread + j);
        queue.enqueue(std::move(value));
      }
    });
  }

  for (auto &t : threads) {
    t.join();
  }
  threads.clear();

  std::vector<int> received_values;
  for (int i = 0; i < num_threads * ops_per_thread; ++i) {
    std::atomic<int> value;
    if (queue.dequeue(value)) {
      received_values.push_back(value.load(std::memory_order_acquire));
    }
  }

  EXPECT_EQ(received_values.size(),
            static_cast<size_t>(num_threads * ops_per_thread));

  std::set<int> unique_values(received_values.begin(), received_values.end());
  EXPECT_EQ(unique_values.size(),
            static_cast<size_t>(num_threads * ops_per_thread));

  for (int i = 0; i < num_threads * ops_per_thread; ++i) {
    EXPECT_TRUE(unique_values.find(i) != unique_values.end())
        << "Value " << i << " not found, possible memory ordering issue";
  }
}
