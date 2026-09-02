#include <catch2/catch_test_macros.hpp>

#include "internal/federation/federation_management_coordinator.hpp"
#include "internal/fom/fdd_document.hpp"
#include "internal/fom/fom_catalog.hpp"
#include "internal/fom/libxml2_fom_composer.hpp"
#include "internal/fom/libxml2_fom_validator.hpp"
#include "internal/runtime/umbra_rti_ambassador.hpp"

#include <atomic>
#include <exception>
#include <filesystem>
#include <optional>
#include <set>
#include <string>
#include <vector>

#include <RTI/NullFederateAmbassador.h>
#include <RTI/RTI1516.h>

#ifndef UMBRA_SOURCE_DIRECTORY
#error "UMBRA_SOURCE_DIRECTORY must be defined for external FOM tests."
#endif

#ifndef UMBRA_EXTERNAL_2010_FOM_RESOURCE_DIRECTORY
#error "The 2010 FOM tests require an external IEEE 1516.2-2010 resource root."
#endif

namespace {

using umbra::detail::FomCompositionStatus;
using umbra::detail::FomModuleKind;
using umbra::detail::FomStandardEdition;
using umbra::detail::FomValidationStatus;
using umbra::detail::LibXml2FomModuleComposer;
using umbra::detail::LibXml2FomValidator;
using umbra::detail::PrevalidatedFomModule;

std::filesystem::path resourceRoot() {
  return std::filesystem::path(UMBRA_EXTERNAL_2010_FOM_RESOURCE_DIRECTORY);
}

std::filesystem::path legacyResource(std::filesystem::path const& compactRelative) {
  auto const compact = resourceRoot() / compactRelative;
  if (std::filesystem::is_regular_file(compact)) {
    return compact;
  }

  // The sibling CERTI corpus keeps 1516.1-owned MIM/FDD files beside the
  // 1516.2 DIF and example modules. Keep this compatibility lookup local to
  // the external test lane; production callers still supply their FOM path.
  if (compactRelative == std::filesystem::path("mim/HLAstandardMIM-2010.xml")) {
    return resourceRoot() / "1516_1-2010" / "HLAstandardMIM.xml";
  }
  if (compactRelative == std::filesystem::path("schemas/IEEE1516-DIF-2010.xsd")) {
    return resourceRoot() / "1516_2-2010" / "IEEE1516-DIF-2010.xsd";
  }
  if (compactRelative == std::filesystem::path("schemas/IEEE1516-FDD-2010.xsd")) {
    return resourceRoot() / "1516_1-2010" / "IEEE1516-FDD-2010.xsd";
  }
  if (compactRelative == std::filesystem::path("examples/RestaurantFOMmodule-2010.xml")) {
    return resourceRoot() / "1516_2-2010" / "RestaurantFOMmodule.xml";
  }
  return compact;
}

PrevalidatedFomModule validated(
    std::filesystem::path const& source,
    FomModuleKind kind,
    std::wstring designator,
    FomStandardEdition edition = FomStandardEdition::ieee1516_2010) {
  LibXml2FomValidator validator;
  auto result = validator.validate({
      source,
      legacyResource("schemas/IEEE1516-DIF-2010.xsd"),
      kind,
      std::move(designator),
      L"IEEE1516-DIF-2010.xsd",
      edition,
  });
  CAPTURE(source.string(), result.diagnostics);
  REQUIRE(result.status == FomValidationStatus::valid);
  REQUIRE(result.module.has_value());
  return *result.module;
}

std::filesystem::path restaurantFom() {
  return legacyResource("examples/RestaurantFOMmodule-2010.xml");
}

#ifdef UMBRA_EXTERNAL_TARGET_RADAR_FOM_PATH
std::filesystem::path targetRadarFom() {
  return std::filesystem::path(UMBRA_EXTERNAL_TARGET_RADAR_FOM_PATH);
}
#endif

std::filesystem::path standardMim() {
  return legacyResource("mim/HLAstandardMIM-2010.xml");
}

std::atomic_uint64_t federationSequence{0};

std::wstring nextFederationName() {
  return L"UmbraExternal2010Fom-" + std::to_wstring(++federationSequence);
}

}  // namespace

