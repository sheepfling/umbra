#pragma once

#include "internal/fom/fom_catalog.hpp"

#include <optional>
#include <string>

namespace umbra::detail {

std::optional<std::string> normalizedUpdateRateDesignator(
    FomCatalog const& catalog,
    std::string const& updateRateDesignator);

std::optional<double> updateRateValueForNormalizedDesignator(
    FomCatalog const& catalog,
    std::string const& normalizedDesignator);

}  // namespace umbra::detail
