#include <catch2/catch_test_macros.hpp>

#include "internal/fdd_document.hpp"
#include "internal/federate_time_state.hpp"
#include "internal/fom_catalog.hpp"
#include "internal/federation_management_coordinator.hpp"
#include "internal/federation_registry.hpp"
#include "internal/libxml2_fom_composer.hpp"
#include "internal/libxml2_fom_validator.hpp"
#include "internal/reference_time_selection.hpp"

#include <atomic>
#include <filesystem>
#include <fstream>
#include <iterator>
#include <memory>
#include <optional>
#include <set>
#include <string>
#include <system_error>
#include <vector>

#include <RTI/time/HLAinteger64Time.h>

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
using umbra::detail::ObjectClassAttributeDeclarationStatus;
using umbra::detail::ObjectInstanceRegistrationStatus;
using umbra::detail::LibXml2FomModuleComposer;
using umbra::detail::LibXml2FomValidator;
using umbra::detail::PrevalidatedFomModule;
using umbra::detail::ReceiveOrderInteractionStatus;
using umbra::detail::ReferenceLogicalTimeSelectionStatus;
using umbra::detail::ReferenceLogicalTimeSelector;

std::filesystem::path resourcePath(std::filesystem::path const& relative) {
  return std::filesystem::path(UMBRA_SOURCE_DIRECTORY) / "third_party" / "ieee1516.2-2025" /
         "resources" / relative;
}

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

LibXml2FomModuleComposer composer() {
  return LibXml2FomModuleComposer(resourcePath("schemas/IEEE1516-FDD-2025.xsd"));
}

PrevalidatedFomModule validated(
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
      validated(resourcePath("mim/HLAstandardMIM-2025.xml"), FomModuleKind::mim, L"urn:umbra:test:mim"),
      validated(
          resourcePath("examples/RestaurantFOMmodule-2025.xml"),
          FomModuleKind::fom,
          L"urn:umbra:test:restaurant"),
  };
  auto materializer = composer();
  auto result = materializer.compose(modules);
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

}  // namespace

