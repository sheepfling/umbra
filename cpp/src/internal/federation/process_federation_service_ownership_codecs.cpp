#include "internal/federation/process_federation_service_protocol.hpp"
#include "internal/federation/process_federation_service_codec_support.hpp"

#include <cstdint>
#include <limits>
#include <set>
#include <span>
#include <utility>
#include <vector>

namespace umbra::detail {
using namespace process_federation_service_codec_support;
std::vector<std::uint8_t>
encodeProcessFederationAttributeOwnershipCheckRequest(
    ProcessFederationAttributeOwnershipCheckRequest const& request) {
  if (request.federationName.empty()) {
    throw ProcessFederationServiceProtocolError(
        "A process attribute-ownership check requires a federation name.");
  }
  requireNonzero(
      request.requestingFederateId,
      "A process attribute-ownership check requires a requester identity.");
  requireNonzero(
      request.objectInstanceHandle,
      "A process attribute-ownership check requires an object instance.");
  requireNonzero(
      request.attributeHandle,
      "A process attribute-ownership check requires an attribute.");
  PayloadWriter writer;
  writer.wideString(request.federationName);
  writer.unsigned64(request.requestingFederateId);
  writer.unsigned64(request.objectInstanceHandle);
  writer.unsigned64(request.attributeHandle);
  return std::move(writer).finish();
}

ProcessFederationAttributeOwnershipCheckRequest
decodeProcessFederationAttributeOwnershipCheckRequest(
    std::span<std::uint8_t const> encoded) {
  PayloadReader reader(encoded);
  ProcessFederationAttributeOwnershipCheckRequest result;
  result.federationName = reader.wideString();
  result.requestingFederateId = reader.unsigned64();
  result.objectInstanceHandle = reader.unsigned64();
  result.attributeHandle = reader.unsigned64();
  reader.finish();
  if (result.federationName.empty()) {
    throw ProcessFederationServiceProtocolError(
        "A process attribute-ownership check requires a federation name.");
  }
  requireNonzero(
      result.requestingFederateId,
      "A process attribute-ownership check requires a requester identity.");
  requireNonzero(
      result.objectInstanceHandle,
      "A process attribute-ownership check requires an object instance.");
  requireNonzero(
      result.attributeHandle,
      "A process attribute-ownership check requires an attribute.");
  return result;
}

std::vector<std::uint8_t>
encodeProcessFederationAttributeOwnershipQueryRequest(
    ProcessFederationAttributeOwnershipQueryRequest const& request) {
  if (request.federationName.empty()) {
    throw ProcessFederationServiceProtocolError(
        "A process attribute-ownership query requires a federation name.");
  }
  requireNonzero(
      request.requestingFederateId,
      "A process attribute-ownership query requires a requester identity.");
  requireNonzero(
      request.objectInstanceHandle,
      "A process attribute-ownership query requires an object instance.");
  if (request.requestedAttributeHandles.empty() ||
      request.requestedAttributeHandles.size() >
          std::numeric_limits<std::uint32_t>::max()) {
    throw ProcessFederationServiceProtocolError(
        "A process attribute-ownership query requires a bounded attribute set.");
  }
  std::set<std::uint64_t> seenHandles;
  for (std::uint64_t const attributeHandle :
       request.requestedAttributeHandles) {
    requireNonzero(
        attributeHandle,
        "A process attribute-ownership query requires attribute identities.");
    if (!seenHandles.insert(attributeHandle).second) {
      throw ProcessFederationServiceProtocolError(
          "A process attribute-ownership query repeats an attribute identity.");
    }
  }
  PayloadWriter writer;
  writer.wideString(request.federationName);
  writer.unsigned64(request.requestingFederateId);
  writer.unsigned64(request.objectInstanceHandle);
  writer.unsigned32(static_cast<std::uint32_t>(
      request.requestedAttributeHandles.size()));
  for (std::uint64_t const attributeHandle :
       request.requestedAttributeHandles) {
    writer.unsigned64(attributeHandle);
  }
  return std::move(writer).finish();
}

ProcessFederationAttributeOwnershipQueryRequest
decodeProcessFederationAttributeOwnershipQueryRequest(
    std::span<std::uint8_t const> encoded) {
  PayloadReader reader(encoded);
  ProcessFederationAttributeOwnershipQueryRequest result;
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
        "A process attribute-ownership query requires attribute identities.");
    if (!seenHandles.insert(attributeHandle).second) {
      throw ProcessFederationServiceProtocolError(
          "A process attribute-ownership query repeats an attribute identity.");
    }
    result.requestedAttributeHandles.push_back(attributeHandle);
  }
  reader.finish();
  if (result.federationName.empty()) {
    throw ProcessFederationServiceProtocolError(
        "A process attribute-ownership query requires a federation name.");
  }
  requireNonzero(
      result.requestingFederateId,
      "A process attribute-ownership query requires a requester identity.");
  requireNonzero(
      result.objectInstanceHandle,
      "A process attribute-ownership query requires an object instance.");
  if (result.requestedAttributeHandles.empty()) {
    throw ProcessFederationServiceProtocolError(
        "A process attribute-ownership query requires attributes.");
  }
  return result;
}


