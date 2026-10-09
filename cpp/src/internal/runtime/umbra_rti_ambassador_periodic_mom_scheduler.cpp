#include "internal/runtime/umbra_rti_ambassador.hpp"
#include "internal/runtime/ambassador_shared_utilities.hpp"

#if defined(UMBRA_ENABLE_EMBEDDED_FEDERATION_MANAGEMENT)

#include <chrono>
#include <mutex>
#include <optional>
#include <thread>

namespace rti1516_2025::umbra_binding_detail {
void UmbraRtiAmbassador::startPeriodicMomScheduler() {
  if (callbackModel_ != HLA_IMMEDIATE || periodicMomScheduler_.joinable()) {
    return;
  }
  periodicMomScheduler_ = std::jthread(
      [this](std::stop_token stopToken) {
        periodicMomSchedulerLoop(stopToken);
      });
}

void UmbraRtiAmbassador::stopPeriodicMomScheduler() noexcept {
  if (!periodicMomScheduler_.joinable()) {
    return;
  }
  periodicMomScheduler_.request_stop();
  periodicMomScheduler_.join();
}

void UmbraRtiAmbassador::periodicMomSchedulerLoop(std::stop_token stopToken) {
  using namespace std::chrono_literals;
  while (!stopToken.stop_requested()) {
    std::optional<std::wstring> federationName;
    bool pumpProcessEvents = false;  // Set while both federation locks are held.
    {
      std::scoped_lock lock(mutex_, ambassadorFederationManagementMutex());
      if (lifecycle_.state() == umbra::detail::FederateLifecycleState::joined &&
          callbackModel_ == HLA_IMMEDIATE && joinedFederationName_ &&
          joinedFederateId_) {
        federationName = *joinedFederationName_;
        pumpProcessEvents =
            processEndpointActive_ && processFederationClient_ != nullptr;
      }
    }
    if (federationName) {
      // The registry owns deadline claiming. This thread only supplies the
      // immediate callback model's missing callback-boundary pump; routing
      // and callback-time revalidation remain in the ordinary planner.
      pumpAmbassadorDueJoinedFederateMomPeriodicUpdates(*federationName);
      auto const lastRtiCallStartNanoseconds =
          lastRtiCallStartNanoseconds_.load(std::memory_order_relaxed);
      auto const nowNanoseconds =
          std::chrono::duration_cast<std::chrono::nanoseconds>(
              std::chrono::steady_clock::now().time_since_epoch())
              .count();
      auto const processPumpIdleThresholdNanoseconds =
          std::chrono::duration_cast<std::chrono::nanoseconds>(100ms).count();
      if (pumpProcessEvents && lastRtiCallStartNanoseconds != 0 &&
          nowNanoseconds - lastRtiCallStartNanoseconds >=
              processPumpIdleThresholdNanoseconds) {
        try {
          pumpProcessReceiveOrder();
        } catch (...) {
          // The transport failure handler records connection loss and queues
          // the official callback. Never let a background pump exception
          // escape the scheduler thread.
          handleProcessTransportFailure(
              L"The asynchronous process callback pump failed.");
        }
        auto const pollCompletionNanoseconds =
            std::chrono::duration_cast<std::chrono::nanoseconds>(
                std::chrono::steady_clock::now().time_since_epoch())
                .count();
        lastRtiCallStartNanoseconds_.store(
            pollCompletionNanoseconds, std::memory_order_relaxed);
      }
    }
    std::this_thread::sleep_for(25ms);
  }
}


}  // namespace rti1516_2025::umbra_binding_detail

#endif