TEST_CASE("The FDD materializer accepts the supplied MIM and base FOM", "[unit][fom][composition]") {
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
    "The federation registry retains independent interaction declarations until removal or resign",
    "[unit][kernel][federation-registry][declaration-management]") {
  umbra::detail::EmbeddedFederationRegistry registry;
  REQUIRE(
      registry.create(L"declaration-exercise", composedRestaurantDefinition()).status ==
      FederationRegistryStatus::applied);

  auto publisher = registry.join(L"declaration-exercise", L"publisher", L"publisher");
  auto subscriber = registry.join(L"declaration-exercise", L"subscriber", L"subscriber");
  REQUIRE(publisher.membership);
  REQUIRE(subscriber.membership);
  auto const takeOrder = registry.interactionClassHandleFor(
      L"declaration-exercise",
      "HLAinteractionRoot.ServerAction.TakeOrder");
  REQUIRE(takeOrder);

  auto publisherState = registry.interactionClassDeclarationFor(
      L"declaration-exercise",
      publisher.membership->id,
      *takeOrder);
  REQUIRE(publisherState);
  REQUIRE_FALSE(publisherState->published);
  REQUIRE_FALSE(publisherState->subscriptionActive.has_value());

  REQUIRE(
      registry.setInteractionClassPublication(
          L"missing",
          publisher.membership->id,
          *takeOrder,
          true) == InteractionClassDeclarationStatus::federation_does_not_exist);
  REQUIRE(
      registry.setInteractionClassSubscription(
          L"declaration-exercise",
          9999,
          *takeOrder,
          true) == InteractionClassDeclarationStatus::federate_not_member);
  REQUIRE(
      registry.setInteractionClassPublication(
          L"declaration-exercise",
          publisher.membership->id,
          0,
          true) == InteractionClassDeclarationStatus::interaction_class_not_defined);

  REQUIRE(
      registry.setInteractionClassPublication(
          L"declaration-exercise",
          publisher.membership->id,
          *takeOrder,
          true) == InteractionClassDeclarationStatus::applied);
  publisherState = registry.interactionClassDeclarationFor(
      L"declaration-exercise",
      publisher.membership->id,
      *takeOrder);
  REQUIRE(publisherState);
  REQUIRE(publisherState->published);
  REQUIRE_FALSE(publisherState->subscriptionActive.has_value());

  auto subscriberState = registry.interactionClassDeclarationFor(
      L"declaration-exercise",
      subscriber.membership->id,
      *takeOrder);
  REQUIRE(subscriberState);
  REQUIRE_FALSE(subscriberState->published);
  REQUIRE_FALSE(subscriberState->subscriptionActive.has_value());

  REQUIRE(
      registry.setInteractionClassSubscription(
          L"declaration-exercise",
          subscriber.membership->id,
          *takeOrder,
          false) == InteractionClassDeclarationStatus::applied);
  subscriberState = registry.interactionClassDeclarationFor(
      L"declaration-exercise",
      subscriber.membership->id,
      *takeOrder);
  REQUIRE(subscriberState);
  REQUIRE_FALSE(subscriberState->published);
  REQUIRE(subscriberState->subscriptionActive == std::optional<bool>{false});

  REQUIRE(
      registry.setInteractionClassSubscription(
          L"declaration-exercise",
          subscriber.membership->id,
          *takeOrder,
          true) == InteractionClassDeclarationStatus::applied);
  subscriberState = registry.interactionClassDeclarationFor(
      L"declaration-exercise",
      subscriber.membership->id,
      *takeOrder);
  REQUIRE(subscriberState);
  REQUIRE(subscriberState->subscriptionActive == std::optional<bool>{true});

  REQUIRE(
      registry.setInteractionClassPublication(
          L"declaration-exercise",
          publisher.membership->id,
          *takeOrder,
          false) == InteractionClassDeclarationStatus::applied);
  publisherState = registry.interactionClassDeclarationFor(
      L"declaration-exercise",
      publisher.membership->id,
      *takeOrder);
  REQUIRE(publisherState);
  REQUIRE_FALSE(publisherState->published);
  REQUIRE_FALSE(publisherState->subscriptionActive.has_value());

  REQUIRE(
      registry.setInteractionClassSubscription(
          L"declaration-exercise",
          subscriber.membership->id,
          *takeOrder,
          std::nullopt) == InteractionClassDeclarationStatus::applied);
  subscriberState = registry.interactionClassDeclarationFor(
      L"declaration-exercise",
      subscriber.membership->id,
      *takeOrder);
  REQUIRE(subscriberState);
  REQUIRE_FALSE(subscriberState->published);
  REQUIRE_FALSE(subscriberState->subscriptionActive.has_value());

  REQUIRE(
      registry.resign(L"declaration-exercise", publisher.membership->id).status ==
      FederationRegistryStatus::applied);
  REQUIRE_FALSE(
      registry.interactionClassDeclarationFor(
          L"declaration-exercise",
          publisher.membership->id,
          *takeOrder)
          .has_value());
}

