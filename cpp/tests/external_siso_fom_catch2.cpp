#include <catch2/catch_test_macros.hpp>

#include "internal/fom/fom_catalog.hpp"
#include "internal/fom/fom_wire_codec.hpp"
#include "internal/fom/libxml2_fom_composer.hpp"
#include "internal/fom/libxml2_fom_validator.hpp"

#ifdef UMBRA_EXTERNAL_2010_FOM_RESOURCE_DIRECTORY
#include "internal/runtime/umbra_rti_ambassador.hpp"

#include <RTI/NullFederateAmbassador.h>
#include <RTI/RTI1516.h>
#include <RTI/encoding/EncodingConfig.h>
#endif
#include "internal/runtime/umbra_rti_ambassador.hpp"

#include <array>
#include <atomic>
#include <cstddef>
#include <cstdint>
#include <filesystem>
#include <fstream>
#include <iterator>
#include <string>
#include <utility>
#include <vector>

#include <RTI/NullFederateAmbassador.h>
#include <RTI/RTI1516.h>
#include <RTI/encoding/EncodingConfig.h>

#ifndef UMBRA_EXTERNAL_SISO_FOM_CORPUS_DIRECTORY
#error "UMBRA_EXTERNAL_SISO_FOM_CORPUS_DIRECTORY must be defined for external SISO corpus tests."
#endif

#ifndef UMBRA_SOURCE_DIRECTORY
#error "UMBRA_SOURCE_DIRECTORY must be defined for FOM resource tests."
#endif

namespace {

using umbra::detail::FomModuleKind;
using umbra::detail::FomCompositionStatus;
using umbra::detail::FomSourceCompatibility;
using umbra::detail::FomStandardEdition;
using umbra::detail::FomValidationStatus;
using umbra::detail::FomWireCodecStatus;
using umbra::detail::FomWireEncodingKind;
using umbra::detail::LibXml2FomModuleComposer;
using umbra::detail::LibXml2FomValidator;
using umbra::detail::decodeRprNullTerminatedArray;
using umbra::detail::encodeRprNullTerminatedArray;
using umbra::detail::normalizeFomWireEncoding;
using umbra::detail::PrevalidatedFomModule;

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

std::string readBinary(std::filesystem::path const& source) {
  std::ifstream input(source, std::ios::binary);
  REQUIRE(input.good());
  return {
      std::istreambuf_iterator<char>(input),
      std::istreambuf_iterator<char>()};
}

#ifdef UMBRA_EXTERNAL_2010_FOM_RESOURCE_DIRECTORY
std::filesystem::path legacyResourcePath(std::filesystem::path const& compactRelative) {
  auto const root = std::filesystem::path(UMBRA_EXTERNAL_2010_FOM_RESOURCE_DIRECTORY);
  auto const compact = root / compactRelative;
  if (std::filesystem::is_regular_file(compact)) {
    return compact;
  }
  if (compactRelative == std::filesystem::path("mim/HLAstandardMIM-2010.xml")) {
    return root / "1516_1-2010" / "HLAstandardMIM.xml";
  }
  if (compactRelative == std::filesystem::path("schemas/IEEE1516-DIF-2010.xsd")) {
    return root / "1516_2-2010" / "IEEE1516-DIF-2010.xsd";
  }
  if (compactRelative == std::filesystem::path("schemas/IEEE1516-FDD-2010.xsd")) {
    return root / "1516_1-2010" / "IEEE1516-FDD-2010.xsd";
  }
  return compact;
}

PrevalidatedFomModule validate2010(
    std::filesystem::path const& source,
    std::wstring designator,
    FomSourceCompatibility sourceCompatibility = FomSourceCompatibility::strict) {
  LibXml2FomValidator validator;
  auto result = validator.validate({
      source,
      legacyResourcePath("schemas/IEEE1516-DIF-2010.xsd"),
      FomModuleKind::fom,
      std::move(designator),
      L"IEEE1516-DIF-2010.xsd",
      FomStandardEdition::ieee1516_2010,
      sourceCompatibility,
  });
  CAPTURE(source.string(), result.diagnostics);
  REQUIRE(result.status == FomValidationStatus::valid);
  REQUIRE(result.module.has_value());
  return *result.module;
}

PrevalidatedFomModule validateStandardMim2010(
    FomSourceCompatibility sourceCompatibility = FomSourceCompatibility::strict) {
  LibXml2FomValidator validator;
  auto result = validator.validate({
      legacyResourcePath("mim/HLAstandardMIM-2010.xml"),
      legacyResourcePath("schemas/IEEE1516-DIF-2010.xsd"),
      FomModuleKind::mim,
      L"HLAstandardMIM",
      L"IEEE1516-DIF-2010.xsd",
      FomStandardEdition::ieee1516_2010,
      sourceCompatibility,
  });
  CAPTURE(result.diagnostics);
  REQUIRE(result.status == FomValidationStatus::valid);
  REQUIRE(result.module.has_value());
  return *result.module;
}

LibXml2FomModuleComposer legacyComposer() {
  return LibXml2FomModuleComposer(
      resourcePath("schemas/IEEE1516-FDD-2025.xsd"),
      legacyResourcePath("schemas/IEEE1516-FDD-2010.xsd"));
}

#ifdef UMBRA_ENABLE_EMBEDDED_FEDERATION_MANAGEMENT
class RprPayloadFederateAmbassador final
    : public rti1516_2025::NullFederateAmbassador {
 public:
  struct Discovery final {
    rti1516_2025::ObjectInstanceHandle objectInstance;
    rti1516_2025::ObjectClassHandle objectClass;
    std::wstring objectInstanceName;
    rti1516_2025::FederateHandle producingFederate;
  };

  struct Reflection final {
    rti1516_2025::ObjectInstanceHandle objectInstance;
    rti1516_2025::AttributeHandleValueMap attributeValues;
    rti1516_2025::VariableLengthData userSuppliedTag;
  };

  void discoverObjectInstance(
      rti1516_2025::ObjectInstanceHandle const& objectInstance,
      rti1516_2025::ObjectClassHandle const& objectClass,
      std::wstring const& objectInstanceName,
      rti1516_2025::FederateHandle const& producingFederate) override {
    discoveries.push_back({
        objectInstance,
        objectClass,
        objectInstanceName,
        producingFederate,
    });
  }

  void reflectAttributeValues(
      rti1516_2025::ObjectInstanceHandle const& objectInstance,
      rti1516_2025::AttributeHandleValueMap const& attributeValues,
      rti1516_2025::VariableLengthData const& userSuppliedTag,
      rti1516_2025::TransportationTypeHandle const&,
      rti1516_2025::FederateHandle const&,
      rti1516_2025::RegionHandleSet const*) override {
    reflections.push_back({objectInstance, attributeValues, userSuppliedTag});
  }

  std::vector<Discovery> discoveries;
  std::vector<Reflection> reflections;
};

std::atomic_uint64_t rprFederationSequence{0};

std::wstring nextRprFederationName() {
  return L"UmbraExternalRpr2010Payload-" +
      std::to_wstring(++rprFederationSequence);
}
#endif
#endif

}  // namespace