std::vector<std::uint8_t>
encodeProcessFederationAttributeOwnershipAcquisitionIfAvailableRequest(
    ProcessFederationAttributeOwnershipAcquisitionIfAvailableRequest const& request) {
  if (request.federationName.empty()) {
    throw ProcessFederationServiceProtocolError(
        "A process attribute-ownership acquisition-if-available request requires a federation name.");
  }
  requireNonzero(
      request.requestingFederateId,
      "A process attribute-ownership acquisition-if-available request requires a requester identity.");
  requireNonzero(
      request.objectInstanceHandle,
      "A process attribute-ownership acquisition-if-available request requires an object instance.");
  if (request.desiredAttributeHandles.size() >
      std::numeric_limits<std::uint32_t>::max()) {
    throw ProcessFederationServiceProtocolError(
        "A process attribute-ownership acquisition-if-available request has too many attributes.");
  }
  std::set<std::uint64_t> seenHandles;
  for (std::uint64_t const attributeHandle : request.desiredAttributeHandles) {
    requireNonzero(
        attributeHandle,
        "A process attribute-ownership acquisition-if-available request requires attribute identities.");
    if (!seenHandles.insert(attributeHandle).second) {
      throw ProcessFederationServiceProtocolError(
          "A process attribute-ownership acquisition-if-available request repeats an attribute identity.");
    }
  }
  PayloadWriter writer;
  writer.wideString(request.federationName);
  writer.unsigned64(request.requestingFederateId);
  writer.unsigned64(request.objectInstanceHandle);
  writer.unsigned32(static_cast<std::uint32_t>(
      request.desiredAttributeHandles.size()));
  for (std::uint64_t const attributeHandle : request.desiredAttributeHandles) {
    writer.unsigned64(attributeHandle);
  }
  writer.bytes(request.userSuppliedTag);
  return std::move(writer).finish();
}

ProcessFederationAttributeOwnershipAcquisitionIfAvailableRequest
decodeProcessFederationAttributeOwnershipAcquisitionIfAvailableRequest(
    std::span<std::uint8_t const> encoded) {
  PayloadReader reader(encoded);
  ProcessFederationAttributeOwnershipAcquisitionIfAvailableRequest result;
  result.federationName = reader.wideString();
  result.requestingFederateId = reader.unsigned64();
  result.objectInstanceHandle = reader.unsigned64();
  auto const attributeCount = reader.unsigned32();
  result.desiredAttributeHandles.reserve(attributeCount);
  std::set<std::uint64_t> seenHandles;
  for (std::uint32_t index = 0U; index < attributeCount; ++index) {
    auto const attributeHandle = reader.unsigned64();
    requireNonzero(
        attributeHandle,
        "A process attribute-ownership acquisition-if-available request requires attribute identities.");
    if (!seenHandles.insert(attributeHandle).second) {
      throw ProcessFederationServiceProtocolError(
          "A process attribute-ownership acquisition-if-available request repeats an attribute identity.");
    }
    result.desiredAttributeHandles.push_back(attributeHandle);
  }
  result.userSuppliedTag = reader.bytes();
  reader.finish();
  if (result.federationName.empty()) {
    throw ProcessFederationServiceProtocolError(
        "A process attribute-ownership acquisition-if-available request requires a federation name.");
  }
  requireNonzero(
      result.requestingFederateId,
      "A process attribute-ownership acquisition-if-available request requires a requester identity.");
  requireNonzero(
      result.objectInstanceHandle,
      "A process attribute-ownership acquisition-if-available request requires an object instance.");
  return result;
}

std::vector<std::uint8_t>
encodeProcessFederationAttributeOwnershipAcquisitionRequest(
    ProcessFederationAttributeOwnershipAcquisitionRequest const& request) {
  if (request.federationName.empty()) {
    throw ProcessFederationServiceProtocolError(
        "A process attribute-ownership acquisition request requires a federation name.");
  }
  requireNonzero(
      request.requestingFederateId,
      "A process attribute-ownership acquisition request requires a requester identity.");
  requireNonzero(
      request.objectInstanceHandle,
      "A process attribute-ownership acquisition request requires an object instance.");
  if (request.desiredAttributeHandles.size() >
      std::numeric_limits<std::uint32_t>::max()) {
    throw ProcessFederationServiceProtocolError(
        "A process attribute-ownership acquisition request has too many attributes.");
  }
  std::set<std::uint64_t> seenHandles;
  for (std::uint64_t const attributeHandle : request.desiredAttributeHandles) {
    requireNonzero(
        attributeHandle,
        "A process attribute-ownership acquisition request requires attribute identities.");
    if (!seenHandles.insert(attributeHandle).second) {
      throw ProcessFederationServiceProtocolError(
          "A process attribute-ownership acquisition request repeats an attribute identity.");
    }
  }
  PayloadWriter writer;
  writer.wideString(request.federationName);
  writer.unsigned64(request.requestingFederateId);
  writer.unsigned64(request.objectInstanceHandle);
  writer.unsigned32(static_cast<std::uint32_t>(
      request.desiredAttributeHandles.size()));
  for (std::uint64_t const attributeHandle : request.desiredAttributeHandles) {
    writer.unsigned64(attributeHandle);
  }
  writer.bytes(request.userSuppliedTag);
  writer.unsigned8(request.callbacksEnabled ? 1U : 0U);
  return std::move(writer).finish();
}

