#pragma once

#include "internal/federation/federation_registry.hpp"

#include <filesystem>
#include <optional>
#include <string>
#include <vector>

namespace umbra::detail {

class FomModuleComposer;
class FomValidator;
class ReferenceLogicalTimeSelector;

// Exact local resources used by the private coordinator. The standard MIM
// path is an RTI-owned resource; a caller's original FOM or supplied-MIM
// designator is preserved separately in each validated descriptor.
struct FomEditionResources final {
  FomStandardEdition standardEdition = FomStandardEdition::ieee1516_2025;
  std::filesystem::path standardMimPath;
  std::filesystem::path difSchemaPath;
  std::filesystem::path fddSchemaPath;
  std::wstring difSchemaDesignator;
};

struct FederationManagementResources {
  std::filesystem::path standardMimPath;
  std::filesystem::path difSchemaPath;
  // 2010 resources remain explicitly external until their provenance and
  // redistribution terms have been reviewed. The ordinary 2025 fields above
  // preserve the existing source-compatible construction for this profile.
  std::optional<FomEditionResources> ieee1516_2010;
};

// These states are intentionally private. The standard-binding adapter will
// later translate them to the exception list on each official service, after
// it has the same atomic registry commit path as this preparation step.
enum class FederationPreparationStatus {
  applied,
  fom_not_found,
  fom_unreadable,
  fom_parse_error,
  invalid_fom,
  mim_not_found,
  mim_unreadable,
  mim_parse_error,
  invalid_mim,
  standard_mim_designator_supplied,
  inconsistent_fom,
  time_factory_unavailable,
  backend_failure,
};

struct FederationPreparationResult {
  FederationPreparationStatus status = FederationPreparationStatus::backend_failure;
  std::optional<FederationDefinition> definition;
  std::string diagnostics;

  [[nodiscard]] bool accepted() const noexcept {
    return status == FederationPreparationStatus::applied && definition.has_value();
  }
};

// The preparation coordinator is independent of a public RTI ambassador. It
// performs no registry mutation and emits a definition only when the supplied
// MIM/FOM sequence, composed FDD, and selected reference time implementation
// form one coherent private runtime input. Dependency injection keeps XML/XSD
// validation and standard-resource resolution outside the state kernel.
class FederationManagementCoordinator final {
 public:
  FederationManagementCoordinator(
      FomValidator const& validator,
      FomModuleComposer const& composer,
      ReferenceLogicalTimeSelector const& timeSelector,
      FederationManagementResources resources);

  [[nodiscard]] FederationPreparationResult prepareCreate(
      std::vector<std::wstring> const& fomDesignators,
      std::optional<std::wstring> const& mimDesignator,
      std::wstring const& logicalTimeImplementationName,
      FomStandardEdition standardEdition = FomStandardEdition::ieee1516_2025) const;

  [[nodiscard]] FederationPreparationResult prepareAdditionalModules(
      FederationDefinition const& existing,
      std::vector<std::wstring> const& additionalFomDesignators) const;

 private:
  FomValidator const& validator_;
  FomModuleComposer const& composer_;
  ReferenceLogicalTimeSelector const& timeSelector_;
  FederationManagementResources resources_;
};

}  // namespace umbra::detail
