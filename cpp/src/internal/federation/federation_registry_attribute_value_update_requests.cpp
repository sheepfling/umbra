#include "internal/federation/federation_registry.hpp"
#include "internal/fom/fom_catalog.hpp"

#include <algorithm>
#include <cstdint>
#include <limits>
#include <map>
#include <mutex>
#include <optional>
#include <set>
#include <string>
#include <tuple>
#include <utility>
#include <vector>

namespace umbra::detail {
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
  result.registeredObjectClassHandle = instance->second.registeredObjectClassHandle;
  result.passels.reserve(attributesByTransportationOrderAndRegions.size());
  for (auto const& [passelKey, passelAttributes] : attributesByTransportationOrderAndRegions) {
    ReceiveOrderAttributeUpdatePassel passel;
    passel.transportationName = std::get<0>(passelKey);
    passel.preferredOrderType = std::get<1>(passelKey);
    passel.sentAttributeHandles = passelAttributes;
    passel.sentRegionHandles = std::get<2>(passelKey);
    passel.defaultRegionUsed = std::get<3>(passelKey);
    // The association planner admits only committed regions owned by the
    // producing federate. Retain the exact committed realization here so an
    // ordinary receive-order callback remains scoped to the accepted source
    // range even if that region is mutated before callback dispatch.
    for (std::uint64_t const regionHandle : passel.sentRegionHandles) {
      auto const region = federation->second.regions.find(regionHandle);
      if (region == federation->second.regions.end() ||
          region->second.ownerFederateId != producingFederateId ||
          !region->second.specificationCommitted ||
          region->second.committedRangeBounds.size() !=
              region->second.dimensionHandles.size()) {
        return {ReceiveOrderAttributeUpdateStatus::inconsistent_catalog};
      }
      passel.sentRegionSnapshots.emplace(
          regionHandle,
          RegionSpecificationSnapshot{
              region->second.dimensionHandles,
              region->second.committedRangeBounds,
              region->second.specificationCommitted});
    }
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
    std::set<std::uint64_t> const* sentRegionHandles,
    std::map<std::uint64_t, RegionSpecificationSnapshot> const* regionOverrides) const {
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
      sentRegionHandles,
      regionOverrides);
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
    bool requireActiveSubscription,
    std::map<std::uint64_t, std::set<std::uint64_t>> const*
        requestRegionsByAttribute) const {
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
      requireActiveSubscription,
      requestRegionsByAttribute);
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
    bool requireActiveSubscription,
    std::map<std::uint64_t, std::set<std::uint64_t>> const*
        requestRegionsByAttribute) const {
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
        requireActiveSubscription,
        requestRegionsByAttribute);
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
    std::set<std::uint64_t> const& requestedAttributeHandles,
    bool requireCurrentScope) const {
  std::scoped_lock lock(mutex_);
  auto const federation = federations_.find(federationName);
  if (federation == federations_.end()) {
    return std::nullopt;
  }
  if (!requireCurrentScope) {
    return candidateAttributeValueUpdateProvideRecipient(
        federation->second,
        requestingFederateId,
        providingFederateId,
        objectInstanceHandle,
        requestedAttributeHandles);
  }

  // Auto Provide work is planned after discovery, but HLA_IMMEDIATE and
  // HLA_EVOKED can both leave a later provider callback on the queue while a
  // different provider callback is executing. The federation-wide switch is
  // therefore part of the callback-time fence: disabling it consumes queued
  // automatic-provision work without affecting explicit Request Attribute
  // Value Update projections (which use the non-revalidating path above).
  if (!federation->second.autoProvideSwitch) {
    return std::nullopt;
  }

  // Auto Provide is planned immediately after discovery but its callback can
  // remain queued while the requester mutates a committed regional
  // subscription. Rebuild the candidate from the current scope at callback
  // time so stale automatic-provision work is consumed without soliciting a
  // provider after the attribute has gone out of scope.
  auto const instance = federation->second.objectInstances.find(objectInstanceHandle);
  if (instance == federation->second.objectInstances.end()) {
    return std::nullopt;
  }
  std::set<std::uint64_t> inScopeAttributes;
  for (auto const attributeHandle : requestedAttributeHandles) {
    if (objectAttributeInScope(
            federation->second,
            instance->second,
            requestingFederateId,
            attributeHandle)) {
      inScopeAttributes.insert(attributeHandle);
    }
  }
  if (inScopeAttributes.empty()) {
    return std::nullopt;
  }
  return candidateAttributeValueUpdateProvideRecipient(
      federation->second,
      requestingFederateId,
      providingFederateId,
      objectInstanceHandle,
      inScopeAttributes);
}

