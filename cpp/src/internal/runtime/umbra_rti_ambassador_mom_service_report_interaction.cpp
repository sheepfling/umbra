#include "internal/runtime/umbra_rti_ambassador.hpp"

#include "internal/fom/hla_names.hpp"
#include "internal/handles/federate_handle.hpp"
#include "internal/handles/interaction_class_handle.hpp"
#include "internal/handles/transportation_type_handle.hpp"
#include "internal/observability/mom_service_report_encoding.hpp"
#include "internal/runtime/ambassador_shared_utilities.hpp"

#include <RTI/FederateAmbassador.h>
#include <RTI/encoding/BasicDataElements.h>

#include <cstdint>
#include <limits>
#include <mutex>
#include <optional>
#include <string>
#include <utility>
#include <vector>

namespace rti1516_2025::umbra_binding_detail {

#if defined(UMBRA_ENABLE_EMBEDDED_FEDERATION_MANAGEMENT)

namespace {

// HLAreportException is selected by the failing member's Exception Reporting
// Switch. It remains distinct from HLAreportMOMexception, which reports a
// rejected MOM interaction independently of that switch.
void queueExceptionReport(
    std::wstring federationName,
    umbra::detail::ExceptionReportPlan report,
    std::wstring service,
    std::wstring exception) {
  if (report.status != umbra::detail::ExceptionReportStatus::applied ||
      report.reportedFederateId == 0U ||
      report.routing.interactionClassHandle == 0U ||
      report.routing.federateParameterHandle == 0U ||
      report.routing.serviceParameterHandle == 0U ||
      report.routing.exceptionParameterHandle == 0U ||
      report.recipients.empty()) {
    return;
  }

  std::vector<AmbassadorInteractionParameterValue> sentParameters;
  sentParameters.reserve(3U);
  sentParameters.emplace_back(
      report.routing.federateParameterHandle,
      makeFederateHandle(report.reportedFederateId).encode());
  sentParameters.emplace_back(
      report.routing.serviceParameterHandle,
      HLAunicodeString{service}.encode());
  sentParameters.emplace_back(
      report.routing.exceptionParameterHandle,
      HLAunicodeString{exception}.encode());
  auto const reliableTransportation = ambassadorTransportationHandleFromEmbeddedName(
      umbra::detail::hla::utf8::mom::reliable,
      L"The embedded federation could not reconstruct HLAreportException transportation.");

  for (auto &plannedRecipient : report.recipients) {
    if (!plannedRecipient.callbackRoute || plannedRecipient.federateId == 0U) {
      continue;
    }
    auto callbackRoute = std::move(plannedRecipient.callbackRoute);
    auto const receivingFederateId = plannedRecipient.federateId;
    submitAmbassadorReceiveOrderCallback(
        std::move(callbackRoute),
        federationName,
        receivingFederateId,
        [
            federationName,
            reportedFederateId = report.reportedFederateId,
            receivingFederateId,
            sentParameters,
            reliableTransportation](FederateAmbassador &recipient) mutable {
      std::optional<umbra::detail::ReceiveOrderInteractionRecipient> projection;
      {
        std::scoped_lock lock(ambassadorFederationManagementMutex());
        projection = embeddedFederationRegistry().exceptionReportRecipientFor(
            federationName,
            reportedFederateId,
            receivingFederateId);
      }
      if (!projection) {
        return;
      }

      auto const parameterValues = ambassadorProjectInteractionParameterValues(
          sentParameters,
          projection->receivedParameterHandles);
      recipient.receiveInteraction(
          makeInteractionClassHandle(projection->receivedInteractionClassHandle),
          parameterValues,
          VariableLengthData{},
          reliableTransportation,
          FederateHandle{},
          nullptr);
    });
  }
}

// HLAreportMOMexception reports a rejected MOM interaction, not a generic
// exception selected by the Exception Reporting Switch. Keep its additional
// parameter-error bit in its own payload path.
void queueMomExceptionReport(
    std::wstring federationName,
    umbra::detail::MomExceptionReportPlan report,
    std::wstring service,
    std::wstring exception,
    bool parameterError) {
  if (report.status != umbra::detail::MomExceptionReportStatus::applied ||
      report.reportedFederateId == 0U ||
      report.routing.interactionClassHandle == 0U ||
      report.routing.federateParameterHandle == 0U ||
      report.routing.serviceParameterHandle == 0U ||
      report.routing.exceptionParameterHandle == 0U ||
      report.routing.parameterErrorParameterHandle == 0U ||
      report.recipients.empty()) {
    return;
  }

  std::vector<AmbassadorInteractionParameterValue> sentParameters;
  sentParameters.reserve(4U);
  sentParameters.emplace_back(
      report.routing.federateParameterHandle,
      makeFederateHandle(report.reportedFederateId).encode());
  sentParameters.emplace_back(
      report.routing.serviceParameterHandle,
      HLAunicodeString{service}.encode());
  sentParameters.emplace_back(
      report.routing.exceptionParameterHandle,
      HLAunicodeString{exception}.encode());
  sentParameters.emplace_back(
      report.routing.parameterErrorParameterHandle,
      HLAboolean{parameterError}.encode());
  auto const reliableTransportation = ambassadorTransportationHandleFromEmbeddedName(
      umbra::detail::hla::utf8::mom::reliable,
      L"The embedded federation could not reconstruct HLAreportMOMexception transportation.");

  for (auto &plannedRecipient : report.recipients) {
    if (!plannedRecipient.callbackRoute || plannedRecipient.federateId == 0U) {
      continue;
    }
    auto callbackRoute = std::move(plannedRecipient.callbackRoute);
    auto const receivingFederateId = plannedRecipient.federateId;
    submitAmbassadorReceiveOrderCallback(
        std::move(callbackRoute),
        federationName,
        receivingFederateId,
        [
            federationName,
            reportedFederateId = report.reportedFederateId,
            receivingFederateId,
            sentParameters,
            reliableTransportation](FederateAmbassador &recipient) mutable {
      std::optional<umbra::detail::ReceiveOrderInteractionRecipient> projection;
      {
        std::scoped_lock lock(ambassadorFederationManagementMutex());
        projection = embeddedFederationRegistry().momExceptionReportRecipientFor(
            federationName,
            reportedFederateId,
            receivingFederateId);
      }
      if (!projection) {
        return;
      }

      auto const parameterValues = ambassadorProjectInteractionParameterValues(
          sentParameters,
          projection->receivedParameterHandles);
      recipient.receiveInteraction(
          makeInteractionClassHandle(projection->receivedInteractionClassHandle),
          parameterValues,
          VariableLengthData{},
          reliableTransportation,
          FederateHandle{},
          nullptr);
    });
  }
}

}  // namespace

void UmbraRtiAmbassador::emitExceptionReport(
    std::wstring const &service,
    rti1516_2025::Exception const &exception) const noexcept {
  try {
    std::optional<std::wstring> federationName;
    std::optional<std::uint64_t> reportedFederateId;
    umbra::detail::ProcessFederationClient *processClient = nullptr;
    {
      std::scoped_lock lock(mutex_, ambassadorFederationManagementMutex());
      if (lifecycle_.state() != umbra::detail::FederateLifecycleState::joined ||
          !joinedFederationName_ || !joinedFederateId_) {
        return;
      }
      federationName = *joinedFederationName_;
      reportedFederateId = *joinedFederateId_;
      if (processEndpointActive_) {
        processClient = processFederationClient_.get();
        if (processClient == nullptr) {
          return;
        }
      }
    }

    auto exceptionText = exception.name();
    auto const detail = exception.what();
    if (!detail.empty()) {
      exceptionText += L": ";
      exceptionText += detail;
    }
    if (processClient != nullptr) {
      processClient->reportServiceException(
          *federationName, *reportedFederateId, service, std::move(exceptionText));
      return;
    }
    auto const report = embeddedFederationRegistry().planExceptionReport(
        *federationName,
        *reportedFederateId);
    if (report.status != umbra::detail::ExceptionReportStatus::applied) {
      return;
    }
    queueExceptionReport(
        *federationName,
        report,
        service,
        std::move(exceptionText));
  } catch (...) {
    // Exception reporting is advisory traffic. Never replace the original
    // C++ service exception with a routing/catalog/callback failure.
  }
}

void UmbraRtiAmbassador::emitMomExceptionReport(
    std::wstring const &service,
    rti1516_2025::Exception const &exception,
    bool parameterError) const noexcept {
  try {
    std::optional<std::wstring> federationName;
    std::optional<std::uint64_t> reportedFederateId;
    {
      std::scoped_lock lock(mutex_, ambassadorFederationManagementMutex());
      if (lifecycle_.state() != umbra::detail::FederateLifecycleState::joined ||
          !joinedFederationName_ || !joinedFederateId_) {
        return;
      }
      federationName = *joinedFederationName_;
      reportedFederateId = *joinedFederateId_;
    }

    auto const report = embeddedFederationRegistry().planMomExceptionReport(
        *federationName,
        *reportedFederateId);
    if (report.status != umbra::detail::MomExceptionReportStatus::applied) {
      return;
    }
    auto exceptionText = exception.name();
    auto const detail = exception.what();
    if (!detail.empty()) {
      exceptionText += L": ";
      exceptionText += detail;
    }
    queueMomExceptionReport(
        *federationName,
        report,
        service,
        std::move(exceptionText),
        parameterError);
  } catch (...) {
    // MOM exception reporting is advisory traffic. Never replace the
    // original public Send Interaction exception with a routing failure.
  }
}

void queueAmbassadorMomServiceReportInteraction(
    std::wstring federationName,
    std::uint64_t reportedFederateId,
    std::uint16_t serviceGroup,
    umbra::detail::ReservedMomServiceReport reservation,
    std::vector<AmbassadorInteractionParameterValue> sentParameters,
    TransportationTypeHandle transportationType,
    bool allowRemovedReportedFederate) {
  if (!reservation.acceptedForEmission ||
      reservation.routing.disposition !=
          umbra::detail::MomServiceReportDisposition::interaction ||
      reservation.routing.interactionClassHandle == 0U ||
      reservation.routing.recipients.empty()) {
    return;
  }

  for (auto& plannedRecipient : reservation.routing.recipients) {
    if (!plannedRecipient.callbackRoute || plannedRecipient.federateId == 0U) {
      continue;
    }
    auto callbackRoute = std::move(plannedRecipient.callbackRoute);
    auto const receivingFederateId = plannedRecipient.federateId;
    auto const receivedInteractionClassHandle =
        plannedRecipient.receivedInteractionClassHandle;
    auto receivedParameterHandles = plannedRecipient.receivedParameterHandles;
    submitAmbassadorReceiveOrderCallback(
        std::move(callbackRoute),
        federationName,
        receivingFederateId,
        [federationName,
         reportedFederateId,
         receivingFederateId,
         serviceGroup,
         interactionClassHandle = reservation.routing.interactionClassHandle,
         receivedInteractionClassHandle,
         receivedParameterHandles = std::move(receivedParameterHandles),
         sentParameters,
         transportationType,
         allowRemovedReportedFederate](FederateAmbassador& recipient) mutable {
      if (allowRemovedReportedFederate) {
        auto const parameterValues = ambassadorProjectInteractionParameterValues(
            sentParameters,
            receivedParameterHandles);
        recipient.receiveInteraction(
            makeInteractionClassHandle(receivedInteractionClassHandle),
            parameterValues,
            VariableLengthData{},
            transportationType,
            FederateHandle{},
            nullptr);
        return;
      }

      std::optional<umbra::detail::ReceiveOrderInteractionRecipient> projection;
      {
        std::scoped_lock lock(ambassadorFederationManagementMutex());
        projection = embeddedFederationRegistry().momServiceReportRecipientFor(
            federationName,
            reportedFederateId,
            receivingFederateId,
            serviceGroup);
      }
      if (!projection) {
        return;
      }

      auto const parameterValues = ambassadorProjectInteractionParameterValues(
          sentParameters,
          projection->receivedParameterHandles);
      recipient.receiveInteraction(
          makeInteractionClassHandle(projection->receivedInteractionClassHandle),
          parameterValues,
          VariableLengthData{},
          transportationType,
          FederateHandle{},
          nullptr);
    });
  }
}

// RTI-initiated restore notifications use the same public service-report
// encoder as direct service wrappers, but originate from a recipient route
// rather than a live ambassador method. Keep reservation and callback routing
// in this free helper so the registry can carry a structured public endpoint
// without introducing a second Java/Python state model.
bool emitAmbassadorSelectedMomServiceReportInteractionForFederate(
    std::wstring const& federationName,
    std::uint64_t reportedFederateId,
    std::wstring const& service,
    umbra::detail::MomServiceType serviceType,
    std::vector<umbra::detail::MomServiceArgument> const& suppliedArguments,
    umbra::detail::MomServiceArgument const& returnedArgument,
    bool success,
    std::wstring const& exception) {
  auto& registry = embeddedFederationRegistry();
  auto const plan = registry.planMomServiceReport(
      federationName,
      reportedFederateId,
      static_cast<std::uint16_t>(serviceType));
  if (plan.disposition != umbra::detail::MomServiceReportDisposition::interaction) {
    return false;
  }
  // A service-report interaction is only an emitted report when the planner
  // found at least one eligible recipient.  In particular, disabling the
  // file sink without subscribing an observer must not consume a serial that
  // no sink can observe.
  if (plan.recipients.empty()) {
    return false;
  }

  auto reservation = registry.reserveMomServiceReport(
      federationName,
      reportedFederateId,
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
      makeFederateHandle(reportedFederateId).encode());
  auto const reliableTransportation = ambassadorTransportationHandleFromEmbeddedName(
      umbra::detail::hla::utf8::mom::reliable,
      L"The embedded federation could not reconstruct HLAreportServiceInvocation transportation.");
  queueAmbassadorMomServiceReportInteraction(
      federationName,
      reportedFederateId,
      static_cast<std::uint16_t>(serviceType),
      std::move(reservation),
      std::move(reportParameters),
      reliableTransportation);
  return true;
}

#endif

}  // namespace rti1516_2025::umbra_binding_detail
