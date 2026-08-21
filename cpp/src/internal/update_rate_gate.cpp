#include "internal/update_rate_gate.hpp"

#include <cmath>

namespace umbra::detail {

UpdateRateGate::UpdateRateGate(Now now) : now_(std::move(now)) {}

bool UpdateRateGate::admit(
    std::string const& key,
    double maximumRate,
    bool reliable) {
  if (reliable || maximumRate <= 0.0 || !std::isfinite(maximumRate)) {
    return true;
  }
  auto const current = now_();
  auto const interval = std::chrono::duration<double>(1.0 / maximumRate);
  std::scoped_lock lock(mutex_);
  auto const previous = lastAdmission_.find(key);
  if (previous != lastAdmission_.end() &&
      current - previous->second < interval) {
    return false;
  }
  lastAdmission_[key] = current;
  return true;
}

void UpdateRateGate::erase(std::string const& key) {
  std::scoped_lock lock(mutex_);
  lastAdmission_.erase(key);
}

void UpdateRateGate::clear() {
  std::scoped_lock lock(mutex_);
  lastAdmission_.clear();
}

}  // namespace umbra::detail
