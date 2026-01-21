#include "lf_queue.hpp"

#include <atomic>
#include <condition_variable>
#include <gtest/gtest.h>
#include <mutex>
#include <thread>
#include <unordered_set>
#include <vector>

void run_producer_consumer_test(int num_threads, int ops_per_thread,
                                const std::string &test_name) {
  lf_lab::LFQueue<int> queue;
  std::atomic_int produced_count(0);
  std::atomic_int consumed_count(0);
  std::unordered_set<int> consumed_values;
  std::mutex consumed_mutex;

  int num_producers = num_threads / 2;
  if (num_producers == 0)
    num_producers = 1;
  int num_consumers = num_threads - num_producers;

  std::vector<std::thread> threads;

  for (int i = 0; i < num_producers; ++i) {
    threads.emplace_back([&, producer_id = i]() {
      for (int j = 0; j < ops_per_thread; ++j) {
        int value = producer_id * ops_per_thread + j;
        queue.enqueue(value);
        produced_count.fetch_add(1, std::memory_order_relaxed);
      }
    });
  }

  for (int i = 0; i < num_consumers; ++i) {
    threads.emplace_back([&]() {
      int attempts = 0;
      while (consumed_count.load(std::memory_order_relaxed) <
                 (num_producers * ops_per_thread) &&
             attempts < ops_per_thread * 2) {
        auto value = queue.dequeue();
        if (value) {
          std::lock_guard<std::mutex> lock(consumed_mutex);
          auto [it, success] = consumed_values.insert(**value);
          if (!success) {
            ADD_FAILURE() << "Duplicate value " << **value << " found in "
                          << test_name;
          }
          consumed_count.fetch_add(1, std::memory_order_relaxed);
        }
        ++attempts;
      }
    });
  }

  for (auto &t : threads) {
    t.join();
  }

  int expected_produced = num_producers * ops_per_thread;
  EXPECT_EQ(produced_count.load(), expected_produced);
  EXPECT_EQ(consumed_count.load(), expected_produced);
  EXPECT_EQ(consumed_values.size(), static_cast<size_t>(expected_produced));

  for (int i = 0; i < expected_produced; ++i) {
    EXPECT_TRUE(consumed_values.find(i) != consumed_values.end())
        << "Value " << i << " not found in " << test_name;
  }
}

void run_high_contention_test(int num_threads, int ops_per_thread,
                              const std::string &test_name) {
  lf_lab::LFQueue<int> queue;
  std::atomic_int enqueue_count(0);
  std::atomic_int dequeue_count(0);
  std::unordered_set<int> received_values;
  std::mutex mtx;

  std::vector<std::thread> threads;

  for (int i = 0; i < num_threads; ++i) {
    threads.emplace_back([&, thread_id = i]() {
      for (int j = 0; j < ops_per_thread; ++j) {
        int value = thread_id * ops_per_thread + j;
        queue.enqueue(value);
        enqueue_count.fetch_add(1, std::memory_order_relaxed);

        auto dequeued = queue.dequeue();
        if (dequeued) {
          std::lock_guard<std::mutex> lock(mtx);
          received_values.insert(**dequeued);
          dequeue_count.fetch_add(1, std::memory_order_relaxed);
        }
      }
    });
  }

  for (auto &t : threads) {
    t.join();
  }

  int expected_total = num_threads * ops_per_thread;
  EXPECT_EQ(enqueue_count.load(), expected_total);

  while (true) {
    auto value = queue.dequeue();
    if (!value) {
      break;
    }
    std::lock_guard<std::mutex> lock(mtx);
    received_values.insert(**value);
    dequeue_count.fetch_add(1, std::memory_order_relaxed);
  }

  EXPECT_EQ(dequeue_count.load(), expected_total);
  EXPECT_EQ(received_values.size(), static_cast<size_t>(expected_total));

  for (int i = 0; i < expected_total; ++i) {
    EXPECT_TRUE(received_values.find(i) != received_values.end())
        << "Value " << i << " not found in " << test_name;
  }
}

