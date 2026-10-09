#pragma once

#include "internal/federation/process_federation_service_protocol.hpp"
#include "internal/federation/process_transport_session.hpp"

#include <RTI/Exception.h>
#include <RTI/VariableLengthData.h>
#include <RTI/time/HLAlogicalTimeFactoryFactory.h>

#include <cstdint>
#include <memory>
#include <optional>
#include <utility>
#include <vector>

namespace umbra::detail {

[[nodiscard]] inline TransportServiceMessage responseFor(
    TransportServiceMessage const& request,
    TransportServiceStatus status,
    std::vector<std::uint8_t> payload = {}) {
  return TransportServiceMessage{
      TransportServiceMessageKind::response,
      request.operation,
      status,
      request.requestId,
      std::move(payload)};
}

inline void validateProcessLogicalTime(
    ProcessFederationLogicalTime const& timestamp,
    std::wstring const& expectedImplementationName) {
  if (expectedImplementationName.empty() ||
      timestamp.implementationName != expectedImplementationName) {
    throw ProcessFederationServiceProtocolError(
        "A process federation timestamp uses a different logical-time implementation.");
  }

  auto factory = rti1516_2025::HLAlogicalTimeFactoryFactory::makeLogicalTimeFactory(
      expectedImplementationName);
  if (!factory || factory->getName() != expectedImplementationName) {
    throw ProcessFederationServiceProtocolError(
        "The process federation could not create its logical-time factory.");
  }

  rti1516_2025::VariableLengthData encoded;
  if (!timestamp.encoding.empty()) {
    encoded.setData(timestamp.encoding.data(), timestamp.encoding.size());
  }
  std::unique_ptr<rti1516_2025::LogicalTime> decoded;
  try {
    decoded = factory->decodeLogicalTime(encoded);
  } catch (rti1516_2025::Exception const&) {
    throw ProcessFederationServiceProtocolError(
        "A process federation timestamp could not be decoded by its logical-time factory.");
  }
  if (!decoded || decoded->implementationName() != expectedImplementationName ||
      decoded->isInitial() || decoded->isFinal()) {
    throw ProcessFederationServiceProtocolError(
        "A process federation timestamp is not a finite logical time.");
  }
}

[[nodiscard]] inline std::shared_ptr<rti1516_2025::LogicalTime const>
decodeProcessLogicalTime(
    ProcessFederationLogicalTime const& timestamp,
    std::wstring const& expectedImplementationName) {
  validateProcessLogicalTime(timestamp, expectedImplementationName);
  auto factory = rti1516_2025::HLAlogicalTimeFactoryFactory::makeLogicalTimeFactory(
      expectedImplementationName);
  rti1516_2025::VariableLengthData encoded;
  if (!timestamp.encoding.empty()) {
    encoded.setData(timestamp.encoding.data(), timestamp.encoding.size());
  }
  auto decoded = factory->decodeLogicalTime(encoded);
  if (!decoded) {
    throw ProcessFederationServiceProtocolError(
        "A process federation timestamp could not be reconstructed.");
  }
  std::shared_ptr<rti1516_2025::LogicalTime const> result = std::move(decoded);
  return result;
}

[[nodiscard]] inline std::optional<ProcessFederationLogicalTime>
encodeProcessLogicalTime(
    std::shared_ptr<rti1516_2025::LogicalTime const> const& value) {
  if (!value || value->implementationName().empty()) {
    return std::nullopt;
  }
  auto const encoded = value->encode();
  std::vector<std::uint8_t> bytes;
  if (encoded.size() != 0U) {
    auto const* data = static_cast<std::uint8_t const*>(encoded.data());
    if (data == nullptr) {
      return std::nullopt;
    }
    bytes.assign(data, data + encoded.size());
  }
  return ProcessFederationLogicalTime{value->implementationName(), std::move(bytes)};
}

}  // namespace umbra::detail
