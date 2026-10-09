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
encodeProcessFederationRegisterObjectInstanceRequest(
    ProcessFederationRegisterObjectInstanceRequest const& request) {
  if (request.federationName.empty()) {
    throw ProcessFederationServiceProtocolError(
        "A process federation object registration requires a federation name.");
  }
  requireNonzero(
      request.federateId,
      "A process federation object registration requires a federate identity.");
  requireNonzero(
      request.objectClassHandle,
      "A process federation object registration requires an object class.");
  if (request.requestedObjectInstanceName.has_value() &&
      request.requestedObjectInstanceName->empty()) {
    throw ProcessFederationServiceProtocolError(
        "A requested process object instance name cannot be empty.");
  }
  PayloadWriter writer;
  writer.wideString(request.federationName);
  writer.unsigned64(request.federateId);
  writer.unsigned64(request.objectClassHandle);
  writer.unsigned8(request.requestedObjectInstanceName.has_value() ? 1U : 0U);
  if (request.requestedObjectInstanceName.has_value()) {
    writer.wideString(*request.requestedObjectInstanceName);
  }
  return std::move(writer).finish();
}

ProcessFederationRegisterObjectInstanceRequest
decodeProcessFederationRegisterObjectInstanceRequest(
    std::span<std::uint8_t const> encoded) {
  PayloadReader reader(encoded);
  ProcessFederationRegisterObjectInstanceRequest result;
  result.federationName = reader.wideString();
  result.federateId = reader.unsigned64();
  result.objectClassHandle = reader.unsigned64();
  auto const hasRequestedName = reader.unsigned8();
  if (hasRequestedName > 1U) {
    throw ProcessFederationServiceProtocolError(
        "A process federation object registration has an invalid name marker.");
  }
  if (hasRequestedName != 0U) {
    result.requestedObjectInstanceName = reader.wideString();
  }
  reader.finish();
  if (result.federationName.empty()) {
    throw ProcessFederationServiceProtocolError(
        "A process federation object registration requires a federation name.");
  }
  requireNonzero(
      result.federateId,
      "A process federation object registration requires a federate identity.");
  requireNonzero(
      result.objectClassHandle,
      "A process federation object registration requires an object class.");
  if (result.requestedObjectInstanceName.has_value() &&
      result.requestedObjectInstanceName->empty()) {
    throw ProcessFederationServiceProtocolError(
        "A requested process object instance name cannot be empty.");
  }
  return result;
}

std::vector<std::uint8_t>
encodeProcessFederationLocalDeleteObjectInstanceRequest(
    ProcessFederationLocalDeleteObjectInstanceRequest const& request) {
  if (request.federationName.empty()) {
    throw ProcessFederationServiceProtocolError(
        "A process local object deletion requires a federation name.");
  }
  requireNonzero(
      request.federateId,
      "A process local object deletion requires a federate identity.");
  requireNonzero(
      request.objectInstanceHandle,
      "A process local object deletion requires an object instance handle.");
  PayloadWriter writer;
  writer.wideString(request.federationName);
  writer.unsigned64(request.federateId);
  writer.unsigned64(request.objectInstanceHandle);
  return std::move(writer).finish();
}

ProcessFederationLocalDeleteObjectInstanceRequest
decodeProcessFederationLocalDeleteObjectInstanceRequest(
    std::span<std::uint8_t const> encoded) {
  PayloadReader reader(encoded);
  ProcessFederationLocalDeleteObjectInstanceRequest result;
  result.federationName = reader.wideString();
  result.federateId = reader.unsigned64();
  result.objectInstanceHandle = reader.unsigned64();
  reader.finish();
  if (result.federationName.empty()) {
    throw ProcessFederationServiceProtocolError(
        "A process local object deletion requires a federation name.");
  }
  requireNonzero(
      result.federateId,
      "A process local object deletion requires a federate identity.");
  requireNonzero(
      result.objectInstanceHandle,
      "A process local object deletion requires an object instance handle.");
  return result;
}

