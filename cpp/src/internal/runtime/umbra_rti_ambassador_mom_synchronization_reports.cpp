#include "internal/handles/federate_handle.hpp"
#include "internal/handles/interaction_class_handle.hpp"
#include "internal/fom/hla_names.hpp"
#include "internal/runtime/ambassador_shared_utilities.hpp"
#include "internal/runtime/umbra_rti_ambassador.hpp"

#include <RTI/encoding/BasicDataElements.h>
#include <RTI/encoding/HLAfixedRecord.h>
#include <RTI/encoding/HLAvariableArray.h>

#include <cstdint>
#include <optional>
#include <utility>
#include <vector>

namespace rti1516_2025::umbra_binding_detail {

#if defined(UMBRA_ENABLE_EMBEDDED_FEDERATION_MANAGEMENT)
namespace {

[[nodiscard]] VariableLengthData encodeMomSynchronizationPointLabels(
    std::vector<std::wstring> const& labels) {
  HLAunicodeString labelPrototype;
  HLAvariableArray encodedLabels{
      static_cast<DataElement const&>(labelPrototype)};
  for (auto const& label : labels) {
    encodedLabels.addElement(HLAunicodeString{label});
  }
  return encodedLabels.encode();
}

[[nodiscard]] VariableLengthData encodeMomSynchronizationPointFederates(
    std::vector<umbra::detail::MomFederationSynchronizationPointStatusEntry> const& entries) {
  HLAvariableArrayT<HLAoctet> federatePrototype;
  HLAfixedRecord recordPrototype;
  recordPrototype.appendElement(federatePrototype)
      .appendElement(HLAinteger32BE{});
  HLAvariableArray encodedEntries{recordPrototype};
  for (auto const& entry : entries) {
    if (entry.federateId == 0U) {
      continue;
    }
    HLAvariableArrayT<HLAoctet> encodedFederate;
    encodedFederate.decode(makeFederateHandle(entry.federateId).encode());
    HLAfixedRecord record;
    record.appendElement(encodedFederate)
        .appendElement(HLAinteger32BE{entry.status});
    encodedEntries.addElement(record);
  }
  return encodedEntries.encode();
}

}  // namespace

void queueMomFederationSynchronizationPointsReport(
    std::wstring federationName,
    umbra::detail::MomFederationSynchronizationPointsReportPlan report) {
  if (report.status !=
          umbra::detail::MomFederationSynchronizationPointsReportStatus::applied ||
      report.routing.interactionClassHandle == 0U ||
      report.routing.synchronizationPointsParameterHandle == 0U ||
      report.recipients.empty()) {
    return;
  }

  auto const encodedPoints =
      encodeMomSynchronizationPointLabels(report.synchronizationPointLabels);
  std::vector<AmbassadorInteractionParameterValue> const sentParameters{
      {report.routing.synchronizationPointsParameterHandle, encodedPoints},
  };
  auto const reliableTransportation = ambassadorTransportationHandleFromEmbeddedName(
      umbra::detail::hla::utf8::mom::reliable,
      L"The embedded federation could not reconstruct HLAreportSynchronizationPoints transportation.");
  for (auto const& plannedRecipient : report.recipients) {
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
         receivingFederateId,
         sentParameters,
         reliableTransportation](FederateAmbassador& recipient) mutable {
          std::optional<umbra::detail::ReceiveOrderInteractionRecipient> projection;
          {
            std::scoped_lock lock(ambassadorFederationManagementMutex());
            projection = embeddedFederationRegistry()
                            .momFederationSynchronizationPointsReportRecipientFor(
                                federationName,
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

void queueMomFederationSynchronizationPointStatusReport(
    std::wstring federationName,
    umbra::detail::MomFederationSynchronizationPointStatusReportPlan report) {
  if (report.status != umbra::detail::
          MomFederationSynchronizationPointStatusReportStatus::applied ||
      report.routing.interactionClassHandle == 0U ||
      report.routing.synchronizationPointNameParameterHandle == 0U ||
      report.routing.synchronizationPointFederatesParameterHandle == 0U ||
      report.recipients.empty()) {
    return;
  }

  std::vector<AmbassadorInteractionParameterValue> const sentParameters{
      {report.routing.synchronizationPointNameParameterHandle,
       HLAunicodeString{report.synchronizationPointName}.encode()},
      {report.routing.synchronizationPointFederatesParameterHandle,
       encodeMomSynchronizationPointFederates(report.federateStatuses)},
  };
  auto const reliableTransportation = ambassadorTransportationHandleFromEmbeddedName(
      umbra::detail::hla::utf8::mom::reliable,
      L"The embedded federation could not reconstruct HLAreportSynchronizationPointStatus transportation.");
  for (auto const& plannedRecipient : report.recipients) {
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
         receivingFederateId,
         sentParameters,
         reliableTransportation](FederateAmbassador& recipient) mutable {
          std::optional<umbra::detail::ReceiveOrderInteractionRecipient> projection;
          {
            std::scoped_lock lock(ambassadorFederationManagementMutex());
            projection = embeddedFederationRegistry()
                            .momFederationSynchronizationPointStatusReportRecipientFor(
                                federationName,
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