std::optional<std::uint64_t>
EmbeddedFederationRegistry::registerAttributeValueUpdateRequest(
    std::wstring const& federationName,
    std::uint64_t requestingFederateId,
    std::uint64_t providingFederateId,
    std::uint64_t objectInstanceHandle,
    std::set<std::uint64_t> const& requestedAttributeHandles,
    std::vector<unsigned char> userSuppliedTag) {
  std::scoped_lock lock(mutex_);
  auto federation = federations_.find(federationName);
  if (federation == federations_.end() ||
      requestingFederateId == 0U || providingFederateId == 0U ||
      requestingFederateId == providingFederateId ||
      !federation->second.members.contains(requestingFederateId) ||
      !federation->second.members.contains(providingFederateId) ||
      requestedAttributeHandles.empty() ||
      !federation->second.definition.catalog ||
      !federation->second.objectClassHandles ||
      !federation->second.attributeHandles) {
    return std::nullopt;
  }
  auto instance = federation->second.objectInstances.find(objectInstanceHandle);
  if (instance == federation->second.objectInstances.end() ||
      instance->second.deleteAccepted) {
    return std::nullopt;
  }
  auto knownClass = instance->second.knownObjectClassHandlesByFederate.find(
      requestingFederateId);
  if (knownClass == instance->second.knownObjectClassHandlesByFederate.end()) {
    return std::nullopt;
  }
  auto knownClassName = federation->second.objectClassHandles->nameFor(
      knownClass->second);
  if (!knownClassName ||
      federation->second.definition.catalog->objectClass(*knownClassName) == nullptr) {
    return std::nullopt;
  }
  for (auto const attributeHandle : requestedAttributeHandles) {
    auto const owner = instance->second.attributeOwnersByHandle.find(attributeHandle);
    if (owner == instance->second.attributeOwnersByHandle.end() ||
        owner->second != providingFederateId ||
        !federation->second.attributeHandles->nameFor(
            federation->second.definition.catalog.get(),
            *knownClassName,
            attributeHandle)) {
      return std::nullopt;
    }
  }
  auto& nextRequestId = federation->second.nextAttributeValueUpdateRequestId;
  if (nextRequestId == 0U ||
      nextRequestId == std::numeric_limits<std::uint64_t>::max()) {
    return std::nullopt;
  }
  auto const requestId = nextRequestId++;
  auto const [position, inserted] =
      instance->second.pendingAttributeValueUpdateRequests.emplace(
          requestId,
          Federation::ObjectInstance::PendingAttributeValueUpdateRequest{
              requestingFederateId,
              providingFederateId,
              requestedAttributeHandles,
              std::move(userSuppliedTag),
          });
  static_cast<void>(position);
  if (!inserted) {
    return std::nullopt;
  }
  return requestId;
}

