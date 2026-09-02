#pragma once

#include "internal/federation/federation_registry.hpp"
#include "internal/federation/process_transport_session.hpp"

#include <cstdint>
#include <deque>
#include <map>
#include <memory>
#include <mutex>
#include <optional>
#include <span>
#include <stdexcept>
#include <set>
#include <string>
#include <utility>
#include <vector>

namespace umbra::detail {

// These payloads are private process-boundary plumbing.  They carry the
// already-decoded values needed by the registry; they are not a replacement
// for the official C++ binding's HLA wire encodings.
struct ProcessFederationCreateRequest final {
  std::wstring federationName;
};

struct ProcessFederationJoinRequest final {
  std::wstring federationName;
  std::wstring federateType;
  std::optional<std::wstring> requestedFederateName;
};

struct ProcessFederationResignRequest final {
  std::wstring federationName;
  std::uint64_t federateId = 0U;
  rti1516_2025::ResignAction resignAction = rti1516_2025::NO_ACTION;
};

// A timestamp is carried across the private process seam as the official
// logical-time implementation name plus its standard VariableLengthData
// encoding.  Keeping the value opaque here prevents the transport from
// inventing a second logical-time representation while still allowing the
// receiving public adapter to reconstruct the official LogicalTime object.
struct ProcessFederationLogicalTime final {
  std::wstring implementationName;
  std::vector<std::uint8_t> encoding;
};

struct ProcessFederationSendInteractionRequest final {
  std::wstring federationName;
  std::uint64_t producingFederateId = 0U;
  std::uint64_t interactionClassHandle = 0U;
  std::vector<std::uint64_t> sentParameterHandles;
  std::vector<std::uint8_t> payload;
  // Absent for the ordinary receive-order overload.  This member is last so
  // existing private aggregate callers remain source-compatible.
  std::optional<ProcessFederationLogicalTime> timestamp;
};

// Directed declarations are carried over the private process seam as the
// normalized object-class/interaction-class pair owned by the registry.  The
// optional set preserves the official whole-object-class overload for
// unpublish/unsubscribe; an engaged empty set is distinct from that overload.
struct ProcessFederationObjectClassDirectedInteractionDeclarationRequest final {
  std::wstring federationName;
  std::uint64_t federateId = 0U;
  std::uint64_t objectClassHandle = 0U;
  std::optional<std::vector<std::uint64_t>> interactionClassHandles;
  bool universally = false;
};

struct ProcessFederationSendDirectedInteractionRequest final {
  std::wstring federationName;
  std::uint64_t producingFederateId = 0U;
  std::uint64_t objectInstanceHandle = 0U;
  std::uint64_t interactionClassHandle = 0U;
  std::vector<std::uint64_t> sentParameterHandles;
  std::vector<std::uint8_t> payload;
  std::optional<ProcessFederationLogicalTime> timestamp;
};

struct ProcessFederationRetractRequest final {
  std::wstring federationName;
  std::uint64_t producingFederateId = 0U;
  std::uint64_t messageId = 0U;
};

// Attribute values cross the private process seam as opaque bytes after the
// public adapter has validated the official handles. An optional logical-time
// value is appended for the bounded timestamped process projection; ordinary
// callers leave it absent and retain the original receive-order behavior.
using ProcessFederationAttributeValue =
    std::pair<std::uint64_t, std::vector<std::uint8_t>>;

struct ProcessFederationUpdateAttributeValuesRequest final {
  std::wstring federationName;
  std::uint64_t producingFederateId = 0U;
  std::uint64_t objectInstanceHandle = 0U;
  std::vector<ProcessFederationAttributeValue> attributeValues;
  std::vector<std::uint8_t> userSuppliedTag;
  // Absent for the ordinary receive-order overload. A present value is
  // reconstructed by the receiving public adapter through the official
  // timestamped Reflect Attribute Values callback.
  std::optional<ProcessFederationLogicalTime> timestamp;
};

struct ProcessFederationReceiveInteractionRequest final {
  std::wstring federationName;
  std::uint64_t receivingFederateId = 0U;
};

// Handle lookup requests carry the joined-federate identity explicitly so the
// process service can apply the same membership gate as message services.
// Parameter lookup deliberately uses the numeric interaction-class handle;
// this mirrors the official C++ API and avoids inventing a second reverse-name
// operation merely to service getParameterHandle.
struct ProcessFederationGetInteractionClassHandleRequest final {
  std::wstring federationName;
  std::uint64_t federateId = 0U;
  std::wstring interactionClassName;
};

// Object-class lookup has the same membership/name payload shape as
// interaction-class lookup, but keeps a distinct type and codec name so the
// private process operation cannot be mistaken for an interaction lookup at
// its call sites.
struct ProcessFederationGetObjectClassHandleRequest final {
  std::wstring federationName;
  std::uint64_t federateId = 0U;
  std::wstring objectClassName;
};

struct ProcessFederationGetParameterHandleRequest final {
  std::wstring federationName;
  std::uint64_t federateId = 0U;
  std::uint64_t interactionClassHandle = 0U;
  std::wstring parameterName;
};

struct ProcessFederationGetAttributeHandleRequest final {
  std::wstring federationName;
  std::uint64_t federateId = 0U;
  std::uint64_t objectClassHandle = 0U;
  std::wstring attributeName;
};

struct ProcessFederationInteractionClassDeclarationRequest final {
  std::wstring federationName;
  std::uint64_t federateId = 0U;
  std::uint64_t interactionClassHandle = 0U;
  // Publish uses this as the desired publication state. Subscribe uses it as
  // the desired active/passive state; the operation itself selects whether
  // the request is a subscription or an unsubscription.
  bool active = false;
};

// Object-class publication is carried as the official attribute set's
// normalized numeric handles.  The process service keeps this request private
// and applies it to the registry's declaration ledger; no transport type is
// exposed through the standards-facing API.
struct ProcessFederationObjectClassAttributeDeclarationRequest final {
  std::wstring federationName;
  std::uint64_t federateId = 0U;
  std::uint64_t objectClassHandle = 0U;
  std::vector<std::uint64_t> attributeHandles;
};

// Ordinary object-class subscription state crosses the private process seam
// as normalized numeric handles plus the official FDD update-rate designator.
// The operation selects subscribe versus unsubscribe; unsubscribe ignores
// active and uses an empty attribute set as the whole-class form.
struct ProcessFederationObjectClassAttributeSubscriptionRequest final {
  std::wstring federationName;
  std::uint64_t federateId = 0U;
  std::uint64_t objectClassHandle = 0U;
  std::vector<std::uint64_t> attributeHandles;
  bool active = true;
  std::string updateRateDesignator = "HLAdefault";
};

// Regional object-class subscription state crosses the private process seam
// as normalized attribute-to-region associations plus the official active and
// update-rate arguments.  The operation selects subscribe versus unsubscribe;
// unsubscribe ignores the active/rate members.
struct ProcessFederationObjectClassAttributeRegionalSubscriptionRequest final {
  std::wstring federationName;
  std::uint64_t federateId = 0U;
  std::uint64_t objectClassHandle = 0U;
  std::map<std::uint64_t, std::set<std::uint64_t>> attributesAndRegions;
  bool active = true;
  std::string updateRateDesignator = "HLAdefault";
};

struct ProcessFederationRegisterObjectInstanceRequest final {
  std::wstring federationName;
  std::uint64_t federateId = 0U;
  std::uint64_t objectClassHandle = 0U;
  std::optional<std::wstring> requestedObjectInstanceName;
};

// Local Delete Object Instance changes only the invoking federate's known
// object ledger. It is distinct from receive-order deletion and has no
// callback fan-out.
struct ProcessFederationLocalDeleteObjectInstanceRequest final {
  std::wstring federationName;
  std::uint64_t federateId = 0U;
  std::uint64_t objectInstanceHandle = 0U;
};

// Receive-order Delete Object Instance removes the producer's known instance
// and reserves a callback for every other known recipient. The user tag is
// carried as opaque bytes until the public callback bridge reconstructs the
// official VariableLengthData value.
struct ProcessFederationDeleteObjectInstanceRequest final {
  std::wstring federationName;
  std::uint64_t federateId = 0U;
  std::uint64_t objectInstanceHandle = 0U;
  std::vector<std::uint8_t> userSuppliedTag;
  // Absent for the ordinary receive-order overload. A present value is
  // reconstructed by the process service and delivered through the official
  // timestamped Remove Object Instance callback overload.
  std::optional<ProcessFederationLogicalTime> timestamp;
};

struct ProcessFederationReserveObjectInstanceNameRequest final {
  std::wstring federationName;
  std::uint64_t federateId = 0U;
  std::wstring objectInstanceName;
};

struct ProcessFederationGetDimensionHandleRequest final {
  std::wstring federationName;
  std::uint64_t federateId = 0U;
  std::string dimensionName;
};

struct ProcessFederationDimensionRequest final {
  std::wstring federationName;
  std::uint64_t federateId = 0U;
  std::uint64_t dimensionHandle = 0U;
};

struct ProcessFederationCreateRegionRequest final {
  std::wstring federationName;
  std::uint64_t federateId = 0U;
  std::vector<std::uint64_t> dimensionHandles;
};

struct ProcessFederationRegionSetRequest final {
  std::wstring federationName;
  std::uint64_t federateId = 0U;
  std::vector<std::uint64_t> regionHandles;
};

struct ProcessFederationRegionRequest final {
  std::wstring federationName;
  std::uint64_t federateId = 0U;
  std::uint64_t regionHandle = 0U;
};

struct ProcessFederationGetRangeBoundsRequest final {
  std::wstring federationName;
  std::uint64_t federateId = 0U;
  std::uint64_t regionHandle = 0U;
  std::uint64_t dimensionHandle = 0U;
};

struct ProcessFederationSetRangeBoundsRequest final {
  std::wstring federationName;
  std::uint64_t federateId = 0U;
  std::uint64_t regionHandle = 0U;
  std::uint64_t dimensionHandle = 0U;
  unsigned long lowerBound = 0UL;
  unsigned long upperBound = 0UL;
};

struct ProcessFederationRegisterObjectInstanceWithRegionsRequest final {
  std::wstring federationName;
  std::uint64_t federateId = 0U;
  std::uint64_t objectClassHandle = 0U;
  std::map<std::uint64_t, std::set<std::uint64_t>> updateRegionsByAttribute;
  std::optional<std::wstring> requestedObjectInstanceName;
};

// Object-instance source-region association state crosses the private
// process seam as normalized attribute-to-region associations. The operation
// selects associate versus unassociate; the public binding owns the official
// handles, validation, and exception surface.
struct ProcessFederationObjectInstanceRegionAssociationRequest final {
  std::wstring federationName;
  std::uint64_t federateId = 0U;
  std::uint64_t objectInstanceHandle = 0U;
  std::map<std::uint64_t, std::set<std::uint64_t>> attributesAndRegions;
};

struct ProcessFederationJoinResult final {
  std::uint64_t federateId = 0U;
  std::wstring federateName;
};

struct ProcessFederationSendInteractionResult final {
  std::uint32_t recipientCount = 0U;
  // Zero retains the ordinary receive-order result. A nonzero value is the
  // execution-owned designator returned by a timestamped directed send.
  std::uint64_t messageId = 0U;
};

enum class ProcessFederationRetractStatus : std::uint8_t {
  applied = 0U,
  invalid_handle = 1U,
  message_no_longer_retractable = 2U,
  federate_not_member = 3U,
};

struct ProcessFederationRetractResult final {
  ProcessFederationRetractStatus status =
      ProcessFederationRetractStatus::applied;
};

struct ProcessFederationRequestRetractionEvent final {
  std::uint64_t messageId = 0U;
};

struct ProcessFederationHandleResult final {
  std::uint64_t handle = 0U;
};

struct ProcessFederationRegisterObjectInstanceResult final {
  std::uint64_t objectInstanceHandle = 0U;
  std::wstring objectInstanceName;
  ObjectInstanceRegistrationStatus status =
      ObjectInstanceRegistrationStatus::applied;
};

struct ProcessFederationReserveObjectInstanceNameResult final {
  bool succeeded = false;
  std::wstring objectInstanceName;
  ObjectInstanceNameReservationStatus status =
      ObjectInstanceNameReservationStatus::applied;
};

struct ProcessFederationRegionStatusResult final {
  RegionServiceStatus status = RegionServiceStatus::applied;
};

struct ProcessFederationLocalDeleteObjectInstanceResult final {
  LocalObjectInstanceDeletionStatus status =
      LocalObjectInstanceDeletionStatus::applied;
};

struct ProcessFederationDeleteObjectInstanceResult final {
  ObjectInstanceDeletionStatus status = ObjectInstanceDeletionStatus::applied;
  std::uint32_t recipientCount = 0U;
  // Zero retains the ordinary receive-order result. A nonzero value is the
  // process service's execution-owned timestamped-message identity; the
  // public process profile currently keeps its returned handle invalid until
  // time-management/retraction services are exposed on that endpoint.
  std::uint64_t messageId = 0U;
};

struct ProcessFederationBooleanResult final {
  bool value = false;
};

struct ProcessFederationAttributeScopeAdvisorySwitchRequest final {
  std::wstring federationName;
  std::uint64_t federateId = 0U;
  bool switchValue = false;
};

struct ProcessFederationObjectInstanceRegionAssociationResult final {
  ObjectInstanceRegionAssociationStatus status =
      ObjectInstanceRegionAssociationStatus::applied;
};

struct ProcessFederationCreateRegionResult final {
  RegionServiceStatus status = RegionServiceStatus::applied;
  std::uint64_t regionHandle = 0U;
};

struct ProcessFederationDimensionSetResult final {
  RegionServiceStatus status = RegionServiceStatus::applied;
  std::vector<std::uint64_t> dimensionHandles;
};

struct ProcessFederationRangeBoundsResult final {
  RegionServiceStatus status = RegionServiceStatus::applied;
  unsigned long lowerBound = 0UL;
  unsigned long upperBound = 0UL;
};

struct ProcessFederationDimensionUpperBoundResult final {
  bool found = false;
  unsigned long upperBound = 0UL;
};

// The first process service carried an opaque interaction payload because the
// private probe only needed a user-tag sentinel.  The public C++ endpoint uses
// this versioned envelope so parameter values and the user-supplied tag cross
// the private seam without exposing a wire format in the standards API.
using ProcessFederationInteractionParameterValue =
    std::pair<std::uint64_t, std::vector<std::uint8_t>>;

struct ProcessFederationInteractionEnvelope final {
  std::vector<ProcessFederationInteractionParameterValue> parameterValues;
  std::vector<std::uint8_t> userSuppliedTag;
};

struct ProcessFederationInteractionEvent final {
  std::uint64_t producingFederateId = 0U;
  std::uint64_t receivingFederateId = 0U;
  std::uint64_t interactionClassHandle = 0U;
  std::vector<std::uint64_t> parameterHandles;
  std::vector<std::uint8_t> payload;
  std::string transportationName;
  // Absent for an ordinary receive-order event.  A present value is delivered
  // through the timestamped FederateAmbassador callback overload.  This is
  // intentionally appended to preserve existing service-test aggregates.
  std::optional<ProcessFederationLogicalTime> timestamp;
  // Present only for a directed interaction.  It is appended to preserve
  // existing ordinary-event aggregate callers and legacy event decoding.
  std::optional<std::uint64_t> objectInstanceHandle;
  // Present for a timestamped directed interaction. The process service uses
  // this execution-wide identity to suppress a queued event when its producer
  // retracts before the receiver crosses the callback boundary.
  std::optional<std::uint64_t> retractionMessageId;
};

struct ProcessFederationAttributeUpdateEvent final {
  std::uint64_t producingFederateId = 0U;
  std::uint64_t receivingFederateId = 0U;
  std::uint64_t objectInstanceHandle = 0U;
  std::vector<ProcessFederationAttributeValue> attributeValues;
  std::vector<std::uint8_t> userSuppliedTag;
  std::string transportationName;
  // Present for a regional passel.  A null value plus defaultRegionUsed=true
  // represents the 2025 default region (an empty callback region set).
  std::optional<std::set<std::uint64_t>> sentRegionHandles;
  bool defaultRegionUsed = false;
  // Present when the originating Update Attribute Values invocation carried
  // a logical timestamp. This is appended so existing private aggregate
  // callers remain source-compatible.
  std::optional<ProcessFederationLogicalTime> timestamp;
};

// Object-instance discovery crosses the private process seam as the already
// resolved values needed by the official FederateAmbassador::discoverObjectInstance
// callback.  The registry remains authoritative for the discovery predicate
// and known-instance transition; this record carries only the immutable event
// projection to the receiving process.
struct ProcessFederationObjectInstanceDiscoveryEvent final {
  std::uint64_t receivingFederateId = 0U;
  std::uint64_t objectInstanceHandle = 0U;
  std::uint64_t objectClassHandle = 0U;
  std::wstring objectInstanceName;
  std::uint64_t producingFederateId = 0U;
};

// A receive-order Remove Object Instance callback crosses the same
// deterministic process receive fence as discovery and scope events. The
// producer identity is supplied by the registry's callback-boundary snapshot.
struct ProcessFederationObjectInstanceRemovalEvent final {
  std::uint64_t receivingFederateId = 0U;
  std::uint64_t objectInstanceHandle = 0U;
  std::uint64_t producingFederateId = 0U;
  std::vector<std::uint8_t> userSuppliedTag;
  // Present for a timestamped Remove Object Instance callback. These fields
  // are appended so ordinary event aggregates and legacy process payloads
  // remain source/wire compatible.
  std::optional<ProcessFederationLogicalTime> timestamp;
  std::optional<std::uint64_t> retractionMessageId;
};

// A planned Attribute In/Out Of Scope transition projected across the private
// process boundary. The service rechecks the live registry before dequeueing
// the event; the client then invokes the official scope callback.
struct ProcessFederationObjectInstanceScopeChangeEvent final {
  std::uint64_t receivingFederateId = 0U;
  std::uint64_t objectInstanceHandle = 0U;
  std::set<std::uint64_t> attributeHandles;
  bool inScope = false;
};

// An owner-directed Attribute Relevance Advisory projected across the
// private process boundary.  The registry uses a zero receiving-federate id
// for this callback family because the advisory is delivered to the owner;
// the providing id therefore identifies the process session that must receive
// the event.  A present update-rate designator selects the rate-bearing
// official Turn Updates On overload.
struct ProcessFederationAttributeRelevanceAdvisoryEvent final {
  std::uint64_t providingFederateId = 0U;
  std::uint64_t receivingFederateId = 0U;
  std::uint64_t objectInstanceHandle = 0U;
  std::set<std::uint64_t> attributeHandles;
  bool turnUpdatesOn = false;
  std::optional<std::string> updateRateDesignator;
};

struct ProcessFederationReceiveInteractionResult final {
  std::optional<ProcessFederationInteractionEvent> event;
  // Polling receive requests share one deterministic service queue.  When an
  // attribute reflection is next, the result carries it in this adjacent
  // slot so existing receive-interaction polling remains wire-compatible
  // while the public adapter can dispatch both callback families.
  std::optional<ProcessFederationAttributeUpdateEvent> attributeEvent;
  // Object discovery shares the same polling fence as interaction and
  // reflection events.  Keeping it as an adjacent result slot lets the public
  // adapter drain one deterministic socket request without introducing a
  // competing poll into older process fixtures.
  std::optional<ProcessFederationObjectInstanceDiscoveryEvent> discoveryEvent;
  // Scope transitions share the deterministic receive-interaction polling
  // fence with interaction, discovery, and reflection events.
  std::optional<ProcessFederationObjectInstanceScopeChangeEvent>
      scopeChangeEvent;
  // Receive-order object removal shares that fence and is committed by the
  // service immediately before the callback crosses the process boundary.
  std::optional<ProcessFederationObjectInstanceRemovalEvent> removalEvent;
  // Attribute relevance advisories are owner-directed and share the same
  // receive-interaction fence.  This field is appended to preserve existing
  // aggregate initialization of the earlier event slots.
  std::optional<ProcessFederationAttributeRelevanceAdvisoryEvent>
      attributeRelevanceAdvisoryEvent;
};

struct ProcessFederationUpdateAttributeValuesResult final {
  std::uint32_t recipientCount = 0U;
};

struct ProcessFederationReceiveAttributeUpdateResult final {
  std::optional<ProcessFederationAttributeUpdateEvent> event;
};

struct ProcessFederationReceiveObjectInstanceDiscoveryResult final {
  std::optional<ProcessFederationObjectInstanceDiscoveryEvent> event;
};

// Private service behavior switches used while the process endpoint is being
// carried toward the public RTI binding.  Polling remains the compatibility
// default for the existing service tests; the independently launched process
// slice enables event delivery so the client can exercise the official
// FederateAmbassador callback bridge over the process boundary.
struct ProcessFederationServiceOptions final {
  bool pushReceiveOrderEvents = false;
};

class ProcessFederationServiceProtocolError final : public std::runtime_error {
 public:
  using std::runtime_error::runtime_error;
};

[[nodiscard]] std::vector<std::uint8_t> encodeProcessFederationCreateRequest(
    ProcessFederationCreateRequest const& request);
[[nodiscard]] ProcessFederationCreateRequest decodeProcessFederationCreateRequest(
    std::span<std::uint8_t const> encoded);

[[nodiscard]] std::vector<std::uint8_t> encodeProcessFederationJoinRequest(
    ProcessFederationJoinRequest const& request);
[[nodiscard]] ProcessFederationJoinRequest decodeProcessFederationJoinRequest(
    std::span<std::uint8_t const> encoded);

[[nodiscard]] std::vector<std::uint8_t> encodeProcessFederationResignRequest(
    ProcessFederationResignRequest const& request);
[[nodiscard]] ProcessFederationResignRequest decodeProcessFederationResignRequest(
    std::span<std::uint8_t const> encoded);

[[nodiscard]] std::vector<std::uint8_t> encodeProcessFederationSendInteractionRequest(
    ProcessFederationSendInteractionRequest const& request);
[[nodiscard]] ProcessFederationSendInteractionRequest
decodeProcessFederationSendInteractionRequest(std::span<std::uint8_t const> encoded);

[[nodiscard]] std::vector<std::uint8_t>
encodeProcessFederationSendDirectedInteractionRequest(
    ProcessFederationSendDirectedInteractionRequest const& request);
[[nodiscard]] ProcessFederationSendDirectedInteractionRequest
decodeProcessFederationSendDirectedInteractionRequest(
    std::span<std::uint8_t const> encoded);

[[nodiscard]] std::vector<std::uint8_t> encodeProcessFederationRetractRequest(
    ProcessFederationRetractRequest const& request);
[[nodiscard]] ProcessFederationRetractRequest
decodeProcessFederationRetractRequest(std::span<std::uint8_t const> encoded);

[[nodiscard]] std::vector<std::uint8_t>
encodeProcessFederationObjectClassDirectedInteractionDeclarationRequest(
    ProcessFederationObjectClassDirectedInteractionDeclarationRequest const& request);
[[nodiscard]] ProcessFederationObjectClassDirectedInteractionDeclarationRequest
decodeProcessFederationObjectClassDirectedInteractionDeclarationRequest(
    std::span<std::uint8_t const> encoded);

[[nodiscard]] std::vector<std::uint8_t>
encodeProcessFederationReceiveInteractionRequest(
    ProcessFederationReceiveInteractionRequest const& request);
[[nodiscard]] ProcessFederationReceiveInteractionRequest
decodeProcessFederationReceiveInteractionRequest(
    std::span<std::uint8_t const> encoded);

[[nodiscard]] std::vector<std::uint8_t>
encodeProcessFederationUpdateAttributeValuesRequest(
    ProcessFederationUpdateAttributeValuesRequest const& request);
[[nodiscard]] ProcessFederationUpdateAttributeValuesRequest
decodeProcessFederationUpdateAttributeValuesRequest(
    std::span<std::uint8_t const> encoded);

[[nodiscard]] std::vector<std::uint8_t>
encodeProcessFederationGetInteractionClassHandleRequest(
    ProcessFederationGetInteractionClassHandleRequest const& request);
[[nodiscard]] ProcessFederationGetInteractionClassHandleRequest
decodeProcessFederationGetInteractionClassHandleRequest(
    std::span<std::uint8_t const> encoded);

[[nodiscard]] std::vector<std::uint8_t>
encodeProcessFederationGetObjectClassHandleRequest(
    ProcessFederationGetObjectClassHandleRequest const& request);
[[nodiscard]] ProcessFederationGetObjectClassHandleRequest
decodeProcessFederationGetObjectClassHandleRequest(
    std::span<std::uint8_t const> encoded);

[[nodiscard]] std::vector<std::uint8_t>
encodeProcessFederationGetParameterHandleRequest(
    ProcessFederationGetParameterHandleRequest const& request);
[[nodiscard]] ProcessFederationGetParameterHandleRequest
decodeProcessFederationGetParameterHandleRequest(
    std::span<std::uint8_t const> encoded);

[[nodiscard]] std::vector<std::uint8_t>
encodeProcessFederationGetAttributeHandleRequest(
    ProcessFederationGetAttributeHandleRequest const& request);
[[nodiscard]] ProcessFederationGetAttributeHandleRequest
decodeProcessFederationGetAttributeHandleRequest(
    std::span<std::uint8_t const> encoded);

[[nodiscard]] std::vector<std::uint8_t>
encodeProcessFederationInteractionClassDeclarationRequest(
    ProcessFederationInteractionClassDeclarationRequest const& request);
[[nodiscard]] ProcessFederationInteractionClassDeclarationRequest
decodeProcessFederationInteractionClassDeclarationRequest(
    std::span<std::uint8_t const> encoded);

[[nodiscard]] std::vector<std::uint8_t>
encodeProcessFederationObjectClassAttributeDeclarationRequest(
    ProcessFederationObjectClassAttributeDeclarationRequest const& request);
[[nodiscard]] ProcessFederationObjectClassAttributeDeclarationRequest
decodeProcessFederationObjectClassAttributeDeclarationRequest(
    std::span<std::uint8_t const> encoded);

[[nodiscard]] std::vector<std::uint8_t>
encodeProcessFederationObjectClassAttributeSubscriptionRequest(
    ProcessFederationObjectClassAttributeSubscriptionRequest const& request);
[[nodiscard]] ProcessFederationObjectClassAttributeSubscriptionRequest
decodeProcessFederationObjectClassAttributeSubscriptionRequest(
    std::span<std::uint8_t const> encoded);

[[nodiscard]] std::vector<std::uint8_t>
encodeProcessFederationObjectClassAttributeRegionalSubscriptionRequest(
    ProcessFederationObjectClassAttributeRegionalSubscriptionRequest const& request);
[[nodiscard]] ProcessFederationObjectClassAttributeRegionalSubscriptionRequest
decodeProcessFederationObjectClassAttributeRegionalSubscriptionRequest(
    std::span<std::uint8_t const> encoded);

[[nodiscard]] std::vector<std::uint8_t>
encodeProcessFederationRegisterObjectInstanceRequest(
    ProcessFederationRegisterObjectInstanceRequest const& request);
[[nodiscard]] ProcessFederationRegisterObjectInstanceRequest
decodeProcessFederationRegisterObjectInstanceRequest(
    std::span<std::uint8_t const> encoded);

[[nodiscard]] std::vector<std::uint8_t>
encodeProcessFederationLocalDeleteObjectInstanceRequest(
    ProcessFederationLocalDeleteObjectInstanceRequest const& request);
[[nodiscard]] ProcessFederationLocalDeleteObjectInstanceRequest
decodeProcessFederationLocalDeleteObjectInstanceRequest(
    std::span<std::uint8_t const> encoded);

[[nodiscard]] std::vector<std::uint8_t>
encodeProcessFederationDeleteObjectInstanceRequest(
    ProcessFederationDeleteObjectInstanceRequest const& request);
[[nodiscard]] ProcessFederationDeleteObjectInstanceRequest
decodeProcessFederationDeleteObjectInstanceRequest(
    std::span<std::uint8_t const> encoded);

[[nodiscard]] std::vector<std::uint8_t>
encodeProcessFederationReserveObjectInstanceNameRequest(
    ProcessFederationReserveObjectInstanceNameRequest const& request);
[[nodiscard]] ProcessFederationReserveObjectInstanceNameRequest
decodeProcessFederationReserveObjectInstanceNameRequest(
    std::span<std::uint8_t const> encoded);

[[nodiscard]] std::vector<std::uint8_t>
encodeProcessFederationGetDimensionHandleRequest(
    ProcessFederationGetDimensionHandleRequest const& request);
[[nodiscard]] ProcessFederationGetDimensionHandleRequest
decodeProcessFederationGetDimensionHandleRequest(
    std::span<std::uint8_t const> encoded);

[[nodiscard]] std::vector<std::uint8_t>
encodeProcessFederationDimensionRequest(
    ProcessFederationDimensionRequest const& request);
[[nodiscard]] ProcessFederationDimensionRequest
decodeProcessFederationDimensionRequest(std::span<std::uint8_t const> encoded);

[[nodiscard]] std::vector<std::uint8_t>
encodeProcessFederationCreateRegionRequest(
    ProcessFederationCreateRegionRequest const& request);
[[nodiscard]] ProcessFederationCreateRegionRequest
decodeProcessFederationCreateRegionRequest(
    std::span<std::uint8_t const> encoded);

[[nodiscard]] std::vector<std::uint8_t>
encodeProcessFederationRegionSetRequest(
    ProcessFederationRegionSetRequest const& request);
[[nodiscard]] ProcessFederationRegionSetRequest
decodeProcessFederationRegionSetRequest(
    std::span<std::uint8_t const> encoded);

[[nodiscard]] std::vector<std::uint8_t>
encodeProcessFederationRegionRequest(
    ProcessFederationRegionRequest const& request);
[[nodiscard]] ProcessFederationRegionRequest
decodeProcessFederationRegionRequest(std::span<std::uint8_t const> encoded);

[[nodiscard]] std::vector<std::uint8_t>
encodeProcessFederationGetRangeBoundsRequest(
    ProcessFederationGetRangeBoundsRequest const& request);
[[nodiscard]] ProcessFederationGetRangeBoundsRequest
decodeProcessFederationGetRangeBoundsRequest(
    std::span<std::uint8_t const> encoded);

[[nodiscard]] std::vector<std::uint8_t>
encodeProcessFederationSetRangeBoundsRequest(
    ProcessFederationSetRangeBoundsRequest const& request);
[[nodiscard]] ProcessFederationSetRangeBoundsRequest
decodeProcessFederationSetRangeBoundsRequest(
    std::span<std::uint8_t const> encoded);

[[nodiscard]] std::vector<std::uint8_t>
encodeProcessFederationRegisterObjectInstanceWithRegionsRequest(
    ProcessFederationRegisterObjectInstanceWithRegionsRequest const& request);
[[nodiscard]] ProcessFederationRegisterObjectInstanceWithRegionsRequest
decodeProcessFederationRegisterObjectInstanceWithRegionsRequest(
    std::span<std::uint8_t const> encoded);

[[nodiscard]] std::vector<std::uint8_t>
encodeProcessFederationObjectInstanceRegionAssociationRequest(
    ProcessFederationObjectInstanceRegionAssociationRequest const& request);
[[nodiscard]] ProcessFederationObjectInstanceRegionAssociationRequest
decodeProcessFederationObjectInstanceRegionAssociationRequest(
    std::span<std::uint8_t const> encoded);

[[nodiscard]] std::vector<std::uint8_t> encodeProcessFederationJoinResult(
    ProcessFederationJoinResult const& result);
[[nodiscard]] ProcessFederationJoinResult decodeProcessFederationJoinResult(
    std::span<std::uint8_t const> encoded);

[[nodiscard]] std::vector<std::uint8_t> encodeProcessFederationSendInteractionResult(
    ProcessFederationSendInteractionResult const& result);
[[nodiscard]] ProcessFederationSendInteractionResult
decodeProcessFederationSendInteractionResult(std::span<std::uint8_t const> encoded);

[[nodiscard]] std::vector<std::uint8_t> encodeProcessFederationRetractResult(
    ProcessFederationRetractResult const& result);
[[nodiscard]] ProcessFederationRetractResult
decodeProcessFederationRetractResult(std::span<std::uint8_t const> encoded);

[[nodiscard]] std::vector<std::uint8_t>
encodeProcessFederationRequestRetractionEvent(
    ProcessFederationRequestRetractionEvent const& event);
[[nodiscard]] ProcessFederationRequestRetractionEvent
decodeProcessFederationRequestRetractionEvent(
    std::span<std::uint8_t const> encoded);

[[nodiscard]] std::vector<std::uint8_t> encodeProcessFederationHandleResult(
    ProcessFederationHandleResult const& result);
[[nodiscard]] ProcessFederationHandleResult decodeProcessFederationHandleResult(
    std::span<std::uint8_t const> encoded);

[[nodiscard]] std::vector<std::uint8_t>
encodeProcessFederationRegisterObjectInstanceResult(
    ProcessFederationRegisterObjectInstanceResult const& result);
[[nodiscard]] ProcessFederationRegisterObjectInstanceResult
decodeProcessFederationRegisterObjectInstanceResult(
    std::span<std::uint8_t const> encoded);

[[nodiscard]] std::vector<std::uint8_t>
encodeProcessFederationReserveObjectInstanceNameResult(
    ProcessFederationReserveObjectInstanceNameResult const& result);
[[nodiscard]] ProcessFederationReserveObjectInstanceNameResult
decodeProcessFederationReserveObjectInstanceNameResult(
    std::span<std::uint8_t const> encoded);

[[nodiscard]] std::vector<std::uint8_t>
encodeProcessFederationRegionStatusResult(
    ProcessFederationRegionStatusResult const& result);
[[nodiscard]] ProcessFederationRegionStatusResult
decodeProcessFederationRegionStatusResult(
    std::span<std::uint8_t const> encoded);

[[nodiscard]] std::vector<std::uint8_t>
encodeProcessFederationLocalDeleteObjectInstanceResult(
    ProcessFederationLocalDeleteObjectInstanceResult const& result);
[[nodiscard]] ProcessFederationLocalDeleteObjectInstanceResult
decodeProcessFederationLocalDeleteObjectInstanceResult(
    std::span<std::uint8_t const> encoded);

[[nodiscard]] std::vector<std::uint8_t>
encodeProcessFederationDeleteObjectInstanceResult(
    ProcessFederationDeleteObjectInstanceResult const& result);
[[nodiscard]] ProcessFederationDeleteObjectInstanceResult
decodeProcessFederationDeleteObjectInstanceResult(
    std::span<std::uint8_t const> encoded);

[[nodiscard]] std::vector<std::uint8_t>
encodeProcessFederationAttributeScopeAdvisorySwitchRequest(
    ProcessFederationAttributeScopeAdvisorySwitchRequest const& request);
[[nodiscard]] ProcessFederationAttributeScopeAdvisorySwitchRequest
decodeProcessFederationAttributeScopeAdvisorySwitchRequest(
    std::span<std::uint8_t const> encoded);
[[nodiscard]] std::vector<std::uint8_t> encodeProcessFederationBooleanResult(
    ProcessFederationBooleanResult const& result);
[[nodiscard]] ProcessFederationBooleanResult decodeProcessFederationBooleanResult(
    std::span<std::uint8_t const> encoded);

[[nodiscard]] std::vector<std::uint8_t>
encodeProcessFederationObjectInstanceRegionAssociationResult(
    ProcessFederationObjectInstanceRegionAssociationResult const& result);
[[nodiscard]] ProcessFederationObjectInstanceRegionAssociationResult
decodeProcessFederationObjectInstanceRegionAssociationResult(
    std::span<std::uint8_t const> encoded);

[[nodiscard]] std::vector<std::uint8_t>
encodeProcessFederationCreateRegionResult(
    ProcessFederationCreateRegionResult const& result);
[[nodiscard]] ProcessFederationCreateRegionResult
decodeProcessFederationCreateRegionResult(
    std::span<std::uint8_t const> encoded);

[[nodiscard]] std::vector<std::uint8_t>
encodeProcessFederationDimensionSetResult(
    ProcessFederationDimensionSetResult const& result);
[[nodiscard]] ProcessFederationDimensionSetResult
decodeProcessFederationDimensionSetResult(
    std::span<std::uint8_t const> encoded);

[[nodiscard]] std::vector<std::uint8_t>
encodeProcessFederationRangeBoundsResult(
    ProcessFederationRangeBoundsResult const& result);
[[nodiscard]] ProcessFederationRangeBoundsResult
decodeProcessFederationRangeBoundsResult(
    std::span<std::uint8_t const> encoded);

[[nodiscard]] std::vector<std::uint8_t>
encodeProcessFederationDimensionUpperBoundResult(
    ProcessFederationDimensionUpperBoundResult const& result);
[[nodiscard]] ProcessFederationDimensionUpperBoundResult
decodeProcessFederationDimensionUpperBoundResult(
    std::span<std::uint8_t const> encoded);

[[nodiscard]] std::vector<std::uint8_t>
encodeProcessFederationInteractionEnvelope(
    ProcessFederationInteractionEnvelope const& envelope);
// An absent value means the payload is from the legacy opaque process probe
// path and should be presented as the user-supplied tag.  A present value is
// the versioned public-endpoint envelope; malformed envelopes throw the
// private protocol error.
[[nodiscard]] std::optional<ProcessFederationInteractionEnvelope>
decodeProcessFederationInteractionEnvelope(
    std::span<std::uint8_t const> encoded);

[[nodiscard]] std::vector<std::uint8_t>
encodeProcessFederationReceiveInteractionResult(
    ProcessFederationReceiveInteractionResult const& result);
[[nodiscard]] ProcessFederationReceiveInteractionResult
decodeProcessFederationReceiveInteractionResult(
    std::span<std::uint8_t const> encoded);

[[nodiscard]] std::vector<std::uint8_t>
encodeProcessFederationUpdateAttributeValuesResult(
    ProcessFederationUpdateAttributeValuesResult const& result);
[[nodiscard]] ProcessFederationUpdateAttributeValuesResult
decodeProcessFederationUpdateAttributeValuesResult(
    std::span<std::uint8_t const> encoded);

[[nodiscard]] std::vector<std::uint8_t>
encodeProcessFederationReceiveAttributeUpdateResult(
    ProcessFederationReceiveAttributeUpdateResult const& result);
[[nodiscard]] ProcessFederationReceiveAttributeUpdateResult
decodeProcessFederationReceiveAttributeUpdateResult(
    std::span<std::uint8_t const> encoded);

[[nodiscard]] std::vector<std::uint8_t>
encodeProcessFederationReceiveObjectInstanceDiscoveryResult(
    ProcessFederationReceiveObjectInstanceDiscoveryResult const& result);
[[nodiscard]] ProcessFederationReceiveObjectInstanceDiscoveryResult
decodeProcessFederationReceiveObjectInstanceDiscoveryResult(
    std::span<std::uint8_t const> encoded);

// Private service binding for the first process-boundary federation path.  A
// validated definition is supplied by the higher-level FOM coordinator; the
// process request only names the execution.  Each transport session gets a
// handler tied to its joined federate identity, while the registry remains the
// authority for publication, subscription, and receive-order routing.
class ProcessFederationService final {
 public:
  ProcessFederationService(
      EmbeddedFederationRegistry& registry,
      FederationDefinition federationDefinition,
      ProcessFederationServiceOptions options = {});

