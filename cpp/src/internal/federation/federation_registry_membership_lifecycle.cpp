#include "internal/federation/federation_registry.hpp"
#include "internal/fom/fom_catalog.hpp"

#include <RTI/RTIambassador.h>

#include <atomic>
#include <limits>
#include <random>

namespace umbra::detail {

namespace {

rti1516_2025::ResignAction resignActionFromFom(std::string const& value) {
  if (value == "UnconditionallyDivestAttributes") {
    return rti1516_2025::UNCONDITIONALLY_DIVEST_ATTRIBUTES;
  }
  if (value == "DeleteObjects") {
    return rti1516_2025::DELETE_OBJECTS;
  }
  if (value == "CancelPendingOwnershipAcquisitions") {
    return rti1516_2025::CANCEL_PENDING_OWNERSHIP_ACQUISITIONS;
  }
  if (value == "DeleteObjectsThenDivest") {
    return rti1516_2025::DELETE_OBJECTS_THEN_DIVEST;
  }
  if (value == "CancelThenDeleteThenDivest") {
    return rti1516_2025::CANCEL_THEN_DELETE_THEN_DIVEST;
  }
  if (value == "NoAction") {
    return rti1516_2025::NO_ACTION;
  }
  return rti1516_2025::CANCEL_THEN_DELETE_THEN_DIVEST;
}

std::uint64_t mixedNormalizationValue(std::uint64_t value) noexcept {
  // SplitMix64's finalizer is a compact, deterministic mixer. It is used for
  // an opaque RTI coordinate, not as a cryptographic primitive.
  value += 0x9E3779B97F4A7C15ULL;
  value = (value ^ (value >> 30U)) * 0xBF58476D1CE4E5B9ULL;
  value = (value ^ (value >> 27U)) * 0x94D049BB133111EBULL;
  return value ^ (value >> 31U);
}

std::uint64_t nextNormalizationSeed() noexcept {
  // A per-execution entropy contribution keeps normalized values independent
  // of the private monotonically allocated handles. The counter remains a
  // deterministic fallback for platforms where the entropy provider fails.
  static std::atomic_uint64_t nextSeed{0x9E3779B97F4A7C15ULL};
  auto seed = nextSeed.fetch_add(0x9E3779B97F4A7C15ULL, std::memory_order_relaxed);
  try {
    std::random_device entropy;
    seed ^= static_cast<std::uint64_t>(entropy()) << 32U;
    seed ^= static_cast<std::uint64_t>(entropy());
  } catch (...) {
    // The normalizer has a safe deterministic fallback and this helper is
    // intentionally noexcept because federation creation must not fail only
    // because an optional entropy source is unavailable.
  }
  return mixedNormalizationValue(seed);
}

} // namespace

FederationRegistryResult EmbeddedFederationRegistry::create(
    std::wstring const& federationName,
    FederationDefinition definition) {
  auto instrumentationScope = beginInstrumentation("create");
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
  TransportationTypeHandleDirectory transportationTypeHandles;
  if (!transportationTypeHandles.reconcile(definition.catalog.get())) {
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
  auto const transportationTypeHandleSnapshot =
      std::make_shared<TransportationTypeHandleDirectory const>(
          std::move(transportationTypeHandles));

  std::scoped_lock lock(mutex_);
  if (federations_.contains(federationName)) {
    return {FederationRegistryStatus::federation_already_exists};
  }

  Federation federationState;
  federationState.normalizationSeed = nextNormalizationSeed();
  auto const logicalTimeImplementationName = definition.logicalTimeImplementationName;
  federationState.definition = std::move(definition);
  if (federationState.definition.catalog) {
    federationState.autoProvideSwitch =
        federationState.definition.catalog->federationSwitches().autoProvide;
    federationState.advisoriesUseKnownClassSwitch =
        federationState.definition.catalog->advisorySwitches().advisoriesUseKnownClass;
    federationState.nonRegulatedGrantSwitch =
        federationState.definition.catalog->timeManagementSwitches().nonRegulatedGrant;
    federationState.delaySubscriptionEvaluationSwitch =
        federationState.definition.catalog->federationSwitches().delaySubscriptionEvaluation;
    federationState.allowRelaxedDDMSwitch =
        federationState.definition.catalog->federationSwitches().allowRelaxedDDM;
  }
  if (!federationState.timeCoordinator.configureImplementationName(
          logicalTimeImplementationName)) {
    return {FederationRegistryStatus::invalid_request};
  }
  federationState.objectClassHandles = std::move(objectClassHandleSnapshot);
  federationState.attributeHandles = std::move(attributeHandleSnapshot);
  federationState.interactionClassHandles = std::move(interactionClassHandleSnapshot);
  federationState.parameterHandles = std::move(parameterHandleSnapshot);
  federationState.dimensionHandles = std::move(dimensionHandleSnapshot);
  federationState.transportationTypeHandles = std::move(transportationTypeHandleSnapshot);
  federations_.emplace(federationName, std::move(federationState));
  return {};
}

FederationRegistryResult EmbeddedFederationRegistry::destroy(std::wstring const& federationName) {
  auto instrumentationScope = beginInstrumentation("destroy");
  std::scoped_lock lock(mutex_);
  auto federation = federations_.find(federationName);
  if (federation == federations_.end()) {
    return {FederationRegistryStatus::federation_does_not_exist};
  }
  if (!federation->second.members.empty() || !federation->second.timeCoordinator.empty()) {
    return {FederationRegistryStatus::federates_currently_joined};
  }

  federations_.erase(federation);
  saveSnapshots_.erase(federationName);
  return {};
}

FederationJoinResult EmbeddedFederationRegistry::join(
    std::wstring const& federationName,
    std::wstring const& federateType,
    std::optional<std::wstring> requestedFederateName,
    InteractionCallbackRoute interactionCallbackRoute) {
  auto instrumentationScope = beginInstrumentation("join");
  return joinImpl(
      federationName,
      nullptr,
      nullptr,
      federateType,
      std::move(requestedFederateName),
      std::move(interactionCallbackRoute),
      {},
      {});
}

FederationJoinResult EmbeddedFederationRegistry::joinWithTimeState(
    std::wstring const& federationName,
    std::shared_ptr<FederateTimeState> timeState,
    std::wstring const& federateType,
    std::optional<std::wstring> requestedFederateName,
    InteractionCallbackRoute interactionCallbackRoute,
    FederationTimeGrantDispatchFactory timeAdvanceGrantDispatchFactory,
    FederationTimeRoleEnableDispatchFactory timeRoleEnableDispatchFactory) {
  auto instrumentationScope = beginInstrumentation("joinWithTimeState");
  return joinImpl(
      federationName,
      nullptr,
      std::move(timeState),
      federateType,
      std::move(requestedFederateName),
      std::move(interactionCallbackRoute),
      std::move(timeAdvanceGrantDispatchFactory),
      std::move(timeRoleEnableDispatchFactory));
}

FederationJoinResult EmbeddedFederationRegistry::joinWithDefinition(
    std::wstring const& federationName,
    FederationDefinition definition,
    std::wstring const& federateType,
    std::optional<std::wstring> requestedFederateName) {
  auto instrumentationScope = beginInstrumentation("joinWithDefinition");
  return joinImpl(
      federationName,
      &definition,
      nullptr,
      federateType,
      std::move(requestedFederateName),
      {},
      {},
      {});
}

FederationJoinResult EmbeddedFederationRegistry::joinWithDefinitionAndTimeState(
    std::wstring const& federationName,
    FederationDefinition definition,
    std::shared_ptr<FederateTimeState> timeState,
    std::wstring const& federateType,
    std::optional<std::wstring> requestedFederateName,
    InteractionCallbackRoute interactionCallbackRoute,
    FederationTimeGrantDispatchFactory timeAdvanceGrantDispatchFactory,
    FederationTimeRoleEnableDispatchFactory timeRoleEnableDispatchFactory) {
  auto instrumentationScope = beginInstrumentation("joinWithDefinitionAndTimeState");
  return joinImpl(
      federationName,
      &definition,
      std::move(timeState),
      federateType,
      std::move(requestedFederateName),
      std::move(interactionCallbackRoute),
      std::move(timeAdvanceGrantDispatchFactory),
      std::move(timeRoleEnableDispatchFactory));
}

FederationJoinResult EmbeddedFederationRegistry::joinImpl(
    std::wstring const& federationName,
    FederationDefinition* replacementDefinition,
    std::shared_ptr<FederateTimeState> timeState,
    std::wstring const& federateType,
    std::optional<std::wstring> requestedFederateName,
    InteractionCallbackRoute interactionCallbackRoute,
    FederationTimeGrantDispatchFactory timeAdvanceGrantDispatchFactory,
    FederationTimeRoleEnableDispatchFactory timeRoleEnableDispatchFactory) {
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
  std::shared_ptr<TransportationTypeHandleDirectory const> replacementTransportationTypeHandles;
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

    TransportationTypeHandleDirectory reconciledTransportationTypes =
        federation->second.transportationTypeHandles
        ? *federation->second.transportationTypeHandles
        : TransportationTypeHandleDirectory{};
    if (!reconciledTransportationTypes.reconcile(replacementDefinition->catalog.get())) {
      return {FederationRegistryStatus::invalid_request, std::nullopt};
    }
    replacementTransportationTypeHandles =
        std::make_shared<TransportationTypeHandleDirectory const>(
            std::move(reconciledTransportationTypes));
  }

  bool const registersTimeState = static_cast<bool>(timeState);
  if ((timeAdvanceGrantDispatchFactory || timeRoleEnableDispatchFactory) &&
      !registersTimeState) {
    return {FederationRegistryStatus::invalid_request, std::nullopt};
  }

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
  // IEEE 1516.2 per-federate switch-table values are the initial settings for
  // a joining federate. Advisories Use Known Class is deliberately excluded:
  // it is a static federation-wide switch captured at create time, so an
  // additional-FOM join cannot give the new member a divergent value.
  FederationDefinition const& membershipDefinition = replacementDefinition != nullptr
      ? *replacementDefinition
      : federation->second.definition;
  if (membershipDefinition.catalog) {
    auto const& advisorySwitches = membershipDefinition.catalog->advisorySwitches();
    membership.attributeScopeAdvisorySwitch = advisorySwitches.attributeScopeAdvisory;
    membership.attributeRelevanceAdvisorySwitch = advisorySwitches.attributeRelevanceAdvisory;
    membership.objectClassRelevanceAdvisorySwitch = advisorySwitches.objectClassRelevanceAdvisory;
    membership.interactionRelevanceAdvisorySwitch = advisorySwitches.interactionRelevanceAdvisory;
    auto const& supportSwitches = membershipDefinition.catalog->federateSupportSwitches();
    membership.conveyRegionDesignatorSetsSwitch = supportSwitches.conveyRegionDesignatorSets;
    membership.automaticResignAction =
        resignActionFromFom(supportSwitches.automaticResignAction);
    membership.serviceReportingSwitch = supportSwitches.serviceReporting;
    membership.exceptionReportingSwitch = supportSwitches.exceptionReporting;
    membership.sendServiceReportsToFileSwitch = supportSwitches.sendServiceReportsToFile;
  }
  auto [namePosition, insertedName] = federation->second.memberIdsByName.emplace(
      membership.name,
      membership.id);
  if (!insertedName) {
    return {FederationRegistryStatus::federate_name_already_in_use, std::nullopt};
  }

  bool coordinatorRegistered = false;
  {
    try {
      auto const registered = registersTimeState
          ? federation->second.timeCoordinator.registerFederate(
                membership.id,
                std::move(timeState))
          : federation->second.timeCoordinator.registerRecipient(membership.id);
      if (registered.status != FederationTimeCoordinatorStatus::applied) {
        federation->second.memberIdsByName.erase(namePosition);
        return {FederationRegistryStatus::invalid_request, std::nullopt};
      }
      coordinatorRegistered = true;
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
        if (coordinatorRegistered) {
          static_cast<void>(federation->second.timeCoordinator.unregisterFederate(membership.id));
        }
        federation->second.memberIdsByName.erase(namePosition);
        return {FederationRegistryStatus::invalid_request, std::nullopt};
      }
      routeRegistered = true;
    } catch (...) {
      if (coordinatorRegistered) {
        static_cast<void>(federation->second.timeCoordinator.unregisterFederate(membership.id));
      }
      federation->second.memberIdsByName.erase(namePosition);
      throw;
    }
  }

