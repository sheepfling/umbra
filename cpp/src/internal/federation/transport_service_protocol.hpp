#pragma once

#include <cstddef>
#include <cstdint>
#include <span>
#include <stdexcept>
#include <vector>

namespace umbra::detail {

// Private service dispatch metadata carried inside a transport data frame.
// The payload remains an opaque service-specific byte sequence until it is
// bound to the federation registry; no Umbra-specific type crosses the public
// IEEE 1516.1-2025 binding.
enum class TransportServiceMessageKind : std::uint8_t {
  request = 1U,
  response = 2U,
  event = 3U,
};

enum class TransportServiceOperation : std::uint16_t {
  create_federation_execution = 1U,
  join_federation_execution = 2U,
  send_interaction = 3U,
  receive_interaction = 4U,
  resign_federation_execution = 5U,
  get_interaction_class_handle = 6U,
  get_parameter_handle = 7U,
  publish_interaction_class = 8U,
  unpublish_interaction_class = 9U,
  subscribe_interaction_class = 10U,
  unsubscribe_interaction_class = 11U,
  get_object_class_handle = 12U,
  publish_object_class_attributes = 13U,
  register_object_instance = 14U,
  get_attribute_handle = 15U,
  update_attribute_values = 16U,
  receive_attribute_update = 17U,
  subscribe_object_class_attributes = 18U,
  unsubscribe_object_class_attributes = 19U,
  // Unsolicited/queued object-instance discovery is kept separate from the
  // interaction and attribute receive operations so the public process
  // adapter can project the official 6.9 callback without overloading an
  // unrelated payload marker.
  receive_object_instance_discovery = 20U,
  // A named registration must be preceded by the official reservation
  // service.  Keep the reservation request distinct from registration so
  // the process service can enforce the registry's ownership ledger.
  reserve_object_instance_name = 21U,
  // DDM region lifecycle and regional registration are kept as distinct
  // private operations.  The public binding still owns the official handle
  // and exception types; these values only identify the process seam.
  get_dimension_handle = 22U,
  get_dimension_upper_bound = 23U,
  create_region = 24U,
  commit_region_modifications = 25U,
  delete_region = 26U,
  get_dimension_handle_set = 27U,
  get_range_bounds = 28U,
  set_range_bounds = 29U,
  register_object_instance_with_regions = 30U,
  subscribe_object_class_attributes_with_regions = 31U,
  unsubscribe_object_class_attributes_with_regions = 32U,
  get_attribute_scope_advisory_switch = 33U,
  set_attribute_scope_advisory_switch = 34U,
  associate_regions_for_updates = 35U,
  unassociate_regions_for_updates = 36U,
  get_attribute_relevance_advisory_switch = 37U,
  set_attribute_relevance_advisory_switch = 38U,
  publish_object_class_directed_interactions = 39U,
  unpublish_object_class_directed_interactions = 40U,
  subscribe_object_class_directed_interactions = 41U,
  unsubscribe_object_class_directed_interactions = 42U,
  send_directed_interaction = 43U,
  retract = 44U,
  // A retraction notification is an unsolicited process event.  The public
  // adapter uses it to discard a timestamped directed interaction that has
  // not crossed the receiving callback boundary yet.
  request_retraction = 45U,
  // Local Delete Object Instance changes only the invoking federate's known
  // object ledger and therefore uses a request/response without callback fan-out.
  local_delete_object_instance = 46U,
  // Receive-order Delete Object Instance returns a status to the producer and
  // projects one removal event to every other known recipient.
  delete_object_instance = 47U,
};

enum class TransportServiceStatus : std::uint16_t {
  ok = 0U,
  rejected = 1U,
  invalid_request = 2U,
  internal_error = 3U,
};

struct TransportServiceMessage final {
  TransportServiceMessageKind kind = TransportServiceMessageKind::request;
  TransportServiceOperation operation =
      TransportServiceOperation::create_federation_execution;
  TransportServiceStatus status = TransportServiceStatus::ok;
  std::uint64_t requestId = 0U;
  std::vector<std::uint8_t> payload;
};

class TransportServiceProtocolError final : public std::runtime_error {
 public:
  using std::runtime_error::runtime_error;
};

inline constexpr std::size_t kTransportServiceHeaderSize = 24U;

[[nodiscard]] bool isTransportServiceMessageKind(
    TransportServiceMessageKind kind) noexcept;
[[nodiscard]] bool isTransportServiceOperation(
    TransportServiceOperation operation) noexcept;
[[nodiscard]] bool isTransportServiceStatus(
    TransportServiceStatus status) noexcept;

[[nodiscard]] std::vector<std::uint8_t> encodeTransportServiceMessage(
    TransportServiceMessage const& message);

[[nodiscard]] TransportServiceMessage decodeTransportServiceMessage(
    std::span<std::uint8_t const> encoded);

}  // namespace umbra::detail
