#pragma once

#include "internal/federation/federation_registry.hpp"
#include "internal/federation/process_transport_session.hpp"

#include <cstdint>
#include <deque>
#include <functional>
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
  // New process clients append the caller's standards-facing FOM/MIM/time
  // inputs after the original federation name.  Keeping the fields after
  // the original member preserves source compatibility for private callers
  // that still aggregate-initialize only the name.
  std::vector<std::wstring> fomModules;
  std::optional<std::wstring> mimModule;
  std::wstring logicalTimeImplementationName;
  // Derived by the decoder when the optional wire suffix is present.  An old
  // one-field request therefore continues to select the server's configured
  // base definition, while a new request cannot be silently ignored when
  // the server has no standards-derived preparation callback.
  bool hasFomInputs = false;
};

struct ProcessFederationJoinRequest final {
  std::wstring federationName;
  std::wstring federateType;
  std::optional<std::wstring> requestedFederateName;
  // Additional FOM designators are kept after the original fields so private
  // aggregate callers that only name an execution/type/name remain source
  // compatible. The service validates and composes them before membership
  // becomes visible.
  std::vector<std::wstring> additionalFomModules;
};

struct ProcessFederationResignRequest final {
  std::wstring federationName;
  std::uint64_t federateId = 0U;
  rti1516_2025::ResignAction resignAction = rti1516_2025::NO_ACTION;
};

// Change Interaction Order Type carries the publisher-scoped declaration
// across the process seam. The registry remains authoritative for publication
// validation and prospective send behavior; this payload contains only the
// already-decoded official values.
struct ProcessFederationChangeInteractionOrderTypeRequest final {
  std::wstring federationName;
  std::uint64_t federateId = 0U;
  std::uint64_t interactionClassHandle = 0U;
  rti1516_2025::OrderType orderType = rti1516_2025::RECEIVE;
};

// Change Attribute Order Type carries the selected object instance and the
// complete owned attribute set across the process seam. The registry remains
// authoritative for known-instance and ownership validation.
struct ProcessFederationChangeAttributeOrderTypeRequest final {
  std::wstring federationName;
  std::uint64_t federateId = 0U;
  std::uint64_t objectInstanceHandle = 0U;
  std::vector<std::uint64_t> attributeHandles;
  rti1516_2025::OrderType orderType = rti1516_2025::RECEIVE;
};

// Change Default Attribute Order Type carries the class designator and the
// complete attribute set across the process seam. The registry remains
// authoritative for class/attribute validation and prospective defaults.
struct ProcessFederationChangeDefaultAttributeOrderTypeRequest final {
  std::wstring federationName;
  std::uint64_t federateId = 0U;
  std::uint64_t objectClassHandle = 0U;
  std::vector<std::uint64_t> attributeHandles;
  rti1516_2025::OrderType orderType = rti1516_2025::RECEIVE;
};

// Change Default Attribute Transportation Type carries the class designator,
// attribute set, and execution-scoped transportation handle. The registry
// remains authoritative for catalog validation and prospective defaults.
struct ProcessFederationChangeDefaultAttributeTransportationTypeRequest final {
  std::wstring federationName;
  std::uint64_t federateId = 0U;
  std::uint64_t objectClassHandle = 0U;
  std::vector<std::uint64_t> attributeHandles;
  std::uint64_t transportationTypeHandle = 0U;
};

// Instance transportation-type control crosses as a typed request. The
// registry owns the pending request identity and commits the selected type at
// the confirmation callback boundary.
struct ProcessFederationRequestAttributeTransportationTypeChangeRequest final {
  std::wstring federationName;
  std::uint64_t requestingFederateId = 0U;
  std::uint64_t objectInstanceHandle = 0U;
  std::vector<std::uint64_t> attributeHandles;
  std::uint64_t transportationTypeHandle = 0U;
};

