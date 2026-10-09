#include "internal/federation/federation_registry.hpp"
#include "internal/federation/federation_registry_constants.hpp"
#include "internal/federation/federation_registry_value_helpers.hpp"
#include "internal/federation/federation_state_image.hpp"
#include "internal/federation/federation_registry_state_image_validation.hpp"
#include "internal/federation/federation_registry_state_image_restore_helpers.hpp"

#include "internal/fom/fom_catalog.hpp"
#include "internal/fom/fdd_document.hpp"
#include "internal/fom/hla_names.hpp"
#include "internal/time/federation_time_bounds.hpp"
#include "internal/time/federation_time_grant_policy.hpp"
#include "internal/handles/handle_variable_array_encoding.hpp"
#include "internal/runtime/utf8_string.hpp"

#include <RTI/encoding/BasicDataElements.h>
#include <RTI/time/HLAlogicalTimeFactoryFactory.h>

#include <algorithm>
#include <atomic>
#include <iterator>
#include <limits>
#include <random>
#include <set>
#include <stdexcept>
#include <string>
#include <string_view>
#include <type_traits>
#include <tuple>
#include <utility>

namespace umbra::detail {

namespace {

using federation_registry_state_image_validation::isRouteFreeInteractionDeclarationImage;
using federation_registry_state_image_validation::isRouteFreeMixedMultipleInteractionDeclarationImage;
using federation_registry_state_image_validation::isRouteFreeMultipleDirectedInteractionDeclarationImage;
using federation_registry_state_image_validation::isRouteFreeMultipleInteractionPublicationImage;
using federation_registry_state_image_validation::isRouteFreeRegionalTsoInteractionImage;
using federation_registry_state_image_validation::isRouteFreePendingNegotiatedIfAvailableOwnershipObject;
using federation_registry_state_image_validation::isRouteFreeNegotiatedOwnershipAssumptionLedger;
using federation_registry_state_image_validation::isRouteFreePendingConfirmDivestitureNotificationObject;
using federation_registry_state_image_validation::isRouteFreePendingConfirmDivestitureWithOwnershipAssumptionObject;
using federation_registry_state_image_validation::isRouteFreePendingDivestitureIfWantedNotificationObject;
using federation_registry_state_image_validation::isRouteFreePendingIfAvailableOwnershipObject;
using federation_registry_state_image_validation::isRouteFreePendingMixedNegotiatedConfirmationAsymmetricOwnershipObject;
using federation_registry_state_image_validation::isRouteFreePendingMixedNegotiatedConfirmationDeliveredOwnershipObject;
using federation_registry_state_image_validation::isRouteFreePendingMixedNegotiatedOwnershipObject;
using federation_registry_state_image_validation::isRouteFreePendingNegotiatedConfirmationDeliveredOwnershipObject;
using federation_registry_state_image_validation::isRouteFreePendingNegotiatedIfAvailableConfirmationDeliveredOwnershipObject;
using federation_registry_state_image_validation::isRouteFreePendingNegotiatedOwnershipObject;
using federation_registry_state_image_validation::isRouteFreePendingAttributeOwnershipQueryObject;
using federation_registry_state_image_validation::isRouteFreePendingAttributeTransportationTypeChangeObject;
using federation_registry_state_image_validation::isRouteFreePendingAttributeValueUpdateClassObject;
using federation_registry_state_image_validation::isRouteFreePendingAttributeValueUpdateObject;
using federation_registry_state_image_validation::isRouteFreePendingAttributeValueUpdateRegionalObject;
using federation_registry_state_image_validation::isRouteFreeDeliveredApplicationValue;
using federation_registry_state_image_validation::isRouteFreeObjectWithTsoAttributeUpdate;
using federation_registry_state_image_validation::isRouteFreePendingOwnershipAcquisitionCancellationObject;
using federation_registry_state_image_validation::isRouteFreePendingOwnershipAssumptionObject;
using federation_registry_state_image_validation::isRouteFreePendingRegularOwnershipObject;
using federation_registry_state_image_validation::pendingOwnershipAssumptionCallbackCount;

constexpr char kReportExceptionInteractionClassName[] =
    "HLAinteractionRoot.HLAmanager.HLAfederate.HLAreport.HLAreportException";

constexpr std::uint64_t kMomExceptionReportEndpointRegionHandle =
    std::numeric_limits<std::uint64_t>::max() - 2U;


}  // namespace

static_assert(std::is_nothrow_swappable_v<FederationDefinition>);



bool EmbeddedFederationRegistry::validDefinition(
    std::wstring const& federationName,
    FederationDefinition const& definition) {
  static_cast<void>(federationName);
  // Public FOM/MIM and naming constraints belong to the standards-derived
  // preparation coordinator. The state kernel receives only a prevalidated
  // definition and must not manufacture extra rules for otherwise representable
  // designators or names.
  return !definition.fomModules.empty();
}

EmbeddedFederationRegistry::EmbeddedFederationRegistry(
    std::shared_ptr<RuntimeInstrumentation> instrumentation,
    std::shared_ptr<FederationSaveCommitStore> saveCommitStore)
    : instrumentation_(
          instrumentation ? std::move(instrumentation)
                           : std::make_shared<RuntimeInstrumentation>()),
      saveCommitStore_(
          saveCommitStore ? std::move(saveCommitStore)
                          : std::make_shared<MemoryFederationSaveCommitStore>()) {}

RuntimeInstrumentationSnapshot
EmbeddedFederationRegistry::runtimeInstrumentationSnapshotForTesting() const {
  return instrumentation_->snapshot();
}

RuntimeInstrumentation::Scope EmbeddedFederationRegistry::beginInstrumentation(
    std::string_view operation) const {
  return instrumentation_->begin(
      InstrumentationLayer::federation_registry,
      operation);
}

SynchronizationPointRegistrationPlan
EmbeddedFederationRegistry::registerSynchronizationPoint(
    std::wstring const& federationName,
    std::uint64_t registeringFederateId,
    std::wstring label,
    std::vector<unsigned char> userSuppliedTag,
    std::set<std::uint64_t> const& requestedSynchronizationSet,
    bool synchronizationSetWasSupplied) {
  auto instrumentationScope = beginInstrumentation("registerSynchronizationPoint");
  SynchronizationPointRegistrationPlan plan;
  plan.label = label;
  plan.userSuppliedTag = userSuppliedTag;

  std::scoped_lock lock(mutex_);
  auto federation = federations_.find(federationName);
  if (federation == federations_.end()) {
    plan.status = SynchronizationPointRegistrationStatus::federation_does_not_exist;
    return plan;
  }
  auto const registeringMember = federation->second.members.find(registeringFederateId);
  if (registeringMember == federation->second.members.end()) {
    plan.status = SynchronizationPointRegistrationStatus::federate_not_member;
    return plan;
  }

  auto const registrationRoute = federation->second.interactionCallbackRoutes.find(
      registeringFederateId);
  if (registrationRoute == federation->second.interactionCallbackRoutes.end() ||
      !registrationRoute->second) {
    plan.status = SynchronizationPointRegistrationStatus::callback_route_missing;
    return plan;
  }
  plan.registrationCallback = registrationRoute->second;

  if (federation->second.synchronizationPoints.contains(label)) {
    plan.failureReason = rti1516_2025::SYNCHRONIZATION_POINT_LABEL_NOT_UNIQUE;
    return plan;
  }

  std::set<std::uint64_t> synchronizationSet = requestedSynchronizationSet;
  if (synchronizationSet.empty()) {
    for (auto const& [federateId, member] : federation->second.members) {
      static_cast<void>(member);
      synchronizationSet.insert(federateId);
    }
  }

  for (std::uint64_t federateId : synchronizationSet) {
    if (!federation->second.members.contains(federateId)) {
      plan.failureReason = rti1516_2025::SYNCHRONIZATION_SET_MEMBER_NOT_JOINED;
      return plan;
    }
    auto const route = federation->second.interactionCallbackRoutes.find(federateId);
    if (route == federation->second.interactionCallbackRoutes.end() || !route->second) {
      plan.status = SynchronizationPointRegistrationStatus::callback_route_missing;
      return plan;
    }
  }

  Federation::SynchronizationPoint point;
  point.userSuppliedTag = std::move(userSuppliedTag);
  point.synchronizationSet = synchronizationSet;
  point.announcedFederates = synchronizationSet;
  point.lateJoinExpansionAllowed = !synchronizationSetWasSupplied;

  plan.announcements.reserve(synchronizationSet.size());
  for (std::uint64_t federateId : synchronizationSet) {
    auto const route = federation->second.interactionCallbackRoutes.find(federateId);
    auto const reportRoute = federation->second.serviceReportRoutes.find(federateId);
    auto const publicReportRoute = federation->second.publicServiceReportRoutes.find(federateId);
    plan.announcements.push_back({
        federateId,
        label,
        point.userSuppliedTag,
        route->second,
        reportRoute == federation->second.serviceReportRoutes.end()
            ? FederateServiceReportRoute{}
            : reportRoute->second,
        publicReportRoute == federation->second.publicServiceReportRoutes.end()
            ? FederatePublicServiceReportRoute{}
            : publicReportRoute->second,
    });
  }

  auto const [pointPosition, inserted] = federation->second.synchronizationPoints.emplace(
      label,
      std::move(point));
  static_cast<void>(pointPosition);
  if (!inserted) {
    // The registry lock makes this unreachable for the current in-process
    // backend, but preserve the standard asynchronous failure shape if the
    // storage implementation changes later.
    plan.announcements.clear();
    plan.failureReason = rti1516_2025::SYNCHRONIZATION_POINT_LABEL_NOT_UNIQUE;
    return plan;
  }
  plan.succeeded = true;
  return plan;
}

SynchronizationPointAnnouncementPlan
EmbeddedFederationRegistry::announcePendingSynchronizationPoints(
    std::wstring const& federationName,
    std::uint64_t newlyJoinedFederateId) {
  SynchronizationPointAnnouncementPlan plan;

  std::scoped_lock lock(mutex_);
  auto federation = federations_.find(federationName);
  if (federation == federations_.end()) {
    plan.status = SynchronizationPointAnnouncementStatus::federation_does_not_exist;
    return plan;
  }
  if (!federation->second.members.contains(newlyJoinedFederateId)) {
    plan.status = SynchronizationPointAnnouncementStatus::federate_not_member;
    return plan;
  }
  auto const route = federation->second.interactionCallbackRoutes.find(newlyJoinedFederateId);
  if (route == federation->second.interactionCallbackRoutes.end() || !route->second) {
    return plan;
  }

  for (auto& [label, point] : federation->second.synchronizationPoints) {
    if (!point.lateJoinExpansionAllowed) {
      continue;
    }
    if (!point.synchronizationSet.insert(newlyJoinedFederateId).second) {
      continue;
    }
    if (!point.announcedFederates.insert(newlyJoinedFederateId).second) {
      continue;
    }
    auto const reportRoute = federation->second.serviceReportRoutes.find(
        newlyJoinedFederateId);
    auto const publicReportRoute = federation->second.publicServiceReportRoutes.find(
        newlyJoinedFederateId);
    plan.announcements.push_back({
        newlyJoinedFederateId,
        label,
        point.userSuppliedTag,
        route->second,
        reportRoute == federation->second.serviceReportRoutes.end()
            ? FederateServiceReportRoute{}
            : reportRoute->second,
        publicReportRoute == federation->second.publicServiceReportRoutes.end()
            ? FederatePublicServiceReportRoute{}
            : publicReportRoute->second,
    });
  }
  return plan;
}

SynchronizationPointAchievedPlan EmbeddedFederationRegistry::achieveSynchronizationPoint(
    std::wstring const& federationName,
    std::uint64_t achievingFederateId,
    std::wstring const& label,
    bool successfully) {
  auto instrumentationScope = beginInstrumentation("achieveSynchronizationPoint");
  SynchronizationPointAchievedPlan plan;

  std::scoped_lock lock(mutex_);
  auto federation = federations_.find(federationName);
  if (federation == federations_.end()) {
    plan.status = SynchronizationPointAchievedStatus::federation_does_not_exist;
    return plan;
  }
  if (!federation->second.members.contains(achievingFederateId)) {
    plan.status = SynchronizationPointAchievedStatus::federate_not_member;
    return plan;
  }
  auto point = federation->second.synchronizationPoints.find(label);
  if (point == federation->second.synchronizationPoints.end() ||
      !point->second.synchronizationSet.contains(achievingFederateId) ||
      !point->second.announcedFederates.contains(achievingFederateId)) {
    plan.status = SynchronizationPointAchievedStatus::synchronization_point_label_not_announced;
    return plan;
  }

  // A repeated achievement is harmless and must not create duplicate
  // Federation Synchronized callbacks.  The first indication remains the
  // authoritative success/failure result for this point.
  point->second.achievedFederates.emplace(achievingFederateId, successfully);
  if (point->second.achievedFederates.size() < point->second.synchronizationSet.size()) {
    return plan;
  }

  std::set<std::uint64_t> failedToSyncFederates;
  for (auto const& [federateId, federateSucceeded] : point->second.achievedFederates) {
    if (!federateSucceeded) {
      failedToSyncFederates.insert(federateId);
    }
  }

  plan.synchronizationNotifications.reserve(point->second.synchronizationSet.size());
  for (std::uint64_t federateId : point->second.synchronizationSet) {
    auto const member = federation->second.members.find(federateId);
    auto const route = federation->second.interactionCallbackRoutes.find(federateId);
    auto const reportRoute = federation->second.serviceReportRoutes.find(federateId);
    auto const publicReportRoute = federation->second.publicServiceReportRoutes.find(federateId);
    if (member == federation->second.members.end() ||
        route == federation->second.interactionCallbackRoutes.end() || !route->second) {
      continue;
    }
    plan.synchronizationNotifications.push_back({
        label,
        failedToSyncFederates,
        route->second,
        reportRoute == federation->second.serviceReportRoutes.end()
            ? FederateServiceReportRoute{}
            : reportRoute->second,
        publicReportRoute == federation->second.publicServiceReportRoutes.end()
            ? FederatePublicServiceReportRoute{}
            : publicReportRoute->second,
        federateId,
    });
  }
  federation->second.synchronizationPoints.erase(point);
  return plan;
}

FederationRegistryResult EmbeddedFederationRegistry::resign(
    std::wstring const& federationName,
    std::uint64_t federateId,
    rti1516_2025::ResignAction resignAction) {
  auto instrumentationScope = beginInstrumentation("resign");
  std::scoped_lock lock(mutex_);
  return resignLocked(
      federationName,
      federateId,
      resignAction,
      false,
      std::nullopt,
      false);
}

FederationRegistryResult
EmbeddedFederationRegistry::resignWithFinalServiceReportReservation(
    std::wstring const& federationName,
    std::uint64_t federateId,
    rti1516_2025::ResignAction resignAction,
    std::uint16_t serviceGroup,
    bool reservePublicInteraction) {
  auto instrumentationScope = beginInstrumentation(
      "resignWithFinalServiceReportReservation");
  std::scoped_lock lock(mutex_);
  return resignLocked(
      federationName,
      federateId,
      resignAction,
      false,
      serviceGroup,
      reservePublicInteraction);
}

FederationRegistryStatus EmbeddedFederationRegistry::setServiceReportRoute(
    std::wstring const& federationName,
    std::uint64_t federateId,
    FederateServiceReportRoute serviceReportRoute) {
  auto instrumentationScope = beginInstrumentation("setServiceReportRoute");
  if (!serviceReportRoute) {
    return FederationRegistryStatus::invalid_request;
  }

  std::scoped_lock lock(mutex_);
  auto federation = federations_.find(federationName);
  if (federation == federations_.end()) {
    return FederationRegistryStatus::federation_does_not_exist;
  }
  if (!federation->second.members.contains(federateId)) {
    return FederationRegistryStatus::federate_not_member;
  }
  federation->second.serviceReportRoutes.insert_or_assign(
      federateId,
      std::move(serviceReportRoute));
  return FederationRegistryStatus::applied;
}

FederationRegistryStatus EmbeddedFederationRegistry::setPublicServiceReportRoute(
    std::wstring const& federationName,
    std::uint64_t federateId,
    FederatePublicServiceReportRoute publicServiceReportRoute) {
  auto instrumentationScope = beginInstrumentation("setPublicServiceReportRoute");
  if (!publicServiceReportRoute) {
    return FederationRegistryStatus::invalid_request;
  }

  std::scoped_lock lock(mutex_);
  auto federation = federations_.find(federationName);
  if (federation == federations_.end()) {
    return FederationRegistryStatus::federation_does_not_exist;
  }
  if (!federation->second.members.contains(federateId)) {
    return FederationRegistryStatus::federate_not_member;
  }
  federation->second.publicServiceReportRoutes.insert_or_assign(
      federateId,
      std::move(publicServiceReportRoute));
  return FederationRegistryStatus::applied;
}

FederationRegistryResult EmbeddedFederationRegistry::connectionLost(
    std::wstring const& federationName,
    std::uint64_t federateId) {
  auto instrumentationScope = beginInstrumentation("connectionLost");
  std::scoped_lock lock(mutex_);
  auto const federation = federations_.find(federationName);
  if (federation == federations_.end()) {
    return {FederationRegistryStatus::federation_does_not_exist};
  }
  auto const member = federation->second.members.find(federateId);
  if (member == federation->second.members.end()) {
    return {FederationRegistryStatus::federate_not_member};
  }
  return resignLocked(
      federationName,
      federateId,
      member->second.automaticResignAction,
      true,
      std::nullopt,
      false);
}

FederationRegistryResult
EmbeddedFederationRegistry::connectionLostWithFinalServiceReportReservation(
    std::wstring const& federationName,
    std::uint64_t federateId,
    std::uint16_t serviceGroup) {
  auto instrumentationScope = beginInstrumentation(
      "connectionLostWithFinalServiceReportReservation");
  std::scoped_lock lock(mutex_);
  auto const federation = federations_.find(federationName);
  if (federation == federations_.end()) {
    return {FederationRegistryStatus::federation_does_not_exist};
  }
  auto const member = federation->second.members.find(federateId);
  if (member == federation->second.members.end()) {
    return {FederationRegistryStatus::federate_not_member};
  }
  return resignLocked(
      federationName,
      federateId,
      member->second.automaticResignAction,
      true,
      serviceGroup,
      false);
}

void EmbeddedFederationRegistry::appendSaveCompletionNotifications(
    Federation& federation,
    bool successful,
    rti1516_2025::SaveFailureReason failureReason,
    std::vector<FederationSaveNotification>& notifications,
    std::optional<std::uint64_t> excludedFederateId) {
  if (!federation.saveOperation.has_value()) {
    return;
  }

  auto const& operation = *federation.saveOperation;
  notifications.reserve(notifications.size() + operation.statuses.size());
  for (auto const& [federateId, status] : operation.statuses) {
    // A constrained member may not yet have reached its Time Advancing
    // callback boundary for an untimed save. IEEE 1516.1-2025 requires the
    // completion callback only for members at which Initiate Federate Save was
    // actually invoked, not for these still-uninstructed members.
    if (status == rti1516_2025::NO_SAVE_IN_PROGRESS) {
      continue;
    }
    if (excludedFederateId.has_value() && federateId == *excludedFederateId) {
      continue;
    }
    auto const route = federation.interactionCallbackRoutes.find(federateId);
    if (route == federation.interactionCallbackRoutes.end() || !route->second) {
      continue;
    }
    auto const reportRoute = federation.serviceReportRoutes.find(federateId);
    auto const publicReportRoute = federation.publicServiceReportRoutes.find(federateId);
    FederationSaveNotification notification;
    notification.kind = FederationSaveNotificationKind::completed;
    notification.receivingFederateId = federateId;
    notification.label = operation.label;
    notification.successful = successful;
    notification.failureReason = failureReason;
    notification.callbackRoute = route->second;
    notification.serviceReportRoute =
        reportRoute == federation.serviceReportRoutes.end()
        ? FederateServiceReportRoute{}
        : reportRoute->second;
    notification.publicServiceReportRoute =
        publicReportRoute == federation.publicServiceReportRoutes.end()
        ? FederatePublicServiceReportRoute{}
        : publicReportRoute->second;
    notifications.push_back(std::move(notification));
  }
}

void EmbeddedFederationRegistry::appendRestoreCompletionNotifications(
    Federation& federation,
    bool successful,
    rti1516_2025::RestoreFailureReason failureReason,
    std::vector<FederationRestoreNotification>& notifications,
    std::optional<std::uint64_t> excludedFederateId) {
  if (!federation.restoreOperation.has_value()) {
    return;
  }

  auto const& operation = *federation.restoreOperation;
  notifications.reserve(notifications.size() + operation.statuses.size());
  for (auto const& [federateId, status] : operation.statuses) {
    static_cast<void>(status);
    if (excludedFederateId.has_value() && federateId == *excludedFederateId) {
      continue;
    }
    auto const member = federation.members.find(federateId);
    auto const route = federation.interactionCallbackRoutes.find(federateId);
    if (member == federation.members.end() ||
        route == federation.interactionCallbackRoutes.end() || !route->second) {
      continue;
    }
    auto const publicReportRoute = federation.publicServiceReportRoutes.find(federateId);
    FederationRestoreNotification notification;
    notification.kind = FederationRestoreNotificationKind::completed;
    notification.receivingFederateId = federateId;
    notification.label = operation.label;
    notification.federateName = member->second.name;
    notification.preRestoreFederateId = federateId;
    notification.postRestoreFederateId = federateId;
    notification.successful = successful;
    notification.failureReason = failureReason;
    notification.callbackRoute = route->second;
    notification.publicServiceReportRoute =
        publicReportRoute == federation.publicServiceReportRoutes.end()
        ? FederatePublicServiceReportRoute{}
        : publicReportRoute->second;
    notifications.push_back(std::move(notification));
  }
}

FederationRegistryStatus
EmbeddedFederationRegistry::recordSuccessfulUpdateAttributeValues(
    std::wstring const& federationName,
    std::uint64_t federateId,
    std::uint64_t objectInstanceHandle,
    std::uint64_t registeredObjectClassHandle,
    std::set<std::string> const& transportationNames,
    std::vector<std::pair<
        std::uint64_t,
        rti1516_2025::VariableLengthData>> const* acceptedAttributeValues) {
  auto instrumentationScope = beginInstrumentation(
      "recordSuccessfulUpdateAttributeValues");
  std::scoped_lock lock(mutex_);
  auto federation = federations_.find(federationName);
  if (federation == federations_.end()) {
    return FederationRegistryStatus::federation_does_not_exist;
  }
  auto member = federation->second.members.find(federateId);
  if (member == federation->second.members.end()) {
    return FederationRegistryStatus::federate_not_member;
  }
  if (acceptedAttributeValues != nullptr) {
    auto const objectInstance = federation->second.objectInstances.find(
        objectInstanceHandle);
    if (objectInstance == federation->second.objectInstances.end() ||
        objectInstance->second.deleteAccepted) {
      return FederationRegistryStatus::invalid_request;
    }
    std::set<std::uint64_t> valueHandles;
    for (auto const& [attributeHandle, value] : *acceptedAttributeValues) {
      static_cast<void>(value);
      if (attributeHandle == 0U ||
          !valueHandles.insert(attributeHandle).second) {
        return FederationRegistryStatus::invalid_request;
      }
      auto const owner = objectInstance->second.attributeOwnersByHandle.find(
          attributeHandle);
      if (owner == objectInstance->second.attributeOwnersByHandle.end() ||
          owner->second != federateId) {
        return FederationRegistryStatus::invalid_request;
      }
    }
  }
  // Value validation is complete before either the telemetry counters or the
  // application-value ledger is mutated, keeping this accepted boundary
  // atomic for callers that retain the copied value vector.
  if (acceptedAttributeValues != nullptr) {
    auto& objectInstance = federation->second.objectInstances.at(objectInstanceHandle);
    for (auto const& [attributeHandle, value] : *acceptedAttributeValues) {
      objectInstance.attributeValues.insert_or_assign(attributeHandle, value);
    }
  }
  if (member->second.successfulUpdateAttributeValuesCount !=
      std::numeric_limits<std::uint64_t>::max()) {
    ++member->second.successfulUpdateAttributeValuesCount;
  }
  member->second.successfullyUpdatedObjectInstanceHandles.insert(
      objectInstanceHandle);
  // Preserve the registered class at the accepted update boundary. The live
  // object ledger may remove the instance later, but the MOM request/report
  // statistic is scoped to the joined-federate lifetime rather than to live
  // objects only.
  auto const objectInstance = federation->second.objectInstances.find(
      objectInstanceHandle);
  if (registeredObjectClassHandle == 0U &&
      objectInstance != federation->second.objectInstances.end() &&
      !objectInstance->second.deleteAccepted) {
    registeredObjectClassHandle = objectInstance->second.registeredObjectClassHandle;
  }
  if (registeredObjectClassHandle != 0U) {
    member->second.successfullyUpdatedObjectInstanceClassHandles.insert_or_assign(
        objectInstanceHandle,
        registeredObjectClassHandle);
    for (auto const& transportationName : transportationNames) {
      auto& count = member->second.successfulUpdateCountsByClassAndTransportation[
          registeredObjectClassHandle][transportationName];
      if (count != std::numeric_limits<std::uint64_t>::max()) {
        ++count;
      }
    }
  }
  return FederationRegistryStatus::applied;
}

FederationRegistryStatus
EmbeddedFederationRegistry::recordSuccessfulObjectInstanceReflection(
    std::wstring const& federationName,
    std::uint64_t federateId,
    std::uint64_t objectInstanceHandle) {
  auto instrumentationScope = beginInstrumentation(
      "recordSuccessfulObjectInstanceReflection");
  std::scoped_lock lock(mutex_);
  auto federation = federations_.find(federationName);
  if (federation == federations_.end()) {
    return FederationRegistryStatus::federation_does_not_exist;
  }
  auto member = federation->second.members.find(federateId);
  if (member == federation->second.members.end()) {
    return FederationRegistryStatus::federate_not_member;
  }
  auto instance = federation->second.objectInstances.find(objectInstanceHandle);
  if (instance == federation->second.objectInstances.end() ||
      instance->second.deleteAccepted ||
      !instance->second.knownObjectClassHandlesByFederate.contains(federateId)) {
    return FederationRegistryStatus::invalid_request;
  }
  member->second.successfullyReflectedObjectInstanceHandles.insert(
      objectInstanceHandle);
  // Preserve the registered class at the accepted reflection boundary. The
  // object may be removed before a later MOM request, but the reflected
  // object-instance report remains scoped to this joined-federate lifetime.
  member->second.successfullyReflectedObjectInstanceClassHandles.insert_or_assign(
      objectInstanceHandle,
      instance->second.registeredObjectClassHandle);
  return FederationRegistryStatus::applied;
}

FederationRegistryStatus
EmbeddedFederationRegistry::recordSuccessfulInteractionSend(
    std::wstring const& federationName,
    std::uint64_t federateId,
    std::uint64_t sentInteractionClassHandle,
    std::string const& transportationName,
    bool directed) {
  auto instrumentationScope = beginInstrumentation(
      "recordSuccessfulInteractionSend");
  std::scoped_lock lock(mutex_);
  auto federation = federations_.find(federationName);
  if (federation == federations_.end()) {
    return FederationRegistryStatus::federation_does_not_exist;
  }
  auto member = federation->second.members.find(federateId);
  if (member == federation->second.members.end()) {
    return FederationRegistryStatus::federate_not_member;
  }
  // The ambassador calls this only after the corresponding ordinary,
  // directed, timestamped, or regional service has passed its final
  // synchronous validation/queue-admission boundary.  Keep one sender-side
  // count independent of recipient fan-out; directed sends additionally feed
  // the MIM-directed subset.
  if (member->second.successfulInteractionsSentCount !=
      std::numeric_limits<std::uint64_t>::max()) {
    ++member->second.successfulInteractionsSentCount;
  }
  if (directed && member->second.successfulDirectedInteractionsSentCount !=
                      std::numeric_limits<std::uint64_t>::max()) {
    ++member->second.successfulDirectedInteractionsSentCount;
  }
  if (sentInteractionClassHandle != 0U &&
      isSupportedTransportationName(
          federation->second.definition.catalog.get(),
          transportationName)) {
    auto& count = member->second.successfulInteractionCountsByClassAndTransportation[
        sentInteractionClassHandle][transportationName];
    if (count != std::numeric_limits<std::uint64_t>::max()) {
      ++count;
    }
    if (directed) {
      auto& directedCount =
          member->second.successfulDirectedInteractionCountsByClassAndTransportation[
              sentInteractionClassHandle][transportationName];
      if (directedCount != std::numeric_limits<std::uint64_t>::max()) {
        ++directedCount;
      }
    }
  }
  return FederationRegistryStatus::applied;
}

FederationRegistryStatus
EmbeddedFederationRegistry::recordSuccessfulInteractionReceipt(
    std::wstring const& federationName,
    std::uint64_t federateId,
    std::uint64_t receivedInteractionClassHandle,
    std::string const& transportationName,
    bool directed) {
  auto instrumentationScope = beginInstrumentation(
      "recordSuccessfulInteractionReceipt");
  std::scoped_lock lock(mutex_);
  auto federation = federations_.find(federationName);
  if (federation == federations_.end()) {
    return FederationRegistryStatus::federation_does_not_exist;
  }
  auto member = federation->second.members.find(federateId);
  if (member == federation->second.members.end()) {
    return FederationRegistryStatus::federate_not_member;
  }
  if (member->second.successfulInteractionsReceivedCount !=
      std::numeric_limits<std::uint64_t>::max()) {
    ++member->second.successfulInteractionsReceivedCount;
  }
  if (directed && member->second.successfulDirectedInteractionsReceivedCount !=
                      std::numeric_limits<std::uint64_t>::max()) {
    ++member->second.successfulDirectedInteractionsReceivedCount;
  }
  if (receivedInteractionClassHandle != 0U &&
      isSupportedTransportationName(
          federation->second.definition.catalog.get(),
          transportationName)) {
    auto& count =
        member->second.successfulInteractionReceiptCountsByClassAndTransportation[
            receivedInteractionClassHandle][transportationName];
    if (count != std::numeric_limits<std::uint64_t>::max()) {
      ++count;
    }
    if (directed) {
      auto& directedCount =
          member->second.successfulDirectedInteractionReceiptCountsByClassAndTransportation[
              receivedInteractionClassHandle][transportationName];
      if (directedCount != std::numeric_limits<std::uint64_t>::max()) {
        ++directedCount;
      }
    }
  }
  return FederationRegistryStatus::applied;
}

FederationRegistryStatus
EmbeddedFederationRegistry::recordSuccessfulReflectionReceipt(
    std::wstring const& federationName,
    std::uint64_t federateId,
    std::uint64_t objectInstanceHandle,
    std::string const& transportationName) {
  auto instrumentationScope = beginInstrumentation(
      "recordSuccessfulReflectionReceipt");
  std::scoped_lock lock(mutex_);
  auto federation = federations_.find(federationName);
  if (federation == federations_.end()) {
    return FederationRegistryStatus::federation_does_not_exist;
  }
  auto member = federation->second.members.find(federateId);
  if (member == federation->second.members.end()) {
    return FederationRegistryStatus::federate_not_member;
  }
  if (member->second.successfulReflectionsReceivedCount !=
      std::numeric_limits<std::uint64_t>::max()) {
    ++member->second.successfulReflectionsReceivedCount;
  }
  if (isSupportedTransportationName(
          federation->second.definition.catalog.get(),
          transportationName)) {
    auto const instance = federation->second.objectInstances.find(
        objectInstanceHandle);
    if (instance != federation->second.objectInstances.end() &&
        !instance->second.deleteAccepted &&
        instance->second.registeredObjectClassHandle != 0U) {
      auto& count = member->second
          .successfulReflectionCountsByClassAndTransportation[
              instance->second.registeredObjectClassHandle][transportationName];
      if (count != std::numeric_limits<std::uint64_t>::max()) {
        ++count;
      }
    }
  }
  return FederationRegistryStatus::applied;
}

bool EmbeddedFederationRegistry::contains(std::wstring const& federationName) const {
  std::scoped_lock lock(mutex_);
  return federations_.contains(federationName);
}

std::size_t EmbeddedFederationRegistry::memberCount(std::wstring const& federationName) const {
  std::scoped_lock lock(mutex_);
  auto const federation = federations_.find(federationName);
  return federation == federations_.end() ? 0 : federation->second.members.size();
}

std::optional<std::uint64_t>
EmbeddedFederationRegistry::candidateObjectInstanceDiscoveryClass(
    Federation const& federation,
    Federation::ObjectInstance const& objectInstance,
    std::uint64_t receivingFederateId,
    std::map<std::uint64_t, RegionSpecificationSnapshot> const* regionOverrides,
    std::map<std::uint64_t, std::set<std::uint64_t>> const* updateRegionOverrides) {
  // Registration makes the object known to its producer. Discovery requires
  // a different joined federate to own a qualifying instance attribute, so
  // the producer cannot receive an induced Discover Object Instance callback.
  if (objectInstance.deleteAccepted ||
      objectInstance.producingFederateId == receivingFederateId ||
      !federation.members.contains(receivingFederateId) ||
      !federation.definition.catalog ||
      !federation.objectClassHandles ||
      !federation.attributeHandles) {
    return std::nullopt;
  }
  // A producer that has resigned normally stops being a discovery source. An
  // exception is an object retained by an active ownership-assumption search:
  // later joined federates must still be able to discover it before becoming
  // eligible recipients of the Request Attribute Ownership Assumption
  // callback.
  if (!federation.members.contains(objectInstance.producingFederateId) &&
      objectInstance.ownershipAssumptionRecipientsByAttribute.empty()) {
    return std::nullopt;
  }

  auto const registeredClassName = federation.objectClassHandles->nameFor(
      objectInstance.registeredObjectClassHandle);
  if (!registeredClassName ||
      federation.definition.catalog->objectClass(*registeredClassName) == nullptr) {
    return std::nullopt;
  }

  auto const declarations = federation.objectClassAttributeDeclarations.find(receivingFederateId);
  if (declarations == federation.objectClassAttributeDeclarations.end()) {
    return std::nullopt;
  }

  // Passive subscriptions do not establish registration relevance, but they
  // still receive the discovery made relevant by another joined federate's
  // active declaration. Evaluate that federation-wide relevance against the
  // registered class and the source object's current region realization.
  bool activeSubscriptionEstablished = false;
  for (auto const& [candidateFederateId, candidateDeclarations] :
       federation.objectClassAttributeDeclarations) {
    if (candidateFederateId == objectInstance.producingFederateId ||
        !federation.members.contains(candidateFederateId)) {
      continue;
    }
    std::set<std::string> candidateVisited;
    std::string candidateClassName = *registeredClassName;
    while (!candidateClassName.empty() && candidateVisited.insert(candidateClassName).second) {
      auto const* candidateClass = federation.definition.catalog->objectClass(candidateClassName);
      auto const candidateClassHandle = federation.objectClassHandles->handleFor(
          candidateClassName);
      if (candidateClass == nullptr || !candidateClassHandle) {
        break;
      }
      auto const candidatePerClass = candidateDeclarations.byObjectClass.find(
          *candidateClassHandle);
      if (candidatePerClass != candidateDeclarations.byObjectClass.end()) {
        for (auto const& [ownedAttributeHandle, owningFederateId] :
             objectInstance.attributeOwnersByHandle) {
          static_cast<void>(owningFederateId);
          if (!federation.attributeHandles->nameFor(
                  federation.definition.catalog.get(),
                  candidateClassName,
                  ownedAttributeHandle)) {
            continue;
          }
          auto const regional = candidatePerClass->second.regionalSubscribedAttributes.find(
              ownedAttributeHandle);
          auto const& updateRegions = updateRegionOverrides != nullptr
              ? *updateRegionOverrides
              : objectInstance.updateRegionsByAttribute;
          auto const associated = updateRegions.find(ownedAttributeHandle);
          bool const hasExplicitSubscriptionRegion =
              regional != candidatePerClass->second.regionalSubscribedAttributes.end() &&
              !regional->second.empty();
          bool const hasExplicitUpdateRegion =
              associated != updateRegions.end() && !associated->second.empty();
          auto const ordinarySubscription = candidatePerClass->second.subscribedAttributes.find(
              ownedAttributeHandle);
          if (ordinarySubscription != candidatePerClass->second.subscribedAttributes.end() &&
              ordinarySubscription->second) {
            if (!hasExplicitUpdateRegion) {
              activeSubscriptionEstablished = true;
              break;
            }
            for (std::uint64_t const associatedRegionHandle : associated->second) {
              if (regionOverlapsDefault(
                      federation,
                      associatedRegionHandle,
                      regionOverrides)) {
                activeSubscriptionEstablished = true;
                break;
              }
            }
            if (activeSubscriptionEstablished) {
              break;
            }
          }
          if (!hasExplicitSubscriptionRegion) {
            continue;
          }
          for (auto const& [subscribedRegionHandle, active] : regional->second) {
            if (!active) {
              continue;
            }
            if (!hasExplicitUpdateRegion) {
              if (regionOverlapsDefault(
                      federation,
                      subscribedRegionHandle,
                      regionOverrides)) {
                activeSubscriptionEstablished = true;
                break;
              }
              continue;
            }
            for (std::uint64_t const associatedRegionHandle : associated->second) {
              if (regionsOverlap(
                      federation,
                      subscribedRegionHandle,
                      associatedRegionHandle,
                      regionOverrides)) {
                activeSubscriptionEstablished = true;
                break;
              }
            }
            if (activeSubscriptionEstablished) {
              break;
            }
          }
          if (activeSubscriptionEstablished) {
            break;
          }
        }
      }
      if (activeSubscriptionEstablished) {
        break;
      }
      candidateClassName = candidateClass->parentName;
    }
    if (activeSubscriptionEstablished) {
      break;
    }
  }

  // The candidate discovery class is the registered class when subscribed,
  // otherwise the closest subscribed superclass. Once a candidate is found,
  // only subscriptions at that candidate may cause discovery; an ancestor
  // must not leak a more-specific instance attribute into the callback.
  std::set<std::string> visited;
  std::string currentClassName = *registeredClassName;
  while (!currentClassName.empty() && visited.insert(currentClassName).second) {
    auto const* currentClass = federation.definition.catalog->objectClass(currentClassName);
    auto const currentClassHandle = federation.objectClassHandles->handleFor(currentClassName);
    if (currentClass == nullptr || !currentClassHandle) {
      return std::nullopt;
    }

    auto const perClass = declarations->second.byObjectClass.find(*currentClassHandle);
    if (perClass != declarations->second.byObjectClass.end()) {
      // A prior unconditional divestiture/resign action can leave the
      // instance attributes unowned while the RTI is still searching for a
      // new owner. Those attributes remain valid discovery seeds; otherwise
      // a federate joining after the divestiture could never become eligible
      // for the required assumption callback. Ordinary instances retain the
      // historical owner-based predicate because this search set is absent.
      std::set<std::uint64_t> discoverableAttributeHandles;
      for (auto const& [ownedAttributeHandle, owningFederateId] :
           objectInstance.attributeOwnersByHandle) {
        static_cast<void>(owningFederateId);
        discoverableAttributeHandles.insert(ownedAttributeHandle);
      }
      for (auto const& [searchAttributeHandle, recipients] :
           objectInstance.ownershipAssumptionRecipientsByAttribute) {
        static_cast<void>(recipients);
        discoverableAttributeHandles.insert(searchAttributeHandle);
      }
      for (std::uint64_t const ownedAttributeHandle : discoverableAttributeHandles) {
        if (!federation.attributeHandles->nameFor(
                federation.definition.catalog.get(),
                currentClassName,
                ownedAttributeHandle)) {
          continue;
        }
        auto const regional = perClass->second.regionalSubscribedAttributes.find(
            ownedAttributeHandle);
        auto const& updateRegions = updateRegionOverrides != nullptr
            ? *updateRegionOverrides
            : objectInstance.updateRegionsByAttribute;
        auto const associated = updateRegions.find(ownedAttributeHandle);
        bool const hasExplicitSubscriptionRegion =
            regional != perClass->second.regionalSubscribedAttributes.end() &&
            !regional->second.empty();
        bool const hasExplicitUpdateRegion =
            associated != updateRegions.end() &&
            !associated->second.empty();

        // Ordinary and regional declarations are independent §9.8 routes.
        // Keep the ordinary default-region realization effective even when a
        // separate regional declaration exists for the same class/attribute.
        auto const ordinarySubscription = perClass->second.subscribedAttributes.find(
            ownedAttributeHandle);
        if (ordinarySubscription != perClass->second.subscribedAttributes.end() &&
            (ordinarySubscription->second || activeSubscriptionEstablished)) {
          if (!hasExplicitUpdateRegion) {
            return currentClassHandle;
          }
          for (std::uint64_t const associatedRegionHandle : associated->second) {
            if (regionOverlapsDefault(
                    federation,
                    associatedRegionHandle,
                    regionOverrides)) {
              return currentClassHandle;
            }
          }
        }
        if (!hasExplicitSubscriptionRegion) {
          continue;
        }
        for (auto const& [subscribedRegionHandle, active] : regional->second) {
          if (!active && !activeSubscriptionEstablished) {
            continue;
          }
          if (!hasExplicitUpdateRegion) {
            if (regionOverlapsDefault(
                    federation,
                    subscribedRegionHandle,
                    regionOverrides)) {
              return currentClassHandle;
            }
            continue;
          }
          for (std::uint64_t const associatedRegionHandle : associated->second) {
            if (regionsOverlap(
                    federation,
                    subscribedRegionHandle,
                    associatedRegionHandle,
                    regionOverrides)) {
              return currentClassHandle;
            }
          }
        }
      }
    }

    currentClassName = currentClass->parentName;
  }
  return std::nullopt;
}

std::optional<KnownObjectInstanceSnapshot>
EmbeddedFederationRegistry::knownObjectInstanceSnapshot(
    Federation const& federation,
    Federation::ObjectInstance const& objectInstance,
    std::uint64_t federateId) {
  auto const knownClass = objectInstance.knownObjectClassHandlesByFederate.find(federateId);
  if (knownClass == objectInstance.knownObjectClassHandlesByFederate.end()) {
    return std::nullopt;
  }
  return KnownObjectInstanceSnapshot{
      objectInstance.handle,
      knownClass->second,
      objectInstance.name,
      objectInstance.producingFederateId,
  };
}

bool EmbeddedFederationRegistry::canPurgeDeletedObjectInstance(
    Federation::ObjectInstance const& objectInstance) noexcept {
  return objectInstance.deleteAccepted &&
         objectInstance.knownObjectClassHandlesByFederate.empty() &&
         objectInstance.pendingDiscoveryFederates.empty() &&
         objectInstance.pendingRemovalFederates.empty() &&
         !objectInstance.pendingTimestampedDeletionMessageId.has_value() &&
         objectInstance.pendingTimestampedRemovalFederates.empty();
}

bool EmbeddedFederationRegistry::hasPendingConnectionLossTsoObjectDelivery(
    Federation const& federation,
    std::uint64_t objectInstanceHandle,
    std::uint64_t receivingFederateId) noexcept {
  if (objectInstanceHandle == 0 || receivingFederateId == 0) {
    return false;
  }

  auto const recipientStillNeedsMarkedDelivery =
      [&federation, receivingFederateId](std::uint64_t messageId) {
        auto const record = federation.tsoRequestRetractionRecords.find(messageId);
        if (record == federation.tsoRequestRetractionRecords.end() ||
            !record->second.deliveryRequiredAfterConnectionLoss) {
          return false;
        }
        auto const recipient = record->second.recipientStates.find(receivingFederateId);
        return recipient != record->second.recipientStates.end() &&
            recipient->second == Federation::TsoRecipientDeliveryState::pending;
      };

  // A timestamped update needs the recipient's known object so its delayed
  // Reflect Attribute Values projection remains valid. A directed interaction
  // has the same dependency on the target object. Ordinary interactions have
  // no object-instance lifetime dependency, while timestamped deletion has
  // its own explicit reconstitution/pending-removal machinery.
  for (auto const& [messageId, message] : federation.tsoAttributeUpdateMessages) {
    if (message.objectInstanceHandle == objectInstanceHandle &&
        recipientStillNeedsMarkedDelivery(messageId)) {
      return true;
    }
  }
  for (auto const& [messageId, message] : federation.tsoDirectedInteractionMessages) {
    if (message.objectInstanceHandle == objectInstanceHandle &&
        recipientStillNeedsMarkedDelivery(messageId)) {
      return true;
    }
  }
  return false;
}

bool EmbeddedFederationRegistry::hasPendingTsoRecipient(
    Federation const& federation,
    Federation::TsoRequestRetractionRecord const& record) noexcept {
  return std::any_of(
      record.recipientStates.begin(),
      record.recipientStates.end(),
      [&federation](auto const& recipient) {
        return recipient.second == Federation::TsoRecipientDeliveryState::pending &&
            federation.members.contains(recipient.first);
      });
}

std::optional<std::shared_ptr<rti1516_2025::LogicalTime const>>
EmbeddedFederationRegistry::earliestConnectionLossTsoDeliveryBoundary(
    Federation const& federation,
    std::uint64_t recipientFederateId) {
  std::optional<std::shared_ptr<rti1516_2025::LogicalTime const>> earliest;
  auto const queued = federation.timeCoordinator.tsoSnapshotFor(recipientFederateId).queued;
  for (auto const& message : queued) {
    if (!message.timestamp) {
      continue;
    }
    auto const record = federation.tsoRequestRetractionRecords.find(message.messageId);
    if (record == federation.tsoRequestRetractionRecords.end() ||
        !record->second.deliveryRequiredAfterConnectionLoss) {
      continue;
    }
    if (!earliest) {
      earliest = message.timestamp;
      continue;
    }
    try {
      if (*message.timestamp < **earliest) {
        earliest = message.timestamp;
      }
    } catch (rti1516_2025::Exception const&) {
      // A malformed private timestamp cannot safely create a mandatory
      // connection-loss delivery boundary. Leave the already valid earliest
      // entry unchanged and let normal validation classify later work.
    }
  }
  return earliest;
}

bool EmbeddedFederationRegistry::connectionLossTsoDeliveryMayGrant(
    Federation const& federation,
    std::uint64_t recipientFederateId,
    FederateTimeSnapshot const& requester,
    FederationTimeBounds const& bounds) {
  if (bounds.status != FederationTimeBoundStatus::undefined ||
      !requester.requestedTime) {
    return false;
  }

  // Once the failed regulator has been removed, ordinary GALT is undefined.
  // A marked queued message is the narrow 4.4 exception: its recipient may
  // cross its already requested TSO boundary so the source-mandated
  // at-or-before-loss delivery can occur. This does not change generic
  // no-regulated-grant behavior, nor does it force a message after the
  // captured loss cutoff to be delivered.
  auto const deliveryBoundary = earliestConnectionLossTsoDeliveryBoundary(
      federation,
      recipientFederateId);
  if (!deliveryBoundary ||
      requester.requestedTime->implementationName() !=
          (*deliveryBoundary)->implementationName()) {
    return false;
  }
  try {
    return *requester.requestedTime >= **deliveryBoundary;
  } catch (rti1516_2025::Exception const&) {
    return false;
  }
}

void EmbeddedFederationRegistry::reclaimTsoMessagePayload(
    Federation& federation,
    std::uint64_t messageId) {
  auto const record = federation.tsoRequestRetractionRecords.find(messageId);
  if (record == federation.tsoRequestRetractionRecords.end() ||
      hasPendingTsoRecipient(federation, record->second)) {
    return;
  }

  // All still-joined recipients have crossed (or abandoned) the original
  // message boundary, so the connection-loss-only grant exception has no
  // remaining delivery work to protect.
  record->second.deliveryRequiredAfterConnectionLoss = false;
  // Commit a timestamped application's opaque values before retiring the
  // heavyweight payload. The helper ignores retracted messages, making this
  // safe for both the no-recipient path and the final recipient boundary.
  applyTsoAttributeUpdateValues(federation, messageId);

  // Normal interaction/update/directed payloads are needed only until every
  // still-joined recipient has crossed its original callback boundary.  A
  // live retraction ledger retains the recipient states needed for a later
  // Request Retraction callback, so it does not need to retain the payload.
  federation.tsoInteractionMessages.erase(messageId);
  federation.tsoAttributeUpdateMessages.erase(messageId);
  federation.tsoDirectedInteractionMessages.erase(messageId);

  // A timestamped object deletion is different: until the designator becomes
  // terminal, its invocation snapshot is still required to reconstitute the
  // object and original ownership after a legal Retract.
  if (!record->second.terminal) {
    return;
  }

  auto const deletion = federation.tsoObjectDeletionMessages.find(messageId);
  if (deletion == federation.tsoObjectDeletionMessages.end()) {
    return;
  }
  auto const objectInstanceHandle = deletion->second.objectInstanceHandle;
  federation.tsoObjectDeletionReconstitutionRecords.erase(messageId);
  federation.tsoObjectDeletionMessages.erase(deletion);

  auto const objectInstance = federation.objectInstances.find(objectInstanceHandle);
  if (objectInstance == federation.objectInstances.end() ||
      objectInstance->second.pendingTimestampedDeletionMessageId != messageId) {
    return;
  }
  objectInstance->second.pendingTimestampedDeletionMessageId.reset();
  objectInstance->second.pendingTimestampedRemovalFederates.clear();
  if (canPurgeDeletedObjectInstance(objectInstance->second)) {
    federation.objectInstanceHandlesByName.erase(objectInstance->second.name);
    federation.objectInstances.erase(objectInstance);
    refreshRegionUsage(federation);
  }
}

void EmbeddedFederationRegistry::applyTsoAttributeUpdateValues(
    Federation& federation,
    std::uint64_t messageId) {
  auto const record = federation.tsoRequestRetractionRecords.find(messageId);
  if (record == federation.tsoRequestRetractionRecords.end() ||
      record->second.retractionApplied) {
    return;
  }
  auto const message = federation.tsoAttributeUpdateMessages.find(messageId);
  if (message == federation.tsoAttributeUpdateMessages.end()) {
    return;
  }
  auto object = federation.objectInstances.find(message->second.objectInstanceHandle);
  if (object == federation.objectInstances.end() || object->second.deleteAccepted) {
    return;
  }
  for (auto const& [attributeHandle, value] : message->second.attributes) {
    if (attributeHandle != 0U) {
      object->second.attributeValues.insert_or_assign(attributeHandle, value);
    }
  }
}

void EmbeddedFederationRegistry::reclaimTsoMessagePayloads(Federation& federation) {
  for (auto const& [messageId, record] : federation.tsoRequestRetractionRecords) {
    static_cast<void>(record);
    reclaimTsoMessagePayload(federation, messageId);
  }
}


std::optional<AttributeOwnershipQueryRecipient>
EmbeddedFederationRegistry::candidateAttributeOwnershipQueryRecipient(
    Federation const& federation,
    std::uint64_t requestingFederateId,
    std::uint64_t objectInstanceHandle,
    AttributeOwnershipQueryReportKind reportKind,
    std::uint64_t owningFederateId,
    std::set<std::uint64_t> const& requestedAttributeHandles) {
  if (!federation.members.contains(requestingFederateId) ||
      !federation.definition.catalog ||
      !federation.objectClassHandles ||
      !federation.attributeHandles) {
    return std::nullopt;
  }

  // RTI-owned joined-federate MOM objects have a separate lifetime and known
  // instance ledger. Keep this branch before the federate-created object map:
  // the object handle namespace is shared, but the two ownership models are
  // deliberately not represented by one synthetic owner ID.
  auto const momObject = federation.rtiOwnedJoinedFederateMomObjects.find(
      objectInstanceHandle);
  bool const rtiOwnedMomObject =
      momObject != federation.rtiOwnedJoinedFederateMomObjects.end();
  auto const instance = federation.objectInstances.find(objectInstanceHandle);
  // A pending Remove Object Instance invalidates every queued ownership report
  // for that instance before a federate callback may be made.
  if (!rtiOwnedMomObject &&
      (instance == federation.objectInstances.end() || instance->second.deleteAccepted)) {
    return std::nullopt;
  }

  std::uint64_t knownClassHandle = 0;
  if (rtiOwnedMomObject) {
    if (!momObject->second.knownFederateIds.contains(requestingFederateId)) {
      return std::nullopt;
    }
    knownClassHandle = momObject->second.objectClassHandle;
  } else {
    auto const knownClass = instance->second.knownObjectClassHandlesByFederate.find(
        requestingFederateId);
    if (knownClass == instance->second.knownObjectClassHandlesByFederate.end()) {
      return std::nullopt;
    }
    knownClassHandle = knownClass->second;
  }
  auto const knownClassName = federation.objectClassHandles->nameFor(knownClassHandle);
  if (!knownClassName ||
      federation.definition.catalog->objectClass(*knownClassName) == nullptr) {
    return std::nullopt;
  }

  AttributeOwnershipQueryRecipient recipient;
  recipient.receivingFederateId = requestingFederateId;
  recipient.objectInstanceHandle = objectInstanceHandle;
  recipient.reportKind = reportKind;
  recipient.owningFederateId = owningFederateId;
  for (std::uint64_t const attributeHandle : requestedAttributeHandles) {
    if (!federation.attributeHandles->nameFor(
            federation.definition.catalog.get(),
            *knownClassName,
            attributeHandle)) {
      continue;
    }

    if (rtiOwnedMomObject &&
        !momObject->second.effectiveAttributeHandles.contains(attributeHandle)) {
      continue;
    }
    std::optional<std::uint64_t> ownerId;
    if (!rtiOwnedMomObject) {
      auto const owner = instance->second.attributeOwnersByHandle.find(attributeHandle);
      if (owner != instance->second.attributeOwnersByHandle.end()) {
        ownerId = owner->second;
      }
    }
    switch (reportKind) {
      case AttributeOwnershipQueryReportKind::federate:
        if (!rtiOwnedMomObject &&
            ownerId && *ownerId == owningFederateId &&
            federation.members.contains(owningFederateId)) {
          recipient.attributeHandles.insert(attributeHandle);
        }
        break;
      case AttributeOwnershipQueryReportKind::unowned:
        if (!rtiOwnedMomObject &&
            !ownerId) {
          recipient.attributeHandles.insert(attributeHandle);
        }
        break;
      case AttributeOwnershipQueryReportKind::rti:
        if (rtiOwnedMomObject) {
          recipient.attributeHandles.insert(attributeHandle);
        }
        break;
    }
  }
  if (recipient.attributeHandles.empty()) {
    return std::nullopt;
  }

  auto const callbackRoute = federation.interactionCallbackRoutes.find(requestingFederateId);
  if (callbackRoute == federation.interactionCallbackRoutes.end() || !callbackRoute->second) {
    return std::nullopt;
  }
  recipient.callbackRoute = callbackRoute->second;
  return recipient;
}

std::optional<ReceiveOrderInteractionRecipient>
EmbeddedFederationRegistry::candidateReceiveOrderInteractionRecipient(
    Federation const& federation,
    InteractionProducer const& producingSource,
    std::uint64_t receivingFederateId,
    std::uint64_t sentInteractionClassHandle,
    std::vector<std::uint64_t> const& sentParameterHandles,
    std::set<std::uint64_t> const* sentRegionHandles,
    std::map<std::uint64_t, RegionSpecificationSnapshot> const* regionOverrides) {
  // IEEE 1516.1-2025 prevents a joined-federate sender from receiving its own
  // induced Receive Interaction callback, independent of subscription state.
  // An RTI-originated MOM interaction has no joined-federate source to
  // exclude. Its source remains private until a standards-backed public
  // callback producer-designator rule is available.
  auto const producingFederateId = producingSource.joinedFederateId();
  if ((producingSource.kind() == InteractionProducer::Kind::joined_federate &&
       (!producingFederateId || *producingFederateId == 0U ||
        *producingFederateId == receivingFederateId)) ||
      !federation.members.contains(receivingFederateId) ||
      !federation.definition.catalog ||
      !federation.interactionClassHandles ||
      !federation.parameterHandles) {
    return std::nullopt;
  }

  auto const sentClassName = federation.interactionClassHandles->nameFor(
      sentInteractionClassHandle);
  if (!sentClassName ||
      federation.definition.catalog->interactionClass(*sentClassName) == nullptr) {
    return std::nullopt;
  }

  auto const declarations = federation.interactionDeclarations.find(receivingFederateId);
  if (declarations == federation.interactionDeclarations.end()) {
    return std::nullopt;
  }
  // Passive subscriptions do not establish interaction relevance, but they
  // still receive an interaction when another joined federate's active
  // subscription has already made that interaction relevant.  Keep this
  // federation-wide predicate separate from the recipient's own region
  // overlap check below.  Relevance is declaration-level (the same boundary
  // used by Turn Interactions On/Off advisories), while delivery remains
  // constrained by the passive recipient's matching region.
  bool activeSubscriptionEstablished = false;
  for (auto const& [candidateFederateId, candidateDeclarations] :
       federation.interactionDeclarations) {
    if ((producingSource.kind() == InteractionProducer::Kind::joined_federate &&
         producingFederateId && candidateFederateId == *producingFederateId) ||
        !federation.members.contains(candidateFederateId)) {
      continue;
    }
    std::set<std::string> candidateVisited;
    std::string candidateClassName = *sentClassName;
    while (!candidateClassName.empty() && candidateVisited.insert(candidateClassName).second) {
      auto const candidateClassHandle = federation.interactionClassHandles->handleFor(
          candidateClassName);
      auto const* candidateClass = federation.definition.catalog->interactionClass(
          candidateClassName);
      if (!candidateClassHandle || candidateClass == nullptr) {
        break;
      }
      auto const ordinarySubscription =
          candidateDeclarations.subscribedInteractionClasses.find(*candidateClassHandle);
      if (ordinarySubscription != candidateDeclarations.subscribedInteractionClasses.end() &&
          ordinarySubscription->second) {
        activeSubscriptionEstablished = true;
        break;
      }
      auto const regionalSubscription =
          candidateDeclarations.regionalSubscribedInteractionClasses.find(*candidateClassHandle);
      if (regionalSubscription !=
              candidateDeclarations.regionalSubscribedInteractionClasses.end() &&
          std::any_of(
              regionalSubscription->second.begin(),
              regionalSubscription->second.end(),
              [](auto const& entry) { return entry.second; })) {
        activeSubscriptionEstablished = true;
        break;
      }
      candidateClassName = candidateClass->parentName;
    }
    if (activeSubscriptionEstablished) {
      break;
    }
  }

  // The candidate received class is the sent class when it is subscribed and
  // eligible for this delivery, or the closest eligible subscribed
  // superclass. Traversing the parent chain rather than iterating
  // subscriptions also guarantees at most one callback for a recipient with
  // subscriptions at multiple hierarchy locations.
  std::set<std::string> visited;
  std::string currentClassName = *sentClassName;
  while (!currentClassName.empty() && visited.insert(currentClassName).second) {
    auto const* currentClass = federation.definition.catalog->interactionClass(currentClassName);
    auto const currentClassHandle = federation.interactionClassHandles->handleFor(currentClassName);
    if (currentClass == nullptr || !currentClassHandle) {
      return std::nullopt;
    }

    auto const ordinarySubscription = declarations->second.subscribedInteractionClasses.find(
        *currentClassHandle);
    auto const regional = declarations->second.regionalSubscribedInteractionClasses.find(
        *currentClassHandle);
    bool const hasExplicitSubscriptionRegion =
        regional != declarations->second.regionalSubscribedInteractionClasses.end() &&
        !regional->second.empty();
    bool const hasExplicitSentRegion =
        sentRegionHandles != nullptr && !sentRegionHandles->empty();
    // An ordinary interaction subscription is bound to the RTI-provided
    // default region.  That default overlaps every committed explicit
    // realization that has at least one dimension, but §9.1.3.2 makes an
    // explicitly empty realization overlap nothing (including the default).
    // Keep the explicit-source distinction here instead of treating every
    // non-null sent-region set as a default-fallback suppression: a positive-
    // dimensional Send Interaction With Regions remains eligible for an
    // ordinary subscription once any shadowing regional declaration is gone.
    bool const explicitSentRegionOverlapsDefault =
        hasExplicitSentRegion &&
        std::any_of(
            sentRegionHandles->begin(),
            sentRegionHandles->end(),
            [&federation, regionOverrides](std::uint64_t regionHandle) {
              return regionOverlapsDefault(federation, regionHandle, regionOverrides);
            });
    bool const subscribedWithoutRegion =
        ordinarySubscription != declarations->second.subscribedInteractionClasses.end() &&
        (ordinarySubscription->second || activeSubscriptionEstablished) &&
        !hasExplicitSubscriptionRegion &&
        (!hasExplicitSentRegion || explicitSentRegionOverlapsDefault);
    bool subscribedWithRegion = false;
    if (!subscribedWithoutRegion && hasExplicitSubscriptionRegion) {
      if (sentRegionHandles == nullptr) {
        for (auto const& [subscribedRegionHandle, active] : regional->second) {
          if ((active || activeSubscriptionEstablished) && regionOverlapsDefault(
                                                           federation,
                                                           subscribedRegionHandle,
                                                           regionOverrides)) {
            subscribedWithRegion = true;
            break;
          }
        }
      } else if (!sentRegionHandles->empty()) {
        for (auto const& [subscribedRegionHandle, active] : regional->second) {
          if (!active && !activeSubscriptionEstablished) {
            continue;
          }
          for (std::uint64_t const sentRegionHandle : *sentRegionHandles) {
            if (regionsOverlap(
                    federation, subscribedRegionHandle, sentRegionHandle, regionOverrides)) {
              subscribedWithRegion = true;
              break;
            }
          }
          if (subscribedWithRegion) {
            break;
          }
        }
      }
    }

    if (subscribedWithoutRegion || subscribedWithRegion) {
      ReceiveOrderInteractionRecipient recipient;
      recipient.federateId = receivingFederateId;
      recipient.conveyRegionDesignatorSets =
          federation.members.at(receivingFederateId).conveyRegionDesignatorSetsSwitch;
      recipient.receivedInteractionClassHandle = *currentClassHandle;
      auto const callbackRoute = federation.interactionCallbackRoutes.find(receivingFederateId);
      if (callbackRoute == federation.interactionCallbackRoutes.end() ||
          !callbackRoute->second) {
        // An eligible subscription without a live callback endpoint cannot be
        // delivered. Keep this planner consistent with object/attribute and
        // ownership callback planning instead of returning a route-less
        // recipient that the adapter can never expose to user code.
        return std::nullopt;
      }
      recipient.callbackRoute = callbackRoute->second;
      for (std::uint64_t const parameterHandle : sentParameterHandles) {
        // ParameterHandleDirectory resolves a parameter through the supplied
        // received class and its superclasses.  A parameter introduced below
        // that class is therefore excluded exactly as 2025 promotion requires.
        if (federation.parameterHandles->nameFor(
                federation.definition.catalog.get(),
                currentClassName,
                parameterHandle)) {
          recipient.receivedParameterHandles.insert(parameterHandle);
        }
      }
      return recipient;
    }

    currentClassName = currentClass->parentName;
  }
  return std::nullopt;
}











std::optional<ExceptionReportRouting>
EmbeddedFederationRegistry::exceptionReportRoutingFor(
    Federation const& federation,
    std::uint64_t reportedFederateId) {
  if (reportedFederateId == 0U || !federation.definition.catalog ||
      !federation.interactionClassHandles || !federation.parameterHandles ||
      !federation.dimensionHandles) {
    return std::nullopt;
  }

  auto const reportClassHandle = federation.interactionClassHandles->handleFor(
      kReportExceptionInteractionClassName);
  auto const federateDimensionHandle = federation.dimensionHandles->handleFor(
      kHlaFederateDimensionName);
  auto const serviceParameterHandle = federation.parameterHandles->handleFor(
      federation.definition.catalog.get(),
      kReportExceptionInteractionClassName,
      kExceptionReportServiceParameterName);
  auto const federateParameterHandle = federation.parameterHandles->handleFor(
      federation.definition.catalog.get(),
      kReportExceptionInteractionClassName,
      kFederateLostFederateParameterName);
  auto const exceptionParameterHandle = federation.parameterHandles->handleFor(
      federation.definition.catalog.get(),
      kReportExceptionInteractionClassName,
      kExceptionReportExceptionParameterName);
  if (!reportClassHandle || !federateDimensionHandle ||
      !federateParameterHandle || !serviceParameterHandle || !exceptionParameterHandle) {
    return std::nullopt;
  }

  auto const normalizedFederate = normalizedHandleValue(
      federation.normalizationSeed,
      reportedFederateId,
      kFederateNormalizationKind);
  ExceptionReportRouting result;
  result.interactionClassHandle = *reportClassHandle;
  result.federateParameterHandle = *federateParameterHandle;
  result.serviceParameterHandle = *serviceParameterHandle;
  result.exceptionParameterHandle = *exceptionParameterHandle;
  result.endpointRegionHandle = kMomExceptionReportEndpointRegionHandle;
  result.endpointRegion = {
      {*federateDimensionHandle},
      {{*federateDimensionHandle, {normalizedFederate, normalizedFederate + 1U}}},
      true,
  };
  return result;
}

ExceptionReportPlan EmbeddedFederationRegistry::planExceptionReport(
    std::wstring const& federationName,
    std::uint64_t reportedFederateId) const {
  auto instrumentationScope = beginInstrumentation("planExceptionReport");
  std::scoped_lock lock(mutex_);
  ExceptionReportPlan result;
  result.reportedFederateId = reportedFederateId;
  auto const federation = federations_.find(federationName);
  if (federation == federations_.end()) {
    result.status = ExceptionReportStatus::federation_does_not_exist;
    return result;
  }
  auto const reportedMember = federation->second.members.find(reportedFederateId);
  if (reportedMember == federation->second.members.end()) {
    result.status = ExceptionReportStatus::reported_federate_not_member;
    return result;
  }
  if (!reportedMember->second.exceptionReportingSwitch) {
    return result;
  }
  auto routing = exceptionReportRoutingFor(federation->second, reportedFederateId);
  if (!routing) {
    result.status = ExceptionReportStatus::inconsistent_catalog;
    return result;
  }

  result.status = ExceptionReportStatus::applied;
  result.routing = std::move(*routing);
  std::vector<std::uint64_t> const sentParameterHandles{
      result.routing.federateParameterHandle,
      result.routing.serviceParameterHandle,
      result.routing.exceptionParameterHandle,
  };
  std::map<std::uint64_t, RegionSpecificationSnapshot> const endpointOverride{
      {result.routing.endpointRegionHandle, result.routing.endpointRegion},
  };
  std::set<std::uint64_t> const endpointRegionHandles{
      result.routing.endpointRegionHandle,
  };
  for (auto const& [federateId, membership] : federation->second.members) {
    static_cast<void>(membership);
    auto recipient = candidateReceiveOrderInteractionRecipient(
        federation->second,
        InteractionProducer::rti(),
        federateId,
        result.routing.interactionClassHandle,
        sentParameterHandles,
        &endpointRegionHandles,
        &endpointOverride);
    if (recipient) {
      result.recipients.push_back(std::move(*recipient));
    }
  }
  return result;
}

std::optional<ReceiveOrderInteractionRecipient>
EmbeddedFederationRegistry::exceptionReportRecipientFor(
    std::wstring const& federationName,
    std::uint64_t reportedFederateId,
    std::uint64_t receivingFederateId) const {
  std::scoped_lock lock(mutex_);
  auto const federation = federations_.find(federationName);
  if (federation == federations_.end() ||
      !federation->second.members.contains(reportedFederateId) ||
      !federation->second.members.contains(receivingFederateId)) {
    return std::nullopt;
  }
  auto const reportedMember = federation->second.members.find(reportedFederateId);
  if (reportedMember == federation->second.members.end() ||
      !reportedMember->second.exceptionReportingSwitch) {
    return std::nullopt;
  }
  auto routing = exceptionReportRoutingFor(federation->second, reportedFederateId);
  if (!routing) {
    return std::nullopt;
  }
  std::vector<std::uint64_t> const sentParameterHandles{
      routing->federateParameterHandle,
      routing->serviceParameterHandle,
      routing->exceptionParameterHandle,
  };
  std::map<std::uint64_t, RegionSpecificationSnapshot> const endpointOverride{
      {routing->endpointRegionHandle, routing->endpointRegion},
  };
  std::set<std::uint64_t> const endpointRegionHandles{
      routing->endpointRegionHandle,
  };
  return candidateReceiveOrderInteractionRecipient(
      federation->second,
      InteractionProducer::rti(),
      receivingFederateId,
      routing->interactionClassHandle,
      sentParameterHandles,
      &endpointRegionHandles,
      &endpointOverride);
}










ReceiveOrderDirectedInteractionPlan
EmbeddedFederationRegistry::planReceiveOrderDirectedInteraction(
    std::wstring const& federationName,
    std::uint64_t producingFederateId,
    std::uint64_t objectInstanceHandle,
    std::uint64_t sentInteractionClassHandle,
    std::vector<std::uint64_t> const& sentParameterHandles) const {
  std::scoped_lock lock(mutex_);
  auto const federation = federations_.find(federationName);
  if (federation == federations_.end()) {
    return {ReceiveOrderDirectedInteractionStatus::federation_does_not_exist};
  }
  if (!federation->second.members.contains(producingFederateId)) {
    return {ReceiveOrderDirectedInteractionStatus::producing_federate_not_member};
  }
  auto const instance = federation->second.objectInstances.find(objectInstanceHandle);
  if (instance == federation->second.objectInstances.end() ||
      instance->second.deleteAccepted ||
      !instance->second.knownObjectClassHandlesByFederate.contains(producingFederateId)) {
    return {ReceiveOrderDirectedInteractionStatus::object_instance_not_known};
  }
  if (!validInteractionClass(federation->second, sentInteractionClassHandle)) {
    return {ReceiveOrderDirectedInteractionStatus::interaction_class_not_defined};
  }
  if (!federation->second.definition.catalog ||
      !federation->second.interactionClassHandles ||
      !federation->second.parameterHandles ||
      !federation->second.objectClassHandles) {
    return {ReceiveOrderDirectedInteractionStatus::inconsistent_catalog};
  }
  if (!validDirectedInteractionForObjectClass(
          federation->second,
          instance->second.registeredObjectClassHandle,
          sentInteractionClassHandle) ||
      !directedInteractionDeclarationApplies(
          federation->second,
          producingFederateId,
          instance->second.registeredObjectClassHandle,
          sentInteractionClassHandle,
          true)) {
    return {ReceiveOrderDirectedInteractionStatus::interaction_class_not_published};
  }

  auto const sentClassName = federation->second.interactionClassHandles->nameFor(
      sentInteractionClassHandle);
  auto const* sentClass = sentClassName
      ? federation->second.definition.catalog->interactionClass(*sentClassName)
      : nullptr;
  if (sentClass == nullptr || sentClass->transportation.empty()) {
    return {ReceiveOrderDirectedInteractionStatus::inconsistent_catalog};
  }

  std::set<std::uint64_t> uniqueParameterHandles;
  for (std::uint64_t const parameterHandle : sentParameterHandles) {
    if (!uniqueParameterHandles.insert(parameterHandle).second ||
        !federation->second.parameterHandles->nameFor(
            federation->second.definition.catalog.get(),
            *sentClassName,
            parameterHandle)) {
      return {ReceiveOrderDirectedInteractionStatus::interaction_parameter_not_defined};
    }
  }

  ReceiveOrderDirectedInteractionPlan result;
  auto const effectiveTransportation = effectiveInteractionTransportationName(
      federation->second,
      producingFederateId,
      sentInteractionClassHandle);
  if (!effectiveTransportation || effectiveTransportation->empty()) {
    return {ReceiveOrderDirectedInteractionStatus::inconsistent_catalog};
  }
  result.transportationName = *effectiveTransportation;
  auto const preferredOrderType = interactionOrderType(
      federation->second,
      producingFederateId,
      sentInteractionClassHandle);
  if (!preferredOrderType) {
    return {ReceiveOrderDirectedInteractionStatus::inconsistent_catalog};
  }
  result.preferredOrderType = *preferredOrderType;
  bool const delaySubscriptionEvaluation =
      federation->second.delaySubscriptionEvaluationSwitch;
  for (auto const& [federateId, membership] : federation->second.members) {
    static_cast<void>(membership);
    auto recipient = candidateReceiveOrderDirectedInteractionRecipient(
        federation->second,
        producingFederateId,
        federateId,
        objectInstanceHandle,
        sentInteractionClassHandle,
        sentParameterHandles);
    if (recipient) {
      result.recipients.push_back(std::move(*recipient));
      continue;
    }
    // §8.1.8 applies the federation-wide delayed subscription policy at the
    // callback boundary.  Retain a joined non-source recipient with a live
    // route even when its directed selector is currently absent; the normal
    // receive-order callback rechecks target knowledge, source publication,
    // and the ownership/universal selector immediately before user code.
    // This is intentionally a route-only projection so a later declaration
    // can make the accepted passel deliverable without reviving stale work
    // after an unsubscribe or target departure.
    if (!delaySubscriptionEvaluation || federateId == producingFederateId) {
      continue;
    }
    auto const callbackRoute = federation->second.interactionCallbackRoutes.find(federateId);
    if (callbackRoute == federation->second.interactionCallbackRoutes.end() ||
        !callbackRoute->second) {
      continue;
    }
    ReceiveOrderDirectedInteractionRecipient deferredRecipient;
    deferredRecipient.federateId = federateId;
    deferredRecipient.objectInstanceHandle = objectInstanceHandle;
    deferredRecipient.receivedInteractionClassHandle = sentInteractionClassHandle;
    deferredRecipient.callbackRoute = callbackRoute->second;
    result.recipients.push_back(std::move(deferredRecipient));
  }
  return result;
}

std::optional<ReceiveOrderDirectedInteractionRecipient>
EmbeddedFederationRegistry::receiveOrderDirectedInteractionRecipientFor(
    std::wstring const& federationName,
    std::uint64_t producingFederateId,
    std::uint64_t receivingFederateId,
    std::uint64_t objectInstanceHandle,
    std::uint64_t sentInteractionClassHandle,
    std::vector<std::uint64_t> const& sentParameterHandles) const {
  std::scoped_lock lock(mutex_);
  auto const federation = federations_.find(federationName);
  if (federation == federations_.end()) {
    return std::nullopt;
  }
  return candidateReceiveOrderDirectedInteractionRecipient(
      federation->second,
      producingFederateId,
      receivingFederateId,
      objectInstanceHandle,
      sentInteractionClassHandle,
      sentParameterHandles);
}

std::optional<ReceiveOrderDirectedInteractionRecipient>
EmbeddedFederationRegistry::timestampedDirectedInteractionRecipientFor(
    std::wstring const& federationName,
    std::uint64_t producingFederateId,
    std::uint64_t receivingFederateId,
    std::uint64_t objectInstanceHandle,
    std::uint64_t sentInteractionClassHandle,
    std::vector<std::uint64_t> const& sentParameterHandles,
    std::uint64_t messageId) const {
  std::scoped_lock lock(mutex_);
  auto const federation = federations_.find(federationName);
  if (federation == federations_.end()) {
    return std::nullopt;
  }

  bool retainDepartedProducerAcceptance = false;
  if (!federation->second.members.contains(producingFederateId)) {
    auto const message = federation->second.tsoDirectedInteractionMessages.find(messageId);
    auto const retraction = federation->second.tsoRequestRetractionRecords.find(messageId);
    if (message == federation->second.tsoDirectedInteractionMessages.end() ||
        retraction == federation->second.tsoRequestRetractionRecords.end() ||
        message->second.producingFederateId != producingFederateId ||
        message->second.objectInstanceHandle != objectInstanceHandle ||
        message->second.sentInteractionClassHandle != sentInteractionClassHandle) {
      return std::nullopt;
    }
    auto const recipient = std::find_if(
        message->second.recipients.begin(),
        message->second.recipients.end(),
        [receivingFederateId](TsoDirectedInteractionRecipient const& candidate) {
          return candidate.receivingFederateId == receivingFederateId;
        });
    auto const recipientState = retraction->second.recipientStates.find(receivingFederateId);
    retainDepartedProducerAcceptance =
        recipient != message->second.recipients.end() &&
        recipientState != retraction->second.recipientStates.end() &&
        recipientState->second == Federation::TsoRecipientDeliveryState::pending &&
        (retraction->second.deliveryRequiredAfterConnectionLoss ||
         retraction->second.producerResigned);
  }

  return candidateReceiveOrderDirectedInteractionRecipient(
      federation->second,
      producingFederateId,
      receivingFederateId,
      objectInstanceHandle,
      sentInteractionClassHandle,
      sentParameterHandles,
      retainDepartedProducerAcceptance);
}

}  // namespace umbra::detail
