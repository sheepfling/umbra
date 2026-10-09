#include "internal/handles/federate_handle.hpp"
#include "internal/handles/interaction_class_handle.hpp"
#include "internal/handles/object_class_handle.hpp"
#include "internal/handles/object_instance_handle.hpp"
#include "internal/fom/hla_names.hpp"
#include "internal/runtime/ambassador_shared_utilities.hpp"
#include "internal/runtime/umbra_rti_ambassador.hpp"

#include <cstdint>
#include <mutex>
#include <optional>
#include <utility>
#include <vector>

namespace rti1516_2025::umbra_binding_detail {

#if defined(UMBRA_ENABLE_EMBEDDED_FEDERATION_MANAGEMENT)
namespace {

template <typename RecipientResolver>
void queueMomObjectInstanceCountsReport(
    std::wstring federationName,
    std::uint64_t reportedFederateId,
    umbra::detail::MomObjectInstanceCountsReportPlan report,
    VariableLengthData encodedCounts,
    wchar_t const* transportationContext,
    RecipientResolver resolveRecipient) {
  if (report.status !=
          umbra::detail::MomObjectInstanceCountsReportStatus::applied ||
      report.routing.interactionClassHandle == 0U ||
      report.routing.objectInstanceCountsParameterHandle == 0U ||
      report.recipients.empty()) {
    return;
  }
  std::vector<AmbassadorInteractionParameterValue> sentParameters;
  sentParameters.emplace_back(
      report.routing.objectInstanceCountsParameterHandle,
      std::move(encodedCounts));
  auto const reliableTransportation = ambassadorTransportationHandleFromEmbeddedName(
      umbra::detail::hla::utf8::mom::reliable,
      transportationContext);

  for (auto& plannedRecipient : report.recipients) {
    if (!plannedRecipient.callbackRoute || plannedRecipient.federateId == 0U) {
      continue;
    }
    auto callbackRoute = std::move(plannedRecipient.callbackRoute);
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

}  // namespace

void queueMomObjectInstancesUpdatedReport(
    std::wstring federationName,
    std::uint64_t reportedFederateId,
    umbra::detail::MomObjectInstanceCountsReportPlan report,
    VariableLengthData encodedCounts) {
  queueMomObjectInstanceCountsReport(
      std::move(federationName),
      reportedFederateId,
      std::move(report),
      std::move(encodedCounts),
      L"The embedded federation could not reconstruct HLAreportObjectInstancesUpdated transportation.",
      [](auto& registry, auto const& federation, auto reportedId, auto receivingId) {
        return registry.momObjectInstancesUpdatedReportRecipientFor(
            federation, reportedId, receivingId);
      });
}

void queueMomObjectInstancesThatCanBeDeletedReport(
    std::wstring federationName,
    std::uint64_t reportedFederateId,
    umbra::detail::MomObjectInstanceCountsReportPlan report,
    VariableLengthData encodedCounts) {
  queueMomObjectInstanceCountsReport(
      std::move(federationName),
      reportedFederateId,
      std::move(report),
      std::move(encodedCounts),
      L"The embedded federation could not reconstruct HLAreportObjectInstancesThatCanBeDeleted transportation.",
      [](auto& registry, auto const& federation, auto reportedId, auto receivingId) {
        return registry.momObjectInstancesThatCanBeDeletedReportRecipientFor(
            federation, reportedId, receivingId);
      });
}

void queueMomObjectInstancesReflectedReport(
    std::wstring federationName,
    std::uint64_t reportedFederateId,
    umbra::detail::MomObjectInstanceCountsReportPlan report,
    VariableLengthData encodedCounts) {
  queueMomObjectInstanceCountsReport(
      std::move(federationName),
      reportedFederateId,
      std::move(report),
      std::move(encodedCounts),
      L"The embedded federation could not reconstruct HLAreportObjectInstancesReflected transportation.",
      [](auto& registry, auto const& federation, auto reportedId, auto receivingId) {
        return registry.momObjectInstancesReflectedReportRecipientFor(
            federation, reportedId, receivingId);
      });
}

// The request-time snapshot always carries the object reference and owned
// attribute list; registered/known classes are omitted for the MIM NULL
// response when the represented federate does not know the object.
void queueMomObjectInstanceInformationReport(
    std::wstring federationName,
    std::uint64_t reportedFederateId,
    umbra::detail::MomObjectInstanceInformationReportPlan report) {
  if (report.status !=
          umbra::detail::MomObjectInstanceInformationReportStatus::applied ||
      report.routing.interactionClassHandle == 0U ||
      report.routing.objectInstanceParameterHandle == 0U ||
      report.routing.ownedInstanceAttributeListParameterHandle == 0U ||
      report.routing.registeredClassParameterHandle == 0U ||
      report.routing.knownClassParameterHandle == 0U ||
      report.recipients.empty()) {
    return;
  }

  std::vector<AmbassadorInteractionParameterValue> sentParameters;
  sentParameters.emplace_back(
      report.routing.objectInstanceParameterHandle,
      makeObjectInstanceHandle(report.objectInstanceHandle).encode());
  sentParameters.emplace_back(
      report.routing.ownedInstanceAttributeListParameterHandle,
      ambassadorEncodeMomAttributeHandleList(report.ownedAttributeHandles));
  if (report.objectKnown) {
    sentParameters.emplace_back(
        report.routing.registeredClassParameterHandle,
        makeObjectClassHandle(report.registeredObjectClassHandle).encode());
    sentParameters.emplace_back(
        report.routing.knownClassParameterHandle,
        makeObjectClassHandle(report.knownObjectClassHandle).encode());
  }

  auto const reliableTransportation = ambassadorTransportationHandleFromEmbeddedName(
      umbra::detail::hla::utf8::mom::reliable,
      L"The embedded federation could not reconstruct HLAreportObjectInstanceInformation transportation.");
  for (auto const &plannedRecipient : report.recipients) {
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
         reliableTransportation](FederateAmbassador &recipient) mutable {
          std::optional<umbra::detail::ReceiveOrderInteractionRecipient> projection;
          {
            std::scoped_lock lock(ambassadorFederationManagementMutex());
            projection = embeddedFederationRegistry()
                            .momObjectInstanceInformationReportRecipientFor(
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
#endif

}  // namespace rti1516_2025::umbra_binding_detail
