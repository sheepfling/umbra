#pragma once

#include "internal/attribute_handle_directory.hpp"
#include "internal/dimension_handle_directory.hpp"
#include "internal/federation_time_coordinator.hpp"
#include "internal/fom_validation.hpp"
#include "internal/interaction_class_handle_directory.hpp"
#include "internal/object_class_handle_directory.hpp"
#include "internal/parameter_handle_directory.hpp"
#include "internal/runtime_instrumentation.hpp"

#include <RTI/Enums.h>
#include <RTI/Typedefs.h>
#include <RTI/VariableLengthData.h>
#include <RTI/time/LogicalTime.h>

#include <cstddef>
#include <cstdint>
#include <functional>
#include <map>
#include <memory>
#include <mutex>
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
using FederateCallbackRoute = std::function<void(FederateCallbackInvocation)>;

class FomCatalog;
class MaterializedFdd;

struct FederationDefinition {
  std::vector<PrevalidatedFomModule> fomModules;
  std::wstring logicalTimeImplementationName;
  std::shared_ptr<FomCatalog const> catalog;
  std::shared_ptr<MaterializedFdd const> fdd;
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
  // HLAreportServiceInvocation uses an HLAcount serial number per joined
  // federate. The next accepted report starts at zero.
  std::uint32_t nextMomServiceReportSerialNumber = 0;
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
// process-local in-memory snapshot after every participating federate reports
// completion; external durable persistence and distributed transport are
// separate layers.
struct FederationSaveNotification {
  FederationSaveNotificationKind kind = FederationSaveNotificationKind::initiate;
  std::wstring label;
  bool successful = false;
  rti1516_2025::SaveFailureReason failureReason = rti1516_2025::SAVE_ABORTED;
  std::vector<std::pair<std::uint64_t, rti1516_2025::SaveStatus>> statuses;
  FederateCallbackRoute callbackRoute;
};

struct FederationSaveControlResult {
  FederationSaveControlStatus status = FederationSaveControlStatus::applied;
  std::vector<FederationSaveNotification> notifications;
};

// A time-constrained member must receive Initiate Federate Save while it is
// still in Time Advancing. The matching time-grant dispatcher therefore
// invokes that one callback directly at its pre-grant boundary; the registry
// returns the label for that direct invocation plus any ordinary callbacks
// made ready once all constrained members have reached the relevant boundary.
struct FederationSaveAdmission {
  FederationSaveControlStatus status = FederationSaveControlStatus::applied;
  std::optional<std::wstring> currentFederateLabel;
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
};

struct FederationRestoreControlResult {
  FederationRestoreControlStatus status =
      FederationRestoreControlStatus::applied;
  std::vector<FederationRestoreNotification> notifications;
};

struct AttributeOwnershipAcquisitionWorkItem;

// Resign-action work is returned as generic callback routes so the registry
// can keep its lock boundary independent of the public object/ownership
// callback record definitions below.  The ambassador converts these records
// to the existing queue-owned types after the federation lock is released.
struct FederationResignObjectRemoval {
  std::uint64_t receivingFederateId = 0;
  std::uint64_t objectInstanceHandle = 0;
  FederateCallbackRoute callbackRoute;
};

struct FederationResignOwnershipAssumption {
  std::uint64_t receivingFederateId = 0;
  std::uint64_t objectInstanceHandle = 0;
  std::set<std::uint64_t> attributeHandles;
  FederateCallbackRoute callbackRoute;
};

struct FederationRegistryResult {
  FederationRegistryStatus status = FederationRegistryStatus::applied;
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
};

struct FederationSynchronizedNotification {
  std::wstring label;
  std::set<std::uint64_t> failedToSyncFederateIds;
  FederateCallbackRoute callbackRoute;
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

// An unpublished, registry-owned representation of the MIM object for one
// joined federate. It reserves an ObjectInstanceHandle from the federation's
// common namespace but is deliberately kept outside the federate-created
// ObjectInstance map until RTI-originated discovery/reflection has a
// source-backed producer-designator rule. No public object service observes
// this snapshot yet.
struct JoinedFederateMomObjectSnapshot {
  std::uint64_t objectInstanceHandle = 0;
  std::uint64_t joinedFederateId = 0;
  std::uint64_t objectClassHandle = 0;
  RegionSpecificationSnapshot immutableFederatePoint;
  // Every effective MIM attribute is retained as metadata, including the
  // inherited optional HLAprivilegeToDeleteObject. Only required initial
  // joined-federate values are encoded here; the scheduler for dynamic values
  // and ordinary reflection delivery remains a later layer.
  std::set<std::uint64_t> effectiveAttributeHandles;
  std::map<std::uint64_t, rti1516_2025::VariableLengthData> initialAttributeValues;
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
// whose calculated scope changed. The adapter invokes the official Turn
// Updates On/Off callback at the owning federate, then rechecks ownership,
// scope, and the owner's advisory switch at callback entry. Update-rate
// designators are intentionally not synthesized until the update-rate model
// is implemented.
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
  std::vector<ObjectInstanceScopeChangeRecipient> recipients;
  std::vector<AttributeRelevanceAdvisoryRecipient> attributeRelevanceAdvisories;
};

struct ObjectInstanceRegionAssociationScopePlan {
  ObjectInstanceRegionAssociationStatus status =
      ObjectInstanceRegionAssociationStatus::applied;
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
};

struct ObjectInstanceDeletionPlan {
  ObjectInstanceDeletionStatus status = ObjectInstanceDeletionStatus::applied;
  std::vector<ObjectInstanceRemovalRecipient> recipients;
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
};

struct AttributeValueUpdateRequestPlan {
  AttributeValueUpdateRequestStatus status = AttributeValueUpdateRequestStatus::applied;
  std::vector<AttributeValueUpdateProvideRecipient> recipients;
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
// foundation. The current runtime has only two ownership states that can be
// reached through implemented services: a joined federate owns an attribute,
// or it is available for acquisition. RTI-owned state is intentionally not
// synthesized with a sentinel handle; it requires the later ownership-transfer
// state model before Attribute Is Owned By RTI can be delivered faithfully.
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
};

struct AttributeOwnershipQueryRecipient {
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
  attribute_not_published,
  object_class_not_published,
  federate_owns_attributes,
  attribute_not_defined,
  inconsistent_catalog,
};

enum class AttributeOwnershipAcquisitionWorkKind {
  acquisition_notification,
  request_release,
  request_divestiture_confirmation,
};

// A registry-generated callback work item always retains the user tag from
// the original regular acquisition.  The adapter submits it only after all
// registry and invoking-ambassador locks have been released.
struct AttributeOwnershipAcquisitionWorkItem {
  AttributeOwnershipAcquisitionWorkKind kind =
      AttributeOwnershipAcquisitionWorkKind::acquisition_notification;
  std::uint64_t requestingFederateId = 0;
  std::uint64_t receivingFederateId = 0;
  std::uint64_t objectInstanceHandle = 0;
  std::uint64_t requestId = 0;
  std::set<std::uint64_t> attributeHandles;
  std::vector<unsigned char> userSuppliedTag;
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
// attribute is waiting, selects a currently pending regular acquirer, and
// changes ownership only after the owner confirms. Continuing owner search and
// Willing-to-Acquire selection remain separate work.
enum class NegotiatedAttributeOwnershipDivestitureStatus {
  applied,
  federation_does_not_exist,
  divesting_federate_not_member,
  object_instance_not_known,
  attribute_not_owned,
  attribute_not_defined,
  attribute_already_being_divested,
  inconsistent_catalog,
};

struct NegotiatedAttributeOwnershipDivestiturePlan {
  NegotiatedAttributeOwnershipDivestitureStatus status =
      NegotiatedAttributeOwnershipDivestitureStatus::applied;
  std::vector<AttributeOwnershipAcquisitionWorkItem> workItems;
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
  attribute_not_owned,
  attribute_not_defined,
  inconsistent_catalog,
};

struct AttributeOwnershipAssumptionRecipient {
  std::uint64_t receivingFederateId = 0;
  std::uint64_t objectInstanceHandle = 0;
  std::set<std::uint64_t> attributeHandles;
  ObjectInstanceCallbackRoute callbackRoute;
};

// The callback-entry recheck reports only attributes that remain unowned and
// for which the original recipient is still publishing at its known class and
// remains outside both acquisition-pending states.  A changed lifecycle or
// ownership boundary becomes an ordinary no-delivery outcome.
struct AttributeOwnershipAssumptionDelivery {
  std::uint64_t objectInstanceHandle = 0;
  std::set<std::uint64_t> attributeHandles;
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
  rti1516_2025::OrderType preferredOrderType = rti1516_2025::RECEIVE;
  std::vector<ReceiveOrderInteractionRecipient> recipients;
};

// §11.5 service reports are RTI-originated receive-order interactions.  This
// routing result deliberately remains private: an application must neither
// publish the MOM report class nor provide its private update region.
enum class MomServiceReportDisposition {
  suppressed,
  report_to_file,
  interaction,
  inconsistent_catalog,
  reported_federate_not_member,
  invalid_service_group,
};

struct MomServiceReportRoutingPlan {
  MomServiceReportDisposition disposition = MomServiceReportDisposition::suppressed;
  // This stays an RTI source fact only.  It must not be converted into a
  // FederateHandle for a public Receive Interaction callback until the 2025
  // producer-designator rule for RTI-created MOM traffic is sourced.
  InteractionProducer producer = InteractionProducer::rti();
  std::uint64_t interactionClassHandle = 0;
  std::uint64_t endpointRegionHandle = 0;
  RegionSpecificationSnapshot endpointRegion;
  std::vector<ReceiveOrderInteractionRecipient> recipients;
};

struct ReservedMomServiceReport {
  MomServiceReportRoutingPlan routing;
  std::uint32_t serialNumber = 0;
  bool acceptedForEmission = false;
};

// A fault report is distinct from §11.5 service reporting: it is mandated for
// a lost federate regardless of that federate's reporting switches.  The
// routing fact remains explicitly RTI-originated; the C++ callback's required
// FederateHandle representation is selected at the binding boundary and is
// not inferred from this private source marker.
enum class FederateLostReportStatus {
  applied,
  federation_does_not_exist,
  reported_federate_not_member,
  inconsistent_catalog,
  inconsistent_time_state,
};

struct FederateLostReportRouting {
  std::uint64_t interactionClassHandle = 0;
  std::uint64_t federateParameterHandle = 0;
  std::uint64_t federateNameParameterHandle = 0;
  std::uint64_t timestampParameterHandle = 0;
  std::uint64_t faultDescriptionParameterHandle = 0;
  // The endpoint is private RTI routing state, not a public RegionHandle that
  // can be conveyed through the Receive Interaction callback.
  std::uint64_t endpointRegionHandle = 0;
  RegionSpecificationSnapshot endpointRegion;
};

struct FederateLostReportPlan {
  FederateLostReportStatus status = FederateLostReportStatus::applied;
  InteractionProducer producer = InteractionProducer::rti();
  std::uint64_t reportedFederateId = 0;
  std::wstring reportedFederateName;
  bool reportedFederateWasTimeRegulating = false;
  // Captured before the registry removes the lost member's time state.  For a
  // regulating federate this is the profile's last granted logical time.
  std::shared_ptr<rti1516_2025::LogicalTime const> lastKnownTime;
  FederateLostReportRouting routing;
  std::vector<ReceiveOrderInteractionRecipient> recipients;
};

// Private state/results for the bounded 2025 order-type control services.
// Order changes are synchronous in the standard API: class defaults affect
// future instances, instance changes affect the selected owned attributes,
// and interaction changes affect future sends by the invoking publisher.
enum class AttributeOrderTypeChangeStatus {
  applied,
  federation_does_not_exist,
  requesting_federate_not_member,
  object_instance_not_known,
  attribute_not_owned,
  attribute_not_defined,
  invalid_order_type,
};

enum class AttributeOrderTypeDefaultStatus {
  applied,
  federation_does_not_exist,
  requesting_federate_not_member,
  object_class_not_defined,
  attribute_not_defined,
  invalid_order_type,
};

enum class InteractionOrderTypeChangeStatus {
  applied,
  federation_does_not_exist,
  requesting_federate_not_member,
  interaction_class_not_published,
  interaction_class_not_defined,
  invalid_order_type,
};

// Private 2025 transportation-type control results.  The embedded profile
// supports the two mandatory standard transport names and keeps the state
// federation-owned so receive-order and timestamped planners observe the
// same effective type.  The callback boundary is the commit point for a
// requested change; a default change is prospective and does not rewrite
// already registered/owned instance attributes.
enum class AttributeTransportationTypeChangeStatus {
  applied,
  federation_does_not_exist,
  requesting_federate_not_member,
  object_instance_not_known,
  attribute_already_being_changed,
  attribute_not_owned,
  attribute_not_defined,
  invalid_transportation_type,
  callback_route_missing,
  inconsistent_catalog,
};

struct AttributeTransportationTypeChangePlan {
  AttributeTransportationTypeChangeStatus status =
      AttributeTransportationTypeChangeStatus::applied;
  std::uint64_t requestId = 0;
  std::uint64_t objectInstanceHandle = 0;
  std::set<std::uint64_t> attributeHandles;
  std::string transportationName;
  ObjectInstanceCallbackRoute callbackRoute;
};

struct AttributeTransportationTypeChangeDelivery {
  std::uint64_t objectInstanceHandle = 0;
  std::set<std::uint64_t> attributeHandles;
  std::string transportationName;
};

enum class AttributeTransportationTypeDefaultStatus {
  applied,
  federation_does_not_exist,
  requesting_federate_not_member,
  object_class_not_defined,
  attribute_not_defined,
  invalid_transportation_type,
  inconsistent_catalog,
};

enum class AttributeTransportationTypeQueryStatus {
  applied,
  federation_does_not_exist,
  requesting_federate_not_member,
  object_instance_not_known,
  attribute_not_defined,
  callback_route_missing,
  inconsistent_catalog,
};

struct AttributeTransportationTypeQueryPlan {
  AttributeTransportationTypeQueryStatus status =
      AttributeTransportationTypeQueryStatus::applied;
  std::uint64_t objectInstanceHandle = 0;
  std::uint64_t attributeHandle = 0;
  std::string transportationName;
  ObjectInstanceCallbackRoute callbackRoute;
};

enum class InteractionTransportationTypeChangeStatus {
  applied,
  federation_does_not_exist,
  requesting_federate_not_member,
  interaction_class_already_being_changed,
  interaction_class_not_published,
  interaction_class_not_defined,
  invalid_transportation_type,
  callback_route_missing,
  inconsistent_catalog,
};

struct InteractionTransportationTypeChangePlan {
  InteractionTransportationTypeChangeStatus status =
      InteractionTransportationTypeChangeStatus::applied;
  std::uint64_t interactionClassHandle = 0;
  std::string transportationName;
  InteractionCallbackRoute callbackRoute;
};

enum class InteractionTransportationTypeQueryStatus {
  applied,
  federation_does_not_exist,
  requesting_federate_not_member,
  interaction_class_not_defined,
  callback_route_missing,
  inconsistent_catalog,
};

struct InteractionTransportationTypeQueryPlan {
  InteractionTransportationTypeQueryStatus status =
      InteractionTransportationTypeQueryStatus::applied;
  std::uint64_t queriedFederateId = 0;
  std::uint64_t interactionClassHandle = 0;
  std::string transportationName;
  InteractionCallbackRoute callbackRoute;
};

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
};

struct TsoObjectDeletionMessage {
  std::uint64_t messageId = 0;
  std::uint64_t producingFederateId = 0;
  std::uint64_t objectInstanceHandle = 0;
  rti1516_2025::VariableLengthData userSuppliedTag;
  std::vector<TsoObjectDeletionRecipient> recipients;
  std::shared_ptr<rti1516_2025::LogicalTime const> timestamp;
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

// A registry-owned pending TAR carries only a private wake-up action. The
// action must enqueue work on the owning ambassador's callback dispatcher; it
// must never invoke a FederateAmbassador while the registry is locked.
using FederationTimeGrantDispatch = std::function<void()>;

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
class EmbeddedFederationRegistry final {
 public:
  explicit EmbeddedFederationRegistry(
      std::shared_ptr<RuntimeInstrumentation> instrumentation = {});

