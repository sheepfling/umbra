#include "internal/federation/federation_registry.hpp"
#include "internal/federation/federation_state_image.hpp"
#include "internal/federation/federation_registry_state_image_validation.hpp"

#include <algorithm>
#include <cstdint>
#include <map>
#include <mutex>
#include <optional>
#include <string>
#include <utility>

namespace umbra::detail {

namespace {

using federation_registry_state_image_validation::isRouteFreeDeliveredApplicationValue;
using federation_registry_state_image_validation::isRouteFreeInteractionDeclarationImage;
using federation_registry_state_image_validation::isRouteFreeMixedMultipleInteractionDeclarationImage;
using federation_registry_state_image_validation::isRouteFreeMultipleDirectedInteractionDeclarationImage;
using federation_registry_state_image_validation::isRouteFreeMultipleInteractionPublicationImage;
using federation_registry_state_image_validation::isRouteFreeObjectWithTsoAttributeUpdate;
using federation_registry_state_image_validation::isRouteFreePendingAttributeOwnershipQueryObject;
using federation_registry_state_image_validation::isRouteFreePendingAttributeTransportationTypeChangeObject;
using federation_registry_state_image_validation::isRouteFreePendingAttributeValueUpdateClassObject;
using federation_registry_state_image_validation::isRouteFreePendingAttributeValueUpdateObject;
using federation_registry_state_image_validation::isRouteFreePendingAttributeValueUpdateRegionalObject;
using federation_registry_state_image_validation::isRouteFreePendingConfirmDivestitureNotificationObject;
using federation_registry_state_image_validation::isRouteFreePendingConfirmDivestitureWithOwnershipAssumptionObject;
using federation_registry_state_image_validation::isRouteFreePendingDivestitureIfWantedNotificationObject;
using federation_registry_state_image_validation::isRouteFreePendingIfAvailableOwnershipObject;
using federation_registry_state_image_validation::isRouteFreePendingMixedNegotiatedConfirmationAsymmetricOwnershipObject;
using federation_registry_state_image_validation::isRouteFreePendingMixedNegotiatedConfirmationDeliveredOwnershipObject;
using federation_registry_state_image_validation::isRouteFreePendingMixedNegotiatedOwnershipObject;
using federation_registry_state_image_validation::isRouteFreePendingNegotiatedConfirmationDeliveredOwnershipObject;
using federation_registry_state_image_validation::isRouteFreePendingNegotiatedIfAvailableConfirmationDeliveredOwnershipObject;
using federation_registry_state_image_validation::isRouteFreePendingNegotiatedIfAvailableOwnershipObject;
using federation_registry_state_image_validation::isRouteFreePendingNegotiatedOwnershipObject;
using federation_registry_state_image_validation::isRouteFreePendingOwnershipAcquisitionCancellationObject;
using federation_registry_state_image_validation::isRouteFreePendingOwnershipAssumptionObject;
using federation_registry_state_image_validation::isRouteFreePendingRegularOwnershipObject;
using federation_registry_state_image_validation::isRouteFreeRegionalTsoInteractionImage;
using federation_registry_state_image_validation::pendingOwnershipAssumptionCallbackCount;

}  // namespace

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
  auto const saved = savedFederations == saveSnapshots_.end()
      ? std::map<std::wstring, Federation>::const_iterator{}
      : savedFederations->second.find(label);
  bool const processLocalSnapshot =
      savedFederations != saveSnapshots_.end() && saved != savedFederations->second.end();

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
  auto const requesterPublicReportRoute =
      federation->second.publicServiceReportRoutes.find(requestingFederateId);
  auto const selectedRequesterPublicReportRoute =
      requesterPublicReportRoute == federation->second.publicServiceReportRoutes.end()
      ? FederatePublicServiceReportRoute{}
      : requesterPublicReportRoute->second;

