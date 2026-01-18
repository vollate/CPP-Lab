#include "lf_queue.hpp"

#include <gtest/gtest.h>
#include <atomic>
#include <thread>
#include <vector>
#include <set>
#include <unordered_set>

void run_concurrent_dequeue_no_duplicates_test(int num_threads, int queue_size, const std::string& test_name) {
  lf_lab::LFQueue<int> queue;

  for (int i = 0; i < queue_size; ++i) {
    queue.enqueue(i);
  }

  std::unordered_set<int> received_values;
  std::vector<std::thread> threads;
  std::atomic<int> dequeued_count(0);

  for (int i = 0; i < num_threads; ++i) {
    threads.emplace_back([&]() {
      int value;
      while (true) {
        if (queue.dequeue(value)) {
          auto [it, success] = received_values.insert(value);
          if (!success) {
            ADD_FAILURE() << "Duplicate value " << value << " found in " << test_name;
          }
          dequeued_count.fetch_add(1, std::memory_order_relaxed);
        } else {
          break;
        }
      }
    });
  }

  for (auto& t : threads) {
    t.join();
  }

  EXPECT_EQ(dequeued_count.load(), queue_size);
  EXPECT_EQ(received_values.size(), static_cast<size_t>(queue_size));

  for (int i = 0; i < queue_size; ++i) {
    EXPECT_TRUE(received_values.find(i) != received_values.end()) 
        << "Value " << i << " not found in " << test_name;
  }
}

void run_concurrent_dequeue_correct_order_test(int num_threads, int queue_size, const std::string& test_name) {
  lf_lab::LFQueue<int> queue;

  for (int i = 0; i < queue_size; ++i) {
    queue.enqueue(i);
  }

  std::vector<int> received_values;
  std::atomic<int> dequeued_count(0);
  std::mutex mtx;

  std::vector<std::thread> threads;
  for (int i = 0; i < num_threads; ++i) {
    threads.emplace_back([&]() {
      std::vector<int> local_values;
      int value;
      while (true) {
        if (queue.dequeue(value)) {
          local_values.push_back(value);
          dequeued_count.fetch_add(1, std::memory_order_relaxed);
        } else {
          break;
        }
      }
      std::lock_guard<std::mutex> lock(mtx);
      received_values.insert(received_values.end(), local_values.begin(), local_values.end());
    });
  }

  for (auto& t : threads) {
    t.join();
  }

  EXPECT_EQ(dequeued_count.load(), queue_size);
  EXPECT_EQ(received_values.size(), static_cast<size_t>(queue_size));

  std::set<int> sorted_values(received_values.begin(), received_values.end());
  EXPECT_EQ(sorted_values.size(), static_cast<size_t>(queue_size));

  int expected = 0;
  for (int val : sorted_values) {
    EXPECT_EQ(val, expected++) << "Values out of order in " << test_name;
  }
}

void run_concurrent_dequeue_with_empty_test(int num_threads, int queue_size, const std::string& test_name) {
  lf_lab::LFQueue<int> queue;

  for (int i = 0; i < queue_size; ++i) {
    queue.enqueue(i);
  }

  std::vector<std::thread> threads;
  std::atomic<int> empty_dequeue_count(0);
  std::atomic<int> successful_dequeue_count(0);

  for (int i = 0; i < num_threads; ++i) {
    threads.emplace_back([&]() {
      int value;
      for (int j = 0; j < queue_size + 100; ++j) {
        if (queue.dequeue(value)) {
          successful_dequeue_count.fetch_add(1, std::memory_order_relaxed);
        } else {
          empty_dequeue_count.fetch_add(1, std::memory_order_relaxed);
        }
      }
    });
  }

  for (auto& t : threads) {
    t.join();
  }

  EXPECT_EQ(successful_dequeue_count.load(), queue_size);
  EXPECT_GT(empty_dequeue_count.load(), 0);
}

void run_concurrent_dequeue_race_conditions_test(int num_threads, int queue_size, const std::string& test_name) {
  lf_lab::LFQueue<int> queue;

  for (int i = 0; i < queue_size; ++i) {
    queue.enqueue(i);
  }

  std::unordered_set<int> received_values;
  std::vector<std::thread> threads;
  std::atomic<int> dequeued_count(0);

  for (int i = 0; i < num_threads; ++i) {
    threads.emplace_back([&]() {
      int value;
      int attempt = 0;
      while (dequeued_count.load(std::memory_order_relaxed) < queue_size && attempt < queue_size * 2) {
        if (queue.dequeue(value)) {
          received_values.insert(value);
          dequeued_count.fetch_add(1, std::memory_order_relaxed);
        }
        ++attempt;
      }
    });
  }

  for (auto& t : threads) {
    t.join();
  }

  EXPECT_EQ(dequeued_count.load(), queue_size);
  EXPECT_EQ(received_values.size(), static_cast<size_t>(queue_size));

  for (int i = 0; i < queue_size; ++i) {
    EXPECT_TRUE(received_values.find(i) != received_values.end()) 
        << "Value " << i << " not found in " << test_name;
  }
}

