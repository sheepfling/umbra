#include "internal/federation/process_federation_service_protocol.hpp"
#include "internal/federation/process_federation_service_codec_support.hpp"
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
namespace umbra::detail {
using namespace process_federation_service_codec_support;
using process_federation_payload::parameterVector;

std::vector<std::uint8_t>
encodeProcessFederationChangeInteractionOrderTypeRequest(
    ProcessFederationChangeInteractionOrderTypeRequest const& request) {
  if (request.federationName.empty()) {
    throw ProcessFederationServiceProtocolError(
        "A process interaction order-type change requires a federation name.");
  }
  requireNonzero(
      request.federateId,
      "A process interaction order-type change requires a federate identity.");
  requireNonzero(
      request.interactionClassHandle,
      "A process interaction order-type change requires an interaction class.");
  if (!validOrderType(request.orderType)) {
    throw ProcessFederationServiceProtocolError(
        "A process interaction order-type change has an invalid order type.");
  }
  PayloadWriter writer;
  writer.wideString(request.federationName);
  writer.unsigned64(request.federateId);
  writer.unsigned64(request.interactionClassHandle);
  writer.unsigned8(static_cast<std::uint8_t>(request.orderType));
  return std::move(writer).finish();
}

ProcessFederationChangeInteractionOrderTypeRequest
decodeProcessFederationChangeInteractionOrderTypeRequest(
    std::span<std::uint8_t const> encoded) {
  PayloadReader reader(encoded);
  ProcessFederationChangeInteractionOrderTypeRequest result;
  result.federationName = reader.wideString();
  result.federateId = reader.unsigned64();
  result.interactionClassHandle = reader.unsigned64();
  auto const encodedOrderType = reader.unsigned8();
  reader.finish();
  if (result.federationName.empty()) {
    throw ProcessFederationServiceProtocolError(
        "A process interaction order-type change requires a federation name.");
  }
  requireNonzero(
      result.federateId,
      "A process interaction order-type change requires a federate identity.");
  requireNonzero(
      result.interactionClassHandle,
      "A process interaction order-type change requires an interaction class.");
  result.orderType = static_cast<rti1516_2025::OrderType>(encodedOrderType);
  if (!validOrderType(result.orderType)) {
    throw ProcessFederationServiceProtocolError(
        "A process interaction order-type change has an invalid order type.");
  }
  return result;
}


std::vector<std::uint8_t>
encodeProcessFederationChangeAttributeOrderTypeRequest(
    ProcessFederationChangeAttributeOrderTypeRequest const& request) {
  if (request.federationName.empty()) {
    throw ProcessFederationServiceProtocolError(
        "A process attribute order-type change requires a federation name.");
  }
  requireNonzero(
      request.federateId,
      "A process attribute order-type change requires a federate identity.");
  requireNonzero(
      request.objectInstanceHandle,
      "A process attribute order-type change requires an object instance.");
  for (auto const attributeHandle : request.attributeHandles) {
    requireNonzero(
        attributeHandle,
        "A process attribute order-type change requires non-zero attributes.");
  }
  if (!validOrderType(request.orderType)) {
    throw ProcessFederationServiceProtocolError(
        "A process attribute order-type change has an invalid order type.");
  }
  PayloadWriter writer;
  writer.wideString(request.federationName);
  writer.unsigned64(request.federateId);
  writer.unsigned64(request.objectInstanceHandle);
  writer.unsigned64Vector(request.attributeHandles);
  writer.unsigned8(static_cast<std::uint8_t>(request.orderType));
  return std::move(writer).finish();
}

ProcessFederationChangeAttributeOrderTypeRequest
decodeProcessFederationChangeAttributeOrderTypeRequest(
    std::span<std::uint8_t const> encoded) {
  PayloadReader reader(encoded);
  ProcessFederationChangeAttributeOrderTypeRequest result;
  result.federationName = reader.wideString();
  result.federateId = reader.unsigned64();
  result.objectInstanceHandle = reader.unsigned64();
  result.attributeHandles = reader.unsigned64Vector();
  auto const encodedOrderType = reader.unsigned8();
  reader.finish();
  if (result.federationName.empty()) {
    throw ProcessFederationServiceProtocolError(
        "A process attribute order-type change requires a federation name.");
  }
  requireNonzero(
      result.federateId,
      "A process attribute order-type change requires a federate identity.");
  requireNonzero(
      result.objectInstanceHandle,
      "A process attribute order-type change requires an object instance.");
  for (auto const attributeHandle : result.attributeHandles) {
    requireNonzero(
        attributeHandle,
        "A process attribute order-type change requires non-zero attributes.");
  }
  result.orderType = static_cast<rti1516_2025::OrderType>(encodedOrderType);
  if (!validOrderType(result.orderType)) {
    throw ProcessFederationServiceProtocolError(
        "A process attribute order-type change has an invalid order type.");
  }
  return result;
}

std::vector<std::uint8_t>
encodeProcessFederationChangeDefaultAttributeOrderTypeRequest(
    ProcessFederationChangeDefaultAttributeOrderTypeRequest const& request) {
  if (request.federationName.empty()) {
    throw ProcessFederationServiceProtocolError(
        "A process default attribute order-type change requires a federation name.");
  }
  requireNonzero(
      request.federateId,
      "A process default attribute order-type change requires a federate identity.");
  requireNonzero(
      request.objectClassHandle,
      "A process default attribute order-type change requires an object class.");
  for (auto const attributeHandle : request.attributeHandles) {
    requireNonzero(
        attributeHandle,
        "A process default attribute order-type change requires non-zero attributes.");
  }
  if (!validOrderType(request.orderType)) {
    throw ProcessFederationServiceProtocolError(
        "A process default attribute order-type change has an invalid order type.");
  }
  PayloadWriter writer;
  writer.wideString(request.federationName);
  writer.unsigned64(request.federateId);
  writer.unsigned64(request.objectClassHandle);
  writer.unsigned64Vector(request.attributeHandles);
  writer.unsigned8(static_cast<std::uint8_t>(request.orderType));
  return std::move(writer).finish();
}

ProcessFederationChangeDefaultAttributeOrderTypeRequest
decodeProcessFederationChangeDefaultAttributeOrderTypeRequest(
    std::span<std::uint8_t const> encoded) {
  PayloadReader reader(encoded);
  ProcessFederationChangeDefaultAttributeOrderTypeRequest result;
  result.federationName = reader.wideString();
  result.federateId = reader.unsigned64();
  result.objectClassHandle = reader.unsigned64();
  result.attributeHandles = reader.unsigned64Vector();
  auto const encodedOrderType = reader.unsigned8();
  reader.finish();
  if (result.federationName.empty()) {
    throw ProcessFederationServiceProtocolError(
        "A process default attribute order-type change requires a federation name.");
  }
  requireNonzero(
      result.federateId,
      "A process default attribute order-type change requires a federate identity.");
  requireNonzero(
      result.objectClassHandle,
      "A process default attribute order-type change requires an object class.");
  for (auto const attributeHandle : result.attributeHandles) {
    requireNonzero(
        attributeHandle,
        "A process default attribute order-type change requires non-zero attributes.");
  }
  result.orderType = static_cast<rti1516_2025::OrderType>(encodedOrderType);
  if (!validOrderType(result.orderType)) {
    throw ProcessFederationServiceProtocolError(
        "A process default attribute order-type change has an invalid order type.");
  }
  return result;
}


std::vector<std::uint8_t>
encodeProcessFederationChangeDefaultAttributeTransportationTypeRequest(
    ProcessFederationChangeDefaultAttributeTransportationTypeRequest const& request) {
  if (request.federationName.empty()) {
    throw ProcessFederationServiceProtocolError(
        "A process default attribute transportation-type change requires a federation name.");
  }
  requireNonzero(
      request.federateId,
      "A process default attribute transportation-type change requires a federate identity.");
  requireNonzero(
      request.objectClassHandle,
      "A process default attribute transportation-type change requires an object class.");
  for (auto const attributeHandle : request.attributeHandles) {
    requireNonzero(
        attributeHandle,
        "A process default attribute transportation-type change requires non-zero attributes.");
  }
  requireNonzero(
      request.transportationTypeHandle,
      "A process default attribute transportation-type change requires a transportation type.");
  PayloadWriter writer;
  writer.wideString(request.federationName);
  writer.unsigned64(request.federateId);
  writer.unsigned64(request.objectClassHandle);
  writer.unsigned64Vector(request.attributeHandles);
  writer.unsigned64(request.transportationTypeHandle);
  return std::move(writer).finish();
}

ProcessFederationChangeDefaultAttributeTransportationTypeRequest
decodeProcessFederationChangeDefaultAttributeTransportationTypeRequest(
    std::span<std::uint8_t const> encoded) {
  PayloadReader reader(encoded);
  ProcessFederationChangeDefaultAttributeTransportationTypeRequest result;
  result.federationName = reader.wideString();
  result.federateId = reader.unsigned64();
  result.objectClassHandle = reader.unsigned64();
  result.attributeHandles = reader.unsigned64Vector();
  result.transportationTypeHandle = reader.unsigned64();
  reader.finish();
  if (result.federationName.empty()) {
    throw ProcessFederationServiceProtocolError(
        "A process default attribute transportation-type change requires a federation name.");
  }
  requireNonzero(
      result.federateId,
      "A process default attribute transportation-type change requires a federate identity.");
  requireNonzero(
      result.objectClassHandle,
      "A process default attribute transportation-type change requires an object class.");
  for (auto const attributeHandle : result.attributeHandles) {
    requireNonzero(
        attributeHandle,
        "A process default attribute transportation-type change requires non-zero attributes.");
  }
  requireNonzero(
      result.transportationTypeHandle,
      "A process default attribute transportation-type change requires a transportation type.");
  return result;
}


std::vector<std::uint8_t>
encodeProcessFederationRequestAttributeTransportationTypeChangeRequest(
    ProcessFederationRequestAttributeTransportationTypeChangeRequest const& request) {
  if (request.federationName.empty()) {
    throw ProcessFederationServiceProtocolError(
        "A process attribute transportation-type change requires a federation name.");
  }
  requireNonzero(
      request.requestingFederateId,
      "A process attribute transportation-type change requires a federate identity.");
  requireNonzero(
      request.objectInstanceHandle,
      "A process attribute transportation-type change requires an object instance.");
  for (auto const attributeHandle : request.attributeHandles) {
    requireNonzero(
        attributeHandle,
        "A process attribute transportation-type change requires non-zero attributes.");
  }
  requireNonzero(
      request.transportationTypeHandle,
      "A process attribute transportation-type change requires a transportation type.");
  PayloadWriter writer;
  writer.wideString(request.federationName);
  writer.unsigned64(request.requestingFederateId);
  writer.unsigned64(request.objectInstanceHandle);
  writer.unsigned64Vector(request.attributeHandles);
  writer.unsigned64(request.transportationTypeHandle);
  return std::move(writer).finish();
}

ProcessFederationRequestAttributeTransportationTypeChangeRequest
decodeProcessFederationRequestAttributeTransportationTypeChangeRequest(
    std::span<std::uint8_t const> encoded) {
  PayloadReader reader(encoded);
  ProcessFederationRequestAttributeTransportationTypeChangeRequest result;
  result.federationName = reader.wideString();
  result.requestingFederateId = reader.unsigned64();
  result.objectInstanceHandle = reader.unsigned64();
  result.attributeHandles = reader.unsigned64Vector();
  result.transportationTypeHandle = reader.unsigned64();
  reader.finish();
  if (result.federationName.empty()) {
    throw ProcessFederationServiceProtocolError(
        "A process attribute transportation-type change requires a federation name.");
  }
  requireNonzero(
      result.requestingFederateId,
      "A process attribute transportation-type change requires a federate identity.");
  requireNonzero(
      result.objectInstanceHandle,
      "A process attribute transportation-type change requires an object instance.");
  for (auto const attributeHandle : result.attributeHandles) {
    requireNonzero(
        attributeHandle,
        "A process attribute transportation-type change requires non-zero attributes.");
  }
  requireNonzero(
      result.transportationTypeHandle,
      "A process attribute transportation-type change requires a transportation type.");
  return result;
}

std::vector<std::uint8_t>
encodeProcessFederationQueryAttributeTransportationTypeRequest(
    ProcessFederationQueryAttributeTransportationTypeRequest const& request) {
  if (request.federationName.empty()) {
    throw ProcessFederationServiceProtocolError(
        "A process attribute transportation-type query requires a federation name.");
  }
  requireNonzero(
      request.requestingFederateId,
      "A process attribute transportation-type query requires a federate identity.");
  requireNonzero(
      request.objectInstanceHandle,
      "A process attribute transportation-type query requires an object instance.");
  requireNonzero(
      request.attributeHandle,
      "A process attribute transportation-type query requires an attribute.");
  PayloadWriter writer;
  writer.wideString(request.federationName);
  writer.unsigned64(request.requestingFederateId);
  writer.unsigned64(request.objectInstanceHandle);
  writer.unsigned64(request.attributeHandle);
  return std::move(writer).finish();
}

ProcessFederationQueryAttributeTransportationTypeRequest
decodeProcessFederationQueryAttributeTransportationTypeRequest(
    std::span<std::uint8_t const> encoded) {
  PayloadReader reader(encoded);
  ProcessFederationQueryAttributeTransportationTypeRequest result;
  result.federationName = reader.wideString();
  result.requestingFederateId = reader.unsigned64();
  result.objectInstanceHandle = reader.unsigned64();
  result.attributeHandle = reader.unsigned64();
  reader.finish();
  if (result.federationName.empty()) {
    throw ProcessFederationServiceProtocolError(
        "A process attribute transportation-type query requires a federation name.");
  }
  requireNonzero(
      result.requestingFederateId,
      "A process attribute transportation-type query requires a federate identity.");
  requireNonzero(
      result.objectInstanceHandle,
      "A process attribute transportation-type query requires an object instance.");
  requireNonzero(
      result.attributeHandle,
      "A process attribute transportation-type query requires an attribute.");
  return result;
}



std::vector<std::uint8_t>
encodeProcessFederationRequestInteractionTransportationTypeChangeRequest(
    ProcessFederationRequestInteractionTransportationTypeChangeRequest const& request) {
  if (request.federationName.empty()) {
    throw ProcessFederationServiceProtocolError(
        "A process interaction transportation-type change requires a federation name.");
  }
  requireNonzero(
      request.requestingFederateId,
      "A process interaction transportation-type change requires a federate identity.");
  requireNonzero(
      request.interactionClassHandle,
      "A process interaction transportation-type change requires an interaction class.");
  requireNonzero(
      request.transportationTypeHandle,
      "A process interaction transportation-type change requires a transportation type.");
  PayloadWriter writer;
  writer.wideString(request.federationName);
  writer.unsigned64(request.requestingFederateId);
  writer.unsigned64(request.interactionClassHandle);
  writer.unsigned64(request.transportationTypeHandle);
  return std::move(writer).finish();
}

ProcessFederationRequestInteractionTransportationTypeChangeRequest
decodeProcessFederationRequestInteractionTransportationTypeChangeRequest(
    std::span<std::uint8_t const> encoded) {
  PayloadReader reader(encoded);
  ProcessFederationRequestInteractionTransportationTypeChangeRequest result;
  result.federationName = reader.wideString();
  result.requestingFederateId = reader.unsigned64();
  result.interactionClassHandle = reader.unsigned64();
  result.transportationTypeHandle = reader.unsigned64();
  reader.finish();
  if (result.federationName.empty()) {
    throw ProcessFederationServiceProtocolError(
        "A process interaction transportation-type change requires a federation name.");
  }
  requireNonzero(
      result.requestingFederateId,
      "A process interaction transportation-type change requires a federate identity.");
  requireNonzero(
      result.interactionClassHandle,
      "A process interaction transportation-type change requires an interaction class.");
  requireNonzero(
      result.transportationTypeHandle,
      "A process interaction transportation-type change requires a transportation type.");
  return result;
}

std::vector<std::uint8_t>
encodeProcessFederationQueryInteractionTransportationTypeRequest(
    ProcessFederationQueryInteractionTransportationTypeRequest const& request) {
  if (request.federationName.empty()) {
    throw ProcessFederationServiceProtocolError(
        "A process interaction transportation-type query requires a federation name.");
  }
  requireNonzero(
      request.requestingFederateId,
      "A process interaction transportation-type query requires a requester identity.");
  requireNonzero(
      request.queriedFederateId,
      "A process interaction transportation-type query requires a queried federate identity.");
  requireNonzero(
      request.interactionClassHandle,
      "A process interaction transportation-type query requires an interaction class.");
  PayloadWriter writer;
  writer.wideString(request.federationName);
  writer.unsigned64(request.requestingFederateId);
  writer.unsigned64(request.queriedFederateId);
  writer.unsigned64(request.interactionClassHandle);
  return std::move(writer).finish();
}

ProcessFederationQueryInteractionTransportationTypeRequest
decodeProcessFederationQueryInteractionTransportationTypeRequest(
    std::span<std::uint8_t const> encoded) {
  PayloadReader reader(encoded);
  ProcessFederationQueryInteractionTransportationTypeRequest result;
  result.federationName = reader.wideString();
  result.requestingFederateId = reader.unsigned64();
  result.queriedFederateId = reader.unsigned64();
  result.interactionClassHandle = reader.unsigned64();
  reader.finish();
  if (result.federationName.empty()) {
    throw ProcessFederationServiceProtocolError(
        "A process interaction transportation-type query requires a federation name.");
  }
  requireNonzero(
      result.requestingFederateId,
      "A process interaction transportation-type query requires a requester identity.");
  requireNonzero(
      result.queriedFederateId,
      "A process interaction transportation-type query requires a queried federate identity.");
  requireNonzero(
      result.interactionClassHandle,
      "A process interaction transportation-type query requires an interaction class.");
  return result;
}


// Interaction delivery and logical-time request codecs.

std::vector<std::uint8_t> encodeProcessFederationSendInteractionRequest(
    ProcessFederationSendInteractionRequest const& request) {
  if (request.federationName.empty()) {
    throw ProcessFederationServiceProtocolError(
        "A process federation interaction request requires a federation name.");
  }
  requireNonzero(
      request.producingFederateId,
      "A process federation interaction request requires a producer identity.");
  requireNonzero(
      request.interactionClassHandle,
      "A process federation interaction request requires an interaction class.");
  PayloadWriter writer;
  writer.wideString(request.federationName);
  writer.unsigned64(request.producingFederateId);
  writer.unsigned64(request.interactionClassHandle);
  writer.unsigned64Vector(request.sentParameterHandles);
  writer.bytes(request.payload);
  writeOptionalLogicalTime(writer, request.timestamp);
  if (request.sentRegionHandles.has_value()) {
    writer.unsigned8(1U);
    writer.unsigned64Vector(parameterVector(*request.sentRegionHandles));
  }
  return std::move(writer).finish();
}

ProcessFederationSendInteractionRequest
decodeProcessFederationSendInteractionRequest(
    std::span<std::uint8_t const> encoded) {
  PayloadReader reader(encoded);
  ProcessFederationSendInteractionRequest result;
  result.federationName = reader.wideString();
  result.producingFederateId = reader.unsigned64();
  result.interactionClassHandle = reader.unsigned64();
  result.sentParameterHandles = reader.unsigned64Vector();
  result.payload = reader.bytes();
  result.timestamp = readOptionalLogicalTime(reader);
  if (reader.remaining() != 0U) {
    auto const hasSentRegions = reader.unsigned8();
    if (hasSentRegions > 1U) {
      throw ProcessFederationServiceProtocolError(
          "A process federation interaction request has an invalid region marker.");
    }
    if (hasSentRegions != 0U) {
      auto regionVector = reader.unsigned64Vector();
      validateHandleVector(
          regionVector,
          "A process federation interaction request requires sorted, unique region handles.");
      result.sentRegionHandles.emplace(
          regionVector.begin(), regionVector.end());
    }
  }
  reader.finish();
  if (result.federationName.empty()) {
    throw ProcessFederationServiceProtocolError(
        "A process federation interaction request requires a federation name.");
  }
  requireNonzero(
      result.producingFederateId,
      "A process federation interaction request requires a producer identity.");
  requireNonzero(
      result.interactionClassHandle,
      "A process federation interaction request requires an interaction class.");
  return result;
}

std::vector<std::uint8_t>
encodeProcessFederationSendDirectedInteractionRequest(
    ProcessFederationSendDirectedInteractionRequest const& request) {
  if (request.federationName.empty()) {
    throw ProcessFederationServiceProtocolError(
        "A process directed-interaction request requires a federation name.");
  }
  requireNonzero(
      request.producingFederateId,
      "A process directed-interaction request requires a producer identity.");
  requireNonzero(
      request.objectInstanceHandle,
      "A process directed-interaction request requires a target object instance.");
  requireNonzero(
      request.interactionClassHandle,
      "A process directed-interaction request requires an interaction class.");
  PayloadWriter writer;
  writer.wideString(request.federationName);
  writer.unsigned64(request.producingFederateId);
  writer.unsigned64(request.objectInstanceHandle);
  writer.unsigned64(request.interactionClassHandle);
  writer.unsigned64Vector(request.sentParameterHandles);
  writer.bytes(request.payload);
  writeOptionalLogicalTime(writer, request.timestamp);
  return std::move(writer).finish();
}

ProcessFederationSendDirectedInteractionRequest
decodeProcessFederationSendDirectedInteractionRequest(
    std::span<std::uint8_t const> encoded) {
  PayloadReader reader(encoded);
  ProcessFederationSendDirectedInteractionRequest result;
  result.federationName = reader.wideString();
  result.producingFederateId = reader.unsigned64();
  result.objectInstanceHandle = reader.unsigned64();
  result.interactionClassHandle = reader.unsigned64();
  result.sentParameterHandles = reader.unsigned64Vector();
  result.payload = reader.bytes();
  result.timestamp = readOptionalLogicalTime(reader);
  reader.finish();
  if (result.federationName.empty()) {
    throw ProcessFederationServiceProtocolError(
        "A process directed-interaction request requires a federation name.");
  }
  requireNonzero(
      result.producingFederateId,
      "A process directed-interaction request requires a producer identity.");
  requireNonzero(
      result.objectInstanceHandle,
      "A process directed-interaction request requires a target object instance.");
  requireNonzero(
      result.interactionClassHandle,
      "A process directed-interaction request requires an interaction class.");
  return result;
}

std::vector<std::uint8_t> encodeProcessFederationRetractRequest(
    ProcessFederationRetractRequest const& request) {
  if (request.federationName.empty()) {
    throw ProcessFederationServiceProtocolError(
        "A process retraction request requires a federation name.");
  }
  requireNonzero(
      request.producingFederateId,
      "A process retraction request requires a producer identity.");
  requireNonzero(
      request.messageId,
      "A process retraction request requires a message identity.");
  PayloadWriter writer;
  writer.wideString(request.federationName);
  writer.unsigned64(request.producingFederateId);
  writer.unsigned64(request.messageId);
  return std::move(writer).finish();
}

ProcessFederationRetractRequest decodeProcessFederationRetractRequest(
    std::span<std::uint8_t const> encoded) {
  PayloadReader reader(encoded);
  ProcessFederationRetractRequest result;
  result.federationName = reader.wideString();
  result.producingFederateId = reader.unsigned64();
  result.messageId = reader.unsigned64();
  reader.finish();
  if (result.federationName.empty()) {
    throw ProcessFederationServiceProtocolError(
        "A process retraction request requires a federation name.");
  }
  requireNonzero(
      result.producingFederateId,
      "A process retraction request requires a producer identity.");
  requireNonzero(
      result.messageId,
      "A process retraction request requires a message identity.");
  return result;
}

std::vector<std::uint8_t> encodeProcessFederationReceiveInteractionRequest(
    ProcessFederationReceiveInteractionRequest const& request) {
  if (request.federationName.empty()) {
    throw ProcessFederationServiceProtocolError(
        "A process federation receive request requires a federation name.");
  }
  requireNonzero(
      request.receivingFederateId,
      "A process federation receive request requires a recipient identity.");
  PayloadWriter writer;
  writer.wideString(request.federationName);
  writer.unsigned64(request.receivingFederateId);
  return std::move(writer).finish();
}

ProcessFederationReceiveInteractionRequest
decodeProcessFederationReceiveInteractionRequest(
    std::span<std::uint8_t const> encoded) {
  PayloadReader reader(encoded);
  ProcessFederationReceiveInteractionRequest result;
  result.federationName = reader.wideString();
  result.receivingFederateId = reader.unsigned64();
  reader.finish();
  if (result.federationName.empty()) {
    throw ProcessFederationServiceProtocolError(
        "A process federation receive request requires a federation name.");
  }
  requireNonzero(
      result.receivingFederateId,
      "A process federation receive request requires a recipient identity.");
  return result;
}

std::vector<std::uint8_t> encodeProcessFederationAcknowledgeTsoDeliveryRequest(
    ProcessFederationAcknowledgeTsoDeliveryRequest const& request) {
  if (request.federationName.empty()) {
    throw ProcessFederationServiceProtocolError(
        "A process TSO delivery acknowledgement requires a federation name.");
  }
  requireNonzero(
      request.receivingFederateId,
      "A process TSO delivery acknowledgement requires a recipient identity.");
  requireNonzero(
      request.messageId,
      "A process TSO delivery acknowledgement requires a message identity.");
  PayloadWriter writer;
  writer.wideString(request.federationName);
  writer.unsigned64(request.receivingFederateId);
  writer.unsigned64(request.messageId);
  return std::move(writer).finish();
}

ProcessFederationAcknowledgeTsoDeliveryRequest
decodeProcessFederationAcknowledgeTsoDeliveryRequest(
    std::span<std::uint8_t const> encoded) {
  PayloadReader reader(encoded);
  ProcessFederationAcknowledgeTsoDeliveryRequest result;
  result.federationName = reader.wideString();
  result.receivingFederateId = reader.unsigned64();
  result.messageId = reader.unsigned64();
  reader.finish();
  if (result.federationName.empty()) {
    throw ProcessFederationServiceProtocolError(
        "A process TSO delivery acknowledgement requires a federation name.");
  }
  requireNonzero(
      result.receivingFederateId,
      "A process TSO delivery acknowledgement requires a recipient identity.");
  requireNonzero(
      result.messageId,
      "A process TSO delivery acknowledgement requires a message identity.");
  return result;
}

std::vector<std::uint8_t>
encodeProcessFederationUpdateAttributeValuesRequest(
    ProcessFederationUpdateAttributeValuesRequest const& request) {
  if (request.federationName.empty()) {
    throw ProcessFederationServiceProtocolError(
        "A process federation attribute update requires a federation name.");
  }
  requireNonzero(
      request.producingFederateId,
      "A process federation attribute update requires a producer identity.");
  requireNonzero(
      request.objectInstanceHandle,
      "A process federation attribute update requires an object instance.");
  if (request.attributeValues.empty()) {
    throw ProcessFederationServiceProtocolError(
        "A process federation attribute update requires at least one attribute.");
  }
  if (request.attributeValues.size() >
      std::numeric_limits<std::uint32_t>::max()) {
    throw ProcessFederationServiceProtocolError(
        "A process federation attribute update has too many attributes.");
  }
  PayloadWriter writer;
  writer.wideString(request.federationName);
  writer.unsigned64(request.producingFederateId);
  writer.unsigned64(request.objectInstanceHandle);
  writer.unsigned32(static_cast<std::uint32_t>(request.attributeValues.size()));
  std::set<std::uint64_t> seenHandles;
  for (auto const& [attributeHandle, value] : request.attributeValues) {
    requireNonzero(
        attributeHandle,
        "A process federation attribute update requires attribute identities.");
    if (!seenHandles.insert(attributeHandle).second) {
      throw ProcessFederationServiceProtocolError(
          "A process federation attribute update repeats an attribute identity.");
    }
    writer.unsigned64(attributeHandle);
    writer.bytes(value);
  }
  writer.bytes(request.userSuppliedTag);
  writeOptionalLogicalTime(writer, request.timestamp);
  return std::move(writer).finish();
}

ProcessFederationUpdateAttributeValuesRequest
decodeProcessFederationUpdateAttributeValuesRequest(
    std::span<std::uint8_t const> encoded) {
  PayloadReader reader(encoded);
  ProcessFederationUpdateAttributeValuesRequest result;
  result.federationName = reader.wideString();
  result.producingFederateId = reader.unsigned64();
  result.objectInstanceHandle = reader.unsigned64();
  auto const attributeCount = reader.unsigned32();
  result.attributeValues.reserve(attributeCount);
  std::set<std::uint64_t> seenHandles;
  for (std::uint32_t index = 0U; index < attributeCount; ++index) {
    auto const attributeHandle = reader.unsigned64();
    requireNonzero(
        attributeHandle,
        "A process federation attribute update requires attribute identities.");
    if (!seenHandles.insert(attributeHandle).second) {
      throw ProcessFederationServiceProtocolError(
          "A process federation attribute update repeats an attribute identity.");
    }
    result.attributeValues.emplace_back(attributeHandle, reader.bytes());
  }
  result.userSuppliedTag = reader.bytes();
  result.timestamp = readOptionalLogicalTime(reader);
  reader.finish();
  if (result.federationName.empty()) {
    throw ProcessFederationServiceProtocolError(
        "A process federation attribute update requires a federation name.");
  }
  requireNonzero(
      result.producingFederateId,
      "A process federation attribute update requires a producer identity.");
  requireNonzero(
      result.objectInstanceHandle,
      "A process federation attribute update requires an object instance.");
  if (result.attributeValues.empty()) {
    throw ProcessFederationServiceProtocolError(
        "A process federation attribute update requires at least one attribute.");
  }
  return result;
}

std::vector<std::uint8_t>
encodeProcessFederationRequestAttributeValueUpdateRequest(
    ProcessFederationRequestAttributeValueUpdateRequest const& request) {
  if (request.federationName.empty()) {
    throw ProcessFederationServiceProtocolError(
        "A process Request Attribute Value Update requires a federation name.");
  }
  requireNonzero(
      request.requestingFederateId,
      "A process Request Attribute Value Update requires a requester identity.");
  requireNonzero(
      request.objectInstanceHandle,
      "A process Request Attribute Value Update requires an object instance.");
  if (request.requestedAttributeHandles.empty()) {
    throw ProcessFederationServiceProtocolError(
        "A process Request Attribute Value Update requires at least one attribute.");
  }
  if (request.requestedAttributeHandles.size() >
      std::numeric_limits<std::uint32_t>::max()) {
    throw ProcessFederationServiceProtocolError(
        "A process Request Attribute Value Update has too many attributes.");
  }
  PayloadWriter writer;
  writer.wideString(request.federationName);
  writer.unsigned64(request.requestingFederateId);
  writer.unsigned64(request.objectInstanceHandle);
  writer.unsigned32(
      static_cast<std::uint32_t>(request.requestedAttributeHandles.size()));
  std::set<std::uint64_t> seenHandles;
  for (std::uint64_t const attributeHandle :
       request.requestedAttributeHandles) {
    requireNonzero(
        attributeHandle,
        "A process Request Attribute Value Update requires attribute identities.");
    if (!seenHandles.insert(attributeHandle).second) {
      throw ProcessFederationServiceProtocolError(
          "A process Request Attribute Value Update repeats an attribute identity.");
    }
    writer.unsigned64(attributeHandle);
  }
  writer.bytes(request.userSuppliedTag);
  return std::move(writer).finish();
}

ProcessFederationRequestAttributeValueUpdateRequest
decodeProcessFederationRequestAttributeValueUpdateRequest(
    std::span<std::uint8_t const> encoded) {
  PayloadReader reader(encoded);
  ProcessFederationRequestAttributeValueUpdateRequest result;
  result.federationName = reader.wideString();
  result.requestingFederateId = reader.unsigned64();
  result.objectInstanceHandle = reader.unsigned64();
  auto const attributeCount = reader.unsigned32();
  result.requestedAttributeHandles.reserve(attributeCount);
  std::set<std::uint64_t> seenHandles;
  for (std::uint32_t index = 0U; index < attributeCount; ++index) {
    auto const attributeHandle = reader.unsigned64();
    requireNonzero(
        attributeHandle,
        "A process Request Attribute Value Update requires attribute identities.");
    if (!seenHandles.insert(attributeHandle).second) {
      throw ProcessFederationServiceProtocolError(
          "A process Request Attribute Value Update repeats an attribute identity.");
    }
    result.requestedAttributeHandles.push_back(attributeHandle);
  }
  result.userSuppliedTag = reader.bytes();
  reader.finish();
  if (result.federationName.empty()) {
    throw ProcessFederationServiceProtocolError(
        "A process Request Attribute Value Update requires a federation name.");
  }
  requireNonzero(
      result.requestingFederateId,
      "A process Request Attribute Value Update requires a requester identity.");
  requireNonzero(
      result.objectInstanceHandle,
      "A process Request Attribute Value Update requires an object instance.");
  if (result.requestedAttributeHandles.empty()) {
    throw ProcessFederationServiceProtocolError(
        "A process Request Attribute Value Update requires at least one attribute.");
  }
  return result;
}

std::vector<std::uint8_t>
encodeProcessFederationRequestAttributeValueUpdateClassRequest(
    ProcessFederationRequestAttributeValueUpdateClassRequest const& request) {
  if (request.federationName.empty()) {
    throw ProcessFederationServiceProtocolError(
        "A process class Request Attribute Value Update requires a federation name.");
  }
  requireNonzero(
      request.requestingFederateId,
      "A process class Request Attribute Value Update requires a requester identity.");
  requireNonzero(
      request.objectClassHandle,
      "A process class Request Attribute Value Update requires an object class.");
  if (request.requestedAttributeHandles.empty()) {
    throw ProcessFederationServiceProtocolError(
        "A process class Request Attribute Value Update requires at least one attribute.");
  }
  if (request.requestedAttributeHandles.size() >
      std::numeric_limits<std::uint32_t>::max()) {
    throw ProcessFederationServiceProtocolError(
        "A process class Request Attribute Value Update has too many attributes.");
  }
  PayloadWriter writer;
  writer.wideString(request.federationName);
  writer.unsigned64(request.requestingFederateId);
  writer.unsigned64(request.objectClassHandle);
  writer.unsigned32(
      static_cast<std::uint32_t>(request.requestedAttributeHandles.size()));
  std::set<std::uint64_t> seenHandles;
  for (std::uint64_t const attributeHandle :
       request.requestedAttributeHandles) {
    requireNonzero(
        attributeHandle,
        "A process class Request Attribute Value Update requires attribute identities.");
    if (!seenHandles.insert(attributeHandle).second) {
      throw ProcessFederationServiceProtocolError(
          "A process class Request Attribute Value Update repeats an attribute identity.");
    }
    writer.unsigned64(attributeHandle);
  }
  writer.bytes(request.userSuppliedTag);
  return std::move(writer).finish();
}

ProcessFederationRequestAttributeValueUpdateClassRequest
decodeProcessFederationRequestAttributeValueUpdateClassRequest(
    std::span<std::uint8_t const> encoded) {
  PayloadReader reader(encoded);
  ProcessFederationRequestAttributeValueUpdateClassRequest result;
  result.federationName = reader.wideString();
  result.requestingFederateId = reader.unsigned64();
  result.objectClassHandle = reader.unsigned64();
  auto const attributeCount = reader.unsigned32();
  result.requestedAttributeHandles.reserve(attributeCount);
  std::set<std::uint64_t> seenHandles;
  for (std::uint32_t index = 0U; index < attributeCount; ++index) {
    auto const attributeHandle = reader.unsigned64();
    requireNonzero(
        attributeHandle,
        "A process class Request Attribute Value Update requires attribute identities.");
    if (!seenHandles.insert(attributeHandle).second) {
      throw ProcessFederationServiceProtocolError(
          "A process class Request Attribute Value Update repeats an attribute identity.");
    }
    result.requestedAttributeHandles.push_back(attributeHandle);
  }
  result.userSuppliedTag = reader.bytes();
  reader.finish();
  if (result.federationName.empty()) {
    throw ProcessFederationServiceProtocolError(
        "A process class Request Attribute Value Update requires a federation name.");
  }
  requireNonzero(
      result.requestingFederateId,
      "A process class Request Attribute Value Update requires a requester identity.");
  requireNonzero(
      result.objectClassHandle,
      "A process class Request Attribute Value Update requires an object class.");
  if (result.requestedAttributeHandles.empty()) {
    throw ProcessFederationServiceProtocolError(
        "A process class Request Attribute Value Update requires at least one attribute.");
  }
  return result;
}

std::vector<std::uint8_t>
encodeProcessFederationRequestAttributeValueUpdateClassWithRegionsRequest(
    ProcessFederationRequestAttributeValueUpdateClassWithRegionsRequest const& request) {
  if (request.federationName.empty()) {
    throw ProcessFederationServiceProtocolError(
        "A process regional class Request Attribute Value Update requires a federation name.");
  }
  requireNonzero(
      request.requestingFederateId,
      "A process regional class Request Attribute Value Update requires a requester identity.");
  requireNonzero(
      request.objectClassHandle,
      "A process regional class Request Attribute Value Update requires an object class.");
  if (request.requestedAttributeHandles.empty()) {
    throw ProcessFederationServiceProtocolError(
        "A process regional class Request Attribute Value Update requires at least one attribute.");
  }
  if (request.requestedAttributeHandles.size() >
      std::numeric_limits<std::uint32_t>::max()) {
    throw ProcessFederationServiceProtocolError(
        "A process regional class Request Attribute Value Update has too many attributes.");
  }
  std::set<std::uint64_t> seenHandles;
  for (std::uint64_t const attributeHandle :
       request.requestedAttributeHandles) {
    requireNonzero(
        attributeHandle,
        "A process regional class Request Attribute Value Update requires attribute identities.");
    if (!seenHandles.insert(attributeHandle).second) {
      throw ProcessFederationServiceProtocolError(
          "A process regional class Request Attribute Value Update repeats an attribute identity.");
    }
  }
  if (request.requestRegionsByAttribute.empty() ||
      request.requestRegionsByAttribute.size() != seenHandles.size()) {
    throw ProcessFederationServiceProtocolError(
        "A process regional class Request Attribute Value Update requires one region set per attribute.");
  }
  for (auto const& [attributeHandle, regionHandles] :
       request.requestRegionsByAttribute) {
    requireNonzero(
        attributeHandle,
        "A process regional class Request Attribute Value Update requires region-map attribute identities.");
    if (!seenHandles.contains(attributeHandle) || regionHandles.empty()) {
      throw ProcessFederationServiceProtocolError(
          "A process regional class Request Attribute Value Update has an invalid attribute-region pair.");
    }
    for (std::uint64_t const regionHandle : regionHandles) {
      requireNonzero(
          regionHandle,
          "A process regional class Request Attribute Value Update requires region identities.");
    }
  }
  PayloadWriter writer;
  writer.wideString(request.federationName);
  writer.unsigned64(request.requestingFederateId);
  writer.unsigned64(request.objectClassHandle);
  writer.unsigned32(
      static_cast<std::uint32_t>(request.requestedAttributeHandles.size()));
  for (std::uint64_t const attributeHandle :
       request.requestedAttributeHandles) {
    writer.unsigned64(attributeHandle);
  }
  writeAttributeRegionMap(writer, request.requestRegionsByAttribute);
  writer.bytes(request.userSuppliedTag);
  return std::move(writer).finish();
}

ProcessFederationRequestAttributeValueUpdateClassWithRegionsRequest
decodeProcessFederationRequestAttributeValueUpdateClassWithRegionsRequest(
    std::span<std::uint8_t const> encoded) {
  PayloadReader reader(encoded);
  ProcessFederationRequestAttributeValueUpdateClassWithRegionsRequest result;
  result.federationName = reader.wideString();
  result.requestingFederateId = reader.unsigned64();
  result.objectClassHandle = reader.unsigned64();
  auto const attributeCount = reader.unsigned32();
  result.requestedAttributeHandles.reserve(attributeCount);
  std::set<std::uint64_t> seenHandles;
  for (std::uint32_t index = 0U; index < attributeCount; ++index) {
    auto const attributeHandle = reader.unsigned64();
    requireNonzero(
        attributeHandle,
        "A process regional class Request Attribute Value Update requires attribute identities.");
    if (!seenHandles.insert(attributeHandle).second) {
      throw ProcessFederationServiceProtocolError(
          "A process regional class Request Attribute Value Update repeats an attribute identity.");
    }
    result.requestedAttributeHandles.push_back(attributeHandle);
  }
  result.requestRegionsByAttribute = readAttributeRegionMap(reader);
  result.userSuppliedTag = reader.bytes();
  reader.finish();
  if (result.federationName.empty()) {
    throw ProcessFederationServiceProtocolError(
        "A process regional class Request Attribute Value Update requires a federation name.");
  }
  requireNonzero(
      result.requestingFederateId,
      "A process regional class Request Attribute Value Update requires a requester identity.");
  requireNonzero(
      result.objectClassHandle,
      "A process regional class Request Attribute Value Update requires an object class.");
  if (seenHandles.empty() ||
      result.requestRegionsByAttribute.empty() ||
      result.requestRegionsByAttribute.size() != seenHandles.size()) {
    throw ProcessFederationServiceProtocolError(
        "A process regional class Request Attribute Value Update requires one region set per attribute.");
  }
  for (auto const& [attributeHandle, regionHandles] :
       result.requestRegionsByAttribute) {
    if (!seenHandles.contains(attributeHandle) || regionHandles.empty()) {
      throw ProcessFederationServiceProtocolError(
          "A process regional class Request Attribute Value Update has an invalid attribute-region pair.");
    }
  }
  return result;
}


std::vector<std::uint8_t> encodeProcessFederationJoinResult(
    ProcessFederationJoinResult const& result) {
  requireNonzero(
      result.federateId,
      "A process federation join result requires a federate identity.");
  if (result.federateName.empty()) {
    throw ProcessFederationServiceProtocolError(
        "A process federation join result requires a federate name.");
  }
  PayloadWriter writer;
  writer.unsigned64(result.federateId);
  writer.wideString(result.federateName);
  // This is an optional wire suffix for compatibility with older private
  // endpoints. New services always populate it from the committed
  // federation definition; an empty value remains decodable for legacy test
  // fixtures and is rejected by the public time-factory path below.
  if (!result.logicalTimeImplementationName.empty()) {
    writer.wideString(result.logicalTimeImplementationName);
  }
  // A nonempty report path is a second, tagged suffix.  The tag keeps the
  // existing one-string logical-time extension unambiguous for older private
  // clients while allowing the server to publish the immutable MOM path.
  if (!result.reportServiceFile.empty()) {
    writer.unsigned8(1U);
    writer.wideString(result.reportServiceFile);
  }
  return std::move(writer).finish();
}

ProcessFederationJoinResult decodeProcessFederationJoinResult(
    std::span<std::uint8_t const> encoded) {
  PayloadReader reader(encoded);
  ProcessFederationJoinResult result;
  result.federateId = reader.unsigned64();
  result.federateName = reader.wideString();
  if (reader.remaining() != 0U) {
    result.logicalTimeImplementationName = reader.wideString();
  }
  if (reader.remaining() != 0U) {
    if (reader.unsigned8() != 1U || reader.remaining() == 0U) {
      throw ProcessFederationServiceProtocolError(
          "A process federation join result has an invalid report-file suffix.");
    }
    result.reportServiceFile = reader.wideString();
    if (result.reportServiceFile.empty()) {
      throw ProcessFederationServiceProtocolError(
          "A process federation join result has an empty report-file location.");
    }
  }
  reader.finish();
  requireNonzero(
      result.federateId,
      "A process federation join result requires a federate identity.");
  if (result.federateName.empty()) {
    throw ProcessFederationServiceProtocolError(
        "A process federation join result requires a federate name.");
  }
  return result;
}

std::vector<std::uint8_t> encodeProcessFederationSendInteractionResult(
    ProcessFederationSendInteractionResult const& result) {
  if (result.messageId == std::numeric_limits<std::uint64_t>::max()) {
    throw ProcessFederationServiceProtocolError(
        "A process federation interaction result has an invalid message identity.");
  }
  PayloadWriter writer;
  writer.unsigned32(result.recipientCount);
  // The trailing identity is optional on decode so older private peers that
  // only returned a recipient count remain wire-compatible.
  writer.unsigned64(result.messageId);
  return std::move(writer).finish();
}

ProcessFederationSendInteractionResult
decodeProcessFederationSendInteractionResult(
    std::span<std::uint8_t const> encoded) {
  PayloadReader reader(encoded);
  ProcessFederationSendInteractionResult result;
  result.recipientCount = reader.unsigned32();
  if (reader.remaining() != 0U) {
    result.messageId = reader.unsigned64();
  }
  reader.finish();
  return result;
}

std::vector<std::uint8_t> encodeProcessFederationRetractResult(
    ProcessFederationRetractResult const& result) {
  if (result.status > ProcessFederationRetractStatus::time_regulation_not_enabled) {
    throw ProcessFederationServiceProtocolError(
        "A process retraction result has an invalid status.");
  }
  PayloadWriter writer;
  writer.unsigned8(static_cast<std::uint8_t>(result.status));
  return std::move(writer).finish();
}

ProcessFederationRetractResult decodeProcessFederationRetractResult(
    std::span<std::uint8_t const> encoded) {
  PayloadReader reader(encoded);
  auto const status = reader.unsigned8();
  reader.finish();
  if (status > static_cast<std::uint8_t>(
                   ProcessFederationRetractStatus::time_regulation_not_enabled)) {
    throw ProcessFederationServiceProtocolError(
        "A process retraction result has an invalid status.");
  }
  return ProcessFederationRetractResult{
      static_cast<ProcessFederationRetractStatus>(status)};
}

std::vector<std::uint8_t>
encodeProcessFederationTsoDeliveryAcknowledgementResult(
    ProcessFederationTsoDeliveryAcknowledgementResult const& result) {
  if (result.status >
      ProcessFederationTsoDeliveryAcknowledgementStatus::not_in_transit) {
    throw ProcessFederationServiceProtocolError(
        "A process TSO delivery acknowledgement result has an invalid status.");
  }
  PayloadWriter writer;
  writer.unsigned8(static_cast<std::uint8_t>(result.status));
  return std::move(writer).finish();
}

ProcessFederationTsoDeliveryAcknowledgementResult
decodeProcessFederationTsoDeliveryAcknowledgementResult(
    std::span<std::uint8_t const> encoded) {
  PayloadReader reader(encoded);
  auto const status = reader.unsigned8();
  reader.finish();
  if (status > static_cast<std::uint8_t>(
                   ProcessFederationTsoDeliveryAcknowledgementStatus::
                       not_in_transit)) {
    throw ProcessFederationServiceProtocolError(
        "A process TSO delivery acknowledgement result has an invalid status.");
  }
  return ProcessFederationTsoDeliveryAcknowledgementResult{
      static_cast<ProcessFederationTsoDeliveryAcknowledgementStatus>(status)};
}



std::vector<std::uint8_t> encodeProcessFederationRequestRetractionEvent(
    ProcessFederationRequestRetractionEvent const& event) {
  requireNonzero(
      event.messageId,
      "A process retraction event requires a message identity.");
  PayloadWriter writer;
  writer.unsigned64(event.messageId);
  return std::move(writer).finish();
}

ProcessFederationRequestRetractionEvent
decodeProcessFederationRequestRetractionEvent(
    std::span<std::uint8_t const> encoded) {
  PayloadReader reader(encoded);
  ProcessFederationRequestRetractionEvent result{reader.unsigned64()};
  reader.finish();
  requireNonzero(
      result.messageId,
      "A process retraction event requires a message identity.");
  return result;
}

std::vector<std::uint8_t> encodeProcessFederationHandleResult(
    ProcessFederationHandleResult const& result) {
  requireNonzero(
      result.handle,
      "A process federation handle result requires a non-zero handle.");
  PayloadWriter writer;
  writer.unsigned64(result.handle);
  return std::move(writer).finish();
}

ProcessFederationHandleResult decodeProcessFederationHandleResult(
    std::span<std::uint8_t const> encoded) {
  PayloadReader reader(encoded);
  ProcessFederationHandleResult result{reader.unsigned64()};
  reader.finish();
  requireNonzero(
      result.handle,
      "A process federation handle result requires a non-zero handle.");
  return result;
}

std::vector<std::uint8_t> encodeProcessFederationStringResult(
    ProcessFederationStringResult const& result) {
  if (result.value.empty()) {
    throw ProcessFederationServiceProtocolError(
        "A process federation string result requires a non-empty value.");
  }
  PayloadWriter writer;
  writer.wideString(result.value);
  return std::move(writer).finish();
}

ProcessFederationStringResult decodeProcessFederationStringResult(
    std::span<std::uint8_t const> encoded) {
  PayloadReader reader(encoded);
  ProcessFederationStringResult result;
  result.value = reader.wideString();
  reader.finish();
  if (result.value.empty()) {
    throw ProcessFederationServiceProtocolError(
        "A process federation string result requires a non-empty value.");
  }
  return result;
}

std::vector<std::uint8_t>
encodeProcessFederationAttributeScopeAdvisorySwitchRequest(
    ProcessFederationAttributeScopeAdvisorySwitchRequest const& request) {
  if (request.federationName.empty() || request.federateId == 0U) {
    throw ProcessFederationServiceProtocolError(
        "A process attribute-scope switch request requires federation and federate identities.");
  }
  PayloadWriter writer;
  writer.wideString(request.federationName);
  writer.unsigned64(request.federateId);
  writer.unsigned8(request.switchValue ? 1U : 0U);
  return std::move(writer).finish();
}

ProcessFederationAttributeScopeAdvisorySwitchRequest
decodeProcessFederationAttributeScopeAdvisorySwitchRequest(
    std::span<std::uint8_t const> encoded) {
  PayloadReader reader(encoded);
  ProcessFederationAttributeScopeAdvisorySwitchRequest request;
  request.federationName = reader.wideString();
  request.federateId = reader.unsigned64();
  auto const switchValue = reader.unsigned8();
  if (switchValue > 1U) {
    throw ProcessFederationServiceProtocolError(
        "A process attribute-scope switch request has an invalid boolean value.");
  }
  request.switchValue = switchValue != 0U;
  reader.finish();
  if (request.federationName.empty() || request.federateId == 0U) {
    throw ProcessFederationServiceProtocolError(
        "A process attribute-scope switch request requires federation and federate identities.");
  }
  return request;
}

std::vector<std::uint8_t>
encodeProcessFederationInteractionOrderTypeChangeResult(
    ProcessFederationInteractionOrderTypeChangeResult const& result) {
  auto const encoded = static_cast<std::uint8_t>(result.status);
  if (encoded > static_cast<std::uint8_t>(
                    InteractionOrderTypeChangeStatus::invalid_order_type)) {
    throw ProcessFederationServiceProtocolError(
        "A process interaction order-type change result has an invalid status.");
  }
  PayloadWriter writer;
  writer.unsigned8(encoded);
  return std::move(writer).finish();
}

ProcessFederationInteractionOrderTypeChangeResult
decodeProcessFederationInteractionOrderTypeChangeResult(
    std::span<std::uint8_t const> encoded) {
  PayloadReader reader(encoded);
  auto const status = reader.unsigned8();
  reader.finish();
  if (status > static_cast<std::uint8_t>(
                   InteractionOrderTypeChangeStatus::invalid_order_type)) {
    throw ProcessFederationServiceProtocolError(
        "A process interaction order-type change result has an invalid status.");
  }
  return {static_cast<InteractionOrderTypeChangeStatus>(status)};
}

std::vector<std::uint8_t>
encodeProcessFederationAttributeOrderTypeChangeResult(
    ProcessFederationAttributeOrderTypeChangeResult const& result) {
  auto const encoded = static_cast<std::uint8_t>(result.status);
  if (encoded > static_cast<std::uint8_t>(
                    AttributeOrderTypeChangeStatus::invalid_order_type)) {
    throw ProcessFederationServiceProtocolError(
        "A process attribute order-type change result has an invalid status.");
  }
  PayloadWriter writer;
  writer.unsigned8(encoded);
  return std::move(writer).finish();
}

ProcessFederationAttributeOrderTypeChangeResult
decodeProcessFederationAttributeOrderTypeChangeResult(
    std::span<std::uint8_t const> encoded) {
  PayloadReader reader(encoded);
  auto const status = reader.unsigned8();
  reader.finish();
  if (status > static_cast<std::uint8_t>(
                   AttributeOrderTypeChangeStatus::invalid_order_type)) {
    throw ProcessFederationServiceProtocolError(
        "A process attribute order-type change result has an invalid status.");
  }
  return {static_cast<AttributeOrderTypeChangeStatus>(status)};
}

std::vector<std::uint8_t>
encodeProcessFederationAttributeOrderTypeDefaultResult(
    ProcessFederationAttributeOrderTypeDefaultResult const& result) {
  auto const encoded = static_cast<std::uint8_t>(result.status);
  if (encoded > static_cast<std::uint8_t>(
                    AttributeOrderTypeDefaultStatus::invalid_order_type)) {
    throw ProcessFederationServiceProtocolError(
        "A process default attribute order-type result has an invalid status.");
  }
  PayloadWriter writer;
  writer.unsigned8(encoded);
  return std::move(writer).finish();
}

ProcessFederationAttributeOrderTypeDefaultResult
decodeProcessFederationAttributeOrderTypeDefaultResult(
    std::span<std::uint8_t const> encoded) {
  PayloadReader reader(encoded);
  auto const status = reader.unsigned8();
  reader.finish();
  if (status > static_cast<std::uint8_t>(
                   AttributeOrderTypeDefaultStatus::invalid_order_type)) {
    throw ProcessFederationServiceProtocolError(
        "A process default attribute order-type result has an invalid status.");
  }
  return {static_cast<AttributeOrderTypeDefaultStatus>(status)};
}

std::vector<std::uint8_t>
encodeProcessFederationAttributeTransportationTypeDefaultResult(
    ProcessFederationAttributeTransportationTypeDefaultResult const& result) {
  auto const encoded = static_cast<std::uint8_t>(result.status);
  if (encoded > static_cast<std::uint8_t>(
                    AttributeTransportationTypeDefaultStatus::inconsistent_catalog)) {
    throw ProcessFederationServiceProtocolError(
        "A process default attribute transportation-type result has an invalid status.");
  }
  PayloadWriter writer;
  writer.unsigned8(encoded);
  return std::move(writer).finish();
}

ProcessFederationAttributeTransportationTypeDefaultResult
decodeProcessFederationAttributeTransportationTypeDefaultResult(
    std::span<std::uint8_t const> encoded) {
  PayloadReader reader(encoded);
  auto const status = reader.unsigned8();
  reader.finish();
  if (status > static_cast<std::uint8_t>(
                   AttributeTransportationTypeDefaultStatus::inconsistent_catalog)) {
    throw ProcessFederationServiceProtocolError(
        "A process default attribute transportation-type result has an invalid status.");
  }
  return {static_cast<AttributeTransportationTypeDefaultStatus>(status)};
}

std::vector<std::uint8_t>
encodeProcessFederationAttributeTransportationTypeChangeResult(
    ProcessFederationAttributeTransportationTypeChangeResult const& result) {
  auto const encoded = static_cast<std::uint8_t>(result.status);
  if (encoded > static_cast<std::uint8_t>(
                    AttributeTransportationTypeChangeStatus::inconsistent_catalog)) {
    throw ProcessFederationServiceProtocolError(
        "A process attribute transportation-type change result has an invalid status.");
  }
  PayloadWriter writer;
  writer.unsigned8(encoded);
  return std::move(writer).finish();
}

ProcessFederationAttributeTransportationTypeChangeResult
decodeProcessFederationAttributeTransportationTypeChangeResult(
    std::span<std::uint8_t const> encoded) {
  PayloadReader reader(encoded);
  auto const status = reader.unsigned8();
  reader.finish();
  if (status > static_cast<std::uint8_t>(
                   AttributeTransportationTypeChangeStatus::inconsistent_catalog)) {
    throw ProcessFederationServiceProtocolError(
        "A process attribute transportation-type change result has an invalid status.");
  }
  return {static_cast<AttributeTransportationTypeChangeStatus>(status)};
}

std::vector<std::uint8_t>
encodeProcessFederationAttributeTransportationTypeQueryResult(
    ProcessFederationAttributeTransportationTypeQueryResult const& result) {
  auto const encoded = static_cast<std::uint8_t>(result.status);
  if (encoded > static_cast<std::uint8_t>(
                    AttributeTransportationTypeQueryStatus::inconsistent_catalog)) {
    throw ProcessFederationServiceProtocolError(
        "A process attribute transportation-type query result has an invalid status.");
  }
  PayloadWriter writer;
  writer.unsigned8(encoded);
  return std::move(writer).finish();
}

ProcessFederationAttributeTransportationTypeQueryResult
decodeProcessFederationAttributeTransportationTypeQueryResult(
    std::span<std::uint8_t const> encoded) {
  PayloadReader reader(encoded);
  auto const status = reader.unsigned8();
  reader.finish();
  if (status > static_cast<std::uint8_t>(
                   AttributeTransportationTypeQueryStatus::inconsistent_catalog)) {
    throw ProcessFederationServiceProtocolError(
        "A process attribute transportation-type query result has an invalid status.");
  }
  return {static_cast<AttributeTransportationTypeQueryStatus>(status)};
}

std::vector<std::uint8_t>
encodeProcessFederationInteractionTransportationTypeChangeResult(
    ProcessFederationInteractionTransportationTypeChangeResult const& result) {
  auto const encoded = static_cast<std::uint8_t>(result.status);
  if (encoded > static_cast<std::uint8_t>(
                    InteractionTransportationTypeChangeStatus::inconsistent_catalog)) {
    throw ProcessFederationServiceProtocolError(
        "A process interaction transportation-type change result has an invalid status.");
  }
  PayloadWriter writer;
  writer.unsigned8(encoded);
  return std::move(writer).finish();
}

ProcessFederationInteractionTransportationTypeChangeResult
decodeProcessFederationInteractionTransportationTypeChangeResult(
    std::span<std::uint8_t const> encoded) {
  PayloadReader reader(encoded);
  auto const status = reader.unsigned8();
  reader.finish();
  if (status > static_cast<std::uint8_t>(
                   InteractionTransportationTypeChangeStatus::inconsistent_catalog)) {
    throw ProcessFederationServiceProtocolError(
        "A process interaction transportation-type change result has an invalid status.");
  }
  return {static_cast<InteractionTransportationTypeChangeStatus>(status)};
}

std::vector<std::uint8_t>
encodeProcessFederationInteractionTransportationTypeQueryResult(
    ProcessFederationInteractionTransportationTypeQueryResult const& result) {
  auto const encoded = static_cast<std::uint8_t>(result.status);
  if (encoded > static_cast<std::uint8_t>(
                    InteractionTransportationTypeQueryStatus::inconsistent_catalog)) {
    throw ProcessFederationServiceProtocolError(
        "A process interaction transportation-type query result has an invalid status.");
  }
  PayloadWriter writer;
  writer.unsigned8(encoded);
  return std::move(writer).finish();
}

ProcessFederationInteractionTransportationTypeQueryResult
decodeProcessFederationInteractionTransportationTypeQueryResult(
    std::span<std::uint8_t const> encoded) {
  PayloadReader reader(encoded);
  auto const status = reader.unsigned8();
  reader.finish();
  if (status > static_cast<std::uint8_t>(
                   InteractionTransportationTypeQueryStatus::inconsistent_catalog)) {
    throw ProcessFederationServiceProtocolError(
        "A process interaction transportation-type query result has an invalid status.");
  }
  return {static_cast<InteractionTransportationTypeQueryStatus>(status)};
}

std::vector<std::uint8_t> encodeProcessFederationUpdateRateValueResult(
    ProcessFederationUpdateRateValueResult const& result) {
  auto const encodedStatus = static_cast<std::uint8_t>(result.status);
  if (encodedStatus > static_cast<std::uint8_t>(
                          ProcessFederationUpdateRateValueStatus::
                              inconsistent_catalog)) {
    throw ProcessFederationServiceProtocolError(
        "A process update-rate result has an invalid status.");
  }
  if (!std::isfinite(result.value)) {
    throw ProcessFederationServiceProtocolError(
        "A process update-rate result must carry a finite value.");
  }
  PayloadWriter writer;
  writer.unsigned8(encodedStatus);
  writer.real64(result.value);
  return std::move(writer).finish();
}

ProcessFederationUpdateRateValueResult
decodeProcessFederationUpdateRateValueResult(
    std::span<std::uint8_t const> encoded) {
  PayloadReader reader(encoded);
  auto const encodedStatus = reader.unsigned8();
  if (encodedStatus > static_cast<std::uint8_t>(
                          ProcessFederationUpdateRateValueStatus::
                              inconsistent_catalog)) {
    throw ProcessFederationServiceProtocolError(
        "A process update-rate result has an invalid status.");
  }
  ProcessFederationUpdateRateValueResult result;
  result.status = static_cast<ProcessFederationUpdateRateValueStatus>(
      encodedStatus);
  result.value = reader.real64();
  reader.finish();
  if (!std::isfinite(result.value)) {
    throw ProcessFederationServiceProtocolError(
        "A process update-rate result must carry a finite value.");
  }
  return result;
}

std::vector<std::uint8_t> encodeProcessFederationUpdateAttributeValuesResult(
    ProcessFederationUpdateAttributeValuesResult const& result) {
  PayloadWriter writer;
  writer.unsigned32(result.recipientCount);
  // Keep the ordinary response byte-for-byte compatible. The optional field
  // is present only for an accepted timestamp-ordered message.
  if (result.messageId != 0U) {
    writer.unsigned64(result.messageId);
  }
  return std::move(writer).finish();
}

ProcessFederationUpdateAttributeValuesResult
decodeProcessFederationUpdateAttributeValuesResult(
    std::span<std::uint8_t const> encoded) {
  PayloadReader reader(encoded);
  ProcessFederationUpdateAttributeValuesResult result{reader.unsigned32()};
  if (reader.remaining() != 0U) {
    if (reader.remaining() != sizeof(std::uint64_t)) {
      throw ProcessFederationServiceProtocolError(
          "A process attribute-update result has an invalid message identity.");
    }
    result.messageId = reader.unsigned64();
    requireNonzero(
        result.messageId,
        "A process attribute-update result cannot carry a zero message identity.");
  }
  reader.finish();
  return result;
}

std::vector<std::uint8_t>
encodeProcessFederationRequestAttributeValueUpdateResult(
    ProcessFederationRequestAttributeValueUpdateResult const& result) {
  PayloadWriter writer;
  writer.unsigned32(result.recipientCount);
  writeAttributeValueUpdateClassRequestStatus(writer, result.status);
  return std::move(writer).finish();
}

ProcessFederationRequestAttributeValueUpdateResult
decodeProcessFederationRequestAttributeValueUpdateResult(
    std::span<std::uint8_t const> encoded) {
  PayloadReader reader(encoded);
  ProcessFederationRequestAttributeValueUpdateResult result;
  result.recipientCount = reader.unsigned32();
  // Keep decoding old private peers that only carried the recipient count;
  // current regional responses append the typed planner status.
  if (reader.remaining() != 0U) {
    result.status = readAttributeValueUpdateClassRequestStatus(reader);
  }
  reader.finish();
  return result;
}

std::vector<std::uint8_t>
encodeProcessFederationReceiveAttributeUpdateResult(
    ProcessFederationReceiveAttributeUpdateResult const& result) {
  PayloadWriter writer;
  writer.unsigned8(result.event.has_value() ? 1U : 0U);
  if (result.event.has_value()) {
    auto const& event = *result.event;
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
  }
  return std::move(writer).finish();
}

ProcessFederationReceiveAttributeUpdateResult
decodeProcessFederationReceiveAttributeUpdateResult(
    std::span<std::uint8_t const> encoded) {
  PayloadReader reader(encoded);
  auto const hasEvent = reader.unsigned8();
  if (hasEvent > 1U) {
    throw ProcessFederationServiceProtocolError(
        "A process federation attribute result has an invalid event marker.");
  }
  ProcessFederationReceiveAttributeUpdateResult result;
  if (hasEvent != 0U) {
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
            "A process attribute result has an invalid RTI-owned MOM marker.");
      }
      event.rtiOwnedMomObject = true;
      if (reader.remaining() != 0U) {
        if (reader.remaining() != sizeof(std::uint64_t)) {
          throw ProcessFederationServiceProtocolError(
              "A process attribute result has invalid trailing metadata.");
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
    result.event = std::move(event);
  }
  reader.finish();
  return result;
}


std::vector<std::uint8_t>
encodeProcessFederationReceiveObjectInstanceDiscoveryResult(
    ProcessFederationReceiveObjectInstanceDiscoveryResult const& result) {
  PayloadWriter writer;
  writer.unsigned8(result.event.has_value() ? 1U : 0U);
  if (result.event.has_value()) {
    auto const& event = *result.event;
    requireNonzero(
        event.receivingFederateId,
        "A process discovery event requires a recipient identity.");
    requireNonzero(
        event.objectInstanceHandle,
        "A process discovery event requires an object instance.");
    requireNonzero(
        event.objectClassHandle,
        "A process discovery event requires an object class.");
    if (!event.rtiOwnedMomObject) {
      requireNonzero(
          event.producingFederateId,
          "A process discovery event requires a producer identity.");
    }
    if (event.objectInstanceName.empty()) {
      throw ProcessFederationServiceProtocolError(
          "A process discovery event requires an object instance name.");
    }
    writer.unsigned64(event.receivingFederateId);
    writer.unsigned64(event.objectInstanceHandle);
    writer.unsigned64(event.objectClassHandle);
    writer.wideString(event.objectInstanceName);
    writer.unsigned64(event.producingFederateId);
    if (event.rtiOwnedMomObject) {
      writer.unsigned8(kRtiOwnedMomDiscoveryEventMarker);
    }
  }
  return std::move(writer).finish();
}

ProcessFederationReceiveObjectInstanceDiscoveryResult
decodeProcessFederationReceiveObjectInstanceDiscoveryResult(
    std::span<std::uint8_t const> encoded) {
  PayloadReader reader(encoded);
  auto const hasEvent = reader.unsigned8();
  if (hasEvent > 1U) {
    throw ProcessFederationServiceProtocolError(
        "A process discovery result has an invalid event marker.");
  }
  ProcessFederationReceiveObjectInstanceDiscoveryResult result;
  if (hasEvent != 0U) {
    ProcessFederationObjectInstanceDiscoveryEvent event;
    event.receivingFederateId = reader.unsigned64();
    event.objectInstanceHandle = reader.unsigned64();
    event.objectClassHandle = reader.unsigned64();
    event.objectInstanceName = reader.wideString();
    event.producingFederateId = reader.unsigned64();
    if (reader.remaining() != 0U) {
      if (reader.unsigned8() != kRtiOwnedMomDiscoveryEventMarker) {
        throw ProcessFederationServiceProtocolError(
            "A process discovery result has an invalid RTI-owned MOM marker.");
      }
      event.rtiOwnedMomObject = true;
    }
    requireNonzero(
        event.receivingFederateId,
        "A process discovery event requires a recipient identity.");
    requireNonzero(
        event.objectInstanceHandle,
        "A process discovery event requires an object instance.");
    requireNonzero(
        event.objectClassHandle,
        "A process discovery event requires an object class.");
    if (!event.rtiOwnedMomObject) {
      requireNonzero(
          event.producingFederateId,
          "A process discovery event requires a producer identity.");
    }
    if (event.objectInstanceName.empty()) {
      throw ProcessFederationServiceProtocolError(
          "A process discovery event requires an object instance name.");
    }
    result.event = std::move(event);
  }
  reader.finish();
  return result;
}



}  // namespace umbra::detail
