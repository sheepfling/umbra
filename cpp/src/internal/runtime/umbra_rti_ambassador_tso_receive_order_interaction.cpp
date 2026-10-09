#include "internal/runtime/umbra_rti_ambassador.hpp"

#include "internal/handles/federate_handle.hpp"
#include "internal/handles/interaction_class_handle.hpp"
#include "internal/handles/message_retraction_handle.hpp"
#include "internal/handles/parameter_handle.hpp"
#include "internal/handles/region_handle.hpp"
#include "internal/runtime/ambassador_shared_utilities.hpp"

#include <mutex>
#include <optional>
#include <set>
#include <utility>
#include <vector>

namespace rti1516_2025::umbra_binding_detail {

#if defined(UMBRA_ENABLE_EMBEDDED_FEDERATION_MANAGEMENT)

// A timestamped callback reservation is created before the callback is
// submitted to the recipient's receive-order queue.  Any defensive payload
// failure before the callback can begin must consume that reservation as a
// suppression, otherwise a later Retract can observe a phantom pending
// recipient and retain lifecycle state indefinitely.
void finishAmbassadorSuppressedTsoRecipientCallback(
    std::wstring const& federationName,
    std::uint64_t receivingFederateId,
    std::optional<std::uint64_t> const& retractionMessageId) {
  if (!retractionMessageId) {
    return;
  }
  std::scoped_lock lock(ambassadorFederationManagementMutex());
  static_cast<void>(embeddedFederationRegistry()
                        .finishTsoRecipientCallbackSuppressed(
                            federationName,
                            receivingFederateId,
                            *retractionMessageId));
}

void finishSuppressedTsoRecipientCallbackIfNeeded(
    std::wstring const& federationName,
    std::uint64_t receivingFederateId,
    std::optional<std::uint64_t> const& retractionMessageId) {
  finishAmbassadorSuppressedTsoRecipientCallback(
      federationName,
      receivingFederateId,
      retractionMessageId);
}

void queueAmbassadorTimestampedReceiveOrderInteraction(
    umbra::detail::InteractionCallbackRoute callbackRoute,
    std::wstring federationName,
    umbra::detail::InteractionProducer producingSource,
    std::uint64_t receivingFederateId,
    std::uint64_t sentInteractionClassHandle,
    std::vector<AmbassadorInteractionParameterValue> sentParameters,
    VariableLengthData userSuppliedTag,
    TransportationTypeHandle transportationType,
    std::string transportationName,
    std::shared_ptr<LogicalTime const> timestamp,
    OrderType sentOrderType,
    OrderType receivedOrderType,
    std::optional<std::uint64_t> retractionMessageId,
    std::optional<std::set<std::uint64_t>> sentRegionHandles,
    bool defaultRegionUsed,
    std::optional<std::map<std::uint64_t, umbra::detail::RegionSpecificationSnapshot>>
        regionOverrides) {
  auto const producingFederateId = producingSource.joinedFederateId();
  if (producingSource.kind() != umbra::detail::InteractionProducer::Kind::joined_federate ||
      !producingFederateId || *producingFederateId == 0U) {
    // See the receive-order counterpart above. A timestamped callback also
    // carries a FederateHandle and cannot be the escape hatch for an
    // unresolved RTI-originated producer designator.
    throw RTIinternalError(
        L"The timestamped Receive Interaction callback path requires a joined-federate producer.");
  }
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
      producingFederateId = *producingFederateId,
      receivingFederateId,
      sentInteractionClassHandle,
      sentParameterHandles = std::move(sentParameterHandles),
      sentParameters = std::move(sentParameters),
      userSuppliedTag = std::move(userSuppliedTag),
      transportationType = std::move(transportationType),
      transportationName = std::move(transportationName),
      timestamp = std::move(timestamp),
      sentOrderType,
      receivedOrderType,
      retractionMessageId,
      sentRegionHandles = std::move(sentRegionHandles),
      defaultRegionUsed,
      regionOverrides = std::move(regionOverrides)](FederateAmbassador& recipient) mutable {
     if (!timestamp) {
       finishAmbassadorSuppressedTsoRecipientCallback(
           federationName,
           receivingFederateId,
           retractionMessageId);
       return;
     }
     std::optional<umbra::detail::ReceiveOrderInteractionRecipient> projection;
     bool callbackMayBegin = !retractionMessageId;
     {
       std::scoped_lock lock(ambassadorFederationManagementMutex());
       auto& registry = embeddedFederationRegistry();
       projection = registry.receiveOrderInteractionRecipientFor(
           federationName,
           producingFederateId,
           receivingFederateId,
           sentInteractionClassHandle,
           sentParameterHandles,
           sentRegionHandles ? &*sentRegionHandles : nullptr,
           regionOverrides ? &*regionOverrides : nullptr);
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
     std::optional<RegionHandleSet> optionalSentRegions;
     if (projection->conveyRegionDesignatorSets &&
         (sentRegionHandles || defaultRegionUsed)) {
       optionalSentRegions.emplace();
       if (sentRegionHandles) {
         for (auto const regionHandle : *sentRegionHandles) {
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
         optionalSentRegions ? &*optionalSentRegions : nullptr,
         *timestamp,
         sentOrderType,
         receivedOrderType,
         retraction ? &*retraction : nullptr);
   });
}

#endif

}  // namespace rti1516_2025::umbra_binding_detail
