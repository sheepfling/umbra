#include "internal/federation/federation_registry.hpp"
#include "internal/federation/federation_state_image.hpp"
#include "internal/federation/federation_registry_value_helpers.hpp"
#include "internal/federation/federation_registry_state_image_validation.hpp"
#include "internal/fom/fom_catalog.hpp"

#include <map>
#include <stdexcept>
#include <utility>
#include <vector>

namespace umbra::detail {

using federation_registry_state_image_validation::isRouteFreeInteractionDeclarationImage;
using federation_registry_state_image_validation::isRouteFreeMixedMultipleInteractionDeclarationImage;
using federation_registry_state_image_validation::isRouteFreeMultipleDirectedInteractionDeclarationImage;
using federation_registry_state_image_validation::isRouteFreeMultipleInteractionPublicationImage;
using federation_registry_state_image_validation::isRouteFreeRegionalTsoInteractionImage;

void EmbeddedFederationRegistry::restoreObjectClassAttributeDeclarationsFromStateImage(
    Federation& federation,
    FederationStateImage const& image,
    Federation const& liveFederation) {
  if (!image.objectClassAttributeDeclarationsPresent) {
    return;
  }

  federation.objectClassAttributeDeclarations.clear();
  for (auto const& savedDeclarations : image.objectClassAttributeDeclarations) {
    if (savedDeclarations.federateId == 0U ||
        !federation.members.contains(savedDeclarations.federateId) ||
        !liveFederation.members.contains(savedDeclarations.federateId)) {
      throw std::logic_error(
          "The saved object-class attribute declarations have no live joined federate.");
    }

    Federation::ObjectClassAttributeDeclarations declarations;
    declarations.subscriptionGeneration = savedDeclarations.subscriptionGeneration;
    for (auto const& savedClass : savedDeclarations.classes) {
      if (!validObjectClass(federation, savedClass.objectClassHandle)) {
        throw std::logic_error(
            "The saved object-class attribute declarations reference an unknown class.");
      }

      std::set<std::uint64_t> attributeHandles;
      attributeHandles.insert(
          savedClass.explicitlyPublishedAttributeHandles.begin(),
          savedClass.explicitlyPublishedAttributeHandles.end());
      for (auto const& subscription : savedClass.subscribedAttributes) {
        static_cast<void>(attributeHandles.insert(subscription.attributeHandle));
      }
      for (auto const& value : savedClass.subscribedUpdateRateDesignators) {
        static_cast<void>(attributeHandles.insert(value.attributeHandle));
      }
      for (auto const& subscription : savedClass.regionalSubscribedAttributes) {
        static_cast<void>(attributeHandles.insert(subscription.attributeHandle));
        if (!federation.regions.contains(subscription.regionHandle)) {
          throw std::logic_error(
              "The saved regional object-class declaration references an unknown region.");
        }
      }
      for (auto const& value : savedClass.regionalSubscribedUpdateRateDesignators) {
        static_cast<void>(attributeHandles.insert(value.attributeHandle));
        if (!federation.regions.contains(value.regionHandle)) {
          throw std::logic_error(
              "The saved regional update-rate declaration references an unknown region.");
        }
      }
      for (auto const& value : savedClass.defaultTransportationTypes) {
        static_cast<void>(attributeHandles.insert(value.attributeHandle));
      }
      for (auto const& value : savedClass.defaultOrderTypes) {
        static_cast<void>(attributeHandles.insert(value.attributeHandle));
        if (value.orderType == 0U || value.orderType > 2U) {
          throw std::logic_error(
              "The saved object-class declaration has an invalid order type.");
        }
      }
      if (!validObjectClassAttributes(
              federation, savedClass.objectClassHandle, attributeHandles)) {
        throw std::logic_error(
            "The saved object-class attribute declarations reference an unknown attribute.");
      }

      Federation::ObjectClassAttributeDeclarations::PerObjectClass perClass;
      perClass.privilegeToDeleteExplicitlyUnpublished =
          savedClass.privilegeToDeleteExplicitlyUnpublished;
      perClass.explicitlyPublishedAttributes.insert(
          savedClass.explicitlyPublishedAttributeHandles.begin(),
          savedClass.explicitlyPublishedAttributeHandles.end());
      for (auto const& subscription : savedClass.subscribedAttributes) {
        auto const [position, inserted] = perClass.subscribedAttributes.emplace(
            subscription.attributeHandle, subscription.active);
        static_cast<void>(position);
        if (!inserted) {
          throw std::logic_error(
              "The saved object-class declarations contain duplicate subscriptions.");
        }
      }
      for (auto const& value : savedClass.subscribedUpdateRateDesignators) {
        auto const [position, inserted] = perClass.subscribedUpdateRateDesignators.emplace(
            value.attributeHandle, value.value);
        static_cast<void>(position);
        if (!inserted) {
          throw std::logic_error(
              "The saved object-class declarations contain duplicate update rates.");
        }
      }
      for (auto const& subscription : savedClass.regionalSubscribedAttributes) {
        auto& regions = perClass.regionalSubscribedAttributes[subscription.attributeHandle];
        auto const [position, inserted] = regions.emplace(
            subscription.regionHandle, subscription.active);
        static_cast<void>(position);
        if (!inserted) {
          throw std::logic_error(
              "The saved regional object-class declarations contain duplicate subscriptions.");
        }
      }
      for (auto const& value : savedClass.regionalSubscribedUpdateRateDesignators) {
        auto& regions =
            perClass.regionalSubscribedUpdateRateDesignators[value.attributeHandle];
        auto const [position, inserted] = regions.emplace(value.regionHandle, value.value);
        static_cast<void>(position);
        if (!inserted) {
          throw std::logic_error(
              "The saved regional declarations contain duplicate update rates.");
        }
      }
      for (auto const& value : savedClass.defaultTransportationTypes) {
        auto const [position, inserted] = perClass.defaultTransportationTypes.emplace(
            value.attributeHandle, value.value);
        static_cast<void>(position);
        if (!inserted || value.value.empty() ||
            !isSupportedTransportationName(
                federation.definition.catalog.get(), value.value)) {
          throw std::logic_error(
              "The saved object-class declaration has an invalid default transportation.");
        }
      }
      for (auto const& value : savedClass.defaultOrderTypes) {
        auto const [position, inserted] = perClass.defaultOrderTypes.emplace(
            value.attributeHandle,
            static_cast<rti1516_2025::OrderType>(value.orderType));
        static_cast<void>(position);
        if (!inserted) {
          throw std::logic_error(
              "The saved object-class declarations contain duplicate default orders.");
        }
      }

      auto const [classPosition, classInserted] = declarations.byObjectClass.emplace(
          savedClass.objectClassHandle, std::move(perClass));
      static_cast<void>(classPosition);
      if (!classInserted) {
        throw std::logic_error(
            "The saved object-class declarations contain duplicate classes.");
      }
    }

    auto const [position, inserted] = federation.objectClassAttributeDeclarations.emplace(
        savedDeclarations.federateId, std::move(declarations));
    static_cast<void>(position);
    if (!inserted) {
      throw std::logic_error(
          "The saved object-class declarations contain duplicate federates.");
    }
  }
}

void EmbeddedFederationRegistry::restoreInteractionDeclarationsFromStateImage(
    Federation& federation,
    FederationStateImage const& image,
    Federation const& liveFederation) {
  if (image.interactionDeclarations.empty()) {
    return;
  }
  if (!isRouteFreeInteractionDeclarationImage(image) &&
      !isRouteFreeMultipleInteractionPublicationImage(image) &&
      !isRouteFreeMixedMultipleInteractionDeclarationImage(image) &&
      !isRouteFreeMultipleDirectedInteractionDeclarationImage(image) &&
      !isRouteFreeRegionalTsoInteractionImage(image)) {
    throw std::logic_error(
        "The saved interaction declarations are outside the route-free restore boundary.");
  }

  std::map<std::uint64_t, Federation::FederateInteractionDeclarations> restored;
  for (auto const& savedDeclarations : image.interactionDeclarations) {
    if (savedDeclarations.federateId == 0U ||
        !federation.members.contains(savedDeclarations.federateId) ||
        !liveFederation.members.contains(savedDeclarations.federateId)) {
      throw std::logic_error(
          "The saved interaction declarations have no live joined federate.");
    }

    std::uint64_t interactionClassHandle = 0U;
    if (!savedDeclarations.publishedInteractionClasses.empty()) {
      interactionClassHandle = savedDeclarations.publishedInteractionClasses.front();
    } else if (!savedDeclarations.subscribedInteractionClasses.empty()) {
      interactionClassHandle =
          savedDeclarations.subscribedInteractionClasses.front().interactionClassHandle;
    } else if (!savedDeclarations.regionalSubscribedInteractionClasses.empty()) {
      interactionClassHandle =
          savedDeclarations.regionalSubscribedInteractionClasses.front().interactionClassHandle;
    } else if (!savedDeclarations.publishedObjectClassDirectedInteractions.empty()) {
      interactionClassHandle =
          savedDeclarations.publishedObjectClassDirectedInteractions.front().interactionClassHandle;
    } else if (!savedDeclarations.subscribedObjectClassDirectedInteractions.empty()) {
      interactionClassHandle =
          savedDeclarations.subscribedObjectClassDirectedInteractions.front().interactionClassHandle;
    }
    if (interactionClassHandle == 0U) {
      throw std::logic_error(
          "The saved interaction declaration has no interaction class identity.");
    }
    Federation::FederateInteractionDeclarations declarations;
    if (!savedDeclarations.publishedInteractionClasses.empty()) {
      for (auto const publishedClassHandle :
           savedDeclarations.publishedInteractionClasses) {
        if (!validInteractionClass(federation, publishedClassHandle)) {
          throw std::logic_error(
              "The saved interaction declaration references an invalid interaction class.");
        }
        declarations.publishedInteractionClasses.insert(publishedClassHandle);
      }
    } else if (!validInteractionClass(federation, interactionClassHandle)) {
      throw std::logic_error(
          "The saved interaction declaration references an invalid interaction class.");
    }
    if (!savedDeclarations.interactionTransportationTypes.empty()) {
      auto const& transportation =
          savedDeclarations.interactionTransportationTypes.front();
      if (transportation.interactionClassHandle != interactionClassHandle ||
          !declarations.publishedInteractionClasses.contains(
              transportation.interactionClassHandle) ||
          !isSupportedTransportationName(
              federation.definition.catalog.get(), transportation.value)) {
        throw std::logic_error(
            "The saved interaction transportation override references an invalid class or type.");
      }
      declarations.interactionTransportationTypes.emplace(
          transportation.interactionClassHandle,
          transportation.value);
    }
    if (!savedDeclarations.interactionOrderTypes.empty()) {
      for (auto const& order : savedDeclarations.interactionOrderTypes) {
        bool const directedPublication = std::any_of(
            savedDeclarations.publishedObjectClassDirectedInteractions.begin(),
            savedDeclarations.publishedObjectClassDirectedInteractions.end(),
            [&](FederationStateImagePublishedDirectedInteraction const& directed) {
              return directed.interactionClassHandle == order.interactionClassHandle;
            });
        if (order.interactionClassHandle != interactionClassHandle ||
            (order.orderType != static_cast<std::uint32_t>(rti1516_2025::RECEIVE) &&
             order.orderType != static_cast<std::uint32_t>(rti1516_2025::TIMESTAMP)) ||
            (!declarations.publishedInteractionClasses.contains(
                 order.interactionClassHandle) &&
             !directedPublication)) {
          throw std::logic_error(
              "The saved interaction order override references an invalid class or order.");
        }
        declarations.interactionOrderTypes.emplace(
            order.interactionClassHandle,
            static_cast<rti1516_2025::OrderType>(order.orderType));
      }
    }
    if (!savedDeclarations.subscribedInteractionClasses.empty()) {
      auto const& subscription =
          savedDeclarations.subscribedInteractionClasses.front();
      if (subscription.interactionClassHandle != interactionClassHandle) {
        throw std::logic_error(
            "The saved interaction subscription references an inconsistent class.");
      }
      declarations.subscribedInteractionClasses.emplace(
          interactionClassHandle,
          subscription.active);
    }
    if (!savedDeclarations.regionalSubscribedInteractionClasses.empty()) {
      auto const& subscription =
          savedDeclarations.regionalSubscribedInteractionClasses.front();
      auto const region = federation.regions.find(subscription.regionHandle);
      auto const availableDimensions =
          availableInteractionDimensions(federation, interactionClassHandle);
      if (subscription.interactionClassHandle != interactionClassHandle ||
          region == federation.regions.end() ||
          region->second.ownerFederateId != savedDeclarations.federateId ||
          !region->second.specificationCommitted ||
          region->second.committedRangeBounds.size() !=
              region->second.dimensionHandles.size() ||
          !availableDimensions ||
          !std::includes(
              availableDimensions->begin(),
              availableDimensions->end(),
              region->second.dimensionHandles.begin(),
              region->second.dimensionHandles.end())) {
        throw std::logic_error(
            "The saved regional interaction subscription references an invalid region.");
      }
      declarations.regionalSubscribedInteractionClasses[interactionClassHandle].emplace(
          subscription.regionHandle,
          subscription.active);
    }
    for (auto const& directed : savedDeclarations.publishedObjectClassDirectedInteractions) {
      if (!validDirectedInteractionForObjectClass(
              federation,
              directed.objectClassHandle,
              directed.interactionClassHandle)) {
        throw std::logic_error(
            "The saved directed interaction publication references an invalid class pair.");
      }
      declarations.publishedObjectClassDirectedInteractions[directed.objectClassHandle].insert(
          directed.interactionClassHandle);
    }
    for (auto const& directed : savedDeclarations.subscribedObjectClassDirectedInteractions) {
      if (!validDirectedInteractionForObjectClass(
              federation,
              directed.objectClassHandle,
              directed.interactionClassHandle)) {
        throw std::logic_error(
            "The saved directed interaction subscription references an invalid class pair.");
      }
      declarations.subscribedObjectClassDirectedInteractions[directed.objectClassHandle].emplace(
          directed.interactionClassHandle,
          directed.active);
    }
    if (!savedDeclarations.pendingInteractionTransportationTypeChanges.empty()) {
      auto const& pending =
          savedDeclarations.pendingInteractionTransportationTypeChanges.front();
      if (pending.interactionClassHandle != interactionClassHandle ||
          !isSupportedTransportationName(
              federation.definition.catalog.get(), pending.value)) {
        throw std::logic_error(
            "The saved interaction transportation change references an invalid class or type.");
      }
      declarations.pendingInteractionTransportationTypeChanges.emplace(
          interactionClassHandle,
          pending.value);
    }
    auto const [position, inserted] = restored.emplace(
        savedDeclarations.federateId,
        std::move(declarations));
    static_cast<void>(position);
    if (!inserted) {
      throw std::logic_error(
          "The saved interaction declarations contain a duplicate federate.");
    }
  }
  federation.interactionDeclarations = std::move(restored);
}

} // namespace umbra::detail
