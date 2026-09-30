#pragma once

#include <memory>

#include <RTI/RtiConfiguration.h>
#include <RTI/auth/AuthorizerFactory.h>

namespace umbra::detail {

// Reads Umbra's implementation-defined authorization section from the RTI
// Initialization Data file selected by UMBRA_RTI_RID_FILE. The file's profile
// is selected by the official RtiConfiguration::configurationName field.
// If no RID locator is configured, or the selected profile disables
// authorization, no factory is returned. A configured but unreadable RID is
// an error rather than a silent fallback to disabled authorization.
[[nodiscard]] std::unique_ptr<rti1516_2025::AuthorizerFactory>
makeAuthorizerFactoryFromRtiInitializationData(
    rti1516_2025::RtiConfiguration const* configuration);

}  // namespace umbra::detail