TEST(ConcurrentDequeue, NoDuplicates_2Threads) {
  run_concurrent_dequeue_no_duplicates_test(2, 10000, "NoDuplicates_2Threads");
}

TEST(ConcurrentDequeue, NoDuplicates_4Threads) {
  run_concurrent_dequeue_no_duplicates_test(4, 10000, "NoDuplicates_4Threads");
}

TEST(ConcurrentDequeue, NoDuplicates_8Threads) {
  run_concurrent_dequeue_no_duplicates_test(8, 10000, "NoDuplicates_8Threads");
}

TEST(ConcurrentDequeue, NoDuplicates_16Threads) {
  run_concurrent_dequeue_no_duplicates_test(16, 10000, "NoDuplicates_16Threads");
}

TEST(ConcurrentDequeue, NoDuplicates_32Threads) {
  run_concurrent_dequeue_no_duplicates_test(32, 10000, "NoDuplicates_32Threads");
}

TEST(ConcurrentDequeue, NoDuplicates_64Threads) {
  run_concurrent_dequeue_no_duplicates_test(64, 10000, "NoDuplicates_64Threads");
}

TEST(ConcurrentDequeue, CorrectOrder_2Threads) {
  run_concurrent_dequeue_correct_order_test(2, 10000, "CorrectOrder_2Threads");
}

TEST(ConcurrentDequeue, CorrectOrder_4Threads) {
  run_concurrent_dequeue_correct_order_test(4, 10000, "CorrectOrder_4Threads");
}

TEST(ConcurrentDequeue, CorrectOrder_8Threads) {
  run_concurrent_dequeue_correct_order_test(8, 10000, "CorrectOrder_8Threads");
}

TEST(ConcurrentDequeue, CorrectOrder_16Threads) {
  run_concurrent_dequeue_correct_order_test(16, 10000, "CorrectOrder_16Threads");
}

TEST(ConcurrentDequeue, CorrectOrder_32Threads) {
  run_concurrent_dequeue_correct_order_test(32, 10000, "CorrectOrder_32Threads");
}

TEST(ConcurrentDequeue, CorrectOrder_64Threads) {
  run_concurrent_dequeue_correct_order_test(64, 10000, "CorrectOrder_64Threads");
}

TEST(ConcurrentDequeue, WithEmpty_2Threads) {
  run_concurrent_dequeue_with_empty_test(2, 10000, "WithEmpty_2Threads");
}

TEST(ConcurrentDequeue, WithEmpty_4Threads) {
  run_concurrent_dequeue_with_empty_test(4, 10000, "WithEmpty_4Threads");
}

TEST(ConcurrentDequeue, WithEmpty_8Threads) {
  run_concurrent_dequeue_with_empty_test(8, 10000, "WithEmpty_8Threads");
}

TEST(ConcurrentDequeue, WithEmpty_16Threads) {
  run_concurrent_dequeue_with_empty_test(16, 10000, "WithEmpty_16Threads");
}

TEST(ConcurrentDequeue, WithEmpty_32Threads) {
  run_concurrent_dequeue_with_empty_test(32, 10000, "WithEmpty_32Threads");
}

TEST(ConcurrentDequeue, WithEmpty_64Threads) {
  run_concurrent_dequeue_with_empty_test(64, 10000, "WithEmpty_64Threads");
}

TEST(ConcurrentDequeue, RaceConditions_2Threads) {
  run_concurrent_dequeue_race_conditions_test(2, 10000, "RaceConditions_2Threads");
}

TEST(ConcurrentDequeue, RaceConditions_4Threads) {
  run_concurrent_dequeue_race_conditions_test(4, 10000, "RaceConditions_4Threads");
}

TEST(ConcurrentDequeue, RaceConditions_8Threads) {
  run_concurrent_dequeue_race_conditions_test(8, 10000, "RaceConditions_8Threads");
}

TEST(ConcurrentDequeue, RaceConditions_16Threads) {
  run_concurrent_dequeue_race_conditions_test(16, 10000, "RaceConditions_16Threads");
}

TEST(ConcurrentDequeue, RaceConditions_32Threads) {
  run_concurrent_dequeue_race_conditions_test(32, 10000, "RaceConditions_32Threads");
}

TEST(ConcurrentDequeue, RaceConditions_64Threads) {
  run_concurrent_dequeue_race_conditions_test(64, 10000, "RaceConditions_64Threads");
}
