#pragma once

#include "internal/attribute_handle_directory.hpp"
#include "internal/dimension_handle_directory.hpp"
#include "internal/federation_time_coordinator.hpp"
#include "internal/fom_validation.hpp"
#include "internal/interaction_class_handle_directory.hpp"
#include "internal/object_class_handle_directory.hpp"
#include "internal/parameter_handle_directory.hpp"

#include <RTI/VariableLengthData.h>

#include <cstddef>
#include <cstdint>
#include <functional>
#include <map>
#include <memory>
#include <mutex>
#include <optional>
#include <set>
#include <string>
#include <utility>
#include <variant>
#include <vector>

namespace rti1516_2025 {
class FederateAmbassador;
}

namespace umbra::detail {

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
  bool attributeScopeAdvisorySwitch = false;
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
};

struct FederationRegistryResult {
  FederationRegistryStatus status = FederationRegistryStatus::applied;
};

struct FederationJoinResult {
  FederationRegistryStatus status = FederationRegistryStatus::applied;
  std::optional<FederateMembership> membership;
};

// A binding-owned callback endpoint is registered in the same private
// membership transaction as its federate.  The registry never calls it while
// locked; the binding queues it onto the recipient's selected callback model.
using FederateCallbackInvocation = std::function<void(rti1516_2025::FederateAmbassador&)>;
using FederateCallbackRoute = std::function<void(FederateCallbackInvocation)>;

// The aliases retain the names used by the existing limited interaction
// slice while making the same callback lifetime boundary available to the
// object-instance discovery foundation.
using InteractionCallbackInvocation = FederateCallbackInvocation;
using InteractionCallbackRoute = FederateCallbackRoute;
using ObjectInstanceCallbackInvocation = FederateCallbackInvocation;
using ObjectInstanceCallbackRoute = FederateCallbackRoute;

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
};

struct InteractionClassDeclarationSnapshot {
  bool published = false;
  std::optional<bool> subscriptionActive;
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
  ownership_acquisition_pending,
  inconsistent_catalog,
};

enum class RegionalObjectClassAttributeDeclarationStatus {
  applied,
  federation_does_not_exist,
  federate_not_member,
  object_class_not_defined,
  attribute_not_defined,
  invalid_region_context,
  region_not_created_by_this_federate,
  invalid_region,
  inconsistent_catalog,
};

struct ObjectClassAttributeDeclarationSnapshot {
  std::set<std::uint64_t> explicitlyPublishedAttributes;
  std::map<std::uint64_t, bool> subscribedAttributes;
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

struct RegionScopeChangePlan {
  RegionServiceStatus status = RegionServiceStatus::applied;
  std::vector<ObjectInstanceScopeChangeRecipient> recipients;
};

struct ObjectInstanceRegionAssociationScopePlan {
  ObjectInstanceRegionAssociationStatus status =
      ObjectInstanceRegionAssociationStatus::applied;
  std::vector<ObjectInstanceScopeChangeRecipient> recipients;
};

struct ObjectClassAttributeSubscriptionScopePlan {
  ObjectClassAttributeDeclarationStatus status =
      ObjectClassAttributeDeclarationStatus::applied;
  std::vector<ObjectInstanceScopeChangeRecipient> recipients;
};

struct RegionalObjectClassAttributeSubscriptionScopePlan {
  RegionalObjectClassAttributeDeclarationStatus status =
      RegionalObjectClassAttributeDeclarationStatus::applied;
  std::vector<ObjectInstanceScopeChangeRecipient> recipients;
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
  ObjectInstanceCallbackRoute callbackRoute;
};

struct ReceiveOrderAttributeUpdatePassel {
  std::string transportationName;
  std::vector<std::uint64_t> sentAttributeHandles;
  std::set<std::uint64_t> sentRegionHandles;
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
// foundation.  The registry computes a recipient's closest subscribed
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

struct ReceiveOrderInteractionRecipient {
  std::uint64_t federateId = 0;
  std::uint64_t receivedInteractionClassHandle = 0;
  std::set<std::uint64_t> receivedParameterHandles;
  InteractionCallbackRoute callbackRoute;
};

struct ReceiveOrderInteractionPlan {
  ReceiveOrderInteractionStatus status = ReceiveOrderInteractionStatus::applied;
  std::string transportationName;
  std::vector<ReceiveOrderInteractionRecipient> recipients;
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
  std::shared_ptr<rti1516_2025::LogicalTime const> timestamp;
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

using TsoPayloadDelivery =
    std::variant<
        TsoInteractionDelivery,
        TsoAttributeUpdateDelivery,
        TsoObjectDeletionDelivery>;

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

struct FederationTsoRetractionResult {
  FederationTsoRegistryStatus status = FederationTsoRegistryStatus::applied;
  TsoMessageRetractionResult queueResult;
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
      std::uint64_t federateId);

