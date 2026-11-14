#include <catch2/catch_test_macros.hpp>

#include "internal/fom/libxml2_fom_validator.hpp"

#include <filesystem>

#ifndef UMBRA_SOURCE_DIRECTORY
#error "UMBRA_SOURCE_DIRECTORY must be defined for standard-MIM schema tests."
#endif

namespace {

using umbra::detail::FomModuleKind;
using umbra::detail::FomValidationRequest;
using umbra::detail::FomValidationStatus;
using umbra::detail::LibXml2FomValidator;

}  // namespace

TEST_CASE(
    "The pinned IEEE 1516.2-2025 standard MIM conforms to the official DIF schema",
    "[unit][fom][fom-module-management][xml-schema][standard-mim][ieee1516-2-2025]") {
  std::filesystem::path const standardsRoot =
      std::filesystem::path(UMBRA_SOURCE_DIRECTORY) / "third_party" /
      "ieee1516.2-2025" / "resources";
  FomValidationRequest const request{
      standardsRoot / "mim" / "HLAstandardMIM-2025.xml",
      standardsRoot / "schemas" / "IEEE1516-DIF-2025.xsd",
      FomModuleKind::mim,
      L"urn:umbra:test:standard-mim-2025",
      L"IEEE1516-DIF-2025.xsd",
  };

  LibXml2FomValidator validator;
  auto const result = validator.validate(request);
  CAPTURE(result.diagnostics);
  REQUIRE(result.status == FomValidationStatus::valid);
  REQUIRE(result.module.has_value());
  REQUIRE(result.module->designator == request.designator);
  REQUIRE(result.module->schemaDesignator == request.schemaDesignator);
}