  bool timeAdvanceGrantDispatchFactoryRegistered = false;
  if (timeAdvanceGrantDispatchFactory) {
    try {
      auto [factoryPosition, insertedFactory] =
          federation->second.timeAdvanceGrantDispatchFactories.emplace(
              membership.id,
              std::move(timeAdvanceGrantDispatchFactory));
      static_cast<void>(factoryPosition);
      if (!insertedFactory) {
        if (routeRegistered) {
          federation->second.interactionCallbackRoutes.erase(membership.id);
        }
        if (coordinatorRegistered) {
          static_cast<void>(federation->second.timeCoordinator.unregisterFederate(membership.id));
        }
        federation->second.memberIdsByName.erase(namePosition);
        return {FederationRegistryStatus::invalid_request, std::nullopt};
      }
      timeAdvanceGrantDispatchFactoryRegistered = true;
    } catch (...) {
      if (routeRegistered) {
        federation->second.interactionCallbackRoutes.erase(membership.id);
      }
      if (coordinatorRegistered) {
        static_cast<void>(federation->second.timeCoordinator.unregisterFederate(membership.id));
      }
      federation->second.memberIdsByName.erase(namePosition);
      throw;
    }
  }

  bool timeRoleEnableDispatchFactoryRegistered = false;
  if (timeRoleEnableDispatchFactory) {
    try {
      auto [factoryPosition, insertedFactory] =
          federation->second.timeRoleEnableDispatchFactories.emplace(
              membership.id,
              std::move(timeRoleEnableDispatchFactory));
      static_cast<void>(factoryPosition);
      if (!insertedFactory) {
        if (timeAdvanceGrantDispatchFactoryRegistered) {
          federation->second.timeAdvanceGrantDispatchFactories.erase(membership.id);
        }
        if (routeRegistered) {
          federation->second.interactionCallbackRoutes.erase(membership.id);
        }
        if (coordinatorRegistered) {
          static_cast<void>(federation->second.timeCoordinator.unregisterFederate(membership.id));
        }
        federation->second.memberIdsByName.erase(namePosition);
        return {FederationRegistryStatus::invalid_request, std::nullopt};
      }
      timeRoleEnableDispatchFactoryRegistered = true;
    } catch (...) {
      if (timeAdvanceGrantDispatchFactoryRegistered) {
        federation->second.timeAdvanceGrantDispatchFactories.erase(membership.id);
      }
      if (routeRegistered) {
        federation->second.interactionCallbackRoutes.erase(membership.id);
      }
      if (coordinatorRegistered) {
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
      if (timeRoleEnableDispatchFactoryRegistered) {
        federation->second.timeRoleEnableDispatchFactories.erase(membership.id);
      }
      if (timeAdvanceGrantDispatchFactoryRegistered) {
        federation->second.timeAdvanceGrantDispatchFactories.erase(membership.id);
      }
      if (routeRegistered) {
        federation->second.interactionCallbackRoutes.erase(membership.id);
      }
      if (coordinatorRegistered) {
        static_cast<void>(federation->second.timeCoordinator.unregisterFederate(membership.id));
      }
      federation->second.memberIdsByName.erase(namePosition);
      return {FederationRegistryStatus::invalid_request, std::nullopt};
    }
  } catch (...) {
    if (timeRoleEnableDispatchFactoryRegistered) {
      federation->second.timeRoleEnableDispatchFactories.erase(membership.id);
    }
    if (timeAdvanceGrantDispatchFactoryRegistered) {
      federation->second.timeAdvanceGrantDispatchFactories.erase(membership.id);
    }
    if (routeRegistered) {
      federation->second.interactionCallbackRoutes.erase(membership.id);
    }
    if (coordinatorRegistered) {
      static_cast<void>(federation->second.timeCoordinator.unregisterFederate(membership.id));
    }
    federation->second.memberIdsByName.erase(namePosition);
    throw;
  }

  // A membership record is intentionally removed during resignation, but the
  // associated returned designator remains valid for this federation
  // execution. Record its immutable name separately after the active member
  // transaction has succeeded, and roll every active write back if that
  // lifetime-index allocation fails.
  try {
    auto const [identityPosition, insertedIdentity] =
        federation->second.federateNamesById.emplace(membership.id, membership.name);
    static_cast<void>(identityPosition);
    if (!insertedIdentity) {
      federation->second.members.erase(membership.id);
      if (timeRoleEnableDispatchFactoryRegistered) {
        federation->second.timeRoleEnableDispatchFactories.erase(membership.id);
      }
      if (timeAdvanceGrantDispatchFactoryRegistered) {
        federation->second.timeAdvanceGrantDispatchFactories.erase(membership.id);
      }
      if (routeRegistered) {
        federation->second.interactionCallbackRoutes.erase(membership.id);
      }
      if (coordinatorRegistered) {
        static_cast<void>(federation->second.timeCoordinator.unregisterFederate(membership.id));
      }
      federation->second.memberIdsByName.erase(namePosition);
      return {FederationRegistryStatus::invalid_request, std::nullopt};
    }
  } catch (...) {
    federation->second.members.erase(membership.id);
    if (timeRoleEnableDispatchFactoryRegistered) {
      federation->second.timeRoleEnableDispatchFactories.erase(membership.id);
    }
    if (timeAdvanceGrantDispatchFactoryRegistered) {
      federation->second.timeAdvanceGrantDispatchFactories.erase(membership.id);
    }
    if (routeRegistered) {
      federation->second.interactionCallbackRoutes.erase(membership.id);
    }
    if (coordinatorRegistered) {
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
    swap(
        federation->second.transportationTypeHandles,
        replacementTransportationTypeHandles);
  }
  nextFederateId_ = federateId + 1;
  return {FederationRegistryStatus::applied, std::move(membership)};
}
unsigned long EmbeddedFederationRegistry::normalizedHandleValue(
    std::uint64_t normalizationSeed,
    std::uint64_t handleValue,
    std::uint64_t handleKind) noexcept {
  // MOM report regions are half-open point ranges. Reserve the maximum
  // unsigned-long coordinate so every normalized handle can become [value,
  // value + 1) without overflow; the value remains opaque and execution
  // scoped rather than revealing the private handle sequence.
  auto constexpr maximumPointCoordinate = std::numeric_limits<unsigned long>::max();
  return static_cast<unsigned long>(
      mixedNormalizationValue(normalizationSeed ^ handleKind ^ handleValue) %
      maximumPointCoordinate);
}

} // namespace umbra::detail
