#include "internal/federation/federation_registry_value_helpers.hpp"

#include <algorithm>

namespace umbra::detail {

std::optional<std::string> normalizedUpdateRateDesignator(
    FomCatalog const& catalog,
    std::string const& updateRateDesignator) {
  if (updateRateDesignator.empty() || updateRateDesignator == "default" ||
      updateRateDesignator == "HLAdefault" ||
      updateRateDesignator == "HLAdefaultUpdateRate") {
    return std::string{"HLAdefault"};
  }
  return catalog.updateRateValue(updateRateDesignator)
      ? std::optional<std::string>{updateRateDesignator}
      : std::nullopt;
}

std::string storedUpdateRateDesignator(
    std::string const& suppliedDesignator,
    std::string const& normalizedDesignator) {
  // An omitted designator selects the default rate but must remain distinct
  // from an explicitly supplied HLAdefault value: the former uses the
  // two-argument Turn Updates On callback, while the latter uses its official
  // rate-bearing overload.
  return suppliedDesignator.empty() ? std::string{} : normalizedDesignator;
}

std::optional<double> updateRateValueForNormalizedDesignator(
    FomCatalog const& catalog,
    std::string const& normalizedDesignator) {
  if (normalizedDesignator == "HLAdefault") {
    return 0.0;
  }
  return catalog.updateRateValue(normalizedDesignator);
}

bool isSupportedTransportationName(
    FomCatalog const* catalog,
    std::string const& transportationName) {
  if (transportationName == "HLAreliable" ||
      transportationName == "HLAbestEffort") {
    return true;
  }
  if (catalog == nullptr) {
    return false;
  }
  auto const names = catalog->transportationTypeNames();
  return std::find(names.begin(), names.end(), transportationName) != names.end();
}

}  // namespace umbra::detail
