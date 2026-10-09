#include "internal/runtime/ambassador_shared_utilities.hpp"
#include "internal/runtime/ambassador_federation_lifecycle_support.hpp"
#include "internal/callbacks/callback_session.hpp"
#include "internal/callbacks/callback_dispatcher.hpp"
#include "internal/federation/federation_registry.hpp"
#include "internal/fom/hla_names.hpp"
#include "internal/handles/attribute_handle.hpp"
#include "internal/handles/federate_handle.hpp"
#include "internal/handles/interaction_class_handle.hpp"
#include "internal/handles/message_retraction_handle.hpp"
#include "internal/handles/object_instance_handle.hpp"
#include "internal/handles/parameter_handle.hpp"
#include "internal/handles/region_handle.hpp"
#include "internal/handles/transportation_type_handle.hpp"
#include "internal/runtime/utf8_string.hpp"
#include "internal/time/federate_time_state.hpp"
#include "internal/time/federation_time_grant_policy.hpp"
#include "internal/time/reference_time_selection.hpp"

#include <RTI/FederateAmbassador.h>
#include <RTI/time/HLAlogicalTimeFactoryFactory.h>

#include <algorithm>
#include <cstdint>
#include <mutex>
#include <optional>
#include <set>
#include <string>
#include <utility>
#include <vector>

