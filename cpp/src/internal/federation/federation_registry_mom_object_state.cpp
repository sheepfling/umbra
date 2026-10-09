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

constexpr char kJoinedFederateMomObjectClassName[] =
    "HLAobjectRoot.HLAmanager.HLAfederate";
constexpr char kFederationMomObjectClassName[] =
    "HLAobjectRoot.HLAmanager.HLAfederation";
constexpr char kHlaReportServiceFileAttributeName[] = "HLAreportServiceFile";
constexpr char kHlaReportServiceFileMimConditionalUpdate[] = "Conditional";
constexpr char kHlaReportServiceFileMimConditionalUpdateCondition[] =
    "The first time that both HLAserviceReporting and "
    "HLAsendServiceReportsToFile become true.";

// The federation-object foundation deliberately keeps MIM Static attributes
// as initial values. Conditional lifecycle values are projected from their
// authoritative ledgers and must not be presented as stale initial values.
constexpr char const* kFederationMomStaticAttributeNames[] = {
    "HLAfederationName",
    "HLARTIversion",
    umbra::detail::hla::utf8::mom::mim_designator,
    "HLAtimeImplementationName",
    "HLAadvisoriesUseKnownClass",
    "HLAdelaySubscriptionEvaluation",
    "HLAnonRegulatedGrant",
    "HLAallowRelaxedDDM",
};

constexpr char kFederationMomStaticAttributeDataTypes[][24] = {
    "HLAunicodeString",
    "HLAunicodeString",
    "HLAunicodeString",
    "HLAunicodeString",
    "HLAswitch",
    "HLAswitch",
    "HLAswitch",
    "HLAswitch",
};

// Table 8's direct required joined-federate values. HLAreportServiceFile is
// included in the initial private snapshot under the selected 1516.1 Static
// policy; the unmodified 1516.2 MIM's contrary Conditional field is retained
// in the composed catalog and must not be overwritten here.
constexpr char const* kJoinedFederateInitialAttributeNames[] = {
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

std::vector<std::wstring> moduleContentsForJoin(
    std::vector<PrevalidatedFomModule> const& modules) {
  std::set<std::filesystem::path> seenSources;
  std::vector<std::wstring> contents;
  contents.reserve(modules.size());
  for (auto const& module : modules) {
    if (module.kind == FomModuleKind::fom &&
        seenSources.insert(module.sourcePath).second) {
      contents.push_back(module.contents);
    }
  }
  return contents;
}

rti1516_2025::VariableLengthData encodeFederationFomModuleList(
    FederationDefinition const& definition) {
  std::vector<PrevalidatedFomModule> fomModules;
  fomModules.reserve(definition.fomModules.size());
  for (auto const& module : definition.fomModules) {
    if (module.kind == FomModuleKind::fom) {
      fomModules.push_back(module);
    }
  }
  return encodeModuleDesignatorList(fomModules);
}

rti1516_2025::VariableLengthData encodeFederateReferenceList(
    std::vector<std::uint64_t> const& federateIds) {
  if (federateIds.size() >
      static_cast<std::size_t>(std::numeric_limits<rti1516_2025::Integer32>::max())) {
    throw rti1516_2025::EncoderException(
        L"The federation contains too many joined federate references.");
  }

  std::vector<rti1516_2025::Octet> bytes;
  rti1516_2025::HLAinteger32BE count{
      static_cast<rti1516_2025::Integer32>(federateIds.size())};
  appendDataElement(bytes, count);
  for (std::uint64_t const federateId : federateIds) {
    auto const encodedHandle =
        rti1516_2025::umbra_binding_detail::encodeUmbraHandleVariableArray(federateId);
    bytes.insert(bytes.end(), encodedHandle.begin(), encodedHandle.end());
  }
  return rti1516_2025::VariableLengthData(bytes.data(), bytes.size());
}

}  // namespace

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
  object.reportServiceFile = descriptor.reportServiceFile;
  object.immutableFederatePoint = {
      {*federateDimensionHandle},
      {{*federateDimensionHandle, {normalizedFederate, normalizedFederate + 1U}}},
      true,
  };
  object.effectiveAttributeHandles = std::move(effectiveAttributeHandles);
  object.periodicAttributeHandles = std::move(periodicAttributeHandles);
  object.initialAttributeValues = std::move(initialAttributeValues);
  object.fomModuleContents = moduleContentsForJoin(
      descriptor.fomModulesSpecifiedAtJoin);

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