ProcessFederationAttributeOwnershipAcquisitionRequest
decodeProcessFederationAttributeOwnershipAcquisitionRequest(
    std::span<std::uint8_t const> encoded) {
  PayloadReader reader(encoded);
  ProcessFederationAttributeOwnershipAcquisitionRequest result;
  result.federationName = reader.wideString();
  result.requestingFederateId = reader.unsigned64();
  result.objectInstanceHandle = reader.unsigned64();
  auto const attributeCount = reader.unsigned32();
  result.desiredAttributeHandles.reserve(attributeCount);
  std::set<std::uint64_t> seenHandles;
  for (std::uint32_t index = 0U; index < attributeCount; ++index) {
    auto const attributeHandle = reader.unsigned64();
    requireNonzero(
        attributeHandle,
        "A process attribute-ownership acquisition request requires attribute identities.");
    if (!seenHandles.insert(attributeHandle).second) {
      throw ProcessFederationServiceProtocolError(
          "A process attribute-ownership acquisition request repeats an attribute identity.");
    }
    result.desiredAttributeHandles.push_back(attributeHandle);
  }
  result.userSuppliedTag = reader.bytes();
  if (reader.remaining() != 0U) {
    result.callbacksEnabled = reader.unsigned8() != 0U;
  }
  reader.finish();
  if (result.federationName.empty()) {
    throw ProcessFederationServiceProtocolError(
        "A process attribute-ownership acquisition request requires a federation name.");
  }
  requireNonzero(
      result.requestingFederateId,
      "A process attribute-ownership acquisition request requires a requester identity.");
  requireNonzero(
      result.objectInstanceHandle,
      "A process attribute-ownership acquisition request requires an object instance.");
  return result;
}

std::vector<std::uint8_t>
encodeProcessFederationAttributeOwnershipReleaseDeniedRequest(
    ProcessFederationAttributeOwnershipReleaseDeniedRequest const& request) {
  if (request.federationName.empty()) {
    throw ProcessFederationServiceProtocolError(
        "A process attribute-ownership release-denied request requires a federation name.");
  }
  requireNonzero(
      request.owningFederateId,
      "A process attribute-ownership release-denied request requires an owner identity.");
  requireNonzero(
      request.objectInstanceHandle,
      "A process attribute-ownership release-denied request requires an object instance.");
  if (request.attributeHandles.size() >
      std::numeric_limits<std::uint32_t>::max()) {
    throw ProcessFederationServiceProtocolError(
        "A process attribute-ownership release-denied request has too many attributes.");
  }
  std::set<std::uint64_t> seenHandles;
  for (std::uint64_t const attributeHandle : request.attributeHandles) {
    requireNonzero(
        attributeHandle,
        "A process attribute-ownership release-denied request requires attribute identities.");
    if (!seenHandles.insert(attributeHandle).second) {
      throw ProcessFederationServiceProtocolError(
          "A process attribute-ownership release-denied request repeats an attribute identity.");
    }
  }
  PayloadWriter writer;
  writer.wideString(request.federationName);
  writer.unsigned64(request.owningFederateId);
  writer.unsigned64(request.objectInstanceHandle);
  writer.unsigned32(static_cast<std::uint32_t>(request.attributeHandles.size()));
  for (std::uint64_t const attributeHandle : request.attributeHandles) {
    writer.unsigned64(attributeHandle);
  }
  writer.bytes(request.userSuppliedTag);
  return std::move(writer).finish();
}

ProcessFederationAttributeOwnershipReleaseDeniedRequest
decodeProcessFederationAttributeOwnershipReleaseDeniedRequest(
    std::span<std::uint8_t const> encoded) {
  PayloadReader reader(encoded);
  ProcessFederationAttributeOwnershipReleaseDeniedRequest result;
  result.federationName = reader.wideString();
  result.owningFederateId = reader.unsigned64();
  result.objectInstanceHandle = reader.unsigned64();
  auto const attributeCount = reader.unsigned32();
  result.attributeHandles.reserve(attributeCount);
  std::set<std::uint64_t> seenHandles;
  for (std::uint32_t index = 0U; index < attributeCount; ++index) {
    auto const attributeHandle = reader.unsigned64();
    requireNonzero(
        attributeHandle,
        "A process attribute-ownership release-denied request requires attribute identities.");
    if (!seenHandles.insert(attributeHandle).second) {
      throw ProcessFederationServiceProtocolError(
          "A process attribute-ownership release-denied request repeats an attribute identity.");
    }
    result.attributeHandles.push_back(attributeHandle);
  }
  result.userSuppliedTag = reader.bytes();
  reader.finish();
  if (result.federationName.empty()) {
    throw ProcessFederationServiceProtocolError(
        "A process attribute-ownership release-denied request requires a federation name.");
  }
  requireNonzero(
      result.owningFederateId,
      "A process attribute-ownership release-denied request requires an owner identity.");
  requireNonzero(
      result.objectInstanceHandle,
      "A process attribute-ownership release-denied request requires an object instance.");
  return result;
}

