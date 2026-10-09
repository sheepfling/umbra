#include <catch2/catch_test_macros.hpp>

#include <filesystem>

#include "internal/fom/fom_catalog.hpp"
#include "fom_composer_test_support.hpp"

using umbra::detail::FomCompositionStatus;
using umbra::detail::FomModuleKind;
TEST_CASE(
    "The FOM composition preflight resolves data-type references after the complete module set is merged",
    "[unit][fom][composition][reference-resolution]") {
  auto const testData = std::filesystem::path(UMBRA_SOURCE_DIRECTORY) / "cpp" / "tests" / "data";
  auto mim = validated(
      resourcePath("mim/HLAstandardMIM-2025.xml"),
      FomModuleKind::mim,
      L"urn:umbra:test:mim");
  auto consumer = validated(
      testData / "data-type-reference-consumer-fom.xml",
      FomModuleKind::fom,
      L"urn:umbra:test:data-type-consumer");
  auto provider = validated(
      testData / "data-type-reference-provider-fom.xml",
      FomModuleKind::fom,
      L"urn:umbra:test:data-type-provider");
  auto unresolved = validated(
      testData / "unresolved-data-type-reference-fom.xml",
      FomModuleKind::fom,
      L"urn:umbra:test:unresolved-data-type");

  auto materializer = composer();
  auto resolved = materializer.compose({mim, consumer, provider});
  CAPTURE(resolved.diagnostics);
  REQUIRE(resolved.status == FomCompositionStatus::valid);
  REQUIRE(resolved.catalog);
  REQUIRE(resolved.fdd);
  REQUIRE(resolved.catalog->dataType("UmbraReferenceFixtureArray") != nullptr);
  REQUIRE(resolved.catalog->dataType("UmbraReferenceFixtureValue") != nullptr);

  auto missing = materializer.compose({mim, unresolved});
  REQUIRE(missing.status == FomCompositionStatus::inconsistent_modules);
  REQUIRE(missing.diagnostics.find("UmbraMissingReferenceFixtureValue") != std::string::npos);
  REQUIRE(missing.diagnostics.find("not declared") != std::string::npos);
}

TEST_CASE(
    "The FOM composition preflight resolves reference-data classes after the complete module set is merged",
    "[unit][fom][composition][reference-resolution]") {
  auto const testData = std::filesystem::path(UMBRA_SOURCE_DIRECTORY) / "cpp" / "tests" / "data";
  auto mim = validated(
      resourcePath("mim/HLAstandardMIM-2025.xml"),
      FomModuleKind::mim,
      L"urn:umbra:test:mim");
  auto consumer = validated(
      testData / "reference-data-class-consumer-fom.xml",
      FomModuleKind::fom,
      L"urn:umbra:test:reference-data-consumer");
  auto provider = validated(
      testData / "reference-data-class-provider-fom.xml",
      FomModuleKind::fom,
      L"urn:umbra:test:reference-data-provider");
  auto unresolved = validated(
      testData / "unresolved-reference-data-class-fom.xml",
      FomModuleKind::fom,
      L"urn:umbra:test:unresolved-reference-data-class");

  auto materializer = composer();
  auto resolved = materializer.compose({mim, consumer, provider});
  CAPTURE(resolved.diagnostics);
  REQUIRE(resolved.status == FomCompositionStatus::valid);
  REQUIRE(resolved.catalog);
  REQUIRE(resolved.fdd);
  REQUIRE(resolved.catalog->objectClass("HLAobjectRoot.UmbraReferenceFixtureClass") != nullptr);
  REQUIRE(resolved.catalog->dataType("UmbraReferenceFixture") != nullptr);

  auto missing = materializer.compose({mim, unresolved});
  REQUIRE(missing.status == FomCompositionStatus::inconsistent_modules);
  REQUIRE(missing.diagnostics.find("UmbraMissingReferenceFixtureClass") != std::string::npos);
  REQUIRE(missing.diagnostics.find("not declared") != std::string::npos);
}

