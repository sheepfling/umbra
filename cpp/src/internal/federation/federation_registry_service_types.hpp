#pragma once

#include "internal/fom/fom_validation.hpp"
#include "internal/observability/mom_service_report_encoding.hpp"
#include "internal/time/federation_time_coordinator.hpp"

#include <RTI/Enums.h>
#include <RTI/Typedefs.h>
#include <RTI/VariableLengthData.h>
#include <RTI/time/LogicalTime.h>

#include <cstddef>
#include <cstdint>
#include <chrono>
#include <functional>
#include <map>
#include <memory>
#include <optional>
#include <set>
#include <string>
#include <string_view>
#include <tuple>
#include <utility>
#include <variant>
#include <vector>

namespace rti1516_2025 {
class FederateAmbassador;
}

namespace umbra::detail {

// A binding-owned callback endpoint is registered in the same private
// membership transaction as its federate.  The registry never calls it while
// locked; the binding queues it onto the recipient's selected callback model.
using FederateCallbackInvocation =
    std::function<void(rti1516_2025::FederateAmbassador&)>;
// A route carries the ordinary callback submission used by RTI-initiated
// control work plus an explicit receive-order submission used by application
// traffic. Keeping the marker at the route boundary lets HLA_ROlength read
// the same C++ queue ledger that the dispatcher drains; it does not infer a
// count from Python callbacks or from payloads.
struct FederateCallbackRoute final {
  std::function<void(FederateCallbackInvocation)> submit;
  std::function<void(FederateCallbackInvocation)> receiveOrderSubmit;
  std::function<std::size_t()> pendingReceiveOrderCount;

  FederateCallbackRoute() = default;

  // Preserve the lightweight callback-route construction used by registry
  // unit tests and older internal callers while allowing the production route
  // to carry receive-order queue instrumentation as additional seams.
  template <typename Callable>
  FederateCallbackRoute(Callable callback)
      : submit(std::move(callback)) {}

  void operator()(FederateCallbackInvocation invocation) const {
    if (submit) {
      submit(std::move(invocation));
    }
  }

  void enqueueReceiveOrder(FederateCallbackInvocation invocation) const {
    if (receiveOrderSubmit) {
      receiveOrderSubmit(std::move(invocation));
    } else if (submit) {
      // Lightweight registry/test routes historically provided only the
      // ordinary submission callback.  Receive-order work must still reach
      // that route rather than being silently discarded; production routes
      // install receiveOrderSubmit so they can add their queue accounting.
      submit(std::move(invocation));
    }
  }

