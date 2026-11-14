#include "internal/federation/process_federation_client.hpp"
#include "internal/federation/process_federation_client_error.hpp"

#include <RTI/Exception.h>

#include <algorithm>
#include <filesystem>
#include <limits>
#include <string>
#include <string_view>
#include <type_traits>
#include <utility>

namespace umbra::detail {
namespace {

bool isAsciiAlpha(wchar_t value) {
  return (value >= L'a' && value <= L'z') ||
      (value >= L'A' && value <= L'Z');
}

bool isAsciiAlphaNumeric(wchar_t value) {
  return isAsciiAlpha(value) || (value >= L'0' && value <= L'9');
}

bool isUriDesignator(std::wstring_view value) {
  auto const colon = value.find(L':');
  if (colon == std::wstring_view::npos || colon == 0U ||
      !isAsciiAlpha(value.front())) {
    return false;
  }
  // Do not mistake a Windows drive prefix for a URI scheme.
  if (colon == 1U && value.size() > 2U &&
      (value[2] == L'/' || value[2] == L'\\')) {
    return false;
  }
  return std::all_of(
      value.begin() + 1,
      value.begin() + static_cast<std::ptrdiff_t>(colon),
      [](wchar_t character) {
        return isAsciiAlphaNumeric(character) || character == L'+' ||
            character == L'-' || character == L'.';
      });
}

bool isInlineXml(std::wstring_view value) {
  auto const first = value.find_first_not_of(L" \t\r\n");
  return first != std::wstring_view::npos && value[first] == L'<';
}

std::wstring fileUrlForLocalFomModule(std::wstring_view designator) {
  auto const path = std::filesystem::absolute(
                        std::filesystem::path(std::wstring(designator)))
                        .lexically_normal()
                        .generic_u8string();
  std::string pathUtf8;
  pathUtf8.reserve(path.size());
  for (char8_t const value : path) {
    pathUtf8.push_back(static_cast<char>(value));
  }

  std::string url;
  if (pathUtf8.starts_with("//")) {
    url = "file:";
  } else if (pathUtf8.starts_with('/')) {
    url = "file://";
  } else {
    url = "file:///";
  }

  constexpr char hex[] = "0123456789ABCDEF";
  for (unsigned char const value : pathUtf8) {
    bool const unreserved =
        (value >= 'a' && value <= 'z') || (value >= 'A' && value <= 'Z') ||
        (value >= '0' && value <= '9') || value == '-' || value == '.' ||
        value == '_' || value == '~' || value == '/' || value == ':';
    if (unreserved) {
      url.push_back(static_cast<char>(value));
    } else {
      url.push_back('%');
      url.push_back(hex[value >> 4U]);
      url.push_back(hex[value & 0x0FU]);
    }
  }
  return std::wstring(url.begin(), url.end());
}

std::wstring federateProtocolFomModuleArgument(std::wstring value) {
  if (value.empty() || isUriDesignator(value) || isInlineXml(value)) {
    return value;
  }
  return fileUrlForLocalFomModule(value);
}

void normalizeFomModuleArguments(std::vector<std::wstring>& modules) {
  for (auto& module : modules) {
    module = federateProtocolFomModuleArgument(std::move(module));
  }
}

}  // namespace

void ProcessFederationClient::createFederationExecution(
    std::wstring federationName,
    std::vector<std::wstring> fomModules,
    std::optional<std::wstring> mimModule,
    std::wstring logicalTimeImplementationName,
    bool hasFomInputs,
    std::vector<std::wstring> unsupportedTransportationTypeNames) {
  normalizeFomModuleArguments(fomModules);
  if (mimModule) {
    *mimModule = federateProtocolFomModuleArgument(std::move(*mimModule));
  }
  auto response = request(
      TransportServiceOperation::create_federation_execution,
      encodeProcessFederationCreateRequest(
          ProcessFederationCreateRequest{
              std::move(federationName),
              std::move(fomModules),
              std::move(mimModule),
              std::move(logicalTimeImplementationName),
              hasFomInputs,
              std::move(unsupportedTransportationTypeNames)}));
  if (response.status != TransportServiceStatus::ok) {
    throw ProcessFederationClientError(
        requestFailure(response.operation, response.status), response.status);
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

ProcessFederationExecutionMembersResult
ProcessFederationClient::listFederationExecutionMembers(
    std::wstring federationName) {
  auto response = request(
      TransportServiceOperation::list_federation_execution_members,
      encodeProcessFederationExecutionMembersRequest(
          ProcessFederationExecutionMembersRequest{std::move(federationName)}));
  if (response.status != TransportServiceStatus::ok) {
    throw ProcessFederationClientError(
        requestFailure(response.operation, response.status));
  }
  return decodeProcessFederationExecutionMembersResult(response.payload);
}

ProcessFederationExecutionsResult
ProcessFederationClient::listFederationExecutions() {
  auto response = request(
      TransportServiceOperation::list_federation_executions,
      std::vector<std::uint8_t>{});
  if (response.status != TransportServiceStatus::ok) {
    throw ProcessFederationClientError(
        requestFailure(response.operation, response.status));
  }
  return decodeProcessFederationExecutionsResult(response.payload);
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
  normalizeFomModuleArguments(additionalFomModules);
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
  {
    std::unique_lock lock(transactionMutex_);
    ++callbackDispatchSuppressionDepth_;
    callbackEventsAvailable_.wait(lock, [this] {
      return dispatchDepth_ == 0U ||
          dispatchOwner_ == std::this_thread::get_id();
    });
  }
  auto releaseCallbackDispatchSuppression = [this] {
    {
      std::scoped_lock lock(transactionMutex_);
      --callbackDispatchSuppressionDepth_;
    }
    callbackEventsAvailable_.notify_all();
  };

  TransportServiceMessage response;
  try {
    // A successful Resign terminates this callback lifetime. Do not let the
    // generic request epilogue admit queued callbacks before we can discard
    // the old lifetime's events.
    response = request(
        TransportServiceOperation::resign_federation_execution,
        encodeProcessFederationResignRequest(
            ProcessFederationResignRequest{
                std::move(federationName), federateId, resignAction}),
        false);
  } catch (...) {
    releaseCallbackDispatchSuppression();
    throw;
  }
  if (response.status != TransportServiceStatus::ok) {
    releaseCallbackDispatchSuppression();
    dispatchPendingPushedEvents();
    flushDeferredTsoDeliveryAcknowledgements();
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
      pendingEvents_.clear();
      bridge = callbackBridge_;
    }
  }
  if (bridge) {
    bridge->cancelExceptionReportProjections();
    bridge->cancelDeferredAttributeUpdates();
    bridge->discardPendingCallbacks();
  }
  releaseCallbackDispatchSuppression();
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
  if (response.status == TransportServiceStatus::service_reporting_interlock) {
    throw rti1516_2025::ReportServiceInvocationsAreSubscribed(
        L"The Service Reporting Switch cannot be enabled while the report-service-invocation interaction is subscribed.");
  }
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
          std::move(service), std::move(exception)}),
      false,
      true);
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
          std::move(exception)}),
      false,
      true);
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