  // A process-local snapshot remains the richest restore source. If it is
  // absent (for example after a registry restart), the filesystem commit is
  // still eligible for the bounded control/temporal materializer below.
  Federation const& snapshot = processLocalSnapshot ? saved->second : federation->second;
  // Restore is admitted only when the durable commit envelope still agrees
  // with the in-process image.  The envelope is intentionally not treated as
  // a complete state image yet, but a missing/corrupt/mismatched record must
  // never be silently ignored by a future disk-backed restore path.
  bool durableCommitMatches = false;
  std::optional<FederationStateImage> durableStateImage;
  try {
    auto const durableCommit = saveCommitStore_->load(federationName, label);
    if (durableCommit && durableCommit->federationName == federationName &&
        durableCommit->label == label &&
         durableCommit->logicalTimeImplementationName ==
             snapshot.definition.logicalTimeImplementationName &&
         durableCommit->memberFederateIds.size() == snapshot.members.size()) {
      durableCommitMatches = true;
      for (auto const& [memberId, member] : snapshot.members) {
        static_cast<void>(member);
        if (std::find(
                durableCommit->memberFederateIds.begin(),
                durableCommit->memberFederateIds.end(),
                memberId) == durableCommit->memberFederateIds.end()) {
          durableCommitMatches = false;
          break;
        }
      }
      if (durableCommitMatches && !durableCommit->stateImage.empty()) {
        try {
          auto image = FederationStateImageCodec::decode(
              durableCommit->stateImage);
          if (image.federationName != federationName ||
              image.logicalTimeImplementationName !=
                  snapshot.definition.logicalTimeImplementationName ||
              image.members.size() != snapshot.members.size()) {
            durableCommitMatches = false;
          }
          if (processLocalSnapshot && durableCommitMatches &&
              image.normalizationSeed != snapshot.normalizationSeed) {
            durableCommitMatches = false;
          }
          if (durableCommitMatches) {
            for (std::size_t index = 0U; index < image.members.size(); ++index) {
              auto const& imageMember = image.members[index];
              auto const snapshotMember = snapshot.members.find(imageMember.id);
              auto const liveMember = federation->second.members.find(imageMember.id);
              if (snapshotMember == snapshot.members.end() ||
                  liveMember == federation->second.members.end() ||
                  snapshotMember->second.name != imageMember.name ||
                  snapshotMember->second.type != imageMember.type ||
                  liveMember->second.name != imageMember.name ||
                  liveMember->second.type != imageMember.type) {
                durableCommitMatches = false;
                break;
              }
            }
          }
          if (durableCommitMatches) {
            if (processLocalSnapshot) {
              // Canonical re-encoding compares every v1 field (switches,
              // section counts, allocator floors, member control state, and
              // official encoded temporal values), not just the identity fence
              // above. A syntactically valid but stale/tampered payload must not
              // be accepted as the image for this in-process snapshot.
              auto const expectedImage = FederationStateImageCodec::encode(
                  stateImageFor(snapshot, federationName));
              durableCommitMatches =
                  FederationStateImageCodec::encode(image) == expectedImage;
            } else {
              // The restart path materializes the scalar/member/time image,
              // route-free declaration ledgers, the bounded object
              // application-value projection, the bounded regular, If
              // Available, negotiated regular, negotiated If Available, and
              // mixed negotiated and confirmation-delivered regular/If Available
              // ownership ledgers,
              // and the ordinary timestamped
              // interaction, directed-interaction, and attribute-update
              // payload/queue ledgers. A timestamped object deletion is also
              // admitted when its invocation snapshot is explicitly bounded
              // to the object value ledger plus the pending deletion marker;
              // other callback-bearing application ledgers still require a
              // live object snapshot until their route-free images have an
              // explicit restart contract.
              auto const hasNoObjectCallbackLedgers =
                  [&image](FederationStateImageObject const& object) {
                    return object.pendingAttributeOwnershipAcquisitionIfAvailableRequests.empty() &&
                        object.pendingAttributeOwnershipAcquisitionRequests.empty() &&
                        object.pendingAttributeOwnershipAcquisitionCancellations.empty() &&
                        object.pendingAttributeOwnershipDivestitureIfWantedNotifications.empty() &&
                        object.pendingConfirmDivestitureNotifications.empty() &&
                        object.pendingAttributeTransportationTypeChanges.empty() &&
                        object.pendingAttributeValueUpdateRequests.empty() &&
                        object.pendingAttributeValueUpdateClassRequests.empty() &&
                        object.pendingAttributeValueUpdateRegionalRequests.empty() &&
                        object.pendingNegotiatedAttributeOwnershipDivestitures.empty() &&
                        object.ownershipAssumptionRecipientsByAttribute.empty() &&
                        object.ownershipAssumptionUserSuppliedTagsByAttribute.empty() &&
                        pendingOwnershipAssumptionCallbackCount(image, object.handle) == 0U &&
                        object.pendingDiscoveryFederateIds.empty() &&
                        object.pendingRemovalFederateIds.empty() &&
                        object.connectionLossAutomaticRemovalFederateIds.empty() &&
                        object.deferredConnectionLossTsoRemovalFederateIds.empty() &&
                        std::all_of(
                            object.attributes.begin(),
                            object.attributes.end(),
                            [](FederationStateImageObjectAttribute const& attribute) {
                              return attribute.updateRegionHandles.empty();
                            });
                  };
              auto const isRouteFreeObjectApplicationValue =
                  [&](FederationStateImageObject const& object) {
                    return object.attributeValuesPresent &&
                        !object.attributeValues.empty() &&
                        !object.deleteAccepted &&
                        object.pendingOperationCount == 0U &&
                        hasNoObjectCallbackLedgers(object) &&
                        object.pendingTimestampedRemovalFederateIds.empty() &&
                        !object.pendingTimestampedDeletionMessageId.has_value();
                  };
              auto const hasMatchingDeletionPayload =
                  [&](FederationStateImageObject const& object) {
                    return object.pendingTimestampedDeletionMessageId.has_value() &&
                        std::any_of(
                            image.tsoObjectDeletionMessages.begin(),
                            image.tsoObjectDeletionMessages.end(),
                            [&](FederationStateImageTsoObjectDeletionMessage const& message) {
                              return message.messageId ==
                                      *object.pendingTimestampedDeletionMessageId &&
                                  message.objectInstanceHandle == object.handle &&
                                  message.producingFederateId == object.producingFederateId &&
                                  message.reconstitution.has_value();
                            });
                  };
              auto const isRouteFreeObjectDeletionInvocation =
                  [&](FederationStateImageObject const& object) {
                    return object.attributeValuesPresent &&
                        !object.attributeValues.empty() &&
                        !object.deleteAccepted &&
                        !object.pendingTimestampedRemovalFederateIds.empty() &&
                        object.pendingTimestampedDeletionMessageId.has_value() &&
                        object.pendingOperationCount ==
                            object.pendingTimestampedRemovalFederateIds.size() + 1U &&
                        hasNoObjectCallbackLedgers(object) &&
                        hasMatchingDeletionPayload(object);
                  };
              bool const restartableObjectLedger =
                  !image.objects.empty() &&
                  std::all_of(
                      image.objects.begin(),
                      image.objects.end(),
                      [&](FederationStateImageObject const& object) {
                        if (!object.pendingAttributeValueUpdateRequests.empty()) {
                          return isRouteFreePendingAttributeValueUpdateObject(object);
                        }
                        if (!object.pendingAttributeValueUpdateClassRequests.empty()) {
                          return isRouteFreePendingAttributeValueUpdateClassObject(object);
                        }
                        if (!object.pendingAttributeValueUpdateRegionalRequests.empty()) {
                          return isRouteFreePendingAttributeValueUpdateRegionalObject(object);
                        }
                        if (isRouteFreePendingAttributeOwnershipQueryObject(object, image)) {
                          return true;
                        }
                        if (isRouteFreePendingOwnershipAssumptionObject(object, image)) {
                          return true;
                        }
                            return isRouteFreeDeliveredApplicationValue(object) ||
                            isRouteFreeObjectApplicationValue(object) ||
                            isRouteFreeObjectWithTsoAttributeUpdate(object, image) ||
                            isRouteFreeObjectDeletionInvocation(object) ||
                            isRouteFreePendingRegularOwnershipObject(object) ||
                            isRouteFreePendingIfAvailableOwnershipObject(object) ||
                            isRouteFreePendingNegotiatedOwnershipObject(object, image) ||
                            isRouteFreePendingNegotiatedIfAvailableOwnershipObject(object, image) ||
                            isRouteFreePendingMixedNegotiatedOwnershipObject(object, image) ||
                            isRouteFreePendingNegotiatedConfirmationDeliveredOwnershipObject(object, image) ||
                            isRouteFreePendingNegotiatedIfAvailableConfirmationDeliveredOwnershipObject(object, image) ||
                            isRouteFreePendingMixedNegotiatedConfirmationDeliveredOwnershipObject(object, image) ||
                            isRouteFreePendingMixedNegotiatedConfirmationAsymmetricOwnershipObject(object, image) ||
                            isRouteFreePendingDivestitureIfWantedNotificationObject(object) ||
                            isRouteFreePendingConfirmDivestitureNotificationObject(object) ||
                            isRouteFreePendingConfirmDivestitureWithOwnershipAssumptionObject(
                                object,
                                image) ||
                            isRouteFreePendingOwnershipAcquisitionCancellationObject(object) ||
                            isRouteFreePendingAttributeTransportationTypeChangeObject(object);
                      });
              bool const restartableTsoPayloadLedger =
                  std::all_of(
                      image.tsoObjectDeletionMessages.begin(),
                      image.tsoObjectDeletionMessages.end(),
                      [](FederationStateImageTsoObjectDeletionMessage const& message) {
                        return message.reconstitution.has_value();
                      });
              bool const restartableInteractionLedger =
                  image.interactionDeclarations.empty() ||
                  isRouteFreeInteractionDeclarationImage(image) ||
                  isRouteFreeMultipleInteractionPublicationImage(image) ||
                  isRouteFreeMixedMultipleInteractionDeclarationImage(image) ||
                  isRouteFreeMultipleDirectedInteractionDeclarationImage(image) ||
                  isRouteFreeRegionalTsoInteractionImage(image);
              durableCommitMatches =
                  restartableInteractionLedger &&
                  (image.objects.empty()
                       ? image.tsoObjectDeletionMessages.empty()
                       : restartableObjectLedger) &&
                  restartableTsoPayloadLedger;
            }
            if (durableCommitMatches) {
              durableStateImage = std::move(image);
            }
          }
        } catch (...) {
          durableCommitMatches = false;
        }
      }
    }
  } catch (...) {
    durableCommitMatches = false;
  }
  if (!durableCommitMatches) {
    result.status = FederationRestoreControlStatus::snapshot_not_found;
    FederationRestoreNotification notification;
    notification.kind = FederationRestoreNotificationKind::request_failed;
    notification.receivingFederateId = requestingFederateId;
    notification.label = label;
    notification.callbackRoute = requesterRoute->second;
    notification.serviceReportRoute = selectedRequesterReportRoute;
    notification.publicServiceReportRoute = selectedRequesterPublicReportRoute;
    result.notifications.push_back(std::move(notification));
    return result;
  }
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
    notification.publicServiceReportRoute = selectedRequesterPublicReportRoute;
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
  operation.stateImage = std::move(durableStateImage);
  operation.processLocalSnapshot = processLocalSnapshot;
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
  succeeded.publicServiceReportRoute = selectedRequesterPublicReportRoute;
  result.notifications.push_back(std::move(succeeded));

