#include "internal/callbacks/callback_dispatcher.hpp"

#include <algorithm>
#include <optional>
#include <utility>

namespace umbra::detail {
namespace {

thread_local std::size_t callbackExecutionDepth = 0;
thread_local std::optional<CallbackDispatchModel> callbackDispatchModel;

class CallbackExecutionScope final {
 public:
  explicit CallbackExecutionScope(CallbackDispatchModel dispatchModel)
      : previousModel_(callbackDispatchModel) {
    ++callbackExecutionDepth;
    callbackDispatchModel = dispatchModel;
  }

  ~CallbackExecutionScope() {
    --callbackExecutionDepth;
    callbackDispatchModel = previousModel_;
  }

 private:
  std::optional<CallbackDispatchModel> previousModel_;
};

}  // namespace

std::optional<CallbackDispatchModel> currentCallbackDispatchModel() noexcept {
  return callbackDispatchModel;
}

CallbackDispatcher::CallbackDispatcher(
    CallbackDispatchModel model,
    std::shared_ptr<RuntimeInstrumentation> instrumentation)
    : model_(model), instrumentation_(std::move(instrumentation)) {}

void CallbackDispatcher::configure(CallbackDispatchModel model) {
  std::deque<QueuedCallback> immediateTasks;
  {
    std::scoped_lock lock(mutex_);
    model_ = model;
    if (model_ == CallbackDispatchModel::immediate && enabled_) {
      immediateTasks = takeAllLocked();
    }
  }
  callbackAvailable_.notify_all();

  for (QueuedCallback& callback : immediateTasks) {
    invoke(std::move(callback), CallbackDispatchModel::immediate);
  }
}

void CallbackDispatcher::reset() {
  {
    std::scoped_lock lock(mutex_);
    enabled_ = true;
    pending_.clear();
  }
  callbackAvailable_.notify_all();
}

void CallbackDispatcher::setEnabled(bool enabled) {
  std::deque<QueuedCallback> immediateTasks;
  {
    std::scoped_lock lock(mutex_);
    enabled_ = enabled;
    if (enabled_ && model_ == CallbackDispatchModel::immediate) {
      immediateTasks = takeAllLocked();
    }
  }
  callbackAvailable_.notify_all();

  for (QueuedCallback& callback : immediateTasks) {
    invoke(std::move(callback), CallbackDispatchModel::immediate);
  }
}

bool CallbackDispatcher::isEnabled() const {
  std::scoped_lock lock(mutex_);
  return enabled_;
}

std::size_t CallbackDispatcher::pendingCount() const {
  std::scoped_lock lock(mutex_);
  return pending_.size();
}

bool CallbackDispatcher::isExecutingCallback() const noexcept {
  return callbackExecutionDepth != 0;
}

void CallbackDispatcher::submit(CallbackTask callback) {
  if (!callback) {
    return;
  }

  auto submitScope = instrumentation_
      ? instrumentation_->begin(
            InstrumentationLayer::callback_dispatch,
            "submit")
      : RuntimeInstrumentation::Scope{};
  QueuedCallback immediateTask;
  {
    std::scoped_lock lock(mutex_);
    pending_.push_back({std::move(callback), RuntimeInstrumentation::Clock::now()});
    if (model_ == CallbackDispatchModel::immediate && enabled_) {
      immediateTask = takeNextLocked();
    }
  }
  callbackAvailable_.notify_one();

  if (immediateTask.callback) {
    invoke(std::move(immediateTask), CallbackDispatchModel::immediate);
  }
}

bool CallbackDispatcher::evokeOne(std::chrono::milliseconds minimumWait) {
  minimumWait = nonNegative(minimumWait);

  QueuedCallback callback;
  {
    std::unique_lock lock(mutex_);
    if (model_ != CallbackDispatchModel::evoked) {
      return false;
    }
    if (!enabled_) {
      return !pending_.empty();
    }

    if (pending_.empty() && minimumWait.count() > 0) {
      callbackAvailable_.wait_for(lock, minimumWait, [this] {
        return !pending_.empty() || !enabled_;
      });
    }
    if (!enabled_ || pending_.empty()) {
      return !pending_.empty();
    }
    callback = takeNextLocked();
  }

  invoke(std::move(callback), CallbackDispatchModel::evoked);

  std::scoped_lock lock(mutex_);
  return !pending_.empty();
}

bool CallbackDispatcher::evokeMultiple(
    std::chrono::milliseconds minimumWait,
    std::chrono::milliseconds maximumWait) {
  minimumWait = nonNegative(minimumWait);
  maximumWait = std::max(nonNegative(maximumWait), minimumWait);

  std::unique_lock lock(mutex_);
  if (model_ != CallbackDispatchModel::evoked) {
    return false;
  }
  if (!enabled_) {
    return !pending_.empty();
  }

  auto const startedAt = std::chrono::steady_clock::now();
  auto const minimumDeadline = startedAt + minimumWait;
  auto const maximumDeadline = startedAt + maximumWait;

  while (true) {
    if (!enabled_) {
      return !pending_.empty();
    }

    if (pending_.empty()) {
      if (std::chrono::steady_clock::now() >= minimumDeadline) {
        return false;
      }
      callbackAvailable_.wait_until(lock, minimumDeadline, [this] {
        return !pending_.empty() || !enabled_;
      });
      continue;
    }

    QueuedCallback callback = takeNextLocked();
    lock.unlock();
    invoke(std::move(callback), CallbackDispatchModel::evoked);
    lock.lock();

    if (std::chrono::steady_clock::now() >= maximumDeadline) {
      return !pending_.empty();
    }
  }
}

std::chrono::milliseconds CallbackDispatcher::nonNegative(std::chrono::milliseconds value) noexcept {
  return std::max(value, std::chrono::milliseconds::zero());
}

void CallbackDispatcher::invoke(
    QueuedCallback callback,
    CallbackDispatchModel dispatchModel) {
  CallbackExecutionScope scope(dispatchModel);
  auto const queueDelayName = dispatchModel == CallbackDispatchModel::immediate
      ? "queue_delay.immediate"
      : "queue_delay.evoked";
  auto const executeName = dispatchModel == CallbackDispatchModel::immediate
      ? "execute.immediate"
      : "execute.evoked";
  auto queueDelayScope = instrumentation_
      ? instrumentation_->beginAt(
            InstrumentationLayer::callback_dispatch,
            queueDelayName,
            callback.submittedAt)
      : RuntimeInstrumentation::Scope{};
  queueDelayScope.complete();
  auto executionScope = instrumentation_
      ? instrumentation_->begin(
            InstrumentationLayer::callback_dispatch,
            executeName)
      : RuntimeInstrumentation::Scope{};
  callback.callback();
}

CallbackDispatcher::QueuedCallback CallbackDispatcher::takeNextLocked() {
  QueuedCallback callback = std::move(pending_.front());
  pending_.pop_front();
  return callback;
}

std::deque<CallbackDispatcher::QueuedCallback> CallbackDispatcher::takeAllLocked() {
  return std::exchange(pending_, {});
}

}  // namespace umbra::detail