void run_alternating_test(int num_threads, int cycles,
                          const std::string &test_name) {
  lf_lab::LFQueue<int> queue;
  std::atomic_int produced_count(0);
  std::atomic_int consumed_count(0);
  std::unordered_set<int> received_values;
  std::mutex mtx;

  std::vector<std::thread> threads;

  for (int i = 0; i < num_threads; ++i) {
    threads.emplace_back([&, thread_id = i]() {
      for (int cycle = 0; cycle < cycles; ++cycle) {
        int value = thread_id * cycles + cycle;
        queue.enqueue(value);
        produced_count.fetch_add(1, std::memory_order_relaxed);
      }
    });
  }

  for (int cycle = 0; cycle < cycles; ++cycle) {
    for (auto &t : threads) {
      t.join();
    }
    threads.clear();

    for (int i = 0; i < num_threads; ++i) {
      threads.emplace_back([&]() {
        while (auto value = queue.dequeue()) {
          std::lock_guard<std::mutex> lock(mtx);
          received_values.insert(**value);
          consumed_count.fetch_add(1, std::memory_order_relaxed);
        }
      });
    }
  }

  for (auto &t : threads) {
    t.join();
  }

  int expected_total = num_threads * cycles;
  EXPECT_EQ(produced_count.load(), expected_total);
  EXPECT_EQ(consumed_count.load(), expected_total);
  EXPECT_EQ(received_values.size(), static_cast<size_t>(expected_total));

  for (int i = 0; i < expected_total; ++i) {
    EXPECT_TRUE(received_values.find(i) != received_values.end())
        << "Value " << i << " not found in " << test_name;
  }
}

void run_burst_producer_consumer_test(int num_threads, int burst_size,
                                      int num_bursts,
                                      const std::string &test_name) {
  lf_lab::LFQueue<int> queue;
  std::atomic_int produced_count(0);
  std::atomic_int consumed_count(0);
  std::unordered_set<int> received_values;
  std::mutex mtx;

  int num_producers = num_threads / 2;
  if (num_producers == 0)
    num_producers = 1;
  int num_consumers = num_threads - num_producers;

  std::vector<std::thread> threads;

  for (int i = 0; i < num_producers; ++i) {
    threads.emplace_back([&, producer_id = i]() {
      for (int burst = 0; burst < num_bursts; ++burst) {
        for (int j = 0; j < burst_size; ++j) {
          int value =
              producer_id * num_bursts * burst_size + burst * burst_size + j;
          queue.enqueue(value);
          produced_count.fetch_add(1, std::memory_order_relaxed);
        }
        std::this_thread::yield();
      }
    });
  }

  for (int i = 0; i < num_consumers; ++i) {
    threads.emplace_back([&]() {
      int attempts = 0;
      int expected_total = num_producers * burst_size * num_bursts;
      while (consumed_count.load(std::memory_order_relaxed) < expected_total &&
             attempts < expected_total * 2) {
        auto value = queue.dequeue();
        if (value) {
          std::lock_guard<std::mutex> lock(mtx);
          received_values.insert(**value);
          consumed_count.fetch_add(1, std::memory_order_relaxed);
        }
        ++attempts;
      }
    });
  }

  for (auto &t : threads) {
    t.join();
  }

  int expected_total = num_producers * burst_size * num_bursts;
  EXPECT_EQ(produced_count.load(), expected_total);
  EXPECT_EQ(consumed_count.load(), expected_total);
  EXPECT_EQ(received_values.size(), static_cast<size_t>(expected_total));

  for (int i = 0; i < expected_total; ++i) {
    EXPECT_TRUE(received_values.find(i) != received_values.end())
        << "Value " << i << " not found in " << test_name;
  }
}

void run_rapid_empty_transition_test(int num_threads, int iterations,
                                     const std::string &test_name) {
  lf_lab::LFQueue<int> queue;
  std::atomic_int produced_count(0);
  std::atomic_int consumed_count(0);
  std::unordered_set<int> received_values;
  std::mutex mtx;

  std::vector<std::thread> threads;

  for (int i = 0; i < num_threads; ++i) {
    threads.emplace_back([&, thread_id = i]() {
      for (int iter = 0; iter < iterations; ++iter) {
        queue.enqueue(thread_id * iterations + iter);
        produced_count.fetch_add(1, std::memory_order_relaxed);

        auto value = queue.dequeue();
        if (value) {
          std::lock_guard<std::mutex> lock(mtx);
          received_values.insert(**value);
          consumed_count.fetch_add(1, std::memory_order_relaxed);
        }
      }
    });
  }

  for (auto &t : threads) {
    t.join();
  }

  int expected_total = num_threads * iterations;
  EXPECT_EQ(produced_count.load(), expected_total);

  while (true) {
    auto value = queue.dequeue();
    if (!value) {
      break;
    }
    std::lock_guard<std::mutex> lock(mtx);
    received_values.insert(**value);
    consumed_count.fetch_add(1, std::memory_order_relaxed);
  }

  EXPECT_EQ(consumed_count.load(), expected_total);
  EXPECT_EQ(received_values.size(), static_cast<size_t>(expected_total));

  for (int i = 0; i < expected_total; ++i) {
    EXPECT_TRUE(received_values.find(i) != received_values.end())
        << "Value " << i << " not found in " << test_name;
  }
}