std::vector<std::uint8_t> encodeProcessFederationDeleteObjectInstanceRequest(
    ProcessFederationDeleteObjectInstanceRequest const& request) {
  if (request.federationName.empty()) {
    throw ProcessFederationServiceProtocolError(
        "A process object deletion requires a federation name.");
  }
  requireNonzero(
      request.federateId,
      "A process object deletion requires a federate identity.");
  requireNonzero(
      request.objectInstanceHandle,
      "A process object deletion requires an object instance handle.");
  PayloadWriter writer;
  writer.wideString(request.federationName);
  writer.unsigned64(request.federateId);
  writer.unsigned64(request.objectInstanceHandle);
  writer.bytes(request.userSuppliedTag);
  writeOptionalLogicalTime(writer, request.timestamp);
  return std::move(writer).finish();
}

ProcessFederationDeleteObjectInstanceRequest
decodeProcessFederationDeleteObjectInstanceRequest(
    std::span<std::uint8_t const> encoded) {
  PayloadReader reader(encoded);
  ProcessFederationDeleteObjectInstanceRequest result;
  result.federationName = reader.wideString();
  result.federateId = reader.unsigned64();
  result.objectInstanceHandle = reader.unsigned64();
  result.userSuppliedTag = reader.bytes();
  result.timestamp = readOptionalLogicalTime(reader);
  reader.finish();
  if (result.federationName.empty()) {
    throw ProcessFederationServiceProtocolError(
        "A process object deletion requires a federation name.");
  }
  requireNonzero(
      result.federateId,
      "A process object deletion requires a federate identity.");
  requireNonzero(
      result.objectInstanceHandle,
      "A process object deletion requires an object instance handle.");
  return result;
}

std::vector<std::uint8_t>
encodeProcessFederationReserveObjectInstanceNameRequest(
    ProcessFederationReserveObjectInstanceNameRequest const& request) {
  if (request.federationName.empty() || request.objectInstanceName.empty()) {
    throw ProcessFederationServiceProtocolError(
        "A process object-instance name reservation requires execution and name values.");
  }
  requireNonzero(
      request.federateId,
      "A process object-instance name reservation requires a federate identity.");
  PayloadWriter writer;
  writer.wideString(request.federationName);
  writer.unsigned64(request.federateId);
  writer.wideString(request.objectInstanceName);
  return std::move(writer).finish();
}

ProcessFederationReserveObjectInstanceNameRequest
decodeProcessFederationReserveObjectInstanceNameRequest(
    std::span<std::uint8_t const> encoded) {
  PayloadReader reader(encoded);
  ProcessFederationReserveObjectInstanceNameRequest result;
  result.federationName = reader.wideString();
  result.federateId = reader.unsigned64();
  result.objectInstanceName = reader.wideString();
  reader.finish();
  if (result.federationName.empty() || result.objectInstanceName.empty()) {
    throw ProcessFederationServiceProtocolError(
        "A process object-instance name reservation requires execution and name values.");
  }
  requireNonzero(
      result.federateId,
      "A process object-instance name reservation requires a federate identity.");
  return result;
}

std::vector<std::uint8_t>
encodeProcessFederationReserveMultipleObjectInstanceNamesRequest(
    ProcessFederationReserveMultipleObjectInstanceNamesRequest const& request) {
  if (request.federationName.empty()) {
    throw ProcessFederationServiceProtocolError(
        "A process multiple object-instance name reservation requires a federation name.");
  }
  requireNonzero(
      request.federateId,
      "A process multiple object-instance name reservation requires a federate identity.");
  PayloadWriter writer;
  writer.wideString(request.federationName);
  writer.unsigned64(request.federateId);
  writer.wideStringVector(
      std::vector<std::wstring>(request.objectInstanceNames.begin(),
                                request.objectInstanceNames.end()));
  return std::move(writer).finish();
}

