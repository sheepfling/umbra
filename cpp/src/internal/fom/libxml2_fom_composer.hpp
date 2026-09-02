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
  // selected FDD schema for the model edition, never against the input DIF
  // schema. The 2010 path is optional so existing 2025-only builds retain
  // their current resource contract.
  explicit LibXml2FomModuleComposer(
      std::filesystem::path fddSchemaPath2025,
      std::filesystem::path fddSchemaPath2010 = {});

  [[nodiscard]] FomCompositionResult compose(
      std::vector<PrevalidatedFomModule> const& modules) const override;

 private:
  std::filesystem::path fddSchemaPath2025_;
  std::filesystem::path fddSchemaPath2010_;
};

}  // namespace umbra::detail