  [[nodiscard]] RuntimeInstrumentationSnapshot
  runtimeInstrumentationSnapshotForTesting() const;

  FederationRegistryResult create(
      std::wstring const& federationName,
      FederationDefinition definition);

  FederationRegistryResult destroy(std::wstring const& federationName);

  FederationJoinResult join(
      std::wstring const& federationName,
      std::wstring const& federateType,
      std::optional<std::wstring> requestedFederateName = std::nullopt);

  // Runtime-backed joins register their FederateTimeState in the same private
  // membership transaction.  The legacy join overload remains useful to unit
  // test the registry independently, but public embedded joins must use this
  // overload so a GALT/LITS coordinator cannot observe an untracked member.
  FederationJoinResult joinWithTimeState(
      std::wstring const& federationName,
      std::shared_ptr<FederateTimeState> timeState,
      std::wstring const& federateType,
      std::optional<std::wstring> requestedFederateName = std::nullopt,
      InteractionCallbackRoute interactionCallbackRoute = {});

  // Commits a freshly prevalidated replacement definition and a membership as
  // one state transition. This is used only after an external coordinator has
  // proved additional FOM modules compatible with the current definition.
  FederationJoinResult joinWithDefinition(
      std::wstring const& federationName,
      FederationDefinition definition,
      std::wstring const& federateType,
      std::optional<std::wstring> requestedFederateName = std::nullopt);

  FederationJoinResult joinWithDefinitionAndTimeState(
      std::wstring const& federationName,
      FederationDefinition definition,
      std::shared_ptr<FederateTimeState> timeState,
      std::wstring const& federateType,
      std::optional<std::wstring> requestedFederateName = std::nullopt,
      InteractionCallbackRoute interactionCallbackRoute = {});

  FederationRegistryResult resign(
      std::wstring const& federationName,
      std::uint64_t federateId,
      rti1516_2025::ResignAction resignAction = rti1516_2025::NO_ACTION);

  // Establishes the registry-owned, as-yet-unpublished MIM object after the
  // real report-file writer has chosen its immutable location. This is kept
  // separate from joinImpl because a filesystem failure must roll membership
  // back before a successful Join becomes externally visible.
  [[nodiscard]] JoinedFederateMomObjectStatus establishJoinedFederateMomObject(
      std::wstring const& federationName,
      std::uint64_t federateId,
      JoinedFederateMomObjectDescriptor const& descriptor);

  [[nodiscard]] std::optional<JoinedFederateMomObjectSnapshot>
  joinedFederateMomObjectFor(
      std::wstring const& federationName,
      std::uint64_t federateId) const;

  // A transport fault applies the member's current Automatic Resign
  // Directive while forcing the membership transition even when a normal
  // caller-initiated resign would reject unresolved ownership work. The
  // returned callback records are for surviving federates; the lost
  // federate's own connectionLost callback is owned by the adapter session.
  FederationRegistryResult connectionLost(
      std::wstring const& federationName,
      std::uint64_t federateId);

  [[nodiscard]] FederationSaveControlResult requestFederationSave(
      std::wstring const& federationName,
      std::uint64_t requestingFederateId,
      std::wstring label);

  [[nodiscard]] FederationSaveControlResult requestFederationSave(
      std::wstring const& federationName,
      std::uint64_t requestingFederateId,
      std::wstring label,
      std::shared_ptr<rti1516_2025::LogicalTime const> timestamp);

  // Admits one pending untimed save at a Time Advance Grant boundary. The
  // adapter calls this after the grant has passed federation scheduling but
  // before it mutates the recipient's FederateTimeState to Time Granted, so
  // the returned direct callback observes the IEEE 1516.1-2025-required Time
  // Advancing state. This is intentionally separate from the timed-save
  // scheduler, whose timestamp/TSO admission remains a different slice.
  [[nodiscard]] FederationSaveAdmission
  admitImmediateFederationSaveAtTimeAdvanceBoundary(
      std::wstring const& federationName,
      std::uint64_t federateId);

  // Admits one pending timestamped save after the current recipient has
  // received its required TSO payloads but before the matching ordinary Time
  // Advance Grant mutates its private state. The registry accepts TAR/NMR at
  // an inclusive save boundary and TARA/NMRA at an exclusive one. FQR uses
  // the separately calculated actual grant supplied through the dedicated
  // admission entry point below.
  [[nodiscard]] FederationSaveAdmission
  admitTimedFederationSaveAtTimeAdvanceBoundary(
      std::wstring const& federationName,
      std::uint64_t federateId);

  // Flush Queue Request calculates its actual grant at callback time. The
  // adapter supplies that calculated grant after draining the recipient's TSO
  // queue but before mutating FederateTimeState, so a qualifying strict save
  // boundary can still deliver Initiate Federate Save in Time Advancing.
  [[nodiscard]] FederationSaveAdmission
  admitTimedFederationSaveAtFlushQueueGrantBoundary(
      std::wstring const& federationName,
      std::uint64_t federateId,
      std::shared_ptr<rti1516_2025::LogicalTime const> flushQueueGrantedTime);

  // Re-evaluates a pending timestamped save after a time/TSO boundary.  The
  // result contains Initiate Federate Save callbacks only when every current
  // time-constrained member has crossed the requested timestamp and has no
  // undelivered TSO message at or below that boundary.
  [[nodiscard]] FederationSaveControlResult reevaluateTimedFederationSave(
      std::wstring const& federationName);

  [[nodiscard]] FederationSaveControlResult federateSaveBegun(
      std::wstring const& federationName,
      std::uint64_t federateId);

  [[nodiscard]] FederationSaveControlResult federateSaveComplete(
      std::wstring const& federationName,
      std::uint64_t federateId);

  [[nodiscard]] FederationSaveControlResult federateSaveNotComplete(
      std::wstring const& federationName,
      std::uint64_t federateId);

  [[nodiscard]] FederationSaveControlResult abortFederationSave(
      std::wstring const& federationName,
      std::uint64_t federateId);

  [[nodiscard]] FederationSaveControlResult queryFederationSaveStatus(
      std::wstring const& federationName,
      std::uint64_t federateId);

  [[nodiscard]] FederationServiceOperationStatus serviceOperationStatus(
      std::wstring const& federationName,
      std::uint64_t federateId) const;

  [[nodiscard]] FederationRestoreControlResult requestFederationRestore(
      std::wstring const& federationName,
      std::uint64_t requestingFederateId,
      std::wstring label);

  [[nodiscard]] FederationRestoreControlResult federateRestoreComplete(
      std::wstring const& federationName,
      std::uint64_t federateId);

  [[nodiscard]] FederationRestoreControlResult federateRestoreNotComplete(
      std::wstring const& federationName,
      std::uint64_t federateId);

  [[nodiscard]] FederationRestoreControlResult abortFederationRestore(
      std::wstring const& federationName,
      std::uint64_t federateId);

  [[nodiscard]] FederationRestoreControlResult queryFederationRestoreStatus(
      std::wstring const& federationName,
      std::uint64_t federateId);

  // Federation synchronization points are retained as one-shot federation
  // state.  Registration and achievement return callback plans rather than
  // invoking user code while the registry is locked.
  [[nodiscard]] SynchronizationPointRegistrationPlan registerSynchronizationPoint(
      std::wstring const& federationName,
      std::uint64_t registeringFederateId,
      std::wstring label,
      std::vector<unsigned char> userSuppliedTag,
      std::set<std::uint64_t> const& synchronizationSet);

  [[nodiscard]] SynchronizationPointAnnouncementPlan
  announcePendingSynchronizationPoints(
      std::wstring const& federationName,
      std::uint64_t newlyJoinedFederateId);

  [[nodiscard]] SynchronizationPointAchievedPlan achieveSynchronizationPoint(
      std::wstring const& federationName,
      std::uint64_t achievingFederateId,
      std::wstring const& label,
      bool successfully);

  [[nodiscard]] bool contains(std::wstring const& federationName) const;
  [[nodiscard]] std::size_t memberCount(std::wstring const& federationName) const;
  [[nodiscard]] std::optional<FederationDefinition> definitionFor(
      std::wstring const& federationName) const;
  [[nodiscard]] std::optional<FederationTimeExecutionSnapshot> timeSnapshotFor(
      std::wstring const& federationName) const;

  // Live per-federate state used by the adapter's receive-order callback gate.
  // It is returned as a shared pointer so the registry lock can be released
  // before temporal state or callback-session code runs.
  [[nodiscard]] std::shared_ptr<FederateTimeState> timeStateFor(
      std::wstring const& federationName,
      std::uint64_t federateId) const;

  [[nodiscard]] std::optional<bool> attributeScopeAdvisorySwitchFor(
      std::wstring const& federationName,
      std::uint64_t federateId) const;

  [[nodiscard]] FederationRegistryStatus setAttributeScopeAdvisorySwitch(
      std::wstring const& federationName,
      std::uint64_t federateId,
      bool switchValue);

  [[nodiscard]] std::optional<bool> objectClassRelevanceAdvisorySwitchFor(
      std::wstring const& federationName,
      std::uint64_t federateId) const;

  [[nodiscard]] FederationRegistryStatus setObjectClassRelevanceAdvisorySwitch(
      std::wstring const& federationName,
      std::uint64_t federateId,
      bool switchValue);

  [[nodiscard]] std::optional<bool> attributeRelevanceAdvisorySwitchFor(
      std::wstring const& federationName,
      std::uint64_t federateId) const;

  [[nodiscard]] FederationRegistryStatus setAttributeRelevanceAdvisorySwitch(
      std::wstring const& federationName,
      std::uint64_t federateId,
      bool switchValue);

  [[nodiscard]] std::optional<bool> interactionRelevanceAdvisorySwitchFor(
      std::wstring const& federationName,
      std::uint64_t federateId) const;

  [[nodiscard]] std::optional<bool> advisoriesUseKnownClassSwitchFor(
      std::wstring const& federationName,
      std::uint64_t federateId) const;

  [[nodiscard]] std::optional<bool> autoProvideSwitchFor(
      std::wstring const& federationName,
      std::uint64_t federateId) const;

  // HLAmanager.HLAfederation.HLAadjust.HLAsetSwitches changes the
  // federation-wide Auto Provide value for all current members.  The public
  // MOM interaction path owns the payload validation; the registry only
  // applies the already-decoded value as one locked federation mutation.
  [[nodiscard]] FederationRegistryStatus setAutoProvideSwitch(
      std::wstring const& federationName,
      std::uint64_t federateId,
      bool switchValue);

  [[nodiscard]] std::optional<bool> nonRegulatedGrantSwitchFor(
      std::wstring const& federationName,
      std::uint64_t federateId) const;

  [[nodiscard]] std::optional<bool> conveyRegionDesignatorSetsSwitchFor(
      std::wstring const& federationName,
      std::uint64_t federateId) const;

  [[nodiscard]] FederationRegistryStatus setConveyRegionDesignatorSetsSwitch(
      std::wstring const& federationName,
      std::uint64_t federateId,
      bool switchValue);

  [[nodiscard]] std::optional<rti1516_2025::ResignAction> automaticResignActionFor(
      std::wstring const& federationName,
      std::uint64_t federateId) const;

  [[nodiscard]] FederationRegistryStatus setAutomaticResignAction(
      std::wstring const& federationName,
      std::uint64_t federateId,
      rti1516_2025::ResignAction resignAction);

  [[nodiscard]] std::optional<bool> serviceReportingSwitchFor(
      std::wstring const& federationName,
      std::uint64_t federateId) const;

  [[nodiscard]] ServiceReportingSwitchStatus setServiceReportingSwitch(
      std::wstring const& federationName,
      std::uint64_t federateId,
      bool switchValue);

  // Applies the standard joined-federate HLAsetSwitches parameter subset as
  // one registry mutation.  This is intentionally private-runtime state: the
  // official public API exposes the individual switch services separately.
  [[nodiscard]] FederateMOMSwitchUpdateStatus applyFederateMOMSwitchUpdate(
      std::wstring const& federationName,
      std::uint64_t federateId,
      FederateMOMSwitchUpdate const& update);