TEST_CASE(
    "The external IEEE 1516-2010 FOM lane validates and composes an ordered model",
    "[unit][external][fom][ieee1516-2010][composition]") {
  auto const mim = validated(standardMim(), FomModuleKind::mim, L"HLAstandardMIM");
  auto const fom = validated(restaurantFom(), FomModuleKind::fom, L"RestaurantFOMmodule");

  REQUIRE(mim.standardEdition == FomStandardEdition::ieee1516_2010);
  REQUIRE(fom.standardEdition == FomStandardEdition::ieee1516_2010);

  LibXml2FomModuleComposer composer(
      legacyResource("schemas/IEEE1516-FDD-2025.xsd"),
      legacyResource("schemas/IEEE1516-FDD-2010.xsd"));
  auto result = composer.compose({mim, fom});

  CAPTURE(result.diagnostics);
  REQUIRE(result.status == FomCompositionStatus::valid);
  REQUIRE(result.catalog);
  REQUIRE_FALSE(result.fdd);
  REQUIRE(result.catalog->objectClass("HLAobjectRoot.Employee.Waiter") != nullptr);
  REQUIRE(result.catalog->objectClass("HLAobjectRoot.Food.Drink.Soda") != nullptr);
  REQUIRE(result.catalog->interactionClass("HLAinteractionRoot.CustomerTransactions") != nullptr);
  REQUIRE(result.catalog->dataType("HLAunicodeString") != nullptr);
}

TEST_CASE(
    "The selected 2025 mode rejects an IEEE 1516-2010 module",
    "[unit][external][fom][ieee1516-2010][edition-policy]") {
  LibXml2FomValidator validator;
  auto const modernResource = [](std::filesystem::path const& relative) {
    return std::filesystem::path(UMBRA_SOURCE_DIRECTORY) / "third_party" /
        "ieee1516.2-2025" / "resources" / relative;
  };
  auto result = validator.validate({
      restaurantFom(),
      modernResource("schemas/IEEE1516-DIF-2025.xsd"),
      FomModuleKind::fom,
      L"RestaurantFOMmodule",
      L"IEEE1516-DIF-2025.xsd",
      FomStandardEdition::ieee1516_2025,
  });

  CAPTURE(result.diagnostics);
  REQUIRE(result.status == FomValidationStatus::invalid_model);
  REQUIRE_FALSE(result.module.has_value());
}

TEST_CASE(
    "The composer rejects mixed IEEE 1516 model editions",
    "[unit][external][fom][edition-policy]") {
  auto const legacyFom = validated(restaurantFom(), FomModuleKind::fom, L"legacy");
  auto const modernFom = [&] {
    LibXml2FomValidator validator;
    auto result = validator.validate({
        std::filesystem::path(UMBRA_SOURCE_DIRECTORY) / "third_party" /
            "ieee1516.2-2025" / "resources" / "examples" /
            "RestaurantFOMmodule-2025.xml",
        std::filesystem::path(UMBRA_SOURCE_DIRECTORY) / "third_party" /
            "ieee1516.2-2025" / "resources" / "schemas" /
            "IEEE1516-DIF-2025.xsd",
        FomModuleKind::fom,
        L"modern",
        L"IEEE1516-DIF-2025.xsd",
        FomStandardEdition::ieee1516_2025,
    });
    REQUIRE(result.status == FomValidationStatus::valid);
    REQUIRE(result.module.has_value());
    return *result.module;
  }();

  LibXml2FomModuleComposer composer(
      std::filesystem::path(UMBRA_SOURCE_DIRECTORY) / "third_party" /
          "ieee1516.2-2025" / "resources" / "schemas" / "IEEE1516-FDD-2025.xsd",
      legacyResource("schemas/IEEE1516-FDD-2010.xsd"));
  auto result = composer.compose({legacyFom, modernFom});

  REQUIRE(result.status == FomCompositionStatus::inconsistent_modules);
  REQUIRE(result.diagnostics.find("2010") != std::string::npos);
  REQUIRE(result.diagnostics.find("2025") != std::string::npos);
}