std::vector<std::uint8_t>
encodeProcessFederationAttributeOwnershipAcquisitionCancellationRequest(
    ProcessFederationAttributeOwnershipAcquisitionCancellationRequest const& request) {
  if (request.federationName.empty()) {
    throw ProcessFederationServiceProtocolError(
        "A process attribute-ownership acquisition-cancellation request requires a federation name.");
  }
  requireNonzero(
      request.requestingFederateId,
      "A process attribute-ownership acquisition-cancellation request requires a requester identity.");
  requireNonzero(
      request.objectInstanceHandle,
      "A process attribute-ownership acquisition-cancellation request requires an object instance.");
  if (request.attributeHandles.size() >
      std::numeric_limits<std::uint32_t>::max()) {
    throw ProcessFederationServiceProtocolError(
        "A process attribute-ownership acquisition-cancellation request has too many attributes.");
  }
  std::set<std::uint64_t> seenHandles;
  for (std::uint64_t const attributeHandle : request.attributeHandles) {
    requireNonzero(
        attributeHandle,
        "A process attribute-ownership acquisition-cancellation request requires attribute identities.");
    if (!seenHandles.insert(attributeHandle).second) {
      throw ProcessFederationServiceProtocolError(
          "A process attribute-ownership acquisition-cancellation request repeats an attribute identity.");
    }
  }
  PayloadWriter writer;
  writer.wideString(request.federationName);
  writer.unsigned64(request.requestingFederateId);
  writer.unsigned64(request.objectInstanceHandle);
  writer.unsigned32(static_cast<std::uint32_t>(request.attributeHandles.size()));
  for (std::uint64_t const attributeHandle : request.attributeHandles) {
    writer.unsigned64(attributeHandle);
  }
  return std::move(writer).finish();
}

ProcessFederationAttributeOwnershipAcquisitionCancellationRequest
decodeProcessFederationAttributeOwnershipAcquisitionCancellationRequest(
    std::span<std::uint8_t const> encoded) {
  PayloadReader reader(encoded);
  ProcessFederationAttributeOwnershipAcquisitionCancellationRequest result;
  result.federationName = reader.wideString();
  result.requestingFederateId = reader.unsigned64();
  result.objectInstanceHandle = reader.unsigned64();
  auto const attributeCount = reader.unsigned32();
  result.attributeHandles.reserve(attributeCount);
  std::set<std::uint64_t> seenHandles;
  for (std::uint32_t index = 0U; index < attributeCount; ++index) {
    auto const attributeHandle = reader.unsigned64();
    requireNonzero(
        attributeHandle,
        "A process attribute-ownership acquisition-cancellation request requires attribute identities.");
    if (!seenHandles.insert(attributeHandle).second) {
      throw ProcessFederationServiceProtocolError(
          "A process attribute-ownership acquisition-cancellation request repeats an attribute identity.");
    }
    result.attributeHandles.push_back(attributeHandle);
  }
  reader.finish();
  if (result.federationName.empty()) {
    throw ProcessFederationServiceProtocolError(
        "A process attribute-ownership acquisition-cancellation request requires a federation name.");
  }
  requireNonzero(
      result.requestingFederateId,
      "A process attribute-ownership acquisition-cancellation request requires a requester identity.");
  requireNonzero(
      result.objectInstanceHandle,
      "A process attribute-ownership acquisition-cancellation request requires an object instance.");
  return result;
}

std::vector<std::uint8_t>
encodeProcessFederationCancelNegotiatedAttributeOwnershipDivestitureRequest(
    ProcessFederationCancelNegotiatedAttributeOwnershipDivestitureRequest const& request) {
  if (request.federationName.empty()) {
    throw ProcessFederationServiceProtocolError(
        "A process negotiated-divestiture cancellation request requires a federation name.");
  }
  requireNonzero(
      request.divestingFederateId,
      "A process negotiated-divestiture cancellation request requires a divesting federate identity.");
  requireNonzero(
      request.objectInstanceHandle,
      "A process negotiated-divestiture cancellation request requires an object instance.");
  if (request.attributeHandles.size() >
      std::numeric_limits<std::uint32_t>::max()) {
    throw ProcessFederationServiceProtocolError(
        "A process negotiated-divestiture cancellation request has too many attributes.");
  }
  std::set<std::uint64_t> seenHandles;
  for (std::uint64_t const attributeHandle : request.attributeHandles) {
    requireNonzero(
        attributeHandle,
        "A process negotiated-divestiture cancellation request requires attribute identities.");
    if (!seenHandles.insert(attributeHandle).second) {
      throw ProcessFederationServiceProtocolError(
          "A process negotiated-divestiture cancellation request repeats an attribute identity.");
    }
  }
  PayloadWriter writer;
  writer.wideString(request.federationName);
  writer.unsigned64(request.divestingFederateId);
  writer.unsigned64(request.objectInstanceHandle);
  writer.unsigned32(static_cast<std::uint32_t>(request.attributeHandles.size()));
  for (std::uint64_t const attributeHandle : request.attributeHandles) {
    writer.unsigned64(attributeHandle);
  }
  return std::move(writer).finish();
}

