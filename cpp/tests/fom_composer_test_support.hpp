#pragma once

#include <catch2/catch_test_macros.hpp>

#include "internal/federation/federation_registry.hpp"
#include "internal/fom/libxml2_fom_composer.hpp"
#include "internal/fom/libxml2_fom_validator.hpp"

#include <filesystem>
#include <string>
#include <utility>
#include <vector>

#ifndef UMBRA_SOURCE_DIRECTORY
#error "UMBRA_SOURCE_DIRECTORY must be defined by CMake for FOM resource tests."
#endif

namespace {

inline std::filesystem::path resourcePath(std::filesystem::path const& relative) {
  return std::filesystem::path(UMBRA_SOURCE_DIRECTORY) / "third_party" / "ieee1516.2-2025" /
         "resources" / relative;
}

inline umbra::detail::LibXml2FomModuleComposer composer() {
  return umbra::detail::LibXml2FomModuleComposer(
      resourcePath("schemas/IEEE1516-FDD-2025.xsd"));
}

inline umbra::detail::PrevalidatedFomModule validated(
    std::filesystem::path const& source,
    umbra::detail::FomModuleKind kind,
    std::wstring designator) {
  umbra::detail::LibXml2FomValidator validator;
  auto result = validator.validate({
      source,
      resourcePath("schemas/IEEE1516-DIF-2025.xsd"),
      kind,
      std::move(designator),
      L"IEEE1516-DIF-2025.xsd",
  });
  REQUIRE(result.status == umbra::detail::FomValidationStatus::valid);
  REQUIRE(result.module.has_value());
  return *result.module;
}

inline umbra::detail::FederationDefinition composedRestaurantDefinition() {
  std::vector<umbra::detail::PrevalidatedFomModule> modules{
      validated(
          resourcePath("mim/HLAstandardMIM-2025.xml"),
          umbra::detail::FomModuleKind::mim,
          L"urn:umbra:test:mim"),
      validated(
          resourcePath("examples/RestaurantFOMmodule-2025.xml"),
          umbra::detail::FomModuleKind::fom,
          L"urn:umbra:test:restaurant"),
  };
  auto materializer = composer();
  auto result = materializer.compose(modules);
  CAPTURE(result.diagnostics);
  REQUIRE(result.status == umbra::detail::FomCompositionStatus::valid);
  REQUIRE(result.catalog);
  REQUIRE(result.fdd);
  return {
      std::move(result.modules),
      L"HLAinteger64Time",
      std::move(result.catalog),
      std::move(result.fdd),
  };
}

}  // namespace