TEST_CASE(
    "The FOM edition setting rejects ambiguous 202x spellings",
    "[unit][external][fom][edition-policy]") {
  using rti1516_2025::HLA_EVOKED;
  using rti1516_2025::RtiConfiguration;

  rti1516_2025::NullFederateAmbassador callbacks;
  rti1516_2025::umbra_binding_detail::UmbraRtiAmbassador rti;
  auto configuration = RtiConfiguration::createConfiguration()
      .withAdditionalSettings(L"fomEdition=202x");
  REQUIRE_THROWS_AS(
      rti.connect(callbacks, HLA_EVOKED, configuration),
      rti1516_2025::RTIinternalError);
}

TEST_CASE(
    "The 2025 API registers objects from an IEEE 1516-2010 model without simulation",
    "[unit][external][fom][ieee1516-2010][object-registration]") {
  using rti1516_2025::AttributeHandleSet;
  using rti1516_2025::HLA_EVOKED;
  using rti1516_2025::RtiConfiguration;

  rti1516_2025::NullFederateAmbassador callbacks;
  rti1516_2025::umbra_binding_detail::UmbraRtiAmbassador rti;
  auto configuration = RtiConfiguration::createConfiguration()
      .withAdditionalSettings(L"fomEdition=2010");
  REQUIRE_NOTHROW(rti.connect(callbacks, HLA_EVOKED, configuration));

  auto const federationName = nextFederationName();
  REQUIRE_NOTHROW(rti.createFederationExecution(
      federationName,
      restaurantFom().wstring(),
      L"HLAinteger64Time"));
  try {
    rti.joinFederationExecution(L"registration-only", federationName);
  } catch (rti1516_2025::Exception const& exception) {
    std::string message;
    for (auto const character : exception.name()) {
      message.push_back(character < 128 ? static_cast<char>(character) : '?');
    }
    message += ": ";
    for (auto const character : exception.what()) {
      message.push_back(character < 128 ? static_cast<char>(character) : '?');
    }
    FAIL(message);
  } catch (std::exception const& exception) {
    FAIL(exception.what());
  } catch (...) {
    FAIL("join threw an unknown non-standard exception");
  }

  auto const server = rti.getObjectClassHandle(L"HLAobjectRoot.Employee.Waiter");
  auto const efficiency = rti.getAttributeHandle(server, L"Efficiency");
  AttributeHandleSet published{efficiency};
  REQUIRE_NOTHROW(rti.publishObjectClassAttributes(server, published));

  rti1516_2025::ObjectInstanceHandle anonymous;
  REQUIRE_NOTHROW(anonymous = rti.registerObjectInstance(server));
  REQUIRE_FALSE(rti.getObjectInstanceName(anonymous).empty());
  REQUIRE(rti.getKnownObjectClassHandle(anonymous) == server);

  auto const named = L"Legacy2010Server";
  REQUIRE_NOTHROW(rti.reserveObjectInstanceName(named));
  rti1516_2025::ObjectInstanceHandle namedHandle;
  REQUIRE_NOTHROW(namedHandle = rti.registerObjectInstance(server, named));
  REQUIRE(rti.getObjectInstanceName(namedHandle) == named);
  REQUIRE(rti.getKnownObjectClassHandle(namedHandle) == server);

  REQUIRE_NOTHROW(rti.resignFederationExecution(
      rti1516_2025::CANCEL_THEN_DELETE_THEN_DIVEST));
  REQUIRE_NOTHROW(rti.destroyFederationExecution(federationName));
  REQUIRE_NOTHROW(rti.disconnect());
}

