#include "internal/callback_dispatcher.hpp"

#include <algorithm>
#include <utility>

namespace umbra::detail {
namespace {

thread_local std::size_t callbackExecutionDepth = 0;

class CallbackExecutionScope final {
 public:
  CallbackExecutionScope() {
    ++callbackExecutionDepth;
  }

  ~CallbackExecutionScope() {
    --callbackExecutionDepth;
  }
};

}  // namespace

CallbackDispatcher::CallbackDispatcher(CallbackDispatchModel model) : model_(model) {}

void CallbackDispatcher::configure(CallbackDispatchModel model) {
  std::deque<CallbackTask> immediateTasks;
  {
    std::scoped_lock lock(mutex_);
    model_ = model;
    if (model_ == CallbackDispatchModel::immediate && enabled_) {
      immediateTasks = takeAllLocked();
    }
  }
  callbackAvailable_.notify_all();

  for (CallbackTask& callback : immediateTasks) {
    invoke(std::move(callback));
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
  std::deque<CallbackTask> immediateTasks;
  {
    std::scoped_lock lock(mutex_);
    enabled_ = enabled;
    if (enabled_ && model_ == CallbackDispatchModel::immediate) {
      immediateTasks = takeAllLocked();
    }
  }
  callbackAvailable_.notify_all();

  for (CallbackTask& callback : immediateTasks) {
    invoke(std::move(callback));
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

  CallbackTask immediateTask;
  {
    std::scoped_lock lock(mutex_);
    pending_.push_back(std::move(callback));
    if (model_ == CallbackDispatchModel::immediate && enabled_) {
      immediateTask = takeNextLocked();
    }
  }
  callbackAvailable_.notify_one();

  if (immediateTask) {
    invoke(std::move(immediateTask));
  }
}

bool CallbackDispatcher::evokeOne(std::chrono::milliseconds minimumWait) {
  minimumWait = nonNegative(minimumWait);

  CallbackTask callback;
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

  invoke(std::move(callback));

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

    CallbackTask callback = takeNextLocked();
    lock.unlock();
    invoke(std::move(callback));
    lock.lock();

    if (std::chrono::steady_clock::now() >= maximumDeadline) {
      return !pending_.empty();
    }
  }
}

std::chrono::milliseconds CallbackDispatcher::nonNegative(std::chrono::milliseconds value) noexcept {
  return std::max(value, std::chrono::milliseconds::zero());
}

void CallbackDispatcher::invoke(CallbackTask callback) {
  CallbackExecutionScope scope;
  callback();
}

CallbackDispatcher::CallbackTask CallbackDispatcher::takeNextLocked() {
  CallbackTask callback = std::move(pending_.front());
  pending_.pop_front();
  return callback;
}

std::deque<CallbackDispatcher::CallbackTask> CallbackDispatcher::takeAllLocked() {
  return std::exchange(pending_, {});
}

}  // namespace umbra::detail
