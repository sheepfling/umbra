#pragma once

#include <cstdint>
#include <optional>
#include <string>
#include <string_view>
#include <utility>
#include <vector>

namespace umbra::detail {

// A route-free, versioned representation of the federation state that is
// allowed to cross the save-store boundary.  Callback functions, mutexes, and
// ambassador-owned writers are deliberately absent; those are rebound from
// the live joined federates during restore.  The v1 image covers the
// federation/control-plane identity, temporal admission state, object
// identity/ownership metadata, per-federate interaction declarations,
// per-federate object-class attribute publication/subscription/default
// declarations, synchronization-point state, and
// ordinary/directed timestamped interaction payloads, timestamped attribute
// update payloads, timestamped object-deletion delivery payloads, the shared
// Request Retraction recipient ledger, and queue phase records. Restore
// rehydrates the queue plus these four delivery payload families, the bounded
// timestamped-deletion invocation snapshot, and the retraction ledger;
// The typed ownership slice includes the If Available and regular
// acquisition-reservation ledgers plus cancellation, Divestiture-If-Wanted,
// and Confirm Divestiture notification reservations, plus pending attribute
// transportation-type changes, negotiated divestiture records, and retained
// ownership-assumption recipient/tag ledgers; remaining typed ownership/value
// ledgers remain intentionally outside v1. Object visibility now also retains
// the known-class projection, pending discovery/removal callback sets, the
// connection-loss removal classifications, and timestamped-removal linkage.
// Joined-federate Update Attribute Values telemetry is likewise retained as a
// typed application-value ledger. The accepted application Reflect Attribute
// Values scalar, class/transportation buckets, and distinct-object/class
// projections are retained as the corresponding reflection ledger.
// Pending time-advance and time-role-enable generation identities, deferred
// decreasing Modify Lookahead requests, and the next-generation allocator
// floor are retained as a typed temporal application-request ledger. Accepted
// Send Interaction total/directed
// counters and class/transportation
// buckets are retained as the sender-side interaction ledger; matching
// Receive Interaction totals, directed receipts, and class/transportation
// buckets are retained as the receiver-side ledger. Exact federate-owned
// object-instance-name reservations are retained in their own section.
// The section counts make that bounded coverage
// explicit so a future image version cannot silently reinterpret a partial
// payload as a complete state image.
struct FederationStateImageMember final {
  std::uint64_t id = 0;
  std::wstring name;
  std::wstring type;
  std::uint32_t switches = 0;
  std::uint32_t automaticResignAction = 0;
  std::int32_t momReportPeriodSeconds = 0;
  std::uint32_t nextMomServiceReportSerialNumber = 0;
  // Accepted Update Attribute Values history is joined-federate lifetime
  // state, not callback routing. Keep the scalar count, per-class/
  // transportation buckets, and distinct updated-object projection typed so
  // a future process-restart restore can reproduce MOM values exactly.
  std::uint64_t successfulUpdateAttributeValuesCount = 0;
  struct UpdateCount final {
    std::uint64_t objectClassHandle = 0;
    std::string transportationName;
    std::uint64_t count = 0;
  };
  struct UpdatedObjectClass final {
    std::uint64_t objectInstanceHandle = 0;
    std::uint64_t objectClassHandle = 0;
  };
  std::vector<UpdateCount> successfulUpdateCountsByClassAndTransportation;
  std::vector<std::uint64_t> successfullyUpdatedObjectInstanceHandles;
  std::vector<UpdatedObjectClass> successfullyUpdatedObjectInstanceClassHandles;
  // Accepted application Reflect Attribute Values callback invocations for
  // this joined-federate lifetime. Keep the scalar separate from the typed
  // class/transportation and distinct-object projections below.
  std::uint64_t successfulReflectionsReceivedCount = 0;
  // Decoder-only compatibility marker: v1 member records with twelve fields
  // carry the reflection scalar, including an explicit zero. It is not
  // serialized separately and keeps legacy seven/eleven-field images intact.
  bool reflectionTelemetryPresent = false;
  struct ReflectionCount final {
    std::uint64_t objectClassHandle = 0;
    std::string transportationName;
    std::uint64_t count = 0;
  };
  struct ReflectedObjectClass final {
    std::uint64_t objectInstanceHandle = 0;
    std::uint64_t objectClassHandle = 0;
  };
  std::vector<ReflectionCount> successfulReflectionCountsByClassAndTransportation;
  std::vector<std::uint64_t> successfullyReflectedObjectInstanceHandles;
  std::vector<ReflectedObjectClass> successfullyReflectedObjectInstanceClassHandles;
  // Decoder-only compatibility marker: v1 member records with nineteen
  // fields carry the richer reflection projections, including empty sets.
  bool reflectionProjectionTelemetryPresent = false;
  // Accepted object-lifecycle MOM statistics for this joined-federate
  // lifetime. The counters advance at their respective service/callback
  // boundaries and remain distinct from live object membership.
  std::uint64_t successfulObjectInstanceRegistrationsCount = 0;
  std::uint64_t successfulObjectInstanceDeletionsCount = 0;
  std::uint64_t successfulObjectInstanceRemovalsCount = 0;
  std::uint64_t successfulObjectInstanceDiscoveriesCount = 0;
  // Decoder-only compatibility marker: v1 member records with sixteen fields
  // carry the four lifecycle scalars, including explicit zeros.
  bool objectLifecycleTelemetryPresent = false;
  // Accepted Send Interaction service history for this joined-federate
  // lifetime. Keep the all-interaction and directed-subset counters separate,
  // together with their class/transportation buckets.
  std::uint64_t successfulInteractionsSentCount = 0;
  std::uint64_t successfulDirectedInteractionsSentCount = 0;
  struct InteractionSendCount final {
    std::uint64_t interactionClassHandle = 0;
    std::string transportationName;
    std::uint64_t count = 0;
  };
  struct DirectedInteractionSendCount final {
    std::uint64_t interactionClassHandle = 0;
    std::string transportationName;
    std::uint64_t count = 0;
  };
  std::vector<InteractionSendCount>
      successfulInteractionCountsByClassAndTransportation;
  std::vector<DirectedInteractionSendCount>
      successfulDirectedInteractionCountsByClassAndTransportation;
  // Decoder-only compatibility marker: v1 member records with 23 fields
  // carry the sender-side interaction ledger, including explicit zeros.
  bool interactionSendTelemetryPresent = false;
  // Accepted application Receive Interaction callback history for this
  // joined-federate lifetime. Directed receipts remain a subset of the total
  // while retaining independent class/transportation buckets.
  std::uint64_t successfulInteractionsReceivedCount = 0;
  std::uint64_t successfulDirectedInteractionsReceivedCount = 0;
  struct InteractionReceiptCount final {
    std::uint64_t interactionClassHandle = 0;
    std::string transportationName;
    std::uint64_t count = 0;
  };
  struct DirectedInteractionReceiptCount final {
    std::uint64_t interactionClassHandle = 0;
    std::string transportationName;
    std::uint64_t count = 0;
  };
  std::vector<InteractionReceiptCount>
      successfulInteractionReceiptCountsByClassAndTransportation;
  std::vector<DirectedInteractionReceiptCount>
      successfulDirectedInteractionReceiptCountsByClassAndTransportation;
  // Decoder-only compatibility marker: v1 member records with 27 fields
  // carry the receiver-side interaction ledger, including explicit zeros.
  bool interactionReceiptTelemetryPresent = false;
};

struct FederationStateImageTimeState final {
  std::uint64_t federateId = 0;
  std::wstring implementationName;
  std::uint32_t flags = 0;
  std::uint64_t pendingGeneration = 0;
  std::uint32_t advanceMode = 0;
  std::optional<std::string> currentTimeEncoding;
  std::optional<std::string> optimisticTimeEncoding;
  std::optional<std::string> requestedTimeEncoding;
  std::optional<std::string> advanceRequestTimeEncoding;
  std::optional<std::string> lookaheadEncoding;
  std::optional<std::string> requestedLookaheadEncoding;
  std::uint64_t queuedTsoCount = 0;
  std::uint64_t inTransitTsoCount = 0;
  std::uint64_t deliveredTsoCount = 0;
  // Callback-generation identities for pending time-role requests and the
  // next allocator floor.  The trailing fields preserve compatibility with
  // fourteen-field v1 records while making the pending application request
  // ledger explicit in newly written images.
  std::uint64_t pendingTimeRegulationGeneration = 0;
  std::uint64_t pendingTimeConstrainedGeneration = 0;
  std::uint64_t nextGeneration = 1;
  // A deferred decreasing Modify Lookahead request is a distinct application
  // request from the active lookahead and a pending role-enable lookahead.
  // Keep it trailing so fourteen- and seventeen-field v1 records remain
  // readable while new images retain the complete temporal request ledger.
  std::optional<std::string> pendingModifiedLookaheadEncoding;
  bool applicationRequestLedgerPresent = false;
};

struct FederationStateImageObjectAttribute final {
  std::uint64_t handle = 0;
  std::uint64_t ownerFederateId = 0;
  std::string transportationName;
  std::uint32_t orderType = 0;
  std::vector<std::uint64_t> updateRegionHandles;
};

// Latest accepted application value for one object attribute. Values are
// opaque route-free bytes; attribute metadata remains in the sibling
// attributes vector so legacy object images remain forward-compatible.
struct FederationStateImageObjectAttributeValue final {
  std::uint64_t attributeHandle = 0;
  std::string value;
};

// Typed application-request ledger entry for one accepted object-instance
// Request Attribute Value Update provider callback. The callback endpoint is
// process-local; the request identity, provider/requester, requested
// attributes, and copied tag are enough to rebind the callback after a fresh
// registry restore.
struct FederationStateImagePendingAttributeValueUpdate final {
  std::uint64_t requestId = 0;
  std::uint64_t requestingFederateId = 0;
  std::uint64_t providingFederateId = 0;
  std::vector<std::uint64_t> requestedAttributeHandles;
  std::string userSuppliedTag;
};

// Typed application-request ledger entry for one accepted object-class
// Request Attribute Value Update provider callback.  A class request expands
// to one durable entry per object/provider delivery, so the requested class
// handle remains part of the identity used by callback-time revalidation.
struct FederationStateImagePendingAttributeValueUpdateClass final {
  std::uint64_t requestId = 0;
  std::uint64_t requestingFederateId = 0;
  std::uint64_t providingFederateId = 0;
  std::uint64_t requestedObjectClassHandle = 0;
  std::vector<std::uint64_t> requestedAttributeHandles;
  std::string userSuppliedTag;
};

// Typed application-request ledger entry for one accepted regional
// object-class Request Attribute Value Update provider callback.  Region
// designators are retained per requested attribute so callback-time DDM
// overlap is evaluated against the same request shape after restore.
struct FederationStateImagePendingAttributeValueUpdateRegional final {
  std::uint64_t requestId = 0;
  std::uint64_t requestingFederateId = 0;
  std::uint64_t providingFederateId = 0;
  std::uint64_t requestedObjectClassHandle = 0;
  std::vector<std::uint64_t> requestedAttributeHandles;
  std::vector<std::pair<std::uint64_t, std::vector<std::uint64_t>>>
      requestRegionsByAttribute;
  std::string userSuppliedTag;
};

// Typed application-request ledger entry for one accepted Query Attribute
// Ownership result callback. The callback route is process-local; the request
// identity, target object, result partition, owner, and attribute set are
// sufficient to rebind that callback after a fresh registry restore.
struct FederationStateImagePendingAttributeOwnershipQuery final {
  std::uint64_t requestId = 0;
  std::uint64_t requestingFederateId = 0;
  std::uint64_t objectInstanceHandle = 0;
  // Stable wire values: 0=federate, 1=unowned, 2=RTI-owned.
  std::uint32_t reportKind = 0;
  std::uint64_t owningFederateId = 0;
  std::vector<std::uint64_t> requestedAttributeHandles;
};

// Typed ownership ledger entry for one accepted If Available acquisition.
// Callback routes are live-only; the request identity, ordering sequence,
// desired attribute set, and user tag are the durable state needed to resume
// the request at the callback boundary.
struct FederationStateImagePendingAttributeOwnershipAcquisitionIfAvailable final {
  std::uint64_t requestId = 0;
  std::uint64_t requestingFederateId = 0;
  std::uint64_t requestSequence = 0;
  std::vector<std::uint64_t> desiredAttributeHandles;
  std::string userSuppliedTag;
};

// Typed ownership ledger entry for one regular acquisition.  The queued
// callback sets and owner-partitioned release callbacks are retained because
// they determine which callback boundary remains live after restore.
struct FederationStateImagePendingAttributeOwnershipAcquisition final {
  std::uint64_t requestId = 0;
  std::uint64_t requestingFederateId = 0;
  std::uint64_t requestSequence = 0;
  std::vector<std::uint64_t> desiredAttributeHandles;
  std::vector<std::uint64_t> notificationQueuedAttributeHandles;
  std::vector<std::uint64_t> unavailableQueuedAttributeHandles;
  std::vector<std::pair<std::uint64_t, std::vector<std::uint64_t>>>
      releaseCallbacksQueuedByOwningFederate;
  std::string userSuppliedTag;
};

// Typed ownership ledger entry for a cancellation admitted against a regular
// acquisition.  Its cancellation identity and selected attributes must
// survive until the callback-time cancellation boundary resolves the request.
struct FederationStateImagePendingAttributeOwnershipAcquisitionCancellation final {
  std::uint64_t cancellationId = 0;
  std::uint64_t requestingFederateId = 0;
  std::vector<std::uint64_t> attributeHandles;
};

// Typed ownership ledger entry for one queued Request Divestiture If Wanted
// notification.  The callback target and exact attribute set are retained;
// callback routes are rebound from the live joined federate on restore.
struct FederationStateImagePendingAttributeOwnershipDivestitureIfWanted final {
  std::uint64_t notificationId = 0;
  std::uint64_t receivingFederateId = 0;
  std::vector<std::uint64_t> attributeHandles;
  // The divesting federate's service tag is part of the eventual
  // Attribute Ownership Acquisition Notification and must survive a
  // fresh-registry restore.
  std::string userSuppliedTag;
};

// Typed ownership ledger entry for one queued Confirm Divestiture
// notification.  The callback target and exact attribute set are retained;
// callback routes are rebound from the live joined federate on restore.
struct FederationStateImagePendingConfirmDivestiture final {
  std::uint64_t notificationId = 0;
  std::uint64_t receivingFederateId = 0;
  std::vector<std::uint64_t> attributeHandles;
  // The confirming owner's user-supplied tag is delivered with the eventual
  // Attribute Ownership Acquisition Notification and must survive a fresh
  // registry restore.
  std::string userSuppliedTag;
};

// Typed ownership/value-adjacent ledger entry for one pending attribute
// transportation-type change.  The request identity, owner, attribute set,
// and selected transportation are retained until the callback boundary
// applies or cancels the change.
struct FederationStateImagePendingAttributeTransportationTypeChange final {
  std::uint64_t requestId = 0;
  std::uint64_t requestingFederateId = 0;
  std::vector<std::uint64_t> attributeHandles;
  std::string transportationName;
};

// Typed ownership ledger entry for one attribute in negotiated divestiture.
// The candidate acquisition and confirmation flags are retained because the
// owner remains in Waiting for a New Owner to be Found until Confirm
// Divestiture completes the transfer.
struct FederationStateImagePendingNegotiatedAttributeOwnershipDivestiture final {
  std::uint64_t attributeHandle = 0;
  std::uint64_t divestingFederateId = 0;
  std::uint64_t acquiringFederateId = 0;
  std::uint64_t acquisitionRequestId = 0;
  bool acquiringFederateIsIfAvailable = false;
  bool confirmationQueued = false;
  bool confirmationDelivered = false;
  std::string userSuppliedTag;
};

// Typed ownership ledger entry for the recipient reservation set retained by
// an unowned attribute's continuing assumption search. An empty recipient
// list is meaningful: it records the search even when no current federate is
// eligible to receive an offer.
struct FederationStateImageOwnershipAssumptionRecipients final {
  std::uint64_t attributeHandle = 0;
  std::vector<std::uint64_t> recipientFederateIds;
};

// Typed ownership ledger entry for the tag associated with an assumption
// search. It is separate from the recipient map so an explicitly empty tag
// and an empty recipient reservation remain durable and distinguishable.
struct FederationStateImageOwnershipAssumptionTag final {
  std::uint64_t attributeHandle = 0;
  std::string userSuppliedTag;
};

// Route-free identity for one queued Request Attribute Ownership Assumption
// callback. The callback route is rebound from the restored recipient's live
// ambassador; the object, recipient, attributes, and tag define the exact
// callback payload that remains pending at the save boundary.
struct FederationStateImagePendingAttributeOwnershipAssumption final {
  std::uint64_t objectInstanceHandle = 0;
  std::uint64_t receivingFederateId = 0;
  std::vector<std::uint64_t> attributeHandles;
  std::string userSuppliedTag;
};

// Typed object-visibility projection for one federate that knows this object.
// The class handle is needed to rebind discovery/reflection eligibility after
// restore without depending on a process-local object snapshot.
struct FederationStateImageKnownObjectClass final {
  std::uint64_t federateId = 0;
  std::uint64_t objectClassHandle = 0;
};

struct FederationStateImageObject final {
  std::uint64_t handle = 0;
  std::wstring name;
  std::uint64_t registeredObjectClassHandle = 0;
  std::uint64_t producingFederateId = 0;
  bool deleteAccepted = false;
  // This count fences pending ownership/transport/discovery work whose live
  // callback endpoints are intentionally not serialized in v1. Typed records
  // are added incrementally; the count remains the compatibility fence for
  // ledger families not yet represented explicitly.
  std::uint64_t pendingOperationCount = 0;
  std::vector<FederationStateImageObjectAttribute> attributes;
  std::vector<FederationStateImageObjectAttributeValue> attributeValues;
  // True when the encoder emitted the value ledger, including an explicit
  // empty ledger. Legacy object images leave this false.
  bool attributeValuesPresent = false;
  std::vector<FederationStateImagePendingAttributeValueUpdate>
      pendingAttributeValueUpdateRequests;
  // True when the encoder emitted the pending application-request ledger,
  // including an explicit empty ledger. Legacy object images leave this false.
  bool pendingAttributeValueUpdateRequestsPresent = false;
  std::vector<FederationStateImagePendingAttributeValueUpdateClass>
      pendingAttributeValueUpdateClassRequests;
  // True when the encoder emitted the pending object-class application-request
  // ledger, including an explicit empty ledger. Legacy object images leave
  // this false.
  bool pendingAttributeValueUpdateClassRequestsPresent = false;
  std::vector<FederationStateImagePendingAttributeValueUpdateRegional>
      pendingAttributeValueUpdateRegionalRequests;
  // True when the encoder emitted the pending regional application-request
  // ledger, including an explicit empty ledger. Legacy object images leave
  // this false.
  bool pendingAttributeValueUpdateRegionalRequestsPresent = false;
  std::vector<FederationStateImagePendingAttributeOwnershipAcquisitionIfAvailable>
      pendingAttributeOwnershipAcquisitionIfAvailableRequests;
  std::vector<FederationStateImagePendingAttributeOwnershipAcquisition>
      pendingAttributeOwnershipAcquisitionRequests;
  std::vector<FederationStateImagePendingAttributeOwnershipAcquisitionCancellation>
      pendingAttributeOwnershipAcquisitionCancellations;
  std::vector<FederationStateImagePendingAttributeOwnershipDivestitureIfWanted>
      pendingAttributeOwnershipDivestitureIfWantedNotifications;
  std::vector<FederationStateImagePendingConfirmDivestiture>
      pendingConfirmDivestitureNotifications;
  std::vector<FederationStateImagePendingAttributeTransportationTypeChange>
      pendingAttributeTransportationTypeChanges;
  std::vector<FederationStateImagePendingNegotiatedAttributeOwnershipDivestiture>
      pendingNegotiatedAttributeOwnershipDivestitures;
  std::vector<FederationStateImageOwnershipAssumptionRecipients>
      ownershipAssumptionRecipientsByAttribute;
  std::vector<FederationStateImageOwnershipAssumptionTag>
      ownershipAssumptionUserSuppliedTagsByAttribute;
  std::vector<FederationStateImageKnownObjectClass>
      knownObjectClassHandlesByFederate;
  std::vector<std::uint64_t> pendingDiscoveryFederateIds;
  std::vector<std::uint64_t> pendingRemovalFederateIds;
  // These classifications are subsets of pendingRemovalFederateIds. They
  // preserve whether an ordinary removal came from forced connection loss and
  // whether its callback was deferred behind an at-or-before-loss TSO item.
  std::vector<std::uint64_t> connectionLossAutomaticRemovalFederateIds;
  std::vector<std::uint64_t> deferredConnectionLossTsoRemovalFederateIds;
  // A timestamped object deletion retains its message identity until the
  // retraction designator becomes terminal. The recipient set is the subset
  // still awaiting its Remove Object Instance callback.
  std::vector<std::uint64_t> pendingTimestampedRemovalFederateIds;
  std::optional<std::uint64_t> pendingTimestampedDeletionMessageId;
};

struct FederationStateImageInteractionSubscription final {
  std::uint64_t interactionClassHandle = 0;
  bool active = false;
};

struct FederationStateImageRegionalInteractionSubscription final {
  std::uint64_t interactionClassHandle = 0;
  std::uint64_t regionHandle = 0;
  bool active = false;
};

struct FederationStateImagePublishedDirectedInteraction final {
  std::uint64_t objectClassHandle = 0;
  std::uint64_t interactionClassHandle = 0;
};

struct FederationStateImageSubscribedDirectedInteraction final {
  std::uint64_t objectClassHandle = 0;
  std::uint64_t interactionClassHandle = 0;
  bool active = false;
};

struct FederationStateImageInteractionStringValue final {
  std::uint64_t interactionClassHandle = 0;
  std::string value;
};

struct FederationStateImageInteractionOrderValue final {
  std::uint64_t interactionClassHandle = 0;
  std::uint32_t orderType = 0;
};

struct FederationStateImageInteractionDeclaration final {
  std::uint64_t federateId = 0;
  std::vector<std::uint64_t> publishedInteractionClasses;
  std::vector<FederationStateImageInteractionSubscription>
      subscribedInteractionClasses;
  std::vector<FederationStateImageRegionalInteractionSubscription>
      regionalSubscribedInteractionClasses;
  std::vector<FederationStateImagePublishedDirectedInteraction>
      publishedObjectClassDirectedInteractions;
  std::vector<FederationStateImageSubscribedDirectedInteraction>
      subscribedObjectClassDirectedInteractions;
  std::vector<FederationStateImageInteractionStringValue>
      interactionTransportationTypes;
  std::vector<FederationStateImageInteractionOrderValue> interactionOrderTypes;
  std::vector<FederationStateImageInteractionStringValue>
      pendingInteractionTransportationTypeChanges;
};

struct FederationStateImageInteractionParameter final {
  std::uint64_t parameterHandle = 0;
  std::string value;
};

struct FederationStateImageRegionRange final {
  std::uint64_t dimensionHandle = 0;
  std::uint64_t lowerBound = 0;
  std::uint64_t upperBound = 0;
};

// Route-free durable state for one federation-owned region specification.
// Pending and committed bounds are kept separately because Set Range Bounds
// mutates the pending specification while Commit Region Modifications makes
// the committed view visible to DDM routing and regional declarations.
struct FederationStateImageRegion final {
  std::uint64_t handle = 0;
  std::uint64_t ownerFederateId = 0;
  std::vector<std::uint64_t> dimensionHandles;
  std::vector<FederationStateImageRegionRange> pendingRangeBounds;
  std::vector<FederationStateImageRegionRange> committedRangeBounds;
  bool specificationCommitted = false;
  bool inUse = false;
};

struct FederationStateImageInteractionRegionSnapshot final {
  std::uint64_t regionHandle = 0;
  bool specificationCommitted = false;
  std::vector<std::uint64_t> dimensionHandles;
  std::vector<FederationStateImageRegionRange> committedRangeBounds;
};

// Route-free payload for one timestamped ordinary interaction. Recipient
// callback routes and queue internals stay live-only; this record preserves
// the accepted interaction bytes and invocation-time regional realization so
// a later image version can rebuild delivery without changing its meaning.
struct FederationStateImageTsoInteractionMessage final {
  std::uint64_t messageId = 0;
  std::uint64_t producingFederateId = 0;
  std::uint64_t sentInteractionClassHandle = 0;
  std::vector<std::uint64_t> sentParameterHandles;
  std::vector<FederationStateImageInteractionParameter> parameters;
  std::string userSuppliedTag;
  std::string transportationName;
  std::vector<std::uint64_t> sentRegionHandles;
  std::vector<FederationStateImageInteractionRegionSnapshot>
      sentRegionSnapshots;
  bool defaultRegionUsed = false;
  std::optional<std::string> timestampEncoding;
  std::uint32_t sentOrderType = 0;
  std::uint32_t receivedOrderType = 0;
};

// Route-free payload for one timestamped Update Attribute Values invocation.
// The passel projections are retained per recipient because regional scope,
// transportation, and order are accepted per callback path at invocation
// time.  Callback routes themselves remain live-only.
struct FederationStateImageAttributeValue final {
  std::uint64_t attributeHandle = 0;
  std::string value;
};

struct FederationStateImageTsoAttributeUpdatePassel final {
  std::string transportationName;
  std::vector<std::uint64_t> sentAttributeHandles;
  std::vector<std::uint64_t> sentRegionHandles;
  std::vector<FederationStateImageInteractionRegionSnapshot>
      sentRegionSnapshots;
  bool defaultRegionUsed = false;
  std::uint32_t preferredOrderType = 0;
};

struct FederationStateImageTsoAttributeUpdateRecipient final {
  std::uint64_t receivingFederateId = 0;
  std::vector<FederationStateImageTsoAttributeUpdatePassel> passels;
};

struct FederationStateImageTsoAttributeUpdateMessage final {
  std::uint64_t messageId = 0;
  std::uint64_t producingFederateId = 0;
  std::uint64_t objectInstanceHandle = 0;
  std::vector<FederationStateImageAttributeValue> attributes;
  std::string userSuppliedTag;
  std::vector<FederationStateImageTsoAttributeUpdateRecipient>
      passelsByRecipient;
  std::vector<FederationStateImageInteractionRegionSnapshot>
      sentRegionSnapshots;
  std::optional<std::string> timestampEncoding;
};

struct FederationStateImageTsoObjectDeletionRecipient final {
  std::uint64_t receivingFederateId = 0;
  std::uint64_t objectInstanceHandle = 0;
};

struct FederationStateImageTsoObjectDeletionReconstitution final {
  FederationStateImageObject object;
  std::vector<std::pair<std::uint64_t, std::uint64_t>>
      knownObjectClassHandlesByFederate;
};

// Route-free payload for one timestamped Delete Object Instance invocation.
// The immutable recipient/object projection is persisted so restore can bind
// current callback and service-report routes without changing the accepted
// fanout. The separate invocation-time object snapshot used by Retract remains
// an intentionally later typed ledger slice.
struct FederationStateImageTsoObjectDeletionMessage final {
  std::uint64_t messageId = 0;
  std::uint64_t producingFederateId = 0;
  std::uint64_t objectInstanceHandle = 0;
  std::string userSuppliedTag;
  std::vector<FederationStateImageTsoObjectDeletionRecipient> recipients;
  std::optional<std::string> timestampEncoding;
  std::optional<FederationStateImageTsoObjectDeletionReconstitution>
      reconstitution;
};

// Route-free snapshot of the shared Request Retraction ledger.  The message
// payload sections above preserve accepted bytes and invocation projections;
// this companion section preserves the producer/timestamp boundary and each
// recipient's callback state even after a payload has been reclaimed.  Live
// callback routes are deliberately rebound from current joined federates.
struct FederationStateImageTsoRequestRetractionRecipient final {
  std::uint64_t receivingFederateId = 0;
  // 0=pending, 1=delivered, 2=suppressed, 3=retracted.
  std::uint32_t state = 0;
};

struct FederationStateImageTsoRequestRetractionRecord final {
  std::uint64_t messageId = 0;
  std::uint64_t producingFederateId = 0;
  std::optional<std::string> timestampEncoding;
  std::vector<FederationStateImageTsoRequestRetractionRecipient>
      recipientStates;
  bool retractionApplied = false;
  bool terminal = false;
  bool producerResigned = false;
  bool deliveryRequiredAfterConnectionLoss = false;
};

struct FederationStateImageTsoDirectedInteractionRecipient final {
  std::uint64_t receivingFederateId = 0;
  std::uint64_t objectInstanceHandle = 0;
  std::uint64_t receivedInteractionClassHandle = 0;
  std::vector<std::uint64_t> receivedParameterHandles;
};

// Route-free payload for one timestamped directed interaction. Callback
// functions are intentionally absent; the recipient's projected target and
// parameter view are retained so a future rehydration step can rebind live
// routes without changing the accepted send.
struct FederationStateImageTsoDirectedInteractionMessage final {
  std::uint64_t messageId = 0;
  std::uint64_t producingFederateId = 0;
  std::uint64_t objectInstanceHandle = 0;
  std::uint64_t sentInteractionClassHandle = 0;
  std::vector<std::uint64_t> sentParameterHandles;
  std::vector<FederationStateImageInteractionParameter> parameters;
  std::string userSuppliedTag;
  std::string transportationName;
  std::vector<FederationStateImageTsoDirectedInteractionRecipient> recipients;
  std::optional<std::string> timestampEncoding;
  std::uint32_t sentOrderType = 0;
  std::uint32_t receivedOrderType = 0;
};

// A route-free snapshot of one recipient's temporal queue phase.  Phase 0 is
// queued, phase 1 is in transit to a callback, and phase 2 is delivered but
// retained until the recipient's next advance.  The sequence and official
// timestamp preserve the scheduler's ordering without serializing callbacks.
struct FederationStateImageTsoQueueEntry final {
  std::uint64_t messageId = 0;
  std::uint64_t recipientFederateId = 0;
  std::uint64_t sequence = 0;
  std::uint32_t phase = 0;
  std::optional<std::string> timestampEncoding;
};

// One accepted object-instance-name reservation. The reservation is keyed by
// the exact user-supplied wide name and remains owned by the joined federate
// until release, named registration, or resignation.
struct FederationStateImageObjectInstanceNameReservation final {
  std::uint64_t federateId = 0;
  std::wstring objectInstanceName;
};

// Route-free state for one pending synchronization point.  The callback
// routes are intentionally absent; restore binds them from the live joined
// federates while preserving the label, tag, participant set, announcement
// boundary, and first achievement result.
struct FederationStateImageSynchronizationPoint final {
  std::wstring label;
  std::string userSuppliedTag;
  std::vector<std::uint64_t> synchronizationSet;
  std::vector<std::uint64_t> announcedFederates;
  std::vector<std::pair<std::uint64_t, bool>> achievedFederates;
};

struct FederationStateImageObjectClassAttributeSubscription final {
  std::uint64_t attributeHandle = 0;
  bool active = false;
};

struct FederationStateImageRegionalObjectClassAttributeSubscription final {
  std::uint64_t attributeHandle = 0;
  std::uint64_t regionHandle = 0;
  bool active = false;
};

struct FederationStateImageObjectClassAttributeStringValue final {
  std::uint64_t attributeHandle = 0;
  // Empty update-rate designators are meaningful: the C++ service uses them
  // to select the FOM default rate, so the codec must preserve the empty
  // string rather than treating it as a missing declaration.
  std::string value;
};

struct FederationStateImageRegionalObjectClassAttributeStringValue final {
  std::uint64_t attributeHandle = 0;
  std::uint64_t regionHandle = 0;
  // As with ordinary subscriptions, an empty designator selects the FOM
  // default rate and is therefore a valid serialized value.
  std::string value;
};

struct FederationStateImageObjectClassAttributeOrderValue final {
  std::uint64_t attributeHandle = 0;
  std::uint32_t orderType = 0;
};

struct FederationStateImageObjectClassAttributeClass final {
  std::uint64_t objectClassHandle = 0;
  bool privilegeToDeleteExplicitlyUnpublished = false;
  std::vector<std::uint64_t> explicitlyPublishedAttributeHandles;
  std::vector<FederationStateImageObjectClassAttributeSubscription>
      subscribedAttributes;
  std::vector<FederationStateImageObjectClassAttributeStringValue>
      subscribedUpdateRateDesignators;
  std::vector<FederationStateImageRegionalObjectClassAttributeSubscription>
      regionalSubscribedAttributes;
  std::vector<FederationStateImageRegionalObjectClassAttributeStringValue>
      regionalSubscribedUpdateRateDesignators;
  std::vector<FederationStateImageObjectClassAttributeStringValue>
      defaultTransportationTypes;
  std::vector<FederationStateImageObjectClassAttributeOrderValue>
      defaultOrderTypes;
};

struct FederationStateImageObjectClassAttributeDeclarations final {
  std::uint64_t federateId = 0;
  std::uint64_t subscriptionGeneration = 0;
  std::vector<FederationStateImageObjectClassAttributeClass> classes;
};

struct FederationStateImage final {
  static constexpr std::string_view format = "umbra-federation-state/v1";

