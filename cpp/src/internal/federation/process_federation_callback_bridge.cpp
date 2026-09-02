#include "internal/federation/process_federation_callback_bridge.hpp"

#include "internal/handles/attribute_handle.hpp"
#include "internal/handles/object_class_handle.hpp"
#include "internal/handles/federate_handle.hpp"
#include "internal/handles/interaction_class_handle.hpp"
#include "internal/handles/object_instance_handle.hpp"
#include "internal/handles/parameter_handle.hpp"
#include "internal/handles/region_handle.hpp"
#include "internal/handles/message_retraction_handle.hpp"
#include "internal/handles/transportation_type_handle.hpp"
#include "internal/runtime/utf8_string.hpp"

#include <RTI/Exception.h>
#include <RTI/FederateAmbassador.h>
#include <RTI/VariableLengthData.h>
#include <RTI/time/HLAlogicalTimeFactoryFactory.h>
#include <RTI/time/LogicalTime.h>

#include <algorithm>
#include <memory>
#include <utility>

namespace umbra::detail {
namespace {

std::unique_ptr<rti1516_2025::LogicalTime> decodeEventTimestamp(
    ProcessFederationLogicalTime const& timestamp) {
  if (timestamp.implementationName.empty()) {
    throw ProcessFederationCallbackBridgeError(
        "A process callback timestamp requires a logical-time implementation name.");
  }
  auto factory = rti1516_2025::HLAlogicalTimeFactoryFactory::makeLogicalTimeFactory(
      timestamp.implementationName);
  if (!factory || factory->getName() != timestamp.implementationName) {
    throw ProcessFederationCallbackBridgeError(
        "A process callback timestamp uses an unavailable logical-time factory.");
  }
  rti1516_2025::VariableLengthData encoded;
  if (!timestamp.encoding.empty()) {
    encoded.setData(timestamp.encoding.data(), timestamp.encoding.size());
  }
  std::unique_ptr<rti1516_2025::LogicalTime> decoded;
  try {
    decoded = factory->decodeLogicalTime(encoded);
  } catch (rti1516_2025::Exception const&) {
    throw ProcessFederationCallbackBridgeError(
        "A process callback timestamp could not be decoded by its logical-time factory.");
  }
  if (!decoded || decoded->implementationName() != timestamp.implementationName ||
      decoded->isInitial() || decoded->isFinal()) {
    throw ProcessFederationCallbackBridgeError(
        "A process callback timestamp is not a finite logical time.");
  }
  return decoded;
}

void deliverReceiveOrder(
    ProcessFederationInteractionEvent const& event,
    rti1516_2025::FederateAmbassador& recipient) {
  if (event.producingFederateId == 0U ||
      event.interactionClassHandle == 0U ||
      event.receivingFederateId == 0U) {
    throw ProcessFederationCallbackBridgeError(
        "A process callback event requires producer, recipient, and interaction identities.");
  }

  auto const transportationValue =
      rti1516_2025::umbra_binding_detail::standardTransportationTypeValue(
          std::wstring(event.transportationName.begin(), event.transportationName.end()));
  if (!transportationValue) {
    throw ProcessFederationCallbackBridgeError(
        "A process callback event has an unknown transportation type.");
  }

  rti1516_2025::ParameterHandleValueMap parameterValues;
  auto const envelope =
      decodeProcessFederationInteractionEnvelope(event.payload);
  for (std::uint64_t const parameterHandle : event.parameterHandles) {
    if (parameterHandle == 0U) {
      throw ProcessFederationCallbackBridgeError(
          "A process callback event contains an invalid parameter identity.");
    }

    rti1516_2025::VariableLengthData parameterValue;
    if (envelope) {
      auto const found = std::find_if(
          envelope->parameterValues.begin(),
          envelope->parameterValues.end(),
          [parameterHandle](
              ProcessFederationInteractionParameterValue const& value) {
            return value.first == parameterHandle;
          });
      if (found != envelope->parameterValues.end()) {
        parameterValue.setData(
            found->second.data(), found->second.size());
      }
    }
    parameterValues.emplace(
        rti1516_2025::umbra_binding_detail::makeParameterHandle(parameterHandle),
        std::move(parameterValue));
  }

  rti1516_2025::VariableLengthData userSuppliedTag;
  if (envelope) {
    userSuppliedTag.setData(
        envelope->userSuppliedTag.data(), envelope->userSuppliedTag.size());
  } else {
    // Keep the legacy private probe payload semantics: before the versioned
    // envelope existed, the opaque bytes represented the user-supplied tag.
    userSuppliedTag.setData(event.payload.data(), event.payload.size());
  }
  auto interactionClass =
      rti1516_2025::umbra_binding_detail::makeInteractionClassHandle(
          event.interactionClassHandle);
  auto transportationType =
      rti1516_2025::umbra_binding_detail::makeTransportationTypeHandle(
          *transportationValue);
  auto producingFederate =
      rti1516_2025::umbra_binding_detail::makeFederateHandle(
          event.producingFederateId);
  std::optional<rti1516_2025::MessageRetractionHandle> optionalRetraction;
  if (event.retractionMessageId) {
    optionalRetraction.emplace(
        rti1516_2025::umbra_binding_detail::makeMessageRetractionHandle(
            *event.retractionMessageId));
  }
  if (event.objectInstanceHandle) {
    auto const objectInstance =
        rti1516_2025::umbra_binding_detail::makeObjectInstanceHandle(
            *event.objectInstanceHandle);
    if (event.timestamp) {
      auto timestamp = decodeEventTimestamp(*event.timestamp);
      recipient.receiveDirectedInteraction(
          interactionClass,
          objectInstance,
          parameterValues,
          userSuppliedTag,
          transportationType,
          producingFederate,
          *timestamp,
          rti1516_2025::RECEIVE,
          rti1516_2025::RECEIVE,
          optionalRetraction ? &*optionalRetraction : nullptr);
    } else {
      recipient.receiveDirectedInteraction(
          interactionClass,
          objectInstance,
          parameterValues,
          userSuppliedTag,
          transportationType,
          producingFederate);
    }
    return;
  }
  if (event.timestamp) {
    auto timestamp = decodeEventTimestamp(*event.timestamp);
    // A process endpoint currently has no time-regulation service. The
    // timestamp is nevertheless delivered through the official timestamped
    // callback overload with receive-order classifications; when the private
    // service retained a directed-message identity, the same identity is
    // exposed through the optional retraction pointer above.
    recipient.receiveInteraction(
        interactionClass,
        parameterValues,
        userSuppliedTag,
        transportationType,
        producingFederate,
        nullptr,
        *timestamp,
        rti1516_2025::RECEIVE,
        rti1516_2025::RECEIVE,
        nullptr);
    return;
  }
  recipient.receiveInteraction(
      interactionClass,
      parameterValues,
      userSuppliedTag,
      transportationType,
      producingFederate,
      nullptr);
}

void deliverRequestRetraction(
    std::uint64_t messageId,
    rti1516_2025::FederateAmbassador& recipient) {
  if (messageId == 0U) {
    throw ProcessFederationCallbackBridgeError(
        "A process Request Retraction callback requires a message identity.");
  }
  recipient.requestRetraction(
      rti1516_2025::umbra_binding_detail::makeMessageRetractionHandle(
          messageId));
}

void deliverAttributeUpdate(
    ProcessFederationAttributeUpdateEvent const& event,
    rti1516_2025::FederateAmbassador& recipient) {
  if (event.producingFederateId == 0U ||
      event.receivingFederateId == 0U ||
      event.objectInstanceHandle == 0U ||
      event.attributeValues.empty()) {
    throw ProcessFederationCallbackBridgeError(
        "A process attribute callback requires producer, recipient, object, and values.");
  }

  auto const transportationValue =
      rti1516_2025::umbra_binding_detail::standardTransportationTypeValue(
          std::wstring(event.transportationName.begin(), event.transportationName.end()));
  if (!transportationValue) {
    throw ProcessFederationCallbackBridgeError(
        "A process attribute callback has an unknown transportation type.");
  }

  rti1516_2025::AttributeHandleValueMap attributeValues;
  for (auto const& [attributeHandle, value] : event.attributeValues) {
    if (attributeHandle == 0U) {
      throw ProcessFederationCallbackBridgeError(
          "A process attribute callback contains an invalid attribute identity.");
    }
    rti1516_2025::VariableLengthData encodedValue;
    if (!value.empty()) {
      encodedValue.setData(value.data(), value.size());
    }
    attributeValues.emplace(
        rti1516_2025::umbra_binding_detail::makeAttributeHandle(attributeHandle),
        std::move(encodedValue));
  }

  rti1516_2025::VariableLengthData userSuppliedTag;
  if (!event.userSuppliedTag.empty()) {
    userSuppliedTag.setData(
        event.userSuppliedTag.data(), event.userSuppliedTag.size());
  }
  std::optional<rti1516_2025::RegionHandleSet> optionalSentRegions;
  if (event.sentRegionHandles.has_value() || event.defaultRegionUsed) {
    optionalSentRegions.emplace();
    if (event.sentRegionHandles.has_value()) {
      for (std::uint64_t const regionHandle : *event.sentRegionHandles) {
        if (regionHandle == 0U) {
          throw ProcessFederationCallbackBridgeError(
              "A process attribute callback contains an invalid region identity.");
        }
        optionalSentRegions->insert(
            rti1516_2025::umbra_binding_detail::makeRegionHandle(regionHandle));
      }
    }
  }
  auto const objectInstance =
      rti1516_2025::umbra_binding_detail::makeObjectInstanceHandle(
          event.objectInstanceHandle);
  auto const transportationType =
      rti1516_2025::umbra_binding_detail::makeTransportationTypeHandle(
          *transportationValue);
  auto const producingFederate =
      rti1516_2025::umbra_binding_detail::makeFederateHandle(
          event.producingFederateId);
  if (event.timestamp) {
    auto timestamp = decodeEventTimestamp(*event.timestamp);
    // The process endpoint currently has no time-regulation/retraction
    // protocol. Preserve the timestamp through the official callback while
    // classifying this bounded remote projection as receive-order on both
    // sides and exposing no fabricated retraction handle.
    recipient.reflectAttributeValues(
        objectInstance,
        attributeValues,
        userSuppliedTag,
        transportationType,
        producingFederate,
        optionalSentRegions ? &*optionalSentRegions : nullptr,
        *timestamp,
        rti1516_2025::RECEIVE,
        rti1516_2025::RECEIVE,
        nullptr);
    return;
  }
  recipient.reflectAttributeValues(
      objectInstance,
      attributeValues,
      userSuppliedTag,
      transportationType,
      producingFederate,
      optionalSentRegions ? &*optionalSentRegions : nullptr);
}

void deliverObjectInstanceDiscovery(
    ProcessFederationObjectInstanceDiscoveryEvent const& event,
    rti1516_2025::FederateAmbassador& recipient) {
  if (event.receivingFederateId == 0U ||
      event.objectInstanceHandle == 0U ||
      event.objectClassHandle == 0U ||
      event.producingFederateId == 0U ||
      event.objectInstanceName.empty()) {
    throw ProcessFederationCallbackBridgeError(
        "A process discovery callback requires recipient, object, class, producer, and name.");
  }
  recipient.discoverObjectInstance(
      rti1516_2025::umbra_binding_detail::makeObjectInstanceHandle(
          event.objectInstanceHandle),
      rti1516_2025::umbra_binding_detail::makeObjectClassHandle(
          event.objectClassHandle),
      event.objectInstanceName,
      rti1516_2025::umbra_binding_detail::makeFederateHandle(
          event.producingFederateId));
}

void deliverObjectInstanceRemoval(
    ProcessFederationObjectInstanceRemovalEvent const& event,
    rti1516_2025::FederateAmbassador& recipient) {
  if (event.receivingFederateId == 0U ||
      event.objectInstanceHandle == 0U ||
      event.producingFederateId == 0U) {
    throw ProcessFederationCallbackBridgeError(
        "A process removal callback requires recipient, object, and producer identities.");
  }
  rti1516_2025::VariableLengthData userSuppliedTag;
  if (!event.userSuppliedTag.empty()) {
    userSuppliedTag.setData(
        event.userSuppliedTag.data(), event.userSuppliedTag.size());
  }
  auto const objectInstance =
      rti1516_2025::umbra_binding_detail::makeObjectInstanceHandle(
          event.objectInstanceHandle);
  auto const producingFederate =
      rti1516_2025::umbra_binding_detail::makeFederateHandle(
          event.producingFederateId);
  if (event.timestamp) {
    auto timestamp = decodeEventTimestamp(*event.timestamp);
    std::optional<rti1516_2025::MessageRetractionHandle> optionalRetraction;
    if (event.retractionMessageId) {
      optionalRetraction.emplace(
          rti1516_2025::umbra_binding_detail::makeMessageRetractionHandle(
              *event.retractionMessageId));
    }
    recipient.removeObjectInstance(
        objectInstance,
        userSuppliedTag,
        producingFederate,
        *timestamp,
        rti1516_2025::RECEIVE,
        rti1516_2025::RECEIVE,
        optionalRetraction ? &*optionalRetraction : nullptr);
    return;
  }
  recipient.removeObjectInstance(
      objectInstance,
      userSuppliedTag,
      producingFederate);
}

void deliverObjectInstanceScopeChange(
    ProcessFederationObjectInstanceScopeChangeEvent const& event,
    rti1516_2025::FederateAmbassador& recipient) {
  if (event.receivingFederateId == 0U ||
      event.objectInstanceHandle == 0U ||
      event.attributeHandles.empty()) {
    throw ProcessFederationCallbackBridgeError(
        "A process scope callback requires recipient, object, and attributes.");
  }
  rti1516_2025::AttributeHandleSet attributes;
  for (std::uint64_t const attributeHandle : event.attributeHandles) {
    if (attributeHandle == 0U) {
      throw ProcessFederationCallbackBridgeError(
          "A process scope callback contains an invalid attribute identity.");
    }
    attributes.insert(
        rti1516_2025::umbra_binding_detail::makeAttributeHandle(attributeHandle));
  }
  auto const objectInstance =
      rti1516_2025::umbra_binding_detail::makeObjectInstanceHandle(
          event.objectInstanceHandle);
  if (event.inScope) {
    recipient.attributesInScope(objectInstance, attributes);
  } else {
    recipient.attributesOutOfScope(objectInstance, attributes);
  }
}

void deliverAttributeRelevanceAdvisory(
    ProcessFederationAttributeRelevanceAdvisoryEvent const& event,
    rti1516_2025::FederateAmbassador& recipient) {
  if (event.providingFederateId == 0U ||
      event.objectInstanceHandle == 0U ||
      event.attributeHandles.empty()) {
    throw ProcessFederationCallbackBridgeError(
        "A process attribute relevance callback requires owner, object, and attributes.");
  }
  rti1516_2025::AttributeHandleSet attributes;
  for (std::uint64_t const attributeHandle : event.attributeHandles) {
    if (attributeHandle == 0U) {
      throw ProcessFederationCallbackBridgeError(
          "A process attribute relevance callback contains an invalid attribute identity.");
    }
    attributes.insert(
        rti1516_2025::umbra_binding_detail::makeAttributeHandle(attributeHandle));
  }
  auto const objectInstance =
      rti1516_2025::umbra_binding_detail::makeObjectInstanceHandle(
          event.objectInstanceHandle);
  if (!event.turnUpdatesOn) {
    recipient.turnUpdatesOffForObjectInstance(objectInstance, attributes);
    return;
  }
  if (event.updateRateDesignator) {
    auto const wideDesignator =
        umbra::detail::wideFromUtf8(*event.updateRateDesignator);
    if (!wideDesignator) {
      throw ProcessFederationCallbackBridgeError(
          "A process attribute relevance callback has an invalid UTF-8 update-rate designator.");
    }
    recipient.turnUpdatesOnForObjectInstance(
        objectInstance, attributes, *wideDesignator);
    return;
  }
  recipient.turnUpdatesOnForObjectInstance(objectInstance, attributes);
}

}  // namespace

ProcessFederationCallbackBridge::ProcessFederationCallbackBridge(
    rti1516_2025::FederateAmbassador& recipient,
    CallbackDispatchModel model)
    : dispatcher_(std::make_shared<CallbackDispatcher>(model)),
      callbackSession_(
          std::make_shared<rti1516_2025::umbra_binding_detail::CallbackSession>(
              recipient)) {}

ProcessFederationCallbackBridge::ProcessFederationCallbackBridge(
    std::shared_ptr<CallbackDispatcher> dispatcher,
    std::shared_ptr<rti1516_2025::umbra_binding_detail::CallbackSession>
        callbackSession)
    : dispatcher_(std::move(dispatcher)),
      callbackSession_(std::move(callbackSession)) {
  if (!dispatcher_ || !callbackSession_) {
    throw ProcessFederationCallbackBridgeError(
        "A process callback bridge requires the runtime callback controls.");
  }
}

ProcessFederationCallbackBridge::~ProcessFederationCallbackBridge() {
  close();
}

void ProcessFederationCallbackBridge::submitReceiveOrder(
    ProcessFederationInteractionEvent event) {
  auto const session = callbackSession_;
  if (!session || !dispatcher_) {
    throw ProcessFederationCallbackBridgeError(
        "A process callback bridge is closed.");
  }
  std::shared_ptr<RetractionState> retractionState;
  if (event.retractionMessageId) {
    retractionState = std::make_shared<RetractionState>();
    std::scoped_lock lock(retractionMutex_);
    retractionStates_[*event.retractionMessageId] = retractionState;
  }
  dispatcher_->submit(
      [session,
       retractionState = std::move(retractionState),
       event = std::move(event)]() mutable {
        if (retractionState) {
          std::scoped_lock lock(retractionState->mutex);
          if (retractionState->retractionRequested) {
            return;
          }
          retractionState->callbackStarted = true;
        }
        session->invoke(
            [event = std::move(event)](
                rti1516_2025::FederateAmbassador& recipient) mutable {
              deliverReceiveOrder(event, recipient);
            });
      });
}

void ProcessFederationCallbackBridge::submitRequestRetraction(
    std::uint64_t messageId) {
  if (messageId == 0U) {
    throw ProcessFederationCallbackBridgeError(
        "A process Request Retraction callback requires a message identity.");
  }
  auto const session = callbackSession_;
  auto const dispatcher = dispatcher_;
  if (!session || !dispatcher) {
    throw ProcessFederationCallbackBridgeError(
        "A process callback bridge is closed.");
  }

  std::shared_ptr<RetractionState> retractionState;
  bool callbackMayDeliver = true;
  {
    std::scoped_lock lock(retractionMutex_);
    auto const found = retractionStates_.find(messageId);
    if (found != retractionStates_.end()) {
      retractionState = found->second;
    }
  }
  if (retractionState) {
    std::scoped_lock lock(retractionState->mutex);
    if (retractionState->retractionRequested) {
      return;
    }
    retractionState->retractionRequested = true;
    // A queued receive-order callback has not crossed the callback boundary;
    // mark it suppressed and do not expose Request Retraction.  Once the
    // original callback starts, the direct callback is a legal consequence.
    callbackMayDeliver = retractionState->callbackStarted;
    if (callbackMayDeliver) {
      retractionState->retractionCallbackQueued = true;
    }
  } else {
    // Without a tracked timestamped interaction there is no callback that
    // crossed this bridge and therefore no legal recipient-local retraction
    // consequence.  This also suppresses a control frame for a pull-mode
    // event that was retracted before the receiver ever polled it.
    return;
  }
  if (!callbackMayDeliver) {
    return;
  }
  dispatcher->submit(
      [session,
       messageId,
       retractionState = std::move(retractionState)]() mutable {
        session->invoke(
            [messageId](rti1516_2025::FederateAmbassador& recipient) {
              deliverRequestRetraction(messageId, recipient);
            });
      });
}

void ProcessFederationCallbackBridge::submitAttributeUpdate(
    ProcessFederationAttributeUpdateEvent event) {
  auto const session = callbackSession_;
  if (!session || !dispatcher_) {
    throw ProcessFederationCallbackBridgeError(
        "A process callback bridge is closed.");
  }
  dispatcher_->submit(
      [session, event = std::move(event)]() mutable {
        session->invoke(
            [event = std::move(event)](
                rti1516_2025::FederateAmbassador& recipient) mutable {
              deliverAttributeUpdate(event, recipient);
            });
      });
}

void ProcessFederationCallbackBridge::submitObjectInstanceDiscovery(
    ProcessFederationObjectInstanceDiscoveryEvent event) {
  auto const session = callbackSession_;
  if (!session || !dispatcher_) {
    throw ProcessFederationCallbackBridgeError(
        "A process callback bridge is closed.");
  }
  dispatcher_->submit(
      [session, event = std::move(event)]() mutable {
        session->invoke(
            [event = std::move(event)](
                rti1516_2025::FederateAmbassador& recipient) mutable {
              deliverObjectInstanceDiscovery(event, recipient);
            });
      });
}

void ProcessFederationCallbackBridge::submitObjectInstanceRemoval(
    ProcessFederationObjectInstanceRemovalEvent event) {
  auto const session = callbackSession_;
  if (!session || !dispatcher_) {
    throw ProcessFederationCallbackBridgeError(
        "A process callback bridge is closed.");
  }
  std::shared_ptr<RetractionState> retractionState;
  if (event.retractionMessageId) {
    retractionState = std::make_shared<RetractionState>();
    std::scoped_lock lock(retractionMutex_);
    retractionStates_[*event.retractionMessageId] = retractionState;
  }
  dispatcher_->submit(
      [session,
       retractionState = std::move(retractionState),
       event = std::move(event)]() mutable {
        if (retractionState) {
          std::scoped_lock lock(retractionState->mutex);
          if (retractionState->retractionRequested) {
            return;
          }
          retractionState->callbackStarted = true;
        }
        session->invoke(
            [event = std::move(event)](
                rti1516_2025::FederateAmbassador& recipient) mutable {
              deliverObjectInstanceRemoval(event, recipient);
            });
      });
}

void ProcessFederationCallbackBridge::submitObjectInstanceScopeChange(
    ProcessFederationObjectInstanceScopeChangeEvent event) {
  auto const session = callbackSession_;
  if (!session || !dispatcher_) {
    throw ProcessFederationCallbackBridgeError(
        "A process callback bridge is closed.");
  }
  dispatcher_->submit(
      [session, event = std::move(event)]() mutable {
        session->invoke(
            [event = std::move(event)](
                rti1516_2025::FederateAmbassador& recipient) mutable {
              deliverObjectInstanceScopeChange(event, recipient);
            });
      });
}

void ProcessFederationCallbackBridge::submitAttributeRelevanceAdvisory(
    ProcessFederationAttributeRelevanceAdvisoryEvent event) {
  auto const session = callbackSession_;
  if (!session || !dispatcher_) {
    throw ProcessFederationCallbackBridgeError(
        "A process callback bridge is closed.");
  }
  auto const switchState = attributeRelevanceAdvisorySwitchState_;
  dispatcher_->submit(
      [session, switchState, event = std::move(event)]() mutable {
        // A queued HLA_EVOKED callback may outlive the switch-setting
        // service request that caused its admission. Re-evaluate the switch
        // at the callback boundary so disabling the switch cannot leak stale
        // Turn Updates work. A standalone bridge without a client-owned
        // state keeps its historical behavior for private unit tests.
        if (switchState && !switchState->load(std::memory_order_acquire)) {
          return;
        }
        session->invoke(
            [event = std::move(event)](
                rti1516_2025::FederateAmbassador& recipient) mutable {
              deliverAttributeRelevanceAdvisory(event, recipient);
            });
      });
}

void ProcessFederationCallbackBridge::setAttributeRelevanceAdvisorySwitchState(
    std::shared_ptr<std::atomic_bool> enabled) noexcept {
  attributeRelevanceAdvisorySwitchState_ = std::move(enabled);
}

bool ProcessFederationCallbackBridge::evokeOne(
    std::chrono::milliseconds minimumWait) {
  if (!dispatcher_) {
    return false;
  }
  return dispatcher_->evokeOne(minimumWait);
}

bool ProcessFederationCallbackBridge::evokeMultiple(
    std::chrono::milliseconds minimumWait,
    std::chrono::milliseconds maximumWait) {
  if (!dispatcher_) {
    return false;
  }
  return dispatcher_->evokeMultiple(minimumWait, maximumWait);
}

std::size_t ProcessFederationCallbackBridge::pendingCount() const {
  return dispatcher_ ? dispatcher_->pendingCount() : 0U;
}

void ProcessFederationCallbackBridge::close() noexcept {
  {
    std::scoped_lock lock(retractionMutex_);
    retractionStates_.clear();
  }
  if (callbackSession_) {
    callbackSession_->close();
  }
  if (dispatcher_) {
    dispatcher_->reset();
  }
  callbackSession_.reset();
  dispatcher_.reset();
}

}  // namespace umbra::detail
