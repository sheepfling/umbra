#include "internal/federation_registry.hpp"

#include "internal/fom_catalog.hpp"
#include "internal/federation_time_bounds.hpp"
#include "internal/federation_time_grant_policy.hpp"

#include <algorithm>
#include <limits>
#include <set>
#include <string>
#include <type_traits>
#include <tuple>
#include <utility>

namespace umbra::detail {

static_assert(std::is_nothrow_swappable_v<FederationDefinition>);

bool objectInstanceNameIsLegal(std::wstring const& objectInstanceName) {
  // Clause 6.2/6.5 reserves the HLA. namespace for the RTI and rejects an
  // empty designator. Other character/name policy remains the standard
  // binding's responsibility rather than being invented by this kernel.
  return !objectInstanceName.empty() &&
      objectInstanceName.rfind(L"HLA.", 0) != 0;
}

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

FederationRegistryResult EmbeddedFederationRegistry::create(
    std::wstring const& federationName,
    FederationDefinition definition) {
  if (!validDefinition(federationName, definition)) {
    return {FederationRegistryStatus::invalid_request};
  }

  ObjectClassHandleDirectory objectClassHandles;
  if (!objectClassHandles.reconcile(definition.catalog.get())) {
    return {FederationRegistryStatus::invalid_request};
  }
  AttributeHandleDirectory attributeHandles;
  if (!attributeHandles.reconcile(definition.catalog.get())) {
    return {FederationRegistryStatus::invalid_request};
  }
  InteractionClassHandleDirectory interactionClassHandles;
  if (!interactionClassHandles.reconcile(definition.catalog.get())) {
    return {FederationRegistryStatus::invalid_request};
  }
  ParameterHandleDirectory parameterHandles;
  if (!parameterHandles.reconcile(definition.catalog.get())) {
    return {FederationRegistryStatus::invalid_request};
  }
  DimensionHandleDirectory dimensionHandles;
  if (!dimensionHandles.reconcile(definition.catalog.get())) {
    return {FederationRegistryStatus::invalid_request};
  }
  auto const objectClassHandleSnapshot =
      std::make_shared<ObjectClassHandleDirectory const>(std::move(objectClassHandles));
  auto const attributeHandleSnapshot =
      std::make_shared<AttributeHandleDirectory const>(std::move(attributeHandles));
  auto const interactionClassHandleSnapshot =
      std::make_shared<InteractionClassHandleDirectory const>(std::move(interactionClassHandles));
  auto const parameterHandleSnapshot =
      std::make_shared<ParameterHandleDirectory const>(std::move(parameterHandles));
  auto const dimensionHandleSnapshot =
      std::make_shared<DimensionHandleDirectory const>(std::move(dimensionHandles));

  std::scoped_lock lock(mutex_);
  if (federations_.contains(federationName)) {
    return {FederationRegistryStatus::federation_already_exists};
  }

  Federation federationState;
  auto const logicalTimeImplementationName = definition.logicalTimeImplementationName;
  federationState.definition = std::move(definition);
  if (!federationState.timeCoordinator.configureImplementationName(
          logicalTimeImplementationName)) {
    return {FederationRegistryStatus::invalid_request};
  }
  federationState.objectClassHandles = std::move(objectClassHandleSnapshot);
  federationState.attributeHandles = std::move(attributeHandleSnapshot);
  federationState.interactionClassHandles = std::move(interactionClassHandleSnapshot);
  federationState.parameterHandles = std::move(parameterHandleSnapshot);
  federationState.dimensionHandles = std::move(dimensionHandleSnapshot);
  federations_.emplace(federationName, std::move(federationState));
  return {};
}

FederationRegistryResult EmbeddedFederationRegistry::destroy(std::wstring const& federationName) {
  std::scoped_lock lock(mutex_);
  auto federation = federations_.find(federationName);
  if (federation == federations_.end()) {
    return {FederationRegistryStatus::federation_does_not_exist};
  }
  if (!federation->second.members.empty() || !federation->second.timeCoordinator.empty()) {
    return {FederationRegistryStatus::federates_currently_joined};
  }

  federations_.erase(federation);
  return {};
}

FederationJoinResult EmbeddedFederationRegistry::join(
    std::wstring const& federationName,
    std::wstring const& federateType,
    std::optional<std::wstring> requestedFederateName) {
  return joinImpl(
      federationName,
      nullptr,
      nullptr,
      federateType,
      std::move(requestedFederateName),
      {});
}

FederationJoinResult EmbeddedFederationRegistry::joinWithTimeState(
    std::wstring const& federationName,
    std::shared_ptr<FederateTimeState> timeState,
    std::wstring const& federateType,
    std::optional<std::wstring> requestedFederateName,
    InteractionCallbackRoute interactionCallbackRoute) {
  return joinImpl(
      federationName,
      nullptr,
      std::move(timeState),
      federateType,
      std::move(requestedFederateName),
      std::move(interactionCallbackRoute));
}

FederationJoinResult EmbeddedFederationRegistry::joinWithDefinition(
    std::wstring const& federationName,
    FederationDefinition definition,
    std::wstring const& federateType,
    std::optional<std::wstring> requestedFederateName) {
  return joinImpl(
      federationName,
      &definition,
      nullptr,
      federateType,
      std::move(requestedFederateName),
      {});
}

FederationJoinResult EmbeddedFederationRegistry::joinWithDefinitionAndTimeState(
    std::wstring const& federationName,
    FederationDefinition definition,
    std::shared_ptr<FederateTimeState> timeState,
    std::wstring const& federateType,
    std::optional<std::wstring> requestedFederateName,
    InteractionCallbackRoute interactionCallbackRoute) {
  return joinImpl(
      federationName,
      &definition,
      std::move(timeState),
      federateType,
      std::move(requestedFederateName),
      std::move(interactionCallbackRoute));
}

FederationJoinResult EmbeddedFederationRegistry::joinImpl(
    std::wstring const& federationName,
    FederationDefinition* replacementDefinition,
    std::shared_ptr<FederateTimeState> timeState,
    std::wstring const& federateType,
    std::optional<std::wstring> requestedFederateName,
    InteractionCallbackRoute interactionCallbackRoute) {
  std::scoped_lock lock(mutex_);
  auto federation = federations_.find(federationName);
  if (federation == federations_.end()) {
    return {FederationRegistryStatus::federation_does_not_exist, std::nullopt};
  }
  if (replacementDefinition != nullptr && !validDefinition(federationName, *replacementDefinition)) {
    return {FederationRegistryStatus::invalid_request, std::nullopt};
  }
  if (replacementDefinition != nullptr && !federation->second.members.empty() &&
      replacementDefinition->logicalTimeImplementationName !=
          federation->second.definition.logicalTimeImplementationName) {
    // All joined federates must keep one logical-time implementation. The
    // higher-level FOM coordinator normally preserves this invariant; retain
    // it at the commit boundary so a future caller cannot atomically add a
    // member while silently changing existing federates' time representation.
    return {FederationRegistryStatus::invalid_request, std::nullopt};
  }
  if (timeState) {
    std::wstring const& expectedImplementation = replacementDefinition != nullptr
        ? replacementDefinition->logicalTimeImplementationName
        : federation->second.definition.logicalTimeImplementationName;
    if (timeState->implementationName() != expectedImplementation) {
      return {FederationRegistryStatus::invalid_request, std::nullopt};
    }
  }

  std::shared_ptr<ObjectClassHandleDirectory const> replacementObjectClassHandles;
  std::shared_ptr<AttributeHandleDirectory const> replacementAttributeHandles;
  std::shared_ptr<InteractionClassHandleDirectory const> replacementInteractionClassHandles;
  std::shared_ptr<ParameterHandleDirectory const> replacementParameterHandles;
  std::shared_ptr<DimensionHandleDirectory const> replacementDimensionHandles;
  if (replacementDefinition != nullptr) {
    ObjectClassHandleDirectory reconciled = federation->second.objectClassHandles
        ? *federation->second.objectClassHandles
        : ObjectClassHandleDirectory{};
    if (!reconciled.reconcile(replacementDefinition->catalog.get())) {
      return {FederationRegistryStatus::invalid_request, std::nullopt};
    }
    replacementObjectClassHandles =
        std::make_shared<ObjectClassHandleDirectory const>(std::move(reconciled));

    AttributeHandleDirectory reconciledAttributes = federation->second.attributeHandles
        ? *federation->second.attributeHandles
        : AttributeHandleDirectory{};
    if (!reconciledAttributes.reconcile(replacementDefinition->catalog.get())) {
      return {FederationRegistryStatus::invalid_request, std::nullopt};
    }
    replacementAttributeHandles =
        std::make_shared<AttributeHandleDirectory const>(std::move(reconciledAttributes));

    InteractionClassHandleDirectory reconciledInteractions = federation->second.interactionClassHandles
        ? *federation->second.interactionClassHandles
        : InteractionClassHandleDirectory{};
    if (!reconciledInteractions.reconcile(replacementDefinition->catalog.get())) {
      return {FederationRegistryStatus::invalid_request, std::nullopt};
    }
    replacementInteractionClassHandles =
        std::make_shared<InteractionClassHandleDirectory const>(std::move(reconciledInteractions));

    ParameterHandleDirectory reconciledParameters = federation->second.parameterHandles
        ? *federation->second.parameterHandles
        : ParameterHandleDirectory{};
    if (!reconciledParameters.reconcile(replacementDefinition->catalog.get())) {
      return {FederationRegistryStatus::invalid_request, std::nullopt};
    }
    replacementParameterHandles =
        std::make_shared<ParameterHandleDirectory const>(std::move(reconciledParameters));

    DimensionHandleDirectory reconciledDimensions = federation->second.dimensionHandles
        ? *federation->second.dimensionHandles
        : DimensionHandleDirectory{};
    if (!reconciledDimensions.reconcile(replacementDefinition->catalog.get())) {
      return {FederationRegistryStatus::invalid_request, std::nullopt};
    }
    replacementDimensionHandles =
        std::make_shared<DimensionHandleDirectory const>(std::move(reconciledDimensions));
  }

  bool const registersTimeState = static_cast<bool>(timeState);

  if (nextFederateId_ == 0) {
    return {FederationRegistryStatus::invalid_request, std::nullopt};
  }

  std::uint64_t federateId = nextFederateId_;
  std::wstring federateName;
  if (requestedFederateName.has_value()) {
    federateName = *requestedFederateName;
    if (federation->second.memberIdsByName.contains(federateName)) {
      return {FederationRegistryStatus::federate_name_already_in_use, std::nullopt};
    }
  } else {
    // The official no-name overload requires a unique RTI-provided name. An
    // explicit federate may already own a string that looks auto-generated, so
    // advance through otherwise valid handle values until both are unique.
    do {
      if (federateId == std::numeric_limits<std::uint64_t>::max()) {
        return {FederationRegistryStatus::invalid_request, std::nullopt};
      }
      federateName = L"federate-" + std::to_wstring(federateId);
      if (!federation->second.memberIdsByName.contains(federateName)) {
        break;
      }
      ++federateId;
    } while (true);
  }

  if (federateId == std::numeric_limits<std::uint64_t>::max()) {
    return {FederationRegistryStatus::invalid_request, std::nullopt};
  }

  FederateMembership membership{federateId, std::move(federateName), federateType};
  auto [namePosition, insertedName] = federation->second.memberIdsByName.emplace(
      membership.name,
      membership.id);
  if (!insertedName) {
    return {FederationRegistryStatus::federate_name_already_in_use, std::nullopt};
  }

  if (registersTimeState) {
    try {
      auto const registered = federation->second.timeCoordinator.registerFederate(
          membership.id,
          std::move(timeState));
      if (registered.status != FederationTimeCoordinatorStatus::applied) {
        federation->second.memberIdsByName.erase(namePosition);
        return {FederationRegistryStatus::invalid_request, std::nullopt};
      }
    } catch (...) {
      federation->second.memberIdsByName.erase(namePosition);
      throw;
    }
  }

  bool routeRegistered = false;
  if (interactionCallbackRoute) {
    try {
      auto [routePosition, insertedRoute] = federation->second.interactionCallbackRoutes.emplace(
          membership.id,
          std::move(interactionCallbackRoute));
      static_cast<void>(routePosition);
      if (!insertedRoute) {
        if (registersTimeState) {
          static_cast<void>(federation->second.timeCoordinator.unregisterFederate(membership.id));
        }
        federation->second.memberIdsByName.erase(namePosition);
        return {FederationRegistryStatus::invalid_request, std::nullopt};
      }
      routeRegistered = true;
    } catch (...) {
      if (registersTimeState) {
        static_cast<void>(federation->second.timeCoordinator.unregisterFederate(membership.id));
      }
      federation->second.memberIdsByName.erase(namePosition);
      throw;
    }
  }

  try {
    auto [memberPosition, insertedMember] = federation->second.members.emplace(
        membership.id,
        membership);
    static_cast<void>(memberPosition);
    if (!insertedMember) {
      if (routeRegistered) {
        federation->second.interactionCallbackRoutes.erase(membership.id);
      }
      if (registersTimeState) {
        static_cast<void>(federation->second.timeCoordinator.unregisterFederate(membership.id));
      }
      federation->second.memberIdsByName.erase(namePosition);
      return {FederationRegistryStatus::invalid_request, std::nullopt};
    }
  } catch (...) {
    if (routeRegistered) {
      federation->second.interactionCallbackRoutes.erase(membership.id);
    }
    if (registersTimeState) {
      static_cast<void>(federation->second.timeCoordinator.unregisterFederate(membership.id));
    }
    federation->second.memberIdsByName.erase(namePosition);
    throw;
  }

  // All potentially allocating membership writes have succeeded. Swapping a
  // prevalidated definition is non-allocating for these standard value
  // members, so an additional-FOM join never leaves a new member behind with a
  // partially replaced federation definition.
  if (replacementDefinition != nullptr) {
    using std::swap;
    swap(federation->second.definition, *replacementDefinition);
    swap(federation->second.objectClassHandles, replacementObjectClassHandles);
    swap(federation->second.attributeHandles, replacementAttributeHandles);
    swap(federation->second.interactionClassHandles, replacementInteractionClassHandles);
    swap(federation->second.parameterHandles, replacementParameterHandles);
    swap(federation->second.dimensionHandles, replacementDimensionHandles);
  }
  nextFederateId_ = federateId + 1;
  return {FederationRegistryStatus::applied, std::move(membership)};
}

FederationRegistryResult EmbeddedFederationRegistry::resign(
    std::wstring const& federationName,
    std::uint64_t federateId) {
  std::scoped_lock lock(mutex_);
  auto federation = federations_.find(federationName);
  if (federation == federations_.end()) {
    return {FederationRegistryStatus::federation_does_not_exist};
  }

  auto member = federation->second.members.find(federateId);
  if (member == federation->second.members.end()) {
    return {FederationRegistryStatus::federate_not_member};
  }

  static_cast<void>(federation->second.timeCoordinator.unregisterFederate(federateId));
  federation->second.pendingTimeAdvanceGrants.erase(federateId);
  federation->second.interactionDeclarations.erase(federateId);
  federation->second.objectClassAttributeDeclarations.erase(federateId);
  federation->second.interactionCallbackRoutes.erase(federateId);
  for (auto reservation = federation->second.reservedObjectInstanceNamesByFederate.begin();
       reservation != federation->second.reservedObjectInstanceNamesByFederate.end();) {
    if (reservation->second == federateId) {
      reservation = federation->second.reservedObjectInstanceNamesByFederate.erase(reservation);
    } else {
      ++reservation;
    }
  }
  for (auto region = federation->second.regions.begin();
       region != federation->second.regions.end();) {
    if (region->second.ownerFederateId == federateId) {
      region = federation->second.regions.erase(region);
    } else {
      ++region;
    }
  }
  for (auto objectInstance = federation->second.objectInstances.begin();
       objectInstance != federation->second.objectInstances.end();) {
    objectInstance->second.knownObjectClassHandlesByFederate.erase(federateId);
    objectInstance->second.pendingDiscoveryFederates.erase(federateId);
    objectInstance->second.pendingRemovalFederates.erase(federateId);
    objectInstance->second.pendingTimestampedRemovalFederates.erase(federateId);
    if (objectInstance->second.producingFederateId == federateId &&
        objectInstance->second.pendingTimestampedDeletionMessageId.has_value()) {
      federation->second.tsoObjectDeletionMessages.erase(
          *objectInstance->second.pendingTimestampedDeletionMessageId);
      objectInstance->second.pendingTimestampedDeletionMessageId.reset();
      objectInstance->second.pendingTimestampedRemovalFederates.clear();
    }
    if (objectInstance->second.pendingTimestampedRemovalFederates.empty()) {
      objectInstance->second.pendingTimestampedDeletionMessageId.reset();
    }
    for (auto pending =
             objectInstance->second.pendingAttributeOwnershipAcquisitionIfAvailableRequests.begin();
         pending != objectInstance->second.pendingAttributeOwnershipAcquisitionIfAvailableRequests.end();) {
      if (pending->second.requestingFederateId == federateId) {
        pending = objectInstance->second.pendingAttributeOwnershipAcquisitionIfAvailableRequests.erase(
            pending);
      } else {
        ++pending;
      }
    }
    for (auto pending = objectInstance->second.pendingAttributeOwnershipAcquisitionRequests.begin();
         pending != objectInstance->second.pendingAttributeOwnershipAcquisitionRequests.end();) {
      if (pending->second.requestingFederateId == federateId) {
        pending = objectInstance->second.pendingAttributeOwnershipAcquisitionRequests.erase(pending);
      } else {
        ++pending;
      }
    }
    for (auto cancellation =
             objectInstance->second.pendingAttributeOwnershipAcquisitionCancellations.begin();
         cancellation !=
         objectInstance->second.pendingAttributeOwnershipAcquisitionCancellations.end();) {
      if (cancellation->second.requestingFederateId == federateId) {
        cancellation =
            objectInstance->second.pendingAttributeOwnershipAcquisitionCancellations.erase(
                cancellation);
      } else {
        ++cancellation;
      }
    }
    for (auto notification =
             objectInstance->second
                 .pendingAttributeOwnershipDivestitureIfWantedNotifications.begin();
         notification !=
         objectInstance->second.pendingAttributeOwnershipDivestitureIfWantedNotifications.end();) {
      if (notification->second.receivingFederateId == federateId) {
        notification =
            objectInstance->second.pendingAttributeOwnershipDivestitureIfWantedNotifications.erase(
                notification);
      } else {
        ++notification;
      }
    }
    for (auto divestiture =
             objectInstance->second.pendingNegotiatedAttributeOwnershipDivestitures.begin();
         divestiture !=
         objectInstance->second.pendingNegotiatedAttributeOwnershipDivestitures.end();) {
      if (divestiture->second.divestingFederateId == federateId ||
          divestiture->second.acquiringFederateId == federateId) {
        divestiture =
            objectInstance->second.pendingNegotiatedAttributeOwnershipDivestitures.erase(
                divestiture);
      } else {
        ++divestiture;
      }
    }
    for (auto notification =
             objectInstance->second.pendingConfirmDivestitureNotifications.begin();
         notification != objectInstance->second.pendingConfirmDivestitureNotifications.end();) {
      if (notification->second.receivingFederateId == federateId) {
        notification = objectInstance->second.pendingConfirmDivestitureNotifications.erase(notification);
      } else {
        ++notification;
      }
    }
    // Region templates owned by the resigning federate were removed above.
    // Drop only those stale object-attribute associations; associations to
    // regions owned by another remaining federate stay intact.
    for (auto association = objectInstance->second.updateRegionsByAttribute.begin();
         association != objectInstance->second.updateRegionsByAttribute.end();) {
      for (auto region = association->second.begin();
           region != association->second.end();) {
        if (!federation->second.regions.contains(*region)) {
          region = association->second.erase(region);
        } else {
          ++region;
        }
      }
      if (association->second.empty()) {
        association = objectInstance->second.updateRegionsByAttribute.erase(association);
      } else {
        ++association;
      }
    }
    if (canPurgeDeletedObjectInstance(objectInstance->second)) {
      federation->second.objectInstanceHandlesByName.erase(objectInstance->second.name);
      objectInstance = federation->second.objectInstances.erase(objectInstance);
    } else {
      ++objectInstance;
    }
  }
  federation->second.memberIdsByName.erase(member->second.name);
  federation->second.members.erase(member);
  refreshRegionUsage(federation->second);
  return {};
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

FederationTimeGrantDispatchResult EmbeddedFederationRegistry::requestTimeAdvanceGrant(
    std::wstring const& federationName,
    std::uint64_t federateId,
    std::uint64_t generation,
    FederationTimeGrantDispatch dispatch) {
  if (federateId == 0 || generation == 0 || !dispatch) {
    return {FederationTimeGrantStatus::inconsistent_temporal_state, {}};
  }

  std::scoped_lock lock(mutex_);
  auto federation = federations_.find(federationName);
  if (federation == federations_.end()) {
    return {FederationTimeGrantStatus::federation_does_not_exist, {}};
  }
  if (!federation->second.members.contains(federateId)) {
    return {FederationTimeGrantStatus::federate_not_member, {}};
  }
  if (federation->second.pendingTimeAdvanceGrants.contains(federateId)) {
    return {FederationTimeGrantStatus::stale_generation, {}};
  }

  auto [pending, inserted] = federation->second.pendingTimeAdvanceGrants.emplace(
      federateId,
      Federation::PendingTimeAdvanceGrant{generation, std::move(dispatch), false});
  static_cast<void>(pending);
  if (!inserted) {
    return {FederationTimeGrantStatus::stale_generation, {}};
  }

  return {FederationTimeGrantStatus::applied, scheduleEligibleTimeAdvanceGrants(federation->second)};
}

FederationTimeGrantDispatchResult EmbeddedFederationRegistry::reevaluateTimeAdvanceGrants(
    std::wstring const& federationName) {
  std::scoped_lock lock(mutex_);
  auto federation = federations_.find(federationName);
  if (federation == federations_.end()) {
    return {FederationTimeGrantStatus::federation_does_not_exist, {}};
  }
  return {FederationTimeGrantStatus::applied, scheduleEligibleTimeAdvanceGrants(federation->second)};
}

FederationTimeGrantStatus EmbeddedFederationRegistry::beginTimeAdvanceGrant(
    std::wstring const& federationName,
    std::uint64_t federateId,
    std::uint64_t generation) {
  if (federateId == 0 || generation == 0) {
    return FederationTimeGrantStatus::inconsistent_temporal_state;
  }

  std::scoped_lock lock(mutex_);
  auto federation = federations_.find(federationName);
  if (federation == federations_.end()) {
    return FederationTimeGrantStatus::federation_does_not_exist;
  }
  if (!federation->second.members.contains(federateId)) {
    return FederationTimeGrantStatus::federate_not_member;
  }
  auto pending = federation->second.pendingTimeAdvanceGrants.find(federateId);
  if (pending == federation->second.pendingTimeAdvanceGrants.end()) {
    return FederationTimeGrantStatus::time_advance_not_pending;
  }
  if (pending->second.generation != generation) {
    return FederationTimeGrantStatus::stale_generation;
  }
  if (!pending->second.dispatchQueued) {
    return FederationTimeGrantStatus::grant_not_ready;
  }

  auto snapshot = makeTimeSnapshot(federation->second);
  if (!snapshot) {
    pending->second.dispatchQueued = false;
    return FederationTimeGrantStatus::inconsistent_temporal_state;
  }
  auto const requester = std::find_if(
      snapshot->federates.begin(),
      snapshot->federates.end(),
      [federateId](FederationTimeFederateSnapshot const& candidate) {
        return candidate.membership.id == federateId;
      });
  if (requester == snapshot->federates.end()) {
    pending->second.dispatchQueued = false;
    return FederationTimeGrantStatus::federate_not_member;
  }

  FederationTimeBounds const bounds = FederationTimeBoundsCalculator{}.calculate(*snapshot, federateId);
  FederationTimeAdvanceGrantDecision const decision =
      FederationTimeAdvanceGrantPolicy{}.decide(requester->time, bounds);
  if (!decision.mayGrant()) {
    pending->second.dispatchQueued = false;
    switch (decision.status) {
      case FederationTimeAdvanceGrantStatus::wait_for_galt:
      case FederationTimeAdvanceGrantStatus::wait_for_non_regulated_grant:
        return FederationTimeGrantStatus::grant_not_ready;
      case FederationTimeAdvanceGrantStatus::no_time_advance_pending:
        return FederationTimeGrantStatus::time_advance_not_pending;
      case FederationTimeAdvanceGrantStatus::inconsistent_temporal_state:
        return FederationTimeGrantStatus::inconsistent_temporal_state;
      case FederationTimeAdvanceGrantStatus::grant:
        break;
    }
    return FederationTimeGrantStatus::inconsistent_temporal_state;
  }

  federation->second.pendingTimeAdvanceGrants.erase(pending);
  static_cast<void>(federation->second.timeCoordinator.clearDeliveredTsoMessages(federateId));
  return FederationTimeGrantStatus::applied;
}

FederationTsoMessageIdResult EmbeddedFederationRegistry::allocateTsoMessageId(
    std::wstring const& federationName) {
  std::scoped_lock lock(mutex_);
  auto federation = federations_.find(federationName);
  if (federation == federations_.end()) {
    return {FederationTsoRegistryStatus::federation_does_not_exist, 0};
  }
  auto const messageId = federation->second.timeCoordinator.allocateTsoMessageId();
  if (messageId == 0) {
    return {FederationTsoRegistryStatus::invalid_request, 0};
  }
  return {FederationTsoRegistryStatus::applied, messageId};
}

FederationTsoEnqueueResult EmbeddedFederationRegistry::enqueueTsoMessage(
    std::wstring const& federationName,
    std::uint64_t messageId,
    std::uint64_t recipientFederateId,
    std::shared_ptr<rti1516_2025::LogicalTime const> timestamp) {
  if (messageId == 0 || recipientFederateId == 0 || !timestamp) {
    return {FederationTsoRegistryStatus::invalid_request,
            TsoMessageQueueStatus::invalid_message_id};
  }
  std::scoped_lock lock(mutex_);
  auto federation = federations_.find(federationName);
  if (federation == federations_.end()) {
    return {FederationTsoRegistryStatus::federation_does_not_exist,
            TsoMessageQueueStatus::invalid_message_id};
  }
  if (!federation->second.members.contains(recipientFederateId)) {
    return {FederationTsoRegistryStatus::federate_not_member,
            TsoMessageQueueStatus::invalid_recipient};
  }
  auto const result = federation->second.timeCoordinator.enqueueTsoMessage(
      messageId,
      recipientFederateId,
      std::move(timestamp));
  return {FederationTsoRegistryStatus::applied, result.status};
}

FederationTsoInteractionEnqueueResult EmbeddedFederationRegistry::enqueueTsoInteraction(
    std::wstring const& federationName,
    TsoInteractionMessage message,
    std::vector<std::uint64_t> const& recipientFederateIds) {
  if (message.producingFederateId == 0 || message.sentInteractionClassHandle == 0 ||
      !message.timestamp || recipientFederateIds.empty()) {
    return {FederationTsoRegistryStatus::invalid_request,
            TsoMessageQueueStatus::invalid_message_id,
            0,
            0};
  }

  std::scoped_lock lock(mutex_);
  auto federation = federations_.find(federationName);
  if (federation == federations_.end()) {
    return {FederationTsoRegistryStatus::federation_does_not_exist,
            TsoMessageQueueStatus::invalid_message_id,
            0,
            0};
  }
  if (!federation->second.members.contains(message.producingFederateId)) {
    return {FederationTsoRegistryStatus::federate_not_member,
            TsoMessageQueueStatus::invalid_recipient,
            0,
            0};
  }

  auto const messageId = federation->second.timeCoordinator.allocateTsoMessageId();
  if (messageId == 0) {
    return {FederationTsoRegistryStatus::invalid_request,
            TsoMessageQueueStatus::invalid_message_id,
            0,
            0};
  }
  message.messageId = messageId;
  auto const [messagePosition, inserted] =
      federation->second.tsoInteractionMessages.emplace(messageId, std::move(message));
  if (!inserted) {
    return {FederationTsoRegistryStatus::invalid_request,
            TsoMessageQueueStatus::invalid_message_id,
            0,
            0};
  }

  std::size_t enqueuedCount = 0;
  TsoMessageQueueStatus queueStatus = TsoMessageQueueStatus::applied;
  for (std::uint64_t const recipientFederateId : recipientFederateIds) {
    auto const result = federation->second.timeCoordinator.enqueueTsoMessage(
        messageId,
        recipientFederateId,
        messagePosition->second.timestamp);
    if (result.status != TsoMessageQueueStatus::applied) {
      queueStatus = result.status;
      break;
    }
    ++enqueuedCount;
  }
  if (queueStatus != TsoMessageQueueStatus::applied || enqueuedCount == 0) {
    static_cast<void>(federation->second.timeCoordinator.retractTsoMessage(messageId));
    federation->second.tsoInteractionMessages.erase(messageId);
    return {FederationTsoRegistryStatus::invalid_request,
            queueStatus,
            0,
            enqueuedCount};
  }

  return {FederationTsoRegistryStatus::applied,
          TsoMessageQueueStatus::applied,
          messageId,
          enqueuedCount};
}

FederationTsoAttributeUpdateEnqueueResult
EmbeddedFederationRegistry::enqueueTsoAttributeUpdate(
    std::wstring const& federationName,
    TsoAttributeUpdateMessage message,
    std::vector<std::uint64_t> const& recipientFederateIds) {
  if (message.producingFederateId == 0 ||
      message.objectInstanceHandle == 0 ||
      message.attributes.empty() ||
      message.passelsByRecipient.empty() ||
      !message.timestamp ||
      recipientFederateIds.empty()) {
    return {FederationTsoRegistryStatus::invalid_request,
            TsoMessageQueueStatus::invalid_message_id,
            0,
            0};
  }

  std::scoped_lock lock(mutex_);
  auto federation = federations_.find(federationName);
  if (federation == federations_.end()) {
    return {FederationTsoRegistryStatus::federation_does_not_exist,
            TsoMessageQueueStatus::invalid_message_id,
            0,
            0};
  }
  if (!federation->second.members.contains(message.producingFederateId)) {
    return {FederationTsoRegistryStatus::federate_not_member,
            TsoMessageQueueStatus::invalid_recipient,
            0,
            0};
  }
  for (std::uint64_t const recipientFederateId : recipientFederateIds) {
    if (!federation->second.members.contains(recipientFederateId) ||
        !message.passelsByRecipient.contains(recipientFederateId)) {
      return {FederationTsoRegistryStatus::federate_not_member,
              TsoMessageQueueStatus::invalid_recipient,
              0,
              0};
    }
  }

  auto const messageId = federation->second.timeCoordinator.allocateTsoMessageId();
  if (messageId == 0) {
    return {FederationTsoRegistryStatus::invalid_request,
            TsoMessageQueueStatus::invalid_message_id,
            0,
            0};
  }
  message.messageId = messageId;
  auto const [messagePosition, inserted] =
      federation->second.tsoAttributeUpdateMessages.emplace(messageId, std::move(message));
  if (!inserted) {
    return {FederationTsoRegistryStatus::invalid_request,
            TsoMessageQueueStatus::invalid_message_id,
            0,
            0};
  }

  std::size_t enqueuedCount = 0;
  TsoMessageQueueStatus queueStatus = TsoMessageQueueStatus::applied;
  for (std::uint64_t const recipientFederateId : recipientFederateIds) {
    auto const result = federation->second.timeCoordinator.enqueueTsoMessage(
        messageId,
        recipientFederateId,
        messagePosition->second.timestamp);
    if (result.status != TsoMessageQueueStatus::applied) {
      queueStatus = result.status;
      break;
    }
    ++enqueuedCount;
  }
  if (queueStatus != TsoMessageQueueStatus::applied || enqueuedCount == 0) {
    static_cast<void>(federation->second.timeCoordinator.retractTsoMessage(messageId));
    federation->second.tsoAttributeUpdateMessages.erase(messageId);
    return {FederationTsoRegistryStatus::invalid_request,
            queueStatus,
            0,
            enqueuedCount};
  }

  return {FederationTsoRegistryStatus::applied,
          TsoMessageQueueStatus::applied,
          messageId,
          enqueuedCount};
}

ObjectInstanceDeletionPlan EmbeddedFederationRegistry::planTsoObjectInstanceDeletion(
    std::wstring const& federationName,
    std::uint64_t producingFederateId,
    std::uint64_t objectInstanceHandle) const {
  std::scoped_lock lock(mutex_);
  auto const federation = federations_.find(federationName);
  if (federation == federations_.end()) {
    return {ObjectInstanceDeletionStatus::federation_does_not_exist};
  }
  if (!federation->second.members.contains(producingFederateId)) {
    return {ObjectInstanceDeletionStatus::federate_not_member};
  }
  auto const instance = federation->second.objectInstances.find(objectInstanceHandle);
  if (instance == federation->second.objectInstances.end() ||
      instance->second.deleteAccepted ||
      instance->second.pendingTimestampedDeletionMessageId.has_value() ||
      !instance->second.knownObjectClassHandlesByFederate.contains(producingFederateId)) {
    return {ObjectInstanceDeletionStatus::object_instance_not_known};
  }
  if (!federation->second.definition.catalog ||
      !federation->second.objectClassHandles ||
      !federation->second.attributeHandles) {
    return {ObjectInstanceDeletionStatus::inconsistent_catalog};
  }

  auto const objectClassName = federation->second.objectClassHandles->nameFor(
      instance->second.registeredObjectClassHandle);
  if (!objectClassName) {
    return {ObjectInstanceDeletionStatus::inconsistent_catalog};
  }
  auto const privilegeToDelete = federation->second.attributeHandles->handleFor(
      federation->second.definition.catalog.get(),
      *objectClassName,
      "HLAprivilegeToDeleteObject");
  if (!privilegeToDelete) {
    return {ObjectInstanceDeletionStatus::inconsistent_catalog};
  }
  auto const privilegeOwner = instance->second.attributeOwnersByHandle.find(*privilegeToDelete);
  if (privilegeOwner == instance->second.attributeOwnersByHandle.end() ||
      privilegeOwner->second != producingFederateId) {
    return {ObjectInstanceDeletionStatus::delete_privilege_not_held};
  }

  ObjectInstanceDeletionPlan result;
  result.recipients.reserve(instance->second.knownObjectClassHandlesByFederate.size());
  for (auto const& [receivingFederateId, knownObjectClassHandle] :
       instance->second.knownObjectClassHandlesByFederate) {
    static_cast<void>(knownObjectClassHandle);
    if (receivingFederateId == producingFederateId ||
        !federation->second.members.contains(receivingFederateId)) {
      continue;
    }
    auto const callbackRoute = federation->second.interactionCallbackRoutes.find(
        receivingFederateId);
    if (callbackRoute == federation->second.interactionCallbackRoutes.end() ||
        !callbackRoute->second) {
      continue;
    }
    result.recipients.push_back({
        receivingFederateId,
        instance->second.handle,
        callbackRoute->second,
    });
  }
  return result;
}

FederationTsoObjectDeletionEnqueueResult
EmbeddedFederationRegistry::enqueueTsoObjectDeletion(
    std::wstring const& federationName,
    std::uint64_t producingFederateId,
    std::uint64_t objectInstanceHandle,
    TsoObjectDeletionMessage message,
    std::vector<std::uint64_t> const& recipientFederateIds) {
  FederationTsoObjectDeletionEnqueueResult invalid;
  invalid.status = FederationTsoRegistryStatus::invalid_request;
  invalid.queueStatus = TsoMessageQueueStatus::invalid_message_id;
  invalid.deletionStatus = ObjectInstanceDeletionStatus::inconsistent_catalog;
  if (producingFederateId == 0 || objectInstanceHandle == 0 || !message.timestamp) {
    return invalid;
  }

  std::scoped_lock lock(mutex_);
  auto federation = federations_.find(federationName);
  if (federation == federations_.end()) {
    invalid.status = FederationTsoRegistryStatus::federation_does_not_exist;
    invalid.deletionStatus = ObjectInstanceDeletionStatus::federation_does_not_exist;
    return invalid;
  }
  if (!federation->second.members.contains(producingFederateId)) {
    invalid.status = FederationTsoRegistryStatus::federate_not_member;
    invalid.deletionStatus = ObjectInstanceDeletionStatus::federate_not_member;
    return invalid;
  }

  auto instance = federation->second.objectInstances.find(objectInstanceHandle);
  if (instance == federation->second.objectInstances.end() ||
      instance->second.deleteAccepted ||
      instance->second.pendingTimestampedDeletionMessageId.has_value() ||
      !instance->second.knownObjectClassHandlesByFederate.contains(producingFederateId)) {
    invalid.deletionStatus = ObjectInstanceDeletionStatus::object_instance_not_known;
    return invalid;
  }
  if (!federation->second.definition.catalog ||
      !federation->second.objectClassHandles ||
      !federation->second.attributeHandles) {
    invalid.deletionStatus = ObjectInstanceDeletionStatus::inconsistent_catalog;
    return invalid;
  }

  auto const objectClassName = federation->second.objectClassHandles->nameFor(
      instance->second.registeredObjectClassHandle);
  if (!objectClassName) {
    invalid.deletionStatus = ObjectInstanceDeletionStatus::inconsistent_catalog;
    return invalid;
  }
  auto const privilegeToDelete = federation->second.attributeHandles->handleFor(
      federation->second.definition.catalog.get(),
      *objectClassName,
      "HLAprivilegeToDeleteObject");
  if (!privilegeToDelete) {
    invalid.deletionStatus = ObjectInstanceDeletionStatus::inconsistent_catalog;
    return invalid;
  }
  auto const privilegeOwner = instance->second.attributeOwnersByHandle.find(*privilegeToDelete);
  if (privilegeOwner == instance->second.attributeOwnersByHandle.end() ||
      privilegeOwner->second != producingFederateId) {
    invalid.deletionStatus = ObjectInstanceDeletionStatus::delete_privilege_not_held;
    return invalid;
  }

  std::vector<TsoObjectDeletionRecipient> recipients;
  recipients.reserve(instance->second.knownObjectClassHandlesByFederate.size());
  for (auto const& [receivingFederateId, knownObjectClassHandle] :
       instance->second.knownObjectClassHandlesByFederate) {
    static_cast<void>(knownObjectClassHandle);
    if (receivingFederateId == producingFederateId ||
        !federation->second.members.contains(receivingFederateId)) {
      continue;
    }
    auto const callbackRoute = federation->second.interactionCallbackRoutes.find(
        receivingFederateId);
    if (callbackRoute == federation->second.interactionCallbackRoutes.end() ||
        !callbackRoute->second) {
      continue;
    }
    recipients.push_back({
        receivingFederateId,
        objectInstanceHandle,
        callbackRoute->second,
    });
  }

  std::set<std::uint64_t> recipientSet;
  for (std::uint64_t const recipientFederateId : recipientFederateIds) {
    if (!recipientSet.insert(recipientFederateId).second ||
        !federation->second.members.contains(recipientFederateId) ||
        recipientFederateId == producingFederateId ||
        std::none_of(
            recipients.begin(),
            recipients.end(),
            [recipientFederateId](TsoObjectDeletionRecipient const& recipient) {
              return recipient.receivingFederateId == recipientFederateId;
            })) {
      invalid.status = FederationTsoRegistryStatus::invalid_request;
      invalid.queueStatus = TsoMessageQueueStatus::invalid_recipient;
      invalid.deletionStatus = ObjectInstanceDeletionStatus::inconsistent_catalog;
      return invalid;
    }
  }

  auto const messageId = federation->second.timeCoordinator.allocateTsoMessageId();
  if (messageId == 0) {
    invalid.deletionStatus = ObjectInstanceDeletionStatus::inconsistent_catalog;
    return invalid;
  }
  message.messageId = messageId;
  message.producingFederateId = producingFederateId;
  message.objectInstanceHandle = objectInstanceHandle;
  message.recipients = recipients;
  auto const [messagePosition, inserted] =
      federation->second.tsoObjectDeletionMessages.emplace(messageId, std::move(message));
  if (!inserted) {
    invalid.deletionStatus = ObjectInstanceDeletionStatus::inconsistent_catalog;
    return invalid;
  }

  instance->second.pendingTimestampedDeletionMessageId = messageId;
  instance->second.pendingTimestampedRemovalFederates.clear();
  for (auto const& recipient : recipients) {
    instance->second.pendingTimestampedRemovalFederates.insert(
        recipient.receivingFederateId);
  }

  std::size_t enqueuedCount = 0;
  TsoMessageQueueStatus queueStatus = TsoMessageQueueStatus::applied;
  for (std::uint64_t const recipientFederateId : recipientFederateIds) {
    auto const result = federation->second.timeCoordinator.enqueueTsoMessage(
        messageId,
        recipientFederateId,
        messagePosition->second.timestamp);
    if (result.status != TsoMessageQueueStatus::applied) {
      queueStatus = result.status;
      break;
    }
    ++enqueuedCount;
  }
  if (queueStatus != TsoMessageQueueStatus::applied) {
    static_cast<void>(federation->second.timeCoordinator.retractTsoMessage(messageId));
    federation->second.tsoObjectDeletionMessages.erase(messageId);
    instance->second.pendingTimestampedDeletionMessageId.reset();
    instance->second.pendingTimestampedRemovalFederates.clear();
    return {FederationTsoRegistryStatus::invalid_request,
            queueStatus,
            0,
            enqueuedCount,
            std::move(recipients),
            ObjectInstanceDeletionStatus::inconsistent_catalog};
  }

  // No other federate knows this object.  The deletion is accepted without a
  // callback, while a queued or immediate recipient keeps the object alive
  // until its corresponding Remove Object Instance boundary.
  if (recipients.empty()) {
    instance->second.deleteAccepted = true;
    instance->second.knownObjectClassHandlesByFederate.erase(producingFederateId);
    instance->second.pendingTimestampedDeletionMessageId.reset();
    instance->second.pendingTimestampedRemovalFederates.clear();
    if (canPurgeDeletedObjectInstance(instance->second)) {
      federation->second.objectInstanceHandlesByName.erase(instance->second.name);
      federation->second.objectInstances.erase(instance);
    }
  }

  return {FederationTsoRegistryStatus::applied,
          TsoMessageQueueStatus::applied,
          messageId,
          enqueuedCount,
          std::move(recipients),
          ObjectInstanceDeletionStatus::applied};
}

FederationTsoRetractionResult EmbeddedFederationRegistry::retractTsoMessage(
    std::wstring const& federationName,
    std::uint64_t messageId) {
  std::scoped_lock lock(mutex_);
  auto federation = federations_.find(federationName);
  if (federation == federations_.end()) {
    return {FederationTsoRegistryStatus::federation_does_not_exist,
            {TsoMessageQueueStatus::message_not_found, 0}};
  }
  if (messageId == 0) {
    return {FederationTsoRegistryStatus::invalid_request,
            {TsoMessageQueueStatus::invalid_message_id, 0}};
  }
  return {FederationTsoRegistryStatus::applied,
          federation->second.timeCoordinator.retractTsoMessage(messageId)};
}

FederationTsoRetractionResult EmbeddedFederationRegistry::retractTsoInteraction(
    std::wstring const& federationName,
    std::uint64_t producingFederateId,
    std::uint64_t messageId) {
  std::scoped_lock lock(mutex_);
  auto federation = federations_.find(federationName);
  if (federation == federations_.end()) {
    return {FederationTsoRegistryStatus::federation_does_not_exist,
            {TsoMessageQueueStatus::message_not_found, 0}};
  }
  if (!federation->second.members.contains(producingFederateId)) {
    return {FederationTsoRegistryStatus::federate_not_member,
            {TsoMessageQueueStatus::invalid_recipient, 0}};
  }
  auto const message = federation->second.tsoInteractionMessages.find(messageId);
  if (message == federation->second.tsoInteractionMessages.end() ||
      message->second.producingFederateId != producingFederateId) {
    return {FederationTsoRegistryStatus::invalid_request,
            {TsoMessageQueueStatus::message_not_found, 0}};
  }
  return {FederationTsoRegistryStatus::applied,
          federation->second.timeCoordinator.retractTsoMessage(messageId)};
}

FederationTsoRetractionResult EmbeddedFederationRegistry::retractTsoMessageForProducer(
    std::wstring const& federationName,
    std::uint64_t producingFederateId,
    std::uint64_t messageId) {
  std::scoped_lock lock(mutex_);
  auto federation = federations_.find(federationName);
  if (federation == federations_.end()) {
    return {FederationTsoRegistryStatus::federation_does_not_exist,
            {TsoMessageQueueStatus::message_not_found, 0}};
  }
  if (!federation->second.members.contains(producingFederateId)) {
    return {FederationTsoRegistryStatus::federate_not_member,
            {TsoMessageQueueStatus::invalid_recipient, 0}};
  }

  bool ownsMessage = false;
  if (auto const interaction = federation->second.tsoInteractionMessages.find(messageId);
      interaction != federation->second.tsoInteractionMessages.end()) {
    ownsMessage = interaction->second.producingFederateId == producingFederateId;
  }
  if (auto const attributeUpdate =
          federation->second.tsoAttributeUpdateMessages.find(messageId);
      attributeUpdate != federation->second.tsoAttributeUpdateMessages.end()) {
    ownsMessage = attributeUpdate->second.producingFederateId == producingFederateId;
  }
  auto const objectDeletion = federation->second.tsoObjectDeletionMessages.find(messageId);
  if (objectDeletion != federation->second.tsoObjectDeletionMessages.end()) {
    if (objectDeletion->second.producingFederateId != producingFederateId) {
      return {FederationTsoRegistryStatus::invalid_request,
              {TsoMessageQueueStatus::message_not_found, 0}};
    }
    ownsMessage = true;
    auto const instance = federation->second.objectInstances.find(
        objectDeletion->second.objectInstanceHandle);
    if (instance == federation->second.objectInstances.end() ||
        instance->second.pendingTimestampedDeletionMessageId != messageId ||
        instance->second.deleteAccepted) {
      return {FederationTsoRegistryStatus::applied,
              {TsoMessageQueueStatus::message_already_delivered, 0}};
    }
  }
  if (!ownsMessage) {
    return {FederationTsoRegistryStatus::invalid_request,
            {TsoMessageQueueStatus::message_not_found, 0}};
  }
  auto const queueResult = federation->second.timeCoordinator.retractTsoMessage(messageId);
  if (objectDeletion != federation->second.tsoObjectDeletionMessages.end() &&
      queueResult.status == TsoMessageQueueStatus::applied) {
    auto const& message = objectDeletion->second;
    auto instance = federation->second.objectInstances.find(message.objectInstanceHandle);
    if (instance == federation->second.objectInstances.end() ||
        instance->second.pendingTimestampedDeletionMessageId != messageId ||
        instance->second.deleteAccepted) {
      return {FederationTsoRegistryStatus::applied,
              {TsoMessageQueueStatus::message_already_delivered, 0}};
    }
    instance->second.pendingTimestampedDeletionMessageId.reset();
    instance->second.pendingTimestampedRemovalFederates.clear();
    federation->second.tsoObjectDeletionMessages.erase(objectDeletion);
  }
  return {FederationTsoRegistryStatus::applied, queueResult};
}

FederationTsoDeliveryRegistryResult EmbeddedFederationRegistry::beginTsoDelivery(
    std::wstring const& federationName,
    std::uint64_t recipientFederateId,
    rti1516_2025::LogicalTime const& boundary,
    bool inclusive) {
  std::scoped_lock lock(mutex_);
  auto federation = federations_.find(federationName);
  if (federation == federations_.end()) {
    return {FederationTsoRegistryStatus::federation_does_not_exist,
            {FederationTsoDeliveryStatus::no_messages, {}}};
  }
  if (!federation->second.members.contains(recipientFederateId)) {
    return {FederationTsoRegistryStatus::federate_not_member,
            {FederationTsoDeliveryStatus::invalid_recipient, {}}};
  }
  return {FederationTsoRegistryStatus::applied,
          federation->second.timeCoordinator.beginTsoDelivery(
              recipientFederateId,
              boundary,
              inclusive)};
}

FederationTsoInteractionDeliveryRegistryResult
EmbeddedFederationRegistry::beginTsoInteractionDelivery(
    std::wstring const& federationName,
    std::uint64_t recipientFederateId,
    rti1516_2025::LogicalTime const& boundary,
    bool inclusive) {
  std::scoped_lock lock(mutex_);
  auto federation = federations_.find(federationName);
  if (federation == federations_.end()) {
    return {FederationTsoRegistryStatus::federation_does_not_exist,
            FederationTsoDeliveryStatus::no_messages,
            {}};
  }
  if (!federation->second.members.contains(recipientFederateId)) {
    return {FederationTsoRegistryStatus::federate_not_member,
            FederationTsoDeliveryStatus::invalid_recipient,
            {}};
  }

  auto const delivery = federation->second.timeCoordinator.beginTsoDelivery(
      recipientFederateId,
      boundary,
      inclusive);
  if (delivery.status != FederationTsoDeliveryStatus::applied) {
    return {FederationTsoRegistryStatus::applied, delivery.status, {}};
  }

  std::vector<TsoInteractionDelivery> result;
  result.reserve(delivery.messages.size());
  for (auto const& queuedMessage : delivery.messages) {
    auto const message = federation->second.tsoInteractionMessages.find(
        queuedMessage.messageId);
    if (message == federation->second.tsoInteractionMessages.end()) {
      // A missing payload means the private temporal state is inconsistent;
      // leave the queue entry in transit so the caller cannot accidentally
      // report a standards callback with invented data.
      return {FederationTsoRegistryStatus::invalid_request,
              FederationTsoDeliveryStatus::message_not_in_transit,
              {}};
    }
    result.push_back({queuedMessage, message->second});
  }
  return {FederationTsoRegistryStatus::applied,
          FederationTsoDeliveryStatus::applied,
          std::move(result)};
}

FederationTsoPayloadDeliveryRegistryResult
EmbeddedFederationRegistry::beginTsoPayloadDelivery(
    std::wstring const& federationName,
    std::uint64_t recipientFederateId,
    rti1516_2025::LogicalTime const& boundary,
    bool inclusive) {
  std::scoped_lock lock(mutex_);
  auto federation = federations_.find(federationName);
  if (federation == federations_.end()) {
    return {FederationTsoRegistryStatus::federation_does_not_exist,
            FederationTsoDeliveryStatus::no_messages,
            {}};
  }
  if (!federation->second.members.contains(recipientFederateId)) {
    return {FederationTsoRegistryStatus::federate_not_member,
            FederationTsoDeliveryStatus::invalid_recipient,
            {}};
  }

  auto const delivery = federation->second.timeCoordinator.beginTsoDelivery(
      recipientFederateId,
      boundary,
      inclusive);
  if (delivery.status != FederationTsoDeliveryStatus::applied) {
    return {FederationTsoRegistryStatus::applied, delivery.status, {}};
  }

  std::vector<TsoPayloadDelivery> result;
  result.reserve(delivery.messages.size());
  for (auto const& queuedMessage : delivery.messages) {
    if (auto const interaction = federation->second.tsoInteractionMessages.find(
            queuedMessage.messageId);
        interaction != federation->second.tsoInteractionMessages.end()) {
      result.emplace_back(TsoInteractionDelivery{queuedMessage, interaction->second});
      continue;
    }
    if (auto const attributeUpdate =
            federation->second.tsoAttributeUpdateMessages.find(queuedMessage.messageId);
        attributeUpdate != federation->second.tsoAttributeUpdateMessages.end()) {
      result.emplace_back(
          TsoAttributeUpdateDelivery{queuedMessage, attributeUpdate->second});
      continue;
    }
    if (auto const objectDeletion =
            federation->second.tsoObjectDeletionMessages.find(queuedMessage.messageId);
        objectDeletion != federation->second.tsoObjectDeletionMessages.end()) {
      result.emplace_back(
          TsoObjectDeletionDelivery{queuedMessage, objectDeletion->second});
      continue;
    }

    // A missing payload means the private temporal state is inconsistent;
    // leave every queue entry in transit so the caller cannot invent a
    // standards callback or silently lose a retraction boundary.
    return {FederationTsoRegistryStatus::invalid_request,
            FederationTsoDeliveryStatus::message_not_in_transit,
            {}};
  }
  return {FederationTsoRegistryStatus::applied,
          FederationTsoDeliveryStatus::applied,
          std::move(result)};
}

std::optional<RemovedObjectInstanceSnapshot>
EmbeddedFederationRegistry::beginTsoObjectInstanceRemoval(
    std::wstring const& federationName,
    std::uint64_t receivingFederateId,
    std::uint64_t objectInstanceHandle,
    std::uint64_t messageId) {
  std::scoped_lock lock(mutex_);
  auto federation = federations_.find(federationName);
  if (federation == federations_.end()) {
    return std::nullopt;
  }
  auto instance = federation->second.objectInstances.find(objectInstanceHandle);
  if (instance == federation->second.objectInstances.end() ||
      instance->second.pendingTimestampedDeletionMessageId != messageId ||
      !instance->second.pendingTimestampedRemovalFederates.contains(receivingFederateId)) {
    return std::nullopt;
  }

  instance->second.pendingTimestampedRemovalFederates.erase(receivingFederateId);
  auto const known = instance->second.knownObjectClassHandlesByFederate.find(
      receivingFederateId);
  if (!federation->second.members.contains(receivingFederateId) ||
      known == instance->second.knownObjectClassHandlesByFederate.end()) {
    if (instance->second.pendingTimestampedRemovalFederates.empty()) {
      instance->second.pendingTimestampedDeletionMessageId.reset();
    }
    if (canPurgeDeletedObjectInstance(instance->second)) {
      federation->second.objectInstanceHandlesByName.erase(instance->second.name);
      federation->second.objectInstances.erase(instance);
    }
    return std::nullopt;
  }

  // The first accepted removal commits the federation-wide deletion and
  // removes the producing federate's local knowledge without inducing a
  // callback on that federate.  Other known recipients transition one at a
  // time as their timestamped callbacks begin.
  if (!instance->second.deleteAccepted) {
    instance->second.deleteAccepted = true;
    instance->second.pendingDiscoveryFederates.clear();
    instance->second.pendingAttributeOwnershipAcquisitionIfAvailableRequests.clear();
    instance->second.pendingAttributeOwnershipAcquisitionRequests.clear();
    instance->second.pendingAttributeOwnershipAcquisitionCancellations.clear();
    instance->second.pendingAttributeOwnershipDivestitureIfWantedNotifications.clear();
    instance->second.pendingNegotiatedAttributeOwnershipDivestitures.clear();
    instance->second.pendingConfirmDivestitureNotifications.clear();
    instance->second.knownObjectClassHandlesByFederate.erase(
        instance->second.producingFederateId);
  }

  RemovedObjectInstanceSnapshot const result{
      instance->second.handle,
      instance->second.producingFederateId,
  };
  instance->second.knownObjectClassHandlesByFederate.erase(known);
  if (instance->second.pendingTimestampedRemovalFederates.empty()) {
    instance->second.pendingTimestampedDeletionMessageId.reset();
  }
  if (canPurgeDeletedObjectInstance(instance->second)) {
    federation->second.objectInstanceHandlesByName.erase(instance->second.name);
    federation->second.objectInstances.erase(instance);
  }
  return result;
}

bool EmbeddedFederationRegistry::cancelTsoObjectInstanceDeletion(
    std::wstring const& federationName,
    std::uint64_t producingFederateId,
    std::uint64_t objectInstanceHandle,
    std::uint64_t messageId) {
  std::scoped_lock lock(mutex_);
  auto federation = federations_.find(federationName);
  if (federation == federations_.end() ||
      !federation->second.members.contains(producingFederateId)) {
    return false;
  }
  auto instance = federation->second.objectInstances.find(objectInstanceHandle);
  if (instance == federation->second.objectInstances.end() ||
      instance->second.pendingTimestampedDeletionMessageId != messageId ||
      instance->second.deleteAccepted) {
    return false;
  }
  instance->second.pendingTimestampedDeletionMessageId.reset();
  instance->second.pendingTimestampedRemovalFederates.clear();
  return true;
}

FederationTsoDeliveryRegistryResult EmbeddedFederationRegistry::completeTsoDelivery(
    std::wstring const& federationName,
    TsoQueuedMessage const& message) {
  std::scoped_lock lock(mutex_);
  auto federation = federations_.find(federationName);
  if (federation == federations_.end()) {
    return {FederationTsoRegistryStatus::federation_does_not_exist,
            {FederationTsoDeliveryStatus::message_not_in_transit, {}}};
  }
  if (!federation->second.members.contains(message.recipientFederateId)) {
    return {FederationTsoRegistryStatus::federate_not_member,
            {FederationTsoDeliveryStatus::invalid_recipient, {}}};
  }
  return {FederationTsoRegistryStatus::applied,
          {federation->second.timeCoordinator.completeTsoDelivery(message), {}}};
}

std::optional<FederationTimeExecutionSnapshot> EmbeddedFederationRegistry::makeTimeSnapshot(
    Federation const& federation) {
  FederationTimeExecutionSnapshot result{federation.definition, {}};
  auto const timeSnapshots = federation.timeCoordinator.snapshot();
  result.federates.reserve(timeSnapshots.size());
  for (auto const& timeSnapshot : timeSnapshots) {
    auto const member = federation.members.find(timeSnapshot.federateId);
    if (member == federation.members.end()) {
      // Registration and membership are committed together. A mismatch is a
      // private corruption signal, so do not expose a partial input set to a
      // GALT calculation or grant scheduler.
      return std::nullopt;
    }
    auto const tso = federation.timeCoordinator.tsoSnapshotFor(timeSnapshot.federateId);
    result.federates.push_back({
        member->second,
        timeSnapshot.time,
        std::move(tso.queued),
        std::move(tso.inTransit),
        std::move(tso.deliveredSinceLastAdvance),
    });
  }
  return result;
}

std::vector<FederationTimeGrantDispatch> EmbeddedFederationRegistry::scheduleEligibleTimeAdvanceGrants(
    Federation& federation) {
  auto snapshot = makeTimeSnapshot(federation);
  if (!snapshot) {
    return {};
  }

  FederationTimeBoundsCalculator boundsCalculator;
  FederationTimeAdvanceGrantPolicy grantPolicy;
  std::vector<FederationTimeGrantDispatch> result;
  for (auto& [federateId, pending] : federation.pendingTimeAdvanceGrants) {
    if (pending.dispatchQueued) {
      continue;
    }
    auto const requester = std::find_if(
        snapshot->federates.begin(),
        snapshot->federates.end(),
        [federateId](FederationTimeFederateSnapshot const& candidate) {
          return candidate.membership.id == federateId;
        });
    if (requester == snapshot->federates.end()) {
      continue;
    }
    FederationTimeBounds const bounds = boundsCalculator.calculate(*snapshot, federateId);
    if (!grantPolicy.decide(requester->time, bounds).mayGrant()) {
      continue;
    }

    // Copy before marking the record queued. If copying the opaque dispatch
    // throws, the request remains eligible for a later re-evaluation.
    result.push_back(pending.dispatch);
    pending.dispatchQueued = true;
  }
  return result;
}

std::vector<FederationExecutionSummary> EmbeddedFederationRegistry::federationExecutions() const {
  std::scoped_lock lock(mutex_);
  std::vector<FederationExecutionSummary> result;
  result.reserve(federations_.size());
  for (auto const& [name, federation] : federations_) {
    result.push_back({name, federation.definition.logicalTimeImplementationName});
  }
  return result;
}

std::optional<std::vector<FederateMembership>> EmbeddedFederationRegistry::membersFor(
    std::wstring const& federationName) const {
  std::scoped_lock lock(mutex_);
  auto const federation = federations_.find(federationName);
  if (federation == federations_.end()) {
    return std::nullopt;
  }

  std::vector<FederateMembership> result;
  result.reserve(federation->second.members.size());
  for (auto const& [id, membership] : federation->second.members) {
    static_cast<void>(id);
    result.push_back(membership);
  }
  return result;
}

std::optional<FederateMembership> EmbeddedFederationRegistry::memberByName(
    std::wstring const& federationName,
    std::wstring const& federateName) const {
  std::scoped_lock lock(mutex_);
  auto const federation = federations_.find(federationName);
  if (federation == federations_.end()) {
    return std::nullopt;
  }

  auto const memberId = federation->second.memberIdsByName.find(federateName);
  if (memberId == federation->second.memberIdsByName.end()) {
    return std::nullopt;
  }
  auto const member = federation->second.members.find(memberId->second);
  if (member == federation->second.members.end()) {
    // The two indexes are committed together. Do not manufacture a lookup
    // result if a future backend breaks that invariant.
    return std::nullopt;
  }
  return member->second;
}

std::optional<FederateMembership> EmbeddedFederationRegistry::memberById(
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
  return member->second;
}

std::optional<std::uint64_t> EmbeddedFederationRegistry::objectClassHandleFor(
    std::wstring const& federationName,
    std::string const& objectClassName) const {
  std::scoped_lock lock(mutex_);
  auto const federation = federations_.find(federationName);
  if (federation == federations_.end() || !federation->second.definition.catalog ||
      !federation->second.objectClassHandles ||
      federation->second.definition.catalog->objectClass(objectClassName) == nullptr) {
    return std::nullopt;
  }
  return federation->second.objectClassHandles->handleFor(objectClassName);
}

std::optional<std::string> EmbeddedFederationRegistry::objectClassNameFor(
    std::wstring const& federationName,
    std::uint64_t objectClassHandle) const {
  std::scoped_lock lock(mutex_);
  auto const federation = federations_.find(federationName);
  if (federation == federations_.end() || !federation->second.definition.catalog ||
      !federation->second.objectClassHandles) {
    return std::nullopt;
  }
  auto name = federation->second.objectClassHandles->nameFor(objectClassHandle);
  if (!name || federation->second.definition.catalog->objectClass(*name) == nullptr) {
    return std::nullopt;
  }
  return name;
}

std::optional<std::uint64_t> EmbeddedFederationRegistry::attributeHandleFor(
    std::wstring const& federationName,
    std::string const& objectClassName,
    std::string const& attributeName) const {
  std::scoped_lock lock(mutex_);
  auto const federation = federations_.find(federationName);
  if (federation == federations_.end() || !federation->second.definition.catalog ||
      !federation->second.attributeHandles ||
      federation->second.definition.catalog->objectClass(objectClassName) == nullptr) {
    return std::nullopt;
  }
  return federation->second.attributeHandles->handleFor(
      federation->second.definition.catalog.get(),
      objectClassName,
      attributeName);
}

std::optional<std::string> EmbeddedFederationRegistry::attributeNameFor(
    std::wstring const& federationName,
    std::string const& objectClassName,
    std::uint64_t attributeHandle) const {
  std::scoped_lock lock(mutex_);
  auto const federation = federations_.find(federationName);
  if (federation == federations_.end() || !federation->second.definition.catalog ||
      !federation->second.attributeHandles ||
      federation->second.definition.catalog->objectClass(objectClassName) == nullptr) {
    return std::nullopt;
  }
  return federation->second.attributeHandles->nameFor(
      federation->second.definition.catalog.get(),
      objectClassName,
      attributeHandle);
}

std::optional<std::uint64_t> EmbeddedFederationRegistry::interactionClassHandleFor(
    std::wstring const& federationName,
    std::string const& interactionClassName) const {
  std::scoped_lock lock(mutex_);
  auto const federation = federations_.find(federationName);
  if (federation == federations_.end() || !federation->second.definition.catalog ||
      !federation->second.interactionClassHandles ||
      federation->second.definition.catalog->interactionClass(interactionClassName) == nullptr) {
    return std::nullopt;
  }
  return federation->second.interactionClassHandles->handleFor(interactionClassName);
}

std::optional<std::string> EmbeddedFederationRegistry::interactionClassNameFor(
    std::wstring const& federationName,
    std::uint64_t interactionClassHandle) const {
  std::scoped_lock lock(mutex_);
  auto const federation = federations_.find(federationName);
  if (federation == federations_.end() || !federation->second.definition.catalog ||
      !federation->second.interactionClassHandles) {
    return std::nullopt;
  }
  auto name = federation->second.interactionClassHandles->nameFor(interactionClassHandle);
  if (!name || federation->second.definition.catalog->interactionClass(*name) == nullptr) {
    return std::nullopt;
  }
  return name;
}

std::optional<std::uint64_t> EmbeddedFederationRegistry::parameterHandleFor(
    std::wstring const& federationName,
    std::string const& interactionClassName,
    std::string const& parameterName) const {
  std::scoped_lock lock(mutex_);
  auto const federation = federations_.find(federationName);
  if (federation == federations_.end() || !federation->second.definition.catalog ||
      !federation->second.parameterHandles ||
      federation->second.definition.catalog->interactionClass(interactionClassName) == nullptr) {
    return std::nullopt;
  }
  return federation->second.parameterHandles->handleFor(
      federation->second.definition.catalog.get(),
      interactionClassName,
      parameterName);
}

std::optional<std::string> EmbeddedFederationRegistry::parameterNameFor(
    std::wstring const& federationName,
    std::string const& interactionClassName,
    std::uint64_t parameterHandle) const {
  std::scoped_lock lock(mutex_);
  auto const federation = federations_.find(federationName);
  if (federation == federations_.end() || !federation->second.definition.catalog ||
      !federation->second.parameterHandles ||
      federation->second.definition.catalog->interactionClass(interactionClassName) == nullptr) {
    return std::nullopt;
  }
  return federation->second.parameterHandles->nameFor(
      federation->second.definition.catalog.get(),
      interactionClassName,
      parameterHandle);
}

std::optional<std::uint64_t> EmbeddedFederationRegistry::dimensionHandleFor(
    std::wstring const& federationName,
    std::string const& dimensionName) const {
  std::scoped_lock lock(mutex_);
  auto const federation = federations_.find(federationName);
  if (federation == federations_.end() || !federation->second.definition.catalog ||
      !federation->second.dimensionHandles ||
      federation->second.definition.catalog->dimension(dimensionName) == nullptr) {
    return std::nullopt;
  }
  return federation->second.dimensionHandles->handleFor(dimensionName);
}

std::optional<std::string> EmbeddedFederationRegistry::dimensionNameFor(
    std::wstring const& federationName,
    std::uint64_t dimensionHandle) const {
  std::scoped_lock lock(mutex_);
  auto const federation = federations_.find(federationName);
  if (federation == federations_.end() || !federation->second.definition.catalog ||
      !federation->second.dimensionHandles) {
    return std::nullopt;
  }
  auto const name = federation->second.dimensionHandles->nameFor(dimensionHandle);
  if (!name || federation->second.definition.catalog->dimension(*name) == nullptr) {
    return std::nullopt;
  }
  return name;
}

std::optional<unsigned long> EmbeddedFederationRegistry::dimensionUpperBoundFor(
    std::wstring const& federationName,
    std::uint64_t dimensionHandle) const {
  std::scoped_lock lock(mutex_);
  auto const federation = federations_.find(federationName);
  if (federation == federations_.end() || !federation->second.definition.catalog ||
      !federation->second.dimensionHandles) {
    return std::nullopt;
  }
  auto const name = federation->second.dimensionHandles->nameFor(dimensionHandle);
  if (!name) {
    return std::nullopt;
  }
  auto const* dimension = federation->second.definition.catalog->dimension(*name);
  return dimension == nullptr ? std::nullopt : std::optional{dimension->upperBound};
}

std::optional<std::set<std::uint64_t>>
EmbeddedFederationRegistry::availableDimensionsForObjectClass(
    std::wstring const& federationName,
    std::uint64_t objectClassHandle) const {
  std::scoped_lock lock(mutex_);
  auto const federation = federations_.find(federationName);
  if (federation == federations_.end() || !federation->second.definition.catalog ||
      !federation->second.objectClassHandles || !federation->second.dimensionHandles) {
    return std::nullopt;
  }
  auto const className = federation->second.objectClassHandles->nameFor(objectClassHandle);
  if (!className) {
    return std::nullopt;
  }

  std::set<std::string> dimensionNames;
  std::set<std::string> visited;
  std::string currentClassName = *className;
  while (!currentClassName.empty() && visited.insert(currentClassName).second) {
    auto const* objectClass = federation->second.definition.catalog->objectClass(currentClassName);
    if (objectClass == nullptr) {
      return std::nullopt;
    }
    dimensionNames.insert(objectClass->dimensions.begin(), objectClass->dimensions.end());
    currentClassName = objectClass->parentName;
  }

  std::set<std::uint64_t> handles;
  for (std::string const& dimensionName : dimensionNames) {
    if (federation->second.definition.catalog->dimension(dimensionName) == nullptr) {
      return std::nullopt;
    }
    auto const handle = federation->second.dimensionHandles->handleFor(dimensionName);
    if (!handle) {
      return std::nullopt;
    }
    handles.insert(*handle);
  }
  return handles;
}

std::optional<std::set<std::uint64_t>>
EmbeddedFederationRegistry::availableDimensionsForInteractionClass(
    std::wstring const& federationName,
    std::uint64_t interactionClassHandle) const {
  std::scoped_lock lock(mutex_);
  auto const federation = federations_.find(federationName);
  if (federation == federations_.end() || !federation->second.definition.catalog ||
      !federation->second.interactionClassHandles || !federation->second.dimensionHandles) {
    return std::nullopt;
  }
  auto const className = federation->second.interactionClassHandles->nameFor(interactionClassHandle);
  if (!className) {
    return std::nullopt;
  }

  std::set<std::string> dimensionNames;
  std::set<std::string> visited;
  std::string currentClassName = *className;
  while (!currentClassName.empty() && visited.insert(currentClassName).second) {
    auto const* interactionClass =
        federation->second.definition.catalog->interactionClass(currentClassName);
    if (interactionClass == nullptr) {
      return std::nullopt;
    }
    dimensionNames.insert(interactionClass->dimensions.begin(), interactionClass->dimensions.end());
    currentClassName = interactionClass->parentName;
  }

  std::set<std::uint64_t> handles;
  for (std::string const& dimensionName : dimensionNames) {
    if (federation->second.definition.catalog->dimension(dimensionName) == nullptr) {
      return std::nullopt;
    }
    auto const handle = federation->second.dimensionHandles->handleFor(dimensionName);
    if (!handle) {
      return std::nullopt;
    }
    handles.insert(*handle);
  }
  return handles;
}

RegionCreateResult EmbeddedFederationRegistry::createRegion(
    std::wstring const& federationName,
    std::uint64_t federateId,
    std::set<std::uint64_t> const& dimensionHandles) {
  std::scoped_lock lock(mutex_);
  auto federation = federations_.find(federationName);
  if (federation == federations_.end()) {
    return {RegionServiceStatus::federation_does_not_exist};
  }
  if (!federation->second.members.contains(federateId)) {
    return {RegionServiceStatus::federate_not_member};
  }
  if (!federation->second.definition.catalog || !federation->second.dimensionHandles) {
    return {RegionServiceStatus::inconsistent_catalog};
  }

  for (std::uint64_t const dimensionHandle : dimensionHandles) {
    auto const dimensionName = federation->second.dimensionHandles->nameFor(dimensionHandle);
    if (!dimensionName ||
        federation->second.definition.catalog->dimension(*dimensionName) == nullptr) {
      return {RegionServiceStatus::invalid_dimension};
    }
  }

  if (federation->second.nextRegionHandle == 0) {
    return {RegionServiceStatus::invalid_region};
  }
  std::uint64_t const regionHandle = federation->second.nextRegionHandle++;
  federation->second.regions.emplace(
      regionHandle,
      Federation::Region{federateId, dimensionHandles, {}, {}, false, false});
  return {RegionServiceStatus::applied, regionHandle};
}

RegionScopeChangePlan EmbeddedFederationRegistry::commitRegionModificationsWithScopeChanges(
    std::wstring const& federationName,
    std::uint64_t federateId,
    std::set<std::uint64_t> const& regionHandles) {
  std::scoped_lock lock(mutex_);
  auto federation = federations_.find(federationName);
  if (federation == federations_.end()) {
    return {RegionServiceStatus::federation_does_not_exist};
  }
  if (!federation->second.members.contains(federateId)) {
    return {RegionServiceStatus::federate_not_member};
  }
  if (!federation->second.definition.catalog || !federation->second.dimensionHandles) {
    return {RegionServiceStatus::inconsistent_catalog};
  }

  for (std::uint64_t const regionHandle : regionHandles) {
    auto const region = federation->second.regions.find(regionHandle);
    if (region == federation->second.regions.end()) {
      return {RegionServiceStatus::invalid_region};
    }
    if (region->second.ownerFederateId != federateId) {
      return {RegionServiceStatus::region_not_created_by_this_federate};
    }
    if (region->second.pendingRangeBounds.size() != region->second.dimensionHandles.size()) {
      return {RegionServiceStatus::incomplete_region};
    }
    for (std::uint64_t const dimensionHandle : region->second.dimensionHandles) {
      auto const pending = region->second.pendingRangeBounds.find(dimensionHandle);
      auto const dimensionName = federation->second.dimensionHandles->nameFor(dimensionHandle);
      auto const* dimension = dimensionName
          ? federation->second.definition.catalog->dimension(*dimensionName)
          : nullptr;
      if (pending == region->second.pendingRangeBounds.end() || dimension == nullptr ||
          pending->second.lowerBound >= pending->second.upperBound ||
          pending->second.upperBound > dimension->upperBound) {
        return {RegionServiceStatus::invalid_range_bound};
      }
    }
  }

  std::map<std::uint64_t, RegionSpecificationSnapshot> previousRegions;
  for (std::uint64_t const regionHandle : regionHandles) {
    auto const& region = federation->second.regions.at(regionHandle);
    previousRegions.emplace(
        regionHandle,
        RegionSpecificationSnapshot{
            region.dimensionHandles,
            region.committedRangeBounds,
            region.specificationCommitted,
        });
  }

  for (std::uint64_t const regionHandle : regionHandles) {
    auto& region = federation->second.regions.at(regionHandle);
    region.committedRangeBounds = region.pendingRangeBounds;
    region.specificationCommitted = true;
  }

  RegionScopeChangePlan result;
  result.status = RegionServiceStatus::applied;
  std::map<std::tuple<std::uint64_t, std::uint64_t, bool>, std::size_t> recipientPositions;
  for (auto const& [objectInstanceHandle, objectInstance] : federation->second.objectInstances) {
    if (objectInstance.deleteAccepted) {
      continue;
    }
    for (auto const& [receivingFederateId, knownClassHandle] :
         objectInstance.knownObjectClassHandlesByFederate) {
      static_cast<void>(knownClassHandle);
      if (receivingFederateId == objectInstance.producingFederateId ||
          !federation->second.members.contains(receivingFederateId)) {
        continue;
      }
      auto const callbackRoute =
          federation->second.interactionCallbackRoutes.find(receivingFederateId);
      if (callbackRoute == federation->second.interactionCallbackRoutes.end() ||
          !callbackRoute->second) {
        continue;
      }
      for (auto const& [attributeHandle, owner] : objectInstance.attributeOwnersByHandle) {
        static_cast<void>(owner);
        bool const wasInScope = objectAttributeInScope(
            federation->second,
            objectInstance,
            receivingFederateId,
            attributeHandle,
            &previousRegions);
        bool const isInScope = objectAttributeInScope(
            federation->second,
            objectInstance,
            receivingFederateId,
            attributeHandle);
        if (wasInScope == isInScope) {
          continue;
        }
        auto const key = std::make_tuple(
            receivingFederateId,
            objectInstanceHandle,
            isInScope);
        auto position = recipientPositions.find(key);
        if (position == recipientPositions.end()) {
          std::size_t const index = result.recipients.size();
          result.recipients.push_back({
              receivingFederateId,
              objectInstanceHandle,
              {},
              isInScope,
              callbackRoute->second,
          });
          recipientPositions.emplace(key, index);
          position = recipientPositions.find(key);
        }
        result.recipients[position->second].attributeHandles.insert(attributeHandle);
      }
    }
  }
  return result;
}

RegionServiceStatus EmbeddedFederationRegistry::commitRegionModifications(
    std::wstring const& federationName,
    std::uint64_t federateId,
    std::set<std::uint64_t> const& regionHandles) {
  return commitRegionModificationsWithScopeChanges(
             federationName,
             federateId,
             regionHandles)
      .status;
}

RegionServiceStatus EmbeddedFederationRegistry::deleteRegion(
    std::wstring const& federationName,
    std::uint64_t federateId,
    std::uint64_t regionHandle) {
  std::scoped_lock lock(mutex_);
  auto federation = federations_.find(federationName);
  if (federation == federations_.end()) {
    return RegionServiceStatus::federation_does_not_exist;
  }
  if (!federation->second.members.contains(federateId)) {
    return RegionServiceStatus::federate_not_member;
  }
  auto region = federation->second.regions.find(regionHandle);
  if (region == federation->second.regions.end()) {
    return RegionServiceStatus::invalid_region;
  }
  if (region->second.ownerFederateId != federateId) {
    return RegionServiceStatus::region_not_created_by_this_federate;
  }
  if (region->second.inUse) {
    return RegionServiceStatus::region_in_use;
  }
  federation->second.regions.erase(region);
  return RegionServiceStatus::applied;
}

RegionDimensionSetResult EmbeddedFederationRegistry::dimensionHandleSetForRegion(
    std::wstring const& federationName,
    std::uint64_t federateId,
    std::uint64_t regionHandle) const {
  std::scoped_lock lock(mutex_);
  auto const federation = federations_.find(federationName);
  if (federation == federations_.end()) {
    return {RegionServiceStatus::federation_does_not_exist};
  }
  if (!federation->second.members.contains(federateId)) {
    return {RegionServiceStatus::federate_not_member};
  }
  auto const region = federation->second.regions.find(regionHandle);
  if (region == federation->second.regions.end() ||
      region->second.ownerFederateId != federateId) {
    return {RegionServiceStatus::invalid_region};
  }
  return {RegionServiceStatus::applied, region->second.dimensionHandles};
}

RegionRangeBoundsResult EmbeddedFederationRegistry::rangeBoundsForRegion(
    std::wstring const& federationName,
    std::uint64_t federateId,
    std::uint64_t regionHandle,
    std::uint64_t dimensionHandle) const {
  std::scoped_lock lock(mutex_);
  auto const federation = federations_.find(federationName);
  if (federation == federations_.end()) {
    return {RegionServiceStatus::federation_does_not_exist};
  }
  if (!federation->second.members.contains(federateId)) {
    return {RegionServiceStatus::federate_not_member};
  }
  auto const region = federation->second.regions.find(regionHandle);
  if (region == federation->second.regions.end() ||
      region->second.ownerFederateId != federateId) {
    return {RegionServiceStatus::invalid_region};
  }
  if (!region->second.dimensionHandles.contains(dimensionHandle)) {
    return {RegionServiceStatus::dimension_not_in_region};
  }

  // A pending value is the value most recently supplied by Set Range Bounds;
  // before a first set, a committed value is the current specification value.
  // A never-initialized dimension has no range and therefore is not a usable
  // region until the caller supplies it and commits the template.
  auto pending = region->second.pendingRangeBounds.find(dimensionHandle);
  if (pending != region->second.pendingRangeBounds.end()) {
    return {RegionServiceStatus::applied, pending->second};
  }
  auto committed = region->second.committedRangeBounds.find(dimensionHandle);
  if (committed != region->second.committedRangeBounds.end()) {
    return {RegionServiceStatus::applied, committed->second};
  }
  return {RegionServiceStatus::invalid_region};
}

RegionServiceStatus EmbeddedFederationRegistry::setRangeBounds(
    std::wstring const& federationName,
    std::uint64_t federateId,
    std::uint64_t regionHandle,
    std::uint64_t dimensionHandle,
    RegionRangeBounds range) {
  std::scoped_lock lock(mutex_);
  auto federation = federations_.find(federationName);
  if (federation == federations_.end()) {
    return RegionServiceStatus::federation_does_not_exist;
  }
  if (!federation->second.members.contains(federateId)) {
    return RegionServiceStatus::federate_not_member;
  }
  if (!federation->second.definition.catalog || !federation->second.dimensionHandles) {
    return RegionServiceStatus::inconsistent_catalog;
  }
  auto region = federation->second.regions.find(regionHandle);
  if (region == federation->second.regions.end()) {
    return RegionServiceStatus::invalid_region;
  }
  if (region->second.ownerFederateId != federateId) {
    return RegionServiceStatus::region_not_created_by_this_federate;
  }
  if (!region->second.dimensionHandles.contains(dimensionHandle)) {
    return RegionServiceStatus::dimension_not_in_region;
  }
  auto const dimensionName = federation->second.dimensionHandles->nameFor(dimensionHandle);
  auto const* dimension = dimensionName
      ? federation->second.definition.catalog->dimension(*dimensionName)
      : nullptr;
  if (dimension == nullptr) {
    return RegionServiceStatus::inconsistent_catalog;
  }
  if (range.lowerBound >= range.upperBound || range.upperBound > dimension->upperBound) {
    return RegionServiceStatus::invalid_range_bound;
  }
  region->second.pendingRangeBounds[dimensionHandle] = range;
  return RegionServiceStatus::applied;
}

bool EmbeddedFederationRegistry::validInteractionClass(
    Federation const& federation,
    std::uint64_t interactionClassHandle) {
  if (!federation.definition.catalog || !federation.interactionClassHandles) {
    return false;
  }
  auto const name = federation.interactionClassHandles->nameFor(interactionClassHandle);
  return name && federation.definition.catalog->interactionClass(*name) != nullptr;
}

std::optional<std::set<std::uint64_t>>
EmbeddedFederationRegistry::availableInteractionDimensions(
    Federation const& federation,
    std::uint64_t interactionClassHandle) {
  if (!federation.definition.catalog ||
      !federation.interactionClassHandles ||
      !federation.dimensionHandles) {
    return std::nullopt;
  }
  auto const className = federation.interactionClassHandles->nameFor(interactionClassHandle);
  if (!className) {
    return std::nullopt;
  }

  std::set<std::string> dimensionNames;
  std::set<std::string> visited;
  std::string currentClassName = *className;
  while (!currentClassName.empty() && visited.insert(currentClassName).second) {
    auto const* interactionClass =
        federation.definition.catalog->interactionClass(currentClassName);
    if (interactionClass == nullptr) {
      return std::nullopt;
    }
    dimensionNames.insert(
        interactionClass->dimensions.begin(), interactionClass->dimensions.end());
    currentClassName = interactionClass->parentName;
  }

  std::set<std::uint64_t> handles;
  for (std::string const& dimensionName : dimensionNames) {
    if (federation.definition.catalog->dimension(dimensionName) == nullptr) {
      return std::nullopt;
    }
    auto const handle = federation.dimensionHandles->handleFor(dimensionName);
    if (!handle) {
      return std::nullopt;
    }
    handles.insert(*handle);
  }
  return handles;
}

std::optional<std::set<std::uint64_t>>
EmbeddedFederationRegistry::availableObjectClassDimensions(
    Federation const& federation,
    std::uint64_t objectClassHandle) {
  if (!federation.definition.catalog ||
      !federation.objectClassHandles ||
      !federation.dimensionHandles) {
    return std::nullopt;
  }
  auto const className = federation.objectClassHandles->nameFor(objectClassHandle);
  if (!className) {
    return std::nullopt;
  }

  std::set<std::string> dimensionNames;
  std::set<std::string> visited;
  std::string currentClassName = *className;
  while (!currentClassName.empty() && visited.insert(currentClassName).second) {
    auto const* objectClass = federation.definition.catalog->objectClass(currentClassName);
    if (objectClass == nullptr) {
      return std::nullopt;
    }
    dimensionNames.insert(objectClass->dimensions.begin(), objectClass->dimensions.end());
    currentClassName = objectClass->parentName;
  }

  std::set<std::uint64_t> handles;
  for (std::string const& dimensionName : dimensionNames) {
    if (federation.definition.catalog->dimension(dimensionName) == nullptr) {
      return std::nullopt;
    }
    auto const handle = federation.dimensionHandles->handleFor(dimensionName);
    if (!handle) {
      return std::nullopt;
    }
    handles.insert(*handle);
  }
  return handles;
}

bool EmbeddedFederationRegistry::regionsOverlap(
    Federation const& federation,
    std::uint64_t firstRegionHandle,
    std::uint64_t secondRegionHandle) {
  return regionsOverlap(federation, firstRegionHandle, secondRegionHandle, nullptr);
}

bool EmbeddedFederationRegistry::regionsOverlap(
    Federation const& federation,
    std::uint64_t firstRegionHandle,
    std::uint64_t secondRegionHandle,
    std::map<std::uint64_t, RegionSpecificationSnapshot> const* overrides) {
  auto snapshotFor = [&](std::uint64_t regionHandle)
      -> std::optional<RegionSpecificationSnapshot> {
    if (overrides != nullptr) {
      auto const overrideRegion = overrides->find(regionHandle);
      if (overrideRegion != overrides->end()) {
        return overrideRegion->second;
      }
    }
    auto const region = federation.regions.find(regionHandle);
    if (region == federation.regions.end()) {
      return std::nullopt;
    }
    return RegionSpecificationSnapshot{
        region->second.dimensionHandles,
        region->second.committedRangeBounds,
        region->second.specificationCommitted,
    };
  };

  auto const first = snapshotFor(firstRegionHandle);
  auto const second = snapshotFor(secondRegionHandle);
  if (!first || !second ||
      !first->specificationCommitted || !second->specificationCommitted) {
    return false;
  }

  bool sharedDimension = false;
  for (auto const& [dimensionHandle, firstRange] : first->committedRangeBounds) {
    auto const secondRange = second->committedRangeBounds.find(dimensionHandle);
    if (secondRange == second->committedRangeBounds.end()) {
      continue;
    }
    sharedDimension = true;
    // Ranges are half-open [lower, upper).  The 2025 overlap definition also
    // treats equal lower bounds as overlapping, even when the upper bound of
    // one range is equal to the lower bound of the other.
    if (firstRange.lowerBound != secondRange->second.lowerBound &&
        (firstRange.lowerBound >= secondRange->second.upperBound ||
         secondRange->second.lowerBound >= firstRange.upperBound)) {
      return false;
    }
  }
  return sharedDimension;
}

void EmbeddedFederationRegistry::refreshRegionUsage(Federation& federation) {
  for (auto& [regionHandle, region] : federation.regions) {
    static_cast<void>(regionHandle);
    region.inUse = false;
  }
  for (auto const& [federateId, declarations] : federation.interactionDeclarations) {
    static_cast<void>(federateId);
    for (auto const& [interactionClassHandle, regions] :
         declarations.regionalSubscribedInteractionClasses) {
      static_cast<void>(interactionClassHandle);
      for (auto const& [regionHandle, active] : regions) {
        static_cast<void>(active);
        auto const region = federation.regions.find(regionHandle);
        if (region != federation.regions.end()) {
          region->second.inUse = true;
        }
      }
    }
  }
  for (auto const& [federateId, declarations] : federation.objectClassAttributeDeclarations) {
    static_cast<void>(federateId);
    for (auto const& [objectClassHandle, perClass] : declarations.byObjectClass) {
      static_cast<void>(objectClassHandle);
      for (auto const& [attributeHandle, regions] : perClass.regionalSubscribedAttributes) {
        static_cast<void>(attributeHandle);
        for (auto const& [regionHandle, active] : regions) {
          static_cast<void>(active);
          auto const region = federation.regions.find(regionHandle);
          if (region != federation.regions.end()) {
            region->second.inUse = true;
          }
        }
      }
    }
  }
  for (auto const& [objectInstanceHandle, objectInstance] : federation.objectInstances) {
    static_cast<void>(objectInstanceHandle);
    for (auto const& [attributeHandle, regions] : objectInstance.updateRegionsByAttribute) {
      static_cast<void>(attributeHandle);
      for (std::uint64_t const regionHandle : regions) {
        auto const region = federation.regions.find(regionHandle);
        if (region != federation.regions.end()) {
          region->second.inUse = true;
        }
      }
    }
  }
}

bool EmbeddedFederationRegistry::validObjectClass(
    Federation const& federation,
    std::uint64_t objectClassHandle) {
  if (!federation.definition.catalog || !federation.objectClassHandles) {
    return false;
  }
  auto const name = federation.objectClassHandles->nameFor(objectClassHandle);
  return name && federation.definition.catalog->objectClass(*name) != nullptr;
}

bool EmbeddedFederationRegistry::validObjectClassAttributes(
    Federation const& federation,
    std::uint64_t objectClassHandle,
    std::set<std::uint64_t> const& attributeHandles) {
  if (!validObjectClass(federation, objectClassHandle) || !federation.attributeHandles) {
    return false;
  }
  auto const objectClassName = federation.objectClassHandles->nameFor(objectClassHandle);
  if (!objectClassName) {
    return false;
  }
  for (std::uint64_t const attributeHandle : attributeHandles) {
    if (!federation.attributeHandles->nameFor(
            federation.definition.catalog.get(),
            *objectClassName,
            attributeHandle)) {
      return false;
    }
  }
  return true;
}

std::optional<std::string> EmbeddedFederationRegistry::attributeTransportationName(
    Federation const& federation,
    std::uint64_t objectClassHandle,
    std::uint64_t attributeHandle) {
  if (!validObjectClass(federation, objectClassHandle) ||
      !federation.definition.catalog ||
      !federation.objectClassHandles ||
      !federation.attributeHandles) {
    return std::nullopt;
  }

  auto const objectClassName = federation.objectClassHandles->nameFor(objectClassHandle);
  if (!objectClassName) {
    return std::nullopt;
  }
  auto const attributeName = federation.attributeHandles->nameFor(
      federation.definition.catalog.get(),
      *objectClassName,
      attributeHandle);
  if (!attributeName) {
    return std::nullopt;
  }

  // AttributeHandleDirectory deliberately exposes one value for an inherited
  // definition. Walk the same object hierarchy to recover the declaration
  // carrying its immutable FOM transportation type.
  std::set<std::string> visited;
  std::string currentClassName = *objectClassName;
  while (!currentClassName.empty() && visited.insert(currentClassName).second) {
    auto const* currentClass = federation.definition.catalog->objectClass(currentClassName);
    if (currentClass == nullptr) {
      return std::nullopt;
    }
    auto const definition = currentClass->declaredAttributes.find(*attributeName);
    if (definition != currentClass->declaredAttributes.end()) {
      return definition->second.transportation.empty()
          ? std::nullopt
          : std::optional{definition->second.transportation};
    }
    currentClassName = currentClass->parentName;
  }
  return std::nullopt;
}

std::optional<std::set<std::uint64_t>> EmbeddedFederationRegistry::publishedObjectClassAttributes(
    Federation const& federation,
    std::uint64_t federateId,
    std::uint64_t objectClassHandle) {
  if (!validObjectClass(federation, objectClassHandle) ||
      !federation.definition.catalog ||
      !federation.objectClassHandles ||
      !federation.attributeHandles) {
    return std::nullopt;
  }

  auto const declarations = federation.objectClassAttributeDeclarations.find(federateId);
  if (declarations == federation.objectClassAttributeDeclarations.end()) {
    return std::set<std::uint64_t>{};
  }
  auto const perClass = declarations->second.byObjectClass.find(objectClassHandle);
  if (perClass == declarations->second.byObjectClass.end()) {
    return std::set<std::uint64_t>{};
  }

  std::set<std::uint64_t> published = perClass->second.explicitlyPublishedAttributes;
  if (published.empty()) {
    return published;
  }

  // IEEE 1516.1-2025 5.1.2 makes HLAprivilegeToDeleteObject implicitly
  // published whenever the established-publication epoch permits it.  The
  // declaration slice retained the epoch flag specifically so registration
  // can snapshot ownership of the actual published attribute set rather than
  // only the explicitly supplied arguments.
  if (!perClass->second.privilegeToDeleteExplicitlyUnpublished) {
    auto const objectClassName = federation.objectClassHandles->nameFor(objectClassHandle);
    if (!objectClassName) {
      return std::nullopt;
    }
    auto const privilegeHandle = federation.attributeHandles->handleFor(
        federation.definition.catalog.get(),
        *objectClassName,
        "HLAprivilegeToDeleteObject");
    if (!privilegeHandle) {
      return std::nullopt;
    }
    published.insert(*privilegeHandle);
  }
  return published;
}

std::optional<std::uint64_t>
EmbeddedFederationRegistry::candidateObjectInstanceDiscoveryClass(
    Federation const& federation,
    Federation::ObjectInstance const& objectInstance,
    std::uint64_t receivingFederateId) {
  // Registration makes the object known to its producer. Discovery requires
  // a different joined federate to own a qualifying instance attribute, so
  // the producer cannot receive an induced Discover Object Instance callback.
  if (objectInstance.deleteAccepted ||
      objectInstance.producingFederateId == receivingFederateId ||
      !federation.members.contains(receivingFederateId) ||
      !federation.members.contains(objectInstance.producingFederateId) ||
      !federation.definition.catalog ||
      !federation.objectClassHandles ||
      !federation.attributeHandles) {
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

  // The candidate discovery class is the registered class when subscribed,
  // otherwise the closest subscribed superclass.  Once a candidate is found,
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
      for (auto const& [ownedAttributeHandle, owningFederateId] :
           objectInstance.attributeOwnersByHandle) {
        static_cast<void>(owningFederateId);
        if (!federation.attributeHandles->nameFor(
                federation.definition.catalog.get(),
                currentClassName,
                ownedAttributeHandle)) {
          continue;
        }
        if (perClass->second.subscribedAttributes.contains(ownedAttributeHandle)) {
          return currentClassHandle;
        }
        auto const regional = perClass->second.regionalSubscribedAttributes.find(
            ownedAttributeHandle);
        auto const associated = objectInstance.updateRegionsByAttribute.find(
            ownedAttributeHandle);
        if (regional == perClass->second.regionalSubscribedAttributes.end() ||
            associated == objectInstance.updateRegionsByAttribute.end() ||
            associated->second.empty()) {
          continue;
        }
        for (auto const& [subscribedRegionHandle, active] : regional->second) {
          static_cast<void>(active);
          for (std::uint64_t const associatedRegionHandle : associated->second) {
            if (regionsOverlap(
                    federation,
                    subscribedRegionHandle,
                    associatedRegionHandle)) {
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

bool EmbeddedFederationRegistry::objectAttributeInScope(
    Federation const& federation,
    Federation::ObjectInstance const& objectInstance,
    std::uint64_t receivingFederateId,
    std::uint64_t attributeHandle,
    std::map<std::uint64_t, RegionSpecificationSnapshot> const* overrides,
    std::map<std::uint64_t, std::set<std::uint64_t>> const* associationOverrides,
    Federation::ObjectClassAttributeDeclarations const* declarationOverrides) {
  if (objectInstance.deleteAccepted ||
      objectInstance.producingFederateId == receivingFederateId ||
      !federation.members.contains(receivingFederateId) ||
      !federation.definition.catalog ||
      !federation.objectClassHandles ||
      !federation.attributeHandles) {
    return false;
  }
  auto const receivingMember = federation.members.find(receivingFederateId);
  if (receivingMember == federation.members.end() ||
      !receivingMember->second.attributeScopeAdvisorySwitch) {
    return false;
  }
  auto const knownClass = objectInstance.knownObjectClassHandlesByFederate.find(
      receivingFederateId);
  if (knownClass == objectInstance.knownObjectClassHandlesByFederate.end()) {
    return false;
  }
  auto const knownClassName = federation.objectClassHandles->nameFor(knownClass->second);
  if (!knownClassName ||
      federation.definition.catalog->objectClass(*knownClassName) == nullptr) {
    return false;
  }
  Federation::ObjectClassAttributeDeclarations const* declarations = declarationOverrides;
  if (declarations == nullptr) {
    auto const currentDeclarations = federation.objectClassAttributeDeclarations.find(
        receivingFederateId);
    if (currentDeclarations == federation.objectClassAttributeDeclarations.end()) {
      return false;
    }
    declarations = &currentDeclarations->second;
  }

  std::set<std::string> visited;
  std::string currentClassName = *knownClassName;
  while (!currentClassName.empty() && visited.insert(currentClassName).second) {
    auto const* currentClass = federation.definition.catalog->objectClass(currentClassName);
    auto const currentClassHandle = federation.objectClassHandles->handleFor(currentClassName);
    if (currentClass == nullptr || !currentClassHandle) {
      return false;
    }
    if (!federation.attributeHandles->nameFor(
            federation.definition.catalog.get(),
            currentClassName,
            attributeHandle)) {
      currentClassName = currentClass->parentName;
      continue;
    }

    auto const perClass = declarations->byObjectClass.find(*currentClassHandle);
    if (perClass != declarations->byObjectClass.end()) {
      if (perClass->second.subscribedAttributes.contains(attributeHandle)) {
        return true;
      }
      auto const regional = perClass->second.regionalSubscribedAttributes.find(
          attributeHandle);
      auto const& updateRegions = associationOverrides == nullptr
          ? objectInstance.updateRegionsByAttribute
          : *associationOverrides;
      auto const associated = updateRegions.find(attributeHandle);
      if (regional != perClass->second.regionalSubscribedAttributes.end() &&
          associated != updateRegions.end() &&
          !associated->second.empty()) {
        for (auto const& [subscribedRegionHandle, active] : regional->second) {
          static_cast<void>(active);
          for (std::uint64_t const associatedRegionHandle : associated->second) {
            if (regionsOverlap(
                    federation,
                    subscribedRegionHandle,
                    associatedRegionHandle,
                    overrides)) {
              return true;
            }
          }
        }
      }
    }
    currentClassName = currentClass->parentName;
  }
  return false;
}

std::vector<ObjectInstanceScopeChangeRecipient>
EmbeddedFederationRegistry::objectInstanceScopeChangesForAssociation(
    Federation const& federation,
    Federation::ObjectInstance const& objectInstance,
    std::set<std::uint64_t> const& attributeHandles,
    std::map<std::uint64_t, std::set<std::uint64_t>> const& associationOverrides) {
  std::vector<ObjectInstanceScopeChangeRecipient> result;
  std::map<std::tuple<std::uint64_t, std::uint64_t, bool>, std::size_t> recipientPositions;
  for (auto const& [receivingFederateId, knownClassHandle] :
       objectInstance.knownObjectClassHandlesByFederate) {
    static_cast<void>(knownClassHandle);
    if (receivingFederateId == objectInstance.producingFederateId ||
        !federation.members.contains(receivingFederateId)) {
      continue;
    }
    auto const callbackRoute = federation.interactionCallbackRoutes.find(receivingFederateId);
    if (callbackRoute == federation.interactionCallbackRoutes.end() ||
        !callbackRoute->second) {
      continue;
    }
    for (std::uint64_t const attributeHandle : attributeHandles) {
      bool const wasInScope = objectAttributeInScope(
          federation,
          objectInstance,
          receivingFederateId,
          attributeHandle);
      bool const isInScope = objectAttributeInScope(
          federation,
          objectInstance,
          receivingFederateId,
          attributeHandle,
          nullptr,
          &associationOverrides);
      if (wasInScope == isInScope) {
        continue;
      }

      auto const key = std::make_tuple(
          receivingFederateId,
          objectInstance.handle,
          isInScope);
      auto position = recipientPositions.find(key);
      if (position == recipientPositions.end()) {
        std::size_t const index = result.size();
        result.push_back({
            receivingFederateId,
            objectInstance.handle,
            {},
            isInScope,
            callbackRoute->second,
        });
        recipientPositions.emplace(key, index);
        position = recipientPositions.find(key);
      }
      result[position->second].attributeHandles.insert(attributeHandle);
    }
  }
  return result;
}

std::vector<ObjectInstanceScopeChangeRecipient>
EmbeddedFederationRegistry::objectInstanceScopeChangesForSubscription(
    Federation const& federation,
    std::uint64_t receivingFederateId,
    std::set<std::uint64_t> const& attributeHandles,
    Federation::ObjectClassAttributeDeclarations const& previousDeclarations) {
  std::vector<ObjectInstanceScopeChangeRecipient> result;
  std::map<std::tuple<std::uint64_t, std::uint64_t, bool>, std::size_t> recipientPositions;
  if (!federation.members.contains(receivingFederateId)) {
    return result;
  }
  auto const callbackRoute = federation.interactionCallbackRoutes.find(receivingFederateId);
  if (callbackRoute == federation.interactionCallbackRoutes.end() ||
      !callbackRoute->second) {
    return result;
  }

  for (auto const& [objectInstanceHandle, objectInstance] : federation.objectInstances) {
    static_cast<void>(objectInstanceHandle);
    if (objectInstance.producingFederateId == receivingFederateId ||
        !objectInstance.knownObjectClassHandlesByFederate.contains(receivingFederateId)) {
      continue;
    }
    for (std::uint64_t const attributeHandle : attributeHandles) {
      bool const wasInScope = objectAttributeInScope(
          federation,
          objectInstance,
          receivingFederateId,
          attributeHandle,
          nullptr,
          nullptr,
          &previousDeclarations);
      bool const isInScope = objectAttributeInScope(
          federation,
          objectInstance,
          receivingFederateId,
          attributeHandle);
      if (wasInScope == isInScope) {
        continue;
      }

      auto const key = std::make_tuple(
          receivingFederateId,
          objectInstance.handle,
          isInScope);
      auto position = recipientPositions.find(key);
      if (position == recipientPositions.end()) {
        std::size_t const index = result.size();
        result.push_back({
            receivingFederateId,
            objectInstance.handle,
            {},
            isInScope,
            callbackRoute->second,
        });
        recipientPositions.emplace(key, index);
        position = recipientPositions.find(key);
      }
      result[position->second].attributeHandles.insert(attributeHandle);
    }
  }
  return result;
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

std::optional<ReceiveOrderAttributeUpdateRecipient>
EmbeddedFederationRegistry::candidateReceiveOrderAttributeUpdateRecipient(
    Federation const& federation,
    std::uint64_t producingFederateId,
    std::uint64_t receivingFederateId,
    std::uint64_t objectInstanceHandle,
    std::vector<std::uint64_t> const& sentAttributeHandles,
    std::set<std::uint64_t> const* sentRegionHandles) {
  // IEEE 1516.1-2025 excludes the federate that invoked Update Attribute
  // Values from its induced Reflect Attribute Values callbacks, independent
  // of subscription state.
  if (producingFederateId == receivingFederateId ||
      !federation.members.contains(receivingFederateId) ||
      !federation.definition.catalog ||
      !federation.objectClassHandles ||
      !federation.attributeHandles) {
    return std::nullopt;
  }

  auto const instance = federation.objectInstances.find(objectInstanceHandle);
  if (instance == federation.objectInstances.end()) {
    return std::nullopt;
  }
  auto const knownClass = instance->second.knownObjectClassHandlesByFederate.find(
      receivingFederateId);
  if (knownClass == instance->second.knownObjectClassHandlesByFederate.end()) {
    return std::nullopt;
  }

  auto const knownClassName = federation.objectClassHandles->nameFor(knownClass->second);
  if (!knownClassName ||
      federation.definition.catalog->objectClass(*knownClassName) == nullptr) {
    return std::nullopt;
  }
  auto const declarations = federation.objectClassAttributeDeclarations.find(receivingFederateId);
  if (declarations == federation.objectClassAttributeDeclarations.end()) {
    return std::nullopt;
  }
  auto const perClass = declarations->second.byObjectClass.find(knownClass->second);
  if (perClass == declarations->second.byObjectClass.end()) {
    return std::nullopt;
  }

  ReceiveOrderAttributeUpdateRecipient recipient;
  recipient.federateId = receivingFederateId;
  for (std::uint64_t const attributeHandle : sentAttributeHandles) {
    // A reflection is evaluated at the receiver's known class, not the
    // registered class. A promoted receiver therefore never sees an attribute
    // introduced below that class, even if the sender supplied it.
    if (!federation.attributeHandles->nameFor(
            federation.definition.catalog.get(),
            *knownClassName,
            attributeHandle)) {
      continue;
    }
    bool subscribed = perClass->second.subscribedAttributes.contains(attributeHandle);
    if (!subscribed && sentRegionHandles != nullptr && !sentRegionHandles->empty()) {
      auto const regional = perClass->second.regionalSubscribedAttributes.find(attributeHandle);
      auto const associated = instance->second.updateRegionsByAttribute.find(attributeHandle);
      if (regional != perClass->second.regionalSubscribedAttributes.end() &&
          associated != instance->second.updateRegionsByAttribute.end()) {
        std::set<std::uint64_t> currentSentRegions;
        for (std::uint64_t const sentRegionHandle : *sentRegionHandles) {
          if (associated->second.contains(sentRegionHandle)) {
            currentSentRegions.insert(sentRegionHandle);
          }
        }
        for (auto const& [subscribedRegionHandle, active] : regional->second) {
          static_cast<void>(active);
          for (std::uint64_t const sentRegionHandle : currentSentRegions) {
            if (regionsOverlap(
                    federation,
                    subscribedRegionHandle,
                    sentRegionHandle)) {
              subscribed = true;
              break;
            }
          }
          if (subscribed) {
            break;
          }
        }
      }
    }
    if (subscribed) {
      recipient.receivedAttributeHandles.insert(attributeHandle);
    }
  }
  if (recipient.receivedAttributeHandles.empty()) {
    return std::nullopt;
  }

  auto const callbackRoute = federation.interactionCallbackRoutes.find(receivingFederateId);
  if (callbackRoute == federation.interactionCallbackRoutes.end() || !callbackRoute->second) {
    return std::nullopt;
  }
  recipient.callbackRoute = callbackRoute->second;
  return recipient;
}

std::optional<AttributeValueUpdateProvideRecipient>
EmbeddedFederationRegistry::candidateAttributeValueUpdateProvideRecipient(
    Federation const& federation,
    std::uint64_t requestingFederateId,
    std::uint64_t providingFederateId,
    std::uint64_t objectInstanceHandle,
    std::set<std::uint64_t> const& requestedAttributeHandles) {
  // The requester owns an implicit local provide. It must not receive a
  // Provide Attribute Value Update callback for attributes it owns itself.
  if (requestingFederateId == providingFederateId ||
      !federation.members.contains(requestingFederateId) ||
      !federation.members.contains(providingFederateId) ||
      !federation.definition.catalog ||
      !federation.objectClassHandles ||
      !federation.attributeHandles) {
    return std::nullopt;
  }

  auto const instance = federation.objectInstances.find(objectInstanceHandle);
  if (instance == federation.objectInstances.end() || instance->second.deleteAccepted) {
    return std::nullopt;
  }
  auto const knownClass = instance->second.knownObjectClassHandlesByFederate.find(
      requestingFederateId);
  if (knownClass == instance->second.knownObjectClassHandlesByFederate.end()) {
    return std::nullopt;
  }
  auto const knownClassName = federation.objectClassHandles->nameFor(knownClass->second);
  if (!knownClassName ||
      federation.definition.catalog->objectClass(*knownClassName) == nullptr) {
    return std::nullopt;
  }

  AttributeValueUpdateProvideRecipient recipient;
  recipient.objectInstanceHandle = objectInstanceHandle;
  recipient.providingFederateId = providingFederateId;
  for (std::uint64_t const attributeHandle : requestedAttributeHandles) {
    auto const owner = instance->second.attributeOwnersByHandle.find(attributeHandle);
    if (owner != instance->second.attributeOwnersByHandle.end() &&
        owner->second == providingFederateId &&
        federation.attributeHandles->nameFor(
            federation.definition.catalog.get(),
            *knownClassName,
            attributeHandle)) {
      recipient.requestedAttributeHandles.insert(attributeHandle);
    }
  }
  if (recipient.requestedAttributeHandles.empty()) {
    return std::nullopt;
  }

  auto const callbackRoute = federation.interactionCallbackRoutes.find(providingFederateId);
  if (callbackRoute == federation.interactionCallbackRoutes.end() || !callbackRoute->second) {
    return std::nullopt;
  }
  recipient.callbackRoute = callbackRoute->second;
  return recipient;
}

bool EmbeddedFederationRegistry::objectInstanceRegisteredAtOrBelowClass(
    Federation const& federation,
    std::uint64_t registeredObjectClassHandle,
    std::uint64_t requestedObjectClassHandle) {
  if (!federation.definition.catalog || !federation.objectClassHandles) {
    return false;
  }
  auto const requestedClassName = federation.objectClassHandles->nameFor(
      requestedObjectClassHandle);
  auto const registeredClassName = federation.objectClassHandles->nameFor(
      registeredObjectClassHandle);
  if (!requestedClassName || !registeredClassName ||
      federation.definition.catalog->objectClass(*requestedClassName) == nullptr) {
    return false;
  }

  std::set<std::string> visited;
  std::string currentClassName = *registeredClassName;
  while (!currentClassName.empty() && visited.insert(currentClassName).second) {
    if (currentClassName == *requestedClassName) {
      return true;
    }
    auto const* currentClass = federation.definition.catalog->objectClass(currentClassName);
    if (currentClass == nullptr) {
      return false;
    }
    currentClassName = currentClass->parentName;
  }
  return false;
}

std::optional<AttributeValueUpdateProvideRecipient>
EmbeddedFederationRegistry::candidateAttributeValueUpdateClassProvideRecipient(
    Federation const& federation,
    std::uint64_t requestingFederateId,
    std::uint64_t providingFederateId,
    std::uint64_t objectInstanceHandle,
    std::uint64_t requestedObjectClassHandle,
    std::set<std::uint64_t> const& requestedAttributeHandles,
    std::map<std::uint64_t, std::set<std::uint64_t>> const* requestRegionsByAttribute) {
  // The requester owns an implicit local provide for attributes it owns on an
  // expanded instance. It must not receive a public Provide callback itself.
  if (requestingFederateId == providingFederateId ||
      !federation.members.contains(requestingFederateId) ||
      !federation.members.contains(providingFederateId) ||
      !federation.definition.catalog ||
      !federation.objectClassHandles ||
      !federation.attributeHandles) {
    return std::nullopt;
  }

  auto const instance = federation.objectInstances.find(objectInstanceHandle);
  if (instance == federation.objectInstances.end() || instance->second.deleteAccepted ||
      !objectInstanceRegisteredAtOrBelowClass(
          federation,
          instance->second.registeredObjectClassHandle,
          requestedObjectClassHandle)) {
    return std::nullopt;
  }
  auto const registeredClassName = federation.objectClassHandles->nameFor(
      instance->second.registeredObjectClassHandle);
  if (!registeredClassName ||
      federation.definition.catalog->objectClass(*registeredClassName) == nullptr) {
    return std::nullopt;
  }

  AttributeValueUpdateProvideRecipient recipient;
  recipient.objectInstanceHandle = objectInstanceHandle;
  recipient.providingFederateId = providingFederateId;
  for (std::uint64_t const attributeHandle : requestedAttributeHandles) {
    auto const owner = instance->second.attributeOwnersByHandle.find(attributeHandle);
    if (owner == instance->second.attributeOwnersByHandle.end() ||
        owner->second != providingFederateId ||
        !federation.attributeHandles->nameFor(
            federation.definition.catalog.get(),
            *registeredClassName,
            attributeHandle)) {
      continue;
    }

    // A regional request solicits an explicit update-region association only
    // when it overlaps one of the request regions for that attribute. An
    // attribute with no explicit association uses the standard default region
    // and is therefore always eligible. Empty request-region sets are a
    // no-op for that attribute and should never reach the provider callback.
    if (requestRegionsByAttribute != nullptr) {
      auto const requestRegions = requestRegionsByAttribute->find(attributeHandle);
      if (requestRegions == requestRegionsByAttribute->end() ||
          requestRegions->second.empty()) {
        continue;
      }
      auto const updateRegions = instance->second.updateRegionsByAttribute.find(attributeHandle);
      if (updateRegions != instance->second.updateRegionsByAttribute.end() &&
          !updateRegions->second.empty()) {
        bool overlaps = false;
        for (std::uint64_t const requestRegionHandle : requestRegions->second) {
          for (std::uint64_t const updateRegionHandle : updateRegions->second) {
            if (regionsOverlap(federation, requestRegionHandle, updateRegionHandle)) {
              overlaps = true;
              break;
            }
          }
          if (overlaps) {
            break;
          }
        }
        if (!overlaps) {
          continue;
        }
      }
    }
    recipient.requestedAttributeHandles.insert(attributeHandle);
  }
  if (recipient.requestedAttributeHandles.empty()) {
    return std::nullopt;
  }

  auto const callbackRoute = federation.interactionCallbackRoutes.find(providingFederateId);
  if (callbackRoute == federation.interactionCallbackRoutes.end() || !callbackRoute->second) {
    return std::nullopt;
  }
  recipient.callbackRoute = callbackRoute->second;
  return recipient;
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

  auto const instance = federation.objectInstances.find(objectInstanceHandle);
  // A pending Remove Object Instance invalidates every queued ownership report
  // for that instance before a federate callback may be made.
  if (instance == federation.objectInstances.end() || instance->second.deleteAccepted) {
    return std::nullopt;
  }
  auto const knownClass = instance->second.knownObjectClassHandlesByFederate.find(
      requestingFederateId);
  if (knownClass == instance->second.knownObjectClassHandlesByFederate.end()) {
    return std::nullopt;
  }
  auto const knownClassName = federation.objectClassHandles->nameFor(knownClass->second);
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

    auto const owner = instance->second.attributeOwnersByHandle.find(attributeHandle);
    switch (reportKind) {
      case AttributeOwnershipQueryReportKind::federate:
        if (owner != instance->second.attributeOwnersByHandle.end() &&
            owner->second == owningFederateId &&
            federation.members.contains(owningFederateId)) {
          recipient.attributeHandles.insert(attributeHandle);
        }
        break;
      case AttributeOwnershipQueryReportKind::unowned:
        if (owner == instance->second.attributeOwnersByHandle.end()) {
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
    std::uint64_t producingFederateId,
    std::uint64_t receivingFederateId,
    std::uint64_t sentInteractionClassHandle,
    std::vector<std::uint64_t> const& sentParameterHandles,
    std::set<std::uint64_t> const* sentRegionHandles) {
  // IEEE 1516.1-2025 prevents a sender from receiving its own induced Receive
  // Interaction callback, independent of its subscription state.
  if (producingFederateId == receivingFederateId ||
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

  // The candidate received class is the sent class when it is subscribed, or
  // the closest subscribed superclass.  Traversing the parent chain rather
  // than iterating subscriptions also guarantees at most one callback for a
  // recipient with subscriptions at multiple hierarchy locations.
  std::set<std::string> visited;
  std::string currentClassName = *sentClassName;
  while (!currentClassName.empty() && visited.insert(currentClassName).second) {
    auto const* currentClass = federation.definition.catalog->interactionClass(currentClassName);
    auto const currentClassHandle = federation.interactionClassHandles->handleFor(currentClassName);
    if (currentClass == nullptr || !currentClassHandle) {
      return std::nullopt;
    }

    bool subscribedWithoutRegion = declarations->second.subscribedInteractionClasses.contains(
        *currentClassHandle);
    bool subscribedWithRegion = false;
    if (!subscribedWithoutRegion && sentRegionHandles != nullptr &&
        !sentRegionHandles->empty()) {
      auto const regional = declarations->second.regionalSubscribedInteractionClasses.find(
          *currentClassHandle);
      if (regional != declarations->second.regionalSubscribedInteractionClasses.end()) {
        for (auto const& [subscribedRegionHandle, active] : regional->second) {
          static_cast<void>(active);
          for (std::uint64_t const sentRegionHandle : *sentRegionHandles) {
            if (regionsOverlap(federation, subscribedRegionHandle, sentRegionHandle)) {
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
      recipient.receivedInteractionClassHandle = *currentClassHandle;
      auto const callbackRoute = federation.interactionCallbackRoutes.find(receivingFederateId);
      if (callbackRoute != federation.interactionCallbackRoutes.end()) {
        recipient.callbackRoute = callbackRoute->second;
      }
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

bool EmbeddedFederationRegistry::validDirectedInteractionForObjectClass(
    Federation const& federation,
    std::uint64_t objectClassHandle,
    std::uint64_t interactionClassHandle) {
  if (!validObjectClass(federation, objectClassHandle) ||
      !validInteractionClass(federation, interactionClassHandle) ||
      !federation.definition.catalog || !federation.objectClassHandles ||
      !federation.interactionClassHandles) {
    return false;
  }

  auto const objectClassName = federation.objectClassHandles->nameFor(objectClassHandle);
  auto const interactionClassName =
      federation.interactionClassHandles->nameFor(interactionClassHandle);
  if (!objectClassName || !interactionClassName) {
    return false;
  }

  std::set<std::string> visited;
  std::string currentClassName = *objectClassName;
  while (!currentClassName.empty() && visited.insert(currentClassName).second) {
    auto const* objectClass = federation.definition.catalog->objectClass(currentClassName);
    if (objectClass == nullptr) {
      return false;
    }
    if (std::find(
            objectClass->directedInteractions.begin(),
            objectClass->directedInteractions.end(),
            *interactionClassName) != objectClass->directedInteractions.end()) {
      return true;
    }
    currentClassName = objectClass->parentName;
  }
  return false;
}

bool EmbeddedFederationRegistry::directedInteractionDeclarationApplies(
    Federation const& federation,
    std::uint64_t federateId,
    std::uint64_t registeredObjectClassHandle,
    std::uint64_t interactionClassHandle,
    bool publication) {
  if (!federation.objectClassHandles || !federation.definition.catalog) {
    return false;
  }
  auto const declarations = federation.interactionDeclarations.find(federateId);
  if (declarations == federation.interactionDeclarations.end()) {
    return false;
  }
  auto const& declarationsByObjectClass = publication
      ? declarations->second.publishedObjectClassDirectedInteractions
      : declarations->second.subscribedObjectClassDirectedInteractions;

  auto const registeredClassName =
      federation.objectClassHandles->nameFor(registeredObjectClassHandle);
  if (!registeredClassName) {
    return false;
  }
  std::set<std::string> visited;
  std::string currentClassName = *registeredClassName;
  while (!currentClassName.empty() && visited.insert(currentClassName).second) {
    auto const currentClassHandle = federation.objectClassHandles->handleFor(currentClassName);
    if (!currentClassHandle) {
      return false;
    }
    auto const declaration = declarationsByObjectClass.find(*currentClassHandle);
    if (declaration != declarationsByObjectClass.end() &&
        declaration->second.contains(interactionClassHandle)) {
      return true;
    }
    auto const* objectClass = federation.definition.catalog->objectClass(currentClassName);
    if (objectClass == nullptr) {
      return false;
    }
    currentClassName = objectClass->parentName;
  }
  return false;
}

std::optional<ReceiveOrderDirectedInteractionRecipient>
EmbeddedFederationRegistry::candidateReceiveOrderDirectedInteractionRecipient(
    Federation const& federation,
    std::uint64_t producingFederateId,
    std::uint64_t receivingFederateId,
    std::uint64_t objectInstanceHandle,
    std::uint64_t sentInteractionClassHandle,
    std::vector<std::uint64_t> const& sentParameterHandles) {
  if (producingFederateId == receivingFederateId ||
      !federation.members.contains(receivingFederateId) ||
      !federation.members.contains(producingFederateId) ||
      !federation.definition.catalog || !federation.objectClassHandles ||
      !federation.interactionClassHandles || !federation.parameterHandles) {
    return std::nullopt;
  }

  auto const instance = federation.objectInstances.find(objectInstanceHandle);
  if (instance == federation.objectInstances.end() || instance->second.deleteAccepted ||
      !instance->second.knownObjectClassHandlesByFederate.contains(receivingFederateId) ||
      !instance->second.knownObjectClassHandlesByFederate.contains(producingFederateId)) {
    return std::nullopt;
  }
  if (!validDirectedInteractionForObjectClass(
          federation,
          instance->second.registeredObjectClassHandle,
          sentInteractionClassHandle) ||
      !directedInteractionDeclarationApplies(
          federation,
          producingFederateId,
          instance->second.registeredObjectClassHandle,
          sentInteractionClassHandle,
          true) ||
      !directedInteractionDeclarationApplies(
          federation,
          receivingFederateId,
          instance->second.registeredObjectClassHandle,
          sentInteractionClassHandle,
          false)) {
    return std::nullopt;
  }

  auto const sentClassName =
      federation.interactionClassHandles->nameFor(sentInteractionClassHandle);
  if (!sentClassName ||
      federation.definition.catalog->interactionClass(*sentClassName) == nullptr) {
    return std::nullopt;
  }

  ReceiveOrderDirectedInteractionRecipient recipient;
  recipient.federateId = receivingFederateId;
  recipient.objectInstanceHandle = objectInstanceHandle;
  recipient.receivedInteractionClassHandle = sentInteractionClassHandle;
  for (std::uint64_t const parameterHandle : sentParameterHandles) {
    if (federation.parameterHandles->nameFor(
            federation.definition.catalog.get(),
            *sentClassName,
            parameterHandle)) {
      recipient.receivedParameterHandles.insert(parameterHandle);
    }
  }
  auto const callbackRoute = federation.interactionCallbackRoutes.find(receivingFederateId);
  if (callbackRoute == federation.interactionCallbackRoutes.end() || !callbackRoute->second) {
    return std::nullopt;
  }
  recipient.callbackRoute = callbackRoute->second;
  return recipient;
}

InteractionClassDeclarationStatus EmbeddedFederationRegistry::setInteractionClassPublication(
    std::wstring const& federationName,
    std::uint64_t federateId,
    std::uint64_t interactionClassHandle,
    bool published) {
  std::scoped_lock lock(mutex_);
  auto federation = federations_.find(federationName);
  if (federation == federations_.end()) {
    return InteractionClassDeclarationStatus::federation_does_not_exist;
  }
  if (!federation->second.members.contains(federateId)) {
    return InteractionClassDeclarationStatus::federate_not_member;
  }
  if (!validInteractionClass(federation->second, interactionClassHandle)) {
    return InteractionClassDeclarationStatus::interaction_class_not_defined;
  }

  auto declarations = federation->second.interactionDeclarations.find(federateId);
  if (!published) {
    if (declarations == federation->second.interactionDeclarations.end()) {
      return InteractionClassDeclarationStatus::applied;
    }
    declarations->second.publishedInteractionClasses.erase(interactionClassHandle);
    if (declarations->second.publishedInteractionClasses.empty() &&
        declarations->second.subscribedInteractionClasses.empty() &&
        declarations->second.regionalSubscribedInteractionClasses.empty() &&
        declarations->second.publishedObjectClassDirectedInteractions.empty() &&
        declarations->second.subscribedObjectClassDirectedInteractions.empty()) {
      federation->second.interactionDeclarations.erase(declarations);
    }
    return InteractionClassDeclarationStatus::applied;
  }

  auto [state, insertedState] = federation->second.interactionDeclarations.try_emplace(federateId);
  try {
    static_cast<void>(state->second.publishedInteractionClasses.insert(interactionClassHandle));
  } catch (...) {
    if (insertedState && state->second.publishedInteractionClasses.empty() &&
        state->second.subscribedInteractionClasses.empty() &&
        state->second.regionalSubscribedInteractionClasses.empty() &&
        state->second.publishedObjectClassDirectedInteractions.empty() &&
        state->second.subscribedObjectClassDirectedInteractions.empty()) {
      federation->second.interactionDeclarations.erase(state);
    }
    throw;
  }
  return InteractionClassDeclarationStatus::applied;
}

DirectedInteractionDeclarationStatus
EmbeddedFederationRegistry::publishObjectClassDirectedInteractions(
    std::wstring const& federationName,
    std::uint64_t federateId,
    std::uint64_t objectClassHandle,
    std::set<std::uint64_t> const& interactionClassHandles) {
  std::scoped_lock lock(mutex_);
  auto federation = federations_.find(federationName);
  if (federation == federations_.end()) {
    return DirectedInteractionDeclarationStatus::federation_does_not_exist;
  }
  if (!federation->second.members.contains(federateId)) {
    return DirectedInteractionDeclarationStatus::federate_not_member;
  }
  if (!validObjectClass(federation->second, objectClassHandle)) {
    return DirectedInteractionDeclarationStatus::object_class_not_defined;
  }
  for (std::uint64_t const interactionClassHandle : interactionClassHandles) {
    if (!validInteractionClass(federation->second, interactionClassHandle)) {
      return DirectedInteractionDeclarationStatus::interaction_class_not_defined;
    }
    if (!validDirectedInteractionForObjectClass(
            federation->second,
            objectClassHandle,
            interactionClassHandle)) {
      return DirectedInteractionDeclarationStatus::interaction_not_defined_for_object_class;
    }
  }
  if (interactionClassHandles.empty()) {
    return DirectedInteractionDeclarationStatus::applied;
  }

  auto [state, insertedState] = federation->second.interactionDeclarations.try_emplace(federateId);
  try {
    auto revised = state->second.publishedObjectClassDirectedInteractions;
    revised[objectClassHandle].insert(
        interactionClassHandles.begin(),
        interactionClassHandles.end());
    state->second.publishedObjectClassDirectedInteractions.swap(revised);
  } catch (...) {
    if (insertedState && state->second.publishedInteractionClasses.empty() &&
        state->second.subscribedInteractionClasses.empty() &&
        state->second.regionalSubscribedInteractionClasses.empty() &&
        state->second.publishedObjectClassDirectedInteractions.empty() &&
        state->second.subscribedObjectClassDirectedInteractions.empty()) {
      federation->second.interactionDeclarations.erase(state);
    }
    throw;
  }
  return DirectedInteractionDeclarationStatus::applied;
}

DirectedInteractionDeclarationStatus
EmbeddedFederationRegistry::unpublishObjectClassDirectedInteractions(
    std::wstring const& federationName,
    std::uint64_t federateId,
    std::uint64_t objectClassHandle,
    std::optional<std::set<std::uint64_t>> const& interactionClassHandles) {
  std::scoped_lock lock(mutex_);
  auto federation = federations_.find(federationName);
  if (federation == federations_.end()) {
    return DirectedInteractionDeclarationStatus::federation_does_not_exist;
  }
  if (!federation->second.members.contains(federateId)) {
    return DirectedInteractionDeclarationStatus::federate_not_member;
  }
  if (!validObjectClass(federation->second, objectClassHandle)) {
    return DirectedInteractionDeclarationStatus::object_class_not_defined;
  }
  if (interactionClassHandles) {
    for (std::uint64_t const interactionClassHandle : *interactionClassHandles) {
      if (!validInteractionClass(federation->second, interactionClassHandle)) {
        return DirectedInteractionDeclarationStatus::interaction_class_not_defined;
      }
      if (!validDirectedInteractionForObjectClass(
              federation->second,
              objectClassHandle,
              interactionClassHandle)) {
        return DirectedInteractionDeclarationStatus::interaction_not_defined_for_object_class;
      }
    }
  }

  auto declarations = federation->second.interactionDeclarations.find(federateId);
  if (declarations != federation->second.interactionDeclarations.end()) {
    auto revised = declarations->second.publishedObjectClassDirectedInteractions;
    if (!interactionClassHandles) {
      revised.erase(objectClassHandle);
    } else {
      auto published = revised.find(objectClassHandle);
      if (published != revised.end()) {
        for (std::uint64_t const interactionClassHandle : *interactionClassHandles) {
          published->second.erase(interactionClassHandle);
        }
        if (published->second.empty()) {
          revised.erase(published);
        }
      }
    }
    declarations->second.publishedObjectClassDirectedInteractions.swap(revised);
    if (declarations->second.publishedInteractionClasses.empty() &&
        declarations->second.subscribedInteractionClasses.empty() &&
        declarations->second.regionalSubscribedInteractionClasses.empty() &&
        declarations->second.publishedObjectClassDirectedInteractions.empty() &&
        declarations->second.subscribedObjectClassDirectedInteractions.empty()) {
      federation->second.interactionDeclarations.erase(declarations);
    }
  }
  return DirectedInteractionDeclarationStatus::applied;
}

DirectedInteractionDeclarationStatus
EmbeddedFederationRegistry::subscribeObjectClassDirectedInteractions(
    std::wstring const& federationName,
    std::uint64_t federateId,
    std::uint64_t objectClassHandle,
    std::set<std::uint64_t> const& interactionClassHandles,
    bool universally) {
  std::scoped_lock lock(mutex_);
  static_cast<void>(universally);
  auto federation = federations_.find(federationName);
  if (federation == federations_.end()) {
    return DirectedInteractionDeclarationStatus::federation_does_not_exist;
  }
  if (!federation->second.members.contains(federateId)) {
    return DirectedInteractionDeclarationStatus::federate_not_member;
  }
  if (!validObjectClass(federation->second, objectClassHandle)) {
    return DirectedInteractionDeclarationStatus::object_class_not_defined;
  }
  for (std::uint64_t const interactionClassHandle : interactionClassHandles) {
    if (!validInteractionClass(federation->second, interactionClassHandle)) {
      return DirectedInteractionDeclarationStatus::interaction_class_not_defined;
    }
    if (!validDirectedInteractionForObjectClass(
            federation->second,
            objectClassHandle,
            interactionClassHandle)) {
      return DirectedInteractionDeclarationStatus::interaction_not_defined_for_object_class;
    }
  }
  if (interactionClassHandles.empty()) {
    return DirectedInteractionDeclarationStatus::applied;
  }

  auto [state, insertedState] = federation->second.interactionDeclarations.try_emplace(federateId);
  try {
    auto revised = state->second.subscribedObjectClassDirectedInteractions;
    revised[objectClassHandle].insert(
        interactionClassHandles.begin(),
        interactionClassHandles.end());
    state->second.subscribedObjectClassDirectedInteractions.swap(revised);
  } catch (...) {
    if (insertedState && state->second.publishedInteractionClasses.empty() &&
        state->second.subscribedInteractionClasses.empty() &&
        state->second.regionalSubscribedInteractionClasses.empty() &&
        state->second.publishedObjectClassDirectedInteractions.empty() &&
        state->second.subscribedObjectClassDirectedInteractions.empty()) {
      federation->second.interactionDeclarations.erase(state);
    }
    throw;
  }
  return DirectedInteractionDeclarationStatus::applied;
}

DirectedInteractionDeclarationStatus
EmbeddedFederationRegistry::unsubscribeObjectClassDirectedInteractions(
    std::wstring const& federationName,
    std::uint64_t federateId,
    std::uint64_t objectClassHandle,
    std::optional<std::set<std::uint64_t>> const& interactionClassHandles) {
  std::scoped_lock lock(mutex_);
  auto federation = federations_.find(federationName);
  if (federation == federations_.end()) {
    return DirectedInteractionDeclarationStatus::federation_does_not_exist;
  }
  if (!federation->second.members.contains(federateId)) {
    return DirectedInteractionDeclarationStatus::federate_not_member;
  }
  if (!validObjectClass(federation->second, objectClassHandle)) {
    return DirectedInteractionDeclarationStatus::object_class_not_defined;
  }
  if (interactionClassHandles) {
    for (std::uint64_t const interactionClassHandle : *interactionClassHandles) {
      if (!validInteractionClass(federation->second, interactionClassHandle)) {
        return DirectedInteractionDeclarationStatus::interaction_class_not_defined;
      }
      if (!validDirectedInteractionForObjectClass(
              federation->second,
              objectClassHandle,
              interactionClassHandle)) {
        return DirectedInteractionDeclarationStatus::interaction_not_defined_for_object_class;
      }
    }
  }

  auto declarations = federation->second.interactionDeclarations.find(federateId);
  if (declarations != federation->second.interactionDeclarations.end()) {
    auto revised = declarations->second.subscribedObjectClassDirectedInteractions;
    if (!interactionClassHandles) {
      revised.erase(objectClassHandle);
    } else {
      auto subscribed = revised.find(objectClassHandle);
      if (subscribed != revised.end()) {
        for (std::uint64_t const interactionClassHandle : *interactionClassHandles) {
          subscribed->second.erase(interactionClassHandle);
        }
        if (subscribed->second.empty()) {
          revised.erase(subscribed);
        }
      }
    }
    declarations->second.subscribedObjectClassDirectedInteractions.swap(revised);
    if (declarations->second.publishedInteractionClasses.empty() &&
        declarations->second.subscribedInteractionClasses.empty() &&
        declarations->second.regionalSubscribedInteractionClasses.empty() &&
        declarations->second.publishedObjectClassDirectedInteractions.empty() &&
        declarations->second.subscribedObjectClassDirectedInteractions.empty()) {
      federation->second.interactionDeclarations.erase(declarations);
    }
  }
  return DirectedInteractionDeclarationStatus::applied;
}

InteractionClassDeclarationStatus EmbeddedFederationRegistry::setInteractionClassSubscription(
    std::wstring const& federationName,
    std::uint64_t federateId,
    std::uint64_t interactionClassHandle,
    std::optional<bool> active) {
  std::scoped_lock lock(mutex_);
  auto federation = federations_.find(federationName);
  if (federation == federations_.end()) {
    return InteractionClassDeclarationStatus::federation_does_not_exist;
  }
  if (!federation->second.members.contains(federateId)) {
    return InteractionClassDeclarationStatus::federate_not_member;
  }
  if (!validInteractionClass(federation->second, interactionClassHandle)) {
    return InteractionClassDeclarationStatus::interaction_class_not_defined;
  }

  auto declarations = federation->second.interactionDeclarations.find(federateId);
  if (!active) {
    if (declarations == federation->second.interactionDeclarations.end()) {
      return InteractionClassDeclarationStatus::applied;
    }
    declarations->second.subscribedInteractionClasses.erase(interactionClassHandle);
    if (declarations->second.publishedInteractionClasses.empty() &&
        declarations->second.subscribedInteractionClasses.empty() &&
        declarations->second.regionalSubscribedInteractionClasses.empty() &&
        declarations->second.publishedObjectClassDirectedInteractions.empty() &&
        declarations->second.subscribedObjectClassDirectedInteractions.empty()) {
      federation->second.interactionDeclarations.erase(declarations);
    }
    return InteractionClassDeclarationStatus::applied;
  }

  auto [state, insertedState] = federation->second.interactionDeclarations.try_emplace(federateId);
  try {
    state->second.subscribedInteractionClasses.insert_or_assign(interactionClassHandle, *active);
  } catch (...) {
    if (insertedState && state->second.publishedInteractionClasses.empty() &&
        state->second.subscribedInteractionClasses.empty() &&
        state->second.regionalSubscribedInteractionClasses.empty() &&
        state->second.publishedObjectClassDirectedInteractions.empty() &&
        state->second.subscribedObjectClassDirectedInteractions.empty()) {
      federation->second.interactionDeclarations.erase(state);
    }
    throw;
  }
  return InteractionClassDeclarationStatus::applied;
}

RegionalInteractionClassDeclarationStatus
EmbeddedFederationRegistry::setInteractionClassRegionalSubscription(
    std::wstring const& federationName,
    std::uint64_t federateId,
    std::uint64_t interactionClassHandle,
    std::set<std::uint64_t> const& regionHandles,
    bool active) {
  std::scoped_lock lock(mutex_);
  auto federation = federations_.find(federationName);
  if (federation == federations_.end()) {
    return RegionalInteractionClassDeclarationStatus::federation_does_not_exist;
  }
  if (!federation->second.members.contains(federateId)) {
    return RegionalInteractionClassDeclarationStatus::federate_not_member;
  }
  if (!validInteractionClass(federation->second, interactionClassHandle)) {
    return RegionalInteractionClassDeclarationStatus::interaction_class_not_defined;
  }
  // The 2025 service explicitly gives an empty region set no effect.  It is
  // still a valid declaration invocation once the interaction class itself
  // has been resolved.
  if (regionHandles.empty()) {
    return RegionalInteractionClassDeclarationStatus::applied;
  }

  auto const availableDimensions = availableInteractionDimensions(
      federation->second,
      interactionClassHandle);
  if (!availableDimensions) {
    return RegionalInteractionClassDeclarationStatus::inconsistent_catalog;
  }
  for (std::uint64_t const regionHandle : regionHandles) {
    auto const region = federation->second.regions.find(regionHandle);
    if (region == federation->second.regions.end() ||
        !region->second.specificationCommitted ||
        region->second.committedRangeBounds.size() != region->second.dimensionHandles.size()) {
      return RegionalInteractionClassDeclarationStatus::invalid_region;
    }
    if (region->second.ownerFederateId != federateId) {
      return RegionalInteractionClassDeclarationStatus::region_not_created_by_this_federate;
    }
    if (!std::includes(
            availableDimensions->begin(),
            availableDimensions->end(),
            region->second.dimensionHandles.begin(),
            region->second.dimensionHandles.end())) {
      return RegionalInteractionClassDeclarationStatus::invalid_region_context;
    }
  }

  auto [state, insertedState] = federation->second.interactionDeclarations.try_emplace(federateId);
  try {
    auto& subscriptions = state->second.regionalSubscribedInteractionClasses[
        interactionClassHandle];
    for (std::uint64_t const regionHandle : regionHandles) {
      // Detailed 9.10 semantics permit an existing pair's active/passive
      // nature to change while keeping the pair in the subscription set.
      subscriptions.insert_or_assign(regionHandle, active);
    }
    refreshRegionUsage(federation->second);
  } catch (...) {
    if (insertedState && state->second.publishedInteractionClasses.empty() &&
        state->second.subscribedInteractionClasses.empty() &&
        state->second.regionalSubscribedInteractionClasses.empty() &&
        state->second.publishedObjectClassDirectedInteractions.empty() &&
        state->second.subscribedObjectClassDirectedInteractions.empty()) {
      federation->second.interactionDeclarations.erase(state);
    }
    throw;
  }
  return RegionalInteractionClassDeclarationStatus::applied;
}

RegionalInteractionClassDeclarationStatus
EmbeddedFederationRegistry::removeInteractionClassRegionalSubscription(
    std::wstring const& federationName,
    std::uint64_t federateId,
    std::uint64_t interactionClassHandle,
    std::set<std::uint64_t> const& regionHandles) {
  std::scoped_lock lock(mutex_);
  auto federation = federations_.find(federationName);
  if (federation == federations_.end()) {
    return RegionalInteractionClassDeclarationStatus::federation_does_not_exist;
  }
  if (!federation->second.members.contains(federateId)) {
    return RegionalInteractionClassDeclarationStatus::federate_not_member;
  }
  if (!validInteractionClass(federation->second, interactionClassHandle)) {
    return RegionalInteractionClassDeclarationStatus::interaction_class_not_defined;
  }
  if (regionHandles.empty()) {
    return RegionalInteractionClassDeclarationStatus::applied;
  }

  for (std::uint64_t const regionHandle : regionHandles) {
    auto const region = federation->second.regions.find(regionHandle);
    if (region == federation->second.regions.end()) {
      return RegionalInteractionClassDeclarationStatus::invalid_region;
    }
    if (region->second.ownerFederateId != federateId) {
      return RegionalInteractionClassDeclarationStatus::region_not_created_by_this_federate;
    }
  }

  auto declarations = federation->second.interactionDeclarations.find(federateId);
  if (declarations != federation->second.interactionDeclarations.end()) {
    auto regional = declarations->second.regionalSubscribedInteractionClasses.find(
        interactionClassHandle);
    if (regional != declarations->second.regionalSubscribedInteractionClasses.end()) {
      for (std::uint64_t const regionHandle : regionHandles) {
        regional->second.erase(regionHandle);
      }
      if (regional->second.empty()) {
        declarations->second.regionalSubscribedInteractionClasses.erase(regional);
      }
    }
    if (declarations->second.publishedInteractionClasses.empty() &&
        declarations->second.subscribedInteractionClasses.empty() &&
        declarations->second.regionalSubscribedInteractionClasses.empty() &&
        declarations->second.publishedObjectClassDirectedInteractions.empty() &&
        declarations->second.subscribedObjectClassDirectedInteractions.empty()) {
      federation->second.interactionDeclarations.erase(declarations);
    }
  }
  refreshRegionUsage(federation->second);
  return RegionalInteractionClassDeclarationStatus::applied;
}

std::optional<InteractionClassDeclarationSnapshot>
EmbeddedFederationRegistry::interactionClassDeclarationFor(
    std::wstring const& federationName,
    std::uint64_t federateId,
    std::uint64_t interactionClassHandle) const {
  std::scoped_lock lock(mutex_);
  auto const federation = federations_.find(federationName);
  if (federation == federations_.end() ||
      !federation->second.members.contains(federateId) ||
      !validInteractionClass(federation->second, interactionClassHandle)) {
    return std::nullopt;
  }

  auto const declarations = federation->second.interactionDeclarations.find(federateId);
  if (declarations == federation->second.interactionDeclarations.end()) {
    return InteractionClassDeclarationSnapshot{};
  }
  auto const subscription = declarations->second.subscribedInteractionClasses.find(
      interactionClassHandle);
  return InteractionClassDeclarationSnapshot{
      declarations->second.publishedInteractionClasses.contains(interactionClassHandle),
      subscription == declarations->second.subscribedInteractionClasses.end()
          ? std::nullopt
          : std::optional<bool>{subscription->second},
  };
}

ObjectClassAttributeDeclarationStatus
EmbeddedFederationRegistry::setObjectClassAttributePublication(
    std::wstring const& federationName,
    std::uint64_t federateId,
    std::uint64_t objectClassHandle,
    std::set<std::uint64_t> const& attributeHandles,
    bool published) {
  std::scoped_lock lock(mutex_);
  auto federation = federations_.find(federationName);
  if (federation == federations_.end()) {
    return {ObjectClassAttributeDeclarationStatus::federation_does_not_exist};
  }
  if (!federation->second.members.contains(federateId)) {
    return {ObjectClassAttributeDeclarationStatus::federate_not_member};
  }
  if (!validObjectClass(federation->second, objectClassHandle)) {
    return {ObjectClassAttributeDeclarationStatus::object_class_not_defined};
  }
  if (!validObjectClassAttributes(federation->second, objectClassHandle, attributeHandles)) {
    return {ObjectClassAttributeDeclarationStatus::attribute_not_defined};
  }

  auto declarations = federation->second.objectClassAttributeDeclarations.find(federateId);
  if (!published) {
    if (declarations == federation->second.objectClassAttributeDeclarations.end()) {
      return ObjectClassAttributeDeclarationStatus::applied;
    }
    auto perClass = declarations->second.byObjectClass.find(objectClassHandle);
    if (perClass == declarations->second.byObjectClass.end()) {
      return ObjectClassAttributeDeclarationStatus::applied;
    }

    auto const currentlyPublished = publishedObjectClassAttributes(
        federation->second,
        federateId,
        objectClassHandle);
    if (!currentlyPublished) {
      return ObjectClassAttributeDeclarationStatus::inconsistent_catalog;
    }

    // IEEE 1516.1-2025 5.3.3(f) forbids removal of a publication needed by a
    // still-pending acquisition.  In this bounded profile both acquisition
    // forms are recorded against the requesting federate's known class, so
    // only an unpublication of that exact known class can remove their
    // publication precondition.  Check the whole call before changing any
    // declaration state, including implicit HLAprivilegeToDeleteObject.
    for (std::uint64_t const attributeHandle : attributeHandles) {
      if (!currentlyPublished->contains(attributeHandle)) {
        continue;
      }
      for (auto const& [objectInstanceHandle, objectInstance] :
           federation->second.objectInstances) {
        static_cast<void>(objectInstanceHandle);
        if (objectInstance.deleteAccepted) {
          continue;
        }
        auto const knownClass = objectInstance.knownObjectClassHandlesByFederate.find(federateId);
        if (knownClass == objectInstance.knownObjectClassHandlesByFederate.end() ||
            knownClass->second != objectClassHandle) {
          continue;
        }
        for (auto const& [notificationId, notification] :
             objectInstance.pendingConfirmDivestitureNotifications) {
          static_cast<void>(notificationId);
          if (notification.receivingFederateId == federateId &&
              notification.attributeHandles.contains(attributeHandle)) {
            return ObjectClassAttributeDeclarationStatus::ownership_acquisition_pending;
          }
        }
        // Divestiture If Wanted moves ownership synchronously, but the new
        // owner still has an Acquisition Notification outstanding. Its
        // reservation must therefore be checked before the normal owner fast
        // path below, which otherwise applies only after a callback boundary.
        for (auto const& [notificationId, notification] :
             objectInstance.pendingAttributeOwnershipDivestitureIfWantedNotifications) {
          static_cast<void>(notificationId);
          if (notification.receivingFederateId == federateId &&
              notification.attributeHandles.contains(attributeHandle)) {
            return ObjectClassAttributeDeclarationStatus::ownership_acquisition_pending;
          }
        }
        auto const owner = objectInstance.attributeOwnersByHandle.find(attributeHandle);
        if (owner != objectInstance.attributeOwnersByHandle.end() && owner->second == federateId) {
          continue;
        }
        for (auto const& [requestId, pending] :
             objectInstance.pendingAttributeOwnershipAcquisitionIfAvailableRequests) {
          static_cast<void>(requestId);
          if (pending.requestingFederateId == federateId &&
              pending.desiredAttributeHandles.contains(attributeHandle)) {
            return ObjectClassAttributeDeclarationStatus::ownership_acquisition_pending;
          }
        }
        for (auto const& [requestId, pending] :
             objectInstance.pendingAttributeOwnershipAcquisitionRequests) {
          static_cast<void>(requestId);
          if (pending.requestingFederateId == federateId &&
              pending.desiredAttributeHandles.contains(attributeHandle)) {
            return ObjectClassAttributeDeclarationStatus::ownership_acquisition_pending;
          }
        }
        for (auto const& [cancellationId, cancellation] :
             objectInstance.pendingAttributeOwnershipAcquisitionCancellations) {
          static_cast<void>(cancellationId);
          if (cancellation.requestingFederateId == federateId &&
              cancellation.attributeHandles.contains(attributeHandle)) {
            return ObjectClassAttributeDeclarationStatus::ownership_acquisition_pending;
          }
        }
      }
    }

    auto revised = perClass->second.explicitlyPublishedAttributes;
    bool privilegeToDeleteExplicitlyUnpublished =
        perClass->second.privilegeToDeleteExplicitlyUnpublished;
    auto const objectClassName = federation->second.objectClassHandles->nameFor(objectClassHandle);
    if (!objectClassName) {
      return ObjectClassAttributeDeclarationStatus::object_class_not_defined;
    }
    for (std::uint64_t const attributeHandle : attributeHandles) {
      revised.erase(attributeHandle);
      auto const attributeName = federation->second.attributeHandles->nameFor(
          federation->second.definition.catalog.get(),
          *objectClassName,
          attributeHandle);
      if (!attributeName) {
        return ObjectClassAttributeDeclarationStatus::attribute_not_defined;
      }
      if (*attributeName == "HLAprivilegeToDeleteObject") {
        privilegeToDeleteExplicitlyUnpublished = true;
      }
    }
    perClass->second.explicitlyPublishedAttributes.swap(revised);
    perClass->second.privilegeToDeleteExplicitlyUnpublished =
        privilegeToDeleteExplicitlyUnpublished;
    if (perClass->second.explicitlyPublishedAttributes.empty() &&
        perClass->second.subscribedAttributes.empty() &&
        perClass->second.regionalSubscribedAttributes.empty()) {
      declarations->second.byObjectClass.erase(perClass);
    }
    if (declarations->second.byObjectClass.empty()) {
      federation->second.objectClassAttributeDeclarations.erase(declarations);
    }
    return ObjectClassAttributeDeclarationStatus::applied;
  }

  auto [state, insertedState] =
      federation->second.objectClassAttributeDeclarations.try_emplace(federateId);
  auto [perClass, insertedClass] = state->second.byObjectClass.try_emplace(objectClassHandle);
  try {
    auto revised = perClass->second.explicitlyPublishedAttributes;
    bool const wasUnpublished = revised.empty();
    for (std::uint64_t const attributeHandle : attributeHandles) {
      static_cast<void>(revised.insert(attributeHandle));
    }
    bool privilegeToDeleteExplicitlyUnpublished =
        perClass->second.privilegeToDeleteExplicitlyUnpublished;
    if (wasUnpublished && !revised.empty()) {
      // A new publication establishment starts a fresh implicit-privilege
      // epoch. The limited registration slice snapshots this state.
      privilegeToDeleteExplicitlyUnpublished = false;
    }
    auto const objectClassName = federation->second.objectClassHandles->nameFor(objectClassHandle);
    if (!objectClassName) {
      return ObjectClassAttributeDeclarationStatus::object_class_not_defined;
    }
    for (std::uint64_t const attributeHandle : attributeHandles) {
      auto const attributeName = federation->second.attributeHandles->nameFor(
          federation->second.definition.catalog.get(),
          *objectClassName,
          attributeHandle);
      if (!attributeName) {
        return ObjectClassAttributeDeclarationStatus::attribute_not_defined;
      }
      if (*attributeName == "HLAprivilegeToDeleteObject") {
        privilegeToDeleteExplicitlyUnpublished = false;
      }
    }
    perClass->second.explicitlyPublishedAttributes.swap(revised);
    perClass->second.privilegeToDeleteExplicitlyUnpublished =
        privilegeToDeleteExplicitlyUnpublished;
  } catch (...) {
    if (insertedClass && perClass->second.explicitlyPublishedAttributes.empty() &&
        perClass->second.subscribedAttributes.empty() &&
        perClass->second.regionalSubscribedAttributes.empty()) {
      state->second.byObjectClass.erase(perClass);
    }
    if (insertedState && state->second.byObjectClass.empty()) {
      federation->second.objectClassAttributeDeclarations.erase(state);
    }
    throw;
  }
  return ObjectClassAttributeDeclarationStatus::applied;
}

ObjectClassAttributeSubscriptionScopePlan
EmbeddedFederationRegistry::setObjectClassAttributeSubscriptionWithScopeChanges(
    std::wstring const& federationName,
    std::uint64_t federateId,
    std::uint64_t objectClassHandle,
    std::set<std::uint64_t> const& attributeHandles,
    std::optional<bool> active) {
  std::scoped_lock lock(mutex_);
  auto federation = federations_.find(federationName);
  if (federation == federations_.end()) {
    return {ObjectClassAttributeDeclarationStatus::federation_does_not_exist};
  }
  if (!federation->second.members.contains(federateId)) {
    return {ObjectClassAttributeDeclarationStatus::federate_not_member};
  }
  if (!validObjectClass(federation->second, objectClassHandle)) {
    return {ObjectClassAttributeDeclarationStatus::object_class_not_defined};
  }
  if (!validObjectClassAttributes(federation->second, objectClassHandle, attributeHandles)) {
    return {ObjectClassAttributeDeclarationStatus::attribute_not_defined};
  }

  Federation::ObjectClassAttributeDeclarations previousDeclarations;
  auto const previous = federation->second.objectClassAttributeDeclarations.find(federateId);
  if (previous != federation->second.objectClassAttributeDeclarations.end()) {
    previousDeclarations = previous->second;
  }

  auto declarations = federation->second.objectClassAttributeDeclarations.find(federateId);
  if (!active) {
    if (declarations == federation->second.objectClassAttributeDeclarations.end()) {
      return {};
    }
    auto perClass = declarations->second.byObjectClass.find(objectClassHandle);
    if (perClass == declarations->second.byObjectClass.end()) {
      return {};
    }
    for (std::uint64_t const attributeHandle : attributeHandles) {
      perClass->second.subscribedAttributes.erase(attributeHandle);
    }
    if (perClass->second.explicitlyPublishedAttributes.empty() &&
        perClass->second.subscribedAttributes.empty() &&
        perClass->second.regionalSubscribedAttributes.empty()) {
      declarations->second.byObjectClass.erase(perClass);
    }
    if (declarations->second.byObjectClass.empty()) {
      federation->second.objectClassAttributeDeclarations.erase(declarations);
    }
    auto result = ObjectClassAttributeSubscriptionScopePlan{};
    result.recipients = objectInstanceScopeChangesForSubscription(
        federation->second,
        federateId,
        attributeHandles,
        previousDeclarations);
    return result;
  }

  auto [state, insertedState] =
      federation->second.objectClassAttributeDeclarations.try_emplace(federateId);
  auto [perClass, insertedClass] = state->second.byObjectClass.try_emplace(objectClassHandle);
  try {
    auto revised = perClass->second.subscribedAttributes;
    for (std::uint64_t const attributeHandle : attributeHandles) {
      revised.insert_or_assign(attributeHandle, *active);
    }
    perClass->second.subscribedAttributes.swap(revised);
  } catch (...) {
    if (insertedClass && perClass->second.explicitlyPublishedAttributes.empty() &&
        perClass->second.subscribedAttributes.empty() &&
        perClass->second.regionalSubscribedAttributes.empty()) {
      state->second.byObjectClass.erase(perClass);
    }
    if (insertedState && state->second.byObjectClass.empty()) {
      federation->second.objectClassAttributeDeclarations.erase(state);
    }
    throw;
  }
  auto result = ObjectClassAttributeSubscriptionScopePlan{};
  result.recipients = objectInstanceScopeChangesForSubscription(
      federation->second,
      federateId,
      attributeHandles,
      previousDeclarations);
  return result;
}

ObjectClassAttributeDeclarationStatus
EmbeddedFederationRegistry::setObjectClassAttributeSubscription(
    std::wstring const& federationName,
    std::uint64_t federateId,
    std::uint64_t objectClassHandle,
    std::set<std::uint64_t> const& attributeHandles,
    std::optional<bool> active) {
  return setObjectClassAttributeSubscriptionWithScopeChanges(
             federationName,
             federateId,
             objectClassHandle,
             attributeHandles,
             active)
      .status;
}

RegionalObjectClassAttributeSubscriptionScopePlan
EmbeddedFederationRegistry::setObjectClassAttributeRegionalSubscriptionWithScopeChanges(
    std::wstring const& federationName,
    std::uint64_t federateId,
    std::uint64_t objectClassHandle,
    std::map<std::uint64_t, std::set<std::uint64_t>> const& attributesAndRegions,
    bool active) {
  std::scoped_lock lock(mutex_);
  auto federation = federations_.find(federationName);
  if (federation == federations_.end()) {
    return {RegionalObjectClassAttributeDeclarationStatus::federation_does_not_exist};
  }
  if (!federation->second.members.contains(federateId)) {
    return {RegionalObjectClassAttributeDeclarationStatus::federate_not_member};
  }
  if (!validObjectClass(federation->second, objectClassHandle)) {
    return {RegionalObjectClassAttributeDeclarationStatus::object_class_not_defined};
  }

  std::set<std::uint64_t> attributeHandles;
  for (auto const& [attributeHandle, regions] : attributesAndRegions) {
    static_cast<void>(regions);
    attributeHandles.insert(attributeHandle);
  }
  if (!validObjectClassAttributes(
          federation->second,
          objectClassHandle,
          attributeHandles)) {
    return {RegionalObjectClassAttributeDeclarationStatus::attribute_not_defined};
  }
  if (attributesAndRegions.empty()) {
    return {};
  }

  auto const availableDimensions = availableObjectClassDimensions(
      federation->second,
      objectClassHandle);
  if (!availableDimensions) {
    return {RegionalObjectClassAttributeDeclarationStatus::inconsistent_catalog};
  }
  for (auto const& [attributeHandle, regionHandles] : attributesAndRegions) {
    static_cast<void>(attributeHandle);
    for (std::uint64_t const regionHandle : regionHandles) {
      auto const region = federation->second.regions.find(regionHandle);
      if (region == federation->second.regions.end()) {
        return {RegionalObjectClassAttributeDeclarationStatus::invalid_region};
      }
      if (region->second.ownerFederateId != federateId) {
        return {RegionalObjectClassAttributeDeclarationStatus::region_not_created_by_this_federate};
      }
      if (!region->second.specificationCommitted ||
          region->second.committedRangeBounds.size() != region->second.dimensionHandles.size()) {
        return {RegionalObjectClassAttributeDeclarationStatus::invalid_region};
      }
      if (!std::includes(
              availableDimensions->begin(),
              availableDimensions->end(),
              region->second.dimensionHandles.begin(),
              region->second.dimensionHandles.end())) {
        return {RegionalObjectClassAttributeDeclarationStatus::invalid_region_context};
      }
    }
  }

  Federation::ObjectClassAttributeDeclarations previousDeclarations;
  auto const previous = federation->second.objectClassAttributeDeclarations.find(federateId);
  if (previous != federation->second.objectClassAttributeDeclarations.end()) {
    previousDeclarations = previous->second;
  }

  auto [state, insertedState] =
      federation->second.objectClassAttributeDeclarations.try_emplace(federateId);
  auto [perClass, insertedClass] = state->second.byObjectClass.try_emplace(objectClassHandle);
  try {
    auto revised = perClass->second.regionalSubscribedAttributes;
    for (auto const& [attributeHandle, regionHandles] : attributesAndRegions) {
      auto& subscriptions = revised[attributeHandle];
      for (std::uint64_t const regionHandle : regionHandles) {
        subscriptions.insert_or_assign(regionHandle, active);
      }
      if (subscriptions.empty()) {
        revised.erase(attributeHandle);
      }
    }
    perClass->second.regionalSubscribedAttributes.swap(revised);
    refreshRegionUsage(federation->second);
  } catch (...) {
    if (insertedClass && perClass->second.explicitlyPublishedAttributes.empty() &&
        perClass->second.subscribedAttributes.empty() &&
        perClass->second.regionalSubscribedAttributes.empty()) {
      state->second.byObjectClass.erase(perClass);
    }
    if (insertedState && state->second.byObjectClass.empty()) {
      federation->second.objectClassAttributeDeclarations.erase(state);
    }
    throw;
  }
  auto result = RegionalObjectClassAttributeSubscriptionScopePlan{};
  result.recipients = objectInstanceScopeChangesForSubscription(
      federation->second,
      federateId,
      attributeHandles,
      previousDeclarations);
  return result;
}

RegionalObjectClassAttributeDeclarationStatus
EmbeddedFederationRegistry::setObjectClassAttributeRegionalSubscription(
    std::wstring const& federationName,
    std::uint64_t federateId,
    std::uint64_t objectClassHandle,
    std::map<std::uint64_t, std::set<std::uint64_t>> const& attributesAndRegions,
    bool active) {
  return setObjectClassAttributeRegionalSubscriptionWithScopeChanges(
             federationName,
             federateId,
             objectClassHandle,
             attributesAndRegions,
             active)
      .status;
}

RegionalObjectClassAttributeSubscriptionScopePlan
EmbeddedFederationRegistry::removeObjectClassAttributeRegionalSubscriptionWithScopeChanges(
    std::wstring const& federationName,
    std::uint64_t federateId,
    std::uint64_t objectClassHandle,
    std::map<std::uint64_t, std::set<std::uint64_t>> const& attributesAndRegions) {
  std::scoped_lock lock(mutex_);
  auto federation = federations_.find(federationName);
  if (federation == federations_.end()) {
    return {RegionalObjectClassAttributeDeclarationStatus::federation_does_not_exist};
  }
  if (!federation->second.members.contains(federateId)) {
    return {RegionalObjectClassAttributeDeclarationStatus::federate_not_member};
  }
  if (!validObjectClass(federation->second, objectClassHandle)) {
    return {RegionalObjectClassAttributeDeclarationStatus::object_class_not_defined};
  }

  std::set<std::uint64_t> attributeHandles;
  for (auto const& [attributeHandle, regions] : attributesAndRegions) {
    static_cast<void>(regions);
    attributeHandles.insert(attributeHandle);
  }
  if (!validObjectClassAttributes(
          federation->second,
          objectClassHandle,
          attributeHandles)) {
    return {RegionalObjectClassAttributeDeclarationStatus::attribute_not_defined};
  }
  for (auto const& [attributeHandle, regionHandles] : attributesAndRegions) {
    static_cast<void>(attributeHandle);
    for (std::uint64_t const regionHandle : regionHandles) {
      auto const region = federation->second.regions.find(regionHandle);
      if (region == federation->second.regions.end()) {
        return {RegionalObjectClassAttributeDeclarationStatus::invalid_region};
      }
      if (region->second.ownerFederateId != federateId) {
        return {RegionalObjectClassAttributeDeclarationStatus::region_not_created_by_this_federate};
      }
    }
  }

  Federation::ObjectClassAttributeDeclarations previousDeclarations;
  auto const previous = federation->second.objectClassAttributeDeclarations.find(federateId);
  if (previous != federation->second.objectClassAttributeDeclarations.end()) {
    previousDeclarations = previous->second;
  }

  auto declarations = federation->second.objectClassAttributeDeclarations.find(federateId);
  if (declarations != federation->second.objectClassAttributeDeclarations.end()) {
    auto perClass = declarations->second.byObjectClass.find(objectClassHandle);
    if (perClass != declarations->second.byObjectClass.end()) {
      for (auto const& [attributeHandle, regionHandles] : attributesAndRegions) {
        auto regional = perClass->second.regionalSubscribedAttributes.find(attributeHandle);
        if (regional == perClass->second.regionalSubscribedAttributes.end()) {
          continue;
        }
        for (std::uint64_t const regionHandle : regionHandles) {
          regional->second.erase(regionHandle);
        }
        if (regional->second.empty()) {
          perClass->second.regionalSubscribedAttributes.erase(regional);
        }
      }
      if (perClass->second.explicitlyPublishedAttributes.empty() &&
          perClass->second.subscribedAttributes.empty() &&
          perClass->second.regionalSubscribedAttributes.empty()) {
        declarations->second.byObjectClass.erase(perClass);
      }
    }
    if (declarations->second.byObjectClass.empty()) {
      federation->second.objectClassAttributeDeclarations.erase(declarations);
    }
  }
  refreshRegionUsage(federation->second);
  auto result = RegionalObjectClassAttributeSubscriptionScopePlan{};
  result.recipients = objectInstanceScopeChangesForSubscription(
      federation->second,
      federateId,
      attributeHandles,
      previousDeclarations);
  return result;
}

RegionalObjectClassAttributeDeclarationStatus
EmbeddedFederationRegistry::removeObjectClassAttributeRegionalSubscription(
    std::wstring const& federationName,
    std::uint64_t federateId,
    std::uint64_t objectClassHandle,
    std::map<std::uint64_t, std::set<std::uint64_t>> const& attributesAndRegions) {
  return removeObjectClassAttributeRegionalSubscriptionWithScopeChanges(
             federationName,
             federateId,
             objectClassHandle,
             attributesAndRegions)
      .status;
}

std::optional<ObjectClassAttributeDeclarationSnapshot>
EmbeddedFederationRegistry::objectClassAttributeDeclarationFor(
    std::wstring const& federationName,
    std::uint64_t federateId,
    std::uint64_t objectClassHandle) const {
  std::scoped_lock lock(mutex_);
  auto const federation = federations_.find(federationName);
  if (federation == federations_.end() ||
      !federation->second.members.contains(federateId) ||
      !validObjectClass(federation->second, objectClassHandle)) {
    return std::nullopt;
  }
  auto const declarations = federation->second.objectClassAttributeDeclarations.find(federateId);
  if (declarations == federation->second.objectClassAttributeDeclarations.end()) {
    return ObjectClassAttributeDeclarationSnapshot{};
  }
  auto const perClass = declarations->second.byObjectClass.find(objectClassHandle);
  if (perClass == declarations->second.byObjectClass.end()) {
    return ObjectClassAttributeDeclarationSnapshot{};
  }
  return ObjectClassAttributeDeclarationSnapshot{
      perClass->second.explicitlyPublishedAttributes,
      perClass->second.subscribedAttributes,
  };
}

ObjectInstanceNameReservationResult
EmbeddedFederationRegistry::reserveObjectInstanceName(
    std::wstring const& federationName,
    std::uint64_t federateId,
    std::wstring const& objectInstanceName) {
  std::scoped_lock lock(mutex_);
  ObjectInstanceNameReservationResult result;
  result.objectInstanceName = objectInstanceName;

  auto federation = federations_.find(federationName);
  if (federation == federations_.end()) {
    result.status = ObjectInstanceNameReservationStatus::federation_does_not_exist;
    return result;
  }
  if (!federation->second.members.contains(federateId)) {
    result.status = ObjectInstanceNameReservationStatus::federate_not_member;
    return result;
  }
  if (!objectInstanceNameIsLegal(objectInstanceName)) {
    result.status = ObjectInstanceNameReservationStatus::illegal_name;
    return result;
  }

  auto const callbackRoute = federation->second.interactionCallbackRoutes.find(federateId);
  if (callbackRoute == federation->second.interactionCallbackRoutes.end() ||
      !callbackRoute->second) {
    result.status = ObjectInstanceNameReservationStatus::callback_route_missing;
    return result;
  }
  result.callbackRoute = callbackRoute->second;

  if (federation->second.objectInstanceHandlesByName.contains(objectInstanceName) ||
      federation->second.reservedObjectInstanceNamesByFederate.contains(objectInstanceName)) {
    // Availability is reported asynchronously by the standard failure
    // callback; it is not an immediate ObjectInstanceNameInUse exception.
    return result;
  }

  auto const [reservation, inserted] =
      federation->second.reservedObjectInstanceNamesByFederate.emplace(
          objectInstanceName,
          federateId);
  static_cast<void>(reservation);
  if (!inserted) {
    return result;
  }
  result.succeeded = true;
  return result;
}

ObjectInstanceNameReservationStatus
EmbeddedFederationRegistry::releaseObjectInstanceName(
    std::wstring const& federationName,
    std::uint64_t federateId,
    std::wstring const& objectInstanceName) {
  std::scoped_lock lock(mutex_);
  auto federation = federations_.find(federationName);
  if (federation == federations_.end()) {
    return ObjectInstanceNameReservationStatus::federation_does_not_exist;
  }
  if (!federation->second.members.contains(federateId)) {
    return ObjectInstanceNameReservationStatus::federate_not_member;
  }

  auto const reservation = federation->second.reservedObjectInstanceNamesByFederate.find(
      objectInstanceName);
  if (reservation == federation->second.reservedObjectInstanceNamesByFederate.end() ||
      reservation->second != federateId) {
    return ObjectInstanceNameReservationStatus::object_instance_name_not_reserved;
  }
  federation->second.reservedObjectInstanceNamesByFederate.erase(reservation);
  return ObjectInstanceNameReservationStatus::applied;
}

MultipleObjectInstanceNameReservationResult
EmbeddedFederationRegistry::reserveMultipleObjectInstanceNames(
    std::wstring const& federationName,
    std::uint64_t federateId,
    std::set<std::wstring> const& objectInstanceNames) {
  std::scoped_lock lock(mutex_);
  MultipleObjectInstanceNameReservationResult result;

  auto federation = federations_.find(federationName);
  if (federation == federations_.end()) {
    result.status = ObjectInstanceNameReservationStatus::federation_does_not_exist;
    return result;
  }
  if (!federation->second.members.contains(federateId)) {
    result.status = ObjectInstanceNameReservationStatus::federate_not_member;
    return result;
  }
  if (objectInstanceNames.empty()) {
    result.status = ObjectInstanceNameReservationStatus::name_set_was_empty;
    return result;
  }
  for (auto const& objectInstanceName : objectInstanceNames) {
    if (!objectInstanceNameIsLegal(objectInstanceName)) {
      result.status = ObjectInstanceNameReservationStatus::illegal_name;
      return result;
    }
  }

  auto const callbackRoute = federation->second.interactionCallbackRoutes.find(federateId);
  if (callbackRoute == federation->second.interactionCallbackRoutes.end() ||
      !callbackRoute->second) {
    result.status = ObjectInstanceNameReservationStatus::callback_route_missing;
    return result;
  }
  result.callbackRoute = callbackRoute->second;

  // Multiple reservation is intentionally one atomic state transition for
  // the successful subset: no callback can observe an intermediate set.
  std::vector<std::wstring> insertedNames;
  try {
    for (auto const& objectInstanceName : objectInstanceNames) {
      if (federation->second.objectInstanceHandlesByName.contains(objectInstanceName) ||
          federation->second.reservedObjectInstanceNamesByFederate.contains(objectInstanceName)) {
        result.failedNames.insert(objectInstanceName);
        continue;
      }
      auto const [reservation, inserted] =
          federation->second.reservedObjectInstanceNamesByFederate.emplace(
              objectInstanceName,
              federateId);
      static_cast<void>(reservation);
      if (!inserted) {
        result.failedNames.insert(objectInstanceName);
        continue;
      }
      insertedNames.push_back(objectInstanceName);
      result.succeededNames.insert(objectInstanceName);
    }
  } catch (...) {
    for (auto const& objectInstanceName : insertedNames) {
      federation->second.reservedObjectInstanceNamesByFederate.erase(objectInstanceName);
    }
    throw;
  }
  return result;
}

ObjectInstanceNameReservationStatus
EmbeddedFederationRegistry::releaseMultipleObjectInstanceNames(
    std::wstring const& federationName,
    std::uint64_t federateId,
    std::set<std::wstring> const& objectInstanceNames) {
  std::scoped_lock lock(mutex_);
  auto federation = federations_.find(federationName);
  if (federation == federations_.end()) {
    return ObjectInstanceNameReservationStatus::federation_does_not_exist;
  }
  if (!federation->second.members.contains(federateId)) {
    return ObjectInstanceNameReservationStatus::federate_not_member;
  }

  // Validate the complete set before erasing anything. One unreserved name
  // therefore aborts the whole release, as required by Clause 6.7.
  for (auto const& objectInstanceName : objectInstanceNames) {
    auto const reservation = federation->second.reservedObjectInstanceNamesByFederate.find(
        objectInstanceName);
    if (reservation == federation->second.reservedObjectInstanceNamesByFederate.end() ||
        reservation->second != federateId) {
      return ObjectInstanceNameReservationStatus::object_instance_name_not_reserved;
    }
  }
  for (auto const& objectInstanceName : objectInstanceNames) {
    federation->second.reservedObjectInstanceNamesByFederate.erase(objectInstanceName);
  }
  return ObjectInstanceNameReservationStatus::applied;
}

ObjectInstanceRegistrationResult EmbeddedFederationRegistry::registerObjectInstance(
    std::wstring const& federationName,
    std::uint64_t federateId,
    std::uint64_t objectClassHandle,
    std::map<std::uint64_t, std::set<std::uint64_t>> const* updateRegionsByAttribute,
    std::wstring const* requestedObjectInstanceName) {
  std::scoped_lock lock(mutex_);
  auto federation = federations_.find(federationName);
  if (federation == federations_.end()) {
    return {ObjectInstanceRegistrationStatus::federation_does_not_exist};
  }
  if (!federation->second.members.contains(federateId)) {
    return {ObjectInstanceRegistrationStatus::federate_not_member};
  }
  if (!validObjectClass(federation->second, objectClassHandle)) {
    return {ObjectInstanceRegistrationStatus::object_class_not_defined};
  }

  auto publishedAttributes = publishedObjectClassAttributes(
      federation->second,
      federateId,
      objectClassHandle);
  if (!publishedAttributes) {
    return {ObjectInstanceRegistrationStatus::inconsistent_catalog};
  }
  if (publishedAttributes->empty()) {
    return {ObjectInstanceRegistrationStatus::object_class_not_published};
  }

  if (updateRegionsByAttribute != nullptr) {
    std::set<std::uint64_t> associatedAttributes;
    for (auto const& [attributeHandle, regions] : *updateRegionsByAttribute) {
      static_cast<void>(regions);
      associatedAttributes.insert(attributeHandle);
    }
    if (!validObjectClassAttributes(
            federation->second,
            objectClassHandle,
            associatedAttributes)) {
      return {ObjectInstanceRegistrationStatus::attribute_not_defined};
    }
    for (std::uint64_t const attributeHandle : associatedAttributes) {
      if (!publishedAttributes->contains(attributeHandle)) {
        return {ObjectInstanceRegistrationStatus::attribute_not_published};
      }
    }
    if (!associatedAttributes.empty()) {
      auto const availableDimensions = availableObjectClassDimensions(
          federation->second,
          objectClassHandle);
      if (!availableDimensions) {
        return {ObjectInstanceRegistrationStatus::inconsistent_catalog};
      }
      for (auto const& [attributeHandle, regionHandles] : *updateRegionsByAttribute) {
        static_cast<void>(attributeHandle);
        for (std::uint64_t const regionHandle : regionHandles) {
          auto const region = federation->second.regions.find(regionHandle);
          if (region == federation->second.regions.end()) {
            return {ObjectInstanceRegistrationStatus::invalid_region};
          }
          if (region->second.ownerFederateId != federateId) {
            return {ObjectInstanceRegistrationStatus::region_not_created_by_this_federate};
          }
          if (!region->second.specificationCommitted ||
              region->second.committedRangeBounds.size() != region->second.dimensionHandles.size()) {
            return {ObjectInstanceRegistrationStatus::invalid_region};
          }
          if (!std::includes(
                  availableDimensions->begin(),
                  availableDimensions->end(),
                  region->second.dimensionHandles.begin(),
                  region->second.dimensionHandles.end())) {
            return {ObjectInstanceRegistrationStatus::invalid_region_context};
          }
        }
      }
    }
  }

  // Named registration is admitted only for the reservation owner. Check
  // actual instance occupancy first so a stale reservation can never hide an
  // already registered name. The reservation is consumed only after both
  // object indexes have been committed below.
  if (requestedObjectInstanceName != nullptr) {
    auto const& requestedName = *requestedObjectInstanceName;
    if (federation->second.objectInstanceHandlesByName.contains(requestedName)) {
      return {ObjectInstanceRegistrationStatus::object_instance_name_in_use};
    }
    auto const reservation =
        federation->second.reservedObjectInstanceNamesByFederate.find(requestedName);
    if (reservation == federation->second.reservedObjectInstanceNamesByFederate.end() ||
        reservation->second != federateId) {
      return {ObjectInstanceRegistrationStatus::object_instance_name_not_reserved};
    }
  }

  std::uint64_t objectInstanceHandle = federation->second.nextObjectInstanceHandle;
  std::wstring objectInstanceName = requestedObjectInstanceName == nullptr
      ? std::wstring{}
      : *requestedObjectInstanceName;
  if (requestedObjectInstanceName == nullptr) {
    do {
      if (objectInstanceHandle == 0 ||
          objectInstanceHandle == std::numeric_limits<std::uint64_t>::max()) {
        return {ObjectInstanceRegistrationStatus::object_instance_handle_exhausted};
      }
      objectInstanceName =
          L"UmbraObjectInstance-" + std::to_wstring(objectInstanceHandle);
      if (!federation->second.objectInstances.contains(objectInstanceHandle) &&
          !federation->second.objectInstanceHandlesByName.contains(objectInstanceName) &&
          !federation->second.reservedObjectInstanceNamesByFederate.contains(objectInstanceName)) {
        break;
      }
      ++objectInstanceHandle;
    } while (true);
  } else {
    while (federation->second.objectInstances.contains(objectInstanceHandle)) {
      if (objectInstanceHandle == 0 ||
          objectInstanceHandle == std::numeric_limits<std::uint64_t>::max()) {
        return {ObjectInstanceRegistrationStatus::object_instance_handle_exhausted};
      }
      ++objectInstanceHandle;
    }
    if (objectInstanceHandle == 0 ||
        objectInstanceHandle == std::numeric_limits<std::uint64_t>::max()) {
      return {ObjectInstanceRegistrationStatus::object_instance_handle_exhausted};
    }
  }

  Federation::ObjectInstance objectInstance;
  objectInstance.handle = objectInstanceHandle;
  objectInstance.name = objectInstanceName;
  objectInstance.registeredObjectClassHandle = objectClassHandle;
  objectInstance.producingFederateId = federateId;
  for (std::uint64_t const attributeHandle : *publishedAttributes) {
    auto const [attributeOwner, insertedAttributeOwner] =
        objectInstance.attributeOwnersByHandle.emplace(attributeHandle, federateId);
    static_cast<void>(attributeOwner);
    if (!insertedAttributeOwner) {
      return {ObjectInstanceRegistrationStatus::inconsistent_catalog};
    }
  }
  if (updateRegionsByAttribute != nullptr) {
    for (auto const& [attributeHandle, regionHandles] : *updateRegionsByAttribute) {
      if (!regionHandles.empty()) {
        objectInstance.updateRegionsByAttribute.insert_or_assign(
            attributeHandle,
            regionHandles);
      }
    }
  }
  auto const [knownClass, insertedKnownClass] =
      objectInstance.knownObjectClassHandlesByFederate.emplace(federateId, objectClassHandle);
  static_cast<void>(knownClass);
  if (!insertedKnownClass) {
    return {ObjectInstanceRegistrationStatus::inconsistent_catalog};
  }

  auto [instancePosition, insertedInstance] = federation->second.objectInstances.emplace(
      objectInstanceHandle,
      std::move(objectInstance));
  if (!insertedInstance) {
    return {ObjectInstanceRegistrationStatus::inconsistent_catalog};
  }
  try {
    auto [namePosition, insertedName] = federation->second.objectInstanceHandlesByName.emplace(
        objectInstanceName,
        objectInstanceHandle);
    static_cast<void>(namePosition);
    if (!insertedName) {
      federation->second.objectInstances.erase(instancePosition);
      return {ObjectInstanceRegistrationStatus::inconsistent_catalog};
    }
  } catch (...) {
    federation->second.objectInstances.erase(instancePosition);
    throw;
  }

  if (requestedObjectInstanceName != nullptr &&
      federation->second.reservedObjectInstanceNamesByFederate.erase(objectInstanceName) != 1) {
    federation->second.objectInstanceHandlesByName.erase(objectInstanceName);
    federation->second.objectInstances.erase(instancePosition);
    return {ObjectInstanceRegistrationStatus::inconsistent_catalog};
  }

  federation->second.nextObjectInstanceHandle = objectInstanceHandle + 1;
  refreshRegionUsage(federation->second);
  return {
      ObjectInstanceRegistrationStatus::applied,
      objectInstanceHandle,
      objectInstanceName,
  };
}

ObjectInstanceRegionAssociationScopePlan
EmbeddedFederationRegistry::associateRegionsForUpdatesWithScopeChanges(
    std::wstring const& federationName,
    std::uint64_t federateId,
    std::uint64_t objectInstanceHandle,
    std::map<std::uint64_t, std::set<std::uint64_t>> const& attributesAndRegions) {
  std::scoped_lock lock(mutex_);
  auto federation = federations_.find(federationName);
  if (federation == federations_.end()) {
    return {ObjectInstanceRegionAssociationStatus::federation_does_not_exist};
  }
  if (!federation->second.members.contains(federateId)) {
    return {ObjectInstanceRegionAssociationStatus::federate_not_member};
  }
  auto instance = federation->second.objectInstances.find(objectInstanceHandle);
  if (instance == federation->second.objectInstances.end() ||
      instance->second.deleteAccepted ||
      !instance->second.knownObjectClassHandlesByFederate.contains(federateId)) {
    return {ObjectInstanceRegionAssociationStatus::object_instance_not_known};
  }
  std::set<std::uint64_t> attributeHandles;
  for (auto const& [attributeHandle, regions] : attributesAndRegions) {
    static_cast<void>(regions);
    attributeHandles.insert(attributeHandle);
  }
  if (!validObjectClassAttributes(
          federation->second,
          instance->second.registeredObjectClassHandle,
          attributeHandles)) {
    return {ObjectInstanceRegionAssociationStatus::attribute_not_defined};
  }
  if (attributesAndRegions.empty()) {
    return {};
  }
  auto const availableDimensions = availableObjectClassDimensions(
      federation->second,
      instance->second.registeredObjectClassHandle);
  if (!availableDimensions) {
    return {ObjectInstanceRegionAssociationStatus::inconsistent_catalog};
  }
  for (auto const& [attributeHandle, regionHandles] : attributesAndRegions) {
    static_cast<void>(attributeHandle);
    for (std::uint64_t const regionHandle : regionHandles) {
      auto const region = federation->second.regions.find(regionHandle);
      if (region == federation->second.regions.end()) {
        return {ObjectInstanceRegionAssociationStatus::invalid_region};
      }
      if (region->second.ownerFederateId != federateId) {
        return {ObjectInstanceRegionAssociationStatus::region_not_created_by_this_federate};
      }
      if (!region->second.specificationCommitted ||
          region->second.committedRangeBounds.size() != region->second.dimensionHandles.size()) {
        return {ObjectInstanceRegionAssociationStatus::invalid_region};
      }
      if (!std::includes(
              availableDimensions->begin(),
              availableDimensions->end(),
              region->second.dimensionHandles.begin(),
              region->second.dimensionHandles.end())) {
        return {ObjectInstanceRegionAssociationStatus::invalid_region_context};
      }
    }
  }
  auto revisedAssociations = instance->second.updateRegionsByAttribute;
  for (auto const& [attributeHandle, regionHandles] : attributesAndRegions) {
    auto& associated = revisedAssociations[attributeHandle];
    associated.insert(regionHandles.begin(), regionHandles.end());
    if (associated.empty()) {
      revisedAssociations.erase(attributeHandle);
    }
  }
  auto result = ObjectInstanceRegionAssociationScopePlan{};
  result.recipients = objectInstanceScopeChangesForAssociation(
      federation->second,
      instance->second,
      attributeHandles,
      revisedAssociations);
  instance->second.updateRegionsByAttribute.swap(revisedAssociations);
  refreshRegionUsage(federation->second);
  return result;
}

ObjectInstanceRegionAssociationStatus EmbeddedFederationRegistry::associateRegionsForUpdates(
    std::wstring const& federationName,
    std::uint64_t federateId,
    std::uint64_t objectInstanceHandle,
    std::map<std::uint64_t, std::set<std::uint64_t>> const& attributesAndRegions) {
  return associateRegionsForUpdatesWithScopeChanges(
             federationName,
             federateId,
             objectInstanceHandle,
             attributesAndRegions)
      .status;
}

ObjectInstanceRegionAssociationScopePlan
EmbeddedFederationRegistry::unassociateRegionsForUpdatesWithScopeChanges(
    std::wstring const& federationName,
    std::uint64_t federateId,
    std::uint64_t objectInstanceHandle,
    std::map<std::uint64_t, std::set<std::uint64_t>> const& attributesAndRegions) {
  std::scoped_lock lock(mutex_);
  auto federation = federations_.find(federationName);
  if (federation == federations_.end()) {
    return {ObjectInstanceRegionAssociationStatus::federation_does_not_exist};
  }
  if (!federation->second.members.contains(federateId)) {
    return {ObjectInstanceRegionAssociationStatus::federate_not_member};
  }
  auto instance = federation->second.objectInstances.find(objectInstanceHandle);
  if (instance == federation->second.objectInstances.end() ||
      instance->second.deleteAccepted ||
      !instance->second.knownObjectClassHandlesByFederate.contains(federateId)) {
    return {ObjectInstanceRegionAssociationStatus::object_instance_not_known};
  }
  std::set<std::uint64_t> attributeHandles;
  for (auto const& [attributeHandle, regions] : attributesAndRegions) {
    static_cast<void>(regions);
    attributeHandles.insert(attributeHandle);
  }
  if (!validObjectClassAttributes(
          federation->second,
          instance->second.registeredObjectClassHandle,
          attributeHandles)) {
    return {ObjectInstanceRegionAssociationStatus::attribute_not_defined};
  }
  auto revisedAssociations = instance->second.updateRegionsByAttribute;
  for (auto const& [attributeHandle, regionHandles] : attributesAndRegions) {
    static_cast<void>(attributeHandle);
    for (std::uint64_t const regionHandle : regionHandles) {
      auto const region = federation->second.regions.find(regionHandle);
      if (region == federation->second.regions.end()) {
        return {ObjectInstanceRegionAssociationStatus::invalid_region};
      }
      if (region->second.ownerFederateId != federateId) {
        return {ObjectInstanceRegionAssociationStatus::region_not_created_by_this_federate};
      }
    }
  }
  for (auto const& [attributeHandle, regionHandles] : attributesAndRegions) {
    auto associated = revisedAssociations.find(attributeHandle);
    if (associated == revisedAssociations.end()) {
      continue;
    }
    for (std::uint64_t const regionHandle : regionHandles) {
      associated->second.erase(regionHandle);
    }
    if (associated->second.empty()) {
      revisedAssociations.erase(associated);
    }
  }
  auto result = ObjectInstanceRegionAssociationScopePlan{};
  result.recipients = objectInstanceScopeChangesForAssociation(
      federation->second,
      instance->second,
      attributeHandles,
      revisedAssociations);
  instance->second.updateRegionsByAttribute.swap(revisedAssociations);
  refreshRegionUsage(federation->second);
  return result;
}

ObjectInstanceRegionAssociationStatus EmbeddedFederationRegistry::unassociateRegionsForUpdates(
    std::wstring const& federationName,
    std::uint64_t federateId,
    std::uint64_t objectInstanceHandle,
    std::map<std::uint64_t, std::set<std::uint64_t>> const& attributesAndRegions) {
  return unassociateRegionsForUpdatesWithScopeChanges(
             federationName,
             federateId,
             objectInstanceHandle,
             attributesAndRegions)
      .status;
}

ObjectInstanceDeletionPlan EmbeddedFederationRegistry::deleteObjectInstance(
    std::wstring const& federationName,
    std::uint64_t deletingFederateId,
    std::uint64_t objectInstanceHandle) {
  std::scoped_lock lock(mutex_);
  auto federation = federations_.find(federationName);
  if (federation == federations_.end()) {
    return {ObjectInstanceDeletionStatus::federation_does_not_exist};
  }
  if (!federation->second.members.contains(deletingFederateId)) {
    return {ObjectInstanceDeletionStatus::federate_not_member};
  }
  auto instance = federation->second.objectInstances.find(objectInstanceHandle);
  if (instance == federation->second.objectInstances.end() ||
      instance->second.deleteAccepted ||
      instance->second.pendingTimestampedDeletionMessageId.has_value() ||
      !instance->second.knownObjectClassHandlesByFederate.contains(deletingFederateId)) {
    return {ObjectInstanceDeletionStatus::object_instance_not_known};
  }
  if (!federation->second.definition.catalog ||
      !federation->second.objectClassHandles ||
      !federation->second.attributeHandles) {
    return {ObjectInstanceDeletionStatus::inconsistent_catalog};
  }

  auto const objectClassName = federation->second.objectClassHandles->nameFor(
      instance->second.registeredObjectClassHandle);
  if (!objectClassName) {
    return {ObjectInstanceDeletionStatus::inconsistent_catalog};
  }
  auto const privilegeToDelete = federation->second.attributeHandles->handleFor(
      federation->second.definition.catalog.get(),
      *objectClassName,
      "HLAprivilegeToDeleteObject");
  if (!privilegeToDelete) {
    return {ObjectInstanceDeletionStatus::inconsistent_catalog};
  }
  auto const privilegeOwner = instance->second.attributeOwnersByHandle.find(*privilegeToDelete);
  if (privilegeOwner == instance->second.attributeOwnersByHandle.end() ||
      privilegeOwner->second != deletingFederateId) {
    return {ObjectInstanceDeletionStatus::delete_privilege_not_held};
  }

  ObjectInstanceDeletionPlan result;
  result.recipients.reserve(instance->second.knownObjectClassHandlesByFederate.size());
  for (auto const& [receivingFederateId, knownObjectClassHandle] :
       instance->second.knownObjectClassHandlesByFederate) {
    static_cast<void>(knownObjectClassHandle);
    if (receivingFederateId == deletingFederateId ||
        !federation->second.members.contains(receivingFederateId)) {
      continue;
    }
    auto const callbackRoute = federation->second.interactionCallbackRoutes.find(receivingFederateId);
    if (callbackRoute == federation->second.interactionCallbackRoutes.end() ||
        !callbackRoute->second) {
      continue;
    }
    result.recipients.push_back({
        receivingFederateId,
        instance->second.handle,
        callbackRoute->second,
    });
  }

  try {
    for (auto const& recipient : result.recipients) {
      auto const [pending, insertedPending] =
          instance->second.pendingRemovalFederates.insert(recipient.receivingFederateId);
      static_cast<void>(pending);
      if (!insertedPending) {
        for (auto const& reserved : result.recipients) {
          instance->second.pendingRemovalFederates.erase(reserved.receivingFederateId);
        }
        return {ObjectInstanceDeletionStatus::inconsistent_catalog};
      }
    }
  } catch (...) {
    for (auto const& recipient : result.recipients) {
      instance->second.pendingRemovalFederates.erase(recipient.receivingFederateId);
    }
    throw;
  }

  // Delete is no longer eligible to induce discovery. The deleting federate
  // becomes unknown immediately; other known federates remain known only
  // until beginObjectInstanceRemoval commits their queued callback.
  instance->second.pendingDiscoveryFederates.clear();
  // A receive-order deletion invalidates every pending ownership-acquisition
  // callback before it can make a deleted object appear to change owner.
  instance->second.pendingAttributeOwnershipAcquisitionIfAvailableRequests.clear();
  instance->second.pendingAttributeOwnershipAcquisitionRequests.clear();
  instance->second.pendingAttributeOwnershipAcquisitionCancellations.clear();
  instance->second.pendingAttributeOwnershipDivestitureIfWantedNotifications.clear();
  instance->second.pendingNegotiatedAttributeOwnershipDivestitures.clear();
  instance->second.pendingConfirmDivestitureNotifications.clear();
  instance->second.knownObjectClassHandlesByFederate.erase(deletingFederateId);
  instance->second.deleteAccepted = true;
  if (canPurgeDeletedObjectInstance(instance->second)) {
    federation->second.objectInstanceHandlesByName.erase(instance->second.name);
    federation->second.objectInstances.erase(instance);
  }
  return result;
}

LocalObjectInstanceDeletionStatus EmbeddedFederationRegistry::localDeleteObjectInstance(
    std::wstring const& federationName,
    std::uint64_t deletingFederateId,
    std::uint64_t objectInstanceHandle) {
  std::scoped_lock lock(mutex_);
  auto federation = federations_.find(federationName);
  if (federation == federations_.end()) {
    return LocalObjectInstanceDeletionStatus::federation_does_not_exist;
  }
  if (!federation->second.members.contains(deletingFederateId)) {
    return LocalObjectInstanceDeletionStatus::federate_not_member;
  }

  auto instance = federation->second.objectInstances.find(objectInstanceHandle);
  if (instance == federation->second.objectInstances.end() ||
      instance->second.deleteAccepted ||
      !instance->second.knownObjectClassHandlesByFederate.contains(deletingFederateId)) {
    return LocalObjectInstanceDeletionStatus::object_instance_not_known;
  }

  // Local deletion is not allowed to discard an in-flight ownership request.
  // Check every private acquisition/divestiture reservation that can still
  // change ownership or deliver an ownership callback for this federate.
  for (auto const& [requestId, request] :
       instance->second.pendingAttributeOwnershipAcquisitionIfAvailableRequests) {
    static_cast<void>(requestId);
    if (request.requestingFederateId == deletingFederateId) {
      return LocalObjectInstanceDeletionStatus::ownership_acquisition_pending;
    }
  }
  for (auto const& [requestId, request] :
       instance->second.pendingAttributeOwnershipAcquisitionRequests) {
    static_cast<void>(requestId);
    if (request.requestingFederateId == deletingFederateId) {
      return LocalObjectInstanceDeletionStatus::ownership_acquisition_pending;
    }
  }
  for (auto const& [cancellationId, cancellation] :
       instance->second.pendingAttributeOwnershipAcquisitionCancellations) {
    static_cast<void>(cancellationId);
    if (cancellation.requestingFederateId == deletingFederateId) {
      return LocalObjectInstanceDeletionStatus::ownership_acquisition_pending;
    }
  }
  for (auto const& [notificationId, notification] :
       instance->second.pendingAttributeOwnershipDivestitureIfWantedNotifications) {
    static_cast<void>(notificationId);
    if (notification.receivingFederateId == deletingFederateId) {
      return LocalObjectInstanceDeletionStatus::ownership_acquisition_pending;
    }
  }
  for (auto const& [requestId, divestiture] :
       instance->second.pendingNegotiatedAttributeOwnershipDivestitures) {
    static_cast<void>(requestId);
    if (divestiture.divestingFederateId == deletingFederateId ||
        divestiture.acquiringFederateId == deletingFederateId) {
      return LocalObjectInstanceDeletionStatus::ownership_acquisition_pending;
    }
  }
  for (auto const& [notificationId, notification] :
       instance->second.pendingConfirmDivestitureNotifications) {
    static_cast<void>(notificationId);
    if (notification.receivingFederateId == deletingFederateId) {
      return LocalObjectInstanceDeletionStatus::ownership_acquisition_pending;
    }
  }

  for (auto const& [attributeHandle, owningFederateId] : instance->second.attributeOwnersByHandle) {
    static_cast<void>(attributeHandle);
    if (owningFederateId == deletingFederateId) {
      return LocalObjectInstanceDeletionStatus::federate_owns_attributes;
    }
  }

  // Keep the execution-wide instance and every other federate's knowledge
  // untouched. A later eligible subscription can plan a fresh discovery.
  instance->second.knownObjectClassHandlesByFederate.erase(deletingFederateId);
  instance->second.pendingDiscoveryFederates.erase(deletingFederateId);
  return LocalObjectInstanceDeletionStatus::applied;
}

std::vector<ObjectInstanceDiscoveryRecipient>
EmbeddedFederationRegistry::planObjectInstanceDiscoveriesForInstance(
    std::wstring const& federationName,
    std::uint64_t objectInstanceHandle) {
  std::scoped_lock lock(mutex_);
  auto federation = federations_.find(federationName);
  if (federation == federations_.end()) {
    return {};
  }
  auto instance = federation->second.objectInstances.find(objectInstanceHandle);
  if (instance == federation->second.objectInstances.end() || instance->second.deleteAccepted) {
    return {};
  }

  std::vector<ObjectInstanceDiscoveryRecipient> result;
  result.reserve(federation->second.members.size());
  try {
    for (auto const& [receivingFederateId, membership] : federation->second.members) {
      static_cast<void>(membership);
      if (instance->second.knownObjectClassHandlesByFederate.contains(receivingFederateId) ||
          instance->second.pendingDiscoveryFederates.contains(receivingFederateId)) {
        continue;
      }
      auto const discoveredClass = candidateObjectInstanceDiscoveryClass(
          federation->second,
          instance->second,
          receivingFederateId);
      if (!discoveredClass) {
        continue;
      }
      auto const callbackRoute = federation->second.interactionCallbackRoutes.find(receivingFederateId);
      if (callbackRoute == federation->second.interactionCallbackRoutes.end() ||
          !callbackRoute->second) {
        continue;
      }

      auto const [pending, insertedPending] =
          instance->second.pendingDiscoveryFederates.insert(receivingFederateId);
      static_cast<void>(pending);
      if (!insertedPending) {
        continue;
      }
      try {
        result.push_back({
            receivingFederateId,
            instance->second.handle,
            *discoveredClass,
            instance->second.name,
            instance->second.producingFederateId,
            callbackRoute->second,
        });
      } catch (...) {
        instance->second.pendingDiscoveryFederates.erase(receivingFederateId);
        throw;
      }
    }
  } catch (...) {
    for (auto const& recipient : result) {
      instance->second.pendingDiscoveryFederates.erase(recipient.receivingFederateId);
    }
    throw;
  }
  return result;
}

std::vector<ObjectInstanceDiscoveryRecipient>
EmbeddedFederationRegistry::planObjectInstanceDiscoveriesForFederate(
    std::wstring const& federationName,
    std::uint64_t receivingFederateId) {
  std::scoped_lock lock(mutex_);
  auto federation = federations_.find(federationName);
  if (federation == federations_.end() ||
      !federation->second.members.contains(receivingFederateId)) {
    return {};
  }
  auto const callbackRoute = federation->second.interactionCallbackRoutes.find(receivingFederateId);
  if (callbackRoute == federation->second.interactionCallbackRoutes.end() ||
      !callbackRoute->second) {
    return {};
  }

  std::vector<ObjectInstanceDiscoveryRecipient> result;
  result.reserve(federation->second.objectInstances.size());
  try {
    for (auto& [objectInstanceHandle, objectInstance] : federation->second.objectInstances) {
      static_cast<void>(objectInstanceHandle);
      if (objectInstance.deleteAccepted ||
          objectInstance.knownObjectClassHandlesByFederate.contains(receivingFederateId) ||
          objectInstance.pendingDiscoveryFederates.contains(receivingFederateId)) {
        continue;
      }
      auto const discoveredClass = candidateObjectInstanceDiscoveryClass(
          federation->second,
          objectInstance,
          receivingFederateId);
      if (!discoveredClass) {
        continue;
      }

      auto const [pending, insertedPending] =
          objectInstance.pendingDiscoveryFederates.insert(receivingFederateId);
      static_cast<void>(pending);
      if (!insertedPending) {
        continue;
      }
      try {
        result.push_back({
            receivingFederateId,
            objectInstance.handle,
            *discoveredClass,
            objectInstance.name,
            objectInstance.producingFederateId,
            callbackRoute->second,
        });
      } catch (...) {
        objectInstance.pendingDiscoveryFederates.erase(receivingFederateId);
        throw;
      }
    }
  } catch (...) {
    for (auto const& recipient : result) {
      auto const instance = federation->second.objectInstances.find(
          recipient.objectInstanceHandle);
      if (instance != federation->second.objectInstances.end()) {
        instance->second.pendingDiscoveryFederates.erase(receivingFederateId);
      }
    }
    throw;
  }
  return result;
}

void EmbeddedFederationRegistry::cancelObjectInstanceDiscovery(
    std::wstring const& federationName,
    std::uint64_t receivingFederateId,
    std::uint64_t objectInstanceHandle) {
  std::scoped_lock lock(mutex_);
  auto federation = federations_.find(federationName);
  if (federation == federations_.end()) {
    return;
  }
  auto instance = federation->second.objectInstances.find(objectInstanceHandle);
  if (instance == federation->second.objectInstances.end()) {
    return;
  }
  instance->second.pendingDiscoveryFederates.erase(receivingFederateId);
}

std::set<std::uint64_t> EmbeddedFederationRegistry::objectInstanceScopeAttributes(
    std::wstring const& federationName,
    std::uint64_t receivingFederateId,
    std::uint64_t objectInstanceHandle,
    std::set<std::uint64_t> const& attributeHandles,
    bool expectedInScope) const {
  std::scoped_lock lock(mutex_);
  auto const federation = federations_.find(federationName);
  if (federation == federations_.end() ||
      !federation->second.members.contains(receivingFederateId)) {
    return {};
  }
  auto const instance = federation->second.objectInstances.find(objectInstanceHandle);
  if (instance == federation->second.objectInstances.end() ||
      instance->second.deleteAccepted ||
      !instance->second.knownObjectClassHandlesByFederate.contains(receivingFederateId)) {
    return {};
  }

  std::set<std::uint64_t> result;
  for (std::uint64_t const attributeHandle : attributeHandles) {
    if (objectAttributeInScope(
            federation->second,
            instance->second,
            receivingFederateId,
            attributeHandle) == expectedInScope) {
      result.insert(attributeHandle);
    }
  }
  return result;
}

std::optional<RemovedObjectInstanceSnapshot>
EmbeddedFederationRegistry::beginObjectInstanceRemoval(
    std::wstring const& federationName,
    std::uint64_t receivingFederateId,
    std::uint64_t objectInstanceHandle) {
  std::scoped_lock lock(mutex_);
  auto federation = federations_.find(federationName);
  if (federation == federations_.end()) {
    return std::nullopt;
  }
  auto instance = federation->second.objectInstances.find(objectInstanceHandle);
  if (instance == federation->second.objectInstances.end() || !instance->second.deleteAccepted) {
    return std::nullopt;
  }

  instance->second.pendingRemovalFederates.erase(receivingFederateId);
  auto const known = instance->second.knownObjectClassHandlesByFederate.find(receivingFederateId);
  if (!federation->second.members.contains(receivingFederateId) ||
      known == instance->second.knownObjectClassHandlesByFederate.end()) {
    if (canPurgeDeletedObjectInstance(instance->second)) {
      federation->second.objectInstanceHandlesByName.erase(instance->second.name);
      federation->second.objectInstances.erase(instance);
    }
    return std::nullopt;
  }

  RemovedObjectInstanceSnapshot const result{
      instance->second.handle,
      instance->second.producingFederateId,
  };
  instance->second.knownObjectClassHandlesByFederate.erase(known);
  if (canPurgeDeletedObjectInstance(instance->second)) {
    federation->second.objectInstanceHandlesByName.erase(instance->second.name);
    federation->second.objectInstances.erase(instance);
  }
  return result;
}

void EmbeddedFederationRegistry::cancelObjectInstanceRemoval(
    std::wstring const& federationName,
    std::uint64_t receivingFederateId,
    std::uint64_t objectInstanceHandle) {
  std::scoped_lock lock(mutex_);
  auto federation = federations_.find(federationName);
  if (federation == federations_.end()) {
    return;
  }
  auto instance = federation->second.objectInstances.find(objectInstanceHandle);
  if (instance == federation->second.objectInstances.end()) {
    return;
  }
  instance->second.pendingRemovalFederates.erase(receivingFederateId);
  if (canPurgeDeletedObjectInstance(instance->second)) {
    federation->second.objectInstanceHandlesByName.erase(instance->second.name);
    federation->second.objectInstances.erase(instance);
  }
}

std::optional<KnownObjectInstanceSnapshot>
EmbeddedFederationRegistry::beginObjectInstanceDiscovery(
    std::wstring const& federationName,
    std::uint64_t receivingFederateId,
    std::uint64_t objectInstanceHandle) {
  std::scoped_lock lock(mutex_);
  auto federation = federations_.find(federationName);
  if (federation == federations_.end()) {
    return std::nullopt;
  }
  auto instance = federation->second.objectInstances.find(objectInstanceHandle);
  if (instance == federation->second.objectInstances.end()) {
    return std::nullopt;
  }

  // A route is single-use. Releasing the reservation before every recheck
  // permits a later declaration change to plan a fresh callback if this one
  // has become ineligible.
  instance->second.pendingDiscoveryFederates.erase(receivingFederateId);
  if (instance->second.deleteAccepted ||
      !federation->second.members.contains(receivingFederateId) ||
      instance->second.knownObjectClassHandlesByFederate.contains(receivingFederateId)) {
    return std::nullopt;
  }
  auto const discoveredClass = candidateObjectInstanceDiscoveryClass(
      federation->second,
      instance->second,
      receivingFederateId);
  if (!discoveredClass) {
    return std::nullopt;
  }

  auto const [knownClass, insertedKnownClass] =
      instance->second.knownObjectClassHandlesByFederate.emplace(
          receivingFederateId,
          *discoveredClass);
  static_cast<void>(knownClass);
  if (!insertedKnownClass) {
    return std::nullopt;
  }
  return knownObjectInstanceSnapshot(federation->second, instance->second, receivingFederateId);
}

std::optional<KnownObjectInstanceSnapshot>
EmbeddedFederationRegistry::knownObjectInstanceFor(
    std::wstring const& federationName,
    std::uint64_t federateId,
    std::uint64_t objectInstanceHandle) const {
  std::scoped_lock lock(mutex_);
  auto const federation = federations_.find(federationName);
  if (federation == federations_.end() ||
      !federation->second.members.contains(federateId)) {
    return std::nullopt;
  }
  auto const instance = federation->second.objectInstances.find(objectInstanceHandle);
  if (instance == federation->second.objectInstances.end()) {
    return std::nullopt;
  }
  return knownObjectInstanceSnapshot(federation->second, instance->second, federateId);
}

std::optional<KnownObjectInstanceSnapshot>
EmbeddedFederationRegistry::knownObjectInstanceByNameFor(
    std::wstring const& federationName,
    std::uint64_t federateId,
    std::wstring const& objectInstanceName) const {
  std::scoped_lock lock(mutex_);
  auto const federation = federations_.find(federationName);
  if (federation == federations_.end() ||
      !federation->second.members.contains(federateId)) {
    return std::nullopt;
  }
  auto const handle = federation->second.objectInstanceHandlesByName.find(objectInstanceName);
  if (handle == federation->second.objectInstanceHandlesByName.end()) {
    return std::nullopt;
  }
  auto const instance = federation->second.objectInstances.find(handle->second);
  if (instance == federation->second.objectInstances.end()) {
    return std::nullopt;
  }
  return knownObjectInstanceSnapshot(federation->second, instance->second, federateId);
}

ReceiveOrderAttributeUpdatePlan EmbeddedFederationRegistry::planReceiveOrderAttributeUpdate(
    std::wstring const& federationName,
    std::uint64_t producingFederateId,
    std::uint64_t objectInstanceHandle,
    std::vector<std::uint64_t> const& sentAttributeHandles) const {
  std::scoped_lock lock(mutex_);
  auto const federation = federations_.find(federationName);
  if (federation == federations_.end()) {
    return {ReceiveOrderAttributeUpdateStatus::federation_does_not_exist};
  }
  if (!federation->second.members.contains(producingFederateId)) {
    return {ReceiveOrderAttributeUpdateStatus::producing_federate_not_member};
  }
  auto const instance = federation->second.objectInstances.find(objectInstanceHandle);
  if (instance == federation->second.objectInstances.end() ||
      instance->second.deleteAccepted ||
      !instance->second.knownObjectClassHandlesByFederate.contains(producingFederateId)) {
    return {ReceiveOrderAttributeUpdateStatus::object_instance_not_known};
  }
  if (!federation->second.definition.catalog ||
      !federation->second.objectClassHandles ||
      !federation->second.attributeHandles) {
    return {ReceiveOrderAttributeUpdateStatus::inconsistent_catalog};
  }

  auto const registeredClassName = federation->second.objectClassHandles->nameFor(
      instance->second.registeredObjectClassHandle);
  if (!registeredClassName ||
      federation->second.definition.catalog->objectClass(*registeredClassName) == nullptr) {
    return {ReceiveOrderAttributeUpdateStatus::inconsistent_catalog};
  }

  std::set<std::uint64_t> uniqueAttributeHandles;
  using PasselKey = std::pair<std::string, std::set<std::uint64_t>>;
  std::map<PasselKey, std::vector<std::uint64_t>> attributesByTransportationAndRegions;
  for (std::uint64_t const attributeHandle : sentAttributeHandles) {
    if (!uniqueAttributeHandles.insert(attributeHandle).second ||
        !federation->second.attributeHandles->nameFor(
            federation->second.definition.catalog.get(),
            *registeredClassName,
            attributeHandle)) {
      return {ReceiveOrderAttributeUpdateStatus::attribute_not_defined};
    }
    auto const owner = instance->second.attributeOwnersByHandle.find(attributeHandle);
    if (owner == instance->second.attributeOwnersByHandle.end() ||
        owner->second != producingFederateId) {
      return {ReceiveOrderAttributeUpdateStatus::attribute_not_owned};
    }
    auto const transportationName = attributeTransportationName(
        federation->second,
        instance->second.registeredObjectClassHandle,
        attributeHandle);
    if (!transportationName) {
      return {ReceiveOrderAttributeUpdateStatus::inconsistent_catalog};
    }
    std::set<std::uint64_t> associatedRegions;
    auto const regionAssociation = instance->second.updateRegionsByAttribute.find(attributeHandle);
    if (regionAssociation != instance->second.updateRegionsByAttribute.end()) {
      associatedRegions = regionAssociation->second;
    }
    attributesByTransportationAndRegions[
        {*transportationName, std::move(associatedRegions)}].push_back(attributeHandle);
  }

  ReceiveOrderAttributeUpdatePlan result;
  result.passels.reserve(attributesByTransportationAndRegions.size());
  for (auto const& [passelKey, passelAttributes] : attributesByTransportationAndRegions) {
    ReceiveOrderAttributeUpdatePassel passel;
    passel.transportationName = passelKey.first;
    passel.sentAttributeHandles = passelAttributes;
    passel.sentRegionHandles = passelKey.second;
    for (auto const& [federateId, membership] : federation->second.members) {
      static_cast<void>(membership);
      auto recipient = candidateReceiveOrderAttributeUpdateRecipient(
          federation->second,
          producingFederateId,
          federateId,
          objectInstanceHandle,
          passel.sentAttributeHandles,
          passel.sentRegionHandles.empty() ? nullptr : &passel.sentRegionHandles);
      if (recipient) {
        passel.recipients.push_back(std::move(*recipient));
      }
    }
    result.passels.push_back(std::move(passel));
  }
  return result;
}

std::optional<ReceiveOrderAttributeUpdateRecipient>
EmbeddedFederationRegistry::receiveOrderAttributeUpdateRecipientFor(
    std::wstring const& federationName,
    std::uint64_t producingFederateId,
    std::uint64_t receivingFederateId,
    std::uint64_t objectInstanceHandle,
    std::vector<std::uint64_t> const& sentAttributeHandles,
    std::set<std::uint64_t> const* sentRegionHandles) const {
  std::scoped_lock lock(mutex_);
  auto const federation = federations_.find(federationName);
  if (federation == federations_.end()) {
    return std::nullopt;
  }
  return candidateReceiveOrderAttributeUpdateRecipient(
      federation->second,
      producingFederateId,
      receivingFederateId,
      objectInstanceHandle,
      sentAttributeHandles,
      sentRegionHandles);
}

AttributeValueUpdateRequestPlan EmbeddedFederationRegistry::planAttributeValueUpdateRequest(
    std::wstring const& federationName,
    std::uint64_t requestingFederateId,
    std::uint64_t objectInstanceHandle,
    std::set<std::uint64_t> const& requestedAttributeHandles) const {
  std::scoped_lock lock(mutex_);
  auto const federation = federations_.find(federationName);
  if (federation == federations_.end()) {
    return {AttributeValueUpdateRequestStatus::federation_does_not_exist};
  }
  if (!federation->second.members.contains(requestingFederateId)) {
    return {AttributeValueUpdateRequestStatus::requesting_federate_not_member};
  }
  auto const instance = federation->second.objectInstances.find(objectInstanceHandle);
  if (instance == federation->second.objectInstances.end() ||
      !instance->second.knownObjectClassHandlesByFederate.contains(requestingFederateId)) {
    return {AttributeValueUpdateRequestStatus::object_instance_not_known};
  }
  if (!federation->second.definition.catalog ||
      !federation->second.objectClassHandles ||
      !federation->second.attributeHandles) {
    return {AttributeValueUpdateRequestStatus::inconsistent_catalog};
  }

  auto const knownClass = instance->second.knownObjectClassHandlesByFederate.find(
      requestingFederateId);
  auto const knownClassName = federation->second.objectClassHandles->nameFor(knownClass->second);
  if (!knownClassName ||
      federation->second.definition.catalog->objectClass(*knownClassName) == nullptr) {
    return {AttributeValueUpdateRequestStatus::inconsistent_catalog};
  }
  for (std::uint64_t const attributeHandle : requestedAttributeHandles) {
    if (!federation->second.attributeHandles->nameFor(
            federation->second.definition.catalog.get(),
            *knownClassName,
            attributeHandle)) {
      return {AttributeValueUpdateRequestStatus::attribute_not_defined};
    }
  }

  std::map<std::uint64_t, std::set<std::uint64_t>> attributesByOwner;
  for (std::uint64_t const attributeHandle : requestedAttributeHandles) {
    auto const owner = instance->second.attributeOwnersByHandle.find(attributeHandle);
    if (owner != instance->second.attributeOwnersByHandle.end() &&
        owner->second != requestingFederateId) {
      attributesByOwner[owner->second].insert(attributeHandle);
    }
  }

  AttributeValueUpdateRequestPlan result;
  result.recipients.reserve(attributesByOwner.size());
  for (auto const& [providingFederateId, ownedAttributeHandles] : attributesByOwner) {
    auto recipient = candidateAttributeValueUpdateProvideRecipient(
        federation->second,
        requestingFederateId,
        providingFederateId,
        objectInstanceHandle,
        ownedAttributeHandles);
    if (recipient) {
      result.recipients.push_back(std::move(*recipient));
    }
  }
  return result;
}

std::optional<AttributeValueUpdateProvideRecipient>
EmbeddedFederationRegistry::attributeValueUpdateProvideRecipientFor(
    std::wstring const& federationName,
    std::uint64_t requestingFederateId,
    std::uint64_t providingFederateId,
    std::uint64_t objectInstanceHandle,
    std::set<std::uint64_t> const& requestedAttributeHandles) const {
  std::scoped_lock lock(mutex_);
  auto const federation = federations_.find(federationName);
  if (federation == federations_.end()) {
    return std::nullopt;
  }
  return candidateAttributeValueUpdateProvideRecipient(
      federation->second,
      requestingFederateId,
      providingFederateId,
      objectInstanceHandle,
      requestedAttributeHandles);
}

AttributeValueUpdateClassRequestPlan
EmbeddedFederationRegistry::planAttributeValueUpdateClassRequest(
    std::wstring const& federationName,
    std::uint64_t requestingFederateId,
    std::uint64_t objectClassHandle,
    std::set<std::uint64_t> const& requestedAttributeHandles,
    std::map<std::uint64_t, std::set<std::uint64_t>> const* requestRegionsByAttribute) const {
  std::scoped_lock lock(mutex_);
  auto const federation = federations_.find(federationName);
  if (federation == federations_.end()) {
    return {AttributeValueUpdateClassRequestStatus::federation_does_not_exist};
  }
  if (!federation->second.members.contains(requestingFederateId)) {
    return {AttributeValueUpdateClassRequestStatus::requesting_federate_not_member};
  }
  if (!federation->second.definition.catalog ||
      !federation->second.objectClassHandles ||
      !federation->second.attributeHandles) {
    return {AttributeValueUpdateClassRequestStatus::inconsistent_catalog};
  }
  if (!validObjectClass(federation->second, objectClassHandle)) {
    return {AttributeValueUpdateClassRequestStatus::object_class_not_defined};
  }
  if (!validObjectClassAttributes(
          federation->second,
          objectClassHandle,
          requestedAttributeHandles)) {
    return {AttributeValueUpdateClassRequestStatus::attribute_not_defined};
  }

  if (requestRegionsByAttribute != nullptr) {
    // A regional request uses the same class DDM context as the regional
    // subscription services. Validate every explicit region before planning
    // any provider callback so an invalid pair cannot partially route.
    auto const availableDimensions = availableObjectClassDimensions(
        federation->second,
        objectClassHandle);
    if (!availableDimensions) {
      return {AttributeValueUpdateClassRequestStatus::inconsistent_catalog};
    }
    for (auto const& [attributeHandle, regionHandles] : *requestRegionsByAttribute) {
      if (!requestedAttributeHandles.contains(attributeHandle)) {
        return {AttributeValueUpdateClassRequestStatus::attribute_not_defined};
      }
      for (std::uint64_t const regionHandle : regionHandles) {
        auto const region = federation->second.regions.find(regionHandle);
        if (region == federation->second.regions.end()) {
          return {AttributeValueUpdateClassRequestStatus::invalid_region};
        }
        if (region->second.ownerFederateId != requestingFederateId) {
          return {
              AttributeValueUpdateClassRequestStatus::region_not_created_by_this_federate};
        }
        if (!region->second.specificationCommitted ||
            region->second.committedRangeBounds.size() != region->second.dimensionHandles.size()) {
          return {AttributeValueUpdateClassRequestStatus::invalid_region};
        }
        if (!std::includes(
                availableDimensions->begin(),
                availableDimensions->end(),
                region->second.dimensionHandles.begin(),
                region->second.dimensionHandles.end())) {
          return {AttributeValueUpdateClassRequestStatus::invalid_region_context};
        }
      }
    }
  }

  AttributeValueUpdateClassRequestPlan result;
  for (auto const& [objectInstanceHandle, objectInstance] : federation->second.objectInstances) {
    if (objectInstance.deleteAccepted ||
        !objectInstanceRegisteredAtOrBelowClass(
            federation->second,
            objectInstance.registeredObjectClassHandle,
            objectClassHandle)) {
      continue;
    }

    std::map<std::uint64_t, std::set<std::uint64_t>> attributesByOwner;
    for (std::uint64_t const attributeHandle : requestedAttributeHandles) {
      auto const owner = objectInstance.attributeOwnersByHandle.find(attributeHandle);
      if (owner != objectInstance.attributeOwnersByHandle.end() &&
          owner->second != requestingFederateId) {
        attributesByOwner[owner->second].insert(attributeHandle);
      }
    }
    for (auto const& [providingFederateId, ownedAttributeHandles] : attributesByOwner) {
      auto recipient = candidateAttributeValueUpdateClassProvideRecipient(
          federation->second,
          requestingFederateId,
          providingFederateId,
          objectInstanceHandle,
          objectClassHandle,
          ownedAttributeHandles,
          requestRegionsByAttribute);
      if (recipient) {
        result.recipients.push_back(std::move(*recipient));
      }
    }
  }
  return result;
}

std::optional<AttributeValueUpdateProvideRecipient>
EmbeddedFederationRegistry::attributeValueUpdateClassProvideRecipientFor(
    std::wstring const& federationName,
    std::uint64_t requestingFederateId,
    std::uint64_t providingFederateId,
    std::uint64_t objectInstanceHandle,
    std::uint64_t requestedObjectClassHandle,
    std::set<std::uint64_t> const& requestedAttributeHandles,
    std::map<std::uint64_t, std::set<std::uint64_t>> const* requestRegionsByAttribute) const {
  std::scoped_lock lock(mutex_);
  auto const federation = federations_.find(federationName);
  if (federation == federations_.end()) {
    return std::nullopt;
  }
  return candidateAttributeValueUpdateClassProvideRecipient(
      federation->second,
      requestingFederateId,
      providingFederateId,
      objectInstanceHandle,
      requestedObjectClassHandle,
      requestedAttributeHandles,
      requestRegionsByAttribute);
}

AttributeOwnershipQueryPlan EmbeddedFederationRegistry::planAttributeOwnershipQuery(
    std::wstring const& federationName,
    std::uint64_t requestingFederateId,
    std::uint64_t objectInstanceHandle,
    std::set<std::uint64_t> const& requestedAttributeHandles) const {
  std::scoped_lock lock(mutex_);
  auto const federation = federations_.find(federationName);
  if (federation == federations_.end()) {
    return {AttributeOwnershipQueryStatus::federation_does_not_exist};
  }
  if (!federation->second.members.contains(requestingFederateId)) {
    return {AttributeOwnershipQueryStatus::requesting_federate_not_member};
  }
  auto const instance = federation->second.objectInstances.find(objectInstanceHandle);
  if (instance == federation->second.objectInstances.end() || instance->second.deleteAccepted ||
      !instance->second.knownObjectClassHandlesByFederate.contains(requestingFederateId)) {
    return {AttributeOwnershipQueryStatus::object_instance_not_known};
  }
  if (!federation->second.definition.catalog ||
      !federation->second.objectClassHandles ||
      !federation->second.attributeHandles) {
    return {AttributeOwnershipQueryStatus::inconsistent_catalog};
  }

  auto const knownClass = instance->second.knownObjectClassHandlesByFederate.find(
      requestingFederateId);
  auto const knownClassName = federation->second.objectClassHandles->nameFor(knownClass->second);
  if (!knownClassName ||
      federation->second.definition.catalog->objectClass(*knownClassName) == nullptr) {
    return {AttributeOwnershipQueryStatus::inconsistent_catalog};
  }
  for (std::uint64_t const attributeHandle : requestedAttributeHandles) {
    if (!federation->second.attributeHandles->nameFor(
            federation->second.definition.catalog.get(),
            *knownClassName,
            attributeHandle)) {
      return {AttributeOwnershipQueryStatus::attribute_not_defined};
    }
  }

  std::map<std::uint64_t, std::set<std::uint64_t>> attributesByOwner;
  std::set<std::uint64_t> unownedAttributes;
  for (std::uint64_t const attributeHandle : requestedAttributeHandles) {
    auto const owner = instance->second.attributeOwnersByHandle.find(attributeHandle);
    if (owner == instance->second.attributeOwnersByHandle.end()) {
      unownedAttributes.insert(attributeHandle);
      continue;
    }
    // The present runtime has no standard ownership-resignation lifecycle
    // slice. A retained owner ID without a joined federate cannot be reported
    // as either a federate or an unowned attribute without falsifying 2025
    // ownership semantics, so make that incomplete state explicit.
    if (!federation->second.members.contains(owner->second)) {
      return {AttributeOwnershipQueryStatus::inconsistent_catalog};
    }
    attributesByOwner[owner->second].insert(attributeHandle);
  }

  AttributeOwnershipQueryPlan result;
  result.recipients.reserve(attributesByOwner.size() + (unownedAttributes.empty() ? 0U : 1U));
  for (auto const& [owningFederateId, ownedAttributeHandles] : attributesByOwner) {
    auto recipient = candidateAttributeOwnershipQueryRecipient(
        federation->second,
        requestingFederateId,
        objectInstanceHandle,
        AttributeOwnershipQueryReportKind::federate,
        owningFederateId,
        ownedAttributeHandles);
    if (recipient) {
      result.recipients.push_back(std::move(*recipient));
    }
  }
  if (!unownedAttributes.empty()) {
    auto recipient = candidateAttributeOwnershipQueryRecipient(
        federation->second,
        requestingFederateId,
        objectInstanceHandle,
        AttributeOwnershipQueryReportKind::unowned,
        0,
        unownedAttributes);
    if (recipient) {
      result.recipients.push_back(std::move(*recipient));
    }
  }
  return result;
}

std::optional<AttributeOwnershipQueryRecipient>
EmbeddedFederationRegistry::attributeOwnershipQueryRecipientFor(
    std::wstring const& federationName,
    std::uint64_t requestingFederateId,
    std::uint64_t objectInstanceHandle,
    AttributeOwnershipQueryReportKind reportKind,
    std::uint64_t owningFederateId,
    std::set<std::uint64_t> const& requestedAttributeHandles) const {
  std::scoped_lock lock(mutex_);
  auto const federation = federations_.find(federationName);
  if (federation == federations_.end()) {
    return std::nullopt;
  }
  return candidateAttributeOwnershipQueryRecipient(
      federation->second,
      requestingFederateId,
      objectInstanceHandle,
      reportKind,
      owningFederateId,
      requestedAttributeHandles);
}

AttributeOwnershipCheckResult EmbeddedFederationRegistry::attributeOwnedByFederate(
    std::wstring const& federationName,
    std::uint64_t requestingFederateId,
    std::uint64_t objectInstanceHandle,
    std::uint64_t attributeHandle) const {
  std::scoped_lock lock(mutex_);
  auto const federation = federations_.find(federationName);
  if (federation == federations_.end()) {
    return {AttributeOwnershipCheckStatus::federation_does_not_exist};
  }
  if (!federation->second.members.contains(requestingFederateId)) {
    return {AttributeOwnershipCheckStatus::requesting_federate_not_member};
  }
  auto const instance = federation->second.objectInstances.find(objectInstanceHandle);
  if (instance == federation->second.objectInstances.end() || instance->second.deleteAccepted ||
      !instance->second.knownObjectClassHandlesByFederate.contains(requestingFederateId)) {
    return {AttributeOwnershipCheckStatus::object_instance_not_known};
  }
  if (!federation->second.definition.catalog ||
      !federation->second.objectClassHandles ||
      !federation->second.attributeHandles) {
    return {AttributeOwnershipCheckStatus::inconsistent_catalog};
  }

  auto const knownClass = instance->second.knownObjectClassHandlesByFederate.find(
      requestingFederateId);
  auto const knownClassName = federation->second.objectClassHandles->nameFor(knownClass->second);
  if (!knownClassName ||
      federation->second.definition.catalog->objectClass(*knownClassName) == nullptr) {
    return {AttributeOwnershipCheckStatus::inconsistent_catalog};
  }
  if (!federation->second.attributeHandles->nameFor(
          federation->second.definition.catalog.get(),
          *knownClassName,
          attributeHandle)) {
    return {AttributeOwnershipCheckStatus::attribute_not_defined};
  }

  auto const owner = instance->second.attributeOwnersByHandle.find(attributeHandle);
  return {
      AttributeOwnershipCheckStatus::applied,
      owner != instance->second.attributeOwnersByHandle.end() &&
          owner->second == requestingFederateId,
  };
}

AttributeOwnershipAcquisitionIfAvailablePlan
EmbeddedFederationRegistry::planAttributeOwnershipAcquisitionIfAvailable(
    std::wstring const& federationName,
    std::uint64_t requestingFederateId,
    std::uint64_t objectInstanceHandle,
    std::set<std::uint64_t> const& desiredAttributeHandles) {
  std::scoped_lock lock(mutex_);
  auto federation = federations_.find(federationName);
  if (federation == federations_.end()) {
    return {AttributeOwnershipAcquisitionIfAvailableStatus::federation_does_not_exist};
  }
  if (!federation->second.members.contains(requestingFederateId)) {
    return {AttributeOwnershipAcquisitionIfAvailableStatus::requesting_federate_not_member};
  }
  auto instance = federation->second.objectInstances.find(objectInstanceHandle);
  if (instance == federation->second.objectInstances.end() || instance->second.deleteAccepted ||
      !instance->second.knownObjectClassHandlesByFederate.contains(requestingFederateId)) {
    return {AttributeOwnershipAcquisitionIfAvailableStatus::object_instance_not_known};
  }
  if (!federation->second.definition.catalog ||
      !federation->second.objectClassHandles ||
      !federation->second.attributeHandles) {
    return {AttributeOwnershipAcquisitionIfAvailableStatus::inconsistent_catalog};
  }

  auto const knownClass = instance->second.knownObjectClassHandlesByFederate.find(
      requestingFederateId);
  auto const knownClassName = federation->second.objectClassHandles->nameFor(knownClass->second);
  if (!knownClassName ||
      federation->second.definition.catalog->objectClass(*knownClassName) == nullptr) {
    return {AttributeOwnershipAcquisitionIfAvailableStatus::inconsistent_catalog};
  }
  for (std::uint64_t const attributeHandle : desiredAttributeHandles) {
    if (!federation->second.attributeHandles->nameFor(
            federation->second.definition.catalog.get(),
            *knownClassName,
            attributeHandle)) {
      return {AttributeOwnershipAcquisitionIfAvailableStatus::attribute_not_defined};
    }
  }

  // An empty request changes neither ownership nor the private ownership
  // state chart. Its object/federate preconditions above still apply.
  if (desiredAttributeHandles.empty()) {
    return {};
  }

  auto const publishedAttributes = publishedObjectClassAttributes(
      federation->second,
      requestingFederateId,
      knownClass->second);
  if (!publishedAttributes) {
    return {AttributeOwnershipAcquisitionIfAvailableStatus::inconsistent_catalog};
  }
  if (publishedAttributes->empty()) {
    return {AttributeOwnershipAcquisitionIfAvailableStatus::object_class_not_published};
  }
  for (std::uint64_t const attributeHandle : desiredAttributeHandles) {
    if (!publishedAttributes->contains(attributeHandle)) {
      return {AttributeOwnershipAcquisitionIfAvailableStatus::attribute_not_published};
    }
  }

  for (std::uint64_t const attributeHandle : desiredAttributeHandles) {
    auto const owner = instance->second.attributeOwnersByHandle.find(attributeHandle);
    if (owner == instance->second.attributeOwnersByHandle.end()) {
      continue;
    }
    if (owner->second == requestingFederateId) {
      return {AttributeOwnershipAcquisitionIfAvailableStatus::federate_owns_attributes};
    }
    if (!federation->second.members.contains(owner->second)) {
      // The current runtime has not implemented resign ownership disposition.
      // Do not claim availability from an ownership record that no longer has
      // a joined federate behind it.
      return {AttributeOwnershipAcquisitionIfAvailableStatus::inconsistent_catalog};
    }
  }

  // IEEE 1516.1-2025 7.1.2.2 prohibits entering Willing to Acquire after
  // this federate has already entered regular Acquisition Pending for the
  // same attribute.  The official C++ binding exposes that condition as
  // AttributeAlreadyBeingAcquired.
  for (auto const& [requestId, pending] :
       instance->second.pendingAttributeOwnershipAcquisitionRequests) {
    static_cast<void>(requestId);
    if (pending.requestingFederateId != requestingFederateId) {
      continue;
    }
    for (std::uint64_t const attributeHandle : desiredAttributeHandles) {
      if (pending.desiredAttributeHandles.contains(attributeHandle)) {
        return {AttributeOwnershipAcquisitionIfAvailableStatus::attribute_already_being_acquired};
      }
    }
  }
  for (auto const& [cancellationId, cancellation] :
       instance->second.pendingAttributeOwnershipAcquisitionCancellations) {
    static_cast<void>(cancellationId);
    if (cancellation.requestingFederateId != requestingFederateId) {
      continue;
    }
    for (std::uint64_t const attributeHandle : desiredAttributeHandles) {
      if (cancellation.attributeHandles.contains(attributeHandle)) {
        return {AttributeOwnershipAcquisitionIfAvailableStatus::attribute_already_being_acquired};
      }
    }
  }

  std::set<std::uint64_t> newlyRequestedAttributeHandles = desiredAttributeHandles;
  for (auto const& [requestId, pending] :
       instance->second.pendingAttributeOwnershipAcquisitionIfAvailableRequests) {
    static_cast<void>(requestId);
    if (pending.requestingFederateId == requestingFederateId) {
      for (std::uint64_t const attributeHandle : pending.desiredAttributeHandles) {
        newlyRequestedAttributeHandles.erase(attributeHandle);
      }
    }
  }

  // IEEE 1516.1-2025 7.9 leaves an attribute unchanged when this federate
  // invokes If Available while it is already Willing to Acquire.  It is not
  // the AttributeAlreadyBeingAcquired exception, which applies to a pending
  // regular Attribute Ownership Acquisition request.  Do not manufacture a
  // second reservation or terminal callback for the existing WTA attribute.
  if (newlyRequestedAttributeHandles.empty()) {
    return {};
  }

  if (federation->second.nextAttributeOwnershipAcquisitionIfAvailableRequestId == 0 ||
      federation->second.nextAttributeOwnershipAcquisitionIfAvailableRequestId ==
          std::numeric_limits<std::uint64_t>::max() ||
      federation->second.nextAttributeOwnershipAcquisitionRequestSequence == 0 ||
      federation->second.nextAttributeOwnershipAcquisitionRequestSequence ==
          std::numeric_limits<std::uint64_t>::max()) {
    return {AttributeOwnershipAcquisitionIfAvailableStatus::inconsistent_catalog};
  }
  auto const callbackRoute = federation->second.interactionCallbackRoutes.find(
      requestingFederateId);
  if (callbackRoute == federation->second.interactionCallbackRoutes.end() ||
      !callbackRoute->second) {
    return {AttributeOwnershipAcquisitionIfAvailableStatus::inconsistent_catalog};
  }

  std::uint64_t const requestId =
      federation->second.nextAttributeOwnershipAcquisitionIfAvailableRequestId;
  std::uint64_t const requestSequence =
      federation->second.nextAttributeOwnershipAcquisitionRequestSequence;
  auto const [pending, inserted] =
      instance->second.pendingAttributeOwnershipAcquisitionIfAvailableRequests.try_emplace(
          requestId,
          Federation::ObjectInstance::PendingAttributeOwnershipAcquisitionIfAvailable{
              requestingFederateId,
              requestSequence,
              std::move(newlyRequestedAttributeHandles),
          });
  static_cast<void>(pending);
  if (!inserted) {
    return {AttributeOwnershipAcquisitionIfAvailableStatus::inconsistent_catalog};
  }
  ++federation->second.nextAttributeOwnershipAcquisitionIfAvailableRequestId;
  ++federation->second.nextAttributeOwnershipAcquisitionRequestSequence;
  return {
      AttributeOwnershipAcquisitionIfAvailableStatus::applied,
      requestId,
      callbackRoute->second,
  };
}

std::optional<AttributeOwnershipAcquisitionIfAvailableDelivery>
EmbeddedFederationRegistry::beginAttributeOwnershipAcquisitionIfAvailable(
    std::wstring const& federationName,
    std::uint64_t requestingFederateId,
    std::uint64_t objectInstanceHandle,
    std::uint64_t requestId) {
  if (requestId == 0) {
    return std::nullopt;
  }

  std::scoped_lock lock(mutex_);
  auto federation = federations_.find(federationName);
  if (federation == federations_.end()) {
    return std::nullopt;
  }
  auto instance = federation->second.objectInstances.find(objectInstanceHandle);
  if (instance == federation->second.objectInstances.end()) {
    return std::nullopt;
  }
  auto pending = instance->second.pendingAttributeOwnershipAcquisitionIfAvailableRequests.find(
      requestId);
  if (pending == instance->second.pendingAttributeOwnershipAcquisitionIfAvailableRequests.end()) {
    return std::nullopt;
  }

  auto clearPending = [&] {
    instance->second.pendingAttributeOwnershipAcquisitionIfAvailableRequests.erase(pending);
  };
  if (pending->second.requestingFederateId != requestingFederateId ||
      instance->second.deleteAccepted ||
      !federation->second.members.contains(requestingFederateId) ||
      !instance->second.knownObjectClassHandlesByFederate.contains(requestingFederateId) ||
      !federation->second.definition.catalog ||
      !federation->second.objectClassHandles ||
      !federation->second.attributeHandles) {
    clearPending();
    return std::nullopt;
  }

  auto const knownClass = instance->second.knownObjectClassHandlesByFederate.find(
      requestingFederateId);
  auto const knownClassName = federation->second.objectClassHandles->nameFor(knownClass->second);
  if (!knownClassName ||
      federation->second.definition.catalog->objectClass(*knownClassName) == nullptr) {
    clearPending();
    return std::nullopt;
  }
  auto const publishedAttributes = publishedObjectClassAttributes(
      federation->second,
      requestingFederateId,
      knownClass->second);
  if (!publishedAttributes) {
    clearPending();
    return std::nullopt;
  }

  AttributeOwnershipAcquisitionIfAvailableDelivery delivery;
  delivery.objectInstanceHandle = objectInstanceHandle;
  auto revisedAttributeOwners = instance->second.attributeOwnersByHandle;
  for (std::uint64_t const attributeHandle : pending->second.desiredAttributeHandles) {
    if (!federation->second.attributeHandles->nameFor(
            federation->second.definition.catalog.get(),
            *knownClassName,
            attributeHandle) ||
        !publishedAttributes->contains(attributeHandle)) {
      // A federate must keep publishing until its terminal callback. If it
      // does not, do not invoke a callback whose standard preconditions are
      // no longer true.
      clearPending();
      return std::nullopt;
    }

    auto const owner = revisedAttributeOwners.find(attributeHandle);
    if (owner == revisedAttributeOwners.end()) {
      delivery.securedAttributeHandles.insert(attributeHandle);
      revisedAttributeOwners.emplace(attributeHandle, requestingFederateId);
      continue;
    }
    if (owner->second == requestingFederateId) {
      // This bounded runtime has no other transfer operation that can reach
      // this branch. If a future ownership transition did so first, the
      // matching terminal notification remains the truthful response.
      delivery.securedAttributeHandles.insert(attributeHandle);
      continue;
    }
    if (!federation->second.members.contains(owner->second)) {
      clearPending();
      return std::nullopt;
    }
    delivery.unavailableAttributeHandles.insert(attributeHandle);
  }

  if (delivery.securedAttributeHandles.empty() && delivery.unavailableAttributeHandles.empty()) {
    clearPending();
    return std::nullopt;
  }

  // Ownership becomes visible immediately before its matching standard
  // callback, never at request acceptance. This preserves the pending
  // Willing to Acquire state for an evoked callback model.
  instance->second.attributeOwnersByHandle.swap(revisedAttributeOwners);
  clearPending();
  return delivery;
}

void EmbeddedFederationRegistry::cancelAttributeOwnershipAcquisitionIfAvailable(
    std::wstring const& federationName,
    std::uint64_t requestingFederateId,
    std::uint64_t objectInstanceHandle,
    std::uint64_t requestId) {
  if (requestId == 0) {
    return;
  }

  std::scoped_lock lock(mutex_);
  auto federation = federations_.find(federationName);
  if (federation == federations_.end()) {
    return;
  }
  auto instance = federation->second.objectInstances.find(objectInstanceHandle);
  if (instance == federation->second.objectInstances.end()) {
    return;
  }
  auto pending = instance->second.pendingAttributeOwnershipAcquisitionIfAvailableRequests.find(
      requestId);
  if (pending != instance->second.pendingAttributeOwnershipAcquisitionIfAvailableRequests.end() &&
      pending->second.requestingFederateId == requestingFederateId) {
    instance->second.pendingAttributeOwnershipAcquisitionIfAvailableRequests.erase(pending);
  }
}

NegotiatedAttributeOwnershipDivestiturePlan
EmbeddedFederationRegistry::planNegotiatedAttributeOwnershipDivestiture(
    std::wstring const& federationName,
    std::uint64_t divestingFederateId,
    std::uint64_t objectInstanceHandle,
    std::set<std::uint64_t> const& attributeHandles,
    std::vector<unsigned char> userSuppliedTag) {
  std::scoped_lock lock(mutex_);
  auto federation = federations_.find(federationName);
  if (federation == federations_.end()) {
    return {NegotiatedAttributeOwnershipDivestitureStatus::federation_does_not_exist};
  }
  if (!federation->second.members.contains(divestingFederateId)) {
    return {NegotiatedAttributeOwnershipDivestitureStatus::divesting_federate_not_member};
  }
  auto instance = federation->second.objectInstances.find(objectInstanceHandle);
  if (instance == federation->second.objectInstances.end() || instance->second.deleteAccepted ||
      !instance->second.knownObjectClassHandlesByFederate.contains(divestingFederateId)) {
    return {NegotiatedAttributeOwnershipDivestitureStatus::object_instance_not_known};
  }
  if (!federation->second.definition.catalog ||
      !federation->second.objectClassHandles ||
      !federation->second.attributeHandles) {
    return {NegotiatedAttributeOwnershipDivestitureStatus::inconsistent_catalog};
  }

  auto const knownClass = instance->second.knownObjectClassHandlesByFederate.find(
      divestingFederateId);
  auto const knownClassName = federation->second.objectClassHandles->nameFor(knownClass->second);
  if (!knownClassName ||
      federation->second.definition.catalog->objectClass(*knownClassName) == nullptr) {
    return {NegotiatedAttributeOwnershipDivestitureStatus::inconsistent_catalog};
  }
  for (std::uint64_t const attributeHandle : attributeHandles) {
    if (!federation->second.attributeHandles->nameFor(
            federation->second.definition.catalog.get(),
            *knownClassName,
            attributeHandle)) {
      return {NegotiatedAttributeOwnershipDivestitureStatus::attribute_not_defined};
    }
    auto const owner = instance->second.attributeOwnersByHandle.find(attributeHandle);
    if (owner == instance->second.attributeOwnersByHandle.end() ||
        owner->second != divestingFederateId) {
      return {NegotiatedAttributeOwnershipDivestitureStatus::attribute_not_owned};
    }
    if (instance->second.pendingNegotiatedAttributeOwnershipDivestitures.contains(
            attributeHandle)) {
      return {NegotiatedAttributeOwnershipDivestitureStatus::attribute_already_being_divested};
    }
  }

  // An empty request has no ownership or pending-divestiture transition after
  // its normal membership/object preconditions have been established.
  if (attributeHandles.empty()) {
    return {};
  }

  // Construct every state record before merging it into the federation so an
  // allocation failure cannot leave part of a supplied attribute set waiting
  // for divestiture.
  std::map<std::uint64_t,
           Federation::ObjectInstance::PendingNegotiatedAttributeOwnershipDivestiture>
      requestedDivestitures;
  for (std::uint64_t const attributeHandle : attributeHandles) {
    requestedDivestitures.emplace(
        attributeHandle,
        Federation::ObjectInstance::PendingNegotiatedAttributeOwnershipDivestiture{
            divestingFederateId,
            0,
            0,
            false,
            false,
            userSuppliedTag,
        });
  }
  instance->second.pendingNegotiatedAttributeOwnershipDivestitures.merge(requestedDivestitures);

  // Keep an already queued ordinary release callback reserved until its
  // callback boundary. If it is invoked while this negotiated divestiture is
  // pending, beginAttributeOwnershipAcquisitionRelease consumes and suppresses
  // it. If cancellation occurs first, the original one-shot callback remains
  // the correct ordinary-release notification and no duplicate is queued.

  NegotiatedAttributeOwnershipDivestiturePlan result;
  result.workItems = planPendingAttributeOwnershipAcquisitionWork(
      federation->second,
      instance->second);
  return result;
}

std::optional<RequestDivestitureConfirmationDelivery>
EmbeddedFederationRegistry::beginRequestDivestitureConfirmation(
    std::wstring const& federationName,
    std::uint64_t divestingFederateId,
    std::uint64_t acquiringFederateId,
    std::uint64_t objectInstanceHandle,
    std::uint64_t acquisitionRequestId,
    std::set<std::uint64_t> const& scheduledAttributeHandles) {
  if (acquisitionRequestId == 0 || scheduledAttributeHandles.empty()) {
    return std::nullopt;
  }

  std::scoped_lock lock(mutex_);
  auto federation = federations_.find(federationName);
  if (federation == federations_.end() ||
      !federation->second.members.contains(divestingFederateId) ||
      !federation->second.members.contains(acquiringFederateId)) {
    return std::nullopt;
  }
  auto instance = federation->second.objectInstances.find(objectInstanceHandle);
  if (instance == federation->second.objectInstances.end() || instance->second.deleteAccepted ||
      !instance->second.knownObjectClassHandlesByFederate.contains(divestingFederateId) ||
      !instance->second.knownObjectClassHandlesByFederate.contains(acquiringFederateId)) {
    return std::nullopt;
  }
  auto const acquisition = instance->second.pendingAttributeOwnershipAcquisitionRequests.find(
      acquisitionRequestId);
  if (acquisition == instance->second.pendingAttributeOwnershipAcquisitionRequests.end() ||
      acquisition->second.requestingFederateId != acquiringFederateId) {
    return std::nullopt;
  }

  RequestDivestitureConfirmationDelivery delivery;
  delivery.objectInstanceHandle = objectInstanceHandle;
  for (std::uint64_t const attributeHandle : scheduledAttributeHandles) {
    auto divestiture = instance->second.pendingNegotiatedAttributeOwnershipDivestitures.find(
        attributeHandle);
    auto const owner = instance->second.attributeOwnersByHandle.find(attributeHandle);
    if (divestiture == instance->second.pendingNegotiatedAttributeOwnershipDivestitures.end() ||
        owner == instance->second.attributeOwnersByHandle.end() ||
        owner->second != divestingFederateId ||
        divestiture->second.divestingFederateId != divestingFederateId ||
        divestiture->second.acquiringFederateId != acquiringFederateId ||
        divestiture->second.acquisitionRequestId != acquisitionRequestId ||
        !divestiture->second.confirmationQueued ||
        !acquisition->second.desiredAttributeHandles.contains(attributeHandle)) {
      continue;
    }
    delivery.releasedAttributeHandles.insert(attributeHandle);
  }
  if (delivery.releasedAttributeHandles.empty()) {
    return std::nullopt;
  }
  for (std::uint64_t const attributeHandle : delivery.releasedAttributeHandles) {
    instance->second.pendingNegotiatedAttributeOwnershipDivestitures.at(attributeHandle)
        .confirmationDelivered = true;
  }
  return delivery;
}

ConfirmDivestiturePlan EmbeddedFederationRegistry::planConfirmDivestiture(
    std::wstring const& federationName,
    std::uint64_t divestingFederateId,
    std::uint64_t objectInstanceHandle,
    std::set<std::uint64_t> const& attributeHandles,
    std::vector<unsigned char> userSuppliedTag) {
  std::scoped_lock lock(mutex_);
  auto federation = federations_.find(federationName);
  if (federation == federations_.end()) {
    return {ConfirmDivestitureStatus::federation_does_not_exist};
  }
  if (!federation->second.members.contains(divestingFederateId)) {
    return {ConfirmDivestitureStatus::divesting_federate_not_member};
  }
  auto instance = federation->second.objectInstances.find(objectInstanceHandle);
  if (instance == federation->second.objectInstances.end() || instance->second.deleteAccepted ||
      !instance->second.knownObjectClassHandlesByFederate.contains(divestingFederateId)) {
    return {ConfirmDivestitureStatus::object_instance_not_known};
  }
  if (!federation->second.definition.catalog ||
      !federation->second.objectClassHandles ||
      !federation->second.attributeHandles) {
    return {ConfirmDivestitureStatus::inconsistent_catalog};
  }

  auto const knownClass = instance->second.knownObjectClassHandlesByFederate.find(
      divestingFederateId);
  auto const knownClassName = federation->second.objectClassHandles->nameFor(knownClass->second);
  if (!knownClassName ||
      federation->second.definition.catalog->objectClass(*knownClassName) == nullptr) {
    return {ConfirmDivestitureStatus::inconsistent_catalog};
  }
  for (std::uint64_t const attributeHandle : attributeHandles) {
    if (!federation->second.attributeHandles->nameFor(
            federation->second.definition.catalog.get(),
            *knownClassName,
            attributeHandle)) {
      return {ConfirmDivestitureStatus::attribute_not_defined};
    }
    auto const owner = instance->second.attributeOwnersByHandle.find(attributeHandle);
    if (owner == instance->second.attributeOwnersByHandle.end() ||
        owner->second != divestingFederateId) {
      return {ConfirmDivestitureStatus::attribute_not_owned};
    }
    auto const divestiture = instance->second.pendingNegotiatedAttributeOwnershipDivestitures.find(
        attributeHandle);
    if (divestiture == instance->second.pendingNegotiatedAttributeOwnershipDivestitures.end() ||
        divestiture->second.divestingFederateId != divestingFederateId ||
        !divestiture->second.confirmationDelivered) {
      return {ConfirmDivestitureStatus::attribute_divestiture_was_not_requested};
    }
  }

  if (attributeHandles.empty()) {
    return {};
  }

  std::map<std::uint64_t, std::set<std::uint64_t>> attributesByAcquiringFederate;
  std::map<std::uint64_t, std::set<std::uint64_t>> attributesByAcquisitionRequest;
  std::set<std::uint64_t> attributesWithoutAnActiveAcquirer;
  for (std::uint64_t const attributeHandle : attributeHandles) {
    auto& divestiture =
        instance->second.pendingNegotiatedAttributeOwnershipDivestitures.at(attributeHandle);
    auto const acquisition = instance->second.pendingAttributeOwnershipAcquisitionRequests.find(
        divestiture.acquisitionRequestId);
    if (divestiture.acquiringFederateId == 0 || divestiture.acquisitionRequestId == 0 ||
        acquisition == instance->second.pendingAttributeOwnershipAcquisitionRequests.end() ||
        acquisition->second.requestingFederateId != divestiture.acquiringFederateId ||
        !acquisition->second.desiredAttributeHandles.contains(attributeHandle) ||
        !federation->second.members.contains(divestiture.acquiringFederateId) ||
        !instance->second.knownObjectClassHandlesByFederate.contains(
            divestiture.acquiringFederateId)) {
      attributesWithoutAnActiveAcquirer.insert(attributeHandle);
      continue;
    }
    auto const callbackRoute = federation->second.interactionCallbackRoutes.find(
        divestiture.acquiringFederateId);
    if (callbackRoute == federation->second.interactionCallbackRoutes.end() ||
        !callbackRoute->second) {
      return {ConfirmDivestitureStatus::inconsistent_catalog};
    }
    attributesByAcquiringFederate[divestiture.acquiringFederateId].insert(attributeHandle);
    attributesByAcquisitionRequest[divestiture.acquisitionRequestId].insert(attributeHandle);
  }

  if (!attributesWithoutAnActiveAcquirer.empty()) {
    // IEEE 1516.1-2025 returns a confirming owner to Waiting for a New Owner
    // to be Found after NoAcquisitionPending. Keep ownership unchanged and
    // make a later regular request eligible for a fresh confirmation.
    for (std::uint64_t const attributeHandle : attributesWithoutAnActiveAcquirer) {
      auto& divestiture =
          instance->second.pendingNegotiatedAttributeOwnershipDivestitures.at(attributeHandle);
      divestiture.acquiringFederateId = 0;
      divestiture.acquisitionRequestId = 0;
      divestiture.confirmationQueued = false;
      divestiture.confirmationDelivered = false;
    }
    return {ConfirmDivestitureStatus::no_acquisition_pending};
  }

  if (federation->second.nextConfirmDivestitureNotificationId == 0 ||
      federation->second.nextConfirmDivestitureNotificationId ==
          std::numeric_limits<std::uint64_t>::max()) {
    return {ConfirmDivestitureStatus::inconsistent_catalog};
  }

  // Build all post-confirmation notification reservations before mutating
  // ownership. Merging the populated map below is non-allocating, preserving
  // the supplied-set failure boundary required by ownership management.
  std::map<std::uint64_t,
           Federation::ObjectInstance::PendingConfirmDivestitureNotification>
      notificationReservations;
  ConfirmDivestiturePlan result;
  result.notifications.reserve(attributesByAcquiringFederate.size());
  std::uint64_t nextNotificationId = federation->second.nextConfirmDivestitureNotificationId;
  for (auto const& [acquiringFederateId, confirmedAttributeHandles] :
       attributesByAcquiringFederate) {
    if (nextNotificationId == 0 ||
        nextNotificationId == std::numeric_limits<std::uint64_t>::max()) {
      return {ConfirmDivestitureStatus::inconsistent_catalog};
    }
    auto const callbackRoute = federation->second.interactionCallbackRoutes.find(acquiringFederateId);
    if (callbackRoute == federation->second.interactionCallbackRoutes.end() ||
        !callbackRoute->second) {
      return {ConfirmDivestitureStatus::inconsistent_catalog};
    }
    notificationReservations.emplace(
        nextNotificationId,
        Federation::ObjectInstance::PendingConfirmDivestitureNotification{
            acquiringFederateId,
            confirmedAttributeHandles,
        });
    result.notifications.push_back({
        nextNotificationId,
        acquiringFederateId,
        objectInstanceHandle,
        confirmedAttributeHandles,
        userSuppliedTag,
        callbackRoute->second,
    });
    ++nextNotificationId;
  }

  for (auto const& [acquisitionRequestId, confirmedAttributeHandles] :
       attributesByAcquisitionRequest) {
    auto acquisition = instance->second.pendingAttributeOwnershipAcquisitionRequests.find(
        acquisitionRequestId);
    if (acquisition == instance->second.pendingAttributeOwnershipAcquisitionRequests.end()) {
      return {ConfirmDivestitureStatus::inconsistent_catalog};
    }
    for (std::uint64_t const attributeHandle : confirmedAttributeHandles) {
      acquisition->second.desiredAttributeHandles.erase(attributeHandle);
      acquisition->second.notificationQueuedAttributeHandles.erase(attributeHandle);
      for (auto release = acquisition->second.releaseCallbacksQueuedByOwningFederate.begin();
           release != acquisition->second.releaseCallbacksQueuedByOwningFederate.end();) {
        release->second.erase(attributeHandle);
        if (release->second.empty()) {
          release = acquisition->second.releaseCallbacksQueuedByOwningFederate.erase(release);
        } else {
          ++release;
        }
      }
    }
    if (acquisition->second.desiredAttributeHandles.empty()) {
      instance->second.pendingAttributeOwnershipAcquisitionRequests.erase(acquisition);
    }
  }
  for (auto const& [acquiringFederateId, confirmedAttributeHandles] :
       attributesByAcquiringFederate) {
    for (std::uint64_t const attributeHandle : confirmedAttributeHandles) {
      instance->second.attributeOwnersByHandle.at(attributeHandle) = acquiringFederateId;
      instance->second.pendingNegotiatedAttributeOwnershipDivestitures.erase(attributeHandle);
    }
  }
  instance->second.pendingConfirmDivestitureNotifications.merge(notificationReservations);
  federation->second.nextConfirmDivestitureNotificationId = nextNotificationId;
  return result;
}

std::optional<ConfirmDivestitureNotificationDelivery>
EmbeddedFederationRegistry::beginConfirmDivestitureNotification(
    std::wstring const& federationName,
    std::uint64_t receivingFederateId,
    std::uint64_t objectInstanceHandle,
    std::uint64_t notificationId,
    std::set<std::uint64_t> const& scheduledAttributeHandles) {
  if (notificationId == 0 || scheduledAttributeHandles.empty()) {
    return std::nullopt;
  }

  std::scoped_lock lock(mutex_);
  auto federation = federations_.find(federationName);
  if (federation == federations_.end()) {
    return std::nullopt;
  }
  auto instance = federation->second.objectInstances.find(objectInstanceHandle);
  if (instance == federation->second.objectInstances.end()) {
    return std::nullopt;
  }
  auto notification = instance->second.pendingConfirmDivestitureNotifications.find(notificationId);
  if (notification == instance->second.pendingConfirmDivestitureNotifications.end() ||
      notification->second.receivingFederateId != receivingFederateId ||
      notification->second.attributeHandles != scheduledAttributeHandles ||
      instance->second.deleteAccepted ||
      !federation->second.members.contains(receivingFederateId) ||
      !instance->second.knownObjectClassHandlesByFederate.contains(receivingFederateId) ||
      !federation->second.definition.catalog ||
      !federation->second.objectClassHandles ||
      !federation->second.attributeHandles) {
    return std::nullopt;
  }

  auto const knownClass = instance->second.knownObjectClassHandlesByFederate.find(
      receivingFederateId);
  auto const knownClassName = federation->second.objectClassHandles->nameFor(knownClass->second);
  if (!knownClassName ||
      federation->second.definition.catalog->objectClass(*knownClassName) == nullptr) {
    return std::nullopt;
  }
  auto const publishedAttributes = publishedObjectClassAttributes(
      federation->second,
      receivingFederateId,
      knownClass->second);
  if (!publishedAttributes) {
    return std::nullopt;
  }
  for (std::uint64_t const attributeHandle : scheduledAttributeHandles) {
    auto const owner = instance->second.attributeOwnersByHandle.find(attributeHandle);
    if (owner == instance->second.attributeOwnersByHandle.end() ||
        owner->second != receivingFederateId ||
        !federation->second.attributeHandles->nameFor(
            federation->second.definition.catalog.get(),
            *knownClassName,
            attributeHandle) ||
        !publishedAttributes->contains(attributeHandle)) {
      return std::nullopt;
    }
  }

  auto securedAttributeHandles = std::move(notification->second.attributeHandles);
  instance->second.pendingConfirmDivestitureNotifications.erase(notification);
  return ConfirmDivestitureNotificationDelivery{
      objectInstanceHandle,
      std::move(securedAttributeHandles),
      planPendingAttributeOwnershipAcquisitionWork(federation->second, instance->second),
  };
}

CancelNegotiatedAttributeOwnershipDivestiturePlan
EmbeddedFederationRegistry::planCancelNegotiatedAttributeOwnershipDivestiture(
    std::wstring const& federationName,
    std::uint64_t divestingFederateId,
    std::uint64_t objectInstanceHandle,
    std::set<std::uint64_t> const& attributeHandles) {
  std::scoped_lock lock(mutex_);
  auto federation = federations_.find(federationName);
  if (federation == federations_.end()) {
    return {CancelNegotiatedAttributeOwnershipDivestitureStatus::federation_does_not_exist};
  }
  if (!federation->second.members.contains(divestingFederateId)) {
    return {CancelNegotiatedAttributeOwnershipDivestitureStatus::divesting_federate_not_member};
  }
  auto instance = federation->second.objectInstances.find(objectInstanceHandle);
  if (instance == federation->second.objectInstances.end() || instance->second.deleteAccepted ||
      !instance->second.knownObjectClassHandlesByFederate.contains(divestingFederateId)) {
    return {CancelNegotiatedAttributeOwnershipDivestitureStatus::object_instance_not_known};
  }
  if (!federation->second.definition.catalog ||
      !federation->second.objectClassHandles ||
      !federation->second.attributeHandles) {
    return {CancelNegotiatedAttributeOwnershipDivestitureStatus::inconsistent_catalog};
  }

  auto const knownClass = instance->second.knownObjectClassHandlesByFederate.find(
      divestingFederateId);
  auto const knownClassName = federation->second.objectClassHandles->nameFor(knownClass->second);
  if (!knownClassName ||
      federation->second.definition.catalog->objectClass(*knownClassName) == nullptr) {
    return {CancelNegotiatedAttributeOwnershipDivestitureStatus::inconsistent_catalog};
  }
  for (std::uint64_t const attributeHandle : attributeHandles) {
    if (!federation->second.attributeHandles->nameFor(
            federation->second.definition.catalog.get(),
            *knownClassName,
            attributeHandle)) {
      return {CancelNegotiatedAttributeOwnershipDivestitureStatus::attribute_not_defined};
    }
    auto const owner = instance->second.attributeOwnersByHandle.find(attributeHandle);
    if (owner == instance->second.attributeOwnersByHandle.end() ||
        owner->second != divestingFederateId) {
      return {CancelNegotiatedAttributeOwnershipDivestitureStatus::attribute_not_owned};
    }
    auto const divestiture = instance->second.pendingNegotiatedAttributeOwnershipDivestitures.find(
        attributeHandle);
    if (divestiture == instance->second.pendingNegotiatedAttributeOwnershipDivestitures.end() ||
        divestiture->second.divestingFederateId != divestingFederateId) {
      return {
          CancelNegotiatedAttributeOwnershipDivestitureStatus::
              attribute_divestiture_was_not_requested};
    }
  }

  if (attributeHandles.empty()) {
    return {};
  }
  for (std::uint64_t const attributeHandle : attributeHandles) {
    instance->second.pendingNegotiatedAttributeOwnershipDivestitures.erase(attributeHandle);
  }
  CancelNegotiatedAttributeOwnershipDivestiturePlan result;
  result.followupWorkItems = planPendingAttributeOwnershipAcquisitionWork(
      federation->second,
      instance->second);
  return result;
}

bool EmbeddedFederationRegistry::isFirstPendingAttributeOwnershipAcquisition(
    Federation::ObjectInstance const& instance,
    std::uint64_t requestId,
    std::uint64_t attributeHandle) {
  for (auto const& [candidateRequestId, pending] :
       instance.pendingAttributeOwnershipAcquisitionRequests) {
    if (pending.desiredAttributeHandles.contains(attributeHandle)) {
      return candidateRequestId == requestId;
    }
  }
  return false;
}

std::vector<AttributeOwnershipAcquisitionWorkItem>
EmbeddedFederationRegistry::planPendingNegotiatedAttributeOwnershipDivestitureConfirmations(
    Federation& federation,
    Federation::ObjectInstance& instance) {
  std::map<std::pair<std::uint64_t, std::uint64_t>, std::set<std::uint64_t>>
      attributesByDivesterAndRequest;

  for (auto& [attributeHandle, divestiture] :
       instance.pendingNegotiatedAttributeOwnershipDivestitures) {
    auto const owner = instance.attributeOwnersByHandle.find(attributeHandle);
    if (owner == instance.attributeOwnersByHandle.end() ||
        owner->second != divestiture.divestingFederateId ||
        !federation.members.contains(divestiture.divestingFederateId)) {
      continue;
    }

    auto selected = instance.pendingAttributeOwnershipAcquisitionRequests.end();
    if (divestiture.acquisitionRequestId != 0) {
      selected = instance.pendingAttributeOwnershipAcquisitionRequests.find(
          divestiture.acquisitionRequestId);
      if (selected == instance.pendingAttributeOwnershipAcquisitionRequests.end() ||
          selected->second.requestingFederateId != divestiture.acquiringFederateId ||
          !selected->second.desiredAttributeHandles.contains(attributeHandle) ||
          !federation.members.contains(divestiture.acquiringFederateId) ||
          !instance.knownObjectClassHandlesByFederate.contains(divestiture.acquiringFederateId)) {
        selected = instance.pendingAttributeOwnershipAcquisitionRequests.end();
        divestiture.acquiringFederateId = 0;
        divestiture.acquisitionRequestId = 0;
        divestiture.confirmationQueued = false;
        divestiture.confirmationDelivered = false;
      }
    }

    if (selected == instance.pendingAttributeOwnershipAcquisitionRequests.end()) {
      for (auto candidate = instance.pendingAttributeOwnershipAcquisitionRequests.begin();
           candidate != instance.pendingAttributeOwnershipAcquisitionRequests.end();
           ++candidate) {
        if (!candidate->second.desiredAttributeHandles.contains(attributeHandle) ||
            !federation.members.contains(candidate->second.requestingFederateId) ||
            !instance.knownObjectClassHandlesByFederate.contains(
                candidate->second.requestingFederateId)) {
          continue;
        }
        if (selected == instance.pendingAttributeOwnershipAcquisitionRequests.end() ||
            candidate->second.requestSequence < selected->second.requestSequence) {
          selected = candidate;
        }
      }
      if (selected == instance.pendingAttributeOwnershipAcquisitionRequests.end()) {
        continue;
      }
      divestiture.acquiringFederateId = selected->second.requestingFederateId;
      divestiture.acquisitionRequestId = selected->first;
      divestiture.confirmationQueued = false;
      divestiture.confirmationDelivered = false;
    }

    if (divestiture.confirmationQueued || divestiture.confirmationDelivered) {
      continue;
    }
    auto const callbackRoute = federation.interactionCallbackRoutes.find(
        divestiture.divestingFederateId);
    auto const acquiringRoute = federation.interactionCallbackRoutes.find(
        divestiture.acquiringFederateId);
    if (callbackRoute == federation.interactionCallbackRoutes.end() || !callbackRoute->second ||
        acquiringRoute == federation.interactionCallbackRoutes.end() || !acquiringRoute->second) {
      continue;
    }
    attributesByDivesterAndRequest[
        {divestiture.divestingFederateId, divestiture.acquisitionRequestId}]
        .insert(attributeHandle);
    divestiture.confirmationQueued = true;
  }

  std::vector<AttributeOwnershipAcquisitionWorkItem> workItems;
  workItems.reserve(attributesByDivesterAndRequest.size());
  for (auto& [key, attributeHandles] : attributesByDivesterAndRequest) {
    auto const [divestingFederateId, acquisitionRequestId] = key;
    auto const pending = instance.pendingAttributeOwnershipAcquisitionRequests.find(
        acquisitionRequestId);
    auto const callbackRoute = federation.interactionCallbackRoutes.find(divestingFederateId);
    if (pending == instance.pendingAttributeOwnershipAcquisitionRequests.end() ||
        callbackRoute == federation.interactionCallbackRoutes.end() || !callbackRoute->second ||
        attributeHandles.empty()) {
      continue;
    }
    workItems.push_back({
        AttributeOwnershipAcquisitionWorkKind::request_divestiture_confirmation,
        pending->second.requestingFederateId,
        divestingFederateId,
        instance.handle,
        acquisitionRequestId,
        std::move(attributeHandles),
        pending->second.userSuppliedTag,
        callbackRoute->second,
    });
  }
  return workItems;
}

std::vector<AttributeOwnershipAcquisitionWorkItem>
EmbeddedFederationRegistry::planPendingAttributeOwnershipAcquisitionWork(
    Federation& federation,
    Federation::ObjectInstance& instance) {
  std::vector<AttributeOwnershipAcquisitionWorkItem> workItems;

  for (auto& [requestId, pending] : instance.pendingAttributeOwnershipAcquisitionRequests) {
    if (!federation.members.contains(pending.requestingFederateId) ||
        !instance.knownObjectClassHandlesByFederate.contains(pending.requestingFederateId)) {
      continue;
    }

    std::set<std::uint64_t> notificationAttributes;
    for (std::uint64_t const attributeHandle : pending.desiredAttributeHandles) {
      if (instance.attributeOwnersByHandle.contains(attributeHandle) ||
          pending.notificationQueuedAttributeHandles.contains(attributeHandle) ||
          !isFirstPendingAttributeOwnershipAcquisition(instance, requestId, attributeHandle)) {
        continue;
      }
      notificationAttributes.insert(attributeHandle);
    }
    if (!notificationAttributes.empty()) {
      auto const callbackRoute = federation.interactionCallbackRoutes.find(
          pending.requestingFederateId);
      if (callbackRoute != federation.interactionCallbackRoutes.end() && callbackRoute->second) {
        pending.notificationQueuedAttributeHandles.insert(
            notificationAttributes.begin(), notificationAttributes.end());
        workItems.push_back({
            AttributeOwnershipAcquisitionWorkKind::acquisition_notification,
            pending.requestingFederateId,
            pending.requestingFederateId,
            instance.handle,
            requestId,
            std::move(notificationAttributes),
            pending.userSuppliedTag,
            callbackRoute->second,
        });
      }
    }

    std::map<std::uint64_t, std::set<std::uint64_t>> releaseAttributesByOwner;
    for (std::uint64_t const attributeHandle : pending.desiredAttributeHandles) {
      auto const owner = instance.attributeOwnersByHandle.find(attributeHandle);
      if (owner == instance.attributeOwnersByHandle.end() ||
          owner->second == pending.requestingFederateId ||
          !federation.members.contains(owner->second)) {
        continue;
      }
      // A waiting negotiated divestiture replaces the ordinary owner-side
      // release request with Request Divestiture Confirmation for the selected
      // regular acquisition. A previously queued release work item is
      // suppressed and consumed at its callback boundary while this state is
      // active.
      if (instance.pendingNegotiatedAttributeOwnershipDivestitures.contains(attributeHandle)) {
        continue;
      }
      auto const queuedAtOwner = pending.releaseCallbacksQueuedByOwningFederate.find(owner->second);
      if (queuedAtOwner != pending.releaseCallbacksQueuedByOwningFederate.end() &&
          queuedAtOwner->second.contains(attributeHandle)) {
        continue;
      }
      releaseAttributesByOwner[owner->second].insert(attributeHandle);
    }

    for (auto& [owningFederateId, releaseAttributes] : releaseAttributesByOwner) {
      auto const callbackRoute = federation.interactionCallbackRoutes.find(owningFederateId);
      if (callbackRoute == federation.interactionCallbackRoutes.end() || !callbackRoute->second) {
        continue;
      }
      auto& queuedAttributes =
          pending.releaseCallbacksQueuedByOwningFederate[owningFederateId];
      queuedAttributes.insert(releaseAttributes.begin(), releaseAttributes.end());
      workItems.push_back({
          AttributeOwnershipAcquisitionWorkKind::request_release,
          pending.requestingFederateId,
          owningFederateId,
          instance.handle,
          requestId,
          std::move(releaseAttributes),
          pending.userSuppliedTag,
          callbackRoute->second,
      });
    }
  }

  auto confirmationWorkItems =
      planPendingNegotiatedAttributeOwnershipDivestitureConfirmations(federation, instance);
  for (auto& workItem : confirmationWorkItems) {
    workItems.push_back(std::move(workItem));
  }

  return workItems;
}

AttributeOwnershipAcquisitionPlan
EmbeddedFederationRegistry::planAttributeOwnershipAcquisition(
    std::wstring const& federationName,
    std::uint64_t requestingFederateId,
    std::uint64_t objectInstanceHandle,
    std::set<std::uint64_t> const& desiredAttributeHandles,
    std::vector<unsigned char> userSuppliedTag) {
  std::scoped_lock lock(mutex_);
  auto federation = federations_.find(federationName);
  if (federation == federations_.end()) {
    return {AttributeOwnershipAcquisitionStatus::federation_does_not_exist};
  }
  if (!federation->second.members.contains(requestingFederateId)) {
    return {AttributeOwnershipAcquisitionStatus::requesting_federate_not_member};
  }
  auto instance = federation->second.objectInstances.find(objectInstanceHandle);
  if (instance == federation->second.objectInstances.end() || instance->second.deleteAccepted ||
      !instance->second.knownObjectClassHandlesByFederate.contains(requestingFederateId)) {
    return {AttributeOwnershipAcquisitionStatus::object_instance_not_known};
  }
  if (!federation->second.definition.catalog ||
      !federation->second.objectClassHandles ||
      !federation->second.attributeHandles) {
    return {AttributeOwnershipAcquisitionStatus::inconsistent_catalog};
  }

  auto const knownClass = instance->second.knownObjectClassHandlesByFederate.find(
      requestingFederateId);
  auto const knownClassName = federation->second.objectClassHandles->nameFor(knownClass->second);
  if (!knownClassName ||
      federation->second.definition.catalog->objectClass(*knownClassName) == nullptr) {
    return {AttributeOwnershipAcquisitionStatus::inconsistent_catalog};
  }
  for (std::uint64_t const attributeHandle : desiredAttributeHandles) {
    if (!federation->second.attributeHandles->nameFor(
            federation->second.definition.catalog.get(),
            *knownClassName,
            attributeHandle)) {
      return {AttributeOwnershipAcquisitionStatus::attribute_not_defined};
    }
  }

  // An empty request changes neither ownership nor the private Acquisition
  // Pending state chart. Its object/federate preconditions above still apply.
  if (desiredAttributeHandles.empty()) {
    return {};
  }

  auto const publishedAttributes = publishedObjectClassAttributes(
      federation->second,
      requestingFederateId,
      knownClass->second);
  if (!publishedAttributes) {
    return {AttributeOwnershipAcquisitionStatus::inconsistent_catalog};
  }
  if (publishedAttributes->empty()) {
    return {AttributeOwnershipAcquisitionStatus::object_class_not_published};
  }
  for (std::uint64_t const attributeHandle : desiredAttributeHandles) {
    if (!publishedAttributes->contains(attributeHandle)) {
      return {AttributeOwnershipAcquisitionStatus::attribute_not_published};
    }
  }

  for (std::uint64_t const attributeHandle : desiredAttributeHandles) {
    auto const owner = instance->second.attributeOwnersByHandle.find(attributeHandle);
    if (owner == instance->second.attributeOwnersByHandle.end()) {
      continue;
    }
    if (owner->second == requestingFederateId) {
      return {AttributeOwnershipAcquisitionStatus::federate_owns_attributes};
    }
    if (!federation->second.members.contains(owner->second)) {
      // Complete resign-action ownership disposition remains outside the
      // limited embedded profile. Do not claim a normal acquisition outcome
      // from an owner record whose federate has already departed.
      return {AttributeOwnershipAcquisitionStatus::inconsistent_catalog};
    }
  }

  std::set<std::uint64_t> newlyRequestedAttributeHandles = desiredAttributeHandles;
  for (auto const& [requestId, pending] :
       instance->second.pendingAttributeOwnershipAcquisitionRequests) {
    static_cast<void>(requestId);
    if (pending.requestingFederateId != requestingFederateId) {
      continue;
    }
    for (std::uint64_t const attributeHandle : pending.desiredAttributeHandles) {
      newlyRequestedAttributeHandles.erase(attributeHandle);
    }
  }
  for (auto const& [cancellationId, cancellation] :
       instance->second.pendingAttributeOwnershipAcquisitionCancellations) {
    static_cast<void>(cancellationId);
    if (cancellation.requestingFederateId != requestingFederateId) {
      continue;
    }
    for (std::uint64_t const attributeHandle : cancellation.attributeHandles) {
      newlyRequestedAttributeHandles.erase(attributeHandle);
    }
  }

  // IEEE 1516.1-2025 7.8 preserves the Acquiring state for a repeat regular
  // request. The existing pending record remains authoritative and no second
  // owner-side release callback is queued for those attributes.
  if (newlyRequestedAttributeHandles.empty()) {
    return {};
  }

  if (federation->second.nextAttributeOwnershipAcquisitionRequestId == 0 ||
      federation->second.nextAttributeOwnershipAcquisitionRequestId ==
          std::numeric_limits<std::uint64_t>::max() ||
      federation->second.nextAttributeOwnershipAcquisitionRequestSequence == 0 ||
      federation->second.nextAttributeOwnershipAcquisitionRequestSequence ==
          std::numeric_limits<std::uint64_t>::max()) {
    return {AttributeOwnershipAcquisitionStatus::inconsistent_catalog};
  }
  auto const requesterCallbackRoute = federation->second.interactionCallbackRoutes.find(
      requestingFederateId);
  if (requesterCallbackRoute == federation->second.interactionCallbackRoutes.end() ||
      !requesterCallbackRoute->second) {
    return {AttributeOwnershipAcquisitionStatus::inconsistent_catalog};
  }
  for (std::uint64_t const attributeHandle : newlyRequestedAttributeHandles) {
    auto const owner = instance->second.attributeOwnersByHandle.find(attributeHandle);
    if (owner == instance->second.attributeOwnersByHandle.end()) {
      continue;
    }
    auto const ownerCallbackRoute = federation->second.interactionCallbackRoutes.find(owner->second);
    if (ownerCallbackRoute == federation->second.interactionCallbackRoutes.end() ||
        !ownerCallbackRoute->second) {
      return {AttributeOwnershipAcquisitionStatus::inconsistent_catalog};
    }
  }

  std::uint64_t const requestId = federation->second.nextAttributeOwnershipAcquisitionRequestId;
  std::uint64_t const requestSequence =
      federation->second.nextAttributeOwnershipAcquisitionRequestSequence;
  auto const [pending, inserted] =
      instance->second.pendingAttributeOwnershipAcquisitionRequests.try_emplace(
          requestId,
          Federation::ObjectInstance::PendingAttributeOwnershipAcquisition{
              requestingFederateId,
              requestSequence,
              std::move(newlyRequestedAttributeHandles),
              {},
              {},
              std::move(userSuppliedTag),
          });
  static_cast<void>(pending);
  if (!inserted) {
    return {AttributeOwnershipAcquisitionStatus::inconsistent_catalog};
  }
  ++federation->second.nextAttributeOwnershipAcquisitionRequestId;
  ++federation->second.nextAttributeOwnershipAcquisitionRequestSequence;

  // A regular acquisition by this same federate and attribute takes
  // precedence over an earlier If Available request. Erasing the private WTA
  // reservation also makes its already queued terminal work a harmless
  // no-delivery outcome.
  for (auto ifAvailable =
           instance->second.pendingAttributeOwnershipAcquisitionIfAvailableRequests.begin();
       ifAvailable !=
       instance->second.pendingAttributeOwnershipAcquisitionIfAvailableRequests.end();) {
    if (ifAvailable->second.requestingFederateId != requestingFederateId) {
      ++ifAvailable;
      continue;
    }
    for (std::uint64_t const attributeHandle : desiredAttributeHandles) {
      ifAvailable->second.desiredAttributeHandles.erase(attributeHandle);
    }
    if (ifAvailable->second.desiredAttributeHandles.empty()) {
      ifAvailable =
          instance->second.pendingAttributeOwnershipAcquisitionIfAvailableRequests.erase(
              ifAvailable);
    } else {
      ++ifAvailable;
    }
  }

  return {
      AttributeOwnershipAcquisitionStatus::applied,
      planPendingAttributeOwnershipAcquisitionWork(federation->second, instance->second),
  };
}

std::optional<AttributeOwnershipAcquisitionNotificationDelivery>
EmbeddedFederationRegistry::beginAttributeOwnershipAcquisitionNotification(
    std::wstring const& federationName,
    std::uint64_t requestingFederateId,
    std::uint64_t objectInstanceHandle,
    std::uint64_t requestId,
    std::set<std::uint64_t> const& scheduledAttributeHandles) {
  if (requestId == 0 || scheduledAttributeHandles.empty()) {
    return std::nullopt;
  }

  std::scoped_lock lock(mutex_);
  auto federation = federations_.find(federationName);
  if (federation == federations_.end()) {
    return std::nullopt;
  }
  auto instance = federation->second.objectInstances.find(objectInstanceHandle);
  if (instance == federation->second.objectInstances.end()) {
    return std::nullopt;
  }
  auto pending = instance->second.pendingAttributeOwnershipAcquisitionRequests.find(requestId);
  if (pending == instance->second.pendingAttributeOwnershipAcquisitionRequests.end() ||
      pending->second.requestingFederateId != requestingFederateId) {
    return std::nullopt;
  }

  auto erasePendingAndPlanFollowup = [&] {
    instance->second.pendingAttributeOwnershipAcquisitionRequests.erase(pending);
    return planPendingAttributeOwnershipAcquisitionWork(federation->second, instance->second);
  };
  if (instance->second.deleteAccepted ||
      !federation->second.members.contains(requestingFederateId) ||
      !instance->second.knownObjectClassHandlesByFederate.contains(requestingFederateId) ||
      !federation->second.definition.catalog ||
      !federation->second.objectClassHandles ||
      !federation->second.attributeHandles) {
    auto followupWorkItems = erasePendingAndPlanFollowup();
    if (followupWorkItems.empty()) {
      return std::nullopt;
    }
    return AttributeOwnershipAcquisitionNotificationDelivery{
        objectInstanceHandle,
        {},
        std::move(followupWorkItems),
    };
  }

  auto const knownClass = instance->second.knownObjectClassHandlesByFederate.find(
      requestingFederateId);
  auto const knownClassName = federation->second.objectClassHandles->nameFor(knownClass->second);
  if (!knownClassName ||
      federation->second.definition.catalog->objectClass(*knownClassName) == nullptr) {
    auto followupWorkItems = erasePendingAndPlanFollowup();
    if (followupWorkItems.empty()) {
      return std::nullopt;
    }
    return AttributeOwnershipAcquisitionNotificationDelivery{
        objectInstanceHandle,
        {},
        std::move(followupWorkItems),
    };
  }
  auto const publishedAttributes = publishedObjectClassAttributes(
      federation->second,
      requestingFederateId,
      knownClass->second);
  if (!publishedAttributes) {
    auto followupWorkItems = erasePendingAndPlanFollowup();
    if (followupWorkItems.empty()) {
      return std::nullopt;
    }
    return AttributeOwnershipAcquisitionNotificationDelivery{
        objectInstanceHandle,
        {},
        std::move(followupWorkItems),
    };
  }
  for (std::uint64_t const attributeHandle : pending->second.desiredAttributeHandles) {
    if (!federation->second.attributeHandles->nameFor(
            federation->second.definition.catalog.get(),
            *knownClassName,
            attributeHandle) ||
        !publishedAttributes->contains(attributeHandle)) {
      auto followupWorkItems = erasePendingAndPlanFollowup();
      if (followupWorkItems.empty()) {
        return std::nullopt;
      }
      return AttributeOwnershipAcquisitionNotificationDelivery{
          objectInstanceHandle,
          {},
          std::move(followupWorkItems),
      };
    }
  }

  std::set<std::uint64_t> securedAttributeHandles;
  for (std::uint64_t const attributeHandle : scheduledAttributeHandles) {
    pending->second.notificationQueuedAttributeHandles.erase(attributeHandle);
    if (!pending->second.desiredAttributeHandles.contains(attributeHandle)) {
      continue;
    }
    if (instance->second.pendingNegotiatedAttributeOwnershipDivestitures.contains(
            attributeHandle)) {
      // A negotiated divestiture began after this regular notification was
      // queued. The attribute remains owned until Confirm Divestiture, so this
      // stale unowned-acquisition boundary cannot transfer it.
      continue;
    }
    auto const owner = instance->second.attributeOwnersByHandle.find(attributeHandle);
    if (owner != instance->second.attributeOwnersByHandle.end() &&
        owner->second != requestingFederateId) {
      continue;
    }
    if (owner == instance->second.attributeOwnersByHandle.end() &&
        !isFirstPendingAttributeOwnershipAcquisition(
            instance->second,
            requestId,
            attributeHandle)) {
      continue;
    }
    if (owner == instance->second.attributeOwnersByHandle.end()) {
      instance->second.attributeOwnersByHandle.emplace(attributeHandle, requestingFederateId);
    }
    pending->second.desiredAttributeHandles.erase(attributeHandle);
    securedAttributeHandles.insert(attributeHandle);
  }

  if (pending->second.desiredAttributeHandles.empty()) {
    instance->second.pendingAttributeOwnershipAcquisitionRequests.erase(pending);
  }
  auto followupWorkItems = planPendingAttributeOwnershipAcquisitionWork(
      federation->second,
      instance->second);
  if (securedAttributeHandles.empty() && followupWorkItems.empty()) {
    return std::nullopt;
  }
  return AttributeOwnershipAcquisitionNotificationDelivery{
      objectInstanceHandle,
      std::move(securedAttributeHandles),
      std::move(followupWorkItems),
  };
}

std::optional<AttributeOwnershipAcquisitionReleaseDelivery>
EmbeddedFederationRegistry::beginAttributeOwnershipAcquisitionRelease(
    std::wstring const& federationName,
    std::uint64_t requestingFederateId,
    std::uint64_t owningFederateId,
    std::uint64_t objectInstanceHandle,
    std::uint64_t requestId,
    std::set<std::uint64_t> const& scheduledAttributeHandles) {
  if (requestId == 0 || scheduledAttributeHandles.empty()) {
    return std::nullopt;
  }

  std::scoped_lock lock(mutex_);
  auto federation = federations_.find(federationName);
  if (federation == federations_.end() ||
      !federation->second.members.contains(requestingFederateId) ||
      !federation->second.members.contains(owningFederateId)) {
    return std::nullopt;
  }
  auto instance = federation->second.objectInstances.find(objectInstanceHandle);
  if (instance == federation->second.objectInstances.end() || instance->second.deleteAccepted ||
      !instance->second.knownObjectClassHandlesByFederate.contains(owningFederateId) ||
      !instance->second.knownObjectClassHandlesByFederate.contains(requestingFederateId)) {
    return std::nullopt;
  }
  auto pending = instance->second.pendingAttributeOwnershipAcquisitionRequests.find(requestId);
  if (pending == instance->second.pendingAttributeOwnershipAcquisitionRequests.end() ||
      pending->second.requestingFederateId != requestingFederateId) {
    return std::nullopt;
  }

  std::set<std::uint64_t> candidateAttributeHandles;
  for (std::uint64_t const attributeHandle : scheduledAttributeHandles) {
    if (!pending->second.desiredAttributeHandles.contains(attributeHandle)) {
      continue;
    }
    auto queuedAtOwner = pending->second.releaseCallbacksQueuedByOwningFederate.find(
        owningFederateId);
    if (queuedAtOwner == pending->second.releaseCallbacksQueuedByOwningFederate.end() ||
        !queuedAtOwner->second.contains(attributeHandle)) {
      // This work item was superseded before the callback boundary.
      continue;
    }

    if (instance->second.pendingNegotiatedAttributeOwnershipDivestitures.contains(
            attributeHandle)) {
      // The owner entered the negotiated Waiting state after this ordinary
      // release callback was queued. It is now consumed and suppressed; the
      // confirmation path owns the next callback decision for this attribute.
      // Ordinary release reservations otherwise remain in place after their
      // callback: the original acquisition is still pending until a terminal
      // ownership service responds, and normal follow-up planning must not
      // enqueue a duplicate release request.
      queuedAtOwner->second.erase(attributeHandle);
      if (queuedAtOwner->second.empty()) {
        pending->second.releaseCallbacksQueuedByOwningFederate.erase(queuedAtOwner);
      }
      continue;
    }
    auto const owner = instance->second.attributeOwnersByHandle.find(attributeHandle);
    if (owner != instance->second.attributeOwnersByHandle.end() &&
        owner->second == owningFederateId) {
      candidateAttributeHandles.insert(attributeHandle);
    }
  }
  if (candidateAttributeHandles.empty()) {
    return std::nullopt;
  }
  return AttributeOwnershipAcquisitionReleaseDelivery{
      objectInstanceHandle,
      std::move(candidateAttributeHandles),
  };
}

AttributeOwnershipReleaseDeniedPlan
EmbeddedFederationRegistry::planAttributeOwnershipReleaseDenied(
    std::wstring const& federationName,
    std::uint64_t owningFederateId,
    std::uint64_t objectInstanceHandle,
    std::set<std::uint64_t> const& attributeHandles) {
  std::scoped_lock lock(mutex_);
  auto federation = federations_.find(federationName);
  if (federation == federations_.end()) {
    return {AttributeOwnershipReleaseDeniedStatus::federation_does_not_exist};
  }
  if (!federation->second.members.contains(owningFederateId)) {
    return {AttributeOwnershipReleaseDeniedStatus::owning_federate_not_member};
  }
  auto instance = federation->second.objectInstances.find(objectInstanceHandle);
  if (instance == federation->second.objectInstances.end() || instance->second.deleteAccepted ||
      !instance->second.knownObjectClassHandlesByFederate.contains(owningFederateId)) {
    return {AttributeOwnershipReleaseDeniedStatus::object_instance_not_known};
  }
  if (!federation->second.definition.catalog ||
      !federation->second.objectClassHandles ||
      !federation->second.attributeHandles) {
    return {AttributeOwnershipReleaseDeniedStatus::inconsistent_catalog};
  }

  auto const knownClass = instance->second.knownObjectClassHandlesByFederate.find(
      owningFederateId);
  auto const knownClassName = federation->second.objectClassHandles->nameFor(knownClass->second);
  if (!knownClassName ||
      federation->second.definition.catalog->objectClass(*knownClassName) == nullptr) {
    return {AttributeOwnershipReleaseDeniedStatus::inconsistent_catalog};
  }
  for (std::uint64_t const attributeHandle : attributeHandles) {
    if (!federation->second.attributeHandles->nameFor(
            federation->second.definition.catalog.get(),
            *knownClassName,
            attributeHandle)) {
      return {AttributeOwnershipReleaseDeniedStatus::attribute_not_defined};
    }
    auto const owner = instance->second.attributeOwnersByHandle.find(attributeHandle);
    if (owner == instance->second.attributeOwnersByHandle.end() ||
        owner->second != owningFederateId) {
      return {AttributeOwnershipReleaseDeniedStatus::attribute_not_owned};
    }
  }

  std::map<std::uint64_t, std::set<std::uint64_t>> attributesByRequestingFederate;
  for (auto const& [requestId, pending] :
       instance->second.pendingAttributeOwnershipAcquisitionRequests) {
    static_cast<void>(requestId);
    for (std::uint64_t const attributeHandle : pending.desiredAttributeHandles) {
      if (attributeHandles.contains(attributeHandle) &&
          !instance->second.pendingNegotiatedAttributeOwnershipDivestitures.contains(
              attributeHandle)) {
        attributesByRequestingFederate[pending.requestingFederateId].insert(attributeHandle);
      }
    }
  }
  for (auto const& [requestingFederateId, ignoredAttributes] :
       attributesByRequestingFederate) {
    static_cast<void>(ignoredAttributes);
    auto const callbackRoute = federation->second.interactionCallbackRoutes.find(
        requestingFederateId);
    if (!federation->second.members.contains(requestingFederateId) ||
        callbackRoute == federation->second.interactionCallbackRoutes.end() ||
        !callbackRoute->second) {
      return {AttributeOwnershipReleaseDeniedStatus::inconsistent_catalog};
    }
  }

  for (auto pending = instance->second.pendingAttributeOwnershipAcquisitionRequests.begin();
       pending != instance->second.pendingAttributeOwnershipAcquisitionRequests.end();) {
    for (std::uint64_t const attributeHandle : attributeHandles) {
      if (instance->second.pendingNegotiatedAttributeOwnershipDivestitures.contains(
              attributeHandle)) {
        continue;
      }
      pending->second.desiredAttributeHandles.erase(attributeHandle);
      pending->second.notificationQueuedAttributeHandles.erase(attributeHandle);
      for (auto release = pending->second.releaseCallbacksQueuedByOwningFederate.begin();
           release != pending->second.releaseCallbacksQueuedByOwningFederate.end();) {
        release->second.erase(attributeHandle);
        if (release->second.empty()) {
          release = pending->second.releaseCallbacksQueuedByOwningFederate.erase(release);
        } else {
          ++release;
        }
      }
    }
    if (pending->second.desiredAttributeHandles.empty()) {
      pending = instance->second.pendingAttributeOwnershipAcquisitionRequests.erase(pending);
    } else {
      ++pending;
    }
  }

  AttributeOwnershipReleaseDeniedPlan result;
  for (auto& [requestingFederateId, unavailableAttributes] : attributesByRequestingFederate) {
    auto const callbackRoute = federation->second.interactionCallbackRoutes.find(
        requestingFederateId);
    result.recipients.push_back({
        requestingFederateId,
        objectInstanceHandle,
        std::move(unavailableAttributes),
        callbackRoute->second,
    });
  }
  return result;
}

UnconditionalAttributeOwnershipDivestiturePlan
EmbeddedFederationRegistry::planUnconditionalAttributeOwnershipDivestiture(
    std::wstring const& federationName,
    std::uint64_t divestingFederateId,
    std::uint64_t objectInstanceHandle,
    std::set<std::uint64_t> const& attributeHandles) {
  std::scoped_lock lock(mutex_);
  auto federation = federations_.find(federationName);
  if (federation == federations_.end()) {
    return {UnconditionalAttributeOwnershipDivestitureStatus::federation_does_not_exist};
  }
  if (!federation->second.members.contains(divestingFederateId)) {
    return {UnconditionalAttributeOwnershipDivestitureStatus::divesting_federate_not_member};
  }
  auto instance = federation->second.objectInstances.find(objectInstanceHandle);
  if (instance == federation->second.objectInstances.end() || instance->second.deleteAccepted ||
      !instance->second.knownObjectClassHandlesByFederate.contains(divestingFederateId)) {
    return {UnconditionalAttributeOwnershipDivestitureStatus::object_instance_not_known};
  }
  if (!federation->second.definition.catalog ||
      !federation->second.objectClassHandles ||
      !federation->second.attributeHandles) {
    return {UnconditionalAttributeOwnershipDivestitureStatus::inconsistent_catalog};
  }

  auto const divestingKnownClass = instance->second.knownObjectClassHandlesByFederate.find(
      divestingFederateId);
  auto const divestingKnownClassName = federation->second.objectClassHandles->nameFor(
      divestingKnownClass->second);
  if (!divestingKnownClassName ||
      federation->second.definition.catalog->objectClass(*divestingKnownClassName) == nullptr) {
    return {UnconditionalAttributeOwnershipDivestitureStatus::inconsistent_catalog};
  }
  for (std::uint64_t const attributeHandle : attributeHandles) {
    if (!federation->second.attributeHandles->nameFor(
            federation->second.definition.catalog.get(),
            *divestingKnownClassName,
            attributeHandle)) {
      return {UnconditionalAttributeOwnershipDivestitureStatus::attribute_not_defined};
    }
    auto const owner = instance->second.attributeOwnersByHandle.find(attributeHandle);
    if (owner == instance->second.attributeOwnersByHandle.end() ||
        owner->second != divestingFederateId) {
      return {UnconditionalAttributeOwnershipDivestitureStatus::attribute_not_owned};
    }
  }

  // An empty supplied set has no ownership, pending-acquisition, or callback
  // effect after the common connection/member/known-instance validation.
  if (attributeHandles.empty()) {
    return {};
  }

  auto recipientHasPendingAcquisition = [&instance](
                                          std::uint64_t receivingFederateId,
                                          std::uint64_t attributeHandle) {
    for (auto const& [requestId, pending] :
         instance->second.pendingAttributeOwnershipAcquisitionIfAvailableRequests) {
      static_cast<void>(requestId);
      if (pending.requestingFederateId == receivingFederateId &&
          pending.desiredAttributeHandles.contains(attributeHandle)) {
        return true;
      }
    }
    for (auto const& [requestId, pending] :
         instance->second.pendingAttributeOwnershipAcquisitionRequests) {
      static_cast<void>(requestId);
      if (pending.requestingFederateId == receivingFederateId &&
          pending.desiredAttributeHandles.contains(attributeHandle)) {
        return true;
      }
    }
    for (auto const& [cancellationId, cancellation] :
         instance->second.pendingAttributeOwnershipAcquisitionCancellations) {
      static_cast<void>(cancellationId);
      if (cancellation.requestingFederateId == receivingFederateId &&
          cancellation.attributeHandles.contains(attributeHandle)) {
        // Keep a cancellation confirmation terminal: an old offer must not
        // race ahead of the corresponding callback boundary.
        return true;
      }
    }
    return false;
  };

  // Complete every potentially allocating candidate lookup before the
  // unconditional state transition. A recipient needs a known class, its
  // corresponding attribute published there, and no still-pending acquisition
  // state for that attribute. Existing regular/If Available requesters are
  // resolved through their own standard callback paths below rather than being
  // offered a duplicate Request Attribute Ownership Assumption callback.
  std::map<std::uint64_t, std::set<std::uint64_t>> offeredAttributesByFederate;
  for (auto const& [receivingFederateId, membership] : federation->second.members) {
    static_cast<void>(membership);
    if (receivingFederateId == divestingFederateId) {
      continue;
    }
    auto const receivingKnownClass = instance->second.knownObjectClassHandlesByFederate.find(
        receivingFederateId);
    if (receivingKnownClass == instance->second.knownObjectClassHandlesByFederate.end()) {
      continue;
    }
    auto const receivingKnownClassName = federation->second.objectClassHandles->nameFor(
        receivingKnownClass->second);
    if (!receivingKnownClassName ||
        federation->second.definition.catalog->objectClass(*receivingKnownClassName) == nullptr) {
      return {UnconditionalAttributeOwnershipDivestitureStatus::inconsistent_catalog};
    }
    auto const publishedAttributes = publishedObjectClassAttributes(
        federation->second,
        receivingFederateId,
        receivingKnownClass->second);
    if (!publishedAttributes) {
      return {UnconditionalAttributeOwnershipDivestitureStatus::inconsistent_catalog};
    }
    for (std::uint64_t const attributeHandle : attributeHandles) {
      if (!federation->second.attributeHandles->nameFor(
              federation->second.definition.catalog.get(),
              *receivingKnownClassName,
              attributeHandle) ||
          !publishedAttributes->contains(attributeHandle) ||
          recipientHasPendingAcquisition(receivingFederateId, attributeHandle)) {
        continue;
      }
      offeredAttributesByFederate[receivingFederateId].insert(attributeHandle);
    }
  }

  UnconditionalAttributeOwnershipDivestiturePlan result;
  result.assumptionRecipients.reserve(offeredAttributesByFederate.size());
  for (auto& [receivingFederateId, offeredAttributes] : offeredAttributesByFederate) {
    auto const callbackRoute = federation->second.interactionCallbackRoutes.find(
        receivingFederateId);
    if (callbackRoute == federation->second.interactionCallbackRoutes.end() ||
        !callbackRoute->second) {
      return {UnconditionalAttributeOwnershipDivestitureStatus::inconsistent_catalog};
    }
    result.assumptionRecipients.push_back({
        receivingFederateId,
        objectInstanceHandle,
        std::move(offeredAttributes),
        callbackRoute->second,
    });
  }

  // IEEE 1516.1-2025 §7.2 makes the supplied attributes unowned immediately;
  // no accepting federate is required for this transition. Existing regular
  // acquisition work is then replanned from this new unowned state, while an
  // existing If Available reservation retains the callback it already owns.
  for (std::uint64_t const attributeHandle : attributeHandles) {
    instance->second.attributeOwnersByHandle.erase(attributeHandle);
    // A successful unconditional divestiture ends any earlier negotiated
    // waiting state for the same attribute. Its queued confirmation callback
    // will recheck this missing state and become harmless.
    instance->second.pendingNegotiatedAttributeOwnershipDivestitures.erase(attributeHandle);
  }
  result.acquisitionWorkItems = planPendingAttributeOwnershipAcquisitionWork(
      federation->second,
      instance->second);
  return result;
}

std::optional<AttributeOwnershipAssumptionDelivery>
EmbeddedFederationRegistry::attributeOwnershipAssumptionDeliveryFor(
    std::wstring const& federationName,
    std::uint64_t receivingFederateId,
    std::uint64_t objectInstanceHandle,
    std::set<std::uint64_t> const& scheduledAttributeHandles) const {
  if (scheduledAttributeHandles.empty()) {
    return std::nullopt;
  }

  std::scoped_lock lock(mutex_);
  auto const federation = federations_.find(federationName);
  if (federation == federations_.end() ||
      !federation->second.members.contains(receivingFederateId)) {
    return std::nullopt;
  }
  auto const instance = federation->second.objectInstances.find(objectInstanceHandle);
  if (instance == federation->second.objectInstances.end() || instance->second.deleteAccepted ||
      !instance->second.knownObjectClassHandlesByFederate.contains(receivingFederateId) ||
      !federation->second.definition.catalog ||
      !federation->second.objectClassHandles ||
      !federation->second.attributeHandles) {
    return std::nullopt;
  }

  auto const receivingKnownClass = instance->second.knownObjectClassHandlesByFederate.find(
      receivingFederateId);
  auto const receivingKnownClassName = federation->second.objectClassHandles->nameFor(
      receivingKnownClass->second);
  if (!receivingKnownClassName ||
      federation->second.definition.catalog->objectClass(*receivingKnownClassName) == nullptr) {
    return std::nullopt;
  }
  auto const publishedAttributes = publishedObjectClassAttributes(
      federation->second,
      receivingFederateId,
      receivingKnownClass->second);
  if (!publishedAttributes) {
    return std::nullopt;
  }

  AttributeOwnershipAssumptionDelivery delivery;
  delivery.objectInstanceHandle = objectInstanceHandle;
  for (std::uint64_t const attributeHandle : scheduledAttributeHandles) {
    if (!federation->second.attributeHandles->nameFor(
            federation->second.definition.catalog.get(),
            *receivingKnownClassName,
            attributeHandle) ||
        !publishedAttributes->contains(attributeHandle) ||
        instance->second.attributeOwnersByHandle.contains(attributeHandle)) {
      continue;
    }

    bool hasPendingAcquisition = false;
    for (auto const& [requestId, pending] :
         instance->second.pendingAttributeOwnershipAcquisitionIfAvailableRequests) {
      static_cast<void>(requestId);
      if (pending.requestingFederateId == receivingFederateId &&
          pending.desiredAttributeHandles.contains(attributeHandle)) {
        hasPendingAcquisition = true;
        break;
      }
    }
    if (!hasPendingAcquisition) {
      for (auto const& [requestId, pending] :
           instance->second.pendingAttributeOwnershipAcquisitionRequests) {
        static_cast<void>(requestId);
        if (pending.requestingFederateId == receivingFederateId &&
            pending.desiredAttributeHandles.contains(attributeHandle)) {
          hasPendingAcquisition = true;
          break;
        }
      }
    }
    if (!hasPendingAcquisition) {
      for (auto const& [cancellationId, cancellation] :
           instance->second.pendingAttributeOwnershipAcquisitionCancellations) {
        static_cast<void>(cancellationId);
        if (cancellation.requestingFederateId == receivingFederateId &&
            cancellation.attributeHandles.contains(attributeHandle)) {
          hasPendingAcquisition = true;
          break;
        }
      }
    }
    if (!hasPendingAcquisition) {
      delivery.attributeHandles.insert(attributeHandle);
    }
  }

  if (delivery.attributeHandles.empty()) {
    return std::nullopt;
  }
  return delivery;
}

AttributeOwnershipDivestitureIfWantedPlan
EmbeddedFederationRegistry::planAttributeOwnershipDivestitureIfWanted(
    std::wstring const& federationName,
    std::uint64_t divestingFederateId,
    std::uint64_t objectInstanceHandle,
    std::set<std::uint64_t> const& attributeHandles,
    std::vector<unsigned char> userSuppliedTag) {
  std::scoped_lock lock(mutex_);
  auto federation = federations_.find(federationName);
  if (federation == federations_.end()) {
    return {AttributeOwnershipDivestitureIfWantedStatus::federation_does_not_exist};
  }
  if (!federation->second.members.contains(divestingFederateId)) {
    return {AttributeOwnershipDivestitureIfWantedStatus::divesting_federate_not_member};
  }
  auto instance = federation->second.objectInstances.find(objectInstanceHandle);
  if (instance == federation->second.objectInstances.end() || instance->second.deleteAccepted ||
      !instance->second.knownObjectClassHandlesByFederate.contains(divestingFederateId)) {
    return {AttributeOwnershipDivestitureIfWantedStatus::object_instance_not_known};
  }
  if (!federation->second.definition.catalog ||
      !federation->second.objectClassHandles ||
      !federation->second.attributeHandles) {
    return {AttributeOwnershipDivestitureIfWantedStatus::inconsistent_catalog};
  }

  auto const knownClass = instance->second.knownObjectClassHandlesByFederate.find(
      divestingFederateId);
  auto const knownClassName = federation->second.objectClassHandles->nameFor(knownClass->second);
  if (!knownClassName ||
      federation->second.definition.catalog->objectClass(*knownClassName) == nullptr) {
    return {AttributeOwnershipDivestitureIfWantedStatus::inconsistent_catalog};
  }
  for (std::uint64_t const attributeHandle : attributeHandles) {
    if (!federation->second.attributeHandles->nameFor(
            federation->second.definition.catalog.get(),
            *knownClassName,
            attributeHandle)) {
      return {AttributeOwnershipDivestitureIfWantedStatus::attribute_not_defined};
    }
    auto const owner = instance->second.attributeOwnersByHandle.find(attributeHandle);
    if (owner == instance->second.attributeOwnersByHandle.end() ||
        owner->second != divestingFederateId) {
      return {AttributeOwnershipDivestitureIfWantedStatus::attribute_not_owned};
    }
  }

  // A Divestiture If Wanted call can legally conclude without moving any
  // ownership when no joined federate is attempting to acquire an attribute.
  // It nevertheless validates all supplied attributes above, because 7.1.5
  // makes a single invalid attribute fail the complete service invocation.
  if (attributeHandles.empty()) {
    return {};
  }

  enum class PendingRequestKind {
    regular,
    if_available,
  };
  struct SelectedAcquirer {
    PendingRequestKind kind = PendingRequestKind::regular;
    std::uint64_t requestId = 0;
    std::uint64_t receivingFederateId = 0;
    std::uint64_t requestSequence = 0;
  };

  // 1516.1-2025 requires a real joined acquirer before this service may
  // divest an attribute, but it does not prescribe an arbitration policy for
  // multiple eligible pending requests. This serial embedded profile chooses
  // the earliest accepted regular-or-WTA request using one private sequence,
  // which avoids implying a standards-level priority between request forms.
  auto const acquirerIsStillEligible = [
      &federation,
      &instance,
      divestingFederateId](std::uint64_t receivingFederateId,
                            std::uint64_t attributeHandle) {
    if (receivingFederateId == divestingFederateId ||
        !federation->second.members.contains(receivingFederateId)) {
      return false;
    }
    auto const knownReceivingClass =
        instance->second.knownObjectClassHandlesByFederate.find(receivingFederateId);
    if (knownReceivingClass == instance->second.knownObjectClassHandlesByFederate.end()) {
      return false;
    }
    auto const publishedAttributes = publishedObjectClassAttributes(
        federation->second,
        receivingFederateId,
        knownReceivingClass->second);
    return publishedAttributes && publishedAttributes->contains(attributeHandle);
  };

  std::map<std::uint64_t, SelectedAcquirer> selectedAcquirersByAttribute;
  for (std::uint64_t const attributeHandle : attributeHandles) {
    std::optional<SelectedAcquirer> selectedAcquirer;
    auto consider = [&selectedAcquirer, attributeHandle, &acquirerIsStillEligible](
                        PendingRequestKind kind,
                        std::uint64_t requestId,
                        auto const& pending) {
      if (!pending.desiredAttributeHandles.contains(attributeHandle) ||
          !acquirerIsStillEligible(pending.requestingFederateId, attributeHandle)) {
        return;
      }
      if (!selectedAcquirer || pending.requestSequence < selectedAcquirer->requestSequence) {
        selectedAcquirer = SelectedAcquirer{
            kind,
            requestId,
            pending.requestingFederateId,
            pending.requestSequence,
        };
      }
    };
    for (auto const& [requestId, pending] :
         instance->second.pendingAttributeOwnershipAcquisitionRequests) {
      consider(PendingRequestKind::regular, requestId, pending);
    }
    for (auto const& [requestId, pending] :
         instance->second.pendingAttributeOwnershipAcquisitionIfAvailableRequests) {
      consider(PendingRequestKind::if_available, requestId, pending);
    }
    if (selectedAcquirer) {
      selectedAcquirersByAttribute.emplace(attributeHandle, *selectedAcquirer);
    }
  }

  std::map<std::uint64_t, std::set<std::uint64_t>> attributesByReceivingFederate;
  for (auto const& [attributeHandle, selectedAcquirer] : selectedAcquirersByAttribute) {
    auto const callbackRoute = federation->second.interactionCallbackRoutes.find(
        selectedAcquirer.receivingFederateId);
    if (callbackRoute == federation->second.interactionCallbackRoutes.end() ||
        !callbackRoute->second) {
      return {AttributeOwnershipDivestitureIfWantedStatus::inconsistent_catalog};
    }
    attributesByReceivingFederate[selectedAcquirer.receivingFederateId].insert(attributeHandle);
  }

  if (attributesByReceivingFederate.empty()) {
    return {};
  }
  if (federation->second.nextAttributeOwnershipDivestitureIfWantedNotificationId == 0 ||
      federation->second.nextAttributeOwnershipDivestitureIfWantedNotificationId ==
          std::numeric_limits<std::uint64_t>::max() ||
      attributesByReceivingFederate.size() >
          std::numeric_limits<std::uint64_t>::max() -
              federation->second.nextAttributeOwnershipDivestitureIfWantedNotificationId) {
    return {AttributeOwnershipDivestitureIfWantedStatus::inconsistent_catalog};
  }

  struct PreparedNotification {
    std::uint64_t notificationId = 0;
    std::uint64_t receivingFederateId = 0;
    std::set<std::uint64_t> attributeHandles;
  };
  std::vector<PreparedNotification> preparedNotifications;
  preparedNotifications.reserve(attributesByReceivingFederate.size());

  AttributeOwnershipDivestitureIfWantedPlan result;
  result.notifications.reserve(attributesByReceivingFederate.size());
  for (auto const& [attributeHandle, selectedAcquirer] : selectedAcquirersByAttribute) {
    static_cast<void>(selectedAcquirer);
    result.divestedAttributeHandles.insert(attributeHandle);
  }

  std::uint64_t notificationId =
      federation->second.nextAttributeOwnershipDivestitureIfWantedNotificationId;
  for (auto const& [receivingFederateId, selectedAttributes] :
       attributesByReceivingFederate) {
    auto const callbackRoute = federation->second.interactionCallbackRoutes.find(
        receivingFederateId);
    // The route was checked while grouping. Retain the defensive branch so a
    // future change cannot turn an accepted transfer into an unrouteable one.
    if (callbackRoute == federation->second.interactionCallbackRoutes.end() ||
        !callbackRoute->second) {
      return {AttributeOwnershipDivestitureIfWantedStatus::inconsistent_catalog};
    }
    preparedNotifications.push_back({
        notificationId,
        receivingFederateId,
        selectedAttributes,
    });
    result.notifications.push_back({
        notificationId,
        receivingFederateId,
        objectInstanceHandle,
        selectedAttributes,
        userSuppliedTag,
        callbackRoute->second,
    });
    ++notificationId;
  }

  // Reserve every notification before changing owner state. A failure to
  // allocate a reservation leaves this service with no partial transfer.
  std::vector<std::uint64_t> reservedNotificationIds;
  reservedNotificationIds.reserve(preparedNotifications.size());
  try {
    for (auto& prepared : preparedNotifications) {
      auto const [reservation, inserted] =
          instance->second.pendingAttributeOwnershipDivestitureIfWantedNotifications.try_emplace(
              prepared.notificationId,
              Federation::ObjectInstance::
                  PendingAttributeOwnershipDivestitureIfWantedNotification{
                      prepared.receivingFederateId,
                      std::move(prepared.attributeHandles),
                  });
      static_cast<void>(reservation);
      if (!inserted) {
        for (std::uint64_t const reservedNotificationId : reservedNotificationIds) {
          instance->second.pendingAttributeOwnershipDivestitureIfWantedNotifications.erase(
              reservedNotificationId);
        }
        return {AttributeOwnershipDivestitureIfWantedStatus::inconsistent_catalog};
      }
      reservedNotificationIds.push_back(prepared.notificationId);
    }
  } catch (...) {
    for (std::uint64_t const reservedNotificationId : reservedNotificationIds) {
      instance->second.pendingAttributeOwnershipDivestitureIfWantedNotifications.erase(
          reservedNotificationId);
    }
    return {AttributeOwnershipDivestitureIfWantedStatus::inconsistent_catalog};
  }

  // A returned attribute moves directly from the divesting owner to the
  // selected acquirer. All older work addressed to the former owner is made
  // stale; requests made by other acquirers remain pending and are considered
  // after the selected acquirer's notification begins.
  for (auto const& [attributeHandle, selectedAcquirer] : selectedAcquirersByAttribute) {
    auto const owner = instance->second.attributeOwnersByHandle.find(attributeHandle);
    // The all-or-nothing validation above guarantees this owner record.
    owner->second = selectedAcquirer.receivingFederateId;
    // Divestiture If Wanted is an alternate terminal divestiture route. It
    // cancels a negotiated waiting state before that earlier confirmation can
    // transfer the same instance attribute.
    instance->second.pendingNegotiatedAttributeOwnershipDivestitures.erase(attributeHandle);

    for (auto pending = instance->second.pendingAttributeOwnershipAcquisitionRequests.begin();
         pending != instance->second.pendingAttributeOwnershipAcquisitionRequests.end();) {
      if (selectedAcquirer.kind == PendingRequestKind::regular &&
          pending->first == selectedAcquirer.requestId) {
        pending->second.desiredAttributeHandles.erase(attributeHandle);
        pending->second.notificationQueuedAttributeHandles.erase(attributeHandle);
      }
      auto release = pending->second.releaseCallbacksQueuedByOwningFederate.find(
          divestingFederateId);
      if (release != pending->second.releaseCallbacksQueuedByOwningFederate.end()) {
        release->second.erase(attributeHandle);
        if (release->second.empty()) {
          pending->second.releaseCallbacksQueuedByOwningFederate.erase(release);
        }
      }
      if (pending->second.desiredAttributeHandles.empty()) {
        pending = instance->second.pendingAttributeOwnershipAcquisitionRequests.erase(pending);
      } else {
        ++pending;
      }
    }

    if (selectedAcquirer.kind == PendingRequestKind::if_available) {
      auto pending =
          instance->second.pendingAttributeOwnershipAcquisitionIfAvailableRequests.find(
              selectedAcquirer.requestId);
      if (pending !=
          instance->second.pendingAttributeOwnershipAcquisitionIfAvailableRequests.end()) {
        pending->second.desiredAttributeHandles.erase(attributeHandle);
        if (pending->second.desiredAttributeHandles.empty()) {
          instance->second.pendingAttributeOwnershipAcquisitionIfAvailableRequests.erase(pending);
        }
      }
    }
  }
  federation->second.nextAttributeOwnershipDivestitureIfWantedNotificationId = notificationId;
  return result;
}

std::optional<AttributeOwnershipDivestitureIfWantedDelivery>
EmbeddedFederationRegistry::beginAttributeOwnershipDivestitureIfWantedNotification(
    std::wstring const& federationName,
    std::uint64_t receivingFederateId,
    std::uint64_t objectInstanceHandle,
    std::uint64_t notificationId,
    std::set<std::uint64_t> const& scheduledAttributeHandles) {
  if (notificationId == 0 || scheduledAttributeHandles.empty()) {
    return std::nullopt;
  }

  std::scoped_lock lock(mutex_);
  auto federation = federations_.find(federationName);
  if (federation == federations_.end()) {
    return std::nullopt;
  }
  auto instance = federation->second.objectInstances.find(objectInstanceHandle);
  if (instance == federation->second.objectInstances.end()) {
    return std::nullopt;
  }
  auto notification =
      instance->second.pendingAttributeOwnershipDivestitureIfWantedNotifications.find(
          notificationId);
  if (notification ==
          instance->second.pendingAttributeOwnershipDivestitureIfWantedNotifications.end() ||
      notification->second.receivingFederateId != receivingFederateId ||
      notification->second.attributeHandles != scheduledAttributeHandles) {
    return std::nullopt;
  }

  auto consumeNotificationAndPlanFollowup = [&] {
    auto securedAttributeHandles = notification->second.attributeHandles;
    instance->second.pendingAttributeOwnershipDivestitureIfWantedNotifications.erase(notification);
    return std::pair{
        std::move(securedAttributeHandles),
        planPendingAttributeOwnershipAcquisitionWork(federation->second, instance->second),
    };
  };
  if (instance->second.deleteAccepted ||
      !federation->second.members.contains(receivingFederateId) ||
      !instance->second.knownObjectClassHandlesByFederate.contains(receivingFederateId)) {
    auto [ignoredAttributes, followupWorkItems] = consumeNotificationAndPlanFollowup();
    static_cast<void>(ignoredAttributes);
    if (followupWorkItems.empty()) {
      return std::nullopt;
    }
    return AttributeOwnershipDivestitureIfWantedDelivery{
        objectInstanceHandle,
        {},
        std::move(followupWorkItems),
    };
  }

  auto [securedAttributeHandles, followupWorkItems] = consumeNotificationAndPlanFollowup();
  return AttributeOwnershipDivestitureIfWantedDelivery{
      objectInstanceHandle,
      std::move(securedAttributeHandles),
      std::move(followupWorkItems),
  };
}

AttributeOwnershipAcquisitionCancellationPlan
EmbeddedFederationRegistry::planAttributeOwnershipAcquisitionCancellation(
    std::wstring const& federationName,
    std::uint64_t requestingFederateId,
    std::uint64_t objectInstanceHandle,
    std::set<std::uint64_t> const& attributeHandles) {
  std::scoped_lock lock(mutex_);
  auto federation = federations_.find(federationName);
  if (federation == federations_.end()) {
    return {AttributeOwnershipAcquisitionCancellationStatus::federation_does_not_exist};
  }
  if (!federation->second.members.contains(requestingFederateId)) {
    return {AttributeOwnershipAcquisitionCancellationStatus::requesting_federate_not_member};
  }
  auto instance = federation->second.objectInstances.find(objectInstanceHandle);
  if (instance == federation->second.objectInstances.end() || instance->second.deleteAccepted ||
      !instance->second.knownObjectClassHandlesByFederate.contains(requestingFederateId)) {
    return {AttributeOwnershipAcquisitionCancellationStatus::object_instance_not_known};
  }
  if (!federation->second.definition.catalog ||
      !federation->second.objectClassHandles ||
      !federation->second.attributeHandles) {
    return {AttributeOwnershipAcquisitionCancellationStatus::inconsistent_catalog};
  }

  auto const knownClass = instance->second.knownObjectClassHandlesByFederate.find(
      requestingFederateId);
  auto const knownClassName = federation->second.objectClassHandles->nameFor(knownClass->second);
  if (!knownClassName ||
      federation->second.definition.catalog->objectClass(*knownClassName) == nullptr) {
    return {AttributeOwnershipAcquisitionCancellationStatus::inconsistent_catalog};
  }
  for (std::uint64_t const attributeHandle : attributeHandles) {
    if (!federation->second.attributeHandles->nameFor(
            federation->second.definition.catalog.get(),
            *knownClassName,
            attributeHandle)) {
      return {AttributeOwnershipAcquisitionCancellationStatus::attribute_not_defined};
    }
  }

  // Empty attribute sets have no cancellation transition or callback, while
  // still honoring the connection, federation, and known-instance checks.
  if (attributeHandles.empty()) {
    return {};
  }

  for (std::uint64_t const attributeHandle : attributeHandles) {
    auto const owner = instance->second.attributeOwnersByHandle.find(attributeHandle);
    if (owner != instance->second.attributeOwnersByHandle.end() &&
        owner->second == requestingFederateId) {
      return {AttributeOwnershipAcquisitionCancellationStatus::attribute_already_owned};
    }

    bool acquisitionWasRequested = false;
    for (auto const& [requestId, pending] :
         instance->second.pendingAttributeOwnershipAcquisitionRequests) {
      static_cast<void>(requestId);
      if (pending.requestingFederateId == requestingFederateId &&
          pending.desiredAttributeHandles.contains(attributeHandle)) {
        acquisitionWasRequested = true;
        break;
      }
    }
    if (!acquisitionWasRequested) {
      return {
          AttributeOwnershipAcquisitionCancellationStatus::attribute_acquisition_was_not_requested};
    }
  }

  if (federation->second.nextAttributeOwnershipAcquisitionCancellationId == 0 ||
      federation->second.nextAttributeOwnershipAcquisitionCancellationId ==
          std::numeric_limits<std::uint64_t>::max()) {
    return {AttributeOwnershipAcquisitionCancellationStatus::inconsistent_catalog};
  }
  auto const callbackRoute = federation->second.interactionCallbackRoutes.find(
      requestingFederateId);
  if (callbackRoute == federation->second.interactionCallbackRoutes.end() ||
      !callbackRoute->second) {
    return {AttributeOwnershipAcquisitionCancellationStatus::inconsistent_catalog};
  }

  // Complete all potentially allocating copies before mutating the pending
  // acquisition state. A failed allocation must not leave a cancellation
  // reservation without the callback payload that makes it terminal.
  std::set<std::uint64_t> cancellationAttributes = attributeHandles;
  ObjectInstanceCallbackRoute cancellationCallbackRoute = callbackRoute->second;

  std::uint64_t const cancellationId =
      federation->second.nextAttributeOwnershipAcquisitionCancellationId;
  auto const [cancellation, inserted] =
      instance->second.pendingAttributeOwnershipAcquisitionCancellations.try_emplace(
          cancellationId,
          Federation::ObjectInstance::PendingAttributeOwnershipAcquisitionCancellation{
              requestingFederateId,
              cancellationAttributes,
          });
  static_cast<void>(cancellation);
  if (!inserted) {
    return {AttributeOwnershipAcquisitionCancellationStatus::inconsistent_catalog};
  }
  ++federation->second.nextAttributeOwnershipAcquisitionCancellationId;

  // The cancellation is accepted before its callback executes, so every
  // queued notification or owner-release request for these attributes becomes
  // stale. The distinct cancellation reservation retains the publication
  // guard through the Confirm callback boundary.
  for (auto pending = instance->second.pendingAttributeOwnershipAcquisitionRequests.begin();
       pending != instance->second.pendingAttributeOwnershipAcquisitionRequests.end();) {
    for (std::uint64_t const attributeHandle : cancellationAttributes) {
      pending->second.desiredAttributeHandles.erase(attributeHandle);
      pending->second.notificationQueuedAttributeHandles.erase(attributeHandle);
      for (auto release = pending->second.releaseCallbacksQueuedByOwningFederate.begin();
           release != pending->second.releaseCallbacksQueuedByOwningFederate.end();) {
        release->second.erase(attributeHandle);
        if (release->second.empty()) {
          release = pending->second.releaseCallbacksQueuedByOwningFederate.erase(release);
        } else {
          ++release;
        }
      }
    }
    if (pending->second.desiredAttributeHandles.empty()) {
      pending = instance->second.pendingAttributeOwnershipAcquisitionRequests.erase(pending);
    } else {
      ++pending;
    }
  }

  return {
      AttributeOwnershipAcquisitionCancellationStatus::applied,
      cancellationId,
      std::move(cancellationAttributes),
      std::move(cancellationCallbackRoute),
  };
}

std::optional<AttributeOwnershipAcquisitionCancellationDelivery>
EmbeddedFederationRegistry::beginAttributeOwnershipAcquisitionCancellation(
    std::wstring const& federationName,
    std::uint64_t requestingFederateId,
    std::uint64_t objectInstanceHandle,
    std::uint64_t cancellationId,
    std::set<std::uint64_t> const& scheduledAttributeHandles) {
  if (cancellationId == 0 || scheduledAttributeHandles.empty()) {
    return std::nullopt;
  }

  std::scoped_lock lock(mutex_);
  auto federation = federations_.find(federationName);
  if (federation == federations_.end()) {
    return std::nullopt;
  }
  auto instance = federation->second.objectInstances.find(objectInstanceHandle);
  if (instance == federation->second.objectInstances.end()) {
    return std::nullopt;
  }
  auto cancellation = instance->second.pendingAttributeOwnershipAcquisitionCancellations.find(
      cancellationId);
  if (cancellation == instance->second.pendingAttributeOwnershipAcquisitionCancellations.end() ||
      cancellation->second.requestingFederateId != requestingFederateId ||
      cancellation->second.attributeHandles != scheduledAttributeHandles) {
    return std::nullopt;
  }

  auto consumeCancellationAndPlanFollowup = [&] {
    auto confirmedAttributeHandles = cancellation->second.attributeHandles;
    instance->second.pendingAttributeOwnershipAcquisitionCancellations.erase(cancellation);
    return std::pair{
        std::move(confirmedAttributeHandles),
        planPendingAttributeOwnershipAcquisitionWork(federation->second, instance->second),
    };
  };
  if (instance->second.deleteAccepted ||
      !federation->second.members.contains(requestingFederateId) ||
      !instance->second.knownObjectClassHandlesByFederate.contains(requestingFederateId)) {
    auto [ignoredConfirmedAttributeHandles, followupWorkItems] =
        consumeCancellationAndPlanFollowup();
    static_cast<void>(ignoredConfirmedAttributeHandles);
    if (followupWorkItems.empty()) {
      return std::nullopt;
    }
    return AttributeOwnershipAcquisitionCancellationDelivery{
        objectInstanceHandle,
        {},
        std::move(followupWorkItems),
    };
  }

  auto [confirmedAttributeHandles, followupWorkItems] =
      consumeCancellationAndPlanFollowup();
  return AttributeOwnershipAcquisitionCancellationDelivery{
      objectInstanceHandle,
      std::move(confirmedAttributeHandles),
      std::move(followupWorkItems),
  };
}

std::optional<AttributeOwnershipUnavailableRecipient>
EmbeddedFederationRegistry::attributeOwnershipUnavailableRecipientFor(
    std::wstring const& federationName,
    std::uint64_t receivingFederateId,
    std::uint64_t objectInstanceHandle,
    std::set<std::uint64_t> const& attributeHandles) const {
  std::scoped_lock lock(mutex_);
  auto const federation = federations_.find(federationName);
  if (federation == federations_.end() ||
      !federation->second.members.contains(receivingFederateId)) {
    return std::nullopt;
  }
  auto const instance = federation->second.objectInstances.find(objectInstanceHandle);
  if (instance == federation->second.objectInstances.end() || instance->second.deleteAccepted) {
    return std::nullopt;
  }
  auto const knownClass = instance->second.knownObjectClassHandlesByFederate.find(
      receivingFederateId);
  if (knownClass == instance->second.knownObjectClassHandlesByFederate.end() ||
      !federation->second.definition.catalog ||
      !federation->second.objectClassHandles ||
      !federation->second.attributeHandles) {
    return std::nullopt;
  }
  auto const knownClassName = federation->second.objectClassHandles->nameFor(knownClass->second);
  if (!knownClassName ||
      federation->second.definition.catalog->objectClass(*knownClassName) == nullptr) {
    return std::nullopt;
  }
  for (std::uint64_t const attributeHandle : attributeHandles) {
    if (!federation->second.attributeHandles->nameFor(
            federation->second.definition.catalog.get(),
            *knownClassName,
            attributeHandle)) {
      return std::nullopt;
    }
  }
  auto const callbackRoute = federation->second.interactionCallbackRoutes.find(receivingFederateId);
  if (callbackRoute == federation->second.interactionCallbackRoutes.end() ||
      !callbackRoute->second) {
    return std::nullopt;
  }
  return AttributeOwnershipUnavailableRecipient{
      receivingFederateId,
      objectInstanceHandle,
      attributeHandles,
      callbackRoute->second,
  };
}

ReceiveOrderInteractionPlan EmbeddedFederationRegistry::planReceiveOrderInteraction(
    std::wstring const& federationName,
    std::uint64_t producingFederateId,
    std::uint64_t sentInteractionClassHandle,
    std::vector<std::uint64_t> const& sentParameterHandles,
    std::set<std::uint64_t> const* sentRegionHandles) const {
  std::scoped_lock lock(mutex_);
  auto const federation = federations_.find(federationName);
  if (federation == federations_.end()) {
    return {ReceiveOrderInteractionStatus::federation_does_not_exist};
  }
  if (!federation->second.members.contains(producingFederateId)) {
    return {ReceiveOrderInteractionStatus::producing_federate_not_member};
  }
  if (!validInteractionClass(federation->second, sentInteractionClassHandle)) {
    return {ReceiveOrderInteractionStatus::interaction_class_not_defined};
  }
  if (!federation->second.definition.catalog ||
      !federation->second.interactionClassHandles ||
      !federation->second.parameterHandles) {
    return {ReceiveOrderInteractionStatus::inconsistent_catalog};
  }

  auto const sentClassName = federation->second.interactionClassHandles->nameFor(
      sentInteractionClassHandle);
  auto const* sentClass = sentClassName
      ? federation->second.definition.catalog->interactionClass(*sentClassName)
      : nullptr;
  if (sentClass == nullptr || sentClass->transportation.empty()) {
    return {ReceiveOrderInteractionStatus::inconsistent_catalog};
  }

  auto const declarations = federation->second.interactionDeclarations.find(producingFederateId);
  if (declarations == federation->second.interactionDeclarations.end() ||
      !declarations->second.publishedInteractionClasses.contains(sentInteractionClassHandle)) {
    return {ReceiveOrderInteractionStatus::interaction_class_not_published};
  }

  std::set<std::uint64_t> uniqueParameterHandles;
  for (std::uint64_t const parameterHandle : sentParameterHandles) {
    if (!uniqueParameterHandles.insert(parameterHandle).second ||
        !federation->second.parameterHandles->nameFor(
            federation->second.definition.catalog.get(),
            *sentClassName,
            parameterHandle)) {
      return {ReceiveOrderInteractionStatus::interaction_parameter_not_defined};
    }
  }

  ReceiveOrderInteractionPlan result;
  result.transportationName = sentClass->transportation;
  if (sentRegionHandles != nullptr) {
    if (sentRegionHandles->empty()) {
      // §9.12 explicitly accepts an empty set as a no-send operation, not as
      // an invocation of the ordinary Send Interaction service.
      return result;
    }
    auto const availableDimensions = availableInteractionDimensions(
        federation->second,
        sentInteractionClassHandle);
    if (!availableDimensions) {
      return {ReceiveOrderInteractionStatus::inconsistent_catalog};
    }
    for (std::uint64_t const regionHandle : *sentRegionHandles) {
      auto const region = federation->second.regions.find(regionHandle);
      if (region == federation->second.regions.end()) {
        return {ReceiveOrderInteractionStatus::invalid_region};
      }
      if (region->second.ownerFederateId != producingFederateId) {
        return {ReceiveOrderInteractionStatus::region_not_created_by_this_federate};
      }
      if (!region->second.specificationCommitted ||
          region->second.committedRangeBounds.size() != region->second.dimensionHandles.size()) {
        return {ReceiveOrderInteractionStatus::invalid_region};
      }
      if (!std::includes(
              availableDimensions->begin(),
              availableDimensions->end(),
              region->second.dimensionHandles.begin(),
              region->second.dimensionHandles.end())) {
        return {ReceiveOrderInteractionStatus::invalid_region_context};
      }
    }
  }
  for (auto const& [federateId, membership] : federation->second.members) {
    static_cast<void>(membership);
    auto recipient = candidateReceiveOrderInteractionRecipient(
        federation->second,
        producingFederateId,
        federateId,
        sentInteractionClassHandle,
        sentParameterHandles,
        sentRegionHandles);
    if (recipient) {
      result.recipients.push_back(std::move(*recipient));
    }
  }
  return result;
}

std::optional<ReceiveOrderInteractionRecipient>
EmbeddedFederationRegistry::receiveOrderInteractionRecipientFor(
    std::wstring const& federationName,
    std::uint64_t producingFederateId,
    std::uint64_t receivingFederateId,
    std::uint64_t sentInteractionClassHandle,
    std::vector<std::uint64_t> const& sentParameterHandles,
    std::set<std::uint64_t> const* sentRegionHandles) const {
  std::scoped_lock lock(mutex_);
  auto const federation = federations_.find(federationName);
  if (federation == federations_.end()) {
    return std::nullopt;
  }
  return candidateReceiveOrderInteractionRecipient(
      federation->second,
      producingFederateId,
      receivingFederateId,
      sentInteractionClassHandle,
      sentParameterHandles,
      sentRegionHandles);
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
  result.transportationName = sentClass->transportation;
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
    }
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

}  // namespace umbra::detail