  [[nodiscard]] std::optional<bool> exceptionReportingSwitchFor(
      std::wstring const& federationName,
      std::uint64_t federateId) const;

  [[nodiscard]] FederationRegistryStatus setExceptionReportingSwitch(
      std::wstring const& federationName,
      std::uint64_t federateId,
      bool switchValue);

  [[nodiscard]] std::optional<bool> sendServiceReportsToFileSwitchFor(
      std::wstring const& federationName,
      std::uint64_t federateId) const;

  [[nodiscard]] FederationRegistryStatus setSendServiceReportsToFileSwitch(
      std::wstring const& federationName,
      std::uint64_t federateId,
      bool switchValue);

  [[nodiscard]] std::optional<bool> delaySubscriptionEvaluationSwitchFor(
      std::wstring const& federationName,
      std::uint64_t federateId) const;

  [[nodiscard]] std::optional<bool> allowRelaxedDDMSwitchFor(
      std::wstring const& federationName,
      std::uint64_t federateId) const;

  [[nodiscard]] FederationRegistryStatus setInteractionRelevanceAdvisorySwitch(
      std::wstring const& federationName,
      std::uint64_t federateId,
      bool switchValue);

  // Re-evaluates a scheduled Turn Updates On/Off advisory immediately before
  // callback entry. This prevents stale queued advisories after a second
  // subscription/region/ownership transition or after the owner disables the
  // Attribute Relevance Advisory Switch.
  [[nodiscard]] std::set<std::uint64_t> attributeRelevanceAdvisoryAttributes(
      std::wstring const& federationName,
      std::uint64_t providingFederateId,
      std::uint64_t receivingFederateId,
      std::uint64_t objectInstanceHandle,
      std::set<std::uint64_t> const& attributeHandles,
      bool expectedInScope) const;

  // Re-resolves the currently retained explicit update-rate designator for a
  // queued Turn Updates On callback. A missing value means the official
  // no-rate overload must be used; an explicit HLAdefault remains distinct
  // and therefore selects the rate-bearing overload.
  [[nodiscard]] std::optional<std::string>
  attributeRelevanceAdvisoryUpdateRateDesignatorFor(
      std::wstring const& federationName,
      std::uint64_t receivingFederateId,
      std::uint64_t objectInstanceHandle,
      std::uint64_t attributeHandle) const;

  // Register a callback delivery for an already-accepted private TAR. The
  // result contains every newly eligible federate's action, not just the
  // requester, because a regulator's advance request can increase another
  // federate's GALT before the regulator receives its own grant.
  [[nodiscard]] FederationTimeGrantDispatchResult requestTimeAdvanceGrant(
      std::wstring const& federationName,
      std::uint64_t federateId,
      std::uint64_t generation,
      FederationTimeGrantDispatch dispatch);

  // Re-evaluate all pending TARs after a time-state change. Callers execute
  // the returned actions only after releasing all runtime locks.
  [[nodiscard]] FederationTimeGrantDispatchResult reevaluateTimeAdvanceGrants(
      std::wstring const& federationName);

  // Re-checks eligibility immediately before the queued callback changes the
  // federate's logical time. A stale or newly blocked callback becomes a
  // harmless no-op and is eligible for a future re-evaluation.
  [[nodiscard]] FederationTimeGrantStatus beginTimeAdvanceGrant(
      std::wstring const& federationName,
      std::uint64_t federateId,
      std::uint64_t generation);

  // Private temporal-coordinator hooks used by the bounded public timestamped
  // interaction, attribute-update, and object-deletion/removal slices. They
  // never invoke a FederateAmbassador; callers re-evaluate pending TAR
  // dispatches after each operation and own the official callback translation.
  [[nodiscard]] FederationTsoMessageIdResult allocateTsoMessageId(
      std::wstring const& federationName);

  [[nodiscard]] FederationTsoEnqueueResult enqueueTsoMessage(
      std::wstring const& federationName,
      std::uint64_t messageId,
      std::uint64_t recipientFederateId,
      std::shared_ptr<rti1516_2025::LogicalTime const> timestamp);

  // Read-only recipient-scoped view used by Next Message Request to select
  // the earliest currently queued TSO timestamp at or below the caller's
  // requested logical time.
  [[nodiscard]] std::optional<std::shared_ptr<rti1516_2025::LogicalTime const>>
  earliestTsoTimestampFor(
      std::wstring const& federationName,
      std::uint64_t recipientFederateId) const;

  // Atomically allocates one message-retraction designator, retains the
  // interaction payload while any recipient needs it, and records every
  // recipient of a timestamped Send Interaction.  The recipient set may be
  // empty: the public service still returns its designator, backed by a
  // lightweight ledger record. Only queuedRecipientFederateIds enter the
  // temporal queue; allTimestampedRecipientFederateIds also include
  // immediate/nonconstrained recipients so Retract can later distinguish
  // delivery from suppression.
  [[nodiscard]] FederationTsoInteractionEnqueueResult enqueueTsoInteraction(
      std::wstring const& federationName,
      TsoInteractionMessage message,
      std::vector<std::uint64_t> const& queuedRecipientFederateIds,
      std::vector<std::uint64_t> const& allTimestampedRecipientFederateIds);

  // Atomically allocates one message-retraction designator, retains the
  // attribute-update payload, and records every recipient of a timestamped
  // Update Attribute Values invocation. The recipient set may be empty: the
  // public service still returns its designator, backed by a lightweight
  // ledger record. Only queuedRecipientFederateIds enter the temporal queue;
  // allTimestampedRecipientFederateIds also include immediate/nonconstrained
  // recipients so Retract can later distinguish delivery from suppression.
  // The payload keeps recipient-specific passels so a changed subscription
  // can only suppress, never invent, a reflection at delivery.
  [[nodiscard]] FederationTsoAttributeUpdateEnqueueResult
  enqueueTsoAttributeUpdate(
      std::wstring const& federationName,
      TsoAttributeUpdateMessage message,
      std::vector<std::uint64_t> const& queuedRecipientFederateIds,
      std::vector<std::uint64_t> const& allTimestampedRecipientFederateIds);

  // Atomically allocates one message-retraction designator, retains a
  // timestamped object deletion payload, and reserves the known recipients'
  // pending-delete boundary.  The public adapter supplies only the selected
  // time-constrained recipient ids for queueing; non-constrained recipients
  // use the same accepted payload for immediate timestamped callbacks.
  [[nodiscard]] FederationTsoObjectDeletionEnqueueResult
  enqueueTsoObjectDeletion(
      std::wstring const& federationName,
      std::uint64_t producingFederateId,
      std::uint64_t objectInstanceHandle,
      TsoObjectDeletionMessage message,
      std::vector<std::uint64_t> const& recipientFederateIds);

  // Atomically allocates one message-retraction designator, retains the
  // directed-interaction payload, and records every recipient captured in that
  // payload. Only queuedRecipientFederateIds enter the temporal queue; the
  // retained recipient set also covers immediate/nonconstrained recipients so
  // Retract can distinguish delivery from suppression.
  [[nodiscard]] FederationTsoDirectedInteractionEnqueueResult
  enqueueTsoDirectedInteraction(
      std::wstring const& federationName,
      TsoDirectedInteractionMessage message,
      std::vector<std::uint64_t> const& queuedRecipientFederateIds);

  // Read-only validation and recipient snapshot for the timestamped delete
  // acceptance boundary.  The enqueue method repeats every check while the
  // federation-management mutex is held by the adapter.
  [[nodiscard]] ObjectInstanceDeletionPlan planTsoObjectInstanceDeletion(
      std::wstring const& federationName,
      std::uint64_t producingFederateId,
      std::uint64_t objectInstanceHandle) const;

  [[nodiscard]] FederationTsoRetractionResult retractTsoMessage(
      std::wstring const& federationName,
      std::uint64_t messageId);

  [[nodiscard]] FederationTsoRetractionResult retractTsoInteraction(
      std::wstring const& federationName,
      std::uint64_t producingFederateId,
      std::uint64_t messageId);

  [[nodiscard]] FederationTsoRetractionResult retractTsoMessageForProducer(
      std::wstring const& federationName,
      std::uint64_t producingFederateId,
      std::uint64_t messageId,
      std::shared_ptr<rti1516_2025::LogicalTime const> const& retractionLowerBound);

  // Retires heavyweight state for message designators whose producing
  // federate can no longer legally retract them at the supplied strict
  // Clause 8.22.3 boundary.  A lightweight producer-owned tombstone remains
  // so a later Retract is classified as MessageCanNoLongerBeRetracted rather
  // than as an invalid handle.  Pending deliveries keep their typed payload
  // until their own callback boundary has resolved.
  [[nodiscard]] std::size_t retireTsoMessagePayloadsAtOrBefore(
      std::wstring const& federationName,
      std::uint64_t producingFederateId,
      rti1516_2025::LogicalTime const& retractionLowerBound);

  // Claims the normal or directed timestamped interaction delivery boundary
  // immediately before the public callback enters user code. A concurrent
  // legal Retract can turn a still-pending recipient into retracted first,
  // causing this method to suppress the stale original callback.
  [[nodiscard]] bool beginTsoInteractionCallback(
      std::wstring const& federationName,
      std::uint64_t receivingFederateId,
      std::uint64_t messageId);

  // Claims one timestamped Reflect Attribute Values callback boundary. A
  // timestamped Update Attribute Values invocation may yield multiple
  // recipient passels under one retraction designator, so a recipient that
  // already began an earlier passel is allowed to continue unless a concurrent
  // legal Retract has changed it to retracted.
  [[nodiscard]] bool beginTsoAttributeUpdateCallback(
      std::wstring const& federationName,
      std::uint64_t receivingFederateId,
      std::uint64_t messageId);

  // Rechecks the recipient's active membership immediately before the
  // binding invokes Request Retraction on a route captured by a legal
  // Retract. The retraction record remains private and is never exposed as a
  // public service state object.
  [[nodiscard]] bool canDeliverTsoRequestRetraction(
      std::wstring const& federationName,
      std::uint64_t receivingFederateId,
      std::uint64_t messageId) const;

  [[nodiscard]] FederationTsoDeliveryRegistryResult beginTsoDelivery(
      std::wstring const& federationName,
      std::uint64_t recipientFederateId,
      rti1516_2025::LogicalTime const& boundary,
      bool inclusive);

  [[nodiscard]] FederationTsoInteractionDeliveryRegistryResult
  beginTsoInteractionDelivery(
      std::wstring const& federationName,
      std::uint64_t recipientFederateId,
      rti1516_2025::LogicalTime const& boundary,
      bool inclusive);

  // Begins one recipient boundary for all currently supported typed payloads
  // while preserving the queue's timestamp/sequence order across families.
  [[nodiscard]] FederationTsoPayloadDeliveryRegistryResult
  beginTsoPayloadDelivery(
      std::wstring const& federationName,
      std::uint64_t recipientFederateId,
      rti1516_2025::LogicalTime const& boundary,
      bool inclusive);

  // Begins one timestamped removal callback.  The execution-owned deletion
  // record remains reconstitutable after delivery as well: a legal Retract
  // either clears the pending marker before this boundary or restores the
  // invocation snapshot before Request Retraction reaches a delivered
  // recipient.
  [[nodiscard]] std::optional<RemovedObjectInstanceSnapshot>
  beginTsoObjectInstanceRemoval(
      std::wstring const& federationName,
      std::uint64_t receivingFederateId,
      std::uint64_t objectInstanceHandle,
      std::uint64_t messageId);

  [[nodiscard]] FederationTsoDeliveryRegistryResult completeTsoDelivery(
      std::wstring const& federationName,
      TsoQueuedMessage const& message);

  [[nodiscard]] std::vector<FederationExecutionSummary> federationExecutions() const;
  [[nodiscard]] std::optional<std::vector<FederateMembership>> membersFor(
      std::wstring const& federationName) const;
  [[nodiscard]] std::optional<FederateMembership> memberByName(
      std::wstring const& federationName,
      std::wstring const& federateName) const;
  [[nodiscard]] std::optional<FederateMembership> memberById(
      std::wstring const& federationName,
      std::uint64_t federateId) const;
  // A returned FederateHandle remains a valid federate designator for the
  // execution after that federate leaves it. Keep this identity lookup
  // distinct from memberById(), which intentionally answers only whether a
  // federate is currently joined and is used as a callback-delivery fence.
  [[nodiscard]] std::optional<std::wstring> federateNameFor(
      std::wstring const& federationName,
      std::uint64_t federateId) const;
  // Normalized values are execution-scoped coordinates used by the standard
  // MIM dimensions. They deliberately do not expose the private sequential
  // handle directories and remain stable for equal designators throughout an
  // execution, including a restore of that execution.
  [[nodiscard]] std::optional<unsigned long> normalizedFederateHandleValueFor(
      std::wstring const& federationName,
      std::uint64_t federateId) const;
  [[nodiscard]] std::optional<std::uint64_t> objectClassHandleFor(
      std::wstring const& federationName,
      std::string const& objectClassName) const;
  [[nodiscard]] std::optional<std::string> objectClassNameFor(
      std::wstring const& federationName,
      std::uint64_t objectClassHandle) const;
  [[nodiscard]] std::optional<unsigned long> normalizedObjectClassHandleValueFor(
      std::wstring const& federationName,
      std::uint64_t objectClassHandle) const;
  [[nodiscard]] std::optional<std::uint64_t> attributeHandleFor(
      std::wstring const& federationName,
      std::string const& objectClassName,
      std::string const& attributeName) const;
  [[nodiscard]] std::optional<std::string> attributeNameFor(
      std::wstring const& federationName,
      std::string const& objectClassName,
      std::uint64_t attributeHandle) const;
  [[nodiscard]] std::optional<std::uint64_t> interactionClassHandleFor(
      std::wstring const& federationName,
      std::string const& interactionClassName) const;
  [[nodiscard]] std::optional<std::string> interactionClassNameFor(
      std::wstring const& federationName,
      std::uint64_t interactionClassHandle) const;
  [[nodiscard]] std::optional<unsigned long>
  normalizedInteractionClassHandleValueFor(
      std::wstring const& federationName,
      std::uint64_t interactionClassHandle) const;
  [[nodiscard]] std::optional<unsigned long> normalizedObjectInstanceHandleValueFor(
      std::wstring const& federationName,
      std::uint64_t objectInstanceHandle) const;
  // MOM administration must recognize compatible extension subclasses as a
  // promoted form of their predefined parent class. This lookup keeps that
  // hierarchy decision inside the federation-owned catalog/handle space.
  [[nodiscard]] std::optional<bool> interactionClassIsSameOrDescendantOf(
      std::wstring const& federationName,
      std::uint64_t interactionClassHandle,
      std::string const& ancestorInteractionClassName) const;
  [[nodiscard]] std::optional<std::uint64_t> parameterHandleFor(
      std::wstring const& federationName,
      std::string const& interactionClassName,
      std::string const& parameterName) const;
  [[nodiscard]] std::optional<std::string> parameterNameFor(
      std::wstring const& federationName,
      std::string const& interactionClassName,
      std::uint64_t parameterHandle) const;
  [[nodiscard]] UpdateRateValueResult updateRateValueForDesignator(
      std::wstring const& federationName,
      std::uint64_t federateId,
      std::string const& updateRateDesignator) const;
  [[nodiscard]] UpdateRateValueResult updateRateValueForAttribute(
      std::wstring const& federationName,
      std::uint64_t federateId,
      std::uint64_t objectInstanceHandle,
      std::uint64_t attributeHandle) const;
  [[nodiscard]] std::optional<std::uint64_t> dimensionHandleFor(
      std::wstring const& federationName,
      std::string const& dimensionName) const;
  [[nodiscard]] std::optional<std::string> dimensionNameFor(
      std::wstring const& federationName,
      std::uint64_t dimensionHandle) const;
  [[nodiscard]] std::optional<unsigned long> dimensionUpperBoundFor(
      std::wstring const& federationName,
      std::uint64_t dimensionHandle) const;
  [[nodiscard]] std::optional<std::set<std::uint64_t>> availableDimensionsForObjectClass(
      std::wstring const& federationName,
      std::uint64_t objectClassHandle) const;
  [[nodiscard]] std::optional<std::set<std::uint64_t>> availableDimensionsForInteractionClass(
      std::wstring const& federationName,
      std::uint64_t interactionClassHandle) const;

