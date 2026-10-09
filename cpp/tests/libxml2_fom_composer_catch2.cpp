#include <catch2/catch_test_macros.hpp>

#include "internal/fom/fdd_document.hpp"
#include "internal/time/federate_time_state.hpp"
#include "internal/fom/fom_catalog.hpp"
#include "internal/federation/federation_management_coordinator.hpp"
#include "internal/federation/federation_registry.hpp"
#include "internal/handles/handle_variable_array_encoding.hpp"
#include "internal/fom/libxml2_fom_composer.hpp"
#include "internal/fom/libxml2_fom_validator.hpp"
#include "internal/time/reference_time_selection.hpp"

#include <algorithm>
#include <atomic>
#include <filesystem>
#include <fstream>
#include <iterator>
#include <memory>
#include <optional>
#include <set>
#include <string>
#include <string_view>
#include <system_error>
#include <tuple>
#include <vector>

#include <RTI/time/HLAinteger64Time.h>
#include <RTI/encoding/BasicDataElements.h>

#include "fom_composer_test_support.hpp"

#ifndef UMBRA_SOURCE_DIRECTORY
#error "UMBRA_SOURCE_DIRECTORY must be defined by CMake for FOM resource tests."
#endif

namespace {

using umbra::detail::FomCompositionStatus;
using umbra::detail::FomModuleKind;
using umbra::detail::FomValidationRequest;
using umbra::detail::FomValidationStatus;
using umbra::detail::FederationManagementCoordinator;
using umbra::detail::FederationManagementResources;
using umbra::detail::FederationPreparationStatus;
using umbra::detail::FederationDefinition;
using umbra::detail::FederationRegistryStatus;
using umbra::detail::FederateTimeState;
using umbra::detail::InteractionClassDeclarationStatus;
using umbra::detail::InteractionProducer;
using umbra::detail::MomServiceReportDisposition;
using umbra::detail::ObjectClassAttributeDeclarationStatus;
using umbra::detail::ObjectInstanceRegistrationStatus;
using umbra::detail::LibXml2FomModuleComposer;
using umbra::detail::LibXml2FomValidator;
using umbra::detail::PrevalidatedFomModule;
using umbra::detail::ReceiveOrderInteractionStatus;
using umbra::detail::ReferenceLogicalTimeSelectionStatus;
using umbra::detail::ReferenceLogicalTimeSelector;


class ScopedTemporaryFile final {
 public:
  explicit ScopedTemporaryFile(std::filesystem::path path) : path_(std::move(path)) {}

  ScopedTemporaryFile(ScopedTemporaryFile const&) = delete;
  ScopedTemporaryFile& operator=(ScopedTemporaryFile const&) = delete;

  ~ScopedTemporaryFile() {
    std::error_code ignored;
    std::filesystem::remove(path_, ignored);
  }

  [[nodiscard]] std::filesystem::path const& path() const noexcept {
    return path_;
  }

 private:
  std::filesystem::path path_;
};

ScopedTemporaryFile nrgEnabledRestaurantModule() {
  static std::atomic_uint64_t counter{0};
  auto const source = resourcePath("examples/RestaurantFOMmodule-2025.xml");
  std::ifstream input(source, std::ios::binary);
  REQUIRE(input.good());
  std::string fomText{
      std::istreambuf_iterator<char>(input),
      std::istreambuf_iterator<char>()};
  // IEEE 1516.2 defines switches as an ordered XML sequence; NRG follows the
  // advisory entries in the official schema, so append it immediately before
  // the existing closing element rather than at the start of the section.
  std::string const marker = "</switches>";
  auto const switchesPosition = fomText.find(marker);
  REQUIRE(switchesPosition != std::string::npos);
  fomText.insert(
      switchesPosition,
      "        <nonRegulatedGrant isEnabled=\"true\"/>\n    ");

  auto const path = std::filesystem::temp_directory_path() /
      ("umbra-nrg-enabled-" + std::to_string(++counter) + ".xml");
  std::ofstream output(path, std::ios::binary | std::ios::trunc);
  REQUIRE(output.good());
  output << fomText;
  REQUIRE(output.good());
  return ScopedTemporaryFile(path);
}

ScopedTemporaryFile restaurantModuleWithConflictingSynchronizationNote() {
  static std::atomic_uint64_t counter{0};
  auto const source = resourcePath("examples/RestaurantFOMmodule-2025.xml");
  std::ifstream input(source, std::ios::binary);
  REQUIRE(input.good());
  std::string fomText{
      std::istreambuf_iterator<char>(input),
      std::istreambuf_iterator<char>()};

  auto const labelPosition = fomText.find("<label>PauseExecution</label>");
  REQUIRE(labelPosition != std::string::npos);
  auto const synchronizationPointPosition = fomText.rfind(
      "<synchronizationPoint>",
      labelPosition);
  REQUIRE(synchronizationPointPosition != std::string::npos);
  fomText.replace(
      synchronizationPointPosition,
      std::string("<synchronizationPoint>").size(),
      "<synchronizationPoint noteReferences=\"Note1\">");

  auto const path = std::filesystem::temp_directory_path() /
      ("umbra-sync-duplicate-note-" + std::to_string(++counter) + ".xml");
  std::ofstream output(path, std::ios::binary | std::ios::trunc);
  REQUIRE(output.good());
  output << fomText;
  REQUIRE(output.good());
  return ScopedTemporaryFile(path);
}

ScopedTemporaryFile mimWithConflictingTransportationNote() {
  static std::atomic_uint64_t counter{0};
  auto const source = resourcePath("mim/HLAstandardMIM-2025.xml");
  std::ifstream input(source, std::ios::binary);
  REQUIRE(input.good());
  std::string mimText{
      std::istreambuf_iterator<char>(input),
      std::istreambuf_iterator<char>()};

  auto const namePosition = mimText.find("<name>HLAreliable</name>");
  REQUIRE(namePosition != std::string::npos);
  auto const transportationPosition = mimText.rfind(
      "<transportation>",
      namePosition);
  REQUIRE(transportationPosition != std::string::npos);
  mimText.replace(
      transportationPosition,
      std::string("<transportation>").size(),
      "<transportation noteReferences=\"MOM1\">");

  auto const path = std::filesystem::temp_directory_path() /
      ("umbra-transport-duplicate-note-" + std::to_string(++counter) + ".xml");
  std::ofstream output(path, std::ios::binary | std::ios::trunc);
  REQUIRE(output.good());
  output << mimText;
  REQUIRE(output.good());
  return ScopedTemporaryFile(path);
}

ScopedTemporaryFile restaurantModuleWithConflictingUpdateRateNote() {
  static std::atomic_uint64_t counter{0};
  auto const source = resourcePath("examples/RestaurantFOMmodule-2025.xml");
  std::ifstream input(source, std::ios::binary);
  REQUIRE(input.good());
  std::string fomText{
      std::istreambuf_iterator<char>(input),
      std::istreambuf_iterator<char>()};

  auto const namePosition = fomText.find("<name>High</name>");
  REQUIRE(namePosition != std::string::npos);
  auto const updateRatePosition = fomText.rfind(
      "<updateRate>",
      namePosition);
  REQUIRE(updateRatePosition != std::string::npos);
  fomText.replace(
      updateRatePosition,
      std::string("<updateRate>").size(),
      "<updateRate noteReferences=\"Note1\">");

  auto const path = std::filesystem::temp_directory_path() /
      ("umbra-update-rate-duplicate-note-" + std::to_string(++counter) + ".xml");
  std::ofstream output(path, std::ios::binary | std::ios::trunc);
  REQUIRE(output.good());
  output << fomText;
  REQUIRE(output.good());
  return ScopedTemporaryFile(path);
}

ScopedTemporaryFile dimensionProviderWithConflictingDimension() {
  static std::atomic_uint64_t counter{0};
  auto const source = std::filesystem::path(UMBRA_SOURCE_DIRECTORY) / "cpp" / "tests" /
      "data" / "dimension-reference-provider-fom.xml";
  std::ifstream input(source, std::ios::binary);
  REQUIRE(input.good());
  std::string fomText{
      std::istreambuf_iterator<char>(input),
      std::istreambuf_iterator<char>()};

  auto const namePosition = fomText.find("<name>UmbraDimensionFixture</name>");
  REQUIRE(namePosition != std::string::npos);
  auto const upperBoundPosition = fomText.find("<upperBound>100</upperBound>", namePosition);
  REQUIRE(upperBoundPosition != std::string::npos);
  fomText.replace(
      upperBoundPosition,
      std::string("<upperBound>100</upperBound>").size(),
      "<upperBound>101</upperBound>");

  auto const path = std::filesystem::temp_directory_path() /
      ("umbra-dimension-duplicate-conflict-" + std::to_string(++counter) + ".xml");
  std::ofstream output(path, std::ios::binary | std::ios::trunc);
  REQUIRE(output.good());
  output << fomText;
  REQUIRE(output.good());
  return ScopedTemporaryFile(path);
}

ScopedTemporaryFile dimensionProviderWithUniqueDimension() {
  static std::atomic_uint64_t counter{0};
  auto const source = std::filesystem::path(UMBRA_SOURCE_DIRECTORY) / "cpp" / "tests" /
      "data" / "dimension-reference-provider-fom.xml";
  std::ifstream input(source, std::ios::binary);
  REQUIRE(input.good());
  std::string fomText{
      std::istreambuf_iterator<char>(input),
      std::istreambuf_iterator<char>()};

  auto const dimensionsEnd = fomText.find("</dimensions>");
  REQUIRE(dimensionsEnd != std::string::npos);
  fomText.insert(
      dimensionsEnd,
      "        <dimension>\n"
      "            <name>UmbraUniqueDimension</name>\n"
      "            <inputDataTypes>\n"
      "                <dataType>HLAcount</dataType>\n"
      "            </inputDataTypes>\n"
      "            <inputDataDescription>NA</inputDataDescription>\n"
      "            <upperBound>2</upperBound>\n"
      "            <normalization>linear (HLAcount, 1, 2)</normalization>\n"
      "            <outputDataSemantics>Umbra dimension merge fixture</outputDataSemantics>\n"
      "            <value>[0..2)</value>\n"
      "        </dimension>\n");

  auto const path = std::filesystem::temp_directory_path() /
      ("umbra-dimension-unique-" + std::to_string(++counter) + ".xml");
  std::ofstream output(path, std::ios::binary | std::ios::trunc);
  REQUIRE(output.good());
  output << fomText;
  REQUIRE(output.good());
  return ScopedTemporaryFile(path);
}

ScopedTemporaryFile restaurantModuleWithNameReplacement(
    std::string_view originalName,
    std::string_view replacement,
    std::string_view label) {
  static std::atomic_uint64_t counter{0};
  auto const source = resourcePath("examples/RestaurantFOMmodule-2025.xml");
  std::ifstream input(source, std::ios::binary);
  REQUIRE(input.good());
  std::string fomText{
      std::istreambuf_iterator<char>(input),
      std::istreambuf_iterator<char>()};

  std::string const original = "<name>" + std::string(originalName) + "</name>";
  auto const position = fomText.find(original);
  REQUIRE(position != std::string::npos);
  std::string const replacementElement =
      "<name>" + std::string(replacement) + "</name>";
  fomText.replace(position, original.size(), replacementElement);

  auto const path = std::filesystem::temp_directory_path() /
      ("umbra-fom-name-" + std::string(label) + "-" +
       std::to_string(++counter) + ".xml");
  std::ofstream output(path, std::ios::binary | std::ios::trunc);
  REQUIRE(output.good());
  output << fomText;
  REQUIRE(output.good());
  return ScopedTemporaryFile(path);
}

ScopedTemporaryFile restaurantModuleWithTimezoneModificationDate() {
  static std::atomic_uint64_t counter{0};
  auto const source = resourcePath("examples/RestaurantFOMmodule-2025.xml");
  std::ifstream input(source, std::ios::binary);
  REQUIRE(input.good());
  std::string fomText{
      std::istreambuf_iterator<char>(input),
      std::istreambuf_iterator<char>()};

  std::string const original =
      "<modificationDate>2025-02-10</modificationDate>";
  auto const position = fomText.find(original);
  REQUIRE(position != std::string::npos);
  fomText.replace(
      position,
      original.size(),
      "<modificationDate>2025-02-10Z</modificationDate>");

  auto const path = std::filesystem::temp_directory_path() /
      ("umbra-fom-modification-date-" + std::to_string(++counter) + ".xml");
  std::ofstream output(path, std::ios::binary | std::ios::trunc);
  REQUIRE(output.good());
  output << fomText;
  REQUIRE(output.good());
  return ScopedTemporaryFile(path);
}

ScopedTemporaryFile restaurantModuleWithConflictingDimensionNote() {
  static std::atomic_uint64_t counter{0};
  auto const source = resourcePath("examples/RestaurantFOMmodule-2025.xml");
  std::ifstream input(source, std::ios::binary);
  REQUIRE(input.good());
  std::string fomText{
      std::istreambuf_iterator<char>(input),
      std::istreambuf_iterator<char>()};

  auto const labelPosition = fomText.find("<label>Note6</label>");
  REQUIRE(labelPosition != std::string::npos);
  auto const semanticsStart = fomText.find("<semantics>", labelPosition);
  REQUIRE(semanticsStart != std::string::npos);
  auto const semanticsEnd = fomText.find("</semantics>", semanticsStart);
  REQUIRE(semanticsEnd != std::string::npos);
  fomText.replace(
      semanticsStart,
      semanticsEnd + std::string("</semantics>").size() - semanticsStart,
      "<semantics>Conflicting dimension normalization note.</semantics>");

  auto const path = std::filesystem::temp_directory_path() /
      ("umbra-dimension-duplicate-note-conflict-" + std::to_string(++counter) + ".xml");
  std::ofstream output(path, std::ios::binary | std::ios::trunc);
  REQUIRE(output.good());
  output << fomText;
  REQUIRE(output.good());
  return ScopedTemporaryFile(path);
}

}  // namespace

