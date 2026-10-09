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
encodeProcessFederationInteractionClassDeclarationRequest(
    ProcessFederationInteractionClassDeclarationRequest const& request) {
  if (request.federationName.empty()) {
    throw ProcessFederationServiceProtocolError(
        "A process federation interaction declaration requires a federation name.");
  }
  requireNonzero(
      request.federateId,
      "A process federation interaction declaration requires a federate identity.");
  requireNonzero(
      request.interactionClassHandle,
      "A process federation interaction declaration requires an interaction class.");
  PayloadWriter writer;
  writer.wideString(request.federationName);
  writer.unsigned64(request.federateId);
  writer.unsigned64(request.interactionClassHandle);
  writer.unsigned8(request.active ? 1U : 0U);
  return std::move(writer).finish();
}

ProcessFederationInteractionClassDeclarationRequest
decodeProcessFederationInteractionClassDeclarationRequest(
    std::span<std::uint8_t const> encoded) {
  PayloadReader reader(encoded);
  ProcessFederationInteractionClassDeclarationRequest result;
  result.federationName = reader.wideString();
  result.federateId = reader.unsigned64();
  result.interactionClassHandle = reader.unsigned64();
  auto const active = reader.unsigned8();
  reader.finish();
  if (result.federationName.empty()) {
    throw ProcessFederationServiceProtocolError(
        "A process federation interaction declaration requires a federation name.");
  }
  requireNonzero(
      result.federateId,
      "A process federation interaction declaration requires a federate identity.");
  requireNonzero(
      result.interactionClassHandle,
      "A process federation interaction declaration requires an interaction class.");
  if (active > 1U) {
    throw ProcessFederationServiceProtocolError(
        "A process federation interaction declaration has an invalid state marker.");
  }
  result.active = active != 0U;
  return result;
}

std::vector<std::uint8_t>
encodeProcessFederationInteractionClassRegionalSubscriptionRequest(
    ProcessFederationInteractionClassRegionalSubscriptionRequest const& request) {
  if (request.federationName.empty()) {
    throw ProcessFederationServiceProtocolError(
        "A process regional interaction subscription requires a federation name.");
  }
  requireNonzero(
      request.federateId,
      "A process regional interaction subscription requires a federate identity.");
  requireNonzero(
      request.interactionClassHandle,
      "A process regional interaction subscription requires an interaction class.");
  PayloadWriter writer;
  writer.wideString(request.federationName);
  writer.unsigned64(request.federateId);
  writer.unsigned64(request.interactionClassHandle);
  writer.unsigned64Vector(parameterVector(request.regionHandles));
  writer.unsigned8(request.active ? 1U : 0U);
  return std::move(writer).finish();
}

ProcessFederationInteractionClassRegionalSubscriptionRequest
decodeProcessFederationInteractionClassRegionalSubscriptionRequest(
    std::span<std::uint8_t const> encoded) {
  PayloadReader reader(encoded);
  ProcessFederationInteractionClassRegionalSubscriptionRequest result;
  result.federationName = reader.wideString();
  result.federateId = reader.unsigned64();
  result.interactionClassHandle = reader.unsigned64();
  auto regionVector = reader.unsigned64Vector();
  validateHandleVector(
      regionVector,
      "A process regional interaction subscription requires sorted, unique region handles.");
  result.regionHandles.insert(regionVector.begin(), regionVector.end());
  auto const active = reader.unsigned8();
  reader.finish();
  if (result.federationName.empty()) {
    throw ProcessFederationServiceProtocolError(
        "A process regional interaction subscription requires a federation name.");
  }
  requireNonzero(
      result.federateId,
      "A process regional interaction subscription requires a federate identity.");
  requireNonzero(
      result.interactionClassHandle,
      "A process regional interaction subscription requires an interaction class.");
  if (active > 1U) {
    throw ProcessFederationServiceProtocolError(
        "A process regional interaction subscription has an invalid state marker.");
  }
  result.active = active != 0U;
  return result;
}