TEST_CASE(
    "The federation registry retains 2025 object class attribute declarations independently",
    "[unit][kernel][federation-registry][declaration-management][object-management]") {
  umbra::detail::EmbeddedFederationRegistry registry;
  std::wstring const federationName = L"object-attribute-declaration-exercise";
  REQUIRE(
      registry.create(federationName, composedRestaurantDefinition()).status ==
      FederationRegistryStatus::applied);

  auto publisher = registry.join(federationName, L"publisher", L"publisher");
  auto subscriber = registry.join(federationName, L"subscriber", L"subscriber");
  REQUIRE(publisher.membership);
  REQUIRE(subscriber.membership);

  std::string const employee = "HLAobjectRoot.Employee";
  std::string const server = "HLAobjectRoot.Employee.Server";
  auto const employeeHandle = registry.objectClassHandleFor(federationName, employee);
  auto const serverHandle = registry.objectClassHandleFor(federationName, server);
  auto const employeeName = registry.attributeHandleFor(federationName, employee, "Name");
  auto const serverName = registry.attributeHandleFor(federationName, server, "Name");
  auto const efficiency = registry.attributeHandleFor(federationName, server, "Efficiency");
  REQUIRE(employeeHandle);
  REQUIRE(serverHandle);
  REQUIRE(employeeName);
  REQUIRE(serverName);
  REQUIRE(efficiency);
  REQUIRE(*serverName == *employeeName);

  auto publisherEmployee = registry.objectClassAttributeDeclarationFor(
      federationName,
      publisher.membership->id,
      *employeeHandle);
  REQUIRE(publisherEmployee);
  REQUIRE(publisherEmployee->explicitlyPublishedAttributes.empty());
  REQUIRE(publisherEmployee->subscribedAttributes.empty());

  REQUIRE(
      registry.setObjectClassAttributePublication(
          L"missing",
          publisher.membership->id,
          *employeeHandle,
          {*employeeName},
          true) == ObjectClassAttributeDeclarationStatus::federation_does_not_exist);
  REQUIRE(
      registry.setObjectClassAttributeSubscription(
          federationName,
          9999,
          *employeeHandle,
          {*employeeName},
          true) == ObjectClassAttributeDeclarationStatus::federate_not_member);
  REQUIRE(
      registry.setObjectClassAttributePublication(
          federationName,
          publisher.membership->id,
          0,
          {*employeeName},
          true) == ObjectClassAttributeDeclarationStatus::object_class_not_defined);
  REQUIRE(
      registry.setObjectClassAttributePublication(
          federationName,
          publisher.membership->id,
          *employeeHandle,
          {*employeeName, *efficiency},
          true) == ObjectClassAttributeDeclarationStatus::attribute_not_defined);

  publisherEmployee = registry.objectClassAttributeDeclarationFor(
      federationName,
      publisher.membership->id,
      *employeeHandle);
  REQUIRE(publisherEmployee);
  REQUIRE(publisherEmployee->explicitlyPublishedAttributes.empty());

  REQUIRE(
      registry.setObjectClassAttributePublication(
          federationName,
          publisher.membership->id,
          *employeeHandle,
          {*employeeName},
          true) == ObjectClassAttributeDeclarationStatus::applied);
  REQUIRE(
      registry.setObjectClassAttributePublication(
          federationName,
          publisher.membership->id,
          *serverHandle,
          {*efficiency},
          true) == ObjectClassAttributeDeclarationStatus::applied);

  publisherEmployee = registry.objectClassAttributeDeclarationFor(
      federationName,
      publisher.membership->id,
      *employeeHandle);
  REQUIRE(publisherEmployee);
  REQUIRE(publisherEmployee->explicitlyPublishedAttributes == std::set<std::uint64_t>{*employeeName});
  auto publisherServer = registry.objectClassAttributeDeclarationFor(
      federationName,
      publisher.membership->id,
      *serverHandle);
  REQUIRE(publisherServer);
  REQUIRE(publisherServer->explicitlyPublishedAttributes == std::set<std::uint64_t>{*efficiency});

  REQUIRE(
      registry.setObjectClassAttributeSubscription(
          federationName,
          subscriber.membership->id,
          *serverHandle,
          {*serverName},
          false) == ObjectClassAttributeDeclarationStatus::applied);
  REQUIRE(
      registry.setObjectClassAttributeSubscription(
          federationName,
          subscriber.membership->id,
          *serverHandle,
          {*efficiency},
          true) == ObjectClassAttributeDeclarationStatus::applied);
  auto subscriberServer = registry.objectClassAttributeDeclarationFor(
      federationName,
      subscriber.membership->id,
      *serverHandle);
  REQUIRE(subscriberServer);
  REQUIRE(subscriberServer->explicitlyPublishedAttributes.empty());
  REQUIRE(subscriberServer->subscribedAttributes.size() == 2);
  REQUIRE(subscriberServer->subscribedAttributes.at(*serverName) == false);
  REQUIRE(subscriberServer->subscribedAttributes.at(*efficiency) == true);
  auto subscriberEmployee = registry.objectClassAttributeDeclarationFor(
      federationName,
      subscriber.membership->id,
      *employeeHandle);
  REQUIRE(subscriberEmployee);
  REQUIRE(subscriberEmployee->subscribedAttributes.empty());

  REQUIRE(
      registry.setObjectClassAttributeSubscription(
          federationName,
          subscriber.membership->id,
          *serverHandle,
          {*serverName},
          std::nullopt) == ObjectClassAttributeDeclarationStatus::applied);
  subscriberServer = registry.objectClassAttributeDeclarationFor(
      federationName,
      subscriber.membership->id,
      *serverHandle);
  REQUIRE(subscriberServer);
  REQUIRE(subscriberServer->subscribedAttributes.size() == 1);
  REQUIRE(subscriberServer->subscribedAttributes.contains(*efficiency));

  REQUIRE(
      registry.setObjectClassAttributePublication(
          federationName,
          publisher.membership->id,
          *employeeHandle,
          {*employeeName},
          false) == ObjectClassAttributeDeclarationStatus::applied);
  publisherEmployee = registry.objectClassAttributeDeclarationFor(
      federationName,
      publisher.membership->id,
      *employeeHandle);
  REQUIRE(publisherEmployee);
  REQUIRE(publisherEmployee->explicitlyPublishedAttributes.empty());
  publisherServer = registry.objectClassAttributeDeclarationFor(
      federationName,
      publisher.membership->id,
      *serverHandle);
  REQUIRE(publisherServer);
  REQUIRE(publisherServer->explicitlyPublishedAttributes == std::set<std::uint64_t>{*efficiency});

  REQUIRE(
      registry.resign(federationName, publisher.membership->id).status ==
      FederationRegistryStatus::applied);
  REQUIRE_FALSE(
      registry.objectClassAttributeDeclarationFor(
          federationName,
          publisher.membership->id,
          *employeeHandle)
          .has_value());
}