  // DDM region-template/specification lifecycle.  Create allocates a region
  // with the requested dimensions; SetRangeBounds records pending values;
  // Commit promotes a complete pending map to the current specification; and
  // Delete removes an unused region.  These methods intentionally do not
  // create region realizations or route messages.
  [[nodiscard]] RegionCreateResult createRegion(
      std::wstring const& federationName,
      std::uint64_t federateId,
      std::set<std::uint64_t> const& dimensionHandles);
  [[nodiscard]] RegionServiceStatus commitRegionModifications(
      std::wstring const& federationName,
      std::uint64_t federateId,
      std::set<std::uint64_t> const& regionHandles);
  [[nodiscard]] RegionScopeChangePlan commitRegionModificationsWithScopeChanges(
      std::wstring const& federationName,
      std::uint64_t federateId,
      std::set<std::uint64_t> const& regionHandles);
  [[nodiscard]] RegionServiceStatus deleteRegion(
      std::wstring const& federationName,
      std::uint64_t federateId,
      std::uint64_t regionHandle);
  [[nodiscard]] RegionDimensionSetResult dimensionHandleSetForRegion(
      std::wstring const& federationName,
      std::uint64_t federateId,
      std::uint64_t regionHandle) const;
  [[nodiscard]] RegionRangeBoundsResult rangeBoundsForRegion(
      std::wstring const& federationName,
      std::uint64_t federateId,
      std::uint64_t regionHandle,
      std::uint64_t dimensionHandle) const;
  [[nodiscard]] RegionServiceStatus setRangeBounds(
      std::wstring const& federationName,
      std::uint64_t federateId,
      std::uint64_t regionHandle,
      std::uint64_t dimensionHandle,
      RegionRangeBounds range);

  InteractionClassDeclarationStatus setInteractionClassPublication(
      std::wstring const& federationName,
      std::uint64_t federateId,
      std::uint64_t interactionClassHandle,
      bool published);
  [[nodiscard]] DirectedInteractionDeclarationStatus
  publishObjectClassDirectedInteractions(
      std::wstring const& federationName,
      std::uint64_t federateId,
      std::uint64_t objectClassHandle,
      std::set<std::uint64_t> const& interactionClassHandles);
  [[nodiscard]] DirectedInteractionDeclarationStatus
  unpublishObjectClassDirectedInteractions(
      std::wstring const& federationName,
      std::uint64_t federateId,
      std::uint64_t objectClassHandle,
      std::optional<std::set<std::uint64_t>> const& interactionClassHandles);
  [[nodiscard]] DirectedInteractionDeclarationStatus
  subscribeObjectClassDirectedInteractions(
      std::wstring const& federationName,
      std::uint64_t federateId,
      std::uint64_t objectClassHandle,
      std::set<std::uint64_t> const& interactionClassHandles,
      bool universally);
  [[nodiscard]] DirectedInteractionDeclarationStatus
  unsubscribeObjectClassDirectedInteractions(
      std::wstring const& federationName,
      std::uint64_t federateId,
      std::uint64_t objectClassHandle,
      std::optional<std::set<std::uint64_t>> const& interactionClassHandles);
  InteractionClassDeclarationStatus setInteractionClassSubscription(
      std::wstring const& federationName,
      std::uint64_t federateId,
      std::uint64_t interactionClassHandle,
      std::optional<bool> active);
  RegionalInteractionClassDeclarationStatus
  setInteractionClassRegionalSubscription(
      std::wstring const& federationName,
      std::uint64_t federateId,
      std::uint64_t interactionClassHandle,
      std::set<std::uint64_t> const& regionHandles,
      bool active);
  RegionalInteractionClassDeclarationStatus
  removeInteractionClassRegionalSubscription(
      std::wstring const& federationName,
      std::uint64_t federateId,
      std::uint64_t interactionClassHandle,
      std::set<std::uint64_t> const& regionHandles);
  [[nodiscard]] std::optional<InteractionClassDeclarationSnapshot>
  interactionClassDeclarationFor(
      std::wstring const& federationName,
      std::uint64_t federateId,
      std::uint64_t interactionClassHandle) const;

  // Computes ordinary declaration-management relevance transitions for the
  // current publication/subscription state. Regional declarations are kept
  // out of this planner because the 2025 regional services must not trigger
  // the ordinary Start/Stop or Turn On/Off advisories.
  [[nodiscard]] std::vector<DeclarationAdvisory> planDeclarationAdvisories(
      std::wstring const& federationName);

  ObjectClassAttributeDeclarationStatus setObjectClassAttributePublication(
      std::wstring const& federationName,
      std::uint64_t federateId,
      std::uint64_t objectClassHandle,
      std::set<std::uint64_t> const& attributeHandles,
      bool published);
  ObjectClassAttributeDeclarationStatus setObjectClassAttributeSubscription(
      std::wstring const& federationName,
      std::uint64_t federateId,
      std::uint64_t objectClassHandle,
      std::set<std::uint64_t> const& attributeHandles,
      std::optional<bool> active,
      std::string const& updateRateDesignator = "HLAdefault");
  [[nodiscard]] ObjectClassAttributeSubscriptionScopePlan
  setObjectClassAttributeSubscriptionWithScopeChanges(
      std::wstring const& federationName,
      std::uint64_t federateId,
      std::uint64_t objectClassHandle,
      std::set<std::uint64_t> const& attributeHandles,
      std::optional<bool> active,
      std::string const& updateRateDesignator = "HLAdefault");
  RegionalObjectClassAttributeDeclarationStatus
  setObjectClassAttributeRegionalSubscription(
      std::wstring const& federationName,
      std::uint64_t federateId,
      std::uint64_t objectClassHandle,
      std::map<std::uint64_t, std::set<std::uint64_t>> const& attributesAndRegions,
      bool active,
      std::string const& updateRateDesignator = "HLAdefault");
  [[nodiscard]] RegionalObjectClassAttributeSubscriptionScopePlan
  setObjectClassAttributeRegionalSubscriptionWithScopeChanges(
      std::wstring const& federationName,
      std::uint64_t federateId,
      std::uint64_t objectClassHandle,
      std::map<std::uint64_t, std::set<std::uint64_t>> const& attributesAndRegions,
      bool active,
      std::string const& updateRateDesignator = "HLAdefault");
  RegionalObjectClassAttributeDeclarationStatus
  removeObjectClassAttributeRegionalSubscription(
      std::wstring const& federationName,
      std::uint64_t federateId,
      std::uint64_t objectClassHandle,
      std::map<std::uint64_t, std::set<std::uint64_t>> const& attributesAndRegions);
  [[nodiscard]] RegionalObjectClassAttributeSubscriptionScopePlan
  removeObjectClassAttributeRegionalSubscriptionWithScopeChanges(
      std::wstring const& federationName,
      std::uint64_t federateId,
      std::uint64_t objectClassHandle,
      std::map<std::uint64_t, std::set<std::uint64_t>> const& attributesAndRegions);
  [[nodiscard]] std::optional<ObjectClassAttributeDeclarationSnapshot>
  objectClassAttributeDeclarationFor(
      std::wstring const& federationName,
      std::uint64_t federateId,
      std::uint64_t objectClassHandle) const;
  [[nodiscard]] std::optional<std::set<std::uint64_t>>
  publishedObjectClassAttributeHandles(
      std::wstring const& federationName,
      std::uint64_t federateId,
      std::uint64_t objectClassHandle) const;

  [[nodiscard]] ObjectInstanceNameReservationResult reserveObjectInstanceName(
      std::wstring const& federationName,
      std::uint64_t federateId,
      std::wstring const& objectInstanceName);
  [[nodiscard]] ObjectInstanceNameReservationStatus releaseObjectInstanceName(
      std::wstring const& federationName,
      std::uint64_t federateId,
      std::wstring const& objectInstanceName);
  [[nodiscard]] MultipleObjectInstanceNameReservationResult
  reserveMultipleObjectInstanceNames(
      std::wstring const& federationName,
      std::uint64_t federateId,
      std::set<std::wstring> const& objectInstanceNames);
  [[nodiscard]] ObjectInstanceNameReservationStatus
  releaseMultipleObjectInstanceNames(
      std::wstring const& federationName,
      std::uint64_t federateId,
      std::set<std::wstring> const& objectInstanceNames);

  [[nodiscard]] ObjectInstanceRegistrationResult registerObjectInstance(
      std::wstring const& federationName,
      std::uint64_t federateId,
      std::uint64_t objectClassHandle,
      std::map<std::uint64_t, std::set<std::uint64_t>> const* updateRegionsByAttribute = nullptr,
      std::wstring const* requestedObjectInstanceName = nullptr);

  [[nodiscard]] ObjectInstanceRegionAssociationStatus associateRegionsForUpdates(
      std::wstring const& federationName,
      std::uint64_t federateId,
      std::uint64_t objectInstanceHandle,
      std::map<std::uint64_t, std::set<std::uint64_t>> const& attributesAndRegions);
  [[nodiscard]] ObjectInstanceRegionAssociationScopePlan
  associateRegionsForUpdatesWithScopeChanges(
      std::wstring const& federationName,
      std::uint64_t federateId,
      std::uint64_t objectInstanceHandle,
      std::map<std::uint64_t, std::set<std::uint64_t>> const& attributesAndRegions);
  [[nodiscard]] ObjectInstanceRegionAssociationStatus unassociateRegionsForUpdates(
      std::wstring const& federationName,
      std::uint64_t federateId,
      std::uint64_t objectInstanceHandle,
      std::map<std::uint64_t, std::set<std::uint64_t>> const& attributesAndRegions);
  [[nodiscard]] ObjectInstanceRegionAssociationScopePlan
  unassociateRegionsForUpdatesWithScopeChanges(
      std::wstring const& federationName,
      std::uint64_t federateId,
      std::uint64_t objectInstanceHandle,
      std::map<std::uint64_t, std::set<std::uint64_t>> const& attributesAndRegions);

  // Accepts the limited no-time deletion request and reserves one removal
  // callback per other known recipient. The deleting federate becomes unknown
  // before this method returns; other recipients remain known until their
  // queued Remove Object Instance callback starts.
  [[nodiscard]] ObjectInstanceDeletionPlan deleteObjectInstance(
      std::wstring const& federationName,
      std::uint64_t deletingFederateId,
      std::uint64_t objectInstanceHandle);

  // Removes only the invoking federate's known-instance state. The object is
  // retained in the federation so a later eligible subscription can discover
  // it again, potentially at a different known class.
  [[nodiscard]] LocalObjectInstanceDeletionStatus localDeleteObjectInstance(
      std::wstring const& federationName,
      std::uint64_t deletingFederateId,
      std::uint64_t objectInstanceHandle);

  // Plans the not-yet-known recipients of one newly registered instance. The
  // registry reserves each returned discovery before releasing its state lock,
  // preventing duplicate queued callbacks while a recipient uses HLA_EVOKED.
  [[nodiscard]] std::vector<ObjectInstanceDiscoveryRecipient>
  planObjectInstanceDiscoveriesForInstance(
      std::wstring const& federationName,
      std::uint64_t objectInstanceHandle);

  // A later subscription can make existing instances discoverable. This
  // mirrors the same private planning boundary as registration without
  // treating a declaration mutation as an object-delivery adapter.
  [[nodiscard]] std::vector<ObjectInstanceDiscoveryRecipient>
  planObjectInstanceDiscoveriesForFederate(
      std::wstring const& federationName,
      std::uint64_t receivingFederateId);

  // Releases a reservation whose callback route could not accept delivery.
  // This deliberately preserves an already-known instance, so it is safe to
  // use after a synchronous HLA_IMMEDIATE callback has entered user code.
  void cancelObjectInstanceDiscovery(
      std::wstring const& federationName,
      std::uint64_t receivingFederateId,
      std::uint64_t objectInstanceHandle);

