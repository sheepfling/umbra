#pragma once

#include "internal/observability/runtime_instrumentation_fwd.hpp"

#include <chrono>
#include <condition_variable>
#include <cstddef>
#include <deque>
#include <functional>
#include <memory>
#include <mutex>
#include <optional>
#include <string_view>
#include <thread>

namespace umbra::detail {

enum class CallbackDispatchModel {
  immediate,
  evoked,
};

namespace callback_dispatch_instrumentation {
inline constexpr std::string_view queue_delay_immediate = "queue_delay.immediate";
inline constexpr std::string_view queue_delay_evoked = "queue_delay.evoked";
inline constexpr std::string_view execute_immediate = "execute.immediate";
inline constexpr std::string_view execute_evoked = "execute.evoked";
}  // namespace callback_dispatch_instrumentation

// Private callback-delivery context. CallbackSession uses this only while a
// dispatcher task is executing so actual FederateAmbassador timing retains the
// selected immediate/evoked model without changing the public callback API.
[[nodiscard]] std::optional<CallbackDispatchModel>
currentCallbackDispatchModel() noexcept;

// Private implementation of the two HLA callback models. Service adapters
// own conversion to the public CallbackModel enum and must never hold their
// state mutex while one of these tasks invokes federate code.
class CallbackDispatcher final {
 public:
  using CallbackTask = std::function<void()>;

  explicit CallbackDispatcher(
      CallbackDispatchModel model = CallbackDispatchModel::evoked,
      std::shared_ptr<RuntimeInstrumentation> instrumentation = {});

  void configure(CallbackDispatchModel model);
  void reset();
  void setEnabled(bool enabled);

  [[nodiscard]] bool isEnabled() const;
  [[nodiscard]] std::size_t pendingCount() const;
  [[nodiscard]] bool isExecutingCallback() const noexcept;

  // Submitting a task invokes it immediately only for an enabled immediate
  // dispatcher. Disabled tasks remain pending until callbacks are enabled.
  void submit(CallbackTask callback);

  // Return whether callbacks remain pending after the invocation. When
  // callbackInvoked is non-null, also report whether this call invoked one;
  // callers must not infer that from the return value because a final callback
  // leaves no pending work. In the immediate model the HLA Evoke services
  // have no effect and return false.
  bool evokeOne(
      std::chrono::milliseconds minimumWait,
      bool* callbackInvoked = nullptr);
  bool evokeMultiple(
      std::chrono::milliseconds minimumWait,
      std::chrono::milliseconds maximumWait);

 private:
  struct QueuedCallback final {
    CallbackTask callback;
    std::chrono::steady_clock::time_point submittedAt;
  };

  static std::chrono::milliseconds nonNegative(std::chrono::milliseconds value) noexcept;
  // Drain an enabled immediate queue while retaining the dispatcher-level
  // serialization lock.  A single drainer owns the queue until it becomes
  // disabled, changes model, or empty; callbacks submitted reentrantly are
  // therefore appended behind the current callback instead of being invoked
  // concurrently by another producer thread.
  void drainImmediate();
  void invoke(const QueuedCallback &callback, CallbackDispatchModel dispatchModel) const;

  QueuedCallback takeNextLocked();
  std::deque<QueuedCallback> takeAllLocked();

  mutable std::mutex mutex_;
  std::condition_variable callbackAvailable_;
  CallbackDispatchModel model_;
  bool enabled_ = true;
  // Protected by mutex_.  This is distinct from dispatchMutex_: it elects one
  // immediate drainer, while dispatchMutex_ also serializes evoked callback
  // extraction/invocation so a FederateAmbassador is never entered
  // concurrently by this dispatcher.
  bool dispatchingImmediate_ = false;
  std::thread::id immediateDrainer_;
  std::size_t immediateDrainDepth_ = 0;
  std::deque<QueuedCallback> pending_;
  std::shared_ptr<RuntimeInstrumentation> instrumentation_;
  std::recursive_mutex dispatchMutex_;
};

}  // namespace umbra::detail
