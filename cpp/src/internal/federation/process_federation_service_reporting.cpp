#include "internal/federation/process_federation_service.hpp"
#include "internal/federation/process_federation_service_helpers.hpp"
#include "internal/federation/process_federation_service_payload_helpers.hpp"
#include "internal/federation/transport_service_protocol.hpp"
#include "internal/handles/federate_handle.hpp"

#include "internal/encoding/variable_length_data_2025.hpp"
#include "internal/fom/hla_names.hpp"
#include "internal/observability/mom_service_report_encoding.hpp"

#include <RTI/encoding/BasicDataElements.h>

#include <algorithm>
#include <iterator>
#include <memory>
#include <mutex>
#include <string>
#include <utility>
#include <vector>

namespace umbra::detail {
using process_federation_payload::parameterVector;

TransportServiceMessage
ProcessFederationService::handleGetServiceReportingSwitch(
    ProcessTransportSession& session,
    TransportServiceMessage const& request) {
  auto const switchRequest =
      decodeProcessFederationAttributeScopeAdvisorySwitchRequest(
          request.payload);
  {
    std::scoped_lock lock(mutex_);
    auto const state = sessions_.find(&session);
    if (state == sessions_.end() ||
        !state->second.federationName.has_value() ||
        *state->second.federationName != switchRequest.federationName ||
        state->second.federateId != switchRequest.federateId ||
        !registry_.memberById(
            switchRequest.federationName, switchRequest.federateId)) {
      return rejected(request);
    }
  }
  auto const switchValue = registry_.serviceReportingSwitchFor(
      switchRequest.federationName, switchRequest.federateId);
  if (!switchValue) {
    return rejected(request);
  }
  return responseFor(
      request,
      TransportServiceStatus::ok,
      encodeProcessFederationBooleanResult(
          ProcessFederationBooleanResult{*switchValue}));
}

TransportServiceMessage
ProcessFederationService::handleSetServiceReportingSwitch(
    ProcessTransportSession& session,
    TransportServiceMessage const& request) {
  auto const switchRequest =
      decodeProcessFederationAttributeScopeAdvisorySwitchRequest(
          request.payload);
  {
    std::scoped_lock lock(mutex_);
    auto const state = sessions_.find(&session);
    if (state == sessions_.end() ||
        !state->second.federationName.has_value() ||
        *state->second.federationName != switchRequest.federationName ||
        state->second.federateId != switchRequest.federateId ||
        !registry_.memberById(
            switchRequest.federationName, switchRequest.federateId)) {
      return rejected(request);
    }
  }
  auto const status = registry_.setServiceReportingSwitch(
      switchRequest.federationName,
      switchRequest.federateId,
      switchRequest.switchValue);
  if (status != ServiceReportingSwitchStatus::applied) {
    return rejected(request);
  }
  return responseFor(
      request,
      TransportServiceStatus::ok,
      encodeProcessFederationBooleanResult(
          ProcessFederationBooleanResult{switchRequest.switchValue}));
}

TransportServiceMessage
ProcessFederationService::handleReportServiceException(
    ProcessTransportSession& session,
    TransportServiceMessage const& request) {
  auto const reportRequest =
      decodeProcessFederationServiceExceptionRequest(request.payload);
  {
    std::scoped_lock lock(mutex_);
    auto const state = sessions_.find(&session);
    if (state == sessions_.end() ||
        state->second.federationName != reportRequest.federationName ||
        state->second.federateId != reportRequest.federateId ||
        !registry_.memberById(reportRequest.federationName,
                              reportRequest.federateId)) {
      return rejected(request);
    }
  }
  // The reporting switch is process-owned and can change through MOM as well
  // as the public setter. Never trust a client-side cached switch value.
  auto const report = registry_.planExceptionReport(
      reportRequest.federationName, reportRequest.federateId);
  if (report.status != ExceptionReportStatus::applied) {
    return responseFor(request, TransportServiceStatus::ok, {});
  }
  auto const payload = encodeProcessFederationInteractionEnvelope({
      {{report.routing.federateParameterHandle,
        variable_length_data_2025::copyBytes(
            rti1516_2025::umbra_binding_detail::makeFederateHandle(
                        reportRequest.federateId).encode())},
       {report.routing.serviceParameterHandle,
        variable_length_data_2025::copyBytes(
            rti1516_2025::HLAunicodeString{reportRequest.service}.encode())},
       {report.routing.exceptionParameterHandle,
        variable_length_data_2025::copyBytes(
            rti1516_2025::HLAunicodeString{reportRequest.exception}.encode())}},
      {}});
  std::vector<std::pair<ProcessTransportSession*, ProcessFederationInteractionEvent>>
      pushedEvents;
  {
    std::scoped_lock lock(mutex_);
    for (auto const& recipient : report.recipients) {
      auto const receivingSession = sessionsByFederateId_.find(recipient.federateId);
      if (receivingSession == sessionsByFederateId_.end()) {
        continue;
      }
      auto const state = sessions_.find(receivingSession->second);
      if (state == sessions_.end() ||
          state->second.federationName != reportRequest.federationName) {
        continue;
      }
      ProcessFederationInteractionEvent event;
      event.receivingFederateId = recipient.federateId;
      event.interactionClassHandle = recipient.receivedInteractionClassHandle;
      event.parameterHandles = parameterVector(recipient.receivedParameterHandles);
      event.payload = payload;
      event.transportationName = hla::utf8::mom::reliable;
      event.rtiOwnedMomInteraction = true;
      event.exceptionReportFederateId = reportRequest.federateId;
      if (options_.pushReceiveOrderEvents) {
        pushedEvents.emplace_back(receivingSession->second, std::move(event));
      } else {
        state->second.interactionEvents.push_back(std::move(event));
      }
    }
  }
  for (auto const& [recipient, event] : pushedEvents) {
    if (recipient == nullptr || !recipient->send({
            TransportServiceMessageKind::event,
            TransportServiceOperation::receive_interaction,
            TransportServiceStatus::ok,
            0U,
            encodeProcessFederationReceiveInteractionResult({event})})) {
      return internalError(request);
    }
  }
  return responseFor(request, TransportServiceStatus::ok, {});
}

TransportServiceMessage
ProcessFederationService::handleReportFailedServiceInvocation(
    ProcessTransportSession& session,
    TransportServiceMessage const& request) {
  auto reportRequest =
      decodeProcessFederationFailedServiceInvocationRequest(request.payload);
  ProcessFederationServiceInvocationReport report;
  report.federationName = std::move(reportRequest.federationName);
  report.federateId = reportRequest.federateId;
  report.serviceType = reportRequest.serviceType;
  report.service = std::move(reportRequest.service);
  report.suppliedArguments = std::move(reportRequest.suppliedArguments);
  report.exception = std::move(reportRequest.exception);
  return handleServiceInvocationReport(session, request, std::move(report));
}

TransportServiceMessage
ProcessFederationService::handleReportSuccessfulServiceInvocation(
    ProcessTransportSession& session,
    TransportServiceMessage const& request) {
  auto reportRequest =
      decodeProcessFederationSuccessfulServiceInvocationRequest(request.payload);
  ProcessFederationServiceInvocationReport report;
  report.federationName = std::move(reportRequest.federationName);
  report.federateId = reportRequest.federateId;
  report.serviceType = reportRequest.serviceType;
  report.service = std::move(reportRequest.service);
  report.suppliedArguments = std::move(reportRequest.suppliedArguments);
  report.successful = true;
  report.returnedArgument = std::move(reportRequest.returnedArgument);
  return handleServiceInvocationReport(session, request, std::move(report));
}

TransportServiceMessage
ProcessFederationService::handleReportSuccessfulVoidServiceInvocation(
    ProcessTransportSession& session,
    TransportServiceMessage const& request) {
  auto reportRequest =
      decodeProcessFederationSuccessfulServiceInvocationRequest(request.payload);
  if (reportRequest.returnedArgument.type != MomArgumentType::null_value ||
      !reportRequest.returnedArgument.name.empty() ||
      reportRequest.returnedArgument.value != formatMomNull()) {
    return invalid(request);
  }
  ProcessFederationServiceInvocationReport report;
  report.federationName = std::move(reportRequest.federationName);
  report.federateId = reportRequest.federateId;
  report.serviceType = reportRequest.serviceType;
  report.service = std::move(reportRequest.service);
  report.suppliedArguments = std::move(reportRequest.suppliedArguments);
  report.successful = true;
  report.successfulVoid = true;
  report.returnedArgument = std::move(reportRequest.returnedArgument);
  return handleServiceInvocationReport(session, request, std::move(report));
}

TransportServiceMessage
ProcessFederationService::handleServiceInvocationReport(
    ProcessTransportSession& session,
    TransportServiceMessage const& request,
    ProcessFederationServiceInvocationReport reportRequest) {
  if (reportRequest.federationName.empty() || reportRequest.federateId == 0U ||
      reportRequest.service.empty() ||
      reportRequest.serviceType > MomServiceType::support_services ||
      (reportRequest.successful &&
       (!reportRequest.returnedArgument || !reportRequest.exception.empty())) ||
      (reportRequest.successfulVoid &&
       (!reportRequest.successful || !reportRequest.returnedArgument ||
        reportRequest.returnedArgument->type != MomArgumentType::null_value ||
        !reportRequest.returnedArgument->name.empty() ||
        reportRequest.returnedArgument->value != formatMomNull())) ||
      (!reportRequest.successful &&
       (reportRequest.returnedArgument || reportRequest.exception.empty()))) {
    return invalid(request);
  }
  {
    std::scoped_lock lock(mutex_);
    auto const state = sessions_.find(&session);
    if (state == sessions_.end() ||
        state->second.federationName != reportRequest.federationName ||
        state->second.federateId != reportRequest.federateId ||
        !registry_.memberById(
            reportRequest.federationName, reportRequest.federateId)) {
      return rejected(request);
    }
  }
  auto const serviceGroup =
      static_cast<std::uint16_t>(reportRequest.serviceType);
  auto const plan = registry_.planMomServiceReport(
      reportRequest.federationName, reportRequest.federateId, serviceGroup);
  switch (plan.disposition) {
    case MomServiceReportDisposition::suppressed:
      return responseFor(request, TransportServiceStatus::ok, {});
    case MomServiceReportDisposition::report_to_file:
      break;
    case MomServiceReportDisposition::interaction:
      // No observer means there is no emitted report and therefore no serial
      // reservation.  The registry will re-evaluate subscriptions atomically
      // when the reservation is made below.
      if (plan.recipients.empty()) {
        return responseFor(request, TransportServiceStatus::ok, {});
      }
      break;
    case MomServiceReportDisposition::inconsistent_catalog:
    case MomServiceReportDisposition::reported_federate_not_member:
    case MomServiceReportDisposition::invalid_service_group:
      return internalError(request);
  }

  auto reservation = registry_.reserveMomServiceReport(
      reportRequest.federationName, reportRequest.federateId, serviceGroup);
  if (!reservation.acceptedForEmission) {
    if (reservation.routing.disposition == MomServiceReportDisposition::suppressed ||
        (reservation.routing.disposition ==
             MomServiceReportDisposition::interaction &&
         reservation.routing.recipients.empty())) {
      return responseFor(request, TransportServiceStatus::ok, {});
    }
    return internalError(request);
  }

  if (reservation.routing.disposition ==
      MomServiceReportDisposition::report_to_file) {
    // Honor the reservation's authoritative destination without allocating a
    // second serial through the general append helper.
    std::scoped_lock lock(mutex_);
    auto const state = sessions_.find(&session);
    if (state == sessions_.end() ||
        state->second.federationName != reportRequest.federationName ||
        state->second.federateId != reportRequest.federateId ||
        !state->second.serviceReportWriter) {
      return internalError(request);
    }
    try {
      auto const encodedRecord = reportRequest.successful
          ? reportRequest.successfulVoid
                ? formatMomSuccessfulVoidServiceReportRecord(
                      reservation.serialNumber,
                      reportRequest.service,
                      reportRequest.suppliedArguments)
                : formatMomSuccessfulServiceReportRecord(
                      reservation.serialNumber,
                      reportRequest.service,
                      reportRequest.suppliedArguments,
                      *reportRequest.returnedArgument)
          : formatMomFailedServiceReportRecord(
                reservation.serialNumber,
                reportRequest.service,
                reportRequest.suppliedArguments,
                reportRequest.exception);
      state->second.serviceReportWriter->append(encodedRecord);
    } catch (std::exception const&) {
      return internalError(request);
    }
    return responseFor(request, TransportServiceStatus::ok, {});
  }
  if (reservation.routing.disposition != MomServiceReportDisposition::interaction ||
      reservation.routing.reportParameterHandles.size() != 8U ||
      reservation.serialNumber >
          static_cast<std::uint32_t>(std::numeric_limits<std::int32_t>::max())) {
    return internalError(request);
  }

  auto const returnedArgument = reportRequest.successful
      ? *reportRequest.returnedArgument
      : MomServiceArgument{MomArgumentType::null_value, L"", formatMomNull()};
  auto const encoded = encodeMomServiceInvocation(
      reportRequest.service,
      static_cast<MomServiceType>(reportRequest.serviceType),
      reportRequest.successful,
      reportRequest.suppliedArguments,
      returnedArgument,
      reportRequest.exception,
      static_cast<std::int32_t>(reservation.serialNumber));
  auto const& parameterHandles = reservation.routing.reportParameterHandles;
  auto const interactionPayload = encodeProcessFederationInteractionEnvelope({
      {{parameterHandles[0], variable_length_data_2025::copyBytes(encoded.service)},
       {parameterHandles[1], variable_length_data_2025::copyBytes(encoded.serviceType)},
       {parameterHandles[2], variable_length_data_2025::copyBytes(encoded.successIndicator)},
       {parameterHandles[3], variable_length_data_2025::copyBytes(encoded.suppliedArguments)},
       {parameterHandles[4], variable_length_data_2025::copyBytes(encoded.returnedArgument)},
       {parameterHandles[5], variable_length_data_2025::copyBytes(encoded.exception)},
       {parameterHandles[6], variable_length_data_2025::copyBytes(encoded.serialNumber)},
       {parameterHandles[7], variable_length_data_2025::copyBytes(
            rti1516_2025::umbra_binding_detail::makeFederateHandle(
                reportRequest.federateId).encode())}},
      {}});

  std::vector<std::pair<ProcessTransportSession*, ProcessFederationInteractionEvent>>
      pushedEvents;
  {
    std::scoped_lock lock(mutex_);
    auto const reportingState = sessions_.find(&session);
    if (reportingState == sessions_.end() ||
        reportingState->second.federationName != reportRequest.federationName ||
        reportingState->second.federateId != reportRequest.federateId) {
      return rejected(request);
    }
    for (auto const& recipient : reservation.routing.recipients) {
      auto const receivingSession =
          sessionsByFederateId_.find(recipient.federateId);
      if (receivingSession == sessionsByFederateId_.end()) {
        continue;
      }
      auto const state = sessions_.find(receivingSession->second);
      if (state == sessions_.end() ||
          state->second.federationName != reportRequest.federationName) {
        continue;
      }
      ProcessFederationInteractionEvent event;
      event.receivingFederateId = recipient.federateId;
      event.interactionClassHandle = recipient.receivedInteractionClassHandle;
      event.parameterHandles = parameterVector(recipient.receivedParameterHandles);
      event.payload = interactionPayload;
      event.transportationName = hla::utf8::mom::reliable;
      event.rtiOwnedMomInteraction = true;
      if (options_.pushReceiveOrderEvents) {
        pushedEvents.emplace_back(receivingSession->second, std::move(event));
      } else {
        state->second.interactionEvents.push_back(std::move(event));
      }
    }
  }
  for (auto const& [recipient, event] : pushedEvents) {
    if (recipient == nullptr || !recipient->send({
            TransportServiceMessageKind::event,
            TransportServiceOperation::receive_interaction,
            TransportServiceStatus::ok,
            0U,
            encodeProcessFederationReceiveInteractionResult({event})})) {
      return internalError(request);
    }
  }
  return responseFor(request, TransportServiceStatus::ok, {});
}

TransportServiceMessage
ProcessFederationService::handleRecheckExceptionReport(
    ProcessTransportSession& session,
    TransportServiceMessage const& request) {
  auto const recheck =
      decodeProcessFederationExceptionReportRecheckRequest(request.payload);
  std::scoped_lock lock(mutex_);
  auto const state = sessions_.find(&session);
  if (state == sessions_.end() ||
      state->second.federationName != recheck.federationName ||
      state->second.federateId != recheck.receivingFederateId ||
      !registry_.memberById(recheck.federationName, recheck.receivingFederateId)) {
    return rejected(request);
  }
  ProcessFederationReceiveInteractionResult result;
  auto const projection = registry_.exceptionReportRecipientFor(
      recheck.federationName,
      recheck.reportedFederateId,
      recheck.receivingFederateId);
  if (projection) {
    ProcessFederationInteractionEvent event;
    event.receivingFederateId = recheck.receivingFederateId;
    event.interactionClassHandle = projection->receivedInteractionClassHandle;
    event.parameterHandles = parameterVector(projection->receivedParameterHandles);
    event.transportationName = hla::utf8::mom::reliable;
    event.rtiOwnedMomInteraction = true;
    event.exceptionReportFederateId = recheck.reportedFederateId;
    result.event = std::move(event);
  }
  return responseFor(request, TransportServiceStatus::ok,
                     encodeProcessFederationReceiveInteractionResult(result));
}

TransportServiceMessage
ProcessFederationService::handleGetExceptionReportingSwitch(
    ProcessTransportSession& session,
    TransportServiceMessage const& request) {
  auto const switchRequest =
      decodeProcessFederationAttributeScopeAdvisorySwitchRequest(
          request.payload);
  {
    std::scoped_lock lock(mutex_);
    auto const state = sessions_.find(&session);
    if (state == sessions_.end() ||
        !state->second.federationName.has_value() ||
        *state->second.federationName != switchRequest.federationName ||
        state->second.federateId != switchRequest.federateId ||
        !registry_.memberById(
            switchRequest.federationName, switchRequest.federateId)) {
      return rejected(request);
    }
  }
  auto const availability = registry_.serviceOperationStatus(
      switchRequest.federationName, switchRequest.federateId);
  if (availability != FederationServiceOperationStatus::available) {
    return responseFor(
        request,
        TransportServiceStatus::ok,
        encodeProcessFederationExceptionReportingSwitchResult({availability}));
  }
  auto const switchValue = registry_.exceptionReportingSwitchFor(
      switchRequest.federationName, switchRequest.federateId);
  if (!switchValue) {
    return rejected(request);
  }
  return responseFor(
      request,
      TransportServiceStatus::ok,
      encodeProcessFederationExceptionReportingSwitchResult(
          {FederationServiceOperationStatus::available, *switchValue}));
}

TransportServiceMessage
ProcessFederationService::handleSetExceptionReportingSwitch(
    ProcessTransportSession& session,
    TransportServiceMessage const& request) {
  auto const switchRequest =
      decodeProcessFederationAttributeScopeAdvisorySwitchRequest(
          request.payload);
  {
    std::scoped_lock lock(mutex_);
    auto const state = sessions_.find(&session);
    if (state == sessions_.end() ||
        !state->second.federationName.has_value() ||
        *state->second.federationName != switchRequest.federationName ||
        state->second.federateId != switchRequest.federateId ||
        !registry_.memberById(
            switchRequest.federationName, switchRequest.federateId)) {
      return rejected(request);
    }
  }
  auto const availability = registry_.serviceOperationStatus(
      switchRequest.federationName, switchRequest.federateId);
  if (availability != FederationServiceOperationStatus::available) {
    return responseFor(
        request,
        TransportServiceStatus::ok,
        encodeProcessFederationExceptionReportingSwitchResult({availability}));
  }
  auto const status = registry_.setExceptionReportingSwitch(
      switchRequest.federationName,
      switchRequest.federateId,
      switchRequest.switchValue);
  if (status != FederationRegistryStatus::applied) {
    return rejected(request);
  }
  if (!enqueueJoinedFederateMomConditionalAttributeUpdate(
          switchRequest.federationName,
          switchRequest.federateId,
          {std::string(hla::utf8::mom::exception_reporting)})) {
    return internalError(request);
  }
  if (!appendSelectedServiceReportRecord(
          session,
          switchRequest.federationName,
          switchRequest.federateId,
          static_cast<std::uint16_t>(MomServiceType::support_services),
          [switchValue = switchRequest.switchValue](std::uint32_t serialNumber) {
            return formatMomSuccessfulVoidServiceReportRecord(
                serialNumber,
                L"SetExceptionReportingSwitch",
                {{MomArgumentType::boolean,
                  L"SwitchValue",
                  formatMomBoolean(switchValue)}});
          })) {
    return internalError(request);
  }
  return responseFor(
      request,
      TransportServiceStatus::ok,
      encodeProcessFederationExceptionReportingSwitchResult(
          {FederationServiceOperationStatus::available, switchRequest.switchValue}));
}

TransportServiceMessage
ProcessFederationService::handleGetSendServiceReportsToFileSwitch(
    ProcessTransportSession& session,
    TransportServiceMessage const& request) {
  auto const switchRequest =
      decodeProcessFederationAttributeScopeAdvisorySwitchRequest(
          request.payload);
  {
    std::scoped_lock lock(mutex_);
    auto const state = sessions_.find(&session);
    if (state == sessions_.end() ||
        !state->second.federationName.has_value() ||
        *state->second.federationName != switchRequest.federationName ||
        state->second.federateId != switchRequest.federateId ||
        !registry_.memberById(
            switchRequest.federationName, switchRequest.federateId)) {
      return rejected(request);
    }
  }
  auto const switchValue = registry_.sendServiceReportsToFileSwitchFor(
      switchRequest.federationName, switchRequest.federateId);
  if (!switchValue) {
    return rejected(request);
  }
  return responseFor(
      request,
      TransportServiceStatus::ok,
      encodeProcessFederationBooleanResult(
          ProcessFederationBooleanResult{*switchValue}));
}

TransportServiceMessage
ProcessFederationService::handleSetSendServiceReportsToFileSwitch(
    ProcessTransportSession& session,
    TransportServiceMessage const& request) {
  auto const switchRequest =
      decodeProcessFederationAttributeScopeAdvisorySwitchRequest(
          request.payload);
  {
    std::scoped_lock lock(mutex_);
    auto const state = sessions_.find(&session);
    if (state == sessions_.end() ||
        !state->second.federationName.has_value() ||
        *state->second.federationName != switchRequest.federationName ||
        state->second.federateId != switchRequest.federateId ||
        !registry_.memberById(
            switchRequest.federationName, switchRequest.federateId)) {
      return rejected(request);
    }
  }
  auto const status = registry_.setSendServiceReportsToFileSwitch(
      switchRequest.federationName,
      switchRequest.federateId,
      switchRequest.switchValue);
  if (status != FederationRegistryStatus::applied) {
    return rejected(request);
  }
  return responseFor(
      request,
      TransportServiceStatus::ok,
      encodeProcessFederationBooleanResult(
          ProcessFederationBooleanResult{switchRequest.switchValue}));
}

}  // namespace umbra::detail
