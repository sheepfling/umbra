#pragma once

#include <RTI/time/LogicalTimeFactory.h>

#include <memory>
#include <string>

namespace umbra::detail {

[[nodiscard]] std::unique_ptr<rti1516_2025::LogicalTimeFactory>
makeFederationLogicalTimeFactory(std::wstring const& implementationName);

}  // namespace umbra::detail
