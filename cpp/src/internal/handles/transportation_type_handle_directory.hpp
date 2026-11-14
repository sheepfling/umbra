#pragma once

#include <cstdint>
#include <map>
#include <optional>
#include <string>

namespace umbra::detail {

class FomCatalog;
class FomHandleAllocator;

// Private transportation handle lookup. The predefined standard pair uses
// Umbra's stable values even when an FDD omits their declarations; user-defined
// names use the execution-wide opaque allocator and retain values across a
// compatible additional-FOM join.
class TransportationTypeHandleDirectory final {
 public:
  [[nodiscard]] bool reconcile(
      FomCatalog const* catalog,
      FomHandleAllocator& handleAllocator);

  [[nodiscard]] std::optional<std::uint64_t> handleFor(
      std::string const& transportationTypeName) const;
  [[nodiscard]] std::optional<std::string> nameFor(std::uint64_t handle) const;

 private:
  std::map<std::string, std::uint64_t> handlesByName_;
  std::map<std::uint64_t, std::string> namesByHandle_;
};

}  // namespace umbra::detail
