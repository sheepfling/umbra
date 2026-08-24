#pragma once

#include <string>
#include <utility>
#include <vector>

namespace umbra::detail {

// An immutable, schema-validated FDD artifact produced solely by the private
// 1516.2 composition backend. It is intentionally not a public C++ binding
// type: federation-management services will translate their inputs and
// failures at a later, reviewed boundary.
class MaterializedFdd final {
 public:
  MaterializedFdd(
      std::string xmlUtf8,
      std::vector<std::string> composedFromModuleNames)
      : xmlUtf8_(std::move(xmlUtf8)),
        composedFromModuleNames_(std::move(composedFromModuleNames)) {}

  [[nodiscard]] std::string const& xmlUtf8() const noexcept {
    return xmlUtf8_;
  }

  [[nodiscard]] std::vector<std::string> const& composedFromModuleNames() const noexcept {
    return composedFromModuleNames_;
  }

 private:
  std::string xmlUtf8_;
  std::vector<std::string> composedFromModuleNames_;
};

}  // namespace umbra::detail