ProcessFederationReserveMultipleObjectInstanceNamesRequest
decodeProcessFederationReserveMultipleObjectInstanceNamesRequest(
    std::span<std::uint8_t const> encoded) {
  PayloadReader reader(encoded);
  ProcessFederationReserveMultipleObjectInstanceNamesRequest result;
  result.federationName = reader.wideString();
  result.federateId = reader.unsigned64();
  auto const names = reader.wideStringVector();
  reader.finish();
  if (result.federationName.empty()) {
    throw ProcessFederationServiceProtocolError(
        "A process multiple object-instance name reservation requires a federation name.");
  }
  requireNonzero(
      result.federateId,
      "A process multiple object-instance name reservation requires a federate identity.");
  for (auto const& name : names) {
    if (!result.objectInstanceNames.insert(name).second) {
      throw ProcessFederationServiceProtocolError(
          "A process multiple object-instance name request contains a duplicate name.");
    }
  }
  return result;
}

std::vector<std::uint8_t>
encodeProcessFederationRegisterObjectInstanceWithRegionsRequest(
    ProcessFederationRegisterObjectInstanceWithRegionsRequest const& request) {
  if (request.federationName.empty()) {
    throw ProcessFederationServiceProtocolError(
        "A process regional registration requires a federation name.");
  }
  requireNonzero(
      request.federateId,
      "A process regional registration requires a federate identity.");
  requireNonzero(
      request.objectClassHandle,
      "A process regional registration requires an object class.");
  if (request.requestedObjectInstanceName.has_value() &&
      request.requestedObjectInstanceName->empty()) {
    throw ProcessFederationServiceProtocolError(
        "A requested process regional object instance name cannot be empty.");
  }
  PayloadWriter writer;
  writer.wideString(request.federationName);
  writer.unsigned64(request.federateId);
  writer.unsigned64(request.objectClassHandle);
  writeAttributeRegionMap(writer, request.updateRegionsByAttribute);
  writer.unsigned8(request.requestedObjectInstanceName.has_value() ? 1U : 0U);
  if (request.requestedObjectInstanceName.has_value()) {
    writer.wideString(*request.requestedObjectInstanceName);
  }
  return std::move(writer).finish();
}

ProcessFederationRegisterObjectInstanceWithRegionsRequest
decodeProcessFederationRegisterObjectInstanceWithRegionsRequest(
    std::span<std::uint8_t const> encoded) {
  PayloadReader reader(encoded);
  ProcessFederationRegisterObjectInstanceWithRegionsRequest result;
  result.federationName = reader.wideString();
  result.federateId = reader.unsigned64();
  result.objectClassHandle = reader.unsigned64();
  result.updateRegionsByAttribute = readAttributeRegionMap(reader);
  auto const hasRequestedName = reader.unsigned8();
  if (hasRequestedName > 1U) {
    throw ProcessFederationServiceProtocolError(
        "A process regional registration has an invalid name marker.");
  }
  if (hasRequestedName != 0U) {
    result.requestedObjectInstanceName = reader.wideString();
  }
  reader.finish();
  if (result.federationName.empty()) {
    throw ProcessFederationServiceProtocolError(
        "A process regional registration requires a federation name.");
  }
  requireNonzero(
      result.federateId,
      "A process regional registration requires a federate identity.");
  requireNonzero(
      result.objectClassHandle,
      "A process regional registration requires an object class.");
  if (result.requestedObjectInstanceName.has_value() &&
      result.requestedObjectInstanceName->empty()) {
    throw ProcessFederationServiceProtocolError(
        "A requested process regional object instance name cannot be empty.");
  }
  return result;
}

