#pragma once

#include <cstdint>
#include <map>
#include <optional>
#include <string>
#include <utility>

namespace umbra::detail {

class FomCatalog;

// Private, per-federation ParameterHandle allocation. A key represents the
// interaction class that declares a parameter rather than each subclass that
// inherits it, so inherited lookups retain the defining parameter's stable
// value across compatible additional-FOM joins; no name-derived hash is used.
class ParameterHandleDirectory final {
 public:
  [[nodiscard]] bool reconcile(FomCatalog const* catalog);

  [[nodiscard]] std::optional<std::uint64_t> handleFor(
      FomCatalog const* catalog,
      std::string const& interactionClassName,
      std::string const& parameterName) const;
  [[nodiscard]] std::optional<std::string> nameFor(
      FomCatalog const* catalog,
      std::string const& interactionClassName,
      std::uint64_t handle) const;

 private:
  using ParameterKey = std::pair<std::string, std::string>;

  [[nodiscard]] static std::optional<ParameterKey> resolveDefinition(
      FomCatalog const* catalog,
      std::string const& interactionClassName,
      std::string const& parameterName);

  std::map<ParameterKey, std::uint64_t> handlesByDefinition_;
  std::map<std::uint64_t, ParameterKey> definitionsByHandle_;
  std::uint64_t nextHandle_ = 1;
};

}  // namespace umbra::detail
