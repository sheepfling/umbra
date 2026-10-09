#pragma once

#include "internal/federation/federation_registry.hpp"

#include <RTI/RTI1516.h>

#include <cstdint>
#include <map>
#include <memory>
#include <mutex>
#include <optional>
#include <set>
#include <string>
#include <string_view>
#include <utility>
#include <vector>

namespace rti1516_2025 {
class Exception;
}

namespace umbra::detail {
class FederateLifecycle;
class FederateTimeState;
struct FederateTimeSnapshot;
struct ProcessFederationLogicalTime;
struct ProcessFederationLogicalTimeInterval;
}

namespace rti1516_2025::umbra_binding_detail {

struct JoinedServiceReportEndpoint;

std::wstring describeAmbassadorException(Exception const& exception);

#if defined(UMBRA_ENABLE_EMBEDDED_FEDERATION_MANAGEMENT)
using AmbassadorAttributeValue = std::pair<std::uint64_t, VariableLengthData>;
using AmbassadorInteractionParameterValue = std::pair<std::uint64_t, VariableLengthData>;
bool ambassadorMomExceptionIsParameterError(Exception const& exception) noexcept;

AttributeHandleValueMap ambassadorProjectAttributeValues(
    std::vector<AmbassadorAttributeValue> const& sentAttributes,
    std::set<std::uint64_t> const& receivedAttributeHandles);
std::optional<std::string> ambassadorUpdateRateAdmissionKey(
    std::wstring const& federationName,
    std::uint64_t receivingFederateId,
    std::uint64_t objectInstanceHandle,
    std::uint64_t subscriptionGeneration,
    std::uint64_t attributeHandle);
bool ambassadorAdmitUpdateRate(
    std::string const& key,
    double maximumRate,
    bool reliable);
void recordAmbassadorSuccessfulReflectionReceipt(
    std::wstring const& federationName,
    std::uint64_t receivingFederateId,
    std::uint64_t objectInstanceHandle,
    std::string transportationName);

std::mutex& ambassadorFederationManagementMutex();
void requireConnectedForFederationManagement(
    umbra::detail::FederateLifecycle const& lifecycle);
void requireAmbassadorValidResignAction(ResignAction resignAction);
umbra::detail::EmbeddedFederationRegistry& embeddedFederationRegistry();
void eraseAmbassadorUpdateRateHistoryForFederate(
    std::wstring const& federationName,
    std::uint64_t federateId);
void appendAmbassadorSelectedServiceReportRecord(
    JoinedServiceReportEndpoint& endpoint,
    std::wstring const& federationName,
    std::uint64_t federateId,
    std::uint16_t serviceGroup,
    umbra::detail::FederateServiceReportRecordEncoder const& encodeRecord);
void queueAmbassadorMomServiceReportInteraction(
    std::wstring federationName,
    std::uint64_t reportedFederateId,
    std::uint16_t serviceGroup,
    umbra::detail::ReservedMomServiceReport reservation,
    std::vector<AmbassadorInteractionParameterValue> sentParameters,
    TransportationTypeHandle transportationType,
    bool allowRemovedReportedFederate = false);
bool emitAmbassadorSelectedMomServiceReportInteractionForFederate(
    std::wstring const& federationName,
    std::uint64_t reportedFederateId,
    std::wstring const& service,
    umbra::detail::MomServiceType serviceType,
    std::vector<umbra::detail::MomServiceArgument> const& suppliedArguments,
    umbra::detail::MomServiceArgument const& returnedArgument,
    bool success,
    std::wstring const& exception);
void submitAmbassadorFederationSynchronizedNotifications(
    std::vector<umbra::detail::FederationSynchronizedNotification> notifications);
void submitAmbassadorSynchronizationPointAnnouncements(
    std::vector<umbra::detail::SynchronizationPointAnnouncement> announcements);
struct AmbassadorAttributeRegionPairValues {
  std::map<std::uint64_t, std::set<std::uint64_t>> values;
  bool invalidAttributeHandle = false;
  bool invalidRegionHandle = false;
};
AmbassadorAttributeRegionPairValues ambassadorAttributeRegionPairValues(
    AttributeHandleSetRegionHandleSetPairVector const& attributesAndRegions);
std::optional<std::set<std::uint64_t>> ambassadorAttributeHandleValues(
    AttributeHandleSet const& attributes);
std::optional<std::set<std::uint64_t>> ambassadorInteractionClassHandleValues(
    InteractionClassHandleSet const& interactionClasses);
[[noreturn]] void throwObjectClassAttributeDeclarationFailureForFederationManagement(
    umbra::detail::ObjectClassAttributeDeclarationStatus status);
[[noreturn]] void throwObjectInstanceNameReservationFailureForFederationManagement(
    umbra::detail::ObjectInstanceNameReservationStatus status,
    std::wstring const& operation);
[[noreturn]] void throwObjectInstanceRegistrationFailureForFederationManagement(
    umbra::detail::ObjectInstanceRegistrationStatus status);
[[noreturn]] void throwObjectInstanceRegionAssociationFailureForFederationManagement(
    umbra::detail::ObjectInstanceRegionAssociationStatus status);
void queueAmbassadorObjectInstanceNameReservation(
    umbra::detail::ObjectInstanceCallbackRoute callbackRoute,
    std::wstring federationName,
    std::uint64_t federateId,
    bool succeeded,
    std::wstring objectInstanceName);
void queueAmbassadorMultipleObjectInstanceNameReservation(
    umbra::detail::ObjectInstanceCallbackRoute callbackRoute,
    std::wstring federationName,
    std::uint64_t federateId,
    bool succeeded,
    std::set<std::wstring> objectInstanceNames);
void queueAmbassadorDeclarationAdvisories(
    std::vector<umbra::detail::DeclarationAdvisory> advisories);
void queueAmbassadorObjectInstanceDiscoveries(
    std::vector<umbra::detail::ObjectInstanceDiscoveryRecipient> discoveries,
    std::wstring const& federationName);
void queueAmbassadorObjectInstanceRemovals(
    std::vector<umbra::detail::ObjectInstanceRemovalRecipient> removals,
    std::wstring const& federationName,
    VariableLengthData const& userSuppliedTag);
void queueAmbassadorReceiveOrderAttributeUpdate(
    umbra::detail::ObjectInstanceCallbackRoute callbackRoute,
    std::wstring federationName,
    std::uint64_t producingFederateId,
    std::uint64_t receivingFederateId,
    std::uint64_t objectInstanceHandle,
    std::vector<std::uint64_t> sentAttributeHandles,
    std::vector<AmbassadorAttributeValue> sentAttributes,
    VariableLengthData userSuppliedTag,
    TransportationTypeHandle transportationType,
    std::string transportationName,
    bool reliableTransportation = false,
    std::optional<std::set<std::uint64_t>> sentRegionHandles = std::nullopt,
    bool defaultRegionUsed = false,
    std::optional<std::map<std::uint64_t, umbra::detail::RegionSpecificationSnapshot>>
        sentRegionSnapshots = std::nullopt);
ParameterHandleValueMap ambassadorProjectInteractionParameterValues(
    std::vector<AmbassadorInteractionParameterValue> const& sentParameters,
    std::set<std::uint64_t> const& receivedParameterHandles);
std::optional<std::vector<std::uint64_t>> ambassadorInteractionParameterHandleValues(
    ParameterHandleValueMap const& parameterValues);
std::vector<AmbassadorInteractionParameterValue> ambassadorCopyInteractionParameterValues(
    ParameterHandleValueMap const& parameterValues);
void recordAmbassadorSuccessfulInteractionReceipt(
    std::wstring const& federationName,
    std::uint64_t receivingFederateId,
    std::uint64_t receivedInteractionClassHandle,
    std::string transportationName,
    bool directed);
void finishAmbassadorSuppressedTsoRecipientCallback(
    std::wstring const& federationName,
    std::uint64_t receivingFederateId,
    std::optional<std::uint64_t> const& retractionMessageId);
void finishSuppressedTsoRecipientCallbackIfNeeded(
    std::wstring const& federationName,
    std::uint64_t receivingFederateId,
    std::optional<std::uint64_t> const& retractionMessageId);
void queueAmbassadorReceiveOrderInteraction(
    umbra::detail::InteractionCallbackRoute callbackRoute,
    std::wstring federationName,
    umbra::detail::InteractionProducer producingSource,
    std::uint64_t receivingFederateId,
    std::uint64_t sentInteractionClassHandle,
    std::vector<AmbassadorInteractionParameterValue> sentParameters,
    VariableLengthData userSuppliedTag,
    TransportationTypeHandle transportationType,
    std::string transportationName,
    std::optional<std::set<std::uint64_t>> sentRegionHandles = std::nullopt,
    bool defaultRegionUsed = false,
    std::optional<std::map<std::uint64_t, umbra::detail::RegionSpecificationSnapshot>>
        regionOverrides = std::nullopt);
void queueAmbassadorTimestampedReceiveOrderInteraction(
    umbra::detail::InteractionCallbackRoute callbackRoute,
    std::wstring federationName,
    umbra::detail::InteractionProducer producingSource,
    std::uint64_t receivingFederateId,
    std::uint64_t sentInteractionClassHandle,
    std::vector<AmbassadorInteractionParameterValue> sentParameters,
    VariableLengthData userSuppliedTag,
    TransportationTypeHandle transportationType,
    std::string transportationName,
    std::shared_ptr<LogicalTime const> timestamp,
    OrderType sentOrderType,
    OrderType receivedOrderType,
    std::optional<std::uint64_t> retractionMessageId,
    std::optional<std::set<std::uint64_t>> sentRegionHandles = std::nullopt,
    bool defaultRegionUsed = false,
    std::optional<std::map<std::uint64_t, umbra::detail::RegionSpecificationSnapshot>>
        regionOverrides = std::nullopt);
void queueAmbassadorReceiveOrderDirectedInteraction(
    umbra::detail::InteractionCallbackRoute callbackRoute,
    std::wstring federationName,
    std::uint64_t producingFederateId,
    std::uint64_t receivingFederateId,
    std::uint64_t objectInstanceHandle,
    std::uint64_t sentInteractionClassHandle,
    std::vector<AmbassadorInteractionParameterValue> sentParameters,
    VariableLengthData userSuppliedTag,
    TransportationTypeHandle transportationType,
    std::string transportationName);
void queueAmbassadorTimestampedReceiveOrderDirectedInteraction(
    umbra::detail::InteractionCallbackRoute callbackRoute,
    std::wstring federationName,
    std::uint64_t producingFederateId,
    std::uint64_t receivingFederateId,
    std::uint64_t objectInstanceHandle,
    std::uint64_t sentInteractionClassHandle,
    std::vector<AmbassadorInteractionParameterValue> sentParameters,
    VariableLengthData userSuppliedTag,
    TransportationTypeHandle transportationType,
    std::string transportationName,
    std::shared_ptr<LogicalTime const> timestamp,
    OrderType sentOrderType,
    OrderType receivedOrderType,
    std::optional<std::uint64_t> retractionMessageId);
void queueAmbassadorTimestampedReflectAttributeUpdate(
    umbra::detail::ObjectInstanceCallbackRoute callbackRoute,
    std::wstring federationName,
    std::uint64_t producingFederateId,
    std::uint64_t receivingFederateId,
    std::uint64_t objectInstanceHandle,
    std::vector<AmbassadorAttributeValue> sentAttributes,
    std::vector<umbra::detail::TsoAttributeUpdatePassel> passels,
    VariableLengthData userSuppliedTag,
    std::shared_ptr<LogicalTime const> timestamp,
    OrderType sentOrderType,
    OrderType receivedOrderType,
    std::optional<std::uint64_t> retractionMessageId);
void queueAmbassadorTimestampedObjectInstanceRemoval(
    umbra::detail::ObjectInstanceCallbackRoute callbackRoute,
    umbra::detail::FederateServiceReportRoute serviceReportRoute,
    std::wstring federationName,
    std::uint64_t receivingFederateId,
    std::uint64_t objectInstanceHandle,
    std::uint64_t messageId,
    VariableLengthData userSuppliedTag,
    std::shared_ptr<LogicalTime const> timestamp,
    bool provideRetraction,
    OrderType sentOrderType,
    OrderType receivedOrderType);
void queueAmbassadorObjectInstanceScopeChanges(
    std::vector<umbra::detail::ObjectInstanceScopeChangeRecipient> changes,
    std::wstring const& federationName);
void queueAmbassadorAttributeRelevanceAdvisories(
    std::vector<umbra::detail::AttributeRelevanceAdvisoryRecipient> advisories,
    std::wstring const& federationName);
std::wstring makeAmbassadorDiscoverObjectInstanceServiceReportRecord(
    std::uint64_t objectInstanceHandle,
    std::uint64_t objectClassHandle,
    std::wstring const& objectInstanceName,
    std::uint64_t producingFederateId,
    std::uint32_t serialNumber);
std::wstring makeAmbassadorRemoveObjectInstanceServiceReportRecord(
    std::uint64_t objectInstanceHandle,
    VariableLengthData const& userSuppliedTag,
    std::uint64_t producingFederateId,
    OrderType sentOrderType,
    LogicalTime const* optionalTimestamp,
    std::optional<OrderType> optionalReceivedOrderType,
    std::optional<std::uint64_t> optionalRetractionHandle,
    std::uint32_t serialNumber);
std::wstring makeAmbassadorProvideAttributeValueUpdateServiceReportRecord(
    std::uint64_t objectInstanceHandle,
    std::set<std::uint64_t> const& requestedAttributeHandles,
    VariableLengthData const& userSuppliedTag,
    std::uint32_t serialNumber);
void queueAmbassadorAttributeOwnershipAssumptionRecipients(
    std::vector<umbra::detail::AttributeOwnershipAssumptionRecipient> recipients,
    std::wstring const& federationName,
    VariableLengthData const& userSuppliedTag);
void queueAmbassadorAttributeOwnershipAcquisitionWorkItems(
    std::vector<umbra::detail::AttributeOwnershipAcquisitionWorkItem> workItems,
    std::wstring const& federationName);
void queueAmbassadorAttributeOwnershipAcquisitionCancellationConfirmation(
    umbra::detail::ObjectInstanceCallbackRoute callbackRoute,
    std::wstring federationName,
    std::uint64_t requestingFederateId,
    std::uint64_t objectInstanceHandle,
    std::uint64_t cancellationId,
    std::set<std::uint64_t> attributeHandles);
void queueAmbassadorAttributeOwnershipDivestitureIfWantedNotifications(
    std::vector<umbra::detail::AttributeOwnershipDivestitureIfWantedNotification>
        notifications,
    std::wstring const& federationName);
void queueAmbassadorConfirmDivestitureNotifications(
    std::vector<umbra::detail::ConfirmDivestitureNotification> notifications,
    std::wstring const& federationName);
void queueAmbassadorConfirmAttributeTransportationTypeChange(
    umbra::detail::ObjectInstanceCallbackRoute callbackRoute,
    std::wstring federationName,
    std::uint64_t requestingFederateId,
    std::uint64_t requestId);
void queueAmbassadorReportAttributeTransportationType(
    umbra::detail::ObjectInstanceCallbackRoute callbackRoute,
    std::wstring federationName,
    std::uint64_t requestingFederateId,
    std::uint64_t objectInstanceHandle,
    std::uint64_t attributeHandle);
void queueAmbassadorAttributeValueUpdateProvide(
    umbra::detail::ObjectInstanceCallbackRoute callbackRoute,
    umbra::detail::FederateServiceReportRoute serviceReportRoute,
    std::wstring federationName,
    std::uint64_t requestingFederateId,
    std::uint64_t providingFederateId,
    std::uint64_t objectInstanceHandle,
    std::set<std::uint64_t> requestedAttributeHandles,
    VariableLengthData userSuppliedTag,
    std::uint64_t pendingRequestId);
void queueAmbassadorAttributeValueUpdateClassProvide(
    umbra::detail::ObjectInstanceCallbackRoute callbackRoute,
    umbra::detail::FederateServiceReportRoute serviceReportRoute,
    std::wstring federationName,
    std::uint64_t requestingFederateId,
    std::uint64_t providingFederateId,
    std::uint64_t objectInstanceHandle,
    std::uint64_t requestedObjectClassHandle,
    std::set<std::uint64_t> requestedAttributeHandles,
    VariableLengthData userSuppliedTag,
    std::optional<std::map<std::uint64_t, std::set<std::uint64_t>>>
        requestRegionsByAttribute = std::nullopt,
    std::uint64_t pendingRequestId = 0U);
void queueAmbassadorJoinedFederateMomAttributeValueUpdate(
    umbra::detail::ObjectInstanceCallbackRoute callbackRoute,
    std::wstring federationName,
    std::uint64_t receivingFederateId,
    std::uint64_t objectInstanceHandle,
    std::set<std::uint64_t> requestedAttributeHandles,
    bool requireActiveSubscription = false,
    std::optional<std::map<std::uint64_t, VariableLengthData>>
        plannedAttributeValues = std::nullopt,
    std::optional<std::map<std::uint64_t, std::set<std::uint64_t>>>
        requestRegionsByAttribute = std::nullopt);
void queueAmbassadorAttributeOwnershipQueryReport(
    umbra::detail::ObjectInstanceCallbackRoute callbackRoute,
    std::wstring federationName,
    std::uint64_t requestId,
    std::uint64_t requestingFederateId,
    std::uint64_t objectInstanceHandle,
    umbra::detail::AttributeOwnershipQueryReportKind reportKind,
    std::uint64_t owningFederateId,
    std::set<std::uint64_t> requestedAttributeHandles);
struct AmbassadorJoinedFederateMomConditionalWork {
  std::wstring federationName;
  std::uint64_t objectInstanceHandle = 0U;
  std::set<std::uint64_t> attributeHandles;
};
std::optional<AmbassadorJoinedFederateMomConditionalWork>
ambassadorJoinedFederateMomConditionalWorkFor(
    umbra::detail::EmbeddedFederationRegistry& registry,
    std::wstring const& federationName,
    std::uint64_t joinedFederateId,
    std::vector<std::string_view> attributeNames);
std::optional<AmbassadorJoinedFederateMomConditionalWork>
ambassadorFederationMomConditionalWorkFor(
    umbra::detail::EmbeddedFederationRegistry& registry,
    std::wstring const& federationName,
    std::vector<std::string_view> attributeNames);
void queueAmbassadorJoinedFederateMomConditionalAttributeUpdate(
    std::wstring const& federationName,
    std::uint64_t objectInstanceHandle,
    std::set<std::uint64_t> requestedAttributeHandles,
    std::optional<std::uint64_t> excludedReceivingFederateId = std::nullopt);
void queueAmbassadorJoinedFederateMomConditionalAttributeUpdateForJoinedFederate(
    std::wstring const& federationName,
    std::uint64_t joinedFederateId,
    std::vector<std::string_view> attributeNames,
    std::optional<std::uint64_t> excludedReceivingFederateId = std::nullopt);
void pumpAmbassadorDueJoinedFederateMomPeriodicUpdates(
    std::wstring const& federationName);
void queueAmbassadorFederationMomConditionalAttributeUpdate(
    std::wstring const& federationName,
    std::vector<std::string_view> attributeNames,
    std::optional<std::uint64_t> excludedReceivingFederateId = std::nullopt);
void submitAmbassadorReceiveOrderCallback(
    umbra::detail::FederateCallbackRoute callbackRoute,
    std::wstring federationName,
    std::uint64_t receivingFederateId,
    umbra::detail::FederateCallbackInvocation invocation);
void queueMomFederationSynchronizationPointsReport(
    std::wstring federationName,
    umbra::detail::MomFederationSynchronizationPointsReportPlan report);
void queueMomFederationSynchronizationPointStatusReport(
    std::wstring federationName,
    umbra::detail::MomFederationSynchronizationPointStatusReportPlan report);
void queueMomObjectInstancesUpdatedReport(
    std::wstring federationName,
    std::uint64_t reportedFederateId,
    umbra::detail::MomObjectInstanceCountsReportPlan report,
    VariableLengthData encodedCounts);
void queueMomObjectInstancesThatCanBeDeletedReport(
    std::wstring federationName,
    std::uint64_t reportedFederateId,
    umbra::detail::MomObjectInstanceCountsReportPlan report,
    VariableLengthData encodedCounts);
void queueMomObjectInstancesReflectedReport(
    std::wstring federationName,
    std::uint64_t reportedFederateId,
    umbra::detail::MomObjectInstanceCountsReportPlan report,
    VariableLengthData encodedCounts);
void queueMomObjectInstanceInformationReport(
    std::wstring federationName,
    std::uint64_t reportedFederateId,
    umbra::detail::MomObjectInstanceInformationReportPlan report);
void queueMomReflectionsReceivedReport(
    std::wstring federationName,
    std::uint64_t reportedFederateId,
    umbra::detail::MomReflectionsReceivedReportPlan report);
void queueMomUpdatesSentReport(
    std::wstring federationName,
    std::uint64_t reportedFederateId,
    umbra::detail::MomUpdatesSentReportPlan report);
void queueMomInteractionsSentReport(
    std::wstring federationName,
    std::uint64_t reportedFederateId,
    umbra::detail::MomInteractionsSentReportPlan report);
void queueMomDirectedInteractionsSentReport(
    std::wstring federationName,
    std::uint64_t reportedFederateId,
    umbra::detail::MomDirectedInteractionsSentReportPlan report);
void queueMomInteractionsReceivedReport(
    std::wstring federationName,
    std::uint64_t reportedFederateId,
    umbra::detail::MomInteractionsReceivedReportPlan report);
void queueMomDirectedInteractionsReceivedReport(
    std::wstring federationName,
    std::uint64_t reportedFederateId,
    umbra::detail::MomDirectedInteractionsReceivedReportPlan report);
void queueMomObjectClassPublicationReports(
    std::wstring federationName,
    umbra::detail::MomPublicationsReportPlan const& report);
void queueMomInteractionPublicationReport(
    std::wstring federationName,
    umbra::detail::MomPublicationsReportPlan const& report);
void queueMomDirectedInteractionPublicationReports(
    std::wstring federationName,
    umbra::detail::MomPublicationsReportPlan const& report);
void queueMomObjectClassSubscriptionReports(
    std::wstring federationName,
    umbra::detail::MomSubscriptionsReportPlan const& report);
void queueMomInteractionSubscriptionReport(
    std::wstring federationName,
    umbra::detail::MomSubscriptionsReportPlan const& report);
void queueMomDirectedInteractionSubscriptionReports(
    std::wstring federationName,
    umbra::detail::MomSubscriptionsReportPlan const& report);
void queueMomFomModuleDataReport(
    std::wstring federationName,
    umbra::detail::MomFomModuleDataReportPlan report);
void queueMomFederationFomModuleDataReport(
    std::wstring federationName,
    umbra::detail::MomFederationFomModuleDataReportPlan report);
void queueMomFederationMimDataReport(
    std::wstring federationName,
    umbra::detail::MomFederationMimDataReportPlan report);
VariableLengthData ambassadorEncodeMomObjectClassBasedCounts(
    std::map<std::uint64_t, std::uint64_t> const& objectClassCounts);
VariableLengthData ambassadorEncodeMomInteractionCounts(
    std::map<std::uint64_t, std::uint64_t> const& interactionClassCounts);
VariableLengthData ambassadorEncodeMomAttributeHandleList(
    std::set<std::uint64_t> const& attributeHandles);
TransportationTypeHandle ambassadorTransportationHandleFromEmbeddedName(
    std::string const& transportationName,
    wchar_t const* context);
TransportationTypeHandle ambassadorTransportationHandleFromFederationName(
    std::wstring const& federationName,
    std::string const& transportationName,
    wchar_t const* context);
std::optional<std::uint64_t> ambassadorTransportationTypeValueForFederation(
    std::wstring const& federationName,
    std::wstring_view transportationTypeName);
std::optional<bool> ambassadorTransportationTypeReliableForFederation(
    std::wstring const& federationName,
    std::string const& transportationTypeName);
void submitAmbassadorFederationSaveNotifications(
    std::wstring const& federationName,
    std::vector<umbra::detail::FederationSaveNotification> notifications);
void submitAmbassadorFederationRestoreNotifications(
    std::wstring const& federationName,
    std::vector<umbra::detail::FederationRestoreNotification> notifications);
void validateAmbassadorTsoTimestamp(
    umbra::detail::FederateTimeSnapshot const& timeSnapshot,
    LogicalTime const& timestamp);
VariableLengthData makeAmbassadorVariableLengthData(
    std::vector<unsigned char> const& bytes);
void submitAmbassadorTimeAdvanceGrantDispatches(
    std::vector<umbra::detail::FederationTimeGrantDispatch> dispatches);
std::shared_ptr<LogicalTime const> makeAmbassadorTsoRetractionLowerBound(
    umbra::detail::FederateTimeSnapshot const& timeSnapshot);
void queueAmbassadorRequestRetraction(
    umbra::detail::FederateCallbackRoute callbackRoute,
    std::wstring federationName,
    std::uint64_t receivingFederateId,
    std::uint64_t messageId);
void flushAmbassadorAsynchronousReceiveCallbacks(
    std::shared_ptr<umbra::detail::FederateTimeState> const& timeState);
std::shared_ptr<LogicalTime> cloneAmbassadorReferenceLogicalTime(
    std::wstring const& implementationName,
    LogicalTime const& time);
void copyAmbassadorQueriedLogicalTime(LogicalTime &target, LogicalTime const &source);
void copyAmbassadorQueriedLogicalTimeInterval(
    LogicalTimeInterval &target,
    LogicalTimeInterval const &source);
std::unique_ptr<LogicalTime> decodeAmbassadorProcessLogicalTimeValue(
    umbra::detail::ProcessFederationLogicalTime const &value);
std::unique_ptr<LogicalTimeInterval> decodeAmbassadorProcessLogicalTimeIntervalValue(
    umbra::detail::ProcessFederationLogicalTimeInterval const &value);
void retireAmbassadorTsoMessagePayloadsAtRetractionBoundary(
    std::wstring const& federationName,
    std::uint64_t producingFederateId,
    umbra::detail::FederateTimeState const& timeState);
std::shared_ptr<LogicalTimeInterval> cloneAmbassadorReferenceLogicalTimeInterval(
    std::wstring const& implementationName,
    LogicalTimeInterval const& interval);
[[noreturn]] void throwAmbassadorInteractionTransportationTypeChangeFailure(
    umbra::detail::InteractionTransportationTypeChangeStatus status);
std::optional<std::string> ambassadorTransportationNameFromFederation(
    std::wstring const& federationName,
    TransportationTypeHandle const& transportationType);
void queueAmbassadorConfirmInteractionTransportationTypeChange(
    umbra::detail::InteractionCallbackRoute callbackRoute,
    std::wstring federationName,
    std::uint64_t requestingFederateId,
    std::uint64_t interactionClassHandle);
[[noreturn]] void throwAmbassadorInteractionTransportationTypeQueryFailure(
    umbra::detail::InteractionTransportationTypeQueryStatus status);
void queueAmbassadorReportInteractionTransportationType(
    umbra::detail::InteractionCallbackRoute callbackRoute,
    std::wstring federationName,
    std::uint64_t requestingFederateId,
    std::uint64_t queriedFederateId,
    std::uint64_t interactionClassHandle);
#endif

}  // namespace rti1516_2025::umbra_binding_detail
