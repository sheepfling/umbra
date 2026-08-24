#include "internal/federation/federation_management_coordinator.hpp"

#include "internal/fom/fom_composition.hpp"
#include "internal/fom/fom_validation.hpp"
#include "internal/time/reference_time_selection.hpp"

#include <utility>

namespace umbra::detail {
namespace {

constexpr wchar_t kStandardMimDesignator[] = L"HLAstandardMIM";
constexpr wchar_t kDifSchemaDesignator[] = L"IEEE1516-DIF-2025.xsd";

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
    std::wstring schemaDesignator = kDifSchemaDesignator) {
  return {
      std::move(sourcePath),
      std::move(schemaPath),
      kind,
      std::move(designator),
      std::move(schemaDesignator),
  };
}

FomValidationRequest requestForExisting(PrevalidatedFomModule const& module) {
  return requestFor(
      module.sourcePath,
      module.schemaPath,
      module.kind,
      module.designator,
      module.schemaDesignator);
}

FederationPreparationResult materializeDefinition(
    FomModuleComposer const& composer,
    ReferenceLogicalTimeSelector const& timeSelector,
    std::vector<PrevalidatedFomModule> modules,
    std::wstring const& logicalTimeImplementationName) {
  auto composition = composer.compose(modules);
  if (composition.status != FomCompositionStatus::valid || !composition.catalog || !composition.fdd) {
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
    std::wstring const& logicalTimeImplementationName) const {
  if (fomDesignators.empty()) {
    return failure(
        FederationPreparationStatus::invalid_fom,
        "Create Federation Execution requires at least one FOM module designator.");
  }
  if (mimDesignator && *mimDesignator == kStandardMimDesignator) {
    return failure(
        FederationPreparationStatus::standard_mim_designator_supplied,
        "A supplied MIM designator must not be HLAstandardMIM.");
  }

  FomValidationRequest mimRequest = mimDesignator
      ? requestFor(
            std::filesystem::path(*mimDesignator),
            resources_.difSchemaPath,
            FomModuleKind::mim,
            *mimDesignator)
      : requestFor(
            resources_.standardMimPath,
            resources_.difSchemaPath,
            FomModuleKind::mim,
            kStandardMimDesignator);
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
        resources_.difSchemaPath,
        FomModuleKind::fom,
        designator));
    if (validation.status != FomValidationStatus::valid || !validation.module) {
      return failure(statusFor(validation.status, FomModuleKind::fom), std::move(validation.diagnostics));
    }
    modules.push_back(std::move(*validation.module));
  }

  return materializeDefinition(composer_, timeSelector_, std::move(modules), logicalTimeImplementationName);
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
        resources_.difSchemaPath,
        FomModuleKind::fom,
        designator));
    if (validation.status != FomValidationStatus::valid || !validation.module) {
      return failure(statusFor(validation.status, FomModuleKind::fom), std::move(validation.diagnostics));
    }
    modules.push_back(std::move(*validation.module));
  }

  return materializeDefinition(
      composer_,
      timeSelector_,
      std::move(modules),
      existing.logicalTimeImplementationName);
}

}  // namespace umbra::detail
