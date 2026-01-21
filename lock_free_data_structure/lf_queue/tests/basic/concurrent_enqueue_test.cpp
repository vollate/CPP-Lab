#include "lf_queue_basic.hpp"

#include <atomic>
#include <gtest/gtest.h>
#include <set>
#include <thread>
#include <vector>

void run_concurrent_enqueue_test(int num_threads, int elements_per_thread,
                                 const std::string &test_name) {
  lf_lab::LFQueue<int> queue;
  std::atomic_int total_enqueued(0);

  std::vector<std::thread> threads;
  threads.reserve(num_threads);
  for (int i = 0; i < num_threads; ++i) {
    threads.emplace_back([&, thread_id = i]() {
      for (int j = 0; j < elements_per_thread; ++j) {
        queue.enqueue(thread_id * elements_per_thread + j);
        total_enqueued.fetch_add(1, std::memory_order_relaxed);
      }
    });
  }

  for (auto &t : threads) {
    t.join();
  }

  const int expected_count = num_threads * elements_per_thread;
  EXPECT_EQ(total_enqueued.load(), expected_count);

  std::set<int> received_values;
  int dequeued_count = 0;
  while (auto value = queue.dequeue()) {
    received_values.insert(**value);
    dequeued_count++;
  }

  EXPECT_EQ(dequeued_count, expected_count);
  EXPECT_EQ(received_values.size(), static_cast<size_t>(expected_count));

  for (int i = 0; i < expected_count; ++i) {
    EXPECT_TRUE(received_values.find(i) != received_values.end())
        << "Value " << i << " not found in queue for " << test_name;
  }
}

void run_concurrent_unique_elements_test(int num_threads,
                                         int elements_per_thread,
                                         const std::string &test_name) {
  lf_lab::LFQueue<std::pair<int, int>> queue;
  std::atomic_int total_enqueued(0);

  std::vector<std::thread> threads;
  for (int i = 0; i < num_threads; ++i) {
    threads.emplace_back([&, thread_id = i]() {
      for (int j = 0; j < elements_per_thread; ++j) {
        queue.enqueue({thread_id, j});
        total_enqueued.fetch_add(1, std::memory_order_relaxed);
      }
    });
  }

  for (auto &t : threads) {
    t.join();
  }

  const int expected_count = num_threads * elements_per_thread;
  EXPECT_EQ(total_enqueued.load(), expected_count);

  std::set<std::pair<int, int>> received_values;
  int dequeued_count = 0;
  while (auto value = queue.dequeue()) {
    received_values.insert(**value);
    dequeued_count++;
  }

  EXPECT_EQ(dequeued_count, expected_count);
  EXPECT_EQ(received_values.size(), static_cast<size_t>(expected_count));

  for (int i = 0; i < num_threads; ++i) {
    for (int j = 0; j < elements_per_thread; ++j) {
      EXPECT_TRUE(received_values.find({i, j}) != received_values.end())
          << "Value {" << i << ", " << j << "} not found in queue for "
          << test_name;
    }
  }
}

void run_concurrent_data_integrity_test(int num_threads,
                                        int elements_per_thread,
                                        const std::string &test_name) {
  lf_lab::LFQueue<std::pair<int, double>> queue;
  std::atomic_int total_enqueued(0);

  std::vector<std::thread> threads;
  for (int i = 0; i < num_threads; ++i) {
    threads.emplace_back([&, thread_id = i]() {
      for (int j = 0; j < elements_per_thread; ++j) {
        int int_val = thread_id * elements_per_thread + j;
        double double_val = int_val * 1.5;
        queue.enqueue({int_val, double_val});
        total_enqueued.fetch_add(1, std::memory_order_relaxed);
      }
    });
  }

  for (auto &t : threads) {
    t.join();
  }

  const int expected_count = num_threads * elements_per_thread;
  EXPECT_EQ(total_enqueued.load(), expected_count);

  std::vector<bool> found(expected_count, false);
  int dequeued_count = 0;
  while (auto value = queue.dequeue()) {
    int expected_value = (*value)->first;
    double expected_double = expected_value * 1.5;
    EXPECT_EQ((*value)->second, expected_double)
        << "Double value corrupted for " << expected_value << " in "
        << test_name;
    found[expected_value] = true;
    dequeued_count++;
  }

  EXPECT_EQ(dequeued_count, expected_count);
  for (size_t i = 0; i < found.size(); ++i) {
    EXPECT_TRUE(found[i]) << "Value " << i << " not found in " << test_name;
  }
}

