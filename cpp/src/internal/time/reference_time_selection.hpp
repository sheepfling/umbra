#pragma once

#include <string>

namespace umbra::detail {

class FomCatalog;

// Private creation-time decision for the federation's selected fedtime
// implementation. The standard factory-factory name is the source of the
// logical-time and interval classes used by the runtime.
enum class ReferenceLogicalTimeSelectionStatus {
  selected,
  factory_unavailable,
  inconsistent_fdd_time_representation,
};

struct ReferenceLogicalTimeSelection {
  ReferenceLogicalTimeSelectionStatus status =
      ReferenceLogicalTimeSelectionStatus::factory_unavailable;
  std::wstring requestedImplementationName;
  std::wstring selectedImplementationName;
  std::string diagnostics;

  [[nodiscard]] bool accepted() const noexcept {
    return status == ReferenceLogicalTimeSelectionStatus::selected;
  }
};

// IEEE 1516.1 creation defaults an omitted implementation name through the
// federation's LogicalTimeFactoryFactory. IEEE 1516.2 documents any applicable
// logical-time and interval representations in the composed FDD.
class ReferenceLogicalTimeSelector final {
 public:
  [[nodiscard]] ReferenceLogicalTimeSelection select(
      FomCatalog const& catalog,
      std::wstring const& requestedImplementationName) const;
};

}  // namespace umbra::detail
