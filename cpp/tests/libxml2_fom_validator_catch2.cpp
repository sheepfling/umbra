#include <catch2/catch_test_macros.hpp>

#include "internal/fom/libxml2_fom_validator.hpp"

#include <filesystem>
#include <string>
#include <tuple>
#include <utility>

#ifndef UMBRA_SOURCE_DIRECTORY
#error "UMBRA_SOURCE_DIRECTORY must be defined by CMake for FOM resource tests."
#endif

namespace {

using umbra::detail::FomModuleKind;
using umbra::detail::FomValidationRequest;
using umbra::detail::FomValidationStatus;
using umbra::detail::LibXml2FomValidator;

std::filesystem::path resourcePath(std::filesystem::path const& relative) {
  return std::filesystem::path(UMBRA_SOURCE_DIRECTORY) / "third_party" / "ieee1516.2-2025" /
         "resources" / relative;
}

FomValidationRequest requestFor(
    std::filesystem::path const& relative,
    FomModuleKind kind,
    std::filesystem::path const& schemaRelative = "schemas/IEEE1516-DIF-2025.xsd") {
  return {
      resourcePath(relative),
      resourcePath(schemaRelative),
      kind,
      L"urn:umbra:test:" + relative.filename().wstring(),
      schemaRelative.filename().wstring(),
  };
}

}  // namespace

TEST_CASE("The libxml2 FOM validator accepts supplied IEEE 1516.2-2025 modules under DIF", "[unit][fom][xml-schema]") {
  LibXml2FomValidator validator;

  for (std::filesystem::path const& relative : {
           std::filesystem::path("examples/RestaurantFOMmodule-2025.xml"),
           std::filesystem::path("examples/RestaurantExtensionFOMmodule-2025.xml"),
           std::filesystem::path("examples/RestaurantSOMmodule-2025.xml"),
       }) {
    CAPTURE(relative.string());
    auto model = validator.validate(requestFor(relative, FomModuleKind::fom));
    REQUIRE(model.status == FomValidationStatus::valid);
    REQUIRE(model.module.has_value());
    REQUIRE(model.module->designator == L"urn:umbra:test:" + relative.filename().wstring());
    REQUIRE(model.module->schemaDesignator == L"IEEE1516-DIF-2025.xsd");
  }

  auto mim = validator.validate(requestFor("mim/HLAstandardMIM-2025.xml", FomModuleKind::mim));
  REQUIRE(mim.status == FomValidationStatus::valid);
  REQUIRE(mim.module.has_value());
}

TEST_CASE("The libxml2 FOM validator distinguishes missing, invalid, and DTD-bearing input", "[unit][fom][xml-schema]") {
  LibXml2FomValidator validator;

  auto missing = validator.validate(requestFor("examples/missing.xml", FomModuleKind::fom));
  REQUIRE(missing.status == FomValidationStatus::source_not_found);

  auto wrongNamespace = validator.validate({
      std::filesystem::path(UMBRA_SOURCE_DIRECTORY) / "cpp" / "tests" / "data" / "wrong-namespace-fom.xml",
      resourcePath("schemas/IEEE1516-DIF-2025.xsd"),
      FomModuleKind::fom,
      L"urn:umbra:test:wrong-namespace",
      L"IEEE1516-DIF-2025.xsd",
  });
  REQUIRE(wrongNamespace.status == FomValidationStatus::invalid_model);

  auto dtd = validator.validate({
      std::filesystem::path(UMBRA_SOURCE_DIRECTORY) / "cpp" / "tests" / "data" / "dtd-bearing-fom.xml",
      resourcePath("schemas/IEEE1516-DIF-2025.xsd"),
      FomModuleKind::fom,
      L"urn:umbra:test:dtd",
      L"IEEE1516-DIF-2025.xsd",
  });
  REQUIRE(dtd.status == FomValidationStatus::invalid_model);

  auto malformed = validator.validate({
      std::filesystem::path(UMBRA_SOURCE_DIRECTORY) / "cpp" / "tests" / "data" / "malformed-fom.xml",
      resourcePath("schemas/IEEE1516-DIF-2025.xsd"),
      FomModuleKind::fom,
      L"urn:umbra:test:malformed",
      L"IEEE1516-DIF-2025.xsd",
  });
  REQUIRE(malformed.status == FomValidationStatus::source_parse_error);
  REQUIRE_FALSE(malformed.module.has_value());
}

TEST_CASE(
    "The official 2025 DIF schema rejects duplicate attribute and parameter names within one class",
    "[unit][fom][xml-schema][fom-member-name-uniqueness]") {
  LibXml2FomValidator validator;
  auto const testData = std::filesystem::path(UMBRA_SOURCE_DIRECTORY) / "cpp" / "tests" / "data";

  for (auto const& [filename, designator, marker] : {
           std::tuple{
               "duplicate-attribute-name-fom.xml",
               L"urn:umbra:test:duplicate-attribute-name",
               "attributeName"},
           std::tuple{
               "duplicate-parameter-name-fom.xml",
               L"urn:umbra:test:duplicate-parameter-name",
               "parameterName"},
       }) {
    auto result = validator.validate({
        testData / filename,
        resourcePath("schemas/IEEE1516-DIF-2025.xsd"),
        FomModuleKind::fom,
        designator,
        L"IEEE1516-DIF-2025.xsd",
    });
    CAPTURE(filename, result.diagnostics);
    REQUIRE(result.status == FomValidationStatus::invalid_model);
    REQUIRE_FALSE(result.module.has_value());
    REQUIRE(result.diagnostics.find(marker) != std::string::npos);
  }
}

TEST_CASE(
    "The libxml2 FOM validator reports an existing non-file source as unreadable",
    "[unit][fom][xml-schema][fom-source-diagnostics]") {
  LibXml2FomValidator validator;
  auto const directory = resourcePath("examples");
  REQUIRE(std::filesystem::is_directory(directory));

  auto result = validator.validate({
      directory,
      resourcePath("schemas/IEEE1516-DIF-2025.xsd"),
      FomModuleKind::fom,
      L"urn:umbra:test:directory-source",
      L"IEEE1516-DIF-2025.xsd",
  });

  REQUIRE(result.status == FomValidationStatus::source_unreadable);
  REQUIRE_FALSE(result.module.has_value());
  REQUIRE(result.diagnostics.find("not a regular file") != std::string::npos);
}

TEST_CASE("The strict OMT schema is not substituted for individual 2025 modules", "[unit][fom][xml-schema][schema-policy]") {
  LibXml2FomValidator validator;

  // OMT's key/keyref constraints describe a complete object model.  Standard
  // MIM/FOM/SOM modules intentionally carry references satisfied only after
  // composition, so DIF remains the individual-module schema policy.
  for (auto const& [relative, kind] : {
           std::pair{std::filesystem::path("mim/HLAstandardMIM-2025.xml"), FomModuleKind::mim},
           std::pair{std::filesystem::path("examples/RestaurantFOMmodule-2025.xml"), FomModuleKind::fom},
           std::pair{std::filesystem::path("examples/RestaurantExtensionFOMmodule-2025.xml"), FomModuleKind::fom},
           std::pair{std::filesystem::path("examples/RestaurantSOMmodule-2025.xml"), FomModuleKind::fom},
       }) {
    auto result = validator.validate(requestFor(relative, kind, "schemas/IEEE1516-OMT-2025.xsd"));
    CAPTURE(relative.string(), result.diagnostics);
    REQUIRE(result.status == FomValidationStatus::invalid_model);
    REQUIRE_FALSE(result.module.has_value());
  }
}
