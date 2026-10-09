#include "internal/handles/federate_handle.hpp"
#include "internal/handles/interaction_class_handle.hpp"
#include "internal/fom/hla_names.hpp"
#include "internal/runtime/ambassador_shared_utilities.hpp"
#include "internal/runtime/umbra_rti_ambassador.hpp"

#include <cstdint>
#include <limits>
#include <mutex>
#include <optional>
#include <utility>
#include <vector>

#include <RTI/encoding/BasicDataElements.h>

namespace rti1516_2025::umbra_binding_detail {

#if defined(UMBRA_ENABLE_EMBEDDED_FEDERATION_MANAGEMENT)

// HLAreportFederateLost is an RTI-originated receive-order MOM interaction.
// It intentionally stays separate from ordinary joined-federate interaction
// delivery because its producer designator is the official default-invalid
// FederateHandle, never a numeric zero or the lost federate's handle.
void queueFederateLostReport(
    std::wstring federationName,
    umbra::detail::FederateLostReportPlan report,
    std::wstring faultDescription) {
  if (report.status != umbra::detail::FederateLostReportStatus::applied ||
      report.reportedFederateId == 0U || !report.lastKnownTime ||
      report.routing.interactionClassHandle == 0U ||
      report.routing.federateParameterHandle == 0U ||
      report.routing.federateNameParameterHandle == 0U ||
      report.routing.timestampParameterHandle == 0U ||
      report.routing.faultDescriptionParameterHandle == 0U) {
    return;
  }

  std::vector<AmbassadorInteractionParameterValue> sentParameters;
  sentParameters.reserve(4U);
  sentParameters.emplace_back(
      report.routing.federateParameterHandle,
      makeFederateHandle(report.reportedFederateId).encode());
  sentParameters.emplace_back(
      report.routing.federateNameParameterHandle,
      HLAunicodeString{report.reportedFederateName}.encode());
  sentParameters.emplace_back(
      report.routing.timestampParameterHandle,
      report.lastKnownTime->encode());
  sentParameters.emplace_back(
      report.routing.faultDescriptionParameterHandle,
      HLAunicodeString{faultDescription}.encode());
  auto const reliableTransportation = ambassadorTransportationHandleFromEmbeddedName(
      umbra::detail::hla::utf8::mom::reliable,
      L"The embedded federation could not reconstruct HLAreportFederateLost transportation.");

  for (auto & plannedRecipient : report.recipients) {
    if (!plannedRecipient.callbackRoute || plannedRecipient.federateId == 0U) {
      continue;
    }
    submitAmbassadorReceiveOrderCallback(
        std::move(plannedRecipient.callbackRoute),
        federationName,
        plannedRecipient.federateId,
        [
        federationName,
        reportedFederateId = report.reportedFederateId,
        receivingFederateId = plannedRecipient.federateId,
        sentParameters,
        reliableTransportation](FederateAmbassador & recipient) mutable {
      std::optional<umbra::detail::ReceiveOrderInteractionRecipient> projection;
      {
        std::scoped_lock lock(ambassadorFederationManagementMutex());
        projection = embeddedFederationRegistry().federateLostReportRecipientFor(
            federationName,
            reportedFederateId,
            receivingFederateId);
      }
      if (!projection) {
        return;
      }

      ParameterHandleValueMap parameterValues = ambassadorProjectInteractionParameterValues(
          sentParameters,
          projection->receivedParameterHandles);
      recipient.receiveInteraction(
          makeInteractionClassHandle(projection->receivedInteractionClassHandle),
          parameterValues,
          VariableLengthData{},
          reliableTransportation,
          FederateHandle{},
          nullptr);
    });
  }
}

void UmbraRtiAmbassador::handleEmbeddedTransportFailure(
    std::wstring faultDescription) {
  auto instrumentationScope = beginRtiCall("handleEmbeddedTransportFailure");
  static_cast<void>(handleEmbeddedMembershipLoss(
      EmbeddedMembershipLossKind::connection_lost,
      std::move(faultDescription)));
}

void UmbraRtiAmbassador::handleProcessTransportFailure(
    std::wstring faultDescription) {
  auto instrumentationScope = beginRtiCall("handleProcessTransportFailure");
  if (faultDescription.empty()) {
    faultDescription = L"The process transport connection was lost.";
  }

  std::shared_ptr<CallbackSession> callbackSession;
  {
    std::scoped_lock lock(mutex_);
    if (!processEndpointActive_ ||
        lifecycle_.state() != umbra::detail::FederateLifecycleState::joined) {
      return;
    }
    if (lifecycle_.apply(umbra::detail::FederateLifecycleEvent::connection_lost) !=
        umbra::detail::FederateLifecycleResult::applied) {
      return;
    }

    // The process service owns the remote registry membership.  The local
    // ambassador must nevertheless become Not Connected immediately so later
    // public calls cannot continue to use a dead session.  Leave the client
    // object for its in-flight request to unwind; its transport has already
    // cleared its failure handler and will be reclaimed on reconnect/destruction.
    processEndpointActive_ = false;
    if (federateTimeState_) {
      federateTimeState_->deactivate();
      federateTimeState_.reset();
    }
    joinedServiceReport_.reset();
    joinedFederationName_.reset();
    joinedFederateId_.reset();
    processLogicalTimeImplementationName_.reset();
    processServiceReportFile_.reset();
    callbackSession = callbackSession_;
  }

  if (callbackSession) {
    callbacks_->submit([
        callbackSession = std::move(callbackSession),
        faultDescription = std::move(faultDescription)]() mutable {
      callbackSession->invoke([
          faultDescription = std::move(faultDescription)](
          FederateAmbassador& recipient) mutable {
        try {
          recipient.connectionLost(faultDescription);
        } catch (...) {
          // Connection Lost is an RTI notification.  A faulty recipient must
          // not resurrect the dead process endpoint or escape the dispatcher.
        }
      });
    });
  }
}

bool UmbraRtiAmbassador::handleEmbeddedFederateResignation(
    std::wstring reasonForResign) {
  auto instrumentationScope = beginRtiCall("handleEmbeddedFederateResignation");
  return handleEmbeddedMembershipLoss(
      EmbeddedMembershipLossKind::rti_resigned,
      std::move(reasonForResign));
}

bool UmbraRtiAmbassador::handleEmbeddedMembershipLoss(
    EmbeddedMembershipLossKind kind,
    std::wstring reason) {
  auto instrumentationScope = beginRtiCall("handleEmbeddedMembershipLoss");
  bool const connectionLost = kind == EmbeddedMembershipLossKind::connection_lost;
  std::vector<umbra::detail::FederationTimeGrantDispatch> newlyEligible;
  std::vector<umbra::detail::FederationSynchronizedNotification>
      synchronizationNotifications;
  std::vector<umbra::detail::FederationSaveNotification> saveNotifications;
  std::vector<umbra::detail::FederationRestoreNotification> restoreNotifications;
  std::vector<umbra::detail::ObjectInstanceRemovalRecipient> objectRemovals;
  std::vector<umbra::detail::AttributeOwnershipAssumptionRecipient>
      ownershipAssumptions;
  std::vector<umbra::detail::AttributeOwnershipAcquisitionWorkItem>
      ownershipAcquisitionWorkItems;
  std::vector<umbra::detail::DeclarationAdvisory> declarationAdvisories;
  std::optional<umbra::detail::FederateLostReportPlan> federateLostReport;
  std::shared_ptr<CallbackSession> callbackSession;
  std::wstring federationName;
  std::uint64_t reportedFederateId = 0;
  umbra::detail::FederationRegistryResult resigned;
  bool finalServiceReportAppendFailed = false;

  {
    std::scoped_lock lock(mutex_, ambassadorFederationManagementMutex());
    if (lifecycle_.state() != umbra::detail::FederateLifecycleState::joined ||
        !joinedFederationName_ || !joinedFederateId_) {
      // A graceful or previous forced transition already removed membership.
      // The endpoint remains usable after an RTI-initiated resignation, but
      // there is no second lifecycle event to report.
      return false;
    }

    federationName = *joinedFederationName_;
    reportedFederateId = *joinedFederateId_;
    if (connectionLost) {
      // The registry owns the last-granted time and active subscription state,
      // both of which disappear as part of the forced resignation. Capture
      // the source-mandated loss report first, then let the normal automatic
      // resign machinery mutate membership and application state.
      auto report = embeddedFederationRegistry().planFederateLostReport(
          federationName,
          *joinedFederateId_);
      if (report.status == umbra::detail::FederateLostReportStatus::applied) {
        federateLostReport = std::move(report);
      }
      // A corrupted private catalog/time state must not keep a known transport
      // fault joined indefinitely. Normal prevalidated executions always have
      // a plan; this defensive escape preserves authoritative loss cleanup
      // without manufacturing a malformed MOM interaction.
    }
    auto& registry = embeddedFederationRegistry();
    resigned = connectionLost
        ? registry.connectionLostWithFinalServiceReportReservation(
              federationName,
              *joinedFederateId_,
              static_cast<std::uint16_t>(
                  umbra::detail::MomServiceType::federation_management))
        : registry.resignWithFinalServiceReportReservation(
              federationName,
              *joinedFederateId_,
              rti1516_2025::NO_ACTION,
              static_cast<std::uint16_t>(
                  umbra::detail::MomServiceType::federation_management),
              true);
    if (!connectionLost &&
        resigned.status != umbra::detail::FederationRegistryStatus::applied) {
      // This deliberately bounded RTI-control seam uses the standard
      // NO_ACTION resign path. It only accepts a member whose ordinary
      // no-disposition preconditions already hold; a future administration or
      // watchdog source must supply its own fully specified disposition policy.
      return false;
    }
    if (resigned.status == umbra::detail::FederationRegistryStatus::applied) {
      synchronizationNotifications = std::move(resigned.synchronizationNotifications);
      saveNotifications = std::move(resigned.saveNotifications);
      restoreNotifications = std::move(resigned.restoreNotifications);
      objectRemovals.reserve(resigned.resignObjectRemovals.size());
      for (auto& removal : resigned.resignObjectRemovals) {
        objectRemovals.push_back({
            removal.receivingFederateId,
            removal.objectInstanceHandle,
            std::move(removal.callbackRoute),
            std::move(removal.serviceReportRoute),
            removal.rtiOwnedMomObject,
        });
      }
      ownershipAssumptions.reserve(resigned.resignOwnershipAssumptions.size());
      for (auto& assumption : resigned.resignOwnershipAssumptions) {
        ownershipAssumptions.push_back({
            assumption.receivingFederateId,
            assumption.objectInstanceHandle,
            std::move(assumption.attributeHandles),
            std::move(assumption.callbackRoute),
        });
      }
      ownershipAcquisitionWorkItems =
          std::move(resigned.resignOwnershipAcquisitionWorkItems);

      // Membership removal can change the relevance of declarations owned by
      // surviving federates. Compute those transitions from the post-removal
      // state before the departing member's route disappears from the adapter.
      declarationAdvisories = embeddedFederationRegistry()
          .planDeclarationAdvisories(federationName);

      auto scheduled = embeddedFederationRegistry()
          .reevaluateTimeAdvanceGrants(federationName);
      if (scheduled.status == umbra::detail::FederationTimeGrantStatus::applied) {
        newlyEligible = std::move(scheduled.dispatches);
      }
      auto timedSave = embeddedFederationRegistry()
          .reevaluateTimedFederationSave(federationName);
      if (timedSave.status == umbra::detail::FederationSaveControlStatus::applied) {
        for (auto& notification : timedSave.notifications) {
          saveNotifications.push_back(std::move(notification));
        }
      }
    }

    // A Connection Lost event is authoritative even if a best-effort cleanup
    // callback could not be prepared: the ambassador becomes Not Connected.
    // The distinct Federate Resigned event stays connected but leaves the
    // Joined state, after the registry has accepted its bounded NO_ACTION
    // disposition above.
    auto const lifecycleEvent = connectionLost
        ? umbra::detail::FederateLifecycleEvent::connection_lost
        : umbra::detail::FederateLifecycleEvent::rti_resign;
    if (lifecycle_.apply(lifecycleEvent) !=
        umbra::detail::FederateLifecycleResult::applied) {
      return false;
    }
    if (resigned.finalServiceReportFileSerialNumber.has_value()) {
      // Both §4.4 Connection Lost and §4.13 Federate Resigned are RTI-invoked
      // HLA services. Each has one source-defined text argument. Section
      // 11.5.1 leaves the descriptive argument name implementation-dependent;
      // keep its name anchored to the corresponding service narrative. As
      // with federate-initiated resignation, reserve and append before the
      // joined member and its writer disappear.
      try {
        appendReservedSuccessfulVoidServiceReportToFile(
            *resigned.finalServiceReportFileSerialNumber,
            connectionLost ? L"ConnectionLost" : L"FederateResigned",
            {{umbra::detail::MomArgumentType::string,
              connectionLost ? L"Fault description" : L"Reason for resigning",
              umbra::detail::formatMomString(reason)}});
      } catch (RTIinternalError const&) {
        // The RTI-side service action has already completed its authoritative
        // membership transition. Finish the lifetime cleanup and callback
        // routing before returning the deterministic storage error; do not
        // leave either lifecycle carrying stale joined state.
        finalServiceReportAppendFailed = true;
      }
    }
    if (federateTimeState_) {
      federateTimeState_->deactivate();
    }
    federateTimeState_.reset();
    // The file itself is retained for its completed joined-federate lifetime;
    // releasing the writer here ensures a later Join receives a new identity.
    joinedServiceReport_.reset();
    joinedFederationName_.reset();
    joinedFederateId_.reset();
    processLogicalTimeImplementationName_.reset();
    if (connectionLost) {
      transportConnection_.reset();
      callbackSession = std::move(callbackSession_);
      // Faulted transport input invalidates queued work from the old endpoint;
      // the Connection Lost callback itself is submitted below on that session.
    } else {
      callbackSession = callbackSession_;
      // Previous joined-state callbacks must not leak across the RTI-initiated
      // resignation. The connected session survives to deliver the official
      // Federate Resigned callback and later work after a fresh Join.
    }
    callbacks_->reset();
  }

  // The joined federate lifetime is over. Drop only its best-effort
  // update-rate admission history; another live federate may still be
  // receiving the same object/attribute stream.
  eraseAmbassadorUpdateRateHistoryForFederate(federationName, reportedFederateId);

  // Queue the RTI-originated loss interaction before every automatic-resign
  // consequence. This preserves report-before-cleanup callback ordering for
  // evoked recipients without invoking user code while federation locks are
  // held. Callback-time subscription/lifecycle rechecks remain in the report
  // route because surviving federates may change state before they evoke it.
  if (federateLostReport) {
    queueFederateLostReport(
        federationName,
        std::move(*federateLostReport),
        reason);
  }
  if (!connectionLost && resigned.finalServiceReportInteractionReservation) {
    auto reservation = std::move(*resigned.finalServiceReportInteractionReservation);
    if (reservation.serialNumber >
            static_cast<std::uint32_t>(std::numeric_limits<std::int32_t>::max()) ||
        reservation.routing.reportParameterHandles.size() != 8U) {
      throw RTIinternalError(
          L"Umbra could not reserve the final FederateResigned service-report interaction.");
    }
    std::vector<umbra::detail::MomServiceArgument> suppliedArguments{
        {umbra::detail::MomArgumentType::string,
         L"Reason for resigning",
         umbra::detail::formatMomString(reason)}};
    umbra::detail::MomServiceArgument returnedArgument{
        umbra::detail::MomArgumentType::null_value,
        L"",
        umbra::detail::formatMomNull()};
    auto const encoded = umbra::detail::encodeMomServiceInvocation(
        L"FederateResigned",
        umbra::detail::MomServiceType::federation_management,
        true,
        suppliedArguments,
        returnedArgument,
        L"",
        static_cast<std::int32_t>(reservation.serialNumber));
    std::vector<AmbassadorInteractionParameterValue> reportParameters;
    reportParameters.reserve(reservation.routing.reportParameterHandles.size());
    reportParameters.emplace_back(
        reservation.routing.reportParameterHandles[0], encoded.service);
    reportParameters.emplace_back(
        reservation.routing.reportParameterHandles[1], encoded.serviceType);
    reportParameters.emplace_back(
        reservation.routing.reportParameterHandles[2], encoded.successIndicator);
    reportParameters.emplace_back(
        reservation.routing.reportParameterHandles[3], encoded.suppliedArguments);
    reportParameters.emplace_back(
        reservation.routing.reportParameterHandles[4], encoded.returnedArgument);
    reportParameters.emplace_back(
        reservation.routing.reportParameterHandles[5], encoded.exception);
    reportParameters.emplace_back(
        reservation.routing.reportParameterHandles[6], encoded.serialNumber);
    reportParameters.emplace_back(
        reservation.routing.reportParameterHandles[7],
        makeFederateHandle(reportedFederateId).encode());
    queueAmbassadorMomServiceReportInteraction(
        federationName,
        reportedFederateId,
        static_cast<std::uint16_t>(umbra::detail::MomServiceType::federation_management),
        std::move(reservation),
        std::move(reportParameters),
        ambassadorTransportationHandleFromEmbeddedName(
            umbra::detail::hla::utf8::mom::reliable,
            L"The embedded federation could not reconstruct HLAreportServiceInvocation transportation."),
        true);
  }
  submitAmbassadorTimeAdvanceGrantDispatches(std::move(newlyEligible));
  submitAmbassadorFederationSaveNotifications(
      federationName,
      std::move(saveNotifications));
  submitAmbassadorFederationRestoreNotifications(
      federationName,
      std::move(restoreNotifications));
  submitAmbassadorFederationSynchronizedNotifications(std::move(synchronizationNotifications));
  queueAmbassadorDeclarationAdvisories(std::move(declarationAdvisories));
  VariableLengthData emptyMembershipLossTag;
  queueAmbassadorAttributeOwnershipAcquisitionWorkItems(
      std::move(ownershipAcquisitionWorkItems),
      federationName);
  queueAmbassadorAttributeOwnershipAssumptionRecipients(
      std::move(ownershipAssumptions),
      federationName,
      emptyMembershipLossTag);
  queueAmbassadorObjectInstanceRemovals(
      std::move(objectRemovals),
      federationName,
      emptyMembershipLossTag);
  queueAmbassadorFederationMomConditionalAttributeUpdate(
      federationName,
      {umbra::detail::hla::utf8::mom::federates_in_federation});

  if (callbackSession) {
    callbacks_->submit([
        callbackSession = std::move(callbackSession),
        connectionLost,
        reason = std::move(reason)]() mutable {
      callbackSession->invoke([
          connectionLost,
          reason = std::move(reason)](
          FederateAmbassador& recipient) mutable {
        // Both callback paths occur only after the irreversible membership
        // transition. Do not allow a faulty recipient to revive or escape the
        // private control/transport path.
        try {
          if (connectionLost) {
            recipient.connectionLost(reason);
          } else {
            recipient.federateResigned(reason);
          }
        } catch (...) {
        }
      });
      });
  }
  if (finalServiceReportAppendFailed) {
    throw RTIinternalError(
        L"Umbra could not append the selected service-report file record.");
  }
  return true;
}

#endif

}  // namespace rti1516_2025::umbra_binding_detail
