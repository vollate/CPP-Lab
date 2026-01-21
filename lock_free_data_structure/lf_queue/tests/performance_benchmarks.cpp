#include "lf_queue.hpp"

#include <algorithm>
#include <atomic>
#include <benchmark/benchmark.h>
#include <cstdio>
#include <queue>
#include <thread>
#include <vector>

namespace basic_lab {

template <typename DataType> class UnsafeQueue {
  struct Node;

private:
  void _enqueue_impl(Node *const new_tail) {
    tail_->next_ = new_tail;
    tail_ = new_tail;
  }

public:
  UnsafeQueue() : head_(&head_guard_), tail_(&head_guard_) {}
  UnsafeQueue(const UnsafeQueue &) = delete;
  UnsafeQueue &operator=(const UnsafeQueue &) = delete;
  ~UnsafeQueue();

  void enqueue(const DataType &data) {
    Node *new_tail = new Node(data);
    _enqueue_impl(new_tail);
  }

  void enqueue(DataType &&data) {
    Node *new_tail = new Node(std::move(data));
    _enqueue_impl(new_tail);
  }

  auto dequeue(DataType &result) -> bool {
    if (empty()) {
      return false;
    }
    Node *to_pop = head_->next_;
    result = std::move(*to_pop->data_);
    if (head_->next_ == tail_) {
      tail_ = head_;
      head_->next_ = nullptr;
    } else {
      head_->next_ = to_pop->next_;
    }
    delete to_pop;
    return true;
  }

  void clear() {
    while (head_->next_) {
      Node *to_pop = head_->next_;
      head_->next_ = to_pop->next_;
      delete to_pop;
    }
  }

  auto empty() -> bool const { return head_->next_ == nullptr; }

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

template <typename DataType> UnsafeQueue<DataType>::~UnsafeQueue() { clear(); }

} // namespace basic_lab

static void BM_SingleThreadEnqueue(benchmark::State &state) {
  lf_lab::LFQueue<int> queue;
  for (auto _ : state) {
    for (int i = 0; i < 1000; ++i) {
      queue.enqueue(i);
    }
    queue.clear();
    state.PauseTiming();
    queue.clear();
    state.ResumeTiming();
  }
  state.SetItemsProcessed(state.iterations() * 1000);
}

static void BM_SingleThreadDequeue(benchmark::State &state) {
  lf_lab::LFQueue<int> queue;
  for (auto _ : state) {
    state.PauseTiming();
    for (int i = 0; i < 1000; ++i) {
      queue.enqueue(i);
    }
    state.ResumeTiming();

    for (int i = 0; i < 1000; ++i) {
      benchmark::DoNotOptimize(queue.dequeue());
    }
  }
  state.SetItemsProcessed(state.iterations() * 1000);
}

static void BM_SingleThreadMixed(benchmark::State &state) {
  lf_lab::LFQueue<int> queue;
  for (auto _ : state) {
    state.PauseTiming();
    for (int i = 0; i < 500; ++i) {
      queue.enqueue(i);
    }
    state.ResumeTiming();

    for (int i = 0; i < 500; ++i) {
      queue.enqueue(500 + i);
      benchmark::DoNotOptimize(queue.dequeue());
    }
    queue.clear();
  }
  state.SetItemsProcessed(state.iterations() * 1000);
}

static void BM_ConcurrentEnqueue(benchmark::State &state) {
  const int num_threads = state.threads();
  const int ops_per_thread = 10000;
  std::atomic_int ready_flag(0);
  std::vector<std::thread> threads;

  for (auto _ : state) {
    lf_lab::LFQueue<int> queue;

    for (int t = 0; t < num_threads; ++t) {
      threads.emplace_back(
          [&queue, t, ops_per_thread, &ready_flag, &num_threads]() {
            while (ready_flag.load(std::memory_order_acquire) < num_threads) {
              ready_flag.fetch_add(1, std::memory_order_release);
            }
            for (int i = 0; i < ops_per_thread; ++i) {
              queue.enqueue(t * ops_per_thread + i);
            }
          });
    }

    for (auto &thread : threads) {
      thread.join();
    }
    threads.clear();

    queue.clear();
    ready_flag.store(0);
  }
  state.SetItemsProcessed(state.iterations() * num_threads * ops_per_thread);
}

static void BM_ConcurrentDequeue(benchmark::State &state) {
  const int num_threads = state.threads();
  const int ops_per_thread = 10000;
  const int total_elements = num_threads * ops_per_thread;
  std::atomic_int ready_flag(0);
  std::vector<std::thread> threads;

  for (auto _ : state) {
    lf_lab::LFQueue<int> queue;

    for (int i = 0; i < total_elements; ++i) {
      queue.enqueue(i);
    }

    for (int t = 0; t < num_threads; ++t) {
      threads.emplace_back(
          [&queue, t, ops_per_thread, &ready_flag, &num_threads]() {
            while (ready_flag.load(std::memory_order_acquire) < num_threads) {
              ready_flag.fetch_add(1, std::memory_order_release);
            }
            int dequeued = 0;
            while (dequeued < ops_per_thread) {
              if (queue.dequeue()) {
                dequeued++;
              }
            }
          });
    }

    for (auto &thread : threads) {
      thread.join();
    }
    threads.clear();

    queue.clear();
    ready_flag.store(0);
  }
  state.SetItemsProcessed(state.iterations() * num_threads * ops_per_thread);
}

static void BM_MixedProducerConsumer(benchmark::State &state) {
  const int num_threads = state.threads();
  const int ops_per_thread = 5000;
  const int num_producers = num_threads / 2;
  const int num_consumers = num_threads - num_producers;
  std::atomic_int ready_flag(0);
  std::vector<std::thread> threads;

  for (auto _ : state) {
    lf_lab::LFQueue<int> queue;

    for (int t = 0; t < num_producers; ++t) {
      threads.emplace_back(
          [&queue, t, ops_per_thread, &ready_flag, &num_threads]() {
            while (ready_flag.load(std::memory_order_acquire) < num_threads) {
              ready_flag.fetch_add(1, std::memory_order_release);
            }
            for (int i = 0; i < ops_per_thread; ++i) {
              queue.enqueue(t * ops_per_thread + i);
            }
          });
    }

    for (int t = 0; t < num_consumers; ++t) {
      threads.emplace_back(
          [&queue, t, ops_per_thread, &ready_flag, &num_threads]() {
            while (ready_flag.load(std::memory_order_acquire) < num_threads) {
              ready_flag.fetch_add(1, std::memory_order_release);
            }
            int dequeued = 0;
            int attempts = 0;
            while (dequeued < ops_per_thread && attempts < ops_per_thread * 2) {
              if (queue.dequeue()) {
                dequeued++;
              }
              attempts++;
            }
          });
    }

    for (auto &thread : threads) {
      thread.join();
    }
    threads.clear();

    queue.clear();
    ready_flag.store(0);
  }
  state.SetItemsProcessed(state.iterations() * num_producers * ops_per_thread);
}

static void BM_LFQueue_Enqueue(benchmark::State &state) {
  lf_lab::LFQueue<int> queue;
  for (auto _ : state) {
    for (int i = 0; i < 10000; ++i) {
      queue.enqueue(i);
    }
    queue.clear();
  }
  state.SetItemsProcessed(state.iterations() * 10000);
}

static void BM_UnsafeQueue_Enqueue(benchmark::State &state) {
  basic_lab::UnsafeQueue<int> queue;
  for (auto _ : state) {
    for (int i = 0; i < 10000; ++i) {
      queue.enqueue(i);
    }
    queue.clear();
  }
  state.SetItemsProcessed(state.iterations() * 10000);
}

static void BM_LFQueue_Dequeue(benchmark::State &state) {
  lf_lab::LFQueue<int> queue;
  for (auto _ : state) {
    state.PauseTiming();
    for (int i = 0; i < 10000; ++i) {
      queue.enqueue(i);
    }
    state.ResumeTiming();

    for (int i = 0; i < 10000; ++i) {
      benchmark::DoNotOptimize(queue.dequeue());
    }
  }
  state.SetItemsProcessed(state.iterations() * 10000);
}

static void BM_UnsafeQueue_Dequeue(benchmark::State &state) {
  basic_lab::UnsafeQueue<int> queue;
  for (auto _ : state) {
    state.PauseTiming();
    for (int i = 0; i < 10000; ++i) {
      queue.enqueue(i);
    }
    state.ResumeTiming();

    int value;
    for (int i = 0; i < 10000; ++i) {
      benchmark::DoNotOptimize(queue.dequeue(value));
    }
  }
  state.SetItemsProcessed(state.iterations() * 10000);
}

static void BM_SmallDataThroughput(benchmark::State &state) {
  lf_lab::LFQueue<int> queue;
  for (auto _ : state) {
    for (int i = 0; i < 10000; ++i) {
      queue.enqueue(i);
    }
    for (int i = 0; i < 10000; ++i) {
      benchmark::DoNotOptimize(queue.dequeue());
    }
  }
  state.SetItemsProcessed(state.iterations() * 20000);
}

static void BM_LargeDataThroughput(benchmark::State &state) {
  lf_lab::LFQueue<std::vector<int>> queue;
  const int vector_size = 1000;
  for (auto _ : state) {
    for (int i = 0; i < 1000; ++i) {
      queue.enqueue(std::vector<int>(vector_size, i));
    }
    for (int i = 0; i < 1000; ++i) {
      benchmark::DoNotOptimize(queue.dequeue());
    }
  }
  state.SetItemsProcessed(state.iterations() * 2000);
}

BENCHMARK(BM_SingleThreadEnqueue)->MinTime(10);
BENCHMARK(BM_SingleThreadDequeue)->MinTime(10);
BENCHMARK(BM_SingleThreadMixed)->MinTime(10);

BENCHMARK(BM_ConcurrentEnqueue)
    ->Threads(2)
    ->Threads(4)
    ->Threads(8)
    ->Threads(16)
    ->Threads(32)
    ->Threads(64)
    ->MinTime(10);

BENCHMARK(BM_ConcurrentDequeue)
    ->Threads(2)
    ->Threads(4)
    ->Threads(8)
    ->Threads(16)
    ->Threads(32)
    ->Threads(64)
    ->MinTime(10);

BENCHMARK(BM_MixedProducerConsumer)
    ->Threads(2)
    ->Threads(4)
    ->Threads(8)
    ->Threads(16)
    ->Threads(32)
    ->Threads(64)
    ->MinTime(10);

BENCHMARK(BM_LFQueue_Enqueue)->MinTime(10);
BENCHMARK(BM_UnsafeQueue_Enqueue)->MinTime(10);

BENCHMARK(BM_LFQueue_Dequeue)->MinTime(10);
BENCHMARK(BM_UnsafeQueue_Dequeue)->MinTime(10);

BENCHMARK(BM_SmallDataThroughput)->MinTime(10);
BENCHMARK(BM_LargeDataThroughput)->MinTime(10);

BENCHMARK_MAIN();
