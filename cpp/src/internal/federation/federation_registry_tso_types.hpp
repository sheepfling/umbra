#pragma once

#include "internal/federation/federation_registry_service_types.hpp"

namespace umbra::detail {

// Immutable payload retained beside the federation-owned TSO queue.  The
// queue intentionally stores only temporal identity and ordering; this record
// lets the binding reconstruct the official 6.13 callback after a recipient's
// grant makes the message eligible.
using TsoInteractionParameterValue =
    std::pair<std::uint64_t, rti1516_2025::VariableLengthData>;

struct TsoInteractionMessage {
  std::uint64_t messageId = 0;
  std::uint64_t producingFederateId = 0;
  std::uint64_t sentInteractionClassHandle = 0;
  std::vector<std::uint64_t> sentParameterHandles;
  std::vector<TsoInteractionParameterValue> parameters;
  rti1516_2025::VariableLengthData userSuppliedTag;
  std::string transportationName;
  std::set<std::uint64_t> sentRegionHandles;
  // Region handles are public opaque identities, but their mutable region
  // specifications belong to the producing federate. Retain the committed
  // invocation-time snapshots so a queued TSO interaction can still be
  // overlap-qualified after that federate resigns and its regions are
  // released from the live federation registry.
  std::map<std::uint64_t, RegionSpecificationSnapshot> sentRegionSnapshots;
  bool defaultRegionUsed = false;
  std::shared_ptr<rti1516_2025::LogicalTime const> timestamp;
  rti1516_2025::OrderType sentOrderType = rti1516_2025::TIMESTAMP;
  rti1516_2025::OrderType receivedOrderType = rti1516_2025::TIMESTAMP;
};

struct TsoInteractionDelivery {
  TsoQueuedMessage queuedMessage;
  TsoInteractionMessage message;
};

// Immutable payload retained beside the federation-owned TSO queue for the
// non-regional timestamped Update Attribute Values slice.  The queue carries
// ordering and recipient state; this record retains the official object,
// passel, value, tag, and timestamp data needed to reconstruct one or more
// Reflect Attribute Values callbacks at the recipient's grant boundary.
struct TsoAttributeUpdatePassel {
  std::string transportationName;
  std::vector<std::uint64_t> sentAttributeHandles;
  std::set<std::uint64_t> sentRegionHandles;
  // Keep the committed source realization with the passel for immediate
  // timestamped recipients as well as federation-owned TSO recipients.  The
  // public RegionHandle identities remain opaque, but a callback may occur
  // after the producer mutates the live range; delivery must not reinterpret
  // the accepted update against that later range.
  std::map<std::uint64_t, RegionSpecificationSnapshot> sentRegionSnapshots;
  bool defaultRegionUsed = false;
  rti1516_2025::OrderType preferredOrderType = rti1516_2025::TIMESTAMP;
};

using TsoAttributeValue =
    std::pair<std::uint64_t, rti1516_2025::VariableLengthData>;

struct TsoAttributeUpdateMessage {
  std::uint64_t messageId = 0;
  std::uint64_t producingFederateId = 0;
  std::uint64_t objectInstanceHandle = 0;
  std::vector<TsoAttributeValue> attributes;
  rti1516_2025::VariableLengthData userSuppliedTag;
  std::map<std::uint64_t, std::vector<TsoAttributeUpdatePassel>>
      passelsByRecipient;
  // Retain committed invocation-time source-region specifications. A
  // voluntary source resignation releases its public RegionHandles and
  // update-region associations before a queued TSO reflection reaches its
  // recipient; callback-time DDM evaluation must still use the accepted
  // passel's original regional realization.
  std::map<std::uint64_t, RegionSpecificationSnapshot> sentRegionSnapshots;
  std::shared_ptr<rti1516_2025::LogicalTime const> timestamp;
};

struct TsoAttributeUpdateDelivery {
  TsoQueuedMessage queuedMessage;
  TsoAttributeUpdateMessage message;
};

// Immutable payload retained beside the federation-owned TSO queue for the
// bounded timestamped Delete Object Instance slice.  The recipient list is
// captured at acceptance so a later declaration mutation can suppress a
// callback, but cannot manufacture a new removal for an instance that was not
// known when the delete was submitted.
struct TsoObjectDeletionRecipient {
  std::uint64_t receivingFederateId = 0;
  std::uint64_t objectInstanceHandle = 0;
  ObjectInstanceCallbackRoute callbackRoute;
  // The timestamped §6.17 callback may be delivered at an immediate or TSO
  // boundary. Preserve its recipient-local report route with the immutable
  // message payload so either boundary can append before user code.
  FederateServiceReportRoute serviceReportRoute;
};

struct TsoObjectDeletionMessage {
  std::uint64_t messageId = 0;
  std::uint64_t producingFederateId = 0;
  std::uint64_t objectInstanceHandle = 0;
  rti1516_2025::VariableLengthData userSuppliedTag;
  std::vector<TsoObjectDeletionRecipient> recipients;
  std::shared_ptr<rti1516_2025::LogicalTime const> timestamp;
  // The Delete Object Instance message's sent order is fixed by the
  // invocation-time HLAprivilegeToDeleteObject order.  Keep it with the
  // immutable payload so immediate and grant-boundary projections agree even
  // after the live object declaration changes.
  rti1516_2025::OrderType sentOrderType = rti1516_2025::RECEIVE;
};

struct TsoObjectDeletionDelivery {
  TsoQueuedMessage queuedMessage;
  TsoObjectDeletionMessage message;
};

// Immutable payload retained beside the federation-owned TSO queue for the
// bounded timestamped directed-interaction slice. Each recipient keeps its
// projected class/parameter view and callback route so delivery can recheck
// current declaration and target state immediately before user code.
struct TsoDirectedInteractionRecipient {
  std::uint64_t receivingFederateId = 0;
  std::uint64_t objectInstanceHandle = 0;
  std::uint64_t receivedInteractionClassHandle = 0;
  std::set<std::uint64_t> receivedParameterHandles;
  InteractionCallbackRoute callbackRoute;
};

struct TsoDirectedInteractionMessage {
  std::uint64_t messageId = 0;
  std::uint64_t producingFederateId = 0;
  std::uint64_t objectInstanceHandle = 0;
  std::uint64_t sentInteractionClassHandle = 0;
  std::vector<std::uint64_t> sentParameterHandles;
  std::vector<TsoInteractionParameterValue> parameters;
  rti1516_2025::VariableLengthData userSuppliedTag;
  std::string transportationName;
  std::vector<TsoDirectedInteractionRecipient> recipients;
  std::shared_ptr<rti1516_2025::LogicalTime const> timestamp;
  rti1516_2025::OrderType sentOrderType = rti1516_2025::TIMESTAMP;
  rti1516_2025::OrderType receivedOrderType = rti1516_2025::TIMESTAMP;
};

struct TsoDirectedInteractionDelivery {
  TsoQueuedMessage queuedMessage;
  TsoDirectedInteractionMessage message;
};

using TsoPayloadDelivery =
    std::variant<
        TsoInteractionDelivery,
        TsoAttributeUpdateDelivery,
        TsoObjectDeletionDelivery,
        TsoDirectedInteractionDelivery>;

// Private declaration state for the bounded IEEE 1516.1-2025 directed
// interaction slice. Publication and subscription are attached to an
// object-class/interaction-class pair; timestamped and region-gated delivery
// remain separate service boundaries.
enum class DirectedInteractionDeclarationStatus {
  applied,
  federation_does_not_exist,
  federate_not_member,
  object_class_not_defined,
  interaction_class_not_defined,
  interaction_not_defined_for_object_class,
  inconsistent_catalog,
};

// Private immutable routing result for receive-order Send Directed
// Interaction. The adapter owns callback delivery and rechecks the target
// object, source publication, recipient subscription, and known-instance
// state immediately before entering user code.
enum class ReceiveOrderDirectedInteractionStatus {
  applied,
  federation_does_not_exist,
  producing_federate_not_member,
  object_instance_not_known,
  interaction_class_not_defined,
  interaction_class_not_published,
  interaction_parameter_not_defined,
  inconsistent_catalog,
};

struct ReceiveOrderDirectedInteractionRecipient {
  std::uint64_t federateId = 0;
  std::uint64_t objectInstanceHandle = 0;
  std::uint64_t receivedInteractionClassHandle = 0;
  std::set<std::uint64_t> receivedParameterHandles;
  InteractionCallbackRoute callbackRoute;
};

struct ReceiveOrderDirectedInteractionPlan {
  ReceiveOrderDirectedInteractionStatus status =
      ReceiveOrderDirectedInteractionStatus::applied;
  std::string transportationName;
  rti1516_2025::OrderType preferredOrderType = rti1516_2025::RECEIVE;
  std::vector<ReceiveOrderDirectedInteractionRecipient> recipients;
};

enum class FederationTimeGrantStatus {
  applied,
  federation_does_not_exist,
  federate_not_member,
  time_advance_not_pending,
  stale_generation,
  grant_not_ready,
  inconsistent_temporal_state,
};

struct FederationTimeGrantDispatchResult {
  FederationTimeGrantStatus status = FederationTimeGrantStatus::applied;
  std::vector<FederationTimeGrantDispatch> dispatches;
};

enum class FederationTsoRegistryStatus {
  applied,
  federation_does_not_exist,
  federate_not_member,
  invalid_request,
};

struct FederationTsoMessageIdResult {
  FederationTsoRegistryStatus status = FederationTsoRegistryStatus::applied;
  std::uint64_t messageId = 0;
};

struct FederationTsoEnqueueResult {
  FederationTsoRegistryStatus status = FederationTsoRegistryStatus::applied;
  TsoMessageQueueStatus queueStatus = TsoMessageQueueStatus::invalid_message_id;
};

// Immutable work returned by the registry after it has atomically changed a
// timestamped-message recipient from delivered to retracted. The binding
// queues this work after releasing federation locks; it never invokes a
// FederateAmbassador from registry state. A message-retraction designator is
// execution-wide, so this applies equally to the supported interaction and
// attribute-update message families.
struct TsoRequestRetractionNotification {
  std::uint64_t receivingFederateId = 0;
  std::uint64_t messageId = 0;
  FederateCallbackRoute callbackRoute;
};

struct FederationTsoRetractionResult {
  FederationTsoRegistryStatus status = FederationTsoRegistryStatus::applied;
  TsoMessageRetractionResult queueResult;
  // The public service maps this standards-derived precondition separately
  // from queue terminal state: the original timestamp must be strictly later
  // than the producer's current/requested time plus actual lookahead.
  bool timestampEligible = true;
  std::vector<TsoRequestRetractionNotification>
      requestRetractionNotifications;
};

struct FederationTsoDeliveryRegistryResult {
  FederationTsoRegistryStatus status = FederationTsoRegistryStatus::applied;
  FederationTsoDeliveryResult delivery;
};

struct FederationTsoInteractionEnqueueResult {
  FederationTsoRegistryStatus status = FederationTsoRegistryStatus::applied;
  TsoMessageQueueStatus queueStatus = TsoMessageQueueStatus::invalid_message_id;
  std::uint64_t messageId = 0;
  std::size_t enqueuedRecipientCount = 0;
};

struct FederationTsoAttributeUpdateEnqueueResult {
  FederationTsoRegistryStatus status = FederationTsoRegistryStatus::applied;
  TsoMessageQueueStatus queueStatus = TsoMessageQueueStatus::invalid_message_id;
  std::uint64_t messageId = 0;
  std::size_t enqueuedRecipientCount = 0;
};

struct FederationTsoObjectDeletionEnqueueResult {
  FederationTsoRegistryStatus status = FederationTsoRegistryStatus::applied;
  TsoMessageQueueStatus queueStatus = TsoMessageQueueStatus::invalid_message_id;
  std::uint64_t messageId = 0;
  std::size_t enqueuedRecipientCount = 0;
  std::vector<TsoObjectDeletionRecipient> recipients;
  ObjectInstanceDeletionStatus deletionStatus = ObjectInstanceDeletionStatus::applied;
};

struct FederationTsoDirectedInteractionEnqueueResult {
  FederationTsoRegistryStatus status = FederationTsoRegistryStatus::applied;
  TsoMessageQueueStatus queueStatus = TsoMessageQueueStatus::invalid_message_id;
  std::uint64_t messageId = 0;
  std::size_t enqueuedRecipientCount = 0;
};

struct FederationTsoInteractionDeliveryRegistryResult {
  FederationTsoRegistryStatus status = FederationTsoRegistryStatus::applied;
  FederationTsoDeliveryStatus deliveryStatus = FederationTsoDeliveryStatus::no_messages;
  std::vector<TsoInteractionDelivery> deliveries;
};

struct FederationTsoPayloadDeliveryRegistryResult {
  FederationTsoRegistryStatus status = FederationTsoRegistryStatus::applied;
  FederationTsoDeliveryStatus deliveryStatus = FederationTsoDeliveryStatus::no_messages;
  std::vector<TsoPayloadDelivery> deliveries;
};

// In-process federation state for the first embedded runtime profile. It is a
// private kernel component rather than a public HLA service implementation:
// the adapter will translate its precise outcomes to standard exceptions only
// after FOM, logical-time, and callback behavior are available.

}  // namespace umbra::detail
