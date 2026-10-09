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

std::vector<std::uint8_t> encodeProcessFederationGetDimensionHandleRequest(
    ProcessFederationGetDimensionHandleRequest const& request) {
  if (request.federationName.empty() || request.dimensionName.empty()) {
    throw ProcessFederationServiceProtocolError(
        "A process dimension lookup requires execution and dimension names.");
  }
  requireNonzero(
      request.federateId,
      "A process dimension lookup requires a federate identity.");
  PayloadWriter writer;
  writer.wideString(request.federationName);
  writer.unsigned64(request.federateId);
  writer.string(request.dimensionName);
  return std::move(writer).finish();
}

ProcessFederationGetDimensionHandleRequest
decodeProcessFederationGetDimensionHandleRequest(
    std::span<std::uint8_t const> encoded) {
  PayloadReader reader(encoded);
  ProcessFederationGetDimensionHandleRequest result;
  result.federationName = reader.wideString();
  result.federateId = reader.unsigned64();
  result.dimensionName = reader.string();
  reader.finish();
  if (result.federationName.empty() || result.dimensionName.empty()) {
    throw ProcessFederationServiceProtocolError(
        "A process dimension lookup requires execution and dimension names.");
  }
  requireNonzero(
      result.federateId,
      "A process dimension lookup requires a federate identity.");
  return result;
}

std::vector<std::uint8_t> encodeProcessFederationDimensionRequest(
    ProcessFederationDimensionRequest const& request) {
  if (request.federationName.empty()) {
    throw ProcessFederationServiceProtocolError(
        "A process dimension request requires a federation name.");
  }
  requireNonzero(
      request.federateId,
      "A process dimension request requires a federate identity.");
  requireNonzero(
      request.dimensionHandle,
      "A process dimension request requires a dimension handle.");
  PayloadWriter writer;
  writer.wideString(request.federationName);
  writer.unsigned64(request.federateId);
  writer.unsigned64(request.dimensionHandle);
  return std::move(writer).finish();
}

ProcessFederationDimensionRequest decodeProcessFederationDimensionRequest(
    std::span<std::uint8_t const> encoded) {
  PayloadReader reader(encoded);
  ProcessFederationDimensionRequest result;
  result.federationName = reader.wideString();
  result.federateId = reader.unsigned64();
  result.dimensionHandle = reader.unsigned64();
  reader.finish();
  if (result.federationName.empty()) {
    throw ProcessFederationServiceProtocolError(
        "A process dimension request requires a federation name.");
  }
  requireNonzero(
      result.federateId,
      "A process dimension request requires a federate identity.");
  requireNonzero(
      result.dimensionHandle,
      "A process dimension request requires a dimension handle.");
  return result;
}

std::vector<std::uint8_t> encodeProcessFederationClassHandleRequest(
    ProcessFederationClassHandleRequest const& request) {
  if (request.federationName.empty()) {
    throw ProcessFederationServiceProtocolError(
        "A process available-dimensions request requires a federation name.");
  }
  requireNonzero(
      request.federateId,
      "A process available-dimensions request requires a federate identity.");
  requireNonzero(
      request.classHandle,
      "A process available-dimensions request requires a class handle.");
  PayloadWriter writer;
  writer.wideString(request.federationName);
  writer.unsigned64(request.federateId);
  writer.unsigned64(request.classHandle);
  return std::move(writer).finish();
}

ProcessFederationClassHandleRequest
decodeProcessFederationClassHandleRequest(
    std::span<std::uint8_t const> encoded) {
  PayloadReader reader(encoded);
  ProcessFederationClassHandleRequest result;
  result.federationName = reader.wideString();
  result.federateId = reader.unsigned64();
  result.classHandle = reader.unsigned64();
  reader.finish();
  if (result.federationName.empty()) {
    throw ProcessFederationServiceProtocolError(
        "A process available-dimensions request requires a federation name.");
  }
  requireNonzero(
      result.federateId,
      "A process available-dimensions request requires a federate identity.");
  requireNonzero(
      result.classHandle,
      "A process available-dimensions request requires a class handle.");
  return result;
}

std::vector<std::uint8_t>
encodeProcessFederationGetTransportationTypeHandleRequest(
    ProcessFederationGetTransportationTypeHandleRequest const& request) {
  if (request.federationName.empty() ||
      request.transportationTypeName.empty()) {
    throw ProcessFederationServiceProtocolError(
        "A process transportation lookup requires execution and transportation names.");
  }
  requireNonzero(
      request.federateId,
      "A process transportation lookup requires a federate identity.");
  PayloadWriter writer;
  writer.wideString(request.federationName);
  writer.unsigned64(request.federateId);
  writer.string(request.transportationTypeName);
  return std::move(writer).finish();
}