std::vector<std::uint8_t>
encodeProcessFederationObjectInstanceRegionAssociationRequest(
    ProcessFederationObjectInstanceRegionAssociationRequest const& request) {
  if (request.federationName.empty()) {
    throw ProcessFederationServiceProtocolError(
        "A process object regional association requires a federation name.");
  }
  requireNonzero(
      request.federateId,
      "A process object regional association requires a federate identity.");
  requireNonzero(
      request.objectInstanceHandle,
      "A process object regional association requires an object instance.");
  PayloadWriter writer;
  writer.wideString(request.federationName);
  writer.unsigned64(request.federateId);
  writer.unsigned64(request.objectInstanceHandle);
  writeAttributeRegionMap(writer, request.attributesAndRegions);
  return std::move(writer).finish();
}

ProcessFederationObjectInstanceRegionAssociationRequest
decodeProcessFederationObjectInstanceRegionAssociationRequest(
    std::span<std::uint8_t const> encoded) {
  PayloadReader reader(encoded);
  ProcessFederationObjectInstanceRegionAssociationRequest result;
  result.federationName = reader.wideString();
  result.federateId = reader.unsigned64();
  result.objectInstanceHandle = reader.unsigned64();
  result.attributesAndRegions = readAttributeRegionMap(reader);
  reader.finish();
  if (result.federationName.empty()) {
    throw ProcessFederationServiceProtocolError(
        "A process object regional association requires a federation name.");
  }
  requireNonzero(
      result.federateId,
      "A process object regional association requires a federate identity.");
  requireNonzero(
      result.objectInstanceHandle,
      "A process object regional association requires an object instance.");
  return result;
}

std::vector<std::uint8_t>
encodeProcessFederationRegisterObjectInstanceResult(
    ProcessFederationRegisterObjectInstanceResult const& result) {
  auto const status = static_cast<std::uint8_t>(result.status);
  if (status > static_cast<std::uint8_t>(
                   ObjectInstanceRegistrationStatus::inconsistent_catalog)) {
    throw ProcessFederationServiceProtocolError(
        "A process federation object registration result has an invalid status.");
  }
  if (result.status == ObjectInstanceRegistrationStatus::applied) {
    requireNonzero(
        result.objectInstanceHandle,
        "A process federation object registration result requires a non-zero handle.");
    if (result.objectInstanceName.empty()) {
      throw ProcessFederationServiceProtocolError(
          "A process federation object registration result requires an instance name.");
    }
  } else if (result.objectInstanceHandle != 0U ||
             !result.objectInstanceName.empty()) {
    throw ProcessFederationServiceProtocolError(
        "A failed process federation object registration result cannot carry an identity.");
  }
  PayloadWriter writer;
  writer.unsigned64(result.objectInstanceHandle);
  writer.wideString(result.objectInstanceName);
  writer.unsigned8(status);
  return std::move(writer).finish();
}

ProcessFederationRegisterObjectInstanceResult
decodeProcessFederationRegisterObjectInstanceResult(
    std::span<std::uint8_t const> encoded) {
  PayloadReader reader(encoded);
  ProcessFederationRegisterObjectInstanceResult result;
  result.objectInstanceHandle = reader.unsigned64();
  result.objectInstanceName = reader.wideString();
  auto const status = reader.unsigned8();
  reader.finish();
  if (status > static_cast<std::uint8_t>(
                   ObjectInstanceRegistrationStatus::inconsistent_catalog)) {
    throw ProcessFederationServiceProtocolError(
        "A process federation object registration result has an invalid status.");
  }
  result.status = static_cast<ObjectInstanceRegistrationStatus>(status);
  if (result.status == ObjectInstanceRegistrationStatus::applied) {
    requireNonzero(
        result.objectInstanceHandle,
        "A process federation object registration result requires a non-zero handle.");
    if (result.objectInstanceName.empty()) {
      throw ProcessFederationServiceProtocolError(
          "A process federation object registration result requires an instance name.");
    }
  } else if (result.objectInstanceHandle != 0U ||
             !result.objectInstanceName.empty()) {
    throw ProcessFederationServiceProtocolError(
        "A failed process federation object registration result cannot carry an identity.");
  }
  return result;
}