  // Rechecks a planned Attribute In/Out Of Scope transition immediately
  // before callback entry, suppressing stale work after another region,
  // association, subscription, removal, or resignation change.
  [[nodiscard]] std::set<std::uint64_t> objectInstanceScopeAttributes(
      std::wstring const& federationName,
      std::uint64_t receivingFederateId,
      std::uint64_t objectInstanceHandle,
      std::set<std::uint64_t> const& attributeHandles,
      bool expectedInScope) const;

  // Rechecks and commits a pending no-time removal immediately before user
  // callback delivery. Unlike discovery, it does not re-evaluate declarations:
  // a recipient already known to the deleted instance must be informed.
  [[nodiscard]] std::optional<RemovedObjectInstanceSnapshot> beginObjectInstanceRemoval(
      std::wstring const& federationName,
      std::uint64_t receivingFederateId,
      std::uint64_t objectInstanceHandle);

  // Releases only an undelivered removal reservation. It preserves the
  // recipient's known-instance state so a future recovery path can replan it.
  void cancelObjectInstanceRemoval(
      std::wstring const& federationName,
      std::uint64_t receivingFederateId,
      std::uint64_t objectInstanceHandle);

  // Rechecks the current discovery predicate immediately before the callback
  // executes, then atomically establishes the recipient's known class. A
  // changed subscription, resignation, or stale queued callback becomes an
  // ordinary no-delivery outcome.
  [[nodiscard]] std::optional<KnownObjectInstanceSnapshot> beginObjectInstanceDiscovery(
      std::wstring const& federationName,
      std::uint64_t receivingFederateId,
      std::uint64_t objectInstanceHandle);

  [[nodiscard]] std::optional<KnownObjectInstanceSnapshot> knownObjectInstanceFor(
      std::wstring const& federationName,
      std::uint64_t federateId,
      std::uint64_t objectInstanceHandle) const;
  [[nodiscard]] std::optional<KnownObjectInstanceSnapshot> knownObjectInstanceByNameFor(
      std::wstring const& federationName,
      std::uint64_t federateId,
      std::wstring const& objectInstanceName) const;

  // Plans the RTI-invoked Provide Attribute Value Update callbacks required
  // by the federation-wide Auto Provide switch after a recipient has become
  // newly aware of an object. The supplied tag is intentionally not part of
  // this private plan: the public adapter invokes the callback with an empty
  // tag for this RTI-originated service.
  [[nodiscard]] AttributeValueUpdateRequestPlan planAutoProvideForDiscovery(
      std::wstring const& federationName,
      std::uint64_t receivingFederateId,
      std::uint64_t objectInstanceHandle) const;

  // Validates an Update Attribute Values request and forms one passel for
  // every submitted FOM transportation type and explicit association region
  // set. The common plan feeds bounded receive-order and timestamped traffic;
  // relaxed DDM and update-rate reduction remain outside this boundary.
  [[nodiscard]] ReceiveOrderAttributeUpdatePlan planReceiveOrderAttributeUpdate(
      std::wstring const& federationName,
      std::uint64_t producingFederateId,
       std::uint64_t objectInstanceHandle,
       std::vector<std::uint64_t> const& sentAttributeHandles) const;

  // Recomputes one originally planned passel immediately before callback
  // delivery. A resigning recipient, changed known-instance state, or changed
  // subscription is an ordinary no-delivery outcome; the returned subset is
  // never combined with another passel.
  [[nodiscard]] std::optional<ReceiveOrderAttributeUpdateRecipient>
  receiveOrderAttributeUpdateRecipientFor(
      std::wstring const& federationName,
      std::uint64_t producingFederateId,
      std::uint64_t receivingFederateId,
       std::uint64_t objectInstanceHandle,
       std::vector<std::uint64_t> const& sentAttributeHandles,
       std::set<std::uint64_t> const* sentRegionHandles = nullptr) const;

  // Validates an object-instance Request Attribute Value Update request at
  // the requester's known class, then groups currently owned requested
  // attributes by their providing federate. Attributes owned by the requester
  // are an implicit local provide and therefore have no callback recipient.
  [[nodiscard]] AttributeValueUpdateRequestPlan planAttributeValueUpdateRequest(
      std::wstring const& federationName,
      std::uint64_t requestingFederateId,
      std::uint64_t objectInstanceHandle,
      std::set<std::uint64_t> const& requestedAttributeHandles) const;

  // Rechecks one original owner group immediately before Provide Attribute
  // Value Update delivery. A removed instance, resigned owner/requester, or
  // changed ownership becomes an ordinary no-delivery outcome.
  [[nodiscard]] std::optional<AttributeValueUpdateProvideRecipient>
  attributeValueUpdateProvideRecipientFor(
      std::wstring const& federationName,
      std::uint64_t requestingFederateId,
      std::uint64_t providingFederateId,
      std::uint64_t objectInstanceHandle,
      std::set<std::uint64_t> const& requestedAttributeHandles) const;

  // Validates an object-class Request Attribute Value Update request, then
  // expands it over all current instances registered at the requested class
  // and its subclasses. Unlike the object-instance form, the standard has no
  // requester-known-instance precondition for this form.
  [[nodiscard]] AttributeValueUpdateClassRequestPlan planAttributeValueUpdateClassRequest(
      std::wstring const& federationName,
      std::uint64_t requestingFederateId,
      std::uint64_t objectClassHandle,
      std::set<std::uint64_t> const& requestedAttributeHandles,
      std::map<std::uint64_t, std::set<std::uint64_t>> const* requestRegionsByAttribute =
          nullptr) const;

  // Rechecks one original object-class request owner group immediately before
  // Provide Attribute Value Update delivery. A removed instance, resigned
  // owner/requester, changed class hierarchy, or changed ownership becomes an
  // ordinary no-delivery outcome.
  [[nodiscard]] std::optional<AttributeValueUpdateProvideRecipient>
  attributeValueUpdateClassProvideRecipientFor(
      std::wstring const& federationName,
      std::uint64_t requestingFederateId,
      std::uint64_t providingFederateId,
      std::uint64_t objectInstanceHandle,
      std::uint64_t requestedObjectClassHandle,
      std::set<std::uint64_t> const& requestedAttributeHandles,
      std::map<std::uint64_t, std::set<std::uint64_t>> const* requestRegionsByAttribute =
          nullptr) const;

  // Validates a Query Attribute Ownership request at the requester's known
  // class, then groups requested attributes into their standard C++ owner
  // reports. The bounded runtime can currently emit federate-owned and
  // unowned reports only; RTI-owned state is reserved for later ownership
  // transfer work rather than being encoded as an invented handle value.
  [[nodiscard]] AttributeOwnershipQueryPlan planAttributeOwnershipQuery(
      std::wstring const& federationName,
      std::uint64_t requestingFederateId,
      std::uint64_t objectInstanceHandle,
      std::set<std::uint64_t> const& requestedAttributeHandles) const;

  // Rechecks one pending ownership report immediately before callback
  // delivery. A removed instance, resigned requester, changed known-class
  // boundary, or changed owner state becomes an ordinary no-delivery outcome.
  // In particular, deletion nullifies pending Inform Attribute Ownership
  // callbacks before they can enter user code.
  [[nodiscard]] std::optional<AttributeOwnershipQueryRecipient>
  attributeOwnershipQueryRecipientFor(
      std::wstring const& federationName,
      std::uint64_t requestingFederateId,
      std::uint64_t objectInstanceHandle,
      AttributeOwnershipQueryReportKind reportKind,
      std::uint64_t owningFederateId,
      std::set<std::uint64_t> const& requestedAttributeHandles) const;

  // Determines whether one valid attribute of a requester's known instance is
  // owned by that same joined federate. It has no callback or transfer effect.
  [[nodiscard]] AttributeOwnershipCheckResult attributeOwnedByFederate(
      std::wstring const& federationName,
      std::uint64_t requestingFederateId,
      std::uint64_t objectInstanceHandle,
      std::uint64_t attributeHandle) const;

  // Validates and records one Attribute Ownership Acquisition If Available
  // request. For a nonempty successful request, the returned request ID owns
  // the private Willing to Acquire reservation until callback delivery or
  // an internal invalidating lifecycle transition. The adapter retains the
  // user tag and queues the callback only after releasing its locks. A
  // repeated request made by the same federate for attributes already in that state leaves those attributes
  // unchanged and returns an applied plan without a duplicate callback route.
  [[nodiscard]] AttributeOwnershipAcquisitionIfAvailablePlan
  planAttributeOwnershipAcquisitionIfAvailable(
      std::wstring const& federationName,
      std::uint64_t requestingFederateId,
      std::uint64_t objectInstanceHandle,
      std::set<std::uint64_t> const& desiredAttributeHandles);

  // Resolves one still-pending request immediately before a federate callback
  // executes. A deleted instance, resigned requester, stale request, or
  // inconsistent ownership state becomes a no-delivery outcome. Attributes
  // that remain unowned are atomically assigned to the requester immediately
  // before Attribute Ownership Acquisition Notification is invoked.
  [[nodiscard]] std::optional<AttributeOwnershipAcquisitionIfAvailableDelivery>
  beginAttributeOwnershipAcquisitionIfAvailable(
      std::wstring const& federationName,
      std::uint64_t requestingFederateId,
      std::uint64_t objectInstanceHandle,
      std::uint64_t requestId);

  // Releases an accepted request only when its callback route could not be
  // submitted. It is harmless after immediate callback delivery or another
  // invalidating lifecycle transition has already consumed the reservation.
  void cancelAttributeOwnershipAcquisitionIfAvailable(
      std::wstring const& federationName,
      std::uint64_t requestingFederateId,
      std::uint64_t objectInstanceHandle,
      std::uint64_t requestId);

  // Records a Negotiated Attribute Ownership Divestiture without changing
  // ownership. Existing and later regular Acquisition Pending requests can
  // cause the owning federate to receive the standard Request Divestiture
  // Confirmation callback. The bounded profile does not yet run the complete
  // owner-search lifecycle or select a Willing-to-Acquire request.
  [[nodiscard]] NegotiatedAttributeOwnershipDivestiturePlan
  planNegotiatedAttributeOwnershipDivestiture(
      std::wstring const& federationName,
      std::uint64_t divestingFederateId,
      std::uint64_t objectInstanceHandle,
      std::set<std::uint64_t> const& attributeHandles,
      std::vector<unsigned char> userSuppliedTag);

  // Rechecks a queued Request Divestiture Confirmation immediately before
  // callback entry. Only attributes that still belong to the divesting owner
  // and still have the selected regular acquisition pending are exposed.
  [[nodiscard]] std::optional<RequestDivestitureConfirmationDelivery>
  beginRequestDivestitureConfirmation(
      std::wstring const& federationName,
      std::uint64_t divestingFederateId,
      std::uint64_t acquiringFederateId,
      std::uint64_t objectInstanceHandle,
      std::uint64_t acquisitionRequestId,
      std::set<std::uint64_t> const& scheduledAttributeHandles);

  // Completes the confirmed negotiated transfer atomically. Ownership is
  // visible before the paired Acquisition Notification callback begins, while
  // a private reservation retains the new owner's publication boundary.
  [[nodiscard]] ConfirmDivestiturePlan planConfirmDivestiture(
      std::wstring const& federationName,
      std::uint64_t divestingFederateId,
      std::uint64_t objectInstanceHandle,
      std::set<std::uint64_t> const& attributeHandles,
      std::vector<unsigned char> userSuppliedTag);

  // Consumes one post-confirmation notification reservation immediately
  // before the official Attribute Ownership Acquisition Notification enters
  // user code, then plans any later regular-acquisition work.
  [[nodiscard]] std::optional<ConfirmDivestitureNotificationDelivery>
  beginConfirmDivestitureNotification(
      std::wstring const& federationName,
      std::uint64_t receivingFederateId,
      std::uint64_t objectInstanceHandle,
      std::uint64_t notificationId,
      std::set<std::uint64_t> const& scheduledAttributeHandles);

  // Cancels a still-pending negotiated divestiture and restores ordinary
  // regular-acquisition release planning. Any previously queued confirmation
  // callback becomes stale before it can enter user code.
  [[nodiscard]] CancelNegotiatedAttributeOwnershipDivestiturePlan
  planCancelNegotiatedAttributeOwnershipDivestiture(
      std::wstring const& federationName,
      std::uint64_t divestingFederateId,
      std::uint64_t objectInstanceHandle,
      std::set<std::uint64_t> const& attributeHandles);

  // Validates and records a regular Attribute Ownership Acquisition request.
  // Repeated attributes already pending for the same federate remain in that
  // state and do not produce a duplicate owner-side release callback.  A
  // regular request overrides any still-pending If Available reservation for
  // the same federate and attribute, as required by IEEE 1516.1-2025 7.1.2.2.
  [[nodiscard]] AttributeOwnershipAcquisitionPlan planAttributeOwnershipAcquisition(
      std::wstring const& federationName,
      std::uint64_t requestingFederateId,
      std::uint64_t objectInstanceHandle,
      std::set<std::uint64_t> const& desiredAttributeHandles,
      std::vector<unsigned char> userSuppliedTag);

  // Resolves one queued regular-acquisition notification immediately before
  // user code runs.  It commits ownership only for still-unowned attributes,
  // then returns any next-step work that must be queued after that callback.
  [[nodiscard]] std::optional<AttributeOwnershipAcquisitionNotificationDelivery>
  beginAttributeOwnershipAcquisitionNotification(
      std::wstring const& federationName,
      std::uint64_t requestingFederateId,
      std::uint64_t objectInstanceHandle,
      std::uint64_t requestId,
      std::set<std::uint64_t> const& scheduledAttributeHandles);

  // Rechecks a queued owner-side Request Attribute Ownership Release callback
  // before delivery.  A stale request, removed instance, resigned federate,
  // or changed owner is an ordinary no-delivery outcome.
  [[nodiscard]] std::optional<AttributeOwnershipAcquisitionReleaseDelivery>
  beginAttributeOwnershipAcquisitionRelease(
      std::wstring const& federationName,
      std::uint64_t requestingFederateId,
      std::uint64_t owningFederateId,
      std::uint64_t objectInstanceHandle,
      std::uint64_t requestId,
      std::set<std::uint64_t> const& scheduledAttributeHandles);