TEST_CASE("The FDD materializer accepts the supplied MIM and base FOM", "[unit][fom][composition][annex-c]") {
  std::vector<PrevalidatedFomModule> modules{
      validated(resourcePath("mim/HLAstandardMIM-2025.xml"), FomModuleKind::mim, L"urn:umbra:test:mim"),
      validated(resourcePath("examples/RestaurantFOMmodule-2025.xml"), FomModuleKind::fom, L"urn:umbra:test:restaurant"),
  };

  auto materializer = composer();
  auto result = materializer.compose(modules);

  CAPTURE(result.diagnostics);
  REQUIRE(result.status == FomCompositionStatus::valid);
  REQUIRE(result.modules.size() == modules.size());
  REQUIRE(result.modules.front().kind == FomModuleKind::mim);
  REQUIRE(result.modules.back().designator == L"urn:umbra:test:restaurant");
  REQUIRE(result.catalog);
  REQUIRE(result.fdd);
  REQUIRE(result.fdd->composedFromModuleNames().size() == modules.size());
  REQUIRE(result.fdd->xmlUtf8().find("IEEE1516-FDD-2025.xsd") != std::string::npos);
  REQUIRE(result.fdd->xmlUtf8().find("<type>Composed_From</type>") != std::string::npos);
  REQUIRE(result.fdd->xmlUtf8().find("Restaurant FOM Module Example") != std::string::npos);

  auto const* employee = result.catalog->objectClass("HLAobjectRoot.Employee");
  REQUIRE(employee != nullptr);
  REQUIRE(employee->declaredAttributes.contains("PayRate"));
  REQUIRE(result.catalog->objectClass("HLAobjectRoot.Food.Drink.Soda") != nullptr);
  REQUIRE(result.catalog->interactionClass("HLAinteractionRoot.ServerAction.TakeOrder") != nullptr);
  REQUIRE(result.catalog->dimension("BarQuantity") != nullptr);

  auto const* drinkGarnish = result.catalog->dataType("DrinkGarnish");
  REQUIRE(drinkGarnish != nullptr);
  REQUIRE(drinkGarnish->kind == umbra::detail::FomDataTypeKind::variant_record);
  REQUIRE(result.catalog->time().logicalTimeDataType == "HLAinteger64Time");
  REQUIRE(result.catalog->time().logicalTimeIntervalDataType == "HLAinteger64Time");
  auto const* pauseExecution = result.catalog->synchronizationPoint("PauseExecution");
  REQUIRE(pauseExecution != nullptr);
  REQUIRE(pauseExecution->label == "PauseExecution");
  REQUIRE(pauseExecution->dataType == "HLAinteger64Time");
  REQUIRE(pauseExecution->capability == "RegisterAchieve");
  REQUIRE(pauseExecution->semantics.find("time advance") != std::string::npos);
  REQUIRE(result.catalog->synchronizationPoint("InitialPublish") != nullptr);
  REQUIRE(result.catalog->synchronizationPoint("MissingSynchronizationPoint") == nullptr);
  REQUIRE(
      result.catalog->synchronizationPointLabels() ==
      std::vector<std::string>{
          "BeginTimeAdvance", "InitialPublish", "InitialUpdate", "PauseExecution"});
  // The Restaurant FOM explicitly enables the object-class and interaction
  // relevance advisories.  The two omitted advisory entries remain Disabled
  // under the 1516.2 switch-table defaults.
  REQUIRE_FALSE(result.catalog->advisorySwitches().attributeScopeAdvisory);
  REQUIRE(result.catalog->advisorySwitches().attributeRelevanceAdvisory);
  REQUIRE(result.catalog->advisorySwitches().objectClassRelevanceAdvisory);
  REQUIRE(result.catalog->advisorySwitches().interactionRelevanceAdvisory);
  REQUIRE(result.catalog->federationSwitches().autoProvide);
  REQUIRE(
      result.catalog->federateSupportSwitches().automaticResignAction ==
      "CancelThenDeleteThenDivest");
  // IEEE 1516.2 makes an omitted switch Disabled.  The supplied Restaurant
  // FOM has no NRG entry, so the catalog must preserve that standard default
  // for a future federation-wide grant calculation.
  REQUIRE_FALSE(result.catalog->timeManagementSwitches().nonRegulatedGrant);
}

TEST_CASE(
    "The FDD materializer emits a strict OMT-positive complete model",
    "[unit][fom][composition][omt][complete-model][omt-complete-model]") {
  std::vector<PrevalidatedFomModule> modules{
      validated(
          std::filesystem::path(UMBRA_SOURCE_DIRECTORY) / "cpp" / "tests" / "data" /
              "strict-omt-complete-fom.xml",
          FomModuleKind::fom,
          L"urn:umbra:test:strict-omt-complete"),
  };
  auto result = composer().compose(modules);
  CAPTURE(result.diagnostics);
  REQUIRE(result.status == FomCompositionStatus::valid);
  REQUIRE(result.fdd);

  static std::atomic_uint64_t counter{0};
  auto const path = std::filesystem::temp_directory_path() /
      ("umbra-strict-omt-complete-" + std::to_string(++counter) + ".xml");
  ScopedTemporaryFile temporary(path);
  std::ofstream output(path, std::ios::binary | std::ios::trunc);
  REQUIRE(output.good());
  output << result.fdd->xmlUtf8();
  REQUIRE(output.good());
  output.close();
  auto const& materializedXml = result.fdd->xmlUtf8();
  REQUIRE(materializedXml.find("Umbra Strict OMT Complete FOM Fixture") != std::string::npos);
  REQUIRE(materializedXml.find("<type>Composed_From</type>") != std::string::npos);
  auto countElements = [&materializedXml](std::string_view element) {
    std::size_t count = 0;
    std::size_t offset = 0;
    while ((offset = materializedXml.find(element, offset)) != std::string::npos) {
      ++count;
      offset += element.size();
    }
    return count;
  };
  REQUIRE(countElements("<poc>") == 2);
  REQUIRE(countElements("<reference>") == 3);

  LibXml2FomValidator validator;
  auto strict = validator.validate({
      path,
      resourcePath("schemas/IEEE1516-OMT-2025.xsd"),
      FomModuleKind::fom,
      L"urn:umbra:test:composed-fdd",
      L"IEEE1516-OMT-2025.xsd",
  });
  CAPTURE(strict.diagnostics);
  REQUIRE(strict.status == FomValidationStatus::valid);
}

TEST_CASE(
    "The FOM composition preflight applies the Annex C.4 dimension duplicate rule",
    "[unit][fom][composition][annex-c][dimension-merge]") {
  auto const mim = resourcePath("mim/HLAstandardMIM-2025.xml");
  auto const baseModule = std::filesystem::path(UMBRA_SOURCE_DIRECTORY) / "cpp" / "tests" /
      "data" / "dimension-reference-provider-fom.xml";
  auto const conflictingModule = dimensionProviderWithConflictingDimension();

  auto result = composer().compose({
      validated(mim, FomModuleKind::mim, L"urn:umbra:test:mim"),
      validated(baseModule, FomModuleKind::fom, L"urn:umbra:test:restaurant"),
      validated(conflictingModule.path(), FomModuleKind::fom,
                L"urn:umbra:test:restaurant-conflicting-dimension"),
  });

  CAPTURE(result.diagnostics);
  REQUIRE(result.status == FomCompositionStatus::inconsistent_modules);
  REQUIRE(result.diagnostics.find("Conflicting duplicate dimension") != std::string::npos);
  REQUIRE(result.diagnostics.find("UmbraDimensionFixture") != std::string::npos);

  // Note references are remapped per module, but the referenced note content
  // remains part of C.4 equality. A changed dimension note therefore fails
  // before any later composition section is considered.
  auto const conflictingNoteModule = restaurantModuleWithConflictingDimensionNote();
  auto conflictingNote = composer().compose({
      validated(mim, FomModuleKind::mim, L"urn:umbra:test:mim-note-conflict"),
      validated(resourcePath("examples/RestaurantFOMmodule-2025.xml"), FomModuleKind::fom,
                L"urn:umbra:test:restaurant-note-base"),
      validated(conflictingNoteModule.path(), FomModuleKind::fom,
                L"urn:umbra:test:restaurant-note-conflict"),
  });

  CAPTURE(conflictingNote.diagnostics);
  REQUIRE(conflictingNote.status == FomCompositionStatus::inconsistent_modules);
  REQUIRE(conflictingNote.diagnostics.find("Conflicting duplicate dimension") !=
          std::string::npos);
  REQUIRE(conflictingNote.diagnostics.find("Gluten") != std::string::npos);

  // Repeating an identical module exercises the equivalent-duplicate path:
  // the same dimension definitions are ignored rather than structurally
  // merged a second time.
  auto equivalent = composer().compose({
      validated(mim, FomModuleKind::mim, L"urn:umbra:test:mim-equivalent"),
      validated(baseModule, FomModuleKind::fom, L"urn:umbra:test:restaurant-a"),
      validated(baseModule, FomModuleKind::fom, L"urn:umbra:test:restaurant-b"),
  });

  CAPTURE(equivalent.diagnostics);
  REQUIRE(equivalent.status == FomCompositionStatus::valid);
  REQUIRE(equivalent.catalog);
  REQUIRE(equivalent.catalog->dimension("UmbraDimensionFixture") != nullptr);

  // A dimension name that is not present in the first module remains a
  // candidate for insertion into the composed dimensions table.
  auto const uniqueModule = dimensionProviderWithUniqueDimension();
  auto unique = composer().compose({
      validated(mim, FomModuleKind::mim, L"urn:umbra:test:mim-unique"),
      validated(baseModule, FomModuleKind::fom, L"urn:umbra:test:restaurant-base"),
      validated(uniqueModule.path(), FomModuleKind::fom,
                L"urn:umbra:test:restaurant-unique-dimension"),
  });

  CAPTURE(unique.diagnostics);
  REQUIRE(unique.status == FomCompositionStatus::valid);
  REQUIRE(unique.catalog);
  REQUIRE(unique.catalog->dimension("UmbraUniqueDimension") != nullptr);
}

TEST_CASE(
    "The FOM composition preflight applies the Annex C.5 synchronization duplicate rule",
    "[unit][fom][composition][annex-c][synchronization-merge]") {
  auto const conflictingModule = restaurantModuleWithConflictingSynchronizationNote();
  auto const baseModule = resourcePath("examples/RestaurantFOMmodule-2025.xml");
  auto result = composer().compose({
      validated(resourcePath("mim/HLAstandardMIM-2025.xml"), FomModuleKind::mim,
                L"urn:umbra:test:mim"),
      validated(baseModule, FomModuleKind::fom, L"urn:umbra:test:restaurant"),
      validated(conflictingModule.path(), FomModuleKind::fom,
                L"urn:umbra:test:restaurant-conflicting-sync"),
  });

  CAPTURE(result.diagnostics);
  REQUIRE(result.status == FomCompositionStatus::inconsistent_modules);
  REQUIRE(result.diagnostics.find("Conflicting duplicate synchronization point") !=
          std::string::npos);
  REQUIRE(result.diagnostics.find("PauseExecution") != std::string::npos);

  // Repeating an identical module exercises the adjacent Annex C.5 duplicate
  // path: the same-label synchronization points are ignored, not merged into
  // a second catalog row.
  auto equivalent = composer().compose({
      validated(resourcePath("mim/HLAstandardMIM-2025.xml"), FomModuleKind::mim,
                L"urn:umbra:test:mim-equivalent"),
      validated(baseModule, FomModuleKind::fom, L"urn:umbra:test:restaurant-a"),
      validated(baseModule, FomModuleKind::fom, L"urn:umbra:test:restaurant-b"),
  });

  CAPTURE(equivalent.diagnostics);
  REQUIRE(equivalent.status == FomCompositionStatus::valid);
  REQUIRE(equivalent.catalog);
  REQUIRE(
      equivalent.catalog->synchronizationPointLabels() ==
      std::vector<std::string>{
          "BeginTimeAdvance", "InitialPublish", "InitialUpdate", "PauseExecution"});

  // Note labels are remapped independently for each source module. Two
  // otherwise identical synchronization definitions must therefore compare
  // their referenced note content, not the generated labels.
  auto noteEquivalent = composer().compose({
      validated(resourcePath("mim/HLAstandardMIM-2025.xml"), FomModuleKind::mim,
                L"urn:umbra:test:mim-note-equivalent"),
      validated(conflictingModule.path(), FomModuleKind::fom,
                L"urn:umbra:test:restaurant-sync-note-a"),
      validated(conflictingModule.path(), FomModuleKind::fom,
                L"urn:umbra:test:restaurant-sync-note-b"),
  });

  CAPTURE(noteEquivalent.diagnostics);
  REQUIRE(noteEquivalent.status == FomCompositionStatus::valid);
  REQUIRE(noteEquivalent.catalog);
}

TEST_CASE(
    "The FOM composition preflight applies the Annex C.6 transportation duplicate rule",
    "[unit][fom][composition][annex-c][transportation-merge]") {
  auto const conflictingModule = mimWithConflictingTransportationNote();
  auto const baseMim = resourcePath("mim/HLAstandardMIM-2025.xml");
  auto const restaurant = resourcePath("examples/RestaurantFOMmodule-2025.xml");

  auto result = composer().compose({
      validated(baseMim, FomModuleKind::mim, L"urn:umbra:test:mim"),
      validated(conflictingModule.path(), FomModuleKind::mim,
                L"urn:umbra:test:mim-conflicting-transport"),
      validated(restaurant, FomModuleKind::fom, L"urn:umbra:test:restaurant"),
  });

  CAPTURE(result.diagnostics);
  REQUIRE(result.status == FomCompositionStatus::inconsistent_modules);
  REQUIRE(result.diagnostics.find("Conflicting duplicate transportation type") !=
          std::string::npos);
  REQUIRE(result.diagnostics.find("HLAreliable") != std::string::npos);

  // Repeating the same MIM exercises the equivalent duplicate path: the
  // standard transportation definitions remain single logical rows and the
  // normal Restaurant extension still composes successfully.
  auto equivalent = composer().compose({
      validated(baseMim, FomModuleKind::mim, L"urn:umbra:test:mim-a"),
      validated(baseMim, FomModuleKind::mim, L"urn:umbra:test:mim-b"),
      validated(restaurant, FomModuleKind::fom, L"urn:umbra:test:restaurant-equivalent"),
  });

  CAPTURE(equivalent.diagnostics);
  REQUIRE(equivalent.status == FomCompositionStatus::valid);
  REQUIRE(equivalent.fdd);
  REQUIRE(equivalent.fdd->xmlUtf8().find("<name>HLAreliable</name>") != std::string::npos);
  REQUIRE(equivalent.fdd->xmlUtf8().find("<name>HLAbestEffort</name>") != std::string::npos);
  REQUIRE(equivalent.fdd->xmlUtf8().find("<name>PriorityBestEffort</name>") !=
          std::string::npos);

  // The modified transportation row references MOM1. Repeating that module
  // must remain equivalent even though each module receives a fresh Umbra
  // note label during composition.
  auto noteEquivalent = composer().compose({
      validated(conflictingModule.path(), FomModuleKind::mim,
                L"urn:umbra:test:mim-note-a"),
      validated(conflictingModule.path(), FomModuleKind::mim,
                L"urn:umbra:test:mim-note-b"),
      validated(restaurant, FomModuleKind::fom, L"urn:umbra:test:restaurant-note-transport"),
  });

  CAPTURE(noteEquivalent.diagnostics);
  REQUIRE(noteEquivalent.status == FomCompositionStatus::valid);
  REQUIRE(noteEquivalent.catalog);
}