  std::wstring federationName;
  std::wstring logicalTimeImplementationName;
  std::uint64_t normalizationSeed = 0;
  std::uint32_t federationSwitches = 0;

  // Counts are intentionally named rather than represented by one opaque
  // checksum.  They provide a compatibility fence for the next state-image
  // version and make incomplete v1 coverage visible in persisted artifacts.
  std::uint64_t interactionDeclarationCount = 0;
  std::uint64_t synchronizationPointCount = 0;
  std::uint64_t objectClassDeclarationCount = 0;
  std::uint64_t regionCount = 0;
  std::uint64_t objectInstanceCount = 0;
  std::uint64_t tsoInteractionMessageCount = 0;
  std::uint64_t tsoAttributeUpdateMessageCount = 0;
  std::uint64_t tsoObjectDeletionMessageCount = 0;
  std::uint64_t tsoDirectedInteractionMessageCount = 0;

  std::uint64_t nextRegionHandle = 1;
  std::uint64_t nextSubscriptionGeneration = 1;
  std::uint64_t nextObjectInstanceHandle = 1;
  std::uint64_t nextAttributeOwnershipAcquisitionIfAvailableRequestId = 1;
  std::uint64_t nextAttributeOwnershipAcquisitionRequestId = 1;
  std::uint64_t nextAttributeOwnershipAcquisitionRequestSequence = 1;
  std::uint64_t nextAttributeOwnershipAcquisitionCancellationId = 1;
  std::uint64_t nextAttributeOwnershipDivestitureIfWantedNotificationId = 1;
  std::uint64_t nextConfirmDivestitureNotificationId = 1;
  std::uint64_t nextAttributeTransportationTypeChangeRequestId = 1;
  std::uint64_t nextAttributeValueUpdateRequestId = 1;
  std::uint64_t nextAttributeOwnershipQueryRequestId = 1;
  std::uint64_t nextTimeAdvanceGrantDispatchIdentity = 1;