ProcessFederationCancelNegotiatedAttributeOwnershipDivestitureRequest
decodeProcessFederationCancelNegotiatedAttributeOwnershipDivestitureRequest(
    std::span<std::uint8_t const> encoded) {
  PayloadReader reader(encoded);
  ProcessFederationCancelNegotiatedAttributeOwnershipDivestitureRequest result;
  result.federationName = reader.wideString();
  result.divestingFederateId = reader.unsigned64();
  result.objectInstanceHandle = reader.unsigned64();
  auto const attributeCount = reader.unsigned32();
  result.attributeHandles.reserve(attributeCount);
  std::set<std::uint64_t> seenHandles;
  for (std::uint32_t index = 0U; index < attributeCount; ++index) {
    auto const attributeHandle = reader.unsigned64();
    requireNonzero(
        attributeHandle,
        "A process negotiated-divestiture cancellation request requires attribute identities.");
    if (!seenHandles.insert(attributeHandle).second) {
      throw ProcessFederationServiceProtocolError(
          "A process negotiated-divestiture cancellation request repeats an attribute identity.");
    }
    result.attributeHandles.push_back(attributeHandle);
  }
  reader.finish();
  if (result.federationName.empty()) {
    throw ProcessFederationServiceProtocolError(
        "A process negotiated-divestiture cancellation request requires a federation name.");
  }
  requireNonzero(
      result.divestingFederateId,
      "A process negotiated-divestiture cancellation request requires a divesting federate identity.");
  requireNonzero(
      result.objectInstanceHandle,
      "A process negotiated-divestiture cancellation request requires an object instance.");
  return result;
}

std::vector<std::uint8_t>
encodeProcessFederationNegotiatedAttributeOwnershipDivestitureRequest(
    ProcessFederationNegotiatedAttributeOwnershipDivestitureRequest const& request) {
  if (request.federationName.empty()) {
    throw ProcessFederationServiceProtocolError(
        "A process negotiated-divestiture request requires a federation name.");
  }
  requireNonzero(
      request.divestingFederateId,
      "A process negotiated-divestiture request requires a divesting federate identity.");
  requireNonzero(
      request.objectInstanceHandle,
      "A process negotiated-divestiture request requires an object instance.");
  if (request.attributeHandles.size() >
      std::numeric_limits<std::uint32_t>::max()) {
    throw ProcessFederationServiceProtocolError(
        "A process negotiated-divestiture request has too many attributes.");
  }
  std::set<std::uint64_t> seenHandles;
  for (std::uint64_t const attributeHandle : request.attributeHandles) {
    requireNonzero(
        attributeHandle,
        "A process negotiated-divestiture request requires attribute identities.");
    if (!seenHandles.insert(attributeHandle).second) {
      throw ProcessFederationServiceProtocolError(
          "A process negotiated-divestiture request repeats an attribute identity.");
    }
  }
  PayloadWriter writer;
  writer.wideString(request.federationName);
  writer.unsigned64(request.divestingFederateId);
  writer.unsigned64(request.objectInstanceHandle);
  writer.unsigned32(static_cast<std::uint32_t>(request.attributeHandles.size()));
  for (std::uint64_t const attributeHandle : request.attributeHandles) {
    writer.unsigned64(attributeHandle);
  }
  writer.bytes(request.userSuppliedTag);
  return std::move(writer).finish();
}

ProcessFederationNegotiatedAttributeOwnershipDivestitureRequest
decodeProcessFederationNegotiatedAttributeOwnershipDivestitureRequest(
    std::span<std::uint8_t const> encoded) {
  PayloadReader reader(encoded);
  ProcessFederationNegotiatedAttributeOwnershipDivestitureRequest result;
  result.federationName = reader.wideString();
  result.divestingFederateId = reader.unsigned64();
  result.objectInstanceHandle = reader.unsigned64();
  auto const attributeCount = reader.unsigned32();
  result.attributeHandles.reserve(attributeCount);
  std::set<std::uint64_t> seenHandles;
  for (std::uint32_t index = 0U; index < attributeCount; ++index) {
    auto const attributeHandle = reader.unsigned64();
    requireNonzero(
        attributeHandle,
        "A process negotiated-divestiture request requires attribute identities.");
    if (!seenHandles.insert(attributeHandle).second) {
      throw ProcessFederationServiceProtocolError(
          "A process negotiated-divestiture request repeats an attribute identity.");
    }
    result.attributeHandles.push_back(attributeHandle);
  }
  result.userSuppliedTag = reader.bytes();
  reader.finish();
  if (result.federationName.empty()) {
    throw ProcessFederationServiceProtocolError(
        "A process negotiated-divestiture request requires a federation name.");
  }
  requireNonzero(
      result.divestingFederateId,
      "A process negotiated-divestiture request requires a divesting federate identity.");
  requireNonzero(
      result.objectInstanceHandle,
      "A process negotiated-divestiture request requires an object instance.");
  return result;
}

