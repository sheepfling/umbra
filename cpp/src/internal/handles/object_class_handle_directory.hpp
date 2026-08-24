#pragma once

#include <cstdint>
#include <map>
#include <optional>
#include <string>

namespace umbra::detail {

class FomCatalog;

// Private, per-federation ObjectClassHandle allocation. Numeric values are
// intentionally not derived from a name hash: a valid additional-FOM join can
// add classes while preserving every already-issued handle. The directory is
// reconciled on the same registry transaction that commits a replacement FOM
// definition.
class ObjectClassHandleDirectory final {
 public:
  // Adds every class currently present in catalog. Existing values are never
  // reassigned. A null catalog is retained for registry-only unit fixtures;
  // a public lookup will treat it as unavailable rather than invent a value.
  [[nodiscard]] bool reconcile(FomCatalog const* catalog);

  [[nodiscard]] std::optional<std::uint64_t> handleFor(
      std::string const& objectClassName) const;
  [[nodiscard]] std::optional<std::string> nameFor(std::uint64_t handle) const;

 private:
  std::map<std::string, std::uint64_t> handlesByName_;
  std::map<std::uint64_t, std::string> namesByHandle_;
  std::uint64_t nextHandle_ = 1;
};

}  // namespace umbra::detail