TEST_CASE(
    "The FOM composition preflight applies the Annex C.7 update-rate duplicate rule",
    "[unit][fom][composition][annex-c][update-rate-merge]") {
  auto const conflictingModule = restaurantModuleWithConflictingUpdateRateNote();
  auto const mim = resourcePath("mim/HLAstandardMIM-2025.xml");
  auto const baseModule = resourcePath("examples/RestaurantFOMmodule-2025.xml");

  auto result = composer().compose({
      validated(mim, FomModuleKind::mim, L"urn:umbra:test:mim"),
      validated(baseModule, FomModuleKind::fom, L"urn:umbra:test:restaurant"),
      validated(conflictingModule.path(), FomModuleKind::fom,
                L"urn:umbra:test:restaurant-conflicting-update-rate"),
  });

  CAPTURE(result.diagnostics);
  REQUIRE(result.status == FomCompositionStatus::inconsistent_modules);
  REQUIRE(result.diagnostics.find("Conflicting duplicate update rate") !=
          std::string::npos);
  REQUIRE(result.diagnostics.find("High") != std::string::npos);

  // Repeating an identical module exercises the equivalent duplicate path;
  // the private catalog retains one deterministic High rate value.
  auto equivalent = composer().compose({
      validated(mim, FomModuleKind::mim, L"urn:umbra:test:mim-equivalent"),
      validated(baseModule, FomModuleKind::fom, L"urn:umbra:test:restaurant-a"),
      validated(baseModule, FomModuleKind::fom, L"urn:umbra:test:restaurant-b"),
  });

  CAPTURE(equivalent.diagnostics);
  REQUIRE(equivalent.status == FomCompositionStatus::valid);
  REQUIRE(equivalent.catalog);
  auto const highRate = equivalent.catalog->updateRateValue("High");
  REQUIRE(highRate.has_value());
  REQUIRE(*highRate == 30.0);

  // The repeated High row references Note1. Its meaning is unchanged when
  // the source module is loaded twice, despite per-module note-label remap.
  auto noteEquivalent = composer().compose({
      validated(mim, FomModuleKind::mim, L"urn:umbra:test:mim-update-note"),
      validated(conflictingModule.path(), FomModuleKind::fom,
                L"urn:umbra:test:restaurant-update-note-a"),
      validated(conflictingModule.path(), FomModuleKind::fom,
                L"urn:umbra:test:restaurant-update-note-b"),
  });

  CAPTURE(noteEquivalent.diagnostics);
  REQUIRE(noteEquivalent.status == FomCompositionStatus::valid);
  REQUIRE(noteEquivalent.catalog);
}

TEST_CASE(
    "The FOM composition preflight rejects a data-type name collision across modules",
    "[unit][fom][composition][data-types]") {
  auto const result = composer().compose({
      validated(resourcePath("mim/HLAstandardMIM-2025.xml"), FomModuleKind::mim,
                L"urn:umbra:test:mim-data-type-collision"),
      validated(
          std::filesystem::path(UMBRA_SOURCE_DIRECTORY) / "cpp" / "tests" / "data" /
              "data-type-name-collision-provider-fom.xml",
          FomModuleKind::fom,
          L"urn:umbra:test:data-type-collision"),
  });

  CAPTURE(result.diagnostics);
  REQUIRE(result.status == FomCompositionStatus::inconsistent_modules);
  REQUIRE(result.diagnostics.find("Data type") != std::string::npos);
  REQUIRE(result.diagnostics.find("HLAinteger32BE") != std::string::npos);
}

TEST_CASE(
    "The FDD catalog preserves MIM attribute policy for a future RTI-owned MOM object",
    "[unit][fom][composition][mom][service-reporting]") {
  auto result = composer().compose({
      validated(resourcePath("mim/HLAstandardMIM-2025.xml"), FomModuleKind::mim,
                L"urn:umbra:test:mim"),
      validated(resourcePath("examples/RestaurantFOMmodule-2025.xml"), FomModuleKind::fom,
                L"urn:umbra:test:restaurant"),
  });

  CAPTURE(result.diagnostics);
  REQUIRE(result.status == FomCompositionStatus::valid);
  REQUIRE(result.catalog);
  auto const* joinedFederate =
      result.catalog->objectClass("HLAobjectRoot.HLAmanager.HLAfederate");
  REQUIRE(joinedFederate != nullptr);

  auto const reportFile = joinedFederate->declaredAttributes.find("HLAreportServiceFile");
  REQUIRE(reportFile != joinedFederate->declaredAttributes.end());
  REQUIRE(reportFile->second.dataType == "HLAunicodeString");
  // This is a catalog assertion over the unmodified vendored 1516.2 MIM. The
  // embedded-profile runtime separately follows Table 8's Static entry when
  // it eventually establishes the MOM object; RL-041 preserves the contrary
  // MIM policy instead of rewriting this source projection.
  REQUIRE(reportFile->second.updateType == "Conditional");
  REQUIRE(
      reportFile->second.updateCondition ==
      "The first time that both HLAserviceReporting and HLAsendServiceReportsToFile become true.");
  REQUIRE(reportFile->second.valueRequired);
  REQUIRE(reportFile->second.ownership == "NoTransfer");
  REQUIRE(reportFile->second.sharing == "Publish");
  REQUIRE(reportFile->second.transportation == "HLAreliable");
  REQUIRE(reportFile->second.order == "Receive");

  auto const federateName = joinedFederate->declaredAttributes.find("HLAfederateName");
  REQUIRE(federateName != joinedFederate->declaredAttributes.end());
  REQUIRE(federateName->second.updateType == "Static");
  REQUIRE(federateName->second.updateCondition == "NA");
}

TEST_CASE(
    "The FDD catalog resolves complete inherited policy for a joined-federate MOM object",
    "[unit][fom][composition][mom]") {
  auto result = composer().compose({
      validated(resourcePath("mim/HLAstandardMIM-2025.xml"), FomModuleKind::mim,
                L"urn:umbra:test:mim"),
      validated(resourcePath("examples/RestaurantFOMmodule-2025.xml"), FomModuleKind::fom,
                L"urn:umbra:test:restaurant"),
  });

  CAPTURE(result.diagnostics);
  REQUIRE(result.status == FomCompositionStatus::valid);
  REQUIRE(result.catalog);
  auto const* joinedFederate =
      result.catalog->objectClass("HLAobjectRoot.HLAmanager.HLAfederate");
  REQUIRE(joinedFederate != nullptr);

  auto const effective = result.catalog->effectiveObjectClassAttributes(joinedFederate->name);
  REQUIRE(effective);
  // The complete instance must retain the root policy rather than treating a
  // direct HLAfederate attribute list as an independently invented MOM
  // schema.  HLAprivilegeToDeleteObject is inherited and intentionally has a
  // different ownership policy from the direct RTI-owned attributes.
  auto const deletePrivilege = effective->find("HLAprivilegeToDeleteObject");
  REQUIRE(deletePrivilege != effective->end());
  REQUIRE(deletePrivilege->second.dataType == "HLAtoken");
  REQUIRE_FALSE(deletePrivilege->second.valueRequired);
  REQUIRE(deletePrivilege->second.ownership == "DivestAcquire");
  REQUIRE(deletePrivilege->second.updateType == "Static");
  REQUIRE(deletePrivilege->second.sharing == "PublishSubscribe");

  auto const reportFile = effective->find("HLAreportServiceFile");
  REQUIRE(reportFile != effective->end());
  REQUIRE(reportFile->second.valueRequired);
  REQUIRE(reportFile->second.ownership == "NoTransfer");
  for (auto const& [attributeName, attribute] : joinedFederate->declaredAttributes) {
    auto const effectiveAttribute = effective->find(attributeName);
    REQUIRE(effectiveAttribute != effective->end());
    REQUIRE(effectiveAttribute->second.valueRequired);
    REQUIRE(effectiveAttribute->second.ownership == "NoTransfer");
  }
  REQUIRE(effective->size() == joinedFederate->declaredAttributes.size() + 1U);
  REQUIRE_FALSE(result.catalog->effectiveObjectClassAttributes(""));
  REQUIRE_FALSE(
      result.catalog->effectiveObjectClassAttributes("HLAobjectRoot.DoesNotExist"));
}

TEST_CASE(
    "The FDD materializer retains the complete 2025 support-switch table",
    "[unit][fom][switches]") {
  std::vector<PrevalidatedFomModule> modules{
      validated(resourcePath("mim/HLAstandardMIM-2025.xml"), FomModuleKind::mim,
                L"urn:umbra:test:mim"),
      validated(
          std::filesystem::path(UMBRA_SOURCE_DIRECTORY) / "cpp" / "tests" / "data" /
              "switch-support-enabled-fom.xml",
          FomModuleKind::fom,
          L"urn:umbra:test:support-switches"),
  };

  auto result = composer().compose(modules);
  CAPTURE(result.diagnostics);
  REQUIRE(result.status == FomCompositionStatus::valid);
  REQUIRE(result.catalog);
  REQUIRE(result.catalog->federationSwitches().delaySubscriptionEvaluation);
  REQUIRE(result.catalog->federationSwitches().allowRelaxedDDM);
  auto const& support = result.catalog->federateSupportSwitches();
  REQUIRE(support.conveyRegionDesignatorSets);
  REQUIRE(support.automaticResignAction == "DeleteObjectsThenDivest");
  REQUIRE(support.serviceReporting);
  REQUIRE(support.exceptionReporting);
  REQUIRE(support.sendServiceReportsToFile);
}

TEST_CASE(
    "The FDD catalog preserves an explicit NoAction automatic-resign setting",
    "[unit][fom][switches]") {
  auto result = composer().compose({
      validated(resourcePath("mim/HLAstandardMIM-2025.xml"), FomModuleKind::mim,
                L"urn:umbra:test:mim"),
      validated(
          std::filesystem::path(UMBRA_SOURCE_DIRECTORY) / "cpp" / "tests" / "data" /
              "switch-explicit-no-action-fom.xml",
          FomModuleKind::fom,
          L"urn:umbra:test:explicit-no-action"),
  });
  CAPTURE(result.diagnostics);
  REQUIRE(result.status == FomCompositionStatus::valid);
  REQUIRE(result.catalog);
  // An explicit schema-valid NoAction value is one of the 2025 table's
  // accepted actions. It must not be collapsed into the distinct omitted-value
  // default selected by the existing RL-024 policy.
  REQUIRE(result.catalog->federateSupportSwitches().automaticResignAction == "NoAction");
}

TEST_CASE(
    "The FDD materializer is repeatable for a fixed official module set",
    "[unit][fom][composition][determinism]") {
  std::vector<PrevalidatedFomModule> modules{
      validated(resourcePath("mim/HLAstandardMIM-2025.xml"), FomModuleKind::mim, L"urn:umbra:test:mim"),
      validated(resourcePath("examples/RestaurantFOMmodule-2025.xml"), FomModuleKind::fom, L"urn:umbra:test:restaurant"),
  };

  auto materializer = composer();
  auto first = materializer.compose(modules);
  auto second = materializer.compose(modules);

  CAPTURE(first.diagnostics);
  CAPTURE(second.diagnostics);
  REQUIRE(first.status == FomCompositionStatus::valid);
  REQUIRE(second.status == FomCompositionStatus::valid);
  REQUIRE(first.catalog);
  REQUIRE(second.catalog);
  REQUIRE(first.fdd);
  REQUIRE(second.fdd);
  // This is a private reproducibility invariant for a fixed module order, not
  // an assertion that an arbitrary source FOM has a canonical XML form.
  REQUIRE(first.fdd->xmlUtf8() == second.fdd->xmlUtf8());
  REQUIRE(first.fdd->composedFromModuleNames() == second.fdd->composedFromModuleNames());
  REQUIRE(first.catalog->modules().size() == second.catalog->modules().size());
  REQUIRE(first.catalog->objectClass("HLAobjectRoot.Employee") != nullptr);
  REQUIRE(second.catalog->objectClass("HLAobjectRoot.Employee") != nullptr);
  REQUIRE(first.catalog->interactionClass("HLAinteractionRoot.ServerAction.TakeOrder") != nullptr);
  REQUIRE(second.catalog->interactionClass("HLAinteractionRoot.ServerAction.TakeOrder") != nullptr);
}

TEST_CASE(
    "The FDD catalog retains an enabled Non-Regulated-Grant switch",
    "[unit][fom][composition][time-management]") {
  auto nrgEnabled = nrgEnabledRestaurantModule();
  std::vector<PrevalidatedFomModule> modules{
      validated(resourcePath("mim/HLAstandardMIM-2025.xml"), FomModuleKind::mim, L"urn:umbra:test:mim"),
      validated(nrgEnabled.path(), FomModuleKind::fom, L"urn:umbra:test:nrg-enabled"),
  };

  auto result = composer().compose(modules);
  CAPTURE(result.diagnostics);
  REQUIRE(result.status == FomCompositionStatus::valid);
  REQUIRE(result.catalog);
  REQUIRE(result.catalog->timeManagementSwitches().nonRegulatedGrant);
}

TEST_CASE(
    "The FOM composition preflight retains the first duplicate switch and reports Annex C.8 warnings",
    "[unit][fom][composition][switches][annex-c]") {
  auto const testData = std::filesystem::path(UMBRA_SOURCE_DIRECTORY) / "cpp" / "tests" / "data";
  auto mim = validated(
      resourcePath("mim/HLAstandardMIM-2025.xml"),
      FomModuleKind::mim,
      L"urn:umbra:test:mim");
  auto disabled = validated(
      testData / "switch-nrg-disabled-fom.xml",
      FomModuleKind::fom,
      L"urn:umbra:test:nrg-disabled");
  auto enabled = validated(
      testData / "switch-nrg-enabled-fom.xml",
      FomModuleKind::fom,
      L"urn:umbra:test:nrg-enabled");
  auto knownClassEnabled = validated(
      testData / "switch-known-class-enabled-fom.xml",
      FomModuleKind::fom,
      L"urn:umbra:test:known-class-enabled");

  auto materializer = composer();
  auto equivalent = materializer.compose({mim, disabled, disabled});
  CAPTURE(equivalent.diagnostics, equivalent.warnings.size());
  REQUIRE(equivalent.status == FomCompositionStatus::valid);
  REQUIRE(equivalent.catalog);
  REQUIRE_FALSE(equivalent.catalog->timeManagementSwitches().nonRegulatedGrant);
  // The test FOM specifies only Non-Regulated-Grant; all omitted advisory
  // entries therefore use the IEEE 1516.2 Disabled default.
  REQUIRE_FALSE(equivalent.catalog->advisorySwitches().attributeScopeAdvisory);
  REQUIRE_FALSE(equivalent.catalog->advisorySwitches().attributeRelevanceAdvisory);
  REQUIRE_FALSE(equivalent.catalog->advisorySwitches().objectClassRelevanceAdvisory);
  REQUIRE_FALSE(equivalent.catalog->advisorySwitches().interactionRelevanceAdvisory);
  REQUIRE_FALSE(equivalent.catalog->advisorySwitches().advisoriesUseKnownClass);
  REQUIRE_FALSE(equivalent.catalog->federationSwitches().autoProvide);
  REQUIRE(equivalent.warnings.empty());

  auto knownClassFirst = materializer.compose({mim, knownClassEnabled});
  CAPTURE(knownClassFirst.diagnostics, knownClassFirst.warnings.size());
  REQUIRE(knownClassFirst.status == FomCompositionStatus::valid);
  REQUIRE(knownClassFirst.catalog);
  REQUIRE(knownClassFirst.catalog->advisorySwitches().advisoriesUseKnownClass);

  auto disabledFirst = materializer.compose({mim, disabled, enabled});
  CAPTURE(disabledFirst.diagnostics, disabledFirst.warnings.size());
  REQUIRE(disabledFirst.status == FomCompositionStatus::valid);
  REQUIRE(disabledFirst.catalog);
  REQUIRE(disabledFirst.fdd);
  REQUIRE_FALSE(disabledFirst.catalog->timeManagementSwitches().nonRegulatedGrant);
  REQUIRE(disabledFirst.catalog->federationSwitches().autoProvide);
  REQUIRE(disabledFirst.fdd->xmlUtf8().find("<autoProvide isEnabled=\"true\"") != std::string::npos);
  REQUIRE(disabledFirst.warnings.size() == 1);
  REQUIRE(disabledFirst.warnings.front().find("nonRegulatedGrant") != std::string::npos);
  REQUIRE(disabledFirst.warnings.front().find("first module") != std::string::npos);

  auto enabledFirst = materializer.compose({mim, enabled, disabled});
  CAPTURE(enabledFirst.diagnostics, enabledFirst.warnings.size());
  REQUIRE(enabledFirst.status == FomCompositionStatus::valid);
  REQUIRE(enabledFirst.catalog);
  REQUIRE(enabledFirst.catalog->timeManagementSwitches().nonRegulatedGrant);
  REQUIRE(enabledFirst.catalog->federationSwitches().autoProvide);
  REQUIRE(enabledFirst.warnings == disabledFirst.warnings);
}

