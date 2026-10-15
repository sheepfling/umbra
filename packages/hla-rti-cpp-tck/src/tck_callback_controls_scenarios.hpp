#pragma once

#include "tck_harness.hpp"

namespace hla_rti_cpp_tck::detail {

void scenarioCallbackControls(Options const&, rti::CallbackModel);
void scenarioCallbackControlsAttributeRelevanceAdvisories(Options const&, rti::CallbackModel);
void scenarioCallbackControlsAttributeRelevanceAdvisoriesContract(Options const&, rti::CallbackModel);
void scenarioCallbackControlsAttributeScopeAdvisories(Options const&, rti::CallbackModel);
void scenarioCallbackControlsAttributeScopeAdvisoriesContract(Options const&, rti::CallbackModel);
void scenarioCallbackControlsAttributeUpdate(Options const&, rti::CallbackModel);
void scenarioCallbackControlsAttributeUpdateContract(Options const&, rti::CallbackModel);
void scenarioCallbackControlsAttributeValueRequest(Options const&, rti::CallbackModel);
void scenarioCallbackControlsAttributeValueRequestContract(Options const&, rti::CallbackModel);
void scenarioCallbackControlsAutoProvide(Options const&, rti::CallbackModel);
void scenarioCallbackControlsAutoProvideContract(Options const&, rti::CallbackModel);
void scenarioCallbackControlsAvailableTimeAdvanceCallbacks(Options const&, rti::CallbackModel);
void scenarioCallbackControlsAvailableTimeAdvanceCallbacksContract(Options const&, rti::CallbackModel);
void scenarioCallbackControlsContract(Options const&, rti::CallbackModel);
void scenarioCallbackControlsDeclarationAdvisories(Options const&, rti::CallbackModel);
void scenarioCallbackControlsDeclarationAdvisoriesContract(Options const&, rti::CallbackModel);
void scenarioCallbackControlsDirectedInteraction(Options const&, rti::CallbackModel);
void scenarioCallbackControlsDirectedInteractionContract(Options const&, rti::CallbackModel);
void scenarioCallbackControlsFederationReports(Options const&, rti::CallbackModel);
void scenarioCallbackControlsFederationReportsContract(Options const&, rti::CallbackModel);
void scenarioCallbackControlsFlushQueueGrant(Options const&, rti::CallbackModel);
void scenarioCallbackControlsFlushQueueGrantContract(Options const&, rti::CallbackModel);
void scenarioCallbackControlsMultipleObjectNameReservation(Options const&, rti::CallbackModel);
void scenarioCallbackControlsMultipleObjectNameReservationContract(Options const&, rti::CallbackModel);
void scenarioCallbackControlsObjectDiscovery(Options const&, rti::CallbackModel);
void scenarioCallbackControlsObjectDiscoveryContract(Options const&, rti::CallbackModel);
void scenarioCallbackControlsObjectNameReservation(Options const&, rti::CallbackModel);
void scenarioCallbackControlsObjectNameReservationContract(Options const&, rti::CallbackModel);
void scenarioCallbackControlsObjectNameReservationFailure(Options const&, rti::CallbackModel);
void scenarioCallbackControlsObjectNameReservationFailureContract(Options const&, rti::CallbackModel);
void scenarioCallbackControlsObjectRemoval(Options const&, rti::CallbackModel);
void scenarioCallbackControlsObjectRemovalContract(Options const&, rti::CallbackModel);
void scenarioCallbackControlsOwnershipAcquisitionCancellation(Options const&, rti::CallbackModel);
void scenarioCallbackControlsOwnershipAcquisitionCancellationContract(Options const&, rti::CallbackModel);
void scenarioCallbackControlsOwnershipAcquisitionNotification(Options const&, rti::CallbackModel);
void scenarioCallbackControlsOwnershipAcquisitionNotificationContract(Options const&, rti::CallbackModel);
void scenarioCallbackControlsOwnershipAssumption(Options const&, rti::CallbackModel);
void scenarioCallbackControlsOwnershipAssumptionContract(Options const&, rti::CallbackModel);
void scenarioCallbackControlsOwnershipDivestitureConfirmation(Options const&, rti::CallbackModel);
void scenarioCallbackControlsOwnershipDivestitureConfirmationContract(Options const&, rti::CallbackModel);
void scenarioCallbackControlsOwnershipQuery(Options const&, rti::CallbackModel);
void scenarioCallbackControlsOwnershipQueryContract(Options const&, rti::CallbackModel);
void scenarioCallbackControlsOwnershipReleaseDenied(Options const&, rti::CallbackModel);
void scenarioCallbackControlsOwnershipReleaseDeniedContract(Options const&, rti::CallbackModel);
void scenarioCallbackControlsOwnershipReleaseRequest(Options const&, rti::CallbackModel);
void scenarioCallbackControlsOwnershipReleaseRequestContract(Options const&, rti::CallbackModel);
void scenarioCallbackControlsOwnershipUnavailable(Options const&, rti::CallbackModel);
void scenarioCallbackControlsOwnershipUnavailableContract(Options const&, rti::CallbackModel);
void scenarioCallbackControlsRestoreRequestFailure(Options const&, rti::CallbackModel);
void scenarioCallbackControlsRestoreRequestFailureContract(Options const&, rti::CallbackModel);
void scenarioCallbackControlsSaveRestore(Options const&, rti::CallbackModel);
void scenarioCallbackControlsSaveRestoreContract(Options const&, rti::CallbackModel);
void scenarioCallbackControlsSaveRestoreFailures(Options const&, rti::CallbackModel);
void scenarioCallbackControlsSaveRestoreFailuresContract(Options const&, rti::CallbackModel);
void scenarioCallbackControlsSynchronization(Options const&, rti::CallbackModel);
void scenarioCallbackControlsSynchronizationContract(Options const&, rti::CallbackModel);
void scenarioCallbackControlsTimeAdvance(Options const&, rti::CallbackModel);
void scenarioCallbackControlsTimeAdvanceContract(Options const&, rti::CallbackModel);
void scenarioCallbackControlsTimeRoleEnablement(Options const&, rti::CallbackModel);
void scenarioCallbackControlsTimeRoleEnablementContract(Options const&, rti::CallbackModel);
void scenarioCallbackControlsTimestampedAttributeUpdate(Options const&, rti::CallbackModel);
void scenarioCallbackControlsTimestampedAttributeUpdateContract(Options const&, rti::CallbackModel);
void scenarioCallbackControlsTimestampedObjectRemoval(Options const&, rti::CallbackModel);
void scenarioCallbackControlsTimestampedObjectRemovalContract(Options const&, rti::CallbackModel);
void scenarioCallbackControlsTimestampedRegionalInteraction(Options const&, rti::CallbackModel);
void scenarioCallbackControlsTimestampedRegionalInteractionContract(Options const&, rti::CallbackModel);
void scenarioCallbackControlsTimestampedRetraction(Options const&, rti::CallbackModel);
void scenarioCallbackControlsTimestampedRetractionContract(Options const&, rti::CallbackModel);
void scenarioCallbackControlsTransportation(Options const&, rti::CallbackModel);
void scenarioCallbackControlsTransportationContract(Options const&, rti::CallbackModel);

}  // namespace hla_rti_cpp_tck::detail
