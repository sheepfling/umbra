#include "internal/federation/process_federation_service_protocol.hpp"
#include "internal/federation/process_federation_service_codec_support.hpp"

#include <algorithm>
#include <utility>

namespace umbra::detail {
using namespace process_federation_service_codec_support;

std::vector<std::uint8_t> encodeProcessFederationCreateRequest(
    ProcessFederationCreateRequest const& request) {
  if (request.federationName.empty()) {
    throw ProcessFederationServiceProtocolError(
        "A process federation create request requires a federation name.");
  }
  PayloadWriter writer;
  writer.wideString(request.federationName);
  // Keep the historical one-field payload byte-for-byte compatible for
  // private callers that still rely on the server-owned definition.  New
  // public process clients opt into the standards-facing suffix whenever a
  // FOM/MIM/time input is supplied.
  if (!request.fomModules.empty() || request.mimModule.has_value() ||
      !request.logicalTimeImplementationName.empty() ||
      request.hasFomInputs) {
    if (request.logicalTimeImplementationName.empty()) {
      throw ProcessFederationServiceProtocolError(
          "A process federation create request requires a logical-time implementation.");
    }
    for (auto const& module : request.fomModules) {
      if (module.empty()) {
        throw ProcessFederationServiceProtocolError(
            "A process federation FOM module designator cannot be empty.");
      }
    }
    if (request.mimModule.has_value() && request.mimModule->empty()) {
      throw ProcessFederationServiceProtocolError(
          "A process federation MIM module designator cannot be empty.");
    }
    writer.wideStringVector(request.fomModules);
    writer.unsigned8(request.mimModule.has_value() ? 1U : 0U);
    if (request.mimModule.has_value()) {
      writer.wideString(*request.mimModule);
    }
    writer.wideString(request.logicalTimeImplementationName);
  }
  return std::move(writer).finish();
}

ProcessFederationCreateRequest decodeProcessFederationCreateRequest(
    std::span<std::uint8_t const> encoded) {
  PayloadReader reader(encoded);
  ProcessFederationCreateRequest result;
  result.federationName = reader.wideString();
  if (reader.remaining() != 0U) {
    result.hasFomInputs = true;
    result.fomModules = reader.wideStringVector();
    auto const hasMim = reader.unsigned8();
    if (hasMim > 1U) {
      throw ProcessFederationServiceProtocolError(
          "A process federation create request has an invalid MIM marker.");
    }
    if (hasMim != 0U) {
      result.mimModule = reader.wideString();
    }
    result.logicalTimeImplementationName = reader.wideString();
  }
  reader.finish();
  if (result.federationName.empty()) {
    throw ProcessFederationServiceProtocolError(
        "A process federation create request requires a federation name.");
  }
  if (result.hasFomInputs && result.logicalTimeImplementationName.empty()) {
    throw ProcessFederationServiceProtocolError(
        "A process federation create request requires a logical-time implementation.");
  }
  if (std::any_of(
          result.fomModules.begin(),
          result.fomModules.end(),
          [](std::wstring const& module) { return module.empty(); }) ||
      (result.mimModule.has_value() && result.mimModule->empty())) {
    throw ProcessFederationServiceProtocolError(
        "A process federation create request requires non-empty module designators.");
  }
  return result;
}

std::vector<std::uint8_t> encodeProcessFederationJoinRequest(
    ProcessFederationJoinRequest const& request) {
  if (request.federationName.empty() || request.federateType.empty()) {
    throw ProcessFederationServiceProtocolError(
        "A process federation join request requires execution and type names.");
  }
  PayloadWriter writer;
  writer.wideString(request.federationName);
  writer.wideString(request.federateType);
  writer.unsigned8(request.requestedFederateName.has_value() ? 1U : 0U);
  if (request.requestedFederateName.has_value()) {
    if (request.requestedFederateName->empty()) {
      throw ProcessFederationServiceProtocolError(
          "A requested process federate name cannot be empty.");
    }
    writer.wideString(*request.requestedFederateName);
  }
  for (auto const& module : request.additionalFomModules) {
    if (module.empty()) {
      throw ProcessFederationServiceProtocolError(
          "An additional process FOM module designator cannot be empty.");
    }
  }
  writer.wideStringVector(request.additionalFomModules);
  writer.unsigned8(1U);
  writer.wideString(request.connection.callbackModel);
  writer.wideString(request.connection.configurationName);
  writer.wideString(request.connection.rtiAddress);
  writer.wideString(request.connection.additionalSettings);
  writer.unsigned8(request.connection.credentials.has_value() ? 1U : 0U);
  if (request.connection.credentials) {
    if (request.connection.credentials->first.empty()) {
      throw ProcessFederationServiceProtocolError(
          "A process federation Join credential type cannot be empty.");
    }
    writer.wideString(request.connection.credentials->first);
    writer.bytes(request.connection.credentials->second);
  }
  return std::move(writer).finish();
}

ProcessFederationJoinRequest decodeProcessFederationJoinRequest(
    std::span<std::uint8_t const> encoded) {
  PayloadReader reader(encoded);
  ProcessFederationJoinRequest result;
  result.federationName = reader.wideString();
  result.federateType = reader.wideString();
  auto const hasRequestedName = reader.unsigned8();
  if (hasRequestedName > 1U) {
    throw ProcessFederationServiceProtocolError(
        "A process federation join request has an invalid name marker.");
  }
  if (hasRequestedName != 0U) {
    result.requestedFederateName = reader.wideString();
  }
  // Keep the original three-field request decodable for an already-running
  // private client. New clients append the module vector and then a versioned
  // Connect snapshot; absent suffixes retain the legacy defaults.
  if (reader.remaining() != 0U) {
    result.additionalFomModules = reader.wideStringVector();
  }
  if (reader.remaining() != 0U) {
    auto const snapshotVersion = reader.unsigned8();
    if (snapshotVersion != 1U) {
      throw ProcessFederationServiceProtocolError(
          "A process federation Join request has an unsupported connection snapshot version.");
    }
    result.connection.callbackModel = reader.wideString();
    result.connection.configurationName = reader.wideString();
    result.connection.rtiAddress = reader.wideString();
    result.connection.additionalSettings = reader.wideString();
    auto const hasCredentials = reader.unsigned8();
    if (hasCredentials > 1U) {
      throw ProcessFederationServiceProtocolError(
          "A process federation Join request has an invalid credential marker.");
    }
    if (hasCredentials != 0U) {
      auto credentialType = reader.wideString();
      if (credentialType.empty()) {
        throw ProcessFederationServiceProtocolError(
            "A process federation Join credential type cannot be empty.");
      }
      result.connection.credentials = std::make_pair(
          std::move(credentialType), reader.bytes());
    }
  }
  reader.finish();
  if (result.federationName.empty() || result.federateType.empty() ||
      (result.requestedFederateName.has_value() &&
       result.requestedFederateName->empty()) ||
      result.connection.callbackModel.empty() ||
      std::any_of(
          result.additionalFomModules.begin(),
          result.additionalFomModules.end(),
          [](std::wstring const& module) { return module.empty(); })) {
    throw ProcessFederationServiceProtocolError(
        "A process federation join request requires non-empty names.");
  }
  return result;
}

std::vector<std::uint8_t> encodeProcessFederationResignRequest(
    ProcessFederationResignRequest const& request) {
  if (request.federationName.empty()) {
    throw ProcessFederationServiceProtocolError(
        "A process federation resign request requires a federation name.");
  }
  requireNonzero(
      request.federateId,
      "A process federation resign request requires a federate identity.");
  if (!validResignAction(request.resignAction)) {
    throw ProcessFederationServiceProtocolError(
        "A process federation resign request has an invalid resign action.");
  }
  PayloadWriter writer;
  writer.wideString(request.federationName);
  writer.unsigned64(request.federateId);
  writer.unsigned8(static_cast<std::uint8_t>(request.resignAction));
  return std::move(writer).finish();
}

ProcessFederationResignRequest decodeProcessFederationResignRequest(
    std::span<std::uint8_t const> encoded) {
  PayloadReader reader(encoded);
  ProcessFederationResignRequest result;
  result.federationName = reader.wideString();
  result.federateId = reader.unsigned64();
  auto const action = reader.unsigned8();
  reader.finish();
  if (action > static_cast<std::uint8_t>(rti1516_2025::NO_ACTION)) {
    throw ProcessFederationServiceProtocolError(
        "A process federation resign request has an invalid resign action.");
  }
  result.resignAction = static_cast<rti1516_2025::ResignAction>(action);
  if (result.federationName.empty()) {
    throw ProcessFederationServiceProtocolError(
        "A process federation resign request requires a federation name.");
  }
  requireNonzero(
      result.federateId,
      "A process federation resign request requires a federate identity.");
  if (!validResignAction(result.resignAction)) {
    throw ProcessFederationServiceProtocolError(
        "A process federation resign request has an invalid resign action.");
  }
  return result;
}

std::vector<std::uint8_t> encodeProcessFederationDestroyRequest(
    ProcessFederationDestroyRequest const& request) {
  if (request.federationName.empty()) {
    throw ProcessFederationServiceProtocolError(
        "A process federation destroy request requires a federation name.");
  }
  PayloadWriter writer;
  writer.wideString(request.federationName);
  return std::move(writer).finish();
}

ProcessFederationDestroyRequest decodeProcessFederationDestroyRequest(
    std::span<std::uint8_t const> encoded) {
  PayloadReader reader(encoded);
  ProcessFederationDestroyRequest result;
  result.federationName = reader.wideString();
  reader.finish();
  if (result.federationName.empty()) {
    throw ProcessFederationServiceProtocolError(
        "A process federation destroy request requires a federation name.");
  }
  return result;
}

std::vector<std::uint8_t> encodeProcessFederationDestroyResult(
    ProcessFederationDestroyResult const& result) {
  switch (result.status) {
    case ProcessFederationDestroyStatus::applied:
    case ProcessFederationDestroyStatus::federation_does_not_exist:
    case ProcessFederationDestroyStatus::federates_currently_joined:
    case ProcessFederationDestroyStatus::invalid_request:
      break;
    default:
      throw ProcessFederationServiceProtocolError(
          "A process federation destroy result has an invalid status.");
  }
  PayloadWriter writer;
  writer.unsigned8(static_cast<std::uint8_t>(result.status));
  return std::move(writer).finish();
}

ProcessFederationDestroyResult decodeProcessFederationDestroyResult(
    std::span<std::uint8_t const> encoded) {
  PayloadReader reader(encoded);
  auto const status = reader.unsigned8();
  reader.finish();
  if (status > static_cast<std::uint8_t>(
                   ProcessFederationDestroyStatus::invalid_request)) {
    throw ProcessFederationServiceProtocolError(
        "A process federation destroy result has an invalid status.");
  }
  return ProcessFederationDestroyResult{
      static_cast<ProcessFederationDestroyStatus>(status)};
}



std::vector<std::uint8_t>
encodeProcessFederationRegisterSynchronizationPointRequest(
    ProcessFederationRegisterSynchronizationPointRequest const& request) {
  if (request.federationName.empty() || request.label.empty()) {
    throw ProcessFederationServiceProtocolError(
        "A process synchronization-point registration requires execution and label names.");
  }
  requireNonzero(
      request.federateId,
      "A process synchronization-point registration requires a federate identity.");
  validateHandleVector(
      request.synchronizationSet,
      "A process synchronization-point registration requires sorted, unique federate handles.");
  PayloadWriter writer;
  writer.wideString(request.federationName);
  writer.unsigned64(request.federateId);
  writer.wideString(request.label);
  writer.bytes(request.userSuppliedTag);
  writer.unsigned8(request.synchronizationSetWasSupplied ? 1U : 0U);
  writer.unsigned64Vector(request.synchronizationSet);
  return std::move(writer).finish();
}

ProcessFederationRegisterSynchronizationPointRequest
decodeProcessFederationRegisterSynchronizationPointRequest(
    std::span<std::uint8_t const> encoded) {
  PayloadReader reader(encoded);
  ProcessFederationRegisterSynchronizationPointRequest result;
  result.federationName = reader.wideString();
  result.federateId = reader.unsigned64();
  result.label = reader.wideString();
  result.userSuppliedTag = reader.bytes();
  auto const supplied = reader.unsigned8();
  if (supplied > 1U) {
    throw ProcessFederationServiceProtocolError(
        "A process synchronization-point registration has an invalid set marker.");
  }
  result.synchronizationSetWasSupplied = supplied != 0U;
  result.synchronizationSet = reader.unsigned64Vector();
  reader.finish();
  if (result.federationName.empty() || result.label.empty()) {
    throw ProcessFederationServiceProtocolError(
        "A process synchronization-point registration requires execution and label names.");
  }
  requireNonzero(
      result.federateId,
      "A process synchronization-point registration requires a federate identity.");
  validateHandleVector(
      result.synchronizationSet,
      "A process synchronization-point registration requires sorted, unique federate handles.");
  if (!result.synchronizationSetWasSupplied &&
      !result.synchronizationSet.empty()) {
    throw ProcessFederationServiceProtocolError(
        "A default process synchronization-point registration cannot carry a federate set.");
  }
  return result;
}

std::vector<std::uint8_t>
encodeProcessFederationSynchronizationPointAchievedRequest(
    ProcessFederationSynchronizationPointAchievedRequest const& request) {
  if (request.federationName.empty() || request.label.empty()) {
    throw ProcessFederationServiceProtocolError(
        "A process synchronization-point achievement requires execution and label names.");
  }
  requireNonzero(
      request.federateId,
      "A process synchronization-point achievement requires a federate identity.");
  PayloadWriter writer;
  writer.wideString(request.federationName);
  writer.unsigned64(request.federateId);
  writer.wideString(request.label);
  writer.unsigned8(request.successfully ? 1U : 0U);
  return std::move(writer).finish();
}

ProcessFederationSynchronizationPointAchievedRequest
decodeProcessFederationSynchronizationPointAchievedRequest(
    std::span<std::uint8_t const> encoded) {
  PayloadReader reader(encoded);
  ProcessFederationSynchronizationPointAchievedRequest result;
  result.federationName = reader.wideString();
  result.federateId = reader.unsigned64();
  result.label = reader.wideString();
  auto const successfully = reader.unsigned8();
  if (successfully > 1U) {
    throw ProcessFederationServiceProtocolError(
        "A process synchronization-point achievement has an invalid success marker.");
  }
  result.successfully = successfully != 0U;
  reader.finish();
  if (result.federationName.empty() || result.label.empty()) {
    throw ProcessFederationServiceProtocolError(
        "A process synchronization-point achievement requires execution and label names.");
  }
  requireNonzero(
      result.federateId,
      "A process synchronization-point achievement requires a federate identity.");
  return result;
}

std::vector<std::uint8_t>
encodeProcessFederationRegisterSynchronizationPointResult(
    ProcessFederationRegisterSynchronizationPointResult const& result) {
  auto const status = static_cast<std::uint8_t>(result.status);
  if (status > static_cast<std::uint8_t>(
                   ProcessFederationSynchronizationPointRegistrationStatus::
                       callback_route_missing)) {
    throw ProcessFederationServiceProtocolError(
        "A process synchronization-point registration result has an invalid status.");
  }
  auto const failureReason = static_cast<std::uint8_t>(result.failureReason);
  if (failureReason > static_cast<std::uint8_t>(
                          rti1516_2025::SYNCHRONIZATION_SET_MEMBER_NOT_JOINED)) {
    throw ProcessFederationServiceProtocolError(
        "A process synchronization-point registration result has an invalid failure reason.");
  }
  PayloadWriter writer;
  writer.unsigned8(status);
  writer.unsigned8(result.succeeded ? 1U : 0U);
  writer.unsigned8(failureReason);
  return std::move(writer).finish();
}

ProcessFederationRegisterSynchronizationPointResult
decodeProcessFederationRegisterSynchronizationPointResult(
    std::span<std::uint8_t const> encoded) {
  PayloadReader reader(encoded);
  ProcessFederationRegisterSynchronizationPointResult result;
  auto const status = reader.unsigned8();
  auto const succeeded = reader.unsigned8();
  auto const failureReason = reader.unsigned8();
  reader.finish();
  if (status > static_cast<std::uint8_t>(
                   ProcessFederationSynchronizationPointRegistrationStatus::
                       callback_route_missing) ||
      succeeded > 1U ||
      failureReason > static_cast<std::uint8_t>(
                          rti1516_2025::SYNCHRONIZATION_SET_MEMBER_NOT_JOINED)) {
    throw ProcessFederationServiceProtocolError(
        "A process synchronization-point registration result is invalid.");
  }
  result.status = static_cast<
      ProcessFederationSynchronizationPointRegistrationStatus>(status);
  result.succeeded = succeeded != 0U;
  result.failureReason = static_cast<rti1516_2025::SynchronizationPointFailureReason>(
      failureReason);
  return result;
}

std::vector<std::uint8_t>
encodeProcessFederationSynchronizationPointAchievedResult(
    ProcessFederationSynchronizationPointAchievedResult const& result) {
  auto const status = static_cast<std::uint8_t>(result.status);
  if (status > static_cast<std::uint8_t>(
                   ProcessFederationSynchronizationPointAchievedStatus::
                       synchronization_point_label_not_announced)) {
    throw ProcessFederationServiceProtocolError(
        "A process synchronization-point achievement result has an invalid status.");
  }
  PayloadWriter writer;
  writer.unsigned8(status);
  return std::move(writer).finish();
}

ProcessFederationSynchronizationPointAchievedResult
decodeProcessFederationSynchronizationPointAchievedResult(
    std::span<std::uint8_t const> encoded) {
  PayloadReader reader(encoded);
  auto const status = reader.unsigned8();
  reader.finish();
  if (status > static_cast<std::uint8_t>(
                   ProcessFederationSynchronizationPointAchievedStatus::
                       synchronization_point_label_not_announced)) {
    throw ProcessFederationServiceProtocolError(
        "A process synchronization-point achievement result has an invalid status.");
  }
  return ProcessFederationSynchronizationPointAchievedResult{
      static_cast<ProcessFederationSynchronizationPointAchievedStatus>(status)};
}



std::vector<std::uint8_t> encodeProcessFederationSaveRequest(
    ProcessFederationSaveRequest const& request) {
  if (request.federationName.empty()) {
    throw ProcessFederationServiceProtocolError(
        "A process federation-save request requires a federation name.");
  }
  requireNonzero(
      request.federateId,
      "A process federation-save request requires a federate identity.");
  PayloadWriter writer;
  writer.wideString(request.federationName);
  writer.unsigned64(request.federateId);
  writer.wideString(request.label);
  writeOptionalLogicalTime(writer, request.timestamp);
  return std::move(writer).finish();
}

ProcessFederationSaveRequest decodeProcessFederationSaveRequest(
    std::span<std::uint8_t const> encoded) {
  PayloadReader reader(encoded);
  ProcessFederationSaveRequest result;
  result.federationName = reader.wideString();
  result.federateId = reader.unsigned64();
  result.label = reader.wideString();
  result.timestamp = readOptionalLogicalTime(reader);
  reader.finish();
  if (result.federationName.empty()) {
    throw ProcessFederationServiceProtocolError(
        "A process federation-save request requires a federation name.");
  }
  requireNonzero(
      result.federateId,
      "A process federation-save request requires a federate identity.");
  return result;
}

std::vector<std::uint8_t> encodeProcessFederationSaveControlResult(
    ProcessFederationSaveControlResult const& result) {
  auto const status = static_cast<std::uint8_t>(result.status);
  if (status > static_cast<std::uint8_t>(
                   FederationSaveControlStatus::inconsistent_temporal_state)) {
    throw ProcessFederationServiceProtocolError(
        "A process federation-save result has an invalid status.");
  }
  PayloadWriter writer;
  writer.unsigned8(status);
  return std::move(writer).finish();
}

ProcessFederationSaveControlResult decodeProcessFederationSaveControlResult(
    std::span<std::uint8_t const> encoded) {
  PayloadReader reader(encoded);
  auto const status = reader.unsigned8();
  reader.finish();
  if (status > static_cast<std::uint8_t>(
                   FederationSaveControlStatus::inconsistent_temporal_state)) {
    throw ProcessFederationServiceProtocolError(
        "A process federation-save result has an invalid status.");
  }
  return ProcessFederationSaveControlResult{
      static_cast<FederationSaveControlStatus>(status)};
}

std::vector<std::uint8_t> encodeProcessFederationRestoreRequest(
    ProcessFederationRestoreRequest const& request) {
  if (request.federationName.empty()) {
    throw ProcessFederationServiceProtocolError(
        "A process federation-restore request requires a federation name.");
  }
  requireNonzero(
      request.federateId,
      "A process federation-restore request requires a federate identity.");
  PayloadWriter writer;
  writer.wideString(request.federationName);
  writer.unsigned64(request.federateId);
  writer.wideString(request.label);
  writer.unsigned8(request.callbacksEnabled ? 1U : 0U);
  return std::move(writer).finish();
}

ProcessFederationRestoreRequest decodeProcessFederationRestoreRequest(
    std::span<std::uint8_t const> encoded) {
  PayloadReader reader(encoded);
  ProcessFederationRestoreRequest result;
  result.federationName = reader.wideString();
  result.federateId = reader.unsigned64();
  result.label = reader.wideString();
  if (reader.remaining() != 0U) {
    result.callbacksEnabled = reader.unsigned8() != 0U;
  }
  reader.finish();
  if (result.federationName.empty()) {
    throw ProcessFederationServiceProtocolError(
        "A process federation-restore request requires a federation name.");
  }
  requireNonzero(
      result.federateId,
      "A process federation-restore request requires a federate identity.");
  return result;
}

std::vector<std::uint8_t> encodeProcessFederationRestoreControlResult(
    ProcessFederationRestoreControlResult const& result) {
  auto const status = static_cast<std::uint8_t>(result.status);
  if (status > static_cast<std::uint8_t>(
                   FederationRestoreControlStatus::callback_route_missing)) {
    throw ProcessFederationServiceProtocolError(
        "A process federation-restore result has an invalid status.");
  }
  PayloadWriter writer;
  writer.unsigned8(status);
  return std::move(writer).finish();
}

ProcessFederationRestoreControlResult decodeProcessFederationRestoreControlResult(
    std::span<std::uint8_t const> encoded) {
  PayloadReader reader(encoded);
  auto const status = reader.unsigned8();
  reader.finish();
  if (status > static_cast<std::uint8_t>(
                   FederationRestoreControlStatus::callback_route_missing)) {
    throw ProcessFederationServiceProtocolError(
        "A process federation-restore result has an invalid status.");
  }
  return ProcessFederationRestoreControlResult{
      static_cast<FederationRestoreControlStatus>(status)};
}

}  // namespace umbra::detail
