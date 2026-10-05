#include "internal/federation/federation_registry.hpp"
#include "internal/federation/federation_registry_constants.hpp"

#include <cstdint>
#include <mutex>
#include <optional>
#include <utility>
#include <vector>

namespace umbra::detail {

namespace {

constexpr char kFederationReportSynchronizationPointsInteractionClassName[] =
    "HLAinteractionRoot.HLAmanager.HLAfederation.HLAreport.HLAreportSynchronizationPoints";
constexpr char kFederationReportSynchronizationPointStatusInteractionClassName[] =
    "HLAinteractionRoot.HLAmanager.HLAfederation.HLAreport.HLAreportSynchronizationPointStatus";

}  // namespace

MomFederationSynchronizationPointsReportPlan
EmbeddedFederationRegistry::planMomFederationSynchronizationPointsReport(
    std::wstring const& federationName,
    std::uint64_t requestingFederateId) const {
  auto instrumentationScope = beginInstrumentation(
      "planMomFederationSynchronizationPointsReport");
  std::scoped_lock lock(mutex_);
  MomFederationSynchronizationPointsReportPlan result;
  auto const federation = federations_.find(federationName);
  if (federation == federations_.end()) {
    result.status =
        MomFederationSynchronizationPointsReportStatus::federation_does_not_exist;
    return result;
  }
  if (!federation->second.members.contains(requestingFederateId)) {
    result.status =
        MomFederationSynchronizationPointsReportStatus::requesting_federate_not_member;
    return result;
  }
  if (!federation->second.definition.catalog ||
      !federation->second.interactionClassHandles ||
      !federation->second.parameterHandles) {
    result.status =
        MomFederationSynchronizationPointsReportStatus::inconsistent_catalog;
    return result;
  }

  auto const reportClassHandle = federation->second.interactionClassHandles->handleFor(
      kFederationReportSynchronizationPointsInteractionClassName);
  auto const synchronizationPointsParameterHandle =
      federation->second.parameterHandles->handleFor(
          federation->second.definition.catalog.get(),
          kFederationReportSynchronizationPointsInteractionClassName,
          "HLAsyncPoints");
  if (!reportClassHandle || !synchronizationPointsParameterHandle) {
    result.status =
        MomFederationSynchronizationPointsReportStatus::inconsistent_catalog;
    return result;
  }

  result.routing.interactionClassHandle = *reportClassHandle;
  result.routing.synchronizationPointsParameterHandle =
      *synchronizationPointsParameterHandle;
  result.synchronizationPointLabels.reserve(
      federation->second.synchronizationPoints.size());
  for (auto const& [label, point] : federation->second.synchronizationPoints) {
    static_cast<void>(point);
    result.synchronizationPointLabels.push_back(label);
  }

  std::vector<std::uint64_t> const sentParameterHandles{
      *synchronizationPointsParameterHandle};
  for (auto const& [federateId, membership] : federation->second.members) {
    static_cast<void>(membership);
    auto recipient = candidateReceiveOrderInteractionRecipient(
        federation->second,
        InteractionProducer::rti(),
        federateId,
        result.routing.interactionClassHandle,
        sentParameterHandles,
        nullptr,
        nullptr);
    if (recipient) {
      result.recipients.push_back(std::move(*recipient));
    }
  }
  return result;
}

std::optional<ReceiveOrderInteractionRecipient>
EmbeddedFederationRegistry::momFederationSynchronizationPointsReportRecipientFor(
    std::wstring const& federationName,
    std::uint64_t receivingFederateId) const {
  std::scoped_lock lock(mutex_);
  auto const federation = federations_.find(federationName);
  if (federation == federations_.end() ||
      !federation->second.members.contains(receivingFederateId) ||
      !federation->second.definition.catalog ||
      !federation->second.interactionClassHandles ||
      !federation->second.parameterHandles) {
    return std::nullopt;
  }
  auto const reportClassHandle = federation->second.interactionClassHandles->handleFor(
      kFederationReportSynchronizationPointsInteractionClassName);
  auto const synchronizationPointsParameterHandle =
      federation->second.parameterHandles->handleFor(
          federation->second.definition.catalog.get(),
          kFederationReportSynchronizationPointsInteractionClassName,
          "HLAsyncPoints");
  if (!reportClassHandle || !synchronizationPointsParameterHandle) {
    return std::nullopt;
  }
  return candidateReceiveOrderInteractionRecipient(
      federation->second,
      InteractionProducer::rti(),
      receivingFederateId,
      *reportClassHandle,
      {*synchronizationPointsParameterHandle},
      nullptr,
      nullptr);
}

MomFederationSynchronizationPointStatusReportPlan
EmbeddedFederationRegistry::planMomFederationSynchronizationPointStatusReport(
    std::wstring const& federationName,
    std::uint64_t requestingFederateId,
    std::wstring synchronizationPointName) const {
  auto instrumentationScope = beginInstrumentation(
      "planMomFederationSynchronizationPointStatusReport");
  std::scoped_lock lock(mutex_);
  MomFederationSynchronizationPointStatusReportPlan result;
  result.synchronizationPointName = std::move(synchronizationPointName);
  auto const federation = federations_.find(federationName);
  if (federation == federations_.end()) {
    result.status = MomFederationSynchronizationPointStatusReportStatus::
        federation_does_not_exist;
    return result;
  }
  if (!federation->second.members.contains(requestingFederateId)) {
    result.status = MomFederationSynchronizationPointStatusReportStatus::
        requesting_federate_not_member;
    return result;
  }
  if (!federation->second.definition.catalog ||
      !federation->second.interactionClassHandles ||
      !federation->second.parameterHandles) {
    result.status = MomFederationSynchronizationPointStatusReportStatus::
        inconsistent_catalog;
    return result;
  }

  auto const reportClassHandle = federation->second.interactionClassHandles->handleFor(
      kFederationReportSynchronizationPointStatusInteractionClassName);
  auto const synchronizationPointNameParameterHandle =
      federation->second.parameterHandles->handleFor(
          federation->second.definition.catalog.get(),
          kFederationReportSynchronizationPointStatusInteractionClassName,
          "HLAsyncPointName");
  auto const synchronizationPointFederatesParameterHandle =
      federation->second.parameterHandles->handleFor(
          federation->second.definition.catalog.get(),
          kFederationReportSynchronizationPointStatusInteractionClassName,
          "HLAsyncPointFederates");
  if (!reportClassHandle || !synchronizationPointNameParameterHandle ||
      !synchronizationPointFederatesParameterHandle) {
    result.status = MomFederationSynchronizationPointStatusReportStatus::
        inconsistent_catalog;
    return result;
  }

  result.routing.interactionClassHandle = *reportClassHandle;
  result.routing.synchronizationPointNameParameterHandle =
      *synchronizationPointNameParameterHandle;
  result.routing.synchronizationPointFederatesParameterHandle =
      *synchronizationPointFederatesParameterHandle;
  auto const point = federation->second.synchronizationPoints.find(
      result.synchronizationPointName);
  if (point != federation->second.synchronizationPoints.end()) {
    result.federateStatuses.reserve(point->second.synchronizationSet.size());
    for (std::uint64_t const federateId : point->second.synchronizationSet) {
      MomFederationSynchronizationPointStatusEntry entry;
      entry.federateId = federateId;
      if (!point->second.announcedFederates.contains(federateId)) {
        entry.status = 0;  // NoActivity is the MIM's zero enumerator.
      } else if (point->second.achievedFederates.contains(federateId)) {
        entry.status = 3;  // WaitingForRestOfFederation.
      } else {
        entry.status = 2;  // MovingToSyncPoint.
      }
      result.federateStatuses.push_back(entry);
    }
  }

  std::vector<std::uint64_t> const sentParameterHandles{
      *synchronizationPointNameParameterHandle,
      *synchronizationPointFederatesParameterHandle};
  for (auto const& [federateId, membership] : federation->second.members) {
    static_cast<void>(membership);
    auto recipient = candidateReceiveOrderInteractionRecipient(
        federation->second,
        InteractionProducer::rti(),
        federateId,
        result.routing.interactionClassHandle,
        sentParameterHandles,
        nullptr,
        nullptr);
    if (recipient) {
      result.recipients.push_back(std::move(*recipient));
    }
  }
  return result;
}

std::optional<ReceiveOrderInteractionRecipient>
EmbeddedFederationRegistry::momFederationSynchronizationPointStatusReportRecipientFor(
    std::wstring const& federationName,
    std::uint64_t receivingFederateId) const {
  std::scoped_lock lock(mutex_);
  auto const federation = federations_.find(federationName);
  if (federation == federations_.end() ||
      !federation->second.members.contains(receivingFederateId) ||
      !federation->second.definition.catalog ||
      !federation->second.interactionClassHandles ||
      !federation->second.parameterHandles) {
    return std::nullopt;
  }
  auto const reportClassHandle = federation->second.interactionClassHandles->handleFor(
      kFederationReportSynchronizationPointStatusInteractionClassName);
  auto const synchronizationPointNameParameterHandle =
      federation->second.parameterHandles->handleFor(
          federation->second.definition.catalog.get(),
          kFederationReportSynchronizationPointStatusInteractionClassName,
          "HLAsyncPointName");
  auto const synchronizationPointFederatesParameterHandle =
      federation->second.parameterHandles->handleFor(
          federation->second.definition.catalog.get(),
          kFederationReportSynchronizationPointStatusInteractionClassName,
          "HLAsyncPointFederates");
  if (!reportClassHandle || !synchronizationPointNameParameterHandle ||
      !synchronizationPointFederatesParameterHandle) {
    return std::nullopt;
  }
  return candidateReceiveOrderInteractionRecipient(
      federation->second,
      InteractionProducer::rti(),
      receivingFederateId,
      *reportClassHandle,
      {*synchronizationPointNameParameterHandle,
       *synchronizationPointFederatesParameterHandle},
      nullptr,
      nullptr);
}

}  // namespace umbra::detail