TEST_CASE(
    "The FOM composition treats omitted switch booleans as their schema default",
    "[unit][fom][composition][switches][annex-c][switch-defaults]") {
  auto const testData = std::filesystem::path(UMBRA_SOURCE_DIRECTORY) / "cpp" / "tests" / "data";
  auto mim = validated(
      resourcePath("mim/HLAstandardMIM-2025.xml"),
      FomModuleKind::mim,
      L"urn:umbra:test:mim");
  auto omitted = validated(
      testData / "switch-nrg-omitted-fom.xml",
      FomModuleKind::fom,
      L"urn:umbra:test:nrg-omitted");
  auto explicitFalse = validated(
      testData / "switch-nrg-disabled-fom.xml",
      FomModuleKind::fom,
      L"urn:umbra:test:nrg-explicit-false");

  auto materializer = composer();
  auto omittedFirst = materializer.compose({mim, omitted, explicitFalse});
  auto explicitFalseFirst = materializer.compose({mim, explicitFalse, omitted});
  CAPTURE(
      omittedFirst.diagnostics,
      omittedFirst.warnings,
      explicitFalseFirst.diagnostics,
      explicitFalseFirst.warnings);

  REQUIRE(omittedFirst.status == FomCompositionStatus::valid);
  REQUIRE(explicitFalseFirst.status == FomCompositionStatus::valid);
  REQUIRE(omittedFirst.catalog);
  REQUIRE(explicitFalseFirst.catalog);
  REQUIRE_FALSE(omittedFirst.catalog->timeManagementSwitches().nonRegulatedGrant);
  REQUIRE_FALSE(explicitFalseFirst.catalog->timeManagementSwitches().nonRegulatedGrant);
  // `switchType/@isEnabled` defaults to false in the official 2025 schema.
  // Reordering equivalent module spellings must not create an Annex C.8
  // warning or make the first spelling observable in the catalog.
  REQUIRE(omittedFirst.warnings.empty());
  REQUIRE(explicitFalseFirst.warnings.empty());
}





TEST_CASE(
    "The FOM composition preflight requires basic representations for enumerated data types",
    "[unit][fom][composition][enumerated-representation]") {
  auto const testData = std::filesystem::path(UMBRA_SOURCE_DIRECTORY) / "cpp" / "tests" / "data";
  auto mim = validated(
      resourcePath("mim/HLAstandardMIM-2025.xml"),
      FomModuleKind::mim,
      L"urn:umbra:test:mim");
  auto nonBasicRepresentation = validated(
      testData / "enumerated-nonbasic-representation-fom.xml",
      FomModuleKind::fom,
      L"urn:umbra:test:enumerated-nonbasic-representation");
  auto omittedRepresentation = validated(
      testData / "enumerated-omitted-representation-fom.xml",
      FomModuleKind::fom,
      L"urn:umbra:test:enumerated-omitted-representation");

  auto materializer = composer();
  auto invalidResult = materializer.compose({mim, nonBasicRepresentation});
  REQUIRE(invalidResult.status == FomCompositionStatus::inconsistent_modules);
  REQUIRE(invalidResult.diagnostics.find("UmbraNonBasicEnumeratedRepresentation") !=
          std::string::npos);
  REQUIRE(invalidResult.diagnostics.find("HLAunicodeString") != std::string::npos);
  REQUIRE(invalidResult.diagnostics.find("basic-data representation") != std::string::npos);

  auto incompleteResult = materializer.compose({mim, omittedRepresentation});
  CAPTURE(incompleteResult.diagnostics);
  REQUIRE(incompleteResult.status == FomCompositionStatus::valid);
  REQUIRE(incompleteResult.catalog);
  REQUIRE(
      incompleteResult.catalog->dataType("UmbraOmittedEnumeratedRepresentation") != nullptr);

  auto standardResult = materializer.compose({
      mim,
      validated(
          resourcePath("examples/RestaurantFOMmodule-2025.xml"),
          FomModuleKind::fom,
          L"urn:umbra:test:restaurant"),
  });
  CAPTURE(standardResult.diagnostics);
  REQUIRE(standardResult.status == FomCompositionStatus::valid);
  REQUIRE(standardResult.catalog);
  REQUIRE(
      standardResult.catalog->dataType("HLAboolean")->kind ==
      umbra::detail::FomDataTypeKind::enumerated);
}

TEST_CASE(
    "The FOM composition preflight requires table-defined data types for array elements",
    "[unit][fom][composition][array-element-data-type]") {
  auto const testData = std::filesystem::path(UMBRA_SOURCE_DIRECTORY) / "cpp" / "tests" / "data";
  auto mim = validated(
      resourcePath("mim/HLAstandardMIM-2025.xml"),
      FomModuleKind::mim,
      L"urn:umbra:test:mim");
  auto rawBasicElementType = validated(
      testData / "basic-data-type-reference-fom.xml",
      FomModuleKind::fom,
      L"urn:umbra:test:array-basic-element-type");
  auto omittedElementType = validated(
      testData / "array-omitted-element-type-fom.xml",
      FomModuleKind::fom,
      L"urn:umbra:test:array-omitted-element-type");

  auto materializer = composer();
  auto rawBasicResult = materializer.compose({mim, rawBasicElementType});
  REQUIRE(rawBasicResult.status == FomCompositionStatus::inconsistent_modules);
  REQUIRE(rawBasicResult.diagnostics.find("UmbraBasicReferenceFixtureArray") != std::string::npos);
  REQUIRE(rawBasicResult.diagnostics.find("HLAinteger32BE") != std::string::npos);
  REQUIRE(rawBasicResult.diagnostics.find("not permitted") != std::string::npos);
  // The basic-data name resolves through the schema's complete data-type key;
  // the failure is Table 35's narrower Element Type category predicate.
  REQUIRE(rawBasicResult.diagnostics.find("not declared") == std::string::npos);

  auto incompleteResult = materializer.compose({mim, omittedElementType});
  CAPTURE(incompleteResult.diagnostics);
  REQUIRE(incompleteResult.status == FomCompositionStatus::valid);
  REQUIRE(incompleteResult.catalog);
  REQUIRE(incompleteResult.catalog->dataType("UmbraOmittedArrayElementType") != nullptr);

  auto standardResult = materializer.compose({
      mim,
      validated(
          resourcePath("examples/RestaurantFOMmodule-2025.xml"),
          FomModuleKind::fom,
          L"urn:umbra:test:restaurant"),
  });
  CAPTURE(standardResult.diagnostics);
  REQUIRE(standardResult.status == FomCompositionStatus::valid);
  REQUIRE(standardResult.catalog);
  REQUIRE(standardResult.catalog->dataType("Employees") != nullptr);
  REQUIRE(standardResult.catalog->dataType("AddressBook") != nullptr);
}

TEST_CASE(
    "The FOM composition preflight requires table-defined data types for fixed-record fields and variant alternatives",
    "[unit][fom][composition][record-member-data-type]") {
  auto const testData = std::filesystem::path(UMBRA_SOURCE_DIRECTORY) / "cpp" / "tests" / "data";
  auto mim = validated(
      resourcePath("mim/HLAstandardMIM-2025.xml"),
      FomModuleKind::mim,
      L"urn:umbra:test:mim");
  auto rawBasicFixedField = validated(
      testData / "fixed-record-field-basic-data-type-fom.xml",
      FomModuleKind::fom,
      L"urn:umbra:test:fixed-record-basic-field");
  auto rawBasicVariantAlternative = validated(
      testData / "variant-record-alternative-basic-data-type-fom.xml",
      FomModuleKind::fom,
      L"urn:umbra:test:variant-record-basic-alternative");
  auto omittedMemberTypes = validated(
      testData / "record-member-omitted-data-type-fom.xml",
      FomModuleKind::fom,
      L"urn:umbra:test:record-member-omitted-data-type");

  auto materializer = composer();
  auto fixedFieldResult = materializer.compose({mim, rawBasicFixedField});
  REQUIRE(fixedFieldResult.status == FomCompositionStatus::inconsistent_modules);
  REQUIRE(fixedFieldResult.diagnostics.find("UmbraRawBasicFixedRecordField") !=
          std::string::npos);
  REQUIRE(fixedFieldResult.diagnostics.find("UmbraRawBasicField") != std::string::npos);
  REQUIRE(fixedFieldResult.diagnostics.find("HLAinteger32BE") != std::string::npos);
  REQUIRE(fixedFieldResult.diagnostics.find("not permitted") != std::string::npos);

  auto variantAlternativeResult = materializer.compose({mim, rawBasicVariantAlternative});
  REQUIRE(variantAlternativeResult.status == FomCompositionStatus::inconsistent_modules);
  REQUIRE(variantAlternativeResult.diagnostics.find("UmbraRawBasicVariantAlternative") !=
          std::string::npos);
  REQUIRE(variantAlternativeResult.diagnostics.find("UmbraRawBasicAlternative") !=
          std::string::npos);
  REQUIRE(variantAlternativeResult.diagnostics.find("HLAinteger32BE") != std::string::npos);
  REQUIRE(variantAlternativeResult.diagnostics.find("not permitted") != std::string::npos);

  auto incompleteResult = materializer.compose({mim, omittedMemberTypes});
  CAPTURE(incompleteResult.diagnostics);
  REQUIRE(incompleteResult.status == FomCompositionStatus::valid);
  REQUIRE(incompleteResult.catalog);
  REQUIRE(incompleteResult.catalog->dataType("UmbraOmittedFixedRecordFieldType") != nullptr);
  REQUIRE(incompleteResult.catalog->dataType("UmbraOmittedVariantAlternativeType") != nullptr);

  auto standardResult = materializer.compose({
      mim,
      validated(
          resourcePath("examples/RestaurantFOMmodule-2025.xml"),
          FomModuleKind::fom,
          L"urn:umbra:test:restaurant"),
  });
  CAPTURE(standardResult.diagnostics);
  REQUIRE(standardResult.status == FomCompositionStatus::valid);
  REQUIRE(standardResult.catalog);
  REQUIRE(standardResult.catalog->dataType("ServiceStat") != nullptr);
  REQUIRE(standardResult.catalog->dataType("ServerValue") != nullptr);
}

TEST_CASE(
    "The FOM composition preflight requires enumerated data types for variant-record discriminants",
    "[unit][fom][composition][variant-discriminant-data-type]") {
  auto const testData = std::filesystem::path(UMBRA_SOURCE_DIRECTORY) / "cpp" / "tests" / "data";
  auto mim = validated(
      resourcePath("mim/HLAstandardMIM-2025.xml"),
      FomModuleKind::mim,
      L"urn:umbra:test:mim");
  auto rawBasicDiscriminant = validated(
      testData / "variant-record-discriminant-basic-data-type-fom.xml",
      FomModuleKind::fom,
      L"urn:umbra:test:variant-discriminant-basic");
  auto simpleDiscriminant = validated(
      testData / "variant-record-discriminant-simple-data-type-fom.xml",
      FomModuleKind::fom,
      L"urn:umbra:test:variant-discriminant-simple");
  auto naDiscriminant = validated(
      testData / "variant-record-discriminant-na-data-type-fom.xml",
      FomModuleKind::fom,
      L"urn:umbra:test:variant-discriminant-na");
  auto omittedDiscriminant = validated(
      testData / "variant-record-omitted-discriminant-data-type-fom.xml",
      FomModuleKind::fom,
      L"urn:umbra:test:variant-discriminant-omitted");

  auto materializer = composer();
  auto rawBasicResult = materializer.compose({mim, rawBasicDiscriminant});
  REQUIRE(rawBasicResult.status == FomCompositionStatus::inconsistent_modules);
  REQUIRE(rawBasicResult.diagnostics.find("UmbraRawBasicVariantDiscriminant") !=
          std::string::npos);
  REQUIRE(rawBasicResult.diagnostics.find("HLAinteger32BE") != std::string::npos);
  REQUIRE(rawBasicResult.diagnostics.find("enumerated data type") != std::string::npos);

  auto simpleResult = materializer.compose({mim, simpleDiscriminant});
  REQUIRE(simpleResult.status == FomCompositionStatus::inconsistent_modules);
  REQUIRE(simpleResult.diagnostics.find("UmbraSimpleVariantDiscriminant") != std::string::npos);
  REQUIRE(simpleResult.diagnostics.find("HLAunicodeString") != std::string::npos);
  REQUIRE(simpleResult.diagnostics.find("enumerated data type") != std::string::npos);

  auto naResult = materializer.compose({mim, naDiscriminant});
  REQUIRE(naResult.status == FomCompositionStatus::inconsistent_modules);
  REQUIRE(naResult.diagnostics.find("UmbraNaVariantDiscriminant") != std::string::npos);
  REQUIRE(naResult.diagnostics.find("NA") != std::string::npos);
  REQUIRE(naResult.diagnostics.find("enumerated data type") != std::string::npos);

  auto incompleteResult = materializer.compose({mim, omittedDiscriminant});
  CAPTURE(incompleteResult.diagnostics);
  REQUIRE(incompleteResult.status == FomCompositionStatus::valid);
  REQUIRE(incompleteResult.catalog);
  REQUIRE(incompleteResult.catalog->dataType("UmbraOmittedVariantDiscriminant") != nullptr);

  auto standardResult = materializer.compose({
      mim,
      validated(
          resourcePath("examples/RestaurantFOMmodule-2025.xml"),
          FomModuleKind::fom,
          L"urn:umbra:test:restaurant"),
  });
  CAPTURE(standardResult.diagnostics);
  REQUIRE(standardResult.status == FomCompositionStatus::valid);
  REQUIRE(standardResult.catalog);
  REQUIRE(standardResult.catalog->dataType("ServerValue") != nullptr);
  REQUIRE(standardResult.catalog->dataType("DrinkGarnish") != nullptr);
}