ProcessFederationGetTransportationTypeHandleRequest
decodeProcessFederationGetTransportationTypeHandleRequest(
    std::span<std::uint8_t const> encoded) {
  PayloadReader reader(encoded);
  ProcessFederationGetTransportationTypeHandleRequest result;
  result.federationName = reader.wideString();
  result.federateId = reader.unsigned64();
  result.transportationTypeName = reader.string();
  reader.finish();
  if (result.federationName.empty() ||
      result.transportationTypeName.empty()) {
    throw ProcessFederationServiceProtocolError(
        "A process transportation lookup requires execution and transportation names.");
  }
  requireNonzero(
      result.federateId,
      "A process transportation lookup requires a federate identity.");
  return result;
}

std::vector<std::uint8_t> encodeProcessFederationTransportationTypeRequest(
    ProcessFederationTransportationTypeRequest const& request) {
  if (request.federationName.empty()) {
    throw ProcessFederationServiceProtocolError(
        "A process transportation name lookup requires a federation name.");
  }
  requireNonzero(
      request.federateId,
      "A process transportation name lookup requires a federate identity.");
  requireNonzero(
      request.transportationTypeHandle,
      "A process transportation name lookup requires a transportation handle.");
  PayloadWriter writer;
  writer.wideString(request.federationName);
  writer.unsigned64(request.federateId);
  writer.unsigned64(request.transportationTypeHandle);
  return std::move(writer).finish();
}

ProcessFederationTransportationTypeRequest
decodeProcessFederationTransportationTypeRequest(
    std::span<std::uint8_t const> encoded) {
  PayloadReader reader(encoded);
  ProcessFederationTransportationTypeRequest result;
  result.federationName = reader.wideString();
  result.federateId = reader.unsigned64();
  result.transportationTypeHandle = reader.unsigned64();
  reader.finish();
  if (result.federationName.empty()) {
    throw ProcessFederationServiceProtocolError(
        "A process transportation name lookup requires a federation name.");
  }
  requireNonzero(
      result.federateId,
      "A process transportation name lookup requires a federate identity.");
  requireNonzero(
      result.transportationTypeHandle,
      "A process transportation name lookup requires a transportation handle.");
  return result;
}

std::vector<std::uint8_t> encodeProcessFederationCreateRegionRequest(
    ProcessFederationCreateRegionRequest const& request) {
  if (request.federationName.empty()) {
    throw ProcessFederationServiceProtocolError(
        "A process create-region request requires a federation name.");
  }
  requireNonzero(
      request.federateId,
      "A process create-region request requires a federate identity.");
  validateHandleVector(
      request.dimensionHandles,
      "A process create-region request requires sorted, unique dimension handles.");
  PayloadWriter writer;
  writer.wideString(request.federationName);
  writer.unsigned64(request.federateId);
  writer.unsigned64Vector(request.dimensionHandles);
  return std::move(writer).finish();
}

ProcessFederationCreateRegionRequest decodeProcessFederationCreateRegionRequest(
    std::span<std::uint8_t const> encoded) {
  PayloadReader reader(encoded);
  ProcessFederationCreateRegionRequest result;
  result.federationName = reader.wideString();
  result.federateId = reader.unsigned64();
  result.dimensionHandles = reader.unsigned64Vector();
  reader.finish();
  if (result.federationName.empty()) {
    throw ProcessFederationServiceProtocolError(
        "A process create-region request requires a federation name.");
  }
  requireNonzero(
      result.federateId,
      "A process create-region request requires a federate identity.");
  validateHandleVector(
      result.dimensionHandles,
      "A process create-region request requires sorted, unique dimension handles.");
  return result;
}

std::vector<std::uint8_t> encodeProcessFederationRegionSetRequest(
    ProcessFederationRegionSetRequest const& request) {
  if (request.federationName.empty()) {
    throw ProcessFederationServiceProtocolError(
        "A process region-set request requires a federation name.");
  }
  requireNonzero(
      request.federateId,
      "A process region-set request requires a federate identity.");
  validateHandleVector(
      request.regionHandles,
      "A process region-set request requires sorted, unique region handles.");
  PayloadWriter writer;
  writer.wideString(request.federationName);
  writer.unsigned64(request.federateId);
  writer.unsigned64Vector(request.regionHandles);
  return std::move(writer).finish();
}