  // Validates an owner-side release denial, retains current ownership, and
  // terminates matching regular pending acquisitions.  The adapter supplies
  // the release-denial tag to each resulting unavailable callback.
  [[nodiscard]] AttributeOwnershipReleaseDeniedPlan
  planAttributeOwnershipReleaseDenied(
      std::wstring const& federationName,
      std::uint64_t owningFederateId,
      std::uint64_t objectInstanceHandle,
      std::set<std::uint64_t> const& attributeHandles);

  // Validates and performs a synchronous Attribute Ownership Divestiture If
  // Wanted transfer. The returned set contains only attributes for which a
  // still-pending regular or If Available acquirer was selected. The callback
  // work uses the divestiture tag, not an older acquisition-request tag.
  [[nodiscard]] AttributeOwnershipDivestitureIfWantedPlan
  planAttributeOwnershipDivestitureIfWanted(
      std::wstring const& federationName,
      std::uint64_t divestingFederateId,
      std::uint64_t objectInstanceHandle,
      std::set<std::uint64_t> const& attributeHandles,
      std::vector<unsigned char> userSuppliedTag);

  // Consumes the private post-transfer notification reservation immediately
  // before the standard Acquisition Notification callback enters user code.
  // This is the terminal boundary for the selected acquirer's publication
  // guard and can make other regular acquisition work eligible afterward.
  [[nodiscard]] std::optional<AttributeOwnershipDivestitureIfWantedDelivery>
  beginAttributeOwnershipDivestitureIfWantedNotification(
      std::wstring const& federationName,
      std::uint64_t receivingFederateId,
      std::uint64_t objectInstanceHandle,
      std::uint64_t notificationId,
      std::set<std::uint64_t> const& scheduledAttributeHandles);

  // Validates and performs an Unconditional Attribute Ownership Divestiture.
  // Every supplied attribute is made unowned before this method returns.  It
  // produces follow-up regular-acquisition work plus one grouped Request
  // Attribute Ownership Assumption callback candidate for each currently
  // eligible non-pending joined federate.  The adapter owns the original
  // divestiture tag and queues all returned work after releasing its locks.
  [[nodiscard]] UnconditionalAttributeOwnershipDivestiturePlan
  planUnconditionalAttributeOwnershipDivestiture(
      std::wstring const& federationName,
      std::uint64_t divestingFederateId,
      std::uint64_t objectInstanceHandle,
      std::set<std::uint64_t> const& attributeHandles);

  // Rechecks one queued Request Attribute Ownership Assumption callback at
  // its delivery boundary.  It emits only the original offered attributes
  // that remain unowned and currently eligible at this recipient.  A stale
  // callback is suppressed before it can enter FederateAmbassador code.
  [[nodiscard]] std::optional<AttributeOwnershipAssumptionDelivery>
  attributeOwnershipAssumptionDeliveryFor(
      std::wstring const& federationName,
      std::uint64_t receivingFederateId,
      std::uint64_t objectInstanceHandle,
      std::set<std::uint64_t> const& scheduledAttributeHandles) const;

  // Continues a prior unconditional-divestiture or resign-action assumption
  // search for one federate whose eligibility may have changed.  The method
  // reserves each newly offered (object, attribute, federate) tuple before
  // returning callback records, so repeated publication/discovery events do
  // not duplicate an outstanding assumption callback.  Ownership itself is
  // still established only by a standard acquisition service.
  [[nodiscard]] std::vector<AttributeOwnershipAssumptionRecipient>
  planAttributeOwnershipAssumptionsForFederate(
      std::wstring const& federationName,
      std::uint64_t receivingFederateId);

  // Validates and accepts a regular-acquisition cancellation. The original
  // acquisition work is made stale immediately, but the separate cancellation
  // reservation keeps the caller's required publication active until the
  // confirmation callback begins.
  [[nodiscard]] AttributeOwnershipAcquisitionCancellationPlan
  planAttributeOwnershipAcquisitionCancellation(
      std::wstring const& federationName,
      std::uint64_t requestingFederateId,
      std::uint64_t objectInstanceHandle,
      std::set<std::uint64_t> const& attributeHandles);

  // Rechecks and consumes one queued Confirm Attribute Ownership Acquisition
  // Cancellation callback immediately before it enters user code. Removing
  // the reservation at that boundary releases the paired publication guard
  // and can make later regular acquisition work eligible.
  [[nodiscard]] std::optional<AttributeOwnershipAcquisitionCancellationDelivery>
  beginAttributeOwnershipAcquisitionCancellation(
      std::wstring const& federationName,
      std::uint64_t requestingFederateId,
      std::uint64_t objectInstanceHandle,
      std::uint64_t cancellationId,
      std::set<std::uint64_t> const& scheduledAttributeHandles);

  // Rechecks a queued unavailable callback after release denial.  Object
  // removal, requester resignation, or a changed known-instance boundary
  // suppresses the callback before user code is entered.
  [[nodiscard]] std::optional<AttributeOwnershipUnavailableRecipient>
  attributeOwnershipUnavailableRecipientFor(
      std::wstring const& federationName,
      std::uint64_t receivingFederateId,
      std::uint64_t objectInstanceHandle,
      std::set<std::uint64_t> const& attributeHandles) const;

  // Validates a receive-order Send Interaction request and captures one
  // recipient projection per eligible joined federate.  A passive
  // subscription still receives interactions; its active flag only controls
  // declaration-management advisory callbacks that are outside this slice.
  [[nodiscard]] ReceiveOrderInteractionPlan planReceiveOrderInteraction(
      std::wstring const& federationName,
      std::uint64_t producingFederateId,
      std::uint64_t sentInteractionClassHandle,
      std::vector<std::uint64_t> const& sentParameterHandles,
      std::set<std::uint64_t> const* sentRegionHandles = nullptr) const;

  // Recomputes one recipient projection immediately before callback delivery.
  // A missing recipient, changed subscription, or removed federation is an
  // ordinary no-delivery outcome rather than a callback-time public error.
  [[nodiscard]] std::optional<ReceiveOrderInteractionRecipient>
  receiveOrderInteractionRecipientFor(
      std::wstring const& federationName,
      std::uint64_t producingFederateId,
      std::uint64_t receivingFederateId,
      std::uint64_t sentInteractionClassHandle,
      std::vector<std::uint64_t> const& sentParameterHandles,
      std::set<std::uint64_t> const* sentRegionHandles = nullptr) const;

  // Plans the private RTI report endpoint mandated by §11.5.1.  The exact
  // endpoint contains HLAfederate and HLAserviceGroup point ranges only;
  // report-to-file selects a distinct sink and therefore never also delivers
  // to interaction subscribers.
  [[nodiscard]] MomServiceReportRoutingPlan planMomServiceReport(
      std::wstring const& federationName,
      std::uint64_t reportedFederateId,
      std::uint16_t serviceGroup) const;

  // Atomically obtains the routing decision and serial value for one report
  // accepted for an interaction or report-file sink. Suppressed and invalid
  // requests leave the joined federate's sequence untouched.
  [[nodiscard]] ReservedMomServiceReport reserveMomServiceReport(
      std::wstring const& federationName,
      std::uint64_t reportedFederateId,
      std::uint16_t serviceGroup);

  [[nodiscard]] std::optional<ReceiveOrderInteractionRecipient>
  momServiceReportRecipientFor(
      std::wstring const& federationName,
      std::uint64_t reportedFederateId,
      std::uint64_t receivingFederateId,
      std::uint16_t serviceGroup) const;

  // Captures the mandatory HLAreportFederateLost traffic before a fault-driven
  // resignation removes the affected membership and its time state. The
  // selected recipients are current surviving subscribers; an empty list is
  // a valid outcome when no surviving federate subscribed.
  [[nodiscard]] FederateLostReportPlan planFederateLostReport(
      std::wstring const& federationName,
      std::uint64_t reportedFederateId) const;

  // Rechecks one queued report after the lost membership was removed. Unlike
  // ordinary send interaction routing, the reported federate is deliberately
  // allowed to be departed; its lifetime designator remains in
  // federateNamesById while the receiving federate must still be joined.
  [[nodiscard]] std::optional<ReceiveOrderInteractionRecipient>
  federateLostReportRecipientFor(
      std::wstring const& federationName,
      std::uint64_t reportedFederateId,
      std::uint64_t receivingFederateId) const;

  [[nodiscard]] AttributeTransportationTypeChangePlan
  planAttributeTransportationTypeChange(
      std::wstring const& federationName,
      std::uint64_t requestingFederateId,
      std::uint64_t objectInstanceHandle,
      std::set<std::uint64_t> const& attributeHandles,
      std::string transportationName);

  [[nodiscard]] AttributeOrderTypeChangeStatus changeAttributeOrderType(
      std::wstring const& federationName,
      std::uint64_t requestingFederateId,
      std::uint64_t objectInstanceHandle,
      std::set<std::uint64_t> const& attributeHandles,
      rti1516_2025::OrderType orderType);

  [[nodiscard]] std::optional<AttributeTransportationTypeChangeDelivery>
  beginAttributeTransportationTypeChange(
      std::wstring const& federationName,
      std::uint64_t requestingFederateId,
      std::uint64_t requestId);

  void cancelAttributeTransportationTypeChange(
      std::wstring const& federationName,
      std::uint64_t requestingFederateId,
      std::uint64_t requestId);

  [[nodiscard]] AttributeTransportationTypeDefaultStatus
  changeDefaultAttributeTransportationType(
      std::wstring const& federationName,
      std::uint64_t requestingFederateId,
      std::uint64_t objectClassHandle,
      std::set<std::uint64_t> const& attributeHandles,
      std::string transportationName);

  [[nodiscard]] AttributeOrderTypeDefaultStatus changeDefaultAttributeOrderType(
      std::wstring const& federationName,
      std::uint64_t requestingFederateId,
      std::uint64_t objectClassHandle,
      std::set<std::uint64_t> const& attributeHandles,
      rti1516_2025::OrderType orderType);

  [[nodiscard]] AttributeTransportationTypeQueryPlan
  planAttributeTransportationTypeQuery(
      std::wstring const& federationName,
      std::uint64_t requestingFederateId,
      std::uint64_t objectInstanceHandle,
      std::uint64_t attributeHandle) const;

  [[nodiscard]] std::optional<AttributeTransportationTypeQueryPlan>
  attributeTransportationTypeQueryFor(
      std::wstring const& federationName,
      std::uint64_t requestingFederateId,
      std::uint64_t objectInstanceHandle,
      std::uint64_t attributeHandle) const;

  [[nodiscard]] InteractionTransportationTypeChangePlan
  planInteractionTransportationTypeChange(
      std::wstring const& federationName,
      std::uint64_t requestingFederateId,
      std::uint64_t interactionClassHandle,
      std::string transportationName);

  [[nodiscard]] InteractionOrderTypeChangeStatus changeInteractionOrderType(
      std::wstring const& federationName,
      std::uint64_t requestingFederateId,
      std::uint64_t interactionClassHandle,
      rti1516_2025::OrderType orderType);

  [[nodiscard]] std::optional<std::string>
  beginInteractionTransportationTypeChange(
      std::wstring const& federationName,
      std::uint64_t requestingFederateId,
      std::uint64_t interactionClassHandle);

  void cancelInteractionTransportationTypeChange(
      std::wstring const& federationName,
      std::uint64_t requestingFederateId,
      std::uint64_t interactionClassHandle);

  [[nodiscard]] InteractionTransportationTypeQueryPlan
  planInteractionTransportationTypeQuery(
      std::wstring const& federationName,
      std::uint64_t requestingFederateId,
      std::uint64_t queriedFederateId,
      std::uint64_t interactionClassHandle) const;

  [[nodiscard]] std::optional<InteractionTransportationTypeQueryPlan>
  interactionTransportationTypeQueryFor(
      std::wstring const& federationName,
      std::uint64_t requestingFederateId,
      std::uint64_t queriedFederateId,
      std::uint64_t interactionClassHandle) const;

  // Validates and plans the bounded receive-order Send Directed Interaction
  // overload. The producer must know the target object and publish the
  // directed interaction for its registered object class; each recipient
  // must know that object and subscribe to the class-directed pair.
  [[nodiscard]] ReceiveOrderDirectedInteractionPlan
  planReceiveOrderDirectedInteraction(
      std::wstring const& federationName,
      std::uint64_t producingFederateId,
      std::uint64_t objectInstanceHandle,
      std::uint64_t sentInteractionClassHandle,
      std::vector<std::uint64_t> const& sentParameterHandles) const;

  [[nodiscard]] std::optional<ReceiveOrderDirectedInteractionRecipient>
 receiveOrderDirectedInteractionRecipientFor(
      std::wstring const& federationName,
      std::uint64_t producingFederateId,
      std::uint64_t receivingFederateId,
      std::uint64_t objectInstanceHandle,
      std::uint64_t sentInteractionClassHandle,
      std::vector<std::uint64_t> const& sentParameterHandles) const;

 private:
  [[nodiscard]] RuntimeInstrumentation::Scope beginInstrumentation(
      std::string_view operation) const;

  [[nodiscard]] FederationSaveAdmission admitTimedFederationSaveAtGrantBoundary(
      std::wstring const& federationName,
      std::uint64_t federateId,
      std::shared_ptr<rti1516_2025::LogicalTime const> flushQueueGrantedTime);

  struct Federation {
    struct PendingTimeAdvanceGrant {
      std::uint64_t generation = 0;
      FederationTimeGrantDispatch dispatch;
      bool dispatchQueued = false;
    };

    enum class TsoRecipientDeliveryState {
      pending,
      delivered,
      retracted,
    };

    // The queue tracks only temporal fanout. This companion record tracks the
    // original recipients of the supported timestamped message families
    // through the distinct delivery/retraction states required by 8.22/8.23.
    struct TsoRequestRetractionRecord {
      std::uint64_t producingFederateId = 0;
      std::shared_ptr<rti1516_2025::LogicalTime const> timestamp;
      std::map<std::uint64_t, TsoRecipientDeliveryState>
          recipientStates;
      // An execution-owned retraction designator is terminal after the first
      // successful Retract invocation, even when the original fanout was
      // empty (which is possible for a timestamped Delete Object Instance).
      bool retractionApplied = false;
      // A terminal record is intentionally lightweight: the producer id and
      // recipient states survive for public exception/callback classification,
      // while its timestamp and every reclaimable payload may be released.
      bool terminal = false;
    };