  [[nodiscard]] ProcessTransportServiceDispatcher::Handler handlerFor(
      ProcessTransportSession& session);

  // The transport owner calls detach after the session's serving loop exits.
  // Detaching does not resign a federate; lifecycle operations will be bound
  // to the registry in a later slice.
  void detach(ProcessTransportSession& session) noexcept;

 private:
  struct SessionState final {
    std::optional<std::wstring> federationName;
    std::uint64_t federateId = 0U;
    std::deque<ProcessFederationInteractionEvent> interactionEvents;
    std::deque<ProcessFederationAttributeUpdateEvent> attributeUpdateEvents;
    std::deque<ProcessFederationObjectInstanceDiscoveryEvent>
        objectInstanceDiscoveryEvents;
    std::deque<ProcessFederationObjectInstanceRemovalEvent>
        objectInstanceRemovalEvents;
    std::deque<ProcessFederationObjectInstanceScopeChangeEvent>
        objectInstanceScopeChangeEvents;
    std::deque<ProcessFederationAttributeRelevanceAdvisoryEvent>
        attributeRelevanceAdvisoryEvents;
  };

  [[nodiscard]] TransportServiceMessage handle(
      ProcessTransportSession& session,
      TransportServiceMessage const& request);
  [[nodiscard]] TransportServiceMessage handleCreate(
      TransportServiceMessage const& request);
  [[nodiscard]] TransportServiceMessage handleJoin(
      ProcessTransportSession& session,
      TransportServiceMessage const& request);
  [[nodiscard]] TransportServiceMessage handleResign(
      ProcessTransportSession& session,
      TransportServiceMessage const& request);
  [[nodiscard]] TransportServiceMessage handleSendInteraction(
      ProcessTransportSession& session,
      TransportServiceMessage const& request);
  [[nodiscard]] TransportServiceMessage handleSendDirectedInteraction(
      ProcessTransportSession& session,
      TransportServiceMessage const& request);
  [[nodiscard]] TransportServiceMessage handleRetract(
      ProcessTransportSession& session,
      TransportServiceMessage const& request);
  [[nodiscard]] TransportServiceMessage handleUpdateAttributeValues(
      ProcessTransportSession& session,
      TransportServiceMessage const& request);
  [[nodiscard]] TransportServiceMessage handleReceiveInteraction(
      ProcessTransportSession& session,
      TransportServiceMessage const& request);
  [[nodiscard]] TransportServiceMessage handleReceiveAttributeUpdate(
      ProcessTransportSession& session,
      TransportServiceMessage const& request);
  [[nodiscard]] TransportServiceMessage handleReceiveObjectInstanceDiscovery(
      ProcessTransportSession& session,
      TransportServiceMessage const& request);
  [[nodiscard]] TransportServiceMessage handleGetInteractionClassHandle(
      ProcessTransportSession& session,
      TransportServiceMessage const& request);
  [[nodiscard]] TransportServiceMessage handleGetObjectClassHandle(
      ProcessTransportSession& session,
      TransportServiceMessage const& request);
  [[nodiscard]] TransportServiceMessage handleGetParameterHandle(
      ProcessTransportSession& session,
      TransportServiceMessage const& request);
  [[nodiscard]] TransportServiceMessage handleGetAttributeHandle(
      ProcessTransportSession& session,
      TransportServiceMessage const& request);
  [[nodiscard]] TransportServiceMessage handleInteractionClassDeclaration(
      ProcessTransportSession& session,
      TransportServiceMessage const& request);
  [[nodiscard]] TransportServiceMessage
  handleObjectClassDirectedInteractionDeclaration(
      ProcessTransportSession& session,
      TransportServiceMessage const& request);
  [[nodiscard]] TransportServiceMessage handleObjectClassAttributeDeclaration(
      ProcessTransportSession& session,
      TransportServiceMessage const& request);
  [[nodiscard]] TransportServiceMessage handleObjectClassAttributeSubscription(
      ProcessTransportSession& session,
      TransportServiceMessage const& request);
  [[nodiscard]] TransportServiceMessage
  handleObjectClassAttributeRegionalSubscription(
      ProcessTransportSession& session,
      TransportServiceMessage const& request);
  [[nodiscard]] TransportServiceMessage handleRegisterObjectInstance(
      ProcessTransportSession& session,
      TransportServiceMessage const& request);
  [[nodiscard]] TransportServiceMessage handleLocalDeleteObjectInstance(
      ProcessTransportSession& session,
      TransportServiceMessage const& request);
  [[nodiscard]] TransportServiceMessage handleDeleteObjectInstance(
      ProcessTransportSession& session,
      TransportServiceMessage const& request);
  [[nodiscard]] TransportServiceMessage handleReserveObjectInstanceName(
      ProcessTransportSession& session,
      TransportServiceMessage const& request);
  [[nodiscard]] TransportServiceMessage handleGetDimensionHandle(
      ProcessTransportSession& session,
      TransportServiceMessage const& request);
  [[nodiscard]] TransportServiceMessage handleGetDimensionUpperBound(
      ProcessTransportSession& session,
      TransportServiceMessage const& request);
  [[nodiscard]] TransportServiceMessage handleCreateRegion(
      ProcessTransportSession& session,
      TransportServiceMessage const& request);
  [[nodiscard]] TransportServiceMessage handleCommitRegionModifications(
      ProcessTransportSession& session,
      TransportServiceMessage const& request);
  [[nodiscard]] TransportServiceMessage handleDeleteRegion(
      ProcessTransportSession& session,
      TransportServiceMessage const& request);
  [[nodiscard]] TransportServiceMessage handleGetDimensionHandleSet(
      ProcessTransportSession& session,
      TransportServiceMessage const& request);
  [[nodiscard]] TransportServiceMessage handleGetRangeBounds(
      ProcessTransportSession& session,
      TransportServiceMessage const& request);
  [[nodiscard]] TransportServiceMessage handleSetRangeBounds(
      ProcessTransportSession& session,
      TransportServiceMessage const& request);
  [[nodiscard]] TransportServiceMessage handleGetAttributeScopeAdvisorySwitch(
      ProcessTransportSession& session,
      TransportServiceMessage const& request);
  [[nodiscard]] TransportServiceMessage handleSetAttributeScopeAdvisorySwitch(
      ProcessTransportSession& session,
      TransportServiceMessage const& request);
  [[nodiscard]] TransportServiceMessage
  handleGetAttributeRelevanceAdvisorySwitch(
      ProcessTransportSession& session,
      TransportServiceMessage const& request);
  [[nodiscard]] TransportServiceMessage
  handleSetAttributeRelevanceAdvisorySwitch(
      ProcessTransportSession& session,
      TransportServiceMessage const& request);
  [[nodiscard]] TransportServiceMessage
  handleRegisterObjectInstanceWithRegions(
      ProcessTransportSession& session,
      TransportServiceMessage const& request);
  [[nodiscard]] TransportServiceMessage
  handleObjectInstanceRegionAssociation(
      ProcessTransportSession& session,
      TransportServiceMessage const& request);

