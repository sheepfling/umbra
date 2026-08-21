#include "internal/federation_registry.hpp"

#include "internal/fom_catalog.hpp"
#include "internal/federation_time_bounds.hpp"
#include "internal/federation_time_grant_policy.hpp"
#include "internal/handle_variable_array_encoding.hpp"

#include <RTI/encoding/BasicDataElements.h>

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

constexpr char kReportServiceInvocationInteractionClassName[] =
    "HLAinteractionRoot.HLAmanager.HLAfederate.HLAreport.HLAreportServiceInvocation";
constexpr char kReportFederateLostInteractionClassName[] =
    "HLAinteractionRoot.HLAmanager.HLAfederate.HLAreport.HLAreportFederateLost";
constexpr char kReportExceptionInteractionClassName[] =
    "HLAinteractionRoot.HLAmanager.HLAfederate.HLAreport.HLAreportException";

constexpr char kJoinedFederateMomObjectClassName[] =
    "HLAobjectRoot.HLAmanager.HLAfederate";
constexpr char kHlaFederateDimensionName[] = "HLAfederate";
constexpr char kHlaPrivilegeToDeleteObjectAttributeName[] =
    "HLAprivilegeToDeleteObject";
constexpr char kHlaReportServiceFileAttributeName[] = "HLAreportServiceFile";
constexpr char kHlaReportServiceFileMimConditionalUpdate[] = "Conditional";
constexpr char kHlaReportServiceFileMimConditionalUpdateCondition[] =
    "The first time that both HLAserviceReporting and "
    "HLAsendServiceReportsToFile become true.";

// Table 8's direct required joined-federate values. HLAreportServiceFile is
// included in the initial private snapshot under the selected 1516.1 Static
// policy; the unmodified 1516.2 MIM's contrary Conditional field is retained
// in the composed catalog and must not be overwritten here.
constexpr char kJoinedFederateInitialAttributeNames[][32] = {
    "HLAfederateHandle",
    "HLAfederateName",
    "HLAfederateType",
    "HLAfederateHost",
    "HLARTIversion",
    "HLAFOMmoduleDesignatorList",
    "HLAreportServiceFile",
};

constexpr char kJoinedFederateInitialAttributeDataTypes[][24] = {
    "HLAfederateHandle",
    "HLAunicodeString",
    "HLAunicodeString",
    "HLAunicodeString",
    "HLAunicodeString",
    "HLAmoduleDesignatorList",
    "HLAunicodeString",
};

constexpr char kMomServiceReportParameterNames[][24] = {
    "HLAservice",
    "HLAserviceType",
    "HLAsuccessIndicator",
    "HLAsuppliedArguments",
    "HLAreturnedArgument",
    "HLAexception",
    "HLAserialNumber",
};

constexpr char kFederateLostFederateParameterName[] = "HLAfederate";
constexpr char kFederateLostFederateNameParameterName[] = "HLAfederateName";
constexpr char kFederateLostTimestampParameterName[] = "HLAtimeStamp";
constexpr char kFederateLostFaultDescriptionParameterName[] = "HLAfaultDescription";
constexpr char kExceptionReportServiceParameterName[] = "HLAservice";
constexpr char kExceptionReportExceptionParameterName[] = "HLAexception";

// This value is never placed in Federation::regions.  It exists only in a
// short-lived RegionSpecificationSnapshot override while evaluating the
// RTI-owned §11.5 report endpoint, so federates cannot modify or delete it.
constexpr std::uint64_t kMomServiceReportEndpointRegionHandle =
    std::numeric_limits<std::uint64_t>::max();
constexpr std::uint64_t kMomFederateLostEndpointRegionHandle =
    std::numeric_limits<std::uint64_t>::max() - 1U;
constexpr std::uint64_t kMomExceptionReportEndpointRegionHandle =
    std::numeric_limits<std::uint64_t>::max() - 2U;

constexpr std::uint64_t kFederateNormalizationKind = 0xEB41A82B7D1E63F5ULL;
constexpr std::uint64_t kObjectClassNormalizationKind = 0x49B17E0D9346AC27ULL;
constexpr std::uint64_t kInteractionClassNormalizationKind = 0xC3D05B987A2E41F9ULL;
constexpr std::uint64_t kObjectInstanceNormalizationKind = 0x76F29C3E0B5DA418ULL;

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

void appendPadding(std::vector<rti1516_2025::Octet>& output, std::size_t boundary) {
  auto const remainder = output.size() % boundary;
  if (remainder != 0U) {
    output.insert(output.end(), boundary - remainder, static_cast<rti1516_2025::Octet>(0));
  }
}

void appendDataElement(
    std::vector<rti1516_2025::Octet>& output,
    rti1516_2025::DataElement const& value) {
  appendPadding(output, value.getOctetBoundary());
  value.encodeInto(output);
}

rti1516_2025::VariableLengthData encodeModuleDesignatorList(
    std::vector<PrevalidatedFomModule> const& modules) {
  std::set<std::filesystem::path> seenSources;
  std::vector<std::wstring> designators;
  designators.reserve(modules.size());
  for (auto const& module : modules) {
    // Federation-management preparation canonicalizes every FOM source
    // before it reaches this descriptor. Use that identity so the first
    // supplied designator represents repeated references to one module.
    if (seenSources.insert(module.sourcePath).second) {
      designators.push_back(module.designator);
    }
  }
  if (designators.size() >
      static_cast<std::size_t>(std::numeric_limits<rti1516_2025::Integer32>::max())) {
    throw rti1516_2025::EncoderException(
        L"The joined federate supplied too many FOM-module designators.");
  }

  std::vector<rti1516_2025::Octet> bytes;
  rti1516_2025::HLAinteger32BE count{
      static_cast<rti1516_2025::Integer32>(designators.size())};
  appendDataElement(bytes, count);
  for (auto const& designator : designators) {
    rti1516_2025::HLAunicodeString encodedDesignator{designator};
    appendDataElement(bytes, encodedDesignator);
  }
  return rti1516_2025::VariableLengthData(bytes.data(), bytes.size());
}

}  // namespace

static_assert(std::is_nothrow_swappable_v<FederationDefinition>);

bool objectInstanceNameIsLegal(std::wstring const& objectInstanceName) {
  // Clause 6.2/6.5 reserves the HLA. namespace for the RTI and rejects an
  // empty designator. Other character/name policy remains the standard
  // binding's responsibility rather than being invented by this kernel.
  return !objectInstanceName.empty() &&
      objectInstanceName.rfind(L"HLA.", 0) != 0;
}

bool isSupportedTransportationName(std::string const& transportationName) {
  return transportationName == "HLAreliable" ||
      transportationName == "HLAbestEffort";
}

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

std::optional<std::string> normalizedUpdateRateDesignator(
    FomCatalog const& catalog,
    std::string const& updateRateDesignator) {
  if (updateRateDesignator.empty() || updateRateDesignator == "default" ||
      updateRateDesignator == "HLAdefault" ||
      updateRateDesignator == "HLAdefaultUpdateRate") {
    return std::string{"HLAdefault"};
  }
  return catalog.updateRateValue(updateRateDesignator)
      ? std::optional<std::string>{updateRateDesignator}
      : std::nullopt;
}

std::string storedUpdateRateDesignator(
    std::string const& suppliedDesignator,
    std::string const& normalizedDesignator) {
  // An omitted designator selects the default rate but must remain distinct
  // from an explicitly supplied HLAdefault value: the former uses the
  // two-argument Turn Updates On callback, while the latter uses its official
  // rate-bearing overload.
  return suppliedDesignator.empty() ? std::string{} : normalizedDesignator;
}

std::optional<double> updateRateValueForNormalizedDesignator(
    FomCatalog const& catalog,
    std::string const& normalizedDesignator) {
  if (normalizedDesignator == "HLAdefault") {
    return 0.0;
  }
  return catalog.updateRateValue(normalizedDesignator);
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

unsigned long EmbeddedFederationRegistry::normalizedHandleValue(
    std::uint64_t normalizationSeed,
    std::uint64_t handleValue,
    std::uint64_t handleKind) noexcept {
  // MOM report regions are half-open point ranges.  Reserve the maximum
  // unsigned-long coordinate so every normalized handle can become [value,
  // value + 1) without overflow; the value remains opaque and execution
  // scoped rather than revealing the private handle sequence.
  auto constexpr maximumPointCoordinate = std::numeric_limits<unsigned long>::max();
  return static_cast<unsigned long>(
      mixedNormalizationValue(normalizationSeed ^ handleKind ^ handleValue) %
      maximumPointCoordinate);
}

EmbeddedFederationRegistry::EmbeddedFederationRegistry(
    std::shared_ptr<RuntimeInstrumentation> instrumentation)
    : instrumentation_(
          instrumentation ? std::move(instrumentation)
                           : std::make_shared<RuntimeInstrumentation>()) {}

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
    std::optional<std::wstring> requestedFederateName) {
  auto instrumentationScope = beginInstrumentation("join");
  return joinImpl(
      federationName,
      nullptr,
      nullptr,
      federateType,
      std::move(requestedFederateName),
      {},
      {});
}

FederationJoinResult EmbeddedFederationRegistry::joinWithTimeState(
    std::wstring const& federationName,
    std::shared_ptr<FederateTimeState> timeState,
    std::wstring const& federateType,
    std::optional<std::wstring> requestedFederateName,
    InteractionCallbackRoute interactionCallbackRoute,
    FederationTimeGrantDispatchFactory timeAdvanceGrantDispatchFactory) {
  auto instrumentationScope = beginInstrumentation("joinWithTimeState");
  return joinImpl(
      federationName,
      nullptr,
      std::move(timeState),
      federateType,
      std::move(requestedFederateName),
      std::move(interactionCallbackRoute),
      std::move(timeAdvanceGrantDispatchFactory));
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
      {});
}

FederationJoinResult EmbeddedFederationRegistry::joinWithDefinitionAndTimeState(
    std::wstring const& federationName,
    FederationDefinition definition,
    std::shared_ptr<FederateTimeState> timeState,
    std::wstring const& federateType,
    std::optional<std::wstring> requestedFederateName,
    InteractionCallbackRoute interactionCallbackRoute,
    FederationTimeGrantDispatchFactory timeAdvanceGrantDispatchFactory) {
  auto instrumentationScope = beginInstrumentation("joinWithDefinitionAndTimeState");
  return joinImpl(
      federationName,
      &definition,
      std::move(timeState),
      federateType,
      std::move(requestedFederateName),
      std::move(interactionCallbackRoute),
      std::move(timeAdvanceGrantDispatchFactory));
}