  [[nodiscard]] bool contains(std::wstring const& federationName) const;
  [[nodiscard]] std::size_t memberCount(std::wstring const& federationName) const;
  [[nodiscard]] std::optional<FederationDefinition> definitionFor(
      std::wstring const& federationName) const;
  [[nodiscard]] std::optional<FederationTimeExecutionSnapshot> timeSnapshotFor(
      std::wstring const& federationName) const;

  [[nodiscard]] std::optional<bool> attributeScopeAdvisorySwitchFor(
      std::wstring const& federationName,
      std::uint64_t federateId) const;

  [[nodiscard]] FederationRegistryStatus setAttributeScopeAdvisorySwitch(
      std::wstring const& federationName,
      std::uint64_t federateId,
      bool switchValue);

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

  // Atomically allocates one message-retraction designator, retains the
  // interaction payload, and queues it for the selected time-constrained
  // recipients.  The public adapter performs the standards-derived
  // publication/parameter validation and passes only those recipient ids
  // that have an active time-constrained role.
  [[nodiscard]] FederationTsoInteractionEnqueueResult enqueueTsoInteraction(
      std::wstring const& federationName,
      TsoInteractionMessage message,
      std::vector<std::uint64_t> const& recipientFederateIds);

  // Atomically allocates one message-retraction designator, retains the
  // attribute-update payload, and queues it for selected time-constrained
  // recipients.  The payload keeps recipient-specific passels so a changed
  // subscription can only suppress, never invent, a reflection at delivery.
  [[nodiscard]] FederationTsoAttributeUpdateEnqueueResult
  enqueueTsoAttributeUpdate(
      std::wstring const& federationName,
      TsoAttributeUpdateMessage message,
      std::vector<std::uint64_t> const& recipientFederateIds);

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
      std::uint64_t messageId);

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

  // Begins one timestamped removal callback.  The object remains
  // reconstitutable until the first delivery; retraction before that boundary
  // clears the pending-delete marker and leaves all known-instance state
  // intact.
  [[nodiscard]] std::optional<RemovedObjectInstanceSnapshot>
  beginTsoObjectInstanceRemoval(
      std::wstring const& federationName,
      std::uint64_t receivingFederateId,
      std::uint64_t objectInstanceHandle,
      std::uint64_t messageId);