  std::vector<FederationStateImageMember> members;
  std::vector<std::pair<std::uint64_t, std::wstring>> federateNamesById;
  std::vector<FederationStateImageTimeState> timeStates;
  std::vector<FederationStateImageObject> objects;
  std::vector<FederationStateImagePendingAttributeOwnershipQuery>
      pendingAttributeOwnershipQueries;
  // Decoder-only compatibility marker: older v1 images have no typed query
  // callback section and therefore leave this false.
  bool pendingAttributeOwnershipQueriesPresent = false;
  std::vector<FederationStateImagePendingAttributeOwnershipAssumption>
      pendingAttributeOwnershipAssumptions;
  // Decoder-only compatibility marker: older v1 images carry no typed
  // assumption-callback section.
  bool pendingAttributeOwnershipAssumptionsPresent = false;
  std::vector<FederationStateImageInteractionDeclaration>
      interactionDeclarations;
  std::vector<FederationStateImageTsoInteractionMessage>
      tsoInteractionMessages;
  std::vector<FederationStateImageTsoAttributeUpdateMessage>
      tsoAttributeUpdateMessages;
  std::vector<FederationStateImageTsoObjectDeletionMessage>
      tsoObjectDeletionMessages;
  std::vector<FederationStateImageTsoRequestRetractionRecord>
      tsoRequestRetractionRecords;
  std::vector<FederationStateImageTsoDirectedInteractionMessage>
      tsoDirectedInteractionMessages;
  std::vector<FederationStateImageTsoQueueEntry> tsoQueueEntries;
  std::vector<FederationStateImageObjectInstanceNameReservation>
      reservedObjectInstanceNames;
  // Decoder-only compatibility marker: older v1 images ended immediately
  // after the TSO queue section and therefore carry no reservation section.
  bool reservedObjectInstanceNamesPresent = false;
  std::vector<FederationStateImageSynchronizationPoint>
      synchronizationPoints;
  // Decoder-only compatibility marker: older v1 images have only the
  // aggregate synchronizationPointCount and no typed synchronization section.
  bool synchronizationPointsPresent = false;
  std::vector<FederationStateImageRegion> regions;
  // Decoder-only compatibility marker: older v1 images have only the
  // aggregate regionCount and no typed region section.
  bool regionsPresent = false;
  std::vector<FederationStateImageObjectClassAttributeDeclarations>
      objectClassAttributeDeclarations;
  // Decoder-only compatibility marker: older v1 images have only the
  // aggregate objectClassDeclarationCount and no typed declaration section.
  bool objectClassAttributeDeclarationsPresent = false;
};

class FederationStateImageCodec final {
 public:
  // Produces a canonical ASCII payload.  Text fields and official encoded
  // values are hex escaped so the store can embed the payload in its JSON
  // envelope without a second binary/base64 dependency.
  [[nodiscard]] static std::string encode(FederationStateImage const& image);

  // Rejects unknown versions, duplicate identities, malformed numbers, and
  // trailing data.  Callers must treat an exception as an unusable durable
  // image rather than falling back to an in-memory approximation.
  [[nodiscard]] static FederationStateImage decode(std::string_view payload);
};

}  // namespace umbra::detail