struct ProcessFederationQueryAttributeTransportationTypeRequest final {
  std::wstring federationName;
  std::uint64_t requestingFederateId = 0U;
  std::uint64_t objectInstanceHandle = 0U;
  std::uint64_t attributeHandle = 0U;
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
  // Present only for Send Interaction With Regions.  Keeping the set
  // optional preserves the ordinary Send Interaction payload and lets the
  // process service distinguish an explicit empty set from omitted regions.
  std::optional<std::set<std::uint64_t>> sentRegionHandles;
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

// Object-instance Request Attribute Value Update crosses the private process
// seam after the public adapter has validated the official handles. The
// provider callback is emitted as a typed receive-fence event below; class
// and regional forms remain separate follow-on slices.
struct ProcessFederationRequestAttributeValueUpdateRequest final {
  std::wstring federationName;
  std::uint64_t requestingFederateId = 0U;
  std::uint64_t objectInstanceHandle = 0U;
  std::vector<std::uint64_t> requestedAttributeHandles;
  std::vector<std::uint8_t> userSuppliedTag;
};

// Object-class Request Attribute Value Update crosses the private process
// seam with its class designator intact.  The service expands the request
// against the current object hierarchy and emits the same provider callback
// event used by the object-instance form for each eligible instance/provider
// group.
struct ProcessFederationRequestAttributeValueUpdateClassRequest final {
  std::wstring federationName;
  std::uint64_t requestingFederateId = 0U;
  std::uint64_t objectClassHandle = 0U;
  std::vector<std::uint64_t> requestedAttributeHandles;
  std::vector<std::uint8_t> userSuppliedTag;
};

// Regional object-class Request Attribute Value Update carries the same class
// expansion as the ordinary class form plus the requester's immutable
// attribute-to-region designators. The process service persists each provider
// delivery through the registry's regional ledger; the callback event itself
// remains the ordinary Provide Attribute Value Update shape.
struct ProcessFederationRequestAttributeValueUpdateClassWithRegionsRequest final {
  std::wstring federationName;
  std::uint64_t requestingFederateId = 0U;
  std::uint64_t objectClassHandle = 0U;
  std::vector<std::uint64_t> requestedAttributeHandles;
  std::map<std::uint64_t, std::set<std::uint64_t>> requestRegionsByAttribute;
  std::vector<std::uint8_t> userSuppliedTag;
};

// Read-only ownership checks cross the process seam with the same known
// object/class boundaries as the embedded implementation. The typed status
// in the response lets the public adapter preserve the official exception
// distinctions instead of collapsing every remote failure into rejection.
struct ProcessFederationAttributeOwnershipCheckRequest final {
  std::wstring federationName;
  std::uint64_t requestingFederateId = 0U;
  std::uint64_t objectInstanceHandle = 0U;
  std::uint64_t attributeHandle = 0U;
};

// Query Attribute Ownership keeps the complete attribute set and requester
// identity across the private process seam. The service returns a typed
// planning status and queues one revalidated report event per ownership kind;
// it never collapses grouped callbacks into a boolean ownership answer.
struct ProcessFederationAttributeOwnershipQueryRequest final {
  std::wstring federationName;
  std::uint64_t requestingFederateId = 0U;
  std::uint64_t objectInstanceHandle = 0U;
  std::vector<std::uint64_t> requestedAttributeHandles;
};

// Attribute Ownership Acquisition If Available crosses the private process
// seam with the same requester/object/attribute/tag values accepted by the
// public C++ binding.  The endpoint returns only a typed admission result;
// ownership delivery is projected through the receive-order event below.
struct ProcessFederationAttributeOwnershipAcquisitionIfAvailableRequest final {
  std::wstring federationName;
  std::uint64_t requestingFederateId = 0U;
  std::uint64_t objectInstanceHandle = 0U;
  std::vector<std::uint64_t> desiredAttributeHandles;
  std::vector<std::uint8_t> userSuppliedTag;
};

// Regular Attribute Ownership Acquisition carries the complete request
// across the process seam.  The registry remains authoritative for pending
// state and callback ordering; this private payload only transports the
// already-decoded official values.
struct ProcessFederationAttributeOwnershipAcquisitionRequest final {
  std::wstring federationName;
  std::uint64_t requestingFederateId = 0U;
  std::uint64_t objectInstanceHandle = 0U;
  std::vector<std::uint64_t> desiredAttributeHandles;
  std::vector<std::uint8_t> userSuppliedTag;
};

// Attribute Ownership Release Denied carries the current owner's complete
// denial request across the private process seam.  The registry decides which
// pending acquisitions terminate and the service projects the resulting
// Attribute Ownership Unavailable callbacks through the receive fence.
struct ProcessFederationAttributeOwnershipReleaseDeniedRequest final {
  std::wstring federationName;
  std::uint64_t owningFederateId = 0U;
  std::uint64_t objectInstanceHandle = 0U;
  std::vector<std::uint64_t> attributeHandles;
  std::vector<std::uint8_t> userSuppliedTag;
};

// Cancel Attribute Ownership Acquisition carries only the official object
// and attribute designators across the private process seam.  The registry
// allocates the cancellation identity and retains the callback reservation;
// the process service projects the later confirmation through the existing
// ownership receive fence.
struct ProcessFederationAttributeOwnershipAcquisitionCancellationRequest final {
  std::wstring federationName;
  std::uint64_t requestingFederateId = 0U;
  std::uint64_t objectInstanceHandle = 0U;
  std::vector<std::uint64_t> attributeHandles;
};

// Cancel Negotiated Attribute Ownership Divestiture removes a pending
// negotiated offer owned by the joined federate.  Any ordinary acquisition
// release work restored by the registry is projected through the existing
// ownership receive fence.
struct ProcessFederationCancelNegotiatedAttributeOwnershipDivestitureRequest
    final {
  std::wstring federationName;
  std::uint64_t divestingFederateId = 0U;
  std::uint64_t objectInstanceHandle = 0U;
  std::vector<std::uint64_t> attributeHandles;
};

// Negotiated Attribute Ownership Divestiture records a pending owner-side
// offer.  The process service returns a typed admission result and projects
// any selected Request Divestiture Confirmation/assumption callbacks through
// the existing ownership receive fence.
struct ProcessFederationNegotiatedAttributeOwnershipDivestitureRequest final {
  std::wstring federationName;
  std::uint64_t divestingFederateId = 0U;
  std::uint64_t objectInstanceHandle = 0U;
  std::vector<std::uint64_t> attributeHandles;
  std::vector<std::uint8_t> userSuppliedTag;
};

// Confirm Divestiture commits the negotiated transfer at the process service.
// Its post-confirmation acquisition notifications remain callback-gated and
// are projected through the ownership receive fence below.
struct ProcessFederationConfirmDivestitureRequest final {
  std::wstring federationName;
  std::uint64_t divestingFederateId = 0U;
  std::uint64_t objectInstanceHandle = 0U;
  std::vector<std::uint64_t> attributeHandles;
  std::vector<std::uint8_t> userSuppliedTag;
};

struct ProcessFederationReceiveInteractionRequest final {
  std::wstring federationName;
  std::uint64_t receivingFederateId = 0U;
};

// A process receiver acknowledges the callback boundary for one
// timestamp-ordered payload.  The queue's private sequence/timestamp tuple
// never crosses the process seam; the federation-owned message id is enough
// for the registry to resolve the in-transit entry.
struct ProcessFederationAcknowledgeTsoDeliveryRequest final {
  std::wstring federationName;
  std::uint64_t receivingFederateId = 0U;
  std::uint64_t messageId = 0U;
};

// Query Logical Time reports the joined federate's current value through the
// same official logical-time encoding used by the process grant scheduler;
// it never invents a second public time representation.
struct ProcessFederationQueryLogicalTimeRequest final {
  std::wstring federationName;
  std::uint64_t federateId = 0U;
};

// A logical-time interval crosses the private process seam with the same
// official implementation-name plus VariableLengthData encoding used by the
// C++ binding.  The process service decodes it through the selected official
// LogicalTimeFactory before touching FederateTimeState.
struct ProcessFederationLogicalTimeInterval final {
  std::wstring implementationName;
  std::vector<std::uint8_t> encoding;
};

struct ProcessFederationEnableTimeRegulationRequest final {
  std::wstring federationName;
  std::uint64_t federateId = 0U;
  ProcessFederationLogicalTimeInterval lookahead;
};

// Modify Lookahead uses the same official implementation-name plus encoded
// interval representation as Enable Time Regulation, but remains a distinct
// request type so the process protocol cannot accidentally apply the wrong
// temporal operation at a call site.
struct ProcessFederationModifyLookaheadRequest final {
  std::wstring federationName;
  std::uint64_t federateId = 0U;
  ProcessFederationLogicalTimeInterval lookahead;
};

// Enable Time Constrained has no public arguments.  Keep the joined-federate
// identity explicit across the private process seam so the service can apply
// the same membership and session gate as the regulation operation.
struct ProcessFederationEnableTimeConstrainedRequest final {
  std::wstring federationName;
  std::uint64_t federateId = 0U;
};

// Time Advance Request crosses the private process seam as the official
// logical-time implementation name plus its VariableLengthData encoding. The
// endpoint owns the request admission; an already-eligible request may carry
// its grant in the response, while a deferred grant is delivered as an
// unsolicited time_advance_grant event.
struct ProcessFederationTimeAdvanceRequest final {
  std::wstring federationName;
  std::uint64_t federateId = 0U;
  ProcessFederationLogicalTime requestedTime;
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

// Federate identity lookups carry the caller's joined-federate identity so
// the process service can apply the same membership gate as every other
// support lookup.  Get Federate Handle resolves active names; Get Federate
// Name resolves the execution-scoped designator ledger, which intentionally
// retains names after normal resignation.
struct ProcessFederationGetFederateHandleRequest final {
  std::wstring federationName;
  std::uint64_t federateId = 0U;
  std::wstring federateName;
};

struct ProcessFederationGetFederateNameRequest final {
  std::wstring federationName;
  std::uint64_t federateId = 0U;
  std::uint64_t targetFederateId = 0U;
};

// Handle normalization carries the joined-federate identity and the
// execution-issued handle explicitly. The operation selects the official
// handle category; keeping the payload shared avoids four subtly divergent
// wire formats while preserving distinct transport operation identities.
struct ProcessFederationNormalizeHandleRequest final {
  std::wstring federationName;
  std::uint64_t federateId = 0U;
  std::uint64_t handle = 0U;
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

// Reverse FOM lookups carry the official handle direction explicitly across
// the private seam.  The service returns the canonical name from the
// federation-owned catalog; it never reconstructs a name from a client cache.
struct ProcessFederationGetObjectClassNameRequest final {
  std::wstring federationName;
  std::uint64_t federateId = 0U;
  std::uint64_t objectClassHandle = 0U;
};

struct ProcessFederationGetInteractionClassNameRequest final {
  std::wstring federationName;
  std::uint64_t federateId = 0U;
  std::uint64_t interactionClassHandle = 0U;
};

struct ProcessFederationGetAttributeNameRequest final {
  std::wstring federationName;
  std::uint64_t federateId = 0U;
  std::uint64_t objectClassHandle = 0U;
  std::uint64_t attributeHandle = 0U;
};

struct ProcessFederationGetParameterNameRequest final {
  std::wstring federationName;
  std::uint64_t federateId = 0U;
  std::uint64_t interactionClassHandle = 0U;
  std::uint64_t parameterHandle = 0U;
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

// Known object-instance lookups retain the official name/handle direction at
// the private process boundary.  The service resolves against the joined
// federate's known-instance ledger; it must not consult a process-local cache
// that can diverge after discovery, deletion, or resignation.
struct ProcessFederationGetObjectInstanceHandleRequest final {
  std::wstring federationName;
  std::uint64_t federateId = 0U;
  std::wstring objectInstanceName;
};

struct ProcessFederationGetObjectInstanceNameRequest final {
  std::wstring federationName;
  std::uint64_t federateId = 0U;
  std::uint64_t objectInstanceHandle = 0U;
};

// Known-object class lookup is resolved against the joined federate's
// discovery ledger rather than the global FOM catalog.  Keep the request
// distinct from the object-instance name lookup even though both carry an
// object handle, so the private operation cannot silently cross semantic
// boundaries.
struct ProcessFederationGetKnownObjectClassHandleRequest final {
  std::wstring federationName;
  std::uint64_t federateId = 0U;
  std::uint64_t objectInstanceHandle = 0U;
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

// Regional interaction subscriptions cross the private process seam as one
// normalized interaction-class handle plus the caller-owned region set.  The
// operation identity selects subscribe versus unsubscribe; `active` is used
// only by the subscribe form and preserves the official passive overload.
struct ProcessFederationInteractionClassRegionalSubscriptionRequest final {
  std::wstring federationName;
  std::uint64_t federateId = 0U;
  std::uint64_t interactionClassHandle = 0U;
  std::set<std::uint64_t> regionHandles;
  bool active = true;
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

// Available-dimension queries carry a class handle and share one private
// request shape for object and interaction class operations.  The operation
// itself selects which federation-owned hierarchy is traversed.
struct ProcessFederationClassHandleRequest final {
  std::wstring federationName;
  std::uint64_t federateId = 0U;
  std::uint64_t classHandle = 0U;
};

struct ProcessFederationGetTransportationTypeHandleRequest final {
  std::wstring federationName;
  std::uint64_t federateId = 0U;
  std::string transportationTypeName;
};

struct ProcessFederationTransportationTypeRequest final {
  std::wstring federationName;
  std::uint64_t federateId = 0U;
  std::uint64_t transportationTypeHandle = 0U;
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
  // The selected logical-time implementation is part of the public Join
  // result even though the process transport keeps the FOM catalog private.
  // Keeping this as a trailing member preserves aggregate initialization for
  // existing private callers while allowing a client to construct the
  // official LogicalTimeFactory without consulting its process-local registry.
  std::wstring logicalTimeImplementationName;
};

struct ProcessFederationQueryLogicalTimeResult final {
  ProcessFederationLogicalTime time;
};

enum class ProcessFederationLookaheadStatus : std::uint8_t {
  applied = 0U,
  not_enabled = 1U,
  inactive = 2U,
};

struct ProcessFederationQueryLookaheadResult final {
  ProcessFederationLookaheadStatus status =
      ProcessFederationLookaheadStatus::inactive;
  std::optional<ProcessFederationLogicalTimeInterval> lookahead;
};

enum class ProcessFederationModifyLookaheadStatus : std::uint8_t {
  applied = 0U,
  time_advance_pending = 1U,
  not_enabled = 2U,
  inactive = 3U,
  invalid_lookahead = 4U,
};

struct ProcessFederationModifyLookaheadResult final {
  ProcessFederationModifyLookaheadStatus status =
      ProcessFederationModifyLookaheadStatus::inactive;
};

enum class ProcessFederationTimeBoundStatus : std::uint8_t {
  available = 0U,
  undefined = 1U,
  requesting_federate_not_registered = 2U,
  factory_unavailable = 3U,
  inconsistent_temporal_state = 4U,
};

// Query GALT and Query LITS share one private response so the process service
// computes one coherent federation snapshot for both values.  The optional
// members remain independent: LITS can be defined from queued/in-transit TSO
// state even when GALT is undefined.
struct ProcessFederationQueryTimeBoundsResult final {
  ProcessFederationTimeBoundStatus status =
      ProcessFederationTimeBoundStatus::inconsistent_temporal_state;
  std::optional<ProcessFederationLogicalTime> galt;
  std::optional<ProcessFederationLogicalTime> lits;
};

enum class ProcessFederationTimeEnableStatus : std::uint8_t {
  applied = 0U,
  time_advance_pending = 1U,
  request_pending = 2U,
  already_enabled = 3U,
  invalid_lookahead = 4U,
  inactive = 5U,
  generation_exhausted = 6U,
};

struct ProcessFederationEnableTimeRegulationResult final {
  ProcessFederationTimeEnableStatus status =
      ProcessFederationTimeEnableStatus::inactive;
  // Present only when the request was applied.  This is the value delivered
  // through FederateAmbassador::timeRegulationEnabled on the client side.
  std::optional<ProcessFederationLogicalTime> enabledTime;
};

struct ProcessFederationEnableTimeConstrainedResult final {
  ProcessFederationTimeEnableStatus status =
      ProcessFederationTimeEnableStatus::inactive;
  // Present only when the request was accepted.  The client callback bridge
  // delivers this value through FederateAmbassador::timeConstrainedEnabled.
  std::optional<ProcessFederationLogicalTime> enabledTime;
};

// Disable Time Regulation and Disable Time Constrained share one private
// status shape. The public adapter keeps the two operations separate so it can
// translate the precise official exception for each role, while the process
// transport does not duplicate an otherwise identical payload.
enum class ProcessFederationTimeDisableStatus : std::uint8_t {
  applied = 0U,
  not_enabled = 1U,
  inactive = 2U,
};

struct ProcessFederationTimeDisableResult final {
  ProcessFederationTimeDisableStatus status =
      ProcessFederationTimeDisableStatus::inactive;
};

enum class ProcessFederationTimeAdvanceStatus : std::uint8_t {
  applied = 0U,
  logical_time_already_passed = 1U,
  time_advance_pending = 2U,
  time_regulation_pending = 3U,
  time_constrained_pending = 4U,
  invalid_logical_time = 5U,
  inactive = 6U,
  generation_exhausted = 7U,
};

struct ProcessFederationTimeAdvanceResult final {
  ProcessFederationTimeAdvanceStatus status =
      ProcessFederationTimeAdvanceStatus::inactive;
  // Present when the endpoint completed the accepted request before returning
  // the response. Deferred accepted requests leave this absent and receive the
  // same value through an unsolicited time_advance_grant event.
  std::optional<ProcessFederationLogicalTime> grantedTime;
  // Present only on a Flush Queue Grant event.  FQR carries both the actual
  // callback time and the optimistic logical-time floor; ordinary temporal
  // responses/events leave this absent.
  std::optional<ProcessFederationLogicalTime> optimisticTime;
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
  time_regulation_not_enabled = 4U,
};

struct ProcessFederationRetractResult final {
  ProcessFederationRetractStatus status =
      ProcessFederationRetractStatus::applied;
};

enum class ProcessFederationTsoDeliveryAcknowledgementStatus : std::uint8_t {
  applied = 0U,
  already_completed = 1U,
  not_in_transit = 2U,
};

struct ProcessFederationTsoDeliveryAcknowledgementResult final {
  ProcessFederationTsoDeliveryAcknowledgementStatus status =
      ProcessFederationTsoDeliveryAcknowledgementStatus::not_in_transit;
};

struct ProcessFederationRequestRetractionEvent final {
  std::uint64_t messageId = 0U;
};

struct ProcessFederationHandleResult final {
  std::uint64_t handle = 0U;
};

struct ProcessFederationStringResult final {
  std::wstring value;
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

struct ProcessFederationInteractionOrderTypeChangeResult final {
  InteractionOrderTypeChangeStatus status =
      InteractionOrderTypeChangeStatus::applied;
};

struct ProcessFederationAttributeOrderTypeChangeResult final {
  AttributeOrderTypeChangeStatus status =
      AttributeOrderTypeChangeStatus::applied;
};

struct ProcessFederationAttributeOrderTypeDefaultResult final {
  AttributeOrderTypeDefaultStatus status =
      AttributeOrderTypeDefaultStatus::applied;
};

struct ProcessFederationAttributeTransportationTypeDefaultResult final {
  AttributeTransportationTypeDefaultStatus status =
      AttributeTransportationTypeDefaultStatus::applied;
};

struct ProcessFederationAttributeTransportationTypeChangeResult final {
  AttributeTransportationTypeChangeStatus status =
      AttributeTransportationTypeChangeStatus::applied;
};

struct ProcessFederationAttributeTransportationTypeQueryResult final {
  AttributeTransportationTypeQueryStatus status =
      AttributeTransportationTypeQueryStatus::applied;
};

struct ProcessFederationAttributeOwnershipCheckResult final {
  AttributeOwnershipCheckStatus status =
      AttributeOwnershipCheckStatus::applied;
  bool ownedByRequestingFederate = false;
};

struct ProcessFederationAttributeOwnershipQueryResult final {
  AttributeOwnershipQueryStatus status =
      AttributeOwnershipQueryStatus::applied;
  std::uint32_t recipientCount = 0U;
};

struct ProcessFederationAttributeOwnershipAcquisitionIfAvailableResult final {
  AttributeOwnershipAcquisitionIfAvailableStatus status =
      AttributeOwnershipAcquisitionIfAvailableStatus::applied;
  std::uint32_t recipientCount = 0U;
};

struct ProcessFederationAttributeOwnershipAcquisitionResult final {
  AttributeOwnershipAcquisitionStatus status =
      AttributeOwnershipAcquisitionStatus::applied;
  std::uint32_t recipientCount = 0U;
};

struct ProcessFederationAttributeOwnershipReleaseDeniedResult final {
  AttributeOwnershipReleaseDeniedStatus status =
      AttributeOwnershipReleaseDeniedStatus::applied;
  std::uint32_t recipientCount = 0U;
};

struct ProcessFederationAttributeOwnershipAcquisitionCancellationResult final {
  AttributeOwnershipAcquisitionCancellationStatus status =
      AttributeOwnershipAcquisitionCancellationStatus::applied;
  std::uint32_t recipientCount = 0U;
};

struct ProcessFederationCancelNegotiatedAttributeOwnershipDivestitureResult
    final {
  CancelNegotiatedAttributeOwnershipDivestitureStatus status =
      CancelNegotiatedAttributeOwnershipDivestitureStatus::applied;
  std::uint32_t recipientCount = 0U;
};

struct ProcessFederationNegotiatedAttributeOwnershipDivestitureResult final {
  NegotiatedAttributeOwnershipDivestitureStatus status =
      NegotiatedAttributeOwnershipDivestitureStatus::applied;
  std::uint32_t recipientCount = 0U;
};

struct ProcessFederationConfirmDivestitureResult final {
  ConfirmDivestitureStatus status = ConfirmDivestitureStatus::applied;
  std::uint32_t recipientCount = 0U;
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

struct ProcessFederationAvailableDimensionsResult final {
  bool found = false;
  std::vector<std::uint64_t> dimensionHandles;
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
  // Present for a regional passel. A null value plus defaultRegionUsed=true
  // represents the official default region and is projected as an engaged,
  // empty RegionHandleSet by the callback bridge.
  std::optional<std::set<std::uint64_t>> sentRegionHandles;
  bool defaultRegionUsed = false;
  // Present when the process service has crossed the timestamped callback
  // boundary. Immediate directed events intentionally leave these absent so
  // the bridge retains the legacy RECEIVE/RECEIVE classification; queued TSO
  // directed events carry the exact order pair required by the official
  // FederateAmbassador overload.
  std::optional<rti1516_2025::OrderType> sentOrderType;
  std::optional<rti1516_2025::OrderType> receivedOrderType;
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
  // Present for a timestamp-ordered update that crossed the callback boundary
  // under an execution-owned message-retraction identity. Appended for
  // compatibility with existing ordinary/timestamp-preservation events.
  std::optional<std::uint64_t> retractionMessageId;
};

// One accepted object-instance Request Attribute Value Update provider
// callback. The process service commits the registry's one-shot request
// reservation immediately before crossing the socket boundary, matching the
// existing process discovery projection; the public callback bridge then
// invokes the official Provide Attribute Value Update callback.
struct ProcessFederationAttributeValueUpdateRequestEvent final {
  std::uint64_t requestingFederateId = 0U;
  std::uint64_t providingFederateId = 0U;
  std::uint64_t objectInstanceHandle = 0U;
  std::set<std::uint64_t> requestedAttributeHandles;
  std::vector<std::uint8_t> userSuppliedTag;
};

// One Query Attribute Ownership result crossing the process receive fence.
// requestId remains attached until the service dequeues the event so the
// registry can revalidate object lifetime and ownership before callback entry.
struct ProcessFederationAttributeOwnershipQueryEvent final {
  std::uint64_t requestId = 0U;
  std::uint64_t receivingFederateId = 0U;
  std::uint64_t objectInstanceHandle = 0U;
  AttributeOwnershipQueryReportKind reportKind =
      AttributeOwnershipQueryReportKind::unowned;
  std::uint64_t owningFederateId = 0U;
  std::set<std::uint64_t> attributeHandles;
};

// One accepted If Available request crosses the process receive fence with
// the callback-time secured/unavailable partition.  The service keeps the
// request identity until dequeue so the registry can atomically commit or
// suppress the pending Willing-to-Acquire reservation at that boundary.
struct ProcessFederationAttributeOwnershipAcquisitionIfAvailableEvent final {
  std::uint64_t requestId = 0U;
  std::uint64_t receivingFederateId = 0U;
  std::uint64_t objectInstanceHandle = 0U;
  std::set<std::uint64_t> securedAttributeHandles;
  std::set<std::uint64_t> unavailableAttributeHandles;
  std::vector<std::uint8_t> userSuppliedTag;
};

enum class ProcessFederationAttributeOwnershipAcquisitionEventKind :
    std::uint8_t {
  acquisition_notification = 0U,
  request_release = 1U,
  cancellation_confirmation = 2U,
  request_divestiture_confirmation = 3U,
  // Request Attribute Ownership Assumption uses the same private ownership
  // event frame. Its request identity is zero because the official callback
  // has no acquisition request id.
  ownership_assumption = 4U,
  // Confirm Divestiture transfers ownership at the service boundary, while
  // this event commits the paired Acquisition Notification immediately before
  // it enters the acquiring federate's callback.
  confirm_divestiture_notification = 5U,
};

// One regular acquisition callback crossing the process receive fence.  For
// acquisition_notification the receiving federate is the requester; for
// request_release it is the current owner.  The service revalidates and
// commits the registry boundary before projecting the event to the client.
struct ProcessFederationAttributeOwnershipAcquisitionEvent final {
  ProcessFederationAttributeOwnershipAcquisitionEventKind kind =
      ProcessFederationAttributeOwnershipAcquisitionEventKind::
          acquisition_notification;
  std::uint64_t requestId = 0U;
  std::uint64_t requestingFederateId = 0U;
  std::uint64_t receivingFederateId = 0U;
  std::uint64_t objectInstanceHandle = 0U;
  std::set<std::uint64_t> attributeHandles;
  std::vector<std::uint8_t> userSuppliedTag;
  // Only meaningful for request-divestiture-confirmation work. Older event
  // kinds leave this false.
  bool candidateIsIfAvailable = false;
};

// A denied release terminates one or more pending regular acquisitions.  The
// resulting unavailable callback has no acquisition request identity in the
// official API, so it crosses as a distinct event and is revalidated by the
// service immediately before callback projection.
struct ProcessFederationAttributeOwnershipUnavailableEvent final {
  std::uint64_t receivingFederateId = 0U;
  std::uint64_t objectInstanceHandle = 0U;
  std::set<std::uint64_t> attributeHandles;
  std::vector<std::uint8_t> userSuppliedTag;
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

// Confirmation and report callbacks retain only the official values needed
// by the process callback bridge. The change request id is private bookkeeping
// and is consumed before the official confirmation callback is invoked.
struct ProcessFederationAttributeTransportationTypeChangeEvent final {
  std::uint64_t requestId = 0U;
  std::uint64_t receivingFederateId = 0U;
  std::uint64_t objectInstanceHandle = 0U;
  std::set<std::uint64_t> attributeHandles;
  std::string transportationName;
};

struct ProcessFederationAttributeTransportationTypeQueryEvent final {
  std::uint64_t receivingFederateId = 0U;
  std::uint64_t objectInstanceHandle = 0U;
  std::uint64_t attributeHandle = 0U;
  std::string transportationName;
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
  // Provider-side Request Attribute Value Update callbacks share the same
  // deterministic polling/push fence as the other process callback families.
  // Appended to preserve aggregate initialization of existing result users.
  std::optional<ProcessFederationAttributeValueUpdateRequestEvent>
      attributeValueUpdateRequestEvent;
  // Query Attribute Ownership results share that same fence. Appending this
  // slot preserves aggregate initialization of older private consumers.
  std::optional<ProcessFederationAttributeOwnershipQueryEvent>
      attributeOwnershipQueryEvent;
  // If Available acquisition callbacks share the receive-order fence with
  // ownership queries. Appending this slot preserves older aggregates.
  std::optional<
      ProcessFederationAttributeOwnershipAcquisitionIfAvailableEvent>
      attributeOwnershipAcquisitionIfAvailableEvent;
  // Regular ownership-acquisition callbacks share the same receive-order
  // fence. Appending this slot preserves older private result aggregates.
  std::optional<ProcessFederationAttributeOwnershipAcquisitionEvent>
      attributeOwnershipAcquisitionEvent;
  // Release-denied terminal callbacks share the receive-order fence while
  // retaining a distinct slot from acquisition notifications and requests.
  std::optional<ProcessFederationAttributeOwnershipUnavailableEvent>
      attributeOwnershipUnavailableEvent;
  std::optional<ProcessFederationAttributeTransportationTypeChangeEvent>
      attributeTransportationTypeChangeEvent;
  std::optional<ProcessFederationAttributeTransportationTypeQueryEvent>
      attributeTransportationTypeQueryEvent;
};

struct ProcessFederationRequestAttributeValueUpdateResult final {
  std::uint32_t recipientCount = 0U;
};

struct ProcessFederationUpdateAttributeValuesResult final {
  std::uint32_t recipientCount = 0U;
  // Zero retains the ordinary receive-order result. A nonzero value is the
  // execution-owned identity returned for an accepted timestamp-ordered
  // update so the public adapter can expose a real MessageRetractionHandle.
  std::uint64_t messageId = 0U;
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
  // Create Federation Execution owns the caller's FOM/MIM/time designators at
  // the public boundary.  The process service accepts them only through the
  // same standards-derived coordinator seam used by the embedded profile;
  // without it, an explicit suffix is rejected rather than ignored.
  using CreateFomPreparation = std::function<std::optional<FederationDefinition>(
      std::vector<std::wstring> const& fomModules,
      std::optional<std::wstring> const& mimModule,
      std::wstring const& logicalTimeImplementationName)>;
  CreateFomPreparation createFomPreparation;
  // The process service is intentionally independent of the public FOM
  // coordinator. A configured server may inject the same standards-derived
  // preparation callback used by the embedded profile; without it, a Join
  // carrying additional modules is rejected rather than silently ignored.
  using AdditionalFomPreparation = std::function<std::optional<FederationDefinition>(
      FederationDefinition const& existing,
      std::vector<std::wstring> const& additionalFomModules)>;
  AdditionalFomPreparation additionalFomPreparation;
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

[[nodiscard]] std::vector<std::uint8_t>
encodeProcessFederationChangeInteractionOrderTypeRequest(
    ProcessFederationChangeInteractionOrderTypeRequest const& request);
[[nodiscard]] ProcessFederationChangeInteractionOrderTypeRequest
decodeProcessFederationChangeInteractionOrderTypeRequest(
    std::span<std::uint8_t const> encoded);
[[nodiscard]] std::vector<std::uint8_t>
encodeProcessFederationChangeAttributeOrderTypeRequest(
    ProcessFederationChangeAttributeOrderTypeRequest const& request);
[[nodiscard]] ProcessFederationChangeAttributeOrderTypeRequest
decodeProcessFederationChangeAttributeOrderTypeRequest(
    std::span<std::uint8_t const> encoded);
[[nodiscard]] std::vector<std::uint8_t>
encodeProcessFederationChangeDefaultAttributeOrderTypeRequest(
    ProcessFederationChangeDefaultAttributeOrderTypeRequest const& request);
[[nodiscard]] ProcessFederationChangeDefaultAttributeOrderTypeRequest
decodeProcessFederationChangeDefaultAttributeOrderTypeRequest(
    std::span<std::uint8_t const> encoded);
[[nodiscard]] std::vector<std::uint8_t>
encodeProcessFederationChangeDefaultAttributeTransportationTypeRequest(
    ProcessFederationChangeDefaultAttributeTransportationTypeRequest const& request);
[[nodiscard]] ProcessFederationChangeDefaultAttributeTransportationTypeRequest
decodeProcessFederationChangeDefaultAttributeTransportationTypeRequest(
    std::span<std::uint8_t const> encoded);
[[nodiscard]] std::vector<std::uint8_t>
encodeProcessFederationRequestAttributeTransportationTypeChangeRequest(
    ProcessFederationRequestAttributeTransportationTypeChangeRequest const& request);
[[nodiscard]] ProcessFederationRequestAttributeTransportationTypeChangeRequest
decodeProcessFederationRequestAttributeTransportationTypeChangeRequest(
    std::span<std::uint8_t const> encoded);
[[nodiscard]] std::vector<std::uint8_t>
encodeProcessFederationQueryAttributeTransportationTypeRequest(
    ProcessFederationQueryAttributeTransportationTypeRequest const& request);
[[nodiscard]] ProcessFederationQueryAttributeTransportationTypeRequest
decodeProcessFederationQueryAttributeTransportationTypeRequest(
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
encodeProcessFederationAcknowledgeTsoDeliveryRequest(
    ProcessFederationAcknowledgeTsoDeliveryRequest const& request);
[[nodiscard]] ProcessFederationAcknowledgeTsoDeliveryRequest
decodeProcessFederationAcknowledgeTsoDeliveryRequest(
    std::span<std::uint8_t const> encoded);

[[nodiscard]] std::vector<std::uint8_t>
encodeProcessFederationQueryLogicalTimeRequest(
    ProcessFederationQueryLogicalTimeRequest const& request);
[[nodiscard]] ProcessFederationQueryLogicalTimeRequest
decodeProcessFederationQueryLogicalTimeRequest(
    std::span<std::uint8_t const> encoded);

[[nodiscard]] std::vector<std::uint8_t>
encodeProcessFederationEnableTimeRegulationRequest(
    ProcessFederationEnableTimeRegulationRequest const& request);
[[nodiscard]] ProcessFederationEnableTimeRegulationRequest
decodeProcessFederationEnableTimeRegulationRequest(
    std::span<std::uint8_t const> encoded);

[[nodiscard]] std::vector<std::uint8_t>
encodeProcessFederationModifyLookaheadRequest(
    ProcessFederationModifyLookaheadRequest const& request);
[[nodiscard]] ProcessFederationModifyLookaheadRequest
decodeProcessFederationModifyLookaheadRequest(
    std::span<std::uint8_t const> encoded);

[[nodiscard]] std::vector<std::uint8_t>
encodeProcessFederationEnableTimeConstrainedRequest(
    ProcessFederationEnableTimeConstrainedRequest const& request);
[[nodiscard]] ProcessFederationEnableTimeConstrainedRequest
decodeProcessFederationEnableTimeConstrainedRequest(
    std::span<std::uint8_t const> encoded);

[[nodiscard]] std::vector<std::uint8_t>
encodeProcessFederationTimeDisableResult(
    ProcessFederationTimeDisableResult const& result);
[[nodiscard]] ProcessFederationTimeDisableResult
decodeProcessFederationTimeDisableResult(
    std::span<std::uint8_t const> encoded);

[[nodiscard]] std::vector<std::uint8_t>
encodeProcessFederationTimeAdvanceRequest(
    ProcessFederationTimeAdvanceRequest const& request);
[[nodiscard]] ProcessFederationTimeAdvanceRequest
decodeProcessFederationTimeAdvanceRequest(
    std::span<std::uint8_t const> encoded);

[[nodiscard]] std::vector<std::uint8_t>
encodeProcessFederationUpdateAttributeValuesRequest(
    ProcessFederationUpdateAttributeValuesRequest const& request);
[[nodiscard]] ProcessFederationUpdateAttributeValuesRequest
decodeProcessFederationUpdateAttributeValuesRequest(
    std::span<std::uint8_t const> encoded);

[[nodiscard]] std::vector<std::uint8_t>
encodeProcessFederationRequestAttributeValueUpdateRequest(
    ProcessFederationRequestAttributeValueUpdateRequest const& request);
[[nodiscard]] ProcessFederationRequestAttributeValueUpdateRequest
decodeProcessFederationRequestAttributeValueUpdateRequest(
    std::span<std::uint8_t const> encoded);

[[nodiscard]] std::vector<std::uint8_t>
encodeProcessFederationRequestAttributeValueUpdateClassRequest(
    ProcessFederationRequestAttributeValueUpdateClassRequest const& request);
[[nodiscard]] ProcessFederationRequestAttributeValueUpdateClassRequest
decodeProcessFederationRequestAttributeValueUpdateClassRequest(
    std::span<std::uint8_t const> encoded);

[[nodiscard]] std::vector<std::uint8_t>
encodeProcessFederationRequestAttributeValueUpdateClassWithRegionsRequest(
    ProcessFederationRequestAttributeValueUpdateClassWithRegionsRequest const& request);
[[nodiscard]] ProcessFederationRequestAttributeValueUpdateClassWithRegionsRequest
decodeProcessFederationRequestAttributeValueUpdateClassWithRegionsRequest(
    std::span<std::uint8_t const> encoded);

[[nodiscard]] std::vector<std::uint8_t>
encodeProcessFederationAttributeOwnershipCheckRequest(
    ProcessFederationAttributeOwnershipCheckRequest const& request);
[[nodiscard]] ProcessFederationAttributeOwnershipCheckRequest
decodeProcessFederationAttributeOwnershipCheckRequest(
    std::span<std::uint8_t const> encoded);

[[nodiscard]] std::vector<std::uint8_t>
encodeProcessFederationAttributeOwnershipQueryRequest(
    ProcessFederationAttributeOwnershipQueryRequest const& request);
[[nodiscard]] ProcessFederationAttributeOwnershipQueryRequest
decodeProcessFederationAttributeOwnershipQueryRequest(
    std::span<std::uint8_t const> encoded);

[[nodiscard]] std::vector<std::uint8_t>
encodeProcessFederationAttributeOwnershipAcquisitionIfAvailableRequest(
    ProcessFederationAttributeOwnershipAcquisitionIfAvailableRequest const& request);
[[nodiscard]] ProcessFederationAttributeOwnershipAcquisitionIfAvailableRequest
decodeProcessFederationAttributeOwnershipAcquisitionIfAvailableRequest(
    std::span<std::uint8_t const> encoded);

[[nodiscard]] std::vector<std::uint8_t>
encodeProcessFederationAttributeOwnershipAcquisitionRequest(
    ProcessFederationAttributeOwnershipAcquisitionRequest const& request);
[[nodiscard]] ProcessFederationAttributeOwnershipAcquisitionRequest
decodeProcessFederationAttributeOwnershipAcquisitionRequest(
    std::span<std::uint8_t const> encoded);

[[nodiscard]] std::vector<std::uint8_t>
encodeProcessFederationAttributeOwnershipReleaseDeniedRequest(
    ProcessFederationAttributeOwnershipReleaseDeniedRequest const& request);
[[nodiscard]] ProcessFederationAttributeOwnershipReleaseDeniedRequest
decodeProcessFederationAttributeOwnershipReleaseDeniedRequest(
    std::span<std::uint8_t const> encoded);

[[nodiscard]] std::vector<std::uint8_t>
encodeProcessFederationAttributeOwnershipAcquisitionCancellationRequest(
    ProcessFederationAttributeOwnershipAcquisitionCancellationRequest const& request);
[[nodiscard]] ProcessFederationAttributeOwnershipAcquisitionCancellationRequest
decodeProcessFederationAttributeOwnershipAcquisitionCancellationRequest(
    std::span<std::uint8_t const> encoded);

[[nodiscard]] std::vector<std::uint8_t>
encodeProcessFederationCancelNegotiatedAttributeOwnershipDivestitureRequest(
    ProcessFederationCancelNegotiatedAttributeOwnershipDivestitureRequest const& request);
[[nodiscard]]
ProcessFederationCancelNegotiatedAttributeOwnershipDivestitureRequest
decodeProcessFederationCancelNegotiatedAttributeOwnershipDivestitureRequest(
    std::span<std::uint8_t const> encoded);

[[nodiscard]] std::vector<std::uint8_t>
encodeProcessFederationNegotiatedAttributeOwnershipDivestitureRequest(
    ProcessFederationNegotiatedAttributeOwnershipDivestitureRequest const& request);
[[nodiscard]] ProcessFederationNegotiatedAttributeOwnershipDivestitureRequest
decodeProcessFederationNegotiatedAttributeOwnershipDivestitureRequest(
    std::span<std::uint8_t const> encoded);

[[nodiscard]] std::vector<std::uint8_t>
encodeProcessFederationConfirmDivestitureRequest(
    ProcessFederationConfirmDivestitureRequest const& request);
[[nodiscard]] ProcessFederationConfirmDivestitureRequest
decodeProcessFederationConfirmDivestitureRequest(
    std::span<std::uint8_t const> encoded);

[[nodiscard]] std::vector<std::uint8_t>
encodeProcessFederationGetInteractionClassHandleRequest(
    ProcessFederationGetInteractionClassHandleRequest const& request);
[[nodiscard]] ProcessFederationGetInteractionClassHandleRequest
decodeProcessFederationGetInteractionClassHandleRequest(
    std::span<std::uint8_t const> encoded);

[[nodiscard]] std::vector<std::uint8_t>
encodeProcessFederationGetFederateHandleRequest(
    ProcessFederationGetFederateHandleRequest const& request);
[[nodiscard]] ProcessFederationGetFederateHandleRequest
decodeProcessFederationGetFederateHandleRequest(
    std::span<std::uint8_t const> encoded);

[[nodiscard]] std::vector<std::uint8_t>
encodeProcessFederationGetFederateNameRequest(
    ProcessFederationGetFederateNameRequest const& request);
[[nodiscard]] ProcessFederationGetFederateNameRequest
decodeProcessFederationGetFederateNameRequest(
    std::span<std::uint8_t const> encoded);

[[nodiscard]] std::vector<std::uint8_t>
encodeProcessFederationNormalizeHandleRequest(
    ProcessFederationNormalizeHandleRequest const& request);
[[nodiscard]] ProcessFederationNormalizeHandleRequest
decodeProcessFederationNormalizeHandleRequest(
    std::span<std::uint8_t const> encoded);

[[nodiscard]] std::vector<std::uint8_t>
encodeProcessFederationGetObjectClassHandleRequest(
    ProcessFederationGetObjectClassHandleRequest const& request);
[[nodiscard]] ProcessFederationGetObjectClassHandleRequest
decodeProcessFederationGetObjectClassHandleRequest(
    std::span<std::uint8_t const> encoded);

[[nodiscard]] std::vector<std::uint8_t>
encodeProcessFederationGetObjectClassNameRequest(
    ProcessFederationGetObjectClassNameRequest const& request);
[[nodiscard]] ProcessFederationGetObjectClassNameRequest
decodeProcessFederationGetObjectClassNameRequest(
    std::span<std::uint8_t const> encoded);

[[nodiscard]] std::vector<std::uint8_t>
encodeProcessFederationGetInteractionClassNameRequest(
    ProcessFederationGetInteractionClassNameRequest const& request);
[[nodiscard]] ProcessFederationGetInteractionClassNameRequest
decodeProcessFederationGetInteractionClassNameRequest(
    std::span<std::uint8_t const> encoded);

[[nodiscard]] std::vector<std::uint8_t>
encodeProcessFederationGetAttributeNameRequest(
    ProcessFederationGetAttributeNameRequest const& request);
[[nodiscard]] ProcessFederationGetAttributeNameRequest
decodeProcessFederationGetAttributeNameRequest(
    std::span<std::uint8_t const> encoded);

[[nodiscard]] std::vector<std::uint8_t>
encodeProcessFederationGetParameterNameRequest(
    ProcessFederationGetParameterNameRequest const& request);
[[nodiscard]] ProcessFederationGetParameterNameRequest
decodeProcessFederationGetParameterNameRequest(
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
encodeProcessFederationGetObjectInstanceHandleRequest(
    ProcessFederationGetObjectInstanceHandleRequest const& request);
[[nodiscard]] ProcessFederationGetObjectInstanceHandleRequest
decodeProcessFederationGetObjectInstanceHandleRequest(
    std::span<std::uint8_t const> encoded);

[[nodiscard]] std::vector<std::uint8_t>
encodeProcessFederationGetObjectInstanceNameRequest(
    ProcessFederationGetObjectInstanceNameRequest const& request);
[[nodiscard]] ProcessFederationGetObjectInstanceNameRequest
decodeProcessFederationGetObjectInstanceNameRequest(
    std::span<std::uint8_t const> encoded);

[[nodiscard]] std::vector<std::uint8_t>
encodeProcessFederationGetKnownObjectClassHandleRequest(
    ProcessFederationGetKnownObjectClassHandleRequest const& request);
[[nodiscard]] ProcessFederationGetKnownObjectClassHandleRequest
decodeProcessFederationGetKnownObjectClassHandleRequest(
    std::span<std::uint8_t const> encoded);

[[nodiscard]] std::vector<std::uint8_t>
encodeProcessFederationInteractionClassDeclarationRequest(
    ProcessFederationInteractionClassDeclarationRequest const& request);
[[nodiscard]] ProcessFederationInteractionClassDeclarationRequest
decodeProcessFederationInteractionClassDeclarationRequest(
    std::span<std::uint8_t const> encoded);

[[nodiscard]] std::vector<std::uint8_t>
encodeProcessFederationInteractionClassRegionalSubscriptionRequest(
    ProcessFederationInteractionClassRegionalSubscriptionRequest const& request);
[[nodiscard]] ProcessFederationInteractionClassRegionalSubscriptionRequest
decodeProcessFederationInteractionClassRegionalSubscriptionRequest(
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
encodeProcessFederationClassHandleRequest(
    ProcessFederationClassHandleRequest const& request);
[[nodiscard]] ProcessFederationClassHandleRequest
decodeProcessFederationClassHandleRequest(
    std::span<std::uint8_t const> encoded);

[[nodiscard]] std::vector<std::uint8_t>
encodeProcessFederationGetTransportationTypeHandleRequest(
    ProcessFederationGetTransportationTypeHandleRequest const& request);
[[nodiscard]] ProcessFederationGetTransportationTypeHandleRequest
decodeProcessFederationGetTransportationTypeHandleRequest(
    std::span<std::uint8_t const> encoded);

[[nodiscard]] std::vector<std::uint8_t>
encodeProcessFederationTransportationTypeRequest(
    ProcessFederationTransportationTypeRequest const& request);
[[nodiscard]] ProcessFederationTransportationTypeRequest
decodeProcessFederationTransportationTypeRequest(
    std::span<std::uint8_t const> encoded);

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

[[nodiscard]] std::vector<std::uint8_t>
encodeProcessFederationQueryLogicalTimeResult(
    ProcessFederationQueryLogicalTimeResult const& result);
[[nodiscard]] ProcessFederationQueryLogicalTimeResult
decodeProcessFederationQueryLogicalTimeResult(
    std::span<std::uint8_t const> encoded);

[[nodiscard]] std::vector<std::uint8_t>
encodeProcessFederationQueryLookaheadResult(
    ProcessFederationQueryLookaheadResult const& result);
[[nodiscard]] ProcessFederationQueryLookaheadResult
decodeProcessFederationQueryLookaheadResult(
    std::span<std::uint8_t const> encoded);

[[nodiscard]] std::vector<std::uint8_t>
encodeProcessFederationModifyLookaheadResult(
    ProcessFederationModifyLookaheadResult const& result);
[[nodiscard]] ProcessFederationModifyLookaheadResult
decodeProcessFederationModifyLookaheadResult(
    std::span<std::uint8_t const> encoded);

[[nodiscard]] std::vector<std::uint8_t>
encodeProcessFederationQueryTimeBoundsResult(
    ProcessFederationQueryTimeBoundsResult const& result);
[[nodiscard]] ProcessFederationQueryTimeBoundsResult
decodeProcessFederationQueryTimeBoundsResult(
    std::span<std::uint8_t const> encoded);

[[nodiscard]] std::vector<std::uint8_t>
encodeProcessFederationEnableTimeRegulationResult(
    ProcessFederationEnableTimeRegulationResult const& result);
[[nodiscard]] ProcessFederationEnableTimeRegulationResult
decodeProcessFederationEnableTimeRegulationResult(
    std::span<std::uint8_t const> encoded);

[[nodiscard]] std::vector<std::uint8_t>
encodeProcessFederationEnableTimeConstrainedResult(
    ProcessFederationEnableTimeConstrainedResult const& result);
[[nodiscard]] ProcessFederationEnableTimeConstrainedResult
decodeProcessFederationEnableTimeConstrainedResult(
    std::span<std::uint8_t const> encoded);

[[nodiscard]] std::vector<std::uint8_t>
encodeProcessFederationTimeAdvanceResult(
    ProcessFederationTimeAdvanceResult const& result);
[[nodiscard]] ProcessFederationTimeAdvanceResult
decodeProcessFederationTimeAdvanceResult(
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
encodeProcessFederationTsoDeliveryAcknowledgementResult(
    ProcessFederationTsoDeliveryAcknowledgementResult const& result);
[[nodiscard]] ProcessFederationTsoDeliveryAcknowledgementResult
decodeProcessFederationTsoDeliveryAcknowledgementResult(
    std::span<std::uint8_t const> encoded);

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

[[nodiscard]] std::vector<std::uint8_t> encodeProcessFederationStringResult(
    ProcessFederationStringResult const& result);
[[nodiscard]] ProcessFederationStringResult decodeProcessFederationStringResult(
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
encodeProcessFederationInteractionOrderTypeChangeResult(
    ProcessFederationInteractionOrderTypeChangeResult const& result);
[[nodiscard]] ProcessFederationInteractionOrderTypeChangeResult
decodeProcessFederationInteractionOrderTypeChangeResult(
    std::span<std::uint8_t const> encoded);
[[nodiscard]] std::vector<std::uint8_t>
encodeProcessFederationAttributeOrderTypeChangeResult(
    ProcessFederationAttributeOrderTypeChangeResult const& result);
[[nodiscard]] ProcessFederationAttributeOrderTypeChangeResult
decodeProcessFederationAttributeOrderTypeChangeResult(
    std::span<std::uint8_t const> encoded);
[[nodiscard]] std::vector<std::uint8_t>
encodeProcessFederationAttributeOrderTypeDefaultResult(
    ProcessFederationAttributeOrderTypeDefaultResult const& result);
[[nodiscard]] ProcessFederationAttributeOrderTypeDefaultResult
decodeProcessFederationAttributeOrderTypeDefaultResult(
    std::span<std::uint8_t const> encoded);
[[nodiscard]] std::vector<std::uint8_t>
encodeProcessFederationAttributeTransportationTypeDefaultResult(
    ProcessFederationAttributeTransportationTypeDefaultResult const& result);
[[nodiscard]] ProcessFederationAttributeTransportationTypeDefaultResult
decodeProcessFederationAttributeTransportationTypeDefaultResult(
    std::span<std::uint8_t const> encoded);
[[nodiscard]] std::vector<std::uint8_t>
encodeProcessFederationAttributeTransportationTypeChangeResult(
    ProcessFederationAttributeTransportationTypeChangeResult const& result);
[[nodiscard]] ProcessFederationAttributeTransportationTypeChangeResult
decodeProcessFederationAttributeTransportationTypeChangeResult(
    std::span<std::uint8_t const> encoded);
[[nodiscard]] std::vector<std::uint8_t>
encodeProcessFederationAttributeTransportationTypeQueryResult(
    ProcessFederationAttributeTransportationTypeQueryResult const& result);
[[nodiscard]] ProcessFederationAttributeTransportationTypeQueryResult
decodeProcessFederationAttributeTransportationTypeQueryResult(
    std::span<std::uint8_t const> encoded);

[[nodiscard]] std::vector<std::uint8_t>
encodeProcessFederationAttributeOwnershipCheckResult(
    ProcessFederationAttributeOwnershipCheckResult const& result);
[[nodiscard]] ProcessFederationAttributeOwnershipCheckResult
decodeProcessFederationAttributeOwnershipCheckResult(
    std::span<std::uint8_t const> encoded);

[[nodiscard]] std::vector<std::uint8_t>
encodeProcessFederationAttributeOwnershipQueryResult(
    ProcessFederationAttributeOwnershipQueryResult const& result);
[[nodiscard]] ProcessFederationAttributeOwnershipQueryResult
decodeProcessFederationAttributeOwnershipQueryResult(
    std::span<std::uint8_t const> encoded);

[[nodiscard]] std::vector<std::uint8_t>
encodeProcessFederationAttributeOwnershipAcquisitionIfAvailableResult(
    ProcessFederationAttributeOwnershipAcquisitionIfAvailableResult const& result);
[[nodiscard]] ProcessFederationAttributeOwnershipAcquisitionIfAvailableResult
decodeProcessFederationAttributeOwnershipAcquisitionIfAvailableResult(
    std::span<std::uint8_t const> encoded);

[[nodiscard]] std::vector<std::uint8_t>
encodeProcessFederationAttributeOwnershipAcquisitionResult(
    ProcessFederationAttributeOwnershipAcquisitionResult const& result);
[[nodiscard]] ProcessFederationAttributeOwnershipAcquisitionResult
decodeProcessFederationAttributeOwnershipAcquisitionResult(
    std::span<std::uint8_t const> encoded);

[[nodiscard]] std::vector<std::uint8_t>
encodeProcessFederationAttributeOwnershipReleaseDeniedResult(
    ProcessFederationAttributeOwnershipReleaseDeniedResult const& result);
[[nodiscard]] ProcessFederationAttributeOwnershipReleaseDeniedResult
decodeProcessFederationAttributeOwnershipReleaseDeniedResult(
    std::span<std::uint8_t const> encoded);

[[nodiscard]] std::vector<std::uint8_t>
encodeProcessFederationAttributeOwnershipAcquisitionCancellationResult(
    ProcessFederationAttributeOwnershipAcquisitionCancellationResult const& result);
[[nodiscard]] ProcessFederationAttributeOwnershipAcquisitionCancellationResult
decodeProcessFederationAttributeOwnershipAcquisitionCancellationResult(
    std::span<std::uint8_t const> encoded);

[[nodiscard]] std::vector<std::uint8_t>
encodeProcessFederationCancelNegotiatedAttributeOwnershipDivestitureResult(
    ProcessFederationCancelNegotiatedAttributeOwnershipDivestitureResult const& result);
[[nodiscard]]
ProcessFederationCancelNegotiatedAttributeOwnershipDivestitureResult
decodeProcessFederationCancelNegotiatedAttributeOwnershipDivestitureResult(
    std::span<std::uint8_t const> encoded);

[[nodiscard]] std::vector<std::uint8_t>
encodeProcessFederationNegotiatedAttributeOwnershipDivestitureResult(
    ProcessFederationNegotiatedAttributeOwnershipDivestitureResult const& result);
[[nodiscard]] ProcessFederationNegotiatedAttributeOwnershipDivestitureResult
decodeProcessFederationNegotiatedAttributeOwnershipDivestitureResult(
    std::span<std::uint8_t const> encoded);

  [[nodiscard]] std::vector<std::uint8_t>
  encodeProcessFederationConfirmDivestitureResult(
      ProcessFederationConfirmDivestitureResult const& result);
[[nodiscard]] ProcessFederationConfirmDivestitureResult
decodeProcessFederationConfirmDivestitureResult(
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
encodeProcessFederationAvailableDimensionsResult(
    ProcessFederationAvailableDimensionsResult const& result);
[[nodiscard]] ProcessFederationAvailableDimensionsResult
decodeProcessFederationAvailableDimensionsResult(
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
encodeProcessFederationRequestAttributeValueUpdateResult(
    ProcessFederationRequestAttributeValueUpdateResult const& result);
[[nodiscard]] ProcessFederationRequestAttributeValueUpdateResult
decodeProcessFederationRequestAttributeValueUpdateResult(
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
// validated base definition is supplied by the higher-level FOM coordinator;
// explicit Create FOM/MIM/time inputs and an optional Join suffix carry
// designators to injected preparation callbacks before the registry commits
// a definition or replacement definition.
// Each transport session gets a handler tied to its joined federate identity,
// while the registry remains the authority for publication, subscription, and
// receive-order routing.
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

  // Project registry work produced by an adapter-managed connection loss onto
  // the surviving process sessions. The registry mutation must happen before
  // this method is called; this method only crosses the process callback
  // boundary for the resulting standard events.
  [[nodiscard]] bool dispatchConnectionLossResult(
      std::wstring const& federationName,
      FederationRegistryResult result,
      std::optional<std::uint64_t> departedFederateId = std::nullopt);

 private:
  struct SessionState final {
    std::optional<std::wstring> federationName;
    std::uint64_t federateId = 0U;
    // The process endpoint keeps the same federation-owned temporal state
    // object for the entire joined-federate lifetime.  Keeping the pointer in
    // the session as well as the registry makes the process service's
    // ownership boundary explicit and lets later role/grant operations use
    // the exact state established at Join.
    std::shared_ptr<FederateTimeState> timeState;
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
    std::deque<ProcessFederationAttributeTransportationTypeChangeEvent>
        attributeTransportationTypeChangeEvents;
    std::deque<ProcessFederationAttributeTransportationTypeQueryEvent>
        attributeTransportationTypeQueryEvents;
    std::deque<ProcessFederationAttributeValueUpdateRequestEvent>
        attributeValueUpdateRequestEvents;
    std::deque<ProcessFederationAttributeOwnershipQueryEvent>
        attributeOwnershipQueryEvents;
    std::deque<
        ProcessFederationAttributeOwnershipAcquisitionIfAvailableEvent>
        attributeOwnershipAcquisitionIfAvailableEvents;
    std::deque<ProcessFederationAttributeOwnershipAcquisitionEvent>
        attributeOwnershipAcquisitionEvents;
    std::deque<ProcessFederationAttributeOwnershipUnavailableEvent>
        attributeOwnershipUnavailableEvents;
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
  [[nodiscard]] TransportServiceMessage handleSendInteractionWithRegions(
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
  [[nodiscard]] TransportServiceMessage handleRequestAttributeValueUpdate(
      ProcessTransportSession& session,
      TransportServiceMessage const& request);
  [[nodiscard]] TransportServiceMessage
  handleRequestAttributeValueUpdateClass(
      ProcessTransportSession& session,
      TransportServiceMessage const& request);
  [[nodiscard]] TransportServiceMessage
  handleRequestAttributeValueUpdateClassWithRegions(
      ProcessTransportSession& session,
      TransportServiceMessage const& request);
  [[nodiscard]] TransportServiceMessage handleAttributeOwnershipCheck(
      ProcessTransportSession& session,
      TransportServiceMessage const& request);
  [[nodiscard]] TransportServiceMessage handleQueryAttributeOwnership(
      ProcessTransportSession& session,
      TransportServiceMessage const& request);
  [[nodiscard]] TransportServiceMessage
  handleAttributeOwnershipAcquisitionIfAvailable(
      ProcessTransportSession& session,
      TransportServiceMessage const& request);
  [[nodiscard]] TransportServiceMessage handleAttributeOwnershipAcquisition(
      ProcessTransportSession& session,
      TransportServiceMessage const& request);
  [[nodiscard]] TransportServiceMessage handleAttributeOwnershipReleaseDenied(
      ProcessTransportSession& session,
      TransportServiceMessage const& request);
  [[nodiscard]] TransportServiceMessage
  handleAttributeOwnershipAcquisitionCancellation(
      ProcessTransportSession& session,
      TransportServiceMessage const& request);
  [[nodiscard]] TransportServiceMessage
  handleCancelNegotiatedAttributeOwnershipDivestiture(
      ProcessTransportSession& session,
      TransportServiceMessage const& request);
  [[nodiscard]] TransportServiceMessage
  handleNegotiatedAttributeOwnershipDivestiture(
      ProcessTransportSession& session,
      TransportServiceMessage const& request);
  [[nodiscard]] TransportServiceMessage handleConfirmDivestiture(
      ProcessTransportSession& session,
      TransportServiceMessage const& request);
  [[nodiscard]] TransportServiceMessage
  handleUnconditionalAttributeOwnershipDivestiture(
      ProcessTransportSession& session,
      TransportServiceMessage const& request);
  [[nodiscard]] TransportServiceMessage handleReceiveInteraction(
      ProcessTransportSession& session,
      TransportServiceMessage const& request);
  [[nodiscard]] TransportServiceMessage handleAcknowledgeTsoDelivery(
      ProcessTransportSession& session,
      TransportServiceMessage const& request);
  [[nodiscard]] TransportServiceMessage handleQueryLogicalTime(
      ProcessTransportSession& session,
      TransportServiceMessage const& request);
  [[nodiscard]] TransportServiceMessage handleQueryLookahead(
      ProcessTransportSession& session,
      TransportServiceMessage const& request);
  [[nodiscard]] TransportServiceMessage handleModifyLookahead(
      ProcessTransportSession& session,
      TransportServiceMessage const& request);
  [[nodiscard]] TransportServiceMessage handleQueryTimeBounds(
      ProcessTransportSession& session,
      TransportServiceMessage const& request);
  [[nodiscard]] TransportServiceMessage handleEnableTimeRegulation(
      ProcessTransportSession& session,
      TransportServiceMessage const& request);
  [[nodiscard]] TransportServiceMessage handleEnableTimeConstrained(
      ProcessTransportSession& session,
      TransportServiceMessage const& request);
  [[nodiscard]] TransportServiceMessage handleDisableTimeRegulation(
      ProcessTransportSession& session,
      TransportServiceMessage const& request);
  [[nodiscard]] TransportServiceMessage handleDisableTimeConstrained(
      ProcessTransportSession& session,
      TransportServiceMessage const& request);
  [[nodiscard]] TransportServiceMessage handleDisableTimeRole(
      ProcessTransportSession& session,
      TransportServiceMessage const& request,
      bool regulation);
  [[nodiscard]] TransportServiceMessage handleTimeAdvanceRequest(
      ProcessTransportSession& session,
      TransportServiceMessage const& request);
  [[nodiscard]] TransportServiceMessage handleTimeAdvanceRequestAvailable(
      ProcessTransportSession& session,
      TransportServiceMessage const& request);
  [[nodiscard]] TransportServiceMessage handleNextMessageRequest(
      ProcessTransportSession& session,
      TransportServiceMessage const& request);
  [[nodiscard]] TransportServiceMessage handleNextMessageRequestAvailable(
      ProcessTransportSession& session,
      TransportServiceMessage const& request);
  [[nodiscard]] TransportServiceMessage handleFlushQueueRequest(
      ProcessTransportSession& session,
      TransportServiceMessage const& request);
  [[nodiscard]] TransportServiceMessage handleTimeAdvanceRequestForMode(
      ProcessTransportSession& session,
      TransportServiceMessage const& request,
      FederateTimeAdvanceMode mode);
  [[nodiscard]] TransportServiceMessage handleReceiveAttributeUpdate(
      ProcessTransportSession& session,
      TransportServiceMessage const& request);
  [[nodiscard]] TransportServiceMessage handleReceiveObjectInstanceDiscovery(
      ProcessTransportSession& session,
      TransportServiceMessage const& request);
  [[nodiscard]] TransportServiceMessage handleGetInteractionClassHandle(
      ProcessTransportSession& session,
      TransportServiceMessage const& request);
  [[nodiscard]] TransportServiceMessage handleGetFederateHandle(
      ProcessTransportSession& session,
      TransportServiceMessage const& request);
  [[nodiscard]] TransportServiceMessage handleGetFederateName(
      ProcessTransportSession& session,
      TransportServiceMessage const& request);
  [[nodiscard]] TransportServiceMessage handleNormalizeHandle(
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
  [[nodiscard]] TransportServiceMessage handleGetObjectInstanceHandle(
      ProcessTransportSession& session,
      TransportServiceMessage const& request);
  [[nodiscard]] TransportServiceMessage handleGetObjectInstanceName(
      ProcessTransportSession& session,
      TransportServiceMessage const& request);
  [[nodiscard]] TransportServiceMessage handleGetKnownObjectClassHandle(
      ProcessTransportSession& session,
      TransportServiceMessage const& request);
  [[nodiscard]] TransportServiceMessage handleGetObjectClassName(
      ProcessTransportSession& session,
      TransportServiceMessage const& request);
  [[nodiscard]] TransportServiceMessage handleGetInteractionClassName(
      ProcessTransportSession& session,
      TransportServiceMessage const& request);
  [[nodiscard]] TransportServiceMessage handleGetAttributeName(
      ProcessTransportSession& session,
      TransportServiceMessage const& request);
  [[nodiscard]] TransportServiceMessage handleGetParameterName(
      ProcessTransportSession& session,
      TransportServiceMessage const& request);
  [[nodiscard]] TransportServiceMessage handleInteractionClassDeclaration(
      ProcessTransportSession& session,
      TransportServiceMessage const& request);
  [[nodiscard]] TransportServiceMessage handleChangeInteractionOrderType(
      ProcessTransportSession& session,
      TransportServiceMessage const& request);
  [[nodiscard]] TransportServiceMessage handleChangeAttributeOrderType(
      ProcessTransportSession& session,
      TransportServiceMessage const& request);
  [[nodiscard]] TransportServiceMessage handleChangeDefaultAttributeOrderType(
      ProcessTransportSession& session,
      TransportServiceMessage const& request);
  [[nodiscard]] TransportServiceMessage
  handleChangeDefaultAttributeTransportationType(
      ProcessTransportSession& session,
      TransportServiceMessage const& request);
  [[nodiscard]] TransportServiceMessage
  handleRequestAttributeTransportationTypeChange(
      ProcessTransportSession& session,
      TransportServiceMessage const& request);
  [[nodiscard]] TransportServiceMessage
  handleQueryAttributeTransportationType(
      ProcessTransportSession& session,
      TransportServiceMessage const& request);
  [[nodiscard]] TransportServiceMessage handleInteractionClassRegionalSubscription(
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
  [[nodiscard]] TransportServiceMessage handleGetDimensionName(
      ProcessTransportSession& session,
      TransportServiceMessage const& request);
  [[nodiscard]] TransportServiceMessage handleGetTransportationTypeHandle(
      ProcessTransportSession& session,
      TransportServiceMessage const& request);
  [[nodiscard]] TransportServiceMessage handleGetTransportationTypeName(
      ProcessTransportSession& session,
      TransportServiceMessage const& request);
  [[nodiscard]] TransportServiceMessage handleGetDimensionUpperBound(
      ProcessTransportSession& session,
      TransportServiceMessage const& request);
  [[nodiscard]] TransportServiceMessage
  handleGetAvailableDimensionsForObjectClass(
      ProcessTransportSession& session,
      TransportServiceMessage const& request);
  [[nodiscard]] TransportServiceMessage
  handleGetAvailableDimensionsForInteractionClass(
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
  handleGetConveyRegionDesignatorSetsSwitch(
      ProcessTransportSession& session,
      TransportServiceMessage const& request);
  [[nodiscard]] TransportServiceMessage
  handleSetConveyRegionDesignatorSetsSwitch(
      ProcessTransportSession& session,
      TransportServiceMessage const& request);
  [[nodiscard]] TransportServiceMessage
  handleGetAutomaticResignDirective(
      ProcessTransportSession& session,
      TransportServiceMessage const& request);
  [[nodiscard]] TransportServiceMessage
  handleSetAutomaticResignDirective(
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

  [[nodiscard]] bool enqueueAttributeOwnershipAcquisitionWorkItems(
      std::wstring const& federationName,
      std::vector<AttributeOwnershipAcquisitionWorkItem> workItems);

  [[nodiscard]] bool enqueueAttributeOwnershipAssumptionRecipients(
      std::wstring const& federationName,
      std::vector<AttributeOwnershipAssumptionRecipient> recipients);

  // Confirm Divestiture notifications use the same receive-order fence as
  // ownership acquisition callbacks, but retain a distinct registry delivery
  // boundary so the transfer commits immediately before callback projection.
  [[nodiscard]] bool enqueueConfirmDivestitureNotifications(
      std::wstring const& federationName,
      std::vector<ConfirmDivestitureNotification> notifications);

  [[nodiscard]] bool enqueueAttributeOwnershipUnavailableRecipients(
      std::wstring const& federationName,
      std::vector<AttributeOwnershipUnavailableRecipient> recipients,
      std::vector<std::uint8_t> userSuppliedTag);

  // Execute registry-owned time-grant work only after the registry lock has
  // been released.  A grant may make another pending request eligible, so the
  // helper drains bounded re-evaluation rounds until the federation reaches a
  // fixed point.
  void dispatchTimeAdvanceGrants(
      std::wstring const& federationName,
      std::vector<FederationTimeGrantDispatch> dispatches);

  // Release regular timestamped interaction payloads at the same process
  // callback boundary as the matching time-advance grant.  The helper keeps
  // the process transport seam private: the registry owns queue/retraction
  // state, while this service projects one typed delivery into the existing
  // interaction event frame before publishing the grant event.
  void dispatchTsoInteractionPayloads(
      std::wstring const& federationName,
      std::uint64_t receivingFederateId,
      rti1516_2025::LogicalTime const& boundary);


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
  // Joining may replace a federation's composed definition. Serialize that
  // preparation/commit pair so two process clients cannot both compose from
  // the same stale definition and race the registry replacement.
  mutable std::mutex joinMutex_;
  mutable std::mutex mutex_;
  std::map<ProcessTransportSession*, SessionState> sessions_;
  std::map<std::uint64_t, ProcessTransportSession*> sessionsByFederateId_;
};

}  // namespace umbra::detail
