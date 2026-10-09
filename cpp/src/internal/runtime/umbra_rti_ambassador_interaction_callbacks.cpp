#include "internal/handles/federate_handle.hpp"
#include "internal/handles/interaction_class_handle.hpp"
#include "internal/handles/message_retraction_handle.hpp"
#include "internal/handles/object_instance_handle.hpp"
#include "internal/handles/parameter_handle.hpp"
#include "internal/handles/region_handle.hpp"
#include "internal/handles/transportation_type_handle.hpp"
#include "internal/runtime/ambassador_shared_utilities.hpp"
#include "internal/runtime/umbra_rti_ambassador_service_failure_translation.hpp"

#include <RTI/FederateAmbassador.h>

#include <cstdint>
#include <map>
#include <mutex>
#include <optional>
#include <set>
#include <string>
#include <utility>
#include <vector>

namespace rti1516_2025::umbra_binding_detail {
using namespace service_failure_translation;

#if defined(UMBRA_ENABLE_EMBEDDED_FEDERATION_MANAGEMENT)
void queueAmbassadorReceiveOrderInteraction(
    umbra::detail::InteractionCallbackRoute callbackRoute,
    std::wstring federationName,
    umbra::detail::InteractionProducer producingSource,
    std::uint64_t receivingFederateId,
    std::uint64_t sentInteractionClassHandle,
    std::vector<AmbassadorInteractionParameterValue> sentParameters,
    VariableLengthData userSuppliedTag,
    TransportationTypeHandle transportationType,
    std::string transportationName,
    std::optional<std::set<std::uint64_t>> sentRegionHandles,
    bool defaultRegionUsed,
    std::optional<std::map<std::uint64_t, umbra::detail::RegionSpecificationSnapshot>>
        regionOverrides) {
  auto const producingFederateId = producingSource.joinedFederateId();
  if (producingSource.kind() != umbra::detail::InteractionProducer::Kind::joined_federate ||
      !producingFederateId || *producingFederateId == 0U) {
    // The official callback requires a FederateHandle. Keep RTI-originated
    // MOM traffic out of this ordinary joined-federate delivery path until
    // its producer-designator rule is sourced; do not manufacture an invalid
    // FederateHandle(0) to make the callback type fit.
    throw RTIinternalError(
        L"The ordinary Receive Interaction callback path requires a joined-federate producer.");
  }
  std::vector<std::uint64_t> sentParameterHandles;
  sentParameterHandles.reserve(sentParameters.size());
  for (auto const & [parameterHandle, parameterValue] : sentParameters) {
    static_cast<void>(parameterValue);
    sentParameterHandles.push_back(parameterHandle);
  }

  submitAmbassadorReceiveOrderCallback(
      std::move(callbackRoute),
      federationName,
      receivingFederateId,
      [
      federationName,
      producingFederateId = *producingFederateId,
      receivingFederateId,
      sentInteractionClassHandle,
      sentParameterHandles = std::move(sentParameterHandles),
      sentParameters = std::move(sentParameters),
      userSuppliedTag = std::move(userSuppliedTag),
      transportationType = std::move(transportationType),
      transportationName = std::move(transportationName),
      sentRegionHandles = std::move(sentRegionHandles),
      defaultRegionUsed,
      regionOverrides = std::move(regionOverrides)](FederateAmbassador & recipient) mutable {
    std::optional<umbra::detail::ReceiveOrderInteractionRecipient> projection;
    {
      std::scoped_lock lock(ambassadorFederationManagementMutex());
      projection = embeddedFederationRegistry().receiveOrderInteractionRecipientFor(
          federationName,
          producingFederateId,
          receivingFederateId,
          sentInteractionClassHandle,
          sentParameterHandles,
          sentRegionHandles ? &*sentRegionHandles : nullptr,
          regionOverrides ? &*regionOverrides : nullptr);
    }
    if (!projection) {
      return;
    }

    // The recipient can resign or change its subscription after a Send
    // Interaction is accepted. The registry projection above is therefore
    // deliberately re-evaluated immediately before user callback delivery.
    ParameterHandleValueMap parameterValues = ambassadorProjectInteractionParameterValues(
        sentParameters,
        projection->receivedParameterHandles);
    std::optional<RegionHandleSet> optionalSentRegions;
    if (projection->conveyRegionDesignatorSets &&
        (sentRegionHandles || defaultRegionUsed)) {
      optionalSentRegions.emplace();
      if (sentRegionHandles) {
        for (std::uint64_t const regionHandle : *sentRegionHandles) {
          optionalSentRegions->insert(makeRegionHandle(regionHandle));
        }
      }
    }
    recordAmbassadorSuccessfulInteractionReceipt(
        federationName,
        receivingFederateId,
        sentInteractionClassHandle,
        transportationName,
        false);
    recipient.receiveInteraction(
        makeInteractionClassHandle(projection->receivedInteractionClassHandle),
        parameterValues,
        userSuppliedTag,
        transportationType,
        makeFederateHandle(producingFederateId),
        optionalSentRegions ? &*optionalSentRegions : nullptr);
  });
}

void queueAmbassadorReceiveOrderDirectedInteraction(
    umbra::detail::InteractionCallbackRoute callbackRoute,
    std::wstring federationName,
    std::uint64_t producingFederateId,
    std::uint64_t receivingFederateId,
    std::uint64_t objectInstanceHandle,
    std::uint64_t sentInteractionClassHandle,
    std::vector<AmbassadorInteractionParameterValue> sentParameters,
    VariableLengthData userSuppliedTag,
    TransportationTypeHandle transportationType,
    std::string transportationName) {
  std::vector<std::uint64_t> sentParameterHandles;
  sentParameterHandles.reserve(sentParameters.size());
  for (auto const& [parameterHandle, parameterValue] : sentParameters) {
    static_cast<void>(parameterValue);
    sentParameterHandles.push_back(parameterHandle);
  }

  submitAmbassadorReceiveOrderCallback(
      std::move(callbackRoute),
      federationName,
      receivingFederateId,
      [
      federationName,
      producingFederateId,
      receivingFederateId,
      objectInstanceHandle,
      sentInteractionClassHandle,
      sentParameterHandles = std::move(sentParameterHandles),
      sentParameters = std::move(sentParameters),
      userSuppliedTag = std::move(userSuppliedTag),
      transportationType = std::move(transportationType),
      transportationName = std::move(transportationName)](FederateAmbassador& recipient) mutable {
    std::optional<umbra::detail::ReceiveOrderDirectedInteractionRecipient> projection;
    {
      std::scoped_lock lock(ambassadorFederationManagementMutex());
      projection = embeddedFederationRegistry()
          .receiveOrderDirectedInteractionRecipientFor(
              federationName,
              producingFederateId,
              receivingFederateId,
              objectInstanceHandle,
              sentInteractionClassHandle,
              sentParameterHandles);
    }
    if (!projection) {
      return;
    }

    ParameterHandleValueMap parameterValues = ambassadorProjectInteractionParameterValues(
        sentParameters,
        projection->receivedParameterHandles);
    recordAmbassadorSuccessfulInteractionReceipt(
        federationName,
        receivingFederateId,
        sentInteractionClassHandle,
        transportationName,
        true);
    recipient.receiveDirectedInteraction(
        makeInteractionClassHandle(projection->receivedInteractionClassHandle),
        makeObjectInstanceHandle(projection->objectInstanceHandle),
        parameterValues,
        userSuppliedTag,
        transportationType,
        makeFederateHandle(producingFederateId));
  });
}

void queueAmbassadorTimestampedReceiveOrderDirectedInteraction(
    umbra::detail::InteractionCallbackRoute callbackRoute,
    std::wstring federationName,
    std::uint64_t producingFederateId,
    std::uint64_t receivingFederateId,
    std::uint64_t objectInstanceHandle,
    std::uint64_t sentInteractionClassHandle,
    std::vector<AmbassadorInteractionParameterValue> sentParameters,
    VariableLengthData userSuppliedTag,
    TransportationTypeHandle transportationType,
    std::string transportationName,
    std::shared_ptr<LogicalTime const> timestamp,
    OrderType sentOrderType,
    OrderType receivedOrderType,
    std::optional<std::uint64_t> retractionMessageId) {
  std::vector<std::uint64_t> sentParameterHandles;
  sentParameterHandles.reserve(sentParameters.size());
  for (auto const& [parameterHandle, parameterValue] : sentParameters) {
    static_cast<void>(parameterValue);
    sentParameterHandles.push_back(parameterHandle);
  }

  submitAmbassadorReceiveOrderCallback(
      std::move(callbackRoute),
      federationName,
      receivingFederateId,
      [
      federationName,
      producingFederateId,
      receivingFederateId,
      objectInstanceHandle,
      sentInteractionClassHandle,
      sentParameterHandles = std::move(sentParameterHandles),
      sentParameters = std::move(sentParameters),
      userSuppliedTag = std::move(userSuppliedTag),
      transportationType = std::move(transportationType),
      transportationName = std::move(transportationName),
      timestamp = std::move(timestamp),
      sentOrderType,
      receivedOrderType,
      retractionMessageId](FederateAmbassador& recipient) mutable {
    if (!timestamp) {
      finishAmbassadorSuppressedTsoRecipientCallback(
          federationName,
          receivingFederateId,
          retractionMessageId);
      return;
    }
    std::optional<umbra::detail::ReceiveOrderDirectedInteractionRecipient> projection;
    bool callbackMayBegin = !retractionMessageId;
    {
      std::scoped_lock lock(ambassadorFederationManagementMutex());
      auto& registry = embeddedFederationRegistry();
      // A timestamped directed interaction has an execution-wide recipient
      // ledger once the sender is regulating.  That ledger is deliberately
      // allowed to outlive a voluntary source resignation, so an immediate
      // (non-time-constrained) recipient must use the same revalidated
      // projection as a queued TSO recipient.  The ordinary receive-order
      // predicate requires the producer to remain joined and would silently
      // drop an already accepted callback at this lifetime boundary.
      if (retractionMessageId) {
        projection = registry.timestampedDirectedInteractionRecipientFor(
            federationName,
            producingFederateId,
            receivingFederateId,
            objectInstanceHandle,
            sentInteractionClassHandle,
            sentParameterHandles,
            *retractionMessageId);
      } else {
        projection = registry.receiveOrderDirectedInteractionRecipientFor(
            federationName,
            producingFederateId,
            receivingFederateId,
            objectInstanceHandle,
            sentInteractionClassHandle,
            sentParameterHandles);
      }
      if (projection && retractionMessageId) {
        callbackMayBegin = registry.beginTsoInteractionCallback(
            federationName,
            receivingFederateId,
            *retractionMessageId);
      }
    }
    if (!projection || !callbackMayBegin) {
      if (retractionMessageId) {
        std::scoped_lock lock(ambassadorFederationManagementMutex());
        static_cast<void>(embeddedFederationRegistry()
                              .finishTsoRecipientCallbackSuppressed(
                                  federationName,
                                  receivingFederateId,
                                  *retractionMessageId));
      }
      return;
    }

    ParameterHandleValueMap parameterValues = ambassadorProjectInteractionParameterValues(
        sentParameters,
        projection->receivedParameterHandles);
    std::optional<MessageRetractionHandle> retraction;
    if (retractionMessageId) {
      retraction.emplace(makeMessageRetractionHandle(*retractionMessageId));
    }
    recordAmbassadorSuccessfulInteractionReceipt(
        federationName,
        receivingFederateId,
        sentInteractionClassHandle,
        transportationName,
        true);
    recipient.receiveDirectedInteraction(
        makeInteractionClassHandle(projection->receivedInteractionClassHandle),
        makeObjectInstanceHandle(projection->objectInstanceHandle),
        parameterValues,
        userSuppliedTag,
        transportationType,
        makeFederateHandle(producingFederateId),
        *timestamp,
        sentOrderType,
        receivedOrderType,
        retraction ? &*retraction : nullptr);
  });
}
#endif
}  // namespace rti1516_2025::umbra_binding_detail