std::vector<std::uint8_t>
encodeProcessFederationObjectClassDirectedInteractionDeclarationRequest(
    ProcessFederationObjectClassDirectedInteractionDeclarationRequest const& request) {
  if (request.federationName.empty()) {
    throw ProcessFederationServiceProtocolError(
        "A process directed-interaction declaration requires a federation name.");
  }
  requireNonzero(
      request.federateId,
      "A process directed-interaction declaration requires a federate identity.");
  requireNonzero(
      request.objectClassHandle,
      "A process directed-interaction declaration requires an object class.");
  if (request.interactionClassHandles) {
    validateHandleVector(
        *request.interactionClassHandles,
        "A process directed-interaction declaration requires sorted, unique interaction handles.");
  }
  PayloadWriter writer;
  writer.wideString(request.federationName);
  writer.unsigned64(request.federateId);
  writer.unsigned64(request.objectClassHandle);
  writer.unsigned8(request.interactionClassHandles.has_value() ? 1U : 0U);
  if (request.interactionClassHandles) {
    writer.unsigned64Vector(*request.interactionClassHandles);
  }
  writer.unsigned8(request.universally ? 1U : 0U);
  return std::move(writer).finish();
}

ProcessFederationObjectClassDirectedInteractionDeclarationRequest
decodeProcessFederationObjectClassDirectedInteractionDeclarationRequest(
    std::span<std::uint8_t const> encoded) {
  PayloadReader reader(encoded);
  ProcessFederationObjectClassDirectedInteractionDeclarationRequest result;
  result.federationName = reader.wideString();
  result.federateId = reader.unsigned64();
  result.objectClassHandle = reader.unsigned64();
  auto const hasInteractionClassHandles = reader.unsigned8();
  if (hasInteractionClassHandles > 1U) {
    throw ProcessFederationServiceProtocolError(
        "A process directed-interaction declaration has an invalid set marker.");
  }
  if (hasInteractionClassHandles != 0U) {
    result.interactionClassHandles = reader.unsigned64Vector();
    validateHandleVector(
        *result.interactionClassHandles,
        "A process directed-interaction declaration requires sorted, unique interaction handles.");
  }
  auto const universally = reader.unsigned8();
  reader.finish();
  if (result.federationName.empty()) {
    throw ProcessFederationServiceProtocolError(
        "A process directed-interaction declaration requires a federation name.");
  }
  requireNonzero(
      result.federateId,
      "A process directed-interaction declaration requires a federate identity.");
  requireNonzero(
      result.objectClassHandle,
      "A process directed-interaction declaration requires an object class.");
  if (universally > 1U) {
    throw ProcessFederationServiceProtocolError(
        "A process directed-interaction declaration has an invalid universal marker.");
  }
  result.universally = universally != 0U;
  return result;
}

std::vector<std::uint8_t>
encodeProcessFederationObjectClassAttributeDeclarationRequest(
    ProcessFederationObjectClassAttributeDeclarationRequest const& request) {
  if (request.federationName.empty()) {
    throw ProcessFederationServiceProtocolError(
        "A process federation object-class declaration requires a federation name.");
  }
  requireNonzero(
      request.federateId,
      "A process federation object-class declaration requires a federate identity.");
  requireNonzero(
      request.objectClassHandle,
      "A process federation object-class declaration requires an object class.");
  PayloadWriter writer;
  writer.wideString(request.federationName);
  writer.unsigned64(request.federateId);
  writer.unsigned64(request.objectClassHandle);
  writer.unsigned64Vector(request.attributeHandles);
  return std::move(writer).finish();
}

ProcessFederationObjectClassAttributeDeclarationRequest
decodeProcessFederationObjectClassAttributeDeclarationRequest(
    std::span<std::uint8_t const> encoded) {
  PayloadReader reader(encoded);
  ProcessFederationObjectClassAttributeDeclarationRequest result;
  result.federationName = reader.wideString();
  result.federateId = reader.unsigned64();
  result.objectClassHandle = reader.unsigned64();
  result.attributeHandles = reader.unsigned64Vector();
  reader.finish();
  if (result.federationName.empty()) {
    throw ProcessFederationServiceProtocolError(
        "A process federation object-class declaration requires a federation name.");
  }
  requireNonzero(
      result.federateId,
      "A process federation object-class declaration requires a federate identity.");
  requireNonzero(
      result.objectClassHandle,
      "A process federation object-class declaration requires an object class.");
  return result;
}