  // Project registry-reserved discoveries onto the receiving process.  The
  // public callback bridge is the only standards-facing consumer; the service
  // uses this private helper for both registration and later subscription.
  [[nodiscard]] bool enqueueObjectInstanceDiscoveries(
      std::wstring const& federationName,
      std::vector<ObjectInstanceDiscoveryRecipient> discoveries);

  [[nodiscard]] bool enqueueObjectInstanceRemovals(
      std::wstring const& federationName,
      std::vector<ObjectInstanceRemovalRecipient> removals,
      std::vector<std::uint8_t> userSuppliedTag,
      std::optional<ProcessFederationLogicalTime> timestamp = std::nullopt,
      std::uint64_t retractionMessageId = 0U);

  [[nodiscard]] bool enqueueObjectInstanceScopeChanges(
      std::wstring const& federationName,
      std::vector<ObjectInstanceScopeChangeRecipient> changes);

  [[nodiscard]] bool enqueueAttributeRelevanceAdvisories(
      std::wstring const& federationName,
      std::vector<AttributeRelevanceAdvisoryRecipient> advisories);

  [[nodiscard]] TransportServiceMessage rejected(
      TransportServiceMessage const& request) const;
  [[nodiscard]] TransportServiceMessage invalid(
      TransportServiceMessage const& request) const;
  [[nodiscard]] TransportServiceMessage internalError(
      TransportServiceMessage const& request) const;

  EmbeddedFederationRegistry& registry_;
  std::shared_ptr<FederationDefinition const> federationDefinition_;
  ProcessFederationServiceOptions options_;
  // Pushed directed events may be buffered in a receiver process client even
  // after they leave the service's queue. Retain the recipient sessions long
  // enough to send a matching retraction notification.
  std::map<std::uint64_t, std::vector<ProcessTransportSession*>>
      pendingPushedRetractionRecipients_;
  std::map<std::uint64_t, std::uint64_t> processTsoMessageProducers_;
  mutable std::mutex mutex_;
  std::map<ProcessTransportSession*, SessionState> sessions_;
  std::map<std::uint64_t, ProcessTransportSession*> sessionsByFederateId_;
};

}  // namespace umbra::detail
