#pragma once

#include <chrono>
#include <functional>
#include <mutex>
#include <string>
#include <unordered_map>

namespace umbra::detail {

// Private wall-clock admission primitive for IEEE 1516.1 update-rate
// reduction.  Logical timestamps are deliberately not consulted here.
class UpdateRateGate final {
 public:
  using Clock = std::chrono::steady_clock;
  using Now = std::function<Clock::time_point()>;

  explicit UpdateRateGate(Now now = [] { return Clock::now(); });

  // Reliable transportation is never dropped by this gate.  A non-positive
  // rate denotes HLAdefault/no reduction.  The key must identify one joined
  // recipient and attribute/passel stream for the lifetime of that join.
  [[nodiscard]] bool admit(
      std::string const& key,
      double maximumRate,
      bool reliable);

  void erase(std::string const& key);
  void clear();

 private:
  Now now_;
  std::unordered_map<std::string, Clock::time_point> lastAdmission_;
  std::mutex mutex_;
};

}  // namespace umbra::detail
