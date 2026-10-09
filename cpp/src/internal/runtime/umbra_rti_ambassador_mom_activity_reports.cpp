#include "internal/handles/federate_handle.hpp"
#include "internal/handles/interaction_class_handle.hpp"
#include "internal/handles/object_class_handle.hpp"
#include "internal/fom/hla_names.hpp"
#include "internal/runtime/ambassador_shared_utilities.hpp"
#include "internal/runtime/umbra_rti_ambassador.hpp"

#include <RTI/encoding/BasicDataElements.h>
#include <RTI/encoding/HLAfixedRecord.h>
#include <RTI/encoding/HLAvariableArray.h>

#include <algorithm>
#include <cstdint>
#include <limits>
#include <map>
#include <mutex>
#include <optional>
#include <string>
#include <utility>
#include <vector>

namespace rti1516_2025::umbra_binding_detail {

#if defined(UMBRA_ENABLE_EMBEDDED_FEDERATION_MANAGEMENT)
namespace {

using MomTransportationCounts =
    std::map<std::string, std::map<std::uint64_t, std::uint64_t>>;

template <typename Report, typename CountEncoder, typename RecipientResolver>
void queueMomActivityReport(
    std::wstring const& federationName,
    std::uint64_t reportedFederateId,
    Report const& report,
    MomTransportationCounts const& countsByTransportation,
    std::uint64_t countsParameterHandle,
    wchar_t const* reliableTransportationContext,
    wchar_t const* transportationContext,
    CountEncoder encodeCounts,
    RecipientResolver resolveRecipient) {
  if (report.status !=
          umbra::detail::MomObjectInstanceCountsReportStatus::applied ||
      report.routing.interactionClassHandle == 0U ||
      report.routing.transportationParameterHandle == 0U ||
      countsParameterHandle == 0U || report.recipients.empty()) {
    return;
  }

  auto const reliableTransportation = ambassadorTransportationHandleFromEmbeddedName(
      umbra::detail::hla::utf8::mom::reliable,
      reliableTransportationContext);
  for (auto const& [transportationName, classCounts] : countsByTransportation) {
    auto const transportation = ambassadorTransportationHandleFromFederationName(
        federationName,
        transportationName,
        transportationContext);
    auto encodedCounts = encodeCounts(classCounts);
    std::vector<AmbassadorInteractionParameterValue> sentParameters;
    sentParameters.emplace_back(
        report.routing.transportationParameterHandle,
        transportation.encode());
    sentParameters.emplace_back(countsParameterHandle, std::move(encodedCounts));

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
}

}  // namespace

VariableLengthData ambassadorEncodeMomObjectClassBasedCounts(
    std::map<std::uint64_t, std::uint64_t> const& objectClassCounts) {
  // HLAobjectClassBasedCounts is an HLAvariableArray of
  // HLAobjectClassBasedCount fixed records. Use the official 2025 data-element
  // types so nested handle-array and fixed-record alignment stay standards-owned.
  HLAvariableArrayT<HLAoctet> objectClassHandlePrototype;
  HLAfixedRecord recordPrototype;
  recordPrototype.appendElement(objectClassHandlePrototype)
      .appendElement(HLAinteger32BE{});
  HLAvariableArray counts{recordPrototype};
  for (auto const& [objectClassHandle, count] : objectClassCounts) {
    if (objectClassHandle == 0U || count == 0U) {
      continue;
    }
    HLAvariableArrayT<HLAoctet> encodedObjectClassHandle;
    encodedObjectClassHandle.decode(makeObjectClassHandle(objectClassHandle).encode());
    auto const boundedCount = std::min(
        count,
        static_cast<std::uint64_t>(std::numeric_limits<std::int32_t>::max()));
    HLAfixedRecord record;
    record.appendElement(encodedObjectClassHandle)
        .appendElement(HLAinteger32BE{static_cast<std::int32_t>(boundedCount)});
    counts.addElement(record);
  }
  return counts.encode();
}

VariableLengthData ambassadorEncodeMomInteractionCounts(
    std::map<std::uint64_t, std::uint64_t> const& interactionClassCounts) {
  // HLAinteractionCounts is an HLAvariableArray of HLAinteractionCount fixed
  // records. Only positive counts are represented.
  HLAvariableArrayT<HLAoctet> interactionClassHandlePrototype;
  HLAfixedRecord recordPrototype;
  recordPrototype.appendElement(interactionClassHandlePrototype)
      .appendElement(HLAinteger32BE{});
  HLAvariableArray counts{recordPrototype};
  for (auto const& [interactionClassHandle, count] : interactionClassCounts) {
    if (interactionClassHandle == 0U || count == 0U) {
      continue;
    }
    HLAvariableArrayT<HLAoctet> encodedInteractionClassHandle;
    encodedInteractionClassHandle.decode(
        makeInteractionClassHandle(interactionClassHandle).encode());
    auto const boundedCount = std::min(
        count,
        static_cast<std::uint64_t>(std::numeric_limits<std::int32_t>::max()));
    HLAfixedRecord record;
    record.appendElement(encodedInteractionClassHandle)
        .appendElement(HLAinteger32BE{static_cast<std::int32_t>(boundedCount)});
    counts.addElement(record);
  }
  return counts.encode();
}

void queueMomReflectionsReceivedReport(
    std::wstring federationName,
    std::uint64_t reportedFederateId,
    umbra::detail::MomReflectionsReceivedReportPlan report) {
  queueMomActivityReport(
      federationName,
      reportedFederateId,
      report,
      report.objectClassCountsByTransportation,
      report.routing.reflectionCountsParameterHandle,
      L"The embedded federation could not reconstruct HLAreportReflectionsReceived transportation.",
      L"The embedded federation could not reconstruct an HLAreportReflectionsReceived transportation type.",
      ambassadorEncodeMomObjectClassBasedCounts,
      [](auto& registry, auto const& federation, auto reportedId, auto receivingId) {
        return registry.momReflectionsReceivedReportRecipientFor(
            federation, reportedId, receivingId);
      });
}

void queueMomUpdatesSentReport(
    std::wstring federationName,
    std::uint64_t reportedFederateId,
    umbra::detail::MomUpdatesSentReportPlan report) {
  queueMomActivityReport(
      federationName,
      reportedFederateId,
      report,
      report.objectClassCountsByTransportation,
      report.routing.updateCountsParameterHandle,
      L"The embedded federation could not reconstruct HLAreportUpdatesSent transportation.",
      L"The embedded federation could not reconstruct an HLAreportUpdatesSent transportation type.",
      ambassadorEncodeMomObjectClassBasedCounts,
      [](auto& registry, auto const& federation, auto reportedId, auto receivingId) {
        return registry.momUpdatesSentReportRecipientFor(
            federation, reportedId, receivingId);
      });
}

void queueMomInteractionsSentReport(
    std::wstring federationName,
    std::uint64_t reportedFederateId,
    umbra::detail::MomInteractionsSentReportPlan report) {
  queueMomActivityReport(
      federationName,
      reportedFederateId,
      report,
      report.interactionClassCountsByTransportation,
      report.routing.interactionCountsParameterHandle,
      L"The embedded federation could not reconstruct HLAreportInteractionsSent transportation.",
      L"The embedded federation could not reconstruct an HLAreportInteractionsSent transportation type.",
      ambassadorEncodeMomInteractionCounts,
      [](auto& registry, auto const& federation, auto reportedId, auto receivingId) {
        return registry.momInteractionsSentReportRecipientFor(
            federation, reportedId, receivingId);
      });
}

void queueMomDirectedInteractionsSentReport(
    std::wstring federationName,
    std::uint64_t reportedFederateId,
    umbra::detail::MomDirectedInteractionsSentReportPlan report) {
  queueMomActivityReport(
      federationName,
      reportedFederateId,
      report,
      report.interactionClassCountsByTransportation,
      report.routing.interactionCountsParameterHandle,
      L"The embedded federation could not reconstruct HLAreportDirectedInteractionsSent transportation.",
      L"The embedded federation could not reconstruct an HLAreportDirectedInteractionsSent transportation type.",
      ambassadorEncodeMomInteractionCounts,
      [](auto& registry, auto const& federation, auto reportedId, auto receivingId) {
        return registry.momDirectedInteractionsSentReportRecipientFor(
            federation, reportedId, receivingId);
      });
}

void queueMomInteractionsReceivedReport(
    std::wstring federationName,
    std::uint64_t reportedFederateId,
    umbra::detail::MomInteractionsReceivedReportPlan report) {
  queueMomActivityReport(
      federationName,
      reportedFederateId,
      report,
      report.interactionClassCountsByTransportation,
      report.routing.interactionCountsParameterHandle,
      L"The embedded federation could not reconstruct HLAreportInteractionsReceived transportation.",
      L"The embedded federation could not reconstruct an HLAreportInteractionsReceived transportation type.",
      ambassadorEncodeMomInteractionCounts,
      [](auto& registry, auto const& federation, auto reportedId, auto receivingId) {
        return registry.momInteractionsReceivedReportRecipientFor(
            federation, reportedId, receivingId);
      });
}

void queueMomDirectedInteractionsReceivedReport(
    std::wstring federationName,
    std::uint64_t reportedFederateId,
    umbra::detail::MomDirectedInteractionsReceivedReportPlan report) {
  queueMomActivityReport(
      federationName,
      reportedFederateId,
      report,
      report.interactionClassCountsByTransportation,
      report.routing.interactionCountsParameterHandle,
      L"The embedded federation could not reconstruct HLAreportDirectedInteractionsReceived transportation.",
      L"The embedded federation could not reconstruct an HLAreportDirectedInteractionsReceived transportation type.",
      ambassadorEncodeMomInteractionCounts,
      [](auto& registry, auto const& federation, auto reportedId, auto receivingId) {
        return registry.momDirectedInteractionsReceivedReportRecipientFor(
            federation, reportedId, receivingId);
      });
}
#endif

}  // namespace rti1516_2025::umbra_binding_detail