std::vector<std::uint8_t>
encodeProcessFederationReserveObjectInstanceNameResult(
    ProcessFederationReserveObjectInstanceNameResult const& result) {
  if (result.objectInstanceName.empty()) {
    throw ProcessFederationServiceProtocolError(
        "A process object-instance name reservation result requires a name.");
  }
  PayloadWriter writer;
  writer.unsigned8(result.succeeded ? 1U : 0U);
  writer.wideString(result.objectInstanceName);
  auto const status = static_cast<std::uint8_t>(result.status);
  if (status > static_cast<std::uint8_t>(
                   ObjectInstanceNameReservationStatus::callback_route_missing)) {
    throw ProcessFederationServiceProtocolError(
        "A process object-instance name reservation result has an invalid status.");
  }
  if (result.status != ObjectInstanceNameReservationStatus::applied &&
      result.succeeded) {
    throw ProcessFederationServiceProtocolError(
        "A failed process object-instance name reservation cannot succeed.");
  }
  writer.unsigned8(status);
  return std::move(writer).finish();
}

ProcessFederationReserveObjectInstanceNameResult
decodeProcessFederationReserveObjectInstanceNameResult(
    std::span<std::uint8_t const> encoded) {
  PayloadReader reader(encoded);
  ProcessFederationReserveObjectInstanceNameResult result;
  auto const succeeded = reader.unsigned8();
  if (succeeded > 1U) {
    throw ProcessFederationServiceProtocolError(
        "A process object-instance name reservation result has an invalid outcome marker.");
  }
  result.succeeded = succeeded != 0U;
  result.objectInstanceName = reader.wideString();
  auto const status = reader.unsigned8();
  reader.finish();
  if (result.objectInstanceName.empty()) {
    throw ProcessFederationServiceProtocolError(
        "A process object-instance name reservation result requires a name.");
  }
  if (status > static_cast<std::uint8_t>(
                   ObjectInstanceNameReservationStatus::callback_route_missing)) {
    throw ProcessFederationServiceProtocolError(
        "A process object-instance name reservation result has an invalid status.");
  }
  result.status = static_cast<ObjectInstanceNameReservationStatus>(status);
  if (result.status != ObjectInstanceNameReservationStatus::applied &&
      result.succeeded) {
    throw ProcessFederationServiceProtocolError(
        "A failed process object-instance name reservation cannot succeed.");
  }
  return result;
}

std::vector<std::uint8_t>
encodeProcessFederationObjectInstanceNameReleaseResult(
    ProcessFederationObjectInstanceNameReleaseResult const& result) {
  auto const status = static_cast<std::uint8_t>(result.status);
  if (status > static_cast<std::uint8_t>(
                   ObjectInstanceNameReservationStatus::callback_route_missing)) {
    throw ProcessFederationServiceProtocolError(
        "A process object-instance name release result has an invalid status.");
  }
  PayloadWriter writer;
  writer.unsigned8(status);
  return std::move(writer).finish();
}

ProcessFederationObjectInstanceNameReleaseResult
decodeProcessFederationObjectInstanceNameReleaseResult(
    std::span<std::uint8_t const> encoded) {
  PayloadReader reader(encoded);
  auto const status = reader.unsigned8();
  reader.finish();
  if (status > static_cast<std::uint8_t>(
                   ObjectInstanceNameReservationStatus::callback_route_missing)) {
    throw ProcessFederationServiceProtocolError(
        "A process object-instance name release result has an invalid status.");
  }
  return ProcessFederationObjectInstanceNameReleaseResult{
      static_cast<ObjectInstanceNameReservationStatus>(status)};
}