TEST_CASE(
    "The federation registry allocates 2025 object identities only from current publication",
    "[unit][kernel][federation-registry][object-management]") {
  umbra::detail::EmbeddedFederationRegistry registry;
  std::wstring const federationName = L"object-instance-registration-exercise";
  REQUIRE(
      registry.create(federationName, composedRestaurantDefinition()).status ==
      FederationRegistryStatus::applied);

  auto publisher = registry.join(federationName, L"publisher", L"publisher");
  auto observer = registry.join(federationName, L"observer", L"observer");
  REQUIRE(publisher.membership);
  REQUIRE(observer.membership);

  std::string const server = "HLAobjectRoot.Employee.Server";
  auto const serverHandle = registry.objectClassHandleFor(federationName, server);
  auto const efficiency = registry.attributeHandleFor(federationName, server, "Efficiency");
  REQUIRE(serverHandle);
  REQUIRE(efficiency);

  auto undefined = registry.registerObjectInstance(
      federationName,
      publisher.membership->id,
      0);
  REQUIRE(undefined.status == ObjectInstanceRegistrationStatus::object_class_not_defined);
  auto unpublished = registry.registerObjectInstance(
      federationName,
      publisher.membership->id,
      *serverHandle);
  REQUIRE(unpublished.status == ObjectInstanceRegistrationStatus::object_class_not_published);

  REQUIRE(
      registry.setObjectClassAttributePublication(
          federationName,
          publisher.membership->id,
          *serverHandle,
          {*efficiency},
          true) == ObjectClassAttributeDeclarationStatus::applied);

  auto first = registry.registerObjectInstance(
      federationName,
      publisher.membership->id,
      *serverHandle);
  auto second = registry.registerObjectInstance(
      federationName,
      publisher.membership->id,
      *serverHandle);
  REQUIRE(first.status == ObjectInstanceRegistrationStatus::applied);
  REQUIRE(second.status == ObjectInstanceRegistrationStatus::applied);
  REQUIRE(first.objectInstanceHandle != 0);
  REQUIRE(second.objectInstanceHandle != 0);
  REQUIRE(first.objectInstanceHandle != second.objectInstanceHandle);
  REQUIRE_FALSE(first.objectInstanceName.empty());
  REQUIRE_FALSE(second.objectInstanceName.empty());
  REQUIRE(first.objectInstanceName != second.objectInstanceName);

  auto publisherKnown = registry.knownObjectInstanceFor(
      federationName,
      publisher.membership->id,
      first.objectInstanceHandle);
  REQUIRE(publisherKnown);
  REQUIRE(publisherKnown->knownObjectClassHandle == *serverHandle);
  REQUIRE(publisherKnown->objectInstanceName == first.objectInstanceName);
  REQUIRE(publisherKnown->producingFederateId == publisher.membership->id);
  auto publisherKnownByName = registry.knownObjectInstanceByNameFor(
      federationName,
      publisher.membership->id,
      first.objectInstanceName);
  REQUIRE(publisherKnownByName);
  REQUIRE(publisherKnownByName->objectInstanceHandle == first.objectInstanceHandle);
  REQUIRE_FALSE(
      registry.knownObjectInstanceFor(
          federationName,
          observer.membership->id,
          first.objectInstanceHandle)
          .has_value());
}