    struct FederateInteractionDeclarations {
      std::set<std::uint64_t> publishedInteractionClasses;
      std::map<std::uint64_t, bool> subscribedInteractionClasses;
      std::map<std::uint64_t, std::map<std::uint64_t, bool>>
          regionalSubscribedInteractionClasses;
      std::map<std::uint64_t, std::set<std::uint64_t>>
          publishedObjectClassDirectedInteractions;
      // Each directed interaction class has its own subscription kind: false
      // means by ownership, true means universal. This matches the 2025
      // service's per-class optional universal-subscription indicator.
      std::map<std::uint64_t, std::map<std::uint64_t, bool>>
          subscribedObjectClassDirectedInteractions;
      // Per-federate interaction transport overrides affect future ordinary
      // and regional Send Interaction calls. A pending change is committed
      // only at the matching confirmation callback boundary.
      std::map<std::uint64_t, std::string> interactionTransportationTypes;
      // Per-federate interaction order overrides affect future sends by this
      // publisher and do not rewrite another federate's preferred order.
      std::map<std::uint64_t, rti1516_2025::OrderType> interactionOrderTypes;
      std::map<std::uint64_t, std::string>
          pendingInteractionTransportationTypeChanges;
    };

    struct SynchronizationPoint {
      std::vector<unsigned char> userSuppliedTag;
      std::set<std::uint64_t> synchronizationSet;
      std::set<std::uint64_t> announcedFederates;
      std::map<std::uint64_t, bool> achievedFederates;
    };

    struct SaveOperation {
      std::wstring label;
      std::map<std::uint64_t, rti1516_2025::SaveStatus> statuses;
      // Untimed saves may reach constrained members at separate Time Advance
      // Grant callback boundaries. Until each direct callback is delivered,
      // it remains outside the save operation's participant set; the standard
      // permits non-constrained members to be instructed only after every
      // constrained member is eligible.
      std::set<std::uint64_t> pendingTimeConstrainedInitiations;
      bool directTimeConstrainedInitiation = false;
      // A non-null value identifies a timestamped direct-admission operation.
      // Each constrained recipient must have received all queued/in-transit
      // TSO payloads through this time before it can receive its direct save
      // callback.
      std::shared_ptr<rti1516_2025::LogicalTime const> scheduledSaveTime;
    };

    struct PendingImmediateSave {
      std::wstring label;
    };

    struct PendingTimedSave {
      std::wstring label;
      std::shared_ptr<rti1516_2025::LogicalTime const> timestamp;
    };

    struct RestoreOperation {
      std::wstring label;
      std::map<std::uint64_t, rti1516_2025::RestoreStatus> statuses;
    };

    struct ObjectClassAttributeDeclarations {
      struct PerObjectClass {
        std::set<std::uint64_t> explicitlyPublishedAttributes;
        std::map<std::uint64_t, bool> subscribedAttributes;
        std::map<std::uint64_t, std::string> subscribedUpdateRateDesignators;
        std::map<std::uint64_t, std::map<std::uint64_t, bool>>
            regionalSubscribedAttributes;
        std::map<std::uint64_t, std::map<std::uint64_t, std::string>>
            regionalSubscribedUpdateRateDesignators;
        // A class default is scoped to one federate's future instance
        // attributes. Existing object instances retain their captured value.
        std::map<std::uint64_t, std::string> defaultTransportationTypes;
        std::map<std::uint64_t, rti1516_2025::OrderType> defaultOrderTypes;
        bool privilegeToDeleteExplicitlyUnpublished = false;
      };

      std::map<std::uint64_t, PerObjectClass> byObjectClass;
    };

    struct Region {
      std::uint64_t ownerFederateId = 0;
      std::set<std::uint64_t> dimensionHandles;
      std::map<std::uint64_t, RegionRangeBounds> pendingRangeBounds;
      std::map<std::uint64_t, RegionRangeBounds> committedRangeBounds;
      bool specificationCommitted = false;
      bool inUse = false;
    };

      struct ObjectInstance {
      struct PendingAttributeOwnershipAcquisitionIfAvailable {
        std::uint64_t requestingFederateId = 0;
        std::uint64_t requestSequence = 0;
        std::set<std::uint64_t> desiredAttributeHandles;
      };

      struct PendingAttributeOwnershipAcquisition {
        std::uint64_t requestingFederateId = 0;
        std::uint64_t requestSequence = 0;
        std::set<std::uint64_t> desiredAttributeHandles;
        std::set<std::uint64_t> notificationQueuedAttributeHandles;
        std::map<std::uint64_t, std::set<std::uint64_t>>
            releaseCallbacksQueuedByOwningFederate;
        std::vector<unsigned char> userSuppliedTag;
      };

      struct PendingAttributeOwnershipAcquisitionCancellation {
        std::uint64_t requestingFederateId = 0;
        std::set<std::uint64_t> attributeHandles;
      };

      struct PendingAttributeOwnershipDivestitureIfWantedNotification {
        std::uint64_t receivingFederateId = 0;
        std::set<std::uint64_t> attributeHandles;
      };

      // A negotiated divestiture leaves the owner in the private Waiting for
      // a New Owner to be Found state. The selected regular acquisition is
      // retained until Confirm Divestiture, so a stale confirmation callback
      // never transfers ownership by itself.
      struct PendingNegotiatedAttributeOwnershipDivestiture {
        std::uint64_t divestingFederateId = 0;
        std::uint64_t acquiringFederateId = 0;
        std::uint64_t acquisitionRequestId = 0;
        bool confirmationQueued = false;
        bool confirmationDelivered = false;
        std::vector<unsigned char> userSuppliedTag;
      };

      struct PendingConfirmDivestitureNotification {
        std::uint64_t receivingFederateId = 0;
        std::set<std::uint64_t> attributeHandles;
      };

      struct PendingAttributeTransportationTypeChange {
        std::uint64_t requestingFederateId = 0;
        std::set<std::uint64_t> attributeHandles;
        std::string transportationName;
      };

      std::uint64_t handle = 0;
      std::wstring name;
      std::uint64_t registeredObjectClassHandle = 0;
      std::uint64_t producingFederateId = 0;
      // Registration seeds every currently published attribute with the
      // producing federate as owner. Keeping the owner per attribute avoids a
      // second state model as the initial 2025 ownership slices are added.
      std::map<std::uint64_t, std::uint64_t> attributeOwnersByHandle;
      // The effective type is captured for every owned attribute at
      // registration/ownership-transfer time, so later class-default changes
      // cannot rewrite existing instances.
      std::map<std::uint64_t, std::string> attributeTransportationTypes;
      // Preferred order is likewise captured per instance attribute. An
      // ownership transfer resets it from the acquiring federate's class
      // default before the ownership notification is delivered.
      std::map<std::uint64_t, rti1516_2025::OrderType> attributeOrderTypes;
      // Explicit region associations used by the no-time Update Attribute
      // Values path. Missing entries retain the ordinary no-region behavior.
      std::map<std::uint64_t, std::set<std::uint64_t>> updateRegionsByAttribute;
      std::map<std::uint64_t, PendingAttributeOwnershipAcquisitionIfAvailable>
          pendingAttributeOwnershipAcquisitionIfAvailableRequests;
      std::map<std::uint64_t, PendingAttributeOwnershipAcquisition>
          pendingAttributeOwnershipAcquisitionRequests;
      std::map<std::uint64_t, PendingAttributeOwnershipAcquisitionCancellation>
          pendingAttributeOwnershipAcquisitionCancellations;
      std::map<std::uint64_t,
               PendingAttributeOwnershipDivestitureIfWantedNotification>
          pendingAttributeOwnershipDivestitureIfWantedNotifications;
      // For each attribute made unowned by an unconditional divestiture or
      // resign action, retain the federates that have already received an
      // assumption offer.  A later join, discovery, or publication can then
      // continue the search without duplicating the same callback.
      std::map<std::uint64_t, std::set<std::uint64_t>>
          ownershipAssumptionRecipientsByAttribute;
      std::map<std::uint64_t, PendingNegotiatedAttributeOwnershipDivestiture>
          pendingNegotiatedAttributeOwnershipDivestitures;
      std::map<std::uint64_t, PendingConfirmDivestitureNotification>
          pendingConfirmDivestitureNotifications;
      std::map<std::uint64_t, PendingAttributeTransportationTypeChange>
          pendingAttributeTransportationTypeChanges;
      std::map<std::uint64_t, std::uint64_t> knownObjectClassHandlesByFederate;
      std::set<std::uint64_t> pendingDiscoveryFederates;
      bool deleteAccepted = false;
      std::set<std::uint64_t> pendingRemovalFederates;
      std::optional<std::uint64_t> pendingTimestampedDeletionMessageId;
      std::set<std::uint64_t> pendingTimestampedRemovalFederates;
    };

    // A timestamped Delete Object Instance is unusual among the currently
    // supported retractable message families: the standard requires a legal
    // post-delivery Retract to reconstitute the object and reassume its
    // invocation-time ownership.  Keep that state execution-owned, rather
    // than trying to reconstruct it from recipient callbacks after the
    // original object has been deleted.
    struct TsoObjectDeletionReconstitutionRecord {
      ObjectInstance objectInstanceAtInvocation;
    };

    FederationDefinition definition;
    // This is deliberately independent from every private handle directory.
    // IEEE 1516.1 normalized values must be RTI-originated rather than a
    // promise that their values follow the implementation's handle sequence.
    std::uint64_t normalizationSeed = 0;
    // Auto Provide is a federation-wide dynamic switch. The current private
    // runtime seeds it from the creation FDD; MOM switch adjustment is a
    // separate service path and is not silently inferred from an additional
    // FOM join.
    bool autoProvideSwitch = false;
    // Advisories Use Known Class is a static, federation-wide switch. It is
    // captured at federation creation and survives additional-FOM joins and
    // save/restore as one value shared by every current member.
    bool advisoriesUseKnownClassSwitch = false;
    // Non-Regulated Grant has the same static federation-wide lifetime as
    // Advisories Use Known Class and is consumed by the time-bound calculator.
    bool nonRegulatedGrantSwitch = false;
    // Static FDD switches retained for the lifetime of the execution.
    bool delaySubscriptionEvaluationSwitch = false;
    bool allowRelaxedDDMSwitch = false;
    std::shared_ptr<ObjectClassHandleDirectory const> objectClassHandles;
    std::shared_ptr<AttributeHandleDirectory const> attributeHandles;
    std::shared_ptr<InteractionClassHandleDirectory const> interactionClassHandles;
    std::shared_ptr<ParameterHandleDirectory const> parameterHandles;
    std::shared_ptr<DimensionHandleDirectory const> dimensionHandles;
    std::map<std::uint64_t, FederateMembership> members;
    std::map<std::wstring, std::uint64_t> memberIdsByName;
    // Unlike memberIdsByName, this lifetime index is not removed at resign.
    // IEEE 1516.1 defines a valid federate designator as one returned by Join
    // during this execution, even if the federate is no longer joined.
    std::map<std::uint64_t, std::wstring> federateNamesById;
    std::map<std::uint64_t, FederateInteractionDeclarations> interactionDeclarations;
    std::map<std::wstring, SynchronizationPoint> synchronizationPoints;
    std::optional<SaveOperation> saveOperation;
    std::optional<PendingImmediateSave> pendingImmediateSave;
    std::optional<PendingTimedSave> pendingTimedSave;
    std::optional<RestoreOperation> restoreOperation;
    std::map<std::uint64_t, ObjectClassAttributeDeclarations> objectClassAttributeDeclarations;
    std::map<std::uint64_t, InteractionCallbackRoute> interactionCallbackRoutes;
    std::set<std::pair<std::uint64_t, std::uint64_t>>
        objectClassRegistrationRelevance;
    std::set<std::pair<std::uint64_t, std::uint64_t>> interactionRelevance;
    FederationTimeCoordinator timeCoordinator;
    std::map<std::uint64_t, TsoInteractionMessage> tsoInteractionMessages;
    std::map<std::uint64_t, TsoRequestRetractionRecord>
        tsoRequestRetractionRecords;
    std::map<std::uint64_t, TsoAttributeUpdateMessage>
        tsoAttributeUpdateMessages;
    std::map<std::uint64_t, TsoObjectDeletionMessage>
        tsoObjectDeletionMessages;
    std::map<std::uint64_t, TsoObjectDeletionReconstitutionRecord>
        tsoObjectDeletionReconstitutionRecords;
    std::map<std::uint64_t, TsoDirectedInteractionMessage>
        tsoDirectedInteractionMessages;
    std::map<std::uint64_t, PendingTimeAdvanceGrant> pendingTimeAdvanceGrants;
    std::map<std::uint64_t, Region> regions;
    // RTI-created joined-federate MOM objects reserve common object-instance
    // handle values but are not inserted into objectInstances. The ordinary
    // object-management paths therefore cannot accidentally treat them as
    // federate-produced, transfer-capable instances before the RTI-owned
    // callback protocol is implemented.
    std::map<std::uint64_t, JoinedFederateMomObjectSnapshot>
        rtiOwnedJoinedFederateMomObjects;
    std::map<std::uint64_t, ObjectInstance> objectInstances;
    std::map<std::wstring, std::uint64_t> objectInstanceHandlesByName;
    std::map<std::wstring, std::uint64_t> reservedObjectInstanceNamesByFederate;
    std::uint64_t nextRegionHandle = 1;
    std::uint64_t nextObjectInstanceHandle = 1;
    std::uint64_t nextAttributeOwnershipAcquisitionIfAvailableRequestId = 1;
    std::uint64_t nextAttributeOwnershipAcquisitionRequestId = 1;
    std::uint64_t nextAttributeOwnershipAcquisitionRequestSequence = 1;
    std::uint64_t nextAttributeOwnershipAcquisitionCancellationId = 1;
    std::uint64_t nextAttributeOwnershipDivestitureIfWantedNotificationId = 1;
    std::uint64_t nextConfirmDivestitureNotificationId = 1;
    std::uint64_t nextAttributeTransportationTypeChangeRequestId = 1;
  };

