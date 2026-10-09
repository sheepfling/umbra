#include "internal/runtime/ambassador_shared_utilities.hpp"
#include "internal/handles/federate_handle.hpp"
#include "internal/fom/hla_names.hpp"
#include "internal/runtime/ambassador_string_helpers.hpp"
#include "internal/runtime/umbra_rti_ambassador.hpp"

#include <exception>
#include <limits>
#include <mutex>
#include <utility>
#include <vector>

namespace rti1516_2025::umbra_binding_detail {

#if defined(UMBRA_ENABLE_EMBEDDED_FEDERATION_MANAGEMENT)


bool UmbraRtiAmbassador::emitSelectedMomServiceReportInteraction(
    std::wstring const& service,
    umbra::detail::MomServiceType serviceType,
    std::vector<umbra::detail::MomServiceArgument> const& suppliedArguments,
    umbra::detail::MomServiceArgument const& returnedArgument,
    bool success,
    std::wstring const& exception) const {
  if (!joinedFederationName_ || !joinedFederateId_ || !joinedServiceReport_ ||
      !joinedServiceReport_->endpoint || !joinedServiceReport_->endpoint->writer) {
    throw RTIinternalError(
        L"Umbra is missing the joined-federate service-report file state.");
  }

  auto& registry = embeddedFederationRegistry();
  auto const plan = registry.planMomServiceReport(
      *joinedFederationName_,
      *joinedFederateId_,
      static_cast<std::uint16_t>(serviceType));
  if (plan.disposition != umbra::detail::MomServiceReportDisposition::interaction ||
      plan.recipients.empty()) {
    // Interaction reporting is best-effort when the service-report switch is
    // on but no joined federate subscribes to HLAreportServiceInvocation.
    // In particular, disabling file output must not make the RTI service fail
    // merely because there is no interaction recipient.
    return false;
  }

  auto reservation = registry.reserveMomServiceReport(
      *joinedFederationName_,
      *joinedFederateId_,
      static_cast<std::uint16_t>(serviceType));
  if (!reservation.acceptedForEmission ||
      reservation.routing.disposition !=
          umbra::detail::MomServiceReportDisposition::interaction ||
      reservation.routing.reportParameterHandles.size() != 8U) {
    throw RTIinternalError(
        L"Umbra could not reserve the selected service-report interaction.");
  }
  if (reservation.serialNumber >
      static_cast<std::uint32_t>(std::numeric_limits<std::int32_t>::max())) {
    throw RTIinternalError(
        L"Umbra exhausted the signed HLAreportServiceInvocation serial range.");
  }

  auto const encoded = umbra::detail::encodeMomServiceInvocation(
      service,
      serviceType,
      success,
      suppliedArguments,
      returnedArgument,
      exception,
      static_cast<std::int32_t>(reservation.serialNumber));
  std::vector<AmbassadorInteractionParameterValue> reportParameters;
  reportParameters.reserve(reservation.routing.reportParameterHandles.size());
  reportParameters.emplace_back(
      reservation.routing.reportParameterHandles[0], encoded.service);
  reportParameters.emplace_back(
      reservation.routing.reportParameterHandles[1], encoded.serviceType);
  reportParameters.emplace_back(
      reservation.routing.reportParameterHandles[2], encoded.successIndicator);
  reportParameters.emplace_back(
      reservation.routing.reportParameterHandles[3], encoded.suppliedArguments);
  reportParameters.emplace_back(
      reservation.routing.reportParameterHandles[4], encoded.returnedArgument);
  reportParameters.emplace_back(
      reservation.routing.reportParameterHandles[5], encoded.exception);
  reportParameters.emplace_back(
      reservation.routing.reportParameterHandles[6], encoded.serialNumber);
  reportParameters.emplace_back(
      reservation.routing.reportParameterHandles[7],
      makeFederateHandle(*joinedFederateId_).encode());
  auto const reliableTransportation = ambassadorTransportationHandleFromEmbeddedName(
      umbra::detail::hla::utf8::mom::reliable,
      L"The embedded federation could not reconstruct HLAreportServiceInvocation transportation.");
  // HLA_IMMEDIATE may enter the observer's Java/Python callback before this
  // helper returns. The caller deliberately invokes this helper without its
  // report-file mutex held.
  queueAmbassadorMomServiceReportInteraction(
      *joinedFederationName_,
      *joinedFederateId_,
      static_cast<std::uint16_t>(serviceType),
      std::move(reservation),
      std::move(reportParameters),
      reliableTransportation);
  return true;
}

void UmbraRtiAmbassador::requireFederationServiceOperationAvailable(
    std::wstring const& operation) const {
  auto instrumentationScope = beginRtiCall("requireFederationServiceOperationAvailable");
  if (!joinedFederationName_ || !joinedFederateId_) {
    throw FederateNotExecutionMember(
        operation + L" requires membership in a federation execution.");
  }

  auto const status = embeddedFederationRegistry().serviceOperationStatus(
      *joinedFederationName_,
      *joinedFederateId_);
  switch (status) {
    case umbra::detail::FederationServiceOperationStatus::available:
      return;
    case umbra::detail::FederationServiceOperationStatus::federation_does_not_exist:
    case umbra::detail::FederationServiceOperationStatus::federate_not_member:
      throw FederateNotExecutionMember(
          operation + L" cannot proceed because this federate is not an execution member.");
    case umbra::detail::FederationServiceOperationStatus::save_in_progress:
      throw SaveInProgress(operation + L" is unavailable during federation save.");
    case umbra::detail::FederationServiceOperationStatus::restore_in_progress:
      throw RestoreInProgress(operation + L" is unavailable during federation restore.");
  }
  throw RTIinternalError(operation + L" encountered an unknown federation operation state.");
}

void UmbraRtiAmbassador::appendSuccessfulVoidServiceReportToFileIfSelected(
    std::wstring const& service,
    umbra::detail::MomServiceType serviceType,
    std::vector<umbra::detail::MomServiceArgument> const& suppliedArguments,
    bool emitInteraction) const {
  if (!joinedFederationName_ || !joinedFederateId_) {
    throw RTIinternalError(
        L"Umbra is missing the joined-federate service-report identity.");
  }
  if (processEndpointActive_) {
    if (!processFederationClient_) {
      throw RTIinternalError(
          L"Umbra is missing the process service-report connection.");
    }
    try {
      processFederationClient_->reportSuccessfulVoidServiceInvocation(
          *joinedFederationName_,
          *joinedFederateId_,
          serviceType,
          service,
          suppliedArguments);
    } catch (umbra::detail::ProcessFederationClientError const& error) {
      throw RTIinternalError(wideAscii(error.what()));
    }
    return;
  }

  // Embedded file-selected service wrappers may call this after their service
  // transaction has completed. Interaction-selected wrappers must opt in only
  // after native planning locks have been released; that route may submit the
  // ordinary receive-order callback without re-entering a held federation lock.
  if (!joinedServiceReport_ ||
      !joinedServiceReport_->endpoint || !joinedServiceReport_->endpoint->writer) {
    throw RTIinternalError(
        L"Umbra is missing the joined-federate service-report file state.");
  }

  if (emitInteraction && emitSelectedMomServiceReportInteraction(
      service,
      serviceType,
      suppliedArguments,
      {umbra::detail::MomArgumentType::null_value,
       L"",
       umbra::detail::formatMomNull()})) {
    return;
  }
  auto const endpoint = joinedServiceReport_->endpoint;
  std::scoped_lock reportLock(endpoint->mutex);
  appendAmbassadorSelectedServiceReportRecord(
      *endpoint,
      *joinedFederationName_,
      *joinedFederateId_,
      static_cast<std::uint16_t>(serviceType),
      [&service, &suppliedArguments](std::uint32_t serialNumber) {
        return umbra::detail::formatMomSuccessfulVoidServiceReportRecord(
            serialNumber,
            service,
            suppliedArguments);
      });
}
void UmbraRtiAmbassador::appendSuccessfulServiceReportToFileIfSelected(
    std::wstring const& service,
    umbra::detail::MomServiceType serviceType,
    std::vector<umbra::detail::MomServiceArgument> const& suppliedArguments,
    umbra::detail::MomServiceArgument const& returnedArgument,
    bool emitInteraction) const {
  if (!joinedFederationName_ || !joinedFederateId_) {
    throw RTIinternalError(
        L"Umbra is missing the joined-federate service-report identity.");
  }
  if (processEndpointActive_) {
    if (!processFederationClient_) {
      throw RTIinternalError(
          L"Umbra is missing the process service-report connection.");
    }
    try {
      processFederationClient_->reportSuccessfulServiceInvocation(
          *joinedFederationName_,
          *joinedFederateId_,
          serviceType,
          service,
          suppliedArguments,
          returnedArgument);
    } catch (umbra::detail::ProcessFederationClientError const& error) {
      throw RTIinternalError(wideAscii(error.what()));
    }
    return;
  }
  if (!joinedServiceReport_ || !joinedServiceReport_->endpoint ||
      !joinedServiceReport_->endpoint->writer) {
    throw RTIinternalError(
        L"Umbra is missing the joined-federate service-report file state.");
  }

  // Non-void service wrappers are called after their public state locks have
  // been released. Keep the interaction path outside the file endpoint lock so
  // HLA_IMMEDIATE observers can safely re-enter the ambassador.
  if (emitInteraction && emitSelectedMomServiceReportInteraction(
      service,
      serviceType,
      suppliedArguments,
      returnedArgument)) {
    return;
  }

  auto const endpoint = joinedServiceReport_->endpoint;
  std::scoped_lock reportLock(ambassadorFederationManagementMutex(), endpoint->mutex);
  appendAmbassadorSelectedServiceReportRecord(
      *endpoint,
      *joinedFederationName_,
      *joinedFederateId_,
      static_cast<std::uint16_t>(serviceType),
      [&service, &suppliedArguments, &returnedArgument](std::uint32_t serialNumber) {
        return umbra::detail::formatMomSuccessfulServiceReportRecord(
            serialNumber,
            service,
            suppliedArguments,
             returnedArgument);
       });
}

void UmbraRtiAmbassador::appendFailedServiceReportToFileIfSelected(
    std::wstring const& service,
    umbra::detail::MomServiceType serviceType,
    std::vector<umbra::detail::MomServiceArgument> const& suppliedArguments,
    std::wstring const& exception,
    bool emitInteraction) const {
  // A public service can fail before a federate has joined. §11.5 reporting is
  // scoped to a joined federate, so preserve the original NotConnected or
  // FederateNotExecutionMember exception rather than manufacturing an RTI
  // service-report state error for that pre-join path.
  if (lifecycle_.state() != umbra::detail::FederateLifecycleState::joined ||
      !joinedFederationName_ || !joinedFederateId_) {
    return;
  }
  if (processEndpointActive_) {
    if (!processFederationClient_) {
      throw RTIinternalError(
          L"Umbra is missing the process service-report connection.");
    }
    try {
      processFederationClient_->reportFailedServiceInvocation(
          *joinedFederationName_,
          *joinedFederateId_,
          serviceType,
          service,
          suppliedArguments,
          exception);
    } catch (umbra::detail::ProcessFederationClientError const& error) {
      throw RTIinternalError(wideAscii(error.what()));
    }
    return;
  }
  {
    std::scoped_lock federationLock(ambassadorFederationManagementMutex());
    if (!embeddedFederationRegistry().memberById(
            *joinedFederationName_,
            *joinedFederateId_)) {
      // A concurrent resignation can leave the public wrapper's lifecycle
      // observation briefly ahead of the registry membership transaction. The
      // failed service still belongs to the original public exception; it is
      // no longer a reportable joined-federate invocation.
      return;
    }
  }
  if (!joinedServiceReport_ || !joinedServiceReport_->endpoint ||
      !joinedServiceReport_->endpoint->writer) {
    throw RTIinternalError(
        L"Umbra is missing the joined-federate service-report file state.");
  }

  auto const nullReturnedArgument = umbra::detail::MomServiceArgument{
      umbra::detail::MomArgumentType::null_value,
      L"",
      umbra::detail::formatMomNull()};
  // Failed non-void calls are reported through the same interaction path as
  // successful non-void calls. The interaction encoder receives the Null
  // return and the exception text required by §11.5.1.
  if (emitInteraction && emitSelectedMomServiceReportInteraction(
      service,
      serviceType,
      suppliedArguments,
      nullReturnedArgument,
      false,
      exception)) {
    return;
  }

  auto const endpoint = joinedServiceReport_->endpoint;
  std::scoped_lock reportLock(ambassadorFederationManagementMutex(), endpoint->mutex);
  appendAmbassadorSelectedServiceReportRecord(
      *endpoint,
      *joinedFederationName_,
      *joinedFederateId_,
      static_cast<std::uint16_t>(serviceType),
      [&service, &suppliedArguments, &exception](std::uint32_t serialNumber) {
        return umbra::detail::formatMomFailedServiceReportRecord(
            serialNumber,
            service,
            suppliedArguments,
            exception);
      });
}

void UmbraRtiAmbassador::appendReservedSuccessfulVoidServiceReportToFile(
    std::uint32_t serialNumber,
    std::wstring const& service,
    std::vector<umbra::detail::MomServiceArgument> const& suppliedArguments) const {
  if (!joinedServiceReport_ || !joinedServiceReport_->endpoint ||
      !joinedServiceReport_->endpoint->writer) {
    throw RTIinternalError(
        L"Umbra is missing the joined-federate service-report file state.");
  }

  auto const endpoint = joinedServiceReport_->endpoint;
  std::scoped_lock reportLock(endpoint->mutex);
  try {
    endpoint->writer->append(
        umbra::detail::formatMomSuccessfulVoidServiceReportRecord(
            serialNumber, service, suppliedArguments));
  } catch (std::exception const&) {
    // Reporting to a configured file is normative embedded-profile behavior;
    // do not replace this writer or silently redirect the record to memory.
    throw RTIinternalError(
        L"Umbra could not append the selected service-report file record.");
  }
}

#endif

}  // namespace rti1516_2025::umbra_binding_detail
