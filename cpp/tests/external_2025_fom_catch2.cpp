#include <catch2/catch_test_macros.hpp>

#include "internal/fdd_document.hpp"
#include "internal/fom_catalog.hpp"
#include "internal/libxml2_fom_composer.hpp"
#include "internal/libxml2_fom_validator.hpp"

#include <array>
#include <filesystem>
#include <memory>
#include <string>
#include <vector>

#ifdef UMBRA_ENABLE_EMBEDDED_FEDERATION_MANAGEMENT
#include <atomic>

#include <RTI/NullFederateAmbassador.h>
#include <RTI/RTI1516.h>
#endif

#ifndef UMBRA_EXTERNAL_2025_FOM_CORPUS_DIRECTORY
#error "UMBRA_EXTERNAL_2025_FOM_CORPUS_DIRECTORY must be defined for external 2025 corpus tests."
#endif

#ifndef UMBRA_SOURCE_DIRECTORY
#error "UMBRA_SOURCE_DIRECTORY must be defined for FOM resource tests."
#endif

namespace {

using umbra::detail::FomModuleKind;
using umbra::detail::FomCompositionStatus;
using umbra::detail::FomValidationResult;
using umbra::detail::LibXml2FomModuleComposer;
using umbra::detail::LibXml2FomValidator;
using umbra::detail::PrevalidatedFomModule;

std::filesystem::path resourcePath(std::filesystem::path const& relative) {
  return std::filesystem::path(UMBRA_SOURCE_DIRECTORY) / "third_party" / "ieee1516.2-2025" /
         "resources" / relative;
}

std::filesystem::path externalCorpusPath(std::filesystem::path const& relative) {
  return std::filesystem::path(UMBRA_EXTERNAL_2025_FOM_CORPUS_DIRECTORY) / relative;
}

FomValidationResult validate2025(
    std::filesystem::path const& source,
    FomModuleKind kind = FomModuleKind::fom) {
  LibXml2FomValidator validator;
  return validator.validate({
      source,
      resourcePath("schemas/IEEE1516-DIF-2025.xsd"),
      kind,
      L"urn:umbra:external-2025:" + source.filename().wstring(),
      L"IEEE1516-DIF-2025.xsd",
  });
}

std::array<std::filesystem::path, 4> externalSources() {
  return {{
      externalCorpusPath("Proto2025_Base.xml"),
      externalCorpusPath("Proto2025_MessageTest.xml"),
      externalCorpusPath("Proto2025_TimeMgmtTest.xml"),
      externalCorpusPath("Proto2025_SpaceLite.xml"),
  }};
}

std::filesystem::path directedTsoRestoreSource() {
  return externalCorpusPath("DirectedTSORestore2025.xml");
}

std::filesystem::path directedDdmRestoreSource() {
  return externalCorpusPath("DirectedDDMRestore2025.xml");
}

#ifdef UMBRA_ENABLE_EMBEDDED_FEDERATION_MANAGEMENT

std::unique_ptr<rti1516_2025::RTIambassador> makeRti() {
  rti1516_2025::RTIambassadorFactory factory;
  return factory.createRTIambassador();
}

std::wstring nextFederationName() {
  static std::atomic_uint64_t counter{0};
  return L"umbra-external-2025-fom-" + std::to_wstring(++counter);
}

#endif

}  // namespace

TEST_CASE("The optional external 2025 FOM corpus validates each module under DIF", "[unit][fom][xml-schema][external-2025]") {
  auto const sources = externalSources();

  for (std::filesystem::path const& source : sources) {
    REQUIRE(std::filesystem::is_regular_file(source));
    auto result = validate2025(source);
    CAPTURE(source.filename().string(), result.diagnostics);
    REQUIRE(result.status == umbra::detail::FomValidationStatus::valid);
    REQUIRE(result.module.has_value());
  }
}

