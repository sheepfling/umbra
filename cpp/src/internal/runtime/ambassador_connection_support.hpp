#pragma once

#include "internal/callbacks/callback_dispatcher.hpp"
#include "internal/federation/process_federation_client.hpp"
#include "internal/fom/fom_catalog.hpp"

#include <RTI/RTI1516.h>
#include <RTI/auth/Authorizer.h>
#include <RTI/auth/Credentials.h>

#include <cstdint>
#include <filesystem>
#include <optional>
#include <string>

namespace rti1516_2025::umbra_binding_detail {

void validateAmbassadorConnectionCallbackModel(CallbackModel callbackModel);
ConfigurationResult ignoredAmbassadorConnectionResult();
std::optional<umbra::detail::ProcessTransportAddress>
configuredAmbassadorProcessEndpoint(RtiConfiguration const* configuration);
std::string ambassadorProcessEndpointIdentity(RtiConfiguration const* configuration);
std::uint64_t nextAmbassadorProcessEndpointSessionId() noexcept;
void requireNoCredentialsWhenAmbassadorAuthorizationIsDisabled(
    Credentials const& credentials);
void authorizeAmbassadorConnectIfConfigured(
    rti1516_2025::Authorizer& authorizer,
    Credentials const* credentials);
std::filesystem::path configuredAmbassadorServiceReportDirectory(
    RtiConfiguration const* configuration,
    bool& settingsApplied);
umbra::detail::FomStandardEdition configuredAmbassadorFomStandardEdition(
    RtiConfiguration const* configuration,
    bool& settingsApplied,
    bool& settingsFailedToParse);
std::filesystem::path validateAmbassadorServiceReportDirectory(
    std::filesystem::path directory);
umbra::detail::CallbackDispatchModel ambassadorCallbackDispatchModel(
    CallbackModel callbackModel);

}  // namespace rti1516_2025::umbra_binding_detail