std::vector<std::uint8_t>
encodeProcessFederationReserveMultipleObjectInstanceNamesResult(
    ProcessFederationReserveMultipleObjectInstanceNamesResult const& result) {
  auto const status = static_cast<std::uint8_t>(result.status);
  if (status > static_cast<std::uint8_t>(
                   ObjectInstanceNameReservationStatus::callback_route_missing)) {
    throw ProcessFederationServiceProtocolError(
        "A process multiple object-instance name reservation result has an invalid status.");
  }
  PayloadWriter writer;
  writer.unsigned8(status);
  writer.wideStringVector(
      std::vector<std::wstring>(result.succeededNames.begin(),
                                result.succeededNames.end()));
  writer.wideStringVector(
      std::vector<std::wstring>(result.failedNames.begin(),
                                result.failedNames.end()));
  return std::move(writer).finish();
}

ProcessFederationReserveMultipleObjectInstanceNamesResult
decodeProcessFederationReserveMultipleObjectInstanceNamesResult(
    std::span<std::uint8_t const> encoded) {
  PayloadReader reader(encoded);
  ProcessFederationReserveMultipleObjectInstanceNamesResult result;
  auto const status = reader.unsigned8();
  auto const succeededNames = reader.wideStringVector();
  auto const failedNames = reader.wideStringVector();
  reader.finish();
  if (status > static_cast<std::uint8_t>(
                   ObjectInstanceNameReservationStatus::callback_route_missing)) {
    throw ProcessFederationServiceProtocolError(
        "A process multiple object-instance name reservation result has an invalid status.");
  }
  result.status = static_cast<ObjectInstanceNameReservationStatus>(status);
  for (auto const& name : succeededNames) {
    if (!result.succeededNames.insert(name).second) {
      throw ProcessFederationServiceProtocolError(
          "A process multiple object-instance name reservation result contains a duplicate success name.");
    }
  }
  for (auto const& name : failedNames) {
    if (!result.failedNames.insert(name).second ||
        result.succeededNames.contains(name)) {
      throw ProcessFederationServiceProtocolError(
          "A process multiple object-instance name reservation result contains overlapping names.");
    }
  }
  return result;
}

std::vector<std::uint8_t>
encodeProcessFederationReleaseMultipleObjectInstanceNamesResult(
    ProcessFederationReleaseMultipleObjectInstanceNamesResult const& result) {
  auto const status = static_cast<std::uint8_t>(result.status);
  if (status > static_cast<std::uint8_t>(
                   ObjectInstanceNameReservationStatus::callback_route_missing)) {
    throw ProcessFederationServiceProtocolError(
        "A process multiple object-instance name release result has an invalid status.");
  }
  PayloadWriter writer;
  writer.unsigned8(status);
  return std::move(writer).finish();
}

ProcessFederationReleaseMultipleObjectInstanceNamesResult
decodeProcessFederationReleaseMultipleObjectInstanceNamesResult(
    std::span<std::uint8_t const> encoded) {
  PayloadReader reader(encoded);
  auto const status = reader.unsigned8();
  reader.finish();
  if (status > static_cast<std::uint8_t>(
                   ObjectInstanceNameReservationStatus::callback_route_missing)) {
    throw ProcessFederationServiceProtocolError(
        "A process multiple object-instance name release result has an invalid status.");
  }
  return ProcessFederationReleaseMultipleObjectInstanceNamesResult{
      static_cast<ObjectInstanceNameReservationStatus>(status)};
}

std::vector<std::uint8_t> encodeProcessFederationRegionStatusResult(
    ProcessFederationRegionStatusResult const& result) {
  PayloadWriter writer;
  writeRegionServiceStatus(writer, result.status);
  return std::move(writer).finish();
}

ProcessFederationRegionStatusResult decodeProcessFederationRegionStatusResult(
    std::span<std::uint8_t const> encoded) {
  PayloadReader reader(encoded);
  ProcessFederationRegionStatusResult result;
  result.status = readRegionServiceStatus(reader);
  reader.finish();
  return result;
}