TEST_CASE(
    "The FOM composition preflight validates variant-record discriminant-enumerator syntax",
    "[unit][fom][composition][variant-discriminant-enumerator]") {
  auto const testData = std::filesystem::path(UMBRA_SOURCE_DIRECTORY) / "cpp" / "tests" / "data";
  auto mim = validated(
      resourcePath("mim/HLAstandardMIM-2025.xml"),
      FomModuleKind::mim,
      L"urn:umbra:test:mim");
  auto malformedRange = validated(
      testData / "variant-record-malformed-discriminant-range-fom.xml",
      FomModuleKind::fom,
      L"urn:umbra:test:variant-discriminant-malformed-range");
  auto compositeHlaOther = validated(
      testData / "variant-record-composite-hlaother-fom.xml",
      FomModuleKind::fom,
      L"urn:umbra:test:variant-discriminant-composite-hlaother");
  auto omittedEnumerator = validated(
      testData / "variant-record-omitted-discriminant-enumerator-fom.xml",
      FomModuleKind::fom,
      L"urn:umbra:test:variant-discriminant-omitted-enumerator");

  auto materializer = composer();
  auto malformedRangeResult = materializer.compose({mim, malformedRange});
  REQUIRE(malformedRangeResult.status == FomCompositionStatus::inconsistent_modules);
  REQUIRE(malformedRangeResult.diagnostics.find("UmbraMalformedVariantDiscriminantRange") !=
          std::string::npos);
  REQUIRE(malformedRangeResult.diagnostics.find("malformed discriminant-enumerator range") !=
          std::string::npos);

  auto compositeHlaOtherResult = materializer.compose({mim, compositeHlaOther});
  REQUIRE(compositeHlaOtherResult.status == FomCompositionStatus::inconsistent_modules);
  REQUIRE(compositeHlaOtherResult.diagnostics.find("UmbraCompositeHlaOtherVariant") !=
          std::string::npos);
  REQUIRE(compositeHlaOtherResult.diagnostics.find("HLAother") != std::string::npos);
  REQUIRE(compositeHlaOtherResult.diagnostics.find("complete discriminant-enumerator") !=
          std::string::npos);

  auto incompleteResult = materializer.compose({mim, omittedEnumerator});
  CAPTURE(incompleteResult.diagnostics);
  REQUIRE(incompleteResult.status == FomCompositionStatus::valid);
  REQUIRE(incompleteResult.catalog);
  REQUIRE(incompleteResult.catalog->dataType("UmbraOmittedVariantEnumerator") != nullptr);

  auto standardResult = materializer.compose({
      mim,
      validated(
          resourcePath("examples/RestaurantFOMmodule-2025.xml"),
          FomModuleKind::fom,
          L"urn:umbra:test:restaurant"),
  });
  CAPTURE(standardResult.diagnostics);
  REQUIRE(standardResult.status == FomCompositionStatus::valid);
  REQUIRE(standardResult.catalog);
  REQUIRE(standardResult.catalog->dataType("ServerValue") != nullptr);
  REQUIRE(standardResult.catalog->dataType("DrinkGarnish") != nullptr);
}

TEST_CASE(
    "The FOM composition preflight requires declared members for variant-record discriminants",
    "[unit][fom][composition][variant-discriminant-enumerator-membership]") {
  auto const testData = std::filesystem::path(UMBRA_SOURCE_DIRECTORY) / "cpp" / "tests" / "data";
  auto mim = validated(
      resourcePath("mim/HLAstandardMIM-2025.xml"),
      FomModuleKind::mim,
      L"urn:umbra:test:mim");
  auto unknownMember = validated(
      testData / "variant-record-unknown-discriminant-enumerator-fom.xml",
      FomModuleKind::fom,
      L"urn:umbra:test:variant-discriminant-unknown-member");
  auto unknownRangeEndpoint = validated(
      testData / "variant-record-unknown-discriminant-range-endpoint-fom.xml",
      FomModuleKind::fom,
      L"urn:umbra:test:variant-discriminant-unknown-range-endpoint");
  auto incompleteEnumeration = validated(
      testData / "variant-record-incomplete-discriminant-enumeration-fom.xml",
      FomModuleKind::fom,
      L"urn:umbra:test:variant-discriminant-incomplete-enumeration");
  auto laterConsumer = validated(
      testData / "variant-record-discriminant-enumerator-later-consumer-fom.xml",
      FomModuleKind::fom,
      L"urn:umbra:test:variant-discriminant-later-consumer");
  auto laterProvider = validated(
      testData / "variant-record-discriminant-enumerator-later-provider-fom.xml",
      FomModuleKind::fom,
      L"urn:umbra:test:variant-discriminant-later-provider");

  auto materializer = composer();
  auto unknownMemberResult = materializer.compose({mim, unknownMember});
  REQUIRE(unknownMemberResult.status == FomCompositionStatus::inconsistent_modules);
  REQUIRE(unknownMemberResult.diagnostics.find("UmbraUnknownVariantMember") != std::string::npos);
  REQUIRE(unknownMemberResult.diagnostics.find("UmbraMissingChoice") != std::string::npos);
  REQUIRE(unknownMemberResult.diagnostics.find("UmbraKnownVariantChoices") != std::string::npos);
  REQUIRE(unknownMemberResult.diagnostics.find("not declared") != std::string::npos);

  auto unknownRangeEndpointResult = materializer.compose({mim, unknownRangeEndpoint});
  REQUIRE(unknownRangeEndpointResult.status == FomCompositionStatus::inconsistent_modules);
  REQUIRE(
      unknownRangeEndpointResult.diagnostics.find("UmbraUnknownVariantRangeEndpoint") !=
      std::string::npos);
  REQUIRE(unknownRangeEndpointResult.diagnostics.find("UmbraMissingRangeChoice") !=
          std::string::npos);
  REQUIRE(unknownRangeEndpointResult.diagnostics.find("UmbraKnownRangeChoices") !=
          std::string::npos);
  REQUIRE(unknownRangeEndpointResult.diagnostics.find("not declared") != std::string::npos);

  auto incompleteResult = materializer.compose({mim, incompleteEnumeration});
  CAPTURE(incompleteResult.diagnostics);
  REQUIRE(incompleteResult.status == FomCompositionStatus::valid);
  REQUIRE(incompleteResult.catalog);
  REQUIRE(incompleteResult.catalog->dataType("UmbraIncompleteVariantEnumeration") != nullptr);

  auto laterResult = materializer.compose({mim, laterConsumer, laterProvider});
  CAPTURE(laterResult.diagnostics);
  REQUIRE(laterResult.status == FomCompositionStatus::valid);
  REQUIRE(laterResult.catalog);
  REQUIRE(laterResult.catalog->dataType("UmbraLaterVariantChoice") != nullptr);

  auto standardResult = materializer.compose({
      mim,
      validated(
          resourcePath("examples/RestaurantFOMmodule-2025.xml"),
          FomModuleKind::fom,
          L"urn:umbra:test:restaurant"),
  });
  CAPTURE(standardResult.diagnostics);
  REQUIRE(standardResult.status == FomCompositionStatus::valid);
  REQUIRE(standardResult.catalog);
  REQUIRE(standardResult.catalog->dataType("ServerValue") != nullptr);
  REQUIRE(standardResult.catalog->dataType("DrinkGarnish") != nullptr);
}

TEST_CASE(
    "The FOM composition preflight expands variant-record ranges in enumerator-table order",
    "[unit][fom][composition][variant-discriminant-enumerator-range-semantics]") {
  auto const testData = std::filesystem::path(UMBRA_SOURCE_DIRECTORY) / "cpp" / "tests" / "data";
  auto mim = validated(
      resourcePath("mim/HLAstandardMIM-2025.xml"),
      FomModuleKind::mim,
      L"urn:umbra:test:mim");
  auto declarationOrderBase = validated(
      testData / "variant-record-declaration-order-merged-range-base-fom.xml",
      FomModuleKind::fom,
      L"urn:umbra:test:variant-discriminant-declaration-order-base");
  auto declarationOrderExtension = validated(
      testData / "variant-record-declaration-order-merged-range-extension-fom.xml",
      FomModuleKind::fom,
      L"urn:umbra:test:variant-discriminant-declaration-order-extension");
  auto overlappingRangesBase = validated(
      testData / "variant-record-overlapping-ranges-base-fom.xml",
      FomModuleKind::fom,
      L"urn:umbra:test:variant-discriminant-overlap-base");
  auto overlappingRangesExtension = validated(
      testData / "variant-record-overlapping-ranges-extension-fom.xml",
      FomModuleKind::fom,
      L"urn:umbra:test:variant-discriminant-overlap-extension");

  auto materializer = composer();
  auto declarationOrderResult = materializer.compose({
      mim,
      declarationOrderBase,
      declarationOrderExtension,
  });
  CAPTURE(declarationOrderResult.diagnostics);
  REQUIRE(declarationOrderResult.status == FomCompositionStatus::inconsistent_modules);
  REQUIRE(declarationOrderResult.diagnostics.find("UmbraDeclarationOrderMergedRange") !=
          std::string::npos);
  REQUIRE(declarationOrderResult.diagnostics.find("Omega") != std::string::npos);
  REQUIRE(declarationOrderResult.diagnostics.find("UmbraMergedRangeAlternative") !=
          std::string::npos);
  REQUIRE(declarationOrderResult.diagnostics.find("UmbraConflictingOmegaAlternative") !=
          std::string::npos);
  REQUIRE(declarationOrderResult.diagnostics.find("after expanding") != std::string::npos);

  auto overlappingRangesResult = materializer.compose({
      mim,
      overlappingRangesBase,
      overlappingRangesExtension,
  });
  REQUIRE(overlappingRangesResult.status == FomCompositionStatus::inconsistent_modules);
  REQUIRE(overlappingRangesResult.diagnostics.find("UmbraOverlappingRanges") !=
          std::string::npos);
  REQUIRE(overlappingRangesResult.diagnostics.find("UmbraTwo") != std::string::npos);
  REQUIRE(overlappingRangesResult.diagnostics.find("UmbraFirstRangeAlternative") !=
          std::string::npos);
  REQUIRE(overlappingRangesResult.diagnostics.find("UmbraSecondRangeAlternative") !=
          std::string::npos);

  // The official non-extendable Restaurant record provides the positive
  // HLAother complement vector alongside explicitly assigned alternatives.
  auto standardResult = materializer.compose({
      mim,
      validated(
          resourcePath("examples/RestaurantFOMmodule-2025.xml"),
          FomModuleKind::fom,
          L"urn:umbra:test:restaurant"),
  });
  CAPTURE(standardResult.diagnostics);
  REQUIRE(standardResult.status == FomCompositionStatus::valid);
  REQUIRE(standardResult.catalog);
  REQUIRE(standardResult.catalog->dataType("ServerValue") != nullptr);
}




TEST_CASE(
    "The FOM composition preflight requires table-defined data types for dimension inputs",
    "[unit][fom][composition][dimension-input-data-type]") {
  auto const testData = std::filesystem::path(UMBRA_SOURCE_DIRECTORY) / "cpp" / "tests" / "data";
  auto mim = validated(
      resourcePath("mim/HLAstandardMIM-2025.xml"),
      FomModuleKind::mim,
      L"urn:umbra:test:mim");
  auto rawBasicInput = validated(
      testData / "dimension-input-basic-data-type-fom.xml",
      FomModuleKind::fom,
      L"urn:umbra:test:dimension-basic-input");
  auto naInput = validated(
      testData / "dimension-na-input-data-type-fom.xml",
      FomModuleKind::fom,
      L"urn:umbra:test:dimension-na-input");

  auto materializer = composer();
  auto rawBasicResult = materializer.compose({mim, rawBasicInput});
  REQUIRE(rawBasicResult.status == FomCompositionStatus::inconsistent_modules);
  REQUIRE(rawBasicResult.diagnostics.find("UmbraRawBasicDimensionInput") != std::string::npos);
  REQUIRE(rawBasicResult.diagnostics.find("HLAinteger32BE") != std::string::npos);
  REQUIRE(rawBasicResult.diagnostics.find("not permitted") != std::string::npos);
  REQUIRE(rawBasicResult.diagnostics.find("not declared") == std::string::npos);

  auto naResult = materializer.compose({mim, naInput});
  CAPTURE(naResult.diagnostics);
  REQUIRE(naResult.status == FomCompositionStatus::valid);
  REQUIRE(naResult.catalog);
  REQUIRE(naResult.catalog->dimension("UmbraNoTypeDimensionInput") != nullptr);

  auto standardResult = materializer.compose({
      mim,
      validated(
          resourcePath("examples/RestaurantFOMmodule-2025.xml"),
          FomModuleKind::fom,
          L"urn:umbra:test:restaurant"),
  });
  CAPTURE(standardResult.diagnostics);
  REQUIRE(standardResult.status == FomCompositionStatus::valid);
  REQUIRE(standardResult.catalog);
  REQUIRE(standardResult.catalog->dimension("SodaFlavor") != nullptr);
  REQUIRE(standardResult.catalog->dimension("Gluten") != nullptr);
}

