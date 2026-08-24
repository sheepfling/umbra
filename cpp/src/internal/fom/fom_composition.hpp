#pragma once

#include "internal/fom/fom_validation.hpp"

#include <memory>
#include <string>
#include <vector>

namespace umbra::detail {

class FomCatalog;
struct MaterializedFdd;

// The composition status remains private until it is translated deliberately
// at an RTIambassador service boundary. It distinguishes a source that cannot
// safely be revalidated from a set of individually valid modules that cannot
// be combined.
enum class FomCompositionStatus {
  valid,
  source_not_found,
  source_unreadable,
  source_parse_error,
  invalid_model,
  inconsistent_modules,
  validator_failure,
};

struct FomCompositionResult {
  FomCompositionStatus status = FomCompositionStatus::validator_failure;
  std::vector<PrevalidatedFomModule> modules;
  std::string diagnostics;
  std::shared_ptr<FomCatalog const> catalog;
  std::shared_ptr<MaterializedFdd const> fdd;
  // Annex C.8 can accept a composition while requiring a warning for a
  // non-equivalent duplicate switch. Warnings remain private preflight data;
  // diagnostics continue to describe an unsuccessful composition.
  std::vector<std::string> warnings;
};

// A compatibility decision is intentionally separated from the federation
// registry. A future coordinator will only create or extend a federation after
// this preflight has accepted the requested module sequence.
class FomModuleComposer {
 public:
  virtual ~FomModuleComposer() = default;

  [[nodiscard]] virtual FomCompositionResult compose(
      std::vector<PrevalidatedFomModule> const& modules) const = 0;
};

}  // namespace umbra::detail
