#pragma once

#include <chrono>
#include <memory>
#include <string_view>

#include <RTI/RtiConfiguration.h>
#include <RTI/auth/AuthorizerFactory.h>

namespace umbra::detail {

struct RtiProtocolTimeoutSettings final {
  // IEEE 1516.1-2025 §12.13.4.5 Table 31 defaults, in seconds.
  std::chrono::milliseconds rtiTimeoutHeart{std::chrono::seconds(180)};
  std::chrono::milliseconds rtiTimeoutPurge{std::chrono::seconds(600)};
};

// Reads Umbra's implementation-defined authorization section from the RTI
// Initialization Data file selected by UMBRA_RTI_RID_FILE. The file's profile
// is selected by the official RtiConfiguration::configurationName field.
// If no RID locator is configured, or the selected profile disables
// authorization, no factory is returned. A configured but unreadable RID is
// an error rather than a silent fallback to disabled authorization.
[[nodiscard]] std::unique_ptr<rti1516_2025::AuthorizerFactory>
makeAuthorizerFactoryFromRtiInitializationData(
    rti1516_2025::RtiConfiguration const* configuration);

[[nodiscard]] RtiProtocolTimeoutSettings
loadRtiProtocolTimeoutSettingsFromInitializationData(
    std::wstring_view configurationName);

}  // namespace umbra::detail
