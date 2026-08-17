#include <catch2/catch_test_macros.hpp>

#include "internal/libxml2_fom_validator.hpp"

#include <array>
#include <filesystem>

#ifndef UMBRA_EXTERNAL_SISO_FOM_CORPUS_DIRECTORY
#error "UMBRA_EXTERNAL_SISO_FOM_CORPUS_DIRECTORY must be defined for external SISO corpus tests."
#endif

#ifndef UMBRA_SOURCE_DIRECTORY
#error "UMBRA_SOURCE_DIRECTORY must be defined for FOM resource tests."
#endif

namespace {

using umbra::detail::FomModuleKind;
using umbra::detail::FomValidationStatus;
using umbra::detail::LibXml2FomValidator;

struct ExternalFixture final {
  char const* id;
  std::filesystem::path relativePath;
};

std::filesystem::path resourcePath(std::filesystem::path const& relative) {
  return std::filesystem::path(UMBRA_SOURCE_DIRECTORY) / "third_party" / "ieee1516.2-2025" /
         "resources" / relative;
}

std::filesystem::path externalCorpusPath(std::filesystem::path const& relative) {
  return std::filesystem::path(UMBRA_EXTERNAL_SISO_FOM_CORPUS_DIRECTORY) / relative;
}

}  // namespace

TEST_CASE(
    "The optional SISO corpus remains explicitly outside the IEEE 1516.2-2025 input policy",
    "[unit][fom][xml-schema][external-corpus][cross-edition]") {
  // These fixtures deliberately span a Space family, an RPR publication that
  // is dated 2025, and a Link 16 extension.  Their XML namespace and selected
  // schema are IEEE 1516-2010, so accepting any of them under the 2025 DIF
  // policy would be a cross-edition bug rather than interoperability support.
  std::array<ExternalFixture, 7> const fixtures{{
      {"siso-space-datatypes", std::filesystem::path("SISO-STD-018 Files Normative") / "SISO_SpaceFOM_datatypes.xml"},
      {"siso-space-environment", std::filesystem::path("SISO-STD-018 Files Normative") / "SISO_SpaceFOM_environment.xml"},
      {"siso-space-switches", std::filesystem::path("SISO-STD-018 Files Normative") / "SISO_SpaceFOM_switches.xml"},
      {"siso-space-management", std::filesystem::path("SISO-STD-018 Files Normative") / "SISO_SpaceFOM_management.xml"},
      {"siso-space-entity", std::filesystem::path("SISO-STD-018 Files Normative") / "SISO_SpaceFOM_entity.xml"},
      {"siso-rpr-3-foundation", std::filesystem::path("SISO-STD-001.1-2025 Annex A Files Normative") / "RPR-Foundation_v3.0.xml"},
      {"siso-link-16", std::filesystem::path("RPR FOM v2.0 Link 16 and Link 11") / "Link_16_v2.0.xml"},
  }};

  LibXml2FomValidator validator;
  for (ExternalFixture const& fixture : fixtures) {
    auto const source = externalCorpusPath(fixture.relativePath);
    CAPTURE(fixture.id, source.string());
    REQUIRE(std::filesystem::is_regular_file(source));

    auto result = validator.validate({
        source,
        resourcePath("schemas/IEEE1516-DIF-2025.xsd"),
        FomModuleKind::fom,
        L"urn:umbra:external-siso:" + source.filename().wstring(),
        L"IEEE1516-DIF-2025.xsd",
    });
    CAPTURE(result.diagnostics);
    REQUIRE(result.status == FomValidationStatus::invalid_model);
    REQUIRE_FALSE(result.module.has_value());
  }
}