TEST_CASE(
    "The FOM composition preflight requires descriptions for dimensions without named input types",
    "[unit][fom][composition][dimension-input-data-description]") {
  auto const testData = std::filesystem::path(UMBRA_SOURCE_DIRECTORY) / "cpp" / "tests" / "data";
  auto mim = validated(
      resourcePath("mim/HLAstandardMIM-2025.xml"),
      FomModuleKind::mim,
      L"urn:umbra:test:mim");
  auto missingDescription = validated(
      testData / "dimension-unspecified-input-description-na-fom.xml",
      FomModuleKind::fom,
      L"urn:umbra:test:dimension-unspecified-input-description-na");
  auto describedInput = validated(
      testData / "dimension-unspecified-input-description-fom.xml",
      FomModuleKind::fom,
      L"urn:umbra:test:dimension-unspecified-input-description");

  auto materializer = composer();
  auto missingDescriptionResult = materializer.compose({mim, missingDescription});
  REQUIRE(missingDescriptionResult.status == FomCompositionStatus::inconsistent_modules);
  REQUIRE(missingDescriptionResult.diagnostics.find("UmbraMissingInputDescription") !=
          std::string::npos);
  REQUIRE(missingDescriptionResult.diagnostics.find("no named input data type") !=
          std::string::npos);
  REQUIRE(missingDescriptionResult.diagnostics.find("non-NA text") != std::string::npos);

  auto describedInputResult = materializer.compose({mim, describedInput});
  CAPTURE(describedInputResult.diagnostics);
  REQUIRE(describedInputResult.status == FomCompositionStatus::valid);
  REQUIRE(describedInputResult.catalog);
  REQUIRE(describedInputResult.catalog->dimension("UmbraDescribedInputWithoutNamedType") != nullptr);

  // The official Restaurant FOM supplies both named inputs with NA
  // descriptions and named inputs with amplifying text; both remain valid.
  auto standardResult = materializer.compose({
      mim,
      validated(
          resourcePath("examples/RestaurantFOMmodule-2025.xml"),
          FomModuleKind::fom,
          L"urn:umbra:test:restaurant"),
  });
  CAPTURE(standardResult.diagnostics);
  REQUIRE(standardResult.status == FomCompositionStatus::valid);
  REQUIRE(standardResult.catalog);
  REQUIRE(standardResult.catalog->dimension("SodaFlavor") != nullptr);
  REQUIRE(standardResult.catalog->dimension("Gluten") != nullptr);
}

TEST_CASE(
    "The FOM composition preflight keeps the NA dimension input marker exclusive",
    "[unit][fom][composition][dimension-input-data-type-na-exclusivity]") {
  auto const testData = std::filesystem::path(UMBRA_SOURCE_DIRECTORY) / "cpp" / "tests" / "data";
  auto mim = validated(
      resourcePath("mim/HLAstandardMIM-2025.xml"),
      FomModuleKind::mim,
      L"urn:umbra:test:mim");
  auto mixedInput = validated(
      testData / "dimension-mixed-na-input-data-types-fom.xml",
      FomModuleKind::fom,
      L"urn:umbra:test:dimension-mixed-na-input");

  auto materializer = composer();
  auto mixedInputResult = materializer.compose({mim, mixedInput});
  REQUIRE(mixedInputResult.status == FomCompositionStatus::inconsistent_modules);
  REQUIRE(mixedInputResult.diagnostics.find("UmbraMixedNaDimensionInput") != std::string::npos);
  REQUIRE(mixedInputResult.diagnostics.find("mixes the NA") != std::string::npos);
  REQUIRE(mixedInputResult.diagnostics.find("HLAcount") == std::string::npos);
}

TEST_CASE(
    "The FOM composition preflight rejects duplicate dimension names within one module",
    "[unit][fom][composition][dimension-names]") {
  auto const testData = std::filesystem::path(UMBRA_SOURCE_DIRECTORY) / "cpp" / "tests" / "data";
  auto mim = validated(
      resourcePath("mim/HLAstandardMIM-2025.xml"),
      FomModuleKind::mim,
      L"urn:umbra:test:mim");
  auto duplicateNames = validated(
      testData / "duplicate-dimension-name-fom.xml",
      FomModuleKind::fom,
      L"urn:umbra:test:duplicate-dimension-name");

  auto result = composer().compose({mim, duplicateNames});
  REQUIRE(result.status == FomCompositionStatus::inconsistent_modules);
  REQUIRE(result.diagnostics.find("UmbraDuplicateDimension") != std::string::npos);
  REQUIRE(result.diagnostics.find("dimension names must be unique") != std::string::npos);
}

TEST_CASE(
    "The FOM composition preflight enforces HLA 3.3.1 XML names",
    "[unit][fom][composition][fom-name-conventions]") {
  auto mim = validated(
      resourcePath("mim/HLAstandardMIM-2025.xml"),
      FomModuleKind::mim,
      L"urn:umbra:test:mim");

  for (auto const& [replacement, label, diagnostic] : {
           std::tuple{"Customer.Name", "qualified-period", "cannot contain a period"},
           std::tuple{"HLAUserCustomer", "reserved-hla", "reserved HLA prefix"},
           std::tuple{"NA", "reserved-na", "reserved for the NA marker"},
       }) {
    auto invalid = restaurantModuleWithNameReplacement("Customer", replacement, label);
    auto module = validated(
        invalid.path(),
        FomModuleKind::fom,
        L"urn:umbra:test:fom-name");
    auto result = composer().compose({mim, module});
    CAPTURE(replacement, label, result.diagnostics);
    REQUIRE(result.status == FomCompositionStatus::inconsistent_modules);
    REQUIRE(result.diagnostics.find(diagnostic) != std::string::npos);
  }

  // XML NCName permits the HLA-conventional hyphen and underscore characters
  // after the first character.  The same otherwise-valid Restaurant module
  // must continue through composition when those characters are used.
  auto valid = restaurantModuleWithNameReplacement(
      "Customer",
      "Customer-2_0",
      "allowed-punctuation");
  auto validModule = validated(
      valid.path(),
      FomModuleKind::fom,
      L"urn:umbra:test:fom-name-allowed-punctuation");
  auto validResult = composer().compose({mim, validModule});
  CAPTURE(validResult.diagnostics);
  REQUIRE(validResult.status == FomCompositionStatus::valid);
  REQUIRE(validResult.catalog);
  REQUIRE(validResult.catalog->objectClass("HLAobjectRoot.Customer-2_0") != nullptr);
}

TEST_CASE(
    "The FOM composition preflight enforces the YYYY-MM-DD modification-date form",
    "[unit][fom][composition][table-constraints][model-identification]") {
  auto mim = validated(
      resourcePath("mim/HLAstandardMIM-2025.xml"),
      FomModuleKind::mim,
      L"urn:umbra:test:mim");
  auto invalidDate = restaurantModuleWithTimezoneModificationDate();
  auto module = validated(
      invalidDate.path(),
      FomModuleKind::fom,
      L"urn:umbra:test:modification-date");

  auto result = composer().compose({mim, module});
  CAPTURE(result.diagnostics);
  REQUIRE(result.status == FomCompositionStatus::inconsistent_modules);
  REQUIRE(result.diagnostics.find("2025-02-10Z") != std::string::npos);
  REQUIRE(result.diagnostics.find("YYYY-MM-DD") != std::string::npos);

  auto valid = validated(
      resourcePath("examples/RestaurantFOMmodule-2025.xml"),
      FomModuleKind::fom,
      L"urn:umbra:test:valid-modification-date");
  auto validResult = composer().compose({mim, valid});
  CAPTURE(validResult.diagnostics);
  REQUIRE(validResult.status == FomCompositionStatus::valid);
  REQUIRE(validResult.fdd);
}

TEST_CASE(
    "The FOM composition preflight enforces positive update rates",
    "[unit][fom][composition][table-constraints]") {
  auto const testData = std::filesystem::path(UMBRA_SOURCE_DIRECTORY) / "cpp" / "tests" / "data";
  auto mim = validated(
      resourcePath("mim/HLAstandardMIM-2025.xml"),
      FomModuleKind::mim,
      L"urn:umbra:test:mim");
  auto invalidRate = validated(
      testData / "invalid-update-rate-fom.xml",
      FomModuleKind::fom,
      L"urn:umbra:test:invalid-update-rate");

  auto materializer = composer();
  auto rateResult = materializer.compose({mim, invalidRate});
  REQUIRE(rateResult.status == FomCompositionStatus::inconsistent_modules);
  REQUIRE(rateResult.diagnostics.find("UmbraZeroRate") != std::string::npos);
  REQUIRE(rateResult.diagnostics.find("greater than zero") != std::string::npos);
}

TEST_CASE(
    "The FOM composition preflight bounds dimension default ranges",
    "[unit][fom][composition][table-constraints]") {
  auto const testData = std::filesystem::path(UMBRA_SOURCE_DIRECTORY) / "cpp" / "tests" / "data";
  auto mim = validated(
      resourcePath("mim/HLAstandardMIM-2025.xml"),
      FomModuleKind::mim,
      L"urn:umbra:test:mim");
  auto invalidDefault = validated(
      testData / "invalid-dimension-default-value-fom.xml",
      FomModuleKind::fom,
      L"urn:umbra:test:invalid-dimension-default");

  auto result = composer().compose({mim, invalidDefault});
  REQUIRE(result.status == FomCompositionStatus::inconsistent_modules);
  REQUIRE(result.diagnostics.find("UmbraInvalidDefaultDimension") != std::string::npos);
  REQUIRE(result.diagnostics.find("nonnegative integer subrange") != std::string::npos);
}

TEST_CASE(
    "The FOM composition preflight validates array cardinality forms and encodings",
    "[unit][fom][composition][table-constraints]") {
  auto const testData = std::filesystem::path(UMBRA_SOURCE_DIRECTORY) / "cpp" / "tests" / "data";
  auto mim = validated(
      resourcePath("mim/HLAstandardMIM-2025.xml"),
      FomModuleKind::mim,
      L"urn:umbra:test:mim");
  auto invalidCardinality = validated(
      testData / "invalid-array-cardinality-fom.xml",
      FomModuleKind::fom,
      L"urn:umbra:test:invalid-array-cardinality");
  auto invalidFixedEncoding = validated(
      testData / "invalid-array-fixed-encoding-fom.xml",
      FomModuleKind::fom,
      L"urn:umbra:test:invalid-array-fixed-encoding");
  auto invalidVariableEncoding = validated(
      testData / "invalid-array-variable-encoding-fom.xml",
      FomModuleKind::fom,
      L"urn:umbra:test:invalid-array-variable-encoding");
  auto validCardinalityForms = validated(
      testData / "valid-array-cardinality-forms-fom.xml",
      FomModuleKind::fom,
      L"urn:umbra:test:valid-array-cardinality-forms");

  auto validResult = composer().compose({mim, validCardinalityForms});
  CAPTURE(validResult.diagnostics);
  REQUIRE(validResult.status == FomCompositionStatus::valid);
  REQUIRE(validResult.catalog);
  REQUIRE(validResult.catalog->dataType("UmbraScalarArrayCardinality") != nullptr);
  REQUIRE(validResult.catalog->dataType("UmbraListArrayCardinality") != nullptr);
  REQUIRE(validResult.catalog->dataType("UmbraRangeArrayCardinality") != nullptr);
  REQUIRE(validResult.catalog->dataType("UmbraDynamicArrayCardinality") != nullptr);
  REQUIRE(validResult.catalog->dataType("UmbraMixedArrayCardinality") != nullptr);

  auto result = composer().compose({mim, invalidCardinality});
  REQUIRE(result.status == FomCompositionStatus::inconsistent_modules);
  REQUIRE(result.diagnostics.find("UmbraInvalidArrayCardinality") != std::string::npos);
  REQUIRE(result.diagnostics.find("invalid cardinality") != std::string::npos);

  auto fixedEncodingResult = composer().compose({mim, invalidFixedEncoding});
  REQUIRE(fixedEncodingResult.status == FomCompositionStatus::inconsistent_modules);
  REQUIRE(fixedEncodingResult.diagnostics.find("UmbraInvalidFixedArrayEncoding") !=
          std::string::npos);
  REQUIRE(fixedEncodingResult.diagnostics.find("HLAfixedArray") != std::string::npos);

  auto variableEncodingResult = composer().compose({mim, invalidVariableEncoding});
  REQUIRE(variableEncodingResult.status == FomCompositionStatus::inconsistent_modules);
  REQUIRE(variableEncodingResult.diagnostics.find("UmbraInvalidVariableArrayEncoding") !=
          std::string::npos);
  REQUIRE(variableEncodingResult.diagnostics.find("HLAvariableArray") != std::string::npos);
}


TEST_CASE(
    "The FOM composition preflight validates time-representation data type categories",
    "[unit][fom][composition][time-representation]") {
  auto const testData = std::filesystem::path(UMBRA_SOURCE_DIRECTORY) / "cpp" / "tests" / "data";
  auto mim = validated(
      resourcePath("mim/HLAstandardMIM-2025.xml"),
      FomModuleKind::mim,
      L"urn:umbra:test:mim");
  auto allowed = validated(
      testData / "time-representation-allowed-fom.xml",
      FomModuleKind::fom,
      L"urn:umbra:test:time-representation-allowed");
  auto float64 = validated(
      testData / "time-representation-float64-fom.xml",
      FomModuleKind::fom,
      L"urn:umbra:test:time-representation-float64");
  auto basicData = validated(
      testData / "time-representation-basic-data-fom.xml",
      FomModuleKind::fom,
      L"urn:umbra:test:time-representation-basic-data");
  auto referenceData = validated(
      testData / "time-representation-reference-data-fom.xml",
      FomModuleKind::fom,
      L"urn:umbra:test:time-representation-reference-data");

  auto materializer = composer();
  auto allowedResult = materializer.compose({mim, allowed});
  CAPTURE(allowedResult.diagnostics);
  REQUIRE(allowedResult.status == FomCompositionStatus::valid);
  REQUIRE(allowedResult.catalog);
  REQUIRE(allowedResult.fdd);
  REQUIRE(allowedResult.catalog->time().logicalTimeDataType == "HLAinteger64Time");
  REQUIRE(allowedResult.catalog->time().logicalTimeIntervalDataType == "HLAinteger64Time");

  auto float64Result = materializer.compose({mim, float64});
  CAPTURE(float64Result.diagnostics);
  REQUIRE(float64Result.status == FomCompositionStatus::valid);
  REQUIRE(float64Result.catalog);
  REQUIRE(float64Result.fdd);
  REQUIRE(float64Result.catalog->time().logicalTimeDataType == "HLAfloat64Time");
  REQUIRE(float64Result.catalog->time().logicalTimeIntervalDataType == "HLAfloat64Time");

  auto basicDataResult = materializer.compose({mim, basicData});
  REQUIRE(basicDataResult.status == FomCompositionStatus::inconsistent_modules);
  REQUIRE(basicDataResult.diagnostics.find("HLAinteger32BE") != std::string::npos);
  REQUIRE(basicDataResult.diagnostics.find("not permitted") != std::string::npos);

  auto referenceDataResult = materializer.compose({mim, referenceData});
  REQUIRE(referenceDataResult.status == FomCompositionStatus::inconsistent_modules);
  REQUIRE(referenceDataResult.diagnostics.find("UmbraTimeReferenceFixture") != std::string::npos);
  REQUIRE(referenceDataResult.diagnostics.find("not permitted") != std::string::npos);
}

