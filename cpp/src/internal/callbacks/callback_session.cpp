#include "internal/callbacks/callback_session.hpp"

#include "internal/callbacks/callback_dispatcher.hpp"

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

CallbackSession::CallbackSession(
    FederateAmbassador& recipient,
    std::shared_ptr<umbra::detail::RuntimeInstrumentation> instrumentation)
    : recipient_(&recipient), instrumentation_(std::move(instrumentation)) {}

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

  auto const dispatchModel = umbra::detail::currentCallbackDispatchModel();
  auto const callbackOperation =
      dispatchModel && *dispatchModel == umbra::detail::CallbackDispatchModel::immediate
      ? "invoke.immediate"
      : dispatchModel && *dispatchModel == umbra::detail::CallbackDispatchModel::evoked
          ? "invoke.evoked"
          : "invoke";
  auto instrumentationScope = instrumentation_
      ? instrumentation_->begin(
            umbra::detail::InstrumentationLayer::federate_ambassador,
            callbackOperation)
      : umbra::detail::RuntimeInstrumentation::Scope{};
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
