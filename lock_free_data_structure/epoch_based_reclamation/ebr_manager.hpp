#pragma once

#include "ebr_ptr.hpp"

#include <atomic>

namespace lf_lab {
template <typename DataType> class EBRManager {
public:
  EBRManager() : epoch_(0) {};
  ~EBRManager() {};
  EBRManager(const EBRManager &) = delete;
  EBRManager(EBRManager &&) = delete;
  auto operator=(const EBRManager &) -> EBRManager & = delete;
  auto operator=(EBRManager &&) -> EBRManager & = delete;

  auto allocate_ptr() -> EBRPtr<DataType> {};

  auto start_recycle() -> bool {};
  void stop_recycle() {};

private:
  auto update_epoch() { uint8_t current_epoch_ = epoch_.load(); }

  std::atomic<uint8_t> epoch_;
};

} // namespace lf_lab
