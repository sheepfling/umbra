#include "internal/federation/federation_registry.hpp"
#include "internal/fom/fom_catalog.hpp"

#include <RTI/encoding/BasicDataElements.h>

#include <algorithm>
#include <chrono>
#include <limits>
#include <mutex>
#include <set>
#include <utility>

namespace umbra::detail {

std::optional<FederationDefinition> EmbeddedFederationRegistry::definitionFor(
    std::wstring const& federationName) const {
  std::scoped_lock lock(mutex_);
  auto const federation = federations_.find(federationName);
  if (federation == federations_.end()) {
    return std::nullopt;
  }
  return federation->second.definition;
}

std::optional<FederationTimeExecutionSnapshot> EmbeddedFederationRegistry::timeSnapshotFor(
    std::wstring const& federationName) const {
  std::scoped_lock lock(mutex_);
  auto const federation = federations_.find(federationName);
  if (federation == federations_.end()) {
    return std::nullopt;
  }

  return makeTimeSnapshot(federation->second);
}

std::shared_ptr<FederateTimeState> EmbeddedFederationRegistry::timeStateFor(
    std::wstring const& federationName,
    std::uint64_t federateId) const {
  std::scoped_lock lock(mutex_);
  auto const federation = federations_.find(federationName);
  if (federation == federations_.end() ||
      !federation->second.members.contains(federateId)) {
    return nullptr;
  }
  return federation->second.timeCoordinator.timeStateFor(federateId);
}

std::optional<bool> EmbeddedFederationRegistry::attributeScopeAdvisorySwitchFor(
    std::wstring const& federationName,
    std::uint64_t federateId) const {
  std::scoped_lock lock(mutex_);
  auto const federation = federations_.find(federationName);
  if (federation == federations_.end()) {
    return std::nullopt;
  }
  auto const member = federation->second.members.find(federateId);
  if (member == federation->second.members.end()) {
    return std::nullopt;
  }
  return member->second.attributeScopeAdvisorySwitch;
}

FederationRegistryStatus EmbeddedFederationRegistry::setAttributeScopeAdvisorySwitch(
    std::wstring const& federationName,
    std::uint64_t federateId,
    bool switchValue) {
  auto instrumentationScope = beginInstrumentation("setAttributeScopeAdvisorySwitch");
  std::scoped_lock lock(mutex_);
  auto federation = federations_.find(federationName);
  if (federation == federations_.end()) {
    return FederationRegistryStatus::federation_does_not_exist;
  }
  auto member = federation->second.members.find(federateId);
  if (member == federation->second.members.end()) {
    return FederationRegistryStatus::federate_not_member;
  }
  member->second.attributeScopeAdvisorySwitch = switchValue;
  return FederationRegistryStatus::applied;
}

std::optional<bool> EmbeddedFederationRegistry::objectClassRelevanceAdvisorySwitchFor(
    std::wstring const& federationName,
    std::uint64_t federateId) const {
  std::scoped_lock lock(mutex_);
  auto const federation = federations_.find(federationName);
  if (federation == federations_.end()) {
    return std::nullopt;
  }
  auto const member = federation->second.members.find(federateId);
  if (member == federation->second.members.end()) {
    return std::nullopt;
  }
  return member->second.objectClassRelevanceAdvisorySwitch;
}

FederationRegistryStatus EmbeddedFederationRegistry::setObjectClassRelevanceAdvisorySwitch(
    std::wstring const& federationName,
    std::uint64_t federateId,
    bool switchValue) {
  auto instrumentationScope = beginInstrumentation("setObjectClassRelevanceAdvisorySwitch");
  std::scoped_lock lock(mutex_);
  auto federation = federations_.find(federationName);
  if (federation == federations_.end()) {
    return FederationRegistryStatus::federation_does_not_exist;
  }
  auto member = federation->second.members.find(federateId);
  if (member == federation->second.members.end()) {
    return FederationRegistryStatus::federate_not_member;
  }
  member->second.objectClassRelevanceAdvisorySwitch = switchValue;
  return FederationRegistryStatus::applied;
}

std::optional<bool> EmbeddedFederationRegistry::attributeRelevanceAdvisorySwitchFor(
    std::wstring const& federationName,
    std::uint64_t federateId) const {
  std::scoped_lock lock(mutex_);
  auto const federation = federations_.find(federationName);
  if (federation == federations_.end()) {
    return std::nullopt;
  }
  auto const member = federation->second.members.find(federateId);
  if (member == federation->second.members.end()) {
    return std::nullopt;
  }
  return member->second.attributeRelevanceAdvisorySwitch;
}

FederationRegistryStatus EmbeddedFederationRegistry::setAttributeRelevanceAdvisorySwitch(
    std::wstring const& federationName,
    std::uint64_t federateId,
    bool switchValue) {
  auto instrumentationScope = beginInstrumentation("setAttributeRelevanceAdvisorySwitch");
  std::scoped_lock lock(mutex_);
  auto federation = federations_.find(federationName);
  if (federation == federations_.end()) {
    return FederationRegistryStatus::federation_does_not_exist;
  }
  auto member = federation->second.members.find(federateId);
  if (member == federation->second.members.end()) {
    return FederationRegistryStatus::federate_not_member;
  }
  member->second.attributeRelevanceAdvisorySwitch = switchValue;
  return FederationRegistryStatus::applied;
}

std::optional<bool> EmbeddedFederationRegistry::interactionRelevanceAdvisorySwitchFor(
    std::wstring const& federationName,
    std::uint64_t federateId) const {
  std::scoped_lock lock(mutex_);
  auto const federation = federations_.find(federationName);
  if (federation == federations_.end()) {
    return std::nullopt;
  }
  auto const member = federation->second.members.find(federateId);
  if (member == federation->second.members.end()) {
    return std::nullopt;
  }
  return member->second.interactionRelevanceAdvisorySwitch;
}

std::optional<bool> EmbeddedFederationRegistry::advisoriesUseKnownClassSwitchFor(
    std::wstring const& federationName,
    std::uint64_t federateId) const {
  std::scoped_lock lock(mutex_);
  auto const federation = federations_.find(federationName);
  if (federation == federations_.end()) {
    return std::nullopt;
  }
  auto const member = federation->second.members.find(federateId);
  if (member == federation->second.members.end()) {
    return std::nullopt;
  }
  return federation->second.advisoriesUseKnownClassSwitch;
}

std::optional<bool> EmbeddedFederationRegistry::autoProvideSwitchFor(
    std::wstring const& federationName,
    std::uint64_t federateId) const {
  std::scoped_lock lock(mutex_);
  auto const federation = federations_.find(federationName);
  if (federation == federations_.end()) {
    return std::nullopt;
  }
  if (!federation->second.members.contains(federateId)) {
    return std::nullopt;
  }
  return federation->second.autoProvideSwitch;
}

FederationRegistryStatus EmbeddedFederationRegistry::setAutoProvideSwitch(
    std::wstring const& federationName,
    std::uint64_t federateId,
    bool switchValue) {
  std::scoped_lock lock(mutex_);
  auto federation = federations_.find(federationName);
  if (federation == federations_.end()) {
    return FederationRegistryStatus::federation_does_not_exist;
  }
  if (!federation->second.members.contains(federateId)) {
    return FederationRegistryStatus::federate_not_member;
  }
  federation->second.autoProvideSwitch = switchValue;
  return FederationRegistryStatus::applied;
}

std::optional<bool> EmbeddedFederationRegistry::nonRegulatedGrantSwitchFor(
    std::wstring const& federationName,
    std::uint64_t federateId) const {
  std::scoped_lock lock(mutex_);
  auto const federation = federations_.find(federationName);
  if (federation == federations_.end()) {
    return std::nullopt;
  }
  if (!federation->second.members.contains(federateId)) {
    return std::nullopt;
  }
  return federation->second.nonRegulatedGrantSwitch;
}

std::optional<bool> EmbeddedFederationRegistry::conveyRegionDesignatorSetsSwitchFor(
    std::wstring const& federationName,
    std::uint64_t federateId) const {
  std::scoped_lock lock(mutex_);
  auto const federation = federations_.find(federationName);
  if (federation == federations_.end()) {
    return std::nullopt;
  }
  auto const member = federation->second.members.find(federateId);
  if (member == federation->second.members.end()) {
    return std::nullopt;
  }
  return member->second.conveyRegionDesignatorSetsSwitch;
}

FederationRegistryStatus EmbeddedFederationRegistry::setConveyRegionDesignatorSetsSwitch(
    std::wstring const& federationName,
    std::uint64_t federateId,
    bool switchValue) {
  auto instrumentationScope = beginInstrumentation("setConveyRegionDesignatorSetsSwitch");
  std::scoped_lock lock(mutex_);
  auto federation = federations_.find(federationName);
  if (federation == federations_.end()) {
    return FederationRegistryStatus::federation_does_not_exist;
  }
  auto member = federation->second.members.find(federateId);
  if (member == federation->second.members.end()) {
    return FederationRegistryStatus::federate_not_member;
  }
  member->second.conveyRegionDesignatorSetsSwitch = switchValue;
  return FederationRegistryStatus::applied;
}

std::optional<rti1516_2025::ResignAction>
EmbeddedFederationRegistry::automaticResignActionFor(
    std::wstring const& federationName,
    std::uint64_t federateId) const {
  std::scoped_lock lock(mutex_);
  auto const federation = federations_.find(federationName);
  if (federation == federations_.end()) {
    return std::nullopt;
  }
  auto const member = federation->second.members.find(federateId);
  if (member == federation->second.members.end()) {
    return std::nullopt;
  }
  return member->second.automaticResignAction;
}

FederationRegistryStatus EmbeddedFederationRegistry::setAutomaticResignAction(
    std::wstring const& federationName,
    std::uint64_t federateId,
    rti1516_2025::ResignAction resignAction) {
  auto instrumentationScope = beginInstrumentation("setAutomaticResignAction");
  switch (resignAction) {
    case rti1516_2025::UNCONDITIONALLY_DIVEST_ATTRIBUTES:
    case rti1516_2025::DELETE_OBJECTS:
    case rti1516_2025::CANCEL_PENDING_OWNERSHIP_ACQUISITIONS:
    case rti1516_2025::DELETE_OBJECTS_THEN_DIVEST:
    case rti1516_2025::CANCEL_THEN_DELETE_THEN_DIVEST:
    case rti1516_2025::NO_ACTION:
      break;
    default:
      return FederationRegistryStatus::invalid_resign_action;
  }

  std::scoped_lock lock(mutex_);
  auto federation = federations_.find(federationName);
  if (federation == federations_.end()) {
    return FederationRegistryStatus::federation_does_not_exist;
  }
  auto member = federation->second.members.find(federateId);
  if (member == federation->second.members.end()) {
    return FederationRegistryStatus::federate_not_member;
  }
  member->second.automaticResignAction = resignAction;
  return FederationRegistryStatus::applied;
}

std::optional<bool> EmbeddedFederationRegistry::serviceReportingSwitchFor(
    std::wstring const& federationName,
    std::uint64_t federateId) const {
  std::scoped_lock lock(mutex_);
  auto const federation = federations_.find(federationName);
  if (federation == federations_.end()) {
    return std::nullopt;
  }
  auto const member = federation->second.members.find(federateId);
  if (member == federation->second.members.end()) {
    return std::nullopt;
  }
  return member->second.serviceReportingSwitch;
}

ServiceReportingSwitchStatus EmbeddedFederationRegistry::setServiceReportingSwitch(
    std::wstring const& federationName,
    std::uint64_t federateId,
    bool switchValue) {
  auto instrumentationScope = beginInstrumentation("setServiceReportingSwitch");
  std::scoped_lock lock(mutex_);
  auto federation = federations_.find(federationName);
  if (federation == federations_.end()) {
    return ServiceReportingSwitchStatus::federation_does_not_exist;
  }
  auto member = federation->second.members.find(federateId);
  if (member == federation->second.members.end()) {
    return ServiceReportingSwitchStatus::federate_not_member;
  }
  // IEEE 1516.1-2025 10.47 / 11.5 makes the report-service subscription and
  // enabled reporting state mutually exclusive.  Disabling remains legal so
  // a federate can leave the reported state before subscribing.
  if (switchValue && hasReportServiceInvocationSubscription(federation->second, federateId)) {
    return ServiceReportingSwitchStatus::report_service_invocations_are_subscribed;
  }
  member->second.serviceReportingSwitch = switchValue;
  return ServiceReportingSwitchStatus::applied;
}

FederateMOMTimingUpdateStatus EmbeddedFederationRegistry::setFederateMomReportPeriod(
    std::wstring const& federationName,
    std::uint64_t requestingFederateId,
    std::uint64_t targetFederateId,
    std::int32_t reportPeriodSeconds) {
  auto instrumentationScope = beginInstrumentation("setFederateMomReportPeriod");
  std::scoped_lock lock(mutex_);
  auto federation = federations_.find(federationName);
  if (federation == federations_.end()) {
    return FederateMOMTimingUpdateStatus::federation_does_not_exist;
  }
  if (!federation->second.members.contains(requestingFederateId)) {
    return FederateMOMTimingUpdateStatus::requesting_federate_not_member;
  }
  auto target = federation->second.members.find(targetFederateId);
  if (target == federation->second.members.end()) {
    return FederateMOMTimingUpdateStatus::target_federate_not_member;
  }
  if (reportPeriodSeconds < 0) {
    return FederateMOMTimingUpdateStatus::invalid_report_period;
  }

  target->second.momReportPeriodSeconds = reportPeriodSeconds;
  if (reportPeriodSeconds == 0) {
    target->second.nextMomReportAt.reset();
  } else {
    target->second.nextMomReportAt =
        std::chrono::steady_clock::now() + std::chrono::seconds(reportPeriodSeconds);
  }
  return FederateMOMTimingUpdateStatus::applied;
}

FederateMOMAttributeStateUpdateStatus
EmbeddedFederationRegistry::setFederateMomAttributeState(
    std::wstring const& federationName,
    std::uint64_t requestingFederateId,
    std::uint64_t targetFederateId,
    std::uint64_t objectInstanceHandle,
    std::uint64_t attributeHandle,
    bool owned) {
  auto instrumentationScope = beginInstrumentation("setFederateMomAttributeState");
  std::scoped_lock lock(mutex_);
  auto federation = federations_.find(federationName);
  if (federation == federations_.end()) {
    return FederateMOMAttributeStateUpdateStatus::federation_does_not_exist;
  }
  if (!federation->second.members.contains(requestingFederateId)) {
    return FederateMOMAttributeStateUpdateStatus::requesting_federate_not_member;
  }
  if (!federation->second.members.contains(targetFederateId)) {
    return FederateMOMAttributeStateUpdateStatus::target_federate_not_member;
  }

  // All predefined MOM object attributes are RTI-owned.  They share the
  // object-handle namespace with application objects but deliberately never
  // enter the application ownership ledger.
  if (federation->second.rtiOwnedJoinedFederateMomObjects.contains(
          objectInstanceHandle)) {
    return FederateMOMAttributeStateUpdateStatus::attribute_owned_by_rti;
  }

  auto instance = federation->second.objectInstances.find(objectInstanceHandle);
  if (instance == federation->second.objectInstances.end() ||
      instance->second.deleteAccepted) {
    return FederateMOMAttributeStateUpdateStatus::object_instance_not_known;
  }
  auto const knownClass = instance->second.knownObjectClassHandlesByFederate.find(
      targetFederateId);
  if (knownClass == instance->second.knownObjectClassHandlesByFederate.end()) {
    return FederateMOMAttributeStateUpdateStatus::target_does_not_know_object_instance;
  }
  if (!federation->second.definition.catalog ||
      !federation->second.objectClassHandles ||
      !federation->second.attributeHandles) {
    return FederateMOMAttributeStateUpdateStatus::inconsistent_catalog;
  }
  auto const knownClassName = federation->second.objectClassHandles->nameFor(
      knownClass->second);
  if (!knownClassName ||
      federation->second.definition.catalog->objectClass(*knownClassName) == nullptr ||
      !federation->second.attributeHandles->nameFor(
          federation->second.definition.catalog.get(),
          *knownClassName,
          attributeHandle)) {
    return FederateMOMAttributeStateUpdateStatus::attribute_not_defined;
  }

  if (owned) {
    auto const publishedAttributes = publishedObjectClassAttributes(
        federation->second,
        targetFederateId,
        knownClass->second);
    if (!publishedAttributes) {
      return FederateMOMAttributeStateUpdateStatus::inconsistent_catalog;
    }
    if (!publishedAttributes->contains(attributeHandle)) {
      return FederateMOMAttributeStateUpdateStatus::target_attribute_not_published;
    }
  }

  if (owned) {
    // A direct MOM transfer has no ownership callback at which to capture the
    // target's defaults, so resolve the same transportation/order state that
    // the ordinary acquisition paths use before mutating the ownership
    // ledger. This keeps an inconsistent catalog from producing a partial
    // ownership transition.
    auto const transportationName = attributeDefaultTransportationName(
        federation->second,
        targetFederateId,
        knownClass->second,
        attributeHandle);
    if (!transportationName) {
      return FederateMOMAttributeStateUpdateStatus::inconsistent_catalog;
    }
    instance->second.attributeTransportationTypes.insert_or_assign(
        attributeHandle,
        *transportationName);
    auto const orderType = attributeDefaultOrderType(
        federation->second,
        targetFederateId,
        knownClass->second,
        attributeHandle);
    if (!orderType) {
      return FederateMOMAttributeStateUpdateStatus::inconsistent_catalog;
    }
    instance->second.attributeOwnersByHandle.insert_or_assign(
        attributeHandle,
        targetFederateId);
    clearOwnershipAssumptionSearch(instance->second, attributeHandle);
    clearUpdateRegionAssociation(instance->second, attributeHandle);
    promoteDeferredUpdateRegionAssociation(
        instance->second,
        targetFederateId,
        attributeHandle);
    instance->second.attributeOrderTypes.insert_or_assign(
        attributeHandle,
        *orderType);
  } else {
    instance->second.attributeOwnersByHandle.erase(attributeHandle);
    clearUpdateRegionAssociation(instance->second, attributeHandle);
    instance->second.attributeTransportationTypes.erase(attributeHandle);
    instance->second.attributeOrderTypes.erase(attributeHandle);
  }
  refreshRegionUsage(federation->second);
  return FederateMOMAttributeStateUpdateStatus::applied;
}

std::vector<JoinedFederateMomPeriodicUpdate>
EmbeddedFederationRegistry::takeDueJoinedFederateMomPeriodicUpdates(
    std::wstring const& federationName,
    std::chrono::steady_clock::time_point now) {
  auto instrumentationScope = beginInstrumentation(
      "takeDueJoinedFederateMomPeriodicUpdates");
  std::scoped_lock lock(mutex_);
  std::vector<JoinedFederateMomPeriodicUpdate> result;
  auto federation = federations_.find(federationName);
  if (federation == federations_.end()) {
    return result;
  }

  for (auto& [targetFederateId, member] : federation->second.members) {
    if (member.momReportPeriodSeconds <= 0 || !member.nextMomReportAt ||
        now < *member.nextMomReportAt) {
      continue;
    }
    auto object = std::find_if(
        federation->second.rtiOwnedJoinedFederateMomObjects.begin(),
        federation->second.rtiOwnedJoinedFederateMomObjects.end(),
        [targetFederateId](auto const& entry) {
          return entry.second.joinedFederateId == targetFederateId;
        });
    if (object != federation->second.rtiOwnedJoinedFederateMomObjects.end() &&
        !object->second.periodicAttributeHandles.empty()) {
      JoinedFederateMomPeriodicUpdate update{
          object->second.objectInstanceHandle,
          object->second.periodicAttributeHandles,
          {}};
      auto const objectClassName = federation->second.objectClassHandles
          ? federation->second.objectClassHandles->nameFor(
                object->second.objectClassHandle)
          : std::nullopt;
      auto const timeState = federation->second.timeCoordinator.timeStateFor(
          targetFederateId);
      if (objectClassName && timeState && federation->second.attributeHandles &&
          federation->second.definition.catalog) {
        std::set<std::uint64_t> durationAttributeHandles;
        for (auto const attributeHandle : update.attributeHandles) {
          auto const attributeName = federation->second.attributeHandles->nameFor(
              federation->second.definition.catalog.get(),
              *objectClassName,
              attributeHandle);
          if (attributeName &&
              (*attributeName == "HLAtimeGrantedTime" ||
               *attributeName == "HLAtimeAdvancingTime")) {
            durationAttributeHandles.insert(attributeHandle);
          }
        }
        auto const durations = durationAttributeHandles.empty()
            ? FederateMomTimeDurations{}
            : timeState->takeMomTimeDurations();
        auto const encodeMilliseconds = [](std::uint64_t milliseconds) {
          auto const bounded = std::min<std::uint64_t>(
              milliseconds,
              static_cast<std::uint64_t>(std::numeric_limits<std::int32_t>::max()));
          return rti1516_2025::HLAinteger32BE{
              static_cast<std::int32_t>(bounded)}.encode();
        };
        for (auto const attributeHandle : durationAttributeHandles) {
          auto const attributeName = federation->second.attributeHandles->nameFor(
              federation->second.definition.catalog.get(),
              *objectClassName,
              attributeHandle);
          if (!attributeName) {
            continue;
          }
          if (*attributeName == "HLAtimeGrantedTime") {
            update.attributeValues.emplace(
                attributeHandle,
                encodeMilliseconds(durations.grantedMilliseconds));
          } else if (*attributeName == "HLAtimeAdvancingTime") {
            update.attributeValues.emplace(
                attributeHandle,
                encodeMilliseconds(durations.advancingMilliseconds));
          }
        }
      }
      result.push_back(std::move(update));
    }

    // Keep a stable cadence if callback polling was delayed for several
    // periods, but claim only one update per pump.  The next deadline is
    // always strictly after the observed wall-clock instant.
    auto next = *member.nextMomReportAt;
    auto const period = std::chrono::seconds(member.momReportPeriodSeconds);
    do {
      next += period;
    } while (next <= now);
    member.nextMomReportAt = next;
  }
  return result;
}

FederateMOMSwitchUpdateStatus EmbeddedFederationRegistry::applyFederateMOMSwitchUpdate(
    std::wstring const& federationName,
    std::uint64_t federateId,
    FederateMOMSwitchUpdate const& update) {
  auto instrumentationScope = beginInstrumentation("applyFederateMOMSwitchUpdate");
  if (update.automaticResignAction) {
    switch (*update.automaticResignAction) {
      case rti1516_2025::UNCONDITIONALLY_DIVEST_ATTRIBUTES:
      case rti1516_2025::DELETE_OBJECTS:
      case rti1516_2025::CANCEL_PENDING_OWNERSHIP_ACQUISITIONS:
      case rti1516_2025::DELETE_OBJECTS_THEN_DIVEST:
      case rti1516_2025::CANCEL_THEN_DELETE_THEN_DIVEST:
      case rti1516_2025::NO_ACTION:
        break;
      default:
        return FederateMOMSwitchUpdateStatus::invalid_resign_action;
    }
  }

  std::scoped_lock lock(mutex_);
  auto federation = federations_.find(federationName);
  if (federation == federations_.end()) {
    return FederateMOMSwitchUpdateStatus::federation_does_not_exist;
  }
  auto member = federation->second.members.find(federateId);
  if (member == federation->second.members.end()) {
    return FederateMOMSwitchUpdateStatus::federate_not_member;
  }

  // The same §10.47 / §11.5.1 exclusion applies when a federate changes its
  // own value through the standard MOM adjustment interaction.  Check it
  // before mutating any supplied parameter so an unsuccessful interaction
  // cannot leave a partially applied switch set behind.
  if (update.serviceReporting && *update.serviceReporting &&
      hasReportServiceInvocationSubscription(federation->second, federateId)) {
    return FederateMOMSwitchUpdateStatus::report_service_invocations_are_subscribed;
  }

  if (update.objectClassRelevanceAdvisory) {
    member->second.objectClassRelevanceAdvisorySwitch = *update.objectClassRelevanceAdvisory;
  }
  if (update.attributeRelevanceAdvisory) {
    member->second.attributeRelevanceAdvisorySwitch = *update.attributeRelevanceAdvisory;
  }
  if (update.attributeScopeAdvisory) {
    member->second.attributeScopeAdvisorySwitch = *update.attributeScopeAdvisory;
  }
  if (update.interactionRelevanceAdvisory) {
    member->second.interactionRelevanceAdvisorySwitch = *update.interactionRelevanceAdvisory;
  }
  if (update.conveyRegionDesignatorSets) {
    member->second.conveyRegionDesignatorSetsSwitch = *update.conveyRegionDesignatorSets;
  }
  if (update.automaticResignAction) {
    member->second.automaticResignAction = *update.automaticResignAction;
  }
  if (update.serviceReporting) {
    member->second.serviceReportingSwitch = *update.serviceReporting;
  }
  if (update.exceptionReporting) {
    member->second.exceptionReportingSwitch = *update.exceptionReporting;
  }
  if (update.sendServiceReportsToFile) {
    member->second.sendServiceReportsToFileSwitch = *update.sendServiceReportsToFile;
  }
  return FederateMOMSwitchUpdateStatus::applied;
}

std::optional<bool> EmbeddedFederationRegistry::exceptionReportingSwitchFor(
    std::wstring const& federationName,
    std::uint64_t federateId) const {
  std::scoped_lock lock(mutex_);
  auto const federation = federations_.find(federationName);
  if (federation == federations_.end()) {
    return std::nullopt;
  }
  auto const member = federation->second.members.find(federateId);
  if (member == federation->second.members.end()) {
    return std::nullopt;
  }
  return member->second.exceptionReportingSwitch;
}

FederationRegistryStatus EmbeddedFederationRegistry::setExceptionReportingSwitch(
    std::wstring const& federationName,
    std::uint64_t federateId,
    bool switchValue) {
  auto instrumentationScope = beginInstrumentation("setExceptionReportingSwitch");
  std::scoped_lock lock(mutex_);
  auto federation = federations_.find(federationName);
  if (federation == federations_.end()) {
    return FederationRegistryStatus::federation_does_not_exist;
  }
  auto member = federation->second.members.find(federateId);
  if (member == federation->second.members.end()) {
    return FederationRegistryStatus::federate_not_member;
  }
  member->second.exceptionReportingSwitch = switchValue;
  return FederationRegistryStatus::applied;
}

std::optional<bool> EmbeddedFederationRegistry::sendServiceReportsToFileSwitchFor(
    std::wstring const& federationName,
    std::uint64_t federateId) const {
  std::scoped_lock lock(mutex_);
  auto const federation = federations_.find(federationName);
  if (federation == federations_.end()) {
    return std::nullopt;
  }
  auto const member = federation->second.members.find(federateId);
  if (member == federation->second.members.end()) {
    return std::nullopt;
  }
  return member->second.sendServiceReportsToFileSwitch;
}

FederationRegistryStatus EmbeddedFederationRegistry::setSendServiceReportsToFileSwitch(
    std::wstring const& federationName,
    std::uint64_t federateId,
    bool switchValue) {
  auto instrumentationScope = beginInstrumentation("setSendServiceReportsToFileSwitch");
  std::scoped_lock lock(mutex_);
  auto federation = federations_.find(federationName);
  if (federation == federations_.end()) {
    return FederationRegistryStatus::federation_does_not_exist;
  }
  auto member = federation->second.members.find(federateId);
  if (member == federation->second.members.end()) {
    return FederationRegistryStatus::federate_not_member;
  }
  member->second.sendServiceReportsToFileSwitch = switchValue;
  return FederationRegistryStatus::applied;
}

std::optional<bool> EmbeddedFederationRegistry::delaySubscriptionEvaluationSwitchFor(
    std::wstring const& federationName,
    std::uint64_t federateId) const {
  std::scoped_lock lock(mutex_);
  auto const federation = federations_.find(federationName);
  if (federation == federations_.end() ||
      !federation->second.members.contains(federateId)) {
    return std::nullopt;
  }
  return federation->second.delaySubscriptionEvaluationSwitch;
}

std::optional<bool> EmbeddedFederationRegistry::allowRelaxedDDMSwitchFor(
    std::wstring const& federationName,
    std::uint64_t federateId) const {
  std::scoped_lock lock(mutex_);
  auto const federation = federations_.find(federationName);
  if (federation == federations_.end() ||
      !federation->second.members.contains(federateId)) {
    return std::nullopt;
  }
  return federation->second.allowRelaxedDDMSwitch;
}

FederationRegistryStatus EmbeddedFederationRegistry::setInteractionRelevanceAdvisorySwitch(
    std::wstring const& federationName,
    std::uint64_t federateId,
    bool switchValue) {
  auto instrumentationScope = beginInstrumentation("setInteractionRelevanceAdvisorySwitch");
  std::scoped_lock lock(mutex_);
  auto federation = federations_.find(federationName);
  if (federation == federations_.end()) {
    return FederationRegistryStatus::federation_does_not_exist;
  }
  auto member = federation->second.members.find(federateId);
  if (member == federation->second.members.end()) {
    return FederationRegistryStatus::federate_not_member;
  }
  member->second.interactionRelevanceAdvisorySwitch = switchValue;
  return FederationRegistryStatus::applied;
}

std::set<std::uint64_t>
EmbeddedFederationRegistry::attributeRelevanceAdvisoryAttributes(
    std::wstring const& federationName,
    std::uint64_t providingFederateId,
    std::uint64_t receivingFederateId,
    std::uint64_t objectInstanceHandle,
    std::set<std::uint64_t> const& attributeHandles,
    bool expectedInScope) const {
  std::scoped_lock lock(mutex_);
  auto const federation = federations_.find(federationName);
  if (federation == federations_.end()) {
    return {};
  }
  auto const providingMember = federation->second.members.find(providingFederateId);
  if (providingMember == federation->second.members.end() ||
      !providingMember->second.attributeRelevanceAdvisorySwitch ||
      (receivingFederateId != 0 &&
       (providingFederateId == receivingFederateId ||
        !federation->second.members.contains(receivingFederateId)))) {
    return {};
  }

  auto const instance = federation->second.objectInstances.find(objectInstanceHandle);
  if (instance == federation->second.objectInstances.end() ||
      instance->second.deleteAccepted ||
      (receivingFederateId != 0 &&
       !instance->second.knownObjectClassHandlesByFederate.contains(receivingFederateId))) {
    return {};
  }

  std::set<std::uint64_t> result;
  for (std::uint64_t const attributeHandle : attributeHandles) {
    auto const owner = instance->second.attributeOwnersByHandle.find(attributeHandle);
    if (owner == instance->second.attributeOwnersByHandle.end() ||
        owner->second != providingFederateId) {
      continue;
    }
    auto const relevant = receivingFederateId == 0
        ? attributeRelevanceRateSnapshotForState(
              federation->second,
              instance->second,
              0,
              attributeHandle)
              .relevant
        : objectAttributeRelevantForAdvisory(
              federation->second,
              instance->second,
              receivingFederateId,
              attributeHandle);
    if (relevant == expectedInScope) {
      result.insert(attributeHandle);
    }
  }
  return result;
}

std::optional<std::string>
EmbeddedFederationRegistry::attributeRelevanceAdvisoryUpdateRateDesignatorFor(
    std::wstring const& federationName,
    std::uint64_t receivingFederateId,
    std::uint64_t objectInstanceHandle,
    std::uint64_t attributeHandle) const {
  std::scoped_lock lock(mutex_);
  auto const federation = federations_.find(federationName);
  if (federation == federations_.end() ||
      (receivingFederateId != 0 &&
       !federation->second.members.contains(receivingFederateId))) {
    return std::nullopt;
  }
  auto const objectInstance = federation->second.objectInstances.find(objectInstanceHandle);
  if (objectInstance == federation->second.objectInstances.end() ||
      objectInstance->second.deleteAccepted) {
    return std::nullopt;
  }
  if (receivingFederateId == 0) {
    return attributeRelevanceRateSnapshotForState(
               federation->second,
               objectInstance->second,
               0,
               attributeHandle)
        .updateRateDesignator;
  }
  return subscribedUpdateRateDesignatorForAttribute(
      federation->second,
      objectInstance->second,
      receivingFederateId,
      attributeHandle);
}


}  // namespace umbra::detail
