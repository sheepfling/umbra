#include "internal/callback_session.hpp"

#include <RTI/FederateAmbassador.h>

#include <utility>

namespace rti1516_2025::umbra_binding_detail {
namespace {

thread_local CallbackSession* activeCallbackSession = nullptr;

class CallbackInvocationScope final {
 public:
  explicit CallbackInvocationScope(CallbackSession& session)
      : previous_(std::exchange(activeCallbackSession, &session)) {}

  ~CallbackInvocationScope() {
    activeCallbackSession = previous_;
  }

 private:
  CallbackSession* previous_;
};

}  // namespace

CallbackSession::CallbackSession(FederateAmbassador& recipient) : recipient_(&recipient) {}

void CallbackSession::close() {
  std::unique_lock lock(mutex_);
  closed_ = true;
  if (activeCallbackSession == this) {
    return;
  }

  noInvocationsInFlight_.wait(lock, [this] { return invocationsInFlight_ == 0; });
  recipient_ = nullptr;
}

void CallbackSession::invoke(Invocation invocation) {
  if (!invocation) {
    return;
  }

  FederateAmbassador* recipient = nullptr;
  {
    std::scoped_lock lock(mutex_);
    if (closed_ || recipient_ == nullptr) {
      return;
    }
    ++invocationsInFlight_;
    recipient = recipient_;
  }

  CallbackInvocationScope callbackScope(*this);
  try {
    invocation(*recipient);
  } catch (...) {
    finishInvocation();
    throw;
  }
  finishInvocation();
}

void CallbackSession::finishInvocation() noexcept {
  std::scoped_lock lock(mutex_);
  --invocationsInFlight_;
  if (closed_ && invocationsInFlight_ == 0) {
    recipient_ = nullptr;
  }
  noInvocationsInFlight_.notify_all();
}

}  // namespace rti1516_2025::umbra_binding_detail
