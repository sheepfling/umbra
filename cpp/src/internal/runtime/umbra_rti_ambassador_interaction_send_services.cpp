#include "internal/federation/federation_registry.hpp"
#include "internal/federation/process_federation_client.hpp"
#include "internal/encoding/variable_length_data_2025.hpp"
#include "internal/fom/hla_names.hpp"
#include "internal/handles/attribute_handle.hpp"
#include "internal/handles/federate_handle.hpp"
#include "internal/handles/interaction_class_handle.hpp"
#include "internal/handles/message_retraction_handle.hpp"
#include "internal/handles/object_instance_handle.hpp"
#include "internal/handles/parameter_handle.hpp"
#include "internal/handles/region_handle.hpp"
#include "internal/handles/transportation_type_handle.hpp"
#include "internal/observability/mom_service_report_encoding.hpp"
#include "internal/runtime/ambassador_shared_utilities.hpp"
#include "internal/runtime/ambassador_string_helpers.hpp"
#include "internal/runtime/utf8_string.hpp"
#include "internal/runtime/umbra_rti_ambassador.hpp"
#include "internal/runtime/umbra_rti_ambassador_service_failure_translation.hpp"
#include "internal/time/federate_time_state.hpp"
#include "internal/time/reference_time_selection.hpp"

#include <RTI/encoding/BasicDataElements.h>
#include <RTI/encoding/HLAvariableArray.h>

#include <cstdint>
#include <mutex>
#include <optional>
#include <set>
#include <string>
#include <utility>
#include <vector>

