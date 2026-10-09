#pragma once

#include "internal/federation/federation_registry_service_types.hpp"

namespace umbra::detail {

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
  // The seven leaf parameter handles followed by inherited HLAfederate are
  // resolved from the composed MIM and carried with the private routing plan
  // so the adapter can construct the complete HLAreportServiceInvocation
  // payload without duplicating catalog lookups.
  std::vector<std::uint64_t> reportParameterHandles;
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

// §11.5.1's HLAreportException is an RTI-originated receive-order MOM
// interaction selected by the reported member's Exception Reporting Switch.
// Keep its source and private HLAfederate endpoint separate from the public
// callback producer handle; the adapter chooses the standard default-invalid
// FederateHandle representation at the callback boundary.
enum class ExceptionReportStatus {
  suppressed,
  applied,
  federation_does_not_exist,
  reported_federate_not_member,
  inconsistent_catalog,
};

struct ExceptionReportRouting {
  std::uint64_t interactionClassHandle = 0;
  std::uint64_t federateParameterHandle = 0;
  std::uint64_t serviceParameterHandle = 0;
  std::uint64_t exceptionParameterHandle = 0;
  std::uint64_t endpointRegionHandle = 0;
  RegionSpecificationSnapshot endpointRegion;
};

struct ExceptionReportPlan {
  ExceptionReportStatus status = ExceptionReportStatus::suppressed;
  std::uint64_t reportedFederateId = 0;
  ExceptionReportRouting routing;
  std::vector<ReceiveOrderInteractionRecipient> recipients;
};

// §11.5.1's HLAreportMOMexception is an RTI-originated receive-order MOM
// interaction for malformed or otherwise rejected MOM interactions.  Unlike
// HLAreportException, this route is not gated by the reported member's
// Exception Reporting Switch; it is selected by subscription to the report
// interaction itself.  The adapter still keeps the private HLAfederate
// endpoint separate from the callback-visible producer handle.
enum class MomExceptionReportStatus {
  applied,
  federation_does_not_exist,
  reported_federate_not_member,
  inconsistent_catalog,
};

struct MomExceptionReportRouting {
  std::uint64_t interactionClassHandle = 0;
  std::uint64_t federateParameterHandle = 0;
  std::uint64_t serviceParameterHandle = 0;
  std::uint64_t exceptionParameterHandle = 0;
  std::uint64_t parameterErrorParameterHandle = 0;
  std::uint64_t endpointRegionHandle = 0;
  RegionSpecificationSnapshot endpointRegion;
};

struct MomExceptionReportPlan {
  MomExceptionReportStatus status = MomExceptionReportStatus::applied;
  std::uint64_t reportedFederateId = 0;
  MomExceptionReportRouting routing;
  std::vector<ReceiveOrderInteractionRecipient> recipients;
};

// Bounded public MOM request/report route for
// HLArequestObjectInstancesUpdated. The request is consumed by the RTI and
// the response is an RTI-originated receive-order report interaction scoped
// to the selected joined federate's HLAfederate dimension point.
enum class MomObjectInstanceCountsReportStatus {
  applied,
  federation_does_not_exist,
  requesting_federate_not_member,
  reported_federate_not_member,
  inconsistent_catalog,
};

struct MomObjectInstanceCountsReportRouting {
  std::uint64_t interactionClassHandle = 0;
  std::uint64_t objectInstanceCountsParameterHandle = 0;
  std::uint64_t endpointRegionHandle = 0;
  RegionSpecificationSnapshot endpointRegion;
};

struct MomObjectInstanceCountsReportPlan {
  MomObjectInstanceCountsReportStatus status =
      MomObjectInstanceCountsReportStatus::applied;
  std::uint64_t reportedFederateId = 0;
  // Positive counts only, as required by HLAobjectClassBasedCounts.
  std::map<std::uint64_t, std::uint64_t> objectClassCounts;
  MomObjectInstanceCountsReportRouting routing;
  std::vector<ReceiveOrderInteractionRecipient> recipients;
};

enum class MomObjectInstanceInformationReportStatus {
  applied,
  federation_does_not_exist,
  requesting_federate_not_member,
  inconsistent_catalog,
};

// Public MOM request/report route for HLArequestObjectInstanceInformation.
// The report is snapshotted for the requesting joined federate: a known
// object carries the registered/known classes and attributes currently owned
// by that federate, while an unknown object carries the MIM-defined NULL
// response (object handle plus an empty owned-attribute list only).
struct MomObjectInstanceInformationReportRouting {
  std::uint64_t interactionClassHandle = 0;
  std::uint64_t objectInstanceParameterHandle = 0;
  std::uint64_t ownedInstanceAttributeListParameterHandle = 0;
  std::uint64_t registeredClassParameterHandle = 0;
  std::uint64_t knownClassParameterHandle = 0;
  std::uint64_t endpointRegionHandle = 0;
  RegionSpecificationSnapshot endpointRegion;
};

struct MomObjectInstanceInformationReportPlan {
  MomObjectInstanceInformationReportStatus status =
      MomObjectInstanceInformationReportStatus::applied;
  std::uint64_t reportedFederateId = 0;
  std::uint64_t objectInstanceHandle = 0;
  bool objectKnown = false;
  std::set<std::uint64_t> ownedAttributeHandles;
  std::uint64_t registeredObjectClassHandle = 0;
  std::uint64_t knownObjectClassHandle = 0;
  MomObjectInstanceInformationReportRouting routing;
  std::vector<ReceiveOrderInteractionRecipient> recipients;
};

// Public MOM request/report route for the federate-scoped
// HLArequestFOMmoduleData interaction.  The module content is snapshotted at
// Join from the validated source, so the report does not depend on the source
// path remaining readable after the federation member was admitted.
enum class MomFomModuleDataReportStatus {
  applied,
  federation_does_not_exist,
  requesting_federate_not_member,
  reported_federate_not_member,
  invalid_module_index,
  inconsistent_catalog,
};

struct MomFomModuleDataReportRouting {
  std::uint64_t interactionClassHandle = 0;
  std::uint64_t moduleIndicatorParameterHandle = 0;
  std::uint64_t moduleDataParameterHandle = 0;
  std::uint64_t endpointRegionHandle = 0;
  RegionSpecificationSnapshot endpointRegion;
};

struct MomFomModuleDataReportPlan {
  MomFomModuleDataReportStatus status =
      MomFomModuleDataReportStatus::applied;
  std::uint64_t reportedFederateId = 0;
  std::uint32_t moduleIndex = 0;
  std::wstring moduleData;
  MomFomModuleDataReportRouting routing;
  std::vector<ReceiveOrderInteractionRecipient> recipients;
};

// Public MOM request/report route for the federation-scoped
// HLArequestFOMmoduleData interaction. Unlike the federate-scoped family,
// this report has no HLAfederate dimension or target parameter: every current
// subscriber to the RTI-originated report is eligible for the federation
// snapshot.
enum class MomFederationFomModuleDataReportStatus {
  applied,
  federation_does_not_exist,
  requesting_federate_not_member,
  invalid_module_index,
  inconsistent_catalog,
};

struct MomFederationFomModuleDataReportRouting {
  std::uint64_t interactionClassHandle = 0;
  std::uint64_t moduleIndicatorParameterHandle = 0;
  std::uint64_t moduleDataParameterHandle = 0;
};

struct MomFederationFomModuleDataReportPlan {
  MomFederationFomModuleDataReportStatus status =
      MomFederationFomModuleDataReportStatus::applied;
  std::uint32_t moduleIndex = 0;
  std::wstring moduleData;
  MomFederationFomModuleDataReportRouting routing;
  std::vector<ReceiveOrderInteractionRecipient> recipients;
};

// Public MOM request/report route for the federation-scoped
// HLArequestMIMdata interaction. The MIM content is retained by the
// validated federation definition and is returned as one RTI-originated
// reliable report to current subscribers.
enum class MomFederationMimDataReportStatus {
  applied,
  federation_does_not_exist,
  requesting_federate_not_member,
  inconsistent_catalog,
};

struct MomFederationMimDataReportRouting {
  std::uint64_t interactionClassHandle = 0;
  std::uint64_t moduleDataParameterHandle = 0;
};

struct MomFederationMimDataReportPlan {
  MomFederationMimDataReportStatus status =
      MomFederationMimDataReportStatus::applied;
  std::wstring moduleData;
  MomFederationMimDataReportRouting routing;
  std::vector<ReceiveOrderInteractionRecipient> recipients;
};

// Public MOM request/report routes for the federation synchronization-point
// queries.  Both report interactions are dimensionless in the 2025 MIM, so
// the RTI response is broadcast to every current subscriber.  The request
// snapshot is retained in the plan while the callback-time recipient method
// rechecks the report subscription immediately before delivery.
struct MomFederationSynchronizationPointStatusEntry {
  std::uint64_t federateId = 0;
  std::int32_t status = 0;
};

enum class MomFederationSynchronizationPointsReportStatus {
  applied,
  federation_does_not_exist,
  requesting_federate_not_member,
  inconsistent_catalog,
};

struct MomFederationSynchronizationPointsReportRouting {
  std::uint64_t interactionClassHandle = 0;
  std::uint64_t synchronizationPointsParameterHandle = 0;
};

struct MomFederationSynchronizationPointsReportPlan {
  MomFederationSynchronizationPointsReportStatus status =
      MomFederationSynchronizationPointsReportStatus::applied;
  std::vector<std::wstring> synchronizationPointLabels;
  MomFederationSynchronizationPointsReportRouting routing;
  std::vector<ReceiveOrderInteractionRecipient> recipients;
};

enum class MomFederationSynchronizationPointStatusReportStatus {
  applied,
  federation_does_not_exist,
  requesting_federate_not_member,
  inconsistent_catalog,
};

struct MomFederationSynchronizationPointStatusReportRouting {
  std::uint64_t interactionClassHandle = 0;
  std::uint64_t synchronizationPointNameParameterHandle = 0;
  std::uint64_t synchronizationPointFederatesParameterHandle = 0;
};

struct MomFederationSynchronizationPointStatusReportPlan {
  MomFederationSynchronizationPointStatusReportStatus status =
      MomFederationSynchronizationPointStatusReportStatus::applied;
  std::wstring synchronizationPointName;
  std::vector<MomFederationSynchronizationPointStatusEntry> federateStatuses;
  MomFederationSynchronizationPointStatusReportRouting routing;
  std::vector<ReceiveOrderInteractionRecipient> recipients;
};

// Bounded public MOM request/report route for HLArequestPublications.  The
// request is inherited from HLAfederate and is answered with the three MIM
// report classes: one object-class publication report per published class,
// one interaction publication report, and one directed-interaction report per
// object class.  Empty ledgers retain the MIM-defined NULL response shape.
enum class MomPublicationsReportStatus {
  applied,
  federation_does_not_exist,
  requesting_federate_not_member,
  inconsistent_catalog,
};

struct MomObjectClassPublicationReportRouting {
  std::uint64_t interactionClassHandle = 0;
  std::uint64_t numberOfClassesParameterHandle = 0;
  std::uint64_t objectClassParameterHandle = 0;
  std::uint64_t attributeListParameterHandle = 0;
  std::uint64_t endpointRegionHandle = 0;
  RegionSpecificationSnapshot endpointRegion;
};

struct MomInteractionPublicationReportRouting {
  std::uint64_t interactionClassHandle = 0;
  std::uint64_t interactionClassListParameterHandle = 0;
  std::uint64_t endpointRegionHandle = 0;
  RegionSpecificationSnapshot endpointRegion;
};

struct MomDirectedInteractionPublicationReportRouting {
  std::uint64_t interactionClassHandle = 0;
  std::uint64_t numberOfClassesParameterHandle = 0;
  std::uint64_t objectClassParameterHandle = 0;
  std::uint64_t interactionClassListParameterHandle = 0;
  std::uint64_t endpointRegionHandle = 0;
  RegionSpecificationSnapshot endpointRegion;
};

struct MomPublicationsReportPlan {
  MomPublicationsReportStatus status = MomPublicationsReportStatus::applied;
  std::uint64_t reportedFederateId = 0;
  std::map<std::uint64_t, std::set<std::uint64_t>> objectClassAttributesByClass;
  std::set<std::uint64_t> interactionClassHandles;
  std::map<std::uint64_t, std::set<std::uint64_t>>
      directedInteractionClassesByObjectClass;
  MomObjectClassPublicationReportRouting objectClassRouting;
  MomInteractionPublicationReportRouting interactionRouting;
  MomDirectedInteractionPublicationReportRouting directedRouting;
  std::vector<ReceiveOrderInteractionRecipient> objectClassRecipients;
  std::vector<ReceiveOrderInteractionRecipient> interactionRecipients;
  std::vector<ReceiveOrderInteractionRecipient> directedRecipients;
};

// Bounded public MOM request/report route for HLArequestSubscriptions.  The
// request-time snapshot keeps ordinary and regional declarations separate
// long enough to report active/passive combinations, update-rate designators,
// interaction subscription pairs, and directed object-class subscriptions.
// The callback-time recipient methods repeat the private HLAfederate endpoint
// and subscription projection checks before user code is invoked.
enum class MomSubscriptionsReportStatus {
  applied,
  federation_does_not_exist,
  requesting_federate_not_member,
  inconsistent_catalog,
};

struct MomObjectClassSubscriptionSnapshot {
  std::uint64_t objectClassHandle = 0;
  bool active = false;
  std::string maxUpdateRate;
  std::set<std::uint64_t> attributeHandles;
};

struct MomObjectClassSubscriptionReportRouting {
  std::uint64_t interactionClassHandle = 0;
  std::uint64_t numberOfClassesParameterHandle = 0;
  std::uint64_t objectClassParameterHandle = 0;
  std::uint64_t activeParameterHandle = 0;
  std::uint64_t maxUpdateRateParameterHandle = 0;
  std::uint64_t attributeListParameterHandle = 0;
  std::uint64_t endpointRegionHandle = 0;
  RegionSpecificationSnapshot endpointRegion;
};

struct MomInteractionSubscriptionReportRouting {
  std::uint64_t interactionClassHandle = 0;
  std::uint64_t interactionClassListParameterHandle = 0;
  std::uint64_t endpointRegionHandle = 0;
  RegionSpecificationSnapshot endpointRegion;
};

struct MomDirectedInteractionSubscriptionReportRouting {
  std::uint64_t interactionClassHandle = 0;
  std::uint64_t numberOfClassesParameterHandle = 0;
  std::uint64_t objectClassParameterHandle = 0;
  // The official 2025 MIM XML does not declare HLAuniversal on this report,
  // but retain an optional handle for a compatible catalog that does.
  std::uint64_t universalParameterHandle = 0;
  std::uint64_t interactionClassListParameterHandle = 0;
  std::uint64_t endpointRegionHandle = 0;
  RegionSpecificationSnapshot endpointRegion;
};

struct MomSubscriptionsReportPlan {
  MomSubscriptionsReportStatus status = MomSubscriptionsReportStatus::applied;
  std::uint64_t reportedFederateId = 0;
  std::map<std::uint64_t, std::map<bool, MomObjectClassSubscriptionSnapshot>>
      objectClassSubscriptions;
  std::map<std::uint64_t, std::set<bool>> interactionSubscriptionActives;
  std::map<std::uint64_t, std::map<bool, std::set<std::uint64_t>>>
      directedInteractionClassesByObjectClassAndUniversal;
  MomObjectClassSubscriptionReportRouting objectClassRouting;
  MomInteractionSubscriptionReportRouting interactionRouting;
  MomDirectedInteractionSubscriptionReportRouting directedRouting;
  std::vector<ReceiveOrderInteractionRecipient> objectClassRecipients;
  std::vector<ReceiveOrderInteractionRecipient> interactionRecipients;
  std::vector<ReceiveOrderInteractionRecipient> directedRecipients;
};

// Bounded public MOM request/report route for HLArequestUpdatesSent. One
// response is emitted for each supported transportation type, including an
// empty NULL response bucket, with class-grouped HLAobjectClassBasedCounts.
struct MomUpdatesSentReportRouting {
  std::uint64_t interactionClassHandle = 0;
  std::uint64_t transportationParameterHandle = 0;
  std::uint64_t updateCountsParameterHandle = 0;
  std::uint64_t endpointRegionHandle = 0;
  RegionSpecificationSnapshot endpointRegion;
};

struct MomUpdatesSentReportPlan {
  MomObjectInstanceCountsReportStatus status =
      MomObjectInstanceCountsReportStatus::applied;
  std::uint64_t reportedFederateId = 0;
  std::map<std::string, std::map<std::uint64_t, std::uint64_t>>
      objectClassCountsByTransportation;
  MomUpdatesSentReportRouting routing;
  std::vector<ReceiveOrderInteractionRecipient> recipients;
};

// Bounded public MOM request/report route for HLArequestInteractionsSent.
// One response is emitted for each supported transportation type, including
// an empty NULL response bucket, with positive HLAinteractionCounts grouped by
// the sent interaction class.
struct MomInteractionsSentReportRouting {
  std::uint64_t interactionClassHandle = 0;
  std::uint64_t transportationParameterHandle = 0;
  std::uint64_t interactionCountsParameterHandle = 0;
  std::uint64_t endpointRegionHandle = 0;
  RegionSpecificationSnapshot endpointRegion;
};

struct MomInteractionsSentReportPlan {
  MomObjectInstanceCountsReportStatus status =
      MomObjectInstanceCountsReportStatus::applied;
  std::uint64_t reportedFederateId = 0;
  std::map<std::string, std::map<std::uint64_t, std::uint64_t>>
      interactionClassCountsByTransportation;
  MomInteractionsSentReportRouting routing;
  std::vector<ReceiveOrderInteractionRecipient> recipients;
};

// Bounded public MOM request/report route for
// HLArequestDirectedInteractionsSent. The payload shape is the same official
// HLAinteractionCounts encoding as HLAreportInteractionsSent, but the source
// ledger contains only accepted directed sends and still emits empty buckets.
struct MomDirectedInteractionsSentReportPlan {
  MomObjectInstanceCountsReportStatus status =
      MomObjectInstanceCountsReportStatus::applied;
  std::uint64_t reportedFederateId = 0;
  std::map<std::string, std::map<std::uint64_t, std::uint64_t>>
      interactionClassCountsByTransportation;
  MomInteractionsSentReportRouting routing;
  std::vector<ReceiveOrderInteractionRecipient> recipients;
};

// Bounded public MOM request/report route for HLArequestInteractionsReceived.
// The payload is the official HLAinteractionCounts array grouped by sent
// interaction class and effective transportation at the receiving callback
// boundary.
struct MomInteractionsReceivedReportRouting {
  std::uint64_t interactionClassHandle = 0;
  std::uint64_t transportationParameterHandle = 0;
  std::uint64_t interactionCountsParameterHandle = 0;
  std::uint64_t endpointRegionHandle = 0;
  RegionSpecificationSnapshot endpointRegion;
};

struct MomInteractionsReceivedReportPlan {
  MomObjectInstanceCountsReportStatus status =
      MomObjectInstanceCountsReportStatus::applied;
  std::uint64_t reportedFederateId = 0;
  std::map<std::string, std::map<std::uint64_t, std::uint64_t>>
      interactionClassCountsByTransportation;
  MomInteractionsReceivedReportRouting routing;
  std::vector<ReceiveOrderInteractionRecipient> recipients;
};

// Public MOM request/report route for HLArequestDirectedInteractionsReceived.
// The payload uses the official HLAinteractionCounts array, but the source
// ledger is restricted to accepted directed-interaction receive callbacks.
struct MomDirectedInteractionsReceivedReportPlan {
  MomObjectInstanceCountsReportStatus status =
      MomObjectInstanceCountsReportStatus::applied;
  std::uint64_t reportedFederateId = 0;
  std::map<std::string, std::map<std::uint64_t, std::uint64_t>>
      interactionClassCountsByTransportation;
  MomInteractionsReceivedReportRouting routing;
  std::vector<ReceiveOrderInteractionRecipient> recipients;
};

// Public MOM request/report route for HLArequestReflectionsReceived. The
// payload is the official HLAobjectClassBasedCounts array grouped by the
// effective transportation type at the application reflection callback
// boundary. Both standard transportation buckets are retained so an empty
// bucket can be emitted as the MIM-defined NULL response.
struct MomReflectionsReceivedReportRouting {
  std::uint64_t interactionClassHandle = 0;
  std::uint64_t transportationParameterHandle = 0;
  std::uint64_t reflectionCountsParameterHandle = 0;
  std::uint64_t endpointRegionHandle = 0;
  RegionSpecificationSnapshot endpointRegion;
};

struct MomReflectionsReceivedReportPlan {
  MomObjectInstanceCountsReportStatus status =
      MomObjectInstanceCountsReportStatus::applied;
  std::uint64_t reportedFederateId = 0;
  std::map<std::string, std::map<std::uint64_t, std::uint64_t>>
      objectClassCountsByTransportation;
  MomReflectionsReceivedReportRouting routing;
  std::vector<ReceiveOrderInteractionRecipient> recipients;
};

}  // namespace umbra::detail