FederationJoinResult EmbeddedFederationRegistry::joinImpl(
    std::wstring const& federationName,
    FederationDefinition* replacementDefinition,
    std::shared_ptr<FederateTimeState> timeState,
    std::wstring const& federateType,
    std::optional<std::wstring> requestedFederateName,
    InteractionCallbackRoute interactionCallbackRoute,
    FederationTimeGrantDispatchFactory timeAdvanceGrantDispatchFactory) {
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
  if (timeAdvanceGrantDispatchFactory && !registersTimeState) {
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
        if (registersTimeState) {
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
      if (timeAdvanceGrantDispatchFactoryRegistered) {
        federation->second.timeAdvanceGrantDispatchFactories.erase(membership.id);
      }
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
    if (timeAdvanceGrantDispatchFactoryRegistered) {
      federation->second.timeAdvanceGrantDispatchFactories.erase(membership.id);
    }
    if (routeRegistered) {
      federation->second.interactionCallbackRoutes.erase(membership.id);
    }
    if (registersTimeState) {
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
      if (timeAdvanceGrantDispatchFactoryRegistered) {
        federation->second.timeAdvanceGrantDispatchFactories.erase(membership.id);
      }
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
    federation->second.members.erase(membership.id);
    if (timeAdvanceGrantDispatchFactoryRegistered) {
      federation->second.timeAdvanceGrantDispatchFactories.erase(membership.id);
    }
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

JoinedFederateMomObjectStatus
EmbeddedFederationRegistry::establishJoinedFederateMomObject(
    std::wstring const& federationName,
    std::uint64_t federateId,
    JoinedFederateMomObjectDescriptor const& descriptor) {
  auto instrumentationScope = beginInstrumentation("establishJoinedFederateMomObject");
  std::scoped_lock lock(mutex_);
  auto federation = federations_.find(federationName);
  if (federation == federations_.end()) {
    return JoinedFederateMomObjectStatus::federation_does_not_exist;
  }
  auto const member = federation->second.members.find(federateId);
  if (member == federation->second.members.end()) {
    return JoinedFederateMomObjectStatus::federate_not_member;
  }
  if (descriptor.reportServiceFile.empty()) {
    return JoinedFederateMomObjectStatus::invalid_descriptor;
  }
  for (auto const& module : descriptor.fomModulesSpecifiedAtJoin) {
    if (module.kind != FomModuleKind::fom || module.sourcePath.empty()) {
      return JoinedFederateMomObjectStatus::invalid_descriptor;
    }
  }
  for (auto const& [objectHandle, object] :
       federation->second.rtiOwnedJoinedFederateMomObjects) {
    static_cast<void>(objectHandle);
    if (object.joinedFederateId == federateId) {
      return JoinedFederateMomObjectStatus::already_established;
    }
  }
  if (!federation->second.definition.catalog ||
      !federation->second.objectClassHandles ||
      !federation->second.attributeHandles ||
      !federation->second.dimensionHandles) {
    return JoinedFederateMomObjectStatus::inconsistent_catalog;
  }

  auto const objectClassHandle = federation->second.objectClassHandles->handleFor(
      kJoinedFederateMomObjectClassName);
  auto const federateDimensionHandle = federation->second.dimensionHandles->handleFor(
      kHlaFederateDimensionName);
  auto const effectiveAttributes =
      federation->second.definition.catalog->effectiveObjectClassAttributes(
          kJoinedFederateMomObjectClassName);
  if (!objectClassHandle || !federateDimensionHandle || !effectiveAttributes) {
    return JoinedFederateMomObjectStatus::inconsistent_catalog;
  }

  std::map<std::string, std::uint64_t> attributeHandlesByName;
  std::set<std::uint64_t> effectiveAttributeHandles;
  std::set<std::uint64_t> periodicAttributeHandles;
  for (auto const& [attributeName, attribute] : *effectiveAttributes) {
    auto const attributeHandle = federation->second.attributeHandles->handleFor(
        federation->second.definition.catalog.get(),
        kJoinedFederateMomObjectClassName,
        attributeName);
    if (!attributeHandle || !effectiveAttributeHandles.insert(*attributeHandle).second ||
        !attributeHandlesByName.emplace(attributeName, *attributeHandle).second) {
      return JoinedFederateMomObjectStatus::inconsistent_catalog;
    }
    if (attribute.updateType == "Periodic") {
      periodicAttributeHandles.insert(*attributeHandle);
    }
  }

  auto const deletePrivilege = effectiveAttributes->find(
      kHlaPrivilegeToDeleteObjectAttributeName);
  if (deletePrivilege == effectiveAttributes->end() ||
      deletePrivilege->second.dataType != "HLAtoken" ||
      deletePrivilege->second.valueRequired ||
      deletePrivilege->second.ownership != "DivestAcquire" ||
      !attributeHandlesByName.contains(kHlaPrivilegeToDeleteObjectAttributeName)) {
    return JoinedFederateMomObjectStatus::inconsistent_catalog;
  }

  for (std::size_t index = 0; index < std::size(kJoinedFederateInitialAttributeNames); ++index) {
    auto const attribute = effectiveAttributes->find(kJoinedFederateInitialAttributeNames[index]);
    if (attribute == effectiveAttributes->end() ||
        attribute->second.dataType != kJoinedFederateInitialAttributeDataTypes[index] ||
        !attribute->second.valueRequired ||
        attribute->second.ownership != "NoTransfer" ||
        !attributeHandlesByName.contains(kJoinedFederateInitialAttributeNames[index])) {
      return JoinedFederateMomObjectStatus::inconsistent_catalog;
    }
    // The 1516.1-2025 Table 8 source calls HLAreportServiceFile Static,
    // while the unmodified MIM catalog records one precise Conditional rule.
    // Preserve the conflict without broadening it into an arbitrary policy:
    // a future malformed catalog must not be treated as a valid joined-
    // federate MOM foundation. The other six direct initial values must have
    // the MIM's Static policy.
    if (std::string_view{kJoinedFederateInitialAttributeNames[index]} ==
        kHlaReportServiceFileAttributeName) {
      bool const tableEightStatic = attribute->second.updateType == "Static";
      bool const vendoredMimConditional =
          attribute->second.updateType == kHlaReportServiceFileMimConditionalUpdate &&
          attribute->second.updateCondition ==
              kHlaReportServiceFileMimConditionalUpdateCondition;
      if (!tableEightStatic && !vendoredMimConditional) {
        return JoinedFederateMomObjectStatus::inconsistent_catalog;
      }
    } else if (attribute->second.updateType != "Static") {
      return JoinedFederateMomObjectStatus::inconsistent_catalog;
    }
  }

  std::map<std::uint64_t, rti1516_2025::VariableLengthData> initialAttributeValues;
  auto addInitialValue = [&](char const* attributeName,
                             rti1516_2025::VariableLengthData value) {
    auto const attributeHandle = attributeHandlesByName.find(attributeName);
    return attributeHandle != attributeHandlesByName.end() &&
        initialAttributeValues.emplace(attributeHandle->second, std::move(value)).second;
  };
  try {
    auto const encodedFederateHandle =
        rti1516_2025::umbra_binding_detail::encodeUmbraHandleVariableArray(federateId);
    if (!addInitialValue(
            "HLAfederateHandle",
            rti1516_2025::VariableLengthData(
                encodedFederateHandle.data(), encodedFederateHandle.size())) ||
        !addInitialValue(
            "HLAfederateName",
            rti1516_2025::HLAunicodeString{member->second.name}.encode()) ||
        !addInitialValue(
            "HLAfederateType",
            rti1516_2025::HLAunicodeString{member->second.type}.encode()) ||
        !addInitialValue(
            "HLAfederateHost",
            rti1516_2025::HLAunicodeString{descriptor.federateHost}.encode()) ||
        !addInitialValue(
            "HLARTIversion",
            rti1516_2025::HLAunicodeString{descriptor.rtiVersion}.encode()) ||
        !addInitialValue(
            "HLAFOMmoduleDesignatorList",
            encodeModuleDesignatorList(descriptor.fomModulesSpecifiedAtJoin)) ||
        !addInitialValue(
            "HLAreportServiceFile",
            rti1516_2025::HLAunicodeString{descriptor.reportServiceFile}.encode())) {
      return JoinedFederateMomObjectStatus::inconsistent_catalog;
    }
  } catch (rti1516_2025::EncoderException const&) {
    return JoinedFederateMomObjectStatus::invalid_descriptor;
  }

  std::uint64_t objectInstanceHandle = federation->second.nextObjectInstanceHandle;
  while (objectInstanceHandle == 0 ||
         objectInstanceHandle == std::numeric_limits<std::uint64_t>::max() ||
         federation->second.objectInstances.contains(objectInstanceHandle) ||
         federation->second.rtiOwnedJoinedFederateMomObjects.contains(objectInstanceHandle)) {
    if (objectInstanceHandle == std::numeric_limits<std::uint64_t>::max()) {
      return JoinedFederateMomObjectStatus::object_instance_handle_exhausted;
    }
    ++objectInstanceHandle;
  }

  auto const normalizedFederate = normalizedHandleValue(
      federation->second.normalizationSeed,
      federateId,
      kFederateNormalizationKind);
  JoinedFederateMomObjectSnapshot object;
  object.objectInstanceHandle = objectInstanceHandle;
  object.joinedFederateId = federateId;
  object.objectClassHandle = *objectClassHandle;
  object.immutableFederatePoint = {
      {*federateDimensionHandle},
      {{*federateDimensionHandle, {normalizedFederate, normalizedFederate + 1U}}},
      true,
  };
  object.effectiveAttributeHandles = std::move(effectiveAttributeHandles);
  object.periodicAttributeHandles = std::move(periodicAttributeHandles);
  object.initialAttributeValues = std::move(initialAttributeValues);

  auto const [position, inserted] = federation->second.rtiOwnedJoinedFederateMomObjects.emplace(
      objectInstanceHandle,
      std::move(object));
  static_cast<void>(position);
  if (!inserted) {
    return JoinedFederateMomObjectStatus::inconsistent_catalog;
  }
  federation->second.nextObjectInstanceHandle = objectInstanceHandle + 1U;
  return JoinedFederateMomObjectStatus::applied;
}

std::optional<JoinedFederateMomObjectSnapshot>
EmbeddedFederationRegistry::joinedFederateMomObjectFor(
    std::wstring const& federationName,
    std::uint64_t federateId) const {
  auto instrumentationScope = beginInstrumentation("joinedFederateMomObjectFor");
  std::scoped_lock lock(mutex_);
  auto const federation = federations_.find(federationName);
  if (federation == federations_.end()) {
    return std::nullopt;
  }
  for (auto const& [objectHandle, object] :
       federation->second.rtiOwnedJoinedFederateMomObjects) {
    static_cast<void>(objectHandle);
    if (object.joinedFederateId == federateId) {
      return object;
    }
  }
  return std::nullopt;
}

SynchronizationPointRegistrationPlan
EmbeddedFederationRegistry::registerSynchronizationPoint(
    std::wstring const& federationName,
    std::uint64_t registeringFederateId,
    std::wstring label,
    std::vector<unsigned char> userSuppliedTag,
    std::set<std::uint64_t> const& requestedSynchronizationSet) {
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

  plan.announcements.reserve(synchronizationSet.size());
  for (std::uint64_t federateId : synchronizationSet) {
    auto const route = federation->second.interactionCallbackRoutes.find(federateId);
    auto const reportRoute = federation->second.serviceReportRoutes.find(federateId);
    plan.announcements.push_back({
        federateId,
        label,
        point.userSuppliedTag,
        route->second,
        reportRoute == federation->second.serviceReportRoutes.end()
            ? FederateServiceReportRoute{}
            : reportRoute->second,
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
    if (!point.synchronizationSet.insert(newlyJoinedFederateId).second) {
      continue;
    }
    if (!point.announcedFederates.insert(newlyJoinedFederateId).second) {
      continue;
    }
    auto const reportRoute = federation->second.serviceReportRoutes.find(
        newlyJoinedFederateId);
    plan.announcements.push_back({
        newlyJoinedFederateId,
        label,
        point.userSuppliedTag,
        route->second,
        reportRoute == federation->second.serviceReportRoutes.end()
            ? FederateServiceReportRoute{}
            : reportRoute->second,
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
      std::nullopt);
}

FederationRegistryResult
EmbeddedFederationRegistry::resignWithFinalServiceReportReservation(
    std::wstring const& federationName,
    std::uint64_t federateId,
    rti1516_2025::ResignAction resignAction,
    std::uint16_t serviceGroup) {
  auto instrumentationScope = beginInstrumentation(
      "resignWithFinalServiceReportReservation");
  std::scoped_lock lock(mutex_);
  return resignLocked(
      federationName,
      federateId,
      resignAction,
      false,
      serviceGroup);
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
      std::nullopt);
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
      serviceGroup);
}

FederationRegistryResult EmbeddedFederationRegistry::resignLocked(
    std::wstring const& federationName,
    std::uint64_t federateId,
    rti1516_2025::ResignAction resignAction,
    bool forcedConnectionLoss,
    std::optional<std::uint16_t> finalServiceReportGroup) {
  auto federation = federations_.find(federationName);
  if (federation == federations_.end()) {
    return {FederationRegistryStatus::federation_does_not_exist};
  }

  auto member = federation->second.members.find(federateId);
  if (member == federation->second.members.end()) {
    return {FederationRegistryStatus::federate_not_member};
  }

  // Capture the time-regulating member's last granted position before the
  // forced resignation unregisters its temporal state. IEEE 1516.1-2025 4.4
  // requires delivery of its TSO messages at or before this point; messages
  // later than it intentionally receive no delivery guarantee.
  std::shared_ptr<rti1516_2025::LogicalTime const> connectionLossCutoff;
  if (forcedConnectionLoss) {
    for (auto const& candidate : federation->second.timeCoordinator.snapshot()) {
      if (candidate.federateId != federateId ||
          !candidate.time.timeRegulating ||
          !candidate.time.currentTime ||
          candidate.time.currentTime->implementationName() !=
              federation->second.definition.logicalTimeImplementationName) {
        continue;
      }
      connectionLossCutoff = candidate.time.currentTime;
      break;
    }
  }

  auto const retainPendingTimestampedDeletionForConnectionLoss =
      [&](Federation::ObjectInstance const& objectInstance) {
        if (!forcedConnectionLoss || !connectionLossCutoff ||
            !objectInstance.pendingTimestampedDeletionMessageId.has_value()) {
          return false;
        }
        auto const deletion = federation->second.tsoObjectDeletionMessages.find(
            *objectInstance.pendingTimestampedDeletionMessageId);
        if (deletion == federation->second.tsoObjectDeletionMessages.end() ||
            deletion->second.producingFederateId != federateId ||
            !deletion->second.timestamp) {
          return false;
        }
        try {
          // Clause 4.4's at-or-before-loss obligation applies to every TSO
          // message family. The accepted deletion and its object state must
          // survive every automatic-resign cleanup pass until the common
          // retraction-ledger pass marks its surviving recipients.
          return *deletion->second.timestamp <= *connectionLossCutoff;
        } catch (rti1516_2025::Exception const&) {
          // Do not manufacture the mandatory-delivery exception when a
          // private timestamp comparison is malformed.
          return false;
        }
      };

  FederationRegistryResult result;
  bool deleteObjects = false;
  bool divestAttributes = false;
  bool cancelPendingAcquisitions = false;
  switch (resignAction) {
    case rti1516_2025::UNCONDITIONALLY_DIVEST_ATTRIBUTES:
      divestAttributes = true;
      break;
    case rti1516_2025::DELETE_OBJECTS:
      deleteObjects = true;
      break;
    case rti1516_2025::CANCEL_PENDING_OWNERSHIP_ACQUISITIONS:
      cancelPendingAcquisitions = true;
      break;
    case rti1516_2025::DELETE_OBJECTS_THEN_DIVEST:
      deleteObjects = true;
      divestAttributes = true;
      break;
    case rti1516_2025::CANCEL_THEN_DELETE_THEN_DIVEST:
      cancelPendingAcquisitions = true;
      deleteObjects = true;
      divestAttributes = true;
      break;
    case rti1516_2025::NO_ACTION:
      break;
    default:
      return {FederationRegistryStatus::invalid_resign_action};
  }

  // A transport failure cannot leave private acquisition work attached to a
  // member that no longer exists. The automatic directive still controls
  // object deletion/divestiture, while this forced cleanup removes the lost
  // member's outstanding acquisition-side state.
  if (forcedConnectionLoss) {
    cancelPendingAcquisitions = true;
  }

  // IEEE 1516.1-2025 4.12.4 requires the RTI to process directive 2 when
  // the resigning federate is the last joined federate, regardless of the
  // action value supplied by that federate.  Keep the supplied directive's
  // other effects (for example, directive 1's divestiture) but add the
  // mandatory delete pass before the member is removed.
  bool const lastJoinedFederate = federation->second.members.size() == 1;
  if (lastJoinedFederate) {
    deleteObjects = true;
  }

  // 4.12.3 requires the RTI to reject actions that would leave an acquisition
  // attempt unresolved.  The request maps below are the private state for
  // regular and If Available acquisition; negotiated acquisition is included
  // when this federate is the prospective acquiring federate.
  bool pendingAcquisition = false;
  for (auto const& [objectInstanceHandle, objectInstance] :
       federation->second.objectInstances) {
    static_cast<void>(objectInstanceHandle);
    for (auto const& [requestId, request] :
         objectInstance.pendingAttributeOwnershipAcquisitionIfAvailableRequests) {
      static_cast<void>(requestId);
      if (request.requestingFederateId == federateId) {
        pendingAcquisition = true;
        break;
      }
    }
    if (!pendingAcquisition) {
      for (auto const& [requestId, request] :
           objectInstance.pendingAttributeOwnershipAcquisitionRequests) {
        static_cast<void>(requestId);
        if (request.requestingFederateId == federateId) {
          pendingAcquisition = true;
          break;
        }
      }
    }
    if (!pendingAcquisition) {
      for (auto const& [cancellationId, cancellation] :
           objectInstance.pendingAttributeOwnershipAcquisitionCancellations) {
        static_cast<void>(cancellationId);
        if (cancellation.requestingFederateId == federateId) {
          pendingAcquisition = true;
          break;
        }
      }
    }
    if (!pendingAcquisition) {
      for (auto const& [attributeHandle, divestiture] :
           objectInstance.pendingNegotiatedAttributeOwnershipDivestitures) {
        static_cast<void>(attributeHandle);
        if (divestiture.acquiringFederateId == federateId) {
          pendingAcquisition = true;
          break;
        }
      }
    }
    if (!pendingAcquisition) {
      for (auto const& [notificationId, notification] :
           objectInstance.pendingConfirmDivestitureNotifications) {
        static_cast<void>(notificationId);
        if (notification.receivingFederateId == federateId) {
          pendingAcquisition = true;
          break;
        }
      }
    }
    if (pendingAcquisition) {
      break;
    }
  }
  if (!forcedConnectionLoss && pendingAcquisition &&
      (resignAction == rti1516_2025::UNCONDITIONALLY_DIVEST_ATTRIBUTES ||
       resignAction == rti1516_2025::DELETE_OBJECTS ||
       resignAction == rti1516_2025::DELETE_OBJECTS_THEN_DIVEST ||
       resignAction == rti1516_2025::NO_ACTION)) {
    return {FederationRegistryStatus::ownership_acquisition_pending};
  }

  std::set<std::uint64_t> objectsToDelete;
  std::map<std::uint64_t, std::set<std::uint64_t>> ownedAttributesByObject;
  std::map<std::uint64_t, bool> deletePrivilegeByObject;
  for (auto const& [objectInstanceHandle, objectInstance] :
       federation->second.objectInstances) {
    if (objectInstance.deleteAccepted) {
      continue;
    }
    if (!federation->second.definition.catalog ||
        !federation->second.objectClassHandles ||
        !federation->second.attributeHandles) {
      return {FederationRegistryStatus::invalid_request};
    }
    auto const objectClassName = federation->second.objectClassHandles->nameFor(
        objectInstance.registeredObjectClassHandle);
    if (!objectClassName) {
      return {FederationRegistryStatus::invalid_request};
    }
    auto const privilegeToDelete = federation->second.attributeHandles->handleFor(
        federation->second.definition.catalog.get(),
        *objectClassName,
        "HLAprivilegeToDeleteObject");
    if (!privilegeToDelete) {
      return {FederationRegistryStatus::invalid_request};
    }
    auto const privilegeOwner = objectInstance.attributeOwnersByHandle.find(*privilegeToDelete);
    bool const hasDeletePrivilege =
        privilegeOwner != objectInstance.attributeOwnersByHandle.end() &&
        privilegeOwner->second == federateId;
    deletePrivilegeByObject.emplace(objectInstanceHandle, hasDeletePrivilege);
    if (hasDeletePrivilege && deleteObjects &&
        !retainPendingTimestampedDeletionForConnectionLoss(objectInstance)) {
      objectsToDelete.insert(objectInstanceHandle);
    }
    for (auto const& [attributeHandle, owner] : objectInstance.attributeOwnersByHandle) {
      if (owner == federateId) {
        ownedAttributesByObject[objectInstanceHandle].insert(attributeHandle);
      }
    }
  }

  bool ownsAttributes = false;
  for (auto const& [objectInstanceHandle, attributes] : ownedAttributesByObject) {
    static_cast<void>(objectInstanceHandle);
    if (!attributes.empty()) {
      ownsAttributes = true;
      break;
    }
  }
  if (forcedConnectionLoss && ownsAttributes) {
    // Once the member is forced out, any attributes it still owns must leave
    // the member's ownership set even when its configured directive was
    // NO_ACTION or DELETE_OBJECTS without delete privilege.
    divestAttributes = true;
  }
  if (!forcedConnectionLoss && !deleteObjects && !divestAttributes && ownsAttributes) {
    return {FederationRegistryStatus::federate_owns_attributes};
  }
  // Directive 2 is intentionally stricter than the combined delete-then-
  // divest forms: it can only delete objects for which the resigning federate
  // owns the delete privilege, and it may not strand another owned attribute.
  // The same rule applies to the mandatory final-federate directive-2 pass.
  if (!forcedConnectionLoss && deleteObjects && !divestAttributes) {
    for (auto const& [objectInstanceHandle, attributes] : ownedAttributesByObject) {
      if (!attributes.empty() && !deletePrivilegeByObject[objectInstanceHandle]) {
        return {FederationRegistryStatus::federate_owns_attributes};
      }
    }
  }

  // Validate every callback route needed by the action before changing any
  // object or ownership state.  This keeps an embedded profile failure
  // atomic and avoids an object becoming deleted without a delivery route.
  if (deleteObjects) {
    for (std::uint64_t const objectInstanceHandle : objectsToDelete) {
      auto const objectInstance = federation->second.objectInstances.find(objectInstanceHandle);
      if (objectInstance == federation->second.objectInstances.end()) {
        return {FederationRegistryStatus::invalid_request};
      }
      for (auto const& [receivingFederateId, knownClassHandle] :
           objectInstance->second.knownObjectClassHandlesByFederate) {
        static_cast<void>(knownClassHandle);
        if (receivingFederateId == federateId ||
            !federation->second.members.contains(receivingFederateId)) {
          continue;
        }
        auto const route = federation->second.interactionCallbackRoutes.find(receivingFederateId);
        if (route == federation->second.interactionCallbackRoutes.end() || !route->second) {
          if (forcedConnectionLoss) {
            continue;
          }
          return {FederationRegistryStatus::invalid_request};
        }
      }
    }
  }

  // RTI-owned joined-federate MOM objects leave the execution whenever their
  // represented joined federate resigns, independent of the supplied
  // federate-created-object resign action. Validate the surviving callback
  // routes before mutating the MOM ledger so a normal resignation remains
  // atomic when a removal cannot be queued.
  for (auto const& [objectInstanceHandle, object] :
       federation->second.rtiOwnedJoinedFederateMomObjects) {
    static_cast<void>(objectInstanceHandle);
    if (object.joinedFederateId != federateId) {
      continue;
    }
    for (std::uint64_t const receivingFederateId : object.knownFederateIds) {
      if (receivingFederateId == federateId ||
          !federation->second.members.contains(receivingFederateId)) {
        continue;
      }
      auto const route = federation->second.interactionCallbackRoutes.find(
          receivingFederateId);
      if (route == federation->second.interactionCallbackRoutes.end() ||
          !route->second) {
        if (forcedConnectionLoss) {
          continue;
        }
        return {FederationRegistryStatus::invalid_request};
      }
    }
  }

  if (federation->second.saveOperation.has_value()) {
    // A resigning participant makes the control-plane save fail.  Do this
    // before removing its callback route, but do not enqueue a callback back
    // to the federate that is leaving the execution.
    appendSaveCompletionNotifications(
        federation->second,
        false,
        rti1516_2025::FEDERATE_RESIGNED_DURING_SAVE,
        result.saveNotifications,
        federateId);
    federation->second.saveOperation.reset();
  }
  if (federation->second.restoreOperation.has_value()) {
    // A resigning participant invalidates an in-flight restore for the
    // remaining members before its callback route is removed.
    appendRestoreCompletionNotifications(
        federation->second,
        false,
        rti1516_2025::FEDERATE_RESIGNED_DURING_RESTORE,
        result.restoreNotifications,
        federateId);
    federation->second.restoreOperation.reset();
  }

  if (cancelPendingAcquisitions) {
    // Directive 3 (and the combined directive 5) cancels only acquisition
    // work initiated by the resigning federate.  Requests made by other
    // joined federates remain eligible for any attributes that are later
    // divested by this resignation.
    for (auto& [objectInstanceHandle, objectInstance] : federation->second.objectInstances) {
      static_cast<void>(objectInstanceHandle);
      for (auto pending =
               objectInstance.pendingAttributeOwnershipAcquisitionIfAvailableRequests.begin();
           pending != objectInstance.pendingAttributeOwnershipAcquisitionIfAvailableRequests.end();) {
        if (pending->second.requestingFederateId == federateId) {
          pending = objectInstance.pendingAttributeOwnershipAcquisitionIfAvailableRequests.erase(
              pending);
        } else {
          ++pending;
        }
      }
      for (auto pending = objectInstance.pendingAttributeOwnershipAcquisitionRequests.begin();
           pending != objectInstance.pendingAttributeOwnershipAcquisitionRequests.end();) {
        if (pending->second.requestingFederateId == federateId) {
          pending = objectInstance.pendingAttributeOwnershipAcquisitionRequests.erase(pending);
        } else {
          ++pending;
        }
      }
      for (auto pending =
               objectInstance.pendingAttributeOwnershipAcquisitionCancellations.begin();
           pending != objectInstance.pendingAttributeOwnershipAcquisitionCancellations.end();) {
        if (pending->second.requestingFederateId == federateId) {
          pending = objectInstance.pendingAttributeOwnershipAcquisitionCancellations.erase(pending);
        } else {
          ++pending;
        }
      }
      for (auto pending =
               objectInstance.pendingAttributeOwnershipDivestitureIfWantedNotifications.begin();
           pending != objectInstance.pendingAttributeOwnershipDivestitureIfWantedNotifications.end();) {
        if (pending->second.receivingFederateId == federateId) {
          pending = objectInstance.pendingAttributeOwnershipDivestitureIfWantedNotifications.erase(
              pending);
        } else {
          ++pending;
        }
      }
      for (auto pending =
               objectInstance.pendingNegotiatedAttributeOwnershipDivestitures.begin();
           pending != objectInstance.pendingNegotiatedAttributeOwnershipDivestitures.end();) {
        if (pending->second.acquiringFederateId == federateId) {
          pending = objectInstance.pendingNegotiatedAttributeOwnershipDivestitures.erase(pending);
        } else {
          ++pending;
        }
      }
      for (auto pending = objectInstance.pendingConfirmDivestitureNotifications.begin();
           pending != objectInstance.pendingConfirmDivestitureNotifications.end();) {
        if (pending->second.receivingFederateId == federateId) {
          pending = objectInstance.pendingConfirmDivestitureNotifications.erase(pending);
        } else {
          ++pending;
        }
      }
    }
  }

  // Delete-privileged objects are handled before any remaining ownership is
  // divested, matching the ordering of directives 4 and 5.  The registry
  // reserves one Remove Object Instance callback per remaining known
  // federate; the adapter submits those callbacks after this lock is gone.
  if (deleteObjects) {
    for (std::uint64_t const objectInstanceHandle : objectsToDelete) {
      auto objectInstance = federation->second.objectInstances.find(objectInstanceHandle);
      if (objectInstance == federation->second.objectInstances.end() ||
          objectInstance->second.deleteAccepted) {
        continue;
      }

      auto& instance = objectInstance->second;
      for (auto const& [receivingFederateId, knownClassHandle] :
           instance.knownObjectClassHandlesByFederate) {
        static_cast<void>(knownClassHandle);
        if (receivingFederateId == federateId ||
            !federation->second.members.contains(receivingFederateId)) {
          continue;
        }
        auto const route = federation->second.interactionCallbackRoutes.find(receivingFederateId);
        if (route == federation->second.interactionCallbackRoutes.end() || !route->second) {
          if (forcedConnectionLoss) {
            continue;
          }
          return {FederationRegistryStatus::invalid_request};
        }
        instance.pendingRemovalFederates.insert(receivingFederateId);
        if (forcedConnectionLoss) {
          instance.connectionLossAutomaticRemovalFederates.insert(receivingFederateId);
        }
        auto const reportRoute = federation->second.serviceReportRoutes.find(
            receivingFederateId);
        result.resignObjectRemovals.push_back({
            receivingFederateId,
            objectInstanceHandle,
            route->second,
            reportRoute == federation->second.serviceReportRoutes.end()
                ? FederateServiceReportRoute{}
                : reportRoute->second,
        });
      }

      instance.pendingDiscoveryFederates.clear();
      instance.pendingAttributeOwnershipAcquisitionIfAvailableRequests.clear();
      instance.pendingAttributeOwnershipAcquisitionRequests.clear();
      instance.pendingAttributeOwnershipAcquisitionCancellations.clear();
      instance.pendingAttributeOwnershipDivestitureIfWantedNotifications.clear();
      instance.pendingNegotiatedAttributeOwnershipDivestitures.clear();
      instance.pendingConfirmDivestitureNotifications.clear();
      if (instance.pendingTimestampedDeletionMessageId.has_value()) {
        auto const deletionMessageId = *instance.pendingTimestampedDeletionMessageId;
        federation->second.tsoObjectDeletionMessages.erase(
            deletionMessageId);
        federation->second.tsoObjectDeletionReconstitutionRecords.erase(deletionMessageId);
        federation->second.tsoRequestRetractionRecords.erase(deletionMessageId);
        instance.pendingTimestampedDeletionMessageId.reset();
      }
      instance.pendingTimestampedRemovalFederates.clear();
      instance.knownObjectClassHandlesByFederate.erase(federateId);
      instance.deleteAccepted = true;
      if (canPurgeDeletedObjectInstance(instance)) {
        federation->second.objectInstanceHandlesByName.erase(instance.name);
        federation->second.objectInstances.erase(objectInstance);
      }
    }
  }

  if (divestAttributes) {
    std::map<std::pair<std::uint64_t, std::uint64_t>, std::set<std::uint64_t>>
        offeredAttributesByRecipient;

    auto recipientHasPendingAcquisition = [](
                                             Federation::ObjectInstance const& instance,
                                             std::uint64_t receivingFederateId,
                                             std::uint64_t attributeHandle) {
      for (auto const& [requestId, pending] :
           instance.pendingAttributeOwnershipAcquisitionIfAvailableRequests) {
        static_cast<void>(requestId);
        if (pending.requestingFederateId == receivingFederateId &&
            pending.desiredAttributeHandles.contains(attributeHandle)) {
          return true;
        }
      }
      for (auto const& [requestId, pending] :
           instance.pendingAttributeOwnershipAcquisitionRequests) {
        static_cast<void>(requestId);
        if (pending.requestingFederateId == receivingFederateId &&
            pending.desiredAttributeHandles.contains(attributeHandle)) {
          return true;
        }
      }
      for (auto const& [cancellationId, cancellation] :
           instance.pendingAttributeOwnershipAcquisitionCancellations) {
        static_cast<void>(cancellationId);
        if (cancellation.requestingFederateId == receivingFederateId &&
            cancellation.attributeHandles.contains(attributeHandle)) {
          return true;
        }
      }
      return false;
    };

    for (auto objectInstance = federation->second.objectInstances.begin();
         objectInstance != federation->second.objectInstances.end();) {
      auto& instance = objectInstance->second;
      if (instance.deleteAccepted) {
        ++objectInstance;
        continue;
      }
      auto owned = ownedAttributesByObject.find(instance.handle);
      if (owned == ownedAttributesByObject.end() || owned->second.empty()) {
        ++objectInstance;
        continue;
      }

      for (std::uint64_t const attributeHandle : owned->second) {
        // Preserve a search record even when every remaining federate is
        // currently unknown, unpublished, or already acquiring. Later
        // eligibility changes must be able to continue the search.
        instance.ownershipAssumptionRecipientsByAttribute.try_emplace(attributeHandle);
      }

      for (std::uint64_t const attributeHandle : owned->second) {
        for (auto const& [receivingFederateId, membership] : federation->second.members) {
          static_cast<void>(membership);
          if (receivingFederateId == federateId) {
            continue;
          }
          auto const knownClass = instance.knownObjectClassHandlesByFederate.find(
              receivingFederateId);
          if (knownClass == instance.knownObjectClassHandlesByFederate.end() ||
              recipientHasPendingAcquisition(instance, receivingFederateId, attributeHandle)) {
            continue;
          }
          auto const knownClassName = federation->second.objectClassHandles->nameFor(
              knownClass->second);
          auto const publishedAttributes = knownClassName
              ? publishedObjectClassAttributes(
                    federation->second,
                    receivingFederateId,
                    knownClass->second)
              : std::nullopt;
          if (!knownClassName || !publishedAttributes ||
              !federation->second.attributeHandles->nameFor(
                  federation->second.definition.catalog.get(),
                  *knownClassName,
                  attributeHandle) ||
              !publishedAttributes->contains(attributeHandle)) {
            continue;
          }
          auto const route = federation->second.interactionCallbackRoutes.find(receivingFederateId);
          if (route == federation->second.interactionCallbackRoutes.end() || !route->second) {
            if (forcedConnectionLoss) {
              continue;
            }
            return {FederationRegistryStatus::invalid_request};
          }
          offeredAttributesByRecipient[{receivingFederateId, instance.handle}].insert(
              attributeHandle);
        }

        instance.attributeOwnersByHandle.erase(attributeHandle);
        clearUpdateRegionAssociation(instance, attributeHandle);
        instance.attributeTransportationTypes.erase(attributeHandle);
        instance.attributeOrderTypes.erase(attributeHandle);
        for (auto pending = instance.pendingAttributeTransportationTypeChanges.begin();
             pending != instance.pendingAttributeTransportationTypeChanges.end();) {
          pending->second.attributeHandles.erase(attributeHandle);
          if (pending->second.attributeHandles.empty()) {
            pending = instance.pendingAttributeTransportationTypeChanges.erase(pending);
          } else {
            ++pending;
          }
        }
        instance.pendingNegotiatedAttributeOwnershipDivestitures.erase(attributeHandle);
      }

      auto followupWorkItems = planPendingAttributeOwnershipAcquisitionWork(
          federation->second,
          instance);
      result.resignOwnershipAcquisitionWorkItems.insert(
          result.resignOwnershipAcquisitionWorkItems.end(),
          std::make_move_iterator(followupWorkItems.begin()),
          std::make_move_iterator(followupWorkItems.end()));
      ++objectInstance;
    }

    for (auto& [recipientKey, attributes] : offeredAttributesByRecipient) {
      auto const route = federation->second.interactionCallbackRoutes.find(recipientKey.first);
      if (route == federation->second.interactionCallbackRoutes.end() || !route->second) {
        if (forcedConnectionLoss) {
          continue;
        }
        return {FederationRegistryStatus::invalid_request};
      }
      auto const instance = federation->second.objectInstances.find(recipientKey.second);
      if (instance == federation->second.objectInstances.end()) {
        return {FederationRegistryStatus::invalid_request};
      }
      for (std::uint64_t const attributeHandle : attributes) {
        instance->second.ownershipAssumptionRecipientsByAttribute[attributeHandle].insert(
            recipientKey.first);
      }
      result.resignOwnershipAssumptions.push_back({
          recipientKey.first,
          recipientKey.second,
          std::move(attributes),
          route->second,
      });
    }
  }

  static_cast<void>(federation->second.timeCoordinator.unregisterFederate(federateId));
  federation->second.pendingTimeAdvanceGrants.erase(federateId);
  federation->second.timeAdvanceGrantDispatchFactories.erase(federateId);
  federation->second.interactionDeclarations.erase(federateId);
  federation->second.objectClassAttributeDeclarations.erase(federateId);
  federation->second.interactionCallbackRoutes.erase(federateId);
  federation->second.serviceReportRoutes.erase(federateId);
  for (auto relevance = federation->second.objectClassRegistrationRelevance.begin();
       relevance != federation->second.objectClassRegistrationRelevance.end();) {
    if (relevance->first == federateId) {
      relevance = federation->second.objectClassRegistrationRelevance.erase(relevance);
    } else {
      ++relevance;
    }
  }
  for (auto relevance = federation->second.interactionRelevance.begin();
       relevance != federation->second.interactionRelevance.end();) {
    if (relevance->first == federateId) {
      relevance = federation->second.interactionRelevance.erase(relevance);
    } else {
      ++relevance;
    }
  }
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
    objectInstance->second.connectionLossAutomaticRemovalFederates.erase(federateId);
    objectInstance->second.deferredConnectionLossTsoRemovalFederates.erase(federateId);
    objectInstance->second.pendingTimestampedRemovalFederates.erase(federateId);
    bool const retainPendingDeletionForConnectionLoss =
        retainPendingTimestampedDeletionForConnectionLoss(objectInstance->second);
    if (objectInstance->second.producingFederateId == federateId &&
        objectInstance->second.pendingTimestampedDeletionMessageId.has_value() &&
        !retainPendingDeletionForConnectionLoss) {
      auto const deletionMessageId =
          *objectInstance->second.pendingTimestampedDeletionMessageId;
      federation->second.tsoObjectDeletionMessages.erase(
          deletionMessageId);
      federation->second.tsoObjectDeletionReconstitutionRecords.erase(deletionMessageId);
      federation->second.tsoRequestRetractionRecords.erase(deletionMessageId);
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
    for (auto assumption =
             objectInstance->second.ownershipAssumptionRecipientsByAttribute.begin();
         assumption !=
             objectInstance->second.ownershipAssumptionRecipientsByAttribute.end();) {
      assumption->second.erase(federateId);
      ++assumption;
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
  for (auto point = federation->second.synchronizationPoints.begin();
       point != federation->second.synchronizationPoints.end();) {
    point->second.synchronizationSet.erase(federateId);
    point->second.announcedFederates.erase(federateId);
    point->second.achievedFederates.erase(federateId);

    bool complete = point->second.synchronizationSet.empty();
    if (!complete) {
      complete = true;
      for (std::uint64_t synchronizationFederateId : point->second.synchronizationSet) {
        if (!point->second.achievedFederates.contains(synchronizationFederateId)) {
          complete = false;
          break;
        }
      }
    }
    if (complete) {
      std::set<std::uint64_t> failedToSyncFederates;
      for (auto const& [synchronizationFederateId, federateSucceeded] :
           point->second.achievedFederates) {
        if (!federateSucceeded) {
          failedToSyncFederates.insert(synchronizationFederateId);
        }
      }
      for (std::uint64_t synchronizationFederateId : point->second.synchronizationSet) {
        auto const route = federation->second.interactionCallbackRoutes.find(
            synchronizationFederateId);
        auto const reportRoute = federation->second.serviceReportRoutes.find(
            synchronizationFederateId);
        if (route == federation->second.interactionCallbackRoutes.end() || !route->second) {
          continue;
        }
        result.synchronizationNotifications.push_back({
            point->first,
            failedToSyncFederates,
            route->second,
            reportRoute == federation->second.serviceReportRoutes.end()
                ? FederateServiceReportRoute{}
                : reportRoute->second,
        });
      }
      point = federation->second.synchronizationPoints.erase(point);
    } else {
      ++point;
    }
  }

  // A federate that has left the execution can never invoke Retract again.
  // Keep any existing record as a lightweight tombstone in case an already
  // queued Request Retraction callback still needs its recipient state, but
  // release its timestamp once no remaining recipient needs typed payload.
  auto timestampForTsoPayload = [&federation](std::uint64_t messageId) {
    if (auto const interaction = federation->second.tsoInteractionMessages.find(messageId);
        interaction != federation->second.tsoInteractionMessages.end()) {
      return interaction->second.timestamp;
    }
    if (auto const update = federation->second.tsoAttributeUpdateMessages.find(messageId);
        update != federation->second.tsoAttributeUpdateMessages.end()) {
      return update->second.timestamp;
    }
    if (auto const deletion = federation->second.tsoObjectDeletionMessages.find(messageId);
        deletion != federation->second.tsoObjectDeletionMessages.end()) {
      return deletion->second.timestamp;
    }
    if (auto const directed = federation->second.tsoDirectedInteractionMessages.find(messageId);
        directed != federation->second.tsoDirectedInteractionMessages.end()) {
      return directed->second.timestamp;
    }
    return std::shared_ptr<rti1516_2025::LogicalTime const>{};
  };
  for (auto& [messageId, record] : federation->second.tsoRequestRetractionRecords) {
    if (record.producingFederateId == federateId) {
      auto const messageTimestamp = record.timestamp
          ? record.timestamp
          : timestampForTsoPayload(messageId);
      if (connectionLossCutoff && messageTimestamp) {
        try {
          record.deliveryRequiredAfterConnectionLoss =
              *messageTimestamp <= *connectionLossCutoff;
        } catch (rti1516_2025::Exception const&) {
          // A malformed private time value must not prevent an authoritative
          // connection-loss cleanup. Without a valid comparison, retain no
          // manufactured mandatory-delivery marker.
          record.deliveryRequiredAfterConnectionLoss = false;
        }
      }
      record.terminal = true;
      record.timestamp.reset();
    }
  }
  // One RTI-owned joined-federate MOM object has exactly the active joined
  // membership's lifetime. It is deliberately not subjected to the
  // resign-action object/ownership machinery above: that machinery governs
  // federate-created instances. Retain a snapshot until every surviving
  // known recipient crosses its callback-time Remove Object Instance
  // boundary, just as the ordinary object ledger retains a deleted instance.
  for (auto momObject = federation->second.rtiOwnedJoinedFederateMomObjects.begin();
       momObject != federation->second.rtiOwnedJoinedFederateMomObjects.end();) {
    if (momObject->second.joinedFederateId == federateId) {
      momObject->second.pendingDiscoveryFederateIds.clear();
      momObject->second.knownFederateIds.erase(federateId);
      for (std::uint64_t const receivingFederateId : momObject->second.knownFederateIds) {
        if (!federation->second.members.contains(receivingFederateId)) {
          continue;
        }
        auto const route = federation->second.interactionCallbackRoutes.find(
            receivingFederateId);
        if (route == federation->second.interactionCallbackRoutes.end() ||
            !route->second) {
          // Forced connection-loss cleanup has already validated that this
          // recipient may be skipped. A normal resignation cannot reach this
          // branch because the route validation above is atomic.
          continue;
        }
        momObject->second.pendingRemovalFederateIds.insert(receivingFederateId);
        auto const reportRoute = federation->second.serviceReportRoutes.find(
            receivingFederateId);
        result.resignObjectRemovals.push_back({
            receivingFederateId,
            momObject->second.objectInstanceHandle,
            route->second,
            reportRoute == federation->second.serviceReportRoutes.end()
                ? FederateServiceReportRoute{}
                : reportRoute->second,
            true,
        });
      }
      if (momObject->second.pendingRemovalFederateIds.empty()) {
        momObject = federation->second.rtiOwnedJoinedFederateMomObjects.erase(momObject);
      } else {
        ++momObject;
      }
    } else {
      ++momObject;
    }
  }
  if (finalServiceReportGroup.has_value()) {
    // Every normal rejection path has already returned above.  Reserve only
    // the file route: interaction delivery remains separately source-gated,
    // exactly as it does in the ordinary post-service helper.  This has to
    // happen before the membership is erased so the final serial cannot be
    // lost or reused by a later joined-federate lifetime.
    auto const reportPlan = momServiceReportRoutingPlanFor(
        federation->second,
        federateId,
        *finalServiceReportGroup);
    if (reportPlan.disposition == MomServiceReportDisposition::report_to_file) {
      result.finalServiceReportFileSerialNumber =
          member->second.nextMomServiceReportSerialNumber;
      ++member->second.nextMomServiceReportSerialNumber;
    }
  }
  federation->second.memberIdsByName.erase(member->second.name);
  federation->second.members.erase(member);
  reclaimTsoMessagePayloads(federation->second);
  refreshRegionUsage(federation->second);
  return result;
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
    notifications.push_back(std::move(notification));
  }
}

void EmbeddedFederationRegistry::restoreFederationFromSnapshot(
    Federation& target,
    Federation const& snapshot) {
  // Callback routes are live connection endpoints, not serialized federation
  // state. Keep the routes registered by the current ambassadors while
  // replacing every other mutable federation component with the saved view.
  auto const& timeAdvanceGrantDispatchFactories =
      target.timeAdvanceGrantDispatchFactories;

  // A report serial is a per-joined-federate audit sequence, not federated
  // application state.  Section 11.5.1 requires it to start at zero and
  // increment for every service-report invocation through the joined
  // federate's lifetime.  Preserve the live counters across a save/restore
  // image so restored state cannot reuse a serial already durably written to
  // that federate's report file.
  std::map<std::uint64_t, std::uint32_t> liveMomServiceReportSerialNumbers;
  for (auto const& [federateId, member] : target.members) {
    liveMomServiceReportSerialNumbers.emplace(
        federateId,
        member.nextMomServiceReportSerialNumber);
  }

  // A saved time state can remain Time Advancing after all federates have
  // completed the save. The opaque callback closure cannot be serialized, so
  // recreate it from the current ambassador's live factory before mutating
  // the federation. A nonzero identity makes any pre-restore closure a
  // harmless stale dispatch even when the saved generation is reused.
  std::map<std::uint64_t, Federation::PendingTimeAdvanceGrant>
      reconstitutedTimeAdvanceGrants;
  auto nextTimeAdvanceGrantDispatchIdentity = target.nextTimeAdvanceGrantDispatchIdentity;
  for (auto const& savedTimeState : snapshot.timeCoordinator.snapshot()) {
    if (!savedTimeState.time.timeAdvancePending) {
      continue;
    }
    if (savedTimeState.federateId == 0 ||
        savedTimeState.time.pendingTimeAdvanceGeneration == 0 ||
        savedTimeState.time.advanceMode == FederateTimeAdvanceMode::none) {
      throw std::logic_error(
          "A saved pending time advance has incomplete private state.");
    }
    auto const factory = timeAdvanceGrantDispatchFactories.find(savedTimeState.federateId);
    if (factory == timeAdvanceGrantDispatchFactories.end() || !factory->second) {
      throw std::logic_error(
          "A saved pending time advance has no live callback dispatcher.");
    }
    if (nextTimeAdvanceGrantDispatchIdentity == 0 ||
        nextTimeAdvanceGrantDispatchIdentity == std::numeric_limits<std::uint64_t>::max()) {
      throw std::overflow_error(
          "The embedded federation exhausted restored time-advance dispatch identities.");
    }
    auto const dispatchIdentity = nextTimeAdvanceGrantDispatchIdentity++;
    auto dispatch = factory->second(
        savedTimeState.federateId,
        savedTimeState.time.pendingTimeAdvanceGeneration,
        dispatchIdentity);
    if (!dispatch) {
      throw std::logic_error(
          "The embedded federation could not recreate a saved time-advance callback.");
    }
    auto const [position, inserted] = reconstitutedTimeAdvanceGrants.emplace(
        savedTimeState.federateId,
        Federation::PendingTimeAdvanceGrant{
            savedTimeState.time.pendingTimeAdvanceGeneration,
            dispatchIdentity,
            std::move(dispatch),
            false,
        });
    static_cast<void>(position);
    if (!inserted) {
      throw std::logic_error(
          "The saved federation contains duplicate pending time advances.");
    }
  }

  // Keep designators issued after the save in addition to those known to the
  // snapshot. A valid federate designator is not invalidated merely because
  // its federate subsequently resigned, and this index never authorizes
  // membership-sensitive behavior.
  auto federateNamesById = target.federateNamesById;
  federateNamesById.insert(
      snapshot.federateNamesById.begin(), snapshot.federateNamesById.end());

  target.definition = snapshot.definition;
  target.normalizationSeed = snapshot.normalizationSeed;
  target.autoProvideSwitch = snapshot.autoProvideSwitch;
  target.advisoriesUseKnownClassSwitch = snapshot.advisoriesUseKnownClassSwitch;
  target.nonRegulatedGrantSwitch = snapshot.nonRegulatedGrantSwitch;
  target.delaySubscriptionEvaluationSwitch = snapshot.delaySubscriptionEvaluationSwitch;
  target.allowRelaxedDDMSwitch = snapshot.allowRelaxedDDMSwitch;
  target.objectClassHandles = snapshot.objectClassHandles;
  target.attributeHandles = snapshot.attributeHandles;
  target.interactionClassHandles = snapshot.interactionClassHandles;
  target.parameterHandles = snapshot.parameterHandles;
  target.dimensionHandles = snapshot.dimensionHandles;
  target.members = snapshot.members;
  for (auto& [federateId, member] : target.members) {
    auto const liveSerial = liveMomServiceReportSerialNumbers.find(federateId);
    if (liveSerial == liveMomServiceReportSerialNumbers.end()) {
      throw std::logic_error(
          "A restored federation member has no live service-report serial state.");
    }
    member.nextMomServiceReportSerialNumber = std::max(
        member.nextMomServiceReportSerialNumber,
        liveSerial->second);
  }
  target.memberIdsByName = snapshot.memberIdsByName;
  target.federateNamesById = std::move(federateNamesById);
  target.interactionDeclarations = snapshot.interactionDeclarations;
  target.synchronizationPoints = snapshot.synchronizationPoints;
  target.objectClassAttributeDeclarations = snapshot.objectClassAttributeDeclarations;
  target.objectClassRegistrationRelevance = snapshot.objectClassRegistrationRelevance;
  target.interactionRelevance = snapshot.interactionRelevance;
  target.timeCoordinator.restoreFrom(snapshot.timeCoordinator);
  target.tsoInteractionMessages = snapshot.tsoInteractionMessages;
  target.tsoRequestRetractionRecords = snapshot.tsoRequestRetractionRecords;
  target.tsoAttributeUpdateMessages = snapshot.tsoAttributeUpdateMessages;
  target.tsoObjectDeletionMessages = snapshot.tsoObjectDeletionMessages;
  target.tsoObjectDeletionReconstitutionRecords =
      snapshot.tsoObjectDeletionReconstitutionRecords;
  target.tsoDirectedInteractionMessages = snapshot.tsoDirectedInteractionMessages;
  // A queued grant callback belongs to the pre-restore execution. Replace
  // that work with the fresh live-route dispatches prepared above instead of
  // copying a stale std::function from the saved federation image.
  target.pendingTimeAdvanceGrants = std::move(reconstitutedTimeAdvanceGrants);
  target.regions = snapshot.regions;
  target.rtiOwnedJoinedFederateMomObjects = snapshot.rtiOwnedJoinedFederateMomObjects;
  target.objectInstances = snapshot.objectInstances;
  target.objectInstanceHandlesByName = snapshot.objectInstanceHandlesByName;
  target.reservedObjectInstanceNamesByFederate =
      snapshot.reservedObjectInstanceNamesByFederate;
  target.nextRegionHandle = snapshot.nextRegionHandle;
  target.nextObjectInstanceHandle = snapshot.nextObjectInstanceHandle;
  target.nextAttributeOwnershipAcquisitionIfAvailableRequestId =
      snapshot.nextAttributeOwnershipAcquisitionIfAvailableRequestId;
  target.nextAttributeOwnershipAcquisitionRequestId =
      snapshot.nextAttributeOwnershipAcquisitionRequestId;
  target.nextAttributeOwnershipAcquisitionRequestSequence =
      snapshot.nextAttributeOwnershipAcquisitionRequestSequence;
  target.nextAttributeOwnershipAcquisitionCancellationId =
      snapshot.nextAttributeOwnershipAcquisitionCancellationId;
  target.nextAttributeOwnershipDivestitureIfWantedNotificationId =
      snapshot.nextAttributeOwnershipDivestitureIfWantedNotificationId;
  target.nextConfirmDivestitureNotificationId = snapshot.nextConfirmDivestitureNotificationId;
  target.nextAttributeTransportationTypeChangeRequestId =
      snapshot.nextAttributeTransportationTypeChangeRequestId;
  target.nextTimeAdvanceGrantDispatchIdentity = nextTimeAdvanceGrantDispatchIdentity;
  target.saveOperation.reset();
  target.pendingImmediateSave = snapshot.pendingImmediateSave;
  target.pendingTimedSave = snapshot.pendingTimedSave;
  target.restoreOperation.reset();
}

bool EmbeddedFederationRegistry::startFederationSave(
    Federation& federation,
    std::wstring label,
    FederationSaveControlResult& result,
    std::shared_ptr<rti1516_2025::LogicalTime const> timestamp) {
  // Validate every callback route before changing the shared save state.  A
  // timed request may become ready at a callback boundary; this keeps that
  // transition atomic if a member disconnected in the meantime.
  for (auto const& [federateId, member] : federation.members) {
    static_cast<void>(member);
    auto const route = federation.interactionCallbackRoutes.find(federateId);
    if (route == federation.interactionCallbackRoutes.end() || !route->second) {
      result.status = FederationSaveControlStatus::callback_route_missing;
      return false;
    }
  }

  Federation::SaveOperation operation;
  operation.label = std::move(label);
  operation.scheduledSaveTime = std::move(timestamp);
  for (auto const& [federateId, member] : federation.members) {
    static_cast<void>(member);
    operation.statuses.emplace(federateId, rti1516_2025::FEDERATE_INSTRUCTED_TO_SAVE);
  }
  federation.saveOperation = std::move(operation);
  // Once Initiate Federate Save is emitted there is no longer an outstanding
  // requested (but not yet initiated) save of either form.
  federation.pendingImmediateSave.reset();
  federation.pendingTimedSave.reset();

  result.notifications.reserve(federation.saveOperation->statuses.size());
  for (auto const& [federateId, status] : federation.saveOperation->statuses) {
    static_cast<void>(status);
    auto const route = federation.interactionCallbackRoutes.find(federateId);
    auto const reportRoute = federation.serviceReportRoutes.find(federateId);
    FederationSaveNotification notification;
    notification.kind = FederationSaveNotificationKind::initiate;
    notification.receivingFederateId = federateId;
    notification.label = federation.saveOperation->label;
    notification.timestamp = federation.saveOperation->scheduledSaveTime;
    notification.callbackRoute = route->second;
    notification.serviceReportRoute =
        reportRoute == federation.serviceReportRoutes.end()
        ? FederateServiceReportRoute{}
        : reportRoute->second;
    result.notifications.push_back(std::move(notification));
  }
  result.status = FederationSaveControlStatus::applied;
  return true;
}

FederationSaveControlResult EmbeddedFederationRegistry::requestFederationSave(
    std::wstring const& federationName,
    std::uint64_t requestingFederateId,
    std::wstring label) {
  auto instrumentationScope = beginInstrumentation("requestFederationSave");
  FederationSaveControlResult result;

  std::scoped_lock lock(mutex_);
  auto federation = federations_.find(federationName);
  if (federation == federations_.end()) {
    result.status = FederationSaveControlStatus::federation_does_not_exist;
    return result;
  }
  if (!federation->second.members.contains(requestingFederateId)) {
    result.status = FederationSaveControlStatus::federate_not_member;
    return result;
  }
  if (federation->second.saveOperation.has_value()) {
    result.status = FederationSaveControlStatus::save_in_progress;
    return result;
  }
  if (federation->second.restoreOperation.has_value()) {
    result.status = FederationSaveControlStatus::restore_in_progress;
    return result;
  }
  auto const execution = makeTimeSnapshot(federation->second);
  if (!execution) {
    result.status = FederationSaveControlStatus::inconsistent_temporal_state;
    return result;
  }
  auto const hasTimeConstrainedMember = std::any_of(
      execution->federates.begin(),
      execution->federates.end(),
      [](FederationTimeFederateSnapshot const& federate) {
        return federate.time.timeConstrained;
      });
  if (!hasTimeConstrainedMember) {
    // An immediate request supersedes any not-yet-initiated request only after
    // all callback routes have been validated by startFederationSave().
    static_cast<void>(startFederationSave(federation->second, std::move(label), result));
    return result;
  }

  // A time-constrained member may receive Initiate Federate Save only at its
  // Time Advancing boundary. Validate every route before replacing a pending
  // timed request, then retain this untimed request until the grant dispatcher
  // can invoke the constrained callbacks directly before their grants.
  for (auto const& [federateId, member] : federation->second.members) {
    static_cast<void>(member);
    auto const route = federation->second.interactionCallbackRoutes.find(federateId);
    if (route == federation->second.interactionCallbackRoutes.end() || !route->second) {
      result.status = FederationSaveControlStatus::callback_route_missing;
      return result;
    }
  }
  federation->second.pendingImmediateSave = Federation::PendingImmediateSave{
      std::move(label)};
  federation->second.pendingTimedSave.reset();
  return result;
}

FederationSaveAdmission
EmbeddedFederationRegistry::admitImmediateFederationSaveAtTimeAdvanceBoundary(
    std::wstring const& federationName,
    std::uint64_t federateId) {
  FederationSaveAdmission result;

  std::scoped_lock lock(mutex_);
  auto federation = federations_.find(federationName);
  if (federation == federations_.end()) {
    result.status = FederationSaveControlStatus::federation_does_not_exist;
    return result;
  }
  if (!federation->second.members.contains(federateId)) {
    result.status = FederationSaveControlStatus::federate_not_member;
    return result;
  }
  if (federation->second.restoreOperation.has_value()) {
    result.status = FederationSaveControlStatus::restore_in_progress;
    return result;
  }
  // The ordinary pre-grant helper runs before timestamped TSO delivery in the
  // shared dispatcher. Once a timestamped operation has started for another
  // constrained member, it must not consume this member's direct initiation:
  // its own timed helper will run only after the recipient's TSO payloads at
  // or below the scheduled save time have completed.
  if (federation->second.saveOperation.has_value() &&
      federation->second.saveOperation->scheduledSaveTime) {
    return result;
  }

  auto const execution = makeTimeSnapshot(federation->second);
  if (!execution) {
    result.status = FederationSaveControlStatus::inconsistent_temporal_state;
    return result;
  }
  auto const current = std::find_if(
      execution->federates.begin(),
      execution->federates.end(),
      [federateId](FederationTimeFederateSnapshot const& candidate) {
        return candidate.membership.id == federateId;
      });
  if (current == execution->federates.end()) {
    result.status = FederationSaveControlStatus::federate_not_member;
    return result;
  }

  if (!federation->second.saveOperation.has_value()) {
    if (!federation->second.pendingImmediateSave.has_value()) {
      return result;
    }

    // The source requires a constrained member to receive the callback in
    // Time Advancing. Do not create the operation until every constrained
    // member is simultaneously at such a callback-capable boundary. This
    // lets the later per-recipient dispatch invoke each constrained callback
    // before it changes that federate to Time Granted.
    bool everyConstrainedMemberIsTimeAdvancing = true;
    for (auto const& candidate : execution->federates) {
      if (candidate.time.timeConstrained && !candidate.time.timeAdvancePending) {
        everyConstrainedMemberIsTimeAdvancing = false;
        break;
      }
    }
    if (!everyConstrainedMemberIsTimeAdvancing) {
      return result;
    }
    if (!current->time.timeConstrained || !current->time.timeAdvancePending) {
      return result;
    }

    for (auto const& [memberId, member] : federation->second.members) {
      static_cast<void>(member);
      auto const route = federation->second.interactionCallbackRoutes.find(memberId);
      if (route == federation->second.interactionCallbackRoutes.end() || !route->second) {
        result.status = FederationSaveControlStatus::callback_route_missing;
        return result;
      }
    }

    Federation::SaveOperation operation;
    operation.label = federation->second.pendingImmediateSave->label;
    operation.directTimeConstrainedInitiation = true;
    for (auto const& candidate : execution->federates) {
      operation.statuses.emplace(candidate.membership.id, rti1516_2025::NO_SAVE_IN_PROGRESS);
      if (candidate.time.timeConstrained) {
        operation.pendingTimeConstrainedInitiations.insert(candidate.membership.id);
      }
    }
    federation->second.saveOperation = std::move(operation);
    federation->second.pendingImmediateSave.reset();
  }

  auto& operation = *federation->second.saveOperation;
  if (!operation.directTimeConstrainedInitiation ||
      !operation.pendingTimeConstrainedInitiations.contains(federateId)) {
    return result;
  }
  if (!current->time.timeConstrained || !current->time.timeAdvancePending) {
    return result;
  }

  auto currentStatus = operation.statuses.find(federateId);
  if (currentStatus == operation.statuses.end() ||
      currentStatus->second != rti1516_2025::NO_SAVE_IN_PROGRESS) {
    return result;
  }
  currentStatus->second = rti1516_2025::FEDERATE_INSTRUCTED_TO_SAVE;
  operation.pendingTimeConstrainedInitiations.erase(federateId);
  result.currentFederateLabel = operation.label;

  if (!operation.pendingTimeConstrainedInitiations.empty()) {
    return result;
  }

  // The standard permits the non-time-constrained members to be instructed
  // only after every constrained member has become eligible. At this point
  // every constrained member has reached a direct Time Advancing callback
  // boundary, so queue the remaining ordinary callbacks after releasing the
  // federation lock.
  operation.directTimeConstrainedInitiation = false;
  for (auto& [memberId, status] : operation.statuses) {
    if (status != rti1516_2025::NO_SAVE_IN_PROGRESS) {
      continue;
    }
    auto const route = federation->second.interactionCallbackRoutes.find(memberId);
    if (route == federation->second.interactionCallbackRoutes.end() || !route->second) {
      result.status = FederationSaveControlStatus::callback_route_missing;
      return result;
    }
    auto const reportRoute = federation->second.serviceReportRoutes.find(memberId);
    status = rti1516_2025::FEDERATE_INSTRUCTED_TO_SAVE;
    FederationSaveNotification notification;
    notification.kind = FederationSaveNotificationKind::initiate;
    notification.receivingFederateId = memberId;
    notification.label = operation.label;
    notification.callbackRoute = route->second;
    notification.serviceReportRoute =
        reportRoute == federation->second.serviceReportRoutes.end()
        ? FederateServiceReportRoute{}
        : reportRoute->second;
    result.notifications.push_back(std::move(notification));
  }
  return result;
}

FederationSaveControlResult EmbeddedFederationRegistry::requestFederationSave(
    std::wstring const& federationName,
    std::uint64_t requestingFederateId,
    std::wstring label,
    std::shared_ptr<rti1516_2025::LogicalTime const> timestamp) {
  auto instrumentationScope = beginInstrumentation("requestFederationSave");
  FederationSaveControlResult result;

  std::scoped_lock lock(mutex_);
  auto federation = federations_.find(federationName);
  if (federation == federations_.end()) {
    result.status = FederationSaveControlStatus::federation_does_not_exist;
    return result;
  }
  if (!federation->second.members.contains(requestingFederateId)) {
    result.status = FederationSaveControlStatus::federate_not_member;
    return result;
  }
  if (federation->second.saveOperation.has_value()) {
    result.status = FederationSaveControlStatus::save_in_progress;
    return result;
  }
  if (federation->second.restoreOperation.has_value()) {
    result.status = FederationSaveControlStatus::restore_in_progress;
    return result;
  }
  if (!timestamp || timestamp->isInitial() || timestamp->isFinal() ||
      timestamp->implementationName() != federation->second.definition.logicalTimeImplementationName) {
    result.status = FederationSaveControlStatus::invalid_timed_save;
    return result;
  }

  // The public adapter performs the caller-side time-regulation/GALT and
  // lower-bound checks.  Repeat the representation checks here because this
  // registry is also exercised directly by the embedded profile tests.
  auto const execution = makeTimeSnapshot(federation->second);
  if (!execution) {
    result.status = FederationSaveControlStatus::invalid_timed_save;
    return result;
  }
  for (auto const& federate : execution->federates) {
    if (federate.time.implementationName !=
            federation->second.definition.logicalTimeImplementationName ||
        !federate.time.currentTime) {
      if (federate.time.timeConstrained) {
        result.status = FederationSaveControlStatus::invalid_timed_save;
        return result;
      }
      continue;
    }
    try {
      if (federate.time.timeConstrained && *timestamp <= *federate.time.currentTime) {
        result.status = FederationSaveControlStatus::invalid_timed_save;
        return result;
      }
    } catch (rti1516_2025::Exception const&) {
      result.status = FederationSaveControlStatus::invalid_timed_save;
      return result;
    }
  }

  // IEEE 1516.1 permits one outstanding requested save; a later request
  // replaces it, including its label and timestamp.  Re-evaluate immediately
  // so a federation with no constrained members starts without waiting for a
  // future time callback.
  federation->second.pendingImmediateSave.reset();
  federation->second.pendingTimedSave = Federation::PendingTimedSave{
      std::move(label), std::move(timestamp)};

  auto pending = federation->second.pendingTimedSave;
  bool ready = true;
  for (auto const& federate : execution->federates) {
    if (!federate.time.timeConstrained) {
      continue;
    }
    if (!federate.time.currentTime) {
      ready = false;
      break;
    }
    try {
      if (*federate.time.currentTime < *pending->timestamp) {
        ready = false;
        break;
      }
      auto const blocks = [&](std::vector<TsoQueuedMessage> const& messages) {
        return std::any_of(
            messages.begin(),
            messages.end(),
            [&pending](TsoQueuedMessage const& message) {
              return message.timestamp && *message.timestamp <= *pending->timestamp;
            });
      };
      if (blocks(federate.queuedTsoMessages) || blocks(federate.inTransitTsoMessages)) {
        ready = false;
        break;
      }
    } catch (rti1516_2025::Exception const&) {
      ready = false;
      break;
    }
  }
  if (ready) {
    auto timestamp = pending->timestamp;
    static_cast<void>(startFederationSave(
        federation->second,
        std::move(pending->label),
        result,
        std::move(timestamp)));
  }
  return result;
}

FederationSaveAdmission
EmbeddedFederationRegistry::admitTimedFederationSaveAtTimeAdvanceBoundary(
    std::wstring const& federationName,
    std::uint64_t federateId) {
  return admitTimedFederationSaveAtGrantBoundary(federationName, federateId, nullptr);
}

FederationSaveAdmission
EmbeddedFederationRegistry::admitTimedFederationSaveAtFlushQueueGrantBoundary(
    std::wstring const& federationName,
    std::uint64_t federateId,
    std::shared_ptr<rti1516_2025::LogicalTime const> flushQueueGrantedTime) {
  return admitTimedFederationSaveAtGrantBoundary(
      federationName,
      federateId,
      std::move(flushQueueGrantedTime));
}

FederationSaveAdmission
EmbeddedFederationRegistry::admitTimedFederationSaveAtGrantBoundary(
    std::wstring const& federationName,
    std::uint64_t federateId,
    std::shared_ptr<rti1516_2025::LogicalTime const> flushQueueGrantedTime) {
  FederationSaveAdmission result;

  auto const advanceCanReachTimestamp = [](
                                           FederateTimeSnapshot const& time,
                                           rti1516_2025::LogicalTime const& timestamp,
                                           std::shared_ptr<rti1516_2025::LogicalTime const> const&
                                               flushQueueGrantTime) {
    if (!time.timeAdvancePending || !time.requestedTime ||
        time.implementationName != timestamp.implementationName() ||
        time.requestedTime->implementationName() != timestamp.implementationName()) {
      return false;
    }
    try {
      switch (time.advanceMode) {
        case FederateTimeAdvanceMode::time_advance_request:
        case FederateTimeAdvanceMode::next_message_request:
          return *time.requestedTime >= timestamp;
        case FederateTimeAdvanceMode::time_advance_request_available:
        case FederateTimeAdvanceMode::next_message_request_available:
          return *time.requestedTime > timestamp;
        case FederateTimeAdvanceMode::flush_queue_request:
          return flushQueueGrantTime &&
              flushQueueGrantTime->implementationName() == timestamp.implementationName() &&
              *flushQueueGrantTime > timestamp;
        case FederateTimeAdvanceMode::none:
          return false;
      }
    } catch (rti1516_2025::Exception const&) {
      return false;
    }
    return false;
  };
  auto const hasUndeliveredTimestampThrough = [](
                                                   FederationTimeFederateSnapshot const& federate,
                                                   rti1516_2025::LogicalTime const& timestamp) {
    auto const blocks = [&timestamp](std::vector<TsoQueuedMessage> const& messages) {
      try {
        return std::any_of(
            messages.begin(),
            messages.end(),
            [&timestamp](TsoQueuedMessage const& message) {
              return message.timestamp && *message.timestamp <= timestamp;
            });
      } catch (rti1516_2025::Exception const&) {
        return true;
      }
    };
    return blocks(federate.queuedTsoMessages) || blocks(federate.inTransitTsoMessages);
  };

  std::scoped_lock lock(mutex_);
  auto federation = federations_.find(federationName);
  if (federation == federations_.end()) {
    result.status = FederationSaveControlStatus::federation_does_not_exist;
    return result;
  }
  if (!federation->second.members.contains(federateId)) {
    result.status = FederationSaveControlStatus::federate_not_member;
    return result;
  }
  if (federation->second.restoreOperation.has_value()) {
    result.status = FederationSaveControlStatus::restore_in_progress;
    return result;
  }

  auto const execution = makeTimeSnapshot(federation->second);
  if (!execution) {
    result.status = FederationSaveControlStatus::inconsistent_temporal_state;
    return result;
  }
  auto const current = std::find_if(
      execution->federates.begin(),
      execution->federates.end(),
      [federateId](FederationTimeFederateSnapshot const& candidate) {
        return candidate.membership.id == federateId;
      });
  if (current == execution->federates.end()) {
    result.status = FederationSaveControlStatus::federate_not_member;
    return result;
  }
  if (flushQueueGrantedTime &&
      (current->time.advanceMode != FederateTimeAdvanceMode::flush_queue_request ||
       flushQueueGrantedTime->implementationName() != current->time.implementationName)) {
    result.status = FederationSaveControlStatus::inconsistent_temporal_state;
    return result;
  }

  auto const flushQueueGrantFor = [&execution,
                                   federateId,
                                   &flushQueueGrantedTime](
                                      FederationTimeFederateSnapshot const& candidate) {
    std::shared_ptr<rti1516_2025::LogicalTime const> grant;
    if (candidate.time.advanceMode != FederateTimeAdvanceMode::flush_queue_request) {
      return grant;
    }
    if (candidate.membership.id == federateId) {
      return flushQueueGrantedTime;
    }
    FederationFlushQueueGrantCalculator flushQueueGrantCalculator;
    auto calculation = flushQueueGrantCalculator.calculate(*execution, candidate.membership.id);
    if (!calculation.calculated()) {
      return grant;
    }
    return std::shared_ptr<rti1516_2025::LogicalTime const>{
        std::move(calculation.grantedTime)};
  };

  // A timestamped save is admitted by the qualifying constrained federate at
  // its own ordinary time-advance boundary. A non-constrained member may
  // request the save and receive its eventual notification, but it must not
  // open the operation while another member is still responsible for the
  // required pre-grant Initiate Federate Save callback.
  if (!current->time.timeConstrained) {
    return result;
  }

  if (!federation->second.saveOperation.has_value()) {
    if (!federation->second.pendingTimedSave.has_value()) {
      return result;
    }
    auto const& pending = *federation->second.pendingTimedSave;
    auto const currentFlushQueueGrant = flushQueueGrantFor(*current);
    if (!pending.timestamp ||
        !advanceCanReachTimestamp(
            current->time,
            *pending.timestamp,
            currentFlushQueueGrant) ||
        hasUndeliveredTimestampThrough(*current, *pending.timestamp)) {
      return result;
    }

    // Beginning a save after one constrained callback would make an idle
    // peer unable to request its own advance. Require every constrained
    // member to have a qualifying, already scheduled grant before this first
    // direct admission. FQR candidates use their shared callback-time grant
    // calculation, so a strict FQR boundary cannot open a save unless every
    // constrained member is already known to cross it. Its outstanding TSO
    // payloads are drained at that member's own callback boundary below.
    for (auto const& candidate : execution->federates) {
      if (!candidate.time.timeConstrained) {
        continue;
      }
      auto const candidateFlushQueueGrant = flushQueueGrantFor(candidate);
      if (!advanceCanReachTimestamp(
              candidate.time,
              *pending.timestamp,
              candidateFlushQueueGrant)) {
        return result;
      }
      if (candidate.membership.id == federateId) {
        continue;
      }
      auto const scheduled = federation->second.pendingTimeAdvanceGrants.find(
          candidate.membership.id);
      if (scheduled == federation->second.pendingTimeAdvanceGrants.end() ||
          !scheduled->second.dispatchQueued) {
        return result;
      }
    }

    for (auto const& [memberId, member] : federation->second.members) {
      static_cast<void>(member);
      auto const route = federation->second.interactionCallbackRoutes.find(memberId);
      if (route == federation->second.interactionCallbackRoutes.end() || !route->second) {
        result.status = FederationSaveControlStatus::callback_route_missing;
        return result;
      }
    }

    Federation::SaveOperation operation;
    operation.label = pending.label;
    operation.directTimeConstrainedInitiation = true;
    operation.scheduledSaveTime = pending.timestamp;
    for (auto const& candidate : execution->federates) {
      operation.statuses.emplace(candidate.membership.id, rti1516_2025::NO_SAVE_IN_PROGRESS);
      if (candidate.time.timeConstrained) {
        operation.pendingTimeConstrainedInitiations.insert(candidate.membership.id);
      }
    }
    federation->second.saveOperation = std::move(operation);
    federation->second.pendingTimedSave.reset();
  }

  auto& operation = *federation->second.saveOperation;
  auto const currentFlushQueueGrant = flushQueueGrantFor(*current);
  if (!operation.directTimeConstrainedInitiation || !operation.scheduledSaveTime ||
      !operation.pendingTimeConstrainedInitiations.contains(federateId) ||
      !advanceCanReachTimestamp(
          current->time,
          *operation.scheduledSaveTime,
          currentFlushQueueGrant) ||
      hasUndeliveredTimestampThrough(*current, *operation.scheduledSaveTime)) {
    return result;
  }

  auto currentStatus = operation.statuses.find(federateId);
  if (currentStatus == operation.statuses.end() ||
      currentStatus->second != rti1516_2025::NO_SAVE_IN_PROGRESS) {
    return result;
  }
  currentStatus->second = rti1516_2025::FEDERATE_INSTRUCTED_TO_SAVE;
  operation.pendingTimeConstrainedInitiations.erase(federateId);
  result.currentFederateLabel = operation.label;
  result.currentFederateTimestamp = operation.scheduledSaveTime;

  if (!operation.pendingTimeConstrainedInitiations.empty()) {
    return result;
  }

  operation.directTimeConstrainedInitiation = false;
  for (auto& [memberId, status] : operation.statuses) {
    if (status != rti1516_2025::NO_SAVE_IN_PROGRESS) {
      continue;
    }
    auto const route = federation->second.interactionCallbackRoutes.find(memberId);
    if (route == federation->second.interactionCallbackRoutes.end() || !route->second) {
      result.status = FederationSaveControlStatus::callback_route_missing;
      return result;
    }
    auto const reportRoute = federation->second.serviceReportRoutes.find(memberId);
    status = rti1516_2025::FEDERATE_INSTRUCTED_TO_SAVE;
    FederationSaveNotification notification;
    notification.kind = FederationSaveNotificationKind::initiate;
    notification.receivingFederateId = memberId;
    notification.label = operation.label;
    notification.timestamp = operation.scheduledSaveTime;
    notification.callbackRoute = route->second;
    notification.serviceReportRoute =
        reportRoute == federation->second.serviceReportRoutes.end()
        ? FederateServiceReportRoute{}
        : reportRoute->second;
    result.notifications.push_back(std::move(notification));
  }
  return result;
}

FederationSaveControlResult EmbeddedFederationRegistry::reevaluateTimedFederationSave(
    std::wstring const& federationName) {
  auto instrumentationScope = beginInstrumentation("reevaluateTimedFederationSave");
  FederationSaveControlResult result;

  std::scoped_lock lock(mutex_);
  auto federation = federations_.find(federationName);
  if (federation == federations_.end()) {
    result.status = FederationSaveControlStatus::federation_does_not_exist;
    return result;
  }
  if (federation->second.saveOperation.has_value()) {
    result.status = FederationSaveControlStatus::save_in_progress;
    return result;
  }
  if (federation->second.restoreOperation.has_value()) {
    result.status = FederationSaveControlStatus::restore_in_progress;
    return result;
  }
  if (!federation->second.pendingTimedSave.has_value()) {
    return result;
  }

  auto const execution = makeTimeSnapshot(federation->second);
  if (!execution) {
    result.status = FederationSaveControlStatus::invalid_timed_save;
    return result;
  }
  auto const hasTimeConstrainedMember = std::any_of(
      execution->federates.begin(),
      execution->federates.end(),
      [](FederationTimeFederateSnapshot const& federate) {
        return federate.time.timeConstrained;
      });
  if (hasTimeConstrainedMember) {
    // A constrained recipient must be admitted while still Time Advancing.
    // Do not revive the former post-grant callback path here: the qualified
    // ordinary-grant dispatcher owns that pre-grant admission instead.
    return result;
  }
  auto const& pending = *federation->second.pendingTimedSave;
  bool ready = true;
  try {
    for (auto const& federate : execution->federates) {
      if (!federate.time.timeConstrained) {
        continue;
      }
      if (!federate.time.currentTime ||
          *federate.time.currentTime < *pending.timestamp) {
        ready = false;
        break;
      }
      auto const blocks = [&](std::vector<TsoQueuedMessage> const& messages) {
        return std::any_of(
            messages.begin(),
            messages.end(),
            [&pending](TsoQueuedMessage const& message) {
              return message.timestamp && *message.timestamp <= *pending.timestamp;
            });
      };
      if (blocks(federate.queuedTsoMessages) || blocks(federate.inTransitTsoMessages)) {
        ready = false;
        break;
      }
    }
  } catch (rti1516_2025::Exception const&) {
    result.status = FederationSaveControlStatus::invalid_timed_save;
    return result;
  }
  if (ready) {
    auto label = pending.label;
    auto timestamp = pending.timestamp;
    static_cast<void>(startFederationSave(
        federation->second,
        std::move(label),
        result,
        std::move(timestamp)));
  }
  return result;
}

FederationSaveControlResult EmbeddedFederationRegistry::federateSaveBegun(
    std::wstring const& federationName,
    std::uint64_t federateId) {
  auto instrumentationScope = beginInstrumentation("federateSaveBegun");
  FederationSaveControlResult result;

  std::scoped_lock lock(mutex_);
  auto federation = federations_.find(federationName);
  if (federation == federations_.end()) {
    result.status = FederationSaveControlStatus::federation_does_not_exist;
    return result;
  }
  if (!federation->second.members.contains(federateId)) {
    result.status = FederationSaveControlStatus::federate_not_member;
    return result;
  }
  if (federation->second.restoreOperation.has_value()) {
    result.status = FederationSaveControlStatus::restore_in_progress;
    return result;
  }
  if (!federation->second.saveOperation.has_value()) {
    result.status = FederationSaveControlStatus::save_not_initiated;
    return result;
  }
  auto status = federation->second.saveOperation->statuses.find(federateId);
  if (status == federation->second.saveOperation->statuses.end() ||
      status->second != rti1516_2025::FEDERATE_INSTRUCTED_TO_SAVE) {
    result.status = FederationSaveControlStatus::save_not_initiated;
    return result;
  }
  status->second = rti1516_2025::FEDERATE_SAVING;
  return result;
}

FederationSaveControlResult EmbeddedFederationRegistry::federateSaveComplete(
    std::wstring const& federationName,
    std::uint64_t federateId) {
  auto instrumentationScope = beginInstrumentation("federateSaveComplete");
  FederationSaveControlResult result;

  std::scoped_lock lock(mutex_);
  auto federation = federations_.find(federationName);
  if (federation == federations_.end()) {
    result.status = FederationSaveControlStatus::federation_does_not_exist;
    return result;
  }
  if (!federation->second.members.contains(federateId)) {
    result.status = FederationSaveControlStatus::federate_not_member;
    return result;
  }
  if (federation->second.restoreOperation.has_value()) {
    result.status = FederationSaveControlStatus::restore_in_progress;
    return result;
  }
  if (!federation->second.saveOperation.has_value()) {
    result.status = FederationSaveControlStatus::federate_has_not_begun_save;
    return result;
  }
  auto status = federation->second.saveOperation->statuses.find(federateId);
  if (status == federation->second.saveOperation->statuses.end() ||
      status->second != rti1516_2025::FEDERATE_SAVING) {
    result.status = FederationSaveControlStatus::federate_has_not_begun_save;
    return result;
  }
  status->second = rti1516_2025::FEDERATE_WAITING_FOR_FEDERATION_TO_SAVE;

  bool allComplete = true;
  for (auto const& [participantId, participantStatus] :
       federation->second.saveOperation->statuses) {
    static_cast<void>(participantId);
    if (participantStatus != rti1516_2025::FEDERATE_WAITING_FOR_FEDERATION_TO_SAVE) {
      allComplete = false;
      break;
    }
  }
  if (allComplete) {
    auto const snapshotLabel = federation->second.saveOperation->label;
    bool snapshotStored = false;
    try {
      Federation snapshot = federation->second;
      snapshot.saveOperation.reset();
      snapshot.pendingImmediateSave.reset();
      snapshot.pendingTimedSave.reset();
      snapshot.restoreOperation.reset();
      snapshot.pendingTimeAdvanceGrants.clear();
      snapshot.timeAdvanceGrantDispatchFactories.clear();
      saveSnapshots_[federationName][snapshotLabel] = std::move(snapshot);
      snapshotStored = true;
    } catch (...) {
      // The control operation remains well-formed even if this process-local
      // snapshot cannot be materialized. Report the standard save failure and
      // never claim that a restorable image exists.
      auto saved = saveSnapshots_.find(federationName);
      if (saved != saveSnapshots_.end()) {
        saved->second.erase(snapshotLabel);
      }
    }
    appendSaveCompletionNotifications(
        federation->second,
        snapshotStored,
        rti1516_2025::RTI_UNABLE_TO_SAVE,
        result.notifications);
    federation->second.saveOperation.reset();
  }
  return result;
}

FederationSaveControlResult EmbeddedFederationRegistry::federateSaveNotComplete(
    std::wstring const& federationName,
    std::uint64_t federateId) {
  auto instrumentationScope = beginInstrumentation("federateSaveNotComplete");
  FederationSaveControlResult result;

  std::scoped_lock lock(mutex_);
  auto federation = federations_.find(federationName);
  if (federation == federations_.end()) {
    result.status = FederationSaveControlStatus::federation_does_not_exist;
    return result;
  }
  if (!federation->second.members.contains(federateId)) {
    result.status = FederationSaveControlStatus::federate_not_member;
    return result;
  }
  if (federation->second.restoreOperation.has_value()) {
    result.status = FederationSaveControlStatus::restore_in_progress;
    return result;
  }
  if (!federation->second.saveOperation.has_value()) {
    result.status = FederationSaveControlStatus::federate_has_not_begun_save;
    return result;
  }
  auto status = federation->second.saveOperation->statuses.find(federateId);
  if (status == federation->second.saveOperation->statuses.end() ||
      status->second != rti1516_2025::FEDERATE_SAVING) {
    result.status = FederationSaveControlStatus::federate_has_not_begun_save;
    return result;
  }

  appendSaveCompletionNotifications(
      federation->second,
      false,
      rti1516_2025::FEDERATE_REPORTED_FAILURE_DURING_SAVE,
      result.notifications);
  federation->second.saveOperation.reset();
  return result;
}

FederationSaveControlResult EmbeddedFederationRegistry::abortFederationSave(
    std::wstring const& federationName,
    std::uint64_t federateId) {
  auto instrumentationScope = beginInstrumentation("abortFederationSave");
  FederationSaveControlResult result;

  std::scoped_lock lock(mutex_);
  auto federation = federations_.find(federationName);
  if (federation == federations_.end()) {
    result.status = FederationSaveControlStatus::federation_does_not_exist;
    return result;
  }
  if (!federation->second.members.contains(federateId)) {
    result.status = FederationSaveControlStatus::federate_not_member;
    return result;
  }
  if (!federation->second.saveOperation.has_value()) {
    result.status = FederationSaveControlStatus::save_not_in_progress;
    return result;
  }
  if (federation->second.restoreOperation.has_value()) {
    result.status = FederationSaveControlStatus::restore_in_progress;
    return result;
  }

  appendSaveCompletionNotifications(
      federation->second,
      false,
      rti1516_2025::SAVE_ABORTED,
      result.notifications);
  federation->second.saveOperation.reset();
  return result;
}

FederationSaveControlResult EmbeddedFederationRegistry::queryFederationSaveStatus(
    std::wstring const& federationName,
    std::uint64_t federateId) {
  auto instrumentationScope = beginInstrumentation("queryFederationSaveStatus");
  FederationSaveControlResult result;

  std::scoped_lock lock(mutex_);
  auto federation = federations_.find(federationName);
  if (federation == federations_.end()) {
    result.status = FederationSaveControlStatus::federation_does_not_exist;
    return result;
  }
  if (!federation->second.members.contains(federateId)) {
    result.status = FederationSaveControlStatus::federate_not_member;
    return result;
  }
  if (federation->second.restoreOperation.has_value()) {
    result.status = FederationSaveControlStatus::restore_in_progress;
    return result;
  }
  auto const route = federation->second.interactionCallbackRoutes.find(federateId);
  if (route == federation->second.interactionCallbackRoutes.end() || !route->second) {
    result.status = FederationSaveControlStatus::callback_route_missing;
    return result;
  }

  FederationSaveNotification notification;
  notification.kind = FederationSaveNotificationKind::status;
  notification.receivingFederateId = federateId;
  notification.callbackRoute = route->second;
  auto const reportRoute = federation->second.serviceReportRoutes.find(federateId);
  notification.serviceReportRoute =
      reportRoute == federation->second.serviceReportRoutes.end()
      ? FederateServiceReportRoute{}
      : reportRoute->second;
  if (federation->second.saveOperation.has_value()) {
    for (auto const& [participantId, status] : federation->second.saveOperation->statuses) {
      notification.statuses.emplace_back(participantId, status);
    }
  } else {
    for (auto const& [memberId, member] : federation->second.members) {
      static_cast<void>(member);
      notification.statuses.emplace_back(memberId, rti1516_2025::NO_SAVE_IN_PROGRESS);
    }
  }
  result.notifications.push_back(std::move(notification));
  return result;
}

FederationServiceOperationStatus EmbeddedFederationRegistry::serviceOperationStatus(
    std::wstring const& federationName,
    std::uint64_t federateId) const {
  auto instrumentationScope = beginInstrumentation("serviceOperationStatus");
  std::scoped_lock lock(mutex_);
  auto federation = federations_.find(federationName);
  if (federation == federations_.end()) {
    return FederationServiceOperationStatus::federation_does_not_exist;
  }
  if (!federation->second.members.contains(federateId)) {
    return FederationServiceOperationStatus::federate_not_member;
  }
  if (federation->second.saveOperation.has_value()) {
    return FederationServiceOperationStatus::save_in_progress;
  }
  if (federation->second.restoreOperation.has_value()) {
    return FederationServiceOperationStatus::restore_in_progress;
  }
  return FederationServiceOperationStatus::available;
}

FederationRestoreControlResult EmbeddedFederationRegistry::requestFederationRestore(
    std::wstring const& federationName,
    std::uint64_t requestingFederateId,
    std::wstring label) {
  auto instrumentationScope = beginInstrumentation("requestFederationRestore");
  FederationRestoreControlResult result;

  std::scoped_lock lock(mutex_);
  auto federation = federations_.find(federationName);
  if (federation == federations_.end()) {
    result.status = FederationRestoreControlStatus::federation_does_not_exist;
    return result;
  }
  auto const requester = federation->second.members.find(requestingFederateId);
  if (requester == federation->second.members.end()) {
    result.status = FederationRestoreControlStatus::federate_not_member;
    return result;
  }
  if (federation->second.saveOperation.has_value()) {
    result.status = FederationRestoreControlStatus::save_in_progress;
    return result;
  }
  if (federation->second.restoreOperation.has_value()) {
    result.status = FederationRestoreControlStatus::restore_in_progress;
    return result;
  }

  auto const savedFederations = saveSnapshots_.find(federationName);

  auto const requesterRoute = federation->second.interactionCallbackRoutes.find(
      requestingFederateId);
  if (requesterRoute == federation->second.interactionCallbackRoutes.end() ||
      !requesterRoute->second) {
    result.status = FederationRestoreControlStatus::callback_route_missing;
    return result;
  }
  auto const requesterReportRoute = federation->second.serviceReportRoutes.find(
      requestingFederateId);
  auto const selectedRequesterReportRoute =
      requesterReportRoute == federation->second.serviceReportRoutes.end()
      ? FederateServiceReportRoute{}
      : requesterReportRoute->second;

  if (savedFederations == saveSnapshots_.end()) {
    result.status = FederationRestoreControlStatus::snapshot_not_found;
    FederationRestoreNotification notification;
    notification.kind = FederationRestoreNotificationKind::request_failed;
    notification.receivingFederateId = requestingFederateId;
    notification.label = label;
    notification.callbackRoute = requesterRoute->second;
    notification.serviceReportRoute = selectedRequesterReportRoute;
    result.notifications.push_back(std::move(notification));
    return result;
  }
  auto const saved = savedFederations->second.find(label);
  if (saved == savedFederations->second.end()) {
    result.status = FederationRestoreControlStatus::snapshot_not_found;
    FederationRestoreNotification notification;
    notification.kind = FederationRestoreNotificationKind::request_failed;
    notification.receivingFederateId = requestingFederateId;
    notification.label = label;
    notification.callbackRoute = requesterRoute->second;
    notification.serviceReportRoute = selectedRequesterReportRoute;
    result.notifications.push_back(std::move(notification));
    return result;
  }

  Federation const& snapshot = saved->second;
  bool membershipMatches = snapshot.members.size() == federation->second.members.size();
  if (membershipMatches) {
    for (auto const& [federateId, member] : federation->second.members) {
      auto const savedMember = snapshot.members.find(federateId);
      if (savedMember == snapshot.members.end() ||
          savedMember->second.name != member.name ||
          savedMember->second.type != member.type) {
        membershipMatches = false;
        break;
      }
    }
  }
  if (!membershipMatches) {
    result.status = FederationRestoreControlStatus::membership_mismatch;
    FederationRestoreNotification notification;
    notification.kind = FederationRestoreNotificationKind::request_failed;
    notification.receivingFederateId = requestingFederateId;
    notification.label = label;
    notification.callbackRoute = requesterRoute->second;
    notification.serviceReportRoute = selectedRequesterReportRoute;
    result.notifications.push_back(std::move(notification));
    return result;
  }

  for (auto const& [federateId, member] : federation->second.members) {
    static_cast<void>(member);
    auto const route = federation->second.interactionCallbackRoutes.find(federateId);
    if (route == federation->second.interactionCallbackRoutes.end() || !route->second) {
      result.status = FederationRestoreControlStatus::callback_route_missing;
      return result;
    }
  }

  Federation::RestoreOperation operation;
  operation.label = label;
  for (auto const& [federateId, member] : federation->second.members) {
    static_cast<void>(member);
    operation.statuses.emplace(federateId, rti1516_2025::FEDERATE_RESTORING);
  }
  federation->second.restoreOperation = std::move(operation);

  FederationRestoreNotification succeeded;
  succeeded.kind = FederationRestoreNotificationKind::request_succeeded;
  succeeded.receivingFederateId = requestingFederateId;
  succeeded.label = label;
  succeeded.callbackRoute = requesterRoute->second;
  succeeded.serviceReportRoute = selectedRequesterReportRoute;
  result.notifications.push_back(std::move(succeeded));

  // The embedded backend begins the restore immediately after accepting the
  // request. The official callbacks still preserve the standard ordering:
  // request success, federation-begun, then one initiate callback per member.
  for (auto const& [federateId, member] : federation->second.members) {
    auto const route = federation->second.interactionCallbackRoutes.find(federateId);
    auto const reportRoute = federation->second.serviceReportRoutes.find(federateId);
    FederationRestoreNotification begun;
    begun.kind = FederationRestoreNotificationKind::begin;
    begun.receivingFederateId = federateId;
    begun.label = label;
    begun.callbackRoute = route->second;
    begun.serviceReportRoute =
        reportRoute == federation->second.serviceReportRoutes.end()
        ? FederateServiceReportRoute{}
        : reportRoute->second;
    result.notifications.push_back(std::move(begun));

    FederationRestoreNotification initiate;
    initiate.kind = FederationRestoreNotificationKind::initiate;
    initiate.receivingFederateId = federateId;
    initiate.label = label;
    initiate.federateName = member.name;
    initiate.preRestoreFederateId = federateId;
    initiate.postRestoreFederateId = federateId;
    initiate.callbackRoute = route->second;
    initiate.serviceReportRoute =
        reportRoute == federation->second.serviceReportRoutes.end()
        ? FederateServiceReportRoute{}
        : reportRoute->second;
    result.notifications.push_back(std::move(initiate));
  }
  return result;
}

FederationRestoreControlResult EmbeddedFederationRegistry::federateRestoreComplete(
    std::wstring const& federationName,
    std::uint64_t federateId) {
  auto instrumentationScope = beginInstrumentation("federateRestoreComplete");
  FederationRestoreControlResult result;

  std::scoped_lock lock(mutex_);
  auto federation = federations_.find(federationName);
  if (federation == federations_.end()) {
    result.status = FederationRestoreControlStatus::federation_does_not_exist;
    return result;
  }
  if (!federation->second.members.contains(federateId)) {
    result.status = FederationRestoreControlStatus::federate_not_member;
    return result;
  }
  if (federation->second.saveOperation.has_value()) {
    result.status = FederationRestoreControlStatus::save_in_progress;
    return result;
  }
  if (!federation->second.restoreOperation.has_value()) {
    result.status = FederationRestoreControlStatus::restore_not_requested;
    return result;
  }
  auto status = federation->second.restoreOperation->statuses.find(federateId);
  if (status == federation->second.restoreOperation->statuses.end() ||
      status->second != rti1516_2025::FEDERATE_RESTORING) {
    result.status = FederationRestoreControlStatus::restore_not_requested;
    return result;
  }
  status->second = rti1516_2025::FEDERATE_WAITING_FOR_FEDERATION_TO_RESTORE;

  bool allComplete = true;
  for (auto const& [participantId, participantStatus] :
       federation->second.restoreOperation->statuses) {
    static_cast<void>(participantId);
    if (participantStatus != rti1516_2025::FEDERATE_WAITING_FOR_FEDERATION_TO_RESTORE) {
      allComplete = false;
      break;
    }
  }
  if (!allComplete) {
    return result;
  }

  auto const label = federation->second.restoreOperation->label;
  auto const savedFederations = saveSnapshots_.find(federationName);
  if (savedFederations == saveSnapshots_.end()) {
    appendRestoreCompletionNotifications(
        federation->second,
        false,
        rti1516_2025::RTI_UNABLE_TO_RESTORE,
        result.notifications);
    federation->second.restoreOperation.reset();
    return result;
  }
  auto const saved = savedFederations->second.find(label);
  if (saved == savedFederations->second.end()) {
    appendRestoreCompletionNotifications(
        federation->second,
        false,
        rti1516_2025::RTI_UNABLE_TO_RESTORE,
        result.notifications);
    federation->second.restoreOperation.reset();
    return result;
  }

  try {
    restoreFederationFromSnapshot(federation->second, saved->second);
    // restoreFederationFromSnapshot clears the operation after the callback
    // work has been built, so build successful completion notifications first
    // and restore the state only after all preconditions have passed.
  } catch (...) {
    appendRestoreCompletionNotifications(
        federation->second,
        false,
        rti1516_2025::RTI_UNABLE_TO_RESTORE,
        result.notifications);
    federation->second.restoreOperation.reset();
    return result;
  }

  // The state transition above clears restoreOperation, so emit the callback
  // records from the saved member set using the live routes now in the target.
  for (auto const& [federateId, member] : federation->second.members) {
    auto const route = federation->second.interactionCallbackRoutes.find(federateId);
    if (route == federation->second.interactionCallbackRoutes.end() || !route->second) {
      continue;
    }
    FederationRestoreNotification notification;
    notification.kind = FederationRestoreNotificationKind::completed;
    notification.receivingFederateId = federateId;
    notification.label = label;
    notification.federateName = member.name;
    notification.preRestoreFederateId = federateId;
    notification.postRestoreFederateId = federateId;
    notification.successful = true;
    notification.failureReason = rti1516_2025::RTI_UNABLE_TO_RESTORE;
    notification.callbackRoute = route->second;
    result.notifications.push_back(std::move(notification));
  }
  // Queue the reconstructed grant only after every successful restore
  // notification has been prepared. The adapter submits those callbacks first,
  // preserving a clear restored-before-grant boundary in both callback models.
  result.timeAdvanceGrantDispatches =
      scheduleEligibleTimeAdvanceGrants(federation->second);
  return result;
}

FederationRestoreControlResult EmbeddedFederationRegistry::federateRestoreNotComplete(
    std::wstring const& federationName,
    std::uint64_t federateId) {
  auto instrumentationScope = beginInstrumentation("federateRestoreNotComplete");
  FederationRestoreControlResult result;

  std::scoped_lock lock(mutex_);
  auto federation = federations_.find(federationName);
  if (federation == federations_.end()) {
    result.status = FederationRestoreControlStatus::federation_does_not_exist;
    return result;
  }
  if (!federation->second.members.contains(federateId)) {
    result.status = FederationRestoreControlStatus::federate_not_member;
    return result;
  }
  if (federation->second.saveOperation.has_value()) {
    result.status = FederationRestoreControlStatus::save_in_progress;
    return result;
  }
  if (!federation->second.restoreOperation.has_value()) {
    result.status = FederationRestoreControlStatus::restore_not_requested;
    return result;
  }
  auto const status = federation->second.restoreOperation->statuses.find(federateId);
  if (status == federation->second.restoreOperation->statuses.end() ||
      status->second != rti1516_2025::FEDERATE_RESTORING) {
    result.status = FederationRestoreControlStatus::restore_not_requested;
    return result;
  }
  appendRestoreCompletionNotifications(
      federation->second,
      false,
      rti1516_2025::FEDERATE_REPORTED_FAILURE_DURING_RESTORE,
      result.notifications);
  federation->second.restoreOperation.reset();
  return result;
}

FederationRestoreControlResult EmbeddedFederationRegistry::abortFederationRestore(
    std::wstring const& federationName,
    std::uint64_t federateId) {
  auto instrumentationScope = beginInstrumentation("abortFederationRestore");
  FederationRestoreControlResult result;

  std::scoped_lock lock(mutex_);
  auto federation = federations_.find(federationName);
  if (federation == federations_.end()) {
    result.status = FederationRestoreControlStatus::federation_does_not_exist;
    return result;
  }
  if (!federation->second.members.contains(federateId)) {
    result.status = FederationRestoreControlStatus::federate_not_member;
    return result;
  }
  if (federation->second.saveOperation.has_value()) {
    result.status = FederationRestoreControlStatus::save_in_progress;
    return result;
  }
  if (!federation->second.restoreOperation.has_value()) {
    result.status = FederationRestoreControlStatus::restore_not_in_progress;
    return result;
  }
  appendRestoreCompletionNotifications(
      federation->second,
      false,
      rti1516_2025::RESTORE_ABORTED,
      result.notifications);
  federation->second.restoreOperation.reset();
  return result;
}

FederationRestoreControlResult EmbeddedFederationRegistry::queryFederationRestoreStatus(
    std::wstring const& federationName,
    std::uint64_t federateId) {
  auto instrumentationScope = beginInstrumentation("queryFederationRestoreStatus");
  FederationRestoreControlResult result;

  std::scoped_lock lock(mutex_);
  auto federation = federations_.find(federationName);
  if (federation == federations_.end()) {
    result.status = FederationRestoreControlStatus::federation_does_not_exist;
    return result;
  }
  if (!federation->second.members.contains(federateId)) {
    result.status = FederationRestoreControlStatus::federate_not_member;
    return result;
  }
  if (federation->second.saveOperation.has_value()) {
    result.status = FederationRestoreControlStatus::save_in_progress;
    return result;
  }
  auto const route = federation->second.interactionCallbackRoutes.find(federateId);
  if (route == federation->second.interactionCallbackRoutes.end() || !route->second) {
    result.status = FederationRestoreControlStatus::callback_route_missing;
    return result;
  }

  FederationRestoreNotification notification;
  notification.kind = FederationRestoreNotificationKind::status;
  notification.receivingFederateId = federateId;
  notification.callbackRoute = route->second;
  auto const reportRoute = federation->second.serviceReportRoutes.find(federateId);
  notification.serviceReportRoute =
      reportRoute == federation->second.serviceReportRoutes.end()
      ? FederateServiceReportRoute{}
      : reportRoute->second;
  if (federation->second.restoreOperation.has_value()) {
    for (auto const& [participantId, status] : federation->second.restoreOperation->statuses) {
      notification.statuses.push_back({participantId, participantId, status});
    }
  } else {
    for (auto const& [memberId, member] : federation->second.members) {
      static_cast<void>(member);
      notification.statuses.push_back({
          memberId,
          0,
          rti1516_2025::NO_RESTORE_IN_PROGRESS,
      });
    }
  }
  result.notifications.push_back(std::move(notification));
  return result;
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
      result.push_back({
          object->second.objectInstanceHandle,
          object->second.periodicAttributeHandles,
      });
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
      providingFederateId == receivingFederateId ||
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
    auto const owner = instance->second.attributeOwnersByHandle.find(attributeHandle);
    if (owner == instance->second.attributeOwnersByHandle.end() ||
        owner->second != providingFederateId) {
      continue;
    }
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

std::optional<std::string>
EmbeddedFederationRegistry::attributeRelevanceAdvisoryUpdateRateDesignatorFor(
    std::wstring const& federationName,
    std::uint64_t receivingFederateId,
    std::uint64_t objectInstanceHandle,
    std::uint64_t attributeHandle) const {
  std::scoped_lock lock(mutex_);
  auto const federation = federations_.find(federationName);
  if (federation == federations_.end() ||
      !federation->second.members.contains(receivingFederateId)) {
    return std::nullopt;
  }
  auto const objectInstance = federation->second.objectInstances.find(objectInstanceHandle);
  if (objectInstance == federation->second.objectInstances.end() ||
      objectInstance->second.deleteAccepted) {
    return std::nullopt;
  }
  return subscribedUpdateRateDesignatorForAttribute(
      federation->second,
      objectInstance->second,
      receivingFederateId,
      attributeHandle);
}

FederationTimeGrantDispatchResult EmbeddedFederationRegistry::requestTimeAdvanceGrant(
    std::wstring const& federationName,
    std::uint64_t federateId,
    std::uint64_t generation) {
  auto instrumentationScope = beginInstrumentation("requestTimeAdvanceGrant");
  if (federateId == 0 || generation == 0) {
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

  auto const factory =
      federation->second.timeAdvanceGrantDispatchFactories.find(federateId);
  if (factory == federation->second.timeAdvanceGrantDispatchFactories.end() ||
      !factory->second ||
      federation->second.nextTimeAdvanceGrantDispatchIdentity == 0 ||
      federation->second.nextTimeAdvanceGrantDispatchIdentity ==
          std::numeric_limits<std::uint64_t>::max()) {
    return {FederationTimeGrantStatus::inconsistent_temporal_state, {}};
  }

  auto const dispatchIdentity = federation->second.nextTimeAdvanceGrantDispatchIdentity++;
  FederationTimeGrantDispatch dispatch;
  try {
    dispatch = factory->second(federateId, generation, dispatchIdentity);
  } catch (...) {
    return {FederationTimeGrantStatus::inconsistent_temporal_state, {}};
  }
  if (!dispatch) {
    return {FederationTimeGrantStatus::inconsistent_temporal_state, {}};
  }

  auto [pending, inserted] = federation->second.pendingTimeAdvanceGrants.emplace(
      federateId,
      Federation::PendingTimeAdvanceGrant{
          generation,
          dispatchIdentity,
          std::move(dispatch),
          false,
      });
  static_cast<void>(pending);
  if (!inserted) {
    return {FederationTimeGrantStatus::stale_generation, {}};
  }

  return {FederationTimeGrantStatus::applied, scheduleEligibleTimeAdvanceGrants(federation->second)};
}

FederationTimeGrantDispatchResult EmbeddedFederationRegistry::requestTimeAdvanceGrant(
    std::wstring const& federationName,
    std::uint64_t federateId,
    std::uint64_t generation,
    FederationTimeGrantDispatch dispatch) {
  auto instrumentationScope = beginInstrumentation("requestTimeAdvanceGrant");
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
      Federation::PendingTimeAdvanceGrant{generation, 0, std::move(dispatch), false});
  static_cast<void>(pending);
  if (!inserted) {
    return {FederationTimeGrantStatus::stale_generation, {}};
  }

  return {FederationTimeGrantStatus::applied, scheduleEligibleTimeAdvanceGrants(federation->second)};
}

FederationTimeGrantDispatchResult EmbeddedFederationRegistry::reevaluateTimeAdvanceGrants(
    std::wstring const& federationName) {
  auto instrumentationScope = beginInstrumentation("reevaluateTimeAdvanceGrants");
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
    std::uint64_t generation,
    std::uint64_t dispatchIdentity) {
  auto instrumentationScope = beginInstrumentation("beginTimeAdvanceGrant");
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
  if (pending->second.generation != generation ||
      pending->second.dispatchIdentity != dispatchIdentity) {
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
  bool const grantEligible = decision.mayGrant() || connectionLossTsoDeliveryMayGrant(
      federation->second,
      federateId,
      requester->time,
      bounds);
  if (!grantEligible) {
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
  auto instrumentationScope = beginInstrumentation("allocateTsoMessageId");
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
  auto instrumentationScope = beginInstrumentation("enqueueTsoMessage");
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

std::optional<std::shared_ptr<rti1516_2025::LogicalTime const>>
EmbeddedFederationRegistry::earliestTsoTimestampFor(
    std::wstring const& federationName,
    std::uint64_t recipientFederateId) const {
  if (recipientFederateId == 0) {
    return std::nullopt;
  }
  std::scoped_lock lock(mutex_);
  auto const federation = federations_.find(federationName);
  if (federation == federations_.end() ||
      !federation->second.members.contains(recipientFederateId)) {
    return std::nullopt;
  }
  return federation->second.timeCoordinator.earliestTsoTimestampFor(recipientFederateId);
}

FederationTsoInteractionEnqueueResult EmbeddedFederationRegistry::enqueueTsoInteraction(
    std::wstring const& federationName,
    TsoInteractionMessage message,
    std::vector<std::uint64_t> const& queuedRecipientFederateIds,
    std::vector<std::uint64_t> const& allTimestampedRecipientFederateIds) {
  auto instrumentationScope = beginInstrumentation("enqueueTsoInteraction");
  if (message.producingFederateId == 0 || message.sentInteractionClassHandle == 0 ||
      !message.timestamp || message.timestamp->isInitial() || message.timestamp->isFinal()) {
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
  if (message.timestamp->implementationName() !=
      federation->second.definition.logicalTimeImplementationName) {
    return {FederationTsoRegistryStatus::invalid_request,
            TsoMessageQueueStatus::logical_time_implementation_mismatch,
            0,
            0};
  }

  std::set<std::uint64_t> allRecipients;
  for (std::uint64_t const recipientFederateId : allTimestampedRecipientFederateIds) {
    if (recipientFederateId == 0 ||
        !federation->second.members.contains(recipientFederateId) ||
        !allRecipients.insert(recipientFederateId).second) {
      return {FederationTsoRegistryStatus::invalid_request,
              TsoMessageQueueStatus::invalid_recipient,
              0,
              0};
    }
  }
  std::set<std::uint64_t> queuedRecipients;
  for (std::uint64_t const recipientFederateId : queuedRecipientFederateIds) {
    if (!allRecipients.contains(recipientFederateId) ||
        !queuedRecipients.insert(recipientFederateId).second) {
      return {FederationTsoRegistryStatus::invalid_request,
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
      federation->second.tsoInteractionMessages.emplace(messageId, std::move(message));
  if (!inserted) {
    return {FederationTsoRegistryStatus::invalid_request,
            TsoMessageQueueStatus::invalid_message_id,
            0,
            0};
  }

  Federation::TsoRequestRetractionRecord retractionRecord;
  retractionRecord.producingFederateId = messagePosition->second.producingFederateId;
  retractionRecord.timestamp = messagePosition->second.timestamp;
  for (std::uint64_t const recipientFederateId : allRecipients) {
    retractionRecord.recipientStates.emplace(
        recipientFederateId,
        Federation::TsoRecipientDeliveryState::pending);
  }
  auto const [recordPosition, recordInserted] =
      federation->second.tsoRequestRetractionRecords.emplace(
          messageId, std::move(retractionRecord));
  static_cast<void>(recordPosition);
  if (!recordInserted) {
    federation->second.tsoInteractionMessages.erase(messagePosition);
    return {FederationTsoRegistryStatus::invalid_request,
            TsoMessageQueueStatus::invalid_message_id,
            0,
            0};
  }

  std::size_t enqueuedCount = 0;
  TsoMessageQueueStatus queueStatus = TsoMessageQueueStatus::applied;
  for (std::uint64_t const recipientFederateId : queuedRecipients) {
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
    federation->second.tsoInteractionMessages.erase(messageId);
    federation->second.tsoRequestRetractionRecords.erase(messageId);
    return {FederationTsoRegistryStatus::invalid_request,
            queueStatus,
            0,
            enqueuedCount};
  }

  // A timestamped Send Interaction returns its designator even when no
  // recipient is eligible. The retraction ledger then suffices for a legal
  // Retract or eventual MessageCanNoLongerBeRetracted classification; no
  // typed interaction payload need be retained in that no-fanout case.
  reclaimTsoMessagePayload(federation->second, messageId);

  return {FederationTsoRegistryStatus::applied,
          TsoMessageQueueStatus::applied,
          messageId,
          enqueuedCount};
}

FederationTsoAttributeUpdateEnqueueResult
EmbeddedFederationRegistry::enqueueTsoAttributeUpdate(
    std::wstring const& federationName,
    TsoAttributeUpdateMessage message,
    std::vector<std::uint64_t> const& queuedRecipientFederateIds,
    std::vector<std::uint64_t> const& allTimestampedRecipientFederateIds) {
  auto instrumentationScope = beginInstrumentation("enqueueTsoAttributeUpdate");
  if (message.producingFederateId == 0 ||
      message.objectInstanceHandle == 0 ||
      message.attributes.empty() ||
      !message.timestamp || message.timestamp->isInitial() || message.timestamp->isFinal()) {
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
  if (message.timestamp->implementationName() !=
      federation->second.definition.logicalTimeImplementationName) {
    return {FederationTsoRegistryStatus::invalid_request,
            TsoMessageQueueStatus::logical_time_implementation_mismatch,
            0,
            0};
  }

  std::set<std::uint64_t> allRecipients;
  for (std::uint64_t const recipientFederateId : allTimestampedRecipientFederateIds) {
    if (recipientFederateId == 0 ||
        !federation->second.members.contains(recipientFederateId) ||
        !message.passelsByRecipient.contains(recipientFederateId) ||
        !allRecipients.insert(recipientFederateId).second) {
      return {FederationTsoRegistryStatus::invalid_request,
              TsoMessageQueueStatus::invalid_recipient,
              0,
              0};
    }
  }
  std::set<std::uint64_t> queuedRecipients;
  for (std::uint64_t const recipientFederateId : queuedRecipientFederateIds) {
    if (!allRecipients.contains(recipientFederateId) ||
        !queuedRecipients.insert(recipientFederateId).second) {
      return {FederationTsoRegistryStatus::invalid_request,
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

  Federation::TsoRequestRetractionRecord retractionRecord;
  retractionRecord.producingFederateId = messagePosition->second.producingFederateId;
  retractionRecord.timestamp = messagePosition->second.timestamp;
  for (std::uint64_t const recipientFederateId : allRecipients) {
    retractionRecord.recipientStates.emplace(
        recipientFederateId,
        Federation::TsoRecipientDeliveryState::pending);
  }
  auto const [recordPosition, recordInserted] =
      federation->second.tsoRequestRetractionRecords.emplace(
          messageId, std::move(retractionRecord));
  static_cast<void>(recordPosition);
  if (!recordInserted) {
    federation->second.tsoAttributeUpdateMessages.erase(messagePosition);
    return {FederationTsoRegistryStatus::invalid_request,
            TsoMessageQueueStatus::invalid_message_id,
            0,
            0};
  }

  std::size_t enqueuedCount = 0;
  TsoMessageQueueStatus queueStatus = TsoMessageQueueStatus::applied;
  for (std::uint64_t const recipientFederateId : queuedRecipients) {
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
    federation->second.tsoAttributeUpdateMessages.erase(messageId);
    federation->second.tsoRequestRetractionRecords.erase(messageId);
    return {FederationTsoRegistryStatus::invalid_request,
            queueStatus,
            0,
            enqueuedCount};
  }

  // A timestamped Update Attribute Values invocation returns its designator
  // even when no recipient is eligible. The retraction ledger then suffices
  // for a legal Retract or eventual MessageCanNoLongerBeRetracted
  // classification; no typed attribute payload need be retained in that
  // no-fanout case.
  reclaimTsoMessagePayload(federation->second, messageId);

  return {FederationTsoRegistryStatus::applied,
          TsoMessageQueueStatus::applied,
          messageId,
          enqueuedCount};
}

ObjectInstanceDeletionPlan EmbeddedFederationRegistry::planTsoObjectInstanceDeletion(
    std::wstring const& federationName,
    std::uint64_t producingFederateId,
    std::uint64_t objectInstanceHandle) const {
  auto instrumentationScope = beginInstrumentation("planTsoObjectInstanceDeletion");
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
  auto instrumentationScope = beginInstrumentation("enqueueTsoObjectDeletion");
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
    auto const reportRoute = federation->second.serviceReportRoutes.find(
        receivingFederateId);
    recipients.push_back({
        receivingFederateId,
        objectInstanceHandle,
        callbackRoute->second,
        reportRoute == federation->second.serviceReportRoutes.end()
            ? FederateServiceReportRoute{}
            : reportRoute->second,
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

  // Preserve the complete invocation-time object state before the first
  // timestamped removal commits deleteAccepted.  IEEE 1516.1-2025 requires a
  // legal later Retract to reconstitute this instance and reassume its
  // attributes' original ownership; callback payload alone cannot do that.
  auto const [reconstitutionPosition, reconstitutionInserted] =
      federation->second.tsoObjectDeletionReconstitutionRecords.emplace(
          messageId,
          Federation::TsoObjectDeletionReconstitutionRecord{instance->second});
  static_cast<void>(reconstitutionPosition);
  if (!reconstitutionInserted) {
    federation->second.tsoObjectDeletionMessages.erase(messagePosition);
    invalid.deletionStatus = ObjectInstanceDeletionStatus::inconsistent_catalog;
    return invalid;
  }

  Federation::TsoRequestRetractionRecord retractionRecord;
  retractionRecord.producingFederateId = producingFederateId;
  retractionRecord.timestamp = messagePosition->second.timestamp;
  for (auto const& recipient : recipients) {
    retractionRecord.recipientStates.emplace(
        recipient.receivingFederateId,
        Federation::TsoRecipientDeliveryState::pending);
  }
  auto const [retractionPosition, retractionInserted] =
      federation->second.tsoRequestRetractionRecords.emplace(
          messageId,
          std::move(retractionRecord));
  static_cast<void>(retractionPosition);
  if (!retractionInserted) {
    federation->second.tsoObjectDeletionReconstitutionRecords.erase(messageId);
    federation->second.tsoObjectDeletionMessages.erase(messagePosition);
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
    federation->second.tsoObjectDeletionReconstitutionRecords.erase(messageId);
    federation->second.tsoRequestRetractionRecords.erase(messageId);
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
  // until its corresponding Remove Object Instance boundary.  Keep the
  // timestamped-deletion marker after acceptance in either case: its message
  // retraction designator remains valid until the producer reaches the
  // standard's time bound, and retaining the object/name prevents a later
  // reconstitution from colliding with a newly registered instance.
  if (recipients.empty()) {
    instance->second.deleteAccepted = true;
    instance->second.knownObjectClassHandlesByFederate.erase(producingFederateId);
    instance->second.pendingTimestampedRemovalFederates.clear();
  }

  return {FederationTsoRegistryStatus::applied,
          TsoMessageQueueStatus::applied,
          messageId,
          enqueuedCount,
          std::move(recipients),
          ObjectInstanceDeletionStatus::applied};
}

FederationTsoDirectedInteractionEnqueueResult
EmbeddedFederationRegistry::enqueueTsoDirectedInteraction(
    std::wstring const& federationName,
    TsoDirectedInteractionMessage message,
    std::vector<std::uint64_t> const& queuedRecipientFederateIds) {
  auto instrumentationScope = beginInstrumentation("enqueueTsoDirectedInteraction");
  if (message.producingFederateId == 0 ||
      message.objectInstanceHandle == 0 ||
      message.sentInteractionClassHandle == 0 ||
      message.sentParameterHandles.size() != message.parameters.size() ||
      message.recipients.empty() || !message.timestamp ||
      message.timestamp->isInitial() || message.timestamp->isFinal()) {
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
  if (message.timestamp->implementationName() !=
      federation->second.definition.logicalTimeImplementationName) {
    return {FederationTsoRegistryStatus::invalid_request,
            TsoMessageQueueStatus::logical_time_implementation_mismatch,
            0,
            0};
  }

  std::set<std::uint64_t> knownRecipients;
  for (auto const& recipient : message.recipients) {
    if (recipient.receivingFederateId == 0 ||
        recipient.receivingFederateId == message.producingFederateId ||
        !recipient.callbackRoute ||
        !federation->second.members.contains(recipient.receivingFederateId) ||
        !knownRecipients.insert(recipient.receivingFederateId).second) {
      return {FederationTsoRegistryStatus::invalid_request,
              TsoMessageQueueStatus::invalid_recipient,
              0,
              0};
    }
  }
  std::set<std::uint64_t> queuedRecipients;
  for (auto const recipientFederateId : queuedRecipientFederateIds) {
    if (!queuedRecipients.insert(recipientFederateId).second ||
        !knownRecipients.contains(recipientFederateId)) {
      return {FederationTsoRegistryStatus::invalid_request,
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
      federation->second.tsoDirectedInteractionMessages.emplace(
          messageId, std::move(message));
  if (!inserted) {
    return {FederationTsoRegistryStatus::invalid_request,
            TsoMessageQueueStatus::invalid_message_id,
            0,
            0};
  }

  Federation::TsoRequestRetractionRecord retractionRecord;
  retractionRecord.producingFederateId = messagePosition->second.producingFederateId;
  retractionRecord.timestamp = messagePosition->second.timestamp;
  for (auto const recipientFederateId : knownRecipients) {
    retractionRecord.recipientStates.emplace(
        recipientFederateId,
        Federation::TsoRecipientDeliveryState::pending);
  }
  auto const [recordPosition, recordInserted] =
      federation->second.tsoRequestRetractionRecords.emplace(
          messageId, std::move(retractionRecord));
  static_cast<void>(recordPosition);
  if (!recordInserted) {
    federation->second.tsoDirectedInteractionMessages.erase(messagePosition);
    return {FederationTsoRegistryStatus::invalid_request,
            TsoMessageQueueStatus::invalid_message_id,
            0,
            0};
  }

  std::size_t enqueuedCount = 0;
  TsoMessageQueueStatus queueStatus = TsoMessageQueueStatus::applied;
  for (auto const recipientFederateId : queuedRecipients) {
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
    federation->second.tsoDirectedInteractionMessages.erase(messageId);
    federation->second.tsoRequestRetractionRecords.erase(messageId);
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

FederationTsoRetractionResult EmbeddedFederationRegistry::retractTsoMessage(
    std::wstring const& federationName,
    std::uint64_t messageId) {
  auto instrumentationScope = beginInstrumentation("retractTsoMessage");
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
  auto instrumentationScope = beginInstrumentation("retractTsoInteraction");
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

std::size_t EmbeddedFederationRegistry::retireTsoMessagePayloadsAtOrBefore(
    std::wstring const& federationName,
    std::uint64_t producingFederateId,
    rti1516_2025::LogicalTime const& retractionLowerBound) {
  std::scoped_lock lock(mutex_);
  auto federation = federations_.find(federationName);
  if (federation == federations_.end() ||
      !federation->second.members.contains(producingFederateId)) {
    return 0;
  }

  std::size_t retiredCount = 0;
  for (auto& [messageId, record] : federation->second.tsoRequestRetractionRecords) {
    static_cast<void>(messageId);
    if (record.producingFederateId != producingFederateId || record.terminal ||
        !record.timestamp) {
      continue;
    }

    bool stillRetractable = false;
    try {
      // Clause 8.22.3 permits a Retract only for a timestamp strictly later
      // than the producer's current/requested time plus actual lookahead.
      stillRetractable = retractionLowerBound < *record.timestamp;
    } catch (...) {
      // A mismatched/corrupt private time value must retain its payload rather
      // than turning a potentially valid public designator into a tombstone.
      continue;
    }
    if (stillRetractable) {
      continue;
    }

    record.terminal = true;
    record.timestamp.reset();
    ++retiredCount;
  }
  reclaimTsoMessagePayloads(federation->second);
  return retiredCount;
}

FederationTsoRetractionResult EmbeddedFederationRegistry::retractTsoMessageForProducer(
    std::wstring const& federationName,
    std::uint64_t producingFederateId,
    std::uint64_t messageId,
    std::shared_ptr<rti1516_2025::LogicalTime const> const& retractionLowerBound) {
  auto instrumentationScope = beginInstrumentation("retractTsoMessageForProducer");
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

  auto timestampIsRetractable = [&retractionLowerBound](
                                    std::shared_ptr<rti1516_2025::LogicalTime const> const& timestamp) {
    if (!retractionLowerBound || !timestamp) {
      return false;
    }
    try {
      // 8.22.3 requires a strict inequality: equal to current/requested time
      // plus actual lookahead is not sufficient for a legal Retract.
      return *retractionLowerBound < *timestamp;
    } catch (...) {
      return false;
    }
  };

  // Supported timestamped message families have a federation-owned recipient
  // ledger, so a legal retraction can both suppress still-pending fanout and
  // request retraction from recipients whose original callback has entered
  // user code.
  if (auto const retractionRecord =
          federation->second.tsoRequestRetractionRecords.find(messageId);
      retractionRecord != federation->second.tsoRequestRetractionRecords.end()) {
    if (retractionRecord->second.producingFederateId != producingFederateId) {
      return {FederationTsoRegistryStatus::invalid_request,
              {TsoMessageQueueStatus::message_not_found, 0}};
    }
    if (retractionRecord->second.terminal) {
      return {FederationTsoRegistryStatus::applied,
              {retractionRecord->second.retractionApplied
                   ? TsoMessageQueueStatus::message_already_retracted
                   : TsoMessageQueueStatus::message_not_found,
               0},
              false};
    }
    if (!timestampIsRetractable(retractionRecord->second.timestamp)) {
      return {FederationTsoRegistryStatus::applied,
              {TsoMessageQueueStatus::message_not_found, 0},
              false};
    }

    if (retractionRecord->second.retractionApplied) {
      return {FederationTsoRegistryStatus::applied,
              {TsoMessageQueueStatus::message_already_retracted, 0}};
    }

    auto const objectDeletion = federation->second.tsoObjectDeletionMessages.find(messageId);
    bool const retractsObjectDeletion =
        objectDeletion != federation->second.tsoObjectDeletionMessages.end();
    auto reconstitution = federation->second.tsoObjectDeletionReconstitutionRecords.end();
    if (retractsObjectDeletion) {
      reconstitution = federation->second.tsoObjectDeletionReconstitutionRecords.find(messageId);
      if (reconstitution == federation->second.tsoObjectDeletionReconstitutionRecords.end()) {
        // A deletion callback may not be retracted without the exact
        // invocation-time object state required by 8.22.  Do not downgrade
        // this to a callback-only notification.
        return {FederationTsoRegistryStatus::invalid_request,
                {TsoMessageQueueStatus::message_not_found, 0}};
      }
    }

    auto queueResult = federation->second.timeCoordinator.retractPendingTsoMessage(messageId);
    if (queueResult.status == TsoMessageQueueStatus::message_not_found) {
      // A supported timestamped message sent only to nonconstrained recipients
      // never enters the temporal queue. A resigned pending recipient may
      // likewise have had its entry removed by lifecycle cleanup. The ledger
      // remains authoritative for the legal service state in either case.
      queueResult = {TsoMessageQueueStatus::applied, 0};
    }
    if (queueResult.status != TsoMessageQueueStatus::applied) {
      return {FederationTsoRegistryStatus::applied, queueResult};
    }

    FederationTsoRetractionResult result{
        FederationTsoRegistryStatus::applied,
        queueResult};
    for (auto& [receivingFederateId, recipientState] :
         retractionRecord->second.recipientStates) {
      if (recipientState == Federation::TsoRecipientDeliveryState::delivered) {
        recipientState = Federation::TsoRecipientDeliveryState::retracted;
        auto const callbackRoute = federation->second.interactionCallbackRoutes.find(
            receivingFederateId);
        if (federation->second.members.contains(receivingFederateId) &&
            callbackRoute != federation->second.interactionCallbackRoutes.end() &&
            callbackRoute->second) {
          result.requestRetractionNotifications.push_back({
              receivingFederateId,
              messageId,
              callbackRoute->second,
          });
        }
      } else if (recipientState == Federation::TsoRecipientDeliveryState::pending) {
        recipientState = Federation::TsoRecipientDeliveryState::retracted;
      }
    }

    if (retractsObjectDeletion) {
      auto const objectInstanceHandle = objectDeletion->second.objectInstanceHandle;
      auto instance = federation->second.objectInstances.find(objectInstanceHandle);
      bool const deletionCommitted =
          instance == federation->second.objectInstances.end() || instance->second.deleteAccepted;

      if (deletionCommitted) {
        // Restore the exact invocation-time state first.  Request Retraction
        // is queued only after this registry transition, so an immediate
        // recipient can observe a reconstituted known instance while handling
        // its callback.  Preserve only currently joined holders: the standard
        // reassumes attributes to federates that are still joined, while a
        // departed handle cannot regain ownership in this bounded profile.
        auto restored = reconstitution->second.objectInstanceAtInvocation;
        for (auto known = restored.knownObjectClassHandlesByFederate.begin();
             known != restored.knownObjectClassHandlesByFederate.end();) {
          if (!federation->second.members.contains(known->first)) {
            known = restored.knownObjectClassHandlesByFederate.erase(known);
          } else {
            ++known;
          }
        }
        for (auto owner = restored.attributeOwnersByHandle.begin();
             owner != restored.attributeOwnersByHandle.end();) {
          if (!federation->second.members.contains(owner->second)) {
            owner = restored.attributeOwnersByHandle.erase(owner);
          } else {
            ++owner;
          }
        }
        for (auto association = restored.updateRegionsByAttribute.begin();
             association != restored.updateRegionsByAttribute.end();) {
          for (auto region = association->second.begin(); region != association->second.end();) {
            if (!federation->second.regions.contains(*region)) {
              region = association->second.erase(region);
            } else {
              ++region;
            }
          }
          if (association->second.empty()) {
            association = restored.updateRegionsByAttribute.erase(association);
          } else {
            ++association;
          }
        }
        restored.deleteAccepted = false;
        restored.pendingTimestampedDeletionMessageId.reset();
        restored.pendingTimestampedRemovalFederates.clear();
        federation->second.objectInstanceHandlesByName.insert_or_assign(
            restored.name,
            restored.handle);
        federation->second.objectInstances.insert_or_assign(
            objectInstanceHandle,
            std::move(restored));
        refreshRegionUsage(federation->second);
      } else if (instance != federation->second.objectInstances.end() &&
                 instance->second.pendingTimestampedDeletionMessageId == messageId) {
        // Before any Remove Object Instance callback starts, the object has
        // not been deleted.  Cancelling the pending deletion must not roll
        // back unrelated state changes that occurred after the invocation.
        instance->second.pendingTimestampedDeletionMessageId.reset();
        instance->second.pendingTimestampedRemovalFederates.clear();
      }

      federation->second.tsoObjectDeletionReconstitutionRecords.erase(messageId);
      federation->second.tsoObjectDeletionMessages.erase(messageId);
    }
    retractionRecord->second.retractionApplied = true;
    retractionRecord->second.terminal = true;
    retractionRecord->second.timestamp.reset();
    reclaimTsoMessagePayload(federation->second, messageId);
    return result;
  }

  // Every timestamped deletion must carry the execution-owned recipient
  // ledger and reconstitution snapshot installed by enqueueTsoObjectDeletion.
  // Do not retain the former snapshot-less fallback here: it could only
  // suppress a pending callback and would make a delivered deletion look
  // retractable without the object state required by 8.22.3.
  if (federation->second.tsoObjectDeletionMessages.contains(messageId)) {
    return {FederationTsoRegistryStatus::invalid_request,
            {TsoMessageQueueStatus::message_not_found, 0}};
  }

  bool ownsMessage = false;
  std::shared_ptr<rti1516_2025::LogicalTime const> messageTimestamp;
  if (auto const interaction = federation->second.tsoInteractionMessages.find(messageId);
      interaction != federation->second.tsoInteractionMessages.end()) {
    ownsMessage = interaction->second.producingFederateId == producingFederateId;
    messageTimestamp = interaction->second.timestamp;
  }
  if (auto const attributeUpdate =
          federation->second.tsoAttributeUpdateMessages.find(messageId);
      attributeUpdate != federation->second.tsoAttributeUpdateMessages.end()) {
    ownsMessage = attributeUpdate->second.producingFederateId == producingFederateId;
    messageTimestamp = attributeUpdate->second.timestamp;
  }
  if (auto const directedInteraction =
          federation->second.tsoDirectedInteractionMessages.find(messageId);
      directedInteraction != federation->second.tsoDirectedInteractionMessages.end()) {
    ownsMessage = directedInteraction->second.producingFederateId == producingFederateId;
    messageTimestamp = directedInteraction->second.timestamp;
  }
  if (!ownsMessage) {
    return {FederationTsoRegistryStatus::invalid_request,
              {TsoMessageQueueStatus::message_not_found, 0}};
  }
  if (!timestampIsRetractable(messageTimestamp)) {
    return {FederationTsoRegistryStatus::applied,
            {TsoMessageQueueStatus::message_not_found, 0},
            false};
  }
  auto const queueResult = federation->second.timeCoordinator.retractTsoMessage(messageId);
  if (queueResult.status == TsoMessageQueueStatus::applied) {
    federation->second.tsoDirectedInteractionMessages.erase(messageId);
  }
  return {FederationTsoRegistryStatus::applied, queueResult};
}

bool EmbeddedFederationRegistry::beginTsoInteractionCallback(
    std::wstring const& federationName,
    std::uint64_t receivingFederateId,
    std::uint64_t messageId) {
  auto instrumentationScope = beginInstrumentation("beginTsoInteractionCallback");
  std::scoped_lock lock(mutex_);
  auto federation = federations_.find(federationName);
  if (federation == federations_.end() || receivingFederateId == 0 ||
      !federation->second.members.contains(receivingFederateId)) {
    return false;
  }
  auto record = federation->second.tsoRequestRetractionRecords.find(messageId);
  if (record == federation->second.tsoRequestRetractionRecords.end()) {
    return false;
  }
  auto recipient = record->second.recipientStates.find(receivingFederateId);
  if (recipient == record->second.recipientStates.end() ||
      recipient->second != Federation::TsoRecipientDeliveryState::pending) {
    return false;
  }
  recipient->second = Federation::TsoRecipientDeliveryState::delivered;
  reclaimTsoMessagePayload(federation->second, messageId);
  return true;
}

bool EmbeddedFederationRegistry::beginTsoAttributeUpdateCallback(
    std::wstring const& federationName,
    std::uint64_t receivingFederateId,
    std::uint64_t messageId) {
  auto instrumentationScope = beginInstrumentation("beginTsoAttributeUpdateCallback");
  std::scoped_lock lock(mutex_);
  auto federation = federations_.find(federationName);
  if (federation == federations_.end() || receivingFederateId == 0 ||
      !federation->second.members.contains(receivingFederateId)) {
    return false;
  }
  auto record = federation->second.tsoRequestRetractionRecords.find(messageId);
  if (record == federation->second.tsoRequestRetractionRecords.end()) {
    return false;
  }
  auto recipient = record->second.recipientStates.find(receivingFederateId);
  if (recipient == record->second.recipientStates.end() ||
      (recipient->second != Federation::TsoRecipientDeliveryState::pending &&
       recipient->second != Federation::TsoRecipientDeliveryState::delivered)) {
    return false;
  }
  recipient->second = Federation::TsoRecipientDeliveryState::delivered;
  reclaimTsoMessagePayload(federation->second, messageId);
  return true;
}

bool EmbeddedFederationRegistry::finishTsoRecipientCallbackSuppressed(
    std::wstring const& federationName,
    std::uint64_t receivingFederateId,
    std::uint64_t messageId) {
  auto instrumentationScope = beginInstrumentation("finishTsoRecipientCallbackSuppressed");
  std::scoped_lock lock(mutex_);
  auto federation = federations_.find(federationName);
  if (federation == federations_.end() || receivingFederateId == 0 ||
      !federation->second.members.contains(receivingFederateId)) {
    return false;
  }
  auto record = federation->second.tsoRequestRetractionRecords.find(messageId);
  if (record == federation->second.tsoRequestRetractionRecords.end()) {
    return false;
  }
  auto recipient = record->second.recipientStates.find(receivingFederateId);
  if (recipient == record->second.recipientStates.end() ||
      recipient->second != Federation::TsoRecipientDeliveryState::pending) {
    return false;
  }

  // The temporal boundary was consumed, but the current declaration did not
  // qualify for a user callback. Preserve this as its own terminal state so a
  // later Retract cannot issue Request Retraction for a callback that never
  // began, while releasing any lifecycle guard that only protects pending
  // object-dependent delivery.
  recipient->second = Federation::TsoRecipientDeliveryState::suppressed;
  reclaimTsoMessagePayload(federation->second, messageId);
  return true;
}

bool EmbeddedFederationRegistry::canDeliverTsoRequestRetraction(
    std::wstring const& federationName,
    std::uint64_t receivingFederateId,
    std::uint64_t messageId) const {
  std::scoped_lock lock(mutex_);
  auto const federation = federations_.find(federationName);
  if (federation == federations_.end() || receivingFederateId == 0 ||
      !federation->second.members.contains(receivingFederateId) ||
      !federation->second.interactionCallbackRoutes.contains(receivingFederateId)) {
    return false;
  }
  auto const record = federation->second.tsoRequestRetractionRecords.find(messageId);
  if (record == federation->second.tsoRequestRetractionRecords.end()) {
    return false;
  }
  auto const recipient = record->second.recipientStates.find(receivingFederateId);
  return recipient != record->second.recipientStates.end() &&
      recipient->second == Federation::TsoRecipientDeliveryState::retracted;
}

FederationTsoDeliveryRegistryResult EmbeddedFederationRegistry::beginTsoDelivery(
    std::wstring const& federationName,
    std::uint64_t recipientFederateId,
    rti1516_2025::LogicalTime const& boundary,
    bool inclusive) {
  auto instrumentationScope = beginInstrumentation("beginTsoDelivery");
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
    if (auto const directedInteraction =
            federation->second.tsoDirectedInteractionMessages.find(
                queuedMessage.messageId);
        directedInteraction != federation->second.tsoDirectedInteractionMessages.end()) {
      result.emplace_back(TsoDirectedInteractionDelivery{
          queuedMessage, directedInteraction->second});
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

  auto retractionRecord = federation->second.tsoRequestRetractionRecords.find(messageId);
  if (retractionRecord == federation->second.tsoRequestRetractionRecords.end()) {
    return std::nullopt;
  }
  auto recipientState = retractionRecord->second.recipientStates.find(receivingFederateId);
  if (recipientState == retractionRecord->second.recipientStates.end() ||
      recipientState->second != Federation::TsoRecipientDeliveryState::pending) {
    // A concurrent legal Retract won the callback boundary.  The queue may
    // already have yielded this typed payload, but it must not commit a stale
    // Remove Object Instance after its recipient state was retracted.
    return std::nullopt;
  }

  instance->second.pendingTimestampedRemovalFederates.erase(receivingFederateId);
  auto const known = instance->second.knownObjectClassHandlesByFederate.find(
      receivingFederateId);
  if (!federation->second.members.contains(receivingFederateId) ||
      known == instance->second.knownObjectClassHandlesByFederate.end()) {
    return std::nullopt;
  }

  // Commit the recipient state before changing the execution-wide object
  // state.  A later Retract observes this as a delivered recipient, restores
  // the invocation snapshot, and queues Request Retraction after releasing
  // the registry lock.
  recipientState->second = Federation::TsoRecipientDeliveryState::delivered;

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
  // Retain the deletion marker and backing object while the returned
  // retraction designator remains execution-owned.  This makes a legal
  // post-delivery reconstitution atomic and prevents a name/handle collision
  // with a subsequently registered object.
  reclaimTsoMessagePayload(federation->second, messageId);
  return result;
}

FederationTsoDeliveryRegistryResult EmbeddedFederationRegistry::completeTsoDelivery(
    std::wstring const& federationName,
    TsoQueuedMessage const& message) {
  auto instrumentationScope = beginInstrumentation("completeTsoDelivery");
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

std::vector<ObjectInstanceRemovalRecipient>
EmbeddedFederationRegistry::releaseConnectionLossDeferredObjectInstanceRemovals(
    std::wstring const& federationName,
    std::uint64_t receivingFederateId) {
  auto instrumentationScope = beginInstrumentation(
      "releaseConnectionLossDeferredObjectInstanceRemovals");
  std::scoped_lock lock(mutex_);
  auto federation = federations_.find(federationName);
  if (federation == federations_.end() || receivingFederateId == 0 ||
      !federation->second.members.contains(receivingFederateId)) {
    return {};
  }

  std::vector<ObjectInstanceRemovalRecipient> result;
  for (auto& [objectInstanceHandle, instance] : federation->second.objectInstances) {
    if (!instance.pendingRemovalFederates.contains(receivingFederateId) ||
        !instance.deferredConnectionLossTsoRemovalFederates.contains(
            receivingFederateId) ||
        hasPendingConnectionLossTsoObjectDelivery(
            federation->second,
            objectInstanceHandle,
            receivingFederateId)) {
      continue;
    }
    auto const route = federation->second.interactionCallbackRoutes.find(receivingFederateId);
    if (route == federation->second.interactionCallbackRoutes.end() || !route->second) {
      // The original forced-resign validation established a route. Retain the
      // reservation if a later private lifecycle transition has made that
      // route unavailable; recipient resignation clears it explicitly.
      continue;
    }
    auto const reportRoute = federation->second.serviceReportRoutes.find(receivingFederateId);

    // Claim the release exactly once. The original callback returned without
    // delivery at the protected boundary; this fresh route is submitted only
    // after the grant callback has completed.
    instance.deferredConnectionLossTsoRemovalFederates.erase(receivingFederateId);
    result.push_back({
        receivingFederateId,
        objectInstanceHandle,
        route->second,
        reportRoute == federation->second.serviceReportRoutes.end()
            ? FederateServiceReportRoute{}
            : reportRoute->second,
    });
  }
  return result;
}

std::optional<FederationTimeExecutionSnapshot> EmbeddedFederationRegistry::makeTimeSnapshot(
    Federation const& federation) {
  FederationTimeExecutionSnapshot result{federation.definition, {}};
  result.nonRegulatedGrant = federation.nonRegulatedGrantSwitch;
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
    bool const grantEligible = grantPolicy.decide(requester->time, bounds).mayGrant() ||
        connectionLossTsoDeliveryMayGrant(
            federation,
            federateId,
            requester->time,
            bounds);
    if (!grantEligible) {
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

std::optional<std::wstring> EmbeddedFederationRegistry::federateNameFor(
    std::wstring const& federationName,
    std::uint64_t federateId) const {
  std::scoped_lock lock(mutex_);
  auto const federation = federations_.find(federationName);
  if (federation == federations_.end()) {
    return std::nullopt;
  }

  auto const knownName = federation->second.federateNamesById.find(federateId);
  if (knownName == federation->second.federateNamesById.end()) {
    return std::nullopt;
  }
  return knownName->second;
}

std::optional<unsigned long> EmbeddedFederationRegistry::normalizedFederateHandleValueFor(
    std::wstring const& federationName,
    std::uint64_t federateId) const {
  std::scoped_lock lock(mutex_);
  auto const federation = federations_.find(federationName);
  if (federation == federations_.end() ||
      !federation->second.federateNamesById.contains(federateId)) {
    return std::nullopt;
  }
  return normalizedHandleValue(
      federation->second.normalizationSeed,
      federateId,
      kFederateNormalizationKind);
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

std::optional<unsigned long>
EmbeddedFederationRegistry::normalizedObjectClassHandleValueFor(
    std::wstring const& federationName,
    std::uint64_t objectClassHandle) const {
  std::scoped_lock lock(mutex_);
  auto const federation = federations_.find(federationName);
  if (federation == federations_.end() || !federation->second.definition.catalog ||
      !federation->second.objectClassHandles) {
    return std::nullopt;
  }
  auto const name = federation->second.objectClassHandles->nameFor(objectClassHandle);
  if (!name || federation->second.definition.catalog->objectClass(*name) == nullptr) {
    return std::nullopt;
  }
  return normalizedHandleValue(
      federation->second.normalizationSeed,
      objectClassHandle,
      kObjectClassNormalizationKind);
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

std::optional<unsigned long>
EmbeddedFederationRegistry::normalizedInteractionClassHandleValueFor(
    std::wstring const& federationName,
    std::uint64_t interactionClassHandle) const {
  std::scoped_lock lock(mutex_);
  auto const federation = federations_.find(federationName);
  if (federation == federations_.end() || !federation->second.definition.catalog ||
      !federation->second.interactionClassHandles) {
    return std::nullopt;
  }
  auto const name = federation->second.interactionClassHandles->nameFor(
      interactionClassHandle);
  if (!name || federation->second.definition.catalog->interactionClass(*name) == nullptr) {
    return std::nullopt;
  }
  return normalizedHandleValue(
      federation->second.normalizationSeed,
      interactionClassHandle,
      kInteractionClassNormalizationKind);
}

std::optional<unsigned long>
EmbeddedFederationRegistry::normalizedObjectInstanceHandleValueFor(
    std::wstring const& federationName,
    std::uint64_t objectInstanceHandle) const {
  std::scoped_lock lock(mutex_);
  auto const federation = federations_.find(federationName);
  if (federation == federations_.end() ||
      (!federation->second.objectInstances.contains(objectInstanceHandle) &&
       !federation->second.rtiOwnedJoinedFederateMomObjects.contains(objectInstanceHandle))) {
    return std::nullopt;
  }
  return normalizedHandleValue(
      federation->second.normalizationSeed,
      objectInstanceHandle,
      kObjectInstanceNormalizationKind);
}

std::optional<bool> EmbeddedFederationRegistry::interactionClassIsSameOrDescendantOf(
    std::wstring const& federationName,
    std::uint64_t interactionClassHandle,
    std::string const& ancestorInteractionClassName) const {
  std::scoped_lock lock(mutex_);
  auto const federation = federations_.find(federationName);
  if (federation == federations_.end() || !federation->second.definition.catalog ||
      !federation->second.interactionClassHandles) {
    return std::nullopt;
  }
  auto const interactionClassName = federation->second.interactionClassHandles->nameFor(
      interactionClassHandle);
  if (!interactionClassName ||
      federation->second.definition.catalog->interactionClass(*interactionClassName) == nullptr) {
    return std::nullopt;
  }

  std::set<std::string> visited;
  std::string currentClassName = *interactionClassName;
  while (!currentClassName.empty() && visited.insert(currentClassName).second) {
    if (currentClassName == ancestorInteractionClassName) {
      return true;
    }
    auto const* definition = federation->second.definition.catalog->interactionClass(
        currentClassName);
    if (definition == nullptr) {
      return std::nullopt;
    }
    currentClassName = definition->parentName;
  }
  return false;
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

UpdateRateValueResult EmbeddedFederationRegistry::updateRateValueForDesignator(
    std::wstring const& federationName,
    std::uint64_t federateId,
    std::string const& updateRateDesignator) const {
  std::scoped_lock lock(mutex_);
  auto const federation = federations_.find(federationName);
  if (federation == federations_.end()) {
    return {UpdateRateValueStatus::federation_does_not_exist, 0.0};
  }
  if (!federation->second.members.contains(federateId)) {
    return {UpdateRateValueStatus::federate_not_member, 0.0};
  }
  if (!federation->second.definition.catalog) {
    return {UpdateRateValueStatus::inconsistent_catalog, 0.0};
  }

  auto const normalized = normalizedUpdateRateDesignator(
      *federation->second.definition.catalog,
      updateRateDesignator);
  if (!normalized) {
    return {UpdateRateValueStatus::invalid_update_rate_designator, 0.0};
  }
  auto const value = updateRateValueForNormalizedDesignator(
      *federation->second.definition.catalog,
      *normalized);
  if (!value) {
    return {UpdateRateValueStatus::inconsistent_catalog, 0.0};
  }
  return {UpdateRateValueStatus::applied, *value};
}

UpdateRateValueResult EmbeddedFederationRegistry::updateRateValueForAttribute(
    std::wstring const& federationName,
    std::uint64_t federateId,
    std::uint64_t objectInstanceHandle,
    std::uint64_t attributeHandle) const {
  std::scoped_lock lock(mutex_);
  auto const federation = federations_.find(federationName);
  if (federation == federations_.end()) {
    return {UpdateRateValueStatus::federation_does_not_exist, 0.0};
  }
  if (!federation->second.members.contains(federateId)) {
    return {UpdateRateValueStatus::federate_not_member, 0.0};
  }
  if (!federation->second.definition.catalog ||
      !federation->second.objectClassHandles ||
      !federation->second.attributeHandles) {
    return {UpdateRateValueStatus::inconsistent_catalog, 0.0};
  }

  auto const instance = federation->second.objectInstances.find(objectInstanceHandle);
  if (instance == federation->second.objectInstances.end() ||
      instance->second.deleteAccepted) {
    return {UpdateRateValueStatus::object_instance_not_known, 0.0};
  }
  auto const knownClass = instance->second.knownObjectClassHandlesByFederate.find(federateId);
  if (knownClass == instance->second.knownObjectClassHandlesByFederate.end()) {
    return {UpdateRateValueStatus::object_instance_not_known, 0.0};
  }
  auto const knownClassName = federation->second.objectClassHandles->nameFor(knownClass->second);
  if (!knownClassName ||
      federation->second.definition.catalog->objectClass(*knownClassName) == nullptr) {
    return {UpdateRateValueStatus::inconsistent_catalog, 0.0};
  }
  auto const attributeName = federation->second.attributeHandles->nameFor(
      federation->second.definition.catalog.get(),
      *knownClassName,
      attributeHandle);
  if (!attributeName) {
    return {UpdateRateValueStatus::attribute_not_defined, 0.0};
  }

  auto const declarations = federation->second.objectClassAttributeDeclarations.find(federateId);
  if (declarations == federation->second.objectClassAttributeDeclarations.end()) {
    return {UpdateRateValueStatus::applied, 0.0};
  }

  // The service returns the maximum rate currently represented by the
  // receiver's applicable ordinary/regional subscriptions.  Walk the known
  // class lineage just as routing does, then take the maximum across regional
  // declarations because the query has no region argument.  Missing designator
  // state is the standard HLAdefault/no-reduction value for declarations
  // created before the rate map was introduced.
  static_cast<void>(*attributeName);
  double maximumRate = 0.0;
  std::set<std::string> visited;
  std::string currentClassName = *knownClassName;
  while (!currentClassName.empty() && visited.insert(currentClassName).second) {
    auto const* currentClass = federation->second.definition.catalog->objectClass(currentClassName);
    auto const currentClassHandle = federation->second.objectClassHandles->handleFor(currentClassName);
    if (currentClass == nullptr || !currentClassHandle) {
      return {UpdateRateValueStatus::inconsistent_catalog, 0.0};
    }
    auto const perClass = declarations->second.byObjectClass.find(*currentClassHandle);
    if (perClass != declarations->second.byObjectClass.end()) {
      auto const ordinary = perClass->second.subscribedAttributes.find(attributeHandle);
      if (ordinary != perClass->second.subscribedAttributes.end()) {
        auto const designator = perClass->second.subscribedUpdateRateDesignators.find(
            attributeHandle);
        std::string const normalized = designator ==
                perClass->second.subscribedUpdateRateDesignators.end() ||
                designator->second.empty()
            ? "HLAdefault"
            : designator->second;
        auto const value = updateRateValueForNormalizedDesignator(
            *federation->second.definition.catalog,
            normalized);
        if (!value) {
          return {UpdateRateValueStatus::inconsistent_catalog, 0.0};
        }
        maximumRate = std::max(maximumRate, *value);
      }

      auto const regional = perClass->second.regionalSubscribedAttributes.find(attributeHandle);
      if (regional != perClass->second.regionalSubscribedAttributes.end()) {
        auto const regionalDesignators =
            perClass->second.regionalSubscribedUpdateRateDesignators.find(attributeHandle);
        for (auto const& [regionHandle, active] : regional->second) {
          static_cast<void>(active);
          std::string normalized = "HLAdefault";
          if (regionalDesignators !=
              perClass->second.regionalSubscribedUpdateRateDesignators.end()) {
            auto const designator = regionalDesignators->second.find(regionHandle);
            if (designator != regionalDesignators->second.end() &&
                !designator->second.empty()) {
              normalized = designator->second;
            }
          }
          auto const value = updateRateValueForNormalizedDesignator(
              *federation->second.definition.catalog,
              normalized);
          if (!value) {
            return {UpdateRateValueStatus::inconsistent_catalog, 0.0};
          }
          maximumRate = std::max(maximumRate, *value);
        }
      }
    }
    currentClassName = currentClass->parentName;
  }
  return {UpdateRateValueStatus::applied, maximumRate};
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
  result.attributeRelevanceAdvisories = attributeRelevanceAdvisoriesForScopeChanges(
      federation->second,
      result.recipients);
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

bool EmbeddedFederationRegistry::isReportServiceInvocationInteractionClass(
    Federation const& federation,
    std::uint64_t interactionClassHandle) {
  if (!federation.interactionClassHandles) {
    return false;
  }
  auto const name = federation.interactionClassHandles->nameFor(interactionClassHandle);
  return name && *name == kReportServiceInvocationInteractionClassName;
}

bool EmbeddedFederationRegistry::hasReportServiceInvocationSubscription(
    Federation const& federation,
    std::uint64_t federateId) {
  if (!federation.interactionClassHandles) {
    return false;
  }
  auto const reportClassHandle = federation.interactionClassHandles->handleFor(
      kReportServiceInvocationInteractionClassName);
  if (!reportClassHandle) {
    return false;
  }
  auto const declarations = federation.interactionDeclarations.find(federateId);
  if (declarations == federation.interactionDeclarations.end()) {
    return false;
  }
  if (declarations->second.subscribedInteractionClasses.contains(*reportClassHandle)) {
    return true;
  }
  auto const regional = declarations->second.regionalSubscribedInteractionClasses.find(
      *reportClassHandle);
  return regional != declarations->second.regionalSubscribedInteractionClasses.end() &&
      !regional->second.empty();
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

  return regionSnapshotsOverlap(federation, *first, *second);
}

bool EmbeddedFederationRegistry::regionSnapshotsOverlap(
    Federation const& federation,
    RegionSpecificationSnapshot const& first,
    RegionSpecificationSnapshot const& second) {
  if (!first.specificationCommitted || !second.specificationCommitted) {
    return false;
  }

  bool sharedDimension = false;
  for (auto const& [dimensionHandle, firstRange] : first.committedRangeBounds) {
    auto const secondRange = second.committedRangeBounds.find(dimensionHandle);
    if (secondRange == second.committedRangeBounds.end()) {
      continue;
    }
    sharedDimension = true;
    // Ranges are half-open [lower, upper).  The strict 2025 definition treats
    // equal lower bounds as overlapping, or otherwise requires ordinary
    // half-open intersection.  When the federation-wide Allow Relaxed DDM
    // switch is enabled, Umbra's documented implementation-specific policy
    // expands only exactly touching committed ranges.  It never removes a
    // strict overlap and does not apply a numerical distance threshold.
    bool const strictOverlap =
        firstRange.lowerBound == secondRange->second.lowerBound ||
        (firstRange.lowerBound < secondRange->second.upperBound &&
         secondRange->second.lowerBound < firstRange.upperBound);
    if (strictOverlap) {
      continue;
    }
    bool const boundaryTouch =
        firstRange.upperBound == secondRange->second.lowerBound ||
        secondRange->second.upperBound == firstRange.lowerBound;
    if (!federation.allowRelaxedDDMSwitch || !boundaryTouch) {
      return false;
    }
  }
  return sharedDimension;
}

bool EmbeddedFederationRegistry::regionOverlapsSnapshot(
    Federation const& federation,
    std::uint64_t regionHandle,
    RegionSpecificationSnapshot const& snapshot) {
  auto const region = federation.regions.find(regionHandle);
  if (region == federation.regions.end() ||
      !snapshot.specificationCommitted ||
      !region->second.specificationCommitted) {
    return false;
  }
  return regionSnapshotsOverlap(
      federation,
      RegionSpecificationSnapshot{
          region->second.dimensionHandles,
          region->second.committedRangeBounds,
          region->second.specificationCommitted,
      },
      snapshot);
}

bool EmbeddedFederationRegistry::regionOverlapsDefault(
    Federation const& federation,
    std::uint64_t regionHandle,
    std::map<std::uint64_t, RegionSpecificationSnapshot> const* overrides) {
  std::optional<RegionSpecificationSnapshot> snapshot;
  if (overrides != nullptr) {
    auto const overrideRegion = overrides->find(regionHandle);
    if (overrideRegion != overrides->end()) {
      snapshot = overrideRegion->second;
    }
  }
  if (!snapshot) {
    auto const region = federation.regions.find(regionHandle);
    if (region == federation.regions.end()) {
      return false;
    }
    snapshot = RegionSpecificationSnapshot{
        region->second.dimensionHandles,
        region->second.committedRangeBounds,
        region->second.specificationCommitted,
    };
  }

  // The default region has the full [0, upperBound) range for every FDD
  // dimension. A committed explicit region therefore overlaps it whenever it
  // has at least one dimension. The standard separately defines an empty
  // realization as overlapping no region, including the default region.
  return snapshot->specificationCommitted &&
      !snapshot->dimensionHandles.empty() &&
      snapshot->committedRangeBounds.size() == snapshot->dimensionHandles.size();
}

void EmbeddedFederationRegistry::clearUpdateRegionAssociation(
    Federation::ObjectInstance& objectInstance,
    std::uint64_t attributeHandle) noexcept {
  objectInstance.updateRegionsByAttribute.erase(attributeHandle);
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

std::optional<rti1516_2025::OrderType> EmbeddedFederationRegistry::orderTypeFromName(
    std::string const& orderName) {
  if (orderName == "Receive") {
    return rti1516_2025::RECEIVE;
  }
  if (orderName == "TimeStamp") {
    return rti1516_2025::TIMESTAMP;
  }
  return std::nullopt;
}

std::optional<rti1516_2025::OrderType>
EmbeddedFederationRegistry::attributeDefaultOrderType(
    Federation const& federation,
    std::uint64_t federateId,
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

  auto declarations = federation.objectClassAttributeDeclarations.find(federateId);
  if (declarations != federation.objectClassAttributeDeclarations.end()) {
    std::set<std::uint64_t> visited;
    std::uint64_t currentHandle = objectClassHandle;
    while (visited.insert(currentHandle).second) {
      auto const perClass = declarations->second.byObjectClass.find(currentHandle);
      if (perClass != declarations->second.byObjectClass.end()) {
        auto const overrideType = perClass->second.defaultOrderTypes.find(attributeHandle);
        if (overrideType != perClass->second.defaultOrderTypes.end()) {
          return overrideType->second;
        }
      }
      auto const currentName = federation.objectClassHandles->nameFor(currentHandle);
      if (!currentName) {
        break;
      }
      auto const* currentClass = federation.definition.catalog->objectClass(*currentName);
      if (currentClass == nullptr || currentClass->parentName.empty()) {
        break;
      }
      auto const parentHandle = federation.objectClassHandles->handleFor(currentClass->parentName);
      if (!parentHandle) {
        break;
      }
      currentHandle = *parentHandle;
    }
  }

  std::set<std::string> visited;
  std::string currentClassName = *objectClassName;
  while (!currentClassName.empty() && visited.insert(currentClassName).second) {
    auto const* currentClass = federation.definition.catalog->objectClass(currentClassName);
    if (currentClass == nullptr) {
      return std::nullopt;
    }
    auto const definition = currentClass->declaredAttributes.find(*attributeName);
    if (definition != currentClass->declaredAttributes.end()) {
      return orderTypeFromName(definition->second.order);
    }
    currentClassName = currentClass->parentName;
  }
  return std::nullopt;
}

std::optional<rti1516_2025::OrderType>
EmbeddedFederationRegistry::effectiveAttributeOrderType(
    Federation const& federation,
    Federation::ObjectInstance const& objectInstance,
    std::uint64_t attributeHandle) {
  auto const owner = objectInstance.attributeOwnersByHandle.find(attributeHandle);
  if (owner == objectInstance.attributeOwnersByHandle.end()) {
    return std::nullopt;
  }
  auto const effective = objectInstance.attributeOrderTypes.find(attributeHandle);
  if (effective != objectInstance.attributeOrderTypes.end()) {
    return effective->second;
  }
  auto const knownClass = objectInstance.knownObjectClassHandlesByFederate.find(owner->second);
  if (knownClass != objectInstance.knownObjectClassHandlesByFederate.end()) {
    return attributeDefaultOrderType(
        federation,
        owner->second,
        knownClass->second,
        attributeHandle);
  }
  return attributeDefaultOrderType(
      federation,
      owner->second,
      objectInstance.registeredObjectClassHandle,
      attributeHandle);
}

std::optional<rti1516_2025::OrderType>
EmbeddedFederationRegistry::interactionOrderType(
    Federation const& federation,
    std::uint64_t federateId,
    std::uint64_t interactionClassHandle) {
  if (!validInteractionClass(federation, interactionClassHandle) ||
      !federation.definition.catalog ||
      !federation.interactionClassHandles) {
    return std::nullopt;
  }
  auto const className = federation.interactionClassHandles->nameFor(interactionClassHandle);
  if (!className) {
    return std::nullopt;
  }
  auto const* interactionClass = federation.definition.catalog->interactionClass(*className);
  if (interactionClass == nullptr) {
    return std::nullopt;
  }
  auto declarations = federation.interactionDeclarations.find(federateId);
  if (declarations != federation.interactionDeclarations.end()) {
    auto const overrideType = declarations->second.interactionOrderTypes.find(
        interactionClassHandle);
    if (overrideType != declarations->second.interactionOrderTypes.end()) {
      return overrideType->second;
    }
  }
  return orderTypeFromName(interactionClass->order);
}

std::optional<std::string> EmbeddedFederationRegistry::attributeDefaultTransportationName(
    Federation const& federation,
    std::uint64_t federateId,
    std::uint64_t objectClassHandle,
    std::uint64_t attributeHandle) {
  if (!validObjectClass(federation, objectClassHandle) ||
      !federation.definition.catalog || !federation.objectClassHandles) {
    return std::nullopt;
  }

  auto declarations = federation.objectClassAttributeDeclarations.find(federateId);
  if (declarations != federation.objectClassAttributeDeclarations.end()) {
    std::set<std::uint64_t> visited;
    std::uint64_t currentHandle = objectClassHandle;
    while (visited.insert(currentHandle).second) {
      auto const perClass = declarations->second.byObjectClass.find(currentHandle);
      if (perClass != declarations->second.byObjectClass.end()) {
        auto const overrideType = perClass->second.defaultTransportationTypes.find(
            attributeHandle);
        if (overrideType != perClass->second.defaultTransportationTypes.end()) {
          return overrideType->second;
        }
      }
      auto const currentName = federation.objectClassHandles->nameFor(currentHandle);
      if (!currentName) {
        break;
      }
      auto const* currentClass = federation.definition.catalog->objectClass(*currentName);
      if (currentClass == nullptr || currentClass->parentName.empty()) {
        break;
      }
      auto const parentHandle = federation.objectClassHandles->handleFor(currentClass->parentName);
      if (!parentHandle) {
        break;
      }
      currentHandle = *parentHandle;
    }
  }

  return attributeTransportationName(federation, objectClassHandle, attributeHandle);
}

std::optional<std::string> EmbeddedFederationRegistry::effectiveAttributeTransportationName(
    Federation const& federation,
    Federation::ObjectInstance const& objectInstance,
    std::uint64_t attributeHandle) {
  auto const owner = objectInstance.attributeOwnersByHandle.find(attributeHandle);
  if (owner == objectInstance.attributeOwnersByHandle.end()) {
    return attributeTransportationName(
        federation,
        objectInstance.registeredObjectClassHandle,
        attributeHandle);
  }

  auto const effective = objectInstance.attributeTransportationTypes.find(attributeHandle);
  if (effective != objectInstance.attributeTransportationTypes.end()) {
    return effective->second;
  }
  return attributeDefaultTransportationName(
      federation,
      owner->second,
      objectInstance.registeredObjectClassHandle,
      attributeHandle);
}

std::optional<std::string> EmbeddedFederationRegistry::effectiveInteractionTransportationName(
    Federation const& federation,
    std::uint64_t federateId,
    std::uint64_t interactionClassHandle) {
  if (!validInteractionClass(federation, interactionClassHandle) ||
      !federation.definition.catalog || !federation.interactionClassHandles) {
    return std::nullopt;
  }
  auto const className = federation.interactionClassHandles->nameFor(interactionClassHandle);
  if (!className) {
    return std::nullopt;
  }
  auto const* interactionClass = federation.definition.catalog->interactionClass(*className);
  if (interactionClass == nullptr || interactionClass->transportation.empty()) {
    return std::nullopt;
  }
  auto declarations = federation.interactionDeclarations.find(federateId);
  if (declarations != federation.interactionDeclarations.end()) {
    auto const overrideType = declarations->second.interactionTransportationTypes.find(
        interactionClassHandle);
    if (overrideType != declarations->second.interactionTransportationTypes.end()) {
      return overrideType->second;
    }
  }
  return interactionClass->transportation;
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

  // The candidate discovery class is the registered class when actively
  // subscribed, otherwise the closest actively subscribed superclass. Once a
  // candidate is found,
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
        auto const associated = objectInstance.updateRegionsByAttribute.find(
            ownedAttributeHandle);
        bool const hasExplicitSubscriptionRegion =
            regional != perClass->second.regionalSubscribedAttributes.end() &&
            !regional->second.empty();
        bool const hasExplicitUpdateRegion =
            associated != objectInstance.updateRegionsByAttribute.end() &&
            !associated->second.empty();

        // Ordinary declaration state uses the invisible default region only
        // while no non-default regional subscription exists for this class
        // attribute. The state remains independently retained, but §9.1.3
        // makes its effective default-region realization mutually exclusive
        // with an explicit regional realization.
        auto const ordinarySubscription = perClass->second.subscribedAttributes.find(
            ownedAttributeHandle);
        if (ordinarySubscription != perClass->second.subscribedAttributes.end() &&
            ordinarySubscription->second && !hasExplicitSubscriptionRegion) {
          return currentClassHandle;
        }
        if (!hasExplicitSubscriptionRegion) {
          continue;
        }
        for (auto const& [subscribedRegionHandle, active] : regional->second) {
          if (!active) {
            continue;
          }
          if (!hasExplicitUpdateRegion) {
            if (regionOverlapsDefault(federation, subscribedRegionHandle)) {
              return currentClassHandle;
            }
            continue;
          }
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

std::optional<std::uint64_t>
EmbeddedFederationRegistry::candidateJoinedFederateMomObjectDiscoveryClass(
    Federation const& federation,
    JoinedFederateMomObjectSnapshot const& object,
    std::uint64_t receivingFederateId) {
  if (!federation.members.contains(receivingFederateId) ||
      !federation.definition.catalog ||
      !federation.objectClassHandles ||
      !federation.attributeHandles ||
      object.objectClassHandle == 0U) {
    return std::nullopt;
  }
  auto const declarations = federation.objectClassAttributeDeclarations.find(
      receivingFederateId);
  if (declarations == federation.objectClassAttributeDeclarations.end()) {
    return std::nullopt;
  }
  auto const objectClassName = federation.objectClassHandles->nameFor(
      object.objectClassHandle);
  if (!objectClassName ||
      federation.definition.catalog->objectClass(*objectClassName) == nullptr) {
    return std::nullopt;
  }

  // MOM discovery uses the same closest-class declaration walk as ordinary
  // object routing.  An explicit regional declaration is evaluated against
  // the immutable HLAfederate point; it must never be treated as an ordinary
  // default-region subscription.
  std::set<std::string> visited;
  std::string currentClassName = *objectClassName;
  while (!currentClassName.empty() && visited.insert(currentClassName).second) {
    auto const* currentClass = federation.definition.catalog->objectClass(currentClassName);
    auto const currentClassHandle = federation.objectClassHandles->handleFor(currentClassName);
    if (currentClass == nullptr || !currentClassHandle) {
      return std::nullopt;
    }
    auto const perClass = declarations->second.byObjectClass.find(*currentClassHandle);
    if (perClass != declarations->second.byObjectClass.end()) {
      for (auto const& [attributeHandle, value] : object.initialAttributeValues) {
        static_cast<void>(value);
        if (!federation.attributeHandles->nameFor(
                federation.definition.catalog.get(),
                currentClassName,
                attributeHandle)) {
          continue;
        }
        auto const regional =
            perClass->second.regionalSubscribedAttributes.find(attributeHandle);
        if (regional != perClass->second.regionalSubscribedAttributes.end() &&
            !regional->second.empty()) {
          for (auto const& [subscribedRegionHandle, active] : regional->second) {
            if (active && regionOverlapsSnapshot(
                              federation,
                              subscribedRegionHandle,
                              object.immutableFederatePoint)) {
              return currentClassHandle;
            }
          }
          // An explicit regional declaration at this class is not an
          // ordinary subscription, even when its point does not overlap.
          continue;
        }
        auto const ordinary = perClass->second.subscribedAttributes.find(attributeHandle);
        if (ordinary != perClass->second.subscribedAttributes.end() && ordinary->second) {
          return currentClassHandle;
        }
      }
    }
    currentClassName = currentClass->parentName;
  }
  return std::nullopt;
}

std::optional<rti1516_2025::VariableLengthData>
EmbeddedFederationRegistry::joinedFederateMomObjectAttributeValue(
    Federation const& federation,
    JoinedFederateMomObjectSnapshot const& object,
    std::uint64_t attributeHandle) {
  auto const initial = object.initialAttributeValues.find(attributeHandle);
  if (initial != object.initialAttributeValues.end()) {
    return initial->second;
  }
  if (!federation.definition.catalog ||
      !federation.objectClassHandles ||
      !federation.attributeHandles) {
    return std::nullopt;
  }
  auto const objectClassName = federation.objectClassHandles->nameFor(
      object.objectClassHandle);
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
  auto const member = federation.members.find(object.joinedFederateId);
  if (member == federation.members.end()) {
    return std::nullopt;
  }

  if (*attributeName == "HLAfederateState") {
    // HLAstandardMIM-2025 defines HLAfederateState as HLAinteger32BE with
    // ActiveFederate = 1, FederateSaveInProgress = 3, and
    // FederateRestoreInProgress = 5.  The save/restore operation ledgers are
    // the authoritative state machine; unlike a callback-derived value, they
    // remain truthful while work is queued in either callback model.
    std::int32_t encodedState = 1;
    if (federation.restoreOperation.has_value()) {
      auto const status = federation.restoreOperation->statuses.find(
          object.joinedFederateId);
      if (status != federation.restoreOperation->statuses.end() &&
          status->second != rti1516_2025::NO_RESTORE_IN_PROGRESS) {
        encodedState = 5;
      }
    } else if (federation.saveOperation.has_value()) {
      auto const status = federation.saveOperation->statuses.find(
          object.joinedFederateId);
      if (status != federation.saveOperation->statuses.end() &&
          status->second != rti1516_2025::NO_SAVE_IN_PROGRESS) {
        encodedState = 3;
      }
    }
    return rti1516_2025::HLAinteger32BE{encodedState}.encode();
  }

  auto const timeState = federation.timeCoordinator.timeStateFor(
      object.joinedFederateId);
  auto const timeSnapshot = timeState ?
      std::optional<FederateTimeSnapshot>{timeState->snapshot()} :
      std::nullopt;

  auto encodeSwitch = [](bool value) {
    return rti1516_2025::HLAinteger32BE{value ? 1 : 0}.encode();
  };
  auto encodeResignAction = [&](rti1516_2025::ResignAction action) {
    std::int32_t encoded = 0;
    switch (action) {
      case rti1516_2025::UNCONDITIONALLY_DIVEST_ATTRIBUTES:
        encoded = 0;
        break;
      case rti1516_2025::DELETE_OBJECTS:
        encoded = 1;
        break;
      case rti1516_2025::CANCEL_PENDING_OWNERSHIP_ACQUISITIONS:
        encoded = 2;
        break;
      case rti1516_2025::DELETE_OBJECTS_THEN_DIVEST:
        encoded = 3;
        break;
      case rti1516_2025::CANCEL_THEN_DELETE_THEN_DIVEST:
        encoded = 4;
        break;
      case rti1516_2025::NO_ACTION:
        encoded = 5;
        break;
    }
    return rti1516_2025::HLAinteger32BE{encoded}.encode();
  };

  if (*attributeName == "HLAobjectClassRelevanceAdvisory") {
    return encodeSwitch(member->second.objectClassRelevanceAdvisorySwitch);
  }
  if (*attributeName == "HLAattributeRelevanceAdvisory") {
    return encodeSwitch(member->second.attributeRelevanceAdvisorySwitch);
  }
  if (*attributeName == "HLAattributeScopeAdvisory") {
    return encodeSwitch(member->second.attributeScopeAdvisorySwitch);
  }
  if (*attributeName == "HLAinteractionRelevanceAdvisory") {
    return encodeSwitch(member->second.interactionRelevanceAdvisorySwitch);
  }
  if (*attributeName == "HLAconveyRegionDesignatorSets") {
    return encodeSwitch(member->second.conveyRegionDesignatorSetsSwitch);
  }
  if (*attributeName == "HLAautomaticResignAction") {
    return encodeResignAction(member->second.automaticResignAction);
  }
  if (*attributeName == "HLAserviceReporting") {
    return encodeSwitch(member->second.serviceReportingSwitch);
  }
  if (*attributeName == "HLAexceptionReporting") {
    return encodeSwitch(member->second.exceptionReportingSwitch);
  }
  if (*attributeName == "HLAsendServiceReportsToFile") {
    return encodeSwitch(member->second.sendServiceReportsToFileSwitch);
  }
  if (!timeSnapshot) {
    return std::nullopt;
  }
  // IEEE 1516.2-2025 HLAstandardMIM marks HLAlogicalTime and HLAlookahead
  // Periodic, but IEEE 1516.1-2025 §11.4.1 still requires the RTI to supply
  // values for a direct Request Attribute Value Update regardless of whether
  // HLAsetTiming has ever enabled periodic reporting.  Use the selected
  // official logical-time provider's own wire representation; an undefined
  // value is represented by the MIM's empty variable-array form.
  if (*attributeName == "HLAlogicalTime") {
    return timeSnapshot->currentTime
        ? timeSnapshot->currentTime->encode()
        : rti1516_2025::VariableLengthData{};
  }
  if (*attributeName == "HLAlookahead") {
    return timeSnapshot->lookahead
        ? timeSnapshot->lookahead->encode()
        : rti1516_2025::VariableLengthData{};
  }
  if (*attributeName == "HLAtimeConstrained") {
    return rti1516_2025::HLAboolean{timeSnapshot->timeConstrained}.encode();
  }
  if (*attributeName == "HLAtimeRegulating") {
    return rti1516_2025::HLAboolean{timeSnapshot->timeRegulating}.encode();
  }
  if (*attributeName == "HLAasynchronousDelivery") {
    return rti1516_2025::HLAboolean{timeSnapshot->asynchronousDeliveryEnabled}.encode();
  }
  if (*attributeName == "HLAtimeManagerState") {
    // HLAstandardMIM-2025 defines HLAtimeState as HLAinteger32BE:
    // TimeGranted = 0 and TimeAdvancing = 1.  The private temporal state is
    // the authoritative source; do not infer this value from a callback that
    // may still be queued in either callback model.
    return rti1516_2025::HLAinteger32BE{
        timeSnapshot->timeAdvancePending ? 1 : 0}.encode();
  }
  return std::nullopt;
}

std::optional<std::map<std::uint64_t, rti1516_2025::VariableLengthData>>
EmbeddedFederationRegistry::joinedFederateMomObjectAttributeValues(
    Federation const& federation,
    JoinedFederateMomObjectSnapshot const& object,
    std::uint64_t receivingFederateId,
    std::set<std::uint64_t> const& requestedAttributeHandles,
    bool requireActiveSubscription) {
  if (!federation.members.contains(receivingFederateId) ||
      !object.knownFederateIds.contains(receivingFederateId) ||
      !federation.definition.catalog ||
      !federation.objectClassHandles ||
      !federation.attributeHandles) {
    return std::nullopt;
  }
  auto const objectClassName = federation.objectClassHandles->nameFor(
      object.objectClassHandle);
  if (!objectClassName ||
      federation.definition.catalog->objectClass(*objectClassName) == nullptr) {
    return std::nullopt;
  }
  auto const declarations = federation.objectClassAttributeDeclarations.find(
      receivingFederateId);
  if (requireActiveSubscription &&
      declarations == federation.objectClassAttributeDeclarations.end()) {
    return std::nullopt;
  }

  std::map<std::uint64_t, rti1516_2025::VariableLengthData> result;
  for (std::uint64_t const attributeHandle : requestedAttributeHandles) {
    if (!federation.attributeHandles->nameFor(
            federation.definition.catalog.get(),
            *objectClassName,
            attributeHandle)) {
      continue;
    }
    if (requireActiveSubscription) {
      bool active = false;
      std::set<std::string> visited;
      std::string currentClassName = *objectClassName;
      while (!currentClassName.empty() && visited.insert(currentClassName).second) {
        auto const* currentClass = federation.definition.catalog->objectClass(currentClassName);
        auto const currentClassHandle = federation.objectClassHandles->handleFor(currentClassName);
        if (currentClass == nullptr || !currentClassHandle) {
          break;
        }
        auto const perClass = declarations->second.byObjectClass.find(*currentClassHandle);
        if (perClass != declarations->second.byObjectClass.end() &&
            federation.attributeHandles->nameFor(
                federation.definition.catalog.get(),
                currentClassName,
                attributeHandle)) {
          auto const regional = perClass->second.regionalSubscribedAttributes.find(
              attributeHandle);
          if (regional != perClass->second.regionalSubscribedAttributes.end() &&
              !regional->second.empty()) {
            // An explicit regional declaration is evaluated at the
            // immutable HLAfederate point and must not fall through to an
            // ancestor's ordinary subscription.
            for (auto const& [subscribedRegionHandle, regionalActive] :
                 regional->second) {
              if (regionalActive && regionOverlapsSnapshot(
                                        federation,
                                        subscribedRegionHandle,
                                        object.immutableFederatePoint)) {
                active = true;
                break;
              }
            }
            break;
          }
          auto const ordinary = perClass->second.subscribedAttributes.find(attributeHandle);
          if (ordinary != perClass->second.subscribedAttributes.end()) {
            active = ordinary->second;
            break;
          }
        }
        currentClassName = currentClass->parentName;
      }
      if (!active) {
        continue;
      }
    }
    auto const value = joinedFederateMomObjectAttributeValue(
        federation,
        object,
        attributeHandle);
    if (value) {
      result.emplace(attributeHandle, *value);
    }
  }
  return result;
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
  if (receivingMember == federation.members.end()) {
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
      auto const regional = perClass->second.regionalSubscribedAttributes.find(
          attributeHandle);
      auto const& updateRegions = associationOverrides == nullptr
          ? objectInstance.updateRegionsByAttribute
          : *associationOverrides;
      auto const associated = updateRegions.find(attributeHandle);
      bool const hasExplicitSubscriptionRegion =
          regional != perClass->second.regionalSubscribedAttributes.end() &&
          !regional->second.empty();
      bool const hasExplicitUpdateRegion =
          associated != updateRegions.end() && !associated->second.empty();

      auto const ordinarySubscription = perClass->second.subscribedAttributes.find(
          attributeHandle);
      if (ordinarySubscription != perClass->second.subscribedAttributes.end() &&
          ordinarySubscription->second && !hasExplicitSubscriptionRegion) {
        return true;
      }
      if (hasExplicitSubscriptionRegion) {
        for (auto const& [subscribedRegionHandle, active] : regional->second) {
          if (!active) {
            continue;
          }
          if (!hasExplicitUpdateRegion) {
            if (regionOverlapsDefault(federation, subscribedRegionHandle, overrides)) {
              return true;
            }
            continue;
          }
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

std::optional<std::string>
EmbeddedFederationRegistry::subscribedUpdateRateDesignatorForAttribute(
    Federation const& federation,
    Federation::ObjectInstance const& objectInstance,
    std::uint64_t receivingFederateId,
    std::uint64_t attributeHandle) {
  if (!federation.definition.catalog ||
      !federation.objectClassHandles ||
      !federation.attributeHandles ||
      !federation.members.contains(receivingFederateId)) {
    return std::nullopt;
  }
  auto const knownClass = objectInstance.knownObjectClassHandlesByFederate.find(
      receivingFederateId);
  if (knownClass == objectInstance.knownObjectClassHandlesByFederate.end()) {
    return std::nullopt;
  }
  auto const declarations = federation.objectClassAttributeDeclarations.find(
      receivingFederateId);
  if (declarations == federation.objectClassAttributeDeclarations.end()) {
    return std::nullopt;
  }

  // This subscription-state query intentionally considers retained active and
  // passive declarations. The closest subscribed class wins, matching the
  // development profile's preferred-designator rule. Regional declarations at
  // that class are considered only when their committed subscription regions
  // overlap the instance's update-region association.
  std::set<std::uint64_t> visited;
  std::uint64_t currentClassHandle = knownClass->second;
  while (visited.insert(currentClassHandle).second) {
    auto const currentClassName = federation.objectClassHandles->nameFor(currentClassHandle);
    if (!currentClassName) {
      return std::nullopt;
    }
    auto const* currentClass = federation.definition.catalog->objectClass(*currentClassName);
    if (currentClass == nullptr) {
      return std::nullopt;
    }
    if (!federation.attributeHandles->nameFor(
            federation.definition.catalog.get(),
            *currentClassName,
            attributeHandle)) {
      if (currentClass->parentName.empty()) {
        break;
      }
      auto const parentHandle = federation.objectClassHandles->handleFor(
          currentClass->parentName);
      if (!parentHandle) {
        break;
      }
      currentClassHandle = *parentHandle;
      continue;
    }

    auto const perClass = declarations->second.byObjectClass.find(currentClassHandle);
    if (perClass != declarations->second.byObjectClass.end()) {
      auto const regional = perClass->second.regionalSubscribedAttributes.find(attributeHandle);
      auto const associated = objectInstance.updateRegionsByAttribute.find(attributeHandle);
      bool const hasExplicitSubscriptionRegion =
          regional != perClass->second.regionalSubscribedAttributes.end() &&
          !regional->second.empty();
      bool const hasExplicitUpdateRegion =
          associated != objectInstance.updateRegionsByAttribute.end() &&
          !associated->second.empty();

      if (perClass->second.subscribedAttributes.contains(attributeHandle) &&
          !hasExplicitSubscriptionRegion) {
        auto const designator = perClass->second.subscribedUpdateRateDesignators.find(
            attributeHandle);
        if (designator == perClass->second.subscribedUpdateRateDesignators.end() ||
            designator->second.empty()) {
          return std::nullopt;
        }
        return designator->second;
      }

      if (hasExplicitSubscriptionRegion) {
        bool matchedRegion = false;
        std::optional<std::string> explicitDesignator;
        auto const regionalDesignators =
            perClass->second.regionalSubscribedUpdateRateDesignators.find(attributeHandle);
        for (auto const& [subscribedRegionHandle, active] : regional->second) {
          static_cast<void>(active);
          bool overlaps = false;
          if (!hasExplicitUpdateRegion) {
            overlaps = regionOverlapsDefault(federation, subscribedRegionHandle);
          } else {
            for (std::uint64_t const associatedRegionHandle : associated->second) {
              if (!regionsOverlap(federation, subscribedRegionHandle, associatedRegionHandle)) {
                continue;
              }
              overlaps = true;
              break;
            }
          }
          if (!overlaps) {
            continue;
          }
          matchedRegion = true;
          if (regionalDesignators !=
                  perClass->second.regionalSubscribedUpdateRateDesignators.end()) {
            auto const designator = regionalDesignators->second.find(subscribedRegionHandle);
            if (designator != regionalDesignators->second.end() &&
                !designator->second.empty() && !explicitDesignator) {
              explicitDesignator = designator->second;
            }
          }
        }
        if (matchedRegion) {
          return explicitDesignator;
        }
      }
    }

    if (currentClass->parentName.empty()) {
      break;
    }
    auto const parentHandle = federation.objectClassHandles->handleFor(
        currentClass->parentName);
    if (!parentHandle) {
      break;
    }
    currentClassHandle = *parentHandle;
  }
  return std::nullopt;
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

std::vector<AttributeRelevanceAdvisoryRecipient>
EmbeddedFederationRegistry::attributeRelevanceAdvisoriesForScopeChanges(
    Federation const& federation,
    std::vector<ObjectInstanceScopeChangeRecipient> const& scopeChanges) {
  std::vector<AttributeRelevanceAdvisoryRecipient> result;
  std::map<
      std::tuple<
          std::uint64_t,
          std::uint64_t,
          std::uint64_t,
          bool,
          std::optional<std::string>>,
      std::size_t> recipientPositions;
  for (auto const& change : scopeChanges) {
    auto const instance = federation.objectInstances.find(change.objectInstanceHandle);
    if (instance == federation.objectInstances.end() || instance->second.deleteAccepted) {
      continue;
    }
    for (std::uint64_t const attributeHandle : change.attributeHandles) {
      auto const owner = instance->second.attributeOwnersByHandle.find(attributeHandle);
      if (owner == instance->second.attributeOwnersByHandle.end() ||
          owner->second == 0 || owner->second == change.receivingFederateId ||
          !federation.members.contains(owner->second)) {
        continue;
      }
      auto const callbackRoute = federation.interactionCallbackRoutes.find(owner->second);
      if (callbackRoute == federation.interactionCallbackRoutes.end() ||
          !callbackRoute->second) {
        continue;
      }

      auto const updateRateDesignator = change.inScope
          ? subscribedUpdateRateDesignatorForAttribute(
                federation,
                instance->second,
                change.receivingFederateId,
                attributeHandle)
          : std::nullopt;
      auto const key = std::make_tuple(
          owner->second,
          change.receivingFederateId,
          change.objectInstanceHandle,
          change.inScope,
          updateRateDesignator);
      auto position = recipientPositions.find(key);
      if (position == recipientPositions.end()) {
        std::size_t const index = result.size();
        result.push_back({
            owner->second,
            change.receivingFederateId,
            change.objectInstanceHandle,
            {},
            change.inScope,
            updateRateDesignator,
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

void EmbeddedFederationRegistry::reclaimTsoMessagePayloads(Federation& federation) {
  for (auto const& [messageId, record] : federation.tsoRequestRetractionRecords) {
    static_cast<void>(record);
    reclaimTsoMessagePayload(federation, messageId);
  }
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
  recipient.subscriptionGeneration = declarations->second.subscriptionGeneration;
  recipient.conveyRegionDesignatorSets =
      federation.members.at(receivingFederateId).conveyRegionDesignatorSetsSwitch;
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
    auto const ordinarySubscription =
        perClass->second.subscribedAttributes.find(attributeHandle);
    auto const regional = perClass->second.regionalSubscribedAttributes.find(attributeHandle);
    bool const hasExplicitSubscriptionRegion =
        regional != perClass->second.regionalSubscribedAttributes.end() &&
        !regional->second.empty();
    bool subscribed = ordinarySubscription != perClass->second.subscribedAttributes.end() &&
        ordinarySubscription->second && !hasExplicitSubscriptionRegion;
    if (!subscribed && hasExplicitSubscriptionRegion) {
      if (sentRegionHandles == nullptr || sentRegionHandles->empty()) {
        for (auto const& [subscribedRegionHandle, active] : regional->second) {
          if (active && regionOverlapsDefault(federation, subscribedRegionHandle)) {
            subscribed = true;
            break;
          }
        }
      } else {
        auto const associated = instance->second.updateRegionsByAttribute.find(attributeHandle);
        if (associated != instance->second.updateRegionsByAttribute.end()) {
          std::set<std::uint64_t> currentSentRegions;
          for (std::uint64_t const sentRegionHandle : *sentRegionHandles) {
            if (associated->second.contains(sentRegionHandle)) {
              currentSentRegions.insert(sentRegionHandle);
            }
          }
          for (auto const& [subscribedRegionHandle, active] : regional->second) {
            if (!active) {
              continue;
            }
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
    }
    if (subscribed) {
      recipient.receivedAttributeHandles.insert(attributeHandle);
      std::string designator = "HLAdefault";
      if (ordinarySubscription != perClass->second.subscribedAttributes.end()) {
        auto const rate = perClass->second.subscribedUpdateRateDesignators.find(attributeHandle);
        if (rate != perClass->second.subscribedUpdateRateDesignators.end()) {
          designator = rate->second;
        }
      }
      if (auto const rate = updateRateValueForNormalizedDesignator(
              *federation.definition.catalog, designator)) {
        recipient.maximumUpdateRatesByAttribute[attributeHandle] = *rate;
        recipient.maximumUpdateRate =
            recipient.maximumUpdateRate == 0.0
                ? *rate
                : std::min(recipient.maximumUpdateRate, *rate);
      }
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
  auto const reportRoute = federation.serviceReportRoutes.find(providingFederateId);
  recipient.serviceReportRoute =
      reportRoute == federation.serviceReportRoutes.end()
      ? FederateServiceReportRoute{}
      : reportRoute->second;
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
  auto const reportRoute = federation.serviceReportRoutes.find(providingFederateId);
  recipient.serviceReportRoute =
      reportRoute == federation.serviceReportRoutes.end()
      ? FederateServiceReportRoute{}
      : reportRoute->second;
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

  // The candidate received class is the sent class when it is actively
  // subscribed, or the closest actively subscribed superclass. Traversing the
  // parent chain rather than iterating subscriptions also guarantees at most
  // one callback for a recipient with subscriptions at multiple hierarchy locations.
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
    bool const subscribedWithoutRegion =
        ordinarySubscription != declarations->second.subscribedInteractionClasses.end() &&
        ordinarySubscription->second && !hasExplicitSubscriptionRegion;
    bool subscribedWithRegion = false;
    if (!subscribedWithoutRegion && hasExplicitSubscriptionRegion) {
      if (sentRegionHandles == nullptr) {
        for (auto const& [subscribedRegionHandle, active] : regional->second) {
          if (active && regionOverlapsDefault(
                            federation, subscribedRegionHandle, regionOverrides)) {
            subscribedWithRegion = true;
            break;
          }
        }
      } else if (!sentRegionHandles->empty()) {
        for (auto const& [subscribedRegionHandle, active] : regional->second) {
          if (!active) {
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
    if (objectClass->directedInteraction(*interactionClassName) != nullptr) {
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
    if (publication) {
      auto const declaration = declarations->second.publishedObjectClassDirectedInteractions.find(
          *currentClassHandle);
      if (declaration != declarations->second.publishedObjectClassDirectedInteractions.end() &&
          declaration->second.contains(interactionClassHandle)) {
        return true;
      }
    } else {
      auto const declaration = declarations->second.subscribedObjectClassDirectedInteractions.find(
          *currentClassHandle);
      if (declaration != declarations->second.subscribedObjectClassDirectedInteractions.end() &&
          declaration->second.contains(interactionClassHandle)) {
        return true;
      }
    }
    auto const* objectClass = federation.definition.catalog->objectClass(currentClassName);
    if (objectClass == nullptr) {
      return false;
    }
    currentClassName = objectClass->parentName;
  }
  return false;
}

std::optional<bool> EmbeddedFederationRegistry::directedInteractionSubscriptionIsUniversal(
    Federation const& federation,
    std::uint64_t federateId,
    std::uint64_t registeredObjectClassHandle,
    std::uint64_t interactionClassHandle) {
  if (!federation.objectClassHandles || !federation.definition.catalog) {
    return std::nullopt;
  }
  auto const declarations = federation.interactionDeclarations.find(federateId);
  if (declarations == federation.interactionDeclarations.end()) {
    return std::nullopt;
  }

  auto const registeredClassName =
      federation.objectClassHandles->nameFor(registeredObjectClassHandle);
  if (!registeredClassName) {
    return std::nullopt;
  }
  std::set<std::string> visited;
  std::string currentClassName = *registeredClassName;
  while (!currentClassName.empty() && visited.insert(currentClassName).second) {
    auto const currentClassHandle = federation.objectClassHandles->handleFor(currentClassName);
    if (!currentClassHandle) {
      return std::nullopt;
    }
    auto const perObjectClass =
        declarations->second.subscribedObjectClassDirectedInteractions.find(*currentClassHandle);
    if (perObjectClass != declarations->second.subscribedObjectClassDirectedInteractions.end()) {
      auto const subscription = perObjectClass->second.find(interactionClassHandle);
      if (subscription != perObjectClass->second.end()) {
        return subscription->second;
      }
    }
    auto const* objectClass = federation.definition.catalog->objectClass(currentClassName);
    if (objectClass == nullptr) {
      return std::nullopt;
    }
    currentClassName = objectClass->parentName;
  }
  return std::nullopt;
}

std::optional<ReceiveOrderDirectedInteractionRecipient>
EmbeddedFederationRegistry::candidateReceiveOrderDirectedInteractionRecipient(
    Federation const& federation,
    std::uint64_t producingFederateId,
    std::uint64_t receivingFederateId,
    std::uint64_t objectInstanceHandle,
    std::uint64_t sentInteractionClassHandle,
    std::vector<std::uint64_t> const& sentParameterHandles,
    bool allowMissingProducingFederate) {
  bool const producingFederateIsJoined = federation.members.contains(producingFederateId);
  bool const retainLostProducerAcceptance =
      allowMissingProducingFederate && !producingFederateIsJoined;
  if (producingFederateId == receivingFederateId ||
      !federation.members.contains(receivingFederateId) ||
      (!producingFederateIsJoined && !retainLostProducerAcceptance) ||
      !federation.definition.catalog || !federation.objectClassHandles ||
      !federation.interactionClassHandles || !federation.parameterHandles) {
    return std::nullopt;
  }

  auto const instance = federation.objectInstances.find(objectInstanceHandle);
  // The normal receive-order predicate suppresses a directed interaction
  // whose target has been deleted. For the narrow connection-loss exception,
  // however, a marked at-or-before-cutoff timestamped interaction must retain
  // its accepted target through its recipient's TSO boundary. The caller can
  // set allowMissingProducingFederate only after it has revalidated that exact
  // marked message/recipient pair, so this does not relax ordinary deletion
  // or source-membership semantics.
  if (instance == federation.objectInstances.end() ||
      (!retainLostProducerAcceptance && instance->second.deleteAccepted) ||
      !instance->second.knownObjectClassHandlesByFederate.contains(receivingFederateId) ||
      (!retainLostProducerAcceptance &&
       !instance->second.knownObjectClassHandlesByFederate.contains(producingFederateId))) {
    return std::nullopt;
  }
  if (!validDirectedInteractionForObjectClass(
          federation,
          instance->second.registeredObjectClassHandle,
          sentInteractionClassHandle) ||
      (!retainLostProducerAcceptance && !directedInteractionDeclarationApplies(
          federation,
          producingFederateId,
          instance->second.registeredObjectClassHandle,
          sentInteractionClassHandle,
          true))) {
    return std::nullopt;
  }
  auto const subscriptionIsUniversal = directedInteractionSubscriptionIsUniversal(
      federation,
      receivingFederateId,
      instance->second.registeredObjectClassHandle,
      sentInteractionClassHandle);
  if (!subscriptionIsUniversal) {
    return std::nullopt;
  }
  bool const ownsTargetAttribute = std::any_of(
      instance->second.attributeOwnersByHandle.begin(),
      instance->second.attributeOwnersByHandle.end(),
      [receivingFederateId](auto const& owner) {
        return owner.second == receivingFederateId;
      });
  if (!*subscriptionIsUniversal && !ownsTargetAttribute) {
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
    auto& revisedClasses = revised[objectClassHandle];
    for (std::uint64_t const interactionClassHandle : interactionClassHandles) {
      revisedClasses.insert_or_assign(interactionClassHandle, universally);
    }
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
  auto const member = federation->second.members.find(federateId);
  if (active && member->second.serviceReportingSwitch &&
      isReportServiceInvocationInteractionClass(federation->second, interactionClassHandle)) {
    return InteractionClassDeclarationStatus::
        federate_service_invocations_are_being_reported_via_mom;
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
  auto const member = federation->second.members.find(federateId);
  if (member->second.serviceReportingSwitch &&
      isReportServiceInvocationInteractionClass(federation->second, interactionClassHandle)) {
    return RegionalInteractionClassDeclarationStatus::
        federate_service_invocations_are_being_reported_via_mom;
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

std::vector<DeclarationAdvisory> EmbeddedFederationRegistry::planDeclarationAdvisories(
    std::wstring const& federationName) {
  auto instrumentationScope = beginInstrumentation("planDeclarationAdvisories");
  std::scoped_lock lock(mutex_);
  auto federation = federations_.find(federationName);
  if (federation == federations_.end()) {
    return {};
  }

  auto& execution = federation->second;

  // A subscription at a class applies to instances/interactions declared at
  // that class and below.  Keep the hierarchy walk local to this planner so
  // it cannot accidentally consult a different federation's handle space.
  auto objectClassIsSameOrAncestor = [&](std::uint64_t ancestorHandle,
                                         std::uint64_t descendantHandle) {
    if (!execution.definition.catalog || !execution.objectClassHandles) {
      return false;
    }
    auto const ancestorName = execution.objectClassHandles->nameFor(ancestorHandle);
    auto const descendantName = execution.objectClassHandles->nameFor(descendantHandle);
    if (!ancestorName || !descendantName) {
      return false;
    }

    std::set<std::string> visited;
    std::string current = *descendantName;
    while (!current.empty() && visited.insert(current).second) {
      if (current == *ancestorName) {
        return true;
      }
      auto const* definition = execution.definition.catalog->objectClass(current);
      if (definition == nullptr) {
        return false;
      }
      current = definition->parentName;
    }
    return false;
  };

  auto interactionClassIsSameOrAncestor = [&](std::uint64_t ancestorHandle,
                                               std::uint64_t descendantHandle) {
    if (!execution.definition.catalog || !execution.interactionClassHandles) {
      return false;
    }
    auto const ancestorName =
        execution.interactionClassHandles->nameFor(ancestorHandle);
    auto const descendantName =
        execution.interactionClassHandles->nameFor(descendantHandle);
    if (!ancestorName || !descendantName) {
      return false;
    }

    std::set<std::string> visited;
    std::string current = *descendantName;
    while (!current.empty() && visited.insert(current).second) {
      if (current == *ancestorName) {
        return true;
      }
      auto const* definition = execution.definition.catalog->interactionClass(current);
      if (definition == nullptr) {
        return false;
      }
      current = definition->parentName;
    }
    return false;
  };

  std::set<std::pair<std::uint64_t, std::uint64_t>> currentObjectRelevance;
  for (auto const& [publishingFederateId, declarations] :
       execution.objectClassAttributeDeclarations) {
    if (!execution.members.contains(publishingFederateId)) {
      continue;
    }
    for (auto const& [publishedClassHandle, perClass] : declarations.byObjectClass) {
      static_cast<void>(perClass);
      auto const publishedAttributes = publishedObjectClassAttributes(
          execution,
          publishingFederateId,
          publishedClassHandle);
      if (!publishedAttributes || publishedAttributes->empty()) {
        continue;
      }

      bool relevant = false;
      for (auto const& [receivingFederateId, subscriptions] :
           execution.objectClassAttributeDeclarations) {
        if (receivingFederateId == publishingFederateId ||
            !execution.members.contains(receivingFederateId)) {
          continue;
        }
        for (auto const& [subscriptionClassHandle, subscribedClass] :
             subscriptions.byObjectClass) {
          if (!objectClassIsSameOrAncestor(
                  subscriptionClassHandle,
                  publishedClassHandle)) {
            continue;
          }
          for (auto const& [attributeHandle, active] :
               subscribedClass.subscribedAttributes) {
            if (active && publishedAttributes->contains(attributeHandle)) {
              relevant = true;
              break;
            }
          }
          if (relevant) {
            break;
          }
        }
        if (relevant) {
          break;
        }
      }
      if (relevant) {
        currentObjectRelevance.emplace(publishingFederateId, publishedClassHandle);
      }
    }
  }

  std::set<std::pair<std::uint64_t, std::uint64_t>> currentInteractionRelevance;
  for (auto const& [publishingFederateId, declarations] :
       execution.interactionDeclarations) {
    if (!execution.members.contains(publishingFederateId)) {
      continue;
    }
    for (std::uint64_t const publishedClassHandle : declarations.publishedInteractionClasses) {
      bool relevant = false;
      for (auto const& [receivingFederateId, subscriptions] :
           execution.interactionDeclarations) {
        if (receivingFederateId == publishingFederateId ||
            !execution.members.contains(receivingFederateId)) {
          continue;
        }
        for (auto const& [subscriptionClassHandle, active] :
             subscriptions.subscribedInteractionClasses) {
          if (active && interactionClassIsSameOrAncestor(
                            subscriptionClassHandle,
                            publishedClassHandle)) {
            relevant = true;
            break;
          }
        }
        if (relevant) {
          break;
        }
      }
      if (relevant) {
        currentInteractionRelevance.emplace(publishingFederateId, publishedClassHandle);
      }
    }
  }

  std::vector<DeclarationAdvisory> advisories;
  auto appendAdvisory = [&](DeclarationAdvisoryKind kind,
                            std::uint64_t receivingFederateId,
                            std::uint64_t classHandle,
                            bool enabled) {
    if (!enabled) {
      return;
    }
    auto const route = execution.interactionCallbackRoutes.find(receivingFederateId);
    if (route == execution.interactionCallbackRoutes.end() || !route->second) {
      return;
    }
    auto const reportRoute = execution.serviceReportRoutes.find(receivingFederateId);
    advisories.push_back({
        kind,
        receivingFederateId,
        classHandle,
        route->second,
        reportRoute == execution.serviceReportRoutes.end()
            ? FederateServiceReportRoute{}
            : reportRoute->second,
    });
  };

  for (auto const& relevance : currentObjectRelevance) {
    if (execution.objectClassRegistrationRelevance.contains(relevance)) {
      continue;
    }
    auto const member = execution.members.find(relevance.first);
    if (member == execution.members.end()) {
      continue;
    }
    appendAdvisory(
        DeclarationAdvisoryKind::start_registration_for_object_class,
        relevance.first,
        relevance.second,
        member->second.objectClassRelevanceAdvisorySwitch);
  }
  for (auto const& relevance : execution.objectClassRegistrationRelevance) {
    if (currentObjectRelevance.contains(relevance)) {
      continue;
    }
    auto const member = execution.members.find(relevance.first);
    auto const publishedAttributes = publishedObjectClassAttributes(
        execution,
        relevance.first,
        relevance.second);
    if (member == execution.members.end() ||
        !publishedAttributes || publishedAttributes->empty()) {
      continue;
    }
    appendAdvisory(
        DeclarationAdvisoryKind::stop_registration_for_object_class,
        relevance.first,
        relevance.second,
        member->second.objectClassRelevanceAdvisorySwitch);
  }

  for (auto const& relevance : currentInteractionRelevance) {
    if (execution.interactionRelevance.contains(relevance)) {
      continue;
    }
    auto const member = execution.members.find(relevance.first);
    if (member == execution.members.end()) {
      continue;
    }
    appendAdvisory(
        DeclarationAdvisoryKind::turn_interactions_on,
        relevance.first,
        relevance.second,
        member->second.interactionRelevanceAdvisorySwitch);
  }
  for (auto const& relevance : execution.interactionRelevance) {
    if (currentInteractionRelevance.contains(relevance)) {
      continue;
    }
    auto const member = execution.members.find(relevance.first);
    if (member == execution.members.end()) {
      continue;
    }
    auto const declarations = execution.interactionDeclarations.find(relevance.first);
    if (declarations == execution.interactionDeclarations.end() ||
        !declarations->second.publishedInteractionClasses.contains(relevance.second)) {
      continue;
    }
    appendAdvisory(
        DeclarationAdvisoryKind::turn_interactions_off,
        relevance.first,
        relevance.second,
        member->second.interactionRelevanceAdvisorySwitch);
  }

  execution.objectClassRegistrationRelevance = std::move(currentObjectRelevance);
  execution.interactionRelevance = std::move(currentInteractionRelevance);
  return advisories;
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

    // IEEE 1516.1-2025 5.3.3 requires an unpublishing federate to lose
    // ownership of every corresponding instance attribute.  Keep that
    // transition federation-wide, including instances registered at a
    // subclass, so a later update cannot use a stale ownership record after
    // the publication boundary has been removed.
    for (auto& [objectInstanceHandle, objectInstance] :
         federation->second.objectInstances) {
      static_cast<void>(objectInstanceHandle);
      if (!objectInstanceRegisteredAtOrBelowClass(
              federation->second,
              objectInstance.registeredObjectClassHandle,
              objectClassHandle)) {
        continue;
      }
      for (std::uint64_t const attributeHandle : attributeHandles) {
        auto assumptionSearch =
            objectInstance.ownershipAssumptionRecipientsByAttribute.find(attributeHandle);
        if (assumptionSearch !=
            objectInstance.ownershipAssumptionRecipientsByAttribute.end()) {
          assumptionSearch->second.erase(federateId);
        }
        auto const owner = objectInstance.attributeOwnersByHandle.find(attributeHandle);
        if (owner != objectInstance.attributeOwnersByHandle.end() &&
            owner->second == federateId) {
          objectInstance.attributeOwnersByHandle.erase(owner);
          clearUpdateRegionAssociation(objectInstance, attributeHandle);
          objectInstance.attributeTransportationTypes.erase(attributeHandle);
          objectInstance.attributeOrderTypes.erase(attributeHandle);
          for (auto pending = objectInstance.pendingAttributeTransportationTypeChanges.begin();
               pending != objectInstance.pendingAttributeTransportationTypeChanges.end();) {
            pending->second.attributeHandles.erase(attributeHandle);
            if (pending->second.attributeHandles.empty()) {
              pending = objectInstance.pendingAttributeTransportationTypeChanges.erase(pending);
            } else {
              ++pending;
            }
          }
        }
      }
    }

    if (perClass->second.explicitlyPublishedAttributes.empty() &&
        perClass->second.subscribedAttributes.empty() &&
        perClass->second.subscribedUpdateRateDesignators.empty() &&
        perClass->second.regionalSubscribedAttributes.empty() &&
        perClass->second.regionalSubscribedUpdateRateDesignators.empty()) {
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
        perClass->second.subscribedUpdateRateDesignators.empty() &&
        perClass->second.regionalSubscribedAttributes.empty() &&
        perClass->second.regionalSubscribedUpdateRateDesignators.empty()) {
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
    std::optional<bool> active,
    std::string const& updateRateDesignator) {
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

  std::optional<std::string> normalizedDesignator;
  if (active) {
    if (!federation->second.definition.catalog) {
      return {ObjectClassAttributeDeclarationStatus::inconsistent_catalog};
    }
    normalizedDesignator = normalizedUpdateRateDesignator(
        *federation->second.definition.catalog,
        updateRateDesignator);
    if (!normalizedDesignator) {
      return {ObjectClassAttributeDeclarationStatus::invalid_update_rate_designator};
    }
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
      perClass->second.subscribedUpdateRateDesignators.erase(attributeHandle);
    }
    declarations->second.subscriptionGeneration =
        federation->second.nextSubscriptionGeneration++;
    if (perClass->second.explicitlyPublishedAttributes.empty() &&
        perClass->second.subscribedAttributes.empty() &&
        perClass->second.subscribedUpdateRateDesignators.empty() &&
        perClass->second.regionalSubscribedAttributes.empty() &&
        perClass->second.regionalSubscribedUpdateRateDesignators.empty()) {
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
    result.attributeRelevanceAdvisories = attributeRelevanceAdvisoriesForScopeChanges(
        federation->second,
        result.recipients);
    return result;
  }

  auto [state, insertedState] =
      federation->second.objectClassAttributeDeclarations.try_emplace(federateId);
  auto [perClass, insertedClass] = state->second.byObjectClass.try_emplace(objectClassHandle);
  try {
    auto revised = perClass->second.subscribedAttributes;
    auto revisedDesignators = perClass->second.subscribedUpdateRateDesignators;
    auto const storedDesignator = storedUpdateRateDesignator(
        updateRateDesignator,
        *normalizedDesignator);
    for (std::uint64_t const attributeHandle : attributeHandles) {
      revised.insert_or_assign(attributeHandle, *active);
      revisedDesignators.insert_or_assign(attributeHandle, storedDesignator);
    }
    perClass->second.subscribedAttributes.swap(revised);
    perClass->second.subscribedUpdateRateDesignators.swap(revisedDesignators);
    state->second.subscriptionGeneration = federation->second.nextSubscriptionGeneration++;
  } catch (...) {
    if (insertedClass && perClass->second.explicitlyPublishedAttributes.empty() &&
        perClass->second.subscribedAttributes.empty() &&
        perClass->second.subscribedUpdateRateDesignators.empty() &&
        perClass->second.regionalSubscribedAttributes.empty() &&
        perClass->second.regionalSubscribedUpdateRateDesignators.empty()) {
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
  result.attributeRelevanceAdvisories = attributeRelevanceAdvisoriesForScopeChanges(
      federation->second,
      result.recipients);
  return result;
}

ObjectClassAttributeDeclarationStatus
EmbeddedFederationRegistry::setObjectClassAttributeSubscription(
    std::wstring const& federationName,
    std::uint64_t federateId,
    std::uint64_t objectClassHandle,
    std::set<std::uint64_t> const& attributeHandles,
    std::optional<bool> active,
    std::string const& updateRateDesignator) {
  return setObjectClassAttributeSubscriptionWithScopeChanges(
             federationName,
             federateId,
             objectClassHandle,
             attributeHandles,
             active,
             updateRateDesignator)
      .status;
}

RegionalObjectClassAttributeSubscriptionScopePlan
EmbeddedFederationRegistry::setObjectClassAttributeRegionalSubscriptionWithScopeChanges(
    std::wstring const& federationName,
    std::uint64_t federateId,
    std::uint64_t objectClassHandle,
    std::map<std::uint64_t, std::set<std::uint64_t>> const& attributesAndRegions,
    bool active,
    std::string const& updateRateDesignator) {
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

  if (!federation->second.definition.catalog) {
    return {RegionalObjectClassAttributeDeclarationStatus::inconsistent_catalog};
  }
  auto const normalizedDesignator = normalizedUpdateRateDesignator(
      *federation->second.definition.catalog,
      updateRateDesignator);
  if (!normalizedDesignator) {
    return {RegionalObjectClassAttributeDeclarationStatus::invalid_update_rate_designator};
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
    auto revisedDesignators = perClass->second.regionalSubscribedUpdateRateDesignators;
    auto const storedDesignator = storedUpdateRateDesignator(
        updateRateDesignator,
        *normalizedDesignator);
    for (auto const& [attributeHandle, regionHandles] : attributesAndRegions) {
      auto& subscriptions = revised[attributeHandle];
      auto& designators = revisedDesignators[attributeHandle];
      for (std::uint64_t const regionHandle : regionHandles) {
        subscriptions.insert_or_assign(regionHandle, active);
        designators.insert_or_assign(regionHandle, storedDesignator);
      }
      if (subscriptions.empty()) {
        revised.erase(attributeHandle);
        revisedDesignators.erase(attributeHandle);
      }
    }
    perClass->second.regionalSubscribedAttributes.swap(revised);
    perClass->second.regionalSubscribedUpdateRateDesignators.swap(revisedDesignators);
    state->second.subscriptionGeneration = federation->second.nextSubscriptionGeneration++;
    refreshRegionUsage(federation->second);
  } catch (...) {
    if (insertedClass && perClass->second.explicitlyPublishedAttributes.empty() &&
        perClass->second.subscribedAttributes.empty() &&
        perClass->second.subscribedUpdateRateDesignators.empty() &&
        perClass->second.regionalSubscribedAttributes.empty() &&
        perClass->second.regionalSubscribedUpdateRateDesignators.empty()) {
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
  result.attributeRelevanceAdvisories = attributeRelevanceAdvisoriesForScopeChanges(
      federation->second,
      result.recipients);
  return result;
}

RegionalObjectClassAttributeDeclarationStatus
EmbeddedFederationRegistry::setObjectClassAttributeRegionalSubscription(
    std::wstring const& federationName,
    std::uint64_t federateId,
    std::uint64_t objectClassHandle,
    std::map<std::uint64_t, std::set<std::uint64_t>> const& attributesAndRegions,
    bool active,
    std::string const& updateRateDesignator) {
  return setObjectClassAttributeRegionalSubscriptionWithScopeChanges(
             federationName,
             federateId,
             objectClassHandle,
             attributesAndRegions,
             active,
             updateRateDesignator)
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
          auto regionalDesignators =
              perClass->second.regionalSubscribedUpdateRateDesignators.find(attributeHandle);
          if (regionalDesignators !=
              perClass->second.regionalSubscribedUpdateRateDesignators.end()) {
            regionalDesignators->second.erase(regionHandle);
            if (regionalDesignators->second.empty()) {
              perClass->second.regionalSubscribedUpdateRateDesignators.erase(
                  regionalDesignators);
            }
          }
        }
        if (regional->second.empty()) {
          perClass->second.regionalSubscribedAttributes.erase(regional);
        }
      }
      if (perClass->second.explicitlyPublishedAttributes.empty() &&
          perClass->second.subscribedAttributes.empty() &&
          perClass->second.subscribedUpdateRateDesignators.empty() &&
          perClass->second.regionalSubscribedAttributes.empty() &&
          perClass->second.regionalSubscribedUpdateRateDesignators.empty()) {
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
  result.attributeRelevanceAdvisories = attributeRelevanceAdvisoriesForScopeChanges(
      federation->second,
      result.recipients);
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
      perClass->second.subscribedUpdateRateDesignators,
  };
}

std::optional<std::set<std::uint64_t>>
EmbeddedFederationRegistry::publishedObjectClassAttributeHandles(
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
  return publishedObjectClassAttributes(
      federation->second,
      federateId,
      objectClassHandle);
}

ObjectInstanceNameReservationResult
EmbeddedFederationRegistry::reserveObjectInstanceName(
    std::wstring const& federationName,
    std::uint64_t federateId,
    std::wstring const& objectInstanceName) {
  auto instrumentationScope = beginInstrumentation("reserveObjectInstanceName");
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
  auto instrumentationScope = beginInstrumentation("releaseObjectInstanceName");
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
  auto instrumentationScope = beginInstrumentation("registerObjectInstance");
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
          !federation->second.rtiOwnedJoinedFederateMomObjects.contains(objectInstanceHandle) &&
          !federation->second.objectInstanceHandlesByName.contains(objectInstanceName) &&
          !federation->second.reservedObjectInstanceNamesByFederate.contains(objectInstanceName)) {
        break;
      }
      ++objectInstanceHandle;
    } while (true);
  } else {
    while (federation->second.objectInstances.contains(objectInstanceHandle) ||
           federation->second.rtiOwnedJoinedFederateMomObjects.contains(objectInstanceHandle)) {
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
    auto const transportationName = attributeDefaultTransportationName(
        federation->second,
        federateId,
        objectClassHandle,
        attributeHandle);
    if (!transportationName) {
      return {ObjectInstanceRegistrationStatus::inconsistent_catalog};
    }
    objectInstance.attributeTransportationTypes.emplace(
        attributeHandle,
        *transportationName);
    auto const orderType = attributeDefaultOrderType(
        federation->second,
        federateId,
        objectClassHandle,
        attributeHandle);
    if (!orderType) {
      return {ObjectInstanceRegistrationStatus::inconsistent_catalog};
    }
    objectInstance.attributeOrderTypes.emplace(attributeHandle, *orderType);
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
  result.attributeRelevanceAdvisories = attributeRelevanceAdvisoriesForScopeChanges(
      federation->second,
      result.recipients);
  instance->second.updateRegionsByAttribute.swap(revisedAssociations);
  refreshRegionUsage(federation->second);
  return result;
}

ObjectInstanceRegionAssociationStatus EmbeddedFederationRegistry::associateRegionsForUpdates(
    std::wstring const& federationName,
    std::uint64_t federateId,
    std::uint64_t objectInstanceHandle,
    std::map<std::uint64_t, std::set<std::uint64_t>> const& attributesAndRegions) {
  auto instrumentationScope = beginInstrumentation("associateRegionsForUpdates");
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
  result.attributeRelevanceAdvisories = attributeRelevanceAdvisoriesForScopeChanges(
      federation->second,
      result.recipients);
  instance->second.updateRegionsByAttribute.swap(revisedAssociations);
  refreshRegionUsage(federation->second);
  return result;
}

ObjectInstanceRegionAssociationStatus EmbeddedFederationRegistry::unassociateRegionsForUpdates(
    std::wstring const& federationName,
    std::uint64_t federateId,
    std::uint64_t objectInstanceHandle,
    std::map<std::uint64_t, std::set<std::uint64_t>> const& attributesAndRegions) {
  auto instrumentationScope = beginInstrumentation("unassociateRegionsForUpdates");
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
  auto instrumentationScope = beginInstrumentation("deleteObjectInstance");
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
    auto const reportRoute = federation->second.serviceReportRoutes.find(receivingFederateId);
    result.recipients.push_back({
        receivingFederateId,
        instance->second.handle,
        callbackRoute->second,
        reportRoute == federation->second.serviceReportRoutes.end()
            ? FederateServiceReportRoute{}
            : reportRoute->second,
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
  auto instrumentationScope = beginInstrumentation("localDeleteObjectInstance");
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
      auto const reportRoute = federation->second.serviceReportRoutes.find(receivingFederateId);

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
            reportRoute == federation->second.serviceReportRoutes.end()
                ? FederateServiceReportRoute{}
                : reportRoute->second,
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
  auto const reportRoute = federation->second.serviceReportRoutes.find(receivingFederateId);

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
            reportRoute == federation->second.serviceReportRoutes.end()
                ? FederateServiceReportRoute{}
                : reportRoute->second,
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

std::vector<ObjectInstanceDiscoveryRecipient>
EmbeddedFederationRegistry::planJoinedFederateMomObjectDiscoveriesForInstance(
    std::wstring const& federationName,
    std::uint64_t objectInstanceHandle) {
  std::scoped_lock lock(mutex_);
  auto federation = federations_.find(federationName);
  if (federation == federations_.end()) {
    return {};
  }
  auto object = federation->second.rtiOwnedJoinedFederateMomObjects.find(
      objectInstanceHandle);
  if (object == federation->second.rtiOwnedJoinedFederateMomObjects.end() ||
      !federation->second.members.contains(object->second.joinedFederateId)) {
    return {};
  }

  std::vector<ObjectInstanceDiscoveryRecipient> result;
  result.reserve(federation->second.members.size());
  try {
    for (auto const& [receivingFederateId, membership] : federation->second.members) {
      static_cast<void>(membership);
      if (object->second.knownFederateIds.contains(receivingFederateId) ||
          object->second.pendingDiscoveryFederateIds.contains(receivingFederateId) ||
          !candidateJoinedFederateMomObjectDiscoveryClass(
              federation->second,
              object->second,
              receivingFederateId)) {
        continue;
      }
      auto const callbackRoute = federation->second.interactionCallbackRoutes.find(
          receivingFederateId);
      if (callbackRoute == federation->second.interactionCallbackRoutes.end() ||
          !callbackRoute->second) {
        continue;
      }
      auto const [pending, insertedPending] =
          object->second.pendingDiscoveryFederateIds.insert(receivingFederateId);
      static_cast<void>(pending);
      if (!insertedPending) {
        continue;
      }
      try {
        result.push_back({
            receivingFederateId,
            object->second.objectInstanceHandle,
            object->second.objectClassHandle,
            L"HLAfederate-" + std::to_wstring(object->second.joinedFederateId),
            0U,
            callbackRoute->second,
            FederateServiceReportRoute{},
            true,
        });
      } catch (...) {
        object->second.pendingDiscoveryFederateIds.erase(receivingFederateId);
        throw;
      }
    }
  } catch (...) {
    for (auto const& recipient : result) {
      object->second.pendingDiscoveryFederateIds.erase(recipient.receivingFederateId);
    }
    throw;
  }
  return result;
}

std::vector<ObjectInstanceDiscoveryRecipient>
EmbeddedFederationRegistry::planJoinedFederateMomObjectDiscoveriesForFederate(
    std::wstring const& federationName,
    std::uint64_t receivingFederateId) {
  std::scoped_lock lock(mutex_);
  auto federation = federations_.find(federationName);
  if (federation == federations_.end() ||
      !federation->second.members.contains(receivingFederateId)) {
    return {};
  }
  auto const callbackRoute = federation->second.interactionCallbackRoutes.find(
      receivingFederateId);
  if (callbackRoute == federation->second.interactionCallbackRoutes.end() ||
      !callbackRoute->second) {
    return {};
  }

  std::vector<ObjectInstanceDiscoveryRecipient> result;
  result.reserve(federation->second.rtiOwnedJoinedFederateMomObjects.size());
  try {
    for (auto& [objectInstanceHandle, object] :
         federation->second.rtiOwnedJoinedFederateMomObjects) {
      static_cast<void>(objectInstanceHandle);
      if (object.knownFederateIds.contains(receivingFederateId) ||
          object.pendingDiscoveryFederateIds.contains(receivingFederateId) ||
          !federation->second.members.contains(object.joinedFederateId) ||
          !candidateJoinedFederateMomObjectDiscoveryClass(
              federation->second,
              object,
              receivingFederateId)) {
        continue;
      }
      auto const [pending, insertedPending] =
          object.pendingDiscoveryFederateIds.insert(receivingFederateId);
      static_cast<void>(pending);
      if (!insertedPending) {
        continue;
      }
      try {
        result.push_back({
            receivingFederateId,
            object.objectInstanceHandle,
            object.objectClassHandle,
            L"HLAfederate-" + std::to_wstring(object.joinedFederateId),
            0U,
            callbackRoute->second,
            FederateServiceReportRoute{},
            true,
        });
      } catch (...) {
        object.pendingDiscoveryFederateIds.erase(receivingFederateId);
        throw;
      }
    }
  } catch (...) {
    for (auto const& recipient : result) {
      auto const object = federation->second.rtiOwnedJoinedFederateMomObjects.find(
          recipient.objectInstanceHandle);
      if (object != federation->second.rtiOwnedJoinedFederateMomObjects.end()) {
        object->second.pendingDiscoveryFederateIds.erase(receivingFederateId);
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

void EmbeddedFederationRegistry::cancelJoinedFederateMomObjectDiscovery(
    std::wstring const& federationName,
    std::uint64_t receivingFederateId,
    std::uint64_t objectInstanceHandle) {
  std::scoped_lock lock(mutex_);
  auto federation = federations_.find(federationName);
  if (federation == federations_.end()) {
    return;
  }
  auto object = federation->second.rtiOwnedJoinedFederateMomObjects.find(
      objectInstanceHandle);
  if (object == federation->second.rtiOwnedJoinedFederateMomObjects.end()) {
    return;
  }
  object->second.pendingDiscoveryFederateIds.erase(receivingFederateId);
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
  auto const receivingMember = federation->second.members.find(receivingFederateId);
  if (receivingMember == federation->second.members.end() ||
      !receivingMember->second.attributeScopeAdvisorySwitch) {
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
  auto instrumentationScope = beginInstrumentation("beginObjectInstanceRemoval");
  std::scoped_lock lock(mutex_);
  auto federation = federations_.find(federationName);
  if (federation == federations_.end()) {
    return std::nullopt;
  }
  auto instance = federation->second.objectInstances.find(objectInstanceHandle);
  if (instance == federation->second.objectInstances.end() || !instance->second.deleteAccepted) {
    return std::nullopt;
  }
  if (!instance->second.pendingRemovalFederates.contains(receivingFederateId)) {
    return std::nullopt;
  }

  // A forced connection-loss resign may have queued an ordinary automatic
  // removal before this recipient later opens its TSO boundary. Do not let
  // that receive-order work erase the object's known state while a marked
  // at-or-before-cutoff update or directed interaction still requires it.
  // Keep the original removal reservation; the matching TSO completion will
  // replan this callback after its Time Advance Grant.
  if (instance->second.connectionLossAutomaticRemovalFederates.contains(
          receivingFederateId) &&
      hasPendingConnectionLossTsoObjectDelivery(
          federation->second,
          objectInstanceHandle,
          receivingFederateId)) {
    instance->second.deferredConnectionLossTsoRemovalFederates.insert(
        receivingFederateId);
    return std::nullopt;
  }

  instance->second.pendingRemovalFederates.erase(receivingFederateId);
  instance->second.connectionLossAutomaticRemovalFederates.erase(receivingFederateId);
  instance->second.deferredConnectionLossTsoRemovalFederates.erase(receivingFederateId);
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

std::optional<RemovedObjectInstanceSnapshot>
EmbeddedFederationRegistry::beginJoinedFederateMomObjectRemoval(
    std::wstring const& federationName,
    std::uint64_t receivingFederateId,
    std::uint64_t objectInstanceHandle) {
  auto instrumentationScope = beginInstrumentation(
      "beginJoinedFederateMomObjectRemoval");
  std::scoped_lock lock(mutex_);
  auto federation = federations_.find(federationName);
  if (federation == federations_.end()) {
    return std::nullopt;
  }
  auto object = federation->second.rtiOwnedJoinedFederateMomObjects.find(
      objectInstanceHandle);
  if (object == federation->second.rtiOwnedJoinedFederateMomObjects.end() ||
      !object->second.pendingRemovalFederateIds.contains(receivingFederateId)) {
    return std::nullopt;
  }
  object->second.pendingRemovalFederateIds.erase(receivingFederateId);
  if (!object->second.knownFederateIds.contains(receivingFederateId)) {
    return std::nullopt;
  }
  if (!federation->second.members.contains(receivingFederateId)) {
    object->second.knownFederateIds.erase(receivingFederateId);
    return std::nullopt;
  }
  object->second.knownFederateIds.erase(receivingFederateId);
  RemovedObjectInstanceSnapshot const result{object->second.objectInstanceHandle, 0U};
  if (object->second.knownFederateIds.empty() &&
      object->second.pendingRemovalFederateIds.empty()) {
    federation->second.rtiOwnedJoinedFederateMomObjects.erase(object);
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
  instance->second.connectionLossAutomaticRemovalFederates.erase(receivingFederateId);
  instance->second.deferredConnectionLossTsoRemovalFederates.erase(receivingFederateId);
  if (canPurgeDeletedObjectInstance(instance->second)) {
    federation->second.objectInstanceHandlesByName.erase(instance->second.name);
    federation->second.objectInstances.erase(instance);
  }
}

void EmbeddedFederationRegistry::cancelJoinedFederateMomObjectRemoval(
    std::wstring const& federationName,
    std::uint64_t receivingFederateId,
    std::uint64_t objectInstanceHandle) {
  std::scoped_lock lock(mutex_);
  auto federation = federations_.find(federationName);
  if (federation == federations_.end()) {
    return;
  }
  auto object = federation->second.rtiOwnedJoinedFederateMomObjects.find(
      objectInstanceHandle);
  if (object == federation->second.rtiOwnedJoinedFederateMomObjects.end()) {
    return;
  }
  object->second.pendingRemovalFederateIds.erase(receivingFederateId);
}

std::optional<KnownObjectInstanceSnapshot>
EmbeddedFederationRegistry::beginObjectInstanceDiscovery(
    std::wstring const& federationName,
    std::uint64_t receivingFederateId,
    std::uint64_t objectInstanceHandle) {
  auto instrumentationScope = beginInstrumentation("beginObjectInstanceDiscovery");
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
EmbeddedFederationRegistry::beginJoinedFederateMomObjectDiscovery(
    std::wstring const& federationName,
    std::uint64_t receivingFederateId,
    std::uint64_t objectInstanceHandle) {
  auto instrumentationScope = beginInstrumentation(
      "beginJoinedFederateMomObjectDiscovery");
  std::scoped_lock lock(mutex_);
  auto federation = federations_.find(federationName);
  if (federation == federations_.end()) {
    return std::nullopt;
  }
  auto object = federation->second.rtiOwnedJoinedFederateMomObjects.find(
      objectInstanceHandle);
  if (object == federation->second.rtiOwnedJoinedFederateMomObjects.end()) {
    return std::nullopt;
  }

  object->second.pendingDiscoveryFederateIds.erase(receivingFederateId);
  if (!federation->second.members.contains(receivingFederateId) ||
      object->second.knownFederateIds.contains(receivingFederateId)) {
    return std::nullopt;
  }
  auto const discoveredClass = candidateJoinedFederateMomObjectDiscoveryClass(
      federation->second,
      object->second,
      receivingFederateId);
  if (!discoveredClass) {
    return std::nullopt;
  }
  object->second.knownFederateIds.insert(receivingFederateId);
  std::set<std::uint64_t> initialAttributeHandles;
  for (auto const& [attributeHandle, value] : object->second.initialAttributeValues) {
    static_cast<void>(value);
    initialAttributeHandles.insert(attributeHandle);
  }
  return KnownObjectInstanceSnapshot{
      object->second.objectInstanceHandle,
      *discoveredClass,
      L"HLAfederate-" + std::to_wstring(object->second.joinedFederateId),
      0U,
      std::move(initialAttributeHandles),
  };
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

AttributeValueUpdateRequestPlan EmbeddedFederationRegistry::planAutoProvideForDiscovery(
    std::wstring const& federationName,
    std::uint64_t receivingFederateId,
    std::uint64_t objectInstanceHandle) const {
  std::scoped_lock lock(mutex_);
  auto const federation = federations_.find(federationName);
  if (federation == federations_.end()) {
    return {AttributeValueUpdateRequestStatus::federation_does_not_exist};
  }
  if (!federation->second.members.contains(receivingFederateId)) {
    return {AttributeValueUpdateRequestStatus::requesting_federate_not_member};
  }

  auto const instance = federation->second.objectInstances.find(objectInstanceHandle);
  if (instance == federation->second.objectInstances.end() ||
      instance->second.deleteAccepted ||
      !instance->second.knownObjectClassHandlesByFederate.contains(receivingFederateId)) {
    return {AttributeValueUpdateRequestStatus::object_instance_not_known};
  }
  // Auto Provide is a federation-wide dynamic switch. Disabled executions
  // still complete discovery normally but do not induce provider callbacks.
  if (!federation->second.autoProvideSwitch) {
    return {};
  }
  if (!federation->second.definition.catalog ||
      !federation->second.objectClassHandles ||
      !federation->second.attributeHandles) {
    return {AttributeValueUpdateRequestStatus::inconsistent_catalog};
  }

  auto const knownClassHandle =
      instance->second.knownObjectClassHandlesByFederate.at(receivingFederateId);
  auto const knownClassName = federation->second.objectClassHandles->nameFor(knownClassHandle);
  if (!knownClassName ||
      federation->second.definition.catalog->objectClass(*knownClassName) == nullptr) {
    return {AttributeValueUpdateRequestStatus::inconsistent_catalog};
  }

  // Group every currently owned, in-scope attribute by its provider. This is
  // deliberately independent of the Attribute Scope Advisory switch: Auto
  // Provide follows actual subscription scope, not whether an advisory was
  // requested for that transition.
  std::map<std::uint64_t, std::set<std::uint64_t>> attributesByProvider;
  for (auto const& [attributeHandle, owner] : instance->second.attributeOwnersByHandle) {
    if (owner == 0 || owner == receivingFederateId ||
        !federation->second.members.contains(owner) ||
        !federation->second.attributeHandles->nameFor(
            federation->second.definition.catalog.get(),
            *knownClassName,
            attributeHandle) ||
        !objectAttributeInScope(
            federation->second,
            instance->second,
            receivingFederateId,
            attributeHandle)) {
      continue;
    }
    attributesByProvider[owner].insert(attributeHandle);
  }

  AttributeValueUpdateRequestPlan result;
  result.recipients.reserve(attributesByProvider.size());
  for (auto const& [providingFederateId, attributeHandles] : attributesByProvider) {
    auto recipient = candidateAttributeValueUpdateProvideRecipient(
        federation->second,
        receivingFederateId,
        providingFederateId,
        objectInstanceHandle,
        attributeHandles);
    if (recipient) {
      result.recipients.push_back(std::move(*recipient));
    }
  }
  return result;
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
  auto const availableDimensions = availableObjectClassDimensions(
      federation->second,
      instance->second.registeredObjectClassHandle);
  if (!availableDimensions) {
    return {ReceiveOrderAttributeUpdateStatus::inconsistent_catalog};
  }
  bool const classHasAvailableDimensions = !availableDimensions->empty();

  std::set<std::uint64_t> uniqueAttributeHandles;
  using PasselKey = std::tuple<
      std::string,
      rti1516_2025::OrderType,
      std::set<std::uint64_t>,
      bool>;
  std::map<PasselKey, std::vector<std::uint64_t>> attributesByTransportationOrderAndRegions;
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
    auto const transportationName = effectiveAttributeTransportationName(
        federation->second,
        instance->second,
        attributeHandle);
    if (!transportationName) {
      return {ReceiveOrderAttributeUpdateStatus::inconsistent_catalog};
    }
    auto const preferredOrderType = effectiveAttributeOrderType(
        federation->second,
        instance->second,
        attributeHandle);
    if (!preferredOrderType) {
      return {ReceiveOrderAttributeUpdateStatus::inconsistent_catalog};
    }
    std::set<std::uint64_t> associatedRegions;
    auto const regionAssociation = instance->second.updateRegionsByAttribute.find(attributeHandle);
    if (regionAssociation != instance->second.updateRegionsByAttribute.end()) {
      associatedRegions = regionAssociation->second;
    }
    bool const defaultRegionUsed =
        classHasAvailableDimensions && associatedRegions.empty();
    attributesByTransportationOrderAndRegions[
        {*transportationName,
         *preferredOrderType,
         std::move(associatedRegions),
         defaultRegionUsed}]
        .push_back(attributeHandle);
  }

  ReceiveOrderAttributeUpdatePlan result;
  result.passels.reserve(attributesByTransportationOrderAndRegions.size());
  for (auto const& [passelKey, passelAttributes] : attributesByTransportationOrderAndRegions) {
    ReceiveOrderAttributeUpdatePassel passel;
    passel.transportationName = std::get<0>(passelKey);
    passel.preferredOrderType = std::get<1>(passelKey);
    passel.sentAttributeHandles = passelAttributes;
    passel.sentRegionHandles = std::get<2>(passelKey);
    passel.defaultRegionUsed = std::get<3>(passelKey);
    // §8.1.8 makes subscription evaluation a federation-wide,
    // creation-time policy. Retain every joined non-source federate with a
    // live callback route when delayed evaluation is enabled, including
    // explicit regional passels. The callback re-evaluates the receiver's
    // current attribute and region projection at its actual receive-order or
    // TSO delivery boundary, so a later declaration can make the passel
    // deliverable.
    bool const delaySubscriptionEvaluation =
        federation->second.delaySubscriptionEvaluationSwitch;
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
        continue;
      }
      if (!delaySubscriptionEvaluation || federateId == producingFederateId) {
        continue;
      }
      auto const callbackRoute = federation->second.interactionCallbackRoutes.find(federateId);
      if (callbackRoute == federation->second.interactionCallbackRoutes.end() ||
          !callbackRoute->second) {
        continue;
      }

      // Leave the received-attribute projection empty. It is determined from
      // the receiver's then-current subscription at the delivery fence.
      ReceiveOrderAttributeUpdateRecipient deferredRecipient;
      deferredRecipient.federateId = federateId;
      deferredRecipient.callbackRoute = callbackRoute->second;
      passel.recipients.push_back(std::move(deferredRecipient));
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

JoinedFederateMomAttributeValueUpdatePlan
EmbeddedFederationRegistry::planJoinedFederateMomAttributeValueUpdate(
    std::wstring const& federationName,
    std::uint64_t requestingFederateId,
    std::uint64_t objectInstanceHandle,
    std::set<std::uint64_t> const& requestedAttributeHandles,
    bool requireActiveSubscription) const {
  std::scoped_lock lock(mutex_);
  JoinedFederateMomAttributeValueUpdatePlan result;
  auto federation = federations_.find(federationName);
  if (federation == federations_.end()) {
    result.status = JoinedFederateMomAttributeValueUpdateStatus::
        federation_does_not_exist;
    return result;
  }
  auto object = federation->second.rtiOwnedJoinedFederateMomObjects.find(
      objectInstanceHandle);
  if (object == federation->second.rtiOwnedJoinedFederateMomObjects.end()) {
    return result;
  }
  result.rtiOwnedMomObject = true;
  if (!federation->second.members.contains(requestingFederateId)) {
    result.status = JoinedFederateMomAttributeValueUpdateStatus::
        requesting_federate_not_member;
    return result;
  }
  if (!object->second.knownFederateIds.contains(requestingFederateId)) {
    result.status = JoinedFederateMomAttributeValueUpdateStatus::
        object_instance_not_known;
    return result;
  }
  if (!federation->second.definition.catalog ||
      !federation->second.objectClassHandles ||
      !federation->second.attributeHandles) {
    result.status = JoinedFederateMomAttributeValueUpdateStatus::
        inconsistent_catalog;
    return result;
  }
  auto const objectClassName = federation->second.objectClassHandles->nameFor(
      object->second.objectClassHandle);
  if (!objectClassName ||
      federation->second.definition.catalog->objectClass(*objectClassName) == nullptr) {
    result.status = JoinedFederateMomAttributeValueUpdateStatus::
        inconsistent_catalog;
    return result;
  }
  for (std::uint64_t const attributeHandle : requestedAttributeHandles) {
    if (!federation->second.attributeHandles->nameFor(
            federation->second.definition.catalog.get(),
            *objectClassName,
            attributeHandle)) {
      result.status = JoinedFederateMomAttributeValueUpdateStatus::attribute_not_defined;
      return result;
    }
  }
  auto values = joinedFederateMomObjectAttributeValues(
      federation->second,
      object->second,
      requestingFederateId,
      requestedAttributeHandles,
      requireActiveSubscription);
  if (!values) {
    result.status = JoinedFederateMomAttributeValueUpdateStatus::object_instance_not_known;
    return result;
  }
  auto const callbackRoute = federation->second.interactionCallbackRoutes.find(
      requestingFederateId);
  if (callbackRoute == federation->second.interactionCallbackRoutes.end() ||
      !callbackRoute->second) {
    result.status = JoinedFederateMomAttributeValueUpdateStatus::inconsistent_catalog;
    return result;
  }
  result.status = JoinedFederateMomAttributeValueUpdateStatus::applied;
  result.recipient.emplace(JoinedFederateMomAttributeValueUpdateRecipient{
      requestingFederateId,
      object->second.objectInstanceHandle,
      std::move(*values),
      callbackRoute->second,
  });
  return result;
}

JoinedFederateMomAttributeValueUpdateClassPlan
EmbeddedFederationRegistry::planJoinedFederateMomAttributeValueUpdateClass(
    std::wstring const& federationName,
    std::uint64_t requestingFederateId,
    std::uint64_t objectClassHandle,
    std::set<std::uint64_t> const& requestedAttributeHandles,
    bool requireActiveSubscription) const {
  std::scoped_lock lock(mutex_);
  JoinedFederateMomAttributeValueUpdateClassPlan result;
  auto federation = federations_.find(federationName);
  if (federation == federations_.end() ||
      !federation->second.members.contains(requestingFederateId) ||
      !federation->second.definition.catalog ||
      !federation->second.objectClassHandles ||
      !federation->second.attributeHandles) {
    return result;
  }
  auto const callbackRoute = federation->second.interactionCallbackRoutes.find(
      requestingFederateId);
  if (callbackRoute == federation->second.interactionCallbackRoutes.end() ||
      !callbackRoute->second) {
    return result;
  }
  for (auto const& [objectInstanceHandle, object] :
       federation->second.rtiOwnedJoinedFederateMomObjects) {
    static_cast<void>(objectInstanceHandle);
    if (object.objectClassHandle == objectClassHandle) {
      result.rtiOwnedMomObject = true;
      break;
    }
  }
  for (auto const& [objectInstanceHandle, object] :
       federation->second.rtiOwnedJoinedFederateMomObjects) {
    static_cast<void>(objectInstanceHandle);
    if (object.objectClassHandle != objectClassHandle ||
        !object.knownFederateIds.contains(requestingFederateId)) {
      continue;
    }
    auto const values = joinedFederateMomObjectAttributeValues(
        federation->second,
        object,
        requestingFederateId,
        requestedAttributeHandles,
        requireActiveSubscription);
    if (!values || values->empty()) {
      continue;
    }
    result.recipients.push_back({
        requestingFederateId,
        object.objectInstanceHandle,
        *values,
        callbackRoute->second,
    });
  }
  return result;
}

JoinedFederateMomAttributeValueUpdateClassPlan
EmbeddedFederationRegistry::planJoinedFederateMomAttributeValueUpdateForObject(
    std::wstring const& federationName,
    std::uint64_t objectInstanceHandle,
    std::set<std::uint64_t> const& requestedAttributeHandles,
    bool requireActiveSubscription,
    std::optional<std::uint64_t> excludedReceivingFederateId) const {
  std::scoped_lock lock(mutex_);
  JoinedFederateMomAttributeValueUpdateClassPlan result;
  auto federation = federations_.find(federationName);
  if (federation == federations_.end()) {
    return result;
  }
  auto object = federation->second.rtiOwnedJoinedFederateMomObjects.find(
      objectInstanceHandle);
  if (object == federation->second.rtiOwnedJoinedFederateMomObjects.end()) {
    return result;
  }
  result.rtiOwnedMomObject = true;
  for (std::uint64_t const receivingFederateId : object->second.knownFederateIds) {
    if (!federation->second.members.contains(receivingFederateId)) {
      continue;
    }
    if (excludedReceivingFederateId.has_value() &&
        receivingFederateId == *excludedReceivingFederateId) {
      // IEEE 1516.1-2025's HLAfederateState update rule suppresses the
      // corresponding reflect at a federate that is itself in
      // FederateSaveInProgress.  This exclusion is intentionally an
      // event-only seam: direct AVU remains able to query the current value,
      // and other MOM attributes are unaffected unless a caller explicitly
      // opts into the exclusion.
      continue;
    }
    auto const values = joinedFederateMomObjectAttributeValues(
        federation->second,
        object->second,
        receivingFederateId,
        requestedAttributeHandles,
        requireActiveSubscription);
    if (!values || values->empty()) {
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
        object->second.objectInstanceHandle,
        *values,
        callbackRoute->second,
    });
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
  // RTI-owned joined-federate MOM instances are not federate-created object
  // instances. Their ownership answer is nevertheless a normal standard
  // Query Attribute Ownership result, delivered to the requesting federate
  // through its callback route.
  auto const momObject = federation->second.rtiOwnedJoinedFederateMomObjects.find(
      objectInstanceHandle);
  if (momObject != federation->second.rtiOwnedJoinedFederateMomObjects.end()) {
    if (!momObject->second.knownFederateIds.contains(requestingFederateId)) {
      return {AttributeOwnershipQueryStatus::object_instance_not_known};
    }
    if (!federation->second.definition.catalog ||
        !federation->second.objectClassHandles ||
        !federation->second.attributeHandles) {
      return {AttributeOwnershipQueryStatus::inconsistent_catalog};
    }
    auto const objectClassName = federation->second.objectClassHandles->nameFor(
        momObject->second.objectClassHandle);
    if (!objectClassName ||
        federation->second.definition.catalog->objectClass(*objectClassName) == nullptr) {
      return {AttributeOwnershipQueryStatus::inconsistent_catalog};
    }
    for (std::uint64_t const attributeHandle : requestedAttributeHandles) {
      if (!momObject->second.effectiveAttributeHandles.contains(attributeHandle) ||
          !federation->second.attributeHandles->nameFor(
              federation->second.definition.catalog.get(),
              *objectClassName,
              attributeHandle)) {
        return {AttributeOwnershipQueryStatus::attribute_not_defined};
      }
    }
    AttributeOwnershipQueryPlan result;
    auto recipient = candidateAttributeOwnershipQueryRecipient(
        federation->second,
        requestingFederateId,
        objectInstanceHandle,
        AttributeOwnershipQueryReportKind::rti,
        0,
        requestedAttributeHandles);
    if (recipient) {
      result.recipients.push_back(std::move(*recipient));
    }
    return result;
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
  // A discovered RTI-owned MOM object is known to the requesting federate
  // through the separate MOM ledger. Its attributes are valid object-class
  // attributes but none are owned by the invoking federate, so answer the
  // standard boolean query directly instead of reporting ObjectInstanceNotKnown.
  auto const momObject = federation->second.rtiOwnedJoinedFederateMomObjects.find(
      objectInstanceHandle);
  if (momObject != federation->second.rtiOwnedJoinedFederateMomObjects.end()) {
    if (!momObject->second.knownFederateIds.contains(requestingFederateId)) {
      return {AttributeOwnershipCheckStatus::object_instance_not_known};
    }
    if (!federation->second.definition.catalog ||
        !federation->second.objectClassHandles ||
        !federation->second.attributeHandles) {
      return {AttributeOwnershipCheckStatus::inconsistent_catalog};
    }
    auto const objectClassName = federation->second.objectClassHandles->nameFor(
        momObject->second.objectClassHandle);
    if (!objectClassName ||
        federation->second.definition.catalog->objectClass(*objectClassName) == nullptr) {
      return {AttributeOwnershipCheckStatus::inconsistent_catalog};
    }
    if (!momObject->second.effectiveAttributeHandles.contains(attributeHandle) ||
        !federation->second.attributeHandles->nameFor(
            federation->second.definition.catalog.get(),
            *objectClassName,
            attributeHandle)) {
      return {AttributeOwnershipCheckStatus::attribute_not_defined};
    }
    return {AttributeOwnershipCheckStatus::applied, false};
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
      // A completed resignation must not leave a stale owner record. Treat a
      // surviving record as an internal catalog inconsistency rather than
      // manufacturing an acquisition outcome from an invalid state.
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
  for (std::uint64_t const attributeHandle : delivery.securedAttributeHandles) {
    // The previous owner's non-default update-region association is not
    // inherited by the acquirer.  A later owner must explicitly create a new
    // association before regional updates can use a region again.
    clearUpdateRegionAssociation(instance->second, attributeHandle);
    auto const transportationName = attributeDefaultTransportationName(
        federation->second,
        requestingFederateId,
        knownClass->second,
        attributeHandle);
    if (transportationName) {
      instance->second.attributeTransportationTypes.insert_or_assign(
        attributeHandle,
        *transportationName);
    }
    auto const orderType = attributeDefaultOrderType(
        federation->second,
        requestingFederateId,
        knownClass->second,
        attributeHandle);
    if (orderType) {
      instance->second.attributeOrderTypes.insert_or_assign(attributeHandle, *orderType);
    } else {
      instance->second.attributeOrderTypes.erase(attributeHandle);
    }
  }
  refreshRegionUsage(federation->second);
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
      // A committed update-region association belongs to the former owner;
      // Confirm Divestiture transfers ownership without transferring that
      // association.
      clearUpdateRegionAssociation(instance->second, attributeHandle);
      auto const acquiringClass = instance->second.knownObjectClassHandlesByFederate.find(
          acquiringFederateId);
      if (acquiringClass != instance->second.knownObjectClassHandlesByFederate.end()) {
        auto const transportationName = attributeDefaultTransportationName(
            federation->second,
            acquiringFederateId,
            acquiringClass->second,
            attributeHandle);
        if (transportationName) {
          instance->second.attributeTransportationTypes.insert_or_assign(
              attributeHandle,
              *transportationName);
        }
        auto const orderType = attributeDefaultOrderType(
            federation->second,
            acquiringFederateId,
            acquiringClass->second,
            attributeHandle);
        if (orderType) {
          instance->second.attributeOrderTypes.insert_or_assign(attributeHandle, *orderType);
        } else {
          instance->second.attributeOrderTypes.erase(attributeHandle);
        }
      } else {
        instance->second.attributeOrderTypes.erase(attributeHandle);
      }
      instance->second.pendingNegotiatedAttributeOwnershipDivestitures.erase(attributeHandle);
    }
  }
  instance->second.pendingConfirmDivestitureNotifications.merge(notificationReservations);
  refreshRegionUsage(federation->second);
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
      // A completed resignation must not leave a stale owner record. Treat a
      // surviving record as an internal catalog inconsistency rather than
      // manufacturing a normal acquisition outcome from invalid state.
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
      auto const transportationName = attributeDefaultTransportationName(
          federation->second,
          requestingFederateId,
          knownClass->second,
          attributeHandle);
      if (transportationName) {
        instance->second.attributeTransportationTypes.insert_or_assign(
            attributeHandle,
            *transportationName);
      }
      auto const orderType = attributeDefaultOrderType(
          federation->second,
          requestingFederateId,
          knownClass->second,
          attributeHandle);
      if (orderType) {
        instance->second.attributeOrderTypes.insert_or_assign(attributeHandle, *orderType);
      } else {
        instance->second.attributeOrderTypes.erase(attributeHandle);
      }
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

  // Retain the recipients already offered so later discovery/publication
  // events can continue the search without duplicating this callback.
  for (auto const& recipient : result.assumptionRecipients) {
    for (std::uint64_t const attributeHandle : recipient.attributeHandles) {
      instance->second.ownershipAssumptionRecipientsByAttribute[attributeHandle].insert(
          recipient.receivingFederateId);
    }
  }
  for (std::uint64_t const attributeHandle : attributeHandles) {
    // Keep an empty search record when no current recipient was eligible;
    // later discovery or publication events must still be able to continue
    // the required assumption search.
    instance->second.ownershipAssumptionRecipientsByAttribute.try_emplace(attributeHandle);
  }

  // IEEE 1516.1-2025 §7.2 makes the supplied attributes unowned immediately;
  // no accepting federate is required for this transition. Existing regular
  // acquisition work is then replanned from this new unowned state, while an
  // existing If Available reservation retains the callback it already owns.
  for (std::uint64_t const attributeHandle : attributeHandles) {
    instance->second.attributeOwnersByHandle.erase(attributeHandle);
    clearUpdateRegionAssociation(instance->second, attributeHandle);
    instance->second.attributeTransportationTypes.erase(attributeHandle);
    instance->second.attributeOrderTypes.erase(attributeHandle);
    for (auto pending = instance->second.pendingAttributeTransportationTypeChanges.begin();
         pending != instance->second.pendingAttributeTransportationTypeChanges.end();) {
      pending->second.attributeHandles.erase(attributeHandle);
      if (pending->second.attributeHandles.empty()) {
        pending = instance->second.pendingAttributeTransportationTypeChanges.erase(pending);
      } else {
        ++pending;
      }
    }
    // A successful unconditional divestiture ends any earlier negotiated
    // waiting state for the same attribute. Its queued confirmation callback
    // will recheck this missing state and become harmless.
    instance->second.pendingNegotiatedAttributeOwnershipDivestitures.erase(attributeHandle);
  }
  refreshRegionUsage(federation->second);
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

std::vector<AttributeOwnershipAssumptionRecipient>
EmbeddedFederationRegistry::planAttributeOwnershipAssumptionsForFederate(
    std::wstring const& federationName,
    std::uint64_t receivingFederateId) {
  std::scoped_lock lock(mutex_);
  auto federation = federations_.find(federationName);
  if (federation == federations_.end() ||
      !federation->second.members.contains(receivingFederateId) ||
      !federation->second.definition.catalog ||
      !federation->second.objectClassHandles ||
      !federation->second.attributeHandles) {
    return {};
  }

  auto const callbackRoute = federation->second.interactionCallbackRoutes.find(
      receivingFederateId);
  if (callbackRoute == federation->second.interactionCallbackRoutes.end() ||
      !callbackRoute->second) {
    return {};
  }

  auto recipientHasPendingAcquisition = [](
                                           Federation::ObjectInstance const& instance,
                                           std::uint64_t federateId,
                                           std::uint64_t attributeHandle) {
    for (auto const& [requestId, pending] :
         instance.pendingAttributeOwnershipAcquisitionIfAvailableRequests) {
      static_cast<void>(requestId);
      if (pending.requestingFederateId == federateId &&
          pending.desiredAttributeHandles.contains(attributeHandle)) {
        return true;
      }
    }
    for (auto const& [requestId, pending] :
         instance.pendingAttributeOwnershipAcquisitionRequests) {
      static_cast<void>(requestId);
      if (pending.requestingFederateId == federateId &&
          pending.desiredAttributeHandles.contains(attributeHandle)) {
        return true;
      }
    }
    for (auto const& [cancellationId, cancellation] :
         instance.pendingAttributeOwnershipAcquisitionCancellations) {
      static_cast<void>(cancellationId);
      if (cancellation.requestingFederateId == federateId &&
          cancellation.attributeHandles.contains(attributeHandle)) {
        return true;
      }
    }
    return false;
  };

  std::map<std::uint64_t, std::set<std::uint64_t>> offeredAttributesByObject;
  for (auto& [objectInstanceHandle, instance] : federation->second.objectInstances) {
    if (instance.deleteAccepted) {
      continue;
    }
    auto const knownClass = instance.knownObjectClassHandlesByFederate.find(
        receivingFederateId);
    if (knownClass == instance.knownObjectClassHandlesByFederate.end()) {
      continue;
    }
    auto const knownClassName = federation->second.objectClassHandles->nameFor(
        knownClass->second);
    if (!knownClassName ||
        federation->second.definition.catalog->objectClass(*knownClassName) == nullptr) {
      continue;
    }
    auto const publishedAttributes = publishedObjectClassAttributes(
        federation->second,
        receivingFederateId,
        knownClass->second);
    if (!publishedAttributes) {
      continue;
    }

    for (auto search = instance.ownershipAssumptionRecipientsByAttribute.begin();
         search != instance.ownershipAssumptionRecipientsByAttribute.end();) {
      std::uint64_t const attributeHandle = search->first;
      auto const owner = instance.attributeOwnersByHandle.find(attributeHandle);
      if (owner != instance.attributeOwnersByHandle.end()) {
        search = instance.ownershipAssumptionRecipientsByAttribute.erase(search);
        continue;
      }
      if (search->second.contains(receivingFederateId) ||
          !federation->second.attributeHandles->nameFor(
              federation->second.definition.catalog.get(),
              *knownClassName,
              attributeHandle) ||
          !publishedAttributes->contains(attributeHandle) ||
          recipientHasPendingAcquisition(instance, receivingFederateId, attributeHandle)) {
        ++search;
        continue;
      }

      // Reserve the tuple before returning it.  The callback-entry recheck
      // still suppresses a now-stale offer, but a repeated declaration event
      // cannot enqueue another copy while this one is outstanding.
      search->second.insert(receivingFederateId);
      offeredAttributesByObject[objectInstanceHandle].insert(attributeHandle);
      ++search;
    }
  }

  std::vector<AttributeOwnershipAssumptionRecipient> result;
  result.reserve(offeredAttributesByObject.size());
  for (auto& [objectInstanceHandle, attributeHandles] : offeredAttributesByObject) {
    result.push_back({
        receivingFederateId,
        objectInstanceHandle,
        std::move(attributeHandles),
        callbackRoute->second,
    });
  }
  return result;
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
    // Divestiture If Wanted transfers the attribute synchronously, but the
    // former owner's explicit update-region association still ends at the
    // ownership boundary.
    clearUpdateRegionAssociation(instance->second, attributeHandle);
    instance->second.attributeOrderTypes.erase(attributeHandle);
    auto const acquiringClass = instance->second.knownObjectClassHandlesByFederate.find(
        selectedAcquirer.receivingFederateId);
    if (acquiringClass != instance->second.knownObjectClassHandlesByFederate.end()) {
      auto const orderType = attributeDefaultOrderType(
          federation->second,
          selectedAcquirer.receivingFederateId,
          acquiringClass->second,
          attributeHandle);
      if (orderType) {
        instance->second.attributeOrderTypes.insert_or_assign(attributeHandle, *orderType);
      }
    }
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
  refreshRegionUsage(federation->second);
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
  auto const effectiveTransportation = effectiveInteractionTransportationName(
      federation->second,
      producingFederateId,
      sentInteractionClassHandle);
  if (!effectiveTransportation) {
    return {ReceiveOrderInteractionStatus::inconsistent_catalog};
  }
  result.transportationName = *effectiveTransportation;
  auto const preferredOrderType = interactionOrderType(
      federation->second,
      producingFederateId,
      sentInteractionClassHandle);
  if (!preferredOrderType) {
    return {ReceiveOrderInteractionStatus::inconsistent_catalog};
  }
  result.preferredOrderType = *preferredOrderType;
  auto const availableDimensions = availableInteractionDimensions(
      federation->second,
      sentInteractionClassHandle);
  if (!availableDimensions) {
    return {ReceiveOrderInteractionStatus::inconsistent_catalog};
  }
  result.defaultRegionUsed = sentRegionHandles == nullptr && !availableDimensions->empty();
  if (sentRegionHandles != nullptr) {
    if (sentRegionHandles->empty()) {
      // §9.12 explicitly accepts an empty set as a no-send operation, not as
      // an invocation of the ordinary Send Interaction service.
      return result;
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
  // 8.1.8 makes subscription evaluation a federation-wide, creation-time
  // policy. Retain every joined non-source recipient with a live callback
  // route when delayed evaluation is enabled, including explicit regional
  // sends. The route re-evaluates the full subscription and region-overlap
  // projection at its actual receive-order or TSO delivery boundary, so a
  // later declaration can make the message deliverable.
  bool const delaySubscriptionEvaluation =
      federation->second.delaySubscriptionEvaluationSwitch;
  for (auto const& [federateId, membership] : federation->second.members) {
    static_cast<void>(membership);
    auto recipient = candidateReceiveOrderInteractionRecipient(
        federation->second,
        InteractionProducer::joinedFederate(producingFederateId),
        federateId,
        sentInteractionClassHandle,
        sentParameterHandles,
        sentRegionHandles);
    if (recipient) {
      result.recipients.push_back(std::move(*recipient));
      continue;
    }
    if (!delaySubscriptionEvaluation || federateId == producingFederateId) {
      continue;
    }
    auto const callbackRoute = federation->second.interactionCallbackRoutes.find(federateId);
    if (callbackRoute == federation->second.interactionCallbackRoutes.end() ||
        !callbackRoute->second) {
      continue;
    }

    // The received class and parameter projection intentionally remain unset:
    // they are determined against the recipient's current subscriptions when
    // the queued callback becomes eligible for delivery.
    ReceiveOrderInteractionRecipient deferredRecipient;
    deferredRecipient.federateId = federateId;
    deferredRecipient.callbackRoute = callbackRoute->second;
    result.recipients.push_back(std::move(deferredRecipient));
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
      InteractionProducer::joinedFederate(producingFederateId),
      receivingFederateId,
      sentInteractionClassHandle,
      sentParameterHandles,
      sentRegionHandles);
}

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

std::optional<FederateLostReportRouting>
EmbeddedFederationRegistry::federateLostReportRoutingFor(
    Federation const& federation,
    std::uint64_t reportedFederateId) {
  if (reportedFederateId == 0U || !federation.definition.catalog ||
      !federation.interactionClassHandles || !federation.parameterHandles ||
      !federation.dimensionHandles) {
    return std::nullopt;
  }

  auto const reportClassHandle = federation.interactionClassHandles->handleFor(
      kReportFederateLostInteractionClassName);
  auto const federateDimensionHandle = federation.dimensionHandles->handleFor(
      kHlaFederateDimensionName);
  auto const federateParameterHandle = federation.parameterHandles->handleFor(
      federation.definition.catalog.get(),
      kReportFederateLostInteractionClassName,
      kFederateLostFederateParameterName);
  auto const federateNameParameterHandle = federation.parameterHandles->handleFor(
      federation.definition.catalog.get(),
      kReportFederateLostInteractionClassName,
      kFederateLostFederateNameParameterName);
  auto const timestampParameterHandle = federation.parameterHandles->handleFor(
      federation.definition.catalog.get(),
      kReportFederateLostInteractionClassName,
      kFederateLostTimestampParameterName);
  auto const faultDescriptionParameterHandle = federation.parameterHandles->handleFor(
      federation.definition.catalog.get(),
      kReportFederateLostInteractionClassName,
      kFederateLostFaultDescriptionParameterName);
  if (!reportClassHandle || !federateDimensionHandle || !federateParameterHandle ||
      !federateNameParameterHandle || !timestampParameterHandle ||
      !faultDescriptionParameterHandle) {
    return std::nullopt;
  }

  auto const normalizedFederate = normalizedHandleValue(
      federation.normalizationSeed,
      reportedFederateId,
      kFederateNormalizationKind);
  FederateLostReportRouting result;
  result.interactionClassHandle = *reportClassHandle;
  result.federateParameterHandle = *federateParameterHandle;
  result.federateNameParameterHandle = *federateNameParameterHandle;
  result.timestampParameterHandle = *timestampParameterHandle;
  result.faultDescriptionParameterHandle = *faultDescriptionParameterHandle;
  result.endpointRegionHandle = kMomFederateLostEndpointRegionHandle;
  result.endpointRegion = {
      {*federateDimensionHandle},
      {{*federateDimensionHandle, {normalizedFederate, normalizedFederate + 1U}}},
      true,
  };
  return result;
}

FederateLostReportPlan EmbeddedFederationRegistry::planFederateLostReport(
    std::wstring const& federationName,
    std::uint64_t reportedFederateId) const {
  auto instrumentationScope = beginInstrumentation("planFederateLostReport");
  std::scoped_lock lock(mutex_);
  FederateLostReportPlan result;
  result.reportedFederateId = reportedFederateId;
  auto const federation = federations_.find(federationName);
  if (federation == federations_.end()) {
    result.status = FederateLostReportStatus::federation_does_not_exist;
    return result;
  }
  auto const reportedMember = federation->second.members.find(reportedFederateId);
  if (reportedMember == federation->second.members.end()) {
    result.status = FederateLostReportStatus::reported_federate_not_member;
    return result;
  }
  auto routing = federateLostReportRoutingFor(federation->second, reportedFederateId);
  if (!routing) {
    result.status = FederateLostReportStatus::inconsistent_catalog;
    return result;
  }
  auto const timeState = federation->second.timeCoordinator.timeStateFor(reportedFederateId);
  if (!timeState) {
    result.status = FederateLostReportStatus::inconsistent_time_state;
    return result;
  }
  auto const time = timeState->snapshot();
  if (!time.active || !time.currentTime) {
    result.status = FederateLostReportStatus::inconsistent_time_state;
    return result;
  }

  result.reportedFederateName = reportedMember->second.name;
  result.reportedFederateWasTimeRegulating = time.timeRegulating;
  result.lastKnownTime = time.currentTime;
  result.routing = std::move(*routing);
  std::vector<std::uint64_t> const sentParameterHandles{
      result.routing.federateParameterHandle,
      result.routing.federateNameParameterHandle,
      result.routing.timestampParameterHandle,
      result.routing.faultDescriptionParameterHandle,
  };
  std::map<std::uint64_t, RegionSpecificationSnapshot> const endpointOverride{
      {result.routing.endpointRegionHandle, result.routing.endpointRegion},
  };
  std::set<std::uint64_t> const endpointRegionHandles{
      result.routing.endpointRegionHandle,
  };
  for (auto const& [federateId, membership] : federation->second.members) {
    static_cast<void>(membership);
    // The source names joined federates that remain in the federation. The
    // transport-fault target is still a member while this plan is captured,
    // so it must be excluded explicitly before its resignation occurs.
    if (federateId == reportedFederateId) {
      continue;
    }
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
EmbeddedFederationRegistry::federateLostReportRecipientFor(
    std::wstring const& federationName,
    std::uint64_t reportedFederateId,
    std::uint64_t receivingFederateId) const {
  std::scoped_lock lock(mutex_);
  auto const federation = federations_.find(federationName);
  if (federation == federations_.end() || receivingFederateId == reportedFederateId ||
      !federation->second.members.contains(receivingFederateId) ||
      !federation->second.federateNamesById.contains(reportedFederateId)) {
    return std::nullopt;
  }
  auto routing = federateLostReportRoutingFor(federation->second, reportedFederateId);
  if (!routing) {
    return std::nullopt;
  }
  std::vector<std::uint64_t> const sentParameterHandles{
      routing->federateParameterHandle,
      routing->federateNameParameterHandle,
      routing->timestampParameterHandle,
      routing->faultDescriptionParameterHandle,
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
  auto const exceptionParameterHandle = federation.parameterHandles->handleFor(
      federation.definition.catalog.get(),
      kReportExceptionInteractionClassName,
      kExceptionReportExceptionParameterName);
  if (!reportClassHandle || !federateDimensionHandle ||
      !serviceParameterHandle || !exceptionParameterHandle) {
    return std::nullopt;
  }

  auto const normalizedFederate = normalizedHandleValue(
      federation.normalizationSeed,
      reportedFederateId,
      kFederateNormalizationKind);
  ExceptionReportRouting result;
  result.interactionClassHandle = *reportClassHandle;
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

AttributeTransportationTypeChangePlan
EmbeddedFederationRegistry::planAttributeTransportationTypeChange(
    std::wstring const& federationName,
    std::uint64_t requestingFederateId,
    std::uint64_t objectInstanceHandle,
    std::set<std::uint64_t> const& attributeHandles,
    std::string transportationName) {
  std::scoped_lock lock(mutex_);
  auto federation = federations_.find(federationName);
  if (federation == federations_.end()) {
    return {AttributeTransportationTypeChangeStatus::federation_does_not_exist};
  }
  if (!federation->second.members.contains(requestingFederateId)) {
    return {AttributeTransportationTypeChangeStatus::requesting_federate_not_member};
  }
  if (!isSupportedTransportationName(transportationName)) {
    return {AttributeTransportationTypeChangeStatus::invalid_transportation_type};
  }
  auto instance = federation->second.objectInstances.find(objectInstanceHandle);
  if (instance == federation->second.objectInstances.end() ||
      instance->second.deleteAccepted ||
      !instance->second.knownObjectClassHandlesByFederate.contains(requestingFederateId)) {
    return {AttributeTransportationTypeChangeStatus::object_instance_not_known};
  }
  auto const knownClass = instance->second.knownObjectClassHandlesByFederate.at(
      requestingFederateId);
  if (!validObjectClassAttributes(
          federation->second,
          knownClass,
          attributeHandles)) {
    return {AttributeTransportationTypeChangeStatus::attribute_not_defined};
  }
  for (std::uint64_t const attributeHandle : attributeHandles) {
    auto const owner = instance->second.attributeOwnersByHandle.find(attributeHandle);
    if (owner == instance->second.attributeOwnersByHandle.end() ||
        owner->second != requestingFederateId) {
      return {AttributeTransportationTypeChangeStatus::attribute_not_owned};
    }
    for (auto const& [requestId, pending] :
         instance->second.pendingAttributeTransportationTypeChanges) {
      static_cast<void>(requestId);
      if (pending.attributeHandles.contains(attributeHandle)) {
        return {AttributeTransportationTypeChangeStatus::attribute_already_being_changed};
      }
    }
  }
  auto const callbackRoute = federation->second.interactionCallbackRoutes.find(
      requestingFederateId);
  if (callbackRoute == federation->second.interactionCallbackRoutes.end() ||
      !callbackRoute->second) {
    return {AttributeTransportationTypeChangeStatus::callback_route_missing};
  }
  if (attributeHandles.empty()) {
    return {
        AttributeTransportationTypeChangeStatus::applied,
        0,
        objectInstanceHandle,
        {},
        std::move(transportationName),
        callbackRoute->second,
    };
  }

  auto const requestId = federation->second.nextAttributeTransportationTypeChangeRequestId++;
  instance->second.pendingAttributeTransportationTypeChanges.emplace(
      requestId,
      Federation::ObjectInstance::PendingAttributeTransportationTypeChange{
          requestingFederateId,
          attributeHandles,
          transportationName,
      });
  return {
      AttributeTransportationTypeChangeStatus::applied,
      requestId,
      objectInstanceHandle,
      attributeHandles,
      std::move(transportationName),
      callbackRoute->second,
  };
}

AttributeOrderTypeChangeStatus EmbeddedFederationRegistry::changeAttributeOrderType(
    std::wstring const& federationName,
    std::uint64_t requestingFederateId,
    std::uint64_t objectInstanceHandle,
    std::set<std::uint64_t> const& attributeHandles,
    rti1516_2025::OrderType orderType) {
  std::scoped_lock lock(mutex_);
  auto federation = federations_.find(federationName);
  if (federation == federations_.end()) {
    return AttributeOrderTypeChangeStatus::federation_does_not_exist;
  }
  if (!federation->second.members.contains(requestingFederateId)) {
    return AttributeOrderTypeChangeStatus::requesting_federate_not_member;
  }
  if (orderType != rti1516_2025::RECEIVE && orderType != rti1516_2025::TIMESTAMP) {
    return AttributeOrderTypeChangeStatus::invalid_order_type;
  }
  auto instance = federation->second.objectInstances.find(objectInstanceHandle);
  if (instance == federation->second.objectInstances.end() ||
      instance->second.deleteAccepted ||
      !instance->second.knownObjectClassHandlesByFederate.contains(requestingFederateId)) {
    return AttributeOrderTypeChangeStatus::object_instance_not_known;
  }
  auto const knownClass = instance->second.knownObjectClassHandlesByFederate.at(
      requestingFederateId);
  if (!validObjectClassAttributes(federation->second, knownClass, attributeHandles)) {
    return AttributeOrderTypeChangeStatus::attribute_not_defined;
  }
  for (std::uint64_t const attributeHandle : attributeHandles) {
    auto const owner = instance->second.attributeOwnersByHandle.find(attributeHandle);
    if (owner == instance->second.attributeOwnersByHandle.end() ||
        owner->second != requestingFederateId) {
      return AttributeOrderTypeChangeStatus::attribute_not_owned;
    }
  }
  for (std::uint64_t const attributeHandle : attributeHandles) {
    instance->second.attributeOrderTypes.insert_or_assign(attributeHandle, orderType);
  }
  return AttributeOrderTypeChangeStatus::applied;
}

std::optional<AttributeTransportationTypeChangeDelivery>
EmbeddedFederationRegistry::beginAttributeTransportationTypeChange(
    std::wstring const& federationName,
    std::uint64_t requestingFederateId,
    std::uint64_t requestId) {
  if (requestId == 0) {
    return std::nullopt;
  }
  std::scoped_lock lock(mutex_);
  auto federation = federations_.find(federationName);
  if (federation == federations_.end()) {
    return std::nullopt;
  }
  for (auto& [objectInstanceHandle, instance] : federation->second.objectInstances) {
    auto pending = instance.pendingAttributeTransportationTypeChanges.find(requestId);
    if (pending == instance.pendingAttributeTransportationTypeChanges.end()) {
      continue;
    }
    auto pendingState = std::move(pending->second);
    instance.pendingAttributeTransportationTypeChanges.erase(pending);
    if (pendingState.requestingFederateId != requestingFederateId ||
        instance.deleteAccepted ||
        !federation->second.members.contains(requestingFederateId) ||
        !instance.knownObjectClassHandlesByFederate.contains(requestingFederateId)) {
      return std::nullopt;
    }
    for (std::uint64_t const attributeHandle : pendingState.attributeHandles) {
      auto const owner = instance.attributeOwnersByHandle.find(attributeHandle);
      if (owner == instance.attributeOwnersByHandle.end() ||
          owner->second != requestingFederateId) {
        return std::nullopt;
      }
    }
    for (std::uint64_t const attributeHandle : pendingState.attributeHandles) {
      instance.attributeTransportationTypes.insert_or_assign(
          attributeHandle,
          pendingState.transportationName);
    }
    return AttributeTransportationTypeChangeDelivery{
        objectInstanceHandle,
        std::move(pendingState.attributeHandles),
        std::move(pendingState.transportationName),
    };
  }
  return std::nullopt;
}

void EmbeddedFederationRegistry::cancelAttributeTransportationTypeChange(
    std::wstring const& federationName,
    std::uint64_t requestingFederateId,
    std::uint64_t requestId) {
  if (requestId == 0) {
    return;
  }
  std::scoped_lock lock(mutex_);
  auto federation = federations_.find(federationName);
  if (federation == federations_.end()) {
    return;
  }
  for (auto& [objectInstanceHandle, instance] : federation->second.objectInstances) {
    static_cast<void>(objectInstanceHandle);
    auto pending = instance.pendingAttributeTransportationTypeChanges.find(requestId);
    if (pending != instance.pendingAttributeTransportationTypeChanges.end() &&
        pending->second.requestingFederateId == requestingFederateId) {
      instance.pendingAttributeTransportationTypeChanges.erase(pending);
      return;
    }
  }
}

AttributeTransportationTypeDefaultStatus
EmbeddedFederationRegistry::changeDefaultAttributeTransportationType(
    std::wstring const& federationName,
    std::uint64_t requestingFederateId,
    std::uint64_t objectClassHandle,
    std::set<std::uint64_t> const& attributeHandles,
    std::string transportationName) {
  std::scoped_lock lock(mutex_);
  auto federation = federations_.find(federationName);
  if (federation == federations_.end()) {
    return AttributeTransportationTypeDefaultStatus::federation_does_not_exist;
  }
  if (!federation->second.members.contains(requestingFederateId)) {
    return AttributeTransportationTypeDefaultStatus::requesting_federate_not_member;
  }
  if (!isSupportedTransportationName(transportationName)) {
    return AttributeTransportationTypeDefaultStatus::invalid_transportation_type;
  }
  if (!validObjectClassAttributes(
          federation->second,
          objectClassHandle,
          attributeHandles)) {
    return validObjectClass(federation->second, objectClassHandle)
        ? AttributeTransportationTypeDefaultStatus::attribute_not_defined
        : AttributeTransportationTypeDefaultStatus::object_class_not_defined;
  }
  auto& declarations = federation->second.objectClassAttributeDeclarations[requestingFederateId];
  auto& perClass = declarations.byObjectClass[objectClassHandle];
  for (std::uint64_t const attributeHandle : attributeHandles) {
    perClass.defaultTransportationTypes.insert_or_assign(
        attributeHandle,
        transportationName);
  }
  return AttributeTransportationTypeDefaultStatus::applied;
}

AttributeOrderTypeDefaultStatus EmbeddedFederationRegistry::changeDefaultAttributeOrderType(
    std::wstring const& federationName,
    std::uint64_t requestingFederateId,
    std::uint64_t objectClassHandle,
    std::set<std::uint64_t> const& attributeHandles,
    rti1516_2025::OrderType orderType) {
  std::scoped_lock lock(mutex_);
  auto federation = federations_.find(federationName);
  if (federation == federations_.end()) {
    return AttributeOrderTypeDefaultStatus::federation_does_not_exist;
  }
  if (!federation->second.members.contains(requestingFederateId)) {
    return AttributeOrderTypeDefaultStatus::requesting_federate_not_member;
  }
  if (orderType != rti1516_2025::RECEIVE && orderType != rti1516_2025::TIMESTAMP) {
    return AttributeOrderTypeDefaultStatus::invalid_order_type;
  }
  if (!validObjectClass(federation->second, objectClassHandle)) {
    return AttributeOrderTypeDefaultStatus::object_class_not_defined;
  }
  if (!validObjectClassAttributes(
          federation->second,
          objectClassHandle,
          attributeHandles)) {
    return AttributeOrderTypeDefaultStatus::attribute_not_defined;
  }
  auto& declarations = federation->second.objectClassAttributeDeclarations[requestingFederateId];
  auto& perClass = declarations.byObjectClass[objectClassHandle];
  for (std::uint64_t const attributeHandle : attributeHandles) {
    perClass.defaultOrderTypes.insert_or_assign(attributeHandle, orderType);
  }
  return AttributeOrderTypeDefaultStatus::applied;
}

AttributeTransportationTypeQueryPlan
EmbeddedFederationRegistry::planAttributeTransportationTypeQuery(
    std::wstring const& federationName,
    std::uint64_t requestingFederateId,
    std::uint64_t objectInstanceHandle,
    std::uint64_t attributeHandle) const {
  std::scoped_lock lock(mutex_);
  auto federation = federations_.find(federationName);
  if (federation == federations_.end()) {
    return {AttributeTransportationTypeQueryStatus::federation_does_not_exist};
  }
  if (!federation->second.members.contains(requestingFederateId)) {
    return {AttributeTransportationTypeQueryStatus::requesting_federate_not_member};
  }
  auto instance = federation->second.objectInstances.find(objectInstanceHandle);
  if (instance == federation->second.objectInstances.end() ||
      instance->second.deleteAccepted ||
      !instance->second.knownObjectClassHandlesByFederate.contains(requestingFederateId)) {
    return {AttributeTransportationTypeQueryStatus::object_instance_not_known};
  }
  auto const knownClass = instance->second.knownObjectClassHandlesByFederate.at(
      requestingFederateId);
  if (!validObjectClassAttributes(
          federation->second,
          knownClass,
          std::set<std::uint64_t>{attributeHandle})) {
    return {AttributeTransportationTypeQueryStatus::attribute_not_defined};
  }
  auto const transportationName = effectiveAttributeTransportationName(
      federation->second,
      instance->second,
      attributeHandle);
  if (!transportationName) {
    return {AttributeTransportationTypeQueryStatus::inconsistent_catalog};
  }
  auto const callbackRoute = federation->second.interactionCallbackRoutes.find(
      requestingFederateId);
  if (callbackRoute == federation->second.interactionCallbackRoutes.end() ||
      !callbackRoute->second) {
    return {AttributeTransportationTypeQueryStatus::callback_route_missing};
  }
  return {
      AttributeTransportationTypeQueryStatus::applied,
      objectInstanceHandle,
      attributeHandle,
      *transportationName,
      callbackRoute->second,
  };
}

std::optional<AttributeTransportationTypeQueryPlan>
EmbeddedFederationRegistry::attributeTransportationTypeQueryFor(
    std::wstring const& federationName,
    std::uint64_t requestingFederateId,
    std::uint64_t objectInstanceHandle,
    std::uint64_t attributeHandle) const {
  auto plan = planAttributeTransportationTypeQuery(
      federationName,
      requestingFederateId,
      objectInstanceHandle,
      attributeHandle);
  if (plan.status != AttributeTransportationTypeQueryStatus::applied) {
    return std::nullopt;
  }
  return plan;
}

InteractionTransportationTypeChangePlan
EmbeddedFederationRegistry::planInteractionTransportationTypeChange(
    std::wstring const& federationName,
    std::uint64_t requestingFederateId,
    std::uint64_t interactionClassHandle,
    std::string transportationName) {
  std::scoped_lock lock(mutex_);
  auto federation = federations_.find(federationName);
  if (federation == federations_.end()) {
    return {InteractionTransportationTypeChangeStatus::federation_does_not_exist};
  }
  if (!federation->second.members.contains(requestingFederateId)) {
    return {InteractionTransportationTypeChangeStatus::requesting_federate_not_member};
  }
  if (!validInteractionClass(federation->second, interactionClassHandle)) {
    return {InteractionTransportationTypeChangeStatus::interaction_class_not_defined};
  }
  if (!isSupportedTransportationName(transportationName)) {
    return {InteractionTransportationTypeChangeStatus::invalid_transportation_type};
  }
  auto declarations = federation->second.interactionDeclarations.find(requestingFederateId);
  if (declarations == federation->second.interactionDeclarations.end() ||
      !declarations->second.publishedInteractionClasses.contains(interactionClassHandle)) {
    return {InteractionTransportationTypeChangeStatus::interaction_class_not_published};
  }
  if (declarations->second.pendingInteractionTransportationTypeChanges.contains(
          interactionClassHandle)) {
    return {InteractionTransportationTypeChangeStatus::interaction_class_already_being_changed};
  }
  auto const callbackRoute = federation->second.interactionCallbackRoutes.find(
      requestingFederateId);
  if (callbackRoute == federation->second.interactionCallbackRoutes.end() ||
      !callbackRoute->second) {
    return {InteractionTransportationTypeChangeStatus::callback_route_missing};
  }
  declarations->second.pendingInteractionTransportationTypeChanges.insert_or_assign(
      interactionClassHandle,
      std::move(transportationName));
  return {
      InteractionTransportationTypeChangeStatus::applied,
      interactionClassHandle,
      declarations->second.pendingInteractionTransportationTypeChanges.at(
          interactionClassHandle),
      callbackRoute->second,
  };
}

InteractionOrderTypeChangeStatus EmbeddedFederationRegistry::changeInteractionOrderType(
    std::wstring const& federationName,
    std::uint64_t requestingFederateId,
    std::uint64_t interactionClassHandle,
    rti1516_2025::OrderType orderType) {
  std::scoped_lock lock(mutex_);
  auto federation = federations_.find(federationName);
  if (federation == federations_.end()) {
    return InteractionOrderTypeChangeStatus::federation_does_not_exist;
  }
  if (!federation->second.members.contains(requestingFederateId)) {
    return InteractionOrderTypeChangeStatus::requesting_federate_not_member;
  }
  if (orderType != rti1516_2025::RECEIVE && orderType != rti1516_2025::TIMESTAMP) {
    return InteractionOrderTypeChangeStatus::invalid_order_type;
  }
  if (!validInteractionClass(federation->second, interactionClassHandle)) {
    return InteractionOrderTypeChangeStatus::interaction_class_not_defined;
  }
  auto declarations = federation->second.interactionDeclarations.find(requestingFederateId);
  bool published =
      declarations != federation->second.interactionDeclarations.end() &&
      declarations->second.publishedInteractionClasses.contains(interactionClassHandle);
  // Directed interactions are published through an object-class declaration,
  // but they remain interaction classes for the order-control service. Treat
  // any active directed publication as the corresponding interaction-class
  // publication boundary.
  if (!published && declarations != federation->second.interactionDeclarations.end()) {
    for (auto const& [objectClassHandle, directedClasses] :
         declarations->second.publishedObjectClassDirectedInteractions) {
      static_cast<void>(objectClassHandle);
      if (directedClasses.contains(interactionClassHandle)) {
        published = true;
        break;
      }
    }
  }
  if (!published) {
    return InteractionOrderTypeChangeStatus::interaction_class_not_published;
  }
  declarations->second.interactionOrderTypes.insert_or_assign(interactionClassHandle, orderType);
  return InteractionOrderTypeChangeStatus::applied;
}

std::optional<std::string>
EmbeddedFederationRegistry::beginInteractionTransportationTypeChange(
    std::wstring const& federationName,
    std::uint64_t requestingFederateId,
    std::uint64_t interactionClassHandle) {
  std::scoped_lock lock(mutex_);
  auto federation = federations_.find(federationName);
  if (federation == federations_.end()) {
    return std::nullopt;
  }
  auto declarations = federation->second.interactionDeclarations.find(requestingFederateId);
  if (declarations == federation->second.interactionDeclarations.end()) {
    return std::nullopt;
  }
  auto pending = declarations->second.pendingInteractionTransportationTypeChanges.find(
      interactionClassHandle);
  if (pending == declarations->second.pendingInteractionTransportationTypeChanges.end()) {
    return std::nullopt;
  }
  auto transportationName = std::move(pending->second);
  declarations->second.pendingInteractionTransportationTypeChanges.erase(pending);
  if (!federation->second.members.contains(requestingFederateId) ||
      !declarations->second.publishedInteractionClasses.contains(interactionClassHandle)) {
    return std::nullopt;
  }
  declarations->second.interactionTransportationTypes.insert_or_assign(
      interactionClassHandle,
      transportationName);
  return transportationName;
}

void EmbeddedFederationRegistry::cancelInteractionTransportationTypeChange(
    std::wstring const& federationName,
    std::uint64_t requestingFederateId,
    std::uint64_t interactionClassHandle) {
  std::scoped_lock lock(mutex_);
  auto federation = federations_.find(federationName);
  if (federation == federations_.end()) {
    return;
  }
  auto declarations = federation->second.interactionDeclarations.find(requestingFederateId);
  if (declarations != federation->second.interactionDeclarations.end()) {
    declarations->second.pendingInteractionTransportationTypeChanges.erase(
        interactionClassHandle);
  }
}

InteractionTransportationTypeQueryPlan
EmbeddedFederationRegistry::planInteractionTransportationTypeQuery(
    std::wstring const& federationName,
    std::uint64_t requestingFederateId,
    std::uint64_t queriedFederateId,
    std::uint64_t interactionClassHandle) const {
  std::scoped_lock lock(mutex_);
  auto federation = federations_.find(federationName);
  if (federation == federations_.end()) {
    return {InteractionTransportationTypeQueryStatus::federation_does_not_exist};
  }
  if (!federation->second.members.contains(requestingFederateId)) {
    return {InteractionTransportationTypeQueryStatus::requesting_federate_not_member};
  }
  if (!validInteractionClass(federation->second, interactionClassHandle) ||
      !federation->second.definition.catalog ||
      !federation->second.interactionClassHandles) {
    return {InteractionTransportationTypeQueryStatus::interaction_class_not_defined};
  }
  auto const className = federation->second.interactionClassHandles->nameFor(
      interactionClassHandle);
  auto const* interactionClass = className
      ? federation->second.definition.catalog->interactionClass(*className)
      : nullptr;
  if (interactionClass == nullptr || interactionClass->transportation.empty()) {
    return {InteractionTransportationTypeQueryStatus::inconsistent_catalog};
  }
  std::string transportationName = interactionClass->transportation;
  auto declarations = federation->second.interactionDeclarations.find(queriedFederateId);
  if (declarations != federation->second.interactionDeclarations.end() &&
      declarations->second.publishedInteractionClasses.contains(interactionClassHandle)) {
    auto const overrideType = declarations->second.interactionTransportationTypes.find(
        interactionClassHandle);
    if (overrideType != declarations->second.interactionTransportationTypes.end()) {
      transportationName = overrideType->second;
    }
  }
  auto const callbackRoute = federation->second.interactionCallbackRoutes.find(
      requestingFederateId);
  if (callbackRoute == federation->second.interactionCallbackRoutes.end() ||
      !callbackRoute->second) {
    return {InteractionTransportationTypeQueryStatus::callback_route_missing};
  }
  return {
      InteractionTransportationTypeQueryStatus::applied,
      queriedFederateId,
      interactionClassHandle,
      std::move(transportationName),
      callbackRoute->second,
  };
}

std::optional<InteractionTransportationTypeQueryPlan>
EmbeddedFederationRegistry::interactionTransportationTypeQueryFor(
    std::wstring const& federationName,
    std::uint64_t requestingFederateId,
    std::uint64_t queriedFederateId,
    std::uint64_t interactionClassHandle) const {
  auto plan = planInteractionTransportationTypeQuery(
      federationName,
      requestingFederateId,
      queriedFederateId,
      interactionClassHandle);
  if (plan.status != InteractionTransportationTypeQueryStatus::applied) {
    return std::nullopt;
  }
  return plan;
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
  auto const preferredOrderType = interactionOrderType(
      federation->second,
      producingFederateId,
      sentInteractionClassHandle);
  if (!preferredOrderType) {
    return {ReceiveOrderDirectedInteractionStatus::inconsistent_catalog};
  }
  result.preferredOrderType = *preferredOrderType;
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

  bool retainLostProducerAcceptance = false;
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
    retainLostProducerAcceptance =
        recipient != message->second.recipients.end() &&
        recipientState != retraction->second.recipientStates.end() &&
        recipientState->second == Federation::TsoRecipientDeliveryState::pending &&
        retraction->second.deliveryRequiredAfterConnectionLoss;
  }

  return candidateReceiveOrderDirectedInteractionRecipient(
      federation->second,
      producingFederateId,
      receivingFederateId,
      objectInstanceHandle,
      sentInteractionClassHandle,
      sentParameterHandles,
      retainLostProducerAcceptance);
}

}  // namespace umbra::detail
