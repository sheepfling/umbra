#include "internal/runtime/umbra_rti_ambassador.hpp"

#include "internal/fom/hla_names.hpp"
#include "internal/handles/federate_handle.hpp"
#include "internal/observability/mom_service_report_encoding.hpp"
#include "internal/runtime/ambassador_shared_utilities.hpp"

#include <utility>
#include <vector>

namespace rti1516_2025::umbra_binding_detail {

#if defined(UMBRA_ENABLE_EMBEDDED_FEDERATION_MANAGEMENT)

void submitAmbassadorFederationSaveNotifications(
    std::wstring const& federationName,
    std::vector<umbra::detail::FederationSaveNotification> notifications) {
  for (auto& notification : notifications) {
    if (!notification.callbackRoute) {
      continue;
    }
    switch (notification.kind) {
      case umbra::detail::FederationSaveNotificationKind::initiate:
        if (notification.publicServiceReportRoute) {
          std::vector<umbra::detail::MomServiceArgument> suppliedArguments{
              {umbra::detail::MomArgumentType::string,
               L"Federation save label",
               umbra::detail::formatMomString(notification.label)}};
          if (notification.timestamp) {
            suppliedArguments.push_back({
                umbra::detail::MomArgumentType::logical_time,
                L"Optional timestamp",
                umbra::detail::formatMomLogicalTime(*notification.timestamp)});
          } else {
            suppliedArguments.push_back({
                umbra::detail::MomArgumentType::null_value,
                L"Optional timestamp",
                umbra::detail::formatMomNull()});
          }
          notification.publicServiceReportRoute(
              L"InitiateFederateSave",
              umbra::detail::MomServiceType::federation_management,
              suppliedArguments,
              {umbra::detail::MomArgumentType::null_value,
               L"",
               umbra::detail::formatMomNull()},
              true,
              L"");
        }
        if (notification.serviceReportRoute) {
          // §4.20 is an RTI-initiated service at each recipient.  The
          // recipient-owned route preserves its selected file and serial
          // sequence before HLA_EVOKED work can expose Initiate Federate Save.
          notification.serviceReportRoute(
              static_cast<std::uint16_t>(
                  umbra::detail::MomServiceType::federation_management),
              [
                  label = notification.label,
                  timestamp = notification.timestamp](std::uint32_t serialNumber) {
                std::vector<umbra::detail::MomServiceArgument> suppliedArguments{
                    {umbra::detail::MomArgumentType::string,
                     L"Federation save label",
                     umbra::detail::formatMomString(label)},
                };
                if (timestamp) {
                  suppliedArguments.push_back({
                      umbra::detail::MomArgumentType::logical_time,
                      L"Optional timestamp",
                      umbra::detail::formatMomLogicalTime(*timestamp),
                  });
                } else {
                  suppliedArguments.push_back({
                      umbra::detail::MomArgumentType::null_value,
                      L"Optional timestamp",
                      umbra::detail::formatMomNull(),
                  });
                }
                return umbra::detail::formatMomSuccessfulVoidServiceReportRecord(
                    serialNumber,
                    L"InitiateFederateSave",
                    suppliedArguments);
              });
        }
        notification.callbackRoute([
            federationName,
            receivingFederateId = notification.receivingFederateId,
            label = std::move(notification.label),
            timestamp = std::move(notification.timestamp)](FederateAmbassador& recipient) {
          if (timestamp) {
            recipient.initiateFederateSave(label, *timestamp);
          } else {
            recipient.initiateFederateSave(label);
          }
          queueAmbassadorJoinedFederateMomConditionalAttributeUpdateForJoinedFederate(
              federationName,
              receivingFederateId,
              {umbra::detail::hla::utf8::mom::federate_state},
              receivingFederateId);
        });
        break;
      case umbra::detail::FederationSaveNotificationKind::completed:
        if (notification.publicServiceReportRoute) {
          std::vector<umbra::detail::MomServiceArgument> suppliedArguments{
              {umbra::detail::MomArgumentType::boolean,
               L"Federation save-success indicator",
               umbra::detail::formatMomBoolean(notification.successful)}};
          if (notification.successful) {
            suppliedArguments.push_back({
                umbra::detail::MomArgumentType::null_value,
                L"Optional failure reason",
                umbra::detail::formatMomNull()});
          } else {
            suppliedArguments.push_back({
                umbra::detail::MomArgumentType::save_failure_reason,
                L"Optional failure reason",
                umbra::detail::formatMomSaveFailureReason(notification.failureReason)});
          }
          notification.publicServiceReportRoute(
              L"FederationSaved",
              umbra::detail::MomServiceType::federation_management,
              suppliedArguments,
              {umbra::detail::MomArgumentType::null_value,
               L"",
               umbra::detail::formatMomNull()},
              true,
              L"");
        }
        if (notification.serviceReportRoute) {
          // §4.23 is RTI-initiated at every joined federate that received the
          // corresponding save instruction. Append the selected recipient's
          // result form before its callback is queued.
          notification.serviceReportRoute(
              static_cast<std::uint16_t>(
                  umbra::detail::MomServiceType::federation_management),
              [
                  successful = notification.successful,
                  failureReason = notification.failureReason](std::uint32_t serialNumber) {
                std::vector<umbra::detail::MomServiceArgument> suppliedArguments{
                    {umbra::detail::MomArgumentType::boolean,
                     L"Federation save-success indicator",
                     umbra::detail::formatMomBoolean(successful)},
                };
                if (successful) {
                  suppliedArguments.push_back({
                      umbra::detail::MomArgumentType::null_value,
                      L"Optional failure reason",
                      umbra::detail::formatMomNull(),
                  });
                } else {
                  suppliedArguments.push_back({
                      umbra::detail::MomArgumentType::save_failure_reason,
                      L"Optional failure reason",
                      umbra::detail::formatMomSaveFailureReason(failureReason),
                  });
                }
                return umbra::detail::formatMomSuccessfulVoidServiceReportRecord(
                    serialNumber,
                    L"FederationSaved",
                    suppliedArguments);
              });
        }
        if (notification.successful) {
          notification.callbackRoute([
              federationName,
              receivingFederateId = notification.receivingFederateId](
              FederateAmbassador& recipient) {
            recipient.federationSaved();
            queueAmbassadorJoinedFederateMomConditionalAttributeUpdateForJoinedFederate(
                federationName,
                receivingFederateId,
                {umbra::detail::hla::utf8::mom::federate_state});
          });
        } else {
          auto const reason = notification.failureReason;
          notification.callbackRoute([reason](FederateAmbassador& recipient) {
            recipient.federationNotSaved(reason);
          });
        }
        break;
      case umbra::detail::FederationSaveNotificationKind::status:
        if (notification.publicServiceReportRoute) {
          FederateHandleSaveStatusPairVector response;
          response.reserve(notification.statuses.size());
          for (auto const& [federateId, status] : notification.statuses) {
            response.emplace_back(makeFederateHandle(federateId), status);
          }
          notification.publicServiceReportRoute(
              L"FederationSaveStatusResponse",
              umbra::detail::MomServiceType::federation_management,
              {{umbra::detail::MomArgumentType::federate_handle_save_status_pair_set,
                L"List of joined federates and save status for each",
                umbra::detail::formatMomFederateHandleSaveStatusPairVector(response)}},
              {umbra::detail::MomArgumentType::null_value,
               L"",
               umbra::detail::formatMomNull()},
              true,
              L"");
        }
        if (notification.serviceReportRoute) {
          // §4.26 is RTI-initiated at the querying joined federate. Preserve
          // the exact Table 5 status-pair array in its selected report file
          // before the callback can expose the same response.
          notification.serviceReportRoute(
              static_cast<std::uint16_t>(
                  umbra::detail::MomServiceType::federation_management),
              [statuses = notification.statuses](std::uint32_t serialNumber) {
                FederateHandleSaveStatusPairVector response;
                response.reserve(statuses.size());
                for (auto const& [federateId, status] : statuses) {
                  response.emplace_back(makeFederateHandle(federateId), status);
                }
                return umbra::detail::formatMomSuccessfulVoidServiceReportRecord(
                    serialNumber,
                    L"FederationSaveStatusResponse",
                    {{umbra::detail::MomArgumentType::federate_handle_save_status_pair_set,
                      L"List of joined federates and save status for each",
                      umbra::detail::formatMomFederateHandleSaveStatusPairVector(response)}});
              });
        }
        notification.callbackRoute([
            statuses = std::move(notification.statuses)](FederateAmbassador& recipient) {
          FederateHandleSaveStatusPairVector response;
          response.reserve(statuses.size());
          for (auto const& [federateId, status] : statuses) {
            response.emplace_back(makeFederateHandle(federateId), status);
          }
          recipient.federationSaveStatusResponse(response);
        });
        break;
    }
  }
}

void submitAmbassadorFederationRestoreNotifications(
    std::wstring const& federationName,
    std::vector<umbra::detail::FederationRestoreNotification> notifications) {
  for (auto& notification : notifications) {
    if (!notification.callbackRoute) {
      continue;
    }
    switch (notification.kind) {
      case umbra::detail::FederationRestoreNotificationKind::request_succeeded:
        if (notification.publicServiceReportRoute) {
          notification.publicServiceReportRoute(
              L"ConfirmFederationRestorationRequest",
              umbra::detail::MomServiceType::federation_management,
              {{umbra::detail::MomArgumentType::string,
                L"Federation save label",
                umbra::detail::formatMomString(notification.label)},
               {umbra::detail::MomArgumentType::boolean,
                L"Request-success indicator",
                umbra::detail::formatMomBoolean(true)}},
              {umbra::detail::MomArgumentType::null_value,
               L"",
               umbra::detail::formatMomNull()},
              true,
              L"");
        }
        if (notification.serviceReportRoute) {
          // §4.28 is RTI-initiated at the requesting joined federate.  Write
          // its immutable selected-file result form before the success
          // callback can become observable in either callback model.
          notification.serviceReportRoute(
              static_cast<std::uint16_t>(
                  umbra::detail::MomServiceType::federation_management),
              [label = notification.label](std::uint32_t serialNumber) {
                return umbra::detail::formatMomSuccessfulVoidServiceReportRecord(
                    serialNumber,
                    L"ConfirmFederationRestorationRequest",
                    {{umbra::detail::MomArgumentType::string,
                      L"Federation save label",
                      umbra::detail::formatMomString(label)},
                     {umbra::detail::MomArgumentType::boolean,
                      L"Request-success indicator",
                      umbra::detail::formatMomBoolean(true)}});
              });
        }
        notification.callbackRoute([
            federationName,
            receivingFederateId = notification.receivingFederateId,
            label = std::move(notification.label)](FederateAmbassador& recipient) {
          recipient.requestFederationRestoreSucceeded(label);
          // The successful Confirm Federation Restoration Request service is
          // one of the MIM-defined HLAfederateState update boundaries.  The
          // restore ledger is already in FederateRestoreInProgress here, so
          // capture that event-time value through the normal MOM seam.
          queueAmbassadorJoinedFederateMomConditionalAttributeUpdateForJoinedFederate(
              federationName,
              receivingFederateId,
              {umbra::detail::hla::utf8::mom::federate_state});
        });
        break;
      case umbra::detail::FederationRestoreNotificationKind::request_failed:
        if (notification.publicServiceReportRoute) {
          notification.publicServiceReportRoute(
              L"ConfirmFederationRestorationRequest",
              umbra::detail::MomServiceType::federation_management,
              {{umbra::detail::MomArgumentType::string,
                L"Federation save label",
                umbra::detail::formatMomString(notification.label)},
               {umbra::detail::MomArgumentType::boolean,
                L"Request-success indicator",
                umbra::detail::formatMomBoolean(false)}},
              {umbra::detail::MomArgumentType::null_value,
               L"",
               umbra::detail::formatMomNull()},
              true,
              L"");
        }
        if (notification.serviceReportRoute) {
          // A rejected restore request remains a normally returned §4.27
          // invocation, whose negative §4.28 result is reported before the
          // requester can receive requestFederationRestoreFailed().
          notification.serviceReportRoute(
              static_cast<std::uint16_t>(
                  umbra::detail::MomServiceType::federation_management),
              [label = notification.label](std::uint32_t serialNumber) {
                return umbra::detail::formatMomSuccessfulVoidServiceReportRecord(
                    serialNumber,
                    L"ConfirmFederationRestorationRequest",
                    {{umbra::detail::MomArgumentType::string,
                      L"Federation save label",
                      umbra::detail::formatMomString(label)},
                     {umbra::detail::MomArgumentType::boolean,
                      L"Request-success indicator",
                      umbra::detail::formatMomBoolean(false)}});
              });
        }
        notification.callbackRoute([
            label = std::move(notification.label)](FederateAmbassador& recipient) {
          recipient.requestFederationRestoreFailed(label);
        });
        break;
      case umbra::detail::FederationRestoreNotificationKind::begin:
        if (notification.publicServiceReportRoute) {
          notification.publicServiceReportRoute(
              L"FederationRestoreBegun",
              umbra::detail::MomServiceType::federation_management,
              {},
              {umbra::detail::MomArgumentType::null_value,
               L"",
               umbra::detail::formatMomNull()},
              true,
              L"");
        }
        if (notification.serviceReportRoute) {
          // §4.29 is RTI-initiated at every joined federate, including the
          // requester. Each recipient's immutable selected-file route must
          // append the no-argument Table 5 form before the callback is queued.
          notification.serviceReportRoute(
              static_cast<std::uint16_t>(
                  umbra::detail::MomServiceType::federation_management),
              [](std::uint32_t serialNumber) {
                return umbra::detail::formatMomSuccessfulVoidServiceReportRecord(
                    serialNumber,
                    L"FederationRestoreBegun",
                    {});
              });
        }
        notification.callbackRoute([
            federationName,
            receivingFederateId = notification.receivingFederateId](
            FederateAmbassador& recipient) {
          recipient.federationRestoreBegun();
          queueAmbassadorJoinedFederateMomConditionalAttributeUpdateForJoinedFederate(
              federationName,
              receivingFederateId,
              {umbra::detail::hla::utf8::mom::federate_state});
        });
        break;
      case umbra::detail::FederationRestoreNotificationKind::initiate:
        if (notification.publicServiceReportRoute) {
          notification.publicServiceReportRoute(
              L"InitiateFederateRestore",
              umbra::detail::MomServiceType::federation_management,
              {{umbra::detail::MomArgumentType::string,
                L"Federation save label",
                umbra::detail::formatMomString(notification.label)},
               {umbra::detail::MomArgumentType::federate_handle,
                L"Joined federate designator",
                umbra::detail::formatMomFederateHandle(
                    makeFederateHandle(notification.postRestoreFederateId))},
               {umbra::detail::MomArgumentType::string,
                L"Federate name",
                umbra::detail::formatMomString(notification.federateName)}},
              {umbra::detail::MomArgumentType::null_value,
               L"",
               umbra::detail::formatMomNull()},
              true,
              L"");
        }
        if (notification.serviceReportRoute) {
          // §4.30 is RTI-initiated at each restoring joined federate.  Keep
          // the recipient's immutable selected-file route with this work so
          // the complete label/designator/name form precedes the callback.
          notification.serviceReportRoute(
              static_cast<std::uint16_t>(
                  umbra::detail::MomServiceType::federation_management),
              [label = notification.label,
               federateName = notification.federateName,
               postRestoreFederateId = notification.postRestoreFederateId](
                  std::uint32_t serialNumber) {
                return umbra::detail::formatMomSuccessfulVoidServiceReportRecord(
                    serialNumber,
                    L"InitiateFederateRestore",
                    {{umbra::detail::MomArgumentType::string,
                      L"Federation save label",
                      umbra::detail::formatMomString(label)},
                     {umbra::detail::MomArgumentType::federate_handle,
                      L"Joined federate designator",
                      umbra::detail::formatMomFederateHandle(
                          makeFederateHandle(postRestoreFederateId))},
                     {umbra::detail::MomArgumentType::string,
                      L"Federate name",
                      umbra::detail::formatMomString(federateName)}});
              });
        }
        notification.callbackRoute([
            federationName,
            receivingFederateId = notification.receivingFederateId,
            label = std::move(notification.label),
            federateName = std::move(notification.federateName),
            postRestoreFederateId = notification.postRestoreFederateId](
            FederateAmbassador& recipient) {
          recipient.initiateFederateRestore(
              label,
              federateName,
              makeFederateHandle(postRestoreFederateId));
          queueAmbassadorJoinedFederateMomConditionalAttributeUpdateForJoinedFederate(
              federationName,
              receivingFederateId,
              {umbra::detail::hla::utf8::mom::federate_state});
        });
        break;
      case umbra::detail::FederationRestoreNotificationKind::completed:
        if (notification.successful) {
          notification.callbackRoute([
              federationName,
              receivingFederateId = notification.receivingFederateId](
              FederateAmbassador& recipient) {
            recipient.federationRestored();
            queueAmbassadorJoinedFederateMomConditionalAttributeUpdateForJoinedFederate(
                federationName,
                receivingFederateId,
                {umbra::detail::hla::utf8::mom::federate_state});
          });
        } else {
          auto const reason = notification.failureReason;
          notification.callbackRoute([reason](FederateAmbassador& recipient) {
            recipient.federationNotRestored(reason);
          });
        }
        break;
      case umbra::detail::FederationRestoreNotificationKind::status:
        if (notification.publicServiceReportRoute) {
          FederateRestoreStatusVector response;
          response.reserve(notification.statuses.size());
          for (auto const& status : notification.statuses) {
            response.emplace_back(
                makeFederateHandle(status.preRestoreFederateId),
                makeFederateHandle(status.postRestoreFederateId),
                status.status);
          }
          notification.publicServiceReportRoute(
              L"FederationRestoreStatusResponse",
              umbra::detail::MomServiceType::federation_management,
              {{umbra::detail::MomArgumentType::federate_restore_status_set,
                L"List of joined federates and restore status for each",
                umbra::detail::formatMomFederateRestoreStatusVector(response)}},
              {umbra::detail::MomArgumentType::null_value,
               L"",
               umbra::detail::formatMomNull()},
              true,
              L"");
        }
        if (notification.serviceReportRoute) {
          // §4.35 is RTI-initiated at the querying joined federate. Preserve
          // the selected file's serial sequence and the exact descriptor
          // vector before the callback can expose that response. The type-20
          // MIM identity reconciles Table 5's malformed collection row.
          notification.serviceReportRoute(
              static_cast<std::uint16_t>(
                  umbra::detail::MomServiceType::federation_management),
              [statuses = notification.statuses](std::uint32_t serialNumber) {
                FederateRestoreStatusVector response;
                response.reserve(statuses.size());
                for (auto const& status : statuses) {
                  response.emplace_back(
                      makeFederateHandle(status.preRestoreFederateId),
                      makeFederateHandle(status.postRestoreFederateId),
                      status.status);
                }
                return umbra::detail::formatMomSuccessfulVoidServiceReportRecord(
                    serialNumber,
                    L"FederationRestoreStatusResponse",
                    {{umbra::detail::MomArgumentType::federate_restore_status_set,
                      L"List of joined federates and restore status for each",
                      umbra::detail::formatMomFederateRestoreStatusVector(response)}});
              });
        }
        notification.callbackRoute([
            statuses = std::move(notification.statuses)](FederateAmbassador& recipient) {
          FederateRestoreStatusVector response;
          response.reserve(statuses.size());
          for (auto const& status : statuses) {
            response.emplace_back(
                makeFederateHandle(status.preRestoreFederateId),
                makeFederateHandle(status.postRestoreFederateId),
                status.status);
          }
          recipient.federationRestoreStatusResponse(response);
        });
        break;
    }
  }
}

#endif

}  // namespace rti1516_2025::umbra_binding_detail
