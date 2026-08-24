#pragma once

#include "internal/fom/fom_composition.hpp"

#include <filesystem>

namespace umbra::detail {

// Opt-in Annex C-guided module compatibility preflight. It reparses and XSD
// validates every input before inspecting it, so a caller cannot compose a
// document that changed after its first individual validation.
class LibXml2FomModuleComposer final : public FomModuleComposer {
 public:
  // A materializer validates its synthesized output against the separately
  // vendored relaxed FDD schema, never against the input DIF schema.
  explicit LibXml2FomModuleComposer(std::filesystem::path fddSchemaPath);

  [[nodiscard]] FomCompositionResult compose(
      std::vector<PrevalidatedFomModule> const& modules) const override;

 private:
  std::filesystem::path fddSchemaPath_;
};

}  // namespace umbra::detail