void run_concurrent_burst_test(int num_threads, int burst_size, int num_bursts,
                               const std::string &test_name) {
  lf_lab::LFQueue<int> queue;
  std::atomic_int total_enqueued(0);

  std::vector<std::thread> threads;
  for (int i = 0; i < num_threads; ++i) {
    threads.emplace_back([&, thread_id = i]() {
      for (int burst = 0; burst < num_bursts; ++burst) {
        for (int j = 0; j < burst_size; ++j) {
          queue.enqueue(thread_id * num_bursts * burst_size +
                        burst * burst_size + j);
          total_enqueued.fetch_add(1, std::memory_order_relaxed);
        }
        std::this_thread::yield();
      }
    });
  }

  for (auto &t : threads) {
    t.join();
  }

  const int expected_count = num_threads * burst_size * num_bursts;
  EXPECT_EQ(total_enqueued.load(), expected_count);

  std::set<int> received_values;
  int dequeued_count = 0;
  while (auto value = queue.dequeue()) {
    received_values.insert(**value);
    dequeued_count++;
  }

  EXPECT_EQ(dequeued_count, expected_count);
  EXPECT_EQ(received_values.size(), static_cast<size_t>(expected_count));

  for (int i = 0; i < expected_count; ++i) {
    EXPECT_TRUE(received_values.find(i) != received_values.end())
        << "Value " << i << " not found in queue for " << test_name;
  }
}

TEST(ConcurrentEnqueue, NoLoss_2Threads) {
  run_concurrent_enqueue_test(2, 10000, "NoLoss_2Threads");
}

TEST(ConcurrentEnqueue, NoLoss_4Threads) {
  run_concurrent_enqueue_test(4, 10000, "NoLoss_4Threads");
}

TEST(ConcurrentEnqueue, NoLoss_8Threads) {
  run_concurrent_enqueue_test(8, 10000, "NoLoss_8Threads");
}

TEST(ConcurrentEnqueue, NoLoss_16Threads) {
  run_concurrent_enqueue_test(16, 10000, "NoLoss_16Threads");
}

TEST(ConcurrentEnqueue, NoLoss_32Threads) {
  run_concurrent_enqueue_test(32, 10000, "NoLoss_32Threads");
}

TEST(ConcurrentEnqueue, NoLoss_64Threads) {
  run_concurrent_enqueue_test(64, 10000, "NoLoss_64Threads");
}

TEST(ConcurrentEnqueue, UniqueElements_2Threads) {
  run_concurrent_unique_elements_test(2, 10000, "UniqueElements_2Threads");
}

TEST(ConcurrentEnqueue, UniqueElements_4Threads) {
  run_concurrent_unique_elements_test(4, 10000, "UniqueElements_4Threads");
}

TEST(ConcurrentEnqueue, UniqueElements_8Threads) {
  run_concurrent_unique_elements_test(8, 10000, "UniqueElements_8Threads");
}

TEST(ConcurrentEnqueue, UniqueElements_16Threads) {
  run_concurrent_unique_elements_test(16, 10000, "UniqueElements_16Threads");
}

TEST(ConcurrentEnqueue, UniqueElements_32Threads) {
  run_concurrent_unique_elements_test(32, 10000, "UniqueElements_32Threads");
}

TEST(ConcurrentEnqueue, UniqueElements_64Threads) {
  run_concurrent_unique_elements_test(64, 10000, "UniqueElements_64Threads");
}

TEST(ConcurrentEnqueue, DataIntegrity_2Threads) {
  run_concurrent_data_integrity_test(2, 10000, "DataIntegrity_2Threads");
}

TEST(ConcurrentEnqueue, DataIntegrity_4Threads) {
  run_concurrent_data_integrity_test(4, 10000, "DataIntegrity_4Threads");
}

TEST(ConcurrentEnqueue, DataIntegrity_8Threads) {
  run_concurrent_data_integrity_test(8, 10000, "DataIntegrity_8Threads");
}

TEST(ConcurrentEnqueue, DataIntegrity_16Threads) {
  run_concurrent_data_integrity_test(16, 10000, "DataIntegrity_16Threads");
}

TEST(ConcurrentEnqueue, DataIntegrity_32Threads) {
  run_concurrent_data_integrity_test(32, 10000, "DataIntegrity_32Threads");
}

TEST(ConcurrentEnqueue, DataIntegrity_64Threads) {
  run_concurrent_data_integrity_test(64, 10000, "DataIntegrity_64Threads");
}

TEST(ConcurrentEnqueue, Burst_2Threads) {
  run_concurrent_burst_test(2, 100, 100, "Burst_2Threads");
}

TEST(ConcurrentEnqueue, Burst_4Threads) {
  run_concurrent_burst_test(4, 100, 100, "Burst_4Threads");
}

TEST(ConcurrentEnqueue, Burst_8Threads) {
  run_concurrent_burst_test(8, 100, 100, "Burst_8Threads");
}

TEST(ConcurrentEnqueue, Burst_16Threads) {
  run_concurrent_burst_test(16, 100, 100, "Burst_16Threads");
}

TEST(ConcurrentEnqueue, Burst_32Threads) {
  run_concurrent_burst_test(32, 100, 100, "Burst_32Threads");
}

TEST(ConcurrentEnqueue, Burst_64Threads) {
  run_concurrent_burst_test(64, 100, 100, "Burst_64Threads");
}