  explicit operator bool() const noexcept {
    return static_cast<bool>(submit);
  }
};

// A private recipient route reserves the recipient's serial and appends the
// supplied fully encoded record before its corresponding callback is exposed.
// A service that requires a callback-time eligibility recheck may retain this
// route with queued work until that delivery boundary. Keeping the encoder
// opaque avoids coupling this federation kernel to the MOM file-text encoder
// while preserving one route for every later RTI-initiated report slice.
using FederateServiceReportRecordEncoder = std::function<std::wstring(std::uint32_t)>;
using FederateServiceReportRoute = std::function<void(
    std::uint16_t serviceGroup,
    FederateServiceReportRecordEncoder encodeRecord)>;

// A live public-report endpoint uses the standard HLAreportServiceInvocation
// interaction rather than the private file sink. Keep the arguments
// structured until the ambassador reserves the recipient serial.
using FederatePublicServiceReportRoute = std::function<void(
    std::wstring const& service,
    MomServiceType serviceType,
    std::vector<MomServiceArgument> const& suppliedArguments,
    MomServiceArgument const& returnedArgument,
    bool success,
    std::wstring const& exception)>;

// A registry-owned pending time advance carries only a private wake-up action.
// The action must enqueue work on the owning ambassador's callback dispatcher;
// it must never invoke a FederateAmbassador while the registry is locked.
using FederationTimeGrantDispatch = std::function<void()>;

// This factory is a live ambassador endpoint, not saved federation state.  It
// recreates a callback-gated grant dispatch when restoring a snapshot that was
// taken while a federate was still Time Advancing.  dispatchIdentity fences a
// reconstituted dispatch from callback work queued before a later restore.
using FederationTimeGrantDispatchFactory = std::function<FederationTimeGrantDispatch(
    std::uint64_t federateId,
    std::uint64_t generation,
    std::uint64_t dispatchIdentity)>;

// Role-enable requests are callback-gated temporal state too. Their callback
// endpoints remain live ambassador concerns, so a restore rebuilds the
// dispatch from this factory after applying the route-free time image.
enum class FederationTimeRoleEnableKind {
  regulation,
  constrained,
};

using FederationTimeRoleEnableDispatchFactory = std::function<FederationTimeGrantDispatch(
    std::uint64_t federateId,
    std::uint64_t generation,
    FederationTimeRoleEnableKind kind)>;

class FomCatalog;
class MaterializedFdd;
struct FederationTimeBounds;

struct FederationDefinition {
  std::vector<PrevalidatedFomModule> fomModules;
  std::wstring logicalTimeImplementationName;
  std::shared_ptr<FomCatalog const> catalog;
  std::shared_ptr<MaterializedFdd const> fdd;
  FomStandardEdition standardEdition = FomStandardEdition::ieee1516_2025;
};

struct FederateMembership {
  std::uint64_t id = 0;
  std::wstring name;
  std::wstring type;
  bool objectClassRelevanceAdvisorySwitch = false;
  bool attributeScopeAdvisorySwitch = false;
  bool attributeRelevanceAdvisorySwitch = false;
  bool interactionRelevanceAdvisorySwitch = false;
  bool conveyRegionDesignatorSetsSwitch = false;
  rti1516_2025::ResignAction automaticResignAction =
      rti1516_2025::CANCEL_THEN_DELETE_THEN_DIVEST;
  bool serviceReportingSwitch = false;
  bool exceptionReportingSwitch = false;
  bool sendServiceReportsToFileSwitch = false;
  // HLAsetTiming stores the target joined federate's wall-clock report
  // period.  Zero means that periodic MOM updates are disabled.  The
  // deadline is runtime state rather than a public API value; keeping it on
  // the membership lets the registry arbitrate one timer per joined-federate
  // MOM object while retaining the standard's target-federate semantics.
  std::int32_t momReportPeriodSeconds = 0;
  std::optional<std::chrono::steady_clock::time_point> nextMomReportAt;
  // HLAreportServiceInvocation uses an HLAcount serial number per joined
  // federate. The next accepted report starts at zero.
  std::uint32_t nextMomServiceReportSerialNumber = 0;
  // HLAupdatesSent counts successful Update Attribute Values service
  // invocations by this joined federate. Keep the counter on membership state
  // so it follows the federate through save/restore without confusing it with
  // the number of individual attribute values or downstream reflections.
  std::uint64_t successfulUpdateAttributeValuesCount = 0;
  // HLArequestUpdatesSent reports accepted update counts by registered class
  // and effective FOM transportation type. The nested map is retained for
  // this joined-federate lifetime, including after an object is removed.
  std::map<std::uint64_t, std::map<std::string, std::uint64_t>>
      successfulUpdateCountsByClassAndTransportation;
  // HLAobjectInstancesUpdated counts distinct object instances for which this
  // joined federate has successfully invoked Update Attribute Values. Keep
  // the object-handle set on the membership lifetime so repeated updates to
  // one instance do not inflate the object-instance count and a later resign
  // naturally starts a fresh set.
  std::set<std::uint64_t> successfullyUpdatedObjectInstanceHandles;
  // HLArequestObjectInstancesUpdated reports counts grouped by the
  // registered object class. Retain the class identity beside the distinct
  // object-instance set so a later deletion cannot erase report history.
  std::map<std::uint64_t, std::uint64_t>
      successfullyUpdatedObjectInstanceClassHandles;
  // HLAobjectInstancesRegistered counts successful Register Object Instance
  // and Register Object Instance with Regions invocations in this joined
  // federate lifetime.
  std::uint64_t successfulObjectInstanceRegistrationsCount = 0;
  // HLAobjectInstancesDeleted counts accepted Delete Object Instance
  // invocations, including timestamped queue admission, in this joined
  // federate lifetime. Retraction does not erase the historical invocation.
  std::uint64_t successfulObjectInstanceDeletionsCount = 0;
  // HLAobjectInstancesRemoved counts committed Remove Object Instance
  // callbacks delivered to this joined federate. A retracted timestamped
  // deletion never reaches this boundary and therefore is not counted.
  std::uint64_t successfulObjectInstanceRemovalsCount = 0;
  // HLAobjectInstancesDiscovered counts committed Discover Object Instance
  // callbacks delivered to this joined federate. A local delete followed by
  // rediscovery therefore contributes another accepted callback.
  std::uint64_t successfulObjectInstanceDiscoveriesCount = 0;
  // HLAreflectionsReceived counts accepted application Reflect Attribute
  // Values callback invocations at this joined federate. RTI-owned MOM
  // reflections use a separate callback path and are intentionally excluded.
  std::uint64_t successfulReflectionsReceivedCount = 0;
  // HLArequestReflectionsReceived reports accepted application reflection
  // callbacks grouped by registered object class and effective
  // transportation type. The callback boundary, rather than sender fan-out,
  // is authoritative for this joined-federate lifetime.
  std::map<std::uint64_t, std::map<std::string, std::uint64_t>>
      successfulReflectionCountsByClassAndTransportation;
  // HLAobjectInstancesReflected counts distinct application object instances
  // for which this joined federate has received an accepted Reflect Attribute
  // Values callback. MOM-owned objects are deliberately excluded.
  std::set<std::uint64_t> successfullyReflectedObjectInstanceHandles;
  // HLArequestObjectInstancesReflected reports distinct reflected instances
  // grouped by registered object class. Retain the class identity at the
  // accepted callback boundary so later deletion cannot erase report history.
  std::map<std::uint64_t, std::uint64_t>
      successfullyReflectedObjectInstanceClassHandles;
  // HLAinteractionsSent counts accepted Send Interaction service invocations
  // by this joined federate. Directed sends are retained separately for the
  // directed-interaction MOM counter while still contributing to the total.
  std::uint64_t successfulInteractionsSentCount = 0;
  std::uint64_t successfulDirectedInteractionsSentCount = 0;
  // HLArequestInteractionsSent reports the accepted sender-side interaction
  // ledger by sent interaction class and effective transportation type. The
  // map is retained for this joined-federate lifetime and includes regional,
  // timestamped, directed, and accepted MOM Send Interaction invocations.
  std::map<std::uint64_t, std::map<std::string, std::uint64_t>>
      successfulInteractionCountsByClassAndTransportation;
  // HLArequestDirectedInteractionsSent reports the directed subset of the
  // accepted sender ledger by sent interaction class and effective
  // transportation type. Keep it separate from the all-interactions ledger,
  // while retaining the same joined-federate lifetime and service boundary.
  std::map<std::uint64_t, std::map<std::string, std::uint64_t>>
      successfulDirectedInteractionCountsByClassAndTransportation;
  // HLAinteractionsReceived and HLAdirectedInteractionsReceived count
  // accepted Receive Interaction callback invocations at this joined
  // federate. Keep the directed subset separate while retaining it in the
  // total interaction-receipt count.
  std::uint64_t successfulInteractionsReceivedCount = 0;
  std::uint64_t successfulDirectedInteractionsReceivedCount = 0;
  // HLArequestInteractionsReceived reports accepted application receive
  // callbacks by sent interaction class and effective transportation. The
  // callback boundary, rather than sender fan-out or queued admission, is
  // authoritative for this joined-federate lifetime.
  std::map<std::uint64_t, std::map<std::string, std::uint64_t>>
      successfulInteractionReceiptCountsByClassAndTransportation;
  // Keep the directed subset separate for the paired directed-receipt MOM
  // report while retaining it in the total receive ledger.
  std::map<std::uint64_t, std::map<std::string, std::uint64_t>>
      successfulDirectedInteractionReceiptCountsByClassAndTransportation;
};

// A federation-owned snapshot captured while the registry holds its member
// lock.  The definition supplies the FDD time declarations and switches;
// each entry supplies the matching joined federate's private time state.  A
// future GALT/LITS scheduler must consume this one view instead of combining
// independently queried membership, FDD, and per-ambassador state.
struct FederationTimeFederateSnapshot {
  FederateMembership membership;
  FederateTimeSnapshot time;
  std::vector<TsoQueuedMessage> queuedTsoMessages;
  std::vector<TsoQueuedMessage> inTransitTsoMessages;
  std::vector<TsoQueuedMessage> deliveredTsoMessagesSinceLastAdvance;
};

struct FederationTimeExecutionSnapshot {
  FederationDefinition definition;
  std::vector<FederationTimeFederateSnapshot> federates;
  // Non-Regulated Grant is a static federation-wide switch captured at
  // federation creation. Keep it outside the replaceable composed definition
  // so an additional-FOM join cannot alter an active execution's time policy.
  bool nonRegulatedGrant = false;
};

struct FederationExecutionSummary {
  std::wstring name;
  std::wstring logicalTimeImplementationName;
};

enum class FederationRegistryStatus {
  applied,
  invalid_request,
  federation_already_exists,
  federation_does_not_exist,
  federates_currently_joined,
  federate_name_already_in_use,
  federate_not_member,
  invalid_resign_action,
  ownership_acquisition_pending,
  federate_owns_attributes,
};

// Set Service Reporting Switch has one service-specific rejection that does
// not belong to the general federation lifecycle vocabulary.  Keeping it
// separate prevents unrelated registry callers from accidentally treating the
// MOM subscription interlock as a generic federation failure.
enum class ServiceReportingSwitchStatus {
  applied,
  federation_does_not_exist,
  federate_not_member,
  report_service_invocations_are_subscribed,
};

// The standard MIM's HLAmanager.HLAfederate.HLAadjust.HLAsetSwitches
// interaction permits a joined federate to provide any subset of these
// parameters.  The public adapter validates the wire encodings; this value
// object lets the registry apply the accepted subset while holding its member
// lock, rather than exposing a homegrown MOM state path beside the ordinary
// switch services.
struct FederateMOMSwitchUpdate {
  std::optional<bool> objectClassRelevanceAdvisory;
  std::optional<bool> attributeRelevanceAdvisory;
  std::optional<bool> attributeScopeAdvisory;
  std::optional<bool> interactionRelevanceAdvisory;
  std::optional<bool> conveyRegionDesignatorSets;
  std::optional<rti1516_2025::ResignAction> automaticResignAction;
  std::optional<bool> serviceReporting;
  std::optional<bool> exceptionReporting;
  std::optional<bool> sendServiceReportsToFile;
};

enum class FederateMOMSwitchUpdateStatus {
  applied,
  federation_does_not_exist,
  federate_not_member,
  invalid_resign_action,
  report_service_invocations_are_subscribed,
};

enum class FederationSaveControlStatus {
  applied,
  federation_does_not_exist,
  federate_not_member,
  save_in_progress,
  restore_in_progress,
  save_not_initiated,
  federate_has_not_begun_save,
  save_not_in_progress,
  callback_route_missing,
  invalid_timed_save,
  inconsistent_temporal_state,
};

// Service operations other than the save/restore control callbacks are
// temporarily unavailable while a federation-wide save or restore is being
// coordinated.  The adapter uses this small read-only view to translate the
// shared operation gate into the official SaveInProgress and
// RestoreInProgress exceptions before it mutates any service state.
enum class FederationServiceOperationStatus {
  available,
  federation_does_not_exist,
  federate_not_member,
  save_in_progress,
  restore_in_progress,
};

enum class FederationSaveNotificationKind {
  initiate,
  completed,
  status,
};

// Immutable save-control callback work. The embedded profile stores a
// process-local snapshot and publishes a route-free versioned state image
// after every participating federate reports completion; distributed
// transport and the remaining typed application-ledger rehydration remain
// separate layers. Typed If Available and regular ownership-acquisition
// reservation ledgers are part of the v1 image; remaining ownership/value
// ledgers remain separate. Pending time-advance and time-role-enable
// generation identities are carried by the route-free temporal image; live
// callback routes remain a separate rebinding concern.
struct FederationSaveNotification {
  FederationSaveNotificationKind kind = FederationSaveNotificationKind::initiate;
  // The joined federate whose callback route receives this notification.  It
  // also identifies the MOM HLAfederate object whose state transition is
  // reflected alongside the save callback.
  std::uint64_t receivingFederateId = 0;
  std::wstring label;
  // Set only for a timestamped Request Federation Save.  The callback must
  // retain the requested save time, not the later grant boundary that admits
  // a constrained federate into the operation.
  std::shared_ptr<rti1516_2025::LogicalTime const> timestamp;
  bool successful = false;
  rti1516_2025::SaveFailureReason failureReason = rti1516_2025::SAVE_ABORTED;
  std::vector<std::pair<std::uint64_t, rti1516_2025::SaveStatus>> statuses;
  FederateCallbackRoute callbackRoute;
  // Each recipient owns an independent report-file selection and serial
  // sequence.  The registry carries the route with queued initiation work so
  // the adapter can append before it exposes the corresponding callback.
  FederateServiceReportRoute serviceReportRoute;
  FederatePublicServiceReportRoute publicServiceReportRoute;
};

struct FederationSaveControlResult {
  FederationSaveControlStatus status = FederationSaveControlStatus::applied;
  std::vector<FederationSaveNotification> notifications;
  // True only when this completion call materialized the federation snapshot
  // and therefore crossed the Federation Saved MOM condition boundary.
  bool saveCompletedSuccessfully = false;
};

// A time-constrained member must receive Initiate Federate Save while it is
// still in Time Advancing. The matching time-grant dispatcher therefore
// invokes that one callback directly at its pre-grant boundary; the registry
// returns the label for that direct invocation plus any ordinary callbacks
// made ready once all constrained members have reached the relevant boundary.
struct FederationSaveAdmission {
  FederationSaveControlStatus status = FederationSaveControlStatus::applied;
  std::optional<std::wstring> currentFederateLabel;
  std::shared_ptr<rti1516_2025::LogicalTime const> currentFederateTimestamp;
  std::vector<FederationSaveNotification> notifications;
};

enum class FederationRestoreControlStatus {
  applied,
  federation_does_not_exist,
  federate_not_member,
  save_in_progress,
  restore_in_progress,
  restore_not_requested,
  restore_not_in_progress,
  snapshot_not_found,
  membership_mismatch,
  callback_route_missing,
};

enum class FederationRestoreNotificationKind {
  request_succeeded,
  request_failed,
  begin,
  initiate,
  completed,
  status,
};

// Immutable restore callback work.  The embedded profile stores the
// completed save as an in-memory federation snapshot and restores it only
// after every current member reports completion.  The post-restore handle is
// currently stable (equal to the pre-restore handle); distributed handle
// remapping and durable on-disk persistence remain later layers.
struct FederationRestoreNotification {
  FederationRestoreNotificationKind kind =
      FederationRestoreNotificationKind::request_failed;
  // The joined federate whose callback route receives this notification.  A
  // restore callback may carry a different pre/post designator pair, so keep
  // the receiving identity explicit for MOM state reflection.
  std::uint64_t receivingFederateId = 0;
  std::wstring label;
  std::wstring federateName;
  std::uint64_t preRestoreFederateId = 0;
  std::uint64_t postRestoreFederateId = 0;
  bool successful = false;
  rti1516_2025::RestoreFailureReason failureReason =
      rti1516_2025::RTI_UNABLE_TO_RESTORE;
  struct StatusRecord {
    std::uint64_t preRestoreFederateId = 0;
    std::uint64_t postRestoreFederateId = 0;
    rti1516_2025::RestoreStatus status = rti1516_2025::NO_RESTORE_IN_PROGRESS;
  };
  std::vector<StatusRecord> statuses;
  FederateCallbackRoute callbackRoute;
  // A restore notification is RTI-initiated at one or more recipients.  Keep
  // that recipient's selected report-file route with the queued work so the
  // adapter can append its Table 5 record before exposing the callback.
  FederateServiceReportRoute serviceReportRoute;
  // The corresponding live public HLAreportServiceInvocation route, when
  // this recipient selected the interaction sink.
  FederatePublicServiceReportRoute publicServiceReportRoute;
};

struct AttributeOwnershipAcquisitionWorkItem;
struct AttributeOwnershipAcquisitionCancellationWorkItem;
struct AttributeOwnershipDivestitureIfWantedNotification;
struct ConfirmDivestitureNotification;
struct AttributeTransportationTypeChangeWorkItem;
struct InteractionTransportationTypeChangeWorkItem;
struct AttributeValueUpdateProvideWorkItem;
struct AttributeValueUpdateClassProvideWorkItem;
struct AttributeValueUpdateRegionalProvideWorkItem;
struct AttributeOwnershipQueryRecipient;
struct AttributeOwnershipAssumptionRecipient;

struct FederationRestoreControlResult {
  FederationRestoreControlStatus status =
      FederationRestoreControlStatus::applied;
  std::vector<FederationRestoreNotification> notifications;
  // Successful restore can reconstitute a saved Time Advance Grant only after
  // the federation-restored callbacks have been submitted to their routes.
  std::vector<FederationTimeGrantDispatch> timeAdvanceGrantDispatches;
  // Pending Enable Time Regulation/Constrained callbacks use the same
  // route-free image boundary. Return live dispatches for every member so a
  // multi-federate completion can rebind each current ambassador, not only
  // the member that called Federate Restore Complete.
  std::vector<FederationTimeGrantDispatch> timeRoleEnableDispatches;
  // A fresh-registry restore has no serialized callback closures. Ownership
  // acquisition reservations are restored as durable state, then their live
  // callback work is rebuilt against the joined ambassadors after the
  // federation-restored notifications are prepared.
  std::vector<AttributeOwnershipAcquisitionWorkItem>
      ownershipAcquisitionWorkItems;
  // Pending Cancel Attribute Ownership Acquisition confirmations are a
  // distinct callback family from the acquisition work-item variants above.
  // Fresh-registry restore rebuilds them against the current requester route.
  std::vector<AttributeOwnershipAcquisitionCancellationWorkItem>
      ownershipAcquisitionCancellationWorkItems;
  // Pending Divestiture If Wanted notifications are rebound to the current
  // requester route only after Federation Restored; the callback boundary
  // remains the one-shot ownership-notification consume point.
  std::vector<AttributeOwnershipDivestitureIfWantedNotification>
      ownershipDivestitureIfWantedWorkItems;
  // Pending Confirm Divestiture notifications are rebound to the current
  // requester route only after Federation Restored. Their callback tag is
  // retained in the route-free ledger and carried into this work item.
  std::vector<ConfirmDivestitureNotification>
      confirmDivestitureWorkItems;
  // Pending attribute transportation-type changes are rebound to the live
  // requester route after a fresh-registry restore; the callback boundary
  // remains the commit point for the effective per-instance type.
  std::vector<AttributeTransportationTypeChangeWorkItem>
      attributeTransportationTypeChangeWorkItems;
  // Pending interaction transportation-type changes are likewise rebound to
  // the current requester route after a fresh-registry restore. The public
  // confirmation callback remains the one-shot commit boundary.
  std::vector<InteractionTransportationTypeChangeWorkItem>
      interactionTransportationTypeChangeWorkItems;
  // Accepted object-instance Request Attribute Value Update provider
  // callbacks are restored from their route-free request ledger and rebound
  // to the current provider routes after Federation Restored callbacks.
  std::vector<AttributeValueUpdateProvideWorkItem>
      attributeValueUpdateProvideWorkItems;
  // Accepted object-class Request Attribute Value Update callbacks are
  // likewise restored from one durable entry per object/provider delivery.
  std::vector<AttributeValueUpdateClassProvideWorkItem>
      attributeValueUpdateClassProvideWorkItems;
  // Accepted regional object-class Request Attribute Value Update callbacks
  // retain their per-attribute region designators across fresh-registry
  // restore and are rebound to current provider routes.
  std::vector<AttributeValueUpdateRegionalProvideWorkItem>
      attributeValueUpdateRegionalProvideWorkItems;
  // Accepted Query Attribute Ownership result callbacks are restored from
  // their route-free request ledger and rebound to the current requester
  // routes after Federation Restored callbacks.
  std::vector<AttributeOwnershipQueryRecipient>
      attributeOwnershipQueryWorkItems;
  // Queued Request Attribute Ownership Assumption callbacks retain their
  // search tuple and tag in the route-free image; fresh-registry restore
  // rebuilds each callback against the current recipient route.
  std::vector<AttributeOwnershipAssumptionRecipient>
      attributeOwnershipAssumptionWorkItems;
};

// Resign-action work is returned as generic callback routes so the registry
// can keep its lock boundary independent of the public object/ownership
// callback record definitions below.  The ambassador converts these records
// to the existing queue-owned types after the federation lock is released.
struct FederationResignObjectRemoval {
  std::uint64_t receivingFederateId = 0;
  std::uint64_t objectInstanceHandle = 0;
  FederateCallbackRoute callbackRoute;
  FederateServiceReportRoute serviceReportRoute;
  bool rtiOwnedMomObject = false;
};

struct FederationResignOwnershipAssumption {
  std::uint64_t receivingFederateId = 0;
  std::uint64_t objectInstanceHandle = 0;
  std::set<std::uint64_t> attributeHandles;
  FederateCallbackRoute callbackRoute;
};

struct ReservedMomServiceReport;

struct FederationRegistryResult {
  FederationRegistryStatus status = FederationRegistryStatus::applied;
  // A successful federate-initiated resignation can be the final reportable
  // service of a joined-federate lifetime.  The member must be removed before
  // the ambassador can complete its lifecycle transition, so reserve this
  // file serial atomically while the membership still exists.  A value is
  // present only when the §11.5 route selected the report-file destination.
  std::optional<std::uint32_t> finalServiceReportFileSerialNumber;
  // A public interaction route is reserved while the resigning member still
  // exists, then queued after membership removal using its prevalidated
  // recipient callback routes and parameter projection.
  std::shared_ptr<ReservedMomServiceReport>
      finalServiceReportInteractionReservation;
  // Resigning a federate can remove the last outstanding member of one or
  // more synchronization sets.  The registry records the resulting
  // Federation Synchronized callbacks here so the adapter can submit them
  // only after releasing the federation lock.
  std::vector<struct FederationSynchronizedNotification>
      synchronizationNotifications;
  std::vector<FederationSaveNotification> saveNotifications;
  std::vector<FederationRestoreNotification> restoreNotifications;
  std::vector<FederationResignObjectRemoval> resignObjectRemovals;
  std::vector<FederationResignOwnershipAssumption> resignOwnershipAssumptions;
  std::vector<AttributeOwnershipAcquisitionWorkItem>
      resignOwnershipAcquisitionWorkItems;
};

struct FederationJoinResult {
  FederationRegistryStatus status = FederationRegistryStatus::applied;
  std::optional<FederateMembership> membership;
};

enum class UpdateRateValueStatus {
  applied,
  federation_does_not_exist,
  federate_not_member,
  invalid_update_rate_designator,
  object_instance_not_known,
  attribute_not_defined,
  inconsistent_catalog,
};

struct UpdateRateValueResult {
  UpdateRateValueStatus status = UpdateRateValueStatus::applied;
  double value = 0.0;
};

// The aliases retain the names used by the existing limited interaction
// slice while making the same callback lifetime boundary available to the
// object-instance discovery foundation.
using InteractionCallbackInvocation = FederateCallbackInvocation;
using InteractionCallbackRoute = FederateCallbackRoute;
using ObjectInstanceCallbackInvocation = FederateCallbackInvocation;
using ObjectInstanceCallbackRoute = FederateCallbackRoute;

// Private synchronization-point delivery records.  The registry owns the
// state transition and returns immutable callback work; the adapter converts
// these records to the official FederateAmbassador callbacks outside the
// registry lock.
struct SynchronizationPointAnnouncement {
  std::uint64_t receivingFederateId = 0;
  std::wstring label;
  std::vector<unsigned char> userSuppliedTag;
  FederateCallbackRoute callbackRoute;
  FederateServiceReportRoute serviceReportRoute;
  FederatePublicServiceReportRoute publicServiceReportRoute;
};

struct FederationSynchronizedNotification {
  std::wstring label;
  std::set<std::uint64_t> failedToSyncFederateIds;
  FederateCallbackRoute callbackRoute;
  FederateServiceReportRoute serviceReportRoute;
  FederatePublicServiceReportRoute publicServiceReportRoute;
  // Retained for process-boundary projection.  Embedded callers continue to
  // use the callback route directly; the process service needs the receiving
  // session identity in order to enqueue the same callback without exposing a
  // callback functor across the socket.
  std::uint64_t receivingFederateId = 0;
};

enum class SynchronizationPointRegistrationStatus {
  applied,
  federation_does_not_exist,
  federate_not_member,
  callback_route_missing,
};

struct SynchronizationPointRegistrationPlan {
  SynchronizationPointRegistrationStatus status =
      SynchronizationPointRegistrationStatus::applied;
  bool succeeded = false;
  std::wstring label;
  std::vector<unsigned char> userSuppliedTag;
  rti1516_2025::SynchronizationPointFailureReason failureReason =
      rti1516_2025::SYNCHRONIZATION_POINT_LABEL_NOT_UNIQUE;
  FederateCallbackRoute registrationCallback;
  std::vector<SynchronizationPointAnnouncement> announcements;
};

enum class SynchronizationPointAchievedStatus {
  applied,
  federation_does_not_exist,
  federate_not_member,
  synchronization_point_label_not_announced,
};

struct SynchronizationPointAchievedPlan {
  SynchronizationPointAchievedStatus status =
      SynchronizationPointAchievedStatus::applied;
  std::vector<FederationSynchronizedNotification> synchronizationNotifications;
};

enum class SynchronizationPointAnnouncementStatus {
  applied,
  federation_does_not_exist,
  federate_not_member,
};

struct SynchronizationPointAnnouncementPlan {
  SynchronizationPointAnnouncementStatus status =
      SynchronizationPointAnnouncementStatus::applied;
  std::vector<SynchronizationPointAnnouncement> announcements;
};

// Private state/result vocabulary for the first 2025 DDM boundary.  Regions
// are federation-owned templates/specifications with pending and committed
// range maps.  Object/attribute association and realization plus broader DDM
// routing remain separate services and do not get inferred from this metadata
// alone; the interaction regional slice below consumes committed specs only.
struct RegionRangeBounds {
  unsigned long lowerBound = 0;
  unsigned long upperBound = 0;
};

// A committed region snapshot is retained only while one Commit Region
// Modifications transaction computes scope transitions. It lets the registry
// compare the pre-commit overlap relation with the new one without exposing
// mutable region state outside its federation lock.
struct RegionSpecificationSnapshot {
  std::set<std::uint64_t> dimensionHandles;
  std::map<std::uint64_t, RegionRangeBounds> committedRangeBounds;
  bool specificationCommitted = false;
};

// Input to the private RTI-owned joined-federate MOM object foundation. It
// intentionally uses validated module records rather than raw Join arguments:
// their canonical source paths let the runtime retain only the first
// designator when a federate supplied the same FOM module more than once.
// This is internal state, not an extension of the public RTI API.
struct JoinedFederateMomObjectDescriptor {
  std::wstring federateHost;
  std::wstring rtiVersion;
  std::vector<PrevalidatedFomModule> fomModulesSpecifiedAtJoin;
  // The embedded production profile supplies the immutable absolute
  // filesystem location allocated at Join. Test-only in-memory report stores
  // never establish this standards-facing object state.
  std::wstring reportServiceFile;
};

enum class JoinedFederateMomObjectStatus {
  applied,
  federation_does_not_exist,
  federate_not_member,
  already_established,
  invalid_descriptor,
  object_instance_handle_exhausted,
  inconsistent_catalog,
};

// A registry-owned representation of the RTI-created MIM object for one
// joined federate. It reserves an ObjectInstanceHandle from the federation's
// common namespace but remains outside the federate-created ObjectInstance map
// so RTI-owned discovery/reflection and ownership state cannot be confused
// with a federate-produced instance.
struct JoinedFederateMomObjectSnapshot {
  std::uint64_t objectInstanceHandle = 0;
  std::uint64_t joinedFederateId = 0;
  std::uint64_t objectClassHandle = 0;
  // HLAreportServiceFile is a static value for the complete joined-federate
  // lifetime. Keep the canonical path as state in its own right instead of
  // relying on a particular encoded attribute-map entry to carry the
  // identity through save/restore.
  std::wstring reportServiceFile;
  // The federation-execution MOM object shares the RTI-owned discovery and
  // reflection machinery with HLAfederate objects, but has execution scope
  // rather than one represented member lifetime.  It is retained in this
  // private snapshot map so the public adapter can keep one callback gate.
  bool federationExecutionObject = false;
  RegionSpecificationSnapshot immutableFederatePoint;
  // Every effective MIM attribute is retained as metadata, including the
  // inherited optional HLAprivilegeToDeleteObject. Required initial values and
  // the bounded direct-request projection for HLAlogicalTime/HLAlookahead/
  // HLAGALT/HLALITS/HLATSOlength/HLAupdatesSent/
  // HLAobjectInstancesThatCanBeDeleted/HLAobjectInstancesUpdated/
  // HLAobjectInstancesRegistered/HLAobjectInstancesDeleted/
  // HLAobjectInstancesRemoved/HLAobjectInstancesDiscovered/
  // HLAobjectInstancesReflected/HLAreflectionsReceived/HLAROlength are
  // encoded through the ordinary reflection planner.
  // HLAsetTiming schedules the catalog-declared Periodic subset at an Evoke
  // boundary or through the
  // embedded HLA_IMMEDIATE scheduler; remaining dynamic values remain later
  // layers.
  std::set<std::uint64_t> effectiveAttributeHandles;
  // MIM updateType metadata is retained as handles so HLAsetTiming can
  // schedule the exact Periodic subset without inventing a second attribute
  // table in the runtime.
  std::set<std::uint64_t> periodicAttributeHandles;
  std::map<std::uint64_t, rti1516_2025::VariableLengthData> initialAttributeValues;
  // The FOM modules supplied at this joined federate's Join, in the same
  // first-designator order used by HLAFOMmoduleDesignatorList.  The content
  // is retained independently of the source path for the public
  // HLArequestFOMmoduleData report path.
  std::vector<std::wstring> fomModuleContents;
  // RTI-owned MOM instances use a separate known-instance ledger. Keeping it
  // on the snapshot preserves the distinction from federate-created object
  // state while retaining the same discovery/reflection lifetime boundary.
  std::set<std::uint64_t> pendingDiscoveryFederateIds;
  std::set<std::uint64_t> pendingRemovalFederateIds;
  std::set<std::uint64_t> knownFederateIds;
};

// One due wall-clock period for an RTI-owned HLAfederate object.  The
// registry advances the target's deadline while holding its federation lock;
// the adapter then uses the ordinary MOM reflection planner to recheck
// subscriptions and queue callbacks without invoking user code under that
// lock.
struct JoinedFederateMomPeriodicUpdate {
  std::uint64_t objectInstanceHandle = 0;
  std::set<std::uint64_t> attributeHandles;
  // Periodic duration values are consumed at the registry-owned deadline so
  // the reflection reports time spent since the preceding update rather than
  // recomputing a later interval in the callback thread.
  std::map<std::uint64_t, rti1516_2025::VariableLengthData> attributeValues;
};

enum class FederateMOMTimingUpdateStatus {
  applied,
  federation_does_not_exist,
  requesting_federate_not_member,
  target_federate_not_member,
  invalid_report_period,
};

// The standard HLAmanager.HLAfederate.HLAadjust.HLAmodifyAttributeState
// interaction is an RTI-owned control path rather than a user-level
// ownership-acquisition service.  Its target is a joined federate selected by
// the inherited HLAfederate parameter; ownership changes are committed
// synchronously and deliberately produce no ownership callbacks.
enum class FederateMOMAttributeStateUpdateStatus {
  applied,
  federation_does_not_exist,
  requesting_federate_not_member,
  target_federate_not_member,
  object_instance_not_known,
  target_does_not_know_object_instance,
  attribute_not_defined,
  target_attribute_not_published,
  attribute_owned_by_rti,
  invalid_attribute_state,
  inconsistent_catalog,
};

enum class RegionServiceStatus {
  applied,
  federation_does_not_exist,
  federate_not_member,
  invalid_dimension,
  invalid_region,
  region_not_created_by_this_federate,
  region_in_use,
  dimension_not_in_region,
  invalid_range_bound,
  incomplete_region,
  inconsistent_catalog,
};

struct RegionCreateResult {
  RegionServiceStatus status = RegionServiceStatus::applied;
  std::uint64_t regionHandle = 0;
};

struct RegionDimensionSetResult {
  RegionServiceStatus status = RegionServiceStatus::applied;
  std::set<std::uint64_t> dimensionHandles;
};

struct RegionRangeBoundsResult {
  RegionServiceStatus status = RegionServiceStatus::applied;
  RegionRangeBounds range;
};

// Private per-federate declaration state for the limited receive-order
// interaction-management slice. The routing kernel consumes this state for
// hierarchy-aware recipient selection and callback delivery. The regional
// interaction declaration state below is deliberately separate from the
// ordinary interaction subscription set, matching the 2025 DDM service rules.
enum class InteractionClassDeclarationStatus {
  applied,
  federation_does_not_exist,
  federate_not_member,
  interaction_class_not_defined,
  federate_service_invocations_are_being_reported_via_mom,
};

struct InteractionClassDeclarationSnapshot {
  bool published = false;
  std::optional<bool> subscriptionActive;
};

// Declaration-management relevance advisories are emitted only when the
// effective relevance of a published class changes.  The registry owns the
// transition calculation; the adapter queues these callback invocations after
// releasing all federation and ambassador locks.
enum class DeclarationAdvisoryKind {
  start_registration_for_object_class,
  stop_registration_for_object_class,
  turn_interactions_on,
  turn_interactions_off,
};

struct DeclarationAdvisory {
  DeclarationAdvisoryKind kind =
      DeclarationAdvisoryKind::start_registration_for_object_class;
  std::uint64_t receivingFederateId = 0;
  std::uint64_t classHandle = 0;
  FederateCallbackRoute callbackRoute;
  // An RTI-initiated declaration advisory is a service at this receiving
  // federate. Carry that federate's selected-file route with the callback
  // work so the adapter can record it before the callback is exposed.
  FederateServiceReportRoute serviceReportRoute;
};

// Regional interaction declarations have a narrower validation vocabulary
// than ordinary interaction declarations. Region ownership, committed
// specification state, and the interaction's available dimensions are
// checked by the registry before a declaration is recorded.
enum class RegionalInteractionClassDeclarationStatus {
  applied,
  federation_does_not_exist,
  federate_not_member,
  interaction_class_not_defined,
  federate_service_invocations_are_being_reported_via_mom,
  invalid_region_context,
  region_not_created_by_this_federate,
  invalid_region,
  inconsistent_catalog,
};

// Private per-federate declaration state for the limited object-management
// profile. It tracks the exact non-region attribute declarations consumed by
// registration, discovery, and receive-order update/reflection. The regional
// declaration state below is kept separate so ordinary subscriptions remain
// independent, as required by the 2025 DDM services.
enum class ObjectClassAttributeDeclarationStatus {
  applied,
  federation_does_not_exist,
  federate_not_member,
  object_class_not_defined,
  attribute_not_defined,
  invalid_update_rate_designator,
  ownership_acquisition_pending,
  inconsistent_catalog,
};

enum class RegionalObjectClassAttributeDeclarationStatus {
  applied,
  federation_does_not_exist,
  federate_not_member,
  object_class_not_defined,
  attribute_not_defined,
  invalid_update_rate_designator,
  invalid_region_context,
  region_not_created_by_this_federate,
  invalid_region,
  inconsistent_catalog,
};

struct ObjectClassAttributeDeclarationSnapshot {
  std::set<std::uint64_t> explicitlyPublishedAttributes;
  std::map<std::uint64_t, bool> subscribedAttributes;
  std::map<std::uint64_t, std::string> subscribedUpdateRateDesignators;
  // Subscription mutations receive a federation-scoped generation so the
  // update-rate gate cannot reuse admission history for a changed
  // declaration.  Retain it in the internal snapshot used by restore tests
  // and callback planning; it is not a public HLA surface.
  std::uint64_t subscriptionGeneration = 0;
};

// Private state/result vocabulary for IEEE 1516.1-2025 object-instance name
// reservation. Reservation is committed before the asynchronous callback is
// delivered; the callback therefore reports the result of the accepted
// reservation attempt rather than acting as the commit point. Named
// registration consumes a reservation in a later object-management slice.
enum class ObjectInstanceNameReservationStatus {
  applied,
  federation_does_not_exist,
  federate_not_member,
  illegal_name,
  name_set_was_empty,
  object_instance_name_not_reserved,
  callback_route_missing,
};

struct ObjectInstanceNameReservationResult {
  ObjectInstanceNameReservationStatus status =
      ObjectInstanceNameReservationStatus::applied;
  bool succeeded = false;
  std::wstring objectInstanceName;
  ObjectInstanceCallbackRoute callbackRoute;
};

struct MultipleObjectInstanceNameReservationResult {
  ObjectInstanceNameReservationStatus status =
      ObjectInstanceNameReservationStatus::applied;
  std::set<std::wstring> succeededNames;
  std::set<std::wstring> failedNames;
  ObjectInstanceCallbackRoute callbackRoute;
};

// Private state/result vocabulary for the limited object-instance
// registration, discovery, receive-order update/reflection, and receive-order
// deletion path. It represents the 2025 unnamed and reservation-consuming
// named registration paths, registration-established attribute ownership,
// candidate discovery promotion, known-instance state, no-time passels, and
// no-time removal. Timestamped/retraction delivery is owned by the separate
// TSO payload records below; local delete, ownership transfer, update-rate
// reduction, and the remaining DDM services remain distinct work. The
// adjacent name-reservation service owns the reservation map consumed by
// named registration.
enum class ObjectInstanceRegistrationStatus {
  applied,
  federation_does_not_exist,
  federate_not_member,
  object_class_not_defined,
  object_class_not_published,
  object_instance_name_in_use,
  object_instance_name_not_reserved,
  attribute_not_published,
  attribute_not_defined,
  invalid_region_context,
  region_not_created_by_this_federate,
  invalid_region,
  object_instance_handle_exhausted,
  inconsistent_catalog,
};

enum class ObjectInstanceRegionAssociationStatus {
  applied,
  federation_does_not_exist,
  federate_not_member,
  object_instance_not_known,
  attribute_not_defined,
  invalid_region_context,
  region_not_created_by_this_federate,
  invalid_region,
  inconsistent_catalog,
};

struct ObjectInstanceRegistrationResult {
  ObjectInstanceRegistrationStatus status = ObjectInstanceRegistrationStatus::applied;
  std::uint64_t objectInstanceHandle = 0;
  std::wstring objectInstanceName;
};

struct ObjectInstanceDiscoveryRecipient {
  std::uint64_t receivingFederateId = 0;
  std::uint64_t objectInstanceHandle = 0;
  std::uint64_t discoveredObjectClassHandle = 0;
  std::wstring objectInstanceName;
  std::uint64_t producingFederateId = 0;
  ObjectInstanceCallbackRoute callbackRoute;
  // §6.9 must not report a discovery that a callback-time recheck cancels, so
  // retain the recipient-local route until actual callback delivery.
  FederateServiceReportRoute serviceReportRoute;
  // RTI-owned MOM instances use a separate registry ledger and the default-
  // invalid callback producer handle. Ordinary instances leave this false.
  bool rtiOwnedMomObject = false;
};

// Private result for the bounded 2025 Attribute Scope Advisory path. A
// recipient is emitted only for attributes whose calculated scope changed for
// a federate that already knows the object instance, whether the cause was a
// committed region, an update association, or a subscription declaration.
// The adapter queues the matching official callback and rechecks the current
// scope at callback entry.
struct ObjectInstanceScopeChangeRecipient {
  std::uint64_t receivingFederateId = 0;
  std::uint64_t objectInstanceHandle = 0;
  std::set<std::uint64_t> attributeHandles;
  bool inScope = false;
  ObjectInstanceCallbackRoute callbackRoute;
};

// Private result for the bounded 2025 Attribute Relevance Advisory path. A
// recipient is emitted for each owner/receiver/object/direction combination
// whose calculated relevance changed, or whose maximum applicable update rate
// changed while it remained relevant. The adapter invokes the official Turn
// Updates On/Off callback at the owning federate, then rechecks ownership,
// scope, and the owner's advisory switch at callback entry.
struct AttributeRelevanceAdvisoryRecipient {
  std::uint64_t providingFederateId = 0;
  std::uint64_t receivingFederateId = 0;
  std::uint64_t objectInstanceHandle = 0;
  std::set<std::uint64_t> attributeHandles;
  bool turnUpdatesOn = false;
  // A null value selects the legacy no-rate callback overload.  A non-null
  // value is the normalized FDD designator explicitly retained by the
  // receiving subscription, including an explicit HLAdefault designator.
  std::optional<std::string> updateRateDesignator;
  ObjectInstanceCallbackRoute callbackRoute;
};

struct RegionScopeChangePlan {
  RegionServiceStatus status = RegionServiceStatus::applied;
  // A committed region can make an already-registered object enter a
  // receiver's scope for the first time.  Discovery must be planned in the
  // same transaction as the region mutation so callback delivery sees the
  // new specification rather than a later, unrelated declaration event.
  std::vector<ObjectInstanceDiscoveryRecipient> discoveries;
  std::vector<ObjectInstanceScopeChangeRecipient> recipients;
  std::vector<AttributeRelevanceAdvisoryRecipient> attributeRelevanceAdvisories;
};

struct ObjectInstanceRegionAssociationScopePlan {
  ObjectInstanceRegionAssociationStatus status =
      ObjectInstanceRegionAssociationStatus::applied;
  // Adding an update-region association can make an already-registered object
  // discoverable to an existing regional subscriber.  Keep those reservations
  // in the same transaction as the association mutation.
  std::vector<ObjectInstanceDiscoveryRecipient> discoveries;
  std::vector<ObjectInstanceScopeChangeRecipient> recipients;
  std::vector<AttributeRelevanceAdvisoryRecipient> attributeRelevanceAdvisories;
};

struct ObjectClassAttributeSubscriptionScopePlan {
  ObjectClassAttributeDeclarationStatus status =
      ObjectClassAttributeDeclarationStatus::applied;
  std::vector<ObjectInstanceScopeChangeRecipient> recipients;
  std::vector<AttributeRelevanceAdvisoryRecipient> attributeRelevanceAdvisories;
};

struct RegionalObjectClassAttributeSubscriptionScopePlan {
  RegionalObjectClassAttributeDeclarationStatus status =
      RegionalObjectClassAttributeDeclarationStatus::applied;
  std::vector<ObjectInstanceScopeChangeRecipient> recipients;
  std::vector<AttributeRelevanceAdvisoryRecipient> attributeRelevanceAdvisories;
};

struct KnownObjectInstanceSnapshot {
  std::uint64_t objectInstanceHandle = 0;
  std::uint64_t knownObjectClassHandle = 0;
  std::wstring objectInstanceName;
  std::uint64_t producingFederateId = 0;
  // Populated for an RTI-owned joined-federate MOM discovery so the adapter
  // can request the complete required initial-value set at callback time.
  std::set<std::uint64_t> initialAttributeHandles;
};

struct ObjectInstanceInitialAttributeReflection final {
  std::string transportationName;
  std::map<std::uint64_t, rti1516_2025::VariableLengthData> attributeValues;
};

// The receive-order Delete Object Instance path owns no timestamp/retraction
// state. The registry accepts deletion only after it has verified that the
// invoking federate is known to own HLAprivilegeToDeleteObject, then reserves
// one no-time Remove Object Instance callback for every other known recipient.
enum class ObjectInstanceDeletionStatus {
  applied,
  federation_does_not_exist,
  federate_not_member,
  object_instance_not_known,
  delete_privilege_not_held,
  inconsistent_catalog,
};

struct ObjectInstanceRemovalRecipient {
  std::uint64_t receivingFederateId = 0;
  std::uint64_t objectInstanceHandle = 0;
  ObjectInstanceCallbackRoute callbackRoute;
  // §6.17 has a callback-time removal boundary. Retain the receiving
  // federate's report route with the queued work so a cancelled removal does
  // not consume a report-file serial or leave a stale record.
  FederateServiceReportRoute serviceReportRoute;
  // RTI-owned MOM objects do not use the federate-created ownership/deletion
  // state machine.
  bool rtiOwnedMomObject = false;
  // Timestamped projections may cross either the immediate receive boundary
  // or a time-constrained grant boundary. Keep the accepted order pair with
  // the planned callback; ordinary removals retain RECEIVE/RECEIVE defaults.
  rti1516_2025::OrderType sentOrderType = rti1516_2025::RECEIVE;
  rti1516_2025::OrderType receivedOrderType = rti1516_2025::RECEIVE;
};

struct ObjectInstanceDeletionPlan {
  ObjectInstanceDeletionStatus status = ObjectInstanceDeletionStatus::applied;
  std::vector<ObjectInstanceRemovalRecipient> recipients;
  // HLAprivilegeToDeleteObject is a timestamp-ordered MIM attribute in the
  // standard catalog, but a loaded FOM may select receive order.  Keep the
  // effective order with the immutable plan so every service boundary makes
  // the same retraction/timestamp decision without re-reading mutable state.
  rti1516_2025::OrderType preferredOrderType = rti1516_2025::RECEIVE;
};

struct RemovedObjectInstanceSnapshot {
  std::uint64_t objectInstanceHandle = 0;
  std::uint64_t producingFederateId = 0;
};

// The local-delete service changes only the invoking federate's known-instance
// state. The federation-wide object, its ownership, and every other federate's
// known-instance state remain intact. A pending ownership acquisition or any
// attribute still owned by the invoking federate blocks the transition.
enum class LocalObjectInstanceDeletionStatus {
  applied,
  federation_does_not_exist,
  federate_not_member,
  object_instance_not_known,
  ownership_acquisition_pending,
  federate_owns_attributes,
};

// Private immutable routing result for the no-time Update Attribute Values
// foundation. A passel contains the submitted attributes that share a FOM
// transportation type and one explicit sent-region set. Recipient projections
// preserve that passel identity while selecting ordinary subscriptions and
// regional subscriptions whose committed regions overlap.
enum class ReceiveOrderAttributeUpdateStatus {
  applied,
  federation_does_not_exist,
  producing_federate_not_member,
  object_instance_not_known,
  attribute_not_owned,
  attribute_not_defined,
  inconsistent_catalog,
};

struct ReceiveOrderAttributeUpdateRecipient {
  std::uint64_t federateId = 0;
  std::set<std::uint64_t> receivedAttributeHandles;
  // Only attributes with an explicit FDD update-rate designator appear in
  // this map.  An absent entry is the HLAdefault/no-reduction case; keeping
  // that distinction per attribute prevents one reduced attribute from
  // throttling another attribute in the same callback passel.
  std::map<std::uint64_t, double> maximumUpdateRatesByAttribute;
  std::uint64_t subscriptionGeneration = 0;
  // The receiver's Convey Region Designator Sets switch is projected at the
  // same callback-time fence as the subscription.  Regional routing still
  // uses the sent regions, but the adapter must omit the optional callback
  // metadata when this policy is disabled.
  bool conveyRegionDesignatorSets = false;
  ObjectInstanceCallbackRoute callbackRoute;
};

struct ReceiveOrderAttributeUpdatePassel {
  std::string transportationName;
  std::vector<std::uint64_t> sentAttributeHandles;
  std::set<std::uint64_t> sentRegionHandles;
  // Regional delivery is admitted against the producer's committed source
  // realization at the Update Attribute Values boundary.  Keep those
  // specifications with the passel so a later source-range mutation cannot
  // reinterpret an already accepted callback.  The live association is still
  // rechecked at delivery, which keeps association replacement suppressive.
  std::map<std::uint64_t, RegionSpecificationSnapshot> sentRegionSnapshots;
  // The 2025 default region is intentionally not exposed as a RegionHandle.
  // Preserve its use separately so an enabled Convey Region Designator Sets
  // switch can distinguish a default-region callback (an empty supplied set)
  // from a callback for data with no available dimensions.
  bool defaultRegionUsed = false;
  rti1516_2025::OrderType preferredOrderType = rti1516_2025::RECEIVE;
  std::vector<ReceiveOrderAttributeUpdateRecipient> recipients;
};

struct ReceiveOrderAttributeUpdatePlan {
  ReceiveOrderAttributeUpdateStatus status = ReceiveOrderAttributeUpdateStatus::applied;
  std::uint64_t registeredObjectClassHandle = 0;
  std::vector<ReceiveOrderAttributeUpdatePassel> passels;
};

// Private immutable routing result for the object-instance Request Attribute
// Value Update foundation. It asks only current non-requesting owners to
// provide a group of requested attributes. The class-level regional form is
// represented by the adjacent planner; automatic provision and ownership
// transfer remain separate work.
enum class AttributeValueUpdateRequestStatus {
  applied,
  federation_does_not_exist,
  requesting_federate_not_member,
  object_instance_not_known,
  attribute_not_defined,
  inconsistent_catalog,
};

struct AttributeValueUpdateProvideRecipient {
  std::uint64_t objectInstanceHandle = 0;
  std::uint64_t providingFederateId = 0;
  std::set<std::uint64_t> requestedAttributeHandles;
  ObjectInstanceCallbackRoute callbackRoute;
  // §6.22 is RTI-initiated at the providing federate. Keep that federate's
  // selected report-file route with queued work until its callback-time
  // eligibility recheck succeeds, so cancelled provides consume no serial.
  FederateServiceReportRoute serviceReportRoute;
};

struct AttributeValueUpdateRequestPlan {
  AttributeValueUpdateRequestStatus status = AttributeValueUpdateRequestStatus::applied;
  std::vector<AttributeValueUpdateProvideRecipient> recipients;
};

// Fresh-registry restore work for an accepted object-instance Request
// Attribute Value Update. The callback route and service-report route are
// rebound from the current joined federates; the copied tag and request
// identity remain durable application state.
struct AttributeValueUpdateProvideWorkItem {
  std::uint64_t requestId = 0;
  std::uint64_t requestingFederateId = 0;
  std::uint64_t providingFederateId = 0;
  std::uint64_t objectInstanceHandle = 0;
  std::set<std::uint64_t> requestedAttributeHandles;
  std::vector<unsigned char> userSuppliedTag;
  ObjectInstanceCallbackRoute callbackRoute;
  FederateServiceReportRoute serviceReportRoute;
};

// Fresh-registry restore work for one accepted object-class Request Attribute
// Value Update provider callback.  The requested class is retained so the
// callback-time hierarchy/ownership recheck cannot silently widen the
// original class-designator request.
struct AttributeValueUpdateClassProvideWorkItem {
  std::uint64_t requestId = 0;
  std::uint64_t requestingFederateId = 0;
  std::uint64_t providingFederateId = 0;
  std::uint64_t objectInstanceHandle = 0;
  std::uint64_t requestedObjectClassHandle = 0;
  std::set<std::uint64_t> requestedAttributeHandles;
  std::vector<unsigned char> userSuppliedTag;
  ObjectInstanceCallbackRoute callbackRoute;
  FederateServiceReportRoute serviceReportRoute;
};

struct AttributeValueUpdateRegionalProvideWorkItem {
  std::uint64_t requestId = 0;
  std::uint64_t requestingFederateId = 0;
  std::uint64_t providingFederateId = 0;
  std::uint64_t objectInstanceHandle = 0;
  std::uint64_t requestedObjectClassHandle = 0;
  std::set<std::uint64_t> requestedAttributeHandles;
  std::map<std::uint64_t, std::set<std::uint64_t>> requestRegionsByAttribute;
  std::vector<unsigned char> userSuppliedTag;
  ObjectInstanceCallbackRoute callbackRoute;
  FederateServiceReportRoute serviceReportRoute;
};

// RTI-owned joined-federate MOM attributes are supplied directly by the RTI;
// they never induce a Provide Attribute Value Update callback at a joined
// federate. This private result carries the recipient route and the immutable
// initial values through the same callback-time revalidation boundary used by
// ordinary reflection.
enum class JoinedFederateMomAttributeValueUpdateStatus {
  applied,
  not_rti_owned_object,
  federation_does_not_exist,
  requesting_federate_not_member,
  object_instance_not_known,
  attribute_not_defined,
  inconsistent_catalog,
};

struct JoinedFederateMomAttributeValueUpdateRecipient {
  std::uint64_t receivingFederateId = 0;
  std::uint64_t objectInstanceHandle = 0;
  std::map<std::uint64_t, rti1516_2025::VariableLengthData> attributeValues;
  ObjectInstanceCallbackRoute callbackRoute;
};

struct JoinedFederateMomAttributeValueUpdatePlan {
  JoinedFederateMomAttributeValueUpdateStatus status =
      JoinedFederateMomAttributeValueUpdateStatus::not_rti_owned_object;
  bool rtiOwnedMomObject = false;
  std::optional<JoinedFederateMomAttributeValueUpdateRecipient> recipient;
};

struct JoinedFederateMomAttributeValueUpdateClassPlan {
  bool rtiOwnedMomObject = false;
  std::vector<JoinedFederateMomAttributeValueUpdateRecipient> recipients;
};

// Private immutable routing result for the object-class Request Attribute
// Value Update foundation. It expands only currently registered instances at
// the requested class or one of its subclasses, then groups owned requested
// attributes by provider and object instance. The optional request-region map
// is used by the 2025 regional form to retain only attribute instances whose
// update-region association overlaps the corresponding request regions;
// missing associations represent the default region and remain eligible.
enum class AttributeValueUpdateClassRequestStatus {
  applied,
  federation_does_not_exist,
  requesting_federate_not_member,
  object_class_not_defined,
  attribute_not_defined,
  invalid_region_context,
  region_not_created_by_this_federate,
  invalid_region,
  inconsistent_catalog,
};

struct AttributeValueUpdateClassRequestPlan {
  AttributeValueUpdateClassRequestStatus status =
      AttributeValueUpdateClassRequestStatus::applied;
  std::vector<AttributeValueUpdateProvideRecipient> recipients;
};

// Private immutable routing result for the 2025 Query Attribute Ownership
// foundation. Federate-created instances report a joined-federate owner or an
// available attribute. RTI-created joined-federate MOM instances use the
// explicit RTI-owned report kind; no sentinel federate handle is synthesized.
enum class AttributeOwnershipQueryStatus {
  applied,
  federation_does_not_exist,
  requesting_federate_not_member,
  object_instance_not_known,
  attribute_not_defined,
  inconsistent_catalog,
};

enum class AttributeOwnershipQueryReportKind {
  federate,
  unowned,
  rti,
};

struct AttributeOwnershipQueryRecipient {
  std::uint64_t requestId = 0;
  std::uint64_t receivingFederateId = 0;
  std::uint64_t objectInstanceHandle = 0;
  AttributeOwnershipQueryReportKind reportKind = AttributeOwnershipQueryReportKind::unowned;
  std::uint64_t owningFederateId = 0;
  std::set<std::uint64_t> attributeHandles;
  ObjectInstanceCallbackRoute callbackRoute;
};

struct AttributeOwnershipQueryPlan {
  AttributeOwnershipQueryStatus status = AttributeOwnershipQueryStatus::applied;
  std::vector<AttributeOwnershipQueryRecipient> recipients;
};

// Private read-only result for the adjacent Is Attribute Owned By Federate
// service. Unlike Query Attribute Ownership, this answer concerns only the
// invoking federate's ownership and therefore stays truthful even while the
// broader ownership transfer lifecycle remains out of scope.
enum class AttributeOwnershipCheckStatus {
  applied,
  federation_does_not_exist,
  requesting_federate_not_member,
  object_instance_not_known,
  attribute_not_defined,
  inconsistent_catalog,
};

struct AttributeOwnershipCheckResult {
  AttributeOwnershipCheckStatus status = AttributeOwnershipCheckStatus::applied;
  bool ownedByRequestingFederate = false;
};

// Private state/result vocabulary for the 2025 Attribute Ownership
// Acquisition If Available service. A successful request establishes the
// invoking federate's Willing to Acquire condition privately until the RTI
// resolves it through exactly one matching acquisition-notification or
// ownership-unavailable callback. Negotiated acquisition, divestiture, and
// RTI-owned state remain separate ownership-management work.
enum class AttributeOwnershipAcquisitionIfAvailableStatus {
  applied,
  federation_does_not_exist,
  requesting_federate_not_member,
  object_instance_not_known,
  attribute_owned_by_rti,
  attribute_already_being_acquired,
  attribute_not_published,
  object_class_not_published,
  federate_owns_attributes,
  attribute_not_defined,
  inconsistent_catalog,
};

struct AttributeOwnershipAcquisitionIfAvailablePlan {
  AttributeOwnershipAcquisitionIfAvailableStatus status =
      AttributeOwnershipAcquisitionIfAvailableStatus::applied;
  std::uint64_t requestId = 0;
  ObjectInstanceCallbackRoute callbackRoute;
};

// The callback invocation boundary resolves all attributes from one accepted
// request against the current federation state. A mixed request may therefore
// deliver the two standard callback forms, each with its corresponding subset.
struct AttributeOwnershipAcquisitionIfAvailableDelivery {
  std::uint64_t objectInstanceHandle = 0;
  std::set<std::uint64_t> securedAttributeHandles;
  std::set<std::uint64_t> unavailableAttributeHandles;
};

// Private state/result vocabulary for the regular 2025 Attribute Ownership
// Acquisition service.  A successful request enters the invoking federate's
// Acquisition Pending state.  The limited embedded profile covers its
// unowned-acquisition notification, the owner-side release request, and the
// owner-side release-denied terminal path. Cancellation remains a separate
// terminal callback state so publication cannot be withdrawn before its
// confirmation callback begins.
enum class AttributeOwnershipAcquisitionStatus {
  applied,
  federation_does_not_exist,
  requesting_federate_not_member,
  object_instance_not_known,
  attribute_owned_by_rti,
  attribute_not_published,
  object_class_not_published,
  federate_owns_attributes,
  attribute_not_defined,
  inconsistent_catalog,
};

enum class AttributeOwnershipAcquisitionWorkKind {
  acquisition_notification,
  // A saved Willing-to-Acquire request has an already-accepted callback
  // boundary but no owner-side release work.  Fresh-registry restore uses
  // this kind to rebind that callback to the current requester route.
  if_available_notification,
  request_release,
  request_divestiture_confirmation,
};

// A registry-generated callback work item always retains the user tag from
// the original acquisition.  The adapter submits it only after all registry
// and invoking-ambassador locks have been released.
struct AttributeOwnershipAcquisitionWorkItem {
  AttributeOwnershipAcquisitionWorkKind kind =
      AttributeOwnershipAcquisitionWorkKind::acquisition_notification;
  std::uint64_t requestingFederateId = 0;
  std::uint64_t receivingFederateId = 0;
  std::uint64_t objectInstanceHandle = 0;
  std::uint64_t requestId = 0;
  std::set<std::uint64_t> attributeHandles;
  std::vector<unsigned char> userSuppliedTag;
  // Request Divestiture Confirmation may be driven by either the regular
  // Acquisition Pending state or the distinct Willing-to-Acquire state. The
  // request ID namespaces are private and independent, so retain the state
  // kind with the queued callback rather than conflating equal numeric IDs.
  bool candidateIsIfAvailable = false;
  ObjectInstanceCallbackRoute callbackRoute;
};

struct AttributeOwnershipAcquisitionPlan {
  AttributeOwnershipAcquisitionStatus status =
      AttributeOwnershipAcquisitionStatus::applied;
  std::vector<AttributeOwnershipAcquisitionWorkItem> workItems;
};

// Private state/result vocabulary for the related 2025 Negotiated Attribute
// Ownership Divestiture / Request Divestiture Confirmation / Confirm
// Divestiture transition. The bounded profile preserves an owner while the
// attribute is waiting, selects the earliest currently pending regular or
// Willing-to-Acquire candidate, and changes ownership only after the owner
// confirms. Continuing owner search and negotiated acquisition remain separate
// work.
enum class NegotiatedAttributeOwnershipDivestitureStatus {
  applied,
  federation_does_not_exist,
  divesting_federate_not_member,
  object_instance_not_known,
  attribute_owned_by_rti,
  attribute_not_owned,
  attribute_not_defined,
  attribute_already_being_divested,
  inconsistent_catalog,
};

struct NegotiatedAttributeOwnershipDivestiturePlan {
  NegotiatedAttributeOwnershipDivestitureStatus status =
      NegotiatedAttributeOwnershipDivestitureStatus::applied;
  std::vector<AttributeOwnershipAcquisitionWorkItem> workItems;
  // A negotiated offer also starts the standard Request Attribute Ownership
  // Assumption search for currently eligible federates.  Keep those callback
  // records beside the acquisition work so the ambassador can preserve the
  // service-before-callback ordering without exposing a second public path.
  std::vector<AttributeOwnershipAssumptionRecipient> assumptionRecipients;
};

struct RequestDivestitureConfirmationDelivery {
  std::uint64_t objectInstanceHandle = 0;
  std::set<std::uint64_t> releasedAttributeHandles;
};

enum class ConfirmDivestitureStatus {
  applied,
  federation_does_not_exist,
  divesting_federate_not_member,
  object_instance_not_known,
  attribute_owned_by_rti,
  attribute_not_owned,
  attribute_not_defined,
  attribute_divestiture_was_not_requested,
  no_acquisition_pending,
  inconsistent_catalog,
};

struct ConfirmDivestitureNotification {
  std::uint64_t notificationId = 0;
  std::uint64_t receivingFederateId = 0;
  std::uint64_t objectInstanceHandle = 0;
  std::set<std::uint64_t> attributeHandles;
  std::vector<unsigned char> userSuppliedTag;
  ObjectInstanceCallbackRoute callbackRoute;
};

struct ConfirmDivestiturePlan {
  ConfirmDivestitureStatus status = ConfirmDivestitureStatus::applied;
  std::vector<ConfirmDivestitureNotification> notifications;
};

struct ConfirmDivestitureNotificationDelivery {
  std::uint64_t objectInstanceHandle = 0;
  std::set<std::uint64_t> securedAttributeHandles;
  std::vector<AttributeOwnershipAcquisitionWorkItem> followupWorkItems;
};

enum class CancelNegotiatedAttributeOwnershipDivestitureStatus {
  applied,
  federation_does_not_exist,
  divesting_federate_not_member,
  object_instance_not_known,
  attribute_owned_by_rti,
  attribute_not_owned,
  attribute_not_defined,
  attribute_divestiture_was_not_requested,
  inconsistent_catalog,
};

struct CancelNegotiatedAttributeOwnershipDivestiturePlan {
  CancelNegotiatedAttributeOwnershipDivestitureStatus status =
      CancelNegotiatedAttributeOwnershipDivestitureStatus::applied;
  std::vector<AttributeOwnershipAcquisitionWorkItem> followupWorkItems;
};

// The notification boundary commits only the attributes that remain unowned
// and are next in the private regular-acquisition order.  It can also expose
// follow-up work after the notification has reached user code, allowing a
// subsequently pending acquisition to ask the new owner for release without
// observing ownership before its own notification.
struct AttributeOwnershipAcquisitionNotificationDelivery {
  std::uint64_t objectInstanceHandle = 0;
  std::set<std::uint64_t> securedAttributeHandles;
  std::vector<AttributeOwnershipAcquisitionWorkItem> followupWorkItems;
};

struct AttributeOwnershipAcquisitionReleaseDelivery {
  std::uint64_t objectInstanceHandle = 0;
  std::set<std::uint64_t> candidateAttributeHandles;
};

// Private validation/routing result for the owner-side Attribute Ownership
// Release Denied service.  It never changes ownership, but ends every
// matching regular pending acquisition and groups its required unavailable
// callbacks by acquiring federate.
enum class AttributeOwnershipReleaseDeniedStatus {
  applied,
  federation_does_not_exist,
  owning_federate_not_member,
  object_instance_not_known,
  attribute_owned_by_rti,
  attribute_not_owned,
  attribute_not_defined,
  inconsistent_catalog,
};

struct AttributeOwnershipUnavailableRecipient {
  std::uint64_t receivingFederateId = 0;
  std::uint64_t objectInstanceHandle = 0;
  std::set<std::uint64_t> attributeHandles;
  ObjectInstanceCallbackRoute callbackRoute;
};

struct AttributeOwnershipReleaseDeniedPlan {
  AttributeOwnershipReleaseDeniedStatus status =
      AttributeOwnershipReleaseDeniedStatus::applied;
  std::vector<AttributeOwnershipUnavailableRecipient> recipients;
  std::vector<AttributeOwnershipAcquisitionWorkItem> followupWorkItems;
};

// Private validation/routing result for the 2025 Attribute Ownership
// Divestiture If Wanted service. The service is synchronous from the
// divesting federate's perspective: an attribute appears in the returned set
// only after ownership has already moved to an eligible pending acquirer.
// The paired notification is still callback-gated, so its private reservation
// preserves the new owner's required publication until callback entry.
enum class AttributeOwnershipDivestitureIfWantedStatus {
  applied,
  federation_does_not_exist,
  divesting_federate_not_member,
  object_instance_not_known,
  attribute_owned_by_rti,
  attribute_not_owned,
  attribute_not_defined,
  inconsistent_catalog,
};

struct AttributeOwnershipDivestitureIfWantedNotification {
  std::uint64_t notificationId = 0;
  std::uint64_t receivingFederateId = 0;
  std::uint64_t objectInstanceHandle = 0;
  std::set<std::uint64_t> attributeHandles;
  std::vector<unsigned char> userSuppliedTag;
  ObjectInstanceCallbackRoute callbackRoute;
};

struct AttributeOwnershipDivestitureIfWantedPlan {
  AttributeOwnershipDivestitureIfWantedStatus status =
      AttributeOwnershipDivestitureIfWantedStatus::applied;
  std::set<std::uint64_t> divestedAttributeHandles;
  std::vector<AttributeOwnershipDivestitureIfWantedNotification> notifications;
};

struct AttributeOwnershipDivestitureIfWantedDelivery {
  std::uint64_t objectInstanceHandle = 0;
  std::set<std::uint64_t> securedAttributeHandles;
  std::vector<AttributeOwnershipAcquisitionWorkItem> followupWorkItems;
};

// Private state/result vocabulary for the 2025 Unconditional Attribute
// Ownership Divestiture service.  This service immediately leaves every
// validated attribute unowned.  The embedded profile then continues already
// pending regular/If Available acquisition work and asks each currently
// eligible, non-pending federate whether it wishes to assume ownership.  The
// resulting callback is a request only; a recipient must still invoke one of
// the standard acquisition services to become an owner.
enum class UnconditionalAttributeOwnershipDivestitureStatus {
  applied,
  federation_does_not_exist,
  divesting_federate_not_member,
  object_instance_not_known,
  attribute_owned_by_rti,
  attribute_not_owned,
  attribute_not_defined,
  inconsistent_catalog,
};

struct AttributeOwnershipAssumptionRecipient {
  std::uint64_t receivingFederateId = 0;
  std::uint64_t objectInstanceHandle = 0;
  std::set<std::uint64_t> attributeHandles;
  ObjectInstanceCallbackRoute callbackRoute;
  // A continuation search can outlive the original service call. Preserve
  // the originating unconditional- or negotiated-divestiture tag per grouped callback so a later
  // publication/discovery re-offer remains standards-visible.
  std::vector<unsigned char> userSuppliedTag;
};

// The callback-entry recheck reports only attributes that remain eligible at
// the recipient's known class and remain outside both acquisition-pending
// states. Unconditional offers require the attribute to be unowned; a
// negotiated offer may retain its current owner until confirmation. A changed
// lifecycle or ownership boundary becomes an ordinary no-delivery outcome.
struct AttributeOwnershipAssumptionDelivery {
  std::uint64_t objectInstanceHandle = 0;
  std::set<std::uint64_t> attributeHandles;
};

// A queued Request Attribute Ownership Assumption callback is a live route
// around a durable search reservation. Keep this callback identity separate
// from ownershipAssumptionRecipientsByAttribute: that map records candidates
// already offered during the current divestiture interval, while this vector records only
// callback work that has not reached its one-shot begin boundary yet.
struct PendingAttributeOwnershipAssumptionCallback {
  std::uint64_t receivingFederateId = 0;
  std::set<std::uint64_t> attributeHandles;
  std::vector<unsigned char> userSuppliedTag;
};

struct UnconditionalAttributeOwnershipDivestiturePlan {
  UnconditionalAttributeOwnershipDivestitureStatus status =
      UnconditionalAttributeOwnershipDivestitureStatus::applied;
  std::vector<AttributeOwnershipAssumptionRecipient> assumptionRecipients;
  std::vector<AttributeOwnershipAcquisitionWorkItem> acquisitionWorkItems;
};

// Private validation/routing result for Cancel Attribute Ownership
// Acquisition. A successfully accepted cancellation moves only regular
// Acquisition Pending attributes into a separate cancellation reservation.
// That reservation keeps the declaration-management publication guard active
// until Confirm Attribute Ownership Acquisition Cancellation begins.
enum class AttributeOwnershipAcquisitionCancellationStatus {
  applied,
  federation_does_not_exist,
  requesting_federate_not_member,
  object_instance_not_known,
  attribute_owned_by_rti,
  attribute_acquisition_was_not_requested,
  attribute_already_owned,
  attribute_not_defined,
  inconsistent_catalog,
};

struct AttributeOwnershipAcquisitionCancellationPlan {
  AttributeOwnershipAcquisitionCancellationStatus status =
      AttributeOwnershipAcquisitionCancellationStatus::applied;
  std::uint64_t cancellationId = 0;
  std::set<std::uint64_t> attributeHandles;
  ObjectInstanceCallbackRoute callbackRoute;
};

// A fresh-registry restore has no serialized callback closure for a pending
// cancellation. Keep the durable callback identity and attribute projection
// separate from the public plan so the ambassador can rebind it to the live
// requester route after Federation Restored.
struct AttributeOwnershipAcquisitionCancellationWorkItem {
  std::uint64_t requestingFederateId = 0;
  std::uint64_t objectInstanceHandle = 0;
  std::uint64_t cancellationId = 0;
  std::set<std::uint64_t> attributeHandles;
  ObjectInstanceCallbackRoute callbackRoute;
};

struct AttributeOwnershipAcquisitionCancellationDelivery {
  std::uint64_t objectInstanceHandle = 0;
  std::set<std::uint64_t> confirmedAttributeHandles;
  std::vector<AttributeOwnershipAcquisitionWorkItem> followupWorkItems;
};

// Private immutable routing result for the receive-order Send Interaction
// foundation.  The registry computes a recipient's closest active subscribed
// superclass and the subset of sent parameters available there; binding code
// owns the later callback delivery and public exception translation.
enum class ReceiveOrderInteractionStatus {
  applied,
  federation_does_not_exist,
  producing_federate_not_member,
  interaction_class_not_defined,
  interaction_class_not_published,
  interaction_parameter_not_defined,
  invalid_region_context,
  region_not_created_by_this_federate,
  invalid_region,
  inconsistent_catalog,
};

// An interaction's source is a private routing fact, not necessarily a
// callback-visible FederateHandle.  In particular, an RTI-originated MOM
// interaction has no sourced public producing-federate designator yet.  Keep
// that distinction explicit so a numeric zero can never become an accidental
// stand-in for a joined federate at a future callback boundary.
class InteractionProducer final {
 public:
  enum class Kind {
    joined_federate,
    rti,
  };

