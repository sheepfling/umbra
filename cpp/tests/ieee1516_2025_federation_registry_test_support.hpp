#pragma once

#include <catch2/catch_test_macros.hpp>

#include "internal/federation/federation_registry.hpp"
#include "internal/federation/federation_state_image.hpp"
#include "internal/fom/libxml2_fom_composer.hpp"
#include "internal/fom/libxml2_fom_validator.hpp"

#include <algorithm>
#include <atomic>
#include <chrono>
#include <filesystem>
#include <fstream>
#include <iterator>
#include <memory>
#include <set>
#include <stdexcept>
#include <string>
#include <variant>
#include <vector>

#include <RTI/time/HLAinteger64Time.h>
#include <RTI/time/HLAinteger64Interval.h>

namespace umbra::test::federation_registry_support {

// Inline definition shares the sequence across the registry test translation units.
inline std::filesystem::path temporarySaveCommitDirectory() {
  static std::atomic_uint64_t next{0U};
  auto const timestamp = std::chrono::high_resolution_clock::now()
                             .time_since_epoch()
                             .count();
  return std::filesystem::temp_directory_path() /
      ("umbra-federation-save-commit-" + std::to_string(timestamp) + "-" +
       std::to_string(++next));
}

}  // namespace umbra::test::federation_registry_support
namespace {

using umbra::detail::EmbeddedFederationRegistry;
using umbra::detail::FederationDefinition;
using umbra::detail::FederationRegistryStatus;
using umbra::detail::FederationTimeGrantStatus;
using umbra::detail::FomModuleKind;
using umbra::detail::PrevalidatedFomModule;
using umbra::detail::InstrumentationLayer;
using umbra::detail::ObjectInstanceNameReservationStatus;
using umbra::detail::FomCompositionStatus;
using umbra::detail::FomValidationStatus;
using umbra::detail::LibXml2FomModuleComposer;
using umbra::detail::LibXml2FomValidator;

PrevalidatedFomModule module(std::wstring designator, std::wstring source) {
  return {
      std::move(designator),
      std::move(source),
      L"C:/fom/IEEE1516-DIF-2025.xsd",
      FomModuleKind::fom,
      L"IEEE1516-DIF-2025.xsd",
  };
}

FederationDefinition validDefinition() {
  return {
      {
          module(L"file:///fom/base.xml", L"C:/fom/base.xml"),
          module(L"file:///fom/extensions.xml", L"C:/fom/extensions.xml"),
      },
      L"HLAinteger64Time",
  };
}

std::filesystem::path resourcePath(std::filesystem::path const& relative) {
  return std::filesystem::path(UMBRA_SOURCE_DIRECTORY) /
      "third_party" / "ieee1516.2-2025" / "resources" / relative;
}

PrevalidatedFomModule validatedModule(
    std::filesystem::path const& source,
    FomModuleKind kind,
    std::wstring designator) {
  LibXml2FomValidator validator;
  auto result = validator.validate({
      source,
      resourcePath("schemas/IEEE1516-DIF-2025.xsd"),
      kind,
      std::move(designator),
      L"IEEE1516-DIF-2025.xsd",
  });
  REQUIRE(result.status == FomValidationStatus::valid);
  REQUIRE(result.module.has_value());
  return *result.module;
}

FederationDefinition composedRestaurantDefinition() {
  std::vector<PrevalidatedFomModule> modules{
      validatedModule(
          resourcePath("mim/HLAstandardMIM-2025.xml"),
          FomModuleKind::mim,
          L"urn:umbra:test:registry-mim"),
      validatedModule(
          resourcePath("examples/RestaurantFOMmodule-2025.xml"),
          FomModuleKind::fom,
          L"urn:umbra:test:registry-restaurant"),
  };
  LibXml2FomModuleComposer composer(
      resourcePath("schemas/IEEE1516-FDD-2025.xsd"));
  auto result = composer.compose(modules);
  CAPTURE(result.diagnostics);
  REQUIRE(result.status == FomCompositionStatus::valid);
  REQUIRE(result.catalog);
  REQUIRE(result.fdd);
  return {
      std::move(result.modules),
      L"HLAinteger64Time",
      std::move(result.catalog),
      std::move(result.fdd),
  };
}

FederationDefinition composedDirectedInteractionDefinition() {
  std::filesystem::path const testData =
      std::filesystem::path(UMBRA_SOURCE_DIRECTORY) / "cpp" / "tests" / "data";
  std::vector<PrevalidatedFomModule> modules{
      validatedModule(
          resourcePath("mim/HLAstandardMIM-2025.xml"),
          FomModuleKind::mim,
          L"urn:umbra:test:directed-registry-mim"),
      validatedModule(
          testData / "directed-interaction-object-consumer-fom.xml",
          FomModuleKind::fom,
          L"urn:umbra:test:directed-registry-object"),
      validatedModule(
          testData / "directed-interaction-interaction-provider-fom.xml",
          FomModuleKind::fom,
          L"urn:umbra:test:directed-registry-interaction"),
  };
  LibXml2FomModuleComposer composer(
      resourcePath("schemas/IEEE1516-FDD-2025.xsd"));
  auto result = composer.compose(modules);
  CAPTURE(result.diagnostics);
  REQUIRE(result.status == FomCompositionStatus::valid);
  REQUIRE(result.catalog);
  REQUIRE(result.fdd);
  return {
      std::move(result.modules),
      L"HLAinteger64Time",
      std::move(result.catalog),
      std::move(result.fdd),
  };
}

FederationDefinition composedParameterizedDirectedInteractionDefinition() {
  std::filesystem::path const testData =
      std::filesystem::path(UMBRA_SOURCE_DIRECTORY) / "cpp" / "tests" / "data";
  std::vector<PrevalidatedFomModule> modules{
      validatedModule(
          resourcePath("mim/HLAstandardMIM-2025.xml"),
          FomModuleKind::mim,
          L"urn:umbra:test:directed-parameter-registry-mim"),
      validatedModule(
          testData / "directed-parameter-interaction-object-consumer-fom.xml",
          FomModuleKind::fom,
          L"urn:umbra:test:directed-parameter-registry-object"),
      validatedModule(
          testData / "directed-parameter-interaction-interaction-provider-fom.xml",
          FomModuleKind::fom,
          L"urn:umbra:test:directed-parameter-registry-interaction"),
  };
  LibXml2FomModuleComposer composer(
      resourcePath("schemas/IEEE1516-FDD-2025.xsd"));
  auto result = composer.compose(modules);
  CAPTURE(result.diagnostics);
  REQUIRE(result.status == FomCompositionStatus::valid);
  REQUIRE(result.catalog);
  REQUIRE(result.fdd);
  return {
      std::move(result.modules),
      L"HLAinteger64Time",
      std::move(result.catalog),
      std::move(result.fdd),
  };
}

using umbra::test::federation_registry_support::temporarySaveCommitDirectory;

umbra::detail::FederateCallbackRoute noOpCallbackRoute() {
  umbra::detail::FederateCallbackRoute route;
  route.submit = [](umbra::detail::FederateCallbackInvocation) {};
  return route;
}

}  // namespace
