#pragma once

#include "internal/fom/fom_validation.hpp"

namespace umbra::detail {

// Opt-in private backend. It validates one source document against a selected,
// already-vendored schema; FOM module composition remains the coordinator's
// responsibility before a registry mutation is allowed.
class LibXml2FomValidator final : public FomValidator {
 public:
  [[nodiscard]] FomValidationResult validate(
      FomValidationRequest const& request) const override;
};

}  // namespace umbra::detail