std::optional<AttributeValueUpdateProvideRecipient>
EmbeddedFederationRegistry::beginAttributeValueUpdateProvideRecipientFor(
    std::wstring const& federationName,
    std::uint64_t requestId,
    std::uint64_t requestingFederateId,
    std::uint64_t providingFederateId,
    std::uint64_t objectInstanceHandle,
    std::set<std::uint64_t> const& requestedAttributeHandles) {
  if (requestId == 0U) {
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
  auto pending = instance->second.pendingAttributeValueUpdateRequests.find(requestId);
  if (pending == instance->second.pendingAttributeValueUpdateRequests.end()) {
    return std::nullopt;
  }
  auto pendingState = std::move(pending->second);
  instance->second.pendingAttributeValueUpdateRequests.erase(pending);
  if (pendingState.requestingFederateId != requestingFederateId ||
      pendingState.providingFederateId != providingFederateId ||
      pendingState.requestedAttributeHandles != requestedAttributeHandles) {
    return std::nullopt;
  }
  return candidateAttributeValueUpdateProvideRecipient(
      federation->second,
      pendingState.requestingFederateId,
      pendingState.providingFederateId,
      objectInstanceHandle,
      pendingState.requestedAttributeHandles);
}

std::optional<std::uint64_t>
EmbeddedFederationRegistry::registerAttributeValueUpdateClassRequest(
    std::wstring const& federationName,
    std::uint64_t requestingFederateId,
    std::uint64_t providingFederateId,
    std::uint64_t objectInstanceHandle,
    std::uint64_t requestedObjectClassHandle,
    std::set<std::uint64_t> const& requestedAttributeHandles,
    std::vector<unsigned char> userSuppliedTag) {
  std::scoped_lock lock(mutex_);
  auto federation = federations_.find(federationName);
  if (federation == federations_.end() ||
      requestingFederateId == 0U || providingFederateId == 0U ||
      requestingFederateId == providingFederateId ||
      objectInstanceHandle == 0U || requestedObjectClassHandle == 0U ||
      requestedAttributeHandles.empty() ||
      !federation->second.members.contains(requestingFederateId) ||
      !federation->second.members.contains(providingFederateId) ||
      !federation->second.definition.catalog ||
      !federation->second.objectClassHandles ||
      !federation->second.attributeHandles ||
      !validObjectClass(federation->second, requestedObjectClassHandle) ||
      !validObjectClassAttributes(
          federation->second,
          requestedObjectClassHandle,
          requestedAttributeHandles)) {
    return std::nullopt;
  }
  auto instance = federation->second.objectInstances.find(objectInstanceHandle);
  if (instance == federation->second.objectInstances.end() ||
      instance->second.deleteAccepted ||
      !objectInstanceRegisteredAtOrBelowClass(
          federation->second,
          instance->second.registeredObjectClassHandle,
          requestedObjectClassHandle)) {
    return std::nullopt;
  }
  auto projection = candidateAttributeValueUpdateClassProvideRecipient(
      federation->second,
      requestingFederateId,
      providingFederateId,
      objectInstanceHandle,
      requestedObjectClassHandle,
      requestedAttributeHandles,
      nullptr);
  if (!projection ||
      projection->requestedAttributeHandles != requestedAttributeHandles) {
    return std::nullopt;
  }
  auto& nextRequestId = federation->second.nextAttributeValueUpdateRequestId;
  if (nextRequestId == 0U ||
      nextRequestId == std::numeric_limits<std::uint64_t>::max()) {
    return std::nullopt;
  }
  auto const requestId = nextRequestId++;
  auto const [position, inserted] =
      instance->second.pendingAttributeValueUpdateClassRequests.emplace(
          requestId,
          Federation::ObjectInstance::PendingAttributeValueUpdateClassRequest{
              requestingFederateId,
              providingFederateId,
              requestedObjectClassHandle,
              requestedAttributeHandles,
              std::move(userSuppliedTag),
          });
  static_cast<void>(position);
  if (!inserted) {
    return std::nullopt;
  }
  return requestId;
}

std::optional<AttributeValueUpdateProvideRecipient>
EmbeddedFederationRegistry::beginAttributeValueUpdateClassProvideRecipientFor(
    std::wstring const& federationName,
    std::uint64_t requestId,
    std::uint64_t requestingFederateId,
    std::uint64_t providingFederateId,
    std::uint64_t objectInstanceHandle,
    std::uint64_t requestedObjectClassHandle,
    std::set<std::uint64_t> const& requestedAttributeHandles) {
  if (requestId == 0U) {
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
  auto pending = instance->second.pendingAttributeValueUpdateClassRequests.find(
      requestId);
  if (pending == instance->second.pendingAttributeValueUpdateClassRequests.end()) {
    return std::nullopt;
  }
  auto pendingState = std::move(pending->second);
  instance->second.pendingAttributeValueUpdateClassRequests.erase(pending);
  if (pendingState.requestingFederateId != requestingFederateId ||
      pendingState.providingFederateId != providingFederateId ||
      pendingState.requestedObjectClassHandle != requestedObjectClassHandle ||
      pendingState.requestedAttributeHandles != requestedAttributeHandles) {
    return std::nullopt;
  }
  return candidateAttributeValueUpdateClassProvideRecipient(
      federation->second,
      pendingState.requestingFederateId,
      pendingState.providingFederateId,
      objectInstanceHandle,
      pendingState.requestedObjectClassHandle,
      pendingState.requestedAttributeHandles,
      nullptr);
}

std::optional<std::uint64_t>
EmbeddedFederationRegistry::registerAttributeValueUpdateRegionalRequest(
    std::wstring const& federationName,
    std::uint64_t requestingFederateId,
    std::uint64_t providingFederateId,
    std::uint64_t objectInstanceHandle,
    std::uint64_t requestedObjectClassHandle,
    std::set<std::uint64_t> const& requestedAttributeHandles,
    std::map<std::uint64_t, std::set<std::uint64_t>> const&
        requestRegionsByAttribute,
    std::vector<unsigned char> userSuppliedTag) {
  std::scoped_lock lock(mutex_);
  auto federation = federations_.find(federationName);
  if (federation == federations_.end() ||
      requestingFederateId == 0U || providingFederateId == 0U ||
      requestingFederateId == providingFederateId ||
      objectInstanceHandle == 0U || requestedObjectClassHandle == 0U ||
      requestedAttributeHandles.empty() || requestRegionsByAttribute.empty() ||
      !federation->second.members.contains(requestingFederateId) ||
      !federation->second.members.contains(providingFederateId) ||
      !federation->second.definition.catalog ||
      !federation->second.objectClassHandles ||
      !federation->second.attributeHandles ||
      !validObjectClass(federation->second, requestedObjectClassHandle) ||
      !validObjectClassAttributes(
          federation->second,
          requestedObjectClassHandle,
          requestedAttributeHandles)) {
    return std::nullopt;
  }
  for (auto const& [attributeHandle, regionHandles] : requestRegionsByAttribute) {
    if (!requestedAttributeHandles.contains(attributeHandle) ||
        regionHandles.empty()) {
      return std::nullopt;
    }
    for (auto const regionHandle : regionHandles) {
      auto const region = federation->second.regions.find(regionHandle);
      if (region == federation->second.regions.end() ||
          region->second.ownerFederateId != requestingFederateId ||
          !region->second.specificationCommitted) {
        return std::nullopt;
      }
    }
  }
  auto instance = federation->second.objectInstances.find(objectInstanceHandle);
  if (instance == federation->second.objectInstances.end() ||
      instance->second.deleteAccepted ||
      !objectInstanceRegisteredAtOrBelowClass(
          federation->second,
          instance->second.registeredObjectClassHandle,
          requestedObjectClassHandle)) {
    return std::nullopt;
  }
  auto projection = candidateAttributeValueUpdateClassProvideRecipient(
      federation->second,
      requestingFederateId,
      providingFederateId,
      objectInstanceHandle,
      requestedObjectClassHandle,
      requestedAttributeHandles,
      &requestRegionsByAttribute);
  if (!projection ||
      projection->requestedAttributeHandles != requestedAttributeHandles) {
    return std::nullopt;
  }
  auto& nextRequestId = federation->second.nextAttributeValueUpdateRequestId;
  if (nextRequestId == 0U ||
      nextRequestId == std::numeric_limits<std::uint64_t>::max()) {
    return std::nullopt;
  }
  auto const requestId = nextRequestId++;
  auto const [position, inserted] =
      instance->second.pendingAttributeValueUpdateRegionalRequests.emplace(
          requestId,
          Federation::ObjectInstance::PendingAttributeValueUpdateRegionalRequest{
              requestingFederateId,
              providingFederateId,
              requestedObjectClassHandle,
              requestedAttributeHandles,
              requestRegionsByAttribute,
              std::move(userSuppliedTag),
          });
  static_cast<void>(position);
  if (!inserted) {
    return std::nullopt;
  }
  return requestId;
}

std::optional<AttributeValueUpdateProvideRecipient>
EmbeddedFederationRegistry::beginAttributeValueUpdateRegionalProvideRecipientFor(
    std::wstring const& federationName,
    std::uint64_t requestId,
    std::uint64_t requestingFederateId,
    std::uint64_t providingFederateId,
    std::uint64_t objectInstanceHandle,
    std::uint64_t requestedObjectClassHandle,
    std::set<std::uint64_t> const& requestedAttributeHandles,
    std::map<std::uint64_t, std::set<std::uint64_t>> const&
        requestRegionsByAttribute) {
  if (requestId == 0U) {
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
  auto pending = instance->second.pendingAttributeValueUpdateRegionalRequests.find(
      requestId);
  if (pending == instance->second.pendingAttributeValueUpdateRegionalRequests.end()) {
    return std::nullopt;
  }
  auto pendingState = std::move(pending->second);
  instance->second.pendingAttributeValueUpdateRegionalRequests.erase(pending);
  if (pendingState.requestingFederateId != requestingFederateId ||
      pendingState.providingFederateId != providingFederateId ||
      pendingState.requestedObjectClassHandle != requestedObjectClassHandle ||
      pendingState.requestedAttributeHandles != requestedAttributeHandles ||
      pendingState.requestRegionsByAttribute != requestRegionsByAttribute) {
    return std::nullopt;
  }
  return candidateAttributeValueUpdateClassProvideRecipient(
      federation->second,
      pendingState.requestingFederateId,
      pendingState.providingFederateId,
      objectInstanceHandle,
      pendingState.requestedObjectClassHandle,
      pendingState.requestedAttributeHandles,
      &pendingState.requestRegionsByAttribute);
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

}  // namespace umbra::detail