std::vector<std::uint8_t>
encodeProcessFederationConfirmDivestitureRequest(
    ProcessFederationConfirmDivestitureRequest const& request) {
  if (request.federationName.empty()) {
    throw ProcessFederationServiceProtocolError(
        "A process Confirm Divestiture request requires a federation name.");
  }
  requireNonzero(
      request.divestingFederateId,
      "A process Confirm Divestiture request requires a divesting federate identity.");
  requireNonzero(
      request.objectInstanceHandle,
      "A process Confirm Divestiture request requires an object instance.");
  if (request.attributeHandles.size() >
      std::numeric_limits<std::uint32_t>::max()) {
    throw ProcessFederationServiceProtocolError(
        "A process Confirm Divestiture request has too many attributes.");
  }
  std::set<std::uint64_t> seenHandles;
  for (std::uint64_t const attributeHandle : request.attributeHandles) {
    requireNonzero(
        attributeHandle,
        "A process Confirm Divestiture request requires attribute identities.");
    if (!seenHandles.insert(attributeHandle).second) {
      throw ProcessFederationServiceProtocolError(
          "A process Confirm Divestiture request repeats an attribute identity.");
    }
  }
  PayloadWriter writer;
  writer.wideString(request.federationName);
  writer.unsigned64(request.divestingFederateId);
  writer.unsigned64(request.objectInstanceHandle);
  writer.unsigned32(static_cast<std::uint32_t>(request.attributeHandles.size()));
  for (std::uint64_t const attributeHandle : request.attributeHandles) {
    writer.unsigned64(attributeHandle);
  }
  writer.bytes(request.userSuppliedTag);
  return std::move(writer).finish();
}

ProcessFederationConfirmDivestitureRequest
decodeProcessFederationConfirmDivestitureRequest(
    std::span<std::uint8_t const> encoded) {
  PayloadReader reader(encoded);
  ProcessFederationConfirmDivestitureRequest result;
  result.federationName = reader.wideString();
  result.divestingFederateId = reader.unsigned64();
  result.objectInstanceHandle = reader.unsigned64();
  auto const attributeCount = reader.unsigned32();
  result.attributeHandles.reserve(attributeCount);
  std::set<std::uint64_t> seenHandles;
  for (std::uint32_t index = 0U; index < attributeCount; ++index) {
    auto const attributeHandle = reader.unsigned64();
    requireNonzero(
        attributeHandle,
        "A process Confirm Divestiture request requires attribute identities.");
    if (!seenHandles.insert(attributeHandle).second) {
      throw ProcessFederationServiceProtocolError(
          "A process Confirm Divestiture request repeats an attribute identity.");
    }
    result.attributeHandles.push_back(attributeHandle);
  }
  result.userSuppliedTag = reader.bytes();
  reader.finish();
  if (result.federationName.empty()) {
    throw ProcessFederationServiceProtocolError(
        "A process Confirm Divestiture request requires a federation name.");
  }
  requireNonzero(
      result.divestingFederateId,
      "A process Confirm Divestiture request requires a divesting federate identity.");
  requireNonzero(
      result.objectInstanceHandle,
      "A process Confirm Divestiture request requires an object instance.");
  return result;
}


std::vector<std::uint8_t>
encodeProcessFederationAttributeOwnershipCheckResult(
    ProcessFederationAttributeOwnershipCheckResult const& result) {
  if (result.status != AttributeOwnershipCheckStatus::applied &&
      result.ownedByRequestingFederate) {
    throw ProcessFederationServiceProtocolError(
        "A failed process attribute-ownership check cannot report ownership.");
  }
  PayloadWriter writer;
  writeAttributeOwnershipCheckStatus(writer, result.status);
  writer.unsigned8(result.ownedByRequestingFederate ? 1U : 0U);
  return std::move(writer).finish();
}

ProcessFederationAttributeOwnershipCheckResult
decodeProcessFederationAttributeOwnershipCheckResult(
    std::span<std::uint8_t const> encoded) {
  PayloadReader reader(encoded);
  ProcessFederationAttributeOwnershipCheckResult result;
  result.status = readAttributeOwnershipCheckStatus(reader);
  auto const owned = reader.unsigned8();
  if (owned > 1U) {
    throw ProcessFederationServiceProtocolError(
        "A process attribute-ownership check result has an invalid boolean value.");
  }
  result.ownedByRequestingFederate = owned != 0U;
  reader.finish();
  if (result.status != AttributeOwnershipCheckStatus::applied &&
      result.ownedByRequestingFederate) {
    throw ProcessFederationServiceProtocolError(
        "A failed process attribute-ownership check cannot report ownership.");
  }
  return result;
}

std::vector<std::uint8_t>
encodeProcessFederationAttributeOwnershipQueryResult(
    ProcessFederationAttributeOwnershipQueryResult const& result) {
  if (result.status != AttributeOwnershipQueryStatus::applied &&
      result.recipientCount != 0U) {
    throw ProcessFederationServiceProtocolError(
        "A failed process attribute-ownership query cannot report recipients.");
  }
  PayloadWriter writer;
  writeAttributeOwnershipQueryStatus(writer, result.status);
  writer.unsigned32(result.recipientCount);
  return std::move(writer).finish();
}

ProcessFederationAttributeOwnershipQueryResult
decodeProcessFederationAttributeOwnershipQueryResult(
    std::span<std::uint8_t const> encoded) {
  PayloadReader reader(encoded);
  ProcessFederationAttributeOwnershipQueryResult result;
  result.status = readAttributeOwnershipQueryStatus(reader);
  result.recipientCount = reader.unsigned32();
  reader.finish();
  if (result.status != AttributeOwnershipQueryStatus::applied &&
      result.recipientCount != 0U) {
    throw ProcessFederationServiceProtocolError(
        "A failed process attribute-ownership query cannot report recipients.");
  }
  return result;
}

