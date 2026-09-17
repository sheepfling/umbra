#include <catch2/catch_test_macros.hpp>

#include "internal/fom/hla_names.hpp"
#include "hla_test_names.hpp"

#include <RTI/NullFederateAmbassador.h>
#include <RTI/RTI1516.h>
#include <RTI/time/HLAinteger64Time.h>

#include <atomic>
#include <filesystem>
#include <memory>
#include <string>
#include <vector>

#include <umbra/embedded_profile_configuration.hpp>

#ifndef UMBRA_SOURCE_DIRECTORY
#error "The declaration-relevance focused test requires the Umbra source directory."
#endif

namespace {

namespace standard_hla = umbra::detail::hla::wide;
namespace fixture_hla = umbra::test::hla::wide;

using rti1516_2025::AttributeHandle;
using rti1516_2025::AttributeHandleSet;
using rti1516_2025::CallbackModel;
using rti1516_2025::HLA_EVOKED;
using rti1516_2025::InteractionClassHandle;
using rti1516_2025::NO_ACTION;
using rti1516_2025::NullFederateAmbassador;
using rti1516_2025::ObjectClassHandle;
using rti1516_2025::RTIambassador;
using rti1516_2025::RTIambassadorFactory;

struct ObjectClassRelevanceReport final {
  ObjectClassHandle objectClass;
};

struct InteractionRelevanceReport final {
  InteractionClassHandle interactionClass;
};

class DeclarationFederateAmbassador final : public NullFederateAmbassador {
 public:
  void startRegistrationForObjectClass(
      ObjectClassHandle const& objectClass) override {
    startRegistrationForObjectClassReports.push_back({objectClass});
  }

  void stopRegistrationForObjectClass(
      ObjectClassHandle const& objectClass) override {
    stopRegistrationForObjectClassReports.push_back({objectClass});
  }

  void turnInteractionsOn(
      InteractionClassHandle const& interactionClass) override {
    turnInteractionsOnReports.push_back({interactionClass});
  }

  void turnInteractionsOff(
      InteractionClassHandle const& interactionClass) override {
    turnInteractionsOffReports.push_back({interactionClass});
  }

  std::vector<ObjectClassRelevanceReport> startRegistrationForObjectClassReports;
  std::vector<ObjectClassRelevanceReport> stopRegistrationForObjectClassReports;
  std::vector<InteractionRelevanceReport> turnInteractionsOnReports;
  std::vector<InteractionRelevanceReport> turnInteractionsOffReports;
};

std::unique_ptr<RTIambassador> makeRti() {
  RTIambassadorFactory factory;
  return factory.createRTIambassador();
}

std::filesystem::path resourcePath(std::filesystem::path const& relativePath) {
  return std::filesystem::path(UMBRA_SOURCE_DIRECTORY) / "third_party" /
      "ieee1516.2-2025" / "resources" / relativePath;
}

std::wstring nextFederationName() {
  static std::atomic_uint64_t counter{0};
  return L"umbra-declaration-relevance-" + std::to_wstring(++counter);
}

}  // namespace