  // The embedded backend begins the restore immediately after accepting the
  // request. The official callbacks still preserve the standard ordering:
  // request success, federation-begun, then one initiate callback per member.
  for (auto const& [federateId, member] : federation->second.members) {
    auto const route = federation->second.interactionCallbackRoutes.find(federateId);
    auto const reportRoute = federation->second.serviceReportRoutes.find(federateId);
    auto const publicReportRoute = federation->second.publicServiceReportRoutes.find(federateId);
    FederationRestoreNotification begun;
    begun.kind = FederationRestoreNotificationKind::begin;
    begun.receivingFederateId = federateId;
    begun.label = label;
    begun.callbackRoute = route->second;
    begun.serviceReportRoute =
        reportRoute == federation->second.serviceReportRoutes.end()
        ? FederateServiceReportRoute{}
        : reportRoute->second;
    begun.publicServiceReportRoute =
        publicReportRoute == federation->second.publicServiceReportRoutes.end()
        ? FederatePublicServiceReportRoute{}
        : publicReportRoute->second;
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
    initiate.publicServiceReportRoute =
        publicReportRoute == federation->second.publicServiceReportRoutes.end()
        ? FederatePublicServiceReportRoute{}
        : publicReportRoute->second;
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
  bool const processLocalSnapshot =
      federation->second.restoreOperation->processLocalSnapshot;
  auto const savedFederations = saveSnapshots_.find(federationName);
  auto const saved = savedFederations == saveSnapshots_.end()
      ? std::map<std::wstring, Federation>::const_iterator{}
      : savedFederations->second.find(label);
  if (processLocalSnapshot &&
      (savedFederations == saveSnapshots_.end() ||
       saved == savedFederations->second.end())) {
    appendRestoreCompletionNotifications(
        federation->second,
        false,
        rti1516_2025::RTI_UNABLE_TO_RESTORE,
        result.notifications);
    federation->second.restoreOperation.reset();
    return result;
  }

  try {
    auto const& restoreOperation = federation->second.restoreOperation;
    if (!restoreOperation || !restoreOperation->stateImage) {
      throw std::logic_error(
          "A successful restore request has no validated durable state image.");
    }
    // A process-local save carries the complete private snapshot. A
    // restarted registry instead starts from its current live membership and
    // applies the route-free control/temporal image before the common typed
    // rehydration helpers run.
    Federation snapshot = processLocalSnapshot ? saved->second : federation->second;
    if (!processLocalSnapshot) {
      restoreControlAndTimeFromStateImage(
          snapshot,
          *restoreOperation->stateImage,
          federation->second);
    }
    restoreMemberInteractionReceiptTelemetryFromStateImage(
        snapshot,
        *restoreOperation->stateImage,
        federation->second);
    restoreObjectInstanceNameReservationsFromStateImage(
        snapshot,
        *restoreOperation->stateImage,
        federation->second);
    restoreRegionsFromStateImage(
        snapshot,
        *restoreOperation->stateImage,
        federation->second);
    restoreSynchronizationPointsFromStateImage(
        snapshot,
        *restoreOperation->stateImage,
        federation->second);
    restoreObjectClassAttributeDeclarationsFromStateImage(
        snapshot,
        *restoreOperation->stateImage,
        federation->second);
    if (!processLocalSnapshot) {
      restoreInteractionDeclarationsFromStateImage(
          snapshot,
          *restoreOperation->stateImage,
          federation->second);
    }
    restoreMemberInteractionSendTelemetryFromStateImage(
        snapshot,
        *restoreOperation->stateImage,
        federation->second);
    restoreMemberObjectLifecycleTelemetryFromStateImage(
        snapshot,
        *restoreOperation->stateImage,
        federation->second);
    restoreMemberReflectionTelemetryFromStateImage(
        snapshot,
        *restoreOperation->stateImage,
        federation->second);
    restoreMemberUpdateTelemetryFromStateImage(
        snapshot,
        *restoreOperation->stateImage,
        federation->second);
    restoreObjectOwnershipLedgersFromStateImage(
        snapshot,
        *restoreOperation->stateImage,
        federation->second,
        !processLocalSnapshot);
    restoreTsoPayloadsFromStateImage(
        snapshot,
        *restoreOperation->stateImage,
        federation->second);
    restoreTsoQueueFromStateImage(snapshot, *restoreOperation->stateImage);
    restoreFederationFromSnapshot(federation->second, snapshot);
    if (!processLocalSnapshot) {
      // A durable image carries declaration state, while the relevance sets
      // are derived transition baselines. Seed them after the image is
      // applied so the next declaration mutation reports only a real edge;
      // the synthetic planner output is intentionally discarded here.
      auto declarationAdvisories =
          planDeclarationAdvisoriesLocked(federationName);
      static_cast<void>(declarationAdvisories);

      // The durable image retains regular acquisition requests but not the
      // callback closures from the process that created it. Re-plan each
      // request now that the fresh registry owns the live membership routes.
      for (auto& [objectInstanceHandle, objectInstance] :
           federation->second.objectInstances) {
        static_cast<void>(objectInstanceHandle);
        auto workItems = planPendingAttributeOwnershipAcquisitionWork(
            federation->second,
            objectInstance);
        result.ownershipAcquisitionWorkItems.insert(
            result.ownershipAcquisitionWorkItems.end(),
            std::make_move_iterator(workItems.begin()),
            std::make_move_iterator(workItems.end()));
      }
      // An If Available request is an accepted requester callback, not an
      // owner-side release reservation.  Rebind one callback work item per
      // restored request to the current requester route; the callback will
      // re-evaluate ownership at its normal begin boundary.
      for (auto const& [objectInstanceHandle, objectInstance] :
           federation->second.objectInstances) {
        for (auto const& [requestId, request] :
             objectInstance.pendingAttributeOwnershipAcquisitionIfAvailableRequests) {
          // A negotiated divestiture that selected this If Available request
          // owns the next callback boundary: replaying the ordinary WTA
          // notification as well would consume the request as unavailable
          // before the rebuilt Request Divestiture Confirmation can transfer
          // ownership.  The bounded restart predicate admits only requests
          // whose complete desired set is covered by this negotiated ledger.
          bool negotiatedIfAvailableCandidate = false;
          for (auto const& [attributeHandle, divestiture] :
               objectInstance.pendingNegotiatedAttributeOwnershipDivestitures) {
            if (divestiture.acquiringFederateIsIfAvailable &&
                divestiture.acquiringFederateId == request.requestingFederateId &&
                divestiture.acquisitionRequestId == requestId &&
                request.desiredAttributeHandles.contains(attributeHandle)) {
              negotiatedIfAvailableCandidate = true;
              break;
            }
          }
          if (negotiatedIfAvailableCandidate) {
            continue;
          }
          auto const callbackRoute = federation->second.interactionCallbackRoutes.find(
              request.requestingFederateId);
          if (callbackRoute == federation->second.interactionCallbackRoutes.end() ||
              !callbackRoute->second) {
            throw std::logic_error(
                "A restored If Available ownership reservation has no live callback route.");
          }
          result.ownershipAcquisitionWorkItems.push_back({
              AttributeOwnershipAcquisitionWorkKind::if_available_notification,
              request.requestingFederateId,
              request.requestingFederateId,
              objectInstanceHandle,
              requestId,
              {},
              request.userSuppliedTag,
              true,
              callbackRoute->second,
          });
        }
      }
      // A cancellation reservation is a separate requester callback from the
      // regular acquisition work-item family. Rebind each durable reservation
      // to the live requester route after restore; the callback-time registry
      // check still owns the one-shot consume boundary.
      for (auto const& [objectInstanceHandle, objectInstance] :
           federation->second.objectInstances) {
        for (auto const& [cancellationId, cancellation] :
             objectInstance.pendingAttributeOwnershipAcquisitionCancellations) {
          auto const callbackRoute = federation->second.interactionCallbackRoutes.find(
              cancellation.requestingFederateId);
          if (callbackRoute == federation->second.interactionCallbackRoutes.end() ||
              !callbackRoute->second) {
            throw std::logic_error(
                "A restored ownership-acquisition cancellation has no live callback route.");
          }
          result.ownershipAcquisitionCancellationWorkItems.push_back({
              cancellation.requestingFederateId,
              objectInstanceHandle,
              cancellationId,
              cancellation.attributeHandles,
              callbackRoute->second,
          });
        }
      }
      // A Divestiture If Wanted transfer changes ownership synchronously, but
      // its Acquisition Notification remains callback-gated. Rebind each
      // durable notification to the current requester route after restore;
      // beginAttributeOwnershipDivestitureIfWantedNotification owns the
      // one-shot consume boundary.
      for (auto const& [objectInstanceHandle, objectInstance] :
           federation->second.objectInstances) {
        for (auto const& [notificationId, notification] :
             objectInstance.pendingAttributeOwnershipDivestitureIfWantedNotifications) {
          auto const callbackRoute = federation->second.interactionCallbackRoutes.find(
              notification.receivingFederateId);
          if (callbackRoute == federation->second.interactionCallbackRoutes.end() ||
              !callbackRoute->second) {
            throw std::logic_error(
                "A restored Divestiture If Wanted notification has no live callback route.");
          }
          result.ownershipDivestitureIfWantedWorkItems.push_back({
              notificationId,
              notification.receivingFederateId,
              objectInstanceHandle,
              notification.attributeHandles,
              notification.userSuppliedTag,
              callbackRoute->second,
          });
        }
      }
      // Confirm Divestiture transfers ownership synchronously, but its
      // Attribute Ownership Acquisition Notification remains callback-gated.
      // Rebind each durable notification to the current requester route only
      // after Federation Restored, preserving the confirming tag.
      for (auto const& [objectInstanceHandle, objectInstance] :
           federation->second.objectInstances) {
        for (auto const& [notificationId, notification] :
             objectInstance.pendingConfirmDivestitureNotifications) {
          auto const callbackRoute = federation->second.interactionCallbackRoutes.find(
              notification.receivingFederateId);
          if (callbackRoute == federation->second.interactionCallbackRoutes.end() ||
              !callbackRoute->second) {
            throw std::logic_error(
                "A restored Confirm Divestiture notification has no live callback route.");
          }
          result.confirmDivestitureWorkItems.push_back({
              notificationId,
              notification.receivingFederateId,
              objectInstanceHandle,
              notification.attributeHandles,
              notification.userSuppliedTag,
              callbackRoute->second,
          });
        }
      }
      // An attribute transportation-type change is a separate requester
      // callback reservation. Rebind the durable request to the live route;
      // beginAttributeTransportationTypeChange still owns the one-shot
      // consume/commit boundary.
      for (auto const& [objectInstanceHandle, objectInstance] :
           federation->second.objectInstances) {
        for (auto const& [requestId, request] :
             objectInstance.pendingAttributeTransportationTypeChanges) {
          auto const callbackRoute = federation->second.interactionCallbackRoutes.find(
              request.requestingFederateId);
          if (callbackRoute == federation->second.interactionCallbackRoutes.end() ||
              !callbackRoute->second) {
            throw std::logic_error(
                "A restored attribute transportation-type change has no live callback route.");
          }
          result.attributeTransportationTypeChangeWorkItems.push_back({
              request.requestingFederateId,
              objectInstanceHandle,
              requestId,
              callbackRoute->second,
          });
        }
      }
      // An interaction transportation-type change is a publisher callback
      // reservation independent of object-instance state. Rebind the saved
      // class identity to the current publisher route; the confirmation
      // callback owns the one-shot consume/commit boundary.
      for (auto const& [requestingFederateId, declarations] :
           federation->second.interactionDeclarations) {
        for (auto const& [interactionClassHandle, transportationName] :
             declarations.pendingInteractionTransportationTypeChanges) {
          static_cast<void>(transportationName);
          auto const callbackRoute = federation->second.interactionCallbackRoutes.find(
              requestingFederateId);
          if (callbackRoute == federation->second.interactionCallbackRoutes.end() ||
              !callbackRoute->second) {
            throw std::logic_error(
                "A restored interaction transportation-type change has no live callback route.");
          }
          result.interactionTransportationTypeChangeWorkItems.push_back({
              requestingFederateId,
              interactionClassHandle,
              callbackRoute->second,
          });
        }
      }
      // Object-instance Request Attribute Value Update callbacks are the
      // remaining route-free application-request ledger in this slice. The
      // request record stays in the restored object until the callback route
      // reaches its begin boundary, where the registry consumes it exactly
      // once (including a stale/no-delivery outcome).
      for (auto const& [objectInstanceHandle, objectInstance] :
           federation->second.objectInstances) {
        for (auto const& [requestId, request] :
             objectInstance.pendingAttributeValueUpdateRequests) {
          auto const callbackRoute = federation->second.interactionCallbackRoutes.find(
              request.providingFederateId);
          if (callbackRoute == federation->second.interactionCallbackRoutes.end() ||
              !callbackRoute->second) {
            throw std::logic_error(
                "A restored attribute value update request has no live provider callback route.");
          }
          auto const reportRoute = federation->second.serviceReportRoutes.find(
              request.providingFederateId);
          result.attributeValueUpdateProvideWorkItems.push_back({
              requestId,
              request.requestingFederateId,
              request.providingFederateId,
              objectInstanceHandle,
              request.requestedAttributeHandles,
              request.userSuppliedTag,
              callbackRoute->second,
              reportRoute == federation->second.serviceReportRoutes.end()
                  ? FederateServiceReportRoute{}
                  : reportRoute->second,
          });
        }
        for (auto const& [requestId, request] :
             objectInstance.pendingAttributeValueUpdateClassRequests) {
          auto const callbackRoute = federation->second.interactionCallbackRoutes.find(
              request.providingFederateId);
          if (callbackRoute == federation->second.interactionCallbackRoutes.end() ||
              !callbackRoute->second) {
            throw std::logic_error(
                "A restored object-class attribute value update request has no live provider callback route.");
          }
          auto const reportRoute = federation->second.serviceReportRoutes.find(
              request.providingFederateId);
          result.attributeValueUpdateClassProvideWorkItems.push_back({
              requestId,
              request.requestingFederateId,
              request.providingFederateId,
              objectInstanceHandle,
              request.requestedObjectClassHandle,
              request.requestedAttributeHandles,
              request.userSuppliedTag,
              callbackRoute->second,
              reportRoute == federation->second.serviceReportRoutes.end()
                  ? FederateServiceReportRoute{}
                  : reportRoute->second,
          });
        }
        for (auto const& [requestId, request] :
             objectInstance.pendingAttributeValueUpdateRegionalRequests) {
          auto const callbackRoute = federation->second.interactionCallbackRoutes.find(
              request.providingFederateId);
          if (callbackRoute == federation->second.interactionCallbackRoutes.end() ||
              !callbackRoute->second) {
            throw std::logic_error(
                "A restored regional attribute value update request has no live provider callback route.");
          }
          auto const reportRoute = federation->second.serviceReportRoutes.find(
              request.providingFederateId);
          result.attributeValueUpdateRegionalProvideWorkItems.push_back({
              requestId,
              request.requestingFederateId,
              request.providingFederateId,
              objectInstanceHandle,
              request.requestedObjectClassHandle,
              request.requestedAttributeHandles,
              request.requestRegionsByAttribute,
              request.userSuppliedTag,
              callbackRoute->second,
              reportRoute == federation->second.serviceReportRoutes.end()
                  ? FederateServiceReportRoute{}
                  : reportRoute->second,
          });
        }
      }
      // Queued Request Attribute Ownership Assumption callbacks retain only
      // their route-free object/recipient/attribute/tag tuple. Rebind each
      // callback to the current recipient route after Federation Restored;
      // attributeOwnershipAssumptionDeliveryFor consumes the tuple exactly
      // once at callback entry and rechecks the live unowned state.
      for (auto const& [objectInstanceHandle, objectInstance] :
           federation->second.objectInstances) {
        for (auto const& pending :
             objectInstance.pendingAttributeOwnershipAssumptionCallbacks) {
          auto const callbackRoute = federation->second.interactionCallbackRoutes.find(
              pending.receivingFederateId);
          if (callbackRoute == federation->second.interactionCallbackRoutes.end() ||
              !callbackRoute->second) {
            throw std::logic_error(
                "A restored ownership-assumption callback has no live recipient route.");
          }
          result.attributeOwnershipAssumptionWorkItems.push_back({
              pending.receivingFederateId,
              objectInstanceHandle,
              pending.attributeHandles,
              callbackRoute->second,
              pending.userSuppliedTag,
          });
        }
      }
      // Query Attribute Ownership result callbacks are a federation-scoped
      // application-request ledger. Rebind each saved result to the current
      // requester route only after Federation Restored, preserving the same
      // one-shot callback boundary as the other restored request families.
      for (auto const& [requestId, request] :
           federation->second.pendingAttributeOwnershipQueries) {
        auto const callbackRoute = federation->second.interactionCallbackRoutes.find(
            request.requestingFederateId);
        if (callbackRoute == federation->second.interactionCallbackRoutes.end() ||
            !callbackRoute->second) {
          throw std::logic_error(
              "A restored Attribute Ownership query has no live callback route.");
        }
        AttributeOwnershipQueryRecipient work;
        work.requestId = requestId;
        work.receivingFederateId = request.requestingFederateId;
        work.objectInstanceHandle = request.objectInstanceHandle;
        work.reportKind = request.reportKind;
        work.owningFederateId = request.owningFederateId;
        work.attributeHandles = request.requestedAttributeHandles;
        work.callbackRoute = callbackRoute->second;
        result.attributeOwnershipQueryWorkItems.push_back(std::move(work));
      }
    }
    if (processLocalSnapshot) {
      // A process-local save keeps the complete private snapshot, but the
      // process endpoint still needs value-only ownership work so it can
      // route the callback to the current session rather than reviving a
      // saved callback closure.  Keep this family aligned with the
      // fresh-registry rebind above; the process service projects it after
      // Federation Restored through its existing ownership event queue.
      for (auto const& [objectInstanceHandle, objectInstance] :
           saved->second.objectInstances) {
        for (auto const& pending :
             objectInstance.pendingAttributeOwnershipAssumptionCallbacks) {
          auto const callbackRoute = federation->second.interactionCallbackRoutes.find(
              pending.receivingFederateId);
          if (callbackRoute == federation->second.interactionCallbackRoutes.end() ||
              !callbackRoute->second) {
            throw std::logic_error(
                "A process-local restored ownership-assumption callback has no live recipient route.");
          }
          result.attributeOwnershipAssumptionWorkItems.push_back({
              pending.receivingFederateId,
              objectInstanceHandle,
              pending.attributeHandles,
              callbackRoute->second,
              pending.userSuppliedTag,
          });
        }
      }
    }
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
    auto const publicReportRoute = federation->second.publicServiceReportRoutes.find(federateId);
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
    notification.publicServiceReportRoute =
        publicReportRoute == federation->second.publicServiceReportRoutes.end()
        ? FederatePublicServiceReportRoute{}
        : publicReportRoute->second;
    result.notifications.push_back(std::move(notification));
  }
  // Queue the reconstructed grant only after every successful restore
  // notification has been prepared. The adapter submits those callbacks first,
  // preserving a clear restored-before-grant boundary in both callback models.
  result.timeAdvanceGrantDispatches =
      scheduleEligibleTimeAdvanceGrants(federation->second);
  // Role-enable callbacks are likewise rebuilt only after the target time
  // coordinator has applied the saved image. The live factory can therefore
  // capture the target state's fresh callback epoch and stale pre-restore
  // role work cannot consume a restored generation.
  result.timeRoleEnableDispatches =
      scheduleRestoredTimeRoleEnableDispatches(federation->second);
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
  auto const publicReportRoute = federation->second.publicServiceReportRoutes.find(federateId);
  notification.publicServiceReportRoute =
      publicReportRoute == federation->second.publicServiceReportRoutes.end()
      ? FederatePublicServiceReportRoute{}
      : publicReportRoute->second;
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

}  // namespace umbra::detail