std::vector<std::uint8_t>
encodeProcessFederationAttributeOwnershipAcquisitionIfAvailableResult(
    ProcessFederationAttributeOwnershipAcquisitionIfAvailableResult const& result) {
  if (result.status !=
          AttributeOwnershipAcquisitionIfAvailableStatus::applied &&
      result.recipientCount != 0U) {
    throw ProcessFederationServiceProtocolError(
        "A failed process attribute-ownership acquisition-if-available result cannot report recipients.");
  }
  PayloadWriter writer;
  writeAttributeOwnershipAcquisitionIfAvailableStatus(writer, result.status);
  writer.unsigned32(result.recipientCount);
  return std::move(writer).finish();
}

ProcessFederationAttributeOwnershipAcquisitionIfAvailableResult
decodeProcessFederationAttributeOwnershipAcquisitionIfAvailableResult(
    std::span<std::uint8_t const> encoded) {
  PayloadReader reader(encoded);
  ProcessFederationAttributeOwnershipAcquisitionIfAvailableResult result;
  result.status = readAttributeOwnershipAcquisitionIfAvailableStatus(reader);
  result.recipientCount = reader.unsigned32();
  reader.finish();
  if (result.status !=
          AttributeOwnershipAcquisitionIfAvailableStatus::applied &&
      result.recipientCount != 0U) {
    throw ProcessFederationServiceProtocolError(
        "A failed process attribute-ownership acquisition-if-available result cannot report recipients.");
  }
  return result;
}

std::vector<std::uint8_t>
encodeProcessFederationAttributeOwnershipAcquisitionResult(
    ProcessFederationAttributeOwnershipAcquisitionResult const& result) {
  if (result.status != AttributeOwnershipAcquisitionStatus::applied &&
      result.recipientCount != 0U) {
    throw ProcessFederationServiceProtocolError(
        "A failed process attribute-ownership acquisition result cannot report recipients.");
  }
  PayloadWriter writer;
  writeAttributeOwnershipAcquisitionStatus(writer, result.status);
  writer.unsigned32(result.recipientCount);
  return std::move(writer).finish();
}

ProcessFederationAttributeOwnershipAcquisitionResult
decodeProcessFederationAttributeOwnershipAcquisitionResult(
    std::span<std::uint8_t const> encoded) {
  PayloadReader reader(encoded);
  ProcessFederationAttributeOwnershipAcquisitionResult result;
  result.status = readAttributeOwnershipAcquisitionStatus(reader);
  result.recipientCount = reader.unsigned32();
  reader.finish();
  if (result.status != AttributeOwnershipAcquisitionStatus::applied &&
      result.recipientCount != 0U) {
    throw ProcessFederationServiceProtocolError(
        "A failed process attribute-ownership acquisition result cannot report recipients.");
  }
  return result;
}

std::vector<std::uint8_t>
encodeProcessFederationAttributeOwnershipReleaseDeniedResult(
    ProcessFederationAttributeOwnershipReleaseDeniedResult const& result) {
  if (result.status != AttributeOwnershipReleaseDeniedStatus::applied &&
      result.recipientCount != 0U) {
    throw ProcessFederationServiceProtocolError(
        "A failed process attribute-ownership release-denied result cannot report recipients.");
  }
  PayloadWriter writer;
  writeAttributeOwnershipReleaseDeniedStatus(writer, result.status);
  writer.unsigned32(result.recipientCount);
  return std::move(writer).finish();
}

ProcessFederationAttributeOwnershipReleaseDeniedResult
decodeProcessFederationAttributeOwnershipReleaseDeniedResult(
    std::span<std::uint8_t const> encoded) {
  PayloadReader reader(encoded);
  ProcessFederationAttributeOwnershipReleaseDeniedResult result;
  result.status = readAttributeOwnershipReleaseDeniedStatus(reader);
  result.recipientCount = reader.unsigned32();
  reader.finish();
  if (result.status != AttributeOwnershipReleaseDeniedStatus::applied &&
      result.recipientCount != 0U) {
    throw ProcessFederationServiceProtocolError(
        "A failed process attribute-ownership release-denied result cannot report recipients.");
  }
  return result;
}

std::vector<std::uint8_t>
encodeProcessFederationAttributeOwnershipAcquisitionCancellationResult(
    ProcessFederationAttributeOwnershipAcquisitionCancellationResult const& result) {
  if (result.status !=
          AttributeOwnershipAcquisitionCancellationStatus::applied &&
      result.recipientCount != 0U) {
    throw ProcessFederationServiceProtocolError(
        "A failed process attribute-ownership acquisition-cancellation result cannot report recipients.");
  }
  PayloadWriter writer;
  writeAttributeOwnershipAcquisitionCancellationStatus(writer, result.status);
  writer.unsigned32(result.recipientCount);
  return std::move(writer).finish();
}

