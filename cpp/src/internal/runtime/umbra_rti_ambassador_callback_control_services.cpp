#include "internal/runtime/ambassador_shared_utilities.hpp"
#include "internal/runtime/ambassador_string_helpers.hpp"
#include "internal/runtime/umbra_rti_ambassador.hpp"

#include <chrono>
#include <cmath>
#include <cstdint>
#include <mutex>
#include <optional>
#include <string>
#include <utility>

namespace rti1516_2025::umbra_binding_detail {
namespace {

std::chrono::milliseconds callbackWaitDuration(double seconds) {
  if (!std::isfinite(seconds) || seconds <= 0.0) {
    return std::chrono::milliseconds::zero();
  }

  double const maximumMilliseconds =
      static_cast<double>(std::chrono::milliseconds::max().count());
  if (seconds >= maximumMilliseconds / 1000.0) {
    return std::chrono::milliseconds::max();
  }
  return std::chrono::duration_cast<std::chrono::milliseconds>(
      std::chrono::duration<double>(seconds));
}

}  // namespace

bool UmbraRtiAmbassador::evokeCallback(double approximateMinimumTimeInSeconds) {
  auto instrumentationScope = beginRtiCall("evokeCallback");
  if (callbacks_->isExecutingCallback()) {
    throw CallNotAllowedFromWithinCallback(
        L"Evoke Callback cannot be called from within a federate callback.");
  }
#if defined(UMBRA_ENABLE_EMBEDDED_FEDERATION_MANAGEMENT)
  auto pumpPeriodicMomUpdates = [this] {
    std::optional<std::wstring> federationName;
    {
      std::scoped_lock lock(mutex_, ambassadorFederationManagementMutex());
      if (lifecycle_.state() == umbra::detail::FederateLifecycleState::joined &&
          joinedFederationName_ && joinedFederateId_) {
        federationName = *joinedFederationName_;
      }
    }
    if (federationName) {
      pumpAmbassadorDueJoinedFederateMomPeriodicUpdates(*federationName);
    }
  };
  pumpPeriodicMomUpdates();
  // Do not issue another process poll while the shared dispatcher already
  // owns a callback from the previous poll. The next Evoke call should drain
  // that callback first; this keeps push-mode fixture lifecycles deterministic.
  if (callbacks_->pendingCount() == 0U) {
    pumpProcessReceiveOrder();
  }
#endif
  auto result = callbacks_->evokeOne(callbackWaitDuration(approximateMinimumTimeInSeconds));
  if (!result) {
    // A deadline may have elapsed while the dispatcher was waiting. Claim
    // and submit it now, then give this Evoke call one normal callback slot.
#if defined(UMBRA_ENABLE_EMBEDDED_FEDERATION_MANAGEMENT)
    pumpPeriodicMomUpdates();
#endif
    if (callbacks_->pendingCount() != 0U) {
      result = callbacks_->evokeOne(std::chrono::milliseconds::zero());
    }
  }
  return result;
}

bool UmbraRtiAmbassador::evokeMultipleCallbacks(
    double approximateMinimumTimeInSeconds,
    double approximateMaximumTimeInSeconds) {
  auto instrumentationScope = beginRtiCall("evokeMultipleCallbacks");
  if (callbacks_->isExecutingCallback()) {
    throw CallNotAllowedFromWithinCallback(
        L"Evoke Multiple Callbacks cannot be called from within a federate callback.");
  }
#if defined(UMBRA_ENABLE_EMBEDDED_FEDERATION_MANAGEMENT)
  auto pumpPeriodicMomUpdates = [this] {
    std::optional<std::wstring> federationName;
    {
      std::scoped_lock lock(mutex_, ambassadorFederationManagementMutex());
      if (lifecycle_.state() == umbra::detail::FederateLifecycleState::joined &&
          joinedFederationName_ && joinedFederateId_) {
        federationName = *joinedFederationName_;
      }
    }
    if (federationName) {
      pumpAmbassadorDueJoinedFederateMomPeriodicUpdates(*federationName);
    }
  };
  pumpPeriodicMomUpdates();
  // Evoke Multiple must admit all currently available process-boundary
  // receive-order events before handing control to the shared dispatcher.
  // The single-callback service keeps its one-poll behavior; only this
  // multiple-callback path drains the deterministic polling queue.
  if (callbacks_->pendingCount() == 0U) {
    pumpProcessReceiveOrder(true);
  }
#endif
  auto const result = callbacks_->evokeMultiple(
      callbackWaitDuration(approximateMinimumTimeInSeconds),
      callbackWaitDuration(approximateMaximumTimeInSeconds));
#if defined(UMBRA_ENABLE_EMBEDDED_FEDERATION_MANAGEMENT)
  pumpPeriodicMomUpdates();
#endif
  return result || callbacks_->pendingCount() != 0U;
}

void UmbraRtiAmbassador::enableCallbacks() {
  auto instrumentationScope = beginRtiCall("enableCallbacks");
  callbacks_->setEnabled(true);
}

void UmbraRtiAmbassador::disableCallbacks() {
  auto instrumentationScope = beginRtiCall("disableCallbacks");
  callbacks_->setEnabled(false);
}

#if defined(UMBRA_ENABLE_EMBEDDED_FEDERATION_MANAGEMENT)
void UmbraRtiAmbassador::pumpProcessReceiveOrder(bool drainAll) {
  if (!processEndpointActive_) {
    return;
  }

  std::wstring federationName;
  std::uint64_t receivingFederateId = 0U;
  umbra::detail::ProcessFederationClient* processClient = nullptr;
  {
    std::scoped_lock lock(mutex_);
    if (lifecycle_.state() != umbra::detail::FederateLifecycleState::joined ||
        !joinedFederationName_ || !joinedFederateId_) {
      return;
    }
    processClient = processFederationClient_.get();
    if (processClient == nullptr) {
      throw RTIinternalError(
          L"The configured process endpoint has no active federation client.");
    }
    federationName = *joinedFederationName_;
    receivingFederateId = *joinedFederateId_;
  }

  try {
    // Drain events captured during earlier service requests before polling.
    // This keeps evoked scope callbacks from triggering unrelated network IO.
    while (true) {
      bool delivered = false;
      auto dispatchPending = [&] {
        while (processClient->pendingPushedEventCount() != 0U) {
          processClient->dispatchPushedReceiveOrder();
          delivered = true;
        }
        while (processClient->pendingPushedObjectInstanceDiscoveryCount() != 0U) {
          processClient->dispatchPushedObjectInstanceDiscovery();
          delivered = true;
        }
        while (processClient->pendingPushedObjectInstanceRemovalCount() != 0U) {
          processClient->dispatchPushedObjectInstanceRemoval();
          delivered = true;
        }
        while (processClient->pendingPushedObjectInstanceScopeChangeCount() != 0U) {
          processClient->dispatchPushedObjectInstanceScopeChange();
          delivered = true;
        }
        while (processClient->pendingPushedAttributeRelevanceAdvisoryCount() != 0U) {
          processClient->dispatchPushedAttributeRelevanceAdvisory();
          delivered = true;
        }
        while (processClient->pendingPushedAttributeTransportationTypeChangeCount() != 0U) {
          processClient->dispatchPushedAttributeTransportationTypeChange();
          delivered = true;
        }
        while (processClient->pendingPushedAttributeTransportationTypeQueryCount() != 0U) {
          processClient->dispatchPushedAttributeTransportationTypeQuery();
          delivered = true;
        }
        while (processClient->pendingPushedInteractionTransportationTypeChangeCount() != 0U) {
          processClient->dispatchPushedInteractionTransportationTypeChange();
          delivered = true;
        }
        while (processClient->pendingPushedInteractionTransportationTypeQueryCount() != 0U) {
          processClient->dispatchPushedInteractionTransportationTypeQuery();
          delivered = true;
        }
        while (processClient->pendingPushedAttributeUpdateCount() != 0U) {
          processClient->dispatchPushedAttributeUpdate();
          delivered = true;
        }
        while (processClient->pendingPushedAttributeValueUpdateRequestCount() != 0U) {
          processClient->dispatchPushedAttributeValueUpdateRequest();
          delivered = true;
        }
        while (processClient->pendingPushedAttributeOwnershipQueryCount() != 0U) {
          processClient->dispatchPushedAttributeOwnershipQuery();
          delivered = true;
        }
        while (processClient->pendingPushedAttributeOwnershipAcquisitionIfAvailableCount() != 0U) {
          processClient->dispatchPushedAttributeOwnershipAcquisitionIfAvailable();
          delivered = true;
        }
        while (processClient->pendingPushedAttributeOwnershipAcquisitionCount() != 0U) {
          processClient->dispatchPushedAttributeOwnershipAcquisition();
          delivered = true;
        }
        while (processClient->pendingPushedAttributeOwnershipUnavailableCount() != 0U) {
          processClient->dispatchPushedAttributeOwnershipUnavailable();
          delivered = true;
        }
        while (processClient->pendingPushedSynchronizationPointAnnouncementCount() != 0U) {
          processClient->dispatchPushedSynchronizationPointAnnouncement();
          delivered = true;
        }
        while (processClient->pendingPushedFederationSynchronizedCount() != 0U) {
          processClient->dispatchPushedFederationSynchronized();
          delivered = true;
        }
        while (processClient->pendingPushedFederationSaveCount() != 0U) {
          processClient->dispatchPushedFederationSave();
          delivered = true;
        }
        while (processClient->pendingPushedFederationRestoreCount() != 0U) {
          processClient->dispatchPushedFederationRestore();
          delivered = true;
        }
        while (processClient->pendingPushedTimeAdvanceGrantCount() != 0U) {
          processClient->dispatchPushedTimeAdvanceGrant();
          delivered = true;
        }
      };
      dispatchPending();
      if (!delivered) {
        auto event = processClient->receiveInteraction(
            federationName, receivingFederateId);
        if (event) {
          processClient->dispatchReceiveOrder(std::move(*event));
          delivered = true;
        }
        dispatchPending();
      }
      // Evoke Multiple admits the complete currently available queue. Single
      // callback entry points avoid this drain when work is already pending.
      if (!drainAll || !delivered) {
        break;
      }
    }
  } catch (umbra::detail::ProcessFederationServiceProtocolError const& error) {
    throw RTIinternalError(wideAscii(error.what()));
  } catch (umbra::detail::ProcessFederationCallbackBridgeError const& error) {
    throw RTIinternalError(wideAscii(error.what()));
  } catch (umbra::detail::ProcessFederationClientError const& error) {
    throw RTIinternalError(wideAscii(error.what()));
  }
}
#endif

}  // namespace rti1516_2025::umbra_binding_detail