#ifdef UMBRA_EXTERNAL_TARGET_RADAR_FOM_PATH
TEST_CASE(
    "The 2025 API registers objects from the external Target Radar FOM without simulation",
    "[unit][external][fom][ieee1516-2010][target-radar][object-registration]") {
  using rti1516_2025::AttributeHandleSet;
  using rti1516_2025::HLA_EVOKED;
  using rti1516_2025::RtiConfiguration;

  rti1516_2025::NullFederateAmbassador callbacks;
  rti1516_2025::umbra_binding_detail::UmbraRtiAmbassador rti;
  auto configuration = RtiConfiguration::createConfiguration()
      .withAdditionalSettings(L"fomEdition=2010");
  REQUIRE_NOTHROW(rti.connect(callbacks, HLA_EVOKED, configuration));

  auto const federationName = nextFederationName();
  try {
    rti.createFederationExecution(
        federationName,
        targetRadarFom().wstring(),
        L"HLAinteger64Time");
  } catch (rti1516_2025::Exception const& exception) {
    std::string message;
    for (auto const character : exception.name()) {
      message.push_back(character < 128 ? static_cast<char>(character) : '?');
    }
    message += ": ";
    for (auto const character : exception.what()) {
      message.push_back(character < 128 ? static_cast<char>(character) : '?');
    }
    FAIL(message);
  } catch (std::exception const& exception) {
    FAIL(exception.what());
  }
  REQUIRE_NOTHROW(rti.joinFederationExecution(L"target-radar-registration-only", federationName));

  auto const targetClass = rti.getObjectClassHandle(L"HLAobjectRoot.Target");
  auto const position = rti.getAttributeHandle(targetClass, L"Position");
  auto const velocity = rti.getAttributeHandle(targetClass, L"Velocity");
  auto const rcs = rti.getAttributeHandle(targetClass, L"RCS");
  REQUIRE_NOTHROW(rti.publishObjectClassAttributes(
      targetClass,
      AttributeHandleSet{position, velocity, rcs}));

  rti1516_2025::ObjectInstanceHandle target;
  REQUIRE_NOTHROW(target = rti.registerObjectInstance(targetClass));
  REQUIRE_FALSE(rti.getObjectInstanceName(target).empty());

  auto const trackClass = rti.getObjectClassHandle(L"HLAobjectRoot.Track");
  auto const trackTargetName = rti.getAttributeHandle(trackClass, L"TargetName");
  auto const trackPosition = rti.getAttributeHandle(trackClass, L"Position");
  auto const trackRange = rti.getAttributeHandle(trackClass, L"Range");
  auto const trackBearing = rti.getAttributeHandle(trackClass, L"Bearing");
  auto const trackRcs = rti.getAttributeHandle(trackClass, L"RCS");
  auto const trackTime = rti.getAttributeHandle(trackClass, L"Time");
  REQUIRE_NOTHROW(rti.publishObjectClassAttributes(
      trackClass,
      AttributeHandleSet{
          trackTargetName,
          trackPosition,
          trackRange,
          trackBearing,
          trackRcs,
          trackTime}));
  auto const trackName = L"TrackedTarget-2010";
  REQUIRE_NOTHROW(rti.reserveObjectInstanceName(trackName));
  rti1516_2025::ObjectInstanceHandle track;
  REQUIRE_NOTHROW(track = rti.registerObjectInstance(trackClass, trackName));
  REQUIRE(rti.getObjectInstanceName(track) == trackName);

  rti1516_2025::InteractionClassHandle reportClass;
  REQUIRE_NOTHROW(reportClass = rti.getInteractionClassHandle(
      L"HLAinteractionRoot.TrackReport"));

  REQUIRE_NOTHROW(rti.resignFederationExecution(
      rti1516_2025::CANCEL_THEN_DELETE_THEN_DIVEST));
  REQUIRE_NOTHROW(rti.destroyFederationExecution(federationName));
  REQUIRE_NOTHROW(rti.disconnect());
}
#endif
