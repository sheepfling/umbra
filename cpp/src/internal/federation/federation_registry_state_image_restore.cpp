#include "internal/federation/federation_registry.hpp"
#include "internal/federation/federation_state_image.hpp"
#include "internal/federation/federation_registry_value_helpers.hpp"
#include "internal/federation/federation_registry_state_image_restore_helpers.hpp"

#include <algorithm>
#include <map>
#include <memory>
#include <optional>
#include <set>
#include <stdexcept>
#include <string>
#include <utility>
#include <vector>

namespace umbra::detail {

using federation_registry_state_image_restore_helpers::decodeLogicalTimeEncoding;
using federation_registry_state_image_restore_helpers::decodeLogicalTimeIntervalEncoding;

void EmbeddedFederationRegistry::restoreControlAndTimeFromStateImage(
    Federation& federation,
    FederationStateImage const& image,
    Federation const& liveFederation) {
  if (image.federationName.empty()) {
    // The federation name is checked by restore admission. Keep this branch
    // intentionally narrow so this helper cannot be used with an unbound
    // image by a future caller.
    throw std::logic_error("The saved federation control image has no identity.");
  }
  if (image.members.size() != liveFederation.members.size()) {
    throw std::logic_error(
        "The saved federation control image has a different member count.");
  }

  auto decodeResignAction = [](std::uint32_t value) {
    if (value > static_cast<std::uint32_t>(rti1516_2025::NO_ACTION)) {
      throw std::logic_error(
          "The saved federation control image has an invalid resign action.");
    }
    return static_cast<rti1516_2025::ResignAction>(value);
  };
  auto applySwitches = [](std::uint32_t switches,
                          FederateMembership& member) {
    member.objectClassRelevanceAdvisorySwitch = (switches & (1U << 0U)) != 0U;
    member.attributeScopeAdvisorySwitch = (switches & (1U << 1U)) != 0U;
    member.attributeRelevanceAdvisorySwitch = (switches & (1U << 2U)) != 0U;
    member.interactionRelevanceAdvisorySwitch = (switches & (1U << 3U)) != 0U;
    member.conveyRegionDesignatorSetsSwitch = (switches & (1U << 4U)) != 0U;
    member.serviceReportingSwitch = (switches & (1U << 5U)) != 0U;
    member.exceptionReportingSwitch = (switches & (1U << 6U)) != 0U;
    member.sendServiceReportsToFileSwitch = (switches & (1U << 7U)) != 0U;
  };

  federation.normalizationSeed = image.normalizationSeed;
  federation.autoProvideSwitch = (image.federationSwitches & (1U << 0U)) != 0U;
  federation.advisoriesUseKnownClassSwitch =
      (image.federationSwitches & (1U << 1U)) != 0U;
  federation.nonRegulatedGrantSwitch =
      (image.federationSwitches & (1U << 2U)) != 0U;
  federation.delaySubscriptionEvaluationSwitch =
      (image.federationSwitches & (1U << 3U)) != 0U;
  federation.allowRelaxedDDMSwitch =
      (image.federationSwitches & (1U << 4U)) != 0U;

  if (image.saveHistoryPresent) {
    if (image.lastSaveName.empty() && image.lastSaveTimeEncoding.has_value()) {
      throw std::logic_error(
          "The saved federation save history has a last-save time without a name.");
    }
    if (image.nextSaveName.empty() && image.nextSaveTimeEncoding.has_value()) {
      throw std::logic_error(
          "The saved federation save history has a next-save time without a name.");
    }
    federation.lastSaveName = image.lastSaveName;
    federation.lastSaveTime = decodeLogicalTimeEncoding(
        image.logicalTimeImplementationName,
        image.lastSaveTimeEncoding);
    federation.nextSaveName = image.nextSaveName;
    federation.nextSaveTime = decodeLogicalTimeEncoding(
        image.logicalTimeImplementationName,
        image.nextSaveTimeEncoding);
  }

  for (auto const& savedMember : image.members) {
    auto member = federation.members.find(savedMember.id);
    auto const liveMember = liveFederation.members.find(savedMember.id);
    if (member == federation.members.end() || liveMember == liveFederation.members.end() ||
        member->second.name != savedMember.name ||
        member->second.type != savedMember.type) {
      throw std::logic_error(
          "The saved federation control image has no matching live member.");
    }
    applySwitches(savedMember.switches, member->second);
    member->second.automaticResignAction =
        decodeResignAction(savedMember.automaticResignAction);
    member->second.momReportPeriodSeconds = savedMember.momReportPeriodSeconds;
    member->second.nextMomReportAt.reset();
    member->second.nextMomServiceReportSerialNumber =
        savedMember.nextMomServiceReportSerialNumber;
  }

  federation.federateNamesById.clear();
  std::uint64_t previousFederateId = 0U;
  for (auto const& [federateId, federateName] : image.federateNamesById) {
    if (federateId == 0U || federateName.empty() || federateId <= previousFederateId) {
      throw std::logic_error(
          "The saved federation control image has an invalid federate-name index.");
    }
    federation.federateNamesById.emplace(federateId, federateName);
    previousFederateId = federateId;
  }

  federation.nextRegionHandle = image.nextRegionHandle;
  federation.nextSubscriptionGeneration = image.nextSubscriptionGeneration;
  federation.nextObjectInstanceHandle = image.nextObjectInstanceHandle;
  federation.nextAttributeOwnershipAcquisitionIfAvailableRequestId =
      image.nextAttributeOwnershipAcquisitionIfAvailableRequestId;
  federation.nextAttributeOwnershipAcquisitionRequestId =
      image.nextAttributeOwnershipAcquisitionRequestId;
  federation.nextAttributeOwnershipAcquisitionRequestSequence =
      image.nextAttributeOwnershipAcquisitionRequestSequence;
  federation.nextAttributeOwnershipAcquisitionCancellationId =
      image.nextAttributeOwnershipAcquisitionCancellationId;
  federation.nextAttributeOwnershipDivestitureIfWantedNotificationId =
      image.nextAttributeOwnershipDivestitureIfWantedNotificationId;
  federation.nextConfirmDivestitureNotificationId =
      image.nextConfirmDivestitureNotificationId;
  federation.nextAttributeTransportationTypeChangeRequestId =
      image.nextAttributeTransportationTypeChangeRequestId;
  federation.nextAttributeValueUpdateRequestId =
      image.nextAttributeValueUpdateRequestId;
  federation.nextAttributeOwnershipQueryRequestId =
      image.nextAttributeOwnershipQueryRequestId;
  federation.nextTimeAdvanceGrantDispatchIdentity =
      image.nextTimeAdvanceGrantDispatchIdentity;

  auto const liveTimeStates = liveFederation.timeCoordinator.snapshot();
  if (liveTimeStates.size() != image.timeStates.size()) {
    throw std::logic_error(
        "The saved federation control image has a different temporal member set.");
  }
  for (auto const& savedTime : image.timeStates) {
    auto const state = federation.timeCoordinator.timeStateFor(savedTime.federateId);
    if (!state || savedTime.implementationName != image.logicalTimeImplementationName) {
      throw std::logic_error(
          "The saved federation control image has no matching live temporal state.");
    }
    if (savedTime.advanceMode >
        static_cast<std::uint32_t>(FederateTimeAdvanceMode::flush_queue_request)) {
      throw std::logic_error(
          "The saved federation control image has an invalid advance mode.");
    }
    FederateTimeSnapshot restoredTime;
    restoredTime.implementationName = savedTime.implementationName;
    restoredTime.active = (savedTime.flags & (1U << 0U)) != 0U;
    restoredTime.timeRegulating = (savedTime.flags & (1U << 1U)) != 0U;
    restoredTime.timeConstrained = (savedTime.flags & (1U << 2U)) != 0U;
    restoredTime.asynchronousDeliveryEnabled = (savedTime.flags & (1U << 3U)) != 0U;
    restoredTime.timeAdvancePending = (savedTime.flags & (1U << 4U)) != 0U;
    restoredTime.pendingTimeAdvanceGeneration = savedTime.pendingGeneration;
    restoredTime.advanceMode = static_cast<FederateTimeAdvanceMode>(savedTime.advanceMode);
    restoredTime.timeRegulationPending = (savedTime.flags & (1U << 5U)) != 0U;
    restoredTime.timeConstrainedPending = (savedTime.flags & (1U << 6U)) != 0U;
    restoredTime.pendingTimeRegulationGeneration =
        savedTime.pendingTimeRegulationGeneration;
    restoredTime.pendingTimeConstrainedGeneration =
        savedTime.pendingTimeConstrainedGeneration;
    restoredTime.nextGeneration = savedTime.nextGeneration;
    restoredTime.minimumTimestampIsExclusive =
        (savedTime.flags & (1U << 7U)) != 0U;
    restoredTime.currentTime = decodeLogicalTimeEncoding(
        image.logicalTimeImplementationName,
        savedTime.currentTimeEncoding);
    restoredTime.optimisticTime = decodeLogicalTimeEncoding(
        image.logicalTimeImplementationName,
        savedTime.optimisticTimeEncoding);
    restoredTime.requestedTime = decodeLogicalTimeEncoding(
        image.logicalTimeImplementationName,
        savedTime.requestedTimeEncoding);
    restoredTime.advanceRequestTime = decodeLogicalTimeEncoding(
        image.logicalTimeImplementationName,
        savedTime.advanceRequestTimeEncoding);
    restoredTime.lookahead = decodeLogicalTimeIntervalEncoding(
        image.logicalTimeImplementationName,
        savedTime.lookaheadEncoding);
    restoredTime.requestedLookahead = decodeLogicalTimeIntervalEncoding(
        image.logicalTimeImplementationName,
        savedTime.requestedLookaheadEncoding);
    restoredTime.pendingModifiedLookahead = decodeLogicalTimeIntervalEncoding(
        image.logicalTimeImplementationName,
        savedTime.pendingModifiedLookaheadEncoding);
    state->restoreFromSnapshot(restoredTime);
  }
}

void EmbeddedFederationRegistry::restoreMemberInteractionReceiptTelemetryFromStateImage(
    Federation& federation,
    FederationStateImage const& image,
    Federation const& liveFederation) {
  for (auto const& savedMember : image.members) {
    auto member = federation.members.find(savedMember.id);
    if (member == federation.members.end() ||
        !liveFederation.members.contains(savedMember.id)) {
      throw std::logic_error(
          "The saved member interaction-receipt telemetry has no live joined federate.");
    }

    bool const hasTypedTelemetry = savedMember.interactionReceiptTelemetryPresent ||
        savedMember.successfulInteractionsReceivedCount != 0U ||
        savedMember.successfulDirectedInteractionsReceivedCount != 0U ||
        !savedMember.successfulInteractionReceiptCountsByClassAndTransportation.empty() ||
        !savedMember.successfulDirectedInteractionReceiptCountsByClassAndTransportation.empty();
    if (!hasTypedTelemetry) {
      continue;
    }
    if (savedMember.successfulDirectedInteractionsReceivedCount >
        savedMember.successfulInteractionsReceivedCount) {
      throw std::logic_error(
          "The saved directed interaction-receipt count exceeds the total count.");
    }

    std::map<std::uint64_t, std::map<std::string, std::uint64_t>> receiptCounts;
    for (auto const& savedCount :
         savedMember.successfulInteractionReceiptCountsByClassAndTransportation) {
      if (savedCount.interactionClassHandle == 0U || savedCount.count == 0U ||
          !isSupportedTransportationName(
              federation.definition.catalog.get(), savedCount.transportationName)) {
        throw std::logic_error(
            "The saved interaction-receipt telemetry has an invalid class or transportation.");
      }
      auto& count = receiptCounts[savedCount.interactionClassHandle][
          savedCount.transportationName];
      if (count != 0U) {
        throw std::logic_error(
            "The saved interaction-receipt telemetry has duplicate class/transport buckets.");
      }
      count = savedCount.count;
    }

    std::map<std::uint64_t, std::map<std::string, std::uint64_t>> directedCounts;
    for (auto const& savedCount :
         savedMember.successfulDirectedInteractionReceiptCountsByClassAndTransportation) {
      if (savedCount.interactionClassHandle == 0U || savedCount.count == 0U ||
          !isSupportedTransportationName(
              federation.definition.catalog.get(), savedCount.transportationName)) {
        throw std::logic_error(
            "The saved directed interaction-receipt telemetry has an invalid class or transportation.");
      }
      auto const totalClass = receiptCounts.find(savedCount.interactionClassHandle);
      auto const total = totalClass == receiptCounts.end()
          ? 0U
          : totalClass->second.contains(savedCount.transportationName)
          ? totalClass->second.at(savedCount.transportationName)
          : 0U;
      if (total == 0U || savedCount.count > total) {
        throw std::logic_error(
            "The saved directed interaction-receipt bucket is not a total-receipt subset.");
      }
      auto& count = directedCounts[savedCount.interactionClassHandle][
          savedCount.transportationName];
      if (count != 0U) {
        throw std::logic_error(
            "The saved directed interaction-receipt telemetry has duplicate buckets.");
      }
      count = savedCount.count;
    }

    member->second.successfulInteractionsReceivedCount =
        savedMember.successfulInteractionsReceivedCount;
    member->second.successfulDirectedInteractionsReceivedCount =
        savedMember.successfulDirectedInteractionsReceivedCount;
    member->second.successfulInteractionReceiptCountsByClassAndTransportation =
        std::move(receiptCounts);
    member->second.successfulDirectedInteractionReceiptCountsByClassAndTransportation =
        std::move(directedCounts);
  }
}

void EmbeddedFederationRegistry::restoreObjectInstanceNameReservationsFromStateImage(
    Federation& federation,
    FederationStateImage const& image,
    Federation const& liveFederation) {
  if (!image.reservedObjectInstanceNamesPresent) {
    return;
  }

  federation.reservedObjectInstanceNamesByFederate.clear();
  for (auto const& reservation : image.reservedObjectInstanceNames) {
    if (reservation.federateId == 0U || reservation.objectInstanceName.empty() ||
        !federation.members.contains(reservation.federateId) ||
        !liveFederation.members.contains(reservation.federateId)) {
      throw std::logic_error(
          "The saved object-instance-name reservation has no live joined federate.");
    }
    if (federation.objectInstanceHandlesByName.contains(
            reservation.objectInstanceName)) {
      throw std::logic_error(
          "The saved object-instance-name reservation conflicts with a registered object.");
    }
    auto const [position, inserted] =
        federation.reservedObjectInstanceNamesByFederate.emplace(
            reservation.objectInstanceName,
            reservation.federateId);
    static_cast<void>(position);
    if (!inserted) {
      throw std::logic_error(
          "The saved object-instance-name reservations contain a duplicate name.");
    }
  }
}

void EmbeddedFederationRegistry::restoreRegionsFromStateImage(
    Federation& federation,
    FederationStateImage const& image,
    Federation const& liveFederation) {
  if (!image.regionsPresent) {
    return;
  }
  if (image.regions.empty()) {
    federation.regions.clear();
    return;
  }
  if (!federation.definition.catalog || !federation.dimensionHandles) {
    throw std::logic_error(
        "The saved regions cannot be resolved against the restored DDM catalog.");
  }

  std::map<std::uint64_t, Federation::Region> restored;
  auto const maximumRangeBound = static_cast<std::uint64_t>(
      std::numeric_limits<unsigned long>::max());
  for (auto const& savedRegion : image.regions) {
    if (savedRegion.handle == 0U || savedRegion.ownerFederateId == 0U ||
        !federation.members.contains(savedRegion.ownerFederateId) ||
        !liveFederation.members.contains(savedRegion.ownerFederateId)) {
      throw std::logic_error(
          "The saved region has no live joined owner.");
    }

    Federation::Region region;
    region.ownerFederateId = savedRegion.ownerFederateId;
    for (auto const dimensionHandle : savedRegion.dimensionHandles) {
      auto const dimensionName = federation.dimensionHandles->nameFor(dimensionHandle);
      if (!dimensionName ||
          federation.definition.catalog->dimension(*dimensionName) == nullptr ||
          !region.dimensionHandles.insert(dimensionHandle).second) {
        throw std::logic_error(
            "The saved region references an unknown or duplicate dimension.");
      }
    }

    auto restoreRanges = [&](std::vector<FederationStateImageRegionRange> const& ranges,
                             std::map<std::uint64_t, RegionRangeBounds>& target,
                             char const* field) {
      for (auto const& savedRange : ranges) {
        if (savedRange.dimensionHandle == 0U ||
            !region.dimensionHandles.contains(savedRange.dimensionHandle) ||
            savedRange.lowerBound >= savedRange.upperBound ||
            savedRange.upperBound > maximumRangeBound) {
          throw std::logic_error(std::string{"The saved region has an invalid "} + field +
                                 ".");
        }
        auto const dimensionName = federation.dimensionHandles->nameFor(
            savedRange.dimensionHandle);
        auto const* dimension = dimensionName
            ? federation.definition.catalog->dimension(*dimensionName)
            : nullptr;
        if (dimension == nullptr ||
            savedRange.upperBound > static_cast<std::uint64_t>(dimension->upperBound)) {
          throw std::logic_error(std::string{"The saved region has an out-of-range "} +
                                 field + ".");
        }
        auto const [position, inserted] = target.emplace(
            savedRange.dimensionHandle,
            RegionRangeBounds{
                static_cast<unsigned long>(savedRange.lowerBound),
                static_cast<unsigned long>(savedRange.upperBound)});
        static_cast<void>(position);
        if (!inserted) {
          throw std::logic_error(std::string{"The saved region has duplicate "} + field +
                                 ".");
        }
      }
    };
    restoreRanges(
        savedRegion.pendingRangeBounds,
        region.pendingRangeBounds,
        "pending range");
    restoreRanges(
        savedRegion.committedRangeBounds,
        region.committedRangeBounds,
        "committed range");
    if (savedRegion.specificationCommitted &&
        region.committedRangeBounds.size() != region.dimensionHandles.size()) {
      throw std::logic_error(
          "The saved committed region has incomplete range bounds.");
    }
    region.specificationCommitted = savedRegion.specificationCommitted;
    region.inUse = savedRegion.inUse;

    auto const [position, inserted] = restored.emplace(savedRegion.handle, std::move(region));
    static_cast<void>(position);
    if (!inserted) {
      throw std::logic_error("The saved regions contain a duplicate handle.");
    }
  }
  federation.regions = std::move(restored);
}

void EmbeddedFederationRegistry::restoreSynchronizationPointsFromStateImage(
    Federation& federation,
    FederationStateImage const& image,
    Federation const& liveFederation) {
  if (!image.synchronizationPointsPresent) {
    return;
  }

  std::map<std::wstring, Federation::SynchronizationPoint> restored;
  for (auto const& savedPoint : image.synchronizationPoints) {
    if (savedPoint.synchronizationSet.empty()) {
      throw std::logic_error(
          "The saved synchronization point has an empty synchronization set.");
    }

    Federation::SynchronizationPoint point;
    point.userSuppliedTag.assign(
        savedPoint.userSuppliedTag.begin(),
        savedPoint.userSuppliedTag.end());
    point.synchronizationSet.insert(
        savedPoint.synchronizationSet.begin(),
        savedPoint.synchronizationSet.end());
    point.announcedFederates.insert(
        savedPoint.announcedFederates.begin(),
        savedPoint.announcedFederates.end());
    point.lateJoinExpansionAllowed = savedPoint.lateJoinExpansionAllowed;

    for (auto const federateId : point.synchronizationSet) {
      if (!federation.members.contains(federateId) ||
          !liveFederation.members.contains(federateId)) {
        throw std::logic_error(
            "The saved synchronization point has no live joined participant.");
      }
    }
    for (auto const federateId : point.announcedFederates) {
      if (!point.synchronizationSet.contains(federateId)) {
        throw std::logic_error(
            "The saved synchronization point announces a non-member.");
      }
    }
    for (auto const& [federateId, succeeded] : savedPoint.achievedFederates) {
      if (!point.synchronizationSet.contains(federateId) ||
          !point.announcedFederates.contains(federateId)) {
        throw std::logic_error(
            "The saved synchronization point has an achievement outside its announced set.");
      }
      auto const [position, inserted] =
          point.achievedFederates.emplace(federateId, succeeded);
      static_cast<void>(position);
      if (!inserted) {
        throw std::logic_error(
            "The saved synchronization point contains a duplicate achievement.");
      }
    }

    auto const [position, inserted] =
        restored.emplace(savedPoint.label, std::move(point));
    static_cast<void>(position);
    if (!inserted) {
      throw std::logic_error(
          "The saved synchronization points contain a duplicate label.");
    }
  }
  federation.synchronizationPoints = std::move(restored);
}

void EmbeddedFederationRegistry::restoreMemberInteractionSendTelemetryFromStateImage(
    Federation& federation,
    FederationStateImage const& image,
    Federation const& liveFederation) {
  for (auto const& savedMember : image.members) {
    auto member = federation.members.find(savedMember.id);
    if (member == federation.members.end() ||
        !liveFederation.members.contains(savedMember.id)) {
      throw std::logic_error(
          "The saved member interaction-send telemetry has no live joined federate.");
    }

    bool const hasTypedTelemetry = savedMember.interactionSendTelemetryPresent ||
        savedMember.successfulInteractionsSentCount != 0U ||
        savedMember.successfulDirectedInteractionsSentCount != 0U ||
        !savedMember.successfulInteractionCountsByClassAndTransportation.empty() ||
        !savedMember.successfulDirectedInteractionCountsByClassAndTransportation.empty();
    if (!hasTypedTelemetry) {
      continue;
    }
    if (savedMember.successfulDirectedInteractionsSentCount >
        savedMember.successfulInteractionsSentCount) {
      throw std::logic_error(
          "The saved directed interaction-send count exceeds the total count.");
    }

    std::map<std::uint64_t, std::map<std::string, std::uint64_t>> interactionCounts;
    for (auto const& savedCount :
         savedMember.successfulInteractionCountsByClassAndTransportation) {
      if (savedCount.interactionClassHandle == 0U || savedCount.count == 0U ||
          !isSupportedTransportationName(
              federation.definition.catalog.get(), savedCount.transportationName)) {
        throw std::logic_error(
            "The saved interaction-send telemetry has an invalid class or transportation.");
      }
      auto& count = interactionCounts[savedCount.interactionClassHandle][
          savedCount.transportationName];
      if (count != 0U) {
        throw std::logic_error(
            "The saved interaction-send telemetry has duplicate class/transport buckets.");
      }
      count = savedCount.count;
    }

    std::map<std::uint64_t, std::map<std::string, std::uint64_t>> directedCounts;
    for (auto const& savedCount :
         savedMember.successfulDirectedInteractionCountsByClassAndTransportation) {
      if (savedCount.interactionClassHandle == 0U || savedCount.count == 0U ||
          !isSupportedTransportationName(
              federation.definition.catalog.get(), savedCount.transportationName)) {
        throw std::logic_error(
            "The saved directed interaction-send telemetry has an invalid class or transportation.");
      }
      auto const totalClass = interactionCounts.find(savedCount.interactionClassHandle);
      auto const total = totalClass == interactionCounts.end()
          ? 0U
          : totalClass->second.contains(savedCount.transportationName)
          ? totalClass->second.at(savedCount.transportationName)
          : 0U;
      if (total == 0U || savedCount.count > total) {
        throw std::logic_error(
            "The saved directed interaction-send bucket is not a total-send subset.");
      }
      auto& count = directedCounts[savedCount.interactionClassHandle][
          savedCount.transportationName];
      if (count != 0U) {
        throw std::logic_error(
            "The saved directed interaction-send telemetry has duplicate buckets.");
      }
      count = savedCount.count;
    }

    member->second.successfulInteractionsSentCount =
        savedMember.successfulInteractionsSentCount;
    member->second.successfulDirectedInteractionsSentCount =
        savedMember.successfulDirectedInteractionsSentCount;
    member->second.successfulInteractionCountsByClassAndTransportation =
        std::move(interactionCounts);
    member->second.successfulDirectedInteractionCountsByClassAndTransportation =
        std::move(directedCounts);
  }
}

void EmbeddedFederationRegistry::restoreMemberObjectLifecycleTelemetryFromStateImage(
    Federation& federation,
    FederationStateImage const& image,
    Federation const& liveFederation) {
  for (auto const& savedMember : image.members) {
    auto member = federation.members.find(savedMember.id);
    if (member == federation.members.end() ||
        !liveFederation.members.contains(savedMember.id)) {
      throw std::logic_error(
          "The saved member object-lifecycle telemetry has no live joined federate.");
    }

    // Older seven-, eleven-, and twelve-field records have no lifecycle
    // scalars. A current sixteen-field record explicitly carries all four,
    // including zeros, so restore never retains post-save live counters.
    if (!savedMember.objectLifecycleTelemetryPresent &&
        savedMember.successfulObjectInstanceRegistrationsCount == 0U &&
        savedMember.successfulObjectInstanceDeletionsCount == 0U &&
        savedMember.successfulObjectInstanceRemovalsCount == 0U &&
        savedMember.successfulObjectInstanceDiscoveriesCount == 0U) {
      continue;
    }
    member->second.successfulObjectInstanceRegistrationsCount =
        savedMember.successfulObjectInstanceRegistrationsCount;
    member->second.successfulObjectInstanceDeletionsCount =
        savedMember.successfulObjectInstanceDeletionsCount;
    member->second.successfulObjectInstanceRemovalsCount =
        savedMember.successfulObjectInstanceRemovalsCount;
    member->second.successfulObjectInstanceDiscoveriesCount =
        savedMember.successfulObjectInstanceDiscoveriesCount;
  }
}

void EmbeddedFederationRegistry::restoreMemberReflectionTelemetryFromStateImage(
    Federation& federation,
    FederationStateImage const& image,
    Federation const& liveFederation) {
  for (auto const& savedMember : image.members) {
    auto member = federation.members.find(savedMember.id);
    if (member == federation.members.end() ||
        !liveFederation.members.contains(savedMember.id)) {
      throw std::logic_error(
          "The saved member reflection telemetry has no live joined federate.");
    }

    // Seven- and eleven-field v1 images predate the scalar reflection
    // extension. A decoded twelve-field (or later) record explicitly carries
    // the value, including zero, so restore does not accidentally retain
    // post-save live state when the saved statistic was zero.
    bool const hasScalar = savedMember.reflectionTelemetryPresent ||
        savedMember.successfulReflectionsReceivedCount != 0U;
    bool const hasProjection =
        savedMember.reflectionProjectionTelemetryPresent ||
        !savedMember.successfulReflectionCountsByClassAndTransportation.empty() ||
        !savedMember.successfullyReflectedObjectInstanceHandles.empty() ||
        !savedMember.successfullyReflectedObjectInstanceClassHandles.empty();
    if (!hasScalar && !hasProjection) {
      continue;
    }
    if (hasScalar) {
      member->second.successfulReflectionsReceivedCount =
          savedMember.successfulReflectionsReceivedCount;
    }

    if (hasProjection) {
      std::map<std::uint64_t, std::map<std::string, std::uint64_t>> reflectionCounts;
      for (auto const& savedCount :
           savedMember.successfulReflectionCountsByClassAndTransportation) {
        if (savedCount.objectClassHandle == 0U || savedCount.count == 0U ||
            !isSupportedTransportationName(
                federation.definition.catalog.get(), savedCount.transportationName)) {
          throw std::logic_error(
              "The saved member reflection telemetry has an invalid class or transportation.");
        }
        auto& count = reflectionCounts[savedCount.objectClassHandle][
            savedCount.transportationName];
        if (count != 0U) {
          throw std::logic_error(
              "The saved member reflection telemetry has duplicate class/transport buckets.");
        }
        count = savedCount.count;
      }

      std::set<std::uint64_t> reflectedObjectHandles;
      for (auto const objectInstanceHandle :
           savedMember.successfullyReflectedObjectInstanceHandles) {
        if (objectInstanceHandle == 0U ||
            !reflectedObjectHandles.insert(objectInstanceHandle).second) {
          throw std::logic_error(
              "The saved member reflection telemetry has duplicate object handles.");
        }
      }
      std::map<std::uint64_t, std::uint64_t> reflectedObjectClasses;
      for (auto const& savedObject :
           savedMember.successfullyReflectedObjectInstanceClassHandles) {
        if (savedObject.objectInstanceHandle == 0U ||
            savedObject.objectClassHandle == 0U ||
            !reflectedObjectHandles.contains(savedObject.objectInstanceHandle) ||
            !reflectedObjectClasses.emplace(
                 savedObject.objectInstanceHandle,
                 savedObject.objectClassHandle)
                 .second) {
          throw std::logic_error(
              "The saved member reflection telemetry has an invalid object-class projection.");
        }
      }

      member->second.successfulReflectionCountsByClassAndTransportation =
          std::move(reflectionCounts);
      member->second.successfullyReflectedObjectInstanceHandles =
          std::move(reflectedObjectHandles);
      member->second.successfullyReflectedObjectInstanceClassHandles =
          std::move(reflectedObjectClasses);
    }
  }
}

void EmbeddedFederationRegistry::restoreMemberUpdateTelemetryFromStateImage(
    Federation& federation,
    FederationStateImage const& image,
    Federation const& liveFederation) {
  for (auto const& savedMember : image.members) {
    auto member = federation.members.find(savedMember.id);
    if (member == federation.members.end() ||
        !liveFederation.members.contains(savedMember.id)) {
      throw std::logic_error(
          "The saved member update telemetry has no live joined federate.");
    }

    // Images written before the typed telemetry extension carry no records.
    // Keep the process-local lifetime ledger in that compatibility case; a
    // current image with nonzero values replaces it below.
    bool const hasTypedTelemetry =
        savedMember.successfulUpdateAttributeValuesCount != 0U ||
        !savedMember.successfulUpdateCountsByClassAndTransportation.empty() ||
        !savedMember.successfullyUpdatedObjectInstanceHandles.empty() ||
        !savedMember.successfullyUpdatedObjectInstanceClassHandles.empty();
    if (!hasTypedTelemetry) {
      continue;
    }

    std::map<std::uint64_t, std::map<std::string, std::uint64_t>> updateCounts;
    for (auto const& savedCount :
         savedMember.successfulUpdateCountsByClassAndTransportation) {
      if (savedCount.objectClassHandle == 0U || savedCount.count == 0U ||
          !isSupportedTransportationName(
              federation.definition.catalog.get(), savedCount.transportationName)) {
        throw std::logic_error(
            "The saved member update telemetry has an invalid class or transportation.");
      }
      auto& count = updateCounts[savedCount.objectClassHandle][
          savedCount.transportationName];
      if (count != 0U) {
        throw std::logic_error(
            "The saved member update telemetry has duplicate class/transport buckets.");
      }
      count = savedCount.count;
    }

    std::set<std::uint64_t> updatedObjectHandles;
    for (auto const objectInstanceHandle :
         savedMember.successfullyUpdatedObjectInstanceHandles) {
      if (objectInstanceHandle == 0U ||
          !updatedObjectHandles.insert(objectInstanceHandle).second) {
        throw std::logic_error(
            "The saved member update telemetry has duplicate object handles.");
      }
    }
    std::map<std::uint64_t, std::uint64_t> updatedObjectClasses;
    for (auto const& savedObject :
         savedMember.successfullyUpdatedObjectInstanceClassHandles) {
      if (savedObject.objectInstanceHandle == 0U ||
          savedObject.objectClassHandle == 0U ||
          !updatedObjectHandles.contains(savedObject.objectInstanceHandle) ||
          !updatedObjectClasses.emplace(
               savedObject.objectInstanceHandle,
               savedObject.objectClassHandle)
               .second) {
        throw std::logic_error(
            "The saved member update telemetry has an invalid object-class projection.");
      }
    }

    member->second.successfulUpdateAttributeValuesCount =
        savedMember.successfulUpdateAttributeValuesCount;
    member->second.successfulUpdateCountsByClassAndTransportation =
        std::move(updateCounts);
    member->second.successfullyUpdatedObjectInstanceHandles =
        std::move(updatedObjectHandles);
    member->second.successfullyUpdatedObjectInstanceClassHandles =
        std::move(updatedObjectClasses);
  }
}

void EmbeddedFederationRegistry::restoreTsoQueueFromStateImage(
    Federation& federation,
    FederationStateImage const& image) {
  if (image.logicalTimeImplementationName !=
      federation.definition.logicalTimeImplementationName) {
    throw std::logic_error(
        "The saved TSO queue uses a different logical-time implementation.");
  }

  std::vector<TsoQueueRestoreEntry> entries;
  entries.reserve(image.tsoQueueEntries.size());
  for (auto const& savedEntry : image.tsoQueueEntries) {
    auto const timestamp = decodeLogicalTimeEncoding(
        image.logicalTimeImplementationName,
        savedEntry.timestampEncoding);
    if (!timestamp) {
      throw std::logic_error(
          "The saved TSO queue entry has no logical-time encoding.");
    }
    entries.push_back({
        TsoQueuedMessage{
            savedEntry.messageId,
            savedEntry.recipientFederateId,
            savedEntry.sequence,
            timestamp},
        static_cast<TsoMessageQueuePhase>(savedEntry.phase),
    });
  }

  auto const restored = federation.timeCoordinator.restoreTsoQueue(entries);
  if (restored.status != FederationTsoQueueRestoreStatus::applied ||
      restored.restoredCount != entries.size()) {
    throw std::logic_error(
        "The saved TSO queue could not be rehydrated for the joined recipients.");
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

  // The report writer is owned by the live ambassador, not by the
  // process-local federation snapshot.  Capture the immutable path associated
  // with each current joined-federate MOM object so a restore cannot silently
  // publish a snapshot with a different HLAreportServiceFile value.
  std::map<std::uint64_t, std::wstring> liveMomServiceReportFiles;
  for (auto const& [objectHandle, object] :
       target.rtiOwnedJoinedFederateMomObjects) {
    static_cast<void>(objectHandle);
    if (object.federationExecutionObject) {
      continue;
    }
    if (object.joinedFederateId == 0U || object.reportServiceFile.empty()) {
      throw std::logic_error(
          "A live joined-federate MOM object has no immutable service-report file.");
    }
    auto const [position, inserted] = liveMomServiceReportFiles.emplace(
        object.joinedFederateId,
        object.reportServiceFile);
    static_cast<void>(position);
    if (!inserted) {
      throw std::logic_error(
          "The live federation contains duplicate joined-federate service-report files.");
    }
  }

  // Validate the saved image before changing any target state.  The writer
  // remains attached to the current joined-federate lifetime, so the saved
  // static MOM value must match that live path exactly.
  for (auto const& [objectHandle, object] :
       snapshot.rtiOwnedJoinedFederateMomObjects) {
    static_cast<void>(objectHandle);
    if (object.federationExecutionObject) {
      continue;
    }
    auto const livePath = liveMomServiceReportFiles.find(object.joinedFederateId);
    if (livePath == liveMomServiceReportFiles.end() ||
        object.reportServiceFile.empty() ||
        object.reportServiceFile != livePath->second) {
      throw std::logic_error(
          "A restored joined-federate MOM object changed its immutable service-report file.");
    }
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
  target.transportationTypeHandles = snapshot.transportationTypeHandles;
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
  // Subscription generations are federation state, not live callback-route
  // state.  Restoring the allocator alongside the declaration ledger keeps
  // the next post-restore mutation from skipping or reusing an identity that
  // was assigned after the saved boundary.
  target.nextSubscriptionGeneration = snapshot.nextSubscriptionGeneration;
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
  target.nextAttributeValueUpdateRequestId =
      snapshot.nextAttributeValueUpdateRequestId;
  target.nextAttributeOwnershipQueryRequestId =
      snapshot.nextAttributeOwnershipQueryRequestId;
  target.nextTimeAdvanceGrantDispatchIdentity = nextTimeAdvanceGrantDispatchIdentity;
  target.pendingAttributeOwnershipQueries = snapshot.pendingAttributeOwnershipQueries;
  target.saveOperation.reset();
  target.pendingImmediateSave = snapshot.pendingImmediateSave;
  target.pendingTimedSave = snapshot.pendingTimedSave;
  target.lastSaveName = snapshot.lastSaveName;
  target.lastSaveTime = snapshot.lastSaveTime;
  target.nextSaveName = snapshot.nextSaveName;
  target.nextSaveTime = snapshot.nextSaveTime;
  target.restoreOperation.reset();
}

} // namespace umbra::detail
