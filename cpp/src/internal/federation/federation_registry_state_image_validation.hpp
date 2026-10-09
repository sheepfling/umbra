#pragma once

#include "internal/federation/federation_state_image.hpp"

#include <cstddef>
#include <cstdint>

namespace umbra::detail::federation_registry_state_image_validation {

bool isRouteFreeInteractionDeclarationImage(FederationStateImage const& image);
bool isRouteFreeMultipleInteractionPublicationImage(FederationStateImage const& image);
bool isRouteFreeRegionalTsoInteractionImage(FederationStateImage const& image);
bool isRouteFreeMixedMultipleInteractionDeclarationImage(
    FederationStateImage const& image);
bool isRouteFreeMultipleDirectedInteractionDeclarationImage(
    FederationStateImage const& image);
bool isRouteFreePendingAttributeValueUpdateObject(
    FederationStateImageObject const& object);
bool isRouteFreePendingAttributeValueUpdateClassObject(
    FederationStateImageObject const& object);
bool isRouteFreePendingAttributeValueUpdateRegionalObject(
    FederationStateImageObject const& object);
bool isRouteFreeDeliveredApplicationValue(FederationStateImageObject const& object);
bool isRouteFreePendingAttributeOwnershipQueryObject(
    FederationStateImageObject const& object,
    FederationStateImage const& image);
bool isRouteFreePendingAttributeTransportationTypeChangeObject(
    FederationStateImageObject const& object);
bool isRouteFreeObjectWithTsoAttributeUpdate(
    FederationStateImageObject const& object,
    FederationStateImage const& image);

std::size_t pendingOwnershipAssumptionCallbackCount(
    FederationStateImage const &image,
    std::uint64_t objectInstanceHandle);

bool isRouteFreeNegotiatedOwnershipAssumptionLedger(
    FederationStateImageObject const &object,
    FederationStateImage const &image);
bool isRouteFreePendingOwnershipAssumptionObject(
    FederationStateImageObject const &object,
    FederationStateImage const &image);
bool isRouteFreePendingRegularOwnershipObject(FederationStateImageObject const &object);
bool isRouteFreePendingIfAvailableOwnershipObject(FederationStateImageObject const &object);
bool isRouteFreePendingNegotiatedOwnershipObject(
    FederationStateImageObject const &object,
    FederationStateImage const &image);
bool isRouteFreePendingNegotiatedIfAvailableOwnershipObject(
    FederationStateImageObject const &object,
    FederationStateImage const &image);
bool isRouteFreePendingMixedNegotiatedOwnershipObject(
    FederationStateImageObject const &object,
    FederationStateImage const &image);
bool isRouteFreePendingNegotiatedConfirmationDeliveredOwnershipObject(
    FederationStateImageObject const &object,
    FederationStateImage const &image);
bool isRouteFreePendingNegotiatedIfAvailableConfirmationDeliveredOwnershipObject(
    FederationStateImageObject const &object,
    FederationStateImage const &image);
bool isRouteFreePendingMixedNegotiatedConfirmationDeliveredOwnershipObject(
    FederationStateImageObject const &object,
    FederationStateImage const &image);
bool isRouteFreePendingMixedNegotiatedConfirmationAsymmetricOwnershipObject(
    FederationStateImageObject const &object,
    FederationStateImage const &image);
bool isRouteFreePendingDivestitureIfWantedNotificationObject(
    FederationStateImageObject const &object);
bool isRouteFreePendingConfirmDivestitureNotificationObject(
    FederationStateImageObject const &object);
bool isRouteFreePendingConfirmDivestitureWithOwnershipAssumptionObject(
    FederationStateImageObject const &object,
    FederationStateImage const &image);
bool isRouteFreePendingOwnershipAcquisitionCancellationObject(
    FederationStateImageObject const &object);

}  // namespace umbra::detail::federation_registry_state_image_validation
