#pragma once

#include "internal/observability/runtime_instrumentation.hpp"

#include <condition_variable>
#include <cstddef>
#include <functional>
#include <memory>
#include <mutex>

namespace rti1516_2025 {
class FederateAmbassador;
}

namespace rti1516_2025::umbra_binding_detail {

// Owns the private delivery lifetime for a caller-owned FederateAmbassador.
// It never takes ownership of the ambassador itself. Instead, Close prevents
// queued work from dereferencing it and waits for an external-thread callback
// that has already started. The same callback may call Disconnect without
// deadlocking; its final return releases the borrowed pointer. Invocation
// entry is serialized for one session, while the recursive gate preserves
// legal same-thread callback re-entry from HLA_IMMEDIATE services.
class CallbackSession final {
 public:
  using Invocation = std::function<void(FederateAmbassador&)>;

  explicit CallbackSession(
      FederateAmbassador& recipient,
      std::shared_ptr<umbra::detail::RuntimeInstrumentation> instrumentation = {});

  CallbackSession(CallbackSession const&) = delete;
  CallbackSession& operator=(CallbackSession const&) = delete;

  void close();
  void invoke(Invocation invocation);

 private:
  void finishInvocation() noexcept;

  mutable std::mutex mutex_;
  std::condition_variable noInvocationsInFlight_;
  FederateAmbassador* recipient_ = nullptr;
  std::size_t invocationsInFlight_ = 0;
  bool closed_ = false;
  std::shared_ptr<umbra::detail::RuntimeInstrumentation> instrumentation_;
  // Keep one caller-owned FederateAmbassador from being entered by multiple
  // RTI paths at once. This is recursive because an immediate callback may
  // synchronously invoke another RTI service that delivers a nested callback.
  mutable std::recursive_mutex invocationMutex_;
};

}  // namespace rti1516_2025::umbra_binding_detail