ProcessFederationAttributeOwnershipAcquisitionCancellationResult
decodeProcessFederationAttributeOwnershipAcquisitionCancellationResult(
    std::span<std::uint8_t const> encoded) {
  PayloadReader reader(encoded);
  ProcessFederationAttributeOwnershipAcquisitionCancellationResult result;
  result.status = readAttributeOwnershipAcquisitionCancellationStatus(reader);
  result.recipientCount = reader.unsigned32();
  reader.finish();
  if (result.status !=
          AttributeOwnershipAcquisitionCancellationStatus::applied &&
      result.recipientCount != 0U) {
    throw ProcessFederationServiceProtocolError(
        "A failed process attribute-ownership acquisition-cancellation result cannot report recipients.");
  }
  return result;
}

std::vector<std::uint8_t>
encodeProcessFederationCancelNegotiatedAttributeOwnershipDivestitureResult(
    ProcessFederationCancelNegotiatedAttributeOwnershipDivestitureResult const& result) {
  if (result.status !=
          CancelNegotiatedAttributeOwnershipDivestitureStatus::applied &&
      result.recipientCount != 0U) {
    throw ProcessFederationServiceProtocolError(
        "A failed process negotiated-divestiture cancellation result cannot report recipients.");
  }
  PayloadWriter writer;
  auto const encoded = static_cast<std::uint8_t>(result.status);
  if (encoded > static_cast<std::uint8_t>(
                    CancelNegotiatedAttributeOwnershipDivestitureStatus::
                        inconsistent_catalog)) {
    throw ProcessFederationServiceProtocolError(
        "A process negotiated-divestiture cancellation result has an invalid status.");
  }
  writer.unsigned8(encoded);
  writer.unsigned32(result.recipientCount);
  return std::move(writer).finish();
}

ProcessFederationCancelNegotiatedAttributeOwnershipDivestitureResult
decodeProcessFederationCancelNegotiatedAttributeOwnershipDivestitureResult(
    std::span<std::uint8_t const> encoded) {
  PayloadReader reader(encoded);
  ProcessFederationCancelNegotiatedAttributeOwnershipDivestitureResult result;
  auto const encodedStatus = reader.unsigned8();
  if (encodedStatus > static_cast<std::uint8_t>(
                         CancelNegotiatedAttributeOwnershipDivestitureStatus::
                             inconsistent_catalog)) {
    throw ProcessFederationServiceProtocolError(
        "A process negotiated-divestiture cancellation result has an invalid status.");
  }
  result.status = static_cast<CancelNegotiatedAttributeOwnershipDivestitureStatus>(
      encodedStatus);
  result.recipientCount = reader.unsigned32();
  reader.finish();
  if (result.status !=
          CancelNegotiatedAttributeOwnershipDivestitureStatus::applied &&
      result.recipientCount != 0U) {
    throw ProcessFederationServiceProtocolError(
        "A failed process negotiated-divestiture cancellation result cannot report recipients.");
  }
  return result;
}

std::vector<std::uint8_t>
encodeProcessFederationNegotiatedAttributeOwnershipDivestitureResult(
    ProcessFederationNegotiatedAttributeOwnershipDivestitureResult const& result) {
  if (result.status !=
          NegotiatedAttributeOwnershipDivestitureStatus::applied &&
      result.recipientCount != 0U) {
    throw ProcessFederationServiceProtocolError(
        "A failed process negotiated-divestiture result cannot report recipients.");
  }
  PayloadWriter writer;
  writeNegotiatedAttributeOwnershipDivestitureStatus(writer, result.status);
  writer.unsigned32(result.recipientCount);
  return std::move(writer).finish();
}

ProcessFederationNegotiatedAttributeOwnershipDivestitureResult
decodeProcessFederationNegotiatedAttributeOwnershipDivestitureResult(
    std::span<std::uint8_t const> encoded) {
  PayloadReader reader(encoded);
  ProcessFederationNegotiatedAttributeOwnershipDivestitureResult result;
  result.status = readNegotiatedAttributeOwnershipDivestitureStatus(reader);
  result.recipientCount = reader.unsigned32();
  reader.finish();
  if (result.status !=
          NegotiatedAttributeOwnershipDivestitureStatus::applied &&
      result.recipientCount != 0U) {
    throw ProcessFederationServiceProtocolError(
        "A failed process negotiated-divestiture result cannot report recipients.");
  }
  return result;
}

std::vector<std::uint8_t> encodeProcessFederationConfirmDivestitureResult(
    ProcessFederationConfirmDivestitureResult const& result) {
  if (result.status != ConfirmDivestitureStatus::applied &&
      result.recipientCount != 0U) {
    throw ProcessFederationServiceProtocolError(
        "A failed process Confirm Divestiture result cannot report recipients.");
  }
  PayloadWriter writer;
  writeConfirmDivestitureStatus(writer, result.status);
  writer.unsigned32(result.recipientCount);
  return std::move(writer).finish();
}

ProcessFederationConfirmDivestitureResult
decodeProcessFederationConfirmDivestitureResult(
    std::span<std::uint8_t const> encoded) {
  PayloadReader reader(encoded);
  ProcessFederationConfirmDivestitureResult result;
  result.status = readConfirmDivestitureStatus(reader);
  result.recipientCount = reader.unsigned32();
  reader.finish();
  if (result.status != ConfirmDivestitureStatus::applied &&
      result.recipientCount != 0U) {
    throw ProcessFederationServiceProtocolError(
        "A failed process Confirm Divestiture result cannot report recipients.");
  }
  return result;
}



}  // namespace umbra::detail