TEST_CASE(
    "The optional external 2025 FOM corpus has a repeatable semantic preflight rejection",
    "[unit][fom][composition][external-2025]") {
  auto const sources = externalSources();
  std::vector<PrevalidatedFomModule> modules;
  modules.reserve(sources.size() + 1);

  auto mim = validate2025(resourcePath("mim/HLAstandardMIM-2025.xml"), FomModuleKind::mim);
  REQUIRE(mim.status == umbra::detail::FomValidationStatus::valid);
  REQUIRE(mim.module.has_value());
  modules.push_back(*mim.module);

  for (std::filesystem::path const& source : sources) {
    REQUIRE(std::filesystem::is_regular_file(source));
    auto result = validate2025(source);
    CAPTURE(source.filename().string(), result.diagnostics);
    REQUIRE(result.status == umbra::detail::FomValidationStatus::valid);
    REQUIRE(result.module.has_value());
    modules.push_back(*result.module);
  }

  LibXml2FomModuleComposer composer(resourcePath("schemas/IEEE1516-FDD-2025.xsd"));
  auto result = composer.compose(modules);
  CAPTURE(result.diagnostics);
  REQUIRE(result.status == FomCompositionStatus::inconsistent_modules);
  REQUIRE(result.diagnostics.find("HLAfloat32BE") != std::string::npos);
  REQUIRE(result.diagnostics.find("not permitted") != std::string::npos);

  auto repeat = composer.compose(modules);
  CAPTURE(repeat.diagnostics);
  REQUIRE(repeat.status == FomCompositionStatus::inconsistent_modules);
  REQUIRE(repeat.diagnostics == result.diagnostics);
}

TEST_CASE(
    "The optional directed restore sources retain their reviewed 2025 schema boundaries",
    "[unit][fom][xml-schema][composition][external-2025]") {
  auto const tsoSource = directedTsoRestoreSource();
  REQUIRE(std::filesystem::is_regular_file(tsoSource));
  auto tso = validate2025(tsoSource);
  CAPTURE(tso.diagnostics);
  REQUIRE(tso.status == umbra::detail::FomValidationStatus::valid);
  REQUIRE(tso.module.has_value());

  auto mim = validate2025(resourcePath("mim/HLAstandardMIM-2025.xml"), FomModuleKind::mim);
  REQUIRE(mim.status == umbra::detail::FomValidationStatus::valid);
  REQUIRE(mim.module.has_value());
  LibXml2FomModuleComposer composer(resourcePath("schemas/IEEE1516-FDD-2025.xsd"));
  auto tsoComposition = composer.compose({*mim.module, *tso.module});
  CAPTURE(tsoComposition.diagnostics);
  REQUIRE(tsoComposition.status == FomCompositionStatus::valid);
  REQUIRE(tsoComposition.catalog);
  REQUIRE(tsoComposition.catalog->objectClass("HLAobjectRoot.Target") != nullptr);
  REQUIRE(tsoComposition.catalog->interactionClass("HLAinteractionRoot.TrackReport") != nullptr);

  auto const ddmSource = directedDdmRestoreSource();
  REQUIRE(std::filesystem::is_regular_file(ddmSource));
  auto ddm = validate2025(ddmSource);
  CAPTURE(ddm.diagnostics);
  REQUIRE(ddm.status == umbra::detail::FomValidationStatus::invalid_model);
  REQUIRE_FALSE(ddm.module.has_value());
  REQUIRE_FALSE(ddm.diagnostics.empty());
}

#ifdef UMBRA_ENABLE_EMBEDDED_FEDERATION_MANAGEMENT

TEST_CASE(
    "The optional external 2025 FOM corpus is rejected by embedded Create preflight",
    "[integration][development-profile][fom][external-2025]") {
  rti1516_2025::NullFederateAmbassador creatorFederate;
  auto creator = makeRti();
  auto const federationName = nextFederationName();
  std::vector<std::wstring> const fomModules = [] {
    std::vector<std::wstring> result;
    for (std::filesystem::path const& source : externalSources()) {
      result.push_back(source.wstring());
    }
    return result;
  }();

  REQUIRE_NOTHROW(creator->connect(creatorFederate, rti1516_2025::HLA_EVOKED));
  REQUIRE_THROWS_AS(
      creator->createFederationExecution(federationName, fomModules, L"HLAinteger64Time"),
      rti1516_2025::InconsistentFOM);
  REQUIRE_NOTHROW(creator->disconnect());
}

#endif
