#include "internal/runtime/umbra_rti_ambassador.hpp"

#include "internal/handles/federate_handle.hpp"
#include "internal/observability/mom_service_report_encoding.hpp"
#include "internal/runtime/ambassador_shared_utilities.hpp"

#include <utility>
#include <vector>

namespace rti1516_2025::umbra_binding_detail {

#if defined(UMBRA_ENABLE_EMBEDDED_FEDERATION_MANAGEMENT)

namespace {

VariableLengthData copySynchronizationPointTag(
    std::vector<unsigned char> const& tag) {
  return VariableLengthData(
      tag.empty() ? nullptr : static_cast<void const*>(tag.data()),
      tag.size());
}

}  // namespace

void submitAmbassadorSynchronizationPointAnnouncements(
    std::vector<umbra::detail::SynchronizationPointAnnouncement> announcements) {
  for (auto& announcement : announcements) {
    if (!announcement.callbackRoute) {
      continue;
    }
    if (announcement.publicServiceReportRoute) {
      auto const tag = copySynchronizationPointTag(announcement.userSuppliedTag);
      announcement.publicServiceReportRoute(
          L"AnnounceSynchronizationPoint",
          umbra::detail::MomServiceType::federation_management,
          {{umbra::detail::MomArgumentType::string,
            L"Synchronization point label",
            umbra::detail::formatMomString(announcement.label)},
           {umbra::detail::MomArgumentType::table_5_user_supplied_tag,
            L"User-supplied tag",
            umbra::detail::formatMomUserSuppliedTag(tag)}},
          {umbra::detail::MomArgumentType::null_value,
           L"",
           umbra::detail::formatMomNull()},
          true,
          L"");
    }
    if (announcement.serviceReportRoute) {
      // §4.16 is an RTI-initiated service at each receiving joined federate.
      // Append that federate's selected-file record before its callback is
      // queued, preserving the per-recipient serial sequence and HLA_EVOKED
      // observation boundary.
      announcement.serviceReportRoute(
          static_cast<std::uint16_t>(
              umbra::detail::MomServiceType::federation_management),
          [
              label = announcement.label,
              userSuppliedTag = announcement.userSuppliedTag](
              std::uint32_t serialNumber) {
            auto tag = copySynchronizationPointTag(userSuppliedTag);
            return umbra::detail::formatMomSuccessfulVoidServiceReportRecord(
                serialNumber,
                L"AnnounceSynchronizationPoint",
                {{umbra::detail::MomArgumentType::string,
                  L"Synchronization point label",
                  umbra::detail::formatMomString(label)},
                 {umbra::detail::MomArgumentType::table_5_user_supplied_tag,
                  L"User-supplied tag",
                  umbra::detail::formatMomUserSuppliedTag(tag)}});
          });
    }
    announcement.callbackRoute([
        label = std::move(announcement.label),
        userSuppliedTag = std::move(announcement.userSuppliedTag)](
        FederateAmbassador& recipient) mutable {
      auto tag = copySynchronizationPointTag(userSuppliedTag);
      recipient.announceSynchronizationPoint(label, tag);
    });
  }
}

void submitAmbassadorFederationSynchronizedNotifications(
    std::vector<umbra::detail::FederationSynchronizedNotification> notifications) {
  for (auto& notification : notifications) {
    if (!notification.callbackRoute) {
      continue;
    }
    if (notification.publicServiceReportRoute) {
      FederateHandleSet failedToSyncSet;
      for (std::uint64_t federateId : notification.failedToSyncFederateIds) {
        failedToSyncSet.insert(makeFederateHandle(federateId));
      }
      notification.publicServiceReportRoute(
          L"FederationSynchronized",
          umbra::detail::MomServiceType::federation_management,
          {{umbra::detail::MomArgumentType::string,
            L"Synchronization point label",
            umbra::detail::formatMomString(notification.label)},
           {umbra::detail::MomArgumentType::federate_handle_set,
            L"Set of joined federate designators",
            umbra::detail::formatMomFederateHandleSet(failedToSyncSet)}},
          {umbra::detail::MomArgumentType::null_value,
           L"",
           umbra::detail::formatMomNull()},
          true,
          L"");
    }
    if (notification.serviceReportRoute) {
      // §4.18, like §4.16, is an RTI-initiated service at every recipient.
      // Preserve each joined federate's own selection and serial sequence
      // before the corresponding C++ callback can be evoked.
      notification.serviceReportRoute(
          static_cast<std::uint16_t>(
              umbra::detail::MomServiceType::federation_management),
          [
              label = notification.label,
              failedToSyncFederateIds = notification.failedToSyncFederateIds](
              std::uint32_t serialNumber) {
            FederateHandleSet failedToSyncSet;
            for (std::uint64_t federateId : failedToSyncFederateIds) {
              failedToSyncSet.insert(makeFederateHandle(federateId));
            }
            return umbra::detail::formatMomSuccessfulVoidServiceReportRecord(
                serialNumber,
                L"FederationSynchronized",
                {{umbra::detail::MomArgumentType::string,
                  L"Synchronization point label",
                  umbra::detail::formatMomString(label)},
                 {umbra::detail::MomArgumentType::federate_handle_set,
                  L"Set of joined federate designators",
                  umbra::detail::formatMomFederateHandleSet(failedToSyncSet)}});
          });
    }
    notification.callbackRoute([
        label = std::move(notification.label),
        failedToSyncFederateIds = std::move(notification.failedToSyncFederateIds)](
        FederateAmbassador& recipient) mutable {
      FederateHandleSet failedToSyncSet;
      for (std::uint64_t federateId : failedToSyncFederateIds) {
        failedToSyncSet.insert(makeFederateHandle(federateId));
      }
      recipient.federationSynchronized(label, failedToSyncSet);
    });
  }
}

#endif

}  // namespace rti1516_2025::umbra_binding_detail