  [[nodiscard]] static InteractionProducer joinedFederate(
      std::uint64_t federateId) noexcept {
    return InteractionProducer{Kind::joined_federate, federateId};
  }

  [[nodiscard]] static InteractionProducer rti() noexcept {
    return InteractionProducer{Kind::rti, std::nullopt};
  }

  [[nodiscard]] Kind kind() const noexcept { return kind_; }

  [[nodiscard]] std::optional<std::uint64_t> joinedFederateId() const noexcept {
    return joinedFederateId_;
  }

 private:
  InteractionProducer(
      Kind kind,
      std::optional<std::uint64_t> joinedFederateId) noexcept
      : kind_(kind), joinedFederateId_(joinedFederateId) {}

  Kind kind_;
  std::optional<std::uint64_t> joinedFederateId_;
};

struct ReceiveOrderInteractionRecipient {
  std::uint64_t federateId = 0;
  std::uint64_t receivedInteractionClassHandle = 0;
  std::set<std::uint64_t> receivedParameterHandles;
  // See ReceiveOrderAttributeUpdateRecipient::conveyRegionDesignatorSets.
  bool conveyRegionDesignatorSets = false;
  InteractionCallbackRoute callbackRoute;
};

struct ReceiveOrderInteractionPlan {
  ReceiveOrderInteractionStatus status = ReceiveOrderInteractionStatus::applied;
  std::string transportationName;
  // See ReceiveOrderAttributeUpdatePassel::defaultRegionUsed.  An ordinary
  // Send Interaction uses the RTI-provided default region when the class has
  // available dimensions, without exposing a caller-visible region handle.
  bool defaultRegionUsed = false;
  // Regional receive-order callbacks may be queued under HLA_EVOKED or an
  // asynchronous-delivery gate. Retain the committed send-time
  // specifications so callback-time rechecks do not reinterpret an accepted
  // interaction against a later source-region mutation.
  std::map<std::uint64_t, RegionSpecificationSnapshot> sentRegionSnapshots;
  rti1516_2025::OrderType preferredOrderType = rti1516_2025::RECEIVE;
  std::vector<ReceiveOrderInteractionRecipient> recipients;
};

}  // namespace umbra::detail
