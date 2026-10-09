#pragma once

#include "internal/federation/federation_registry_service_types.hpp"

#include <cstdint>
#include <memory>
#include <string>
#include <string_view>
#include <vector>

namespace rti1516_2025 {
class LogicalTimeFactory;
}

namespace umbra::detail {
class CallbackDispatcher;
class FederateTimeState;
class FederationManagementCoordinator;
}

namespace rti1516_2025::umbra_binding_detail {

class CallbackSession;
struct JoinedServiceReportEndpoint;

inline constexpr std::wstring_view kUmbraEmbeddedFederateHost = L"umbra-embedded";
inline constexpr std::wstring_view kUmbraRtiVersion = L"Umbra 0.1.0";

#if defined(UMBRA_ENABLE_EMBEDDED_FEDERATION_MANAGEMENT)
umbra::detail::FederationManagementCoordinator& embeddedFederationCoordinator();
void eraseAmbassadorUpdateRateHistoryForFederation(std::wstring const& federationName);
std::shared_ptr<umbra::detail::FederateTimeState> ambassadorMakeFederateTimeState(
    umbra::detail::FederationDefinition const& definition);
std::unique_ptr<LogicalTimeFactory> ambassadorMakeDefinitionTimeFactory(
    umbra::detail::FederationDefinition const& definition);
umbra::detail::FederateCallbackRoute ambassadorMakeFederateCallbackRoute(
    std::shared_ptr<umbra::detail::CallbackDispatcher> const& callbackDispatcher,
    std::shared_ptr<CallbackSession> const& callbackSession);
umbra::detail::FederationTimeGrantDispatchFactory
ambassadorMakeTimeAdvanceGrantDispatchFactory(
    std::shared_ptr<umbra::detail::CallbackDispatcher> const& callbackDispatcher,
    std::shared_ptr<CallbackSession> const& callbackSession,
    std::shared_ptr<umbra::detail::FederateTimeState> const& timeState,
    std::wstring federationName);
umbra::detail::FederationTimeRoleEnableDispatchFactory
ambassadorMakeTimeRoleEnableDispatchFactory(
    std::shared_ptr<umbra::detail::CallbackDispatcher> const& callbackDispatcher,
    std::shared_ptr<CallbackSession> const& callbackSession,
    std::shared_ptr<umbra::detail::FederateTimeState> const& timeState,
    std::wstring federationName);
umbra::detail::FederateServiceReportRoute ambassadorMakeFederateServiceReportRoute(
    std::shared_ptr<JoinedServiceReportEndpoint> const& endpoint,
    std::wstring federationName,
    std::uint64_t federateId);
umbra::detail::FederatePublicServiceReportRoute ambassadorMakeFederatePublicServiceReportRoute(
    std::wstring federationName,
    std::uint64_t federateId);
void submitAmbassadorSynchronizationPointAnnouncements(
    std::vector<umbra::detail::SynchronizationPointAnnouncement> announcements);
void submitAmbassadorSynchronizationPointRegistration(
    umbra::detail::SynchronizationPointRegistrationPlan plan);
#endif

}  // namespace rti1516_2025::umbra_binding_detail