TEST_CASE(
    "The FOM composition preflight rejects raw basic data types for attributes and parameters",
    "[unit][fom][composition][attribute-parameter-data-type]") {
  auto const testData = std::filesystem::path(UMBRA_SOURCE_DIRECTORY) / "cpp" / "tests" / "data";
  auto mim = validated(
      resourcePath("mim/HLAstandardMIM-2025.xml"),
      FomModuleKind::mim,
      L"urn:umbra:test:mim");
  auto attributeBasicData = validated(
      testData / "attribute-basic-data-type-fom.xml",
      FomModuleKind::fom,
      L"urn:umbra:test:attribute-basic-data-type");
  auto parameterBasicData = validated(
      testData / "parameter-basic-data-type-fom.xml",
      FomModuleKind::fom,
      L"urn:umbra:test:parameter-basic-data-type");

  auto materializer = composer();
  auto attributeResult = materializer.compose({mim, attributeBasicData});
  REQUIRE(attributeResult.status == FomCompositionStatus::inconsistent_modules);
  REQUIRE(attributeResult.diagnostics.find("Object attribute data type \"HLAinteger32BE\"") !=
          std::string::npos);
  REQUIRE(attributeResult.diagnostics.find("not permitted") != std::string::npos);

  auto parameterResult = materializer.compose({mim, parameterBasicData});
  REQUIRE(parameterResult.status == FomCompositionStatus::inconsistent_modules);
  REQUIRE(parameterResult.diagnostics.find("Interaction parameter data type \"HLAinteger32BE\"") !=
          std::string::npos);
  REQUIRE(parameterResult.diagnostics.find("not permitted") != std::string::npos);

  // The standard MIM uses the predefined HLAtoken array type for the root
  // delete privilege. The generic category rule must retain that valid case.
  auto standardResult = materializer.compose({
      mim,
      validated(
          resourcePath("examples/RestaurantFOMmodule-2025.xml"),
          FomModuleKind::fom,
          L"urn:umbra:test:restaurant"),
  });
  CAPTURE(standardResult.diagnostics);
  REQUIRE(standardResult.status == FomCompositionStatus::valid);
  REQUIRE(standardResult.catalog);
  REQUIRE(standardResult.catalog->dataType("HLAtoken") != nullptr);
  REQUIRE(standardResult.catalog->dataType("HLAtoken")->kind == umbra::detail::FomDataTypeKind::array);
}

TEST_CASE(
    "The FOM composition preflight validates supplied NA attribute companions",
    "[unit][fom][composition][attribute-na-companion-fields]") {
  auto const testData = std::filesystem::path(UMBRA_SOURCE_DIRECTORY) / "cpp" / "tests" / "data";
  auto mim = validated(
      resourcePath("mim/HLAstandardMIM-2025.xml"),
      FomModuleKind::mim,
      L"urn:umbra:test:mim");
  auto invalidUpdateType = validated(
      testData / "attribute-na-invalid-update-type-fom.xml",
      FomModuleKind::fom,
      L"urn:umbra:test:attribute-na-invalid-update-type");
  auto invalidUpdateCondition = validated(
      testData / "attribute-na-invalid-update-condition-fom.xml",
      FomModuleKind::fom,
      L"urn:umbra:test:attribute-na-invalid-update-condition");
  auto invalidTransportation = validated(
      testData / "attribute-na-invalid-transportation-fom.xml",
      FomModuleKind::fom,
      L"urn:umbra:test:attribute-na-invalid-transportation");
  auto validCompanions = validated(
      testData / "attribute-na-valid-companions-fom.xml",
      FomModuleKind::fom,
      L"urn:umbra:test:attribute-na-valid-companions");
  // DIF accepts a partial attribute row.  The later module completes its
  // direct companion columns before the composed FDD is materialized.
  auto omittedCompanions = validated(
      testData / "attribute-na-omitted-companions-fom.xml",
      FomModuleKind::fom,
      L"urn:umbra:test:attribute-na-omitted-companions");
  auto omittedCompanionsExtension = validated(
      testData / "attribute-na-omitted-companions-extension-fom.xml",
      FomModuleKind::fom,
      L"urn:umbra:test:attribute-na-omitted-companions-extension");

  auto materializer = composer();
  auto updateTypeResult = materializer.compose({mim, invalidUpdateType});
  REQUIRE(updateTypeResult.status == FomCompositionStatus::inconsistent_modules);
  REQUIRE(updateTypeResult.diagnostics.find("UmbraNaInvalidUpdateType") != std::string::npos);
  REQUIRE(updateTypeResult.diagnostics.find("update type") != std::string::npos);
  REQUIRE(updateTypeResult.diagnostics.find("must be NA") != std::string::npos);

  auto updateConditionResult = materializer.compose({mim, invalidUpdateCondition});
  REQUIRE(updateConditionResult.status == FomCompositionStatus::inconsistent_modules);
  REQUIRE(updateConditionResult.diagnostics.find("UmbraNaInvalidUpdateCondition") !=
          std::string::npos);
  REQUIRE(updateConditionResult.diagnostics.find("update condition") != std::string::npos);
  REQUIRE(updateConditionResult.diagnostics.find("must be NA") != std::string::npos);

  auto transportationResult = materializer.compose({mim, invalidTransportation});
  REQUIRE(transportationResult.status == FomCompositionStatus::inconsistent_modules);
  REQUIRE(transportationResult.diagnostics.find("UmbraNaInvalidTransportation") !=
          std::string::npos);
  REQUIRE(transportationResult.diagnostics.find("transportation") != std::string::npos);
  REQUIRE(transportationResult.diagnostics.find("non-NA") != std::string::npos);

  auto validResult = materializer.compose({mim, validCompanions});
  CAPTURE(validResult.diagnostics);
  REQUIRE(validResult.status == FomCompositionStatus::valid);
  REQUIRE(validResult.catalog);
  REQUIRE(validResult.catalog->objectClass("HLAobjectRoot") != nullptr);

  auto completedAcrossModulesResult =
      materializer.compose({mim, omittedCompanions, omittedCompanionsExtension});
  CAPTURE(completedAcrossModulesResult.diagnostics);
  REQUIRE(completedAcrossModulesResult.status == FomCompositionStatus::valid);
  REQUIRE(completedAcrossModulesResult.catalog);
  REQUIRE(completedAcrossModulesResult.catalog->objectClass("HLAobjectRoot") != nullptr);

  // A non-NA data type remains outside this predicate, preserving the
  // separately recorded Static/Update Condition source/example tension.
  auto standardResult = materializer.compose({
      mim,
      validated(
          resourcePath("examples/RestaurantFOMmodule-2025.xml"),
          FomModuleKind::fom,
          L"urn:umbra:test:restaurant"),
  });
  CAPTURE(standardResult.diagnostics);
  REQUIRE(standardResult.status == FomCompositionStatus::valid);
  REQUIRE(standardResult.catalog);
}

TEST_CASE(
    "The FOM composition preflight requires supplied dynamic update conditions",
    "[unit][fom][composition][attribute-dynamic-update-condition]") {
  auto const testData = std::filesystem::path(UMBRA_SOURCE_DIRECTORY) / "cpp" / "tests" / "data";
  auto mim = validated(
      resourcePath("mim/HLAstandardMIM-2025.xml"),
      FomModuleKind::mim,
      L"urn:umbra:test:mim");
  auto conditionalNa = validated(
      testData / "attribute-conditional-update-condition-na-fom.xml",
      FomModuleKind::fom,
      L"urn:umbra:test:attribute-conditional-update-condition-na");
  auto periodicNa = validated(
      testData / "attribute-periodic-update-condition-na-fom.xml",
      FomModuleKind::fom,
      L"urn:umbra:test:attribute-periodic-update-condition-na");
  auto conditionalEmpty = validated(
      testData / "attribute-conditional-update-condition-empty-fom.xml",
      FomModuleKind::fom,
      L"urn:umbra:test:attribute-conditional-update-condition-empty");
  auto validConditions = validated(
      testData / "attribute-dynamic-update-condition-valid-fom.xml",
      FomModuleKind::fom,
      L"urn:umbra:test:attribute-dynamic-update-condition-valid");

  auto materializer = composer();
  auto conditionalNaResult = materializer.compose({mim, conditionalNa});
  REQUIRE(conditionalNaResult.status == FomCompositionStatus::inconsistent_modules);
  REQUIRE(conditionalNaResult.diagnostics.find("UmbraConditionalNaCondition") != std::string::npos);
  REQUIRE(conditionalNaResult.diagnostics.find("Conditional update type") != std::string::npos);
  REQUIRE(conditionalNaResult.diagnostics.find("non-NA text") != std::string::npos);

  auto periodicNaResult = materializer.compose({mim, periodicNa});
  REQUIRE(periodicNaResult.status == FomCompositionStatus::inconsistent_modules);
  REQUIRE(periodicNaResult.diagnostics.find("UmbraPeriodicNaCondition") != std::string::npos);
  REQUIRE(periodicNaResult.diagnostics.find("Periodic update type") != std::string::npos);
  REQUIRE(periodicNaResult.diagnostics.find("non-NA text") != std::string::npos);

  auto conditionalEmptyResult = materializer.compose({mim, conditionalEmpty});
  REQUIRE(conditionalEmptyResult.status == FomCompositionStatus::inconsistent_modules);
  REQUIRE(conditionalEmptyResult.diagnostics.find("UmbraConditionalEmptyCondition") !=
          std::string::npos);
  REQUIRE(conditionalEmptyResult.diagnostics.find("non-NA text") != std::string::npos);

  auto validResult = materializer.compose({mim, validConditions});
  CAPTURE(validResult.diagnostics);
  REQUIRE(validResult.status == FomCompositionStatus::valid);
  REQUIRE(validResult.catalog);
  REQUIRE(validResult.catalog->objectClass("HLAobjectRoot") != nullptr);

  // Preserve both complete official dynamic rows and the recorded Static /
  // Update Condition tension outside this bounded predicate.
  auto standardResult = materializer.compose({
      mim,
      validated(
          resourcePath("examples/RestaurantFOMmodule-2025.xml"),
          FomModuleKind::fom,
          L"urn:umbra:test:restaurant"),
  });
  CAPTURE(standardResult.diagnostics);
  REQUIRE(standardResult.status == FomCompositionStatus::valid);
  REQUIRE(standardResult.catalog);
}

TEST_CASE(
    "The FOM composition preflight requires unshared attributes to be not value-required",
    "[unit][fom][composition][attribute-sharing-value-required]") {
  auto const testData = std::filesystem::path(UMBRA_SOURCE_DIRECTORY) / "cpp" / "tests" / "data";
  auto mim = validated(
      resourcePath("mim/HLAstandardMIM-2025.xml"),
      FomModuleKind::mim,
      L"urn:umbra:test:mim");
  auto invalidRequired = validated(
      testData / "attribute-neither-value-required-true-fom.xml",
      FomModuleKind::fom,
      L"urn:umbra:test:attribute-neither-value-required-true");
  auto validNotRequired = validated(
      testData / "attribute-neither-value-required-false-fom.xml",
      FomModuleKind::fom,
      L"urn:umbra:test:attribute-neither-value-required-false");

  auto materializer = composer();
  auto invalidResult = materializer.compose({mim, invalidRequired});
  REQUIRE(invalidResult.status == FomCompositionStatus::inconsistent_modules);
  REQUIRE(invalidResult.diagnostics.find("UmbraNeitherButValueRequired") != std::string::npos);
  REQUIRE(invalidResult.diagnostics.find("neither published nor subscribed") != std::string::npos);
  REQUIRE(invalidResult.diagnostics.find("must be false") != std::string::npos);

  auto validResult = materializer.compose({mim, validNotRequired});
  CAPTURE(validResult.diagnostics);
  REQUIRE(validResult.status == FomCompositionStatus::valid);
  REQUIRE(validResult.catalog);
  REQUIRE(validResult.catalog->objectClass("HLAobjectRoot") != nullptr);

  auto standardResult = materializer.compose({
      mim,
      validated(
          resourcePath("examples/RestaurantFOMmodule-2025.xml"),
          FomModuleKind::fom,
          L"urn:umbra:test:restaurant"),
  });
  CAPTURE(standardResult.diagnostics);
  REQUIRE(standardResult.status == FomCompositionStatus::valid);
  REQUIRE(standardResult.catalog);
}

TEST_CASE(
    "The FOM composition preflight requires standard object and interaction roots",
    "[unit][fom][composition][root-hierarchy]") {
  auto const testData = std::filesystem::path(UMBRA_SOURCE_DIRECTORY) / "cpp" / "tests" / "data";
  auto mim = validated(
      resourcePath("mim/HLAstandardMIM-2025.xml"),
      FomModuleKind::mim,
      L"urn:umbra:test:mim");
  auto objectRoot = validated(
      testData / "nonstandard-object-root-fom.xml",
      FomModuleKind::fom,
      L"urn:umbra:test:nonstandard-object-root");
  auto interactionRoot = validated(
      testData / "nonstandard-interaction-root-fom.xml",
      FomModuleKind::fom,
      L"urn:umbra:test:nonstandard-interaction-root");

  auto materializer = composer();
  auto objectResult = materializer.compose({mim, objectRoot});
  REQUIRE(objectResult.status == FomCompositionStatus::inconsistent_modules);
  REQUIRE(objectResult.diagnostics.find("UmbraUnexpectedObjectRoot") != std::string::npos);
  REQUIRE(objectResult.diagnostics.find("HLAobjectRoot") != std::string::npos);

  auto interactionResult = materializer.compose({mim, interactionRoot});
  REQUIRE(interactionResult.status == FomCompositionStatus::inconsistent_modules);
  REQUIRE(interactionResult.diagnostics.find("UmbraUnexpectedInteractionRoot") != std::string::npos);
  REQUIRE(interactionResult.diagnostics.find("HLAinteractionRoot") != std::string::npos);

  auto standardResult = materializer.compose({
      mim,
      validated(
          resourcePath("examples/RestaurantFOMmodule-2025.xml"),
          FomModuleKind::fom,
          L"urn:umbra:test:restaurant"),
  });
  CAPTURE(standardResult.diagnostics);
  REQUIRE(standardResult.status == FomCompositionStatus::valid);
  REQUIRE(standardResult.catalog);
  REQUIRE(standardResult.catalog->objectClass("HLAobjectRoot.Employee") != nullptr);
  REQUIRE(standardResult.catalog->interactionClass("HLAinteractionRoot.ServerAction.TakeOrder") != nullptr);
}

