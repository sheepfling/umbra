#include "internal/runtime/ambassador_shared_utilities.hpp"
#include "internal/runtime/umbra_rti_ambassador_ownership_callback_dispatch.hpp"

#include "internal/handles/attribute_handle.hpp"
#include "internal/handles/federate_handle.hpp"
#include "internal/handles/object_instance_handle.hpp"
#include "internal/encoding/variable_length_data_2025.hpp"

#if defined(UMBRA_ENABLE_EMBEDDED_FEDERATION_MANAGEMENT)
#include <RTI/FederateAmbassador.h>

#include <mutex>
#include <set>
#include <utility>
#include <vector>

namespace rti1516_2025::umbra_binding_detail {
namespace ownership_callback_detail {

void queueAttributeOwnershipQueryReport(
    umbra::detail::ObjectInstanceCallbackRoute callbackRoute,
    std::wstring federationName,
    std::uint64_t requestId,
    std::uint64_t requestingFederateId,
    std::uint64_t objectInstanceHandle,
    umbra::detail::AttributeOwnershipQueryReportKind reportKind,
    std::uint64_t owningFederateId,
    std::set<std::uint64_t> requestedAttributeHandles) {
  callbackRoute([
      federationName = std::move(federationName),
      requestId,
      requestingFederateId,
      objectInstanceHandle,
      reportKind,
      owningFederateId,
      requestedAttributeHandles = std::move(requestedAttributeHandles)](
                    FederateAmbassador& requester) mutable {
    std::optional<umbra::detail::AttributeOwnershipQueryRecipient> projection;
    {
      std::scoped_lock lock(ambassadorFederationManagementMutex());
      projection = embeddedFederationRegistry().attributeOwnershipQueryRecipientFor(
          federationName,
          requestId,
          requestingFederateId,
          objectInstanceHandle,
          reportKind,
          owningFederateId,
          requestedAttributeHandles);
    }
    if (!projection) {
      return;
    }

    AttributeHandleSet attributes;
    for (std::uint64_t const attributeHandle : projection->attributeHandles) {
      attributes.insert(makeAttributeHandle(attributeHandle));
    }
    switch (projection->reportKind) {
      case umbra::detail::AttributeOwnershipQueryReportKind::federate:
        requester.informAttributeOwnership(
            makeObjectInstanceHandle(projection->objectInstanceHandle),
            attributes,
            makeFederateHandle(projection->owningFederateId));
        return;
      case umbra::detail::AttributeOwnershipQueryReportKind::unowned:
        requester.attributeIsNotOwned(
            makeObjectInstanceHandle(projection->objectInstanceHandle),
            attributes);
        return;
      case umbra::detail::AttributeOwnershipQueryReportKind::rti:
        requester.attributeIsOwnedByRTI(
            makeObjectInstanceHandle(projection->objectInstanceHandle),
            attributes);
        return;
    }
  });
}

void queueAttributeOwnershipAcquisitionIfAvailableReport(
    umbra::detail::ObjectInstanceCallbackRoute callbackRoute,
    std::wstring federationName,
    std::uint64_t requestingFederateId,
    std::uint64_t objectInstanceHandle,
    std::uint64_t requestId,
    VariableLengthData userSuppliedTag) {
  callbackRoute([
      federationName = std::move(federationName),
      requestingFederateId,
      objectInstanceHandle,
      requestId,
      userSuppliedTag = std::move(userSuppliedTag)](FederateAmbassador& requester) mutable {
    std::optional<umbra::detail::AttributeOwnershipAcquisitionIfAvailableDelivery> delivery;
    {
      std::scoped_lock lock(ambassadorFederationManagementMutex());
      delivery = embeddedFederationRegistry()
                     .beginAttributeOwnershipAcquisitionIfAvailable(
                         federationName,
                         requestingFederateId,
                         objectInstanceHandle,
                         requestId);
    }
    if (!delivery) {
      return;
    }

    if (!delivery->securedAttributeHandles.empty()) {
      AttributeHandleSet securedAttributes;
      for (std::uint64_t const attributeHandle : delivery->securedAttributeHandles) {
        securedAttributes.insert(makeAttributeHandle(attributeHandle));
      }
      requester.attributeOwnershipAcquisitionNotification(
          makeObjectInstanceHandle(delivery->objectInstanceHandle),
          securedAttributes,
          userSuppliedTag);
    }
    if (!delivery->unavailableAttributeHandles.empty()) {
      AttributeHandleSet unavailableAttributes;
      for (std::uint64_t const attributeHandle : delivery->unavailableAttributeHandles) {
        unavailableAttributes.insert(makeAttributeHandle(attributeHandle));
      }
      requester.attributeOwnershipUnavailable(
          makeObjectInstanceHandle(delivery->objectInstanceHandle),
          unavailableAttributes,
          userSuppliedTag);
    }
  });
}

void queueAttributeOwnershipAcquisitionWorkItems(
    std::vector<umbra::detail::AttributeOwnershipAcquisitionWorkItem> workItems,
    std::wstring const & federationName);

void queueAttributeOwnershipAcquisitionWorkItem(
    umbra::detail::AttributeOwnershipAcquisitionWorkItem workItem,
    std::wstring federationName) {
  if (!workItem.callbackRoute) {
    throw RTIinternalError(
        L"The embedded federation has an ownership-acquisition callback without a route.");
  }

  switch (workItem.kind) {
    case umbra::detail::AttributeOwnershipAcquisitionWorkKind::acquisition_notification:
      workItem.callbackRoute([
          federationName = std::move(federationName),
          requestingFederateId = workItem.requestingFederateId,
          objectInstanceHandle = workItem.objectInstanceHandle,
          requestId = workItem.requestId,
          scheduledAttributeHandles = std::move(workItem.attributeHandles),
          userSuppliedTag = std::move(workItem.userSuppliedTag)](
                                 FederateAmbassador& requester) mutable {
        std::optional<umbra::detail::AttributeOwnershipAcquisitionNotificationDelivery> delivery;
        {
          std::scoped_lock lock(ambassadorFederationManagementMutex());
          delivery = embeddedFederationRegistry()
                         .beginAttributeOwnershipAcquisitionNotification(
                             federationName,
                             requestingFederateId,
                             objectInstanceHandle,
                             requestId,
                             scheduledAttributeHandles);
        }
        if (!delivery) {
          return;
        }

        auto queueFollowup = [&] {
          queueAttributeOwnershipAcquisitionWorkItems(
              std::move(delivery->followupWorkItems),
              federationName);
        };
        if (!delivery->securedAttributeHandles.empty()) {
          AttributeHandleSet securedAttributes;
          for (std::uint64_t const attributeHandle : delivery->securedAttributeHandles) {
            securedAttributes.insert(makeAttributeHandle(attributeHandle));
          }
          VariableLengthData const callbackTag = makeAmbassadorVariableLengthData(userSuppliedTag);
          try {
            requester.attributeOwnershipAcquisitionNotification(
                makeObjectInstanceHandle(delivery->objectInstanceHandle),
                securedAttributes,
                callbackTag);
          } catch (...) {
            // The registry has committed the ownership transition before user
            // code. Wake any subsequently pending acquisition even if this
            // callback reports FederateInternalError to its dispatcher.
            queueFollowup();
            throw;
          }
        }
        queueFollowup();
      });
      return;
    case umbra::detail::AttributeOwnershipAcquisitionWorkKind::if_available_notification:
      queueAttributeOwnershipAcquisitionIfAvailableReport(
          std::move(workItem.callbackRoute),
          std::move(federationName),
          workItem.requestingFederateId,
          workItem.objectInstanceHandle,
          workItem.requestId,
          makeAmbassadorVariableLengthData(workItem.userSuppliedTag));
      return;
    case umbra::detail::AttributeOwnershipAcquisitionWorkKind::
        request_divestiture_confirmation:
      workItem.callbackRoute([
          federationName = std::move(federationName),
          acquiringFederateId = workItem.requestingFederateId,
          divestingFederateId = workItem.receivingFederateId,
          objectInstanceHandle = workItem.objectInstanceHandle,
          acquisitionRequestId = workItem.requestId,
          candidateIsIfAvailable = workItem.candidateIsIfAvailable,
          scheduledAttributeHandles = std::move(workItem.attributeHandles),
          userSuppliedTag = std::move(workItem.userSuppliedTag)](
                                     FederateAmbassador& owner) mutable {
        std::optional<umbra::detail::RequestDivestitureConfirmationDelivery> delivery;
        {
          std::scoped_lock lock(ambassadorFederationManagementMutex());
          delivery = embeddedFederationRegistry()
                         .beginRequestDivestitureConfirmation(
                             federationName,
                             divestingFederateId,
                             acquiringFederateId,
                             objectInstanceHandle,
                             acquisitionRequestId,
                             candidateIsIfAvailable,
                             scheduledAttributeHandles);
        }
        if (!delivery) {
          return;
        }

        AttributeHandleSet releasedAttributes;
        for (std::uint64_t const attributeHandle : delivery->releasedAttributeHandles) {
          releasedAttributes.insert(makeAttributeHandle(attributeHandle));
        }
        VariableLengthData const callbackTag = makeAmbassadorVariableLengthData(userSuppliedTag);
        owner.requestDivestitureConfirmation(
            makeObjectInstanceHandle(delivery->objectInstanceHandle),
            releasedAttributes,
            callbackTag);
      });
      return;
    case umbra::detail::AttributeOwnershipAcquisitionWorkKind::request_release:
      workItem.callbackRoute([
          federationName = std::move(federationName),
          requestingFederateId = workItem.requestingFederateId,
          owningFederateId = workItem.receivingFederateId,
          objectInstanceHandle = workItem.objectInstanceHandle,
          requestId = workItem.requestId,
          scheduledAttributeHandles = std::move(workItem.attributeHandles),
          userSuppliedTag = std::move(workItem.userSuppliedTag)](
                                 FederateAmbassador& owner) mutable {
        std::optional<umbra::detail::AttributeOwnershipAcquisitionReleaseDelivery> delivery;
        {
          std::scoped_lock lock(ambassadorFederationManagementMutex());
          delivery = embeddedFederationRegistry()
                         .beginAttributeOwnershipAcquisitionRelease(
                             federationName,
                             requestingFederateId,
                             owningFederateId,
                             objectInstanceHandle,
                             requestId,
                             scheduledAttributeHandles);
        }
        if (!delivery) {
          return;
        }

        AttributeHandleSet candidateAttributes;
        for (std::uint64_t const attributeHandle : delivery->candidateAttributeHandles) {
          candidateAttributes.insert(makeAttributeHandle(attributeHandle));
        }
        VariableLengthData const callbackTag = makeAmbassadorVariableLengthData(userSuppliedTag);
        owner.requestAttributeOwnershipRelease(
            makeObjectInstanceHandle(delivery->objectInstanceHandle),
            candidateAttributes,
            callbackTag);
      });
      return;
  }
  throw RTIinternalError(
      L"Umbra encountered an unknown regular ownership-acquisition callback kind.");
}

void queueAttributeOwnershipAcquisitionWorkItems(
    std::vector<umbra::detail::AttributeOwnershipAcquisitionWorkItem> workItems,
    std::wstring const & federationName) {
  // Submit only after the registry and invoking ambassador have released
  // their locks. HLA_IMMEDIATE can enter either the acquirer or current owner
  // synchronously, including a Release Denied response.
  for (auto& workItem : workItems) {
    queueAttributeOwnershipAcquisitionWorkItem(std::move(workItem), federationName);
  }
}

void queueAttributeOwnershipAcquisitionCancellationConfirmation(
    umbra::detail::ObjectInstanceCallbackRoute callbackRoute,
    std::wstring federationName,
    std::uint64_t requestingFederateId,
    std::uint64_t objectInstanceHandle,
    std::uint64_t cancellationId,
    std::set<std::uint64_t> attributeHandles) {
  if (!callbackRoute) {
    throw RTIinternalError(
        L"The embedded federation has an ownership-acquisition cancellation without a callback route.");
  }

  callbackRoute([
      federationName = std::move(federationName),
      requestingFederateId,
      objectInstanceHandle,
      cancellationId,
      attributeHandles = std::move(attributeHandles)](FederateAmbassador& requester) mutable {
    std::optional<umbra::detail::AttributeOwnershipAcquisitionCancellationDelivery> delivery;
    {
      std::scoped_lock lock(ambassadorFederationManagementMutex());
      delivery = embeddedFederationRegistry()
                     .beginAttributeOwnershipAcquisitionCancellation(
                         federationName,
                         requestingFederateId,
                         objectInstanceHandle,
                         cancellationId,
                         attributeHandles);
    }
    if (!delivery) {
      return;
    }

    auto queueFollowup = [&] {
      queueAttributeOwnershipAcquisitionWorkItems(
          std::move(delivery->followupWorkItems),
          federationName);
    };
    if (!delivery->confirmedAttributeHandles.empty()) {
      AttributeHandleSet confirmedAttributes;
      for (std::uint64_t const attributeHandle : delivery->confirmedAttributeHandles) {
        confirmedAttributes.insert(makeAttributeHandle(attributeHandle));
      }
      try {
        requester.confirmAttributeOwnershipAcquisitionCancellation(
            makeObjectInstanceHandle(delivery->objectInstanceHandle),
            confirmedAttributes);
      } catch (...) {
        queueFollowup();
        throw;
      }
    }
    queueFollowup();
  });
}

void queueAttributeOwnershipDivestitureIfWantedNotifications(
    std::vector<umbra::detail::AttributeOwnershipDivestitureIfWantedNotification> notifications,
    std::wstring const & federationName) {
  for (auto& notification : notifications) {
    if (!notification.callbackRoute) {
      throw RTIinternalError(
          L"The embedded federation has an ownership-divestiture notification without a route.");
    }

    notification.callbackRoute([
        federationName,
        receivingFederateId = notification.receivingFederateId,
        objectInstanceHandle = notification.objectInstanceHandle,
        notificationId = notification.notificationId,
        scheduledAttributeHandles = std::move(notification.attributeHandles),
        userSuppliedTag = std::move(notification.userSuppliedTag)](
                                    FederateAmbassador& requester) mutable {
      std::optional<umbra::detail::AttributeOwnershipDivestitureIfWantedDelivery> delivery;
      {
        std::scoped_lock lock(ambassadorFederationManagementMutex());
        delivery = embeddedFederationRegistry()
                       .beginAttributeOwnershipDivestitureIfWantedNotification(
                           federationName,
                           receivingFederateId,
                           objectInstanceHandle,
                           notificationId,
                           scheduledAttributeHandles);
      }
      if (!delivery) {
        return;
      }

      auto queueFollowup = [&] {
        queueAttributeOwnershipAcquisitionWorkItems(
            std::move(delivery->followupWorkItems),
            federationName);
      };
      if (!delivery->securedAttributeHandles.empty()) {
        AttributeHandleSet securedAttributes;
        for (std::uint64_t const attributeHandle : delivery->securedAttributeHandles) {
          securedAttributes.insert(makeAttributeHandle(attributeHandle));
        }
        VariableLengthData const callbackTag = makeAmbassadorVariableLengthData(userSuppliedTag);
        try {
          requester.attributeOwnershipAcquisitionNotification(
              makeObjectInstanceHandle(delivery->objectInstanceHandle),
              securedAttributes,
              callbackTag);
        } catch (...) {
          // Ownership moved synchronously at the service return. Continue
          // planning older regular requests even if the federate callback
          // reports FederateInternalError to its dispatcher.
          queueFollowup();
          throw;
        }
      }
      queueFollowup();
    });
  }
}

void queueConfirmDivestitureNotifications(
    std::vector<umbra::detail::ConfirmDivestitureNotification> notifications,
    std::wstring const & federationName) {
  for (auto& notification : notifications) {
    if (!notification.callbackRoute) {
      throw RTIinternalError(
          L"The embedded federation has a Confirm Divestiture notification without a route.");
    }
    notification.callbackRoute([
        federationName,
        receivingFederateId = notification.receivingFederateId,
        objectInstanceHandle = notification.objectInstanceHandle,
        notificationId = notification.notificationId,
        scheduledAttributeHandles = std::move(notification.attributeHandles),
        userSuppliedTag = std::move(notification.userSuppliedTag)](
                                    FederateAmbassador& requester) mutable {
      std::optional<umbra::detail::ConfirmDivestitureNotificationDelivery> delivery;
      {
        std::scoped_lock lock(ambassadorFederationManagementMutex());
        delivery = embeddedFederationRegistry()
                       .beginConfirmDivestitureNotification(
                           federationName,
                           receivingFederateId,
                           objectInstanceHandle,
                           notificationId,
                           scheduledAttributeHandles);
      }
      if (!delivery) {
        return;
      }

      auto queueFollowup = [&] {
        queueAttributeOwnershipAcquisitionWorkItems(
            std::move(delivery->followupWorkItems),
            federationName);
      };
      if (!delivery->securedAttributeHandles.empty()) {
        AttributeHandleSet securedAttributes;
        for (std::uint64_t const attributeHandle : delivery->securedAttributeHandles) {
          securedAttributes.insert(makeAttributeHandle(attributeHandle));
        }
        VariableLengthData const callbackTag = makeAmbassadorVariableLengthData(userSuppliedTag);
        try {
          requester.attributeOwnershipAcquisitionNotification(
              makeObjectInstanceHandle(delivery->objectInstanceHandle),
              securedAttributes,
              callbackTag);
        } catch (...) {
          queueFollowup();
          throw;
        }
      }
      queueFollowup();
    });
  }
}

std::vector<umbra::detail::AttributeOwnershipAssumptionRecipient>
planAttributeOwnershipAssumptionSearchContinuation(
    std::wstring const & federationName,
    std::uint64_t objectInstanceHandle,
    std::set<std::uint64_t> const & scheduledAttributeHandles) {
  std::vector<umbra::detail::AttributeOwnershipAssumptionRecipient> followupRecipients;
  if (auto members = embeddedFederationRegistry().membersFor(federationName)) {
    for (auto const & member : *members) {
      auto planned = embeddedFederationRegistry()
                         .planAttributeOwnershipAssumptionsForFederate(
                             federationName,
                             member.id,
                             objectInstanceHandle,
                             &scheduledAttributeHandles);
      followupRecipients.insert(
          followupRecipients.end(),
          std::make_move_iterator(planned.begin()),
          std::make_move_iterator(planned.end()));
    }
  }
  return followupRecipients;
}

void queueAttributeOwnershipAssumptionRecipients(
    std::vector<umbra::detail::AttributeOwnershipAssumptionRecipient> recipients,
    std::wstring const & federationName,
    VariableLengthData const & userSuppliedTag) {
  for (auto& recipient : recipients) {
    if (!recipient.callbackRoute) {
      throw RTIinternalError(
          L"The embedded federation has an ownership-assumption callback without a route.");
    }

    // A continuation planner carries the original unconditional-divestiture
    // tag per grouped recipient. Initial divestiture and resign-action plans
    // leave this field empty and use the call-site tag (which may itself be
    // empty), preserving one queueing helper for all ownership paths.
    auto callbackTagBytes = recipient.userSuppliedTag.empty()
                                ? umbra::detail::variable_length_data_2025::copyBytes(userSuppliedTag)
                                : std::move(recipient.userSuppliedTag);

    recipient.callbackRoute([
        federationName,
        receivingFederateId = recipient.receivingFederateId,
        objectInstanceHandle = recipient.objectInstanceHandle,
        scheduledAttributeHandles = std::move(recipient.attributeHandles),
        callbackTagBytes = std::move(callbackTagBytes)](FederateAmbassador& candidate) mutable {
      std::optional<umbra::detail::AttributeOwnershipAssumptionDelivery> delivery;
      {
        std::scoped_lock lock(ambassadorFederationManagementMutex());
        delivery = embeddedFederationRegistry()
                       .attributeOwnershipAssumptionDeliveryFor(
                           federationName,
                           receivingFederateId,
                           objectInstanceHandle,
                           scheduledAttributeHandles);
      }
      if (!delivery) {
        // The candidate may have become ineligible before callback entry.
        // Continue the same unowned search immediately so a later eligible
        // federate is not delayed until an unrelated declaration event.
        auto followupRecipients = planAttributeOwnershipAssumptionSearchContinuation(
            federationName,
            objectInstanceHandle,
            scheduledAttributeHandles);
        queueAttributeOwnershipAssumptionRecipients(
            std::move(followupRecipients),
            federationName,
            VariableLengthData());
        return;
      }

      AttributeHandleSet offeredAttributes;
      for (std::uint64_t const attributeHandle : delivery->attributeHandles) {
        offeredAttributes.insert(makeAttributeHandle(attributeHandle));
      }
      VariableLengthData const callbackTag = makeAmbassadorVariableLengthData(callbackTagBytes);
      try {
        candidate.requestAttributeOwnershipAssumption(
            makeObjectInstanceHandle(delivery->objectInstanceHandle),
            offeredAttributes,
            callbackTag);
      } catch (...) {
        // The callback has entered federate code, so this candidate remains
        // reserved for this unowned search epoch. Continue searching after
        // preserving the user's exception.
        auto followupRecipients = planAttributeOwnershipAssumptionSearchContinuation(
            federationName,
            objectInstanceHandle,
            scheduledAttributeHandles);
        queueAttributeOwnershipAssumptionRecipients(
            std::move(followupRecipients),
            federationName,
            VariableLengthData());
        throw;
      }

      // A callback that returns without acquiring leaves the attribute
      // unowned. Advance the search to the next eligible federate now. If
      // user code acquired the attribute synchronously, the registry's owner
      // recheck suppresses all further assumption callbacks.
      auto followupRecipients = planAttributeOwnershipAssumptionSearchContinuation(
          federationName,
          objectInstanceHandle,
          scheduledAttributeHandles);
      queueAttributeOwnershipAssumptionRecipients(
          std::move(followupRecipients),
          federationName,
          VariableLengthData());
    });
  }
}

void queueAttributeOwnershipUnavailableRecipients(
    std::vector<umbra::detail::AttributeOwnershipUnavailableRecipient> recipients,
    std::wstring const & federationName,
    VariableLengthData const & userSuppliedTag) {
  for (auto& recipient : recipients) {
    if (!recipient.callbackRoute) {
      throw RTIinternalError(
          L"The embedded federation has an ownership-unavailable recipient without a callback route.");
    }
    recipient.callbackRoute([
        federationName,
        receivingFederateId = recipient.receivingFederateId,
        objectInstanceHandle = recipient.objectInstanceHandle,
        attributeHandles = std::move(recipient.attributeHandles),
        userSuppliedTag](FederateAmbassador& requester) mutable {
      std::optional<umbra::detail::AttributeOwnershipUnavailableRecipient> delivery;
      {
        std::scoped_lock lock(ambassadorFederationManagementMutex());
        delivery = embeddedFederationRegistry()
                       .attributeOwnershipUnavailableRecipientFor(
                           federationName,
                           receivingFederateId,
                           objectInstanceHandle,
                           attributeHandles);
      }
      if (!delivery) {
        return;
      }

      AttributeHandleSet unavailableAttributes;
      for (std::uint64_t const attributeHandle : delivery->attributeHandles) {
        unavailableAttributes.insert(makeAttributeHandle(attributeHandle));
      }
      requester.attributeOwnershipUnavailable(
          makeObjectInstanceHandle(delivery->objectInstanceHandle),
          unavailableAttributes,
          userSuppliedTag);
    });
  }
}

}  // namespace ownership_callback_detail

void queueAmbassadorAttributeOwnershipAssumptionRecipients(
    std::vector<umbra::detail::AttributeOwnershipAssumptionRecipient> recipients,
    std::wstring const &federationName,
    VariableLengthData const &userSuppliedTag) {
  ownership_callback_detail::queueAttributeOwnershipAssumptionRecipients(
      std::move(recipients), federationName, userSuppliedTag);
}

void queueAmbassadorAttributeOwnershipAcquisitionWorkItems(
    std::vector<umbra::detail::AttributeOwnershipAcquisitionWorkItem> workItems,
    std::wstring const &federationName) {
  ownership_callback_detail::queueAttributeOwnershipAcquisitionWorkItems(
      std::move(workItems), federationName);
}

void queueAmbassadorAttributeOwnershipAcquisitionCancellationConfirmation(
    umbra::detail::ObjectInstanceCallbackRoute callbackRoute,
    std::wstring federationName,
    std::uint64_t requestingFederateId,
    std::uint64_t objectInstanceHandle,
    std::uint64_t cancellationId,
    std::set<std::uint64_t> attributeHandles) {
  ownership_callback_detail::queueAttributeOwnershipAcquisitionCancellationConfirmation(
      std::move(callbackRoute),
      std::move(federationName),
      requestingFederateId,
      objectInstanceHandle,
      cancellationId,
      std::move(attributeHandles));
}

void queueAmbassadorAttributeOwnershipDivestitureIfWantedNotifications(
    std::vector<umbra::detail::AttributeOwnershipDivestitureIfWantedNotification>
        notifications,
    std::wstring const &federationName) {
  ownership_callback_detail::queueAttributeOwnershipDivestitureIfWantedNotifications(
      std::move(notifications), federationName);
}

void queueAmbassadorConfirmDivestitureNotifications(
    std::vector<umbra::detail::ConfirmDivestitureNotification> notifications,
    std::wstring const &federationName) {
  ownership_callback_detail::queueConfirmDivestitureNotifications(
      std::move(notifications), federationName);
}

void queueAmbassadorAttributeOwnershipQueryReport(
    umbra::detail::ObjectInstanceCallbackRoute callbackRoute,
    std::wstring federationName,
    std::uint64_t requestId,
    std::uint64_t requestingFederateId,
    std::uint64_t objectInstanceHandle,
    umbra::detail::AttributeOwnershipQueryReportKind reportKind,
    std::uint64_t owningFederateId,
    std::set<std::uint64_t> requestedAttributeHandles) {
  ownership_callback_detail::queueAttributeOwnershipQueryReport(
      std::move(callbackRoute),
      std::move(federationName),
      requestId,
      requestingFederateId,
      objectInstanceHandle,
      reportKind,
      owningFederateId,
      std::move(requestedAttributeHandles));
}

}  // namespace rti1516_2025::umbra_binding_detail
#endif
