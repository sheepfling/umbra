#include "internal/runtime_instrumentation.hpp"

#include <algorithm>
#include <exception>
#include <limits>
#include <utility>

namespace umbra::detail {

struct RuntimeInstrumentation::OperationCell final {
  OperationCell(InstrumentationLayer layer, std::string name)
      : layer(layer), name(std::move(name)) {}

  InstrumentationLayer layer;
  std::string name;
  std::atomic<std::uint64_t> calls = 0;
  std::atomic<std::uint64_t> successes = 0;
  std::atomic<std::uint64_t> failures = 0;
  std::atomic<std::uint64_t> exceptions = 0;
  std::atomic<std::uint64_t> totalDurationNanoseconds = 0;
  std::atomic<std::uint64_t> minimumDurationNanoseconds =
      std::numeric_limits<std::uint64_t>::max();
  std::atomic<std::uint64_t> maximumDurationNanoseconds = 0;
  std::atomic<std::uint64_t> activeCalls = 0;
  std::atomic<std::uint64_t> peakActiveCalls = 0;
};

namespace {

void updateMinimum(
    std::atomic<std::uint64_t>& target,
    std::uint64_t value) noexcept {
  auto current = target.load(std::memory_order_relaxed);
  while (value < current &&
         !target.compare_exchange_weak(
             current,
             value,
             std::memory_order_relaxed,
             std::memory_order_relaxed)) {
  }
}

void updateMaximum(
    std::atomic<std::uint64_t>& target,
    std::uint64_t value) noexcept {
  auto current = target.load(std::memory_order_relaxed);
  while (value > current &&
         !target.compare_exchange_weak(
             current,
             value,
             std::memory_order_relaxed,
             std::memory_order_relaxed)) {
  }
}

}  // namespace

RuntimeInstrumentation::Scope::Scope(
    std::shared_ptr<OperationCell> cell,
    Clock::time_point startedAt) noexcept
    : cell_(std::move(cell)),
      startedAt_(startedAt),
      uncaughtExceptionsAtStart_(std::uncaught_exceptions()),
      completed_(false) {
  if (cell_) {
    auto const active =
        cell_->activeCalls.fetch_add(1, std::memory_order_relaxed) + 1;
    updateMaximum(cell_->peakActiveCalls, active);
  }
}

RuntimeInstrumentation::Scope::Scope(Scope&& other) noexcept
    : cell_(std::move(other.cell_)),
      startedAt_(other.startedAt_),
      uncaughtExceptionsAtStart_(other.uncaughtExceptionsAtStart_),
      outcome_(other.outcome_),
      completed_(other.completed_) {
  other.completed_ = true;
}

RuntimeInstrumentation::Scope& RuntimeInstrumentation::Scope::operator=(
    Scope&& other) noexcept {
  if (this == &other) {
    return *this;
  }
  if (!completed_) {
    finish(outcome_);
  }
  cell_ = std::move(other.cell_);
  startedAt_ = other.startedAt_;
  uncaughtExceptionsAtStart_ = other.uncaughtExceptionsAtStart_;
  outcome_ = other.outcome_;
  completed_ = other.completed_;
  other.completed_ = true;
  return *this;
}

RuntimeInstrumentation::Scope::~Scope() {
  if (completed_) {
    return;
  }
  auto outcome = outcome_;
  if (std::uncaught_exceptions() > uncaughtExceptionsAtStart_) {
    outcome = InstrumentationOutcome::exception;
  }
  finish(outcome);
}

void RuntimeInstrumentation::Scope::complete(InstrumentationOutcome outcome) noexcept {
  if (completed_) {
    return;
  }
  outcome_ = outcome;
  finish(outcome);
}

void RuntimeInstrumentation::Scope::finish(InstrumentationOutcome outcome) noexcept {
  if (completed_) {
    return;
  }
  completed_ = true;
  if (!cell_) {
    return;
  }

  auto const endedAt = Clock::now();
  auto const elapsed = endedAt >= startedAt_ ? endedAt - startedAt_ : Clock::duration::zero();
  auto const duration = static_cast<std::uint64_t>(
      std::chrono::duration_cast<std::chrono::nanoseconds>(elapsed).count());

  cell_->calls.fetch_add(1, std::memory_order_relaxed);
  switch (outcome) {
    case InstrumentationOutcome::success:
      cell_->successes.fetch_add(1, std::memory_order_relaxed);
      break;
    case InstrumentationOutcome::failure:
      cell_->failures.fetch_add(1, std::memory_order_relaxed);
      break;
    case InstrumentationOutcome::exception:
      cell_->exceptions.fetch_add(1, std::memory_order_relaxed);
      break;
  }
  cell_->totalDurationNanoseconds.fetch_add(duration, std::memory_order_relaxed);
  updateMinimum(cell_->minimumDurationNanoseconds, duration);
  updateMaximum(cell_->maximumDurationNanoseconds, duration);
  cell_->activeCalls.fetch_sub(1, std::memory_order_relaxed);
}

RuntimeInstrumentation::Scope RuntimeInstrumentation::begin(
    InstrumentationLayer layer,
    std::string_view name) noexcept {
  return beginAt(layer, name, Clock::now());
}

RuntimeInstrumentation::Scope RuntimeInstrumentation::beginAt(
    InstrumentationLayer layer,
    std::string_view name,
    Clock::time_point startedAt) noexcept {
  try {
    return Scope(cellFor(layer, name), startedAt);
  } catch (...) {
    // Diagnostics must never change RTI behavior. A failed metric allocation
    // simply makes this one scope a no-op; later snapshots remain valid for
    // every operation that was registered successfully.
    return Scope{};
  }
}

std::uint64_t RuntimeInstrumentation::nextCorrelationId() noexcept {
  return nextCorrelationId_.fetch_add(1, std::memory_order_relaxed);
}

RuntimeInstrumentationSnapshot RuntimeInstrumentation::snapshot() const {
  std::vector<std::shared_ptr<OperationCell>> cells;
  {
    std::scoped_lock lock(mutex_);
    cells.reserve(cells_.size());
    for (auto const& [key, cell] : cells_) {
      static_cast<void>(key);
      cells.push_back(cell);
    }
  }

  RuntimeInstrumentationSnapshot result;
  result.operations.reserve(cells.size());
  for (auto const& cell : cells) {
    auto const minimum =
        cell->minimumDurationNanoseconds.load(std::memory_order_relaxed);
    result.operations.push_back({
        cell->layer,
        cell->name,
        cell->calls.load(std::memory_order_relaxed),
        cell->successes.load(std::memory_order_relaxed),
        cell->failures.load(std::memory_order_relaxed),
        cell->exceptions.load(std::memory_order_relaxed),
        cell->totalDurationNanoseconds.load(std::memory_order_relaxed),
        minimum == std::numeric_limits<std::uint64_t>::max() ? 0 : minimum,
        cell->maximumDurationNanoseconds.load(std::memory_order_relaxed),
        cell->activeCalls.load(std::memory_order_relaxed),
        cell->peakActiveCalls.load(std::memory_order_relaxed),
    });
  }
  result.nextCorrelationId = nextCorrelationId_.load(std::memory_order_relaxed);
  return result;
}

std::shared_ptr<RuntimeInstrumentation::OperationCell>
RuntimeInstrumentation::cellFor(
    InstrumentationLayer layer,
    std::string_view name) {
  std::scoped_lock lock(mutex_);
  auto const key = std::make_pair(layer, std::string(name));
  auto const found = cells_.find(key);
  if (found != cells_.end()) {
    return found->second;
  }
  auto cell = std::make_shared<OperationCell>(layer, key.second);
  cells_.emplace(key, cell);
  return cell;
}

}  // namespace umbra::detail
