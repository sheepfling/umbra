#include "internal/federation/process_federation_client.hpp"
#include "internal/federation/process_federation_client_error.hpp"

#include <RTI/Exception.h>

#include <algorithm>
#include <limits>
#include <type_traits>
#include <utility>

namespace umbra::detail {

void ProcessFederationClient::createFederationExecution(
    std::wstring federationName,
    std::vector<std::wstring> fomModules,
    std::optional<std::wstring> mimModule,
    std::wstring logicalTimeImplementationName) {
  auto response = request(
      TransportServiceOperation::create_federation_execution,
      encodeProcessFederationCreateRequest(
          ProcessFederationCreateRequest{
              std::move(federationName),
              std::move(fomModules),
              std::move(mimModule),
              std::move(logicalTimeImplementationName)}));
  if (response.status != TransportServiceStatus::ok) {
    throw ProcessFederationClientError(
        requestFailure(response.operation, response.status));
  }
}

ProcessFederationDestroyResult
ProcessFederationClient::destroyFederationExecution(
    std::wstring federationName) {
  auto response = request(
      TransportServiceOperation::destroy_federation_execution,
      encodeProcessFederationDestroyRequest(
          ProcessFederationDestroyRequest{std::move(federationName)}));
  if (response.status != TransportServiceStatus::ok) {
    throw ProcessFederationClientError(
        requestFailure(response.operation, response.status));
  }
  return decodeProcessFederationDestroyResult(response.payload);
}

ProcessFederationJoinResult ProcessFederationClient::joinFederationExecution(
    std::wstring federationName,
    std::wstring federateType,
    std::optional<std::wstring> requestedFederateName,
    std::vector<std::wstring> additionalFomModules) {
  return joinFederationExecution(
      std::move(federationName),
      std::move(federateType),
      std::move(requestedFederateName),
      std::move(additionalFomModules),
      ProcessFederationJoinConnectionSnapshot{});
}

ProcessFederationJoinResult ProcessFederationClient::joinFederationExecution(
    std::wstring federationName,
    std::wstring federateType,
    std::optional<std::wstring> requestedFederateName,
    std::vector<std::wstring> additionalFomModules,
    ProcessFederationJoinConnectionSnapshot connection) {
  auto joinedFederationName = federationName;
  auto response = request(
      TransportServiceOperation::join_federation_execution,
      encodeProcessFederationJoinRequest(
          ProcessFederationJoinRequest{
              std::move(federationName),
              std::move(federateType),
              std::move(requestedFederateName),
              std::move(additionalFomModules),
              std::move(connection)}));
  if (response.status != TransportServiceStatus::ok) {
    throw ProcessFederationClientError(
        requestFailure(response.operation, response.status));
  }
  auto result = decodeProcessFederationJoinResult(response.payload);
  std::scoped_lock lock(transactionMutex_);
  if (!session_) {
    throw ProcessFederationClientError("A process federation join completed after close.");
  }
  joinedFederationName_ = std::move(joinedFederationName);
  joinedFederateId_ = result.federateId;
  return result;
}

void ProcessFederationClient::resignFederationExecution(
    std::wstring federationName,
    std::uint64_t federateId,
    rti1516_2025::ResignAction resignAction) {
  auto resignedFederationName = federationName;
  auto response = request(
      TransportServiceOperation::resign_federation_execution,
      encodeProcessFederationResignRequest(
          ProcessFederationResignRequest{
              std::move(federationName), federateId, resignAction}));
  if (response.status != TransportServiceStatus::ok) {
    throw ProcessFederationClientError(
        requestFailure(response.operation, response.status));
  }
  std::shared_ptr<ProcessFederationCallbackBridge> bridge;
  {
    std::scoped_lock lock(transactionMutex_);
    if (joinedFederationName_ &&
        *joinedFederationName_ == resignedFederationName &&
        joinedFederateId_ == federateId) {
      joinedFederationName_.reset();
      joinedFederateId_ = 0U;
      bridge = callbackBridge_;
    }
  }
  if (bridge) {
    bridge->cancelExceptionReportProjections();
  }
}

ProcessFederationRegisterSynchronizationPointResult
ProcessFederationClient::registerFederationSynchronizationPoint(
    std::wstring federationName,
    std::uint64_t federateId,
    std::wstring label,
    std::vector<std::uint8_t> userSuppliedTag,
    std::vector<std::uint64_t> synchronizationSet,
    bool synchronizationSetWasSupplied) {
  auto response = request(
      TransportServiceOperation::register_federation_synchronization_point,
      encodeProcessFederationRegisterSynchronizationPointRequest(
          ProcessFederationRegisterSynchronizationPointRequest{
              std::move(federationName),
              federateId,
              std::move(label),
              std::move(userSuppliedTag),
              std::move(synchronizationSet),
              synchronizationSetWasSupplied}));
  if (response.status != TransportServiceStatus::ok) {
    throw ProcessFederationClientError(
        requestFailure(response.operation, response.status));
  }
  return decodeProcessFederationRegisterSynchronizationPointResult(
      response.payload);
}

ProcessFederationSynchronizationPointAchievedResult
ProcessFederationClient::synchronizationPointAchieved(
    std::wstring federationName,
    std::uint64_t federateId,
    std::wstring label,
    bool successfully) {
  auto response = request(
      TransportServiceOperation::synchronization_point_achieved,
      encodeProcessFederationSynchronizationPointAchievedRequest(
          ProcessFederationSynchronizationPointAchievedRequest{
              std::move(federationName), federateId, std::move(label), successfully}));
  if (response.status != TransportServiceStatus::ok) {
    throw ProcessFederationClientError(
        requestFailure(response.operation, response.status));
  }
  return decodeProcessFederationSynchronizationPointAchievedResult(
      response.payload);
}

ProcessFederationSaveControlResult ProcessFederationClient::requestFederationSave(
    std::wstring federationName,
    std::uint64_t federateId,
    std::wstring label) {
  return requestFederationSave(
      std::move(federationName),
      federateId,
      std::move(label),
      std::nullopt);
}

ProcessFederationSaveControlResult ProcessFederationClient::requestFederationSave(
    std::wstring federationName,
    std::uint64_t federateId,
    std::wstring label,
    std::optional<ProcessFederationLogicalTime> timestamp) {
  auto response = request(
      TransportServiceOperation::request_federation_save,
      encodeProcessFederationSaveRequest(
          ProcessFederationSaveRequest{
              std::move(federationName),
              federateId,
              std::move(label),
              std::move(timestamp)}));
  if (response.status != TransportServiceStatus::ok) {
    throw ProcessFederationClientError(
        requestFailure(response.operation, response.status));
  }
  return decodeProcessFederationSaveControlResult(response.payload);
}

ProcessFederationSaveControlResult ProcessFederationClient::federateSaveBegun(
    std::wstring federationName,
    std::uint64_t federateId) {
  auto response = request(
      TransportServiceOperation::federate_save_begun,
      encodeProcessFederationSaveRequest(
          ProcessFederationSaveRequest{std::move(federationName), federateId, {}}));
  if (response.status != TransportServiceStatus::ok) {
    throw ProcessFederationClientError(
        requestFailure(response.operation, response.status));
  }
  return decodeProcessFederationSaveControlResult(response.payload);
}

ProcessFederationSaveControlResult ProcessFederationClient::federateSaveComplete(
    std::wstring federationName,
    std::uint64_t federateId) {
  auto response = request(
      TransportServiceOperation::federate_save_complete,
      encodeProcessFederationSaveRequest(
          ProcessFederationSaveRequest{std::move(federationName), federateId, {}}));
  if (response.status != TransportServiceStatus::ok) {
    throw ProcessFederationClientError(
        requestFailure(response.operation, response.status));
  }
  return decodeProcessFederationSaveControlResult(response.payload);
}

ProcessFederationSaveControlResult
ProcessFederationClient::federateSaveNotComplete(
    std::wstring federationName,
    std::uint64_t federateId) {
  auto response = request(
      TransportServiceOperation::federate_save_not_complete,
      encodeProcessFederationSaveRequest(
          ProcessFederationSaveRequest{std::move(federationName), federateId, {}}));
  if (response.status != TransportServiceStatus::ok) {
    throw ProcessFederationClientError(
        requestFailure(response.operation, response.status));
  }
  return decodeProcessFederationSaveControlResult(response.payload);
}

ProcessFederationSaveControlResult
ProcessFederationClient::queryFederationSaveStatus(
    std::wstring federationName,
    std::uint64_t federateId) {
  auto response = request(
      TransportServiceOperation::query_federation_save_status,
      encodeProcessFederationSaveRequest(
          ProcessFederationSaveRequest{std::move(federationName), federateId, {}}));
  if (response.status != TransportServiceStatus::ok) {
    throw ProcessFederationClientError(
        requestFailure(response.operation, response.status));
  }
  return decodeProcessFederationSaveControlResult(response.payload);
}

ProcessFederationSaveControlResult ProcessFederationClient::abortFederationSave(
    std::wstring federationName,
    std::uint64_t federateId) {
  auto response = request(
      TransportServiceOperation::abort_federation_save,
      encodeProcessFederationSaveRequest(
          ProcessFederationSaveRequest{std::move(federationName), federateId, {}}));
  if (response.status != TransportServiceStatus::ok) {
    throw ProcessFederationClientError(
        requestFailure(response.operation, response.status));
  }
  return decodeProcessFederationSaveControlResult(response.payload);
}

ProcessFederationRestoreControlResult
ProcessFederationClient::requestFederationRestore(
    std::wstring federationName,
    std::uint64_t federateId,
    std::wstring label) {
  auto response = request(
      TransportServiceOperation::request_federation_restore,
      encodeProcessFederationRestoreRequest(
          ProcessFederationRestoreRequest{
              std::move(federationName), federateId, std::move(label)}));
  if (response.status != TransportServiceStatus::ok) {
    throw ProcessFederationClientError(
        requestFailure(response.operation, response.status));
  }
  return decodeProcessFederationRestoreControlResult(response.payload);
}

ProcessFederationRestoreControlResult
ProcessFederationClient::federateRestoreComplete(
    std::wstring federationName,
    std::uint64_t federateId,
    bool callbacksEnabled) {
  auto response = request(
      TransportServiceOperation::federate_restore_complete,
      encodeProcessFederationRestoreRequest(
          ProcessFederationRestoreRequest{
              std::move(federationName), federateId, {}, callbacksEnabled}));
  if (response.status != TransportServiceStatus::ok) {
    throw ProcessFederationClientError(
        requestFailure(response.operation, response.status));
  }
  return decodeProcessFederationRestoreControlResult(response.payload);
}

ProcessFederationRestoreControlResult
ProcessFederationClient::federateRestoreNotComplete(
    std::wstring federationName,
    std::uint64_t federateId) {
  auto response = request(
      TransportServiceOperation::federate_restore_not_complete,
      encodeProcessFederationRestoreRequest(
          ProcessFederationRestoreRequest{
              std::move(federationName), federateId, {}}));
  if (response.status != TransportServiceStatus::ok) {
    throw ProcessFederationClientError(
        requestFailure(response.operation, response.status));
  }
  return decodeProcessFederationRestoreControlResult(response.payload);
}

ProcessFederationRestoreControlResult
ProcessFederationClient::abortFederationRestore(
    std::wstring federationName,
    std::uint64_t federateId) {
  auto response = request(
      TransportServiceOperation::abort_federation_restore,
      encodeProcessFederationRestoreRequest(
          ProcessFederationRestoreRequest{
              std::move(federationName), federateId, {}}));
  if (response.status != TransportServiceStatus::ok) {
    throw ProcessFederationClientError(
        requestFailure(response.operation, response.status));
  }
  return decodeProcessFederationRestoreControlResult(response.payload);
}

ProcessFederationRestoreControlResult
ProcessFederationClient::queryFederationRestoreStatus(
    std::wstring federationName,
    std::uint64_t federateId) {
  auto response = request(
      TransportServiceOperation::query_federation_restore_status,
      encodeProcessFederationRestoreRequest(
          ProcessFederationRestoreRequest{
              std::move(federationName), federateId, {}}));
  if (response.status != TransportServiceStatus::ok) {
    throw ProcessFederationClientError(
        requestFailure(response.operation, response.status));
  }
  return decodeProcessFederationRestoreControlResult(response.payload);
}

rti1516_2025::ResignAction ProcessFederationClient::getAutomaticResignDirective(
    std::wstring federationName,
    std::uint64_t federateId) {
  auto response = request(
      TransportServiceOperation::get_automatic_resign_directive,
      encodeProcessFederationResignRequest(
          ProcessFederationResignRequest{
              std::move(federationName),
              federateId,
              rti1516_2025::NO_ACTION}));
  if (response.status != TransportServiceStatus::ok) {
    throw ProcessFederationClientError(
        requestFailure(response.operation, response.status));
  }
  return decodeProcessFederationResignRequest(response.payload).resignAction;
}

void ProcessFederationClient::setAutomaticResignDirective(
    std::wstring federationName,
    std::uint64_t federateId,
    rti1516_2025::ResignAction resignAction) {
  auto response = request(
      TransportServiceOperation::set_automatic_resign_directive,
      encodeProcessFederationResignRequest(
          ProcessFederationResignRequest{
              std::move(federationName), federateId, resignAction}));
  if (response.status != TransportServiceStatus::ok) {
    throw ProcessFederationClientError(
        requestFailure(response.operation, response.status));
  }
  static_cast<void>(decodeProcessFederationBooleanResult(response.payload));
}

bool ProcessFederationClient::getServiceReportingSwitch(
    std::wstring federationName,
    std::uint64_t federateId) {
  auto response = request(
      TransportServiceOperation::get_service_reporting_switch,
      encodeProcessFederationAttributeScopeAdvisorySwitchRequest(
          ProcessFederationAttributeScopeAdvisorySwitchRequest{
              std::move(federationName), federateId, false}));
  if (response.status != TransportServiceStatus::ok) {
    throw ProcessFederationClientError(
        requestFailure(response.operation, response.status));
  }
  return decodeProcessFederationBooleanResult(response.payload).value;
}

void ProcessFederationClient::setServiceReportingSwitch(
    std::wstring federationName,
    std::uint64_t federateId,
    bool switchValue) {
  auto response = request(
      TransportServiceOperation::set_service_reporting_switch,
      encodeProcessFederationAttributeScopeAdvisorySwitchRequest(
          ProcessFederationAttributeScopeAdvisorySwitchRequest{
              std::move(federationName), federateId, switchValue}));
  if (response.status != TransportServiceStatus::ok) {
    throw ProcessFederationClientError(
        requestFailure(response.operation, response.status));
  }
  static_cast<void>(decodeProcessFederationBooleanResult(response.payload));
}

ProcessFederationExceptionReportingSwitchResult
ProcessFederationClient::getExceptionReportingSwitch(
    std::wstring federationName,
    std::uint64_t federateId) {
  auto response = request(
      TransportServiceOperation::get_exception_reporting_switch,
      encodeProcessFederationAttributeScopeAdvisorySwitchRequest(
          ProcessFederationAttributeScopeAdvisorySwitchRequest{
              std::move(federationName), federateId, false}));
  if (response.status != TransportServiceStatus::ok) {
    throw ProcessFederationClientError(
        requestFailure(response.operation, response.status));
  }
  return decodeProcessFederationExceptionReportingSwitchResult(response.payload);
}

void ProcessFederationClient::reportServiceException(
    std::wstring federationName,
    std::uint64_t federateId,
    std::wstring service,
    std::wstring exception) {
  auto const response = request(
      TransportServiceOperation::report_service_exception,
      encodeProcessFederationServiceExceptionRequest({
          std::move(federationName), federateId,
          std::move(service), std::move(exception)}));
  if (response.status != TransportServiceStatus::ok) {
    throw ProcessFederationClientError(
        requestFailure(response.operation, response.status));
  }
}

void ProcessFederationClient::reportFailedServiceInvocation(
    std::wstring federationName,
    std::uint64_t federateId,
    MomServiceType serviceType,
    std::wstring service,
    std::vector<MomServiceArgument> suppliedArguments,
    std::wstring exception) {
  auto const response = request(
      TransportServiceOperation::report_failed_service_invocation,
      encodeProcessFederationFailedServiceInvocationRequest({
          std::move(federationName), federateId, serviceType,
          std::move(service), std::move(suppliedArguments),
          std::move(exception)}));
  if (response.status != TransportServiceStatus::ok) {
    throw ProcessFederationClientError(
        requestFailure(response.operation, response.status));
  }
}

void ProcessFederationClient::reportSuccessfulServiceInvocation(
    std::wstring federationName,
    std::uint64_t federateId,
    MomServiceType serviceType,
    std::wstring service,
    std::vector<MomServiceArgument> suppliedArguments,
    MomServiceArgument returnedArgument) {
  auto const response = request(
      TransportServiceOperation::report_successful_service_invocation,
      encodeProcessFederationSuccessfulServiceInvocationRequest({
          std::move(federationName), federateId, serviceType,
          std::move(service), std::move(suppliedArguments),
          std::move(returnedArgument)}));
  if (response.status != TransportServiceStatus::ok) {
    throw ProcessFederationClientError(
        requestFailure(response.operation, response.status));
  }
}

void ProcessFederationClient::reportSuccessfulVoidServiceInvocation(
    std::wstring federationName,
    std::uint64_t federateId,
    MomServiceType serviceType,
    std::wstring service,
    std::vector<MomServiceArgument> suppliedArguments) {
  auto const response = request(
      TransportServiceOperation::report_successful_void_service_invocation,
      encodeProcessFederationSuccessfulServiceInvocationRequest({
          std::move(federationName), federateId, serviceType,
          std::move(service), std::move(suppliedArguments),
          {MomArgumentType::null_value, L"", formatMomNull()}}));
  if (response.status != TransportServiceStatus::ok) {
    throw ProcessFederationClientError(
        requestFailure(response.operation, response.status));
  }
}

std::optional<ProcessFederationInteractionEvent>
ProcessFederationClient::recheckExceptionReport(
    ProcessFederationInteractionEvent event) {
  if (!event.exceptionReportFederateId) {
    return event;
  }
  std::wstring federationName;
  std::uint64_t federateId = 0U;
  {
    std::scoped_lock lock(transactionMutex_);
    if (!session_ || !joinedFederationName_ ||
        joinedFederateId_ != event.receivingFederateId) {
      return std::nullopt;
    }
    federationName = *joinedFederationName_;
    federateId = joinedFederateId_;
  }
  try {
    auto const response = request(
        TransportServiceOperation::recheck_exception_report,
        encodeProcessFederationExceptionReportRecheckRequest({
            std::move(federationName), federateId,
            *event.exceptionReportFederateId}), false);
    if (response.status != TransportServiceStatus::ok) {
      return std::nullopt;
    }
    auto projection = decodeProcessFederationReceiveInteractionResult(response.payload);
    if (!projection.event) {
      return std::nullopt;
    }
    event.interactionClassHandle = projection.event->interactionClassHandle;
    event.parameterHandles = std::move(projection.event->parameterHandles);
    return event;
  } catch (...) {
    // A stale or unreachable advisory report cannot replace a service error
    // or interrupt ordinary callback delivery.
    return std::nullopt;
  }
}

ProcessFederationExceptionReportingSwitchResult
ProcessFederationClient::setExceptionReportingSwitch(
    std::wstring federationName,
    std::uint64_t federateId,
    bool switchValue) {
  auto response = request(
      TransportServiceOperation::set_exception_reporting_switch,
      encodeProcessFederationAttributeScopeAdvisorySwitchRequest(
          ProcessFederationAttributeScopeAdvisorySwitchRequest{
              std::move(federationName), federateId, switchValue}));
  if (response.status != TransportServiceStatus::ok) {
    throw ProcessFederationClientError(
        requestFailure(response.operation, response.status));
  }
  return decodeProcessFederationExceptionReportingSwitchResult(response.payload);
}

bool ProcessFederationClient::getSendServiceReportsToFileSwitch(
    std::wstring federationName,
    std::uint64_t federateId) {
  auto response = request(
      TransportServiceOperation::get_send_service_reports_to_file_switch,
      encodeProcessFederationAttributeScopeAdvisorySwitchRequest(
          ProcessFederationAttributeScopeAdvisorySwitchRequest{
              std::move(federationName), federateId, false}));
  if (response.status != TransportServiceStatus::ok) {
    throw ProcessFederationClientError(
        requestFailure(response.operation, response.status));
  }
  return decodeProcessFederationBooleanResult(response.payload).value;
}

void ProcessFederationClient::setSendServiceReportsToFileSwitch(
    std::wstring federationName,
    std::uint64_t federateId,
    bool switchValue) {
  auto response = request(
      TransportServiceOperation::set_send_service_reports_to_file_switch,
      encodeProcessFederationAttributeScopeAdvisorySwitchRequest(
          ProcessFederationAttributeScopeAdvisorySwitchRequest{
              std::move(federationName), federateId, switchValue}));
  if (response.status != TransportServiceStatus::ok) {
    throw ProcessFederationClientError(
        requestFailure(response.operation, response.status));
  }
  static_cast<void>(decodeProcessFederationBooleanResult(response.payload));
}


}  // namespace umbra::detail