namespace rti1516_2025::umbra_binding_detail {
#if defined(UMBRA_ENABLE_EMBEDDED_FEDERATION_MANAGEMENT)
namespace {
void submitTimeAdvanceGrantDispatches(
    std::vector<umbra::detail::FederationTimeGrantDispatch> dispatches) {
  for (auto& dispatch : dispatches) {
    if (dispatch) {
      dispatch();
    }
  }
}

umbra::detail::FederationTimeGrantDispatch makeTimeAdvanceGrantDispatch(
    std::shared_ptr<umbra::detail::CallbackDispatcher> const& callbackDispatcher,
    std::shared_ptr<CallbackSession> const& callbackSession,
    std::shared_ptr<umbra::detail::FederateTimeState> const& timeState,
    std::wstring federationName,
    std::uint64_t federateId,
    std::uint64_t generation,
    std::uint64_t dispatchIdentity) {
  std::weak_ptr<umbra::detail::CallbackDispatcher> const dispatcher = callbackDispatcher;
  std::weak_ptr<CallbackSession> const session = callbackSession;
  std::weak_ptr<umbra::detail::FederateTimeState> const federateTimeState = timeState;

  return [
      dispatcher,
      session,
      federateTimeState,
      federationName = std::move(federationName),
      federateId,
      generation,
      dispatchIdentity] {
    auto callbackDispatcher = dispatcher.lock();
    if (!callbackDispatcher) {
      return;
    }

    callbackDispatcher->submit([
        session,
        federateTimeState,
        federationName,
        federateId,
        generation,
        dispatchIdentity] {
      auto callbackSession = session.lock();
      auto timeState = federateTimeState.lock();
      if (!callbackSession || !timeState) {
        return;
      }

      std::vector<umbra::detail::FederationTimeGrantDispatch> newlyEligible;
      std::vector<umbra::detail::FederationSaveNotification> immediateSaveNotifications;
      std::vector<umbra::detail::FederationSaveNotification> timedSaveNotifications;
      std::vector<umbra::detail::ObjectInstanceRemovalRecipient>
          postGrantObjectRemovals;
      bool grantCompleted = false;
      bool nextSaveConditionalsChanged = false;
      callbackSession->invoke([
          timeState = std::move(timeState),
          federationName,
          federateId,
          generation,
          dispatchIdentity,
          &newlyEligible,
          &immediateSaveNotifications,
          &timedSaveNotifications,
          &postGrantObjectRemovals,
          &grantCompleted,
          &nextSaveConditionalsChanged](FederateAmbassador& recipient) {
        auto const pendingSnapshot = timeState->snapshot();
        bool const flushQueue =
            pendingSnapshot.advanceMode ==
            umbra::detail::FederateTimeAdvanceMode::flush_queue_request;
        std::shared_ptr<LogicalTime const> grantedTime;
        std::shared_ptr<LogicalTime const> optimisticTime;
        std::shared_ptr<LogicalTime> flushQueueGrantedTime;
        std::shared_ptr<LogicalTime> flushQueueOptimisticTime;
        std::vector<umbra::detail::TsoPayloadDelivery> tsoDeliveries;
        std::optional<std::wstring> immediateSaveLabel;
        if (!flushQueue) {
          {
            std::scoped_lock lock(ambassadorFederationManagementMutex());
            auto& registry = embeddedFederationRegistry();
            if (registry.beginTimeAdvanceGrant(
                    federationName,
                    federateId,
                    generation,
                    dispatchIdentity) !=
                umbra::detail::FederationTimeGrantStatus::applied) {
              return;
            }
            auto admission = registry.admitImmediateFederationSaveAtTimeAdvanceBoundary(
                federationName,
                federateId);
            if (admission.status != umbra::detail::FederationSaveControlStatus::applied) {
              throw RTIinternalError(
                  L"The embedded federation could not admit an untimed save at the time-advance boundary.");
            }
            immediateSaveLabel = std::move(admission.currentFederateLabel);
            immediateSaveNotifications = std::move(admission.notifications);
          }

          // Initiate Federate Save must reach a time-constrained recipient
          // while its private temporal state is still Time Advancing. This is
          // deliberately a direct pre-grant callback rather than ordinary
          // queued federation-control work, whose FIFO position could follow
          // this recipient's Time Advance Grant.
          if (immediateSaveLabel.has_value()) {
            nextSaveConditionalsChanged = true;
            recipient.initiateFederateSave(*immediateSaveLabel);
            queueAmbassadorJoinedFederateMomConditionalAttributeUpdateForJoinedFederate(
                federationName,
                federateId,
                {umbra::detail::hla::utf8::mom::federate_state},
                federateId);
          }

          {
            std::scoped_lock lock(ambassadorFederationManagementMutex());
            auto& registry = embeddedFederationRegistry();
            if (!pendingSnapshot.requestedTime ||
                pendingSnapshot.requestedTime->implementationName() !=
                    pendingSnapshot.implementationName) {
              throw RTIinternalError(
                  L"The embedded Time Advance Grant has incomplete temporal state.");
            }
            // Keep the federate Time Advancing while its TSO payloads and a
            // possible timestamped Initiate Federate Save are delivered. The
            // actual private grant mutation occurs below immediately before
            // the matching Time Advance Grant callback.
            grantedTime = cloneAmbassadorReferenceLogicalTime(
                pendingSnapshot.implementationName,
                *pendingSnapshot.requestedTime);
            auto tso = registry.beginTsoPayloadDelivery(
                federationName,
                federateId,
                *grantedTime,
                true);
            if (tso.status != umbra::detail::FederationTsoRegistryStatus::applied ||
                (tso.deliveryStatus != umbra::detail::FederationTsoDeliveryStatus::applied &&
                 tso.deliveryStatus != umbra::detail::FederationTsoDeliveryStatus::no_messages)) {
              throw RTIinternalError(
                  L"The embedded federation could not begin the timestamped delivery boundary.");
            }
            tsoDeliveries = std::move(tso.deliveries);
          }
        } else {
          std::scoped_lock lock(ambassadorFederationManagementMutex());
          auto& registry = embeddedFederationRegistry();
          if (!pendingSnapshot.currentTime || !pendingSnapshot.requestedTime ||
                pendingSnapshot.currentTime->implementationName() !=
                    pendingSnapshot.implementationName ||
                pendingSnapshot.requestedTime->implementationName() !=
                    pendingSnapshot.implementationName) {
              throw RTIinternalError(
                  L"The embedded Flush Queue Request has incomplete temporal state.");
            }

            auto execution = registry.timeSnapshotFor(federationName);
            if (!execution) {
              throw FederateNotExecutionMember(
                  L"The embedded federation no longer records the Flush Queue requester.");
            }
            umbra::detail::FederationFlushQueueGrantCalculator flushQueueGrantCalculator;
            auto flushQueueCalculation = flushQueueGrantCalculator.calculate(
                *execution,
                federateId);
            if (!flushQueueCalculation.calculated()) {
              throw RTIinternalError(
                  L"The embedded federation could not calculate the Flush Queue Grant times.");
            }

            auto factory = HLAlogicalTimeFactoryFactory::makeLogicalTimeFactory(
                pendingSnapshot.implementationName);
            if (!factory || factory->getName() != pendingSnapshot.implementationName) {
              throw RTIinternalError(
                  L"The embedded federation could not create the Flush Queue time factory.");
            }
            auto finalTime = factory->makeFinal();
            if (!finalTime || finalTime->implementationName() != pendingSnapshot.implementationName) {
              throw RTIinternalError(
                  L"The embedded federation could not create the Flush Queue boundary.");
            }

            if (registry.beginTimeAdvanceGrant(
                    federationName,
                    federateId,
                    generation,
                    dispatchIdentity) !=
                umbra::detail::FederationTimeGrantStatus::applied) {
              return;
            }
            auto tso = registry.beginTsoPayloadDelivery(
                federationName,
                federateId,
                *finalTime,
                true);
            if (tso.status != umbra::detail::FederationTsoRegistryStatus::applied ||
                (tso.deliveryStatus != umbra::detail::FederationTsoDeliveryStatus::applied &&
                 tso.deliveryStatus != umbra::detail::FederationTsoDeliveryStatus::no_messages)) {
              throw RTIinternalError(
                  L"The embedded federation could not flush the timestamped delivery queue.");
            }
            tsoDeliveries = std::move(tso.deliveries);
            // Preserve Time Advancing through queued TSO delivery and any
            // timestamped-save admission. The actual mutation to the FQR
            // grant time is deliberately deferred until the direct callback
            // boundary below.
            flushQueueGrantedTime = std::move(flushQueueCalculation.grantedTime);
            flushQueueOptimisticTime = std::move(flushQueueCalculation.optimisticTime);
        }

        // TSO callbacks are delivered before the matching Time Advance Grant
        // callback. Each entry is completed even when user code throws, so a
        // callback exception cannot leave a private in-transit reservation.
        for (auto const& delivery : tsoDeliveries) {
          try {
            std::visit(
                [&recipient, &federationName, federateId](auto const& typedDelivery) {
                  using Delivery = std::decay_t<decltype(typedDelivery)>;
                  if constexpr (std::is_same_v<
                                    Delivery,
                                    umbra::detail::TsoInteractionDelivery>) {
                    auto const& message = typedDelivery.message;
                    if (!message.timestamp) {
                      finishSuppressedTsoRecipientCallbackIfNeeded(
                          federationName,
                          federateId,
                          message.messageId);
                      throw RTIinternalError(
                          L"The embedded federation could not reconstruct a timestamped interaction callback.");
                    }
                    std::optional<umbra::detail::ReceiveOrderInteractionRecipient>
                        projection;
                    bool callbackMayBegin = false;
                    {
                      std::scoped_lock lock(ambassadorFederationManagementMutex());
                      auto& registry = embeddedFederationRegistry();
                      projection = registry.receiveOrderInteractionRecipientFor(
                              federationName,
                              message.producingFederateId,
                              federateId,
                              message.sentInteractionClassHandle,
                              message.sentParameterHandles,
                              message.sentRegionHandles.empty()
                                  ? nullptr
                                  : &message.sentRegionHandles,
                              message.sentRegionSnapshots.empty()
                                  ? nullptr
                                  : &message.sentRegionSnapshots);
                      if (projection) {
                        callbackMayBegin = registry.beginTsoInteractionCallback(
                            federationName,
                            federateId,
                            message.messageId);
                      }
                    }
                    if (!projection || !callbackMayBegin) {
                      std::scoped_lock lock(ambassadorFederationManagementMutex());
                      static_cast<void>(embeddedFederationRegistry()
                                            .finishTsoRecipientCallbackSuppressed(
                                                federationName,
                                                federateId,
                                                message.messageId));
                      return;
                    }

                    ParameterHandleValueMap parameterValues = ambassadorProjectInteractionParameterValues(
                        message.parameters,
                        projection->receivedParameterHandles);
                    auto const transportationName = umbra::detail::wideFromUtf8(
                        message.transportationName);
                    auto const transportationValue = transportationName
                        ? ambassadorTransportationTypeValueForFederation(federationName, *transportationName)
                        : std::nullopt;
                    if (!transportationValue) {
                      throw RTIinternalError(
                          L"The embedded federation could not reconstruct a timestamped interaction callback transportation type.");
                    }
                    std::optional<RegionHandleSet> optionalSentRegions;
                    if (projection->conveyRegionDesignatorSets &&
                        (!message.sentRegionHandles.empty() || message.defaultRegionUsed)) {
                      optionalSentRegions.emplace();
                      for (auto const regionHandle : message.sentRegionHandles) {
                        optionalSentRegions->insert(makeRegionHandle(regionHandle));
                      }
                    }
                    auto retraction = makeMessageRetractionHandle(message.messageId);
                    recipient.receiveInteraction(
                        makeInteractionClassHandle(projection->receivedInteractionClassHandle),
                        parameterValues,
                        message.userSuppliedTag,
                        makeTransportationTypeHandle(*transportationValue),
                        makeFederateHandle(message.producingFederateId),
                        optionalSentRegions ? &*optionalSentRegions : nullptr,
                        *message.timestamp,
                        message.sentOrderType,
                        message.receivedOrderType,
                        &retraction);
                  } else if constexpr (std::is_same_v<
                                   Delivery,
                                   umbra::detail::TsoAttributeUpdateDelivery>) {
                    auto const& message = typedDelivery.message;
                    if (!message.timestamp) {
                      finishSuppressedTsoRecipientCallbackIfNeeded(
                          federationName,
                          federateId,
                          message.messageId);
                      throw RTIinternalError(
                          L"The embedded federation could not reconstruct a timestamped attribute callback.");
                    }
                    auto const passels = message.passelsByRecipient.find(federateId);
                    if (passels == message.passelsByRecipient.end()) {
                      finishSuppressedTsoRecipientCallbackIfNeeded(
                          federationName,
                          federateId,
                          message.messageId);
                      return;
                    }
                    auto retraction = makeMessageRetractionHandle(message.messageId);
                    bool callbackBegan = false;
                    for (auto const& passel : passels->second) {
                      std::optional<umbra::detail::ReceiveOrderAttributeUpdateRecipient>
                          projection;
                      bool callbackMayBegin = false;
                      {
                        std::scoped_lock lock(ambassadorFederationManagementMutex());
                        auto& registry = embeddedFederationRegistry();
                        projection = registry
                            .receiveOrderAttributeUpdateRecipientFor(
                                federationName,
                                message.producingFederateId,
                                federateId,
                                message.objectInstanceHandle,
                                passel.sentAttributeHandles,
                                passel.sentRegionHandles.empty()
                                    ? nullptr
                                    : &passel.sentRegionHandles,
                                message.sentRegionSnapshots.empty()
                                    ? nullptr
                                    : &message.sentRegionSnapshots);
                      }
                      if (!projection) {
                        continue;
                      }

                      auto const transportationName = umbra::detail::wideFromUtf8(
                          passel.transportationName);
                      auto const transportationValue = transportationName
                          ? ambassadorTransportationTypeValueForFederation(federationName, *transportationName)
                          : std::nullopt;
                      auto const reliableTransportation = ambassadorTransportationTypeReliableForFederation(
                          federationName,
                          passel.transportationName);
                      if (!transportationValue || !reliableTransportation) {
                        throw RTIinternalError(
                            L"The embedded federation could not reconstruct a timestamped attribute callback transportation policy.");
                      }
                      AttributeHandleValueMap attributeValues = ambassadorProjectAttributeValues(
                          message.attributes,
                          projection->receivedAttributeHandles);
                      // Update-rate reduction is a delivery-boundary policy.
                      // The queued TSO path must apply the same per-attribute
                      // gate as the immediate timestamped path; otherwise a
                      // time-constrained recipient would bypass the FDD rate
                      // selected by its subscription.
                      if (!*reliableTransportation) {
                        for (auto iterator = attributeValues.begin();
                             iterator != attributeValues.end();) {
                          std::uint64_t numeric = 0;
                          for (auto const candidate : passel.sentAttributeHandles) {
                            if (makeAttributeHandle(candidate) == iterator->first) {
                              numeric = candidate;
                              break;
                            }
                          }
                          auto const rate = projection->maximumUpdateRatesByAttribute.find(
                              numeric);
                          auto const key = ambassadorUpdateRateAdmissionKey(
                              federationName,
                              federateId,
                              message.objectInstanceHandle,
                              projection->subscriptionGeneration,
                              numeric);
                          bool const admitted = !key || ambassadorAdmitUpdateRate(
                              *key,
                              rate == projection->maximumUpdateRatesByAttribute.end()
                                  ? 0.0
                                  : rate->second,
                              false);
                          if (!admitted) {
                            iterator = attributeValues.erase(iterator);
                          } else {
                            ++iterator;
                          }
                        }
                      }
                      if (attributeValues.empty()) {
                        continue;
                      }
                      {
                        std::scoped_lock lock(ambassadorFederationManagementMutex());
                        callbackMayBegin = embeddedFederationRegistry()
                            .beginTsoAttributeUpdateCallback(
                                federationName,
                                federateId,
                                message.messageId);
                      }
                      if (!callbackMayBegin) {
                        finishSuppressedTsoRecipientCallbackIfNeeded(
                            federationName,
                            federateId,
                            message.messageId);
                        return;
                      }

                      callbackBegan = true;
                      {
                        std::scoped_lock lock(ambassadorFederationManagementMutex());
                        static_cast<void>(embeddedFederationRegistry()
                                              .recordSuccessfulObjectInstanceReflection(
                                                  federationName,
                                                  federateId,
                                                  message.objectInstanceHandle));
                      }
                      recordAmbassadorSuccessfulReflectionReceipt(
                          federationName,
                          federateId,
                          message.objectInstanceHandle,
                          passel.transportationName);

                      std::optional<RegionHandleSet> optionalSentRegions;
                      if (projection->conveyRegionDesignatorSets &&
                          (!passel.sentRegionHandles.empty() || passel.defaultRegionUsed)) {
                        optionalSentRegions.emplace();
                        for (std::uint64_t const regionHandle : passel.sentRegionHandles) {
                          optionalSentRegions->insert(makeRegionHandle(regionHandle));
                        }
                      }
                      recipient.reflectAttributeValues(
                          makeObjectInstanceHandle(message.objectInstanceHandle),
                          attributeValues,
                          message.userSuppliedTag,
                          makeTransportationTypeHandle(*transportationValue),
                          makeFederateHandle(message.producingFederateId),
                          optionalSentRegions ? &*optionalSentRegions : nullptr,
                          *message.timestamp,
                          passel.preferredOrderType,
                          TIMESTAMP,
                          &retraction);
                    }
                    if (!callbackBegan) {
                      std::scoped_lock lock(ambassadorFederationManagementMutex());
                      static_cast<void>(embeddedFederationRegistry()
                                            .finishTsoRecipientCallbackSuppressed(
                                                federationName,
                                                federateId,
                                                message.messageId));
                    }
                  } else if constexpr (std::is_same_v<
                                           Delivery,
                                           umbra::detail::TsoObjectDeletionDelivery>) {
                    auto const& message = typedDelivery.message;
                    if (!message.timestamp) {
                      {
                        std::scoped_lock lock(ambassadorFederationManagementMutex());
                        static_cast<void>(embeddedFederationRegistry()
                                              .finishTsoRecipientCallbackSuppressed(
                                                  federationName,
                                                  federateId,
                                                  message.messageId));
                      }
                      throw RTIinternalError(
                          L"The embedded federation could not reconstruct a timestamped object-removal callback.");
                    }
                    auto const target = std::find_if(
                        message.recipients.begin(),
                        message.recipients.end(),
                        [federateId](umbra::detail::TsoObjectDeletionRecipient const& candidate) {
                          return candidate.receivingFederateId == federateId;
                        });
                    if (target == message.recipients.end()) {
                      std::scoped_lock lock(ambassadorFederationManagementMutex());
                      static_cast<void>(embeddedFederationRegistry()
                                            .finishTsoRecipientCallbackSuppressed(
                                                federationName,
                                                federateId,
                                                message.messageId));
                      return;
                    }

                    std::optional<umbra::detail::RemovedObjectInstanceSnapshot> removal;
                    {
                      std::scoped_lock lock(ambassadorFederationManagementMutex());
                      removal = embeddedFederationRegistry()
                          .beginTsoObjectInstanceRemoval(
                          federationName,
                          federateId,
                          message.objectInstanceHandle,
                          message.messageId);
                    }
                    if (!removal) {
                      // The queued delivery reached its callback boundary,
                      // but the object was locally forgotten or a legal
                      // Retract won the race. No Remove Object Instance
                      // callback was entered, so close the recipient as a
                      // suppressed delivery rather than leaving it pending.
                      {
                        std::scoped_lock lock(ambassadorFederationManagementMutex());
                        static_cast<void>(embeddedFederationRegistry()
                                              .finishTsoRecipientCallbackSuppressed(
                                                  federationName,
                                                  federateId,
                                                  message.messageId));
                      }
                      return;
                    }

                    if (target->serviceReportRoute) {
                      // The TSO delivery boundary has now committed this
                      // recipient's removal. Its selected report file must
                      // receive the full timestamped §6.17 form before the
                      // recipient can observe the callback.
                      target->serviceReportRoute(
                          static_cast<std::uint16_t>(
                              umbra::detail::MomServiceType::object_management),
                          [
                              objectInstanceHandle = removal->objectInstanceHandle,
                              userSuppliedTag = message.userSuppliedTag,
                              producingFederateId = removal->producingFederateId,
                              timestamp = message.timestamp,
                              messageId = message.messageId](std::uint32_t serialNumber) {
                            return makeAmbassadorRemoveObjectInstanceServiceReportRecord(
                                objectInstanceHandle,
                                userSuppliedTag,
                                producingFederateId,
                                TIMESTAMP,
                                timestamp.get(),
                                TIMESTAMP,
                                messageId,
                                serialNumber);
                          });
                    }

                    auto retraction = makeMessageRetractionHandle(message.messageId);
                    recipient.removeObjectInstance(
                        makeObjectInstanceHandle(removal->objectInstanceHandle),
                        message.userSuppliedTag,
                        makeFederateHandle(removal->producingFederateId),
                        *message.timestamp,
                        TIMESTAMP,
                        TIMESTAMP,
                        &retraction);
                  } else {
                    auto const& message = typedDelivery.message;
                    if (!message.timestamp) {
                      finishSuppressedTsoRecipientCallbackIfNeeded(
                          federationName,
                          federateId,
                          message.messageId);
                      throw RTIinternalError(
                          L"The embedded federation could not reconstruct a timestamped directed-interaction callback.");
                    }
                    auto const target = std::find_if(
                        message.recipients.begin(),
                        message.recipients.end(),
                        [federateId](umbra::detail::TsoDirectedInteractionRecipient const& candidate) {
                          return candidate.receivingFederateId == federateId;
                        });
                    if (target == message.recipients.end()) {
                      finishSuppressedTsoRecipientCallbackIfNeeded(
                          federationName,
                          federateId,
                          message.messageId);
                      return;
                    }
                    auto const transportationName = umbra::detail::wideFromUtf8(
                        message.transportationName);
                    auto const transportationValue = transportationName
                        ? ambassadorTransportationTypeValueForFederation(
                              federationName,
                              *transportationName)
                        : std::nullopt;
                    if (!transportationValue) {
                      throw RTIinternalError(
                          L"The embedded federation could not reconstruct a timestamped directed-interaction transportation type.");
                    }
                    std::optional<umbra::detail::ReceiveOrderDirectedInteractionRecipient>
                        projection;
                    bool callbackMayBegin = false;
                    {
                      std::scoped_lock lock(ambassadorFederationManagementMutex());
                      auto& registry = embeddedFederationRegistry();
                      projection = registry.timestampedDirectedInteractionRecipientFor(
                          federationName,
                          message.producingFederateId,
                          federateId,
                          message.objectInstanceHandle,
                          message.sentInteractionClassHandle,
                          message.sentParameterHandles,
                          message.messageId);
                      if (projection) {
                        callbackMayBegin = registry.beginTsoInteractionCallback(
                            federationName,
                            federateId,
                            message.messageId);
                      }
                    }
                    if (!projection || !callbackMayBegin) {
                      std::scoped_lock lock(ambassadorFederationManagementMutex());
                      static_cast<void>(embeddedFederationRegistry()
                                            .finishTsoRecipientCallbackSuppressed(
                                                federationName,
                                                federateId,
                                                message.messageId));
                      return;
                    }
                    ParameterHandleValueMap parameterValues = ambassadorProjectInteractionParameterValues(
                        message.parameters,
                        projection->receivedParameterHandles);
                    auto retraction = makeMessageRetractionHandle(message.messageId);
                    recipient.receiveDirectedInteraction(
                        makeInteractionClassHandle(projection->receivedInteractionClassHandle),
                        makeObjectInstanceHandle(projection->objectInstanceHandle),
                        parameterValues,
                        message.userSuppliedTag,
                        makeTransportationTypeHandle(*transportationValue),
                        makeFederateHandle(message.producingFederateId),
                        *message.timestamp,
                        message.sentOrderType,
                        message.receivedOrderType,
                        &retraction);
                  }
                },
                delivery);
          } catch (...) {
            std::scoped_lock lock(ambassadorFederationManagementMutex());
            static_cast<void>(embeddedFederationRegistry().completeTsoDelivery(
                federationName,
                std::visit(
                    [](auto const& typedDelivery) {
                      return typedDelivery.queuedMessage;
                    },
                    delivery)));
            throw;
          }

          std::scoped_lock lock(ambassadorFederationManagementMutex());
          auto& registry = embeddedFederationRegistry();
          auto const completed = registry.completeTsoDelivery(
              federationName,
              std::visit(
                  [](auto const& typedDelivery) {
                    return typedDelivery.queuedMessage;
                  },
                  delivery));
          if (completed.status != umbra::detail::FederationTsoRegistryStatus::applied ||
              (completed.delivery.status != umbra::detail::FederationTsoDeliveryStatus::applied &&
               completed.delivery.status !=
                   umbra::detail::FederationTsoDeliveryStatus::message_already_completed)) {
            throw RTIinternalError(
                L"The embedded federation could not complete the timestamped delivery.");
          }
          auto releasedRemovals = registry
              .releaseConnectionLossDeferredObjectInstanceRemovals(
                  federationName,
                  federateId);
          for (auto& removal : releasedRemovals) {
            postGrantObjectRemovals.push_back(std::move(removal));
          }
        }

        if (!flushQueue) {
          std::optional<std::wstring> timedSaveLabel;
          std::shared_ptr<LogicalTime const> timedSaveTimestamp;
          {
            std::scoped_lock lock(ambassadorFederationManagementMutex());
            auto admission = embeddedFederationRegistry()
                .admitTimedFederationSaveAtTimeAdvanceBoundary(
                    federationName,
                    federateId);
            if (admission.status != umbra::detail::FederationSaveControlStatus::applied) {
              throw RTIinternalError(
                  L"The embedded federation could not admit a timestamped save at the time-advance boundary.");
            }
            timedSaveLabel = std::move(admission.currentFederateLabel);
            timedSaveTimestamp = std::move(admission.currentFederateTimestamp);
            timedSaveNotifications = std::move(admission.notifications);
          }

          // The timestamped path reaches this point only after the recipient
          // has received all in-process TSO payloads through the scheduled
          // save time. It is still Time Advancing because grant() has not yet
          // changed its private state.
          if (timedSaveLabel.has_value()) {
            nextSaveConditionalsChanged = true;
            if (!timedSaveTimestamp) {
              throw RTIinternalError(
                  L"The embedded federation admitted a timestamped save without its requested time.");
            }
            recipient.initiateFederateSave(
                *timedSaveLabel,
                *timedSaveTimestamp);
            queueAmbassadorJoinedFederateMomConditionalAttributeUpdateForJoinedFederate(
                federationName,
                federateId,
                {umbra::detail::hla::utf8::mom::federate_state},
                federateId);
          }

          {
            std::scoped_lock lock(ambassadorFederationManagementMutex());
            auto& registry = embeddedFederationRegistry();
            grantedTime = timeState->grant(generation);
            if (!grantedTime) {
              return;
            }
            grantCompleted = true;
            auto scheduled = registry.reevaluateTimeAdvanceGrants(federationName);
            if (scheduled.status == umbra::detail::FederationTimeGrantStatus::applied) {
              newlyEligible = std::move(scheduled.dispatches);
            }
          }
        } else {
          if (!flushQueueGrantedTime || !flushQueueOptimisticTime) {
            throw RTIinternalError(
                L"The embedded Flush Queue Grant has no calculated logical times.");
          }

          std::optional<std::wstring> timedSaveLabel;
          std::shared_ptr<LogicalTime const> timedSaveTimestamp;
          std::shared_ptr<LogicalTime const> const flushQueueGrantForAdmission =
              flushQueueGrantedTime;
          {
            std::scoped_lock lock(ambassadorFederationManagementMutex());
            auto admission = embeddedFederationRegistry()
                .admitTimedFederationSaveAtFlushQueueGrantBoundary(
                    federationName,
                    federateId,
                    flushQueueGrantForAdmission);
            if (admission.status != umbra::detail::FederationSaveControlStatus::applied) {
              throw RTIinternalError(
                  L"The embedded federation could not admit a timestamped save at the Flush Queue boundary.");
            }
            timedSaveLabel = std::move(admission.currentFederateLabel);
            timedSaveTimestamp = std::move(admission.currentFederateTimestamp);
            timedSaveNotifications = std::move(admission.notifications);
          }

          // A qualifying FQR direct admission follows its complete queued TSO
          // delivery set but precedes the private grant transition, preserving
          // the recipient's required Time Advancing state.
          if (timedSaveLabel.has_value()) {
            nextSaveConditionalsChanged = true;
            if (!timedSaveTimestamp) {
              throw RTIinternalError(
                  L"The embedded federation admitted a timestamped save without its requested time.");
            }
            recipient.initiateFederateSave(
                *timedSaveLabel,
                *timedSaveTimestamp);
            queueAmbassadorJoinedFederateMomConditionalAttributeUpdateForJoinedFederate(
                federationName,
                federateId,
                {umbra::detail::hla::utf8::mom::federate_state},
                federateId);
          }

          {
            std::scoped_lock lock(ambassadorFederationManagementMutex());
            auto& registry = embeddedFederationRegistry();
            auto stateOptimistic = cloneAmbassadorReferenceLogicalTime(
                pendingSnapshot.implementationName,
                *flushQueueOptimisticTime);
            if (!stateOptimistic) {
              throw RTIinternalError(
                  L"The embedded Flush Queue Grant could not clone its optimistic logical time.");
            }
            auto const flushResult = timeState->grantFlushQueue(
                generation,
                std::move(flushQueueGrantedTime),
                std::move(stateOptimistic));
            if (flushResult.status != umbra::detail::FederateTimeAdvanceStatus::applied) {
              throw RTIinternalError(
                  L"The embedded federate time state rejected the Flush Queue Grant.");
            }
            grantedTime = timeState->currentTime();
            optimisticTime = std::move(flushQueueOptimisticTime);
            grantCompleted = true;
            auto scheduled = registry.reevaluateTimeAdvanceGrants(federationName);
            if (scheduled.status == umbra::detail::FederationTimeGrantStatus::applied) {
              newlyEligible = std::move(scheduled.dispatches);
            }
          }
        }

        // The state mutation happens immediately before its matching standard
        // callback, after the registry has rechecked the current shared bound.
        if (flushQueue) {
          if (!optimisticTime) {
            throw RTIinternalError(
                L"The embedded Flush Queue Grant has no optimistic logical time.");
          }
          recipient.flushQueueGrant(*grantedTime, *optimisticTime);
        } else {
          recipient.timeAdvanceGrant(*grantedTime);
        }

      });

      if (grantCompleted) {
        // The grant has completed the private transition back to Time Granted
        // (or the Flush Queue equivalent).  Publish the conditional MOM value
        // only after the matching public callback has been invoked.
        queueAmbassadorJoinedFederateMomConditionalAttributeUpdateForJoinedFederate(
            federationName,
            federateId,
            {umbra::detail::hla::utf8::mom::time_manager_state});
      }

      if (nextSaveConditionalsChanged) {
        queueAmbassadorFederationMomConditionalAttributeUpdate(
            federationName,
            {umbra::detail::hla::utf8::mom::next_save_name, umbra::detail::hla::utf8::mom::next_save_time});
      }

      // A receive-order automatic-resign removal that encountered a protected
      // connection-loss TSO boundary is resubmitted only after the matching
      // grant callback. That preserves the reflection/directed callback first
      // and lets the normal asynchronous-delivery gate decide its next
      // receive-order window.
      queueAmbassadorObjectInstanceRemovals(
          std::move(postGrantObjectRemovals),
          federationName,
          VariableLengthData());
      submitAmbassadorFederationSaveNotifications(
          federationName,
          std::move(immediateSaveNotifications));
      submitAmbassadorFederationSaveNotifications(
          federationName,
          std::move(timedSaveNotifications));
      submitAmbassadorTimeAdvanceGrantDispatches(std::move(newlyEligible));
    });
  };
}

umbra::detail::FederationTimeGrantDispatchFactory makeTimeAdvanceGrantDispatchFactory(
    std::shared_ptr<umbra::detail::CallbackDispatcher> const& callbackDispatcher,
    std::shared_ptr<CallbackSession> const& callbackSession,
    std::shared_ptr<umbra::detail::FederateTimeState> const& timeState,
    std::wstring federationName) {
  std::weak_ptr<umbra::detail::CallbackDispatcher> const dispatcher = callbackDispatcher;
  std::weak_ptr<CallbackSession> const session = callbackSession;
  std::weak_ptr<umbra::detail::FederateTimeState> const federateTimeState = timeState;

  return [
      dispatcher,
      session,
      federateTimeState,
      federationName = std::move(federationName)](
      std::uint64_t federateId,
      std::uint64_t generation,
      std::uint64_t dispatchIdentity) {
    auto callbackDispatcher = dispatcher.lock();
    auto callbackSession = session.lock();
    auto timeState = federateTimeState.lock();
    if (!callbackDispatcher || !callbackSession || !timeState) {
      return umbra::detail::FederationTimeGrantDispatch{};
    }
    return makeTimeAdvanceGrantDispatch(
        callbackDispatcher,
        callbackSession,
        timeState,
        federationName,
        federateId,
        generation,
        dispatchIdentity);
  };
}

umbra::detail::FederationTimeRoleEnableDispatchFactory
makeTimeRoleEnableDispatchFactory(
    std::shared_ptr<umbra::detail::CallbackDispatcher> const& callbackDispatcher,
    std::shared_ptr<CallbackSession> const& callbackSession,
    std::shared_ptr<umbra::detail::FederateTimeState> const& timeState,
    std::wstring federationName) {
  std::weak_ptr<umbra::detail::CallbackDispatcher> const dispatcher = callbackDispatcher;
  std::weak_ptr<CallbackSession> const session = callbackSession;
  std::weak_ptr<umbra::detail::FederateTimeState> const federateTimeState = timeState;

  return [
      dispatcher,
      session,
      federateTimeState,
      federationName = std::move(federationName)](
      std::uint64_t federateId,
      std::uint64_t generation,
      umbra::detail::FederationTimeRoleEnableKind kind)
      -> umbra::detail::FederationTimeGrantDispatch {
    auto callbackDispatcher = dispatcher.lock();
    auto callbackSession = session.lock();
    auto state = federateTimeState.lock();
    if (!callbackDispatcher || !callbackSession || !state || federateId == 0U ||
        generation == 0U) {
      return umbra::detail::FederationTimeGrantDispatch{};
    }

    // This factory is called after restore has applied the target temporal
    // image. Capture that fresh epoch so a later restore before delivery
    // fences this dispatch through the atomic grant-if-current gate.
    auto const callbackEpoch = state->callbackEpoch();
    return [
        dispatcher,
        session,
        federateTimeState,
        federationName,
        federateId,
        generation,
        callbackEpoch,
        kind] {
      auto callbackDispatcher = dispatcher.lock();
      if (!callbackDispatcher) {
        return;
      }
      callbackDispatcher->submit([
          session,
          federateTimeState,
          federationName,
          federateId,
          generation,
          callbackEpoch,
          kind] {
        auto callbackSession = session.lock();
        auto state = federateTimeState.lock();
        if (!callbackSession || !state) {
          return;
        }

        std::vector<umbra::detail::FederationTimeGrantDispatch> newlyEligible;
        bool granted = false;
        callbackSession->invoke([
            state,
            federationName,
            federateId,
            generation,
            callbackEpoch,
            kind,
            &newlyEligible,
            &granted](FederateAmbassador& recipient) {
          std::shared_ptr<LogicalTime const> enabledTime;
          {
            std::scoped_lock lock(ambassadorFederationManagementMutex());
            if (kind == umbra::detail::FederationTimeRoleEnableKind::regulation) {
              enabledTime = state->grantTimeRegulationIfCurrent(
                  generation,
                  callbackEpoch);
              if (!enabledTime) {
                return;
              }
              auto scheduled = embeddedFederationRegistry()
                                   .reevaluateTimeAdvanceGrants(federationName);
              if (scheduled.status ==
                  umbra::detail::FederationTimeGrantStatus::applied) {
                newlyEligible = std::move(scheduled.dispatches);
              }
            } else {
              enabledTime = state->grantTimeConstrainedIfCurrent(
                  generation,
                  callbackEpoch);
              if (!enabledTime) {
                return;
              }
            }
            granted = true;
          }

          if (kind == umbra::detail::FederationTimeRoleEnableKind::regulation) {
            recipient.timeRegulationEnabled(*enabledTime);
          } else {
            recipient.timeConstrainedEnabled(*enabledTime);
          }
        });

        if (!granted) {
          return;
        }
        if (kind == umbra::detail::FederationTimeRoleEnableKind::regulation) {
          queueAmbassadorJoinedFederateMomConditionalAttributeUpdateForJoinedFederate(
              federationName,
              federateId,
              {umbra::detail::hla::utf8::mom::time_regulating});
          submitAmbassadorTimeAdvanceGrantDispatches(std::move(newlyEligible));
        } else {
          queueAmbassadorJoinedFederateMomConditionalAttributeUpdateForJoinedFederate(
              federationName,
              federateId,
              {umbra::detail::hla::utf8::mom::time_constrained});
        }
      });
    };
  };
}

}
umbra::detail::FederationTimeGrantDispatchFactory
ambassadorMakeTimeAdvanceGrantDispatchFactory(
    std::shared_ptr<umbra::detail::CallbackDispatcher> const& callbackDispatcher,
    std::shared_ptr<CallbackSession> const& callbackSession,
    std::shared_ptr<umbra::detail::FederateTimeState> const& timeState,
    std::wstring federationName) {
  return makeTimeAdvanceGrantDispatchFactory(
      callbackDispatcher,
      callbackSession,
      timeState,
      std::move(federationName));
}

umbra::detail::FederationTimeRoleEnableDispatchFactory
ambassadorMakeTimeRoleEnableDispatchFactory(
    std::shared_ptr<umbra::detail::CallbackDispatcher> const& callbackDispatcher,
    std::shared_ptr<CallbackSession> const& callbackSession,
    std::shared_ptr<umbra::detail::FederateTimeState> const& timeState,
    std::wstring federationName) {
  return makeTimeRoleEnableDispatchFactory(
      callbackDispatcher,
      callbackSession,
      timeState,
      std::move(federationName));
}

void submitAmbassadorTimeAdvanceGrantDispatches(
    std::vector<umbra::detail::FederationTimeGrantDispatch> dispatches) {
  submitTimeAdvanceGrantDispatches(std::move(dispatches));
}

#endif
}  // namespace rti1516_2025::umbra_binding_detail
