#include "internal/federation/process_federation_service_protocol.hpp"
#include "internal/federation/process_federation_service_codec_support.hpp"
#include "internal/observability/mom_service_report_encoding.hpp"

#include <cstddef>
#include <cstdint>
#include <limits>
#include <span>
#include <utility>
#include <vector>

namespace umbra::detail {
using namespace process_federation_service_codec_support;
std::vector<std::uint8_t> encodeProcessFederationExceptionReportingSwitchResult(
    ProcessFederationExceptionReportingSwitchResult const& result) {
  auto const status = static_cast<std::uint8_t>(result.status);
  if (status > static_cast<std::uint8_t>(
                   FederationServiceOperationStatus::restore_in_progress)) {
    throw ProcessFederationServiceProtocolError(
        "An exception-reporting switch result has an invalid operation status.");
  }
  PayloadWriter writer;
  writer.unsigned8(status);
  writer.unsigned8(result.value ? 1U : 0U);
  return std::move(writer).finish();
}

ProcessFederationExceptionReportingSwitchResult
decodeProcessFederationExceptionReportingSwitchResult(
    std::span<std::uint8_t const> encoded) {
  PayloadReader reader(encoded);
  auto const status = reader.unsigned8();
  auto const value = reader.unsigned8();
  if (status > static_cast<std::uint8_t>(
                   FederationServiceOperationStatus::restore_in_progress) ||
      value > 1U) {
    throw ProcessFederationServiceProtocolError(
        "An exception-reporting switch result has an invalid status or boolean.");
  }
  reader.finish();
  return {static_cast<FederationServiceOperationStatus>(status), value != 0U};
}

std::vector<std::uint8_t> encodeProcessFederationServiceExceptionRequest(
    ProcessFederationServiceExceptionRequest const& request) {
  if (request.federationName.empty() || request.federateId == 0U ||
      request.service.empty() || request.exception.empty()) {
    throw ProcessFederationServiceProtocolError(
        "A service exception report requires member identity, service, and exception.");
  }
  PayloadWriter writer;
  writer.wideString(request.federationName);
  writer.unsigned64(request.federateId);
  writer.wideString(request.service);
  writer.wideString(request.exception);
  return std::move(writer).finish();
}

ProcessFederationServiceExceptionRequest
decodeProcessFederationServiceExceptionRequest(
    std::span<std::uint8_t const> encoded) {
  PayloadReader reader(encoded);
  ProcessFederationServiceExceptionRequest result;
  result.federationName = reader.wideString();
  result.federateId = reader.unsigned64();
  result.service = reader.wideString();
  result.exception = reader.wideString();
  reader.finish();
  if (result.federationName.empty() || result.federateId == 0U ||
      result.service.empty() || result.exception.empty()) {
    throw ProcessFederationServiceProtocolError(
        "A service exception report requires member identity, service, and exception.");
  }
  return result;
}

std::vector<std::uint8_t> encodeProcessFederationFailedServiceInvocationRequest(
    ProcessFederationFailedServiceInvocationRequest const& request) {
  if (request.federationName.empty() || request.federateId == 0U ||
      request.service.empty() || request.exception.empty() ||
      request.serviceType > MomServiceType::support_services ||
      request.suppliedArguments.size() > std::numeric_limits<std::uint32_t>::max()) {
    throw ProcessFederationServiceProtocolError(
        "A failed service invocation report requires valid member identity, service, type, and exception.");
  }
  PayloadWriter writer;
  writer.wideString(request.federationName);
  writer.unsigned64(request.federateId);
  writer.unsigned32(static_cast<std::uint32_t>(request.serviceType));
  writer.wideString(request.service);
  writer.unsigned32(static_cast<std::uint32_t>(request.suppliedArguments.size()));
  for (auto const& argument : request.suppliedArguments) {
    auto const argumentType = static_cast<std::int32_t>(argument.type);
    if (argumentType < 0) {
      throw ProcessFederationServiceProtocolError(
          "A failed service invocation argument has an invalid MIM type.");
    }
    writer.unsigned32(static_cast<std::uint32_t>(argumentType));
    writer.wideString(argument.name);
    writer.wideString(argument.value);
  }
  writer.wideString(request.exception);
  return std::move(writer).finish();
}

ProcessFederationFailedServiceInvocationRequest
decodeProcessFederationFailedServiceInvocationRequest(
    std::span<std::uint8_t const> encoded) {
  PayloadReader reader(encoded);
  ProcessFederationFailedServiceInvocationRequest result;
  result.federationName = reader.wideString();
  result.federateId = reader.unsigned64();
  auto const serviceType = reader.unsigned32();
  if (serviceType > static_cast<std::uint32_t>(MomServiceType::support_services)) {
    throw ProcessFederationServiceProtocolError(
        "A failed service invocation report has an invalid MIM service type.");
  }
  result.serviceType = static_cast<MomServiceType>(serviceType);
  result.service = reader.wideString();
  auto const argumentCount = reader.count(20U);
  result.suppliedArguments.reserve(argumentCount);
  for (std::size_t index = 0U; index < argumentCount; ++index) {
    auto const argumentType = reader.unsigned32();
    if (argumentType > static_cast<std::uint32_t>(
                           std::numeric_limits<std::int32_t>::max())) {
      throw ProcessFederationServiceProtocolError(
          "A failed service invocation argument has an invalid MIM type.");
    }
    MomServiceArgument argument;
    argument.type = static_cast<MomArgumentType>(
        static_cast<std::int32_t>(argumentType));
    argument.name = reader.wideString();
    argument.value = reader.wideString();
    result.suppliedArguments.push_back(std::move(argument));
  }
  result.exception = reader.wideString();
  reader.finish();
  if (result.federationName.empty() || result.federateId == 0U ||
      result.service.empty() || result.exception.empty()) {
    throw ProcessFederationServiceProtocolError(
        "A failed service invocation report requires member identity, service, and exception.");
  }
  return result;
}

std::vector<std::uint8_t>
encodeProcessFederationSuccessfulServiceInvocationRequest(
    ProcessFederationSuccessfulServiceInvocationRequest const& request) {
  auto const returnedArgumentType =
      static_cast<std::int32_t>(request.returnedArgument.type);
  if (request.federationName.empty() || request.federateId == 0U ||
      request.service.empty() ||
      request.serviceType > MomServiceType::support_services ||
      request.suppliedArguments.size() > std::numeric_limits<std::uint32_t>::max() ||
      returnedArgumentType < 0) {
    throw ProcessFederationServiceProtocolError(
        "A successful service invocation report requires valid member identity, service, type, and return value.");
  }
  PayloadWriter writer;
  writer.wideString(request.federationName);
  writer.unsigned64(request.federateId);
  writer.unsigned32(static_cast<std::uint32_t>(request.serviceType));
  writer.wideString(request.service);
  writer.unsigned32(static_cast<std::uint32_t>(request.suppliedArguments.size()));
  for (auto const& argument : request.suppliedArguments) {
    auto const argumentType = static_cast<std::int32_t>(argument.type);
    if (argumentType < 0) {
      throw ProcessFederationServiceProtocolError(
          "A successful service invocation argument has an invalid MIM type.");
    }
    writer.unsigned32(static_cast<std::uint32_t>(argumentType));
    writer.wideString(argument.name);
    writer.wideString(argument.value);
  }
  writer.unsigned32(static_cast<std::uint32_t>(returnedArgumentType));
  writer.wideString(request.returnedArgument.name);
  writer.wideString(request.returnedArgument.value);
  return std::move(writer).finish();
}

ProcessFederationSuccessfulServiceInvocationRequest
decodeProcessFederationSuccessfulServiceInvocationRequest(
    std::span<std::uint8_t const> encoded) {
  PayloadReader reader(encoded);
  ProcessFederationSuccessfulServiceInvocationRequest result;
  result.federationName = reader.wideString();
  result.federateId = reader.unsigned64();
  auto const serviceType = reader.unsigned32();
  if (serviceType > static_cast<std::uint32_t>(MomServiceType::support_services)) {
    throw ProcessFederationServiceProtocolError(
        "A successful service invocation report has an invalid MIM service type.");
  }
  result.serviceType = static_cast<MomServiceType>(serviceType);
  result.service = reader.wideString();
  auto const argumentCount = reader.count(20U);
  result.suppliedArguments.reserve(argumentCount);
  for (std::size_t index = 0U; index < argumentCount; ++index) {
    auto const argumentType = reader.unsigned32();
    if (argumentType > static_cast<std::uint32_t>(
                           std::numeric_limits<std::int32_t>::max())) {
      throw ProcessFederationServiceProtocolError(
          "A successful service invocation argument has an invalid MIM type.");
    }
    MomServiceArgument argument;
    argument.type = static_cast<MomArgumentType>(
        static_cast<std::int32_t>(argumentType));
    argument.name = reader.wideString();
    argument.value = reader.wideString();
    result.suppliedArguments.push_back(std::move(argument));
  }
  auto const returnedArgumentType = reader.unsigned32();
  if (returnedArgumentType > static_cast<std::uint32_t>(
                                 std::numeric_limits<std::int32_t>::max())) {
    throw ProcessFederationServiceProtocolError(
        "A successful service invocation return value has an invalid MIM type.");
  }
  result.returnedArgument.type = static_cast<MomArgumentType>(
      static_cast<std::int32_t>(returnedArgumentType));
  result.returnedArgument.name = reader.wideString();
  result.returnedArgument.value = reader.wideString();
  reader.finish();
  if (result.federationName.empty() || result.federateId == 0U ||
      result.service.empty()) {
    throw ProcessFederationServiceProtocolError(
        "A successful service invocation report requires member identity and service.");
  }
  return result;
}

std::vector<std::uint8_t> encodeProcessFederationExceptionReportRecheckRequest(
    ProcessFederationExceptionReportRecheckRequest const& request) {
  if (request.federationName.empty() || request.receivingFederateId == 0U ||
      request.reportedFederateId == 0U) {
    throw ProcessFederationServiceProtocolError(
        "An exception report recheck requires federation, recipient, and source identities.");
  }
  PayloadWriter writer;
  writer.wideString(request.federationName);
  writer.unsigned64(request.receivingFederateId);
  writer.unsigned64(request.reportedFederateId);
  return std::move(writer).finish();
}

ProcessFederationExceptionReportRecheckRequest
decodeProcessFederationExceptionReportRecheckRequest(
    std::span<std::uint8_t const> encoded) {
  PayloadReader reader(encoded);
  ProcessFederationExceptionReportRecheckRequest result;
  result.federationName = reader.wideString();
  result.receivingFederateId = reader.unsigned64();
  result.reportedFederateId = reader.unsigned64();
  reader.finish();
  if (result.federationName.empty() || result.receivingFederateId == 0U ||
      result.reportedFederateId == 0U) {
    throw ProcessFederationServiceProtocolError(
        "An exception report recheck requires federation, recipient, and source identities.");
  }
  return result;
}

std::vector<std::uint8_t> encodeProcessFederationBooleanResult(
    ProcessFederationBooleanResult const& result) {
  PayloadWriter writer;
  writer.unsigned8(result.value ? 1U : 0U);
  return std::move(writer).finish();
}

ProcessFederationBooleanResult decodeProcessFederationBooleanResult(
    std::span<std::uint8_t const> encoded) {
  PayloadReader reader(encoded);
  auto const value = reader.unsigned8();
  if (value > 1U) {
    throw ProcessFederationServiceProtocolError(
        "A process boolean result has an invalid value.");
  }
  reader.finish();
  return ProcessFederationBooleanResult{value != 0U};
}


}  // namespace umbra::detail
