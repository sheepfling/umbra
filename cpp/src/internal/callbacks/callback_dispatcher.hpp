#pragma once

#include "internal/observability/runtime_instrumentation.hpp"

#include <chrono>
#include <condition_variable>
#include <cstddef>
#include <deque>
#include <functional>
#include <memory>
#include <mutex>
#include <optional>

namespace umbra::detail {

enum class CallbackDispatchModel {
  immediate,
  evoked,
};

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

  // Return whether callbacks remain pending after the invocation. In the
  // immediate model the HLA Evoke services have no effect and return false.
  bool evokeOne(std::chrono::milliseconds minimumWait);
  bool evokeMultiple(
      std::chrono::milliseconds minimumWait,
      std::chrono::milliseconds maximumWait);

 private:
  struct QueuedCallback final {
    CallbackTask callback;
    RuntimeInstrumentation::Clock::time_point submittedAt;
  };

  static std::chrono::milliseconds nonNegative(std::chrono::milliseconds value) noexcept;
  void invoke(QueuedCallback callback, CallbackDispatchModel dispatchModel);

  QueuedCallback takeNextLocked();
  std::deque<QueuedCallback> takeAllLocked();

  mutable std::mutex mutex_;
  std::condition_variable callbackAvailable_;
  CallbackDispatchModel model_;
  bool enabled_ = true;
  std::deque<QueuedCallback> pending_;
  std::shared_ptr<RuntimeInstrumentation> instrumentation_;
};

}  // namespace umbra::detail
