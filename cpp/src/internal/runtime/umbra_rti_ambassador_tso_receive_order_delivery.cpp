#include "internal/callbacks/callback_session.hpp"
#include "internal/federation/federation_registry.hpp"
#include "internal/handles/attribute_handle.hpp"
#include "internal/handles/federate_handle.hpp"
#include "internal/handles/interaction_class_handle.hpp"
#include "internal/handles/object_instance_handle.hpp"
#include "internal/handles/parameter_handle.hpp"
#include "internal/handles/region_handle.hpp"
#include "internal/handles/transportation_type_handle.hpp"
#include "internal/runtime/ambassador_shared_utilities.hpp"
#include "internal/runtime/callback_region_designator_copies.hpp"
#include "internal/runtime/utf8_string.hpp"

#include <RTI/FederateAmbassador.h>
#include <RTI/Exception.h>

#include <algorithm>
#include <mutex>
#include <optional>
#include <set>
#include <type_traits>
#include <utility>

namespace rti1516_2025::umbra_binding_detail {
namespace {

using umbra::detail::FederationTsoDeliveryStatus;
using umbra::detail::FederationTsoRegistryStatus;
using umbra::detail::TsoPayloadDelivery;

void completeTsoDelivery(
    std::wstring const& federationName,
    umbra::detail::TsoQueuedMessage const& queuedMessage) {
  std::scoped_lock lock(ambassadorFederationManagementMutex());
  auto const completed = embeddedFederationRegistry().completeTsoDelivery(
      federationName, queuedMessage);
  if (completed.status != FederationTsoRegistryStatus::applied ||
      (completed.delivery.status != FederationTsoDeliveryStatus::applied &&
       completed.delivery.status !=
           FederationTsoDeliveryStatus::message_already_completed)) {
    throw RTIinternalError(
        L"The embedded federation could not complete receive-order TSO delivery.");
  }
}

void suppressAndCompleteTsoDelivery(
    std::wstring const& federationName,
    std::uint64_t receivingFederateId,
    umbra::detail::TsoQueuedMessage const& queuedMessage) {
  {
    std::scoped_lock lock(ambassadorFederationManagementMutex());
    static_cast<void>(embeddedFederationRegistry()
                          .finishTsoRecipientCallbackSuppressed(
                              federationName,
                              receivingFederateId,
                              queuedMessage.messageId));
  }
  completeTsoDelivery(federationName, queuedMessage);
}

template <typename Delivery>
void queueInteractionAsReceiveOrder(
    std::wstring const& federationName,
    std::uint64_t receivingFederateId,
    Delivery delivery) {
  auto const& message = delivery.message;
  std::optional<umbra::detail::ReceiveOrderInteractionRecipient> initialProjection;
  {
    std::scoped_lock lock(ambassadorFederationManagementMutex());
    initialProjection = embeddedFederationRegistry().receiveOrderInteractionRecipientFor(
        federationName,
        message.producingFederateId,
        receivingFederateId,
        message.sentInteractionClassHandle,
        message.sentParameterHandles,
        message.sentRegionHandles.empty() ? nullptr : &message.sentRegionHandles,
        message.sentRegionSnapshots.empty() ? nullptr : &message.sentRegionSnapshots);
  }
  if (!initialProjection) {
    suppressAndCompleteTsoDelivery(
        federationName, receivingFederateId, delivery.queuedMessage);
    return;
  }

  try {
    submitAmbassadorReceiveOrderCallback(
        initialProjection->callbackRoute,
        federationName,
        receivingFederateId,
        [federationName,
         receivingFederateId,
         delivery = std::move(delivery)](FederateAmbassador& recipient) mutable {
          auto const& message = delivery.message;
          std::optional<umbra::detail::ReceiveOrderInteractionRecipient> projection;
          bool callbackMayBegin = false;
          {
            std::scoped_lock lock(ambassadorFederationManagementMutex());
            auto& registry = embeddedFederationRegistry();
            projection = registry.receiveOrderInteractionRecipientFor(
                federationName,
                message.producingFederateId,
                receivingFederateId,
                message.sentInteractionClassHandle,
                message.sentParameterHandles,
                message.sentRegionHandles.empty()
                    ? nullptr
                    : &message.sentRegionHandles,
                message.sentRegionSnapshots.empty()
                    ? nullptr
                    : &message.sentRegionSnapshots);
            if (projection) {
              callbackMayBegin = registry.beginTsoInteractionCallback(
                  federationName,
                  receivingFederateId,
                  message.messageId,
                  true);
            }
          }
          if (!projection || !callbackMayBegin) {
            suppressAndCompleteTsoDelivery(
                federationName,
                receivingFederateId,
                delivery.queuedMessage);
            return;
          }

          try {
            auto const transportationName = umbra::detail::wideFromUtf8(
                message.transportationName);
            auto const transportationValue = transportationName
                ? ambassadorTransportationTypeValueForFederation(
                      federationName, *transportationName)
                : std::nullopt;
            if (!transportationValue) {
              throw RTIinternalError(
                  L"The embedded federation could not reconstruct a receive-order interaction transportation type.");
            }
            ParameterHandleValueMap parameterValues =
                ambassadorProjectInteractionParameterValues(
                    message.parameters,
                    projection->receivedParameterHandles);
            std::optional<RegionHandleSet> sentRegions;
            CallbackRegionDesignatorCopies regionCopies(
                federationName, receivingFederateId);
            if (projection->conveyRegionDesignatorSets &&
                (!message.sentRegionHandles.empty() || message.defaultRegionUsed)) {
              sentRegions.emplace();
              if (!message.sentRegionHandles.empty()) {
                auto const copyStatus = regionCopies.create(
                    message.sentRegionHandles,
                    message.sentRegionSnapshots,
                    *sentRegions);
                if (copyStatus != umbra::detail::RegionServiceStatus::applied) {
                  throw RTIinternalError(
                      L"The RTI could not create receive-order callback region designator copies.");
                }
              }
            }
            recordAmbassadorSuccessfulInteractionReceipt(
                federationName,
                receivingFederateId,
                message.sentInteractionClassHandle,
                message.transportationName,
                false);
            recipient.receiveInteraction(
                makeInteractionClassHandle(
                    projection->receivedInteractionClassHandle),
                parameterValues,
                message.userSuppliedTag,
                makeTransportationTypeHandle(*transportationValue),
                makeFederateHandle(message.producingFederateId),
                sentRegions ? &*sentRegions : nullptr);
          } catch (...) {
            completeTsoDelivery(federationName, delivery.queuedMessage);
            throw;
          }
          completeTsoDelivery(federationName, delivery.queuedMessage);
        });
  } catch (...) {
    completeTsoDelivery(federationName, delivery.queuedMessage);
    throw;
  }
}

void queueAttributeUpdateAsReceiveOrder(
    std::wstring const& federationName,
    std::uint64_t receivingFederateId,
    umbra::detail::TsoAttributeUpdateDelivery delivery) {
  auto const& message = delivery.message;
  std::optional<umbra::detail::ReceiveOrderAttributeUpdateRecipient>
      initialProjection;
  auto const passels = message.passelsByRecipient.find(receivingFederateId);
  if (passels != message.passelsByRecipient.end()) {
    for (auto const& passel : passels->second) {
      std::scoped_lock lock(ambassadorFederationManagementMutex());
      initialProjection = embeddedFederationRegistry()
          .receiveOrderAttributeUpdateRecipientFor(
              federationName,
              message.producingFederateId,
              receivingFederateId,
              message.objectInstanceHandle,
              passel.sentAttributeHandles,
              passel.sentRegionHandles.empty()
                  ? nullptr
                  : &passel.sentRegionHandles,
              passel.sentRegionSnapshots.empty()
                  ? nullptr
                  : &passel.sentRegionSnapshots);
      if (initialProjection) {
        break;
      }
    }
  }
  if (!initialProjection) {
    suppressAndCompleteTsoDelivery(
        federationName, receivingFederateId, delivery.queuedMessage);
    return;
  }

  try {
    submitAmbassadorReceiveOrderCallback(
        initialProjection->callbackRoute,
        federationName,
        receivingFederateId,
        [federationName,
         receivingFederateId,
         delivery = std::move(delivery)](FederateAmbassador& recipient) mutable {
          auto const& message = delivery.message;
          bool callbackBegan = false;
          auto const passels = message.passelsByRecipient.find(receivingFederateId);
          if (passels != message.passelsByRecipient.end()) {
            try {
              for (auto const& passel : passels->second) {
                std::optional<umbra::detail::ReceiveOrderAttributeUpdateRecipient>
                    projection;
                {
                  std::scoped_lock lock(ambassadorFederationManagementMutex());
                  projection = embeddedFederationRegistry()
                      .receiveOrderAttributeUpdateRecipientFor(
                          federationName,
                          message.producingFederateId,
                          receivingFederateId,
                          message.objectInstanceHandle,
                          passel.sentAttributeHandles,
                          passel.sentRegionHandles.empty()
                              ? nullptr
                              : &passel.sentRegionHandles,
                          passel.sentRegionSnapshots.empty()
                              ? nullptr
                              : &passel.sentRegionSnapshots);
                }
                if (!projection) {
                  continue;
                }
                auto const transportationName = umbra::detail::wideFromUtf8(
                    passel.transportationName);
                auto const transportationValue = transportationName
                    ? ambassadorTransportationTypeValueForFederation(
                          federationName, *transportationName)
                    : std::nullopt;
                auto const reliableTransportation =
                    ambassadorTransportationTypeReliableForFederation(
                        federationName, passel.transportationName);
                if (!transportationValue || !reliableTransportation) {
                  throw RTIinternalError(
                      L"The embedded federation could not reconstruct receive-order attribute transportation policy.");
                }
                AttributeHandleValueMap attributeValues =
                    ambassadorProjectAttributeValues(
                        message.attributes,
                        projection->receivedAttributeHandles);
                if (!*reliableTransportation) {
                  for (auto iterator = attributeValues.begin();
                       iterator != attributeValues.end();) {
                    std::uint64_t numeric = 0U;
                    for (auto const candidate : passel.sentAttributeHandles) {
                      if (makeAttributeHandle(candidate) == iterator->first) {
                        numeric = candidate;
                        break;
                      }
                    }
                    auto const rate =
                        projection->maximumUpdateRatesByAttribute.find(numeric);
                    auto const key = ambassadorUpdateRateAdmissionKey(
                        federationName,
                        receivingFederateId,
                        message.objectInstanceHandle,
                        projection->subscriptionGeneration,
                        numeric);
                    bool const admitted = !key || ambassadorAdmitUpdateRate(
                        *key,
                        rate == projection->maximumUpdateRatesByAttribute.end()
                            ? 0.0
                            : rate->second,
                        false);
                    if (!admitted) {
                      iterator = attributeValues.erase(iterator);
                    } else {
                      ++iterator;
                    }
                  }
                }
                if (attributeValues.empty()) {
                  continue;
                }
                bool callbackMayBegin = false;
                {
                  std::scoped_lock lock(ambassadorFederationManagementMutex());
                  callbackMayBegin = embeddedFederationRegistry()
                      .beginTsoAttributeUpdateCallback(
                          federationName,
                          receivingFederateId,
                          message.messageId,
                          true);
                }
                if (!callbackMayBegin) {
                  continue;
                }
                callbackBegan = true;
                {
                  std::scoped_lock lock(ambassadorFederationManagementMutex());
                  static_cast<void>(embeddedFederationRegistry()
                                        .recordSuccessfulObjectInstanceReflection(
                                            federationName,
                                            receivingFederateId,
                                            message.objectInstanceHandle));
                }
                recordAmbassadorSuccessfulReflectionReceipt(
                    federationName,
                    receivingFederateId,
                    message.objectInstanceHandle,
                    passel.transportationName);
                CallbackRegionDesignatorCopies regionCopies(
                    federationName, receivingFederateId);
                std::optional<RegionHandleSet> sentRegions;
                if (projection->conveyRegionDesignatorSets &&
                    (!passel.sentRegionHandles.empty() || passel.defaultRegionUsed)) {
                  sentRegions.emplace();
                  if (!passel.sentRegionHandles.empty()) {
                    auto const copyStatus = regionCopies.create(
                        passel.sentRegionHandles,
                        passel.sentRegionSnapshots,
                        *sentRegions);
                    if (copyStatus != umbra::detail::RegionServiceStatus::applied) {
                      throw RTIinternalError(
                          L"The RTI could not create receive-order callback region designator copies.");
                    }
                  }
                }
                recipient.reflectAttributeValues(
                    makeObjectInstanceHandle(message.objectInstanceHandle),
                    attributeValues,
                    message.userSuppliedTag,
                    makeTransportationTypeHandle(*transportationValue),
                    makeFederateHandle(message.producingFederateId),
                    sentRegions ? &*sentRegions : nullptr);
              }
            } catch (...) {
              if (!callbackBegan) {
                suppressAndCompleteTsoDelivery(
                    federationName,
                    receivingFederateId,
                    delivery.queuedMessage);
              } else {
                completeTsoDelivery(federationName, delivery.queuedMessage);
              }
              throw;
            }
          }
          if (!callbackBegan) {
            suppressAndCompleteTsoDelivery(
                federationName,
                receivingFederateId,
                delivery.queuedMessage);
            return;
          }
          completeTsoDelivery(federationName, delivery.queuedMessage);
        });
  } catch (...) {
    completeTsoDelivery(federationName, delivery.queuedMessage);
    throw;
  }
}

void queueObjectDeletionAsReceiveOrder(
    std::wstring const& federationName,
    std::uint64_t receivingFederateId,
    umbra::detail::TsoObjectDeletionDelivery delivery) {
  auto const& message = delivery.message;
  auto const target = std::find_if(
      message.recipients.begin(),
      message.recipients.end(),
      [receivingFederateId](umbra::detail::TsoObjectDeletionRecipient const& value) {
        return value.receivingFederateId == receivingFederateId;
      });
  if (target == message.recipients.end() || !target->callbackRoute) {
    suppressAndCompleteTsoDelivery(
        federationName, receivingFederateId, delivery.queuedMessage);
    return;
  }
  try {
    target->callbackRoute.enqueueReceiveOrder(
        [federationName,
         receivingFederateId,
         delivery = std::move(delivery)](FederateAmbassador& recipient) mutable {
          auto const& message = delivery.message;
          auto const target = std::find_if(
              message.recipients.begin(),
              message.recipients.end(),
              [receivingFederateId](umbra::detail::TsoObjectDeletionRecipient const& value) {
                return value.receivingFederateId == receivingFederateId;
              });
          std::optional<umbra::detail::RemovedObjectInstanceSnapshot> removal;
          if (target != message.recipients.end()) {
            std::scoped_lock lock(ambassadorFederationManagementMutex());
            removal = embeddedFederationRegistry().beginTsoObjectInstanceRemoval(
                federationName,
                receivingFederateId,
                message.objectInstanceHandle,
                message.messageId,
                true);
          }
          if (!removal) {
            suppressAndCompleteTsoDelivery(
                federationName,
                receivingFederateId,
                delivery.queuedMessage);
            return;
          }
          try {
            if (target->serviceReportRoute) {
              target->serviceReportRoute(
                  static_cast<std::uint16_t>(
                      umbra::detail::MomServiceType::object_management),
                  [objectInstanceHandle = removal->objectInstanceHandle,
                   userSuppliedTag = message.userSuppliedTag,
                   producingFederateId = removal->producingFederateId,
                   sentOrderType = message.sentOrderType](
                      std::uint32_t serialNumber) {
                    return makeAmbassadorRemoveObjectInstanceServiceReportRecord(
                        objectInstanceHandle,
                        userSuppliedTag,
                        producingFederateId,
                        sentOrderType,
                        nullptr,
                        std::nullopt,
                        std::nullopt,
                        serialNumber);
                  });
            }
            recipient.removeObjectInstance(
                makeObjectInstanceHandle(removal->objectInstanceHandle),
                message.userSuppliedTag,
                makeFederateHandle(removal->producingFederateId));
          } catch (...) {
            completeTsoDelivery(federationName, delivery.queuedMessage);
            throw;
          }
          completeTsoDelivery(federationName, delivery.queuedMessage);
        });
  } catch (...) {
    completeTsoDelivery(federationName, delivery.queuedMessage);
    throw;
  }
}

void queueDirectedInteractionAsReceiveOrder(
    std::wstring const& federationName,
    std::uint64_t receivingFederateId,
    umbra::detail::TsoDirectedInteractionDelivery delivery) {
  auto const& message = delivery.message;
  auto const target = std::find_if(
      message.recipients.begin(),
      message.recipients.end(),
      [receivingFederateId](umbra::detail::TsoDirectedInteractionRecipient const& value) {
        return value.receivingFederateId == receivingFederateId;
      });
  if (target == message.recipients.end() || !target->callbackRoute) {
    suppressAndCompleteTsoDelivery(
        federationName, receivingFederateId, delivery.queuedMessage);
    return;
  }
  try {
    target->callbackRoute.enqueueReceiveOrder(
        [federationName,
         receivingFederateId,
         delivery = std::move(delivery)](FederateAmbassador& recipient) mutable {
          auto const& message = delivery.message;
          std::optional<umbra::detail::ReceiveOrderDirectedInteractionRecipient>
              projection;
          bool callbackMayBegin = false;
          {
            std::scoped_lock lock(ambassadorFederationManagementMutex());
            auto& registry = embeddedFederationRegistry();
            projection = registry.timestampedDirectedInteractionRecipientFor(
                federationName,
                message.producingFederateId,
                receivingFederateId,
                message.objectInstanceHandle,
                message.sentInteractionClassHandle,
                message.sentParameterHandles,
                message.messageId);
            if (projection) {
              callbackMayBegin = registry.beginTsoInteractionCallback(
                  federationName,
                  receivingFederateId,
                  message.messageId,
                  true);
            }
          }
          if (!projection || !callbackMayBegin) {
            suppressAndCompleteTsoDelivery(
                federationName,
                receivingFederateId,
                delivery.queuedMessage);
            return;
          }
          try {
            auto const transportationName = umbra::detail::wideFromUtf8(
                message.transportationName);
            auto const transportationValue = transportationName
                ? ambassadorTransportationTypeValueForFederation(
                      federationName, *transportationName)
                : std::nullopt;
            if (!transportationValue) {
              throw RTIinternalError(
                  L"The embedded federation could not reconstruct a receive-order directed-interaction transportation type.");
            }
            ParameterHandleValueMap parameterValues =
                ambassadorProjectInteractionParameterValues(
                    message.parameters,
                    projection->receivedParameterHandles);
            recordAmbassadorSuccessfulInteractionReceipt(
                federationName,
                receivingFederateId,
                message.sentInteractionClassHandle,
                message.transportationName,
                true);
            recipient.receiveDirectedInteraction(
                makeInteractionClassHandle(
                    projection->receivedInteractionClassHandle),
                makeObjectInstanceHandle(projection->objectInstanceHandle),
                parameterValues,
                message.userSuppliedTag,
                makeTransportationTypeHandle(*transportationValue),
                makeFederateHandle(message.producingFederateId));
          } catch (...) {
            completeTsoDelivery(federationName, delivery.queuedMessage);
            throw;
          }
          completeTsoDelivery(federationName, delivery.queuedMessage);
        });
  } catch (...) {
    completeTsoDelivery(federationName, delivery.queuedMessage);
    throw;
  }
}

}  // namespace

void queueAmbassadorTsoPayloadsAsReceiveOrder(
    std::wstring const& federationName,
    std::uint64_t receivingFederateId,
    std::vector<umbra::detail::TsoPayloadDelivery> deliveries) {
  for (auto& delivery : deliveries) {
    std::visit(
        [&federationName, receivingFederateId](auto&& typedDelivery) {
          using Delivery = std::decay_t<decltype(typedDelivery)>;
          if constexpr (std::is_same_v<
                            Delivery,
                            umbra::detail::TsoInteractionDelivery>) {
            queueInteractionAsReceiveOrder(
                federationName,
                receivingFederateId,
                std::move(typedDelivery));
          } else if constexpr (std::is_same_v<
                                   Delivery,
                                   umbra::detail::TsoAttributeUpdateDelivery>) {
            queueAttributeUpdateAsReceiveOrder(
                federationName,
                receivingFederateId,
                std::move(typedDelivery));
          } else if constexpr (std::is_same_v<
                                   Delivery,
                                   umbra::detail::TsoObjectDeletionDelivery>) {
            queueObjectDeletionAsReceiveOrder(
                federationName,
                receivingFederateId,
                std::move(typedDelivery));
          } else {
            queueDirectedInteractionAsReceiveOrder(
                federationName,
                receivingFederateId,
                std::move(typedDelivery));
          }
        },
        std::move(delivery));
  }
}

}  // namespace rti1516_2025::umbra_binding_detail