TEST_CASE(
    "The FOM composition preflight resolves reference-data attributes and representations",
    "[unit][fom][composition][reference-resolution]") {
  auto const testData = std::filesystem::path(UMBRA_SOURCE_DIRECTORY) / "cpp" / "tests" / "data";
  auto mim = validated(
      resourcePath("mim/HLAstandardMIM-2025.xml"),
      FomModuleKind::mim,
      L"urn:umbra:test:mim");
  auto consumer = validated(
      testData / "reference-data-attribute-consumer-fom.xml",
      FomModuleKind::fom,
      L"urn:umbra:test:reference-data-attribute-consumer");
  auto provider = validated(
      testData / "reference-data-attribute-provider-fom.xml",
      FomModuleKind::fom,
      L"urn:umbra:test:reference-data-attribute-provider");
  auto missingAttribute = validated(
      testData / "unresolved-reference-data-attribute-fom.xml",
      FomModuleKind::fom,
      L"urn:umbra:test:unresolved-reference-data-attribute");
  auto mismatchedRepresentation = validated(
      testData / "mismatched-reference-data-representation-fom.xml",
      FomModuleKind::fom,
      L"urn:umbra:test:mismatched-reference-data-representation");

  auto materializer = composer();
  auto resolved = materializer.compose({mim, consumer, provider});
  CAPTURE(resolved.diagnostics);
  REQUIRE(resolved.status == FomCompositionStatus::valid);
  REQUIRE(resolved.catalog);
  REQUIRE(resolved.fdd);
  REQUIRE(
      resolved.catalog->objectClass(
          "HLAobjectRoot.UmbraAttributeFixtureBase.UmbraAttributeFixtureClass") != nullptr);
  REQUIRE(resolved.catalog->dataType("UmbraAttributeReferenceFixture") != nullptr);

  auto missing = materializer.compose({mim, missingAttribute, provider});
  REQUIRE(missing.status == FomCompositionStatus::inconsistent_modules);
  REQUIRE(missing.diagnostics.find("UmbraMissingAttributeFixture") != std::string::npos);
  REQUIRE(missing.diagnostics.find("not declared") != std::string::npos);

  auto mismatched = materializer.compose({mim, mismatchedRepresentation, provider});
  REQUIRE(mismatched.status == FomCompositionStatus::inconsistent_modules);
  REQUIRE(mismatched.diagnostics.find("HLAinteger64Time") != std::string::npos);
  REQUIRE(mismatched.diagnostics.find("does not match") != std::string::npos);
}

TEST_CASE(
    "The FOM composition preflight resolves representations and special instance identifiers",
    "[unit][fom][composition][reference-resolution]") {
  auto const testData = std::filesystem::path(UMBRA_SOURCE_DIRECTORY) / "cpp" / "tests" / "data";
  auto mim = validated(
      resourcePath("mim/HLAstandardMIM-2025.xml"),
      FomModuleKind::mim,
      L"urn:umbra:test:mim");
  auto unresolvedSimple = validated(
      testData / "unresolved-simple-representation-fom.xml",
      FomModuleKind::fom,
      L"urn:umbra:test:unresolved-simple-representation");
  auto unresolvedEnumerated = validated(
      testData / "unresolved-enumerated-representation-fom.xml",
      FomModuleKind::fom,
      L"urn:umbra:test:unresolved-enumerated-representation");
  auto specialIdentifiers = validated(
      testData / "special-instance-identifier-reference-fom.xml",
      FomModuleKind::fom,
      L"urn:umbra:test:special-instance-identifiers");
  auto invalidSpecialIdentifier = validated(
      testData / "invalid-special-instance-identifier-reference-fom.xml",
      FomModuleKind::fom,
      L"urn:umbra:test:invalid-special-instance-identifier");
  auto invalidBasicReference = validated(
      testData / "invalid-basic-reference-representation-fom.xml",
      FomModuleKind::fom,
      L"urn:umbra:test:invalid-basic-reference-representation");
  auto attributeProvider = validated(
      testData / "reference-data-attribute-provider-fom.xml",
      FomModuleKind::fom,
      L"urn:umbra:test:reference-data-attribute-provider");

  auto materializer = composer();
  auto resolved = materializer.compose({mim, specialIdentifiers});
  CAPTURE(resolved.diagnostics);
  REQUIRE(resolved.status == FomCompositionStatus::valid);
  REQUIRE(resolved.catalog);
  REQUIRE(resolved.fdd);
  REQUIRE(resolved.catalog->dataType("UmbraObjectInstanceNameReference") != nullptr);
  REQUIRE(resolved.catalog->dataType("UmbraObjectInstanceHandleReference") != nullptr);

  auto missingSimple = materializer.compose({mim, unresolvedSimple});
  REQUIRE(missingSimple.status == FomCompositionStatus::inconsistent_modules);
  REQUIRE(missingSimple.diagnostics.find("UmbraMissingBasicRepresentation") != std::string::npos);
  REQUIRE(missingSimple.diagnostics.find("not declared") != std::string::npos);

  auto missingEnumerated = materializer.compose({mim, unresolvedEnumerated});
  REQUIRE(missingEnumerated.status == FomCompositionStatus::inconsistent_modules);
  REQUIRE(
      missingEnumerated.diagnostics.find("UmbraMissingDiscreteRepresentation") !=
      std::string::npos);
  REQUIRE(missingEnumerated.diagnostics.find("not declared") != std::string::npos);

  auto invalidSpecial = materializer.compose({mim, invalidSpecialIdentifier});
  REQUIRE(invalidSpecial.status == FomCompositionStatus::inconsistent_modules);
  REQUIRE(invalidSpecial.diagnostics.find("HLAunicodeString") != std::string::npos);
  REQUIRE(invalidSpecial.diagnostics.find("HLAobjectInstanceName") != std::string::npos);

  auto invalidBasic = materializer.compose({mim, invalidBasicReference, attributeProvider});
  REQUIRE(invalidBasic.status == FomCompositionStatus::inconsistent_modules);
  REQUIRE(invalidBasic.diagnostics.find("HLAinteger32BE") != std::string::npos);
  REQUIRE(invalidBasic.diagnostics.find("must name a simple") != std::string::npos);
}

