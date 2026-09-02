#include "internal/federation/federation_management_coordinator.hpp"

#include "internal/fom/hla_names.hpp"
#include "internal/fom/fom_composition.hpp"
#include "internal/fom/fom_validation.hpp"
#include "internal/time/reference_time_selection.hpp"

#include <utility>

namespace umbra::detail {
namespace {

FederationPreparationStatus statusFor(FomValidationStatus status, FomModuleKind kind) {
  switch (status) {
    case FomValidationStatus::valid:
      return FederationPreparationStatus::applied;
    case FomValidationStatus::source_not_found:
      return kind == FomModuleKind::mim ? FederationPreparationStatus::mim_not_found
                                        : FederationPreparationStatus::fom_not_found;
    case FomValidationStatus::source_unreadable:
      return kind == FomModuleKind::mim ? FederationPreparationStatus::mim_unreadable
                                        : FederationPreparationStatus::fom_unreadable;
    case FomValidationStatus::source_parse_error:
      return kind == FomModuleKind::mim ? FederationPreparationStatus::mim_parse_error
                                        : FederationPreparationStatus::fom_parse_error;
    case FomValidationStatus::invalid_model:
      return kind == FomModuleKind::mim ? FederationPreparationStatus::invalid_mim
                                        : FederationPreparationStatus::invalid_fom;
    case FomValidationStatus::validator_failure:
      return FederationPreparationStatus::backend_failure;
  }
  return FederationPreparationStatus::backend_failure;
}

FederationPreparationStatus statusFor(FomCompositionStatus status) {
  switch (status) {
    case FomCompositionStatus::valid:
      return FederationPreparationStatus::applied;
    case FomCompositionStatus::invalid_model:
    case FomCompositionStatus::inconsistent_modules:
      return FederationPreparationStatus::inconsistent_fom;
    // Every source is explicitly validated immediately before composition.
    // A source that changes between those two phases cannot be attributed
    // reliably to FOM or MIM, so it is retained as an internal backend fault
    // until the public adapter gains richer origin diagnostics.
    case FomCompositionStatus::source_not_found:
    case FomCompositionStatus::source_unreadable:
    case FomCompositionStatus::source_parse_error:
    case FomCompositionStatus::validator_failure:
      return FederationPreparationStatus::backend_failure;
  }
  return FederationPreparationStatus::backend_failure;
}

FederationPreparationResult failure(FederationPreparationStatus status, std::string diagnostics) {
  return {status, std::nullopt, std::move(diagnostics)};
}

FomValidationRequest requestFor(
    std::filesystem::path sourcePath,
    std::filesystem::path schemaPath,
    FomModuleKind kind,
    std::wstring designator,
    std::wstring schemaDesignator,
    FomStandardEdition standardEdition,
    FomSourceCompatibility sourceCompatibility) {
  return {
      std::move(sourcePath),
      std::move(schemaPath),
      kind,
      std::move(designator),
      std::move(schemaDesignator),
      standardEdition,
      sourceCompatibility,
  };
}

FomValidationRequest requestForExisting(PrevalidatedFomModule const& module) {
  return requestFor(
      module.sourcePath,
      module.schemaPath,
      module.kind,
      module.designator,
      module.schemaDesignator,
      module.standardEdition,
      module.sourceCompatibility);
}

std::optional<FomEditionResources> resourcesFor(
    FederationManagementResources const& resources,
    FomStandardEdition standardEdition) {
  if (standardEdition == FomStandardEdition::ieee1516_2010) {
    return resources.ieee1516_2010;
  }
  return FomEditionResources{
      FomStandardEdition::ieee1516_2025,
      resources.standardMimPath,
      resources.difSchemaPath,
      {},
      std::wstring{fomDifSchemaDesignator(FomStandardEdition::ieee1516_2025)},
  };
}

FederationPreparationResult materializeDefinition(
    FomModuleComposer const& composer,
    ReferenceLogicalTimeSelector const& timeSelector,
    std::vector<PrevalidatedFomModule> modules,
    std::wstring const& logicalTimeImplementationName,
    FomStandardEdition standardEdition) {
  auto composition = composer.compose(modules);
  if (composition.status != FomCompositionStatus::valid || !composition.catalog) {
    return failure(statusFor(composition.status), std::move(composition.diagnostics));
  }

  auto selection = timeSelector.select(*composition.catalog, logicalTimeImplementationName);
  switch (selection.status) {
    case ReferenceLogicalTimeSelectionStatus::selected:
      return {
          FederationPreparationStatus::applied,
          FederationDefinition{
              std::move(composition.modules),
              std::move(selection.selectedImplementationName),
              std::move(composition.catalog),
              std::move(composition.fdd),
              standardEdition,
          },
          {},
      };
    case ReferenceLogicalTimeSelectionStatus::factory_unavailable:
      return failure(
          FederationPreparationStatus::time_factory_unavailable,
          std::move(selection.diagnostics));
    case ReferenceLogicalTimeSelectionStatus::inconsistent_fdd_time_representation:
      return failure(FederationPreparationStatus::inconsistent_fom, std::move(selection.diagnostics));
  }
  return failure(FederationPreparationStatus::backend_failure, "Unknown reference-time selection outcome.");
}

}  // namespace

FederationManagementCoordinator::FederationManagementCoordinator(
    FomValidator const& validator,
    FomModuleComposer const& composer,
    ReferenceLogicalTimeSelector const& timeSelector,
    FederationManagementResources resources)
    : validator_(validator),
      composer_(composer),
      timeSelector_(timeSelector),
      resources_(std::move(resources)) {}

FederationPreparationResult FederationManagementCoordinator::prepareCreate(
    std::vector<std::wstring> const& fomDesignators,
    std::optional<std::wstring> const& mimDesignator,
    std::wstring const& logicalTimeImplementationName,
    FomStandardEdition standardEdition) const {
  if (fomDesignators.empty()) {
    return failure(
        FederationPreparationStatus::invalid_fom,
        "Create Federation Execution requires at least one FOM module designator.");
  }
  if (mimDesignator && *mimDesignator == umbra::detail::hla::wide::mom::standard_mim) {
    return failure(
        FederationPreparationStatus::standard_mim_designator_supplied,
        "A supplied MIM designator must not be HLAstandardMIM.");
  }

  auto const selectedResources = resourcesFor(resources_, standardEdition);
  if (!selectedResources || selectedResources->standardMimPath.empty() ||
      selectedResources->difSchemaPath.empty()) {
    return failure(
        FederationPreparationStatus::backend_failure,
        standardEdition == FomStandardEdition::ieee1516_2010
            ? "IEEE 1516-2010 FOM resources are not configured for this runtime."
            : "IEEE 1516-2025 FOM resources are not configured for this runtime.");
  }

  FomValidationRequest mimRequest = mimDesignator
      ? requestFor(
            std::filesystem::path(*mimDesignator),
            selectedResources->difSchemaPath,
            FomModuleKind::mim,
            *mimDesignator,
            selectedResources->difSchemaDesignator,
            standardEdition,
            standardEdition == FomStandardEdition::ieee1516_2010
                ? FomSourceCompatibility::rpr_2010
                : FomSourceCompatibility::strict)
      : requestFor(
            selectedResources->standardMimPath,
            selectedResources->difSchemaPath,
            FomModuleKind::mim,
            umbra::detail::hla::wide::mom::standard_mim,
            selectedResources->difSchemaDesignator,
            standardEdition,
            standardEdition == FomStandardEdition::ieee1516_2010
                ? FomSourceCompatibility::rpr_2010
                : FomSourceCompatibility::strict);
  auto mimValidation = validator_.validate(mimRequest);
  if (mimValidation.status != FomValidationStatus::valid || !mimValidation.module) {
    return failure(statusFor(mimValidation.status, FomModuleKind::mim), std::move(mimValidation.diagnostics));
  }

  std::vector<PrevalidatedFomModule> modules;
  modules.reserve(fomDesignators.size() + 1);
  modules.push_back(std::move(*mimValidation.module));
  for (auto const& designator : fomDesignators) {
    auto validation = validator_.validate(requestFor(
        std::filesystem::path(designator),
        selectedResources->difSchemaPath,
        FomModuleKind::fom,
        designator,
        selectedResources->difSchemaDesignator,
        standardEdition,
        standardEdition == FomStandardEdition::ieee1516_2010
            ? FomSourceCompatibility::rpr_2010
            : FomSourceCompatibility::strict));
    if (validation.status != FomValidationStatus::valid || !validation.module) {
      return failure(statusFor(validation.status, FomModuleKind::fom), std::move(validation.diagnostics));
    }
    modules.push_back(std::move(*validation.module));
  }

  return materializeDefinition(
      composer_,
      timeSelector_,
      std::move(modules),
      logicalTimeImplementationName,
      standardEdition);
}

FederationPreparationResult FederationManagementCoordinator::prepareAdditionalModules(
    FederationDefinition const& existing,
    std::vector<std::wstring> const& additionalFomDesignators) const {
  if (existing.fomModules.empty() || existing.logicalTimeImplementationName.empty()) {
    return failure(
        FederationPreparationStatus::backend_failure,
        "The existing federation definition has no validated modules or logical-time implementation.");
  }
  if (additionalFomDesignators.empty()) {
    return {FederationPreparationStatus::applied, existing, {}};
  }

  auto const selectedResources = resourcesFor(resources_, existing.standardEdition);
  if (!selectedResources || selectedResources->difSchemaPath.empty()) {
    return failure(
        FederationPreparationStatus::backend_failure,
        "The existing federation definition's FOM edition is not configured for this runtime.");
  }

  std::vector<PrevalidatedFomModule> modules;
  modules.reserve(existing.fomModules.size() + additionalFomDesignators.size());
  for (auto const& existingModule : existing.fomModules) {
    auto validation = validator_.validate(requestForExisting(existingModule));
    if (validation.status != FomValidationStatus::valid || !validation.module) {
      return failure(
          statusFor(validation.status, existingModule.kind),
          std::move(validation.diagnostics));
    }
    modules.push_back(std::move(*validation.module));
  }
  for (auto const& designator : additionalFomDesignators) {
    auto validation = validator_.validate(requestFor(
        std::filesystem::path(designator),
        selectedResources->difSchemaPath,
        FomModuleKind::fom,
        designator,
        selectedResources->difSchemaDesignator,
        existing.standardEdition,
        existing.standardEdition == FomStandardEdition::ieee1516_2010
            ? FomSourceCompatibility::rpr_2010
            : FomSourceCompatibility::strict));
    if (validation.status != FomValidationStatus::valid || !validation.module) {
      return failure(statusFor(validation.status, FomModuleKind::fom), std::move(validation.diagnostics));
    }
    modules.push_back(std::move(*validation.module));
  }

  return materializeDefinition(
      composer_,
      timeSelector_,
      std::move(modules),
      existing.logicalTimeImplementationName,
      existing.standardEdition);
}

}  // namespace umbra::detail