ProcessFederationRegionSetRequest decodeProcessFederationRegionSetRequest(
    std::span<std::uint8_t const> encoded) {
  PayloadReader reader(encoded);
  ProcessFederationRegionSetRequest result;
  result.federationName = reader.wideString();
  result.federateId = reader.unsigned64();
  result.regionHandles = reader.unsigned64Vector();
  reader.finish();
  if (result.federationName.empty()) {
    throw ProcessFederationServiceProtocolError(
        "A process region-set request requires a federation name.");
  }
  requireNonzero(
      result.federateId,
      "A process region-set request requires a federate identity.");
  validateHandleVector(
      result.regionHandles,
      "A process region-set request requires sorted, unique region handles.");
  return result;
}

std::vector<std::uint8_t> encodeProcessFederationRegionRequest(
    ProcessFederationRegionRequest const& request) {
  if (request.federationName.empty()) {
    throw ProcessFederationServiceProtocolError(
        "A process region request requires a federation name.");
  }
  requireNonzero(
      request.federateId,
      "A process region request requires a federate identity.");
  requireNonzero(
      request.regionHandle,
      "A process region request requires a region handle.");
  PayloadWriter writer;
  writer.wideString(request.federationName);
  writer.unsigned64(request.federateId);
  writer.unsigned64(request.regionHandle);
  return std::move(writer).finish();
}

ProcessFederationRegionRequest decodeProcessFederationRegionRequest(
    std::span<std::uint8_t const> encoded) {
  PayloadReader reader(encoded);
  ProcessFederationRegionRequest result;
  result.federationName = reader.wideString();
  result.federateId = reader.unsigned64();
  result.regionHandle = reader.unsigned64();
  reader.finish();
  if (result.federationName.empty()) {
    throw ProcessFederationServiceProtocolError(
        "A process region request requires a federation name.");
  }
  requireNonzero(
      result.federateId,
      "A process region request requires a federate identity.");
  requireNonzero(
      result.regionHandle,
      "A process region request requires a region handle.");
  return result;
}

std::vector<std::uint8_t> encodeProcessFederationGetRangeBoundsRequest(
    ProcessFederationGetRangeBoundsRequest const& request) {
  if (request.federationName.empty()) {
    throw ProcessFederationServiceProtocolError(
        "A process range-bounds request requires a federation name.");
  }
  requireNonzero(
      request.federateId,
      "A process range-bounds request requires a federate identity.");
  requireNonzero(
      request.regionHandle,
      "A process range-bounds request requires a region handle.");
  requireNonzero(
      request.dimensionHandle,
      "A process range-bounds request requires a dimension handle.");
  PayloadWriter writer;
  writer.wideString(request.federationName);
  writer.unsigned64(request.federateId);
  writer.unsigned64(request.regionHandle);
  writer.unsigned64(request.dimensionHandle);
  return std::move(writer).finish();
}

ProcessFederationGetRangeBoundsRequest
decodeProcessFederationGetRangeBoundsRequest(
    std::span<std::uint8_t const> encoded) {
  PayloadReader reader(encoded);
  ProcessFederationGetRangeBoundsRequest result;
  result.federationName = reader.wideString();
  result.federateId = reader.unsigned64();
  result.regionHandle = reader.unsigned64();
  result.dimensionHandle = reader.unsigned64();
  reader.finish();
  if (result.federationName.empty()) {
    throw ProcessFederationServiceProtocolError(
        "A process range-bounds request requires a federation name.");
  }
  requireNonzero(
      result.federateId,
      "A process range-bounds request requires a federate identity.");
  requireNonzero(
      result.regionHandle,
      "A process range-bounds request requires a region handle.");
  requireNonzero(
      result.dimensionHandle,
      "A process range-bounds request requires a dimension handle.");
  return result;
}

std::vector<std::uint8_t> encodeProcessFederationSetRangeBoundsRequest(
    ProcessFederationSetRangeBoundsRequest const& request) {
  if (request.federationName.empty()) {
    throw ProcessFederationServiceProtocolError(
        "A process set-range-bounds request requires a federation name.");
  }
  requireNonzero(
      request.federateId,
      "A process set-range-bounds request requires a federate identity.");
  requireNonzero(
      request.regionHandle,
      "A process set-range-bounds request requires a region handle.");
  requireNonzero(
      request.dimensionHandle,
      "A process set-range-bounds request requires a dimension handle.");
  PayloadWriter writer;
  writer.wideString(request.federationName);
  writer.unsigned64(request.federateId);
  writer.unsigned64(request.regionHandle);
  writer.unsigned64(request.dimensionHandle);
  writer.unsigned64(static_cast<std::uint64_t>(request.lowerBound));
  writer.unsigned64(static_cast<std::uint64_t>(request.upperBound));
  return std::move(writer).finish();
}

