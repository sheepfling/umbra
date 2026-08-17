#pragma once

#include <cstdint>
#include <map>
#include <optional>
#include <string>

namespace umbra::detail {

class FomCatalog;

// Private, per-federation DimensionHandle allocation. Values are retained
// across compatible additional-FOM joins so an existing public handle never
// changes when a later module contributes more dimensions.
class DimensionHandleDirectory final {
 public:
  [[nodiscard]] bool reconcile(FomCatalog const* catalog);

  [[nodiscard]] std::optional<std::uint64_t> handleFor(
      std::string const& dimensionName) const;
  [[nodiscard]] std::optional<std::string> nameFor(std::uint64_t handle) const;

 private:
  std::map<std::string, std::uint64_t> handlesByName_;
  std::map<std::uint64_t, std::string> namesByHandle_;
  std::uint64_t nextHandle_ = 1;
};

}  // namespace umbra::detail
