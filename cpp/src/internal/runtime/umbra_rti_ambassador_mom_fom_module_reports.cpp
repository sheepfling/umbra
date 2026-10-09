#include "internal/handles/federate_handle.hpp"
#include "internal/handles/interaction_class_handle.hpp"
#include "internal/fom/hla_names.hpp"
#include "internal/runtime/ambassador_shared_utilities.hpp"
#include "internal/runtime/umbra_rti_ambassador.hpp"

#include <RTI/encoding/BasicDataElements.h>

#include <cstddef>
#include <cstdint>
#include <mutex>
#include <optional>
#include <utility>
#include <vector>

namespace rti1516_2025::umbra_binding_detail {

#if defined(UMBRA_ENABLE_EMBEDDED_FEDERATION_MANAGEMENT)
namespace {

template <typename RecipientResolver>
void submitMomFomDataReport(
    std::wstring const& federationName,
    std::uint64_t reportedFederateId,
    std::vector<umbra::detail::ReceiveOrderInteractionRecipient> const& plannedRecipients,
    std::vector<AmbassadorInteractionParameterValue> sentParameters,
    TransportationTypeHandle reliableTransportation,
    RecipientResolver resolveRecipient) {
  for (auto const& plannedRecipient : plannedRecipients) {
    if (!plannedRecipient.callbackRoute || plannedRecipient.federateId == 0U) {
      continue;
    }
    auto callbackRoute = plannedRecipient.callbackRoute;
    auto const receivingFederateId = plannedRecipient.federateId;
    submitAmbassadorReceiveOrderCallback(
        std::move(callbackRoute),
        federationName,
        receivingFederateId,
        [federationName,
         reportedFederateId,
         receivingFederateId,
         sentParameters,
         reliableTransportation,
         resolveRecipient](FederateAmbassador& recipient) mutable {
          std::optional<umbra::detail::ReceiveOrderInteractionRecipient> projection;
          {
            std::scoped_lock lock(ambassadorFederationManagementMutex());
            projection = resolveRecipient(
                embeddedFederationRegistry(),
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

std::vector<AmbassadorInteractionParameterValue> encodeFomModuleDataParameters(
    std::uint64_t moduleIndicatorParameterHandle,
    std::uint64_t moduleDataParameterHandle,
    std::size_t moduleIndex,
    std::wstring const& moduleData,
    wchar_t const* errorContext) {
  VariableLengthData encodedIndicator;
  VariableLengthData encodedModuleData;
  try {
    encodedIndicator = HLAinteger32BE{static_cast<Integer32>(moduleIndex)}.encode();
    encodedModuleData = HLAunicodeString{moduleData}.encode();
  } catch (Exception const&) {
    throw;
  } catch (...) {
    throw RTIinternalError(errorContext);
  }
  return {
      {moduleIndicatorParameterHandle, std::move(encodedIndicator)},
      {moduleDataParameterHandle, std::move(encodedModuleData)},
  };
}

}  // namespace

void queueMomFomModuleDataReport(
    std::wstring federationName,
    umbra::detail::MomFomModuleDataReportPlan report) {
  if (report.status != umbra::detail::MomFomModuleDataReportStatus::applied ||
      report.routing.interactionClassHandle == 0U ||
      report.routing.moduleIndicatorParameterHandle == 0U ||
      report.routing.moduleDataParameterHandle == 0U ||
      report.recipients.empty()) {
    return;
  }
  auto sentParameters = encodeFomModuleDataParameters(
      report.routing.moduleIndicatorParameterHandle,
      report.routing.moduleDataParameterHandle,
      report.moduleIndex,
      report.moduleData,
      L"The embedded federation could not encode HLAreportFOMmoduleData values.");
  auto const reliableTransportation = ambassadorTransportationHandleFromEmbeddedName(
      umbra::detail::hla::utf8::mom::reliable,
      L"The embedded federation could not reconstruct HLAreportFOMmoduleData transportation.");
  submitMomFomDataReport(
      federationName,
      report.reportedFederateId,
      report.recipients,
      std::move(sentParameters),
      reliableTransportation,
      [](auto& registry, auto const& federation, auto reportedId, auto receivingId) {
        return registry.momFomModuleDataReportRecipientFor(
            federation, reportedId, receivingId);
      });
}

void queueMomFederationFomModuleDataReport(
    std::wstring federationName,
    umbra::detail::MomFederationFomModuleDataReportPlan report) {
  if (report.status !=
          umbra::detail::MomFederationFomModuleDataReportStatus::applied ||
      report.routing.interactionClassHandle == 0U ||
      report.routing.moduleIndicatorParameterHandle == 0U ||
      report.routing.moduleDataParameterHandle == 0U ||
      report.recipients.empty()) {
    return;
  }
  auto sentParameters = encodeFomModuleDataParameters(
      report.routing.moduleIndicatorParameterHandle,
      report.routing.moduleDataParameterHandle,
      report.moduleIndex,
      report.moduleData,
      L"The embedded federation could not encode federation HLAreportFOMmoduleData values.");
  auto const reliableTransportation = ambassadorTransportationHandleFromEmbeddedName(
      umbra::detail::hla::utf8::mom::reliable,
      L"The embedded federation could not reconstruct federation HLAreportFOMmoduleData transportation.");
  submitMomFomDataReport(
      federationName,
      0U,
      report.recipients,
      std::move(sentParameters),
      reliableTransportation,
      [](auto& registry, auto const& federation, auto, auto receivingId) {
        return registry.momFederationFomModuleDataReportRecipientFor(
            federation, receivingId);
      });
}

void queueMomFederationMimDataReport(
    std::wstring federationName,
    umbra::detail::MomFederationMimDataReportPlan report) {
  if (report.status != umbra::detail::MomFederationMimDataReportStatus::applied ||
      report.routing.interactionClassHandle == 0U ||
      report.routing.moduleDataParameterHandle == 0U ||
      report.recipients.empty()) {
    return;
  }

  VariableLengthData encodedMimData;
  try {
    encodedMimData = HLAunicodeString{report.moduleData}.encode();
  } catch (Exception const&) {
    throw;
  } catch (...) {
    throw RTIinternalError(
        L"The embedded federation could not encode HLAreportMIMdata values.");
  }
  std::vector<AmbassadorInteractionParameterValue> sentParameters{
      {report.routing.moduleDataParameterHandle, std::move(encodedMimData)},
  };
  auto const reliableTransportation = ambassadorTransportationHandleFromEmbeddedName(
      umbra::detail::hla::utf8::mom::reliable,
      L"The embedded federation could not reconstruct HLAreportMIMdata transportation.");
  submitMomFomDataReport(
      federationName,
      0U,
      report.recipients,
      std::move(sentParameters),
      reliableTransportation,
      [](auto& registry, auto const& federation, auto, auto receivingId) {
        return registry.momFederationMimDataReportRecipientFor(
            federation, receivingId);
      });
}
#endif

}  // namespace rti1516_2025::umbra_binding_detail