  // Cancels an accepted timestamped delete before any removal callback has
  // begun.  The caller must have already withdrawn the queue fanout.
  [[nodiscard]] bool cancelTsoObjectInstanceDeletion(
      std::wstring const& federationName,
      std::uint64_t producingFederateId,
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
  [[nodiscard]] std::optional<std::uint64_t> objectClassHandleFor(
      std::wstring const& federationName,
      std::string const& objectClassName) const;
  [[nodiscard]] std::optional<std::string> objectClassNameFor(
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
  [[nodiscard]] std::optional<std::uint64_t> parameterHandleFor(
      std::wstring const& federationName,
      std::string const& interactionClassName,
      std::string const& parameterName) const;
  [[nodiscard]] std::optional<std::string> parameterNameFor(
      std::wstring const& federationName,
      std::string const& interactionClassName,
      std::uint64_t parameterHandle) const;
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
      std::optional<bool> active);
  [[nodiscard]] ObjectClassAttributeSubscriptionScopePlan
  setObjectClassAttributeSubscriptionWithScopeChanges(
      std::wstring const& federationName,
      std::uint64_t federateId,
      std::uint64_t objectClassHandle,
      std::set<std::uint64_t> const& attributeHandles,
      std::optional<bool> active);
  RegionalObjectClassAttributeDeclarationStatus
  setObjectClassAttributeRegionalSubscription(
      std::wstring const& federationName,
      std::uint64_t federateId,
      std::uint64_t objectClassHandle,
      std::map<std::uint64_t, std::set<std::uint64_t>> const& attributesAndRegions,
      bool active);
  [[nodiscard]] RegionalObjectClassAttributeSubscriptionScopePlan
  setObjectClassAttributeRegionalSubscriptionWithScopeChanges(
      std::wstring const& federationName,
      std::uint64_t federateId,
      std::uint64_t objectClassHandle,
      std::map<std::uint64_t, std::set<std::uint64_t>> const& attributesAndRegions,
      bool active);
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

  // Validates a no-time Update Attribute Values request and forms one passel
  // for every submitted FOM transportation type and explicit association
  // region set. Timestamped traffic, relaxed DDM, attribute-transport
  // changes, and update-rate reduction remain outside this boundary.
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
  struct Federation {
    struct PendingTimeAdvanceGrant {
      std::uint64_t generation = 0;
      FederationTimeGrantDispatch dispatch;
      bool dispatchQueued = false;
    };

    struct FederateInteractionDeclarations {
      std::set<std::uint64_t> publishedInteractionClasses;
      std::map<std::uint64_t, bool> subscribedInteractionClasses;
      std::map<std::uint64_t, std::map<std::uint64_t, bool>>
          regionalSubscribedInteractionClasses;
      std::map<std::uint64_t, std::set<std::uint64_t>>
          publishedObjectClassDirectedInteractions;
      std::map<std::uint64_t, std::set<std::uint64_t>>
          subscribedObjectClassDirectedInteractions;
    };

    struct ObjectClassAttributeDeclarations {
      struct PerObjectClass {
        std::set<std::uint64_t> explicitlyPublishedAttributes;
        std::map<std::uint64_t, bool> subscribedAttributes;
        std::map<std::uint64_t, std::map<std::uint64_t, bool>>
            regionalSubscribedAttributes;
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

      std::uint64_t handle = 0;
      std::wstring name;
      std::uint64_t registeredObjectClassHandle = 0;
      std::uint64_t producingFederateId = 0;
      // Registration seeds every currently published attribute with the
      // producing federate as owner. Keeping the owner per attribute avoids a
      // second state model as the initial 2025 ownership slices are added.
      std::map<std::uint64_t, std::uint64_t> attributeOwnersByHandle;
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
      std::map<std::uint64_t, PendingNegotiatedAttributeOwnershipDivestiture>
          pendingNegotiatedAttributeOwnershipDivestitures;
      std::map<std::uint64_t, PendingConfirmDivestitureNotification>
          pendingConfirmDivestitureNotifications;
      std::map<std::uint64_t, std::uint64_t> knownObjectClassHandlesByFederate;
      std::set<std::uint64_t> pendingDiscoveryFederates;
      bool deleteAccepted = false;
      std::set<std::uint64_t> pendingRemovalFederates;
      std::optional<std::uint64_t> pendingTimestampedDeletionMessageId;
      std::set<std::uint64_t> pendingTimestampedRemovalFederates;
    };

    FederationDefinition definition;
    std::shared_ptr<ObjectClassHandleDirectory const> objectClassHandles;
    std::shared_ptr<AttributeHandleDirectory const> attributeHandles;
    std::shared_ptr<InteractionClassHandleDirectory const> interactionClassHandles;
    std::shared_ptr<ParameterHandleDirectory const> parameterHandles;
    std::shared_ptr<DimensionHandleDirectory const> dimensionHandles;
    std::map<std::uint64_t, FederateMembership> members;
    std::map<std::wstring, std::uint64_t> memberIdsByName;
    std::map<std::uint64_t, FederateInteractionDeclarations> interactionDeclarations;
    std::map<std::uint64_t, ObjectClassAttributeDeclarations> objectClassAttributeDeclarations;
    std::map<std::uint64_t, InteractionCallbackRoute> interactionCallbackRoutes;
    FederationTimeCoordinator timeCoordinator;
    std::map<std::uint64_t, TsoInteractionMessage> tsoInteractionMessages;
    std::map<std::uint64_t, TsoAttributeUpdateMessage>
        tsoAttributeUpdateMessages;
    std::map<std::uint64_t, TsoObjectDeletionMessage>
        tsoObjectDeletionMessages;
    std::map<std::uint64_t, PendingTimeAdvanceGrant> pendingTimeAdvanceGrants;
    std::map<std::uint64_t, Region> regions;
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
      std::uint64_t producingFederateId,
      std::uint64_t receivingFederateId,
      std::uint64_t sentInteractionClassHandle,
      std::vector<std::uint64_t> const& sentParameterHandles,
      std::set<std::uint64_t> const* sentRegionHandles);
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
  [[nodiscard]] static bool regionsOverlap(
      Federation const& federation,
      std::uint64_t firstRegionHandle,
      std::uint64_t secondRegionHandle,
      std::map<std::uint64_t, RegionSpecificationSnapshot> const* overrides);
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
  [[nodiscard]] static std::optional<KnownObjectInstanceSnapshot> knownObjectInstanceSnapshot(
      Federation const& federation,
      Federation::ObjectInstance const& objectInstance,
      std::uint64_t federateId);
  [[nodiscard]] static bool canPurgeDeletedObjectInstance(
      Federation::ObjectInstance const& objectInstance) noexcept;

  [[nodiscard]] static std::vector<FederationTimeGrantDispatch> scheduleEligibleTimeAdvanceGrants(
      Federation& federation);

  mutable std::mutex mutex_;
  std::map<std::wstring, Federation> federations_;
  std::uint64_t nextFederateId_ = 1;
};

}  // namespace umbra::detail
