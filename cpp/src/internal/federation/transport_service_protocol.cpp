#include "internal/federation/transport_service_protocol.hpp"

#include "internal/federation/transport_protocol.hpp"

#include <algorithm>
#include <array>
#include <limits>

namespace umbra::detail {
namespace {

constexpr std::array<std::uint8_t, 4U> kTransportServiceMagic{'U', 'M', 'S', 'V'};
constexpr std::uint16_t kTransportServiceProtocolVersion = 1U;

[[noreturn]] void fail(char const* message) {
  throw TransportServiceProtocolError(message);
}

void appendUnsigned16(std::vector<std::uint8_t>& bytes, std::uint16_t value) {
  bytes.push_back(static_cast<std::uint8_t>((value >> 8U) & 0xffU));
  bytes.push_back(static_cast<std::uint8_t>(value & 0xffU));
}

void appendUnsigned64(std::vector<std::uint8_t>& bytes, std::uint64_t value) {
  for (std::size_t shift = 56U; shift != 0U; shift -= 8U) {
    bytes.push_back(static_cast<std::uint8_t>((value >> shift) & 0xffU));
  }
  bytes.push_back(static_cast<std::uint8_t>(value & 0xffU));
}

void appendUnsigned32(std::vector<std::uint8_t>& bytes, std::uint32_t value) {
  bytes.push_back(static_cast<std::uint8_t>((value >> 24U) & 0xffU));
  bytes.push_back(static_cast<std::uint8_t>((value >> 16U) & 0xffU));
  bytes.push_back(static_cast<std::uint8_t>((value >> 8U) & 0xffU));
  bytes.push_back(static_cast<std::uint8_t>(value & 0xffU));
}

[[nodiscard]] std::uint16_t readUnsigned16(
    std::span<std::uint8_t const> bytes,
    std::size_t offset) {
  return static_cast<std::uint16_t>(
      (static_cast<std::uint16_t>(bytes[offset]) << 8U) |
      static_cast<std::uint16_t>(bytes[offset + 1U]));
}

[[nodiscard]] std::uint32_t readUnsigned32(
    std::span<std::uint8_t const> bytes,
    std::size_t offset) {
  return (static_cast<std::uint32_t>(bytes[offset]) << 24U) |
      (static_cast<std::uint32_t>(bytes[offset + 1U]) << 16U) |
      (static_cast<std::uint32_t>(bytes[offset + 2U]) << 8U) |
      static_cast<std::uint32_t>(bytes[offset + 3U]);
}

[[nodiscard]] std::uint64_t readUnsigned64(
    std::span<std::uint8_t const> bytes,
    std::size_t offset) {
  std::uint64_t value = 0U;
  for (std::size_t index = 0U; index < sizeof(std::uint64_t); ++index) {
    value = (value << 8U) | static_cast<std::uint64_t>(bytes[offset + index]);
  }
  return value;
}

}  // namespace

bool isTransportServiceMessageKind(TransportServiceMessageKind kind) noexcept {
  switch (kind) {
    case TransportServiceMessageKind::request:
    case TransportServiceMessageKind::response:
    case TransportServiceMessageKind::event:
      return true;
  }
  return false;
}

bool isTransportServiceOperation(TransportServiceOperation operation) noexcept {
  switch (operation) {
    case TransportServiceOperation::create_federation_execution:
    case TransportServiceOperation::join_federation_execution:
    case TransportServiceOperation::send_interaction:
    case TransportServiceOperation::receive_interaction:
    case TransportServiceOperation::resign_federation_execution:
    case TransportServiceOperation::get_interaction_class_handle:
    case TransportServiceOperation::get_parameter_handle:
    case TransportServiceOperation::publish_interaction_class:
    case TransportServiceOperation::unpublish_interaction_class:
    case TransportServiceOperation::subscribe_interaction_class:
    case TransportServiceOperation::unsubscribe_interaction_class:
    case TransportServiceOperation::get_object_class_handle:
    case TransportServiceOperation::publish_object_class_attributes:
    case TransportServiceOperation::register_object_instance:
    case TransportServiceOperation::get_attribute_handle:
    case TransportServiceOperation::update_attribute_values:
    case TransportServiceOperation::receive_attribute_update:
    case TransportServiceOperation::subscribe_object_class_attributes:
    case TransportServiceOperation::unsubscribe_object_class_attributes:
    case TransportServiceOperation::receive_object_instance_discovery:
    case TransportServiceOperation::reserve_object_instance_name:
    case TransportServiceOperation::get_dimension_handle:
    case TransportServiceOperation::get_dimension_upper_bound:
    case TransportServiceOperation::create_region:
    case TransportServiceOperation::commit_region_modifications:
    case TransportServiceOperation::delete_region:
    case TransportServiceOperation::get_dimension_handle_set:
    case TransportServiceOperation::get_range_bounds:
    case TransportServiceOperation::set_range_bounds:
    case TransportServiceOperation::register_object_instance_with_regions:
    case TransportServiceOperation::subscribe_object_class_attributes_with_regions:
    case TransportServiceOperation::unsubscribe_object_class_attributes_with_regions:
    case TransportServiceOperation::get_attribute_scope_advisory_switch:
    case TransportServiceOperation::set_attribute_scope_advisory_switch:
    case TransportServiceOperation::associate_regions_for_updates:
    case TransportServiceOperation::unassociate_regions_for_updates:
    case TransportServiceOperation::get_attribute_relevance_advisory_switch:
    case TransportServiceOperation::set_attribute_relevance_advisory_switch:
    case TransportServiceOperation::publish_object_class_directed_interactions:
    case TransportServiceOperation::unpublish_object_class_directed_interactions:
    case TransportServiceOperation::subscribe_object_class_directed_interactions:
    case TransportServiceOperation::unsubscribe_object_class_directed_interactions:
    case TransportServiceOperation::send_directed_interaction:
    case TransportServiceOperation::retract:
    case TransportServiceOperation::request_retraction:
    case TransportServiceOperation::local_delete_object_instance:
    case TransportServiceOperation::delete_object_instance:
      return true;
  }
  return false;
}

bool isTransportServiceStatus(TransportServiceStatus status) noexcept {
  switch (status) {
    case TransportServiceStatus::ok:
    case TransportServiceStatus::rejected:
    case TransportServiceStatus::invalid_request:
    case TransportServiceStatus::internal_error:
      return true;
  }
  return false;
}

std::vector<std::uint8_t> encodeTransportServiceMessage(
    TransportServiceMessage const& message) {
  if (!isTransportServiceMessageKind(message.kind)) {
    fail("The transport service message kind is not recognized.");
  }
  if (!isTransportServiceOperation(message.operation)) {
    fail("The transport service operation is not recognized.");
  }
  if (!isTransportServiceStatus(message.status)) {
    fail("The transport service status is not recognized.");
  }
  if ((message.kind == TransportServiceMessageKind::request ||
       message.kind == TransportServiceMessageKind::response) &&
      message.requestId == 0U) {
    fail("Transport service requests and responses require a request identity.");
  }
  if (message.kind == TransportServiceMessageKind::event &&
      message.requestId != 0U) {
    fail("Transport service events cannot carry a request identity.");
  }
  if (message.kind == TransportServiceMessageKind::request &&
      message.status != TransportServiceStatus::ok) {
    fail("Transport service requests must use the OK status.");
  }
  if (message.payload.size() > kTransportMaximumPayloadBytes ||
      message.payload.size() > std::numeric_limits<std::uint32_t>::max()) {
    fail("The transport service payload exceeds the protocol maximum.");
  }

  std::vector<std::uint8_t> encoded;
  encoded.reserve(kTransportServiceHeaderSize + message.payload.size());
  encoded.insert(encoded.end(), kTransportServiceMagic.begin(), kTransportServiceMagic.end());
  appendUnsigned16(encoded, kTransportServiceProtocolVersion);
  encoded.push_back(static_cast<std::uint8_t>(message.kind));
  encoded.push_back(0U);
  appendUnsigned16(encoded, static_cast<std::uint16_t>(message.operation));
  appendUnsigned16(encoded, static_cast<std::uint16_t>(message.status));
  appendUnsigned64(encoded, message.requestId);
  appendUnsigned32(encoded, static_cast<std::uint32_t>(message.payload.size()));
  encoded.insert(encoded.end(), message.payload.begin(), message.payload.end());
  return encoded;
}

TransportServiceMessage decodeTransportServiceMessage(
    std::span<std::uint8_t const> encoded) {
  if (encoded.size() < kTransportServiceHeaderSize) {
    fail("The transport service message is shorter than its header.");
  }
  if (!std::equal(kTransportServiceMagic.begin(), kTransportServiceMagic.end(), encoded.begin())) {
    fail("The transport service message magic is invalid.");
  }
  if (readUnsigned16(encoded, 4U) != kTransportServiceProtocolVersion) {
    fail("The transport service protocol version is unsupported.");
  }
  if (encoded[7U] != 0U) {
    fail("The transport service message reserved byte is not zero.");
  }

  auto const kind = static_cast<TransportServiceMessageKind>(encoded[6U]);
  auto const operation = static_cast<TransportServiceOperation>(readUnsigned16(encoded, 8U));
  auto const status = static_cast<TransportServiceStatus>(readUnsigned16(encoded, 10U));
  auto const requestId = readUnsigned64(encoded, 12U);
  auto const payloadSize = readUnsigned32(encoded, 20U);
  if (!isTransportServiceMessageKind(kind)) {
    fail("The transport service message kind is not recognized.");
  }
  if (!isTransportServiceOperation(operation)) {
    fail("The transport service operation is not recognized.");
  }
  if (!isTransportServiceStatus(status)) {
    fail("The transport service status is not recognized.");
  }
  if (payloadSize > kTransportMaximumPayloadBytes ||
      encoded.size() != kTransportServiceHeaderSize + payloadSize) {
    fail("The transport service message length is invalid.");
  }

  TransportServiceMessage message{
      kind,
      operation,
      status,
      requestId,
      std::vector<std::uint8_t>(
          encoded.begin() + static_cast<std::ptrdiff_t>(kTransportServiceHeaderSize),
          encoded.end())};
  // Reuse the same semantic checks as encoding, including request identity
  // and request-status invariants, after the wire fields have been decoded.
  static_cast<void>(encodeTransportServiceMessage(message));
  return message;
}

}  // namespace umbra::detail
