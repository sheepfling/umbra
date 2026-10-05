#include "internal/federation/federation_registry.hpp"
#include "internal/federation/federation_registry_constants.hpp"
#include "internal/fom/fom_catalog.hpp"

#include <cstdint>
#include <iterator>
#include <limits>
#include <map>
#include <mutex>
#include <optional>
#include <set>
#include <string>
#include <utility>
#include <vector>

namespace umbra::detail {

namespace {

constexpr char kMomServiceReportParameterNames[][24] = {
    "HLAservice",
    "HLAserviceType",
    "HLAsuccessIndicator",
    "HLAsuppliedArguments",
    "HLAreturnedArgument",
    "HLAexception",
    "HLAserialNumber",
    "HLAfederate",
};

// This value is never placed in Federation::regions.  It exists only in a
// short-lived RegionSpecificationSnapshot override while evaluating the
// RTI-owned §11.5 report endpoint, so federates cannot modify or delete it.
constexpr std::uint64_t kMomServiceReportEndpointRegionHandle =
    std::numeric_limits<std::uint64_t>::max();

}  // namespace

MomServiceReportRoutingPlan EmbeddedFederationRegistry::planMomServiceReport(
    std::wstring const& federationName,
    std::uint64_t reportedFederateId,
    std::uint16_t serviceGroup) const {
  auto instrumentationScope = beginInstrumentation("planMomServiceReport");
  std::scoped_lock lock(mutex_);
  auto const federation = federations_.find(federationName);
  if (federation == federations_.end()) {
    return {MomServiceReportDisposition::inconsistent_catalog};
  }
  return momServiceReportRoutingPlanFor(federation->second, reportedFederateId, serviceGroup);
}

MomServiceReportRoutingPlan EmbeddedFederationRegistry::momServiceReportRoutingPlanFor(
    Federation const& federation,
    std::uint64_t reportedFederateId,
    std::uint16_t serviceGroup) {
  auto const reportedMember = federation.members.find(reportedFederateId);
  if (reportedMember == federation.members.end()) {
    return {MomServiceReportDisposition::reported_federate_not_member};
  }
  if (serviceGroup > 6U) {
    return {MomServiceReportDisposition::invalid_service_group};
  }
  if (!reportedMember->second.serviceReportingSwitch) {
    return {};
  }
  if (reportedMember->second.sendServiceReportsToFileSwitch) {
    return {MomServiceReportDisposition::report_to_file};
  }
  if (!federation.definition.catalog ||
      !federation.interactionClassHandles ||
      !federation.parameterHandles ||
      !federation.dimensionHandles) {
    return {MomServiceReportDisposition::inconsistent_catalog};
  }

  auto const reportClassHandle = federation.interactionClassHandles->handleFor(
      kReportServiceInvocationInteractionClassName);
  auto const federateDimensionHandle = federation.dimensionHandles->handleFor(
      "HLAfederate");
  auto const serviceGroupDimensionHandle = federation.dimensionHandles->handleFor(
      "HLAserviceGroup");
  if (!reportClassHandle || !federateDimensionHandle || !serviceGroupDimensionHandle) {
    return {MomServiceReportDisposition::inconsistent_catalog};
  }

  std::vector<std::uint64_t> reportParameterHandles;
  reportParameterHandles.reserve(std::size(kMomServiceReportParameterNames));
  for (char const* parameterName : kMomServiceReportParameterNames) {
    auto const parameterHandle = federation.parameterHandles->handleFor(
        federation.definition.catalog.get(),
        kReportServiceInvocationInteractionClassName,
        parameterName);
    if (!parameterHandle) {
      return {MomServiceReportDisposition::inconsistent_catalog};
    }
    reportParameterHandles.push_back(*parameterHandle);
  }

  auto const normalizedFederate = normalizedHandleValue(
      federation.normalizationSeed,
      reportedFederateId,
      kFederateNormalizationKind);
  RegionSpecificationSnapshot endpointRegion{
      {*federateDimensionHandle, *serviceGroupDimensionHandle},
      {
          {*federateDimensionHandle, {normalizedFederate, normalizedFederate + 1U}},
          {*serviceGroupDimensionHandle,
           {static_cast<unsigned long>(serviceGroup),
            static_cast<unsigned long>(serviceGroup) + 1U}},
      },
      true,
  };
  std::map<std::uint64_t, RegionSpecificationSnapshot> endpointOverride{
      {kMomServiceReportEndpointRegionHandle, endpointRegion},
  };
  std::set<std::uint64_t> const endpointRegionHandles{
      kMomServiceReportEndpointRegionHandle,
  };

  MomServiceReportRoutingPlan result;
  result.disposition = MomServiceReportDisposition::interaction;
  result.interactionClassHandle = *reportClassHandle;
  result.reportParameterHandles = reportParameterHandles;
  result.endpointRegionHandle = kMomServiceReportEndpointRegionHandle;
  result.endpointRegion = endpointRegion;
  for (auto const& [federateId, membership] : federation.members) {
    static_cast<void>(membership);
    auto recipient = candidateReceiveOrderInteractionRecipient(
        federation,
        InteractionProducer::rti(),
        federateId,
        *reportClassHandle,
        reportParameterHandles,
        &endpointRegionHandles,
        &endpointOverride);
    if (recipient) {
      result.recipients.push_back(std::move(*recipient));
    }
  }
  return result;
}

ReservedMomServiceReport EmbeddedFederationRegistry::reserveMomServiceReport(
    std::wstring const& federationName,
    std::uint64_t reportedFederateId,
    std::uint16_t serviceGroup) {
  auto instrumentationScope = beginInstrumentation("reserveMomServiceReport");
  std::scoped_lock lock(mutex_);
  auto federation = federations_.find(federationName);
  if (federation == federations_.end()) {
    return {{MomServiceReportDisposition::inconsistent_catalog}};
  }
  ReservedMomServiceReport result;
  result.routing = momServiceReportRoutingPlanFor(
      federation->second, reportedFederateId, serviceGroup);
  if (result.routing.disposition != MomServiceReportDisposition::interaction &&
      result.routing.disposition != MomServiceReportDisposition::report_to_file) {
    return result;
  }
  if (result.routing.disposition == MomServiceReportDisposition::interaction &&
      result.routing.recipients.empty()) {
    // No eligible observer means no interaction is emitted; do not advance
    // the joined-federate's sequence for a report with no destination.
    return result;
  }
  auto const reportedMember = federation->second.members.find(reportedFederateId);
  if (reportedMember == federation->second.members.end()) {
    // The private helper already checked this invariant. Preserve an explicit
    // no-reservation outcome if future state evolution changes that boundary.
    result.routing = {MomServiceReportDisposition::reported_federate_not_member};
    return result;
  }
  result.serialNumber = reportedMember->second.nextMomServiceReportSerialNumber;
  ++reportedMember->second.nextMomServiceReportSerialNumber;
  result.acceptedForEmission = true;
  return result;
}

std::optional<ReceiveOrderInteractionRecipient>
EmbeddedFederationRegistry::momServiceReportRecipientFor(
    std::wstring const& federationName,
    std::uint64_t reportedFederateId,
    std::uint64_t receivingFederateId,
    std::uint16_t serviceGroup) const {
  std::scoped_lock lock(mutex_);
  auto const federation = federations_.find(federationName);
  if (federation == federations_.end() || serviceGroup > 6U) {
    return std::nullopt;
  }
  auto const reportedMember = federation->second.members.find(reportedFederateId);
  if (reportedMember == federation->second.members.end() ||
      !reportedMember->second.serviceReportingSwitch ||
      reportedMember->second.sendServiceReportsToFileSwitch ||
      !federation->second.definition.catalog ||
      !federation->second.interactionClassHandles ||
      !federation->second.parameterHandles ||
      !federation->second.dimensionHandles) {
    return std::nullopt;
  }
  auto const reportClassHandle = federation->second.interactionClassHandles->handleFor(
      kReportServiceInvocationInteractionClassName);
  auto const federateDimensionHandle = federation->second.dimensionHandles->handleFor(
      "HLAfederate");
  auto const serviceGroupDimensionHandle = federation->second.dimensionHandles->handleFor(
      "HLAserviceGroup");
  if (!reportClassHandle || !federateDimensionHandle || !serviceGroupDimensionHandle) {
    return std::nullopt;
  }
  std::vector<std::uint64_t> reportParameterHandles;
  for (char const* parameterName : kMomServiceReportParameterNames) {
    auto const parameterHandle = federation->second.parameterHandles->handleFor(
        federation->second.definition.catalog.get(),
        kReportServiceInvocationInteractionClassName,
        parameterName);
    if (!parameterHandle) {
      return std::nullopt;
    }
    reportParameterHandles.push_back(*parameterHandle);
  }
  auto const normalizedFederate = normalizedHandleValue(
      federation->second.normalizationSeed,
      reportedFederateId,
      kFederateNormalizationKind);
  std::map<std::uint64_t, RegionSpecificationSnapshot> endpointOverride{
      {kMomServiceReportEndpointRegionHandle,
       {{*federateDimensionHandle, *serviceGroupDimensionHandle},
        {{*federateDimensionHandle, {normalizedFederate, normalizedFederate + 1U}},
         {*serviceGroupDimensionHandle,
          {static_cast<unsigned long>(serviceGroup), static_cast<unsigned long>(serviceGroup) + 1U}}},
        true}},
  };
  std::set<std::uint64_t> const endpointRegionHandles{
      kMomServiceReportEndpointRegionHandle,
  };
  return candidateReceiveOrderInteractionRecipient(
      federation->second,
      InteractionProducer::rti(),
      receivingFederateId,
      *reportClassHandle,
      reportParameterHandles,
      &endpointRegionHandles,
      &endpointOverride);
}


}  // namespace umbra::detail