TEST(Mixed, ProducerConsumer_2Threads) {
  run_producer_consumer_test(2, 10000, "ProducerConsumer_2Threads");
}

TEST(Mixed, ProducerConsumer_4Threads) {
  run_producer_consumer_test(4, 10000, "ProducerConsumer_4Threads");
}

TEST(Mixed, ProducerConsumer_8Threads) {
  run_producer_consumer_test(8, 10000, "ProducerConsumer_8Threads");
}

TEST(Mixed, ProducerConsumer_16Threads) {
  run_producer_consumer_test(16, 10000, "ProducerConsumer_16Threads");
}

TEST(Mixed, ProducerConsumer_32Threads) {
  run_producer_consumer_test(32, 10000, "ProducerConsumer_32Threads");
}

TEST(Mixed, ProducerConsumer_64Threads) {
  run_producer_consumer_test(64, 10000, "ProducerConsumer_64Threads");
}

TEST(Mixed, HighContention_2Threads) {
  run_high_contention_test(2, 5000, "HighContention_2Threads");
}

TEST(Mixed, HighContention_4Threads) {
  run_high_contention_test(4, 5000, "HighContention_4Threads");
}

TEST(Mixed, HighContention_8Threads) {
  run_high_contention_test(8, 5000, "HighContention_8Threads");
}

TEST(Mixed, HighContention_16Threads) {
  run_high_contention_test(16, 5000, "HighContention_16Threads");
}

TEST(Mixed, HighContention_32Threads) {
  run_high_contention_test(32, 5000, "HighContention_32Threads");
}

TEST(Mixed, HighContention_64Threads) {
  run_high_contention_test(64, 5000, "HighContention_64Threads");
}

TEST(Mixed, Alternating_2Threads) {
  run_alternating_test(2, 100, "Alternating_2Threads");
}

TEST(Mixed, Alternating_4Threads) {
  run_alternating_test(4, 100, "Alternating_4Threads");
}

TEST(Mixed, Alternating_8Threads) {
  run_alternating_test(8, 100, "Alternating_8Threads");
}

TEST(Mixed, Alternating_16Threads) {
  run_alternating_test(16, 100, "Alternating_16Threads");
}

TEST(Mixed, Alternating_32Threads) {
  run_alternating_test(32, 100, "Alternating_32Threads");
}

TEST(Mixed, Alternating_64Threads) {
  run_alternating_test(64, 100, "Alternating_64Threads");
}

TEST(Mixed, BurstProducerConsumer_2Threads) {
  run_burst_producer_consumer_test(2, 100, 100,
                                   "BurstProducerConsumer_2Threads");
}

TEST(Mixed, BurstProducerConsumer_4Threads) {
  run_burst_producer_consumer_test(4, 100, 100,
                                   "BurstProducerConsumer_4Threads");
}

TEST(Mixed, BurstProducerConsumer_8Threads) {
  run_burst_producer_consumer_test(8, 100, 100,
                                   "BurstProducerConsumer_8Threads");
}

TEST(Mixed, BurstProducerConsumer_16Threads) {
  run_burst_producer_consumer_test(16, 100, 100,
                                   "BurstProducerConsumer_16Threads");
}

TEST(Mixed, BurstProducerConsumer_32Threads) {
  run_burst_producer_consumer_test(32, 100, 100,
                                   "BurstProducerConsumer_32Threads");
}

TEST(Mixed, BurstProducerConsumer_64Threads) {
  run_burst_producer_consumer_test(64, 100, 100,
                                   "BurstProducerConsumer_64Threads");
}

TEST(Mixed, RapidEmptyTransition_2Threads) {
  run_rapid_empty_transition_test(2, 5000, "RapidEmptyTransition_2Threads");
}

TEST(Mixed, RapidEmptyTransition_4Threads) {
  run_rapid_empty_transition_test(4, 5000, "RapidEmptyTransition_4Threads");
}

TEST(Mixed, RapidEmptyTransition_8Threads) {
  run_rapid_empty_transition_test(8, 5000, "RapidEmptyTransition_8Threads");
}

TEST(Mixed, RapidEmptyTransition_16Threads) {
  run_rapid_empty_transition_test(16, 5000, "RapidEmptyTransition_16Threads");
}

TEST(Mixed, RapidEmptyTransition_32Threads) {
  run_rapid_empty_transition_test(32, 5000, "RapidEmptyTransition_32Threads");
}

TEST(Mixed, RapidEmptyTransition_64Threads) {
  run_rapid_empty_transition_test(64, 5000, "RapidEmptyTransition_64Threads");
}