TEST_CASE(
    "The federation registry releases undelivered 2025 discovery reservations",
    "[unit][kernel][federation-registry][object-management][callbacks]") {
  umbra::detail::EmbeddedFederationRegistry registry;
  std::wstring const federationName = L"object-instance-discovery-reservation-exercise";
  REQUIRE(
      registry.create(federationName, composedRestaurantDefinition()).status ==
      FederationRegistryStatus::applied);

  auto publisher = registry.join(federationName, L"publisher", L"publisher");
  auto observerTimeState = std::make_shared<FederateTimeState>(
      L"HLAinteger64Time",
      std::make_shared<rti1516_2025::HLAinteger64Time>());
  auto observer = registry.joinWithTimeState(
      federationName,
      observerTimeState,
      L"observer",
      L"observer",
      [](umbra::detail::FederateCallbackInvocation) {});
  REQUIRE(publisher.membership);
  REQUIRE(observer.membership);

  std::string const server = "HLAobjectRoot.Employee.Server";
  auto const serverHandle = registry.objectClassHandleFor(federationName, server);
  auto const efficiency = registry.attributeHandleFor(federationName, server, "Efficiency");
  REQUIRE(serverHandle);
  REQUIRE(efficiency);
  REQUIRE(
      registry.setObjectClassAttributePublication(
          federationName,
          publisher.membership->id,
          *serverHandle,
          {*efficiency},
          true) == ObjectClassAttributeDeclarationStatus::applied);
  REQUIRE(
      registry.setObjectClassAttributeSubscription(
          federationName,
          observer.membership->id,
          *serverHandle,
          {*efficiency},
          true) == ObjectClassAttributeDeclarationStatus::applied);

  auto const registration = registry.registerObjectInstance(
      federationName,
      publisher.membership->id,
      *serverHandle);
  REQUIRE(registration.status == ObjectInstanceRegistrationStatus::applied);

  auto firstPlan = registry.planObjectInstanceDiscoveriesForInstance(
      federationName,
      registration.objectInstanceHandle);
  REQUIRE(firstPlan.size() == 1);
  REQUIRE(firstPlan.front().receivingFederateId == observer.membership->id);

  registry.cancelObjectInstanceDiscovery(
      federationName,
      observer.membership->id,
      registration.objectInstanceHandle);
  auto replacementPlan = registry.planObjectInstanceDiscoveriesForInstance(
      federationName,
      registration.objectInstanceHandle);
  REQUIRE(replacementPlan.size() == 1);
  REQUIRE(replacementPlan.front().receivingFederateId == observer.membership->id);

  auto const discovery = registry.beginObjectInstanceDiscovery(
      federationName,
      observer.membership->id,
      registration.objectInstanceHandle);
  REQUIRE(discovery);
  REQUIRE(discovery->knownObjectClassHandle == *serverHandle);
  REQUIRE(
      registry.planObjectInstanceDiscoveriesForInstance(
          federationName,
          registration.objectInstanceHandle)
          .empty());
}

