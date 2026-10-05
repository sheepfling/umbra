#include "internal/federation/federation_registry.hpp"
#include "internal/federation/federation_registry_constants.hpp"
#include "internal/federation/federation_registry_value_helpers.hpp"
#include "internal/federation/federation_state_image.hpp"

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
      // A passive ordinary subscription is retained for later activation but
      // does not arrange delivery and therefore must not contribute to the
      // effective update-rate query.
      if (ordinary != perClass->second.subscribedAttributes.end() && ordinary->second) {
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
          // A passive regional declaration is retained for later activation,
          // but it does not currently arrange delivery and therefore must not
          // contribute to the effective update-rate query.
          if (!active) {
            continue;
          }
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

std::optional<std::uint64_t> EmbeddedFederationRegistry::transportationTypeHandleFor(
    std::wstring const& federationName,
    std::string const& transportationTypeName) const {
  std::scoped_lock lock(mutex_);
  auto const federation = federations_.find(federationName);
  if (federation == federations_.end() ||
      !federation->second.transportationTypeHandles) {
    return std::nullopt;
  }

  // The two predefined transportation names are part of the standard handle
  // universe even when a deliberately reduced FOM omits their declaration.
  // Every other name must be present in the execution's composed catalog.
  bool const isPredefined = transportationTypeName == "HLAreliable" ||
      transportationTypeName == "HLAbestEffort";
  if (!isPredefined &&
      (!federation->second.definition.catalog ||
       federation->second.definition.catalog->transportationType(
           transportationTypeName) == nullptr)) {
    return std::nullopt;
  }
  return federation->second.transportationTypeHandles->handleFor(
      transportationTypeName);
}

std::optional<std::string> EmbeddedFederationRegistry::transportationTypeNameFor(
    std::wstring const& federationName,
    std::uint64_t transportationTypeHandle) const {
  std::scoped_lock lock(mutex_);
  auto const federation = federations_.find(federationName);
  if (federation == federations_.end() ||
      !federation->second.transportationTypeHandles) {
    return std::nullopt;
  }
  auto const name = federation->second.transportationTypeHandles->nameFor(
      transportationTypeHandle);
  if (!name) {
    return std::nullopt;
  }

  // As with name-to-handle lookup, predefined names remain available in a
  // reduced FOM while an implementation-specific name is valid only while it
  // belongs to the current composed execution catalog.
  bool const isPredefined = *name == "HLAreliable" || *name == "HLAbestEffort";
  if (!isPredefined &&
      (!federation->second.definition.catalog ||
       federation->second.definition.catalog->transportationType(*name) == nullptr)) {
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

}  // namespace umbra::detail
