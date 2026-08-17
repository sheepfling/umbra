#pragma once

#include <cstdint>
#include <map>
#include <optional>
#include <string>
#include <utility>

namespace umbra::detail {

class FomCatalog;

// Private, per-federation AttributeHandle allocation. A key represents the
// class that declares an attribute rather than each subclass that inherits it,
// so a lookup of an inherited attribute returns the defining attribute's
// stable value. Values are retained across compatible additional-FOM joins;
// no name-derived hash is used.
class AttributeHandleDirectory final {
 public:
  [[nodiscard]] bool reconcile(FomCatalog const* catalog);

  [[nodiscard]] std::optional<std::uint64_t> handleFor(
      FomCatalog const* catalog,
      std::string const& objectClassName,
      std::string const& attributeName) const;
  [[nodiscard]] std::optional<std::string> nameFor(
      FomCatalog const* catalog,
      std::string const& objectClassName,
      std::uint64_t handle) const;

 private:
  using AttributeKey = std::pair<std::string, std::string>;

  [[nodiscard]] static std::optional<AttributeKey> resolveDefinition(
      FomCatalog const* catalog,
      std::string const& objectClassName,
      std::string const& attributeName);

  std::map<AttributeKey, std::uint64_t> handlesByDefinition_;
  std::map<std::uint64_t, AttributeKey> definitionsByHandle_;
  std::uint64_t nextHandle_ = 1;
};

}  // namespace umbra::detail
