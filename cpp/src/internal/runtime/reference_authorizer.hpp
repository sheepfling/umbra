#pragma once

#include <memory>
#include <optional>
#include <string>

namespace rti1516_2025 {
class Authorizer;
}

namespace umbra::detail {

// This is an RTI-internal configuration seam, not an installed binding type.
// A later RID reader can supply the global password without placing a secret
// in RtiConfiguration::additionalSettings or in a service-report record.
struct ReferenceAuthorizerConfiguration final {
  std::optional<std::wstring> globalPlainTextPassword;
};

[[nodiscard]] std::unique_ptr<rti1516_2025::Authorizer> makeReferenceAuthorizer(
    ReferenceAuthorizerConfiguration configuration);

}  // namespace umbra::detail
