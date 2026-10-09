#include "internal/federation/process_federation_service_protocol.hpp"
#include "internal/federation/process_federation_service_helpers.hpp"

#include "internal/encoding/byte_order.hpp"
#include "internal/encoding/variable_length_data_2025.hpp"
#include "internal/fom/hla_names.hpp"
#include "internal/handles/dimension_handle.hpp"
#include "internal/handles/federate_handle.hpp"
#include "internal/handles/interaction_class_handle.hpp"
#include "internal/handles/object_class_handle.hpp"
#include "internal/handles/region_handle.hpp"
#include "internal/handles/transportation_type_handle.hpp"
#include "internal/observability/mom_service_report_encoding.hpp"
#include "internal/runtime/utf8_string.hpp"
#include "internal/time/federation_time_bounds.hpp"
#include "internal/time/federation_time_grant_policy.hpp"

#include <RTI/Exception.h>
#include <RTI/VariableLengthData.h>
#include <RTI/encoding/BasicDataElements.h>
#include <RTI/time/HLAlogicalTimeFactoryFactory.h>

#include <algorithm>
#include <array>
#include <atomic>
#include <bit>
#include <cmath>
#include <limits>
#include <memory>
#include <iterator>
#include <set>
#include <utility>

#include "internal/federation/process_federation_service_payload_helpers.hpp"
#include "internal/federation/process_federation_service_codec_support.hpp"