JoinedFederateMomObjectStatus
EmbeddedFederationRegistry::establishFederationMomObject(
    std::wstring const& federationName,
    std::wstring const& rtiVersion,
    std::wstring const& mimDesignator) {
  auto instrumentationScope = beginInstrumentation("establishFederationMomObject");
  std::scoped_lock lock(mutex_);
  auto federation = federations_.find(federationName);
  if (federation == federations_.end()) {
    return JoinedFederateMomObjectStatus::federation_does_not_exist;
  }
  for (auto const& [objectHandle, object] :
       federation->second.rtiOwnedJoinedFederateMomObjects) {
    static_cast<void>(objectHandle);
    if (object.federationExecutionObject) {
      return JoinedFederateMomObjectStatus::already_established;
    }
  }
  if (!federation->second.definition.catalog ||
      !federation->second.objectClassHandles ||
      !federation->second.attributeHandles) {
    return JoinedFederateMomObjectStatus::inconsistent_catalog;
  }
  auto const objectClassHandle = federation->second.objectClassHandles->handleFor(
      kFederationMomObjectClassName);
  auto const effectiveAttributes =
      federation->second.definition.catalog->effectiveObjectClassAttributes(
          kFederationMomObjectClassName);
  if (!objectClassHandle || !effectiveAttributes) {
    return JoinedFederateMomObjectStatus::inconsistent_catalog;
  }

  std::map<std::string, std::uint64_t> attributeHandlesByName;
  std::set<std::uint64_t> effectiveAttributeHandles;
  for (auto const& [attributeName, attribute] : *effectiveAttributes) {
    auto const attributeHandle = federation->second.attributeHandles->handleFor(
        federation->second.definition.catalog.get(),
        kFederationMomObjectClassName,
        attributeName);
    if (!attributeHandle || !effectiveAttributeHandles.insert(*attributeHandle).second ||
        !attributeHandlesByName.emplace(attributeName, *attributeHandle).second) {
      return JoinedFederateMomObjectStatus::inconsistent_catalog;
    }
  }
  for (std::size_t index = 0; index < std::size(kFederationMomStaticAttributeNames); ++index) {
    auto const attribute = effectiveAttributes->find(kFederationMomStaticAttributeNames[index]);
    if (attribute == effectiveAttributes->end() ||
        attribute->second.dataType != kFederationMomStaticAttributeDataTypes[index] ||
        !attribute->second.valueRequired ||
        attribute->second.ownership != "NoTransfer" ||
        attribute->second.updateType != "Static" ||
        !attributeHandlesByName.contains(kFederationMomStaticAttributeNames[index])) {
      return JoinedFederateMomObjectStatus::inconsistent_catalog;
    }
  }

  auto encodeSwitch = [](bool value) {
    return rti1516_2025::HLAinteger32BE{value ? 1 : 0}.encode();
  };
  std::map<std::uint64_t, rti1516_2025::VariableLengthData> initialAttributeValues;
  auto addInitialValue = [&](char const* attributeName,
                             rti1516_2025::VariableLengthData value) {
    auto const attributeHandle = attributeHandlesByName.find(attributeName);
    return attributeHandle != attributeHandlesByName.end() &&
        initialAttributeValues.emplace(attributeHandle->second, std::move(value)).second;
  };
  try {
    if (!addInitialValue(
            "HLAfederationName",
            rti1516_2025::HLAunicodeString{federationName}.encode()) ||
        !addInitialValue(
            "HLARTIversion",
            rti1516_2025::HLAunicodeString{rtiVersion}.encode()) ||
        !addInitialValue(
            umbra::detail::hla::utf8::mom::mim_designator,
            rti1516_2025::HLAunicodeString{mimDesignator}.encode()) ||
        !addInitialValue(
            "HLAtimeImplementationName",
            rti1516_2025::HLAunicodeString{
                federation->second.definition.logicalTimeImplementationName}.encode()) ||
        !addInitialValue(
            "HLAadvisoriesUseKnownClass",
            encodeSwitch(federation->second.advisoriesUseKnownClassSwitch)) ||
        !addInitialValue(
            "HLAdelaySubscriptionEvaluation",
            encodeSwitch(federation->second.delaySubscriptionEvaluationSwitch)) ||
        !addInitialValue(
            "HLAnonRegulatedGrant",
            encodeSwitch(federation->second.nonRegulatedGrantSwitch)) ||
        !addInitialValue(
            "HLAallowRelaxedDDM",
            encodeSwitch(federation->second.allowRelaxedDDMSwitch))) {
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
  JoinedFederateMomObjectSnapshot object;
  object.objectInstanceHandle = objectInstanceHandle;
  object.objectClassHandle = *objectClassHandle;
  object.federationExecutionObject = true;
  object.effectiveAttributeHandles = std::move(effectiveAttributeHandles);
  object.initialAttributeValues = std::move(initialAttributeValues);
  auto const [position, inserted] =
      federation->second.rtiOwnedJoinedFederateMomObjects.emplace(
          objectInstanceHandle, std::move(object));
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

std::optional<JoinedFederateMomObjectSnapshot>
EmbeddedFederationRegistry::federationMomObjectFor(
    std::wstring const& federationName) const {
  auto instrumentationScope = beginInstrumentation("federationMomObjectFor");
  std::scoped_lock lock(mutex_);
  auto const federation = federations_.find(federationName);
  if (federation == federations_.end()) {
    return std::nullopt;
  }
  for (auto const& [objectHandle, object] :
       federation->second.rtiOwnedJoinedFederateMomObjects) {
    static_cast<void>(objectHandle);
    if (object.federationExecutionObject) {
      return object;
    }
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
      for (std::uint64_t const attributeHandle : object.effectiveAttributeHandles) {
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
  if (object.federationExecutionObject && *attributeName == "HLAautoProvide") {
    // HLAautoProvide is Conditional on the execution-scoped federation MOM
    // object.  The federation-wide switch is the authoritative value; unlike
    // joined-federate attributes it has no represented member lookup.
    return rti1516_2025::HLAinteger32BE{
        federation.autoProvideSwitch ? 1 : 0}.encode();
  }
  if (object.federationExecutionObject &&
      *attributeName == "HLAfederatesInFederation") {
    std::vector<std::uint64_t> federateIds;
    federateIds.reserve(federation.members.size());
    for (auto const& [federateId, membership] : federation.members) {
      static_cast<void>(membership);
      federateIds.push_back(federateId);
    }
    // The MIM value is a variable array of HLAfederateReference values;
    // each reference is the standard HLAfederateHandle variable-array wire
    // representation.  The execution membership map is the sole source of
    // truth, so the value changes exactly at join/resign boundaries.
    return encodeFederateReferenceList(federateIds);
  }
  if (object.federationExecutionObject &&
      *attributeName == "HLAFOMmoduleDesignatorList") {
    // The execution-scoped MIM value is the current FOM subset, excluding
    // HLAstandardMIM itself. Federation-management preparation owns the
    // canonical designators and source identity; encode that ledger through
    // the same standard HLAvariableArray<HLAunicodeString> representation as
    // the joined-federate MOM snapshot.
    return encodeFederationFomModuleList(federation.definition);
  }
  if (object.federationExecutionObject && *attributeName == "HLAcurrentFDD") {
    // MaterializedFdd is the immutable output of the same 1516.2 composition
    // transaction that owns the current catalog and module ledger. Convert
    // its validated UTF-8 XML to the official binding's unicode string rather
    // than exposing a path, a private wrapper, or a stale module snapshot.
    if (!federation.definition.fdd) {
      return std::nullopt;
    }
    auto const currentFdd = wideFromUtf8(federation.definition.fdd->xmlUtf8());
    if (!currentFdd) {
      throw rti1516_2025::EncoderException(
          L"The composed Current FDD is not valid UTF-8 text.");
    }
    return rti1516_2025::HLAunicodeString{*currentFdd}.encode();
  }
  if (object.federationExecutionObject && *attributeName == "HLAlastSaveName") {
    // The official MIM has no null wire value for HLAunicodeString; an empty
    // string is the standard representation before the first successful save.
    return rti1516_2025::HLAunicodeString{federation.lastSaveName}.encode();
  }
  if (object.federationExecutionObject && *attributeName == "HLAnextSaveName") {
    return rti1516_2025::HLAunicodeString{federation.nextSaveName}.encode();
  }
  if (object.federationExecutionObject && *attributeName == "HLAlastSaveTime") {
    return federation.lastSaveTime
        ? federation.lastSaveTime->encode()
        : rti1516_2025::VariableLengthData{};
  }
  if (object.federationExecutionObject && *attributeName == "HLAnextSaveTime") {
    return federation.nextSaveTime
        ? federation.nextSaveTime->encode()
        : rti1516_2025::VariableLengthData{};
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

  auto encodeMomMilliseconds = [](std::uint64_t milliseconds) {
    auto const bounded = std::min<std::uint64_t>(
        milliseconds,
        static_cast<std::uint64_t>(std::numeric_limits<std::int32_t>::max()));
    return rti1516_2025::HLAinteger32BE{
        static_cast<std::int32_t>(bounded)}.encode();
  };

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
  if (*attributeName == "HLAobjectInstancesThatCanBeDeleted") {
    // HLAstandardMIM defines this HLAcount as the number of live
    // federate-created object instances whose HLAprivilegeToDeleteObject
    // attribute is owned by the represented joined federate. The object
    // ownership ledger is authoritative; RTI-owned MOM objects are kept in a
    // separate map and are intentionally not included.
    std::size_t count = 0U;
    if (federation.definition.catalog && federation.objectClassHandles &&
        federation.attributeHandles) {
      for (auto const& [objectHandle, objectInstance] : federation.objectInstances) {
        static_cast<void>(objectHandle);
        auto const objectClassName = federation.objectClassHandles->nameFor(
            objectInstance.registeredObjectClassHandle);
        if (!objectClassName) {
          continue;
        }
        auto const privilegeToDelete = federation.attributeHandles->handleFor(
            federation.definition.catalog.get(),
            *objectClassName,
            "HLAprivilegeToDeleteObject");
        if (!privilegeToDelete) {
          continue;
        }
        auto const privilegeOwner = objectInstance.attributeOwnersByHandle.find(
            *privilegeToDelete);
        if (privilegeOwner != objectInstance.attributeOwnersByHandle.end() &&
            privilegeOwner->second == object.joinedFederateId) {
          ++count;
        }
      }
    }
    auto const encodedCount = count >
            static_cast<std::size_t>(std::numeric_limits<std::int32_t>::max())
        ? std::numeric_limits<std::int32_t>::max()
        : static_cast<std::int32_t>(count);
    return rti1516_2025::HLAinteger32BE{encodedCount}.encode();
  }
  if (*attributeName == "HLAupdatesSent") {
    // HLAstandardMIM defines HLAupdatesSent as the total number of times the
    // represented joined federate has successfully invoked Update Attribute
    // Values.  The membership counter is advanced at the accepted service
    // boundary, so this value does not mistake individual attribute values or
    // downstream reflections for additional service invocations.
    auto const count = member->second.successfulUpdateAttributeValuesCount;
    auto const encodedCount = count >
            static_cast<std::uint64_t>(std::numeric_limits<std::int32_t>::max())
        ? std::numeric_limits<std::int32_t>::max()
        : static_cast<std::int32_t>(count);
    return rti1516_2025::HLAinteger32BE{encodedCount}.encode();
  }
  if (*attributeName == "HLAobjectInstancesUpdated") {
    // HLAstandardMIM defines this HLAcount as the number of distinct object
    // instances for which the represented joined federate has successfully
    // invoked Update Attribute Values. The membership-owned handle set is
    // retained for the joined lifetime, so repeated updates to one instance
    // do not count as additional object instances.
    auto const count = member->second.successfullyUpdatedObjectInstanceHandles.size();
    auto const encodedCount = count >
            static_cast<std::size_t>(std::numeric_limits<std::int32_t>::max())
        ? std::numeric_limits<std::int32_t>::max()
        : static_cast<std::int32_t>(count);
    return rti1516_2025::HLAinteger32BE{encodedCount}.encode();
  }
  if (*attributeName == "HLAobjectInstancesRegistered") {
    // HLAstandardMIM defines this HLAcount as the total number of successful
    // Register Object Instance and Register Object Instance with Regions
    // invocations by the represented joined federate since Join.
    auto const count = member->second.successfulObjectInstanceRegistrationsCount;
    auto const encodedCount = count >
            static_cast<std::uint64_t>(std::numeric_limits<std::int32_t>::max())
        ? std::numeric_limits<std::int32_t>::max()
        : static_cast<std::int32_t>(count);
    return rti1516_2025::HLAinteger32BE{encodedCount}.encode();
  }
  if (*attributeName == "HLAobjectInstancesDeleted") {
    // HLAstandardMIM defines this HLAcount as the total number of Delete
    // Object Instance service invocations accepted for the represented joined
    // federate since Join, regardless of whether a later Retract restores a
    // timestamped deletion's object state.
    auto const count = member->second.successfulObjectInstanceDeletionsCount;
    auto const encodedCount = count >
            static_cast<std::uint64_t>(std::numeric_limits<std::int32_t>::max())
        ? std::numeric_limits<std::int32_t>::max()
        : static_cast<std::int32_t>(count);
    return rti1516_2025::HLAinteger32BE{encodedCount}.encode();
  }
  if (*attributeName == "HLAobjectInstancesRemoved") {
    // HLAstandardMIM defines this HLAcount as the number of Remove Object
    // Instance callbacks committed for the represented joined federate. The
    // recipient membership ledger advances at callback admission, after any
    // legal timestamped retraction can still cancel the removal.
    auto const count = member->second.successfulObjectInstanceRemovalsCount;
    auto const encodedCount = count >
            static_cast<std::uint64_t>(std::numeric_limits<std::int32_t>::max())
        ? std::numeric_limits<std::int32_t>::max()
        : static_cast<std::int32_t>(count);
    return rti1516_2025::HLAinteger32BE{encodedCount}.encode();
  }
  if (*attributeName == "HLAobjectInstancesDiscovered") {
    // HLAstandardMIM defines this HLAcount as the number of Discover Object
    // Instance callbacks committed for the represented joined federate. The
    // application-object discovery ledger advances only after the callback
    // route passes its final membership/interest checks.
    auto const count = member->second.successfulObjectInstanceDiscoveriesCount;
    auto const encodedCount = count >
            static_cast<std::uint64_t>(std::numeric_limits<std::int32_t>::max())
        ? std::numeric_limits<std::int32_t>::max()
        : static_cast<std::int32_t>(count);
    return rti1516_2025::HLAinteger32BE{encodedCount}.encode();
  }
  if (*attributeName == "HLAobjectInstancesReflected") {
    // HLAstandardMIM defines this HLAcount as the number of distinct object
    // instances for which the represented joined federate has received a
    // Reflect Attribute Values callback. The membership-owned handle set
    // prevents repeated updates for one object from inflating the value.
    auto const count = member->second.successfullyReflectedObjectInstanceHandles.size();
    auto const encodedCount = count >
            static_cast<std::size_t>(std::numeric_limits<std::int32_t>::max())
        ? std::numeric_limits<std::int32_t>::max()
        : static_cast<std::int32_t>(count);
    return rti1516_2025::HLAinteger32BE{encodedCount}.encode();
  }
  if (*attributeName == "HLAreflectionsReceived") {
    // HLAstandardMIM defines this HLAcount as the number of Reflect
    // Attribute Values callback invocations at the joined federate. This
    // application ledger advances immediately before each callback and does
    // not infer from attribute-value fan-out or distinct object handles.
    auto const count = member->second.successfulReflectionsReceivedCount;
    auto const encodedCount = count >
            static_cast<std::uint64_t>(std::numeric_limits<std::int32_t>::max())
        ? std::numeric_limits<std::int32_t>::max()
        : static_cast<std::int32_t>(count);
    return rti1516_2025::HLAinteger32BE{encodedCount}.encode();
  }
  if (*attributeName == "HLAinteractionsSent") {
    // HLAstandardMIM defines this HLAcount as accepted Send Interaction
    // service invocations by the represented joined federate. The counter is
    // advanced at the C++ service boundary and therefore includes sends with
    // no eligible recipients without inferring from callback fan-out.
    auto const count = member->second.successfulInteractionsSentCount;
    auto const encodedCount = count >
            static_cast<std::uint64_t>(std::numeric_limits<std::int32_t>::max())
        ? std::numeric_limits<std::int32_t>::max()
        : static_cast<std::int32_t>(count);
    return rti1516_2025::HLAinteger32BE{encodedCount}.encode();
  }
  if (*attributeName == "HLAdirectedInteractionsSent") {
    // Directed sends are a separately reported subset of the total
    // interaction-send count, keyed from the same accepted-service ledger.
    auto const count = member->second.successfulDirectedInteractionsSentCount;
    auto const encodedCount = count >
            static_cast<std::uint64_t>(std::numeric_limits<std::int32_t>::max())
        ? std::numeric_limits<std::int32_t>::max()
        : static_cast<std::int32_t>(count);
    return rti1516_2025::HLAinteger32BE{encodedCount}.encode();
  }
  if (*attributeName == "HLAinteractionsReceived") {
    // HLAstandardMIM defines this HLAcount as accepted Receive Interaction
    // callback invocations at the represented joined federate. The callback
    // boundary, not sender fan-out, is the source of the total.
    auto const count = member->second.successfulInteractionsReceivedCount;
    auto const encodedCount = count >
            static_cast<std::uint64_t>(std::numeric_limits<std::int32_t>::max())
        ? std::numeric_limits<std::int32_t>::max()
        : static_cast<std::int32_t>(count);
    return rti1516_2025::HLAinteger32BE{encodedCount}.encode();
  }
  if (*attributeName == "HLAdirectedInteractionsReceived") {
    // Directed receives are a separately reported subset of all interaction
    // callbacks admitted at the receiver boundary.
    auto const count = member->second.successfulDirectedInteractionsReceivedCount;
    auto const encodedCount = count >
            static_cast<std::uint64_t>(std::numeric_limits<std::int32_t>::max())
        ? std::numeric_limits<std::int32_t>::max()
        : static_cast<std::int32_t>(count);
    return rti1516_2025::HLAinteger32BE{encodedCount}.encode();
  }
  if (*attributeName == "HLAROlength") {
    // HLAstandardMIM defines HLAROlength as the number of receive-order
    // messages waiting for the represented federate. The live callback route
    // counts application receive-order tasks still on the selected callback
    // dispatcher; a constrained federate can also hold eligible work in its
    // temporal receive-order gate before the dispatcher sees it.
    std::size_t count = 0U;
    auto const callbackRoute = federation.interactionCallbackRoutes.find(
        object.joinedFederateId);
    if (callbackRoute != federation.interactionCallbackRoutes.end() &&
        callbackRoute->second.pendingReceiveOrderCount) {
      count = callbackRoute->second.pendingReceiveOrderCount();
    }
    if (timeState) {
      count += timeState->deferredAsynchronousReceiveCount();
    }
    auto const encodedCount = count >
            static_cast<std::size_t>(std::numeric_limits<std::int32_t>::max())
        ? std::numeric_limits<std::int32_t>::max()
        : static_cast<std::int32_t>(count);
    return rti1516_2025::HLAinteger32BE{encodedCount}.encode();
  }
  if (*attributeName == "HLAtimeGrantedTime" ||
      *attributeName == "HLAtimeAdvancingTime") {
    if (!timeState) {
      return std::nullopt;
    }
    auto const durations = timeState->momTimeDurations();
    return encodeMomMilliseconds(
        *attributeName == "HLAtimeGrantedTime"
            ? durations.grantedMilliseconds
            : durations.advancingMilliseconds);
  }
  if (!timeSnapshot) {
    return std::nullopt;
  }
  // IEEE 1516.2-2025 HLAstandardMIM marks HLAlogicalTime, HLAlookahead,
  // HLAGALT, and HLALITS Periodic, but IEEE 1516.1-2025 §11.4.1 still
  // requires the RTI to supply values for a direct Request Attribute Value
  // Update regardless of whether HLAsetTiming has ever enabled periodic
  // reporting.  Use the selected official logical-time provider's own wire
  // representation; an undefined value is represented by the MIM's empty
  // variable-array form.
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
  if (*attributeName == "HLAGALT" || *attributeName == "HLALITS") {
    // HLAGALT and HLALITS are Periodic HLAlogicalTime values in the official
    // HLAstandardMIM.  Reuse the same federation-owned snapshot and
    // calculator that back Query GALT/Query LITS instead of deriving a MOM
    // value from a separate, potentially stale view of the time state.  The
    // MIM represents an undefined bound with the empty HLAlogicalTime array.
    auto const execution = makeTimeSnapshot(federation);
    if (!execution) {
      return std::nullopt;
    }
    auto const bounds = FederationTimeBoundsCalculator{}.calculate(
        *execution,
        object.joinedFederateId);
    auto const& selected = *attributeName == "HLAGALT" ? bounds.galt : bounds.lits;
    return selected ? selected->encode() : rti1516_2025::VariableLengthData{};
  }
  if (*attributeName == "HLATSOlength") {
    // HLAstandardMIM defines HLATSOlength as HLAcount: the number of TSO
    // messages queued for the joined federate.  The federation coordinator's
    // recipient-scoped queue is the authoritative source; in-transit and
    // already-completed delivery state are intentionally not counted as
    // queued messages.
    auto const tso = federation.timeCoordinator.tsoSnapshotFor(
        object.joinedFederateId);
    auto const count = tso.queued.size() >
            static_cast<std::size_t>(std::numeric_limits<std::int32_t>::max())
        ? std::numeric_limits<std::int32_t>::max()
        : static_cast<std::int32_t>(tso.queued.size());
    return rti1516_2025::HLAinteger32BE{count}.encode();
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
    bool requireActiveSubscription,
    std::map<std::uint64_t, std::set<std::uint64_t>> const*
        requestRegionsByAttribute) {
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
    if (requestRegionsByAttribute != nullptr) {
      auto const requestRegions = requestRegionsByAttribute->find(attributeHandle);
      if (requestRegions == requestRegionsByAttribute->end() ||
          requestRegions->second.empty()) {
        // An empty region set is the regional service's explicit no-op for
        // this attribute.  Keep it out of the direct MOM reflection just as
        // the ordinary object-class planner does for provider solicitation.
        continue;
      }
      bool overlaps = false;
      for (std::uint64_t const requestRegionHandle : requestRegions->second) {
        if (regionOverlapsSnapshot(
                federation,
                requestRegionHandle,
                object.immutableFederatePoint)) {
          overlaps = true;
          break;
        }
      }
      if (!overlaps) {
        // A regional request is evaluated against the immutable private
        // HLAfederate point at both planning and callback time.  A disjoint
        // region must not produce a reflection or a Provide callback.
        continue;
      }
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

}  // namespace umbra::detail