TEST_CASE(
    "The optional SISO corpus remains explicitly outside the IEEE 1516.2-2025 input policy",
    "[unit][fom][xml-schema][external-corpus][cross-edition]") {
  // These fixtures deliberately span a Space family, an RPR publication that
  // is dated 2025, and a Link 16 extension.  Their XML namespace and selected
  // schema are IEEE 1516-2010, so accepting any of them under the 2025 DIF
  // policy would be a cross-edition bug rather than interoperability support.
  std::array<ExternalFixture, 22> const fixtures{{
      {"siso-space-datatypes", std::filesystem::path("SISO-STD-018 Files Normative") / "SISO_SpaceFOM_datatypes.xml"},
      {"siso-space-environment", std::filesystem::path("SISO-STD-018 Files Normative") / "SISO_SpaceFOM_environment.xml"},
      {"siso-space-switches", std::filesystem::path("SISO-STD-018 Files Normative") / "SISO_SpaceFOM_switches.xml"},
      {"siso-space-management", std::filesystem::path("SISO-STD-018 Files Normative") / "SISO_SpaceFOM_management.xml"},
      {"siso-space-entity", std::filesystem::path("SISO-STD-018 Files Normative") / "SISO_SpaceFOM_entity.xml"},
      {"siso-rpr-3-foundation", std::filesystem::path("SISO-STD-001.1-2025 Annex A Files Normative") / "RPR-Foundation_v3.0.xml"},
      {"siso-rpr-2-foundation", std::filesystem::path("RPR FOM v2.0 Link 16 and Link 11") / "RPR-Foundation_v2.0.xml"},
      {"siso-rpr-2-enumerations", std::filesystem::path("RPR FOM v2.0 Link 16 and Link 11") / "RPR-Enumerations_v2.0.xml"},
      {"siso-rpr-2-base", std::filesystem::path("RPR FOM v2.0 Link 16 and Link 11") / "RPR-Base_v2.0.xml"},
      {"siso-rpr-2-physical", std::filesystem::path("RPR FOM v2.0 Link 16 and Link 11") / "RPR-Physical_v2.0.xml"},
      {"siso-rpr-2-logistics", std::filesystem::path("RPR FOM v2.0 Link 16 and Link 11") / "RPR-Logistics_v2.0.xml"},
      {"siso-rpr-2-minefield", std::filesystem::path("RPR FOM v2.0 Link 16 and Link 11") / "RPR-Minefield_v2.0.xml"},
      {"siso-rpr-2-switches", std::filesystem::path("RPR FOM v2.0 Link 16 and Link 11") / "RPR-Switches_v2.0.xml"},
      {"siso-rpr-2-der", std::filesystem::path("RPR FOM v2.0 Link 16 and Link 11") / "RPR-DER_v2.0.xml"},
      {"siso-rpr-2-warfare", std::filesystem::path("RPR FOM v2.0 Link 16 and Link 11") / "RPR-Warfare_v2.0.xml"},
      {"siso-rpr-2-communication", std::filesystem::path("RPR FOM v2.0 Link 16 and Link 11") / "RPR-Communication_v2.0.xml"},
      {"siso-rpr-2-siman", std::filesystem::path("RPR FOM v2.0 Link 16 and Link 11") / "RPR-SIMAN_v2.0.xml"},
      {"siso-rpr-2-ua", std::filesystem::path("RPR FOM v2.0 Link 16 and Link 11") / "RPR-UA_v2.0.xml"},
      {"siso-rpr-2-aggregate", std::filesystem::path("RPR FOM v2.0 Link 16 and Link 11") / "RPR-Aggregate_v2.0.xml"},
      {"siso-link-16", std::filesystem::path("RPR FOM v2.0 Link 16 and Link 11") / "Link_16_v2.0.xml"},
      {"siso-rpr-2-link11", std::filesystem::path("RPR FOM v2.0 Link 16 and Link 11") / "Link_11_11B_v1.0.xml"},
      {"siso-rpr-2-se", std::filesystem::path("RPR FOM v2.0 Link 16 and Link 11") / "RPR-SE_v2.0.xml"},
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

#ifdef UMBRA_EXTERNAL_2010_FOM_RESOURCE_DIRECTORY
TEST_CASE(
    "The external SISO Space family composes into a 2010 lookup catalog",
    "[unit][fom][xml-schema][external-corpus][ieee1516-2010][space]") {
  std::array<std::filesystem::path, 5> const modules{{
      std::filesystem::path("SISO-STD-018 Files Normative") / "SISO_SpaceFOM_datatypes.xml",
      std::filesystem::path("SISO-STD-018 Files Normative") / "SISO_SpaceFOM_environment.xml",
      std::filesystem::path("SISO-STD-018 Files Normative") / "SISO_SpaceFOM_switches.xml",
      std::filesystem::path("SISO-STD-018 Files Normative") / "SISO_SpaceFOM_management.xml",
      std::filesystem::path("SISO-STD-018 Files Normative") / "SISO_SpaceFOM_entity.xml",
  }};

  std::vector<PrevalidatedFomModule> ordered;
  ordered.reserve(modules.size() + 1);
  ordered.push_back(validateStandardMim2010());
  for (auto const& relative : modules) {
    ordered.push_back(validate2010(externalCorpusPath(relative), relative.filename().wstring()));
  }

  auto composer = legacyComposer();
  auto result = composer.compose(ordered);
  CAPTURE(result.diagnostics);
  REQUIRE(result.status == FomCompositionStatus::valid);
  REQUIRE(result.catalog);
  REQUIRE_FALSE(result.fdd);
  REQUIRE(result.catalog->dataType("SpaceTimeCoordinateState") != nullptr);
  REQUIRE(result.catalog->dataType("PositionVector") != nullptr);
  REQUIRE(result.catalog->objectClass("HLAobjectRoot.ReferenceFrame") != nullptr);
  REQUIRE(result.catalog->objectClass("HLAobjectRoot.PhysicalEntity.DynamicalEntity") != nullptr);
  REQUIRE(result.catalog->interactionClass("HLAinteractionRoot.ModeTransitionRequest") != nullptr);
}

TEST_CASE(
    "The external RPR 2.0 foundation composes into a 2010 lookup catalog",
    "[unit][fom][xml-schema][external-corpus][ieee1516-2010][rpr]") {
  auto mim = validateStandardMim2010();
  auto foundation = validate2010(
      externalCorpusPath(
          std::filesystem::path("RPR FOM v2.0 Link 16 and Link 11") /
              "RPR-Foundation_v2.0.xml"),
      L"RPR-Foundation_v2.0.xml");

  auto composer = legacyComposer();
  auto result = composer.compose({std::move(mim), std::move(foundation)});
  CAPTURE(result.diagnostics);
  REQUIRE(result.status == FomCompositionStatus::valid);
  REQUIRE(result.catalog);
  REQUIRE_FALSE(result.fdd);
  REQUIRE(result.catalog->dataType("RPRboolean") != nullptr);
  REQUIRE(result.catalog->dataType("RTIobjectId") != nullptr);
}

TEST_CASE(
    "The external RPR 2.0 family reports its strict 2010 schema boundary",
    "[unit][fom][xml-schema][external-corpus][ieee1516-2010][rpr][schema-boundary]") {
  std::array<std::filesystem::path, 16> const modules{{
      std::filesystem::path("RPR FOM v2.0 Link 16 and Link 11") / "RPR-Foundation_v2.0.xml",
      std::filesystem::path("RPR FOM v2.0 Link 16 and Link 11") / "RPR-Enumerations_v2.0.xml",
      std::filesystem::path("RPR FOM v2.0 Link 16 and Link 11") / "RPR-Base_v2.0.xml",
      std::filesystem::path("RPR FOM v2.0 Link 16 and Link 11") / "RPR-Physical_v2.0.xml",
      std::filesystem::path("RPR FOM v2.0 Link 16 and Link 11") / "RPR-Logistics_v2.0.xml",
      std::filesystem::path("RPR FOM v2.0 Link 16 and Link 11") / "RPR-Minefield_v2.0.xml",
      std::filesystem::path("RPR FOM v2.0 Link 16 and Link 11") / "RPR-Switches_v2.0.xml",
      std::filesystem::path("RPR FOM v2.0 Link 16 and Link 11") / "RPR-DER_v2.0.xml",
      std::filesystem::path("RPR FOM v2.0 Link 16 and Link 11") / "RPR-Warfare_v2.0.xml",
      std::filesystem::path("RPR FOM v2.0 Link 16 and Link 11") / "RPR-Communication_v2.0.xml",
      std::filesystem::path("RPR FOM v2.0 Link 16 and Link 11") / "RPR-SIMAN_v2.0.xml",
      std::filesystem::path("RPR FOM v2.0 Link 16 and Link 11") / "RPR-UA_v2.0.xml",
      std::filesystem::path("RPR FOM v2.0 Link 16 and Link 11") / "RPR-Aggregate_v2.0.xml",
      std::filesystem::path("RPR FOM v2.0 Link 16 and Link 11") / "Link_16_v2.0.xml",
      std::filesystem::path("RPR FOM v2.0 Link 16 and Link 11") / "Link_11_11B_v1.0.xml",
      std::filesystem::path("RPR FOM v2.0 Link 16 and Link 11") / "RPR-SE_v2.0.xml",
  }};

  std::size_t validModuleCount = 0;
  bool enumerationsRejected = false;
  for (auto const& relative : modules) {
    LibXml2FomValidator validator;
    auto result = validator.validate({
        externalCorpusPath(relative),
        legacyResourcePath("schemas/IEEE1516-DIF-2010.xsd"),
        FomModuleKind::fom,
        relative.filename().wstring(),
        L"IEEE1516-DIF-2010.xsd",
        FomStandardEdition::ieee1516_2010,
    });
    INFO(relative.string());
    INFO(result.diagnostics);
    if (relative.filename() == "RPR-Enumerations_v2.0.xml") {
      enumerationsRejected = true;
      REQUIRE(result.status == FomValidationStatus::invalid_model);
      REQUIRE_FALSE(result.module.has_value());
      REQUIRE(result.diagnostics.find("anyURI") != std::string::npos);
    } else {
      CHECK(result.status == FomValidationStatus::valid);
      if (result.module.has_value()) {
        ++validModuleCount;
      }
    }
  }

  REQUIRE(validModuleCount == 15);
REQUIRE(enumerationsRejected);
}

TEST_CASE(
    "The RPR AnyURI compatibility fix is an in-memory, warning-producing normalization",
    "[unit][fom][xml-schema][external-corpus][ieee1516-2010][rpr][anyuri][compatibility]") {
  auto const source = externalCorpusPath(
      std::filesystem::path("RPR FOM v2.0 Link 16 and Link 11") /
          "RPR-Enumerations_v2.0.xml");
  auto const originalBytes = readBinary(source);

  LibXml2FomValidator validator;
  auto strict = validator.validate({
      source,
      legacyResourcePath("schemas/IEEE1516-DIF-2010.xsd"),
      FomModuleKind::fom,
      L"RPR-Enumerations_v2.0.xml",
      L"IEEE1516-DIF-2010.xsd",
      FomStandardEdition::ieee1516_2010,
  });
  CAPTURE(strict.diagnostics);
  REQUIRE(strict.status == FomValidationStatus::invalid_model);
  REQUIRE_FALSE(strict.module.has_value());
  REQUIRE(strict.diagnostics.find("anyURI") != std::string::npos);

  auto compatible = validator.validate({
      source,
      legacyResourcePath("schemas/IEEE1516-DIF-2010.xsd"),
      FomModuleKind::fom,
      L"RPR-Enumerations_v2.0.xml",
      L"IEEE1516-DIF-2010.xsd",
      FomStandardEdition::ieee1516_2010,
      FomSourceCompatibility::rpr_2010,
  });
  CAPTURE(compatible.diagnostics);
  REQUIRE(compatible.status == FomValidationStatus::valid);
  REQUIRE(compatible.module.has_value());
  REQUIRE(compatible.module->warnings.size() == 1U);
  REQUIRE(compatible.module->warnings.front().find("anyURI") != std::string::npos);
  REQUIRE(compatible.module->contents.find(
              L"urn:siso:reference:SISO-REF-010-00v20-0") !=
          std::wstring::npos);
  REQUIRE(readBinary(source) == originalBytes);
}

TEST_CASE(
    "The RPR 2.0 family loads through the explicit 2010 compatibility path",
    "[unit][fom][xml-schema][external-corpus][ieee1516-2010][rpr][compatibility]") {
  std::array<std::filesystem::path, 16> const modules{{
      std::filesystem::path("RPR FOM v2.0 Link 16 and Link 11") / "RPR-Foundation_v2.0.xml",
      std::filesystem::path("RPR FOM v2.0 Link 16 and Link 11") / "RPR-Enumerations_v2.0.xml",
      std::filesystem::path("RPR FOM v2.0 Link 16 and Link 11") / "RPR-Base_v2.0.xml",
      std::filesystem::path("RPR FOM v2.0 Link 16 and Link 11") / "RPR-Physical_v2.0.xml",
      std::filesystem::path("RPR FOM v2.0 Link 16 and Link 11") / "RPR-Logistics_v2.0.xml",
      std::filesystem::path("RPR FOM v2.0 Link 16 and Link 11") / "RPR-Minefield_v2.0.xml",
      std::filesystem::path("RPR FOM v2.0 Link 16 and Link 11") / "RPR-Switches_v2.0.xml",
      std::filesystem::path("RPR FOM v2.0 Link 16 and Link 11") / "RPR-DER_v2.0.xml",
      std::filesystem::path("RPR FOM v2.0 Link 16 and Link 11") / "RPR-Warfare_v2.0.xml",
      std::filesystem::path("RPR FOM v2.0 Link 16 and Link 11") / "RPR-Communication_v2.0.xml",
      std::filesystem::path("RPR FOM v2.0 Link 16 and Link 11") / "RPR-SIMAN_v2.0.xml",
      std::filesystem::path("RPR FOM v2.0 Link 16 and Link 11") / "RPR-UA_v2.0.xml",
      std::filesystem::path("RPR FOM v2.0 Link 16 and Link 11") / "RPR-Aggregate_v2.0.xml",
      std::filesystem::path("RPR FOM v2.0 Link 16 and Link 11") / "Link_16_v2.0.xml",
      std::filesystem::path("RPR FOM v2.0 Link 16 and Link 11") / "Link_11_11B_v1.0.xml",
      std::filesystem::path("RPR FOM v2.0 Link 16 and Link 11") / "RPR-SE_v2.0.xml",
  }};

  std::vector<PrevalidatedFomModule> ordered;
  ordered.reserve(modules.size() + 1);
  ordered.push_back(validateStandardMim2010(FomSourceCompatibility::rpr_2010));
  for (auto const& relative : modules) {
    ordered.push_back(validate2010(
        externalCorpusPath(relative),
        relative.filename().wstring(),
        FomSourceCompatibility::rpr_2010));
  }

  auto composer = legacyComposer();
  auto result = composer.compose(ordered);
  CAPTURE(result.diagnostics, result.warnings.size());
  REQUIRE(result.status == FomCompositionStatus::valid);
  REQUIRE(result.catalog);
  REQUIRE_FALSE(result.fdd);
  REQUIRE(result.catalog->dataType("RPRboolean") != nullptr);
  REQUIRE(result.catalog->dataType("BreachedStatusEnum8") != nullptr);
  REQUIRE(result.modules.size() == 17);
  REQUIRE(result.warnings.size() == 1);

  // The RPR adapter exposes source labels and neutral shape metadata, and the
  // structural codec claim is now backed by the isolated byte-codec tests.
  auto const* objectId = result.catalog->dataType("RTIobjectId");
  REQUIRE(objectId != nullptr);
  REQUIRE(objectId->wireEncoding.sourceLabel == "RPRnullTerminatedArray");
  REQUIRE(objectId->wireEncoding.kind == FomWireEncodingKind::null_terminated_array);
  REQUIRE(objectId->wireEncoding.codecStatus == FomWireCodecStatus::available);
  REQUIRE(objectId->wireEncoding.sentinelTerminated);
  REQUIRE_FALSE(objectId->wireEncoding.extentSuppliedExternally);
  REQUIRE(objectId->elementDataType == "HLAASCIIchar");
  REQUIRE(objectId->cardinality == "Dynamic");

  auto const* objectIdArray = result.catalog->dataType("RTIobjectIdArray");
  REQUIRE(objectIdArray != nullptr);
  REQUIRE(objectIdArray->wireEncoding.kind == FomWireEncodingKind::variable_array);
  REQUIRE(objectIdArray->wireEncoding.codecStatus == FomWireCodecStatus::available);
  REQUIRE(objectIdArray->wireEncoding.elementCountInPayload);

  auto const* rprBoolean = result.catalog->dataType("RPRboolean");
  REQUIRE(rprBoolean != nullptr);
  REQUIRE(rprBoolean->representation == "HLAoctet");

  auto const* lengthless = result.catalog->dataType("WorldLocationStructLengthlessArray");
  REQUIRE(lengthless != nullptr);
  REQUIRE(lengthless->wireEncoding.kind == FomWireEncodingKind::lengthless_array);
  REQUIRE(lengthless->wireEncoding.codecStatus == FomWireCodecStatus::available);
  REQUIRE(lengthless->wireEncoding.extentSuppliedExternally);
  REQUIRE(lengthless->elementDataType == "WorldLocationStruct");

  auto const* padding32 = result.catalog->dataType("OctetPadding32Array");
  REQUIRE(padding32 != nullptr);
  REQUIRE(padding32->wireEncoding.kind == FomWireEncodingKind::alignment_padding_array);
  REQUIRE(padding32->wireEncoding.alignmentOctets == 4);
  REQUIRE(padding32->wireEncoding.codecStatus == FomWireCodecStatus::available);

  auto const* padding64 = result.catalog->dataType("OctetPadding64Array");
  REQUIRE(padding64 != nullptr);
  REQUIRE(padding64->wireEncoding.kind == FomWireEncodingKind::alignment_padding_array);
  REQUIRE(padding64->wireEncoding.alignmentOctets == 8);
  REQUIRE(padding64->wireEncoding.codecStatus == FomWireCodecStatus::available);

  auto const* extended = result.catalog->dataType("EnvironmentRecVariantStruct");
  REQUIRE(extended != nullptr);
  REQUIRE(extended->wireEncoding.sourceLabel == "RPRextendedVariantRecord");
  REQUIRE(extended->wireEncoding.kind == FomWireEncodingKind::extendable_variant_record);
  REQUIRE(extended->wireEncoding.codecStatus == FomWireCodecStatus::available);
  REQUIRE(extended->wireEncoding.extensionLengthInPayload);
  REQUIRE(extended->discriminantDataType == "EnvironmentRecordTypeEnum32");
  REQUIRE(extended->alternatives.size() == 19);
  REQUIRE(extended->alternatives.front().name == "Point1GeometryData");
  REQUIRE(extended->alternatives.front().dataType == "WorldLocationStruct");
  REQUIRE(extended->alternatives.front().discriminantEnumerators ==
          std::vector<std::string>{"PointRecord1Type"});

  auto const* entityType = result.catalog->dataType("EntityTypeStruct");
  REQUIRE(entityType != nullptr);
  REQUIRE(entityType->wireEncoding.kind == FomWireEncodingKind::fixed_record);
  REQUIRE(entityType->fields.size() == 7);
  REQUIRE(entityType->fields.front().name == "EntityKind");

  auto const* unsignedInteger = result.catalog->dataType("RPRunsignedInteger16BE");
  REQUIRE(unsignedInteger != nullptr);
  REQUIRE(unsignedInteger->wireEncoding.kind == FomWireEncodingKind::fixed_width);
  REQUIRE(unsignedInteger->wireEncoding.codecStatus == FomWireCodecStatus::available);
  REQUIRE(unsignedInteger->wireEncoding.sizeBits == 16);
  REQUIRE(unsignedInteger->wireEncoding.byteOrder == umbra::detail::FomByteOrder::big);

  auto const* unsignedRepresentation =
      result.catalog->dataType("NetworkParticipationGroupNumber");
  REQUIRE(unsignedRepresentation != nullptr);
  REQUIRE(unsignedRepresentation->representation == "RPRunsignedInteger16BE");
  REQUIRE(unsignedRepresentation->wireEncoding.sourceLabel ==
          "RPRunsignedInteger16BE");
  REQUIRE(unsignedRepresentation->wireEncoding.kind ==
          FomWireEncodingKind::fixed_width);
  REQUIRE(unsignedRepresentation->wireEncoding.codecStatus ==
          FomWireCodecStatus::available);
  REQUIRE(unsignedRepresentation->wireEncoding.sizeBits == 16U);
  REQUIRE(unsignedRepresentation->wireEncoding.byteOrder ==
          umbra::detail::FomByteOrder::big);

  auto const strictRpr = normalizeFomWireEncoding(
      "RPRlengthlessArray",
      FomSourceCompatibility::strict);
  REQUIRE(strictRpr.kind == FomWireEncodingKind::unrecognized);
  REQUIRE(strictRpr.codecStatus == FomWireCodecStatus::unsupported);
  auto const strictUnsigned = normalizeFomWireEncoding(
      "RPRunsignedInteger16BE",
      FomSourceCompatibility::strict);
  REQUIRE(strictUnsigned.kind == FomWireEncodingKind::unrecognized);
  REQUIRE(strictUnsigned.codecStatus == FomWireCodecStatus::unsupported);
}

TEST_CASE(
    "The 2025 API creates and registers objects from the full RPR 2.0 family",
    "[unit][fom][xml-schema][external-corpus][ieee1516-2010][rpr][object-registration]") {
  using rti1516_2025::AttributeHandleSet;
  using rti1516_2025::HLA_EVOKED;
  using rti1516_2025::RtiConfiguration;

  std::array<std::filesystem::path, 16> const modules{{
      std::filesystem::path("RPR FOM v2.0 Link 16 and Link 11") / "RPR-Foundation_v2.0.xml",
      std::filesystem::path("RPR FOM v2.0 Link 16 and Link 11") / "RPR-Enumerations_v2.0.xml",
      std::filesystem::path("RPR FOM v2.0 Link 16 and Link 11") / "RPR-Base_v2.0.xml",
      std::filesystem::path("RPR FOM v2.0 Link 16 and Link 11") / "RPR-Physical_v2.0.xml",
      std::filesystem::path("RPR FOM v2.0 Link 16 and Link 11") / "RPR-Logistics_v2.0.xml",
      std::filesystem::path("RPR FOM v2.0 Link 16 and Link 11") / "RPR-Minefield_v2.0.xml",
      std::filesystem::path("RPR FOM v2.0 Link 16 and Link 11") / "RPR-Switches_v2.0.xml",
      std::filesystem::path("RPR FOM v2.0 Link 16 and Link 11") / "RPR-DER_v2.0.xml",
      std::filesystem::path("RPR FOM v2.0 Link 16 and Link 11") / "RPR-Warfare_v2.0.xml",
      std::filesystem::path("RPR FOM v2.0 Link 16 and Link 11") / "RPR-Communication_v2.0.xml",
      std::filesystem::path("RPR FOM v2.0 Link 16 and Link 11") / "RPR-SIMAN_v2.0.xml",
      std::filesystem::path("RPR FOM v2.0 Link 16 and Link 11") / "RPR-UA_v2.0.xml",
      std::filesystem::path("RPR FOM v2.0 Link 16 and Link 11") / "RPR-Aggregate_v2.0.xml",
      std::filesystem::path("RPR FOM v2.0 Link 16 and Link 11") / "Link_16_v2.0.xml",
      std::filesystem::path("RPR FOM v2.0 Link 16 and Link 11") / "Link_11_11B_v1.0.xml",
      std::filesystem::path("RPR FOM v2.0 Link 16 and Link 11") / "RPR-SE_v2.0.xml",
  }};
  std::vector<std::wstring> fomModules;
  fomModules.reserve(modules.size());
  for (auto const& relative : modules) {
    fomModules.push_back(externalCorpusPath(relative).wstring());
  }

  rti1516_2025::NullFederateAmbassador callbacks;
  rti1516_2025::umbra_binding_detail::UmbraRtiAmbassador rti;
  auto configuration = RtiConfiguration::createConfiguration()
      .withAdditionalSettings(L"fomEdition=2010");
  REQUIRE_NOTHROW(rti.connect(callbacks, HLA_EVOKED, configuration));

  auto const federationName = L"UmbraExternalRpr2010";
  REQUIRE_NOTHROW(rti.createFederationExecution(
      federationName,
      fomModules,
      L"HLAinteger64Time"));
  REQUIRE_NOTHROW(rti.joinFederationExecution(L"rpr-registration-only", federationName));

  auto const physicalEntity = rti.getObjectClassHandle(
      L"HLAobjectRoot.BaseEntity.PhysicalEntity");
  auto const entityType = rti.getAttributeHandle(physicalEntity, L"EntityType");
  REQUIRE_NOTHROW(rti.publishObjectClassAttributes(
      physicalEntity,
      AttributeHandleSet{entityType}));

  rti1516_2025::ObjectInstanceHandle entity;
  REQUIRE_NOTHROW(entity = rti.registerObjectInstance(physicalEntity));
  REQUIRE_FALSE(rti.getObjectInstanceName(entity).empty());
  REQUIRE(rti.getKnownObjectClassHandle(entity) == physicalEntity);

  REQUIRE_NOTHROW(rti.resignFederationExecution(
      rti1516_2025::CANCEL_THEN_DELETE_THEN_DIVEST));
  REQUIRE_NOTHROW(rti.destroyFederationExecution(federationName));
  REQUIRE_NOTHROW(rti.disconnect());
}

#ifdef UMBRA_ENABLE_EMBEDDED_FEDERATION_MANAGEMENT
TEST_CASE(
    "The 2025 API delivers RPR custom payloads through a two-federate exchange",
    "[integration][development-profile][fom][xml-schema][external-corpus]"
    "[ieee1516-2010][rpr][payload-delivery][simulation]") {
  using Ambassador = rti1516_2025::umbra_binding_detail::UmbraRtiAmbassador;
  using rti1516_2025::AttributeHandleSet;
  using rti1516_2025::AttributeHandleValueMap;
  using rti1516_2025::HLA_EVOKED;
  using rti1516_2025::RtiConfiguration;

  std::array<std::filesystem::path, 16> const modules{{
      std::filesystem::path("RPR FOM v2.0 Link 16 and Link 11") / "RPR-Foundation_v2.0.xml",
      std::filesystem::path("RPR FOM v2.0 Link 16 and Link 11") / "RPR-Enumerations_v2.0.xml",
      std::filesystem::path("RPR FOM v2.0 Link 16 and Link 11") / "RPR-Base_v2.0.xml",
      std::filesystem::path("RPR FOM v2.0 Link 16 and Link 11") / "RPR-Physical_v2.0.xml",
      std::filesystem::path("RPR FOM v2.0 Link 16 and Link 11") / "RPR-Logistics_v2.0.xml",
      std::filesystem::path("RPR FOM v2.0 Link 16 and Link 11") / "RPR-Minefield_v2.0.xml",
      std::filesystem::path("RPR FOM v2.0 Link 16 and Link 11") / "RPR-Switches_v2.0.xml",
      std::filesystem::path("RPR FOM v2.0 Link 16 and Link 11") / "RPR-DER_v2.0.xml",
      std::filesystem::path("RPR FOM v2.0 Link 16 and Link 11") / "RPR-Warfare_v2.0.xml",
      std::filesystem::path("RPR FOM v2.0 Link 16 and Link 11") / "RPR-Communication_v2.0.xml",
      std::filesystem::path("RPR FOM v2.0 Link 16 and Link 11") / "RPR-SIMAN_v2.0.xml",
      std::filesystem::path("RPR FOM v2.0 Link 16 and Link 11") / "RPR-UA_v2.0.xml",
      std::filesystem::path("RPR FOM v2.0 Link 16 and Link 11") / "RPR-Aggregate_v2.0.xml",
      std::filesystem::path("RPR FOM v2.0 Link 16 and Link 11") / "Link_16_v2.0.xml",
      std::filesystem::path("RPR FOM v2.0 Link 16 and Link 11") / "Link_11_11B_v1.0.xml",
      std::filesystem::path("RPR FOM v2.0 Link 16 and Link 11") / "RPR-SE_v2.0.xml",
  }};
  std::vector<std::wstring> fomModules;
  fomModules.reserve(modules.size());
  for (auto const& relative : modules) {
    fomModules.push_back(externalCorpusPath(relative).wstring());
  }

  rti1516_2025::NullFederateAmbassador producerCallbacks;
  RprPayloadFederateAmbassador consumerCallbacks;
  Ambassador producer;
  Ambassador consumer;

  struct Cleanup final {
    Ambassador& producer;
    Ambassador& consumer;
    std::wstring federationName;
    bool producerConnected = false;
    bool consumerConnected = false;
    bool federationCreated = false;
    bool producerJoined = false;
    bool consumerJoined = false;

    ~Cleanup() noexcept {
      if (consumerJoined) {
        try {
          consumer.resignFederationExecution(
              rti1516_2025::CANCEL_THEN_DELETE_THEN_DIVEST);
        } catch (...) {
        }
      }
      if (producerJoined) {
        try {
          producer.resignFederationExecution(
              rti1516_2025::CANCEL_THEN_DELETE_THEN_DIVEST);
        } catch (...) {
        }
      }
      if (federationCreated) {
        try {
          producer.destroyFederationExecution(federationName);
        } catch (...) {
        }
      }
      if (consumerConnected) {
        try {
          consumer.disconnect();
        } catch (...) {
        }
      }
      if (producerConnected) {
        try {
          producer.disconnect();
        } catch (...) {
        }
      }
    }
  } cleanup{producer, consumer, nextRprFederationName()};

  auto const configuration = RtiConfiguration::createConfiguration()
      .withAdditionalSettings(L"fomEdition=2010");
  REQUIRE_NOTHROW(producer.connect(producerCallbacks, HLA_EVOKED, configuration));
  cleanup.producerConnected = true;
  REQUIRE_NOTHROW(consumer.connect(consumerCallbacks, HLA_EVOKED, configuration));
  cleanup.consumerConnected = true;
  REQUIRE_NOTHROW(producer.createFederationExecution(
      cleanup.federationName,
      fomModules,
      L"HLAinteger64Time"));
  cleanup.federationCreated = true;
  REQUIRE_NOTHROW(producer.joinFederationExecution(
      L"rpr-payload-producer",
      cleanup.federationName));
  cleanup.producerJoined = true;
  REQUIRE_NOTHROW(consumer.joinFederationExecution(
      L"rpr-payload-consumer",
      cleanup.federationName));
  cleanup.consumerJoined = true;

  auto const producerClass = producer.getObjectClassHandle(
      L"HLAobjectRoot.EmbeddedSystem.RadioTransmitter");
  auto const consumerClass = consumer.getObjectClassHandle(
      L"HLAobjectRoot.EmbeddedSystem.RadioTransmitter");
  REQUIRE(producerClass == consumerClass);
  auto const producerHostObjectIdentifier = producer.getAttributeHandle(
      producerClass,
      L"HostObjectIdentifier");
  auto const producerTimeHopInUse = producer.getAttributeHandle(
      producerClass,
      L"TimeHopInUse");
  auto const producerStreamTag = producer.getAttributeHandle(
      producerClass,
      L"StreamTag");
  auto const consumerHostObjectIdentifier = consumer.getAttributeHandle(
      consumerClass,
      L"HostObjectIdentifier");
  auto const consumerTimeHopInUse = consumer.getAttributeHandle(
      consumerClass,
      L"TimeHopInUse");
  auto const consumerStreamTag = consumer.getAttributeHandle(
      consumerClass,
      L"StreamTag");
  REQUIRE(producerHostObjectIdentifier == consumerHostObjectIdentifier);
  REQUIRE(producerTimeHopInUse == consumerTimeHopInUse);
  REQUIRE(producerStreamTag == consumerStreamTag);

  REQUIRE_NOTHROW(consumer.subscribeObjectClassAttributes(
      consumerClass,
      AttributeHandleSet{
          consumerHostObjectIdentifier,
          consumerTimeHopInUse,
          consumerStreamTag}));
  REQUIRE_NOTHROW(producer.publishObjectClassAttributes(
      producerClass,
      AttributeHandleSet{
          producerHostObjectIdentifier,
          producerTimeHopInUse,
          producerStreamTag}));

  rti1516_2025::ObjectInstanceHandle object;
  REQUIRE_NOTHROW(object = producer.registerObjectInstance(producerClass));

  auto const drainConsumer = [&consumer] {
    while (consumer.evokeCallback(0.0)) {
    }
  };
  drainConsumer();
  REQUIRE(consumerCallbacks.discoveries.size() == 1U);
  REQUIRE(consumerCallbacks.discoveries.front().objectInstance == object);
  REQUIRE(consumerCallbacks.discoveries.front().objectClass == consumerClass);
  REQUIRE_FALSE(consumerCallbacks.discoveries.front().objectInstanceName.empty());

  umbra::detail::FomWireBytes const hostIdentifierText{
      static_cast<std::uint8_t>('H'),
      static_cast<std::uint8_t>('O'),
      static_cast<std::uint8_t>('S'),
      static_cast<std::uint8_t>('T'),
      static_cast<std::uint8_t>('-'),
      static_cast<std::uint8_t>('0'),
      static_cast<std::uint8_t>('0'),
      static_cast<std::uint8_t>('1'),
  };
  auto const hostIdentifierWire =
      encodeRprNullTerminatedArray(hostIdentifierText);
  umbra::detail::FomWireBytes const rprBooleanWire{0x01U};
  auto const streamTagValue = 0x0123456789abcdefULL;
  auto const streamTagWire = umbra::detail::encodeRprUnsignedInteger(
      streamTagValue,
      64U);
  AttributeHandleValueMap values;
  values.emplace(
      producerHostObjectIdentifier,
      rti1516_2025::VariableLengthData(
          hostIdentifierWire.data(),
          hostIdentifierWire.size()));
  values.emplace(
      producerTimeHopInUse,
      rti1516_2025::VariableLengthData(
          rprBooleanWire.data(),
          rprBooleanWire.size()));
  values.emplace(
      producerStreamTag,
      rti1516_2025::VariableLengthData(
          streamTagWire.data(),
          streamTagWire.size()));
  std::vector<unsigned char> const updateTagBytes{
      0x52U,
      0x50U,
      0x52U,
      0x01U};
  rti1516_2025::VariableLengthData const updateTag(
      updateTagBytes.data(),
      updateTagBytes.size());
  REQUIRE_NOTHROW(producer.updateAttributeValues(object, values, updateTag));

  drainConsumer();
  REQUIRE(consumerCallbacks.reflections.size() == 1U);
  auto const& reflection = consumerCallbacks.reflections.front();
  REQUIRE(reflection.objectInstance == object);
  REQUIRE(reflection.attributeValues.size() == 3U);
  auto const reflectedHost = reflection.attributeValues.at(
      consumerHostObjectIdentifier);
  auto const reflectedBoolean = reflection.attributeValues.at(
      consumerTimeHopInUse);
  auto const reflectedStreamTag = reflection.attributeValues.at(
      consumerStreamTag);
  auto const bytesOf = [](rti1516_2025::VariableLengthData const& value) {
    auto const* data = static_cast<unsigned char const*>(value.data());
    if (data == nullptr) {
      return umbra::detail::FomWireBytes{};
    }
    return umbra::detail::FomWireBytes(data, data + value.size());
  };
  REQUIRE(bytesOf(reflectedHost) == hostIdentifierWire);
  REQUIRE(bytesOf(reflectedBoolean) == rprBooleanWire);
  REQUIRE(bytesOf(reflectedStreamTag) == streamTagWire);
  REQUIRE(bytesOf(reflection.userSuppliedTag) ==
          umbra::detail::FomWireBytes(
              updateTagBytes.begin(),
              updateTagBytes.end()));

  auto const decodedHost = decodeRprNullTerminatedArray(bytesOf(reflectedHost));
  REQUIRE(decodedHost.value == hostIdentifierText);
  REQUIRE(decodedHost.nextOffset == hostIdentifierWire.size());
  auto const decodedStreamTag = umbra::detail::decodeRprUnsignedInteger(
      bytesOf(reflectedStreamTag),
      0U,
      64U);
  REQUIRE(decodedStreamTag.value == streamTagValue);
  REQUIRE(decodedStreamTag.nextOffset == streamTagWire.size());
}
#endif
#endif