ProcessFederationSetRangeBoundsRequest
decodeProcessFederationSetRangeBoundsRequest(
    std::span<std::uint8_t const> encoded) {
  PayloadReader reader(encoded);
  ProcessFederationSetRangeBoundsRequest result;
  result.federationName = reader.wideString();
  result.federateId = reader.unsigned64();
  result.regionHandle = reader.unsigned64();
  result.dimensionHandle = reader.unsigned64();
  auto const lower = reader.unsigned64();
  auto const upper = reader.unsigned64();
  reader.finish();
  if (result.federationName.empty()) {
    throw ProcessFederationServiceProtocolError(
        "A process set-range-bounds request requires a federation name.");
  }
  requireNonzero(
      result.federateId,
      "A process set-range-bounds request requires a federate identity.");
  requireNonzero(
      result.regionHandle,
      "A process set-range-bounds request requires a region handle.");
  requireNonzero(
      result.dimensionHandle,
      "A process set-range-bounds request requires a dimension handle.");
  if (lower > std::numeric_limits<unsigned long>::max() ||
      upper > std::numeric_limits<unsigned long>::max()) {
    throw ProcessFederationServiceProtocolError(
        "A process set-range-bounds request exceeds the native bound type.");
  }
  result.lowerBound = static_cast<unsigned long>(lower);
  result.upperBound = static_cast<unsigned long>(upper);
  return result;
}

std::vector<std::uint8_t> encodeProcessFederationCreateRegionResult(
    ProcessFederationCreateRegionResult const& result) {
  if (result.status == RegionServiceStatus::applied) {
    requireNonzero(
        result.regionHandle,
        "A successful process create-region result requires a region handle.");
  } else if (result.regionHandle != 0U) {
    throw ProcessFederationServiceProtocolError(
        "A failed process create-region result cannot carry a region handle.");
  }
  PayloadWriter writer;
  writeRegionServiceStatus(writer, result.status);
  writer.unsigned64(result.regionHandle);
  return std::move(writer).finish();
}

ProcessFederationCreateRegionResult decodeProcessFederationCreateRegionResult(
    std::span<std::uint8_t const> encoded) {
  PayloadReader reader(encoded);
  ProcessFederationCreateRegionResult result;
  result.status = readRegionServiceStatus(reader);
  result.regionHandle = reader.unsigned64();
  reader.finish();
  if (result.status == RegionServiceStatus::applied) {
    requireNonzero(
        result.regionHandle,
        "A successful process create-region result requires a region handle.");
  } else if (result.regionHandle != 0U) {
    throw ProcessFederationServiceProtocolError(
        "A failed process create-region result cannot carry a region handle.");
  }
  return result;
}

std::vector<std::uint8_t> encodeProcessFederationDimensionSetResult(
    ProcessFederationDimensionSetResult const& result) {
  if (result.status != RegionServiceStatus::applied &&
      !result.dimensionHandles.empty()) {
    throw ProcessFederationServiceProtocolError(
        "A failed process dimension-set result cannot carry handles.");
  }
  validateHandleVector(
      result.dimensionHandles,
      "A process dimension-set result requires sorted, unique handles.");
  PayloadWriter writer;
  writeRegionServiceStatus(writer, result.status);
  writer.unsigned64Vector(result.dimensionHandles);
  return std::move(writer).finish();
}

ProcessFederationDimensionSetResult decodeProcessFederationDimensionSetResult(
    std::span<std::uint8_t const> encoded) {
  PayloadReader reader(encoded);
  ProcessFederationDimensionSetResult result;
  result.status = readRegionServiceStatus(reader);
  result.dimensionHandles = reader.unsigned64Vector();
  reader.finish();
  validateHandleVector(
      result.dimensionHandles,
      "A process dimension-set result requires sorted, unique handles.");
  if (result.status != RegionServiceStatus::applied &&
      !result.dimensionHandles.empty()) {
    throw ProcessFederationServiceProtocolError(
        "A failed process dimension-set result cannot carry handles.");
  }
  return result;
}

