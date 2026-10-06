#pragma once

#include "internal/federation/federation_registry.hpp"

#include <RTI/RTI1516.h>

#include <cstdint>
#include <map>
#include <memory>
#include <mutex>
#include <optional>
#include <set>
#include <string>
#include <string_view>
#include <vector>

namespace rti1516_2025 {
class Exception;
}

namespace umbra::detail {
class FederateLifecycle;
class FederateTimeState;
}

namespace rti1516_2025::umbra_binding_detail {

#if defined(UMBRA_ENABLE_EMBEDDED_FEDERATION_MANAGEMENT)
std::mutex& ambassadorFederationManagementMutex();
void requireConnectedForFederationManagement(
    umbra::detail::FederateLifecycle const& lifecycle);
umbra::detail::EmbeddedFederationRegistry& embeddedFederationRegistry();
std::wstring describeAmbassadorException(Exception const& exception);
struct AmbassadorAttributeRegionPairValues {
  std::map<std::uint64_t, std::set<std::uint64_t>> values;
  bool invalidAttributeHandle = false;
  bool invalidRegionHandle = false;
};
AmbassadorAttributeRegionPairValues ambassadorAttributeRegionPairValues(
    AttributeHandleSetRegionHandleSetPairVector const& attributesAndRegions);
std::optional<std::set<std::uint64_t>> ambassadorAttributeHandleValues(
    AttributeHandleSet const& attributes);
[[noreturn]] void throwObjectClassAttributeDeclarationFailureForFederationManagement(
    umbra::detail::ObjectClassAttributeDeclarationStatus status);
[[noreturn]] void throwObjectInstanceNameReservationFailureForFederationManagement(
    umbra::detail::ObjectInstanceNameReservationStatus status,
    std::wstring const& operation);
[[noreturn]] void throwObjectInstanceRegistrationFailureForFederationManagement(
    umbra::detail::ObjectInstanceRegistrationStatus status);
[[noreturn]] void throwObjectInstanceRegionAssociationFailureForFederationManagement(
    umbra::detail::ObjectInstanceRegionAssociationStatus status);
void queueAmbassadorObjectInstanceNameReservation(
    umbra::detail::ObjectInstanceCallbackRoute callbackRoute,
    std::wstring federationName,
    std::uint64_t federateId,
    bool succeeded,
    std::wstring objectInstanceName);
void queueAmbassadorMultipleObjectInstanceNameReservation(
    umbra::detail::ObjectInstanceCallbackRoute callbackRoute,
    std::wstring federationName,
    std::uint64_t federateId,
    bool succeeded,
    std::set<std::wstring> objectInstanceNames);
void queueAmbassadorDeclarationAdvisories(
    std::vector<umbra::detail::DeclarationAdvisory> advisories);
void queueAmbassadorObjectInstanceDiscoveries(
    std::vector<umbra::detail::ObjectInstanceDiscoveryRecipient> discoveries,
    std::wstring const& federationName);
void queueAmbassadorObjectInstanceScopeChanges(
    std::vector<umbra::detail::ObjectInstanceScopeChangeRecipient> changes,
    std::wstring const& federationName);
void queueAmbassadorAttributeRelevanceAdvisories(
    std::vector<umbra::detail::AttributeRelevanceAdvisoryRecipient> advisories,
    std::wstring const& federationName);
void queueAmbassadorAttributeOwnershipAssumptionRecipients(
    std::vector<umbra::detail::AttributeOwnershipAssumptionRecipient> recipients,
    std::wstring const& federationName,
    VariableLengthData const& userSuppliedTag);
struct AmbassadorJoinedFederateMomConditionalWork {
  std::wstring federationName;
  std::uint64_t objectInstanceHandle = 0U;
  std::set<std::uint64_t> attributeHandles;
};
std::optional<AmbassadorJoinedFederateMomConditionalWork>
ambassadorJoinedFederateMomConditionalWorkFor(
    umbra::detail::EmbeddedFederationRegistry& registry,
    std::wstring const& federationName,
    std::uint64_t joinedFederateId,
    std::vector<std::string_view> attributeNames);
void queueAmbassadorJoinedFederateMomConditionalAttributeUpdate(
    std::wstring const& federationName,
    std::uint64_t objectInstanceHandle,
    std::set<std::uint64_t> requestedAttributeHandles,
    std::optional<std::uint64_t> excludedReceivingFederateId = std::nullopt);
void queueAmbassadorJoinedFederateMomConditionalAttributeUpdateForJoinedFederate(
    std::wstring const& federationName,
    std::uint64_t joinedFederateId,
    std::vector<std::string_view> attributeNames,
    std::optional<std::uint64_t> excludedReceivingFederateId = std::nullopt);
void submitAmbassadorTimeAdvanceGrantDispatches(
    std::vector<umbra::detail::FederationTimeGrantDispatch> dispatches);
void flushAmbassadorAsynchronousReceiveCallbacks(
    std::shared_ptr<umbra::detail::FederateTimeState> const& timeState);
std::shared_ptr<LogicalTime> cloneAmbassadorReferenceLogicalTime(
    std::wstring const& implementationName,
    LogicalTime const& time);
void retireAmbassadorTsoMessagePayloadsAtRetractionBoundary(
    std::wstring const& federationName,
    std::uint64_t producingFederateId,
    umbra::detail::FederateTimeState const& timeState);
std::shared_ptr<LogicalTimeInterval> cloneAmbassadorReferenceLogicalTimeInterval(
    std::wstring const& implementationName,
    LogicalTimeInterval const& interval);
[[noreturn]] void throwAmbassadorInteractionTransportationTypeChangeFailure(
    umbra::detail::InteractionTransportationTypeChangeStatus status);
std::optional<std::string> ambassadorTransportationNameFromFederation(
    std::wstring const& federationName,
    TransportationTypeHandle const& transportationType);
void queueAmbassadorConfirmInteractionTransportationTypeChange(
    umbra::detail::InteractionCallbackRoute callbackRoute,
    std::wstring federationName,
    std::uint64_t requestingFederateId,
    std::uint64_t interactionClassHandle);
[[noreturn]] void throwAmbassadorInteractionTransportationTypeQueryFailure(
    umbra::detail::InteractionTransportationTypeQueryStatus status);
void queueAmbassadorReportInteractionTransportationType(
    umbra::detail::InteractionCallbackRoute callbackRoute,
    std::wstring federationName,
    std::uint64_t requestingFederateId,
    std::uint64_t queriedFederateId,
    std::uint64_t interactionClassHandle);
#endif

}  // namespace rti1516_2025::umbra_binding_detail
