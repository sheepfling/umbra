#pragma once

#include <string>

namespace umbra::detail {

class FomCatalog;

// Private creation-time decision for Umbra's two IEEE reference time
// implementations. It intentionally does not invent a mapping for a
// user-supplied fedtime library: that needs an explicit provider contract
// before it can become a federation-management service.
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

// IEEE 1516.1 creation defaults an omitted implementation name to
// HLAfloat64Time. IEEE 1516.2 documents any applicable logical-time and
// interval representations in the composed FDD. This selector verifies the
// only two built-in name-to-data-type correspondences that Umbra currently
// provides; all custom time implementations stop at factory_unavailable.
class ReferenceLogicalTimeSelector final {
 public:
  [[nodiscard]] ReferenceLogicalTimeSelection select(
      FomCatalog const& catalog,
      std::wstring const& requestedImplementationName) const;
};

}  // namespace umbra::detail