namespace umbra::detail {
using process_federation_payload::parameterVector;
using namespace process_federation_service_codec_support;
namespace {
constexpr std::array<std::uint8_t, 4U> kInteractionEnvelopeMagic{
    0x55U, 0x31U, 0x35U, 0x49U};
constexpr std::uint8_t kInteractionEnvelopeVersion = 1U;
}  // namespace

std::vector<std::uint8_t> encodeProcessFederationInteractionEnvelope(
    ProcessFederationInteractionEnvelope const& envelope) {
  if (envelope.parameterValues.size() >
      std::numeric_limits<std::uint32_t>::max()) {
    throw ProcessFederationServiceProtocolError(
        "A process federation interaction envelope has too many parameters.");
  }
  PayloadWriter writer;
  writer.raw(kInteractionEnvelopeMagic);
  writer.unsigned8(kInteractionEnvelopeVersion);
  writer.unsigned32(
      static_cast<std::uint32_t>(envelope.parameterValues.size()));
  std::set<std::uint64_t> seenHandles;
  for (auto const& [parameterHandle, parameterValue] :
       envelope.parameterValues) {
    requireNonzero(
        parameterHandle,
        "A process federation interaction envelope requires parameter identities.");
    if (!seenHandles.insert(parameterHandle).second) {
      throw ProcessFederationServiceProtocolError(
          "A process federation interaction envelope repeats a parameter identity.");
    }
    writer.unsigned64(parameterHandle);
    writer.bytes(parameterValue);
  }
  writer.bytes(envelope.userSuppliedTag);
  return std::move(writer).finish();
}

std::optional<ProcessFederationInteractionEnvelope>
decodeProcessFederationInteractionEnvelope(
    std::span<std::uint8_t const> encoded) {
  if (encoded.size() < kInteractionEnvelopeMagic.size() + 1U) {
    return std::nullopt;
  }
  for (std::size_t index = 0U; index < kInteractionEnvelopeMagic.size();
       ++index) {
    if (encoded[index] != kInteractionEnvelopeMagic[index]) {
      return std::nullopt;
    }
  }

  PayloadReader reader(encoded.subspan(kInteractionEnvelopeMagic.size()));
  if (reader.unsigned8() != kInteractionEnvelopeVersion) {
    throw ProcessFederationServiceProtocolError(
        "A process federation interaction envelope has an unsupported version.");
  }
  auto const parameterCount = reader.unsigned32();
  ProcessFederationInteractionEnvelope result;
  result.parameterValues.reserve(parameterCount);
  std::set<std::uint64_t> seenHandles;
  for (std::uint32_t index = 0U; index < parameterCount; ++index) {
    auto const parameterHandle = reader.unsigned64();
    requireNonzero(
        parameterHandle,
        "A process federation interaction envelope contains an invalid parameter identity.");
    if (!seenHandles.insert(parameterHandle).second) {
      throw ProcessFederationServiceProtocolError(
          "A process federation interaction envelope repeats a parameter identity.");
    }
    result.parameterValues.emplace_back(parameterHandle, reader.bytes());
  }
  result.userSuppliedTag = reader.bytes();
  reader.finish();
  return result;
}

std::vector<std::uint8_t> encodeProcessFederationReceiveInteractionResult(
    ProcessFederationReceiveInteractionResult const& result) {
  auto const eventCount = static_cast<unsigned>(result.event.has_value()) +
      static_cast<unsigned>(result.attributeEvent.has_value()) +
      static_cast<unsigned>(result.discoveryEvent.has_value()) +
      static_cast<unsigned>(result.scopeChangeEvent.has_value()) +
      static_cast<unsigned>(result.removalEvent.has_value()) +
      static_cast<unsigned>(result.attributeRelevanceAdvisoryEvent.has_value()) +
      static_cast<unsigned>(result.attributeValueUpdateRequestEvent.has_value()) +
      static_cast<unsigned>(result.attributeOwnershipQueryEvent.has_value()) +
      static_cast<unsigned>(
          result.attributeOwnershipAcquisitionIfAvailableEvent.has_value());
  const auto eventCountWithRegularAcquisition = eventCount + static_cast<unsigned>(
      result.attributeOwnershipAcquisitionEvent.has_value());
  const auto eventCountWithOwnershipUnavailable =
      eventCountWithRegularAcquisition + static_cast<unsigned>(
          result.attributeOwnershipUnavailableEvent.has_value());
  const auto eventCountWithTransportation =
      eventCountWithOwnershipUnavailable + static_cast<unsigned>(
          result.attributeTransportationTypeChangeEvent.has_value()) +
      static_cast<unsigned>(result.attributeTransportationTypeQueryEvent.has_value());
  const auto eventCountWithInteractionTransportation =
      eventCountWithTransportation + static_cast<unsigned>(
          result.interactionTransportationTypeChangeEvent.has_value()) +
      static_cast<unsigned>(result.interactionTransportationTypeQueryEvent.has_value());
  const auto eventCountWithSynchronization =
      eventCountWithInteractionTransportation +
      static_cast<unsigned>(result.synchronizationPointAnnouncementEvent.has_value()) +
      static_cast<unsigned>(result.federationSynchronizedEvent.has_value());
  const auto eventCountWithSave = eventCountWithSynchronization +
      static_cast<unsigned>(result.saveEvent.has_value());
  const auto eventCountWithRestore = eventCountWithSave +
      static_cast<unsigned>(result.restoreEvent.has_value());
  if (eventCountWithRestore > 1U) {
    throw ProcessFederationServiceProtocolError(
        "A process federation receive result cannot contain multiple events.");
  }
  PayloadWriter writer;
  unsigned eventKind = 0U;
  if (result.event) {
    eventKind = 1U;
  } else if (result.attributeEvent) {
    eventKind = 2U;
  } else if (result.discoveryEvent) {
    eventKind = 3U;
  } else if (result.scopeChangeEvent) {
    eventKind = 4U;
  } else if (result.attributeRelevanceAdvisoryEvent) {
    eventKind = 5U;
  } else if (result.removalEvent) {
    eventKind = 6U;
  } else if (result.attributeValueUpdateRequestEvent) {
    eventKind = 7U;
  } else if (result.attributeOwnershipQueryEvent) {
    eventKind = 8U;
  } else if (result.attributeOwnershipAcquisitionIfAvailableEvent) {
    eventKind = 9U;
  } else if (result.attributeOwnershipAcquisitionEvent) {
    eventKind = 10U;
  } else if (result.attributeOwnershipUnavailableEvent) {
    eventKind = 11U;
  } else if (result.attributeTransportationTypeChangeEvent) {
    eventKind = 12U;
  } else if (result.attributeTransportationTypeQueryEvent) {
    eventKind = 13U;
  } else if (result.interactionTransportationTypeChangeEvent) {
    eventKind = 14U;
  } else if (result.interactionTransportationTypeQueryEvent) {
    eventKind = 15U;
  } else if (result.synchronizationPointAnnouncementEvent) {
    eventKind = 16U;
  } else if (result.federationSynchronizedEvent) {
    eventKind = 17U;
  } else if (result.saveEvent) {
    eventKind = 18U;
  } else if (result.restoreEvent) {
    eventKind = 19U;
  }
  writer.unsigned8(eventKind);
  if (result.event.has_value()) {
    auto const& event = *result.event;
    if (!event.rtiOwnedMomInteraction) {
      requireNonzero(
          event.producingFederateId,
          "A process federation interaction event requires a producer identity.");
    }
    requireNonzero(
        event.receivingFederateId,
        "A process federation interaction event requires a recipient identity.");
    requireNonzero(
        event.interactionClassHandle,
        "A process federation interaction event requires an interaction class.");
    writer.unsigned64(event.producingFederateId);
    writer.unsigned64(event.receivingFederateId);
    writer.unsigned64(event.interactionClassHandle);
    writer.unsigned64Vector(event.parameterHandles);
    writer.bytes(event.payload);
    writer.string(event.transportationName);
    writeOptionalLogicalTime(writer, event.timestamp);
    writer.unsigned8(event.objectInstanceHandle.has_value() ? 1U : 0U);
    if (event.objectInstanceHandle) {
      requireNonzero(
          *event.objectInstanceHandle,
          "A process directed-interaction event requires a target object instance.");
      writer.unsigned64(*event.objectInstanceHandle);
    }
    writer.unsigned8(event.retractionMessageId.has_value() ? 1U : 0U);
    if (event.retractionMessageId) {
      requireNonzero(
          *event.retractionMessageId,
          "A process directed-interaction event requires a retraction identity.");
      writer.unsigned64(*event.retractionMessageId);
    }
    writeInteractionRegionMetadata(writer, event);
    writeInteractionOrderMetadata(writer, event);
    if (event.rtiOwnedMomInteraction) {
      writer.unsigned8(kRtiOwnedMomInteractionEventMarker);
      if (event.exceptionReportFederateId) {
        requireNonzero(*event.exceptionReportFederateId,
                       "An exception report event requires its reported member.");
        writer.unsigned64(*event.exceptionReportFederateId);
      }
    } else if (event.exceptionReportFederateId) {
      throw ProcessFederationServiceProtocolError(
          "Exception report metadata requires an RTI-owned MOM event.");
    }
  } else if (result.attributeEvent.has_value()) {
    auto const& event = *result.attributeEvent;
    if (!event.rtiOwnedMomObject) {
      requireNonzero(
          event.producingFederateId,
          "A process federation attribute event requires a producer identity.");
    }
    requireNonzero(
        event.receivingFederateId,
        "A process federation attribute event requires a recipient identity.");
    requireNonzero(
        event.objectInstanceHandle,
        "A process federation attribute event requires an object instance.");
    if (event.attributeValues.empty() || event.transportationName.empty()) {
      throw ProcessFederationServiceProtocolError(
          "A process federation attribute event requires values and transportation.");
    }
    if (event.attributeValues.size() >
        std::numeric_limits<std::uint32_t>::max()) {
      throw ProcessFederationServiceProtocolError(
          "A process federation attribute event has too many attributes.");
    }
    writer.unsigned64(event.producingFederateId);
    writer.unsigned64(event.receivingFederateId);
    writer.unsigned64(event.objectInstanceHandle);
    writer.unsigned32(static_cast<std::uint32_t>(event.attributeValues.size()));
    std::set<std::uint64_t> seenHandles;
    for (auto const& [attributeHandle, value] : event.attributeValues) {
      requireNonzero(
          attributeHandle,
          "A process federation attribute event requires attribute identities.");
      if (!seenHandles.insert(attributeHandle).second) {
        throw ProcessFederationServiceProtocolError(
            "A process federation attribute event repeats an attribute identity.");
      }
      writer.unsigned64(attributeHandle);
      writer.bytes(value);
    }
    writer.bytes(event.userSuppliedTag);
    writer.string(event.transportationName);
    writeAttributeUpdateRegionMetadata(writer, event);
    writeOptionalLogicalTime(writer, event.timestamp);
    if (event.rtiOwnedMomObject) {
      writer.unsigned8(kRtiOwnedMomAttributeEventMarker);
    }
    if (event.retractionMessageId) {
      requireNonzero(
          *event.retractionMessageId,
          "A process attribute event requires a retraction identity.");
      writer.unsigned64(*event.retractionMessageId);
    }
  } else if (result.discoveryEvent.has_value()) {
    auto const& event = *result.discoveryEvent;
    requireNonzero(
        event.receivingFederateId,
        "A process federation discovery event requires a recipient identity.");
    requireNonzero(
        event.objectInstanceHandle,
        "A process federation discovery event requires an object instance.");
    requireNonzero(
        event.objectClassHandle,
        "A process federation discovery event requires an object class.");
    if (!event.rtiOwnedMomObject) {
      requireNonzero(
          event.producingFederateId,
          "A process federation discovery event requires a producer identity.");
    }
    if (event.objectInstanceName.empty()) {
      throw ProcessFederationServiceProtocolError(
          "A process federation discovery event requires an object instance name.");
    }
    writer.unsigned64(event.receivingFederateId);
    writer.unsigned64(event.objectInstanceHandle);
    writer.unsigned64(event.objectClassHandle);
    writer.wideString(event.objectInstanceName);
    writer.unsigned64(event.producingFederateId);
    if (event.rtiOwnedMomObject) {
      writer.unsigned8(kRtiOwnedMomDiscoveryEventMarker);
    }
  } else if (result.scopeChangeEvent.has_value()) {
    auto const& event = *result.scopeChangeEvent;
    requireNonzero(
        event.receivingFederateId,
        "A process scope event requires a recipient identity.");
    requireNonzero(
        event.objectInstanceHandle,
        "A process scope event requires an object instance.");
    if (event.attributeHandles.empty()) {
      throw ProcessFederationServiceProtocolError(
          "A process scope event requires attributes.");
    }
    if (event.attributeHandles.size() >
        std::numeric_limits<std::uint32_t>::max()) {
      throw ProcessFederationServiceProtocolError(
          "A process scope event has too many attributes.");
    }
    writer.unsigned64(event.receivingFederateId);
    writer.unsigned64(event.objectInstanceHandle);
    writer.unsigned32(static_cast<std::uint32_t>(event.attributeHandles.size()));
    for (std::uint64_t const attributeHandle : event.attributeHandles) {
      requireNonzero(
          attributeHandle,
          "A process scope event requires attribute identities.");
      writer.unsigned64(attributeHandle);
    }
    writer.unsigned8(event.inScope ? 1U : 0U);
  } else if (result.removalEvent.has_value()) {
    auto const& event = *result.removalEvent;
    requireNonzero(
        event.receivingFederateId,
        "A process object removal event requires a recipient identity.");
    requireNonzero(
        event.objectInstanceHandle,
        "A process object removal event requires an object instance.");
    if (event.rtiOwnedMomObject) {
      if (event.producingFederateId != 0U || event.timestamp ||
          event.retractionMessageId) {
        throw ProcessFederationServiceProtocolError(
            "An RTI-owned MOM removal event cannot carry a producer or timestamped deletion metadata.");
      }
    } else {
      requireNonzero(
          event.producingFederateId,
          "A process object removal event requires a producer identity.");
    }
    writer.unsigned64(event.receivingFederateId);
    writer.unsigned64(event.objectInstanceHandle);
    writer.unsigned64(event.producingFederateId);
    writer.bytes(event.userSuppliedTag);
    writeOptionalLogicalTime(writer, event.timestamp);
    writeOptionalMessageId(
        writer,
        event.retractionMessageId.value_or(0U));
    // The private message identity remains present for timestamped queue
    // bookkeeping even when the public API must not expose a retraction
    // designator.  Append the projection bit so older payloads (which always
    // exposed the id) remain decodable by defaulting to true when absent.
    if (event.retractionMessageId) {
      writer.unsigned8(event.provideRetraction ? 1U : 0U);
    }
    if (event.timestamp) {
      if (!event.sentOrderType || !event.receivedOrderType) {
        throw ProcessFederationServiceProtocolError(
            "A timestamped process object removal event must carry both order classifications when present.");
      }
      auto const validOrder = [](rti1516_2025::OrderType order) {
        return order == rti1516_2025::RECEIVE ||
            order == rti1516_2025::TIMESTAMP;
      };
      if (!validOrder(*event.sentOrderType) ||
          !validOrder(*event.receivedOrderType)) {
        throw ProcessFederationServiceProtocolError(
            "A timestamped process object removal event has an invalid order classification.");
      }
      writer.unsigned8(1U);
      writer.unsigned8(static_cast<std::uint8_t>(*event.sentOrderType));
      writer.unsigned8(static_cast<std::uint8_t>(*event.receivedOrderType));
    }
    if (event.rtiOwnedMomObject) {
      writer.unsigned8(kRtiOwnedMomRemovalEventMarker);
    }
  } else if (result.attributeRelevanceAdvisoryEvent.has_value()) {
    auto const& event = *result.attributeRelevanceAdvisoryEvent;
    requireNonzero(
        event.providingFederateId,
        "A process attribute relevance advisory requires an owning federate.");
    requireNonzero(
        event.objectInstanceHandle,
        "A process attribute relevance advisory requires an object instance.");
    if (event.attributeHandles.empty()) {
      throw ProcessFederationServiceProtocolError(
          "A process attribute relevance advisory requires attributes.");
    }
    if (event.attributeHandles.size() >
        std::numeric_limits<std::uint32_t>::max()) {
      throw ProcessFederationServiceProtocolError(
          "A process attribute relevance advisory has too many attributes.");
    }
    if (event.updateRateDesignator && event.updateRateDesignator->empty()) {
      throw ProcessFederationServiceProtocolError(
          "A process attribute relevance advisory cannot carry an empty update-rate designator.");
    }
    writer.unsigned64(event.providingFederateId);
    writer.unsigned64(event.receivingFederateId);
    writer.unsigned64(event.objectInstanceHandle);
    writer.unsigned32(static_cast<std::uint32_t>(event.attributeHandles.size()));
    for (std::uint64_t const attributeHandle : event.attributeHandles) {
      requireNonzero(
          attributeHandle,
          "A process attribute relevance advisory requires attribute identities.");
      writer.unsigned64(attributeHandle);
    }
    writer.unsigned8(event.turnUpdatesOn ? 1U : 0U);
    writer.unsigned8(event.updateRateDesignator.has_value() ? 1U : 0U);
    if (event.updateRateDesignator) {
      writer.string(*event.updateRateDesignator);
    }
  } else if (result.attributeValueUpdateRequestEvent.has_value()) {
    auto const& event = *result.attributeValueUpdateRequestEvent;
    requireNonzero(
        event.requestingFederateId,
        "A process attribute-value-update request event requires a requester.");
    requireNonzero(
        event.providingFederateId,
        "A process attribute-value-update request event requires a provider.");
    requireNonzero(
        event.objectInstanceHandle,
        "A process attribute-value-update request event requires an object instance.");
    if (event.requestedAttributeHandles.empty()) {
      throw ProcessFederationServiceProtocolError(
          "A process attribute-value-update request event requires attributes.");
    }
    if (event.requestedAttributeHandles.size() >
        std::numeric_limits<std::uint32_t>::max()) {
      throw ProcessFederationServiceProtocolError(
          "A process attribute-value-update request event has too many attributes.");
    }
    writer.unsigned64(event.requestingFederateId);
    writer.unsigned64(event.providingFederateId);
    writer.unsigned64(event.objectInstanceHandle);
    writer.unsigned32(static_cast<std::uint32_t>(
        event.requestedAttributeHandles.size()));
    for (std::uint64_t const attributeHandle :
         event.requestedAttributeHandles) {
      requireNonzero(
          attributeHandle,
          "A process attribute-value-update request event requires attribute identities.");
      writer.unsigned64(attributeHandle);
    }
    writer.bytes(event.userSuppliedTag);
  } else if (result.attributeOwnershipQueryEvent.has_value()) {
    auto const& event = *result.attributeOwnershipQueryEvent;
    requireNonzero(
        event.requestId,
        "A process attribute-ownership query event requires a request identity.");
    requireNonzero(
        event.receivingFederateId,
        "A process attribute-ownership query event requires a recipient identity.");
    requireNonzero(
        event.objectInstanceHandle,
        "A process attribute-ownership query event requires an object instance.");
    if (event.attributeHandles.empty() ||
        event.attributeHandles.size() >
            std::numeric_limits<std::uint32_t>::max()) {
      throw ProcessFederationServiceProtocolError(
          "A process attribute-ownership query event requires attributes.");
    }
    if (event.reportKind == AttributeOwnershipQueryReportKind::federate) {
      requireNonzero(
          event.owningFederateId,
          "A federate-owned process attribute-ownership query event requires an owner.");
    } else if (event.owningFederateId != 0U) {
      throw ProcessFederationServiceProtocolError(
          "A non-federate process attribute-ownership query event cannot carry an owner.");
    }
    writer.unsigned64(event.requestId);
    writer.unsigned64(event.receivingFederateId);
    writer.unsigned64(event.objectInstanceHandle);
    writer.unsigned8(static_cast<std::uint8_t>(event.reportKind));
    writer.unsigned64(event.owningFederateId);
    writer.unsigned32(static_cast<std::uint32_t>(event.attributeHandles.size()));
    for (std::uint64_t const attributeHandle : event.attributeHandles) {
      requireNonzero(
          attributeHandle,
          "A process attribute-ownership query event requires attribute identities.");
      writer.unsigned64(attributeHandle);
    }
  } else if (result.attributeOwnershipAcquisitionIfAvailableEvent.has_value()) {
    auto const& event = *result.attributeOwnershipAcquisitionIfAvailableEvent;
    requireNonzero(
        event.requestId,
        "A process attribute-ownership acquisition-if-available event requires a request identity.");
    requireNonzero(
        event.receivingFederateId,
        "A process attribute-ownership acquisition-if-available event requires a recipient identity.");
    requireNonzero(
        event.objectInstanceHandle,
        "A process attribute-ownership acquisition-if-available event requires an object instance.");
    if (event.securedAttributeHandles.empty() &&
        event.unavailableAttributeHandles.empty()) {
      throw ProcessFederationServiceProtocolError(
          "A process attribute-ownership acquisition-if-available event requires a delivery.");
    }
    if (event.securedAttributeHandles.size() >
            std::numeric_limits<std::uint32_t>::max() ||
        event.unavailableAttributeHandles.size() >
            std::numeric_limits<std::uint32_t>::max()) {
      throw ProcessFederationServiceProtocolError(
          "A process attribute-ownership acquisition-if-available event has too many attributes.");
    }
    std::set<std::uint64_t> seenHandles;
    writer.unsigned64(event.requestId);
    writer.unsigned64(event.receivingFederateId);
    writer.unsigned64(event.objectInstanceHandle);
    writer.unsigned32(static_cast<std::uint32_t>(
        event.securedAttributeHandles.size()));
    for (std::uint64_t const attributeHandle : event.securedAttributeHandles) {
      requireNonzero(
          attributeHandle,
          "A process attribute-ownership acquisition-if-available event requires attribute identities.");
      seenHandles.insert(attributeHandle);
      writer.unsigned64(attributeHandle);
    }
    writer.unsigned32(static_cast<std::uint32_t>(
        event.unavailableAttributeHandles.size()));
    for (std::uint64_t const attributeHandle : event.unavailableAttributeHandles) {
      requireNonzero(
          attributeHandle,
          "A process attribute-ownership acquisition-if-available event requires attribute identities.");
      if (!seenHandles.insert(attributeHandle).second) {
        throw ProcessFederationServiceProtocolError(
            "A process attribute-ownership acquisition-if-available event repeats an attribute identity.");
      }
      writer.unsigned64(attributeHandle);
    }
    writer.bytes(event.userSuppliedTag);
  } else if (result.attributeOwnershipAcquisitionEvent.has_value()) {
    auto const& event = *result.attributeOwnershipAcquisitionEvent;
    if (event.kind != ProcessFederationAttributeOwnershipAcquisitionEventKind::
                    ownership_assumption) {
      requireNonzero(
          event.requestId,
          "A process attribute-ownership acquisition event requires a request identity.");
    }
    requireNonzero(
        event.requestingFederateId,
        "A process attribute-ownership acquisition event requires a requester identity.");
    requireNonzero(
        event.receivingFederateId,
        "A process attribute-ownership acquisition event requires a recipient identity.");
    requireNonzero(
        event.objectInstanceHandle,
        "A process attribute-ownership acquisition event requires an object instance.");
    if (event.attributeHandles.empty() ||
        event.attributeHandles.size() > std::numeric_limits<std::uint32_t>::max()) {
      throw ProcessFederationServiceProtocolError(
          "A process attribute-ownership acquisition event requires attributes.");
    }
    auto const encodedKind = static_cast<std::uint8_t>(event.kind);
    if (encodedKind > static_cast<std::uint8_t>(
                          ProcessFederationAttributeOwnershipAcquisitionEventKind::
                              confirm_divestiture_notification)) {
      throw ProcessFederationServiceProtocolError(
          "A process attribute-ownership acquisition event has an invalid kind.");
    }
    writer.unsigned8(encodedKind);
    writer.unsigned64(event.requestId);
    writer.unsigned64(event.requestingFederateId);
    writer.unsigned64(event.receivingFederateId);
    writer.unsigned64(event.objectInstanceHandle);
    writer.unsigned32(static_cast<std::uint32_t>(event.attributeHandles.size()));
    for (std::uint64_t const attributeHandle : event.attributeHandles) {
      requireNonzero(
          attributeHandle,
          "A process attribute-ownership acquisition event requires attribute identities.");
      writer.unsigned64(attributeHandle);
    }
    writer.bytes(event.userSuppliedTag);
    if (event.kind == ProcessFederationAttributeOwnershipAcquisitionEventKind::
                      request_divestiture_confirmation) {
      writer.unsigned8(event.candidateIsIfAvailable ? 1U : 0U);
    }
  } else if (result.attributeOwnershipUnavailableEvent.has_value()) {
    auto const& event = *result.attributeOwnershipUnavailableEvent;
    requireNonzero(
        event.receivingFederateId,
        "A process attribute-ownership unavailable event requires a recipient identity.");
    requireNonzero(
        event.objectInstanceHandle,
        "A process attribute-ownership unavailable event requires an object instance.");
    if (event.attributeHandles.empty() ||
        event.attributeHandles.size() > std::numeric_limits<std::uint32_t>::max()) {
      throw ProcessFederationServiceProtocolError(
          "A process attribute-ownership unavailable event requires attributes.");
    }
    writer.unsigned64(event.receivingFederateId);
    writer.unsigned64(event.objectInstanceHandle);
    writer.unsigned32(static_cast<std::uint32_t>(event.attributeHandles.size()));
    for (std::uint64_t const attributeHandle : event.attributeHandles) {
      requireNonzero(
          attributeHandle,
          "A process attribute-ownership unavailable event requires attribute identities.");
      writer.unsigned64(attributeHandle);
    }
    writer.bytes(event.userSuppliedTag);
  } else if (result.attributeTransportationTypeChangeEvent.has_value()) {
    auto const& event = *result.attributeTransportationTypeChangeEvent;
    requireNonzero(
        event.requestId,
        "A process attribute transportation-type change event requires a request identity.");
    requireNonzero(
        event.receivingFederateId,
        "A process attribute transportation-type change event requires a recipient identity.");
    requireNonzero(
        event.objectInstanceHandle,
        "A process attribute transportation-type change event requires an object instance.");
    if (event.attributeHandles.empty() || event.transportationName.empty() ||
        event.attributeHandles.size() > std::numeric_limits<std::uint32_t>::max()) {
      throw ProcessFederationServiceProtocolError(
          "A process attribute transportation-type change event requires attributes and transportation.");
    }
    writer.unsigned64(event.requestId);
    writer.unsigned64(event.receivingFederateId);
    writer.unsigned64(event.objectInstanceHandle);
    writer.unsigned32(static_cast<std::uint32_t>(event.attributeHandles.size()));
    for (std::uint64_t const attributeHandle : event.attributeHandles) {
      requireNonzero(
          attributeHandle,
          "A process attribute transportation-type change event requires attribute identities.");
      writer.unsigned64(attributeHandle);
    }
    writer.string(event.transportationName);
  } else if (result.attributeTransportationTypeQueryEvent.has_value()) {
    auto const& event = *result.attributeTransportationTypeQueryEvent;
    requireNonzero(
        event.receivingFederateId,
        "A process attribute transportation-type query event requires a recipient identity.");
    requireNonzero(
        event.objectInstanceHandle,
        "A process attribute transportation-type query event requires an object instance.");
    requireNonzero(
        event.attributeHandle,
        "A process attribute transportation-type query event requires an attribute identity.");
    if (event.transportationName.empty()) {
      throw ProcessFederationServiceProtocolError(
          "A process attribute transportation-type query event requires transportation.");
    }
    writer.unsigned64(event.receivingFederateId);
    writer.unsigned64(event.objectInstanceHandle);
    writer.unsigned64(event.attributeHandle);
    writer.string(event.transportationName);
  } else if (result.interactionTransportationTypeChangeEvent.has_value()) {
    auto const& event = *result.interactionTransportationTypeChangeEvent;
    requireNonzero(
        event.receivingFederateId,
        "A process interaction transportation-type change event requires a recipient identity.");
    requireNonzero(
        event.interactionClassHandle,
        "A process interaction transportation-type change event requires an interaction class.");
    if (event.transportationName.empty()) {
      throw ProcessFederationServiceProtocolError(
          "A process interaction transportation-type change event requires transportation.");
    }
    writer.unsigned64(event.receivingFederateId);
    writer.unsigned64(event.interactionClassHandle);
    writer.string(event.transportationName);
  } else if (result.interactionTransportationTypeQueryEvent.has_value()) {
    auto const& event = *result.interactionTransportationTypeQueryEvent;
    requireNonzero(
        event.receivingFederateId,
        "A process interaction transportation-type query event requires a recipient identity.");
    requireNonzero(
        event.queriedFederateId,
        "A process interaction transportation-type query event requires a queried federate identity.");
    requireNonzero(
        event.interactionClassHandle,
        "A process interaction transportation-type query event requires an interaction class.");
    if (event.transportationName.empty()) {
      throw ProcessFederationServiceProtocolError(
          "A process interaction transportation-type query event requires transportation.");
    }
    writer.unsigned64(event.receivingFederateId);
    writer.unsigned64(event.queriedFederateId);
    writer.unsigned64(event.interactionClassHandle);
    writer.string(event.transportationName);
  } else if (result.synchronizationPointAnnouncementEvent.has_value()) {
    auto const& event = *result.synchronizationPointAnnouncementEvent;
    requireNonzero(
        event.receivingFederateId,
        "A process synchronization-point announcement requires a recipient identity.");
    if (event.label.empty()) {
      throw ProcessFederationServiceProtocolError(
          "A process synchronization-point announcement requires a label.");
    }
    writer.unsigned64(event.receivingFederateId);
    writer.wideString(event.label);
    writer.bytes(event.userSuppliedTag);
  } else if (result.federationSynchronizedEvent.has_value()) {
    auto const& event = *result.federationSynchronizedEvent;
    requireNonzero(
        event.receivingFederateId,
        "A process Federation Synchronized event requires a recipient identity.");
    if (event.label.empty()) {
      throw ProcessFederationServiceProtocolError(
          "A process Federation Synchronized event requires a label.");
    }
    auto const failedToSyncFederateIds =
        parameterVector(event.failedToSyncFederateIds);
    validateHandleVector(
        failedToSyncFederateIds,
        "A process Federation Synchronized event requires sorted, unique federate handles.");
    writer.unsigned64(event.receivingFederateId);
    writer.wideString(event.label);
    writer.unsigned64Vector(failedToSyncFederateIds);
  } else if (result.saveEvent.has_value()) {
    auto const& event = *result.saveEvent;
    requireNonzero(
        event.receivingFederateId,
        "A process federation-save event requires a recipient identity.");
    if (event.kind != FederationSaveNotificationKind::status &&
        event.label.empty()) {
      throw ProcessFederationServiceProtocolError(
          "A process federation-save event requires a label.");
    }
    if (event.kind == FederationSaveNotificationKind::status &&
        !event.label.empty()) {
      throw ProcessFederationServiceProtocolError(
          "A process federation-save status event cannot carry a label.");
    }
    if (event.kind != FederationSaveNotificationKind::initiate &&
        event.kind != FederationSaveNotificationKind::completed &&
        event.kind != FederationSaveNotificationKind::status) {
      throw ProcessFederationServiceProtocolError(
          "A process federation-save event has an unsupported notification kind.");
    }
    if (event.kind != FederationSaveNotificationKind::status &&
        !event.statuses.empty()) {
      throw ProcessFederationServiceProtocolError(
          "A non-status federation-save event cannot carry status pairs.");
    }
    if (event.kind == FederationSaveNotificationKind::status &&
        event.statuses.empty()) {
      throw ProcessFederationServiceProtocolError(
          "A federation-save status event requires status pairs.");
    }
    if (event.kind != FederationSaveNotificationKind::initiate &&
        event.timestamp) {
      throw ProcessFederationServiceProtocolError(
          "Only an initiate federation-save event may carry a timestamp.");
    }
    if (event.statuses.size() > std::numeric_limits<std::uint32_t>::max()) {
      throw ProcessFederationServiceProtocolError(
          "A federation-save status event has too many status pairs.");
    }
    auto const failureReason = static_cast<std::uint8_t>(event.failureReason);
    if (failureReason > static_cast<std::uint8_t>(rti1516_2025::SAVE_ABORTED)) {
      throw ProcessFederationServiceProtocolError(
          "A process federation-save event has an invalid failure reason.");
    }
    writer.unsigned8(static_cast<std::uint8_t>(event.kind));
    writer.unsigned64(event.receivingFederateId);
    writer.wideString(event.label);
    writer.unsigned8(event.successful ? 1U : 0U);
    writer.unsigned8(failureReason);
    writer.unsigned32(static_cast<std::uint32_t>(event.statuses.size()));
    std::uint64_t previousFederateId = 0U;
    for (auto const& [federateId, status] : event.statuses) {
      requireNonzero(
          federateId,
          "A federation-save status event requires federate identities.");
      if (federateId <= previousFederateId) {
        throw ProcessFederationServiceProtocolError(
            "A federation-save status event requires sorted, unique federate identities.");
      }
      switch (status) {
        case rti1516_2025::NO_SAVE_IN_PROGRESS:
        case rti1516_2025::FEDERATE_INSTRUCTED_TO_SAVE:
        case rti1516_2025::FEDERATE_SAVING:
        case rti1516_2025::FEDERATE_WAITING_FOR_FEDERATION_TO_SAVE:
          break;
        default:
          throw ProcessFederationServiceProtocolError(
              "A federation-save status event has an invalid SaveStatus.");
      }
      writer.unsigned64(federateId);
      writer.unsigned8(static_cast<std::uint8_t>(status));
      previousFederateId = federateId;
    }
    writeOptionalLogicalTime(writer, event.timestamp);
  } else if (result.restoreEvent.has_value()) {
    auto const& event = *result.restoreEvent;
    requireNonzero(
        event.receivingFederateId,
        "A process federation-restore event requires a recipient identity.");
    if (event.label.empty() &&
        event.kind != FederationRestoreNotificationKind::begin &&
        event.kind != FederationRestoreNotificationKind::status) {
      throw ProcessFederationServiceProtocolError(
          "A process federation-restore event requires a label.");
    }
    auto const encodedKind = static_cast<std::uint8_t>(event.kind);
    if (encodedKind > static_cast<std::uint8_t>(
                          FederationRestoreNotificationKind::status)) {
      throw ProcessFederationServiceProtocolError(
          "A process federation-restore event has an invalid notification kind.");
    }
    auto const failureReason = static_cast<std::uint8_t>(event.failureReason);
    if (failureReason > static_cast<std::uint8_t>(
                             rti1516_2025::RESTORE_ABORTED)) {
      throw ProcessFederationServiceProtocolError(
          "A process federation-restore event has an invalid failure reason.");
    }
    if (event.statuses.size() > std::numeric_limits<std::uint32_t>::max()) {
      throw ProcessFederationServiceProtocolError(
          "A process federation-restore event has too many status records.");
    }
    writer.unsigned8(encodedKind);
    writer.unsigned64(event.receivingFederateId);
    writer.wideString(event.label);
    writer.wideString(event.federateName);
    writer.unsigned64(event.preRestoreFederateId);
    writer.unsigned64(event.postRestoreFederateId);
    writer.unsigned8(event.successful ? 1U : 0U);
    writer.unsigned8(failureReason);
    writer.unsigned32(static_cast<std::uint32_t>(event.statuses.size()));
    std::uint64_t previousPreRestoreId = 0U;
    for (auto const& status : event.statuses) {
      requireNonzero(
          status.preRestoreFederateId,
          "A federation-restore status event requires pre-restore federate identities.");
      if (status.preRestoreFederateId <= previousPreRestoreId) {
        throw ProcessFederationServiceProtocolError(
            "A federation-restore status event requires sorted, unique pre-restore federate identities.");
      }
      auto const encodedStatus = static_cast<std::uint8_t>(status.status);
      if (encodedStatus > static_cast<std::uint8_t>(
              rti1516_2025::FEDERATE_WAITING_FOR_FEDERATION_TO_RESTORE)) {
        throw ProcessFederationServiceProtocolError(
            "A federation-restore status event has an invalid RestoreStatus.");
      }
      if (status.status == rti1516_2025::NO_RESTORE_IN_PROGRESS) {
        if (status.postRestoreFederateId != 0U) {
          throw ProcessFederationServiceProtocolError(
              "A no-restore-in-progress status event requires an invalid post-restore federate identity.");
        }
      } else {
        requireNonzero(
            status.postRestoreFederateId,
            "A federation-restore status event requires post-restore federate identities.");
      }
      writer.unsigned64(status.preRestoreFederateId);
      writer.unsigned64(status.postRestoreFederateId);
      writer.unsigned8(encodedStatus);
      previousPreRestoreId = status.preRestoreFederateId;
    }
  }
  return std::move(writer).finish();
}

ProcessFederationReceiveInteractionResult
decodeProcessFederationReceiveInteractionResult(
    std::span<std::uint8_t const> encoded) {
  PayloadReader reader(encoded);
  auto const eventKind = reader.unsigned8();
  if (eventKind > 19U) {
    throw ProcessFederationServiceProtocolError(
        "A process federation receive result has an invalid event marker.");
  }
  ProcessFederationReceiveInteractionResult result;
  if (eventKind == 1U) {
    ProcessFederationInteractionEvent event;
    event.producingFederateId = reader.unsigned64();
    event.receivingFederateId = reader.unsigned64();
    event.interactionClassHandle = reader.unsigned64();
    event.parameterHandles = reader.unsigned64Vector();
    event.payload = reader.bytes();
    event.transportationName = reader.string();
    event.timestamp = readOptionalLogicalTime(reader);
    if (reader.remaining() != 0U) {
      auto const hasTarget = reader.unsigned8();
      if (hasTarget > 1U) {
        throw ProcessFederationServiceProtocolError(
            "A process interaction event has an invalid directed-target marker.");
      }
      if (hasTarget != 0U) {
        event.objectInstanceHandle = reader.unsigned64();
        requireNonzero(
            *event.objectInstanceHandle,
            "A process directed-interaction event requires a target object instance.");
      }
      if (reader.remaining() != 0U) {
        auto const hasRetraction = reader.unsigned8();
        if (hasRetraction > 1U) {
          throw ProcessFederationServiceProtocolError(
              "A process interaction event has an invalid retraction marker.");
        }
        if (hasRetraction != 0U) {
          event.retractionMessageId = reader.unsigned64();
          requireNonzero(
              *event.retractionMessageId,
              "A process directed-interaction event requires a retraction identity.");
        }
      }
      readInteractionRegionMetadata(reader, event);
      readInteractionOrderMetadata(reader, event);
      // With no explicit order pair the order reader already consumed the
      // RTI-owned marker. With an order pair it remains in the suffix.
      if (reader.remaining() != 0U && !event.rtiOwnedMomInteraction) {
        if (reader.unsigned8() != kRtiOwnedMomInteractionEventMarker) {
          throw ProcessFederationServiceProtocolError(
              "A process interaction event has invalid trailing RTI-owned MOM metadata.");
        }
        event.rtiOwnedMomInteraction = true;
      }
      if (event.rtiOwnedMomInteraction && reader.remaining() != 0U) {
        event.exceptionReportFederateId = reader.unsigned64();
        requireNonzero(*event.exceptionReportFederateId,
                       "An exception report event requires its reported member.");
      }
    }
    if (!event.rtiOwnedMomInteraction) {
      requireNonzero(
          event.producingFederateId,
          "A process federation interaction event requires a producer identity.");
    }
    requireNonzero(
        event.receivingFederateId,
        "A process federation interaction event requires a recipient identity.");
    requireNonzero(
        event.interactionClassHandle,
        "A process federation interaction event requires an interaction class.");
    result.event = std::move(event);
  } else if (eventKind == 2U) {
    ProcessFederationAttributeUpdateEvent event;
    event.producingFederateId = reader.unsigned64();
    event.receivingFederateId = reader.unsigned64();
    event.objectInstanceHandle = reader.unsigned64();
    auto const attributeCount = reader.unsigned32();
    event.attributeValues.reserve(attributeCount);
    std::set<std::uint64_t> seenHandles;
    for (std::uint32_t index = 0U; index < attributeCount; ++index) {
      auto const attributeHandle = reader.unsigned64();
      requireNonzero(
          attributeHandle,
          "A process federation attribute event requires attribute identities.");
      if (!seenHandles.insert(attributeHandle).second) {
        throw ProcessFederationServiceProtocolError(
            "A process federation attribute event repeats an attribute identity.");
      }
      event.attributeValues.emplace_back(attributeHandle, reader.bytes());
    }
    event.userSuppliedTag = reader.bytes();
    event.transportationName = reader.string();
    readAttributeUpdateRegionMetadata(reader, event);
    event.timestamp = readOptionalLogicalTime(reader);
    if (reader.remaining() == sizeof(std::uint64_t)) {
      event.retractionMessageId = reader.unsigned64();
      requireNonzero(
          *event.retractionMessageId,
          "A process attribute event requires a retraction identity.");
    } else if (reader.remaining() != 0U) {
      if (reader.unsigned8() != kRtiOwnedMomAttributeEventMarker) {
        throw ProcessFederationServiceProtocolError(
            "A process attribute event has an invalid RTI-owned MOM marker.");
      }
      event.rtiOwnedMomObject = true;
      if (reader.remaining() != 0U) {
        if (reader.remaining() != sizeof(std::uint64_t)) {
          throw ProcessFederationServiceProtocolError(
              "A process attribute event has invalid trailing metadata.");
        }
        event.retractionMessageId = reader.unsigned64();
        requireNonzero(
            *event.retractionMessageId,
            "A process attribute event requires a retraction identity.");
      }
    }
    if (!event.rtiOwnedMomObject) {
      requireNonzero(
          event.producingFederateId,
          "A process federation attribute event requires a producer identity.");
    }
    requireNonzero(
        event.receivingFederateId,
        "A process federation attribute event requires a recipient identity.");
    requireNonzero(
        event.objectInstanceHandle,
        "A process federation attribute event requires an object instance.");
    if (event.attributeValues.empty() || event.transportationName.empty()) {
      throw ProcessFederationServiceProtocolError(
          "A process federation attribute event requires values and transportation.");
    }
    result.attributeEvent = std::move(event);
  } else if (eventKind == 3U) {
    ProcessFederationObjectInstanceDiscoveryEvent event;
    event.receivingFederateId = reader.unsigned64();
    event.objectInstanceHandle = reader.unsigned64();
    event.objectClassHandle = reader.unsigned64();
    event.objectInstanceName = reader.wideString();
    event.producingFederateId = reader.unsigned64();
    if (reader.remaining() != 0U) {
      if (reader.unsigned8() != kRtiOwnedMomDiscoveryEventMarker) {
        throw ProcessFederationServiceProtocolError(
            "A process discovery event has an invalid RTI-owned MOM marker.");
      }
      event.rtiOwnedMomObject = true;
    }
    requireNonzero(
        event.receivingFederateId,
        "A process federation discovery event requires a recipient identity.");
    requireNonzero(
        event.objectInstanceHandle,
        "A process federation discovery event requires an object instance.");
    requireNonzero(
        event.objectClassHandle,
        "A process federation discovery event requires an object class.");
    if (!event.rtiOwnedMomObject) {
      requireNonzero(
          event.producingFederateId,
          "A process federation discovery event requires a producer identity.");
    }
    if (event.objectInstanceName.empty()) {
      throw ProcessFederationServiceProtocolError(
          "A process federation discovery event requires an object instance name.");
    }
    result.discoveryEvent = std::move(event);
  } else if (eventKind == 4U) {
    ProcessFederationObjectInstanceScopeChangeEvent event;
    event.receivingFederateId = reader.unsigned64();
    event.objectInstanceHandle = reader.unsigned64();
    auto const attributeCount = reader.unsigned32();
    for (std::uint32_t index = 0U; index < attributeCount; ++index) {
      auto const attributeHandle = reader.unsigned64();
      requireNonzero(
          attributeHandle,
          "A process scope event requires attribute identities.");
      if (!event.attributeHandles.insert(attributeHandle).second) {
        throw ProcessFederationServiceProtocolError(
            "A process scope event repeats an attribute identity.");
      }
    }
    auto const inScope = reader.unsigned8();
    if (inScope > 1U) {
      throw ProcessFederationServiceProtocolError(
          "A process scope event has an invalid scope marker.");
    }
    event.inScope = inScope != 0U;
    requireNonzero(
        event.receivingFederateId,
        "A process scope event requires a recipient identity.");
    requireNonzero(
        event.objectInstanceHandle,
        "A process scope event requires an object instance.");
    if (event.attributeHandles.empty()) {
      throw ProcessFederationServiceProtocolError(
          "A process scope event requires attributes.");
    }
    result.scopeChangeEvent = std::move(event);
  } else if (eventKind == 6U) {
    ProcessFederationObjectInstanceRemovalEvent event;
    event.receivingFederateId = reader.unsigned64();
    event.objectInstanceHandle = reader.unsigned64();
    event.producingFederateId = reader.unsigned64();
    event.userSuppliedTag = reader.bytes();
    event.timestamp = readOptionalLogicalTime(reader);
    auto const retractionMessageId = readOptionalMessageId(reader);
    if (retractionMessageId != 0U) {
      event.retractionMessageId = retractionMessageId;
      if (reader.remaining() != 0U) {
        auto const provideRetraction = reader.unsigned8();
        if (provideRetraction > 1U) {
          throw ProcessFederationServiceProtocolError(
              "A process object removal event has an invalid retraction projection marker.");
        }
        event.provideRetraction = provideRetraction != 0U;
      } else {
        // Legacy process payloads had no projection marker and therefore
        // treated a present message id as publicly retraction-capable.
        event.provideRetraction = true;
      }
    }
    if (event.timestamp && reader.remaining() != 0U) {
      auto const orderMarker = reader.unsigned8();
      if (orderMarker > 1U) {
        throw ProcessFederationServiceProtocolError(
            "A process object removal event has an invalid order metadata marker.");
      }
      if (orderMarker != 0U) {
        auto decodeOrder = [](std::uint8_t encoded, char const* description) {
          if (encoded != static_cast<std::uint8_t>(rti1516_2025::RECEIVE) &&
              encoded != static_cast<std::uint8_t>(rti1516_2025::TIMESTAMP)) {
            throw ProcessFederationServiceProtocolError(description);
          }
          return static_cast<rti1516_2025::OrderType>(encoded);
        };
        event.sentOrderType = decodeOrder(
            reader.unsigned8(),
            "A process object removal event has an invalid sent order classification.");
        event.receivedOrderType = decodeOrder(
            reader.unsigned8(),
            "A process object removal event has an invalid received order classification.");
      }
    }
    if (reader.remaining() != 0U) {
      if (reader.unsigned8() != kRtiOwnedMomRemovalEventMarker) {
        throw ProcessFederationServiceProtocolError(
            "A process object removal event has an invalid RTI-owned MOM marker.");
      }
      event.rtiOwnedMomObject = true;
      if (event.producingFederateId != 0U || event.timestamp ||
          event.retractionMessageId) {
        throw ProcessFederationServiceProtocolError(
            "An RTI-owned MOM removal event has conflicting producer or timestamp metadata.");
      }
    }
    requireNonzero(
        event.receivingFederateId,
        "A process object removal event requires a recipient identity.");
    requireNonzero(
        event.objectInstanceHandle,
        "A process object removal event requires an object instance.");
    if (!event.rtiOwnedMomObject) {
      requireNonzero(
          event.producingFederateId,
          "A process object removal event requires a producer identity.");
    }
    result.removalEvent = std::move(event);
  } else if (eventKind == 5U) {
    ProcessFederationAttributeRelevanceAdvisoryEvent event;
    event.providingFederateId = reader.unsigned64();
    event.receivingFederateId = reader.unsigned64();
    event.objectInstanceHandle = reader.unsigned64();
    auto const attributeCount = reader.unsigned32();
    for (std::uint32_t index = 0U; index < attributeCount; ++index) {
      auto const attributeHandle = reader.unsigned64();
      requireNonzero(
          attributeHandle,
          "A process attribute relevance advisory requires attribute identities.");
      if (!event.attributeHandles.insert(attributeHandle).second) {
        throw ProcessFederationServiceProtocolError(
            "A process attribute relevance advisory repeats an attribute identity.");
      }
    }
    auto const turnUpdatesOn = reader.unsigned8();
    if (turnUpdatesOn > 1U) {
      throw ProcessFederationServiceProtocolError(
          "A process attribute relevance advisory has an invalid direction marker.");
    }
    event.turnUpdatesOn = turnUpdatesOn != 0U;
    auto const hasUpdateRateDesignator = reader.unsigned8();
    if (hasUpdateRateDesignator > 1U) {
      throw ProcessFederationServiceProtocolError(
          "A process attribute relevance advisory has an invalid update-rate marker.");
    }
    if (hasUpdateRateDesignator != 0U) {
      event.updateRateDesignator = reader.string();
      if (event.updateRateDesignator->empty()) {
        throw ProcessFederationServiceProtocolError(
            "A process attribute relevance advisory cannot carry an empty update-rate designator.");
      }
    }
    requireNonzero(
        event.providingFederateId,
        "A process attribute relevance advisory requires an owning federate.");
    requireNonzero(
        event.objectInstanceHandle,
        "A process attribute relevance advisory requires an object instance.");
    if (event.attributeHandles.empty()) {
      throw ProcessFederationServiceProtocolError(
          "A process attribute relevance advisory requires attributes.");
    }
    result.attributeRelevanceAdvisoryEvent = std::move(event);
  } else if (eventKind == 7U) {
    ProcessFederationAttributeValueUpdateRequestEvent event;
    event.requestingFederateId = reader.unsigned64();
    event.providingFederateId = reader.unsigned64();
    event.objectInstanceHandle = reader.unsigned64();
    auto const attributeCount = reader.unsigned32();
    for (std::uint32_t index = 0U; index < attributeCount; ++index) {
      auto const attributeHandle = reader.unsigned64();
      requireNonzero(
          attributeHandle,
          "A process attribute-value-update request event requires attribute identities.");
      if (!event.requestedAttributeHandles.insert(attributeHandle).second) {
        throw ProcessFederationServiceProtocolError(
            "A process attribute-value-update request event repeats an attribute identity.");
      }
    }
    event.userSuppliedTag = reader.bytes();
    requireNonzero(
        event.requestingFederateId,
        "A process attribute-value-update request event requires a requester.");
    requireNonzero(
        event.providingFederateId,
        "A process attribute-value-update request event requires a provider.");
    requireNonzero(
        event.objectInstanceHandle,
        "A process attribute-value-update request event requires an object instance.");
    if (event.requestedAttributeHandles.empty()) {
      throw ProcessFederationServiceProtocolError(
          "A process attribute-value-update request event requires attributes.");
    }
    result.attributeValueUpdateRequestEvent = std::move(event);
  } else if (eventKind == 8U) {
    ProcessFederationAttributeOwnershipQueryEvent event;
    event.requestId = reader.unsigned64();
    event.receivingFederateId = reader.unsigned64();
    event.objectInstanceHandle = reader.unsigned64();
    auto const encodedReportKind = reader.unsigned8();
    if (encodedReportKind > static_cast<std::uint8_t>(
                                AttributeOwnershipQueryReportKind::rti)) {
      throw ProcessFederationServiceProtocolError(
          "A process attribute-ownership query event has an invalid report kind.");
    }
    event.reportKind = static_cast<AttributeOwnershipQueryReportKind>(
        encodedReportKind);
    event.owningFederateId = reader.unsigned64();
    auto const attributeCount = reader.unsigned32();
    for (std::uint32_t index = 0U; index < attributeCount; ++index) {
      auto const attributeHandle = reader.unsigned64();
      requireNonzero(
          attributeHandle,
          "A process attribute-ownership query event requires attribute identities.");
      if (!event.attributeHandles.insert(attributeHandle).second) {
        throw ProcessFederationServiceProtocolError(
            "A process attribute-ownership query event repeats an attribute identity.");
      }
    }
    requireNonzero(
        event.requestId,
        "A process attribute-ownership query event requires a request identity.");
    requireNonzero(
        event.receivingFederateId,
        "A process attribute-ownership query event requires a recipient identity.");
    requireNonzero(
        event.objectInstanceHandle,
        "A process attribute-ownership query event requires an object instance.");
    if (event.attributeHandles.empty()) {
      throw ProcessFederationServiceProtocolError(
          "A process attribute-ownership query event requires attributes.");
    }
    if (event.reportKind == AttributeOwnershipQueryReportKind::federate) {
      requireNonzero(
          event.owningFederateId,
          "A federate-owned process attribute-ownership query event requires an owner.");
    } else if (event.owningFederateId != 0U) {
      throw ProcessFederationServiceProtocolError(
          "A non-federate process attribute-ownership query event cannot carry an owner.");
    }
    result.attributeOwnershipQueryEvent = std::move(event);
  } else if (eventKind == 9U) {
    ProcessFederationAttributeOwnershipAcquisitionIfAvailableEvent event;
    event.requestId = reader.unsigned64();
    event.receivingFederateId = reader.unsigned64();
    event.objectInstanceHandle = reader.unsigned64();
    auto const securedCount = reader.unsigned32();
    for (std::uint32_t index = 0U; index < securedCount; ++index) {
      auto const attributeHandle = reader.unsigned64();
      requireNonzero(
          attributeHandle,
          "A process attribute-ownership acquisition-if-available event requires attribute identities.");
      if (!event.securedAttributeHandles.insert(attributeHandle).second) {
        throw ProcessFederationServiceProtocolError(
            "A process attribute-ownership acquisition-if-available event repeats an attribute identity.");
      }
    }
    auto const unavailableCount = reader.unsigned32();
    for (std::uint32_t index = 0U; index < unavailableCount; ++index) {
      auto const attributeHandle = reader.unsigned64();
      requireNonzero(
          attributeHandle,
          "A process attribute-ownership acquisition-if-available event requires attribute identities.");
      if (event.securedAttributeHandles.contains(attributeHandle) ||
          !event.unavailableAttributeHandles.insert(attributeHandle).second) {
        throw ProcessFederationServiceProtocolError(
            "A process attribute-ownership acquisition-if-available event repeats an attribute identity.");
      }
    }
    event.userSuppliedTag = reader.bytes();
    requireNonzero(
        event.requestId,
        "A process attribute-ownership acquisition-if-available event requires a request identity.");
    requireNonzero(
        event.receivingFederateId,
        "A process attribute-ownership acquisition-if-available event requires a recipient identity.");
    requireNonzero(
        event.objectInstanceHandle,
        "A process attribute-ownership acquisition-if-available event requires an object instance.");
    if (event.securedAttributeHandles.empty() &&
        event.unavailableAttributeHandles.empty()) {
      throw ProcessFederationServiceProtocolError(
          "A process attribute-ownership acquisition-if-available event requires a delivery.");
    }
    result.attributeOwnershipAcquisitionIfAvailableEvent = std::move(event);
  } else if (eventKind == 10U) {
    ProcessFederationAttributeOwnershipAcquisitionEvent event;
    auto const encodedKind = reader.unsigned8();
    if (encodedKind > static_cast<std::uint8_t>(
                          ProcessFederationAttributeOwnershipAcquisitionEventKind::
                              confirm_divestiture_notification)) {
      throw ProcessFederationServiceProtocolError(
          "A process attribute-ownership acquisition event has an invalid kind.");
    }
    event.kind = static_cast<
        ProcessFederationAttributeOwnershipAcquisitionEventKind>(encodedKind);
    event.requestId = reader.unsigned64();
    event.requestingFederateId = reader.unsigned64();
    event.receivingFederateId = reader.unsigned64();
    event.objectInstanceHandle = reader.unsigned64();
    auto const attributeCount = reader.unsigned32();
    for (std::uint32_t index = 0U; index < attributeCount; ++index) {
      auto const attributeHandle = reader.unsigned64();
      requireNonzero(
          attributeHandle,
          "A process attribute-ownership acquisition event requires attribute identities.");
      if (!event.attributeHandles.insert(attributeHandle).second) {
        throw ProcessFederationServiceProtocolError(
            "A process attribute-ownership acquisition event repeats an attribute identity.");
      }
    }
    event.userSuppliedTag = reader.bytes();
    if (event.kind == ProcessFederationAttributeOwnershipAcquisitionEventKind::
                        request_divestiture_confirmation) {
      auto const candidateIsIfAvailable = reader.unsigned8();
      if (candidateIsIfAvailable > 1U) {
        throw ProcessFederationServiceProtocolError(
            "A process request-divestiture-confirmation event has an invalid candidate marker.");
      }
      event.candidateIsIfAvailable = candidateIsIfAvailable != 0U;
    }
    if (event.kind != ProcessFederationAttributeOwnershipAcquisitionEventKind::
                    ownership_assumption) {
      requireNonzero(
          event.requestId,
          "A process attribute-ownership acquisition event requires a request identity.");
    }
    requireNonzero(
        event.requestingFederateId,
        "A process attribute-ownership acquisition event requires a requester identity.");
    requireNonzero(
        event.receivingFederateId,
        "A process attribute-ownership acquisition event requires a recipient identity.");
    requireNonzero(
        event.objectInstanceHandle,
        "A process attribute-ownership acquisition event requires an object instance.");
    if (event.attributeHandles.empty()) {
      throw ProcessFederationServiceProtocolError(
          "A process attribute-ownership acquisition event requires attributes.");
    }
    result.attributeOwnershipAcquisitionEvent = std::move(event);
  } else if (eventKind == 11U) {
    ProcessFederationAttributeOwnershipUnavailableEvent event;
    event.receivingFederateId = reader.unsigned64();
    event.objectInstanceHandle = reader.unsigned64();
    auto const attributeCount = reader.unsigned32();
    for (std::uint32_t index = 0U; index < attributeCount; ++index) {
      auto const attributeHandle = reader.unsigned64();
      requireNonzero(
          attributeHandle,
          "A process attribute-ownership unavailable event requires attribute identities.");
      if (!event.attributeHandles.insert(attributeHandle).second) {
        throw ProcessFederationServiceProtocolError(
            "A process attribute-ownership unavailable event repeats an attribute identity.");
      }
    }
    event.userSuppliedTag = reader.bytes();
    requireNonzero(
        event.receivingFederateId,
        "A process attribute-ownership unavailable event requires a recipient identity.");
    requireNonzero(
        event.objectInstanceHandle,
        "A process attribute-ownership unavailable event requires an object instance.");
    if (event.attributeHandles.empty()) {
      throw ProcessFederationServiceProtocolError(
          "A process attribute-ownership unavailable event requires attributes.");
    }
    result.attributeOwnershipUnavailableEvent = std::move(event);
  } else if (eventKind == 12U) {
    ProcessFederationAttributeTransportationTypeChangeEvent event;
    event.requestId = reader.unsigned64();
    event.receivingFederateId = reader.unsigned64();
    event.objectInstanceHandle = reader.unsigned64();
    auto const attributeCount = reader.unsigned32();
    for (std::uint32_t index = 0U; index < attributeCount; ++index) {
      auto const attributeHandle = reader.unsigned64();
      requireNonzero(
          attributeHandle,
          "A process attribute transportation-type change event requires attribute identities.");
      if (!event.attributeHandles.insert(attributeHandle).second) {
        throw ProcessFederationServiceProtocolError(
            "A process attribute transportation-type change event repeats an attribute identity.");
      }
    }
    event.transportationName = reader.string();
    requireNonzero(
        event.requestId,
        "A process attribute transportation-type change event requires a request identity.");
    requireNonzero(
        event.receivingFederateId,
        "A process attribute transportation-type change event requires a recipient identity.");
    requireNonzero(
        event.objectInstanceHandle,
        "A process attribute transportation-type change event requires an object instance.");
    if (event.attributeHandles.empty() || event.transportationName.empty()) {
      throw ProcessFederationServiceProtocolError(
          "A process attribute transportation-type change event requires attributes and transportation.");
    }
    result.attributeTransportationTypeChangeEvent = std::move(event);
  } else if (eventKind == 13U) {
    ProcessFederationAttributeTransportationTypeQueryEvent event;
    event.receivingFederateId = reader.unsigned64();
    event.objectInstanceHandle = reader.unsigned64();
    event.attributeHandle = reader.unsigned64();
    event.transportationName = reader.string();
    requireNonzero(
        event.receivingFederateId,
        "A process attribute transportation-type query event requires a recipient identity.");
    requireNonzero(
        event.objectInstanceHandle,
        "A process attribute transportation-type query event requires an object instance.");
    requireNonzero(
        event.attributeHandle,
        "A process attribute transportation-type query event requires an attribute identity.");
    if (event.transportationName.empty()) {
      throw ProcessFederationServiceProtocolError(
          "A process attribute transportation-type query event requires transportation.");
    }
    result.attributeTransportationTypeQueryEvent = std::move(event);
  } else if (eventKind == 14U) {
    ProcessFederationInteractionTransportationTypeChangeEvent event;
    event.receivingFederateId = reader.unsigned64();
    event.interactionClassHandle = reader.unsigned64();
    event.transportationName = reader.string();
    requireNonzero(
        event.receivingFederateId,
        "A process interaction transportation-type change event requires a recipient identity.");
    requireNonzero(
        event.interactionClassHandle,
        "A process interaction transportation-type change event requires an interaction class.");
    if (event.transportationName.empty()) {
      throw ProcessFederationServiceProtocolError(
          "A process interaction transportation-type change event requires transportation.");
    }
    result.interactionTransportationTypeChangeEvent = std::move(event);
  } else if (eventKind == 15U) {
    ProcessFederationInteractionTransportationTypeQueryEvent event;
    event.receivingFederateId = reader.unsigned64();
    event.queriedFederateId = reader.unsigned64();
    event.interactionClassHandle = reader.unsigned64();
    event.transportationName = reader.string();
    requireNonzero(
        event.receivingFederateId,
        "A process interaction transportation-type query event requires a recipient identity.");
    requireNonzero(
        event.queriedFederateId,
        "A process interaction transportation-type query event requires a queried federate identity.");
    requireNonzero(
        event.interactionClassHandle,
        "A process interaction transportation-type query event requires an interaction class.");
    if (event.transportationName.empty()) {
      throw ProcessFederationServiceProtocolError(
          "A process interaction transportation-type query event requires transportation.");
    }
    result.interactionTransportationTypeQueryEvent = std::move(event);
  } else if (eventKind == 16U) {
    ProcessFederationSynchronizationPointAnnouncementEvent event;
    event.receivingFederateId = reader.unsigned64();
    event.label = reader.wideString();
    event.userSuppliedTag = reader.bytes();
    requireNonzero(
        event.receivingFederateId,
        "A process synchronization-point announcement requires a recipient identity.");
    if (event.label.empty()) {
      throw ProcessFederationServiceProtocolError(
          "A process synchronization-point announcement requires a label.");
    }
    result.synchronizationPointAnnouncementEvent = std::move(event);
  } else if (eventKind == 17U) {
    ProcessFederationFederationSynchronizedEvent event;
    event.receivingFederateId = reader.unsigned64();
    event.label = reader.wideString();
    auto const failed = reader.unsigned64Vector();
    validateHandleVector(
        failed,
        "A process Federation Synchronized event requires sorted, unique federate handles.");
    event.failedToSyncFederateIds.insert(failed.begin(), failed.end());
    requireNonzero(
        event.receivingFederateId,
        "A process Federation Synchronized event requires a recipient identity.");
    if (event.label.empty()) {
      throw ProcessFederationServiceProtocolError(
          "A process Federation Synchronized event requires a label.");
    }
    result.federationSynchronizedEvent = std::move(event);
  } else if (eventKind == 18U) {
    ProcessFederationSaveEvent event;
    auto const encodedKind = reader.unsigned8();
    if (encodedKind > static_cast<std::uint8_t>(
                          FederationSaveNotificationKind::status)) {
      throw ProcessFederationServiceProtocolError(
          "A process federation-save event has an invalid notification kind.");
    }
    event.kind = static_cast<FederationSaveNotificationKind>(encodedKind);
    event.receivingFederateId = reader.unsigned64();
    event.label = reader.wideString();
    auto const successful = reader.unsigned8();
    if (successful > 1U) {
      throw ProcessFederationServiceProtocolError(
          "A process federation-save event has an invalid success marker.");
    }
    event.successful = successful != 0U;
    auto const failureReason = reader.unsigned8();
    if (failureReason > static_cast<std::uint8_t>(rti1516_2025::SAVE_ABORTED)) {
      throw ProcessFederationServiceProtocolError(
          "A process federation-save event has an invalid failure reason.");
    }
    event.failureReason =
        static_cast<rti1516_2025::SaveFailureReason>(failureReason);
    auto const statusCount = reader.unsigned32();
    event.statuses.reserve(statusCount);
    std::uint64_t previousFederateId = 0U;
    for (std::uint32_t index = 0U; index < statusCount; ++index) {
      auto const federateId = reader.unsigned64();
      requireNonzero(
          federateId,
          "A federation-save status event requires federate identities.");
      if (federateId <= previousFederateId) {
        throw ProcessFederationServiceProtocolError(
            "A federation-save status event requires sorted, unique federate identities.");
      }
      auto const encodedStatus = reader.unsigned8();
      if (encodedStatus > static_cast<std::uint8_t>(
                              rti1516_2025::FEDERATE_WAITING_FOR_FEDERATION_TO_SAVE)) {
        throw ProcessFederationServiceProtocolError(
            "A federation-save status event has an invalid SaveStatus.");
      }
      auto const status = static_cast<rti1516_2025::SaveStatus>(encodedStatus);
      switch (status) {
        case rti1516_2025::NO_SAVE_IN_PROGRESS:
        case rti1516_2025::FEDERATE_INSTRUCTED_TO_SAVE:
        case rti1516_2025::FEDERATE_SAVING:
        case rti1516_2025::FEDERATE_WAITING_FOR_FEDERATION_TO_SAVE:
          break;
        default:
          throw ProcessFederationServiceProtocolError(
              "A federation-save status event has an invalid SaveStatus.");
      }
      event.statuses.emplace_back(federateId, status);
      previousFederateId = federateId;
    }
    event.timestamp = readOptionalLogicalTime(reader);
    requireNonzero(
        event.receivingFederateId,
        "A process federation-save event requires a recipient identity.");
    if (event.kind != FederationSaveNotificationKind::status &&
        event.label.empty()) {
      throw ProcessFederationServiceProtocolError(
          "A process federation-save event requires a label.");
    }
    if (event.kind == FederationSaveNotificationKind::status &&
        (!event.label.empty() || event.statuses.empty())) {
      throw ProcessFederationServiceProtocolError(
          "A process federation-save status event requires status pairs and no label.");
    }
    if (event.kind != FederationSaveNotificationKind::status &&
        !event.statuses.empty()) {
      throw ProcessFederationServiceProtocolError(
          "A non-status federation-save event cannot carry status pairs.");
    }
    if (event.kind != FederationSaveNotificationKind::initiate &&
        event.timestamp) {
      throw ProcessFederationServiceProtocolError(
          "Only an initiate federation-save event may carry a timestamp.");
    }
    result.saveEvent = std::move(event);
  } else if (eventKind == 19U) {
    ProcessFederationRestoreEvent event;
    auto const encodedKind = reader.unsigned8();
    if (encodedKind > static_cast<std::uint8_t>(
                          FederationRestoreNotificationKind::status)) {
      throw ProcessFederationServiceProtocolError(
          "A process federation-restore event has an invalid notification kind.");
    }
    event.kind = static_cast<FederationRestoreNotificationKind>(encodedKind);
    event.receivingFederateId = reader.unsigned64();
    event.label = reader.wideString();
    event.federateName = reader.wideString();
    event.preRestoreFederateId = reader.unsigned64();
    event.postRestoreFederateId = reader.unsigned64();
    auto const successful = reader.unsigned8();
    if (successful > 1U) {
      throw ProcessFederationServiceProtocolError(
          "A process federation-restore event has an invalid success marker.");
    }
    event.successful = successful != 0U;
    auto const failureReason = reader.unsigned8();
    if (failureReason > static_cast<std::uint8_t>(
                             rti1516_2025::RESTORE_ABORTED)) {
      throw ProcessFederationServiceProtocolError(
          "A process federation-restore event has an invalid failure reason.");
    }
    event.failureReason =
        static_cast<rti1516_2025::RestoreFailureReason>(failureReason);
    auto const statusCount = reader.unsigned32();
    event.statuses.reserve(statusCount);
    std::uint64_t previousPreRestoreId = 0U;
    for (std::uint32_t index = 0U; index < statusCount; ++index) {
      ProcessFederationRestoreEvent::StatusRecord status;
      status.preRestoreFederateId = reader.unsigned64();
      status.postRestoreFederateId = reader.unsigned64();
      requireNonzero(
          status.preRestoreFederateId,
          "A federation-restore status event requires pre-restore federate identities.");
      if (status.preRestoreFederateId <= previousPreRestoreId) {
        throw ProcessFederationServiceProtocolError(
            "A federation-restore status event requires sorted, unique pre-restore federate identities.");
      }
      auto const encodedStatus = reader.unsigned8();
      if (encodedStatus > static_cast<std::uint8_t>(
                              rti1516_2025::FEDERATE_WAITING_FOR_FEDERATION_TO_RESTORE)) {
        throw ProcessFederationServiceProtocolError(
            "A federation-restore status event has an invalid RestoreStatus.");
      }
      status.status = static_cast<rti1516_2025::RestoreStatus>(encodedStatus);
      if (status.status == rti1516_2025::NO_RESTORE_IN_PROGRESS) {
        if (status.postRestoreFederateId != 0U) {
          throw ProcessFederationServiceProtocolError(
              "A no-restore-in-progress status event requires an invalid post-restore federate identity.");
        }
      } else {
        requireNonzero(
            status.postRestoreFederateId,
            "A federation-restore status event requires post-restore federate identities.");
      }
      event.statuses.push_back(status);
      previousPreRestoreId = status.preRestoreFederateId;
    }
    requireNonzero(
        event.receivingFederateId,
        "A process federation-restore event requires a recipient identity.");
    if (event.label.empty() &&
        event.kind != FederationRestoreNotificationKind::begin &&
        event.kind != FederationRestoreNotificationKind::status) {
      throw ProcessFederationServiceProtocolError(
          "A process federation-restore event requires a label.");
    }
    result.restoreEvent = std::move(event);
  }
  reader.finish();
  return result;
}



}  // namespace umbra::detail
