#pragma once

#include <atomic>
#include <chrono>
#include <cstdint>
#include <map>
#include <memory>
#include <mutex>
#include <string>
#include <string_view>
#include <vector>

namespace umbra::detail {

enum class InstrumentationLayer {
  rti_ambassador,
  federation_registry,
  callback_dispatch,
  federate_ambassador,
  transport,
  service_reporting,
};

enum class InstrumentationOutcome {
  success,
  failure,
  exception,
};

struct InstrumentationOperationSnapshot final {
  InstrumentationLayer layer = InstrumentationLayer::rti_ambassador;
  std::string name;
  std::uint64_t calls = 0;
  std::uint64_t successes = 0;
  std::uint64_t failures = 0;
  std::uint64_t exceptions = 0;
  std::uint64_t totalDurationNanoseconds = 0;
  std::uint64_t minimumDurationNanoseconds = 0;
  std::uint64_t maximumDurationNanoseconds = 0;
  std::uint64_t activeCalls = 0;
  std::uint64_t peakActiveCalls = 0;
};

struct RuntimeInstrumentationSnapshot final {
  std::vector<InstrumentationOperationSnapshot> operations;
  std::uint64_t nextCorrelationId = 1;
};

struct InstrumentationOperationCell;

class InstrumentationScope final {
 public:
  using Clock = std::chrono::steady_clock;

  InstrumentationScope() = default;
  InstrumentationScope(InstrumentationScope const&) = delete;
  InstrumentationScope& operator=(InstrumentationScope const&) = delete;
  InstrumentationScope(InstrumentationScope&& other) noexcept;
  InstrumentationScope& operator=(InstrumentationScope&& other) noexcept;
  ~InstrumentationScope();

  void complete(InstrumentationOutcome outcome = InstrumentationOutcome::success) noexcept;

 private:
  friend class RuntimeInstrumentation;

  InstrumentationScope(
      std::shared_ptr<InstrumentationOperationCell> cell,
      Clock::time_point startedAt) noexcept;
  void finish(InstrumentationOutcome outcome) noexcept;

  std::shared_ptr<InstrumentationOperationCell> cell_;
  Clock::time_point startedAt_{};
  int uncaughtExceptionsAtStart_ = 0;
  InstrumentationOutcome outcome_ = InstrumentationOutcome::success;
  bool completed_ = true;
};

class RuntimeInstrumentation final
    : public std::enable_shared_from_this<RuntimeInstrumentation> {
 public:
  using Clock = std::chrono::steady_clock;
  using Scope = InstrumentationScope;

  RuntimeInstrumentation() = default;

  [[nodiscard]] Scope begin(
      InstrumentationLayer layer,
      std::string_view name) noexcept;

  [[nodiscard]] Scope beginAt(
      InstrumentationLayer layer,
      std::string_view name,
      Clock::time_point startedAt) noexcept;

  [[nodiscard]] std::uint64_t nextCorrelationId() noexcept;

  [[nodiscard]] RuntimeInstrumentationSnapshot snapshot() const;

 private:
  [[nodiscard]] std::shared_ptr<InstrumentationOperationCell> cellFor(
      InstrumentationLayer layer,
      std::string_view name);

  mutable std::mutex mutex_;
  std::map<
      std::pair<InstrumentationLayer, std::string>,
      std::shared_ptr<InstrumentationOperationCell>>
      cells_;
  std::atomic<std::uint64_t> nextCorrelationId_ = 1;
};

}  // namespace umbra::detail
