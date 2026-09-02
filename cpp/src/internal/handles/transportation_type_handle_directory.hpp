#pragma once

#include <cstdint>
#include <map>
#include <optional>
#include <string>

namespace umbra::detail {

class FomCatalog;

// Private, per-federation TransportationTypeHandle allocation. The two
// mandatory standard names retain their official values; FOM-declared names
// receive execution-scoped values that are never reassigned when a compatible
// additional-FOM join extends the catalog.
class TransportationTypeHandleDirectory final {
 public:
  [[nodiscard]] bool reconcile(FomCatalog const* catalog);

  [[nodiscard]] std::optional<std::uint64_t> handleFor(
      std::string const& transportationTypeName) const;
  [[nodiscard]] std::optional<std::string> nameFor(std::uint64_t handle) const;

 private:
  std::map<std::string, std::uint64_t> handlesByName_;
  std::map<std::uint64_t, std::string> namesByHandle_;
  std::uint64_t nextHandle_ = 3;
};

}  // namespace umbra::detail
