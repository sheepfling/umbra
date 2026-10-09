#include "internal/federation/federation_registry.hpp"
#include "internal/federation/federation_registry_value_helpers.hpp"
#include "internal/fom/fom_catalog.hpp"

#include <cstdint>
#include <map>
#include <optional>
#include <set>
#include <string>
#include <utility>
#include <vector>

namespace umbra::detail {

std::optional<ReceiveOrderAttributeUpdateRecipient>
EmbeddedFederationRegistry::candidateReceiveOrderAttributeUpdateRecipient(
    Federation const& federation,
    std::uint64_t producingFederateId,
    std::uint64_t receivingFederateId,
    std::uint64_t objectInstanceHandle,
    std::vector<std::uint64_t> const& sentAttributeHandles,
    std::set<std::uint64_t> const* sentRegionHandles,
    std::map<std::uint64_t, RegionSpecificationSnapshot> const* regionOverrides) {
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
  // A receive-order Delete Object Instance commits the federation-wide
  // deletion before each recipient's queued Remove Object Instance callback
  // begins.  A timestamped reflection may already be in flight at that
  // recipient's grant boundary, so do not project it through the still-known
  // object until the removal callback has had a chance to run.  A departed
  // producer is the deliberate exception: the connection-loss cutoff retains
  // accepted payloads through the producer's last-known time before its
  // automatic cleanup is delivered.
  if (instance == federation.objectInstances.end() ||
      (instance->second.deleteAccepted &&
       federation.members.contains(producingFederateId))) {
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

  auto const registeredClassName = federation.objectClassHandles->nameFor(
      instance->second.registeredObjectClassHandle);
  if (!registeredClassName ||
      federation.definition.catalog->objectClass(*registeredClassName) == nullptr) {
    return std::nullopt;
  }

  // Passive subscriptions do not establish update relevance, but they still
  // receive a reflection when another joined federate's active declaration
  // has made this attribute update relevant. Keep the source-region test in
  // this predicate so an active regional declaration cannot make a disjoint
  // passive regional subscriber eligible.
  auto activeSubscriptionEstablishedForAttribute = [&](std::uint64_t attributeHandle) {
    bool established = false;
    for (auto const& [candidateFederateId, candidateDeclarations] :
         federation.objectClassAttributeDeclarations) {
      if (candidateFederateId == producingFederateId ||
          !federation.members.contains(candidateFederateId)) {
        continue;
      }
      std::set<std::string> candidateVisited;
      std::string candidateClassName = *registeredClassName;
      while (!candidateClassName.empty() && candidateVisited.insert(candidateClassName).second) {
        auto const* candidateClass = federation.definition.catalog->objectClass(candidateClassName);
        auto const candidateClassHandle = federation.objectClassHandles->handleFor(
            candidateClassName);
        if (candidateClass == nullptr || !candidateClassHandle) {
          break;
        }
        auto const candidatePerClass = candidateDeclarations.byObjectClass.find(
            *candidateClassHandle);
        if (candidatePerClass != candidateDeclarations.byObjectClass.end() &&
            federation.attributeHandles->nameFor(
                federation.definition.catalog.get(),
                candidateClassName,
                attributeHandle)) {
          auto const regional = candidatePerClass->second.regionalSubscribedAttributes.find(
              attributeHandle);
          auto const associated = instance->second.updateRegionsByAttribute.find(
              attributeHandle);
          bool const hasExplicitSubscriptionRegion =
              regional != candidatePerClass->second.regionalSubscribedAttributes.end() &&
              !regional->second.empty();
          bool const hasExplicitUpdateRegion =
              (sentRegionHandles != nullptr && !sentRegionHandles->empty()) ||
              (associated != instance->second.updateRegionsByAttribute.end() &&
               !associated->second.empty());
          auto const ordinarySubscription = candidatePerClass->second.subscribedAttributes.find(
              attributeHandle);
          if (ordinarySubscription != candidatePerClass->second.subscribedAttributes.end() &&
              ordinarySubscription->second &&
              !hasExplicitUpdateRegion) {
            established = true;
            break;
          }
          if (ordinarySubscription != candidatePerClass->second.subscribedAttributes.end() &&
              ordinarySubscription->second &&
              hasExplicitUpdateRegion) {
            // An ordinary subscription uses the RTI-owned default region.
            // An explicit source association remains eligible when its
            // committed realization overlaps that default region.
            if (sentRegionHandles != nullptr && !sentRegionHandles->empty()) {
              for (std::uint64_t const sentRegionHandle : *sentRegionHandles) {
                if (regionOverlapsDefault(
                        federation,
                        sentRegionHandle,
                        regionOverrides)) {
                  established = true;
                  break;
                }
              }
            } else if (associated != instance->second.updateRegionsByAttribute.end()) {
              for (std::uint64_t const associatedRegionHandle : associated->second) {
                if (regionOverlapsDefault(
                        federation,
                        associatedRegionHandle,
                        regionOverrides)) {
                  established = true;
                  break;
                }
              }
            }
            if (established) {
              break;
            }
          }
          if (hasExplicitSubscriptionRegion) {
            std::set<std::uint64_t> currentSentRegions;
            bool const sourceHasResigned =
                regionOverrides != nullptr && !federation.members.contains(producingFederateId);
            if (sentRegionHandles != nullptr && !sentRegionHandles->empty()) {
              if (sourceHasResigned) {
                currentSentRegions.insert(
                    sentRegionHandles->begin(),
                    sentRegionHandles->end());
              } else if (associated != instance->second.updateRegionsByAttribute.end()) {
                for (std::uint64_t const sentRegionHandle : *sentRegionHandles) {
                  if (associated->second.contains(sentRegionHandle)) {
                    currentSentRegions.insert(sentRegionHandle);
                  }
                }
              }
            } else if (associated != instance->second.updateRegionsByAttribute.end()) {
              currentSentRegions = associated->second;
            }
            for (auto const& [subscribedRegionHandle, active] : regional->second) {
              if (!active) {
                continue;
              }
              if (!hasExplicitUpdateRegion) {
                if (regionOverlapsDefault(
                        federation,
                        subscribedRegionHandle,
                        regionOverrides)) {
                  established = true;
                  break;
                }
                continue;
              }
              for (std::uint64_t const sentRegionHandle : currentSentRegions) {
                if (regionsOverlap(
                        federation,
                        subscribedRegionHandle,
                        sentRegionHandle,
                        regionOverrides)) {
                  established = true;
                  break;
                }
              }
              if (established) {
                break;
              }
            }
          }
        }
        if (established) {
          break;
        }
        candidateClassName = candidateClass->parentName;
      }
      if (established) {
        break;
      }
    }
    return established;
  };

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
    auto const associatedUpdateRegions = instance->second.updateRegionsByAttribute.find(
        attributeHandle);
    bool const hasExplicitUpdateRegion =
        (sentRegionHandles != nullptr && !sentRegionHandles->empty()) ||
        (associatedUpdateRegions != instance->second.updateRegionsByAttribute.end() &&
         !associatedUpdateRegions->second.empty());
    std::set<std::uint64_t> currentSentRegions;
    bool const sourceHasResigned =
        regionOverrides != nullptr && !federation.members.contains(producingFederateId);
    bool const activeSubscriptionEstablished =
        activeSubscriptionEstablishedForAttribute(attributeHandle);
    bool subscribed = ordinarySubscription != perClass->second.subscribedAttributes.end() &&
        (ordinarySubscription->second || activeSubscriptionEstablished);
    if (subscribed && hasExplicitUpdateRegion) {
      subscribed = false;
      if (sentRegionHandles != nullptr && !sentRegionHandles->empty()) {
        for (std::uint64_t const sentRegionHandle : *sentRegionHandles) {
          if (regionOverlapsDefault(
                  federation,
                  sentRegionHandle,
                  regionOverrides)) {
            subscribed = true;
            break;
          }
        }
      } else if (associatedUpdateRegions != instance->second.updateRegionsByAttribute.end()) {
        for (std::uint64_t const associatedRegionHandle : associatedUpdateRegions->second) {
          if (regionOverlapsDefault(
                  federation,
                  associatedRegionHandle,
                  regionOverrides)) {
            subscribed = true;
            break;
          }
        }
      }
    }
    if (!subscribed && hasExplicitSubscriptionRegion) {
      // A voluntary source resignation removes the producer from the live
      // membership/region ledgers before a queued callback is reconstructed.
      // The invocation snapshot is valid for both that lifetime boundary and
      // source-range mutation while the producer remains joined. The live
      // object association is still checked below, so replacing the
      // association suppresses a stale passel rather than retargeting it.
      if (sentRegionHandles == nullptr || sentRegionHandles->empty()) {
        for (auto const& [subscribedRegionHandle, active] : regional->second) {
          if (active && regionOverlapsDefault(federation, subscribedRegionHandle)) {
            subscribed = true;
            break;
          }
        }
      } else {
        if (sourceHasResigned) {
          // The producer's live object ledger is gone after resignation; use
          // the accepted invocation-time association in that case.
          currentSentRegions.insert(
              sentRegionHandles->begin(),
              sentRegionHandles->end());
        } else {
          auto const associated = instance->second.updateRegionsByAttribute.find(
              attributeHandle);
          if (associated == instance->second.updateRegionsByAttribute.end()) {
            continue;
          }
          for (std::uint64_t const sentRegionHandle : *sentRegionHandles) {
            if (associated->second.contains(sentRegionHandle)) {
              currentSentRegions.insert(sentRegionHandle);
            }
          }
        }
        for (auto const& [subscribedRegionHandle, active] : regional->second) {
          if (!active && !activeSubscriptionEstablished) {
            continue;
          }
          for (std::uint64_t const sentRegionHandle : currentSentRegions) {
            if (regionsOverlap(
                    federation,
                    subscribedRegionHandle,
                    sentRegionHandle,
                    regionOverrides)) {
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
    if (subscribed) {
      recipient.receivedAttributeHandles.insert(attributeHandle);
      // Regional subscriptions carry their own rate per subscribed region.
      // Select only active regions that overlap this passel's source region;
      // an active omitted/default declaration means no reduction for the
      // projected attribute.  Ordinary and regional declarations remain
      // independent, so their eligible rates are considered together.
      bool hasUnspecifiedDefault = false;
      std::optional<std::pair<double, std::string>> maximumExplicitRate;
      auto considerDesignator = [&](std::string const& candidate) {
        if (candidate.empty()) {
          hasUnspecifiedDefault = true;
          return;
        }
        auto const value = updateRateValueForNormalizedDesignator(
            *federation.definition.catalog,
            candidate);
        if (!value) {
          return;
        }
        if (!maximumExplicitRate ||
            *value > maximumExplicitRate->first ||
            (*value == maximumExplicitRate->first &&
             candidate < maximumExplicitRate->second)) {
          maximumExplicitRate = std::make_pair(*value, candidate);
        }
      };

      auto const ordinary = perClass->second.subscribedAttributes.find(
          attributeHandle);
      if (ordinary != perClass->second.subscribedAttributes.end() &&
          ordinary->second &&
          (!hasExplicitUpdateRegion || [&] {
            for (std::uint64_t const associatedRegionHandle : associatedUpdateRegions->second) {
              if (regionOverlapsDefault(federation, associatedRegionHandle, regionOverrides)) {
                return true;
              }
            }
            return false;
          }())) {
        auto const rate = perClass->second.subscribedUpdateRateDesignators.find(
            attributeHandle);
        considerDesignator(
            rate == perClass->second.subscribedUpdateRateDesignators.end()
                ? std::string{}
                : rate->second);
      }
      if (hasExplicitSubscriptionRegion) {
        auto const regionalDesignators =
            perClass->second.regionalSubscribedUpdateRateDesignators.find(
                attributeHandle);
        for (auto const& [subscribedRegionHandle, active] : regional->second) {
          if (!active) {
            continue;
          }
          bool overlaps = false;
          if (sentRegionHandles == nullptr || sentRegionHandles->empty()) {
            overlaps = regionOverlapsDefault(
                federation,
                subscribedRegionHandle,
                regionOverrides);
          } else {
            for (std::uint64_t const sentRegionHandle : currentSentRegions) {
              if (regionsOverlap(
                      federation,
                      subscribedRegionHandle,
                      sentRegionHandle,
                      regionOverrides)) {
                overlaps = true;
                break;
              }
            }
          }
          if (!overlaps) {
            continue;
          }
          std::string designator;
          if (regionalDesignators !=
              perClass->second.regionalSubscribedUpdateRateDesignators.end()) {
            auto const stored = regionalDesignators->second.find(subscribedRegionHandle);
            if (stored != regionalDesignators->second.end()) {
              designator = stored->second;
            }
          }
          considerDesignator(designator);
        }
      }
      // HLAdefault is represented by an absent entry.  It is the
      // no-reduction path, not a projection-wide fallback; retaining only an
      // all-explicit maximum keeps mixed subscriptions independent.
      if (!hasUnspecifiedDefault && maximumExplicitRate) {
        recipient.maximumUpdateRatesByAttribute[attributeHandle] =
            maximumExplicitRate->first;
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

} // namespace umbra::detail