TEST_CASE(
    "The FOM composition preflight validates user-supplied and synchronization tag data type categories",
    "[unit][fom][composition][tag-data-type]") {
  auto const testData = std::filesystem::path(UMBRA_SOURCE_DIRECTORY) / "cpp" / "tests" / "data";
  auto mim = validated(
      resourcePath("mim/HLAstandardMIM-2025.xml"),
      FomModuleKind::mim,
      L"urn:umbra:test:mim");
  auto allowed = validated(
      testData / "tag-data-type-category-allowed-fom.xml",
      FomModuleKind::fom,
      L"urn:umbra:test:tag-data-type-allowed");
  auto basicUserTag = validated(
      testData / "tag-data-type-basic-data-fom.xml",
      FomModuleKind::fom,
      L"urn:umbra:test:tag-data-type-basic-user-tag");
  auto basicSynchronizationTag = validated(
      testData / "synchronization-tag-basic-data-fom.xml",
      FomModuleKind::fom,
      L"urn:umbra:test:tag-data-type-basic-synchronization");

  auto materializer = composer();
  auto allowedResult = materializer.compose({mim, allowed});
  CAPTURE(allowedResult.diagnostics);
  REQUIRE(allowedResult.status == FomCompositionStatus::valid);
  REQUIRE(allowedResult.catalog);
  REQUIRE(allowedResult.fdd);
  REQUIRE(allowedResult.catalog->dataType("UmbraTagReferenceFixture") != nullptr);

  auto basicUserTagResult = materializer.compose({mim, basicUserTag});
  REQUIRE(basicUserTagResult.status == FomCompositionStatus::inconsistent_modules);
  REQUIRE(basicUserTagResult.diagnostics.find("HLAinteger32BE") != std::string::npos);
  REQUIRE(basicUserTagResult.diagnostics.find("not permitted") != std::string::npos);

  auto basicSynchronizationTagResult = materializer.compose({mim, basicSynchronizationTag});
  REQUIRE(basicSynchronizationTagResult.status == FomCompositionStatus::inconsistent_modules);
  REQUIRE(basicSynchronizationTagResult.diagnostics.find("HLAinteger32BE") != std::string::npos);
  REQUIRE(basicSynchronizationTagResult.diagnostics.find("not permitted") != std::string::npos);
}

TEST_CASE(
    "The FOM composition preflight rejects inherited attribute and parameter name overloading",
    "[unit][fom][composition][inheritance]") {
  auto const testData = std::filesystem::path(UMBRA_SOURCE_DIRECTORY) / "cpp" / "tests" / "data";
  auto mim = validated(
      resourcePath("mim/HLAstandardMIM-2025.xml"),
      FomModuleKind::mim,
      L"urn:umbra:test:mim");
  auto duplicateAttribute = validated(
      testData / "inherited-attribute-duplicate-fom.xml",
      FomModuleKind::fom,
      L"urn:umbra:test:inherited-attribute-duplicate");
  auto duplicateParameter = validated(
      testData / "inherited-parameter-duplicate-fom.xml",
      FomModuleKind::fom,
      L"urn:umbra:test:inherited-parameter-duplicate");

  auto materializer = composer();
  auto attributeResult = materializer.compose({mim, duplicateAttribute});
  REQUIRE(attributeResult.status == FomCompositionStatus::inconsistent_modules);
  REQUIRE(attributeResult.diagnostics.find("duplicates inherited attribute") != std::string::npos);
  REQUIRE(attributeResult.diagnostics.find("UmbraInheritedAttributeFixture") != std::string::npos);

  auto parameterResult = materializer.compose({mim, duplicateParameter});
  REQUIRE(parameterResult.status == FomCompositionStatus::inconsistent_modules);
  REQUIRE(parameterResult.diagnostics.find("duplicates inherited parameter") != std::string::npos);
  REQUIRE(parameterResult.diagnostics.find("UmbraInheritedParameterFixture") != std::string::npos);
}

TEST_CASE(
    "The FDD materializer refuses the supplied extension when the official FDD schema cannot represent its directed-interaction merge",
    "[unit][fom][composition][schema-conflict]"
    "[directed-interaction-multiple-subscription-kinds]") {
  std::vector<PrevalidatedFomModule> modules{
      validated(resourcePath("mim/HLAstandardMIM-2025.xml"), FomModuleKind::mim, L"urn:umbra:test:mim"),
      validated(resourcePath("examples/RestaurantFOMmodule-2025.xml"), FomModuleKind::fom, L"urn:umbra:test:restaurant"),
      validated(
          resourcePath("examples/RestaurantExtensionFOMmodule-2025.xml"),
          FomModuleKind::fom,
          L"urn:umbra:test:restaurant-extension"),
  };

  auto materializer = composer();
  auto result = materializer.compose(modules);

  REQUIRE(result.status == FomCompositionStatus::inconsistent_modules);
  REQUIRE(result.diagnostics.find("directedInteraction") != std::string::npos);
}

TEST_CASE("The FOM composition preflight permits equivalent duplicates and rejects a real class conflict", "[unit][fom][composition][annex-c]") {
  auto mim = validated(
      resourcePath("mim/HLAstandardMIM-2025.xml"), FomModuleKind::mim, L"urn:umbra:test:mim");
  auto base = validated(
      resourcePath("examples/RestaurantFOMmodule-2025.xml"),
      FomModuleKind::fom,
      L"urn:umbra:test:restaurant");
  auto conflicting = validated(
      std::filesystem::path(UMBRA_SOURCE_DIRECTORY) / "cpp" / "tests" / "data" /
          "conflicting-employee-fom.xml",
      FomModuleKind::fom,
      L"urn:umbra:test:conflicting-employee");

  auto materializer = composer();
  auto duplicateResult = materializer.compose({mim, base, base});
  CAPTURE(duplicateResult.diagnostics);
  REQUIRE(duplicateResult.status == FomCompositionStatus::valid);
  REQUIRE(duplicateResult.warnings.empty());

  auto conflictResult = materializer.compose({mim, base, conflicting});
  REQUIRE(conflictResult.status == FomCompositionStatus::inconsistent_modules);
  REQUIRE(conflictResult.diagnostics.find("sharing") != std::string::npos);
}

TEST_CASE("The FOM composition preflight enforces enumerated and variant-record merge invariants", "[unit][fom][composition][annex-c]") {
  auto const testData = std::filesystem::path(UMBRA_SOURCE_DIRECTORY) / "cpp" / "tests" / "data";
  auto materializer = composer();

  auto enumeratedBase = validated(
      testData / "enumerated-value-base-fom.xml", FomModuleKind::fom, L"urn:umbra:test:enum-base");
  auto enumeratedConflict = validated(
      testData / "enumerated-value-conflict-fom.xml",
      FomModuleKind::fom,
      L"urn:umbra:test:enum-conflict");
  auto enumeratedResult = materializer.compose({enumeratedBase, enumeratedConflict});
  REQUIRE(enumeratedResult.status == FomCompositionStatus::inconsistent_modules);
  REQUIRE(enumeratedResult.diagnostics.find("assigns value") != std::string::npos);

  auto nonextendableBase = validated(
      testData / "nonextendable-variant-base-fom.xml",
      FomModuleKind::fom,
      L"urn:umbra:test:variant-base");
  auto nonextendableExtension = validated(
      testData / "nonextendable-variant-extension-fom.xml",
      FomModuleKind::fom,
      L"urn:umbra:test:variant-extension");
  auto nonextendableResult = materializer.compose({nonextendableBase, nonextendableExtension});
  REQUIRE(nonextendableResult.status == FomCompositionStatus::inconsistent_modules);
  REQUIRE(nonextendableResult.diagnostics.find("non-extendable") != std::string::npos);

  auto prohibitedHlaOther = validated(
      testData / "extendable-variant-hlaother-fom.xml",
      FomModuleKind::fom,
      L"urn:umbra:test:variant-hlaother");
  auto hlaOtherResult = materializer.compose({prohibitedHlaOther});
  REQUIRE(hlaOtherResult.status == FomCompositionStatus::inconsistent_modules);
  REQUIRE(hlaOtherResult.diagnostics.find("HLAother") != std::string::npos);
}

TEST_CASE(
    "The FDD materializer remaps referenced notes and logically ORs service usage",
    "[unit][fom][composition][annex-c]") {
  auto const testData = std::filesystem::path(UMBRA_SOURCE_DIRECTORY) / "cpp" / "tests" / "data";
  std::vector<PrevalidatedFomModule> modules{
      validated(resourcePath("mim/HLAstandardMIM-2025.xml"), FomModuleKind::mim, L"urn:umbra:test:mim"),
      validated(testData / "service-notes-a-fom.xml", FomModuleKind::fom, L"urn:umbra:test:service-a"),
      validated(testData / "service-notes-b-fom.xml", FomModuleKind::fom, L"urn:umbra:test:service-b"),
  };

  auto materializer = composer();
  auto result = materializer.compose(modules);

  REQUIRE(result.status == FomCompositionStatus::valid);
  REQUIRE(result.fdd);
  std::string const& xml = result.fdd->xmlUtf8();
  auto const disconnectStart = xml.find("<disconnect");
  REQUIRE(disconnectStart != std::string::npos);
  auto const disconnectEnd = xml.find("/>", disconnectStart);
  REQUIRE(disconnectEnd != std::string::npos);
  std::string const disconnect = xml.substr(disconnectStart, disconnectEnd - disconnectStart);
  REQUIRE(disconnect.find("isUsed=\"true\"") != std::string::npos);
  REQUIRE(disconnect.find("UmbraNote") != std::string::npos);
  REQUIRE(xml.find("<label>Note1</label>") == std::string::npos);
  REQUIRE(xml.find("<label>UmbraNote") != std::string::npos);
  REQUIRE(xml.find("Service Notes A") != std::string::npos);
  REQUIRE(xml.find("Service Notes B") != std::string::npos);
}

TEST_CASE(
    "Reference logical-time selection defaults to HLAfloat64Time and rejects incompatible FDD documentation",
    "[unit][fom][time]") {
  auto mim = validated(
      resourcePath("mim/HLAstandardMIM-2025.xml"), FomModuleKind::mim, L"urn:umbra:test:mim");
  auto integerTimeFom = validated(
      resourcePath("examples/RestaurantFOMmodule-2025.xml"),
      FomModuleKind::fom,
      L"urn:umbra:test:integer-time");
  auto noTimeFom = validated(
      std::filesystem::path(UMBRA_SOURCE_DIRECTORY) / "cpp" / "tests" / "data" / "service-notes-a-fom.xml",
      FomModuleKind::fom,
      L"urn:umbra:test:no-time");

  auto materializer = composer();
  auto integerTimeResult = materializer.compose({mim, integerTimeFom});
  REQUIRE(integerTimeResult.status == FomCompositionStatus::valid);
  REQUIRE(integerTimeResult.catalog);

  ReferenceLogicalTimeSelector selector;
  auto defaultSelection = selector.select(*integerTimeResult.catalog, L"");
  REQUIRE(defaultSelection.status ==
          ReferenceLogicalTimeSelectionStatus::inconsistent_fdd_time_representation);
  REQUIRE(defaultSelection.selectedImplementationName == L"HLAfloat64Time");
  REQUIRE_FALSE(defaultSelection.accepted());

  auto integerSelection = selector.select(*integerTimeResult.catalog, L"HLAinteger64Time");
  REQUIRE(integerSelection.status == ReferenceLogicalTimeSelectionStatus::selected);
  REQUIRE(integerSelection.selectedImplementationName == L"HLAinteger64Time");
  REQUIRE(integerSelection.accepted());

  auto explicitFloatSelection = selector.select(*integerTimeResult.catalog, L"HLAfloat64Time");
  REQUIRE(explicitFloatSelection.status ==
          ReferenceLogicalTimeSelectionStatus::inconsistent_fdd_time_representation);

  auto unavailableSelection = selector.select(*integerTimeResult.catalog, L"ExampleCustomTime");
  REQUIRE(unavailableSelection.status == ReferenceLogicalTimeSelectionStatus::factory_unavailable);
  REQUIRE_FALSE(unavailableSelection.accepted());

  auto noTimeResult = materializer.compose({mim, noTimeFom});
  REQUIRE(noTimeResult.status == FomCompositionStatus::valid);
  REQUIRE(noTimeResult.catalog);
  auto unconstrainedSelection = selector.select(*noTimeResult.catalog, L"");
  REQUIRE(unconstrainedSelection.status == ReferenceLogicalTimeSelectionStatus::selected);
  REQUIRE(unconstrainedSelection.selectedImplementationName == L"HLAfloat64Time");
}

TEST_CASE(
    "Federation preparation loads MIM first and emits only an FDD-backed reference-time definition",
    "[unit][fom][federation-management]") {
  LibXml2FomValidator validator;
  auto materializer = composer();
  ReferenceLogicalTimeSelector selector;
  FederationManagementCoordinator coordinator(
      validator,
      materializer,
      selector,
      {
          resourcePath("mim/HLAstandardMIM-2025.xml"),
          resourcePath("schemas/IEEE1516-DIF-2025.xsd"),
      });

  std::wstring const restaurant = resourcePath("examples/RestaurantFOMmodule-2025.xml").wstring();
  std::wstring const extension =
      resourcePath("examples/RestaurantExtensionFOMmodule-2025.xml").wstring();

  auto prepared = coordinator.prepareCreate({restaurant}, std::nullopt, L"HLAinteger64Time");
  CAPTURE(prepared.diagnostics);
  REQUIRE(prepared.status == FederationPreparationStatus::applied);
  REQUIRE(prepared.accepted());
  REQUIRE(prepared.definition);
  REQUIRE(prepared.definition->fomModules.size() == 2);
  REQUIRE(prepared.definition->fomModules.front().kind == FomModuleKind::mim);
  REQUIRE(prepared.definition->fomModules.front().designator == L"HLAstandardMIM");
  REQUIRE(prepared.definition->fomModules.back().designator == restaurant);
  REQUIRE(prepared.definition->logicalTimeImplementationName == L"HLAinteger64Time");
  REQUIRE(prepared.definition->catalog);
  REQUIRE(prepared.definition->fdd);

  auto defaultMismatch = coordinator.prepareCreate({restaurant}, std::nullopt, L"");
  REQUIRE(defaultMismatch.status == FederationPreparationStatus::inconsistent_fom);
  REQUIRE_FALSE(defaultMismatch.definition);

  auto unavailableTime = coordinator.prepareCreate({restaurant}, std::nullopt, L"ExampleCustomTime");
  REQUIRE(unavailableTime.status == FederationPreparationStatus::time_factory_unavailable);
  REQUIRE_FALSE(unavailableTime.definition);

  auto noFom = coordinator.prepareCreate({}, std::nullopt, L"HLAinteger64Time");
  REQUIRE(noFom.status == FederationPreparationStatus::invalid_fom);

  auto prohibitedMim = coordinator.prepareCreate(
      {restaurant}, std::wstring(L"HLAstandardMIM"), L"HLAinteger64Time");
  REQUIRE(prohibitedMim.status == FederationPreparationStatus::standard_mim_designator_supplied);

  auto missingFom = coordinator.prepareCreate({L"missing-fom.xml"}, std::nullopt, L"HLAfloat64Time");
  REQUIRE(missingFom.status == FederationPreparationStatus::fom_not_found);

  auto duplicateAdditional =
      coordinator.prepareAdditionalModules(*prepared.definition, {restaurant});
  CAPTURE(duplicateAdditional.diagnostics);
  REQUIRE(duplicateAdditional.status == FederationPreparationStatus::applied);
  REQUIRE(duplicateAdditional.definition);
  REQUIRE(duplicateAdditional.definition->fomModules.size() == 3);

  auto incompatibleAdditional =
      coordinator.prepareAdditionalModules(*prepared.definition, {extension});
  REQUIRE(incompatibleAdditional.status == FederationPreparationStatus::inconsistent_fom);
  REQUIRE_FALSE(incompatibleAdditional.definition);
}