std::vector<std::uint8_t> encodeProcessFederationRangeBoundsResult(
    ProcessFederationRangeBoundsResult const& result) {
  if (result.status != RegionServiceStatus::applied &&
      (result.lowerBound != 0UL || result.upperBound != 0UL)) {
    throw ProcessFederationServiceProtocolError(
        "A failed process range-bounds result cannot carry bounds.");
  }
  PayloadWriter writer;
  writeRegionServiceStatus(writer, result.status);
  writer.unsigned64(static_cast<std::uint64_t>(result.lowerBound));
  writer.unsigned64(static_cast<std::uint64_t>(result.upperBound));
  return std::move(writer).finish();
}

ProcessFederationRangeBoundsResult decodeProcessFederationRangeBoundsResult(
    std::span<std::uint8_t const> encoded) {
  PayloadReader reader(encoded);
  ProcessFederationRangeBoundsResult result;
  result.status = readRegionServiceStatus(reader);
  auto const lower = reader.unsigned64();
  auto const upper = reader.unsigned64();
  reader.finish();
  if (lower > std::numeric_limits<unsigned long>::max() ||
      upper > std::numeric_limits<unsigned long>::max()) {
    throw ProcessFederationServiceProtocolError(
        "A process range-bounds result exceeds the native bound type.");
  }
  result.lowerBound = static_cast<unsigned long>(lower);
  result.upperBound = static_cast<unsigned long>(upper);
  if (result.status != RegionServiceStatus::applied &&
      (result.lowerBound != 0UL || result.upperBound != 0UL)) {
    throw ProcessFederationServiceProtocolError(
        "A failed process range-bounds result cannot carry bounds.");
  }
  return result;
}

std::vector<std::uint8_t> encodeProcessFederationDimensionUpperBoundResult(
    ProcessFederationDimensionUpperBoundResult const& result) {
  if (!result.found && result.upperBound != 0UL) {
    throw ProcessFederationServiceProtocolError(
        "An absent process dimension bound cannot carry a value.");
  }
  PayloadWriter writer;
  writer.unsigned8(result.found ? 1U : 0U);
  writer.unsigned64(static_cast<std::uint64_t>(result.upperBound));
  return std::move(writer).finish();
}

ProcessFederationDimensionUpperBoundResult
decodeProcessFederationDimensionUpperBoundResult(
    std::span<std::uint8_t const> encoded) {
  PayloadReader reader(encoded);
  auto const found = reader.unsigned8();
  if (found > 1U) {
    throw ProcessFederationServiceProtocolError(
        "A process dimension bound result has an invalid presence marker.");
  }
  auto const upper = reader.unsigned64();
  reader.finish();
  if (upper > std::numeric_limits<unsigned long>::max()) {
    throw ProcessFederationServiceProtocolError(
        "A process dimension bound result exceeds the native bound type.");
  }
  ProcessFederationDimensionUpperBoundResult result;
  result.found = found != 0U;
  result.upperBound = static_cast<unsigned long>(upper);
  if (!result.found && result.upperBound != 0UL) {
    throw ProcessFederationServiceProtocolError(
        "An absent process dimension bound cannot carry a value.");
  }
  return result;
}

std::vector<std::uint8_t> encodeProcessFederationAvailableDimensionsResult(
    ProcessFederationAvailableDimensionsResult const& result) {
  if (!result.found && !result.dimensionHandles.empty()) {
    throw ProcessFederationServiceProtocolError(
        "An absent process available-dimensions result cannot carry handles.");
  }
  validateHandleVector(
      result.dimensionHandles,
      "A process available-dimensions result requires sorted, unique handles.");
  PayloadWriter writer;
  writer.unsigned8(result.found ? 1U : 0U);
  writer.unsigned64Vector(result.dimensionHandles);
  return std::move(writer).finish();
}

ProcessFederationAvailableDimensionsResult
decodeProcessFederationAvailableDimensionsResult(
    std::span<std::uint8_t const> encoded) {
  PayloadReader reader(encoded);
  auto const found = reader.unsigned8();
  if (found > 1U) {
    throw ProcessFederationServiceProtocolError(
        "A process available-dimensions result has an invalid presence marker.");
  }
  ProcessFederationAvailableDimensionsResult result;
  result.found = found != 0U;
  result.dimensionHandles = reader.unsigned64Vector();
  reader.finish();
  validateHandleVector(
      result.dimensionHandles,
      "A process available-dimensions result requires sorted, unique handles.");
  if (!result.found && !result.dimensionHandles.empty()) {
    throw ProcessFederationServiceProtocolError(
        "An absent process available-dimensions result cannot carry handles.");
  }
  return result;
}
}  // namespace umbra::detail