namespace rti1516_2025::umbra_binding_detail {
using namespace service_failure_translation;

#if defined(UMBRA_ENABLE_EMBEDDED_FEDERATION_MANAGEMENT)
namespace {
using InteractionParameterValue = AmbassadorInteractionParameterValue;



}  // namespace

// IEEE 1516.1-2025 Send Interaction service overloads.

void UmbraRtiAmbassador::sendInteraction(
    InteractionClassHandle const& interactionClass,
    ParameterHandleValueMap const& parameterValues,
    VariableLengthData const& userSuppliedTag) {
  auto instrumentationScope = beginRtiCall("sendInteraction");
#if defined(UMBRA_ENABLE_EMBEDDED_FEDERATION_MANAGEMENT)
  // The configured process endpoint owns this first public message-service
  // slice. Keep it ahead of the embedded registry path: a process client has
  // no local registry, and silently consulting the embedded hub would create
  // a second federation control plane. The private envelope preserves the
  // parameter values and user tag until the installable protocol is promoted.
  if (processEndpointActive_) {
    std::wstring federationName;
    std::uint64_t producingFederateId = 0U;
    umbra::detail::ProcessFederationClient* processClient = nullptr;
    {
      std::scoped_lock lock(mutex_);
      requireConnectedForFederationManagement(lifecycle_);
      if (lifecycle_.state() != umbra::detail::FederateLifecycleState::joined ||
          !joinedFederationName_ || !joinedFederateId_) {
        throw FederateNotExecutionMember(
            L"Send Interaction requires membership in a federation execution.");
      }
      processClient = processFederationClient_.get();
      if (processClient == nullptr) {
        throw RTIinternalError(
            L"The configured process endpoint has no active federation client.");
      }
      federationName = *joinedFederationName_;
      producingFederateId = *joinedFederateId_;
    }

    auto const interactionClassHandle = interactionClassHandleValue(interactionClass);
    if (!interactionClassHandle) {
      throw InteractionClassNotDefined(
          L"Send Interaction requires a defined InteractionClassHandle.");
    }
    auto const parameterHandles = ambassadorInteractionParameterHandleValues(parameterValues);
    if (!parameterHandles) {
      throw InteractionParameterNotDefined(
          L"Send Interaction requires defined ParameterHandle values.");
    }

    std::vector<umbra::detail::ProcessFederationInteractionParameterValue>
        processParameterValues;
    auto const sentParameters = ambassadorCopyInteractionParameterValues(parameterValues);
    VariableLengthData copiedTag(userSuppliedTag);
    ParameterHandleValueMap reportParameterValues;
    processParameterValues.reserve(sentParameters.size());
    for (auto const& [parameterHandle, parameterValue] : sentParameters) {
      reportParameterValues.emplace(
          makeParameterHandle(parameterHandle), parameterValue);
      auto bytes = umbra::detail::variable_length_data_2025::copyBytes(parameterValue);
      processParameterValues.emplace_back(parameterHandle, std::move(bytes));
    }
    auto processTag =
        umbra::detail::variable_length_data_2025::copyBytes(userSuppliedTag);
    auto const reportArguments =
        std::vector<umbra::detail::MomServiceArgument>{
            {umbra::detail::MomArgumentType::interaction_class_handle,
             L"Interaction class designator",
             umbra::detail::formatMomInteractionClassHandle(interactionClass)},
            {umbra::detail::MomArgumentType::parameter_handle_value_map,
             L"Constrained set of interaction parameter designator and value pairs",
             umbra::detail::formatMomParameterHandleValueMap(reportParameterValues)},
            {umbra::detail::MomArgumentType::table_5_user_supplied_tag,
             L"User-supplied tag",
             umbra::detail::formatMomUserSuppliedTag(copiedTag)},
            {umbra::detail::MomArgumentType::null_value,
             L"Optional timestamp",
             umbra::detail::formatMomNull()},
        };

    std::vector<std::uint8_t> processPayload;
    try {
      processPayload = umbra::detail::encodeProcessFederationInteractionEnvelope(
          umbra::detail::ProcessFederationInteractionEnvelope{
              std::move(processParameterValues), std::move(processTag)});
      static_cast<void>(processClient->sendInteraction(
          std::move(federationName),
          producingFederateId,
          *interactionClassHandle,
          *parameterHandles,
          std::move(processPayload)));
      appendSuccessfulVoidServiceReportToFileIfSelected(
          L"SendInteraction",
          umbra::detail::MomServiceType::object_management,
          reportArguments,
          true);
    } catch (umbra::detail::ProcessFederationServiceProtocolError const& error) {
      throw RTIinternalError(wideAscii(error.what()));
    } catch (umbra::detail::ProcessFederationClientError const& error) {
      throw RTIinternalError(wideAscii(error.what()));
    }

    // A pushed event is captured while the synchronous send request is being
    // completed. Convert each captured event into the shared official callback
    // dispatcher before returning; HLA_EVOKED then exposes it through the
    // normal public Evoke service, while HLA_IMMEDIATE invokes it here.
    while (processClient->pendingPushedEventCount() != 0U) {
      try {
        processClient->dispatchPushedReceiveOrder();
      } catch (umbra::detail::ProcessFederationServiceProtocolError const& error) {
        throw RTIinternalError(wideAscii(error.what()));
      } catch (umbra::detail::ProcessFederationCallbackBridgeError const& error) {
        throw RTIinternalError(wideAscii(error.what()));
      }
    }
    while (processClient->pendingPushedObjectInstanceDiscoveryCount() != 0U) {
      try {
        processClient->dispatchPushedObjectInstanceDiscovery();
      } catch (umbra::detail::ProcessFederationServiceProtocolError const& error) {
        throw RTIinternalError(wideAscii(error.what()));
      } catch (umbra::detail::ProcessFederationCallbackBridgeError const& error) {
        throw RTIinternalError(wideAscii(error.what()));
      }
    }
    return;
  }
#endif
  // Embedded entry state and object/federation MOM report requests.

  std::optional<std::wstring> momExceptionService;
  try {
  std::optional<std::wstring> federationName;
  std::optional<std::uint64_t> producingFederateId;
  // Preserve the 2025 service's connection and membership preconditions ahead
  // of caller-supplied handle validation, matching the other public services.
  {
    std::scoped_lock lock(mutex_, ambassadorFederationManagementMutex());
    requireConnectedForFederationManagement(lifecycle_);
    requireFederationServiceOperationAvailable(L"Send Interaction");
    if (lifecycle_.state() != umbra::detail::FederateLifecycleState::joined ||
        !joinedFederationName_ || !joinedFederateId_) {
      throw FederateNotExecutionMember(
          L"Send Interaction requires membership in a federation execution.");
    }
    if (!embeddedFederationRegistry().memberById(
            *joinedFederationName_,
            *joinedFederateId_)) {
      throw FederateNotExecutionMember(
          L"The embedded federation no longer records this RTI ambassador as a member.");
    }
    federationName = *joinedFederationName_;
    producingFederateId = *joinedFederateId_;
  }

  auto const interactionClassHandle = interactionClassHandleValue(interactionClass);
  if (!interactionClassHandle) {
    throw InteractionClassNotDefined(
        L"Send Interaction requires a defined InteractionClassHandle.");
  }
  auto const parameterHandles = ambassadorInteractionParameterHandleValues(parameterValues);
  if (!parameterHandles) {
    throw InteractionParameterNotDefined(
        L"Send Interaction requires defined ParameterHandle values.");
  }

  // Keep the fully-qualified MOM interaction name for the distinct
  // HLAreportMOMexception route.  Application interactions retain the normal
  // HLAreportException behavior and never get a MOM-failure projection.
  if (auto const interactionName = embeddedFederationRegistry()
          .interactionClassNameFor(*federationName, *interactionClassHandle)) {
    constexpr std::string_view kMomInteractionPrefix =
        umbra::detail::hla::utf8::mom::interaction_manager_prefix;
    if (interactionName->compare(0, kMomInteractionPrefix.size(),
                                 kMomInteractionPrefix) == 0) {
      momExceptionService = umbra::detail::wideFromUtf8(*interactionName);
    }
  }

  std::optional<umbra::detail::MomObjectInstanceCountsReportPlan>
      momObjectInstancesUpdatedReport;
  std::optional<umbra::detail::MomObjectInstanceCountsReportPlan>
      momObjectInstancesThatCanBeDeletedReport;
  std::optional<umbra::detail::MomObjectInstanceCountsReportPlan>
      momObjectInstancesReflectedReport;
  std::optional<umbra::detail::MomUpdatesSentReportPlan>
      momUpdatesSentReport;
  std::optional<umbra::detail::MomInteractionsSentReportPlan>
      momInteractionsSentReport;
  std::optional<umbra::detail::MomDirectedInteractionsSentReportPlan>
      momDirectedInteractionsSentReport;
  std::optional<umbra::detail::MomInteractionsReceivedReportPlan>
      momInteractionsReceivedReport;
  std::optional<umbra::detail::MomDirectedInteractionsReceivedReportPlan>
      momDirectedInteractionsReceivedReport;
  std::optional<umbra::detail::MomReflectionsReceivedReportPlan>
      momReflectionsReceivedReport;
  std::optional<umbra::detail::MomObjectInstanceInformationReportPlan>
      momObjectInstanceInformationReport;
  std::optional<umbra::detail::MomFomModuleDataReportPlan>
      momFomModuleDataReport;
  std::optional<umbra::detail::MomFederationFomModuleDataReportPlan>
      momFederationFomModuleDataReport;
  std::optional<umbra::detail::MomFederationMimDataReportPlan>
      momFederationMimDataReport;
  std::optional<umbra::detail::MomFederationSynchronizationPointsReportPlan>
      momFederationSynchronizationPointsReport;
  std::optional<umbra::detail::MomFederationSynchronizationPointStatusReportPlan>
      momFederationSynchronizationPointStatusReport;
  std::optional<umbra::detail::MomPublicationsReportPlan>
      momPublicationsReport;
  std::optional<umbra::detail::MomSubscriptionsReportPlan>
      momSubscriptionsReport;
  auto const handleMomObjectInstanceReportRequest = [&]() {
    std::scoped_lock lock(mutex_, ambassadorFederationManagementMutex());
    requireConnectedForFederationManagement(lifecycle_);
    requireFederationServiceOperationAvailable(L"Send Interaction");
    if (lifecycle_.state() != umbra::detail::FederateLifecycleState::joined ||
        !joinedFederationName_ || !joinedFederateId_ ||
        *joinedFederationName_ != *federationName ||
        *joinedFederateId_ != *producingFederateId) {
      throw FederateNotExecutionMember(
          L"The RTI ambassador's federation membership changed during Send Interaction.");
    }
    auto& registry = embeddedFederationRegistry();
    if (!registry.memberById(*federationName, *producingFederateId)) {
      throw FederateNotExecutionMember(
          L"The embedded federation no longer records this RTI ambassador as a member.");
    }
    auto const interactionClassName = registry.interactionClassNameFor(
        *federationName,
        *interactionClassHandle);
    if (!interactionClassName) {
      return false;
    }
    auto const isObjectInstancesUpdatedRequest =
        registry.interactionClassIsSameOrDescendantOf(
            *federationName,
            *interactionClassHandle,
            umbra::detail::hla::utf8::mom::request_object_instances_updated);
    if (!isObjectInstancesUpdatedRequest) {
      throw RTIinternalError(
          L"The embedded federation cannot resolve the HLArequestObjectInstancesUpdated MOM class hierarchy.");
    }
    auto const isObjectInstancesThatCanBeDeletedRequest =
        registry.interactionClassIsSameOrDescendantOf(
            *federationName,
            *interactionClassHandle,
            umbra::detail::hla::utf8::mom::request_object_instances_that_can_be_deleted);
    if (!isObjectInstancesThatCanBeDeletedRequest) {
      throw RTIinternalError(
          L"The embedded federation cannot resolve the HLArequestObjectInstancesThatCanBeDeleted MOM class hierarchy.");
    }
    auto const isObjectInstancesReflectedRequest =
        registry.interactionClassIsSameOrDescendantOf(
            *federationName,
            *interactionClassHandle,
            umbra::detail::hla::utf8::mom::request_object_instances_reflected);
    if (!isObjectInstancesReflectedRequest) {
      throw RTIinternalError(
          L"The embedded federation cannot resolve the HLArequestObjectInstancesReflected MOM class hierarchy.");
    }
    auto const requestFamilyCount =
        static_cast<unsigned>(*isObjectInstancesUpdatedRequest) +
        static_cast<unsigned>(*isObjectInstancesThatCanBeDeletedRequest) +
        static_cast<unsigned>(*isObjectInstancesReflectedRequest);
    if (requestFamilyCount == 0U) {
      return false;
    }
    if (requestFamilyCount > 1U) {
      throw RTIinternalError(
          L"The embedded federation gives a MOM object-instance request class ambiguous report semantics.");
    }

    std::optional<std::uint64_t> reportedFederateId;
    for (auto const& [parameterHandle, parameterValue] : parameterValues) {
      auto const suppliedParameterHandleValue = parameterHandleValue(parameterHandle);
      if (!suppliedParameterHandleValue) {
          throw InteractionParameterNotDefined(
            L"The object-instance MOM request received an invalid parameter handle.");
      }
      auto const parameterName = registry.parameterNameFor(
          *federationName,
          *interactionClassName,
          *suppliedParameterHandleValue);
      if (!parameterName) {
          throw InteractionParameterNotDefined(
            L"The object-instance MOM request received an unknown parameter for this interaction class.");
      }
      if (*parameterName != umbra::detail::hla::utf8::mom::federate) {
        // Compatible MOM extensions may carry additional request parameters;
        // the predefined request target is the only value this slice consumes.
        continue;
      }
      try {
        auto const decodedHandle =
            ::rti1516_2025::umbra_binding_detail::decodeFederateHandle(parameterValue);
        reportedFederateId =
            ::rti1516_2025::umbra_binding_detail::federateHandleValue(decodedHandle);
      } catch (Exception const&) {
        throw RTIinternalError(
            L"The object-instance MOM request received an invalid HLAfederateReference value.");
      }
      if (!reportedFederateId) {
        throw RTIinternalError(
            L"The object-instance MOM request received an invalid HLAfederateReference value.");
      }
    }
    if (!reportedFederateId) {
      throw InteractionParameterNotDefined(
          L"The object-instance MOM request requires the HLAfederate parameter.");
    }
    auto report = *isObjectInstancesUpdatedRequest
        ? registry.planMomObjectInstancesUpdatedReport(
              *federationName,
              *producingFederateId,
              *reportedFederateId)
        : *isObjectInstancesThatCanBeDeletedRequest
            ? registry.planMomObjectInstancesThatCanBeDeletedReport(
                  *federationName,
                  *producingFederateId,
                  *reportedFederateId)
            : registry.planMomObjectInstancesReflectedReport(
                  *federationName,
                  *producingFederateId,
                  *reportedFederateId);
    switch (report.status) {
      case umbra::detail::MomObjectInstanceCountsReportStatus::applied:
        if (*isObjectInstancesUpdatedRequest) {
          momObjectInstancesUpdatedReport = std::move(report);
        } else if (*isObjectInstancesThatCanBeDeletedRequest) {
          momObjectInstancesThatCanBeDeletedReport = std::move(report);
        } else {
          momObjectInstancesReflectedReport = std::move(report);
        }
        return true;
      case umbra::detail::MomObjectInstanceCountsReportStatus::federation_does_not_exist:
      case umbra::detail::MomObjectInstanceCountsReportStatus::requesting_federate_not_member:
      case umbra::detail::MomObjectInstanceCountsReportStatus::reported_federate_not_member:
        throw FederateNotExecutionMember(
            L"The object-instance MOM request requires joined requesting and reported federates.");
      case umbra::detail::MomObjectInstanceCountsReportStatus::inconsistent_catalog:
        throw RTIinternalError(
            L"The embedded federation cannot reconstruct the requested object-instance MOM report.");
    }
    throw RTIinternalError(
        L"The embedded federation rejected the object-instance MOM request.");
  };

  if (handleMomObjectInstanceReportRequest()) {
    std::vector<InteractionParameterValue> sentParameters =
        ambassadorCopyInteractionParameterValues(parameterValues);
    VariableLengthData copiedTag(userSuppliedTag);
    ParameterHandleValueMap reportParameterValues;
    for (auto const& [parameterHandle, parameterValue] : sentParameters) {
      reportParameterValues.emplace(makeParameterHandle(parameterHandle), parameterValue);
    }
    auto const reportArguments = std::vector<umbra::detail::MomServiceArgument>{
        {umbra::detail::MomArgumentType::interaction_class_handle,
         L"Interaction class designator",
         umbra::detail::formatMomInteractionClassHandle(interactionClass)},
        {umbra::detail::MomArgumentType::parameter_handle_value_map,
         L"Constrained set of interaction parameter designator and value pairs",
         umbra::detail::formatMomParameterHandleValueMap(reportParameterValues)},
        {umbra::detail::MomArgumentType::table_5_user_supplied_tag,
         L"User-supplied tag",
         umbra::detail::formatMomUserSuppliedTag(copiedTag)},
        {umbra::detail::MomArgumentType::null_value,
         L"Optional timestamp",
         umbra::detail::formatMomNull()},
    };
    appendSuccessfulVoidServiceReportToFileIfSelected(
        L"SendInteraction",
        umbra::detail::MomServiceType::object_management,
        reportArguments,
        true);
    {
      std::scoped_lock lock(ambassadorFederationManagementMutex());
      auto const status = embeddedFederationRegistry()
          .recordSuccessfulInteractionSend(
              *federationName,
              *producingFederateId,
              *interactionClassHandle,
              umbra::detail::hla::utf8::mom::reliable,
              false);
      if (status != umbra::detail::FederationRegistryStatus::applied) {
        throw RTIinternalError(
            L"The embedded federation lost the Send Interaction membership before its accepted MOM request boundary.");
      }
    }
    if (momObjectInstancesUpdatedReport) {
      auto report = std::move(*momObjectInstancesUpdatedReport);
      auto encodedCounts = ambassadorEncodeMomObjectClassBasedCounts(report.objectClassCounts);
      queueMomObjectInstancesUpdatedReport(
          *federationName,
          report.reportedFederateId,
          std::move(report),
          std::move(encodedCounts));
    } else if (momObjectInstancesThatCanBeDeletedReport) {
      auto report = std::move(*momObjectInstancesThatCanBeDeletedReport);
      auto encodedCounts = ambassadorEncodeMomObjectClassBasedCounts(report.objectClassCounts);
      queueMomObjectInstancesThatCanBeDeletedReport(
          *federationName,
          report.reportedFederateId,
          std::move(report),
          std::move(encodedCounts));
    } else if (momObjectInstancesReflectedReport) {
      auto report = std::move(*momObjectInstancesReflectedReport);
      auto encodedCounts = ambassadorEncodeMomObjectClassBasedCounts(report.objectClassCounts);
      queueMomObjectInstancesReflectedReport(
          *federationName,
          report.reportedFederateId,
          std::move(report),
          std::move(encodedCounts));
    }
    return;
  }

  auto const handleMomObjectInstanceInformationReportRequest = [&]() {
    std::scoped_lock lock(mutex_, ambassadorFederationManagementMutex());
    requireConnectedForFederationManagement(lifecycle_);
    requireFederationServiceOperationAvailable(L"Send Interaction");
    if (lifecycle_.state() != umbra::detail::FederateLifecycleState::joined ||
        !joinedFederationName_ || !joinedFederateId_ ||
        *joinedFederationName_ != *federationName ||
        *joinedFederateId_ != *producingFederateId) {
      throw FederateNotExecutionMember(
          L"The RTI ambassador's federation membership changed during Send Interaction.");
    }
    auto& registry = embeddedFederationRegistry();
    if (!registry.memberById(*federationName, *producingFederateId)) {
      throw FederateNotExecutionMember(
          L"The embedded federation no longer records this RTI ambassador as a member.");
    }
    auto const interactionClassName = registry.interactionClassNameFor(
        *federationName,
        *interactionClassHandle);
    if (!interactionClassName) {
      return false;
    }
    auto const isObjectInstanceInformationRequest =
        registry.interactionClassIsSameOrDescendantOf(
            *federationName,
            *interactionClassHandle,
            umbra::detail::hla::utf8::mom::request_object_instance_information);
    if (!isObjectInstanceInformationRequest) {
      throw RTIinternalError(
          L"The embedded federation cannot resolve the HLArequestObjectInstanceInformation MOM class hierarchy.");
    }
    if (!*isObjectInstanceInformationRequest) {
      return false;
    }

    std::optional<std::uint64_t> objectInstanceHandle;
    for (auto const& [parameterHandle, parameterValue] : parameterValues) {
      auto const suppliedParameterHandleValue = parameterHandleValue(parameterHandle);
      if (!suppliedParameterHandleValue) {
        throw InteractionParameterNotDefined(
            L"HLArequestObjectInstanceInformation received an invalid parameter handle.");
      }
      auto const parameterName = registry.parameterNameFor(
          *federationName,
          *interactionClassName,
          *suppliedParameterHandleValue);
      if (!parameterName) {
        throw InteractionParameterNotDefined(
            L"HLArequestObjectInstanceInformation received an unknown parameter for this interaction class.");
      }
      if (*parameterName != umbra::detail::hla::utf8::mom::object_instance) {
        continue;
      }
      try {
        auto const decodedHandle =
            ::rti1516_2025::umbra_binding_detail::decodeObjectInstanceHandle(
                parameterValue);
        objectInstanceHandle =
            ::rti1516_2025::umbra_binding_detail::objectInstanceHandleValue(
                decodedHandle);
      } catch (Exception const&) {
        throw RTIinternalError(
            L"HLArequestObjectInstanceInformation received an invalid HLAobjectInstanceReference value.");
      }
      if (!objectInstanceHandle) {
        throw RTIinternalError(
            L"HLArequestObjectInstanceInformation received an invalid HLAobjectInstanceReference value.");
      }
    }
    if (!objectInstanceHandle) {
      throw InteractionParameterNotDefined(
          L"HLArequestObjectInstanceInformation requires the HLAobjectInstance parameter.");
    }

    auto report = registry.planMomObjectInstanceInformationReport(
        *federationName,
        *producingFederateId,
        *objectInstanceHandle);
    switch (report.status) {
      case umbra::detail::MomObjectInstanceInformationReportStatus::applied:
        momObjectInstanceInformationReport = std::move(report);
        return true;
      case umbra::detail::MomObjectInstanceInformationReportStatus::federation_does_not_exist:
      case umbra::detail::MomObjectInstanceInformationReportStatus::requesting_federate_not_member:
        throw FederateNotExecutionMember(
            L"HLArequestObjectInstanceInformation requires a joined requesting federate.");
      case umbra::detail::MomObjectInstanceInformationReportStatus::inconsistent_catalog:
        throw RTIinternalError(
            L"The embedded federation cannot reconstruct the requested HLAreportObjectInstanceInformation interaction.");
    }
    throw RTIinternalError(
        L"The embedded federation rejected the HLArequestObjectInstanceInformation interaction.");
  };

  if (handleMomObjectInstanceInformationReportRequest()) {
    std::vector<InteractionParameterValue> sentParameters =
        ambassadorCopyInteractionParameterValues(parameterValues);
    VariableLengthData copiedTag(userSuppliedTag);
    ParameterHandleValueMap reportParameterValues;
    for (auto const& [parameterHandle, parameterValue] : sentParameters) {
      reportParameterValues.emplace(makeParameterHandle(parameterHandle), parameterValue);
    }
    auto const reportArguments = std::vector<umbra::detail::MomServiceArgument>{
        {umbra::detail::MomArgumentType::interaction_class_handle,
         L"Interaction class designator",
         umbra::detail::formatMomInteractionClassHandle(interactionClass)},
        {umbra::detail::MomArgumentType::parameter_handle_value_map,
         L"Constrained set of interaction parameter designator and value pairs",
         umbra::detail::formatMomParameterHandleValueMap(reportParameterValues)},
        {umbra::detail::MomArgumentType::table_5_user_supplied_tag,
         L"User-supplied tag",
         umbra::detail::formatMomUserSuppliedTag(copiedTag)},
        {umbra::detail::MomArgumentType::null_value,
         L"Optional timestamp",
         umbra::detail::formatMomNull()},
    };
    appendSuccessfulVoidServiceReportToFileIfSelected(
        L"SendInteraction",
        umbra::detail::MomServiceType::object_management,
        reportArguments,
        true);
    {
      std::scoped_lock lock(ambassadorFederationManagementMutex());
      auto const status = embeddedFederationRegistry()
          .recordSuccessfulInteractionSend(
              *federationName,
              *producingFederateId,
              *interactionClassHandle,
              umbra::detail::hla::utf8::mom::reliable,
              false);
      if (status != umbra::detail::FederationRegistryStatus::applied) {
        throw RTIinternalError(
            L"The embedded federation lost the Send Interaction membership before its accepted MOM request boundary.");
      }
    }
    queueMomObjectInstanceInformationReport(
        *federationName,
        momObjectInstanceInformationReport->reportedFederateId,
        std::move(*momObjectInstanceInformationReport));
    return;
  }

  auto const handleMomFederationSynchronizationReportRequest = [&]() {
    std::scoped_lock lock(mutex_, ambassadorFederationManagementMutex());
    requireConnectedForFederationManagement(lifecycle_);
    requireFederationServiceOperationAvailable(L"Send Interaction");
    if (lifecycle_.state() != umbra::detail::FederateLifecycleState::joined ||
        !joinedFederationName_ || !joinedFederateId_ ||
        *joinedFederationName_ != *federationName ||
        *joinedFederateId_ != *producingFederateId) {
      throw FederateNotExecutionMember(
          L"The RTI ambassador's federation membership changed during Send Interaction.");
    }
    auto& registry = embeddedFederationRegistry();
    if (!registry.memberById(*federationName, *producingFederateId)) {
      throw FederateNotExecutionMember(
          L"The embedded federation no longer records this RTI ambassador as a member.");
    }
    auto const interactionClassName = registry.interactionClassNameFor(
        *federationName,
        *interactionClassHandle);
    if (!interactionClassName) {
      return false;
    }
    auto const isSynchronizationPointsRequest =
        registry.interactionClassIsSameOrDescendantOf(
            *federationName,
            *interactionClassHandle,
            umbra::detail::hla::utf8::mom::request_synchronization_points);
    if (!isSynchronizationPointsRequest) {
      throw RTIinternalError(
          L"The embedded federation cannot resolve HLArequestSynchronizationPoints hierarchy.");
    }
    auto const isSynchronizationPointStatusRequest =
        registry.interactionClassIsSameOrDescendantOf(
            *federationName,
            *interactionClassHandle,
            umbra::detail::hla::utf8::mom::request_synchronization_point_status);
    if (!isSynchronizationPointStatusRequest) {
      throw RTIinternalError(
          L"The embedded federation cannot resolve HLArequestSynchronizationPointStatus hierarchy.");
    }
    auto const requestFamilyCount =
        static_cast<unsigned>(*isSynchronizationPointsRequest) +
        static_cast<unsigned>(*isSynchronizationPointStatusRequest);
    if (requestFamilyCount == 0U) {
      return false;
    }
    if (requestFamilyCount > 1U) {
      throw RTIinternalError(
          L"The embedded federation gives a synchronization-point MOM request class ambiguous report semantics.");
    }

    if (*isSynchronizationPointsRequest) {
      for (auto const& [parameterHandle, parameterValue] : parameterValues) {
        static_cast<void>(parameterValue);
        auto const suppliedParameterHandleValue = parameterHandleValue(parameterHandle);
        if (!suppliedParameterHandleValue ||
            !registry.parameterNameFor(
                *federationName,
                *interactionClassName,
                *suppliedParameterHandleValue)) {
          throw InteractionParameterNotDefined(
              L"HLArequestSynchronizationPoints received an invalid parameter handle.");
        }
        throw InteractionParameterNotDefined(
            L"HLArequestSynchronizationPoints does not accept parameters.");
      }
      auto report = registry.planMomFederationSynchronizationPointsReport(
          *federationName,
          *producingFederateId);
      switch (report.status) {
        case umbra::detail::MomFederationSynchronizationPointsReportStatus::applied:
          momFederationSynchronizationPointsReport = std::move(report);
          return true;
        case umbra::detail::MomFederationSynchronizationPointsReportStatus::federation_does_not_exist:
        case umbra::detail::MomFederationSynchronizationPointsReportStatus::requesting_federate_not_member:
          throw FederateNotExecutionMember(
              L"HLArequestSynchronizationPoints requires a joined requesting federate.");
        case umbra::detail::MomFederationSynchronizationPointsReportStatus::inconsistent_catalog:
          throw RTIinternalError(
              L"The embedded federation cannot reconstruct HLAreportSynchronizationPoints.");
      }
    }

    std::optional<std::wstring> synchronizationPointName;
    for (auto const& [parameterHandle, parameterValue] : parameterValues) {
      auto const suppliedParameterHandleValue = parameterHandleValue(parameterHandle);
      if (!suppliedParameterHandleValue) {
        throw InteractionParameterNotDefined(
            L"HLArequestSynchronizationPointStatus received an invalid parameter handle.");
      }
      auto const parameterName = registry.parameterNameFor(
          *federationName,
          *interactionClassName,
          *suppliedParameterHandleValue);
      if (!parameterName || *parameterName != umbra::detail::hla::utf8::mom::sync_point_name) {
        throw InteractionParameterNotDefined(
            L"HLArequestSynchronizationPointStatus received an unexpected parameter.");
      }
      rti1516_2025::HLAunicodeString decodedName;
      try {
        decodedName.decode(parameterValue);
      } catch (Exception const&) {
        throw RTIinternalError(
            L"HLArequestSynchronizationPointStatus received an invalid HLAsyncPointName value.");
      }
      synchronizationPointName = decodedName.get();
    }
    if (!synchronizationPointName) {
      throw InteractionParameterNotDefined(
          L"HLArequestSynchronizationPointStatus requires the HLAsyncPointName parameter.");
    }
    auto report = registry.planMomFederationSynchronizationPointStatusReport(
        *federationName,
        *producingFederateId,
        std::move(*synchronizationPointName));
    switch (report.status) {
      case umbra::detail::MomFederationSynchronizationPointStatusReportStatus::applied:
        momFederationSynchronizationPointStatusReport = std::move(report);
        return true;
      case umbra::detail::MomFederationSynchronizationPointStatusReportStatus::federation_does_not_exist:
      case umbra::detail::MomFederationSynchronizationPointStatusReportStatus::requesting_federate_not_member:
        throw FederateNotExecutionMember(
            L"HLArequestSynchronizationPointStatus requires a joined requesting federate.");
      case umbra::detail::MomFederationSynchronizationPointStatusReportStatus::inconsistent_catalog:
        throw RTIinternalError(
            L"The embedded federation cannot reconstruct HLAreportSynchronizationPointStatus.");
    }
    throw RTIinternalError(
        L"The embedded federation rejected the synchronization-point MOM request.");
  };

  if (handleMomFederationSynchronizationReportRequest()) {
    std::vector<InteractionParameterValue> sentParameters =
        ambassadorCopyInteractionParameterValues(parameterValues);
    VariableLengthData copiedTag(userSuppliedTag);
    ParameterHandleValueMap reportParameterValues;
    for (auto const& [parameterHandle, parameterValue] : sentParameters) {
      reportParameterValues.emplace(makeParameterHandle(parameterHandle), parameterValue);
    }
    auto const reportArguments = std::vector<umbra::detail::MomServiceArgument>{
        {umbra::detail::MomArgumentType::interaction_class_handle,
         L"Interaction class designator",
         umbra::detail::formatMomInteractionClassHandle(interactionClass)},
        {umbra::detail::MomArgumentType::parameter_handle_value_map,
         L"Constrained set of interaction parameter designator and value pairs",
         umbra::detail::formatMomParameterHandleValueMap(reportParameterValues)},
        {umbra::detail::MomArgumentType::table_5_user_supplied_tag,
         L"User-supplied tag",
         umbra::detail::formatMomUserSuppliedTag(copiedTag)},
        {umbra::detail::MomArgumentType::null_value,
         L"Optional timestamp",
         umbra::detail::formatMomNull()},
    };
    appendSuccessfulVoidServiceReportToFileIfSelected(
        L"SendInteraction",
        umbra::detail::MomServiceType::object_management,
        reportArguments,
        true);
    {
      std::scoped_lock lock(ambassadorFederationManagementMutex());
      auto const status = embeddedFederationRegistry()
          .recordSuccessfulInteractionSend(
              *federationName,
              *producingFederateId,
              *interactionClassHandle,
              umbra::detail::hla::utf8::mom::reliable,
              false);
      if (status != umbra::detail::FederationRegistryStatus::applied) {
        throw RTIinternalError(
            L"The embedded federation lost the Send Interaction membership before its accepted synchronization-point MOM request boundary.");
      }
    }
    if (momFederationSynchronizationPointsReport) {
      queueMomFederationSynchronizationPointsReport(
          *federationName,
          std::move(*momFederationSynchronizationPointsReport));
    } else if (momFederationSynchronizationPointStatusReport) {
      queueMomFederationSynchronizationPointStatusReport(
          *federationName,
          std::move(*momFederationSynchronizationPointStatusReport));
    }
    return;
  }

  auto const handleMomFederationContentReportRequest = [&]() {
    std::scoped_lock lock(mutex_, ambassadorFederationManagementMutex());
    requireConnectedForFederationManagement(lifecycle_);
    requireFederationServiceOperationAvailable(L"Send Interaction");
    if (lifecycle_.state() != umbra::detail::FederateLifecycleState::joined ||
        !joinedFederationName_ || !joinedFederateId_ ||
        *joinedFederationName_ != *federationName ||
        *joinedFederateId_ != *producingFederateId) {
      throw FederateNotExecutionMember(
          L"The RTI ambassador's federation membership changed during Send Interaction.");
    }
    auto& registry = embeddedFederationRegistry();
    if (!registry.memberById(*federationName, *producingFederateId)) {
      throw FederateNotExecutionMember(
          L"The embedded federation no longer records this RTI ambassador as a member.");
    }
    auto const interactionClassName = registry.interactionClassNameFor(
        *federationName,
        *interactionClassHandle);
    if (!interactionClassName) {
      return false;
    }
    auto const isFomModuleDataRequest =
        registry.interactionClassIsSameOrDescendantOf(
            *federationName,
            *interactionClassHandle,
            umbra::detail::hla::utf8::mom::request_fom_module_data_federation);
    if (!isFomModuleDataRequest) {
      throw RTIinternalError(
          L"The embedded federation cannot resolve the federation HLArequestFOMmoduleData MOM class hierarchy.");
    }
    auto const isMimDataRequest = registry.interactionClassIsSameOrDescendantOf(
        *federationName,
        *interactionClassHandle,
        umbra::detail::hla::utf8::mom::request_mim_data);
    if (!isMimDataRequest) {
      throw RTIinternalError(
          L"The embedded federation cannot resolve the federation HLArequestMIMdata MOM class hierarchy.");
    }
    auto const requestFamilyCount =
        static_cast<unsigned>(*isFomModuleDataRequest) +
        static_cast<unsigned>(*isMimDataRequest);
    if (requestFamilyCount == 0U) {
      return false;
    }
    if (requestFamilyCount > 1U) {
      throw RTIinternalError(
          L"The embedded federation gives a federation MOM request class ambiguous content-report semantics.");
    }

    if (*isFomModuleDataRequest) {
      std::optional<std::uint32_t> moduleIndex;
      for (auto const& [parameterHandle, parameterValue] : parameterValues) {
        auto const suppliedParameterHandleValue = parameterHandleValue(parameterHandle);
        if (!suppliedParameterHandleValue) {
          throw InteractionParameterNotDefined(
              L"HLArequestFOMmoduleData received an invalid parameter handle.");
        }
        auto const parameterName = registry.parameterNameFor(
            *federationName,
            *interactionClassName,
            *suppliedParameterHandleValue);
        if (!parameterName || *parameterName != umbra::detail::hla::utf8::mom::fom_module_indicator) {
          throw InteractionParameterNotDefined(
              L"HLArequestFOMmoduleData received an unexpected parameter.");
        }
        rti1516_2025::HLAinteger32BE decodedIndicator;
        try {
          decodedIndicator.decode(parameterValue);
        } catch (Exception const&) {
          throw RTIinternalError(
              L"HLArequestFOMmoduleData received an invalid HLAindex value.");
        }
        auto const decodedValue = decodedIndicator.get();
        if (decodedValue < 0) {
          throw RTIinternalError(
              L"HLArequestFOMmoduleData requires a non-negative HLAindex value.");
        }
        moduleIndex = static_cast<std::uint32_t>(decodedValue);
      }
      if (!moduleIndex) {
        throw InteractionParameterNotDefined(
            L"HLArequestFOMmoduleData requires the HLAFOMmoduleIndicator parameter.");
      }
      auto report = registry.planMomFederationFomModuleDataReport(
          *federationName,
          *producingFederateId,
          *moduleIndex);
      switch (report.status) {
        case umbra::detail::MomFederationFomModuleDataReportStatus::applied:
          momFederationFomModuleDataReport = std::move(report);
          return true;
        case umbra::detail::MomFederationFomModuleDataReportStatus::federation_does_not_exist:
        case umbra::detail::MomFederationFomModuleDataReportStatus::requesting_federate_not_member:
          throw FederateNotExecutionMember(
              L"HLArequestFOMmoduleData requires a joined requesting federate.");
        case umbra::detail::MomFederationFomModuleDataReportStatus::invalid_module_index:
          throw RTIinternalError(
              L"HLArequestFOMmoduleData specified an unavailable federation FOM module index.");
        case umbra::detail::MomFederationFomModuleDataReportStatus::inconsistent_catalog:
          throw RTIinternalError(
              L"The embedded federation cannot reconstruct federation HLAreportFOMmoduleData.");
      }
    }

    if (!parameterValues.empty()) {
      for (auto const& [parameterHandle, parameterValue] : parameterValues) {
        static_cast<void>(parameterValue);
        auto const suppliedParameterHandleValue = parameterHandleValue(parameterHandle);
        if (!suppliedParameterHandleValue ||
            !registry.parameterNameFor(
                *federationName,
                *interactionClassName,
                *suppliedParameterHandleValue)) {
          throw InteractionParameterNotDefined(
              L"HLArequestMIMdata received an invalid parameter handle.");
        }
        throw InteractionParameterNotDefined(
            L"HLArequestMIMdata received an unexpected parameter.");
      }
    }
    auto report = registry.planMomFederationMimDataReport(
        *federationName,
        *producingFederateId);
    switch (report.status) {
      case umbra::detail::MomFederationMimDataReportStatus::applied:
        momFederationMimDataReport = std::move(report);
        return true;
      case umbra::detail::MomFederationMimDataReportStatus::federation_does_not_exist:
      case umbra::detail::MomFederationMimDataReportStatus::requesting_federate_not_member:
        throw FederateNotExecutionMember(
            L"HLArequestMIMdata requires a joined requesting federate.");
      case umbra::detail::MomFederationMimDataReportStatus::inconsistent_catalog:
        throw RTIinternalError(
            L"The embedded federation cannot reconstruct federation HLAreportMIMdata.");
    }
    throw RTIinternalError(
        L"The embedded federation rejected the federation MOM content request.");
  };

  if (handleMomFederationContentReportRequest()) {
    std::vector<InteractionParameterValue> sentParameters =
        ambassadorCopyInteractionParameterValues(parameterValues);
    VariableLengthData copiedTag(userSuppliedTag);
    ParameterHandleValueMap reportParameterValues;
    for (auto const& [parameterHandle, parameterValue] : sentParameters) {
      reportParameterValues.emplace(makeParameterHandle(parameterHandle), parameterValue);
    }
    auto const reportArguments = std::vector<umbra::detail::MomServiceArgument>{
        {umbra::detail::MomArgumentType::interaction_class_handle,
         L"Interaction class designator",
         umbra::detail::formatMomInteractionClassHandle(interactionClass)},
        {umbra::detail::MomArgumentType::parameter_handle_value_map,
         L"Constrained set of interaction parameter designator and value pairs",
         umbra::detail::formatMomParameterHandleValueMap(reportParameterValues)},
        {umbra::detail::MomArgumentType::table_5_user_supplied_tag,
         L"User-supplied tag",
         umbra::detail::formatMomUserSuppliedTag(copiedTag)},
        {umbra::detail::MomArgumentType::null_value,
         L"Optional timestamp",
         umbra::detail::formatMomNull()},
    };
    appendSuccessfulVoidServiceReportToFileIfSelected(
        L"SendInteraction",
        umbra::detail::MomServiceType::object_management,
        reportArguments,
        true);
    {
      std::scoped_lock lock(ambassadorFederationManagementMutex());
      auto const status = embeddedFederationRegistry()
          .recordSuccessfulInteractionSend(
              *federationName,
              *producingFederateId,
              *interactionClassHandle,
              umbra::detail::hla::utf8::mom::reliable,
              false);
      if (status != umbra::detail::FederationRegistryStatus::applied) {
        throw RTIinternalError(
            L"The embedded federation lost the Send Interaction membership before its accepted federation MOM request boundary.");
      }
    }
    if (momFederationFomModuleDataReport) {
      queueMomFederationFomModuleDataReport(
          *federationName,
          std::move(*momFederationFomModuleDataReport));
    } else if (momFederationMimDataReport) {
      queueMomFederationMimDataReport(
          *federationName,
          std::move(*momFederationMimDataReport));
    }
    return;
  }
  // FOM-module and declaration MOM report requests.

  auto const handleMomFomModuleDataReportRequest = [&]() {
    std::scoped_lock lock(mutex_, ambassadorFederationManagementMutex());
    requireConnectedForFederationManagement(lifecycle_);
    requireFederationServiceOperationAvailable(L"Send Interaction");
    if (lifecycle_.state() != umbra::detail::FederateLifecycleState::joined ||
        !joinedFederationName_ || !joinedFederateId_ ||
        *joinedFederationName_ != *federationName ||
        *joinedFederateId_ != *producingFederateId) {
      throw FederateNotExecutionMember(
          L"The RTI ambassador's federation membership changed during Send Interaction.");
    }
    auto& registry = embeddedFederationRegistry();
    if (!registry.memberById(*federationName, *producingFederateId)) {
      throw FederateNotExecutionMember(
          L"The embedded federation no longer records this RTI ambassador as a member.");
    }
    auto const interactionClassName = registry.interactionClassNameFor(
        *federationName,
        *interactionClassHandle);
    if (!interactionClassName) {
      return false;
    }
    auto const isFomModuleDataRequest = registry.interactionClassIsSameOrDescendantOf(
        *federationName,
        *interactionClassHandle,
        umbra::detail::hla::utf8::mom::request_fom_module_data_federate);
    if (!isFomModuleDataRequest) {
      throw RTIinternalError(
          L"The embedded federation cannot resolve the HLArequestFOMmoduleData MOM class hierarchy.");
    }
    if (!*isFomModuleDataRequest) {
      return false;
    }

    std::optional<std::uint64_t> reportedFederateId;
    std::optional<std::uint32_t> moduleIndex;
    for (auto const& [parameterHandle, parameterValue] : parameterValues) {
      auto const suppliedParameterHandleValue = parameterHandleValue(parameterHandle);
      if (!suppliedParameterHandleValue) {
        throw InteractionParameterNotDefined(
            L"HLArequestFOMmoduleData received an invalid parameter handle.");
      }
      auto const parameterName = registry.parameterNameFor(
          *federationName,
          *interactionClassName,
          *suppliedParameterHandleValue);
      if (!parameterName) {
        throw InteractionParameterNotDefined(
            L"HLArequestFOMmoduleData received an unknown parameter for this interaction class.");
      }
      if (*parameterName == umbra::detail::hla::utf8::mom::federate) {
        try {
          auto const decodedHandle =
              ::rti1516_2025::umbra_binding_detail::decodeFederateHandle(parameterValue);
          reportedFederateId =
              ::rti1516_2025::umbra_binding_detail::federateHandleValue(decodedHandle);
        } catch (Exception const&) {
          throw RTIinternalError(
              L"HLArequestFOMmoduleData received an invalid HLAfederateReference value.");
        }
        if (!reportedFederateId) {
          throw RTIinternalError(
              L"HLArequestFOMmoduleData received an invalid HLAfederateReference value.");
        }
        continue;
      }
      if (*parameterName == umbra::detail::hla::utf8::mom::fom_module_indicator) {
        rti1516_2025::HLAinteger32BE decodedIndicator;
        try {
          decodedIndicator.decode(parameterValue);
        } catch (Exception const&) {
          throw RTIinternalError(
              L"HLArequestFOMmoduleData received an invalid HLAindex value.");
        }
        auto const decodedValue = decodedIndicator.get();
        if (decodedValue < 0) {
          throw RTIinternalError(
              L"HLArequestFOMmoduleData requires a non-negative HLAindex value.");
        }
        moduleIndex = static_cast<std::uint32_t>(decodedValue);
      }
    }
    if (!reportedFederateId || !moduleIndex) {
      throw InteractionParameterNotDefined(
          L"HLArequestFOMmoduleData requires HLAfederate and HLAFOMmoduleIndicator parameters.");
    }

    auto report = registry.planMomFomModuleDataReport(
        *federationName,
        *producingFederateId,
        *reportedFederateId,
        *moduleIndex);
    switch (report.status) {
      case umbra::detail::MomFomModuleDataReportStatus::applied:
        momFomModuleDataReport = std::move(report);
        return true;
      case umbra::detail::MomFomModuleDataReportStatus::federation_does_not_exist:
      case umbra::detail::MomFomModuleDataReportStatus::requesting_federate_not_member:
      case umbra::detail::MomFomModuleDataReportStatus::reported_federate_not_member:
        throw FederateNotExecutionMember(
            L"HLArequestFOMmoduleData requires joined requesting and reported federates.");
      case umbra::detail::MomFomModuleDataReportStatus::invalid_module_index:
        throw RTIinternalError(
            L"HLArequestFOMmoduleData specified an unavailable FOM module index.");
      case umbra::detail::MomFomModuleDataReportStatus::inconsistent_catalog:
        throw RTIinternalError(
            L"The embedded federation cannot reconstruct HLAreportFOMmoduleData.");
    }
    throw RTIinternalError(
        L"The embedded federation rejected the HLArequestFOMmoduleData interaction.");
  };

  if (handleMomFomModuleDataReportRequest()) {
    std::vector<InteractionParameterValue> sentParameters =
        ambassadorCopyInteractionParameterValues(parameterValues);
    VariableLengthData copiedTag(userSuppliedTag);
    ParameterHandleValueMap reportParameterValues;
    for (auto const& [parameterHandle, parameterValue] : sentParameters) {
      reportParameterValues.emplace(makeParameterHandle(parameterHandle), parameterValue);
    }
    auto const reportArguments = std::vector<umbra::detail::MomServiceArgument>{
        {umbra::detail::MomArgumentType::interaction_class_handle,
         L"Interaction class designator",
         umbra::detail::formatMomInteractionClassHandle(interactionClass)},
        {umbra::detail::MomArgumentType::parameter_handle_value_map,
         L"Constrained set of interaction parameter designator and value pairs",
         umbra::detail::formatMomParameterHandleValueMap(reportParameterValues)},
        {umbra::detail::MomArgumentType::table_5_user_supplied_tag,
         L"User-supplied tag",
         umbra::detail::formatMomUserSuppliedTag(copiedTag)},
        {umbra::detail::MomArgumentType::null_value,
         L"Optional timestamp",
         umbra::detail::formatMomNull()},
    };
    appendSuccessfulVoidServiceReportToFileIfSelected(
        L"SendInteraction",
        umbra::detail::MomServiceType::object_management,
        reportArguments,
        true);
    {
      std::scoped_lock lock(ambassadorFederationManagementMutex());
      auto const status = embeddedFederationRegistry()
          .recordSuccessfulInteractionSend(
              *federationName,
              *producingFederateId,
              *interactionClassHandle,
              umbra::detail::hla::utf8::mom::reliable,
              false);
      if (status != umbra::detail::FederationRegistryStatus::applied) {
        throw RTIinternalError(
            L"The embedded federation lost the Send Interaction membership before its accepted HLArequestFOMmoduleData boundary.");
      }
    }
    queueMomFomModuleDataReport(
        *federationName,
        std::move(*momFomModuleDataReport));
    return;
  }

  auto const handleMomPublicationsReportRequest = [&]() {
    std::scoped_lock lock(mutex_, ambassadorFederationManagementMutex());
    requireConnectedForFederationManagement(lifecycle_);
    requireFederationServiceOperationAvailable(L"Send Interaction");
    if (lifecycle_.state() != umbra::detail::FederateLifecycleState::joined ||
        !joinedFederationName_ || !joinedFederateId_ ||
        *joinedFederationName_ != *federationName ||
        *joinedFederateId_ != *producingFederateId) {
      throw FederateNotExecutionMember(
          L"The RTI ambassador's federation membership changed during Send Interaction.");
    }
    auto& registry = embeddedFederationRegistry();
    if (!registry.memberById(*federationName, *producingFederateId)) {
      throw FederateNotExecutionMember(
          L"The embedded federation no longer records this RTI ambassador as a member.");
    }
    auto const interactionClassName = registry.interactionClassNameFor(
        *federationName,
        *interactionClassHandle);
    if (!interactionClassName) {
      return false;
    }
    auto const isPublicationsRequest = registry.interactionClassIsSameOrDescendantOf(
        *federationName,
        *interactionClassHandle,
        umbra::detail::hla::utf8::mom::request_publications);
    if (!isPublicationsRequest) {
      throw RTIinternalError(
          L"The embedded federation cannot resolve the HLArequestPublications MOM class hierarchy.");
    }
    if (!*isPublicationsRequest) {
      return false;
    }

    std::optional<std::uint64_t> reportedFederateId;
    for (auto const& [parameterHandle, parameterValue] : parameterValues) {
      auto const suppliedParameterHandleValue = parameterHandleValue(parameterHandle);
      if (!suppliedParameterHandleValue) {
        throw InteractionParameterNotDefined(
            L"HLArequestPublications received an invalid parameter handle.");
      }
      auto const parameterName = registry.parameterNameFor(
          *federationName,
          *interactionClassName,
          *suppliedParameterHandleValue);
      if (!parameterName) {
        throw InteractionParameterNotDefined(
            L"HLArequestPublications received an unknown parameter for this interaction class.");
      }
      if (*parameterName != umbra::detail::hla::utf8::mom::federate) {
        continue;
      }
      try {
        auto const decodedHandle =
            ::rti1516_2025::umbra_binding_detail::decodeFederateHandle(parameterValue);
        reportedFederateId =
            ::rti1516_2025::umbra_binding_detail::federateHandleValue(decodedHandle);
      } catch (Exception const&) {
        throw RTIinternalError(
            L"HLArequestPublications received an invalid HLAfederateReference value.");
      }
      if (!reportedFederateId) {
        throw RTIinternalError(
            L"HLArequestPublications received an invalid HLAfederateReference value.");
      }
    }
    if (!reportedFederateId) {
      throw InteractionParameterNotDefined(
          L"HLArequestPublications requires the HLAfederate parameter.");
    }

    auto report = registry.planMomPublicationsReport(
        *federationName,
        *reportedFederateId);
    switch (report.status) {
      case umbra::detail::MomPublicationsReportStatus::applied:
        momPublicationsReport = std::move(report);
        return true;
      case umbra::detail::MomPublicationsReportStatus::federation_does_not_exist:
      case umbra::detail::MomPublicationsReportStatus::requesting_federate_not_member:
        throw FederateNotExecutionMember(
            L"HLArequestPublications requires a joined target federate.");
      case umbra::detail::MomPublicationsReportStatus::inconsistent_catalog:
        throw RTIinternalError(
            L"The embedded federation cannot reconstruct the HLArequestPublications reports.");
    }
    throw RTIinternalError(
        L"The embedded federation rejected the HLArequestPublications interaction.");
  };

  if (handleMomPublicationsReportRequest()) {
    std::vector<InteractionParameterValue> sentParameters =
        ambassadorCopyInteractionParameterValues(parameterValues);
    VariableLengthData copiedTag(userSuppliedTag);
    ParameterHandleValueMap reportParameterValues;
    for (auto const& [parameterHandle, parameterValue] : sentParameters) {
      reportParameterValues.emplace(makeParameterHandle(parameterHandle), parameterValue);
    }
    auto const reportArguments = std::vector<umbra::detail::MomServiceArgument>{
        {umbra::detail::MomArgumentType::interaction_class_handle,
         L"Interaction class designator",
         umbra::detail::formatMomInteractionClassHandle(interactionClass)},
        {umbra::detail::MomArgumentType::parameter_handle_value_map,
         L"Constrained set of interaction parameter designator and value pairs",
         umbra::detail::formatMomParameterHandleValueMap(reportParameterValues)},
        {umbra::detail::MomArgumentType::table_5_user_supplied_tag,
         L"User-supplied tag",
         umbra::detail::formatMomUserSuppliedTag(copiedTag)},
        {umbra::detail::MomArgumentType::null_value,
         L"Optional timestamp",
         umbra::detail::formatMomNull()},
    };
    appendSuccessfulVoidServiceReportToFileIfSelected(
        L"SendInteraction",
        umbra::detail::MomServiceType::object_management,
        reportArguments,
        true);
    {
      std::scoped_lock lock(ambassadorFederationManagementMutex());
      auto const status = embeddedFederationRegistry()
          .recordSuccessfulInteractionSend(
              *federationName,
              *producingFederateId,
              *interactionClassHandle,
              umbra::detail::hla::utf8::mom::reliable,
              false);
      if (status != umbra::detail::FederationRegistryStatus::applied) {
        throw RTIinternalError(
            L"The embedded federation lost the Send Interaction membership before its accepted HLArequestPublications boundary.");
      }
    }
    auto const report = *momPublicationsReport;
    queueMomObjectClassPublicationReports(*federationName, report);
    queueMomInteractionPublicationReport(*federationName, report);
    queueMomDirectedInteractionPublicationReports(*federationName, report);
    return;
  }

  auto const handleMomSubscriptionsReportRequest = [&]() {
    std::scoped_lock lock(mutex_, ambassadorFederationManagementMutex());
    requireConnectedForFederationManagement(lifecycle_);
    requireFederationServiceOperationAvailable(L"Send Interaction");
    if (lifecycle_.state() != umbra::detail::FederateLifecycleState::joined ||
        !joinedFederationName_ || !joinedFederateId_ ||
        *joinedFederationName_ != *federationName ||
        *joinedFederateId_ != *producingFederateId) {
      throw FederateNotExecutionMember(
          L"The RTI ambassador's federation membership changed during Send Interaction.");
    }
    auto& registry = embeddedFederationRegistry();
    if (!registry.memberById(*federationName, *producingFederateId)) {
      throw FederateNotExecutionMember(
          L"The embedded federation no longer records this RTI ambassador as a member.");
    }
    auto const interactionClassName = registry.interactionClassNameFor(
        *federationName,
        *interactionClassHandle);
    if (!interactionClassName) {
      return false;
    }
    auto const isSubscriptionsRequest = registry.interactionClassIsSameOrDescendantOf(
        *federationName,
        *interactionClassHandle,
        umbra::detail::hla::utf8::mom::request_subscriptions);
    if (!isSubscriptionsRequest) {
      throw RTIinternalError(
          L"The embedded federation cannot resolve the HLArequestSubscriptions MOM class hierarchy.");
    }
    if (!*isSubscriptionsRequest) {
      return false;
    }

    std::optional<std::uint64_t> reportedFederateId;
    for (auto const& [parameterHandle, parameterValue] : parameterValues) {
      auto const suppliedParameterHandleValue = parameterHandleValue(parameterHandle);
      if (!suppliedParameterHandleValue) {
        throw InteractionParameterNotDefined(
            L"HLArequestSubscriptions received an invalid parameter handle.");
      }
      auto const parameterName = registry.parameterNameFor(
          *federationName,
          *interactionClassName,
          *suppliedParameterHandleValue);
      if (!parameterName) {
        throw InteractionParameterNotDefined(
            L"HLArequestSubscriptions received an unknown parameter for this interaction class.");
      }
      if (*parameterName != umbra::detail::hla::utf8::mom::federate) {
        continue;
      }
      try {
        auto const decodedHandle =
            ::rti1516_2025::umbra_binding_detail::decodeFederateHandle(parameterValue);
        reportedFederateId =
            ::rti1516_2025::umbra_binding_detail::federateHandleValue(decodedHandle);
      } catch (Exception const&) {
        throw RTIinternalError(
            L"HLArequestSubscriptions received an invalid HLAfederateReference value.");
      }
      if (!reportedFederateId) {
        throw RTIinternalError(
            L"HLArequestSubscriptions received an invalid HLAfederateReference value.");
      }
    }
    if (!reportedFederateId) {
      throw InteractionParameterNotDefined(
          L"HLArequestSubscriptions requires the HLAfederate parameter.");
    }

    auto report = registry.planMomSubscriptionsReport(
        *federationName,
        *reportedFederateId);
    switch (report.status) {
      case umbra::detail::MomSubscriptionsReportStatus::applied:
        momSubscriptionsReport = std::move(report);
        return true;
      case umbra::detail::MomSubscriptionsReportStatus::federation_does_not_exist:
      case umbra::detail::MomSubscriptionsReportStatus::requesting_federate_not_member:
        throw FederateNotExecutionMember(
            L"HLArequestSubscriptions requires a joined target federate.");
      case umbra::detail::MomSubscriptionsReportStatus::inconsistent_catalog:
        throw RTIinternalError(
            L"The embedded federation cannot reconstruct the HLArequestSubscriptions reports.");
    }
    throw RTIinternalError(
        L"The embedded federation rejected the HLArequestSubscriptions interaction.");
  };

  if (handleMomSubscriptionsReportRequest()) {
    std::vector<InteractionParameterValue> sentParameters =
        ambassadorCopyInteractionParameterValues(parameterValues);
    VariableLengthData copiedTag(userSuppliedTag);
    ParameterHandleValueMap reportParameterValues;
    for (auto const& [parameterHandle, parameterValue] : sentParameters) {
      reportParameterValues.emplace(makeParameterHandle(parameterHandle), parameterValue);
    }
    auto const reportArguments = std::vector<umbra::detail::MomServiceArgument>{
        {umbra::detail::MomArgumentType::interaction_class_handle,
         L"Interaction class designator",
         umbra::detail::formatMomInteractionClassHandle(interactionClass)},
        {umbra::detail::MomArgumentType::parameter_handle_value_map,
         L"Constrained set of interaction parameter designator and value pairs",
         umbra::detail::formatMomParameterHandleValueMap(reportParameterValues)},
        {umbra::detail::MomArgumentType::table_5_user_supplied_tag,
         L"User-supplied tag",
         umbra::detail::formatMomUserSuppliedTag(copiedTag)},
        {umbra::detail::MomArgumentType::null_value,
         L"Optional timestamp",
         umbra::detail::formatMomNull()},
    };
    appendSuccessfulVoidServiceReportToFileIfSelected(
        L"SendInteraction",
        umbra::detail::MomServiceType::object_management,
        reportArguments,
        true);
    {
      std::scoped_lock lock(ambassadorFederationManagementMutex());
      auto const status = embeddedFederationRegistry()
          .recordSuccessfulInteractionSend(
              *federationName,
              *producingFederateId,
              *interactionClassHandle,
              umbra::detail::hla::utf8::mom::reliable,
              false);
      if (status != umbra::detail::FederationRegistryStatus::applied) {
        throw RTIinternalError(
            L"The embedded federation lost the Send Interaction membership before its accepted HLArequestSubscriptions boundary.");
      }
    }
    auto const report = *momSubscriptionsReport;
    queueMomObjectClassSubscriptionReports(*federationName, report);
    queueMomInteractionSubscriptionReport(*federationName, report);
    queueMomDirectedInteractionSubscriptionReports(*federationName, report);
    return;
  }
  // Reflection, update, and interaction MOM report requests.

  auto const handleMomReflectionsReceivedReportRequest = [&]() {
    std::scoped_lock lock(mutex_, ambassadorFederationManagementMutex());
    requireConnectedForFederationManagement(lifecycle_);
    requireFederationServiceOperationAvailable(L"Send Interaction");
    if (lifecycle_.state() != umbra::detail::FederateLifecycleState::joined ||
        !joinedFederationName_ || !joinedFederateId_ ||
        *joinedFederationName_ != *federationName ||
        *joinedFederateId_ != *producingFederateId) {
      throw FederateNotExecutionMember(
          L"The RTI ambassador's federation membership changed during Send Interaction.");
    }
    auto& registry = embeddedFederationRegistry();
    if (!registry.memberById(*federationName, *producingFederateId)) {
      throw FederateNotExecutionMember(
          L"The embedded federation no longer records this RTI ambassador as a member.");
    }
    auto const interactionClassName = registry.interactionClassNameFor(
        *federationName,
        *interactionClassHandle);
    if (!interactionClassName) {
      return false;
    }
    auto const isReflectionsReceivedRequest =
        registry.interactionClassIsSameOrDescendantOf(
            *federationName,
            *interactionClassHandle,
            umbra::detail::hla::utf8::mom::request_reflections_received);
    if (!isReflectionsReceivedRequest) {
      throw RTIinternalError(
          L"The embedded federation cannot resolve the HLArequestReflectionsReceived MOM class hierarchy.");
    }
    if (!*isReflectionsReceivedRequest) {
      return false;
    }

    std::optional<std::uint64_t> reportedFederateId;
    for (auto const& [parameterHandle, parameterValue] : parameterValues) {
      auto const suppliedParameterHandleValue = parameterHandleValue(parameterHandle);
      if (!suppliedParameterHandleValue) {
        throw InteractionParameterNotDefined(
            L"HLArequestReflectionsReceived received an invalid parameter handle.");
      }
      auto const parameterName = registry.parameterNameFor(
          *federationName,
          *interactionClassName,
          *suppliedParameterHandleValue);
      if (!parameterName) {
        throw InteractionParameterNotDefined(
            L"HLArequestReflectionsReceived received an unknown parameter for this interaction class.");
      }
      if (*parameterName != umbra::detail::hla::utf8::mom::federate) {
        continue;
      }
      try {
        auto const decodedHandle =
            ::rti1516_2025::umbra_binding_detail::decodeFederateHandle(parameterValue);
        reportedFederateId =
            ::rti1516_2025::umbra_binding_detail::federateHandleValue(decodedHandle);
      } catch (Exception const&) {
        throw RTIinternalError(
            L"HLArequestReflectionsReceived received an invalid HLAfederateReference value.");
      }
      if (!reportedFederateId) {
        throw RTIinternalError(
            L"HLArequestReflectionsReceived received an invalid HLAfederateReference value.");
      }
    }
    if (!reportedFederateId) {
      throw InteractionParameterNotDefined(
          L"HLArequestReflectionsReceived requires the HLAfederate parameter.");
    }
    auto report = registry.planMomReflectionsReceivedReport(
        *federationName,
        *producingFederateId,
        *reportedFederateId);
    switch (report.status) {
      case umbra::detail::MomObjectInstanceCountsReportStatus::applied:
        momReflectionsReceivedReport = std::move(report);
        return true;
      case umbra::detail::MomObjectInstanceCountsReportStatus::federation_does_not_exist:
      case umbra::detail::MomObjectInstanceCountsReportStatus::requesting_federate_not_member:
      case umbra::detail::MomObjectInstanceCountsReportStatus::reported_federate_not_member:
        throw FederateNotExecutionMember(
            L"HLArequestReflectionsReceived requires joined requesting and reported federates.");
      case umbra::detail::MomObjectInstanceCountsReportStatus::inconsistent_catalog:
        throw RTIinternalError(
            L"The embedded federation cannot reconstruct the requested HLAreportReflectionsReceived interaction.");
    }
    throw RTIinternalError(
        L"The embedded federation rejected the HLArequestReflectionsReceived interaction.");
  };

  if (handleMomReflectionsReceivedReportRequest()) {
    std::vector<InteractionParameterValue> sentParameters =
        ambassadorCopyInteractionParameterValues(parameterValues);
    VariableLengthData copiedTag(userSuppliedTag);
    ParameterHandleValueMap reportParameterValues;
    for (auto const& [parameterHandle, parameterValue] : sentParameters) {
      reportParameterValues.emplace(makeParameterHandle(parameterHandle), parameterValue);
    }
    auto const reportArguments = std::vector<umbra::detail::MomServiceArgument>{
        {umbra::detail::MomArgumentType::interaction_class_handle,
         L"Interaction class designator",
         umbra::detail::formatMomInteractionClassHandle(interactionClass)},
        {umbra::detail::MomArgumentType::parameter_handle_value_map,
         L"Constrained set of interaction parameter designator and value pairs",
         umbra::detail::formatMomParameterHandleValueMap(reportParameterValues)},
        {umbra::detail::MomArgumentType::table_5_user_supplied_tag,
         L"User-supplied tag",
         umbra::detail::formatMomUserSuppliedTag(copiedTag)},
        {umbra::detail::MomArgumentType::null_value,
         L"Optional timestamp",
         umbra::detail::formatMomNull()},
    };
    appendSuccessfulVoidServiceReportToFileIfSelected(
        L"SendInteraction",
        umbra::detail::MomServiceType::object_management,
        reportArguments,
        true);
    {
      std::scoped_lock lock(ambassadorFederationManagementMutex());
      auto const status = embeddedFederationRegistry()
          .recordSuccessfulInteractionSend(
              *federationName,
              *producingFederateId,
              *interactionClassHandle,
              umbra::detail::hla::utf8::mom::reliable,
              false);
      if (status != umbra::detail::FederationRegistryStatus::applied) {
        throw RTIinternalError(
            L"The embedded federation lost the Send Interaction membership before its accepted MOM request boundary.");
      }
    }
    queueMomReflectionsReceivedReport(
        *federationName,
        momReflectionsReceivedReport->reportedFederateId,
        std::move(*momReflectionsReceivedReport));
    return;
  }

  auto const handleMomUpdatesSentReportRequest = [&]() {
    std::scoped_lock lock(mutex_, ambassadorFederationManagementMutex());
    requireConnectedForFederationManagement(lifecycle_);
    requireFederationServiceOperationAvailable(L"Send Interaction");
    if (lifecycle_.state() != umbra::detail::FederateLifecycleState::joined ||
        !joinedFederationName_ || !joinedFederateId_ ||
        *joinedFederationName_ != *federationName ||
        *joinedFederateId_ != *producingFederateId) {
      throw FederateNotExecutionMember(
          L"The RTI ambassador's federation membership changed during Send Interaction.");
    }
    auto& registry = embeddedFederationRegistry();
    if (!registry.memberById(*federationName, *producingFederateId)) {
      throw FederateNotExecutionMember(
          L"The embedded federation no longer records this RTI ambassador as a member.");
    }
    auto const interactionClassName = registry.interactionClassNameFor(
        *federationName,
        *interactionClassHandle);
    if (!interactionClassName) {
      return false;
    }
    auto const isUpdatesSentRequest = registry.interactionClassIsSameOrDescendantOf(
        *federationName,
        *interactionClassHandle,
        umbra::detail::hla::utf8::mom::request_updates_sent);
    if (!isUpdatesSentRequest) {
      throw RTIinternalError(
          L"The embedded federation cannot resolve the HLArequestUpdatesSent MOM class hierarchy.");
    }
    if (!*isUpdatesSentRequest) {
      return false;
    }

    std::optional<std::uint64_t> reportedFederateId;
    for (auto const& [parameterHandle, parameterValue] : parameterValues) {
      auto const suppliedParameterHandleValue = parameterHandleValue(parameterHandle);
      if (!suppliedParameterHandleValue) {
        throw InteractionParameterNotDefined(
            L"HLArequestUpdatesSent received an invalid parameter handle.");
      }
      auto const parameterName = registry.parameterNameFor(
          *federationName,
          *interactionClassName,
          *suppliedParameterHandleValue);
      if (!parameterName) {
        throw InteractionParameterNotDefined(
            L"HLArequestUpdatesSent received an unknown parameter for this interaction class.");
      }
      if (*parameterName != umbra::detail::hla::utf8::mom::federate) {
        continue;
      }
      try {
        auto const decodedHandle =
            ::rti1516_2025::umbra_binding_detail::decodeFederateHandle(parameterValue);
        reportedFederateId =
            ::rti1516_2025::umbra_binding_detail::federateHandleValue(decodedHandle);
      } catch (Exception const&) {
        throw RTIinternalError(
            L"HLArequestUpdatesSent received an invalid HLAfederateReference value.");
      }
      if (!reportedFederateId) {
        throw RTIinternalError(
            L"HLArequestUpdatesSent received an invalid HLAfederateReference value.");
      }
    }
    if (!reportedFederateId) {
      throw InteractionParameterNotDefined(
          L"HLArequestUpdatesSent requires the HLAfederate parameter.");
    }
    auto report = registry.planMomUpdatesSentReport(
        *federationName,
        *producingFederateId,
        *reportedFederateId);
    switch (report.status) {
      case umbra::detail::MomObjectInstanceCountsReportStatus::applied:
        momUpdatesSentReport = std::move(report);
        return true;
      case umbra::detail::MomObjectInstanceCountsReportStatus::federation_does_not_exist:
      case umbra::detail::MomObjectInstanceCountsReportStatus::requesting_federate_not_member:
      case umbra::detail::MomObjectInstanceCountsReportStatus::reported_federate_not_member:
        throw FederateNotExecutionMember(
            L"HLArequestUpdatesSent requires joined requesting and reported federates.");
      case umbra::detail::MomObjectInstanceCountsReportStatus::inconsistent_catalog:
        throw RTIinternalError(
            L"The embedded federation cannot reconstruct the requested HLAreportUpdatesSent interaction.");
    }
    throw RTIinternalError(
        L"The embedded federation rejected the HLArequestUpdatesSent interaction.");
  };

  if (handleMomUpdatesSentReportRequest()) {
    std::vector<InteractionParameterValue> sentParameters =
        ambassadorCopyInteractionParameterValues(parameterValues);
    VariableLengthData copiedTag(userSuppliedTag);
    ParameterHandleValueMap reportParameterValues;
    for (auto const& [parameterHandle, parameterValue] : sentParameters) {
      reportParameterValues.emplace(makeParameterHandle(parameterHandle), parameterValue);
    }
    auto const reportArguments = std::vector<umbra::detail::MomServiceArgument>{
        {umbra::detail::MomArgumentType::interaction_class_handle,
         L"Interaction class designator",
         umbra::detail::formatMomInteractionClassHandle(interactionClass)},
        {umbra::detail::MomArgumentType::parameter_handle_value_map,
         L"Constrained set of interaction parameter designator and value pairs",
         umbra::detail::formatMomParameterHandleValueMap(reportParameterValues)},
        {umbra::detail::MomArgumentType::table_5_user_supplied_tag,
         L"User-supplied tag",
         umbra::detail::formatMomUserSuppliedTag(copiedTag)},
        {umbra::detail::MomArgumentType::null_value,
         L"Optional timestamp",
         umbra::detail::formatMomNull()},
    };
    appendSuccessfulVoidServiceReportToFileIfSelected(
        L"SendInteraction",
        umbra::detail::MomServiceType::object_management,
        reportArguments,
        true);
    {
      std::scoped_lock lock(ambassadorFederationManagementMutex());
      auto const status = embeddedFederationRegistry()
          .recordSuccessfulInteractionSend(
              *federationName,
              *producingFederateId,
              *interactionClassHandle,
              umbra::detail::hla::utf8::mom::reliable,
              false);
      if (status != umbra::detail::FederationRegistryStatus::applied) {
        throw RTIinternalError(
            L"The embedded federation lost the Send Interaction membership before its accepted MOM request boundary.");
      }
    }
    queueMomUpdatesSentReport(
        *federationName,
        momUpdatesSentReport->reportedFederateId,
        std::move(*momUpdatesSentReport));
    return;
  }

  auto const handleMomInteractionsSentReportRequest = [&]() {
    std::scoped_lock lock(mutex_, ambassadorFederationManagementMutex());
    requireConnectedForFederationManagement(lifecycle_);
    requireFederationServiceOperationAvailable(L"Send Interaction");
    if (lifecycle_.state() != umbra::detail::FederateLifecycleState::joined ||
        !joinedFederationName_ || !joinedFederateId_ ||
        *joinedFederationName_ != *federationName ||
        *joinedFederateId_ != *producingFederateId) {
      throw FederateNotExecutionMember(
          L"The RTI ambassador's federation membership changed during Send Interaction.");
    }
    auto& registry = embeddedFederationRegistry();
    if (!registry.memberById(*federationName, *producingFederateId)) {
      throw FederateNotExecutionMember(
          L"The embedded federation no longer records this RTI ambassador as a member.");
    }
    auto const interactionClassName = registry.interactionClassNameFor(
        *federationName,
        *interactionClassHandle);
    if (!interactionClassName) {
      return false;
    }
    auto const isInteractionsSentRequest = registry.interactionClassIsSameOrDescendantOf(
        *federationName,
        *interactionClassHandle,
        umbra::detail::hla::utf8::mom::request_interactions_sent);
    if (!isInteractionsSentRequest) {
      throw RTIinternalError(
          L"The embedded federation cannot resolve the HLArequestInteractionsSent MOM class hierarchy.");
    }
    if (!*isInteractionsSentRequest) {
      return false;
    }

    std::optional<std::uint64_t> reportedFederateId;
    for (auto const& [parameterHandle, parameterValue] : parameterValues) {
      auto const suppliedParameterHandleValue = parameterHandleValue(parameterHandle);
      if (!suppliedParameterHandleValue) {
        throw InteractionParameterNotDefined(
            L"HLArequestInteractionsSent received an invalid parameter handle.");
      }
      auto const parameterName = registry.parameterNameFor(
          *federationName,
          *interactionClassName,
          *suppliedParameterHandleValue);
      if (!parameterName) {
        throw InteractionParameterNotDefined(
            L"HLArequestInteractionsSent received an unknown parameter for this interaction class.");
      }
      if (*parameterName != umbra::detail::hla::utf8::mom::federate) {
        continue;
      }
      try {
        auto const decodedHandle =
            ::rti1516_2025::umbra_binding_detail::decodeFederateHandle(parameterValue);
        reportedFederateId =
            ::rti1516_2025::umbra_binding_detail::federateHandleValue(decodedHandle);
      } catch (Exception const&) {
        throw RTIinternalError(
            L"HLArequestInteractionsSent received an invalid HLAfederateReference value.");
      }
      if (!reportedFederateId) {
        throw RTIinternalError(
            L"HLArequestInteractionsSent received an invalid HLAfederateReference value.");
      }
    }
    if (!reportedFederateId) {
      throw InteractionParameterNotDefined(
          L"HLArequestInteractionsSent requires the HLAfederate parameter.");
    }
    auto report = registry.planMomInteractionsSentReport(
        *federationName,
        *producingFederateId,
        *reportedFederateId);
    switch (report.status) {
      case umbra::detail::MomObjectInstanceCountsReportStatus::applied:
        momInteractionsSentReport = std::move(report);
        return true;
      case umbra::detail::MomObjectInstanceCountsReportStatus::federation_does_not_exist:
      case umbra::detail::MomObjectInstanceCountsReportStatus::requesting_federate_not_member:
      case umbra::detail::MomObjectInstanceCountsReportStatus::reported_federate_not_member:
        throw FederateNotExecutionMember(
            L"HLArequestInteractionsSent requires joined requesting and reported federates.");
      case umbra::detail::MomObjectInstanceCountsReportStatus::inconsistent_catalog:
        throw RTIinternalError(
            L"The embedded federation cannot reconstruct the requested HLAreportInteractionsSent interaction.");
    }
    throw RTIinternalError(
        L"The embedded federation rejected the HLArequestInteractionsSent interaction.");
  };

  if (handleMomInteractionsSentReportRequest()) {
    std::vector<InteractionParameterValue> sentParameters =
        ambassadorCopyInteractionParameterValues(parameterValues);
    VariableLengthData copiedTag(userSuppliedTag);
    ParameterHandleValueMap reportParameterValues;
    for (auto const& [parameterHandle, parameterValue] : sentParameters) {
      reportParameterValues.emplace(makeParameterHandle(parameterHandle), parameterValue);
    }
    auto const reportArguments = std::vector<umbra::detail::MomServiceArgument>{
        {umbra::detail::MomArgumentType::interaction_class_handle,
         L"Interaction class designator",
         umbra::detail::formatMomInteractionClassHandle(interactionClass)},
        {umbra::detail::MomArgumentType::parameter_handle_value_map,
         L"Constrained set of interaction parameter designator and value pairs",
         umbra::detail::formatMomParameterHandleValueMap(reportParameterValues)},
        {umbra::detail::MomArgumentType::table_5_user_supplied_tag,
         L"User-supplied tag",
         umbra::detail::formatMomUserSuppliedTag(copiedTag)},
        {umbra::detail::MomArgumentType::null_value,
         L"Optional timestamp",
         umbra::detail::formatMomNull()},
    };
    appendSuccessfulVoidServiceReportToFileIfSelected(
        L"SendInteraction",
        umbra::detail::MomServiceType::object_management,
        reportArguments,
        true);
    {
      std::scoped_lock lock(ambassadorFederationManagementMutex());
      auto const status = embeddedFederationRegistry()
          .recordSuccessfulInteractionSend(
              *federationName,
              *producingFederateId,
              *interactionClassHandle,
              umbra::detail::hla::utf8::mom::reliable,
              false);
      if (status != umbra::detail::FederationRegistryStatus::applied) {
        throw RTIinternalError(
            L"The embedded federation lost the Send Interaction membership before its accepted MOM request boundary.");
      }
    }
    queueMomInteractionsSentReport(
        *federationName,
        momInteractionsSentReport->reportedFederateId,
        std::move(*momInteractionsSentReport));
    return;
  }

  auto const handleMomDirectedInteractionsSentReportRequest = [&]() {
    std::scoped_lock lock(mutex_, ambassadorFederationManagementMutex());
    requireConnectedForFederationManagement(lifecycle_);
    requireFederationServiceOperationAvailable(L"Send Interaction");
    if (lifecycle_.state() != umbra::detail::FederateLifecycleState::joined ||
        !joinedFederationName_ || !joinedFederateId_ ||
        *joinedFederationName_ != *federationName ||
        *joinedFederateId_ != *producingFederateId) {
      throw FederateNotExecutionMember(
          L"The RTI ambassador's federation membership changed during Send Interaction.");
    }
    auto& registry = embeddedFederationRegistry();
    if (!registry.memberById(*federationName, *producingFederateId)) {
      throw FederateNotExecutionMember(
          L"The embedded federation no longer records this RTI ambassador as a member.");
    }
    auto const interactionClassName = registry.interactionClassNameFor(
        *federationName,
        *interactionClassHandle);
    if (!interactionClassName) {
      return false;
    }
    auto const isDirectedInteractionsSentRequest = registry.interactionClassIsSameOrDescendantOf(
        *federationName,
        *interactionClassHandle,
        umbra::detail::hla::utf8::mom::request_directed_interactions_sent);
    if (!isDirectedInteractionsSentRequest) {
      throw RTIinternalError(
          L"The embedded federation cannot resolve the HLArequestDirectedInteractionsSent MOM class hierarchy.");
    }
    if (!*isDirectedInteractionsSentRequest) {
      return false;
    }

    std::optional<std::uint64_t> reportedFederateId;
    for (auto const& [parameterHandle, parameterValue] : parameterValues) {
      auto const suppliedParameterHandleValue = parameterHandleValue(parameterHandle);
      if (!suppliedParameterHandleValue) {
        throw InteractionParameterNotDefined(
            L"HLArequestDirectedInteractionsSent received an invalid parameter handle.");
      }
      auto const parameterName = registry.parameterNameFor(
          *federationName,
          *interactionClassName,
          *suppliedParameterHandleValue);
      if (!parameterName) {
        throw InteractionParameterNotDefined(
            L"HLArequestDirectedInteractionsSent received an unknown parameter for this interaction class.");
      }
      if (*parameterName != umbra::detail::hla::utf8::mom::federate) {
        continue;
      }
      try {
        auto const decodedHandle =
            ::rti1516_2025::umbra_binding_detail::decodeFederateHandle(parameterValue);
        reportedFederateId =
            ::rti1516_2025::umbra_binding_detail::federateHandleValue(decodedHandle);
      } catch (Exception const&) {
        throw RTIinternalError(
            L"HLArequestDirectedInteractionsSent received an invalid HLAfederateReference value.");
      }
      if (!reportedFederateId) {
        throw RTIinternalError(
            L"HLArequestDirectedInteractionsSent received an invalid HLAfederateReference value.");
      }
    }
    if (!reportedFederateId) {
      throw InteractionParameterNotDefined(
          L"HLArequestDirectedInteractionsSent requires the HLAfederate parameter.");
    }
    auto report = registry.planMomDirectedInteractionsSentReport(
        *federationName,
        *producingFederateId,
        *reportedFederateId);
    switch (report.status) {
      case umbra::detail::MomObjectInstanceCountsReportStatus::applied:
        momDirectedInteractionsSentReport = std::move(report);
        return true;
      case umbra::detail::MomObjectInstanceCountsReportStatus::federation_does_not_exist:
      case umbra::detail::MomObjectInstanceCountsReportStatus::requesting_federate_not_member:
      case umbra::detail::MomObjectInstanceCountsReportStatus::reported_federate_not_member:
        throw FederateNotExecutionMember(
            L"HLArequestDirectedInteractionsSent requires joined requesting and reported federates.");
      case umbra::detail::MomObjectInstanceCountsReportStatus::inconsistent_catalog:
        throw RTIinternalError(
            L"The embedded federation cannot reconstruct the requested HLAreportDirectedInteractionsSent interaction.");
    }
    throw RTIinternalError(
        L"The embedded federation rejected the HLArequestDirectedInteractionsSent interaction.");
  };

  if (handleMomDirectedInteractionsSentReportRequest()) {
    std::vector<InteractionParameterValue> sentParameters =
        ambassadorCopyInteractionParameterValues(parameterValues);
    VariableLengthData copiedTag(userSuppliedTag);
    ParameterHandleValueMap reportParameterValues;
    for (auto const& [parameterHandle, parameterValue] : sentParameters) {
      reportParameterValues.emplace(makeParameterHandle(parameterHandle), parameterValue);
    }
    auto const reportArguments = std::vector<umbra::detail::MomServiceArgument>{
        {umbra::detail::MomArgumentType::interaction_class_handle,
         L"Interaction class designator",
         umbra::detail::formatMomInteractionClassHandle(interactionClass)},
        {umbra::detail::MomArgumentType::parameter_handle_value_map,
         L"Constrained set of interaction parameter designator and value pairs",
         umbra::detail::formatMomParameterHandleValueMap(reportParameterValues)},
        {umbra::detail::MomArgumentType::table_5_user_supplied_tag,
         L"User-supplied tag",
         umbra::detail::formatMomUserSuppliedTag(copiedTag)},
        {umbra::detail::MomArgumentType::null_value,
         L"Optional timestamp",
         umbra::detail::formatMomNull()},
    };
    appendSuccessfulVoidServiceReportToFileIfSelected(
        L"SendInteraction",
        umbra::detail::MomServiceType::object_management,
        reportArguments,
        true);
    {
      std::scoped_lock lock(ambassadorFederationManagementMutex());
      auto const status = embeddedFederationRegistry()
          .recordSuccessfulInteractionSend(
              *federationName,
              *producingFederateId,
              *interactionClassHandle,
              umbra::detail::hla::utf8::mom::reliable,
              false);
      if (status != umbra::detail::FederationRegistryStatus::applied) {
        throw RTIinternalError(
            L"The embedded federation lost the Send Interaction membership before its accepted MOM request boundary.");
      }
    }
    queueMomDirectedInteractionsSentReport(
        *federationName,
        momDirectedInteractionsSentReport->reportedFederateId,
        std::move(*momDirectedInteractionsSentReport));
    return;
  }

  auto const handleMomInteractionsReceivedReportRequest = [&]() {
    std::scoped_lock lock(mutex_, ambassadorFederationManagementMutex());
    requireConnectedForFederationManagement(lifecycle_);
    requireFederationServiceOperationAvailable(L"Send Interaction");
    if (lifecycle_.state() != umbra::detail::FederateLifecycleState::joined ||
        !joinedFederationName_ || !joinedFederateId_ ||
        *joinedFederationName_ != *federationName ||
        *joinedFederateId_ != *producingFederateId) {
      throw FederateNotExecutionMember(
          L"The RTI ambassador's federation membership changed during Send Interaction.");
    }
    auto& registry = embeddedFederationRegistry();
    if (!registry.memberById(*federationName, *producingFederateId)) {
      throw FederateNotExecutionMember(
          L"The embedded federation no longer records this RTI ambassador as a member.");
    }
    auto const interactionClassName = registry.interactionClassNameFor(
        *federationName,
        *interactionClassHandle);
    if (!interactionClassName) {
      return false;
    }
    auto const isInteractionsReceivedRequest = registry.interactionClassIsSameOrDescendantOf(
        *federationName,
        *interactionClassHandle,
        umbra::detail::hla::utf8::mom::request_interactions_received);
    if (!isInteractionsReceivedRequest) {
      throw RTIinternalError(
          L"The embedded federation cannot resolve the HLArequestInteractionsReceived MOM class hierarchy.");
    }
    if (!*isInteractionsReceivedRequest) {
      return false;
    }

    std::optional<std::uint64_t> reportedFederateId;
    for (auto const& [parameterHandle, parameterValue] : parameterValues) {
      auto const suppliedParameterHandleValue = parameterHandleValue(parameterHandle);
      if (!suppliedParameterHandleValue) {
        throw InteractionParameterNotDefined(
            L"HLArequestInteractionsReceived received an invalid parameter handle.");
      }
      auto const parameterName = registry.parameterNameFor(
          *federationName,
          *interactionClassName,
          *suppliedParameterHandleValue);
      if (!parameterName) {
        throw InteractionParameterNotDefined(
            L"HLArequestInteractionsReceived received an unknown parameter for this interaction class.");
      }
      if (*parameterName != umbra::detail::hla::utf8::mom::federate) {
        continue;
      }
      try {
        auto const decodedHandle =
            ::rti1516_2025::umbra_binding_detail::decodeFederateHandle(parameterValue);
        reportedFederateId =
            ::rti1516_2025::umbra_binding_detail::federateHandleValue(decodedHandle);
      } catch (Exception const&) {
        throw RTIinternalError(
            L"HLArequestInteractionsReceived received an invalid HLAfederateReference value.");
      }
      if (!reportedFederateId) {
        throw RTIinternalError(
            L"HLArequestInteractionsReceived received an invalid HLAfederateReference value.");
      }
    }
    if (!reportedFederateId) {
      throw InteractionParameterNotDefined(
          L"HLArequestInteractionsReceived requires the HLAfederate parameter.");
    }
    auto report = registry.planMomInteractionsReceivedReport(
        *federationName,
        *producingFederateId,
        *reportedFederateId);
    switch (report.status) {
      case umbra::detail::MomObjectInstanceCountsReportStatus::applied:
        momInteractionsReceivedReport = std::move(report);
        return true;
      case umbra::detail::MomObjectInstanceCountsReportStatus::federation_does_not_exist:
      case umbra::detail::MomObjectInstanceCountsReportStatus::requesting_federate_not_member:
      case umbra::detail::MomObjectInstanceCountsReportStatus::reported_federate_not_member:
        throw FederateNotExecutionMember(
            L"HLArequestInteractionsReceived requires joined requesting and reported federates.");
      case umbra::detail::MomObjectInstanceCountsReportStatus::inconsistent_catalog:
        throw RTIinternalError(
            L"The embedded federation cannot reconstruct the requested HLAreportInteractionsReceived interaction.");
    }
    throw RTIinternalError(
        L"The embedded federation rejected the HLArequestInteractionsReceived interaction.");
  };

  if (handleMomInteractionsReceivedReportRequest()) {
    std::vector<InteractionParameterValue> sentParameters =
        ambassadorCopyInteractionParameterValues(parameterValues);
    VariableLengthData copiedTag(userSuppliedTag);
    ParameterHandleValueMap reportParameterValues;
    for (auto const& [parameterHandle, parameterValue] : sentParameters) {
      reportParameterValues.emplace(makeParameterHandle(parameterHandle), parameterValue);
    }
    auto const reportArguments = std::vector<umbra::detail::MomServiceArgument>{
        {umbra::detail::MomArgumentType::interaction_class_handle,
         L"Interaction class designator",
         umbra::detail::formatMomInteractionClassHandle(interactionClass)},
        {umbra::detail::MomArgumentType::parameter_handle_value_map,
         L"Constrained set of interaction parameter designator and value pairs",
         umbra::detail::formatMomParameterHandleValueMap(reportParameterValues)},
        {umbra::detail::MomArgumentType::table_5_user_supplied_tag,
         L"User-supplied tag",
         umbra::detail::formatMomUserSuppliedTag(copiedTag)},
        {umbra::detail::MomArgumentType::null_value,
         L"Optional timestamp",
         umbra::detail::formatMomNull()},
    };
    appendSuccessfulVoidServiceReportToFileIfSelected(
        L"SendInteraction",
        umbra::detail::MomServiceType::object_management,
        reportArguments,
        true);
    {
      std::scoped_lock lock(ambassadorFederationManagementMutex());
      auto const status = embeddedFederationRegistry()
          .recordSuccessfulInteractionSend(
              *federationName,
              *producingFederateId,
              *interactionClassHandle,
              umbra::detail::hla::utf8::mom::reliable,
              false);
      if (status != umbra::detail::FederationRegistryStatus::applied) {
        throw RTIinternalError(
            L"The embedded federation lost the Send Interaction membership before its accepted MOM request boundary.");
      }
    }
    queueMomInteractionsReceivedReport(
        *federationName,
        momInteractionsReceivedReport->reportedFederateId,
        std::move(*momInteractionsReceivedReport));
    return;
  }

  auto const handleMomDirectedInteractionsReceivedReportRequest = [&]() {
    std::scoped_lock lock(mutex_, ambassadorFederationManagementMutex());
    requireConnectedForFederationManagement(lifecycle_);
    requireFederationServiceOperationAvailable(L"Send Interaction");
    if (lifecycle_.state() != umbra::detail::FederateLifecycleState::joined ||
        !joinedFederationName_ || !joinedFederateId_ ||
        *joinedFederationName_ != *federationName ||
        *joinedFederateId_ != *producingFederateId) {
      throw FederateNotExecutionMember(
          L"The RTI ambassador's federation membership changed during Send Interaction.");
    }
    auto& registry = embeddedFederationRegistry();
    if (!registry.memberById(*federationName, *producingFederateId)) {
      throw FederateNotExecutionMember(
          L"The embedded federation no longer records this RTI ambassador as a member.");
    }
    auto const interactionClassName = registry.interactionClassNameFor(
        *federationName,
        *interactionClassHandle);
    if (!interactionClassName) {
      return false;
    }
    auto const isDirectedInteractionsReceivedRequest =
        registry.interactionClassIsSameOrDescendantOf(
            *federationName,
            *interactionClassHandle,
            umbra::detail::hla::utf8::mom::request_directed_interactions_received);
    if (!isDirectedInteractionsReceivedRequest) {
      throw RTIinternalError(
          L"The embedded federation cannot resolve the HLArequestDirectedInteractionsReceived MOM class hierarchy.");
    }
    if (!*isDirectedInteractionsReceivedRequest) {
      return false;
    }

    std::optional<std::uint64_t> reportedFederateId;
    for (auto const& [parameterHandle, parameterValue] : parameterValues) {
      auto const suppliedParameterHandleValue = parameterHandleValue(parameterHandle);
      if (!suppliedParameterHandleValue) {
        throw InteractionParameterNotDefined(
            L"HLArequestDirectedInteractionsReceived received an invalid parameter handle.");
      }
      auto const parameterName = registry.parameterNameFor(
          *federationName,
          *interactionClassName,
          *suppliedParameterHandleValue);
      if (!parameterName) {
        throw InteractionParameterNotDefined(
            L"HLArequestDirectedInteractionsReceived received an unknown parameter for this interaction class.");
      }
      if (*parameterName != umbra::detail::hla::utf8::mom::federate) {
        continue;
      }
      try {
        auto const decodedHandle =
            ::rti1516_2025::umbra_binding_detail::decodeFederateHandle(parameterValue);
        reportedFederateId =
            ::rti1516_2025::umbra_binding_detail::federateHandleValue(decodedHandle);
      } catch (Exception const&) {
        throw RTIinternalError(
            L"HLArequestDirectedInteractionsReceived received an invalid HLAfederateReference value.");
      }
      if (!reportedFederateId) {
        throw RTIinternalError(
            L"HLArequestDirectedInteractionsReceived received an invalid HLAfederateReference value.");
      }
    }
    if (!reportedFederateId) {
      throw InteractionParameterNotDefined(
          L"HLArequestDirectedInteractionsReceived requires the HLAfederate parameter.");
    }
    auto report = registry.planMomDirectedInteractionsReceivedReport(
        *federationName,
        *producingFederateId,
        *reportedFederateId);
    switch (report.status) {
      case umbra::detail::MomObjectInstanceCountsReportStatus::applied:
        momDirectedInteractionsReceivedReport = std::move(report);
        return true;
      case umbra::detail::MomObjectInstanceCountsReportStatus::federation_does_not_exist:
      case umbra::detail::MomObjectInstanceCountsReportStatus::requesting_federate_not_member:
      case umbra::detail::MomObjectInstanceCountsReportStatus::reported_federate_not_member:
        throw FederateNotExecutionMember(
            L"HLArequestDirectedInteractionsReceived requires joined requesting and reported federates.");
      case umbra::detail::MomObjectInstanceCountsReportStatus::inconsistent_catalog:
        throw RTIinternalError(
            L"The embedded federation cannot reconstruct the requested HLAreportDirectedInteractionsReceived interaction.");
    }
    throw RTIinternalError(
        L"The embedded federation rejected the HLArequestDirectedInteractionsReceived interaction.");
  };

  if (handleMomDirectedInteractionsReceivedReportRequest()) {
    std::vector<InteractionParameterValue> sentParameters =
        ambassadorCopyInteractionParameterValues(parameterValues);
    VariableLengthData copiedTag(userSuppliedTag);
    ParameterHandleValueMap reportParameterValues;
    for (auto const& [parameterHandle, parameterValue] : sentParameters) {
      reportParameterValues.emplace(makeParameterHandle(parameterHandle), parameterValue);
    }
    auto const reportArguments = std::vector<umbra::detail::MomServiceArgument>{
        {umbra::detail::MomArgumentType::interaction_class_handle,
         L"Interaction class designator",
         umbra::detail::formatMomInteractionClassHandle(interactionClass)},
        {umbra::detail::MomArgumentType::parameter_handle_value_map,
         L"Constrained set of interaction parameter designator and value pairs",
         umbra::detail::formatMomParameterHandleValueMap(reportParameterValues)},
        {umbra::detail::MomArgumentType::table_5_user_supplied_tag,
         L"User-supplied tag",
         umbra::detail::formatMomUserSuppliedTag(copiedTag)},
        {umbra::detail::MomArgumentType::null_value,
         L"Optional timestamp",
         umbra::detail::formatMomNull()},
    };
    appendSuccessfulVoidServiceReportToFileIfSelected(
        L"SendInteraction",
        umbra::detail::MomServiceType::object_management,
        reportArguments,
        true);
    {
      std::scoped_lock lock(ambassadorFederationManagementMutex());
      auto const status = embeddedFederationRegistry()
          .recordSuccessfulInteractionSend(
              *federationName,
              *producingFederateId,
              *interactionClassHandle,
              umbra::detail::hla::utf8::mom::reliable,
              false);
      if (status != umbra::detail::FederationRegistryStatus::applied) {
        throw RTIinternalError(
            L"The embedded federation lost the Send Interaction membership before its accepted MOM request boundary.");
      }
    }
    queueMomDirectedInteractionsReceivedReport(
        *federationName,
        momDirectedInteractionsReceivedReport->reportedFederateId,
        std::move(*momDirectedInteractionsReceivedReport));
    return;
  }
  // Keep MOM control requests separate from ordinary application delivery.
  if (handleEmbeddedMomInteractionControlRequest(
          federationName,
          producingFederateId,
          interactionClassHandle,
          interactionClass,
          parameterValues,
          userSuppliedTag)) {
    return;
  }
  // Ordinary embedded application validation, delivery, and exception reporting.

  auto planCurrentRequest = [&]() {
    std::scoped_lock lock(mutex_, ambassadorFederationManagementMutex());
    requireConnectedForFederationManagement(lifecycle_);
    requireFederationServiceOperationAvailable(L"Send Interaction");
    if (lifecycle_.state() != umbra::detail::FederateLifecycleState::joined ||
        !joinedFederationName_ || !joinedFederateId_) {
      throw FederateNotExecutionMember(
          L"Send Interaction requires membership in a federation execution.");
    }
    if (!federationName) {
      federationName = *joinedFederationName_;
      producingFederateId = *joinedFederateId_;
    } else if (*joinedFederationName_ != *federationName ||
               *joinedFederateId_ != *producingFederateId) {
      throw FederateNotExecutionMember(
          L"The RTI ambassador's federation membership changed during Send Interaction.");
    }

    auto& registry = embeddedFederationRegistry();
    if (!registry.memberById(*federationName, *producingFederateId)) {
      throw FederateNotExecutionMember(
          L"The embedded federation no longer records this RTI ambassador as a member.");
    }
    auto plan = registry.planReceiveOrderInteraction(
        *federationName,
        *producingFederateId,
        *interactionClassHandle,
        *parameterHandles);
    if (plan.status != umbra::detail::ReceiveOrderInteractionStatus::applied) {
      throwReceiveOrderInteractionFailure(plan.status);
    }
    return plan;
  };

  // Validate publication, membership, class, and parameter availability before
  // reading caller-owned VariableLengthData storage.  The durable copies below
  // are made without a runtime lock, then the plan is rechecked before routing.
  static_cast<void>(planCurrentRequest());
  std::vector<InteractionParameterValue> sentParameters =
      ambassadorCopyInteractionParameterValues(parameterValues);
  VariableLengthData copiedTag(userSuppliedTag);
  ParameterHandleValueMap reportParameterValues;
  for (auto const& [parameterHandle, parameterValue] : sentParameters) {
    reportParameterValues.emplace(makeParameterHandle(parameterHandle), parameterValue);
  }
  // Section 6.12.1 names four supplied values. The receive-order overload
  // leaves the optional timestamp unused, and Section 11.5.1 therefore
  // requires its corresponding supplied-argument element to be Null. The
  // accepted parameter map is rebuilt from durable copies before formatting so
  // caller-owned buffers cannot alter the report boundary.
  auto const reportArguments = std::vector<umbra::detail::MomServiceArgument>{
      {umbra::detail::MomArgumentType::interaction_class_handle,
       L"Interaction class designator",
       umbra::detail::formatMomInteractionClassHandle(interactionClass)},
      {umbra::detail::MomArgumentType::parameter_handle_value_map,
       L"Constrained set of interaction parameter designator and value pairs",
       umbra::detail::formatMomParameterHandleValueMap(reportParameterValues)},
      {umbra::detail::MomArgumentType::table_5_user_supplied_tag,
       L"User-supplied tag",
       umbra::detail::formatMomUserSuppliedTag(copiedTag)},
      {umbra::detail::MomArgumentType::null_value,
       L"Optional timestamp",
       umbra::detail::formatMomNull()},
  };
  auto plan = planCurrentRequest();

  auto const transportationName = umbra::detail::wideFromUtf8(plan.transportationName);
  if (!transportationName) {
    throw RTIinternalError(
        L"The composed FOM contains an invalid UTF-8 transportation type name.");
  }
  auto const transportationValue = ambassadorTransportationTypeValueForFederation(
      *federationName,
      *transportationName);
  if (!transportationValue) {
    throw RTIinternalError(
        L"The composed FOM transportation type is not declared in this federation execution.");
  }
  TransportationTypeHandle const transportationType = makeTransportationTypeHandle(*transportationValue);

  struct Delivery {
    umbra::detail::InteractionCallbackRoute callbackRoute;
    std::uint64_t recipientId = 0;
  };
  std::vector<Delivery> deliveries;
  deliveries.reserve(plan.recipients.size());
  for (auto const& recipient : plan.recipients) {
    if (!recipient.callbackRoute) {
      throw RTIinternalError(
          L"The embedded federation has an eligible recipient without a callback route.");
    }
    deliveries.push_back({recipient.callbackRoute, recipient.federateId});
  }

  // Every synchronous pre-callback delivery check has now succeeded. Section
  // 6.12's accepted interaction is the report boundary, and the §11.5 file
  // record must precede any induced Receive Interaction callback.
  appendSuccessfulVoidServiceReportToFileIfSelected(
      L"SendInteraction",
      umbra::detail::MomServiceType::object_management,
      reportArguments,
      true);

  // The accepted Send Interaction invocation is the MOM counter boundary;
  // recipient callback fan-out must not inflate HLAinteractionsSent.
  {
    std::scoped_lock lock(ambassadorFederationManagementMutex());
    auto const status = embeddedFederationRegistry()
        .recordSuccessfulInteractionSend(
            *federationName,
            *producingFederateId,
            *interactionClassHandle,
            plan.transportationName,
            false);
    if (status != umbra::detail::FederationRegistryStatus::applied) {
      throw RTIinternalError(
          L"The embedded federation lost the Send Interaction membership before its accepted boundary.");
    }
  }

  // Do not hold either sender lock while submitting a route: HLA_IMMEDIATE may
  // synchronously enter a different federate's Receive Interaction callback.
  for (auto& delivery : deliveries) {
    queueAmbassadorReceiveOrderInteraction(
        std::move(delivery.callbackRoute),
        *federationName,
        umbra::detail::InteractionProducer::joinedFederate(*producingFederateId),
        delivery.recipientId,
        *interactionClassHandle,
        sentParameters,
        copiedTag,
        transportationType,
        plan.transportationName,
        std::nullopt,
        plan.defaultRegionUsed);
  }
  } catch (Exception const& exception) {
    if (momExceptionService) {
      emitMomExceptionReport(
          *momExceptionService,
          exception,
          ambassadorMomExceptionIsParameterError(exception));
    }
    emitExceptionReport(L"Send Interaction", exception);
    appendFailedServiceReportToFileIfSelected(
        L"SendInteraction",
        umbra::detail::MomServiceType::object_management,
        {{umbra::detail::MomArgumentType::interaction_class_handle,
          L"Interaction class designator",
          umbra::detail::formatMomInteractionClassHandle(interactionClass)},
         {umbra::detail::MomArgumentType::parameter_handle_value_map,
          L"Constrained set of interaction parameter designator and value pairs",
          umbra::detail::formatMomParameterHandleValueMap(parameterValues)},
         {umbra::detail::MomArgumentType::table_5_user_supplied_tag,
          L"User-supplied tag",
          umbra::detail::formatMomUserSuppliedTag(userSuppliedTag)},
         {umbra::detail::MomArgumentType::null_value,
          L"Optional timestamp",
          umbra::detail::formatMomNull()}},
        describeAmbassadorException(exception),
        true);
    throw;
  }
}
// IEEE 1516.1-2025 interaction-with-regions send overloads.

#endif

}  // namespace rti1516_2025::umbra_binding_detail
