#pragma once

#include "internal/callbacks/callback_dispatcher.hpp"
#include "internal/callbacks/callback_session.hpp"
#include "internal/federation/process_federation_service.hpp"

#include <atomic>
#include <chrono>
#include <cstddef>
#include <memory>
#include <mutex>
#include <stdexcept>
#include <unordered_map>

namespace rti1516_2025 {
class FederateAmbassador;
}

namespace umbra::detail {

class ProcessFederationCallbackBridgeError final : public std::runtime_error {
 public:
  using std::runtime_error::runtime_error;
};

// Private process-client adapter from a decoded service event to the official
// IEEE 1516.1-2025 FederateAmbassador callback.  It deliberately reuses the
// same CallbackDispatcher/CallbackSession lifetime and dispatch-model seam as
// the embedded public binding; it is not a public API or a conformance claim.
class ProcessFederationCallbackBridge final {
 public:
  explicit ProcessFederationCallbackBridge(
      rti1516_2025::FederateAmbassador& recipient,
      CallbackDispatchModel model = CallbackDispatchModel::evoked);

  // Runtime integration constructor.  The public ambassador owns the
  // dispatcher/session that enforce its callback model and borrowed
  // FederateAmbassador lifetime; the process adapter may borrow those same
  // objects instead of silently creating a second callback control plane.
  ProcessFederationCallbackBridge(
      std::shared_ptr<CallbackDispatcher> dispatcher,
      std::shared_ptr<rti1516_2025::umbra_binding_detail::CallbackSession>
          callbackSession);

  ProcessFederationCallbackBridge(ProcessFederationCallbackBridge const&) = delete;
  ProcessFederationCallbackBridge& operator=(
      ProcessFederationCallbackBridge const&) = delete;

  ~ProcessFederationCallbackBridge();

  // Queue one receive-order interaction for the selected callback model.  In
  // the immediate model this invokes the callback before returning; in the
  // evoked model the caller must use evokeOne/evokeMultiple.
  void submitReceiveOrder(ProcessFederationInteractionEvent event);

  // Apply a process-side Request Retraction consequence to a timestamped
  // interaction that already crossed the callback boundary.  If the original
  // interaction is still queued, the request marks it suppressed instead of
  // manufacturing a callback; this mirrors the embedded recipient ledger.
  void submitRequestRetraction(std::uint64_t messageId);

  // Queue one ordinary process-boundary attribute reflection.  This uses the
  // same callback dispatcher/session as interaction events so callback model,
  // enable/disable gating, and ambassador lifetime remain identical.
  void submitAttributeUpdate(ProcessFederationAttributeUpdateEvent event);

  // Queue one object-instance discovery through the official 6.9 callback
  // surface.  Discovery shares the process callback dispatcher/session but
  // remains a distinct event type so the receiving ambassador cannot mistake
  // it for an interaction or attribute reflection.
  void submitObjectInstanceDiscovery(
      ProcessFederationObjectInstanceDiscoveryEvent event);

  // Queue one receive-order Remove Object Instance callback through the
  // official FederateAmbassador surface.
  void submitObjectInstanceRemoval(
      ProcessFederationObjectInstanceRemovalEvent event);

  // Queue one Attribute In/Out Of Scope transition through the official
  // FederateAmbassador callback surface.
  void submitObjectInstanceScopeChange(
      ProcessFederationObjectInstanceScopeChangeEvent event);

  // Queue one owner-directed Attribute Relevance Advisory through the
  // official Turn Updates On/Off callback surface.
  void submitAttributeRelevanceAdvisory(
      ProcessFederationAttributeRelevanceAdvisoryEvent event);

  // Install the client-owned switch state used to suppress an advisory that
  // was already queued for HLA_EVOKED before the RTIambassador disabled the
  // switch.  The state is shared with queued callback tasks so the check is
  // made at callback entry, not only when the process frame is admitted.
  void setAttributeRelevanceAdvisorySwitchState(
      std::shared_ptr<std::atomic_bool> enabled) noexcept;

  bool evokeOne(std::chrono::milliseconds minimumWait);
  bool evokeMultiple(
      std::chrono::milliseconds minimumWait,
      std::chrono::milliseconds maximumWait);

  [[nodiscard]] std::size_t pendingCount() const;

  // Close the borrowed ambassador endpoint and discard callbacks that have not
  // crossed the callback boundary.  It is safe to call more than once.
  void close() noexcept;

 private:
  struct RetractionState final {
    std::mutex mutex;
    bool callbackStarted = false;
    bool retractionRequested = false;
    bool retractionCallbackQueued = false;
  };

  std::shared_ptr<CallbackDispatcher> dispatcher_;
  std::shared_ptr<rti1516_2025::umbra_binding_detail::CallbackSession>
      callbackSession_;
  std::shared_ptr<std::atomic_bool> attributeRelevanceAdvisorySwitchState_;
  mutable std::mutex retractionMutex_;
  std::unordered_map<
      std::uint64_t,
      std::shared_ptr<RetractionState>> retractionStates_;
};

}  // namespace umbra::detail