TEST_CASE(
    "The FOM composition preflight resolves directed interactions and retains sharing metadata",
    "[unit][fom][composition][reference-resolution][directed-interaction-sharing]") {
  auto const testData = std::filesystem::path(UMBRA_SOURCE_DIRECTORY) / "cpp" / "tests" / "data";
  auto mim = validated(
      resourcePath("mim/HLAstandardMIM-2025.xml"),
      FomModuleKind::mim,
      L"urn:umbra:test:mim");
  auto objectConsumer = validated(
      testData / "directed-interaction-object-consumer-fom.xml",
      FomModuleKind::fom,
      L"urn:umbra:test:directed-interaction-object-consumer");
  auto interactionProvider = validated(
      testData / "directed-interaction-interaction-provider-fom.xml",
      FomModuleKind::fom,
      L"urn:umbra:test:directed-interaction-interaction-provider");
  auto unresolved = validated(
      testData / "unresolved-directed-interaction-fom.xml",
      FomModuleKind::fom,
      L"urn:umbra:test:unresolved-directed-interaction");

  auto materializer = composer();
  auto resolved = materializer.compose({mim, objectConsumer, interactionProvider});
  CAPTURE(resolved.diagnostics);
  REQUIRE(resolved.status == FomCompositionStatus::valid);
  REQUIRE(resolved.catalog);
  REQUIRE(resolved.fdd);
  auto const* objectClass = resolved.catalog->objectClass("HLAobjectRoot.UmbraDirectedFixtureObject");
  REQUIRE(objectClass != nullptr);
  REQUIRE(objectClass->sharing == "PublishSubscribe");
  REQUIRE(objectClass->directedInteractions.size() == 1);
  auto const& directedInteraction = objectClass->directedInteractions.front();
  REQUIRE(
      directedInteraction.interactionClassName ==
      "HLAinteractionRoot.UmbraDirectedFixtureInteraction");
  REQUIRE(directedInteraction.sharing == "PublishSubscribe");
  REQUIRE(
      objectClass->directedInteraction("HLAinteractionRoot.UmbraDirectedFixtureInteraction") ==
      &directedInteraction);
  auto const* interactionClass =
      resolved.catalog->interactionClass("HLAinteractionRoot.UmbraDirectedFixtureInteraction");
  REQUIRE(interactionClass != nullptr);
  REQUIRE(interactionClass->sharing == "PublishSubscribe");
  REQUIRE(interactionClass->semantics == "Fixture interaction for directed-reference resolution.");

  auto missing = materializer.compose({mim, unresolved});
  REQUIRE(missing.status == FomCompositionStatus::inconsistent_modules);
  REQUIRE(missing.diagnostics.find("HLAinteractionRoot.UmbraMissingDirectedFixtureInteraction") != std::string::npos);
  REQUIRE(missing.diagnostics.find("not declared") != std::string::npos);
}

TEST_CASE(
    "The FDD materializer surfaces the multiple-directed-class schema conflict",
    "[unit][fom][composition][reference-resolution][schema-conflict]"
    "[directed-interaction-multiple-subscription-kinds]") {
  auto const testData = std::filesystem::path(UMBRA_SOURCE_DIRECTORY) / "cpp" / "tests" / "data";
  auto mim = validated(
      resourcePath("mim/HLAstandardMIM-2025.xml"),
      FomModuleKind::mim,
      L"urn:umbra:test:mim");
  auto objectConsumer = validated(
      testData / "directed-interaction-selector-matrix-object-fom.xml",
      FomModuleKind::fom,
      L"urn:umbra:test:directed-selector-matrix-object");
  auto interactionProvider = validated(
      testData / "directed-interaction-selector-matrix-interaction-fom.xml",
      FomModuleKind::fom,
      L"urn:umbra:test:directed-selector-matrix-interactions");

  auto materializer = composer();
  auto result = materializer.compose({mim, objectConsumer, interactionProvider});
  CAPTURE(result.diagnostics);
  // The two source modules are independently validated by the official DIF
  // schema above. The composed FDD cannot be emitted because the supplied
  // FDD schema caps directedInteraction at one occurrence, although the DIF
  // schema permits an unbounded list and §5.12 defines per-class selector
  // modes. Keep the conflict visible rather than dropping one declaration.
  REQUIRE(result.status == FomCompositionStatus::inconsistent_modules);
  REQUIRE(result.diagnostics.find("directedInteraction") != std::string::npos);
  REQUIRE(result.diagnostics.find("not expected") != std::string::npos);
}