  [[nodiscard]] static bool isFirstPendingAttributeOwnershipAcquisition(
      Federation::ObjectInstance const& instance,
      std::uint64_t requestId,
      std::uint64_t attributeHandle);
  [[nodiscard]] static std::vector<AttributeOwnershipAcquisitionWorkItem>
  planPendingAttributeOwnershipAcquisitionWork(
      Federation& federation,
      Federation::ObjectInstance& instance);
  [[nodiscard]] static std::vector<AttributeOwnershipAcquisitionWorkItem>
  planPendingNegotiatedAttributeOwnershipDivestitureConfirmations(
      Federation& federation,
      Federation::ObjectInstance& instance);

  [[nodiscard]] static std::optional<ReceiveOrderInteractionRecipient>
  candidateReceiveOrderInteractionRecipient(
      Federation const& federation,
      InteractionProducer const& producingSource,
      std::uint64_t receivingFederateId,
      std::uint64_t sentInteractionClassHandle,
      std::vector<std::uint64_t> const& sentParameterHandles,
      std::set<std::uint64_t> const* sentRegionHandles,
      std::map<std::uint64_t, RegionSpecificationSnapshot> const* regionOverrides = nullptr);
  [[nodiscard]] static MomServiceReportRoutingPlan momServiceReportRoutingPlanFor(
      Federation const& federation,
      std::uint64_t reportedFederateId,
      std::uint16_t serviceGroup);
  [[nodiscard]] static std::optional<FederateLostReportRouting>
  federateLostReportRoutingFor(
      Federation const& federation,
      std::uint64_t reportedFederateId);
  [[nodiscard]] static bool validDirectedInteractionForObjectClass(
      Federation const& federation,
      std::uint64_t objectClassHandle,
      std::uint64_t interactionClassHandle);
  [[nodiscard]] static bool directedInteractionDeclarationApplies(
      Federation const& federation,
      std::uint64_t federateId,
      std::uint64_t registeredObjectClassHandle,
      std::uint64_t interactionClassHandle,
      bool publication);
  [[nodiscard]] static std::optional<bool>
  directedInteractionSubscriptionIsUniversal(
      Federation const& federation,
      std::uint64_t federateId,
      std::uint64_t registeredObjectClassHandle,
      std::uint64_t interactionClassHandle);
  [[nodiscard]] static std::optional<ReceiveOrderDirectedInteractionRecipient>
  candidateReceiveOrderDirectedInteractionRecipient(
      Federation const& federation,
      std::uint64_t producingFederateId,
      std::uint64_t receivingFederateId,
      std::uint64_t objectInstanceHandle,
      std::uint64_t sentInteractionClassHandle,
      std::vector<std::uint64_t> const& sentParameterHandles);
  [[nodiscard]] static std::optional<ReceiveOrderAttributeUpdateRecipient>
  candidateReceiveOrderAttributeUpdateRecipient(
      Federation const& federation,
      std::uint64_t producingFederateId,
      std::uint64_t receivingFederateId,
      std::uint64_t objectInstanceHandle,
      std::vector<std::uint64_t> const& sentAttributeHandles,
      std::set<std::uint64_t> const* sentRegionHandles);
  [[nodiscard]] static std::optional<AttributeValueUpdateProvideRecipient>
  candidateAttributeValueUpdateProvideRecipient(
      Federation const& federation,
      std::uint64_t requestingFederateId,
      std::uint64_t providingFederateId,
      std::uint64_t objectInstanceHandle,
      std::set<std::uint64_t> const& requestedAttributeHandles);
  [[nodiscard]] static std::optional<AttributeValueUpdateProvideRecipient>
  candidateAttributeValueUpdateClassProvideRecipient(
      Federation const& federation,
      std::uint64_t requestingFederateId,
      std::uint64_t providingFederateId,
      std::uint64_t objectInstanceHandle,
      std::uint64_t requestedObjectClassHandle,
      std::set<std::uint64_t> const& requestedAttributeHandles,
      std::map<std::uint64_t, std::set<std::uint64_t>> const* requestRegionsByAttribute =
          nullptr);
  [[nodiscard]] static std::optional<AttributeOwnershipQueryRecipient>
  candidateAttributeOwnershipQueryRecipient(
      Federation const& federation,
      std::uint64_t requestingFederateId,
      std::uint64_t objectInstanceHandle,
      AttributeOwnershipQueryReportKind reportKind,
      std::uint64_t owningFederateId,
      std::set<std::uint64_t> const& requestedAttributeHandles);
  [[nodiscard]] static bool objectInstanceRegisteredAtOrBelowClass(
      Federation const& federation,
      std::uint64_t registeredObjectClassHandle,
      std::uint64_t requestedObjectClassHandle);

  [[nodiscard]] static bool validDefinition(
      std::wstring const& federationName,
      FederationDefinition const& definition);
  [[nodiscard]] static unsigned long normalizedHandleValue(
      std::uint64_t normalizationSeed,
      std::uint64_t handleValue,
      std::uint64_t handleKind) noexcept;

  FederationJoinResult joinImpl(
      std::wstring const& federationName,
      FederationDefinition* replacementDefinition,
      std::shared_ptr<FederateTimeState> timeState,
      std::wstring const& federateType,
      std::optional<std::wstring> requestedFederateName,
      InteractionCallbackRoute interactionCallbackRoute);

  [[nodiscard]] static std::optional<FederationTimeExecutionSnapshot> makeTimeSnapshot(
      Federation const& federation);

  [[nodiscard]] static bool validInteractionClass(
      Federation const& federation,
      std::uint64_t interactionClassHandle);
  [[nodiscard]] static bool isReportServiceInvocationInteractionClass(
      Federation const& federation,
      std::uint64_t interactionClassHandle);
  [[nodiscard]] static bool hasReportServiceInvocationSubscription(
      Federation const& federation,
      std::uint64_t federateId);
  [[nodiscard]] static std::optional<std::set<std::uint64_t>>
  availableInteractionDimensions(
      Federation const& federation,
      std::uint64_t interactionClassHandle);
  [[nodiscard]] static std::optional<std::set<std::uint64_t>>
  availableObjectClassDimensions(
      Federation const& federation,
      std::uint64_t objectClassHandle);
  [[nodiscard]] static bool regionsOverlap(
      Federation const& federation,
      std::uint64_t firstRegionHandle,
      std::uint64_t secondRegionHandle);
  // Applies strict IEEE 1516.1-2025 range overlap and Umbra's documented
  // Allow Relaxed DDM policy. With the static federation switch enabled,
  // exactly touching committed ranges qualify; a strict overlap always
  // qualifies and a nonzero gap never does.
  [[nodiscard]] static bool regionsOverlap(
      Federation const& federation,
      std::uint64_t firstRegionHandle,
      std::uint64_t secondRegionHandle,
      std::map<std::uint64_t, RegionSpecificationSnapshot> const* overrides);
  // The standard default region spans every FDD dimension and cannot be
  // referenced by a federate.  This predicate preserves that invisible
  // relationship without allocating a synthetic public RegionHandle.  An
  // explicit realization with no dimensions still cannot overlap it.
  [[nodiscard]] static bool regionOverlapsDefault(
      Federation const& federation,
      std::uint64_t regionHandle,
      std::map<std::uint64_t, RegionSpecificationSnapshot> const* overrides = nullptr);
  // An explicit update-region association belongs to the current owner of an
  // instance attribute.  IEEE 1516.1-2025 removes that association whenever
  // ownership is divested, transferred, or otherwise lost; it is not carried
  // forward to a later owner.
  static void clearUpdateRegionAssociation(
      Federation::ObjectInstance& objectInstance,
      std::uint64_t attributeHandle) noexcept;
  static void refreshRegionUsage(Federation& federation);

  [[nodiscard]] static bool validObjectClass(
      Federation const& federation,
      std::uint64_t objectClassHandle);
  [[nodiscard]] static bool validObjectClassAttributes(
      Federation const& federation,
      std::uint64_t objectClassHandle,
      std::set<std::uint64_t> const& attributeHandles);
  [[nodiscard]] static std::optional<std::string> attributeTransportationName(
      Federation const& federation,
      std::uint64_t objectClassHandle,
      std::uint64_t attributeHandle);
  [[nodiscard]] static std::optional<rti1516_2025::OrderType>
  orderTypeFromName(std::string const& orderName);
  [[nodiscard]] static std::optional<rti1516_2025::OrderType>
  attributeDefaultOrderType(
      Federation const& federation,
      std::uint64_t federateId,
      std::uint64_t objectClassHandle,
      std::uint64_t attributeHandle);
  [[nodiscard]] static std::optional<rti1516_2025::OrderType>
  effectiveAttributeOrderType(
      Federation const& federation,
      Federation::ObjectInstance const& objectInstance,
      std::uint64_t attributeHandle);
  [[nodiscard]] static std::optional<rti1516_2025::OrderType>
  interactionOrderType(
      Federation const& federation,
      std::uint64_t federateId,
      std::uint64_t interactionClassHandle);
  [[nodiscard]] static std::optional<std::string>
  attributeDefaultTransportationName(
      Federation const& federation,
      std::uint64_t federateId,
      std::uint64_t objectClassHandle,
      std::uint64_t attributeHandle);
  [[nodiscard]] static std::optional<std::string>
  effectiveAttributeTransportationName(
      Federation const& federation,
      Federation::ObjectInstance const& objectInstance,
      std::uint64_t attributeHandle);
  [[nodiscard]] static std::optional<std::string>
  effectiveInteractionTransportationName(
      Federation const& federation,
      std::uint64_t federateId,
      std::uint64_t interactionClassHandle);
  [[nodiscard]] static std::optional<std::set<std::uint64_t>> publishedObjectClassAttributes(
      Federation const& federation,
      std::uint64_t federateId,
      std::uint64_t objectClassHandle);
  [[nodiscard]] static std::optional<std::uint64_t> candidateObjectInstanceDiscoveryClass(
      Federation const& federation,
      Federation::ObjectInstance const& objectInstance,
      std::uint64_t receivingFederateId);
  [[nodiscard]] static bool objectAttributeInScope(
      Federation const& federation,
      Federation::ObjectInstance const& objectInstance,
      std::uint64_t receivingFederateId,
      std::uint64_t attributeHandle,
      std::map<std::uint64_t, RegionSpecificationSnapshot> const* overrides = nullptr,
      std::map<std::uint64_t, std::set<std::uint64_t>> const* associationOverrides = nullptr,
      Federation::ObjectClassAttributeDeclarations const* declarationOverrides = nullptr);
  [[nodiscard]] static std::optional<std::string>
  subscribedUpdateRateDesignatorForAttribute(
      Federation const& federation,
      Federation::ObjectInstance const& objectInstance,
      std::uint64_t receivingFederateId,
      std::uint64_t attributeHandle);
  [[nodiscard]] static std::vector<ObjectInstanceScopeChangeRecipient>
  objectInstanceScopeChangesForAssociation(
      Federation const& federation,
      Federation::ObjectInstance const& objectInstance,
      std::set<std::uint64_t> const& attributeHandles,
      std::map<std::uint64_t, std::set<std::uint64_t>> const& associationOverrides);
  [[nodiscard]] static std::vector<ObjectInstanceScopeChangeRecipient>
  objectInstanceScopeChangesForSubscription(
      Federation const& federation,
      std::uint64_t receivingFederateId,
      std::set<std::uint64_t> const& attributeHandles,
      Federation::ObjectClassAttributeDeclarations const& previousDeclarations);
  [[nodiscard]] static std::vector<AttributeRelevanceAdvisoryRecipient>
  attributeRelevanceAdvisoriesForScopeChanges(
      Federation const& federation,
      std::vector<ObjectInstanceScopeChangeRecipient> const& scopeChanges);
  [[nodiscard]] static std::optional<KnownObjectInstanceSnapshot> knownObjectInstanceSnapshot(
      Federation const& federation,
      Federation::ObjectInstance const& objectInstance,
      std::uint64_t federateId);
  [[nodiscard]] static bool canPurgeDeletedObjectInstance(
      Federation::ObjectInstance const& objectInstance) noexcept;

  [[nodiscard]] static bool hasPendingTsoRecipient(
      Federation const& federation,
      Federation::TsoRequestRetractionRecord const& record) noexcept;
  static void reclaimTsoMessagePayload(
      Federation& federation,
      std::uint64_t messageId);
  static void reclaimTsoMessagePayloads(Federation& federation);

  [[nodiscard]] static std::vector<FederationTimeGrantDispatch> scheduleEligibleTimeAdvanceGrants(
      Federation& federation);

  // Starts a federation-wide save after all admission/readiness checks have
  // passed.  The caller owns mutex_ and supplies a result whose callbacks are
  // submitted only after that lock is released.
  [[nodiscard]] static bool startFederationSave(
      Federation& federation,
      std::wstring label,
      FederationSaveControlResult& result);

  static void appendSaveCompletionNotifications(
      Federation& federation,
      bool successful,
      rti1516_2025::SaveFailureReason failureReason,
      std::vector<FederationSaveNotification>& notifications,
      std::optional<std::uint64_t> excludedFederateId = std::nullopt);

  static void appendRestoreCompletionNotifications(
      Federation& federation,
      bool successful,
      rti1516_2025::RestoreFailureReason failureReason,
      std::vector<FederationRestoreNotification>& notifications,
      std::optional<std::uint64_t> excludedFederateId = std::nullopt);

  static void restoreFederationFromSnapshot(
      Federation& target,
      Federation const& snapshot);

  [[nodiscard]] FederationRegistryResult resignLocked(
      std::wstring const& federationName,
      std::uint64_t federateId,
      rti1516_2025::ResignAction resignAction,
      bool forcedConnectionLoss);

  mutable std::mutex mutex_;
  std::shared_ptr<RuntimeInstrumentation> instrumentation_;
  std::map<std::wstring, Federation> federations_;
  // One or more completed labels may be restored while the federation
  // remains alive.  These snapshots are intentionally process-local and
  // immutable after save completion; no filesystem serialization is implied.
  std::map<std::wstring, std::map<std::wstring, Federation>> saveSnapshots_;
  std::uint64_t nextFederateId_ = 1;
};

}  // namespace umbra::detail
