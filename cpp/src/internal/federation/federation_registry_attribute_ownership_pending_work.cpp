#include "internal/federation/federation_registry.hpp"

#include <algorithm>
#include <cstdint>
#include <limits>
#include <map>
#include <ranges>
#include <set>
#include <tuple>
#include <utility>
#include <vector>

namespace umbra::detail {
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

void EmbeddedFederationRegistry::clearOwnershipAssumptionSearch(
    Federation::ObjectInstance& instance,
    std::uint64_t attributeHandle) {
  instance.ownershipAssumptionRecipientsByAttribute.erase(attributeHandle);
  instance.ownershipAssumptionUserSuppliedTagsByAttribute.erase(attributeHandle);
  for (auto pending = instance.pendingAttributeOwnershipAssumptionCallbacks.begin();
       pending != instance.pendingAttributeOwnershipAssumptionCallbacks.end();) {
    pending->attributeHandles.erase(attributeHandle);
    if (pending->attributeHandles.empty()) {
      pending = instance.pendingAttributeOwnershipAssumptionCallbacks.erase(pending);
    } else {
      ++pending;
    }
  }
}

void EmbeddedFederationRegistry::clearPendingAttributeOwnershipQueries(
    Federation& federation,
    std::uint64_t objectInstanceHandle) {
  for (auto query = federation.pendingAttributeOwnershipQueries.begin();
       query != federation.pendingAttributeOwnershipQueries.end();) {
    if (query->second.objectInstanceHandle == objectInstanceHandle) {
      query = federation.pendingAttributeOwnershipQueries.erase(query);
    } else {
      ++query;
    }
  }
}

std::vector<AttributeOwnershipAcquisitionWorkItem>
EmbeddedFederationRegistry::planPendingNegotiatedAttributeOwnershipDivestitureConfirmations(
    Federation& federation,
    Federation::ObjectInstance& instance) {
  // Preserve the federation-wide acquisition request sequence when several
  // candidate kinds are selected for one negotiated divestiture.  Regular and
  // If Available request IDs use independent namespaces, so candidate kind
  // remains part of the key; sequence is the cross-namespace ordering signal.
  std::map<std::tuple<std::uint64_t, std::uint64_t, bool, std::uint64_t>,
           std::set<std::uint64_t>>
      attributesByDivesterAndRequest;

  for (auto& [attributeHandle, divestiture] :
       instance.pendingNegotiatedAttributeOwnershipDivestitures) {
    auto const owner = instance.attributeOwnersByHandle.find(attributeHandle);
    if (owner == instance.attributeOwnersByHandle.end() ||
        owner->second != divestiture.divestingFederateId ||
        !federation.members.contains(divestiture.divestingFederateId)) {
      continue;
    }

    bool selectedIfAvailable = divestiture.acquiringFederateIsIfAvailable;
    std::uint64_t selectedRequestId = divestiture.acquisitionRequestId;
    bool selected = false;
    if (divestiture.acquisitionRequestId != 0) {
      if (selectedIfAvailable) {
        auto const selectedRequest =
            instance.pendingAttributeOwnershipAcquisitionIfAvailableRequests.find(
                divestiture.acquisitionRequestId);
        selected = selectedRequest !=
                       instance.pendingAttributeOwnershipAcquisitionIfAvailableRequests.end() &&
                   selectedRequest->second.requestingFederateId ==
                       divestiture.acquiringFederateId &&
                   selectedRequest->second.desiredAttributeHandles.contains(attributeHandle);
      } else {
        auto const selectedRequest = instance.pendingAttributeOwnershipAcquisitionRequests.find(
            divestiture.acquisitionRequestId);
        selected = selectedRequest != instance.pendingAttributeOwnershipAcquisitionRequests.end() &&
                   selectedRequest->second.requestingFederateId ==
                       divestiture.acquiringFederateId &&
                   selectedRequest->second.desiredAttributeHandles.contains(attributeHandle) &&
                   !std::ranges::any_of(
                       instance.pendingAttributeOwnershipAcquisitionCancellations,
                       [&](auto const& cancellation) {
                         return cancellation.second.requestingFederateId ==
                                    divestiture.acquiringFederateId &&
                                cancellation.second.attributeHandles.contains(attributeHandle);
                       });
      }
      if (!selected || !federation.members.contains(divestiture.acquiringFederateId) ||
          !instance.knownObjectClassHandlesByFederate.contains(divestiture.acquiringFederateId)) {
        selected = false;
        divestiture.acquiringFederateId = 0;
        divestiture.acquisitionRequestId = 0;
        divestiture.acquiringFederateIsIfAvailable = false;
        divestiture.confirmationQueued = false;
        divestiture.confirmationDelivered = false;
      }
    }

    if (!selected) {
      std::uint64_t selectedSequence = std::numeric_limits<std::uint64_t>::max();
      auto considerCandidate = [&](std::uint64_t candidateRequestId,
                                   bool candidateIfAvailable,
                                   std::uint64_t candidateFederateId,
                                   std::uint64_t candidateSequence,
                                   bool containsAttribute) {
        if (!containsAttribute || !federation.members.contains(candidateFederateId) ||
            !instance.knownObjectClassHandlesByFederate.contains(candidateFederateId) ||
            (!candidateIfAvailable &&
             std::ranges::any_of(
                 instance.pendingAttributeOwnershipAcquisitionCancellations,
                 [&](auto const& cancellation) {
                   return cancellation.second.requestingFederateId == candidateFederateId &&
                          cancellation.second.attributeHandles.contains(attributeHandle);
                 }))) {
          return;
        }
        if (!selected || candidateSequence < selectedSequence ||
            (candidateSequence == selectedSequence &&
             std::tie(candidateIfAvailable, candidateRequestId) <
                 std::tie(selectedIfAvailable, selectedRequestId))) {
          selected = true;
          selectedIfAvailable = candidateIfAvailable;
          selectedRequestId = candidateRequestId;
          divestiture.acquiringFederateId = candidateFederateId;
          selectedSequence = candidateSequence;
        }
      };

      for (auto const& [candidateRequestId, candidate] :
           instance.pendingAttributeOwnershipAcquisitionRequests) {
        considerCandidate(
            candidateRequestId,
            false,
            candidate.requestingFederateId,
            candidate.requestSequence,
            candidate.desiredAttributeHandles.contains(attributeHandle));
      }
      for (auto const& [candidateRequestId, candidate] :
           instance.pendingAttributeOwnershipAcquisitionIfAvailableRequests) {
        considerCandidate(
            candidateRequestId,
            true,
            candidate.requestingFederateId,
            candidate.requestSequence,
            candidate.desiredAttributeHandles.contains(attributeHandle));
      }
      if (!selected) {
        continue;
      }
      divestiture.acquiringFederateIsIfAvailable = selectedIfAvailable;
      divestiture.acquisitionRequestId = selectedRequestId;
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
    std::uint64_t selectedRequestSequence = 0;
    if (divestiture.acquiringFederateIsIfAvailable) {
      auto const pending = instance.pendingAttributeOwnershipAcquisitionIfAvailableRequests.find(
          divestiture.acquisitionRequestId);
      if (pending == instance.pendingAttributeOwnershipAcquisitionIfAvailableRequests.end()) {
        continue;
      }
      selectedRequestSequence = pending->second.requestSequence;
    } else {
      auto const pending = instance.pendingAttributeOwnershipAcquisitionRequests.find(
          divestiture.acquisitionRequestId);
      if (pending == instance.pendingAttributeOwnershipAcquisitionRequests.end()) {
        continue;
      }
      selectedRequestSequence = pending->second.requestSequence;
    }
    if (selectedRequestSequence == 0) {
      continue;
    }
    attributesByDivesterAndRequest[
        {divestiture.divestingFederateId,
         selectedRequestSequence,
         divestiture.acquiringFederateIsIfAvailable,
         divestiture.acquisitionRequestId}]
        .insert(attributeHandle);
    divestiture.confirmationQueued = true;
  }

  std::vector<AttributeOwnershipAcquisitionWorkItem> workItems;
  workItems.reserve(attributesByDivesterAndRequest.size());
  for (auto& [key, attributeHandles] : attributesByDivesterAndRequest) {
    auto const [divestingFederateId,
                selectedRequestSequence,
                candidateIfAvailable,
                acquisitionRequestId] = key;
    static_cast<void>(selectedRequestSequence);
    std::uint64_t acquiringFederateId = 0;
    std::vector<unsigned char> userSuppliedTag;
    if (candidateIfAvailable) {
      auto const pending = instance.pendingAttributeOwnershipAcquisitionIfAvailableRequests.find(
          acquisitionRequestId);
      if (pending == instance.pendingAttributeOwnershipAcquisitionIfAvailableRequests.end()) {
        continue;
      }
      acquiringFederateId = pending->second.requestingFederateId;
      userSuppliedTag = pending->second.userSuppliedTag;
    } else {
      auto const pending = instance.pendingAttributeOwnershipAcquisitionRequests.find(
          acquisitionRequestId);
      if (pending == instance.pendingAttributeOwnershipAcquisitionRequests.end()) {
        continue;
      }
      acquiringFederateId = pending->second.requestingFederateId;
      userSuppliedTag = pending->second.userSuppliedTag;
    }
    auto const callbackRoute = federation.interactionCallbackRoutes.find(divestingFederateId);
    if (callbackRoute == federation.interactionCallbackRoutes.end() || !callbackRoute->second ||
        attributeHandles.empty()) {
      continue;
    }
    workItems.push_back({
        AttributeOwnershipAcquisitionWorkKind::request_divestiture_confirmation,
        acquiringFederateId,
        divestingFederateId,
        instance.handle,
        acquisitionRequestId,
        std::move(attributeHandles),
        std::move(userSuppliedTag),
        candidateIfAvailable,
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

  auto cancellationPending = [&instance](
                                 std::uint64_t requestingFederateId,
                                 std::uint64_t attributeHandle) {
    for (auto const& [cancellationId, cancellation] :
         instance.pendingAttributeOwnershipAcquisitionCancellations) {
      static_cast<void>(cancellationId);
      if (cancellation.requestingFederateId == requestingFederateId &&
          cancellation.attributeHandles.contains(attributeHandle)) {
        return true;
      }
    }
    return false;
  };

  for (auto& [requestId, pending] : instance.pendingAttributeOwnershipAcquisitionRequests) {
    if (!federation.members.contains(pending.requestingFederateId) ||
        !instance.knownObjectClassHandlesByFederate.contains(pending.requestingFederateId)) {
      continue;
    }

    std::set<std::uint64_t> notificationAttributes;
    for (std::uint64_t const attributeHandle : pending.desiredAttributeHandles) {
      if (instance.attributeOwnersByHandle.contains(attributeHandle) ||
          pending.notificationQueuedAttributeHandles.contains(attributeHandle) ||
          pending.unavailableQueuedAttributeHandles.contains(attributeHandle) ||
          cancellationPending(pending.requestingFederateId, attributeHandle) ||
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
            false,
            callbackRoute->second,
        });
      }
    }

    std::map<std::uint64_t, std::set<std::uint64_t>> releaseAttributesByOwner;
    for (std::uint64_t const attributeHandle : pending.desiredAttributeHandles) {
      auto const owner = instance.attributeOwnersByHandle.find(attributeHandle);
      if (owner == instance.attributeOwnersByHandle.end() ||
          owner->second == pending.requestingFederateId ||
          !federation.members.contains(owner->second) ||
          pending.unavailableQueuedAttributeHandles.contains(attributeHandle) ||
          cancellationPending(pending.requestingFederateId, attributeHandle)) {
        continue;
      }
      // A waiting negotiated divestiture replaces the ordinary owner-side
      // release request with Request Divestiture Confirmation for the selected
      // regular or If Available acquisition. A previously queued release work item is
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
          false,
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

}  // namespace umbra::detail