std::vector<std::uint8_t>
encodeProcessFederationLocalDeleteObjectInstanceResult(
    ProcessFederationLocalDeleteObjectInstanceResult const& result) {
  auto const encoded = static_cast<std::uint8_t>(result.status);
  if (encoded > static_cast<std::uint8_t>(
                   LocalObjectInstanceDeletionStatus::federate_owns_attributes)) {
    throw ProcessFederationServiceProtocolError(
        "A process local object deletion result has an invalid status.");
  }
  PayloadWriter writer;
  writer.unsigned8(encoded);
  return std::move(writer).finish();
}

ProcessFederationLocalDeleteObjectInstanceResult
decodeProcessFederationLocalDeleteObjectInstanceResult(
    std::span<std::uint8_t const> encoded) {
  PayloadReader reader(encoded);
  auto const status = reader.unsigned8();
  reader.finish();
  if (status > static_cast<std::uint8_t>(
                   LocalObjectInstanceDeletionStatus::federate_owns_attributes)) {
    throw ProcessFederationServiceProtocolError(
        "A process local object deletion result has an invalid status.");
  }
  return {static_cast<LocalObjectInstanceDeletionStatus>(status)};
}

std::vector<std::uint8_t> encodeProcessFederationDeleteObjectInstanceResult(
    ProcessFederationDeleteObjectInstanceResult const& result) {
  auto const encoded = static_cast<std::uint8_t>(result.status);
  if (encoded > static_cast<std::uint8_t>(
                   ObjectInstanceDeletionStatus::inconsistent_catalog)) {
    throw ProcessFederationServiceProtocolError(
        "A process object deletion result has an invalid status.");
  }
  if (result.status != ObjectInstanceDeletionStatus::applied &&
      result.recipientCount != 0U) {
    throw ProcessFederationServiceProtocolError(
        "A failed process object deletion result cannot carry recipients.");
  }
  PayloadWriter writer;
  writer.unsigned8(encoded);
  writer.unsigned32(result.recipientCount);
  writeOptionalMessageId(writer, result.messageId);
  return std::move(writer).finish();
}

ProcessFederationDeleteObjectInstanceResult
decodeProcessFederationDeleteObjectInstanceResult(
    std::span<std::uint8_t const> encoded) {
  PayloadReader reader(encoded);
  auto const status = reader.unsigned8();
  auto const recipientCount = reader.unsigned32();
  auto const messageId = readOptionalMessageId(reader);
  reader.finish();
  if (status > static_cast<std::uint8_t>(
                   ObjectInstanceDeletionStatus::inconsistent_catalog)) {
    throw ProcessFederationServiceProtocolError(
        "A process object deletion result has an invalid status.");
  }
  if (status != static_cast<std::uint8_t>(ObjectInstanceDeletionStatus::applied) &&
      recipientCount != 0U) {
    throw ProcessFederationServiceProtocolError(
        "A failed process object deletion result cannot carry recipients.");
  }
  return {
      static_cast<ObjectInstanceDeletionStatus>(status), recipientCount, messageId};
}

std::vector<std::uint8_t>
encodeProcessFederationObjectInstanceRegionAssociationResult(
    ProcessFederationObjectInstanceRegionAssociationResult const& result) {
  PayloadWriter writer;
  writeObjectInstanceRegionAssociationStatus(writer, result.status);
  return std::move(writer).finish();
}

ProcessFederationObjectInstanceRegionAssociationResult
decodeProcessFederationObjectInstanceRegionAssociationResult(
    std::span<std::uint8_t const> encoded) {
  PayloadReader reader(encoded);
  ProcessFederationObjectInstanceRegionAssociationResult result;
  result.status = readObjectInstanceRegionAssociationStatus(reader);
  reader.finish();
  return result;
}
}  // namespace umbra::detail
