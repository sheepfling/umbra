#pragma once

#if defined(UMBRA_ENABLE_EMBEDDED_FEDERATION_MANAGEMENT)

#include "internal/federation/process_federation_service.hpp"

#include <RTI/RTI1516.h>

#include <filesystem>
#include <memory>

namespace umbra::test {

[[nodiscard]] detail::FederationDefinition
makeProcessTaraBoundaryFederationDefinition();
[[nodiscard]] std::unique_ptr<rti1516_2025::RTIambassador>
makeProcessTaraBoundaryRti();
[[nodiscard]] std::filesystem::path
makeProcessTaraBoundaryServiceReportDirectory();

}  // namespace umbra::test

#endif