TEST_CASE(
    "Embedded declaration relevance advisories follow ordinary 2025 publication and subscription transitions",
    "[integration][development-profile][federation-management]"
    "[declaration-management][callbacks]"
    "[rti.service.start-registration-for-object-class]"
    "[rti.service.stop-registration-for-object-class]"
    "[rti.service.turn-interactions-on]"
    "[rti.service.turn-interactions-off]"
    "[rti.service.get-object-class-relevance-advisory-switch]"
    "[rti.service.set-object-class-relevance-advisory-switch]"
    "[rti.service.get-interaction-relevance-advisory-switch]"
    "[rti.service.set-interaction-relevance-advisory-switch]"
    "[declaration-relevance-advisory-service-report][2025]") {
  DeclarationFederateAmbassador publisherReports;
  DeclarationFederateAmbassador subscriberReports;
  auto publisher = makeRti();
  auto subscriber = makeRti();
  auto const federationName = nextFederationName();
  auto const restaurantFom =
      resourcePath("examples/RestaurantFOMmodule-2025.xml").wstring();
  auto const switchFom =
      (std::filesystem::path(UMBRA_SOURCE_DIRECTORY) / "cpp" / "tests" /
       "data" / "switch-support-enabled-fom.xml")
          .wstring();
  std::vector<std::wstring> const fomModules{restaurantFom, switchFom};

  REQUIRE_NOTHROW(publisher->connect(publisherReports, HLA_EVOKED));
  REQUIRE_NOTHROW(subscriber->connect(subscriberReports, HLA_EVOKED));
  REQUIRE_NOTHROW(publisher->createFederationExecution(
      federationName, fomModules, standard_hla::mom::integer64_time));
  REQUIRE_NOTHROW(publisher->joinFederationExecution(
      L"relevance-publisher", L"publisher", federationName));
  REQUIRE_NOTHROW(subscriber->joinFederationExecution(
      L"relevance-subscriber", L"subscriber", federationName));

  auto const employee =
      publisher->getObjectClassHandle(fixture_hla::fom::employee);
  auto const name = publisher->getAttributeHandle(
      employee, fixture_hla::fixture::name);
  auto const takeOrder = publisher->getInteractionClassHandle(
      fixture_hla::fom::server_take_order);
  REQUIRE(employee.isValid());
  REQUIRE(name.isValid());
  REQUIRE(takeOrder.isValid());

  // The FDD seeds each switch for each joining federate. The official
  // accessors remain mutable on the publishing federate only.
  REQUIRE(publisher->getObjectClassRelevanceAdvisorySwitch());
  REQUIRE(subscriber->getObjectClassRelevanceAdvisorySwitch());
  REQUIRE(publisher->getInteractionRelevanceAdvisorySwitch());
  REQUIRE(subscriber->getInteractionRelevanceAdvisorySwitch());
  REQUIRE_NOTHROW(publisher->setObjectClassRelevanceAdvisorySwitch(false));
  REQUIRE_NOTHROW(publisher->setInteractionRelevanceAdvisorySwitch(false));
  REQUIRE_FALSE(publisher->getObjectClassRelevanceAdvisorySwitch());
  REQUIRE_FALSE(publisher->getInteractionRelevanceAdvisorySwitch());
  REQUIRE_NOTHROW(publisher->setObjectClassRelevanceAdvisorySwitch(true));
  REQUIRE_NOTHROW(publisher->setInteractionRelevanceAdvisorySwitch(true));
  REQUIRE(publisher->getObjectClassRelevanceAdvisorySwitch());
  REQUIRE(publisher->getInteractionRelevanceAdvisorySwitch());

  // An active object subscription followed by publication establishes one
  // Start advisory at the publisher. Repeating the declaration is idempotent.
  REQUIRE_NOTHROW(subscriber->subscribeObjectClassAttributes(
      employee, AttributeHandleSet{name}, true));
  REQUIRE_NOTHROW(
      publisher->publishObjectClassAttributes(employee, AttributeHandleSet{name}));
  REQUIRE(publisherReports.startRegistrationForObjectClassReports.empty());
  REQUIRE_FALSE(publisher->evokeMultipleCallbacks(0.0, 0.0));
  REQUIRE_FALSE(publisher->evokeMultipleCallbacks(0.0, 0.0));
  REQUIRE(publisherReports.startRegistrationForObjectClassReports.size() == 1);
  REQUIRE(publisherReports.startRegistrationForObjectClassReports.front().objectClass ==
          employee);
  REQUIRE_NOTHROW(
      publisher->publishObjectClassAttributes(employee, AttributeHandleSet{name}));
  REQUIRE_FALSE(publisher->evokeMultipleCallbacks(0.0, 0.0));
  REQUIRE(publisherReports.startRegistrationForObjectClassReports.size() == 1);

  // Removing the only active subscription establishes one Stop advisory.
  REQUIRE_NOTHROW(subscriber->unsubscribeObjectClassAttributes(
      employee, AttributeHandleSet{name}));
  REQUIRE(publisherReports.stopRegistrationForObjectClassReports.empty());
  REQUIRE_FALSE(publisher->evokeMultipleCallbacks(0.0, 0.0));
  REQUIRE_FALSE(publisher->evokeMultipleCallbacks(0.0, 0.0));
  REQUIRE(publisherReports.stopRegistrationForObjectClassReports.size() == 1);
  REQUIRE(publisherReports.stopRegistrationForObjectClassReports.front().objectClass ==
          employee);

  // Passive ordinary subscriptions do not establish relevance. Switching to
  // active and back to passive creates exactly one Start/Stop pair.
  REQUIRE_NOTHROW(subscriber->subscribeObjectClassAttributes(
      employee, AttributeHandleSet{name}, false));
  REQUIRE_FALSE(publisher->evokeMultipleCallbacks(0.0, 0.0));
  REQUIRE(publisherReports.startRegistrationForObjectClassReports.size() == 1);
  REQUIRE_NOTHROW(subscriber->subscribeObjectClassAttributes(
      employee, AttributeHandleSet{name}, true));
  REQUIRE_FALSE(publisher->evokeMultipleCallbacks(0.0, 0.0));
  REQUIRE_FALSE(publisher->evokeMultipleCallbacks(0.0, 0.0));
  REQUIRE(publisherReports.startRegistrationForObjectClassReports.size() == 2);
  REQUIRE_NOTHROW(subscriber->subscribeObjectClassAttributes(
      employee, AttributeHandleSet{name}, false));
  REQUIRE_FALSE(publisher->evokeMultipleCallbacks(0.0, 0.0));
  REQUIRE_FALSE(publisher->evokeMultipleCallbacks(0.0, 0.0));
  REQUIRE(publisherReports.stopRegistrationForObjectClassReports.size() == 2);

  // Interaction relevance follows the same active/passive boundary and is
  // independent of object-class relevance.
  REQUIRE_NOTHROW(subscriber->subscribeInteractionClass(takeOrder, true));
  REQUIRE_NOTHROW(publisher->publishInteractionClass(takeOrder));
  REQUIRE_FALSE(publisher->evokeMultipleCallbacks(0.0, 0.0));
  REQUIRE_FALSE(publisher->evokeMultipleCallbacks(0.0, 0.0));
  REQUIRE(publisherReports.turnInteractionsOnReports.size() == 1);
  REQUIRE(publisherReports.turnInteractionsOnReports.front().interactionClass ==
          takeOrder);
  REQUIRE_NOTHROW(publisher->publishInteractionClass(takeOrder));
  REQUIRE_FALSE(publisher->evokeMultipleCallbacks(0.0, 0.0));
  REQUIRE(publisherReports.turnInteractionsOnReports.size() == 1);
  REQUIRE_NOTHROW(subscriber->unsubscribeInteractionClass(takeOrder));
  REQUIRE_FALSE(publisher->evokeMultipleCallbacks(0.0, 0.0));
  REQUIRE_FALSE(publisher->evokeMultipleCallbacks(0.0, 0.0));
  REQUIRE(publisherReports.turnInteractionsOffReports.size() == 1);

  REQUIRE_NOTHROW(subscriber->subscribeInteractionClass(takeOrder, false));
  REQUIRE_FALSE(publisher->evokeMultipleCallbacks(0.0, 0.0));
  REQUIRE_NOTHROW(subscriber->subscribeInteractionClass(takeOrder, true));
  REQUIRE_FALSE(publisher->evokeMultipleCallbacks(0.0, 0.0));
  REQUIRE_FALSE(publisher->evokeMultipleCallbacks(0.0, 0.0));
  REQUIRE(publisherReports.turnInteractionsOnReports.size() == 2);
  REQUIRE_NOTHROW(subscriber->subscribeInteractionClass(takeOrder, false));
  REQUIRE_FALSE(publisher->evokeMultipleCallbacks(0.0, 0.0));
  REQUIRE_FALSE(publisher->evokeMultipleCallbacks(0.0, 0.0));
  REQUIRE(publisherReports.turnInteractionsOffReports.size() == 2);

  REQUIRE_NOTHROW(publisher->unpublishInteractionClass(takeOrder));
  REQUIRE_NOTHROW(publisher->unpublishObjectClassAttributes(
      employee, AttributeHandleSet{name}));
  REQUIRE_NOTHROW(subscriber->resignFederationExecution(NO_ACTION));
  REQUIRE_NOTHROW(publisher->resignFederationExecution(NO_ACTION));
  REQUIRE_NOTHROW(publisher->destroyFederationExecution(federationName));
  REQUIRE_NOTHROW(publisher->disconnect());
  REQUIRE_NOTHROW(subscriber->disconnect());
}
