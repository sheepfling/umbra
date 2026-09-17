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
  // Query Logical Time returns the process endpoint's current logical-time
  // representation for the joined federate.  It is intentionally read-only;
  // time-role and grant control remain separate process slices.
  query_logical_time = 48U,
  // Enable Time Regulation carries one official encoded interval to the
  // process-owned FederateTimeState and returns the callback-time value.
  enable_time_regulation = 49U,
  // Query GALT/LITS returns the process endpoint's federation-wide temporal
  // bounds using the same official logical-time encoding as Query Logical
  // Time.  The result keeps an undefined GALT distinct from an undefined
  // LITS, because LITS may remain defined from queued TSO state.
  query_time_bounds = 50U,
  // Enable Time Constrained completes the endpoint-owned role transition and
  // returns the callback-time logical value.  It remains separate from
  // Enable Time Regulation so the public callback surface cannot be mixed.
  enable_time_constrained = 51U,
  // Time Advance Request is accepted and completed against the endpoint-owned
  // FederateTimeState.  A deferred grant is delivered as an unsolicited event
  // once the federation-owned scheduler admits it.
  time_advance_request = 52U,
  // A federation-owned Time Advance Grant callback crosses the process seam as
  // an unsolicited event.  Its payload uses the same result codec as the
  // request response, with an applied status and a required grant value.
  time_advance_grant = 53U,
  // Query Lookahead returns the joined federate's active logical-time
  // interval without exposing a second public representation across the
  // private process seam.
  query_lookahead = 54U,
  // Modify Lookahead carries one official logical-time interval to the
  // endpoint-owned FederateTimeState and returns the precise service status.
  modify_lookahead = 55U,
  // Time Advance Request Available shares the logical-time payload and grant
  // event contract with ordinary TAR, but keeps its inclusive GALT admission
  // semantics distinct at the federation scheduler.
  time_advance_request_available = 56U,
  // Next Message Request selects the earliest queued TSO timestamp no greater
  // than the supplied boundary.  It shares the logical-time/result codec with
  // TAR while retaining a distinct process operation for the public service.
  next_message_request = 57U,
  // Next Message Request Available combines the queued-message selection with
  // the inclusive GALT admission rules used by the Available forms.
  next_message_request_available = 58U,
  // Object-instance name/handle lookups cross the process seam as distinct
  // operations so the service can apply the recipient's known-instance
  // ledger, rather than exposing a process-local cache through the public API.
  get_object_instance_handle = 59U,
  get_object_instance_name = 60U,
  get_object_class_name = 61U,
  get_interaction_class_name = 62U,
  get_attribute_name = 63U,
  get_parameter_name = 64U,
  get_dimension_name = 65U,
  get_transportation_type_handle = 66U,
  get_transportation_type_name = 67U,
  // Available-dimension queries remain distinct from handle/name lookup so
  // the process endpoint can resolve the federation-owned FOM hierarchy.
  get_available_dimensions_for_object_class = 68U,
  get_available_dimensions_for_interaction_class = 69U,
  // Process DDM interaction declarations and switch state use distinct
  // operations so the public regional interaction surface never falls back
  // to a process-local embedded registry.
  get_convey_region_designator_sets_switch = 70U,
  set_convey_region_designator_sets_switch = 71U,
  subscribe_interaction_class_with_regions = 72U,
  unsubscribe_interaction_class_with_regions = 73U,
  // Regional Send Interaction shares the versioned interaction envelope with
  // ordinary Send Interaction but keeps a separate operation identity so
  // older private peers can reject it deterministically.
  send_interaction_with_regions = 74U,
  // Object-instance Request Attribute Value Update crosses the process seam
  // as a request/response; the provider callback is projected through the
  // existing receive-interaction event fence.
  request_attribute_value_update = 75U,
  // Object-class Request Attribute Value Update expands over the provider's
  // currently registered instances at the process service.  It remains a
  // distinct operation so older private peers reject the class form rather
  // than silently treating the class handle as an object instance.
  request_attribute_value_update_class = 76U,
  // Regional object-class Request Attribute Value Update carries the
  // requester-owned attribute-to-region designators across the same process
  // boundary. Keep it distinct so a peer cannot silently drop DDM context.
  request_attribute_value_update_class_with_regions = 77U,
  // Disable Time Regulation and Disable Time Constrained carry the joined
  // federate identity through the same private seam as their enable
  // counterparts. Keep them distinct so a process peer cannot silently
  // apply the wrong temporal role transition.
  disable_time_regulation = 78U,
  disable_time_constrained = 79U,
  // Read-only ownership status is kept distinct from ownership-transfer
  // services so a remote peer cannot silently turn a boolean query into a
  // mutating operation.
  is_attribute_owned_by_federate = 80U,
  // Known-object class lookup must use the joined federate's discovery ledger
  // across the process seam; it is distinct from global FOM class lookup.
  get_known_object_class_handle = 81U,
  // Query Attribute Ownership returns a typed admission result and projects
  // one or more ownership-result callbacks through the receive-order fence.
  query_attribute_ownership = 82U,
  // Attribute Ownership Acquisition If Available carries a typed admission
  // result and one requester-side terminal callback projection through the
  // same receive-order fence.
  attribute_ownership_acquisition_if_available = 83U,
  // Regular Attribute Ownership Acquisition carries a typed admission result
  // and requester/owner callback projections through the same receive-order
  // fence.  It remains distinct from the If Available form so a private peer
  // cannot silently apply the wrong ownership state chart.
  attribute_ownership_acquisition = 84U,
  // Attribute Ownership Release Denied terminates matching pending
  // acquisitions and projects Attribute Ownership Unavailable callbacks.
  attribute_ownership_release_denied = 85U,
  // Cancel Attribute Ownership Acquisition commits the cancellation at the
  // process receive fence and projects the official confirmation callback.
  cancel_attribute_ownership_acquisition = 86U,
  // Cancel Negotiated Attribute Ownership Divestiture removes a pending
  // negotiated offer at the process service and re-plans any ordinary
  // acquisition release work without exposing registry state to the client.
  cancel_negotiated_attribute_ownership_divestiture = 87U,
  // Negotiated Attribute Ownership Divestiture records an owner-side offer and
  // projects Request Divestiture Confirmation through the ownership receive
  // fence.  It remains distinct from cancellation so a private peer cannot
  // silently apply the wrong ownership transition.
  negotiated_attribute_ownership_divestiture = 88U,
  // Confirm Divestiture commits the negotiated ownership transfer at the
  // process service and projects the callback-gated acquisition notification
  // through the same ownership receive fence.
  confirm_divestiture = 89U,
  // Federate identity lookups are kept on the process seam so the public
  // adapter never consults a process-local membership cache.  The name form
  // preserves the execution-scoped designator identity after resignation;
  // the handle form resolves only currently active member names.
  get_federate_handle = 90U,
  get_federate_name = 91U,
  // Handle normalization is execution-scoped and must be resolved by the
  // process-owned registry rather than a client-local handle cache. Keep each
  // official handle category distinct at the private boundary.
  normalize_federate_handle = 92U,
  normalize_object_class_handle = 93U,
  normalize_interaction_class_handle = 94U,
  normalize_object_instance_handle = 95U,
  // Flush Queue Request uses the same logical-time payload as the other
  // temporal requests, but has distinct admission and callback semantics.
  // Keep the operation identity separate so a private peer cannot silently
  // treat FQR as an ordinary Time Advance Request.
  flush_queue_request = 96U,
  // A process receiver acknowledges the callback boundary for one
  // timestamp-ordered payload.  The service keeps the coordinator entry
  // in-transit until this explicit process request arrives.
  acknowledge_tso_delivery = 97U,
  // Automatic resign directives are execution-member state, so process
  // endpoints resolve and mutate them through the federation service rather
  // than consulting a client-local registry.
  get_automatic_resign_directive = 98U,
  set_automatic_resign_directive = 99U,
  // Unconditional Attribute Ownership Divestiture commits the owner-side
  // registry transition and projects assumption callbacks through the normal
  // process ownership receive fence.
  unconditional_attribute_ownership_divestiture = 100U,
  // Change Interaction Order Type updates the publisher-scoped order
  // declaration at the process-owned registry. Keep it distinct from the
  // interaction declaration operations so a private peer cannot silently
  // treat an order-control request as publication state.
  change_interaction_order_type = 101U,
  // Change Attribute Order Type updates the owner-scoped order declaration
  // for one registered object instance at the process-owned registry.
  change_attribute_order_type = 102U,
  // Change Default Attribute Order Type updates the class-scoped default for
  // subsequently registered instances in the process-owned registry.
  change_default_attribute_order_type = 103U,
  // Change Default Attribute Transportation Type updates the class-scoped
  // transportation default for subsequently registered instances.
  change_default_attribute_transportation_type = 104U,
  // Instance transportation-type control is split into request/response
  // operations so a private peer cannot confuse the asynchronous confirmation
  // callback with the read-only report callback.
  request_attribute_transportation_type_change = 105U,
  query_attribute_transportation_type = 106U,
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
