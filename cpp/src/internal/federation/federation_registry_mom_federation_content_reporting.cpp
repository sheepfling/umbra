#include "internal/federation/federation_registry.hpp"
#include "internal/federation/federation_registry_constants.hpp"
#include "internal/fom/fom_catalog.hpp"

#include <RTI/encoding/BasicDataElements.h>

#include <cstdint>
#include <filesystem>
#include <limits>
#include <mutex>
#include <optional>
#include <set>
#include <utility>
#include <vector>

namespace umbra::detail {

namespace {

constexpr char kFederationReportFomModuleDataInteractionClassName[] =
    "HLAinteractionRoot.HLAmanager.HLAfederation.HLAreport.HLAreportFOMmoduleData";
constexpr char kFederationReportMimDataInteractionClassName[] =
    "HLAinteractionRoot.HLAmanager.HLAfederation.HLAreport.HLAreportMIMdata";

std::vector<std::wstring> moduleContentsForFederation(
    FederationDefinition const& definition) {
  std::set<std::filesystem::path> seenSources;
  std::vector<std::wstring> contents;
  contents.reserve(definition.fomModules.size());
  for (auto const& module : definition.fomModules) {
    if (module.kind == FomModuleKind::fom &&
        seenSources.insert(module.sourcePath).second) {
      contents.push_back(module.contents);
    }
  }
  return contents;
}

std::optional<std::wstring> mimModuleContentsForFederation(
    FederationDefinition const& definition) {
  for (auto const& module : definition.fomModules) {
    if (module.kind == FomModuleKind::mim) {
      return module.contents;
    }
  }
  return std::nullopt;
}

}  // namespace

MomFederationFomModuleDataReportPlan
EmbeddedFederationRegistry::planMomFederationFomModuleDataReport(
    std::wstring const& federationName,
    std::uint64_t requestingFederateId,
    std::uint32_t moduleIndex) const {
  auto instrumentationScope =
      beginInstrumentation("planMomFederationFomModuleDataReport");
  std::scoped_lock lock(mutex_);
  MomFederationFomModuleDataReportPlan result;
  result.moduleIndex = moduleIndex;
  auto const federation = federations_.find(federationName);
  if (federation == federations_.end()) {
    result.status = MomFederationFomModuleDataReportStatus::federation_does_not_exist;
    return result;
  }
  if (!federation->second.members.contains(requestingFederateId)) {
    result.status = MomFederationFomModuleDataReportStatus::requesting_federate_not_member;
    return result;
  }
  if (moduleIndex > static_cast<std::uint32_t>(
          std::numeric_limits<rti1516_2025::Integer32>::max())) {
    result.status = MomFederationFomModuleDataReportStatus::invalid_module_index;
    return result;
  }
  if (!federation->second.definition.catalog ||
      !federation->second.interactionClassHandles ||
      !federation->second.parameterHandles) {
    result.status = MomFederationFomModuleDataReportStatus::inconsistent_catalog;
    return result;
  }

  auto const reportClassHandle = federation->second.interactionClassHandles->handleFor(
      kFederationReportFomModuleDataInteractionClassName);
  auto const moduleIndicatorParameterHandle = federation->second.parameterHandles->handleFor(
      federation->second.definition.catalog.get(),
      kFederationReportFomModuleDataInteractionClassName,
      "HLAFOMmoduleIndicator");
  auto const moduleDataParameterHandle = federation->second.parameterHandles->handleFor(
      federation->second.definition.catalog.get(),
      kFederationReportFomModuleDataInteractionClassName,
      "HLAFOMmoduleData");
  if (!reportClassHandle || !moduleIndicatorParameterHandle ||
      !moduleDataParameterHandle) {
    result.status = MomFederationFomModuleDataReportStatus::inconsistent_catalog;
    return result;
  }

  auto const moduleContents = moduleContentsForFederation(federation->second.definition);
  if (moduleIndex >= moduleContents.size()) {
    result.status = MomFederationFomModuleDataReportStatus::invalid_module_index;
    return result;
  }
  result.moduleData = moduleContents[moduleIndex];
  result.routing.interactionClassHandle = *reportClassHandle;
  result.routing.moduleIndicatorParameterHandle = *moduleIndicatorParameterHandle;
  result.routing.moduleDataParameterHandle = *moduleDataParameterHandle;
  std::vector<std::uint64_t> const sentParameterHandles{
      *moduleIndicatorParameterHandle,
      *moduleDataParameterHandle,
  };
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
EmbeddedFederationRegistry::momFederationFomModuleDataReportRecipientFor(
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
      kFederationReportFomModuleDataInteractionClassName);
  auto const moduleIndicatorParameterHandle = federation->second.parameterHandles->handleFor(
      federation->second.definition.catalog.get(),
      kFederationReportFomModuleDataInteractionClassName,
      "HLAFOMmoduleIndicator");
  auto const moduleDataParameterHandle = federation->second.parameterHandles->handleFor(
      federation->second.definition.catalog.get(),
      kFederationReportFomModuleDataInteractionClassName,
      "HLAFOMmoduleData");
  if (!reportClassHandle || !moduleIndicatorParameterHandle ||
      !moduleDataParameterHandle) {
    return std::nullopt;
  }
  return candidateReceiveOrderInteractionRecipient(
      federation->second,
      InteractionProducer::rti(),
      receivingFederateId,
      *reportClassHandle,
      {*moduleIndicatorParameterHandle, *moduleDataParameterHandle},
      nullptr,
      nullptr);
}

MomFederationMimDataReportPlan
EmbeddedFederationRegistry::planMomFederationMimDataReport(
    std::wstring const& federationName,
    std::uint64_t requestingFederateId) const {
  auto instrumentationScope =
      beginInstrumentation("planMomFederationMimDataReport");
  std::scoped_lock lock(mutex_);
  MomFederationMimDataReportPlan result;
  auto const federation = federations_.find(federationName);
  if (federation == federations_.end()) {
    result.status = MomFederationMimDataReportStatus::federation_does_not_exist;
    return result;
  }
  if (!federation->second.members.contains(requestingFederateId)) {
    result.status = MomFederationMimDataReportStatus::requesting_federate_not_member;
    return result;
  }
  if (!federation->second.definition.catalog ||
      !federation->second.interactionClassHandles ||
      !federation->second.parameterHandles) {
    result.status = MomFederationMimDataReportStatus::inconsistent_catalog;
    return result;
  }
  auto const reportClassHandle = federation->second.interactionClassHandles->handleFor(
      kFederationReportMimDataInteractionClassName);
  auto const moduleDataParameterHandle = federation->second.parameterHandles->handleFor(
      federation->second.definition.catalog.get(),
      kFederationReportMimDataInteractionClassName,
      "HLAMIMdata");
  auto const mimContents = mimModuleContentsForFederation(federation->second.definition);
  if (!reportClassHandle || !moduleDataParameterHandle || !mimContents) {
    result.status = MomFederationMimDataReportStatus::inconsistent_catalog;
    return result;
  }
  result.moduleData = *mimContents;
  result.routing.interactionClassHandle = *reportClassHandle;
  result.routing.moduleDataParameterHandle = *moduleDataParameterHandle;
  std::vector<std::uint64_t> const sentParameterHandles{
      *moduleDataParameterHandle,
  };
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
EmbeddedFederationRegistry::momFederationMimDataReportRecipientFor(
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
      kFederationReportMimDataInteractionClassName);
  auto const moduleDataParameterHandle = federation->second.parameterHandles->handleFor(
      federation->second.definition.catalog.get(),
      kFederationReportMimDataInteractionClassName,
      "HLAMIMdata");
  if (!reportClassHandle || !moduleDataParameterHandle) {
    return std::nullopt;
  }
  return candidateReceiveOrderInteractionRecipient(
      federation->second,
      InteractionProducer::rti(),
      receivingFederateId,
      *reportClassHandle,
      {*moduleDataParameterHandle},
      nullptr,
      nullptr);
}

}  // namespace umbra::detail