TEST_CASE(
    "The FOM composition preflight resolves available dimensions after the complete module set is merged",
    "[unit][fom][composition][reference-resolution]") {
  auto const testData = std::filesystem::path(UMBRA_SOURCE_DIRECTORY) / "cpp" / "tests" / "data";
  auto mim = validated(
      resourcePath("mim/HLAstandardMIM-2025.xml"),
      FomModuleKind::mim,
      L"urn:umbra:test:mim");
  auto consumer = validated(
      testData / "dimension-reference-consumer-fom.xml",
      FomModuleKind::fom,
      L"urn:umbra:test:dimension-reference-consumer");
  auto provider = validated(
      testData / "dimension-reference-provider-fom.xml",
      FomModuleKind::fom,
      L"urn:umbra:test:dimension-reference-provider");
  auto unresolved = validated(
      testData / "unresolved-dimension-reference-fom.xml",
      FomModuleKind::fom,
      L"urn:umbra:test:unresolved-dimension-reference");

  auto materializer = composer();
  auto resolved = materializer.compose({mim, consumer, provider});
  CAPTURE(resolved.diagnostics);
  REQUIRE(resolved.status == FomCompositionStatus::valid);
  REQUIRE(resolved.catalog);
  REQUIRE(resolved.fdd);
  REQUIRE(resolved.catalog->dimension("UmbraDimensionFixture") != nullptr);
  REQUIRE(resolved.catalog->objectClass("HLAobjectRoot.UmbraDimensionFixtureObject") != nullptr);
  REQUIRE(
      resolved.catalog->interactionClass("HLAinteractionRoot.UmbraDimensionFixtureInteraction") !=
      nullptr);

  auto missing = materializer.compose({mim, unresolved});
  REQUIRE(missing.status == FomCompositionStatus::inconsistent_modules);
  REQUIRE(missing.diagnostics.find("UmbraMissingDimensionFixture") != std::string::npos);
  REQUIRE(missing.diagnostics.find("not declared") != std::string::npos);
}

TEST_CASE(
    "The FOM composition preflight resolves transportation names after the complete module set is merged",
    "[unit][fom][composition][reference-resolution]") {
  auto const testData = std::filesystem::path(UMBRA_SOURCE_DIRECTORY) / "cpp" / "tests" / "data";
  auto mim = validated(
      resourcePath("mim/HLAstandardMIM-2025.xml"),
      FomModuleKind::mim,
      L"urn:umbra:test:mim");
  auto consumer = validated(
      testData / "transportation-reference-consumer-fom.xml",
      FomModuleKind::fom,
      L"urn:umbra:test:transportation-reference-consumer");
  auto provider = validated(
      testData / "transportation-reference-provider-fom.xml",
      FomModuleKind::fom,
      L"urn:umbra:test:transportation-reference-provider");
  auto unresolved = validated(
      testData / "unresolved-transportation-reference-fom.xml",
      FomModuleKind::fom,
      L"urn:umbra:test:unresolved-transportation-reference");

  auto materializer = composer();
  auto resolved = materializer.compose({mim, consumer, provider});
  CAPTURE(resolved.diagnostics);
  REQUIRE(resolved.status == FomCompositionStatus::valid);
  REQUIRE(resolved.catalog);
  REQUIRE(resolved.fdd);
  auto const* objectClass =
      resolved.catalog->objectClass("HLAobjectRoot.UmbraTransportationFixtureObject");
  REQUIRE(objectClass != nullptr);
  REQUIRE(objectClass->declaredAttributes.contains("UmbraTransportationFixtureAttribute"));
  REQUIRE(
      objectClass->declaredAttributes.at("UmbraTransportationFixtureAttribute").transportation ==
      "UmbraTransportationFixture");
  auto const* interactionClass =
      resolved.catalog->interactionClass("HLAinteractionRoot.UmbraTransportationFixtureInteraction");
  REQUIRE(interactionClass != nullptr);
  REQUIRE(interactionClass->transportation == "UmbraTransportationFixture");

  auto missing = materializer.compose({mim, unresolved});
  REQUIRE(missing.status == FomCompositionStatus::inconsistent_modules);
  REQUIRE(missing.diagnostics.find("UmbraMissingTransportationFixture") != std::string::npos);
  REQUIRE(missing.diagnostics.find("not declared") != std::string::npos);
}
