#pragma once

#include <cstdint>
#include <map>
#include <optional>
#include <string>

namespace umbra::detail {

class FomCatalog;

// Private, per-federation InteractionClassHandle allocation. Values are kept
// stable across compatible additional-FOM joins; no name-derived hash is used.
class InteractionClassHandleDirectory final {
 public:
  [[nodiscard]] bool reconcile(FomCatalog const* catalog);

  [[nodiscard]] std::optional<std::uint64_t> handleFor(
      std::string const& interactionClassName) const;
  [[nodiscard]] std::optional<std::string> nameFor(std::uint64_t handle) const;

 private:
  std::map<std::string, std::uint64_t> handlesByName_;
  std::map<std::uint64_t, std::string> namesByHandle_;
  std::uint64_t nextHandle_ = 1;
};

}  // namespace umbra::detail
