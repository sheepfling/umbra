#pragma once

#include "tck_harness.hpp"
#include "tck_callback_controls_scenarios.hpp"
#include "tck_delay_subscription_scenarios.hpp"
#include "tck_federation_lifecycle_scenarios.hpp"
#include "tck_timestamped_interaction_scheduling_scenarios.hpp"

namespace hla_rti_cpp_tck::detail {

using Clock = std::chrono::steady_clock;

struct DdmHandles {
  rti::ObjectClassHandle objectClass;
  rti::AttributeHandle attribute;
  rti::InteractionClassHandle interactionClass;
  rti::ParameterHandle parameter;
  rti::DimensionHandle firstDimension;
  rti::DimensionHandle secondDimension;
};

struct PortableServiceReportArgument final {
  rti::Integer32 type;
  std::wstring name;
  std::wstring value;
};

std::vector<std::pair<std::string, rti::CallbackModel>>
callbackModels(Options const& options);
rti::RegionHandle createDdmRegion(
    Session& session, DdmHandles const& handles,
    unsigned long firstLower, unsigned long firstUpper,
    unsigned long secondLower, unsigned long secondUpper);
void verifyDdmRegion(
    Session& session, DdmHandles const& handles,
    rti::RegionHandle const& region,
    unsigned long firstLower, unsigned long firstUpper,
    unsigned long secondLower, unsigned long secondUpper,
    std::string const& description);
DdmHandles ddmHandles(Session& session, Options const& options);
void enableTimestampedRoles(
    Session& regulator, Session& constrained,
    TimeContext const& regulatorTime, TimeContext const& constrainedTime,
    Options const& options, std::string const& description);
std::vector<std::wstring> fomModuleNames(
    std::vector<std::filesystem::path> const& paths);
void handles(
    Session& session, Options const& options,
    rti::ObjectClassHandle& objectClass, rti::AttributeHandle& attribute,
    rti::InteractionClassHandle& interactionClass,
    rti::ParameterHandle& parameter);
Options parseOptions(int argc, char** argv);
void requirePortableServiceReport(
    Session& observer, InteractionRecord const& report,
    rti::InteractionClassHandle reportClass,
    std::vector<rti::ParameterHandle> const& reportParameters,
    std::wstring const& service,
    std::vector<PortableServiceReportArgument> const& expectedSupplied,
    PortableServiceReportArgument const& expectedReturned,
    rti::Integer32 expectedSerial, std::string const& description,
    rti::Integer16 expectedServiceType = 6);
void requirePortableServiceReportFailure(
    Session& observer, InteractionRecord const& report,
    rti::InteractionClassHandle reportClass,
    std::vector<rti::ParameterHandle> const& reportParameters,
    std::wstring const& service,
    std::vector<PortableServiceReportArgument> const& expectedSupplied,
    std::wstring const& expectedException, rti::Integer32 expectedSerial,
    std::string const& description);
void verifyDdmClassDimensions(
    Session& session, Options const& options, DdmHandles const& handles,
    std::string const& description);
void waitForConnectionLossAndSignal(
    Session& lost, Options const& options, std::string const& description);
template <typename Predicate>
void waitForSessions(
    std::vector<Session*> const& sessions, Predicate predicate,
    Options const& options, std::string const& description) {
  auto const deadline = std::chrono::steady_clock::now() +
      std::chrono::milliseconds(options.timeoutMilliseconds);
  while (std::chrono::steady_clock::now() < deadline) {
    for (auto* session : sessions) session->pump();
    if (predicate()) return;
    std::this_thread::sleep_for(std::chrono::milliseconds(1));
  }
  throw std::runtime_error("Timed out waiting for " + description);
}
void writeJUnit(
    std::filesystem::path const& path,
    std::vector<ScenarioResult> const& results);
void writeResults(
    std::filesystem::path const& path, Options const& options,
    std::vector<ScenarioResult> const& results);
void scenarioJoinedFederateMomTsoLengthPeriodic(Options const&, rti::CallbackModel);
void scenarioJoinedFederateMomRemovedObjectCount(Options const&, rti::CallbackModel);
void scenarioJoinedFederateMomGaltLitsPeriodic(Options const&, rti::CallbackModel);
void scenarioJoinedFederateMomTsoLengthPeriodicContract(Options const&, rti::CallbackModel);
void scenarioRegionalAttributeUpdateCallbackDdmRecheck(Options const&, rti::CallbackModel);
void scenarioRegionalAttributeUpdateCallbackDdmRecheckContract(Options const&, rti::CallbackModel);
void scenarioRegionalAttributeValueRequestFiltering(Options const&, rti::CallbackModel);
void scenarioRegionalAttributeValueRequestFilteringContract(Options const&, rti::CallbackModel);
void scenarioRegionalAttributeValueUpdateResponseRecheck(Options const&, rti::CallbackModel);
void scenarioRegionalAttributeValueUpdateResponseRecheckContract(Options const&, rti::CallbackModel);

void scenarioAllowRelaxedDdm(Options const&, rti::CallbackModel);
void scenarioAlternateTimeAdvances(Options const&, rti::CallbackModel);
void scenarioAlternateTimeAdvancesContract(Options const&, rti::CallbackModel);
void scenarioAttributeUpdate(Options const&, rti::CallbackModel);
void scenarioAttributeValueRequest(Options const&, rti::CallbackModel);
void scenarioAvailableTimeAdvancesInclusiveGalt(Options const&, rti::CallbackModel);
void scenarioAvailableTimeAdvancesInclusiveGaltContract(Options const&, rti::CallbackModel);
void scenarioCallbackReentrancy(Options const&, rti::CallbackModel);
void scenarioCallbackReentrancyContract(Options const&, rti::CallbackModel);
void scenarioConnection(Options const&, rti::CallbackModel);
void scenarioConnectionCallbackContract(Options const&, rti::CallbackModel);
void scenarioFederationMomContentReports(Options const&, rti::CallbackModel);
void scenarioFederationMomContentReportsContract(Options const&, rti::CallbackModel);
void scenarioFederationMomCurrentFdd(Options const&, rti::CallbackModel);
void scenarioFederationMomCurrentFddContract(Options const&, rti::CallbackModel);
void scenarioFederationMomFomModuleDesignatorList(Options const&, rti::CallbackModel);
void scenarioFederationMomFomModuleDesignatorListContract(Options const&, rti::CallbackModel);
void scenarioFederationSaveRestoreInterlocks(Options const&, rti::CallbackModel);
void scenarioHandleLookups(Options const&, rti::CallbackModel);
void scenarioHandleWireFormats(Options const&, rti::CallbackModel);
void scenarioInteraction(Options const&, rti::CallbackModel);
void scenarioLogicalTimeDataElementsContract(Options const&, rti::CallbackModel);
void scenarioMalformedInputsContract(Options const&, rti::CallbackModel);
void scenarioMomTransportationTypeChangeRequest(Options const&, rti::CallbackModel);
void scenarioNextMessageRequest(Options const&, rti::CallbackModel);
void scenarioNextMessageRequestContract(Options const&, rti::CallbackModel);
void scenarioNmrNoPregrantTsoFloor(Options const&, rti::CallbackModel);
void scenarioNmrNoPregrantTsoFloorContract(Options const&, rti::CallbackModel);
void scenarioNmrReceivedTsoFloor(Options const&, rti::CallbackModel);
void scenarioNmrReceivedTsoFloorContract(Options const&, rti::CallbackModel);
void scenarioNmrReceivedTsoAttributeUpdateFloor(Options const&, rti::CallbackModel);
void scenarioNmrReceivedTsoAttributeUpdateFloorContract(Options const&, rti::CallbackModel);
void scenarioNmrReceivedTsoDirectedInteractionFloor(Options const&, rti::CallbackModel);
void scenarioNmrReceivedTsoDirectedInteractionFloorContract(Options const&, rti::CallbackModel);
void scenarioNmrEmptyMessageSetGrantTime(Options const&, rti::CallbackModel);
void scenarioNmrEmptyMessageSetGrantTimeContract(Options const&, rti::CallbackModel);
void scenarioNmrRequestBelowGaltImmediateGrant(Options const&, rti::CallbackModel);
void scenarioNmrRequestBelowGaltImmediateGrantContract(Options const&, rti::CallbackModel);
void scenarioNmraNoPregrantTsoFloor(Options const&, rti::CallbackModel);
void scenarioNmraNoPregrantTsoFloorContract(Options const&, rti::CallbackModel);
void scenarioSendMessageTimeStatePreservationGranted(Options const&, rti::CallbackModel);
void scenarioSendMessageTimeStatePreservationGrantedContract(Options const&, rti::CallbackModel);
void scenarioTimestampedInteractionPreferredReceiveConversion(Options const&, rti::CallbackModel);
void scenarioTimestampedInteractionPreferredReceiveConversionContract(Options const&, rti::CallbackModel);
void scenarioTimestampedInteractionSenderRegulationTransition(Options const&, rti::CallbackModel);
void scenarioTimestampedInteractionSenderRegulationTransitionContract(Options const&, rti::CallbackModel);
void scenarioInteractionFddPreferredOrderDefault(Options const&, rti::CallbackModel);
void scenarioInteractionFddPreferredOrderDefaultContract(Options const&, rti::CallbackModel);
void scenarioSentTsoPerRecipientOrderConversion(Options const&, rti::CallbackModel);
void scenarioSentTsoPerRecipientOrderConversionContract(Options const&, rti::CallbackModel);
void scenarioObjectDeletion(Options const&, rti::CallbackModel);
void scenarioObjectRegistration(Options const&, rti::CallbackModel);
void scenarioOwnership(Options const&, rti::CallbackModel);
void scenarioOwnershipManagementContract(Options const&, rti::CallbackModel);
void scenarioPrivilegeToDeleteTransfer(Options const&, rti::CallbackModel);
void scenarioPrivilegeToDeleteTransferContract(Options const&, rti::CallbackModel);
void scenarioOwnershipTransferRegionalUpdate(Options const&, rti::CallbackModel);
void scenarioRegionalBoundaries(Options const&, rti::CallbackModel);
void scenarioRegionalMultiAttributeUpdate(Options const&, rti::CallbackModel);
void scenarioRegionalThreeDimensionalOverlap(Options const&, rti::CallbackModel);
void scenarioServiceReportAttributeUpdate(Options const&, rti::CallbackModel);
void scenarioServiceReportAttributeUpdateContract(Options const&, rti::CallbackModel);
void scenarioServiceReportAttributeUpdateFailure(Options const&, rti::CallbackModel);
void scenarioServiceReportAttributeUpdateFailureContract(Options const&, rti::CallbackModel);
void scenarioServiceReportDeleteObjectInstance(Options const&, rti::CallbackModel);
void scenarioServiceReportDeleteObjectInstanceContract(Options const&, rti::CallbackModel);
void scenarioServiceReportDeleteObjectInstanceFailure(Options const&, rti::CallbackModel);
void scenarioServiceReportDeleteObjectInstanceFailureContract(Options const&, rti::CallbackModel);
void scenarioServiceReportDimensionHandle(Options const&, rti::CallbackModel);
void scenarioServiceReportDimensionHandleContract(Options const&, rti::CallbackModel);
void scenarioServiceReportDimensionName(Options const&, rti::CallbackModel);
void scenarioServiceReportDimensionNameContract(Options const&, rti::CallbackModel);
void scenarioServiceReportDimensionUpperBound(Options const&, rti::CallbackModel);
void scenarioServiceReportDimensionUpperBoundContract(Options const&, rti::CallbackModel);
void scenarioServiceReportDirectedInteractionDeclaration(Options const&, rti::CallbackModel);
void scenarioServiceReportDirectedInteractionDeclarationContract(Options const&, rti::CallbackModel);
void scenarioServiceReportDirectedInteractionPublication(Options const&, rti::CallbackModel);
void scenarioServiceReportDirectedInteractionPublicationContract(Options const&, rti::CallbackModel);
void scenarioServiceReportFederateObjectClassLookupFailures(Options const&, rti::CallbackModel);
void scenarioServiceReportFederateObjectClassLookupFailuresContract(Options const&, rti::CallbackModel);
void scenarioServiceReportFederateObjectClassLookups(Options const&, rti::CallbackModel);
void scenarioServiceReportFederateObjectClassLookupsContract(Options const&, rti::CallbackModel);
void scenarioServiceReportInteraction(Options const&, rti::CallbackModel);
void scenarioServiceReportInteractionContract(Options const&, rti::CallbackModel);
void scenarioServiceReportInteractionDimensions(Options const&, rti::CallbackModel);
void scenarioServiceReportInteractionDimensionsContract(Options const&, rti::CallbackModel);
void scenarioServiceReportInteractionFailure(Options const&, rti::CallbackModel);
void scenarioServiceReportInteractionFailureContract(Options const&, rti::CallbackModel);
void scenarioServiceReportInteractionParameterLookupFailures(Options const&, rti::CallbackModel);
void scenarioServiceReportInteractionParameterLookupFailuresContract(Options const&, rti::CallbackModel);
void scenarioServiceReportInteractionParameterLookups(Options const&, rti::CallbackModel);
void scenarioServiceReportInteractionParameterLookupsContract(Options const&, rti::CallbackModel);
void scenarioServiceReportInteractionPublication(Options const&, rti::CallbackModel);
void scenarioServiceReportInteractionPublicationContract(Options const&, rti::CallbackModel);
void scenarioServiceReportInteractionSubscription(Options const&, rti::CallbackModel);
void scenarioServiceReportQueryGalt(Options const&, rti::CallbackModel);
void scenarioServiceReportNextMessageRequestAvailableBoundary(Options const&, rti::CallbackModel);
void scenarioServiceReportInteractionSubscriptionContract(Options const&, rti::CallbackModel);
void scenarioServiceReportInterlock(Options const&, rti::CallbackModel);
void scenarioServiceReportInterlockContract(Options const&, rti::CallbackModel);
void scenarioServiceReportLocalDeleteObjectInstance(Options const&, rti::CallbackModel);
void scenarioServiceReportLocalDeleteObjectInstanceContract(Options const&, rti::CallbackModel);
void scenarioServiceReportLocalDeleteObjectInstanceFailure(Options const&, rti::CallbackModel);
void scenarioServiceReportLocalDeleteObjectInstanceFailureContract(Options const&, rti::CallbackModel);
void scenarioServiceReportObjectAttributeDeclaration(Options const&, rti::CallbackModel);
void scenarioServiceReportObjectAttributeDeclarationContract(Options const&, rti::CallbackModel);
void scenarioServiceReportObjectAttributeUpdateRateLookupFailures(Options const&, rti::CallbackModel);
void scenarioServiceReportObjectAttributeUpdateRateLookupFailuresContract(Options const&, rti::CallbackModel);
void scenarioServiceReportObjectAttributeUpdateRateLookups(Options const&, rti::CallbackModel);
void scenarioServiceReportObjectAttributeUpdateRateLookupsContract(Options const&, rti::CallbackModel);
void scenarioServiceReportObjectDimensions(Options const&, rti::CallbackModel);
void scenarioServiceReportObjectDimensionsContract(Options const&, rti::CallbackModel);
void scenarioServiceReportOrderTransportationLookupFailures(Options const&, rti::CallbackModel);
void scenarioServiceReportOrderTransportationLookupFailuresContract(Options const&, rti::CallbackModel);
void scenarioServiceReportOrderTransportationLookups(Options const&, rti::CallbackModel);
void scenarioServiceReportOrderTransportationLookupsContract(Options const&, rti::CallbackModel);
void scenarioServiceReportReceiveOrderInteraction(Options const&, rti::CallbackModel);
void scenarioServiceReportReceiveOrderInteractionContract(Options const&, rti::CallbackModel);
void scenarioServiceReportRegisterObjectInstance(Options const&, rti::CallbackModel);
void scenarioServiceReportRegisterObjectInstanceContract(Options const&, rti::CallbackModel);
void scenarioServiceReportReleaseMultipleObjectInstanceNames(Options const&, rti::CallbackModel);
void scenarioServiceReportReleaseMultipleObjectInstanceNamesContract(Options const&, rti::CallbackModel);
void scenarioServiceReportReleaseObjectInstanceName(Options const&, rti::CallbackModel);
void scenarioServiceReportReleaseObjectInstanceNameContract(Options const&, rti::CallbackModel);
void scenarioServiceReportRequestAttributeValueUpdate(Options const&, rti::CallbackModel);
void scenarioServiceReportRequestAttributeValueUpdateContract(Options const&, rti::CallbackModel);
void scenarioServiceReportReserveObjectInstanceName(Options const&, rti::CallbackModel);
void scenarioServiceReportReserveObjectInstanceNameContract(Options const&, rti::CallbackModel);
void scenarioServiceReportSynchronization(Options const&, rti::CallbackModel);
void scenarioServiceReportSynchronizationContract(Options const&, rti::CallbackModel);
void scenarioServiceReportFederateSaveBegun(Options const&, rti::CallbackModel);
void scenarioServiceReportFederateSaveBegunContract(Options const&, rti::CallbackModel);
void scenarioServiceReportTimestampedAttributeUpdate(Options const&, rti::CallbackModel);
void scenarioServiceReportTimestampedAttributeUpdateContract(Options const&, rti::CallbackModel);
void scenarioServiceReportTimestampedAttributeUpdateFailure(Options const&, rti::CallbackModel);
void scenarioServiceReportTimestampedAttributeUpdateFailureContract(Options const&, rti::CallbackModel);
void scenarioServiceReportTimestampedDeleteObjectInstance(Options const&, rti::CallbackModel);
void scenarioServiceReportTimestampedDeleteObjectInstanceContract(Options const&, rti::CallbackModel);
void scenarioServiceReportTimestampedDeleteObjectInstanceFailure(Options const&, rti::CallbackModel);
void scenarioServiceReportTimestampedDeleteObjectInstanceFailureContract(Options const&, rti::CallbackModel);
void scenarioServiceReportTimestampedDirectedInteraction(Options const&, rti::CallbackModel);
void scenarioServiceReportTimestampedDirectedInteractionContract(Options const&, rti::CallbackModel);
void scenarioServiceReportTimestampedInteraction(Options const&, rti::CallbackModel);
void scenarioServiceReportTimestampedInteractionContract(Options const&, rti::CallbackModel);
void scenarioServiceReportTimestampedInteractionFailure(Options const&, rti::CallbackModel);
void scenarioServiceReportTimestampedInteractionFailureContract(Options const&, rti::CallbackModel);
void scenarioTimedDefaultRegionAttributeSaveRestore(Options const&, rti::CallbackModel);
void scenarioTimedDefaultRegionInteractionSaveRestore(Options const&, rti::CallbackModel);
void scenarioTimedFederationSaveRestore(Options const&, rti::CallbackModel);
void scenarioFqrTimedFederationSaveCallback(Options const&, rti::CallbackModel);
void scenarioTimedRegionalInteractionSaveRestore(Options const&, rti::CallbackModel);
void scenarioTimestampedAttributeOrderCohort(Options const&, rti::CallbackModel);
void scenarioTimestampedAttributeOrderCohortContract(Options const&, rti::CallbackModel);
void scenarioTimestampedAttributeSourceResignation(Options const&, rti::CallbackModel);
void scenarioTimestampedAttributeSourceResignationContract(Options const&, rti::CallbackModel);
void scenarioTimestampedAttributeSourceResignationFanout(Options const&, rti::CallbackModel);
void scenarioTimestampedAttributeSourceResignationFanoutContract(Options const&, rti::CallbackModel);
void scenarioTimestampedAttributeUpdate(Options const&, rti::CallbackModel);
void scenarioTimestampedAttributeUpdateTarPostGrantCutoff(
    Options const&, rti::CallbackModel);
void scenarioTimestampedAttributeUpdateTarPostGrantCutoffContract(
    Options const&, rti::CallbackModel);
void scenarioTimestampedAttributeUpdateAlternateAdvances(Options const&, rti::CallbackModel);
void scenarioTimestampedAttributeUpdateAlternateAdvancesContract(Options const&, rti::CallbackModel);
void scenarioTimestampedAttributeUpdateContract(Options const&, rti::CallbackModel);
void scenarioTimestampedAttributeUpdateFlushQueueFutureInput(Options const&, rti::CallbackModel);
void scenarioTimestampedAttributeUpdateFlushQueueFutureInputContract(Options const&, rti::CallbackModel);
void scenarioTimestampedAttributeUpdateNoFanout(Options const&, rti::CallbackModel);
void scenarioTimestampedAttributeUpdateNoFanoutContract(Options const&, rti::CallbackModel);
void scenarioTimestampedAttributeUpdateOwnershipTransfer(Options const&, rti::CallbackModel);
void scenarioTimestampedAttributeUpdateOwnershipTransferContract(Options const&, rti::CallbackModel);
void scenarioTimestampedAttributeUpdateQueuedPasselRetraction(Options const&, rti::CallbackModel);
void scenarioMixedOrderAttributePasselRetraction(Options const&, rti::CallbackModel);
void scenarioMixedOrderAttributePasselRetractionContract(Options const&, rti::CallbackModel);
void scenarioTimestampedAttributeSenderRegulationTransition(Options const&, rti::CallbackModel);
void scenarioTimestampedAttributeSenderRegulationTransitionContract(Options const&, rti::CallbackModel);
void scenarioTimestampedAttributeUpdateQueuedPasselRetractionContract(Options const&, rti::CallbackModel);
void scenarioTimestampedAttributeUpdateReenable(Options const&, rti::CallbackModel);
void scenarioTimestampedAttributeUpdateReenableContract(Options const&, rti::CallbackModel);
void scenarioTimestampedAttributeUpdateRegulationReenable(Options const&, rti::CallbackModel);
void scenarioTimestampedAttributeUpdateRegulationReenableContract(Options const&, rti::CallbackModel);
void scenarioTimestampedDirectedAlternateAdvances(Options const&, rti::CallbackModel);
void scenarioTimestampedDirectedAlternateAdvancesContract(Options const&, rti::CallbackModel);
void scenarioTimestampedDirectedInteractionImmediateSourceResignation(Options const&, rti::CallbackModel);
void scenarioTimestampedDirectedInteractionImmediateSourceResignationContract(Options const&, rti::CallbackModel);
void scenarioTimestampedDirectedInteractionReenable(Options const&, rti::CallbackModel);
void scenarioTimestampedDirectedInteractionReenableContract(Options const&, rti::CallbackModel);
void scenarioTimestampedDirectedInteractionRegulationReenable(Options const&, rti::CallbackModel);
void scenarioTimestampedDirectedInteractionRegulationReenableContract(Options const&, rti::CallbackModel);
void scenarioTimestampedDirectedInteractionSourceResignation(Options const&, rti::CallbackModel);
void scenarioTimestampedDirectedInteractionSourceResignationContract(Options const&, rti::CallbackModel);
void scenarioTimestampedDirectedInteractionSourceResignationFanout(Options const&, rti::CallbackModel);
void scenarioTimestampedDirectedInteractionSourceResignationFanoutContract(Options const&, rti::CallbackModel);
void scenarioTimestampedDirectedInteractionTarNmr(Options const&, rti::CallbackModel);
void scenarioTimestampedDirectedInteractionTarNmrContract(Options const&, rti::CallbackModel);
void scenarioTimestampedInteractionCrossProducerOrderContract(Options const&, rti::CallbackModel);
void scenarioTimestampedInteractionFlushQueueFutureInputContract(Options const&, rti::CallbackModel);
void scenarioTimestampedInteractionMixedAdvances(Options const&, rti::CallbackModel);
void scenarioTimestampedInteractionMixedAdvancesContract(Options const&, rti::CallbackModel);
void scenarioTimestampedInteractionNoFanoutContract(Options const&, rti::CallbackModel);
void scenarioTimestampedInteractionReenable(Options const&, rti::CallbackModel);
void scenarioTimestampedInteractionReenableContract(Options const&, rti::CallbackModel);
void scenarioTimestampedInteractionRegulationReenable(Options const&, rti::CallbackModel);
void scenarioTimestampedInteractionRegulationReenableContract(Options const&, rti::CallbackModel);
void scenarioTimestampedInteractionRetractionFanoutContract(Options const&, rti::CallbackModel);
void scenarioTimestampedInteractionSourceResignationContract(Options const&, rti::CallbackModel);
void scenarioTimestampedInteractionSourceResignationFanoutContract(Options const&, rti::CallbackModel);
void scenarioTimestampedInteractionTsoDesignatorTerminalizationContract(Options const&, rti::CallbackModel);
void scenarioTimestampedInteractions(Options const&, rti::CallbackModel);
void scenarioTimestampedInteractionsContract(Options const&, rti::CallbackModel);
void scenarioUnregulatedSenderTimestampedInteraction(Options const&, rti::CallbackModel);
void scenarioUnregulatedSenderTimestampedInteractionContract(Options const&, rti::CallbackModel);
void scenarioTimestampedInteractionPreferredReceiveConversion(Options const&, rti::CallbackModel);
void scenarioTimestampedInteractionPreferredReceiveConversionContract(Options const&, rti::CallbackModel);
void scenarioTimestampedInteractionSenderRegulationTransition(Options const&, rti::CallbackModel);
void scenarioTimestampedInteractionSenderRegulationTransitionContract(Options const&, rti::CallbackModel);
void scenarioSentTsoPerRecipientOrderConversion(Options const&, rti::CallbackModel);
void scenarioSentTsoPerRecipientOrderConversionContract(Options const&, rti::CallbackModel);
void scenarioTimestampedObjectDeletion(Options const&, rti::CallbackModel);
void scenarioTimestampedObjectDeletionContract(Options const&, rti::CallbackModel);
void scenarioTimestampedObjectDeletionJoinedOwnerRetraction(Options const&, rti::CallbackModel);
void scenarioTimestampedObjectDeletionJoinedOwnerRetractionContract(Options const&, rti::CallbackModel);
void scenarioTimestampedObjectDeletionMixedAdvances(Options const&, rti::CallbackModel);
void scenarioTimestampedObjectDeletionMixedAdvancesContract(Options const&, rti::CallbackModel);
void scenarioTimestampedObjectDeletionNoFanout(Options const&, rti::CallbackModel);
void scenarioTimestampedObjectDeletionNoFanoutContract(Options const&, rti::CallbackModel);
void scenarioTimestampedObjectDeletionRegulationReenable(Options const&, rti::CallbackModel);
void scenarioTimestampedObjectDeletionRegulationReenableContract(Options const&, rti::CallbackModel);
void scenarioTimestampedObjectDeletionSourceResignationFanout(Options const&, rti::CallbackModel);
void scenarioTimestampedObjectDeletionSourceResignationFanoutContract(Options const&, rti::CallbackModel);
void scenarioTimestampedObjectDeletionTombstone(Options const&, rti::CallbackModel);
void scenarioTimestampedObjectDeletionTombstoneContract(Options const&, rti::CallbackModel);
void scenarioTimestampedRegionalInteraction(Options const&, rti::CallbackModel);

void scenarioAsynchronousDelivery(Options const&, rti::CallbackModel);
void scenarioAsynchronousDeliveryContract(Options const&, rti::CallbackModel);
void scenarioAttributeInteraction(Options const&, rti::CallbackModel);
void scenarioAttributeInteractionContract(Options const&, rti::CallbackModel);
void scenarioAttributeMultiRecipientFifo(Options const&, rti::CallbackModel);
void scenarioAttributeMultiRecipientFifoContract(Options const&, rti::CallbackModel);
void scenarioAttributeScopeAdvisories(Options const&, rti::CallbackModel);
void scenarioAttributeScopeAdvisoriesContract(Options const&, rti::CallbackModel);
void scenarioAttributeValueUpdateRequestBaseline(Options const&, rti::CallbackModel);
void scenarioAttributeValueUpdateRequestBaselineContract(Options const&, rti::CallbackModel);
void scenarioAttributeValueUpdateRequestMultiRequester(Options const&, rti::CallbackModel);
void scenarioAttributeValueUpdateRequestMultiRequesterContract(Options const&, rti::CallbackModel);
void scenarioAttributeValueUpdateResponse(Options const&, rti::CallbackModel);
void scenarioAttributeValueUpdateResponseContract(Options const&, rti::CallbackModel);
void scenarioAuthorizerFactoryFactoryContract(Options const&, rti::CallbackModel);
void scenarioAutoProvide(Options const&, rti::CallbackModel);
void scenarioAutoProvideContract(Options const&, rti::CallbackModel);
void scenarioBasicDataElementsContract(Options const&, rti::CallbackModel);
void scenarioCompositeDataElementsContract(Options const&, rti::CallbackModel);
void scenarioConfigurationAndAuthorizationContract(Options const&, rti::CallbackModel);
void scenarioConnectionLoss(Options const&, rti::CallbackModel);
void scenarioConnectionLossAutomaticCancelPendingAcquisition(Options const&, rti::CallbackModel);
void scenarioConnectionLossAutomaticCancelPendingAcquisitionContract(Options const&, rti::CallbackModel);
void scenarioConnectionLossAutomaticUnconditionalDivestiture(Options const&, rti::CallbackModel);
void scenarioConnectionLossAutomaticUnconditionalDivestitureContract(Options const&, rti::CallbackModel);
void scenarioConnectionLossAutomaticDeleteThenDivest(Options const&, rti::CallbackModel);
void scenarioConnectionLossAutomaticDeleteThenDivestContract(Options const&, rti::CallbackModel);
void scenarioConnectionLossContract(Options const&, rti::CallbackModel);
void scenarioCustomTransportationRegionalAttributeDelivery(Options const&, rti::CallbackModel);
void scenarioCustomTransportationRegionalAttributeDeliveryContract(Options const&, rti::CallbackModel);
void scenarioCustomTransportationRegionalInteractionDelivery(Options const&, rti::CallbackModel);
void scenarioCustomTransportationRegionalInteractionDeliveryContract(Options const&, rti::CallbackModel);
void scenarioCustomTransportationTimestampedRegionalAttributeDelivery(Options const&, rti::CallbackModel);
void scenarioCustomTransportationTimestampedRegionalAttributeDeliveryContract(Options const&, rti::CallbackModel);
void scenarioDataElementContract(Options const&, rti::CallbackModel);
void scenarioDeclarationManagementContract(Options const&, rti::CallbackModel);
void scenarioDeclarations(Options const&, rti::CallbackModel);
void scenarioInteractionFullUnpublication(Options const&, rti::CallbackModel);
void scenarioDefaultRegionInteractionRouting(Options const&, rti::CallbackModel);
void scenarioDefaultRegionInteractionRoutingContract(Options const&, rti::CallbackModel);
void scenarioDefaultRegionObjectRouting(Options const&, rti::CallbackModel);
void scenarioDefaultRegionObjectRoutingContract(Options const&, rti::CallbackModel);
void scenarioDefaultRegionRegistrationNames(Options const&, rti::CallbackModel);
void scenarioDefaultRegionRegistrationNamesContract(Options const&, rti::CallbackModel);
void scenarioDirectedInteractionContract(Options const&, rti::CallbackModel);
void scenarioDirectedInteractionMixedSubscriptionFanout(Options const&, rti::CallbackModel);
void scenarioDirectedInteractionMixedSubscriptionFanoutContract(Options const&, rti::CallbackModel);
void scenarioDirectedInteractionMultiRecipientFifo(Options const&, rti::CallbackModel);
void scenarioDirectedInteractionMultiRecipientFifoContract(Options const&, rti::CallbackModel);
void scenarioDirectedInteractionPublicationSendFence(Options const&, rti::CallbackModel);
void scenarioDirectedInteractionPublicationSendFenceContract(Options const&, rti::CallbackModel);
void scenarioDirectedInteractionSubscriptionKind(Options const&, rti::CallbackModel);
void scenarioDirectedInteractionSubscriptionKindContract(Options const&, rti::CallbackModel);
void scenarioDirectedInteractionTargetLifecycle(Options const&, rti::CallbackModel);
void scenarioDirectedInteractionTargetLifecycleContract(Options const&, rti::CallbackModel);
void scenarioDirectedInteractions(Options const&, rti::CallbackModel);
void scenarioDivestitureIfWantedMixedAcquirers(Options const&, rti::CallbackModel);
void scenarioDivestitureIfWantedMixedAcquirersContract(Options const&, rti::CallbackModel);
void scenarioEnumContract(Options const&, rti::CallbackModel);
void scenarioExceptionHierarchyContract(Options const&, rti::CallbackModel);
void scenarioFactoryDiscovery(Options const&, rti::CallbackModel);
void scenarioFederationMomSaveConditionals(Options const&, rti::CallbackModel);
void scenarioFederationMomSaveConditionalsContract(Options const&, rti::CallbackModel);
void scenarioFederationMomSaveStatus(Options const&, rti::CallbackModel);
void scenarioFederationMomSaveStatusContract(Options const&, rti::CallbackModel);
void scenarioFederationSaveRestore(Options const&, rti::CallbackModel);
void scenarioFederationSaveRestoreContract(Options const&, rti::CallbackModel);
void scenarioFederationTeardownIsolation(Options const&, rti::CallbackModel);
void scenarioFederationTeardownIsolationContract(Options const&, rti::CallbackModel);
void scenarioFinalFederateResignationCleanup(Options const&, rti::CallbackModel);
void scenarioFinalFederateResignationCleanupContract(Options const&, rti::CallbackModel);
void scenarioFomModelContract(Options const&, rti::CallbackModel);
void scenarioHandleAndCollectionContract(Options const&, rti::CallbackModel);
void scenarioHandleWireFormatsContract(Options const&, rti::CallbackModel);
void scenarioIfAvailableOwnershipRestoreUnavailableCallback(Options const&, rti::CallbackModel);
void scenarioIfAvailableOwnershipRestoreUnavailableCallbackContract(Options const&, rti::CallbackModel);
void scenarioInteractionMultiRecipientFifo(Options const&, rti::CallbackModel);
void scenarioInteractionMultiRecipientFifoContract(Options const&, rti::CallbackModel);
void scenarioInteractionPublicationSendFence(Options const&, rti::CallbackModel);
void scenarioInteractionPublicationSendFenceContract(Options const&, rti::CallbackModel);
void scenarioInteractionSubscriptionLifecycle(Options const&, rti::CallbackModel);
void scenarioInteractionSubscriptionLifecycleContract(Options const&, rti::CallbackModel);
void scenarioJoinedFederateMomDeletedObjectCountPeriodic(Options const&, rti::CallbackModel);
void scenarioJoinedFederateMomDeletedObjectCountPeriodicContract(Options const&, rti::CallbackModel);
void scenarioJoinedFederateMomDirectedInteractionsReceivedCounts(Options const&, rti::CallbackModel);
void scenarioJoinedFederateMomDirectedInteractionsReceivedCountsContract(Options const&, rti::CallbackModel);
void scenarioJoinedFederateMomDirectedInteractionsSentCounts(Options const&, rti::CallbackModel);
void scenarioJoinedFederateMomDirectedInteractionsSentCountsContract(Options const&, rti::CallbackModel);
void scenarioJoinedFederateMomDiscoveredObjectCountPeriodic(Options const&, rti::CallbackModel);
void scenarioJoinedFederateMomDiscoveredObjectCountPeriodicContract(Options const&, rti::CallbackModel);
void scenarioJoinedFederateMomFederateStateSaveRestore(Options const&, rti::CallbackModel);
void scenarioJoinedFederateMomFederateStateSaveRestoreContract(Options const&, rti::CallbackModel);
void scenarioJoinedFederateMomGaltLitsPeriodic(Options const&, rti::CallbackModel);
void scenarioJoinedFederateMomGaltLitsPeriodicContract(Options const&, rti::CallbackModel);
void scenarioJoinedFederateMomInteractionsReceivedCounts(Options const&, rti::CallbackModel);
void scenarioJoinedFederateMomInteractionsReceivedCountsContract(Options const&, rti::CallbackModel);
void scenarioJoinedFederateMomInteractionsSentCounts(Options const&, rti::CallbackModel);
void scenarioJoinedFederateMomInteractionsSentCountsContract(Options const&, rti::CallbackModel);
void scenarioJoinedFederateMomReflectionCounts(Options const&, rti::CallbackModel);
void scenarioJoinedFederateMomReflectionCountsContract(Options const&, rti::CallbackModel);
void scenarioJoinedFederateMomReflectionsReceivedCounts(Options const&, rti::CallbackModel);
void scenarioJoinedFederateMomReflectionsReceivedCountsContract(Options const&, rti::CallbackModel);
void scenarioJoinedFederateMomRemovedObjectCount(Options const&, rti::CallbackModel);
void scenarioJoinedFederateMomRemovedObjectCountContract(Options const&, rti::CallbackModel);
void scenarioJoinedFederateMomTimeStateDurations(Options const&, rti::CallbackModel);
void scenarioJoinedFederateMomTimeStateDurationsContract(Options const&, rti::CallbackModel);
void scenarioJoinedFederateMomUpdatedObjectCountPeriodic(Options const&, rti::CallbackModel);
void scenarioJoinedFederateMomUpdatedObjectCountPeriodicContract(Options const&, rti::CallbackModel);
void scenarioJoinedFederateMomUpdatesSentCounts(Options const&, rti::CallbackModel);
void scenarioJoinedFederateMomUpdatesSentCountsContract(Options const&, rti::CallbackModel);
void scenarioKnownClassAttributeRelevanceContract(Options const&, rti::CallbackModel);
void scenarioKnownClassAttributeRelevanceDisabledContract(Options const&, rti::CallbackModel);
void scenarioLocalDeleteObjectInstance(Options const&, rti::CallbackModel);
void scenarioLocalDeleteObjectInstanceContract(Options const&, rti::CallbackModel);
void scenarioLogicalTimeContract(Options const&, rti::CallbackModel);
void scenarioLogicalTimeFactory(Options const&, rti::CallbackModel);
void scenarioLogicalTimeFactoryFactoryContract(Options const&, rti::CallbackModel);
void scenarioMultiRegionInteractionRouting(Options const&, rti::CallbackModel);
void scenarioMultiRegionInteractionRoutingContract(Options const&, rti::CallbackModel);
void scenarioNamedRegistration(Options const&, rti::CallbackModel);
void scenarioNamedRegistrationContract(Options const&, rti::CallbackModel);
void scenarioNamedRegistrationMultiRecipient(Options const&, rti::CallbackModel);
void scenarioNamedRegistrationMultiRecipientContract(Options const&, rti::CallbackModel);
void scenarioNegotiatedDivestitureCancellation(Options const&, rti::CallbackModel);
void scenarioNegotiatedDivestitureCancellationContract(Options const&, rti::CallbackModel);
void scenarioNegotiatedDivestiturePartialAcquisitionCancellation(Options const&, rti::CallbackModel);
void scenarioNegotiatedDivestiturePartialAcquisitionCancellationContract(Options const&, rti::CallbackModel);
void scenarioNegotiatedDivestiturePreDeliveryCancellation(Options const&, rti::CallbackModel);
void scenarioNegotiatedDivestiturePreDeliveryCancellationContract(Options const&, rti::CallbackModel);
void scenarioNegotiatedWillingToAcquireContinuation(Options const&, rti::CallbackModel);
void scenarioNegotiatedWillingToAcquireContinuationContract(Options const&, rti::CallbackModel);
void scenarioObjectAttributeSubscriptionLifecycle(Options const&, rti::CallbackModel);
void scenarioObjectAttributeSubscriptionLifecycleContract(Options const&, rti::CallbackModel);
void scenarioObjectClassAttributeValueUpdateRequestBaseline(Options const&, rti::CallbackModel);
void scenarioObjectClassAttributeValueUpdateRequestBaselineContract(Options const&, rti::CallbackModel);
void scenarioObjectManagement(Options const&, rti::CallbackModel);
void scenarioObjectManagementContract(Options const&, rti::CallbackModel);
void scenarioObjectNameReservationLifecycle(Options const&, rti::CallbackModel);
void scenarioObjectNameReservationLifecycleContract(Options const&, rti::CallbackModel);
void scenarioReservedObjectNameDoesNotDiscover(Options const&, rti::CallbackModel);
void scenarioObjectPublicationRegistrationFence(Options const&, rti::CallbackModel);
void scenarioObjectPublicationRegistrationFenceContract(Options const&, rti::CallbackModel);
void scenarioObjectRegistrationDiscoveryLifecycle(Options const&, rti::CallbackModel);
void scenarioObjectRegistrationDiscoveryLifecycleContract(Options const&, rti::CallbackModel);
void scenarioOverlappingObjectClassSubscriptionsSingleDiscovery(
    Options const&, rti::CallbackModel);
void scenarioOverlappingObjectClassSubscriptionsSingleDiscoveryContract(
    Options const&, rti::CallbackModel);
void scenarioPerFederateDiscoveredClass(Options const&, rti::CallbackModel);
void scenarioPerFederateDiscoveredClassContract(Options const&, rti::CallbackModel);
void scenarioDiscoveredClassStabilityAfterSpecificSubscription(
    Options const&, rti::CallbackModel);
void scenarioDiscoveredClassStabilityAfterSpecificSubscriptionContract(
    Options const&, rti::CallbackModel);
void scenarioRegisteredObjectClassImmutability(Options const&, rti::CallbackModel);
void scenarioRegisteredObjectClassImmutabilityContract(Options const&, rti::CallbackModel);
void scenarioObjectRegistrationDiscoveryMultiRecipient(Options const&, rti::CallbackModel);
void scenarioObjectRegistrationDiscoveryMultiRecipientContract(Options const&, rti::CallbackModel);
void scenarioOrderTypeControls(Options const&, rti::CallbackModel);
void scenarioOrderTypeControlsContract(Options const&, rti::CallbackModel);
void scenarioOrdinaryEdges(Options const&, rti::CallbackModel);
void scenarioOrdinaryEdgesContract(Options const&, rti::CallbackModel);
void scenarioOwnershipAcquisitionCancellationSaveRestore(Options const&, rti::CallbackModel);
void scenarioOwnershipAcquisitionCancellationSaveRestoreContract(Options const&, rti::CallbackModel);
void scenarioOwnershipAcquisitionCancellationTransferRace(Options const&, rti::CallbackModel);
void scenarioOwnershipAcquisitionCancellationTransferRaceContract(Options const&, rti::CallbackModel);
void scenarioOwnershipAcquisitionPublicationFence(Options const&, rti::CallbackModel);
void scenarioOwnershipAcquisitionPublicationFenceContract(Options const&, rti::CallbackModel);
void scenarioOwnershipQueryPartitionCleanup(Options const&, rti::CallbackModel);
void scenarioOwnershipQueryPartitionCleanupContract(Options const&, rti::CallbackModel);
void scenarioOwnershipQuerySaveRestoreResults(Options const&, rti::CallbackModel);
void scenarioOwnershipQuerySaveRestoreResultsContract(Options const&, rti::CallbackModel);
void scenarioOwnershipSaveRestore(Options const&, rti::CallbackModel);
void scenarioOwnershipSaveRestoreContract(Options const&, rti::CallbackModel);
void scenarioOwnershipTransferDeferredRegionalUpdate(Options const&, rti::CallbackModel);
void scenarioOwnershipTransferDeferredRegionalUpdateContract(Options const&, rti::CallbackModel);
void scenarioPartialAttributeOwnershipTransfer(Options const&, rti::CallbackModel);
void scenarioPartialAttributeOwnershipTransferContract(Options const&, rti::CallbackModel);
void scenarioPassiveRegionalInteractionTransition(Options const&, rti::CallbackModel);
void scenarioPassiveRegionalInteractionTransitionContract(Options const&, rti::CallbackModel);
void scenarioPassiveRegionalSubscription(Options const&, rti::CallbackModel);
void scenarioPassiveRegionalSubscriptionContract(Options const&, rti::CallbackModel);
void scenarioPendingNegotiatedOwnershipSaveRestore(Options const&, rti::CallbackModel);
void scenarioPendingNegotiatedOwnershipSaveRestoreContract(Options const&, rti::CallbackModel);
void scenarioQueryLitsAfterSourceResignation(Options const&, rti::CallbackModel);
void scenarioQueryLitsAfterSourceResignationContract(Options const&, rti::CallbackModel);
void scenarioRTIambassadorFactoryContract(Options const&, rti::CallbackModel);
void scenarioReceiveOrderAttributeUpdateCallbackCancellation(Options const&, rti::CallbackModel);
void scenarioReceiveOrderAttributeUpdateCallbackCancellationContract(Options const&, rti::CallbackModel);
void scenarioReceiveOrderAttributeUpdateReceiverResignationFenceContract(Options const&, rti::CallbackModel);
void scenarioReceiveOrderInteractionCallbackCancellation(Options const&, rti::CallbackModel);
void scenarioReceiveOrderInteractionCallbackCancellationContract(Options const&, rti::CallbackModel);
void scenarioReceiveOrderMultiAttributeUpdateCallbackCancellation(Options const&, rti::CallbackModel);
void scenarioReceiveOrderMultiAttributeUpdateCallbackCancellationContract(Options const&, rti::CallbackModel);
void scenarioRegionLifecycle(Options const&, rti::CallbackModel);
void scenarioRegionLifecycleContract(Options const&, rti::CallbackModel);
void scenarioRegionalDeclarationRelevanceAdvisories(Options const&, rti::CallbackModel);
void scenarioRegionalDeclarationRelevanceAdvisoriesContract(Options const&, rti::CallbackModel);
void scenarioRegionalInteractionEmptySubscriptionSets(Options const&, rti::CallbackModel);
void scenarioRegionalInteractionEmptySubscriptionSetsContract(Options const&, rti::CallbackModel);
void scenarioRegionalInteractionRegionValidation(Options const&, rti::CallbackModel);
void scenarioRegionalInteractionRegionValidationContract(Options const&, rti::CallbackModel);
void scenarioRegionalInteractionRouting(Options const&, rti::CallbackModel);
void scenarioRegionalInteractionRoutingContract(Options const&, rti::CallbackModel);
void scenarioRegionalInteractionSourceRegionSnapshot(Options const&, rti::CallbackModel);
void scenarioRegionalInteractionSourceRegionSnapshotContract(Options const&, rti::CallbackModel);
void scenarioRegionalInteractionSubscriptionFiltering(Options const&, rti::CallbackModel);
void scenarioRegionalInteractionSubscriptionFilteringContract(Options const&, rti::CallbackModel);
void scenarioRegionalObjectUpdate(Options const&, rti::CallbackModel);
void scenarioRegionalObjectUpdateContract(Options const&, rti::CallbackModel);
void scenarioRegionalUnpublishRegionRelease(Options const&, rti::CallbackModel);
void scenarioRegionalUnpublishRegionReleaseContract(Options const&, rti::CallbackModel);
void scenarioRelevanceAdvisoriesContract(Options const&, rti::CallbackModel);
void scenarioResignCancelIfAvailablePending(Options const&, rti::CallbackModel);
void scenarioResignCancelIfAvailablePendingContract(Options const&, rti::CallbackModel);
void scenarioResignCancelNegotiatedPending(Options const&, rti::CallbackModel);
void scenarioResignCancelNegotiatedPendingContract(Options const&, rti::CallbackModel);
void scenarioResignCancelPendingAcquisition(Options const&, rti::CallbackModel);
void scenarioResignCancelPendingAcquisitionContract(Options const&, rti::CallbackModel);
void scenarioResignDeleteObjects(Options const&, rti::CallbackModel);
void scenarioResignDeleteObjectsContract(Options const&, rti::CallbackModel);
void scenarioResignPendingAcquisitionRejection(Options const&, rti::CallbackModel);
void scenarioResignPendingAcquisitionRejectionContract(Options const&, rti::CallbackModel);
void scenarioResignUnconditionalDivestiture(Options const&, rti::CallbackModel);
void scenarioResignUnconditionalDivestitureContract(Options const&, rti::CallbackModel);
void scenarioRuntimeIdentityContract(Options const&, rti::CallbackModel);
void scenarioServiceReportRegionalInteraction(Options const&, rti::CallbackModel);
void scenarioServiceReportRegionalInteractionContract(Options const&, rti::CallbackModel);
void scenarioServiceReportRegionalInteractionFailure(Options const&, rti::CallbackModel);
void scenarioServiceReportRegionalInteractionFailureContract(Options const&, rti::CallbackModel);
void scenarioServiceReportRegionalInteractionSubscription(Options const&, rti::CallbackModel);
void scenarioServiceReportRegionalInteractionSubscriptionContract(Options const&, rti::CallbackModel);
void scenarioFederateApplicationMultipleExecutions(
    Options const&, rti::CallbackModel);
void scenarioFederateApplicationMultipleExecutionsContract(
    Options const&, rti::CallbackModel);
void scenarioFederateSaveInitiationEligibility(
    Options const&, rti::CallbackModel);
void scenarioFederateSaveInitiationTimeConstrainedEligibility(
    Options const&, rti::CallbackModel);
void verifyResignExceptionPrecedence(Options const&, rti::CallbackModel);
void verifyDisconnectedFederationManagementServices(
    Options const&, rti::CallbackModel);
void scenarioStandardOrderAndTransportationLookups(Options const&, rti::CallbackModel);
void scenarioStandardOrderAndTransportationLookupsContract(Options const&, rti::CallbackModel);
void scenarioSupportServicesContract(Options const&, rti::CallbackModel);
void scenarioSynchronizationPointContract(Options const&, rti::CallbackModel);
void scenarioSynchronizationPointExplicitSetLateJoin(Options const&, rti::CallbackModel);
void scenarioSynchronizationPointExplicitSetLateJoinContract(Options const&, rti::CallbackModel);
void scenarioSynchronizationPoints(Options const&, rti::CallbackModel);
void scenarioTimeAdvance(Options const&, rti::CallbackModel);
void scenarioTimeAdvanceContract(Options const&, rti::CallbackModel);
void scenarioTimeBoundsQueries(Options const&, rti::CallbackModel);
void scenarioTimeBoundsQueriesContract(Options const&, rti::CallbackModel);
void scenarioTimeConstrainedEnableAtGaltEquality(Options const&, rti::CallbackModel);
void scenarioTimeConstrainedEnableAtGaltEqualityContract(Options const&, rti::CallbackModel);
void scenarioTimeRegulationImplicitAdvance(Options const&, rti::CallbackModel);
void scenarioTimeRegulationImplicitAdvanceContract(Options const&, rti::CallbackModel);
void scenarioTimedRegionalAttributeNegotiatedRegularCandidateContinuationAfterRestore(Options const&, rti::CallbackModel);
void scenarioTimedRegionalAttributeNegotiatedRegularCandidateContinuationAfterRestoreContract(Options const&, rti::CallbackModel);
void scenarioTimedRegionalAttributeNegotiatedRegularCandidateContinuationImpl(Options const&, rti::CallbackModel, bool cancelAfterConfirmation, bool cancelBeforeConfirmation);
void scenarioTimedRegionalAttributeNegotiatedRegularConfirmationCancelAfterRestore(Options const&, rti::CallbackModel);
void scenarioTimedRegionalAttributeNegotiatedRegularConfirmationCancelAfterRestoreContract(Options const&, rti::CallbackModel);
void scenarioTimedRegionalAttributeNegotiatedRegularPreDeliveryCancelAfterRestore(Options const&, rti::CallbackModel);
void scenarioTimedRegionalAttributeNegotiatedRegularPreDeliveryCancelAfterRestoreContract(Options const&, rti::CallbackModel);
void scenarioTimestampedDefaultRegionAttributeAlternateAdvances(Options const&, rti::CallbackModel);
void scenarioTimestampedDefaultRegionAttributeAlternateAdvancesContract(Options const&, rti::CallbackModel);
void scenarioTimestampedDefaultRegionAttributeMixedFanout(Options const&, rti::CallbackModel);
void scenarioTimestampedDefaultRegionAttributeMixedFanoutContract(Options const&, rti::CallbackModel);
void scenarioTimestampedDefaultRegionAttributeReenable(Options const&, rti::CallbackModel);
void scenarioTimestampedDefaultRegionAttributeReenableContract(Options const&, rti::CallbackModel);
void scenarioTimestampedDefaultRegionAttributeRegulationReenable(Options const&, rti::CallbackModel);
void scenarioTimestampedDefaultRegionAttributeRegulationReenableContract(Options const&, rti::CallbackModel);
void scenarioTimestampedDefaultRegionInteraction(Options const&, rti::CallbackModel);
void scenarioTimestampedDefaultRegionInteractionAlternateAdvances(Options const&, rti::CallbackModel);
void scenarioTimestampedDefaultRegionInteractionAlternateAdvancesContract(Options const&, rti::CallbackModel);
void scenarioTimestampedDefaultRegionInteractionContract(Options const&, rti::CallbackModel);
void scenarioTimestampedDefaultRegionInteractionMixedFanout(Options const&, rti::CallbackModel);
void scenarioTimestampedDefaultRegionInteractionMixedFanoutContract(Options const&, rti::CallbackModel);
void scenarioTimestampedDefaultRegionInteractionReenable(Options const&, rti::CallbackModel);
void scenarioTimestampedDefaultRegionInteractionReenableContract(Options const&, rti::CallbackModel);
void scenarioTimestampedDefaultRegionInteractionRegulationReenable(Options const&, rti::CallbackModel);
void scenarioTimestampedDefaultRegionInteractionRegulationReenableContract(Options const&, rti::CallbackModel);
void scenarioTimestampedDefaultRegionInteractionSourceResignation(Options const&, rti::CallbackModel);
void scenarioTimestampedDefaultRegionInteractionSourceResignationContract(Options const&, rti::CallbackModel);
void scenarioTimestampedDirectedInteractions(Options const&, rti::CallbackModel);
void scenarioTimestampedDirectedInteractionsContract(Options const&, rti::CallbackModel);
void scenarioTimestampedLocalDeleteAttribute(Options const&, rti::CallbackModel);
void scenarioTimestampedLocalDeleteAttributeContract(Options const&, rti::CallbackModel);
void scenarioTimestampedLocalDeleteObject(Options const&, rti::CallbackModel);
void scenarioTimestampedLocalDeleteObjectContract(Options const&, rti::CallbackModel);
void scenarioTimestampedRegionalAttributeAlternateAdvances(Options const&, rti::CallbackModel);
void scenarioTimestampedRegionalAttributeAlternateAdvancesContract(Options const&, rti::CallbackModel);
void scenarioTimestampedRegionalAttributeAssociationReplacement(Options const&, rti::CallbackModel);
void scenarioTimestampedRegionalAttributeAssociationReplacementContract(Options const&, rti::CallbackModel);
void scenarioTimestampedRegionalAttributeRegulationReenable(Options const&, rti::CallbackModel);
void scenarioTimestampedRegionalAttributeRegulationReenableContract(Options const&, rti::CallbackModel);
void scenarioTimestampedRegionalAttributeSourceResignation(Options const&, rti::CallbackModel);
void scenarioTimestampedRegionalAttributeSourceResignationContract(Options const&, rti::CallbackModel);
void scenarioTimestampedRegionalAttributeUpdate(Options const&, rti::CallbackModel);
void scenarioTimestampedRegionalAttributeUpdateContract(Options const&, rti::CallbackModel);
void scenarioTimestampedRegionalInteractionAlternateAdvances(Options const&, rti::CallbackModel);
void scenarioTimestampedRegionalInteractionAlternateAdvancesContract(Options const&, rti::CallbackModel);
void scenarioTimestampedRegionalInteractionContract(Options const&, rti::CallbackModel);
void scenarioTimestampedRegionalInteractionNoOverlap(Options const&, rti::CallbackModel);
void scenarioTimestampedRegionalInteractionNoOverlapContract(Options const&, rti::CallbackModel);
void scenarioTimestampedRegionalInteractionRegulationReenable(Options const&, rti::CallbackModel);
void scenarioTimestampedRegionalInteractionRegulationReenableContract(Options const&, rti::CallbackModel);
void scenarioTimestampedRegionalInteractionSourceResignation(Options const&, rti::CallbackModel);
void scenarioTimestampedRegionalInteractionSourceResignationContract(Options const&, rti::CallbackModel);
void scenarioTimestampedRegionalInteractionSubscriptionReplacement(Options const&, rti::CallbackModel);
void scenarioTimestampedRegionalInteractionSubscriptionReplacementContract(Options const&, rti::CallbackModel);
void scenarioTimestampedRegionalInteractionTarNmr(Options const&, rti::CallbackModel);
void scenarioTimestampedRegionalInteractionTarNmrContract(Options const&, rti::CallbackModel);
void scenarioTransportOrder(Options const&, rti::CallbackModel);
void scenarioTransportOrderContract(Options const&, rti::CallbackModel);
void scenarioVariableLengthDataContract(Options const&, rti::CallbackModel);
void scenarioZeroDimensionalRegionalInteraction(Options const&, rti::CallbackModel);
void scenarioZeroDimensionalRegionalInteractionContract(Options const&, rti::CallbackModel);
}  // namespace hla_rti_cpp_tck::detail