TEST_CASE(
    "The federation registry plans 2025 interaction promotion and parameter projection",
    "[unit][kernel][federation-registry][interaction-routing]") {
  umbra::detail::EmbeddedFederationRegistry registry;
  std::wstring const federationName = L"interaction-routing-exercise";
  REQUIRE(
      registry.create(federationName, composedRestaurantDefinition()).status ==
      FederationRegistryStatus::applied);

  auto publisher = registry.join(federationName, L"publisher", L"publisher");
  auto exactSubscriber = registry.join(federationName, L"subscriber", L"exact");
  auto promotedSubscriber = registry.join(federationName, L"subscriber", L"promoted");
  auto ancestorSubscriber = registry.join(federationName, L"subscriber", L"ancestor");
  auto unrelatedSubscriber = registry.join(federationName, L"subscriber", L"unrelated");
  REQUIRE(publisher.membership);
  REQUIRE(exactSubscriber.membership);
  REQUIRE(promotedSubscriber.membership);
  REQUIRE(ancestorSubscriber.membership);
  REQUIRE(unrelatedSubscriber.membership);

  std::string const mainCourse =
      "HLAinteractionRoot.CustomerTransactions.FoodServed.MainCourseServed";
  std::string const foodServed =
      "HLAinteractionRoot.CustomerTransactions.FoodServed";
  std::string const customerTransactions = "HLAinteractionRoot.CustomerTransactions";
  std::string const unrelated = "HLAinteractionRoot.ServerAction.TakeOrder";
  auto const mainCourseHandle = registry.interactionClassHandleFor(federationName, mainCourse);
  auto const foodServedHandle = registry.interactionClassHandleFor(federationName, foodServed);
  auto const customerTransactionsHandle = registry.interactionClassHandleFor(
      federationName,
      customerTransactions);
  auto const unrelatedHandle = registry.interactionClassHandleFor(federationName, unrelated);
  auto const temperatureOk = registry.parameterHandleFor(
      federationName,
      mainCourse,
      "TemperatureOk");
  auto const accuracyOk = registry.parameterHandleFor(federationName, mainCourse, "AccuracyOk");
  auto const timelinessOk = registry.parameterHandleFor(federationName, mainCourse, "TimelinessOk");
  REQUIRE(mainCourseHandle);
  REQUIRE(foodServedHandle);
  REQUIRE(customerTransactionsHandle);
  REQUIRE(unrelatedHandle);
  REQUIRE(temperatureOk);
  REQUIRE(accuracyOk);
  REQUIRE(timelinessOk);

  std::vector<std::uint64_t> const sentParameters{
      *temperatureOk,
      *accuracyOk,
      *timelinessOk,
  };

  auto unpublished = registry.planReceiveOrderInteraction(
      federationName,
      publisher.membership->id,
      *mainCourseHandle,
      sentParameters);
  REQUIRE(unpublished.status == ReceiveOrderInteractionStatus::interaction_class_not_published);

  REQUIRE(
      registry.setInteractionClassPublication(
          federationName,
          publisher.membership->id,
          *mainCourseHandle,
          true) == InteractionClassDeclarationStatus::applied);
  REQUIRE(
      registry.setInteractionClassSubscription(
          federationName,
          publisher.membership->id,
          *mainCourseHandle,
          true) == InteractionClassDeclarationStatus::applied);
  REQUIRE(
      registry.setInteractionClassSubscription(
          federationName,
          exactSubscriber.membership->id,
          *mainCourseHandle,
          true) == InteractionClassDeclarationStatus::applied);
  // Promotion is the subject here, so the superclass declaration is active.
  // Passive-delivery suppression is covered by the dedicated public regression.
  REQUIRE(
      registry.setInteractionClassSubscription(
          federationName,
          promotedSubscriber.membership->id,
          *foodServedHandle,
          true) == InteractionClassDeclarationStatus::applied);
  REQUIRE(
      registry.setInteractionClassSubscription(
          federationName,
          ancestorSubscriber.membership->id,
          *customerTransactionsHandle,
          true) == InteractionClassDeclarationStatus::applied);
  REQUIRE(
      registry.setInteractionClassSubscription(
          federationName,
          unrelatedSubscriber.membership->id,
          *unrelatedHandle,
          true) == InteractionClassDeclarationStatus::applied);

  auto invalidParameter = registry.planReceiveOrderInteraction(
      federationName,
      publisher.membership->id,
      *mainCourseHandle,
      {*temperatureOk, 9999});
  REQUIRE(invalidParameter.status == ReceiveOrderInteractionStatus::interaction_parameter_not_defined);

  auto plan = registry.planReceiveOrderInteraction(
      federationName,
      publisher.membership->id,
      *mainCourseHandle,
      sentParameters);
  REQUIRE(plan.status == ReceiveOrderInteractionStatus::applied);
  REQUIRE(plan.transportationName == "HLAreliable");
  REQUIRE(plan.recipients.size() == 3);

  std::set<std::uint64_t> const exactParameters{
      *temperatureOk,
      *accuracyOk,
      *timelinessOk,
  };
  bool sawExact = false;
  bool sawPromoted = false;
  bool sawAncestor = false;
  for (auto const& recipient : plan.recipients) {
    REQUIRE(recipient.federateId != publisher.membership->id);
    if (recipient.federateId == exactSubscriber.membership->id) {
      sawExact = true;
      REQUIRE(recipient.receivedInteractionClassHandle == *mainCourseHandle);
      REQUIRE(recipient.receivedParameterHandles == exactParameters);
    } else if (recipient.federateId == promotedSubscriber.membership->id) {
      sawPromoted = true;
      REQUIRE(recipient.receivedInteractionClassHandle == *foodServedHandle);
      REQUIRE(recipient.receivedParameterHandles.empty());
    } else if (recipient.federateId == ancestorSubscriber.membership->id) {
      sawAncestor = true;
      REQUIRE(recipient.receivedInteractionClassHandle == *customerTransactionsHandle);
      REQUIRE(recipient.receivedParameterHandles.empty());
    } else {
      FAIL("The routing plan selected an unrelated interaction subscription.");
    }
  }
  REQUIRE(sawExact);
  REQUIRE(sawPromoted);
  REQUIRE(sawAncestor);

  auto deliveryProjection = registry.receiveOrderInteractionRecipientFor(
      federationName,
      publisher.membership->id,
      promotedSubscriber.membership->id,
      *mainCourseHandle,
      sentParameters);
  REQUIRE(deliveryProjection);
  REQUIRE(deliveryProjection->receivedInteractionClassHandle == *foodServedHandle);
  REQUIRE(deliveryProjection->receivedParameterHandles.empty());

  REQUIRE(
      registry.setInteractionClassSubscription(
          federationName,
          promotedSubscriber.membership->id,
          *foodServedHandle,
          std::nullopt) == InteractionClassDeclarationStatus::applied);
  REQUIRE_FALSE(
      registry.receiveOrderInteractionRecipientFor(
          federationName,
          publisher.membership->id,
          promotedSubscriber.membership->id,
          *mainCourseHandle,
          sentParameters)
          .has_value());
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
    "[unit][fom][composition][switches]") {
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
  auto basicRepresentation = validated(
      testData / "basic-data-type-reference-fom.xml",
      FomModuleKind::fom,
      L"urn:umbra:test:basic-data-type");

  auto materializer = composer();
  auto resolved = materializer.compose({mim, consumer, provider});
  CAPTURE(resolved.diagnostics);
  REQUIRE(resolved.status == FomCompositionStatus::valid);
  REQUIRE(resolved.catalog);
  REQUIRE(resolved.fdd);
  REQUIRE(resolved.catalog->dataType("UmbraReferenceFixtureArray") != nullptr);
  REQUIRE(resolved.catalog->dataType("UmbraReferenceFixtureValue") != nullptr);

  // IEEE1516-OMT-2025.xsd includes basicData names in dataTypeKey. The MIM
  // supplies HLAinteger32BE, so it is a valid direct reference in a complete
  // composed model even though it is a basic representation rather than a
  // higher-level type declaration.
  auto basicResolved = materializer.compose({mim, basicRepresentation});
  CAPTURE(basicResolved.diagnostics);
  REQUIRE(basicResolved.status == FomCompositionStatus::valid);
  REQUIRE(basicResolved.catalog);
  REQUIRE(basicResolved.catalog->dataType("HLAinteger32BE") != nullptr);
  REQUIRE(basicResolved.catalog->dataType("UmbraBasicReferenceFixtureArray") != nullptr);

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
    "The FOM composition preflight resolves directed interactions after the complete module set is merged",
    "[unit][fom][composition][reference-resolution]") {
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
  REQUIRE(
      objectClass->directedInteractions ==
      std::vector<std::string>{"HLAinteractionRoot.UmbraDirectedFixtureInteraction"});
  REQUIRE(
      resolved.catalog->interactionClass("HLAinteractionRoot.UmbraDirectedFixtureInteraction") !=
      nullptr);

  auto missing = materializer.compose({mim, unresolved});
  REQUIRE(missing.status == FomCompositionStatus::inconsistent_modules);
  REQUIRE(missing.diagnostics.find("HLAinteractionRoot.UmbraMissingDirectedFixtureInteraction") != std::string::npos);
  REQUIRE(missing.diagnostics.find("not declared") != std::string::npos);
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
    "[unit][fom][composition]") {
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

TEST_CASE("The FOM composition preflight permits equivalent duplicates and rejects a real class conflict", "[unit][fom][composition]") {
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

TEST_CASE("The FOM composition preflight enforces enumerated and variant-record merge invariants", "[unit][fom][composition]") {
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
    "[unit][fom][composition]") {
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