std::vector<std::uint8_t>
encodeProcessFederationObjectClassAttributeSubscriptionRequest(
    ProcessFederationObjectClassAttributeSubscriptionRequest const& request) {
  if (request.federationName.empty()) {
    throw ProcessFederationServiceProtocolError(
        "A process federation object-class subscription requires a federation name.");
  }
  requireNonzero(
      request.federateId,
      "A process federation object-class subscription requires a federate identity.");
  requireNonzero(
      request.objectClassHandle,
      "A process federation object-class subscription requires an object class.");
  PayloadWriter writer;
  writer.wideString(request.federationName);
  writer.unsigned64(request.federateId);
  writer.unsigned64(request.objectClassHandle);
  writer.unsigned64Vector(request.attributeHandles);
  writer.unsigned8(request.active ? 1U : 0U);
  writer.string(request.updateRateDesignator);
  return std::move(writer).finish();
}

ProcessFederationObjectClassAttributeSubscriptionRequest
decodeProcessFederationObjectClassAttributeSubscriptionRequest(
    std::span<std::uint8_t const> encoded) {
  PayloadReader reader(encoded);
  ProcessFederationObjectClassAttributeSubscriptionRequest result;
  result.federationName = reader.wideString();
  result.federateId = reader.unsigned64();
  result.objectClassHandle = reader.unsigned64();
  result.attributeHandles = reader.unsigned64Vector();
  auto const active = reader.unsigned8();
  result.updateRateDesignator = reader.string();
  reader.finish();
  if (result.federationName.empty()) {
    throw ProcessFederationServiceProtocolError(
        "A process federation object-class subscription requires a federation name.");
  }
  requireNonzero(
      result.federateId,
      "A process federation object-class subscription requires a federate identity.");
  requireNonzero(
      result.objectClassHandle,
      "A process federation object-class subscription requires an object class.");
  if (active > 1U) {
    throw ProcessFederationServiceProtocolError(
        "A process federation object-class subscription has an invalid state marker.");
  }
  result.active = active != 0U;
  return result;
}

std::vector<std::uint8_t>
encodeProcessFederationObjectClassAttributeRegionalSubscriptionRequest(
    ProcessFederationObjectClassAttributeRegionalSubscriptionRequest const& request) {
  if (request.federationName.empty()) {
    throw ProcessFederationServiceProtocolError(
        "A process regional object-class subscription requires a federation name.");
  }
  requireNonzero(
      request.federateId,
      "A process regional object-class subscription requires a federate identity.");
  requireNonzero(
      request.objectClassHandle,
      "A process regional object-class subscription requires an object class.");
  PayloadWriter writer;
  writer.wideString(request.federationName);
  writer.unsigned64(request.federateId);
  writer.unsigned64(request.objectClassHandle);
  writeAttributeRegionMap(writer, request.attributesAndRegions);
  writer.unsigned8(request.active ? 1U : 0U);
  writer.string(request.updateRateDesignator);
  return std::move(writer).finish();
}

ProcessFederationObjectClassAttributeRegionalSubscriptionRequest
decodeProcessFederationObjectClassAttributeRegionalSubscriptionRequest(
    std::span<std::uint8_t const> encoded) {
  PayloadReader reader(encoded);
  ProcessFederationObjectClassAttributeRegionalSubscriptionRequest result;
  result.federationName = reader.wideString();
  result.federateId = reader.unsigned64();
  result.objectClassHandle = reader.unsigned64();
  result.attributesAndRegions = readAttributeRegionMap(reader);
  auto const active = reader.unsigned8();
  result.updateRateDesignator = reader.string();
  reader.finish();
  if (result.federationName.empty()) {
    throw ProcessFederationServiceProtocolError(
        "A process regional object-class subscription requires a federation name.");
  }
  requireNonzero(
      result.federateId,
      "A process regional object-class subscription requires a federate identity.");
  requireNonzero(
      result.objectClassHandle,
      "A process regional object-class subscription requires an object class.");
  if (active > 1U) {
    throw ProcessFederationServiceProtocolError(
        "A process regional object-class subscription has an invalid state marker.");
  }
  result.active = active != 0U;
  return result;
}
}  // namespace umbra::detail
