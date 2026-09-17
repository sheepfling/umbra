#include "internal/fom/hla_names.hpp"
#include "hla_test_names.hpp"

#include <atomic>
#include <filesystem>
#include <memory>
#include <string>
#include <vector>

#include <catch2/catch_test_macros.hpp>
#include <RTI/NullFederateAmbassador.h>
#include <RTI/RTI1516.h>

#ifndef UMBRA_SOURCE_DIRECTORY
#error "The known-class advisory tests require the Umbra source directory."
#endif

namespace {

namespace standard_hla = umbra::detail::hla::wide;
namespace fixture_hla = umbra::test::hla::wide;

using rti1516_2025::AttributeHandle;
using rti1516_2025::AttributeHandleSet;
using rti1516_2025::CallbackModel;
using rti1516_2025::HLA_EVOKED;
using rti1516_2025::NO_ACTION;
using rti1516_2025::NullFederateAmbassador;
using rti1516_2025::ObjectClassHandle;
using rti1516_2025::ObjectInstanceHandle;
using rti1516_2025::RTIambassador;
using rti1516_2025::RTIambassadorFactory;
using rti1516_2025::CANCEL_THEN_DELETE_THEN_DIVEST;

class RecordingFederateAmbassador final : public NullFederateAmbassador {
 public:
  struct DiscoveryReport {
    ObjectInstanceHandle objectInstance;
    ObjectClassHandle objectClass;
  };

  struct AttributeReport {
    ObjectInstanceHandle objectInstance;
    AttributeHandleSet attributes;
  };

  void discoverObjectInstance(
      ObjectInstanceHandle const& objectInstance,
      ObjectClassHandle const& objectClass,
      std::wstring const&,
      rti1516_2025::FederateHandle const&) override {
    discoveryReports.push_back({objectInstance, objectClass});
  }

  void turnUpdatesOnForObjectInstance(
      ObjectInstanceHandle const& objectInstance,
      AttributeHandleSet const& attributes) override {
    turnUpdatesOnReports.push_back({objectInstance, attributes});
  }

  void turnUpdatesOffForObjectInstance(
      ObjectInstanceHandle const& objectInstance,
      AttributeHandleSet const& attributes) override {
    turnUpdatesOffReports.push_back({objectInstance, attributes});
  }

  std::vector<DiscoveryReport> discoveryReports;
  std::vector<AttributeReport> turnUpdatesOnReports;
  std::vector<AttributeReport> turnUpdatesOffReports;
};

std::unique_ptr<RTIambassador> makeRti() {
  RTIambassadorFactory factory;
  return factory.createRTIambassador();
}

std::wstring nextFederationName() {
  static std::atomic_uint64_t sequence{0U};
  return L"embedded-known-class-enabled-" +
      std::to_wstring(sequence.fetch_add(1U, std::memory_order_relaxed));
}

std::filesystem::path resourcePath(std::filesystem::path const& relativePath) {
  return std::filesystem::path(UMBRA_SOURCE_DIRECTORY) /
      "third_party" / "ieee1516.2-2025" / "resources" / relativePath;
}

void suppressDeclarationRelevanceAdvisories(RTIambassador& rti) {
  if (rti.getObjectClassRelevanceAdvisorySwitch()) {
    rti.setObjectClassRelevanceAdvisorySwitch(false);
  }
  if (rti.getInteractionRelevanceAdvisorySwitch()) {
    rti.setInteractionRelevanceAdvisorySwitch(false);
  }
}

TEST_CASE(
    "Embedded attribute relevance advisories honor known class when the static policy is enabled",
    "[integration][development-profile][federation-management][object-management][ddm]"
    "[callbacks][attribute-relevance-advisory][known-class-enabled][2025]"
    "[rti.service.get-advisories-use-known-class-switch]"
    "[rti.service.get-attribute-relevance-advisory-switch]"
    "[rti.service.get-object-class-handle]"
    "[rti.service.get-attribute-handle]"
    "[rti.service.publish-object-class-attributes]"
    "[rti.service.subscribe-object-class-attributes]"
    "[rti.service.register-object-instance]"
    "[rti.service.evoke-multiple-callbacks]"
    "[rti.service.unsubscribe-object-class-attributes]"
    "[federate.callback.discover-object-instance]"
    "[federate.callback.turn-updates-on-for-object-instance]"
    "[federate.callback.turn-updates-off-for-object-instance]") {
  RecordingFederateAmbassador ownerReports;
  RecordingFederateAmbassador subscriberReports;
  auto owner = makeRti();
  auto subscriber = makeRti();
  auto const federationName = nextFederationName();
  auto const restaurantFom =
      resourcePath("examples/RestaurantFOMmodule-2025.xml").wstring();
  auto const knownClassFom =
      (std::filesystem::path(UMBRA_SOURCE_DIRECTORY) / "cpp" / "tests" /
       "data" / "switch-known-class-enabled-fom.xml")
          .wstring();
  std::vector<std::wstring> const fomModules{restaurantFom, knownClassFom};

  REQUIRE_NOTHROW(owner->connect(ownerReports, HLA_EVOKED));
  REQUIRE_NOTHROW(subscriber->connect(subscriberReports, HLA_EVOKED));
  REQUIRE_NOTHROW(owner->createFederationExecution(
      federationName,
      fomModules,
      standard_hla::mom::integer64_time));
  REQUIRE_NOTHROW(owner->joinFederationExecution(
      L"known-class-enabled-owner", L"owner", federationName));
  REQUIRE_NOTHROW(subscriber->joinFederationExecution(
      L"known-class-enabled-subscriber", L"subscriber", federationName));
  REQUIRE((owner->getAdvisoriesUseKnownClassSwitch() &&
      subscriber->getAdvisoriesUseKnownClassSwitch()));
  REQUIRE(owner->getAttributeRelevanceAdvisorySwitch());
  REQUIRE(subscriber->getAttributeRelevanceAdvisorySwitch());
  suppressDeclarationRelevanceAdvisories(*owner);

  auto const employee = owner->getObjectClassHandle(fixture_hla::fom::employee);
  auto const server = owner->getObjectClassHandle(fixture_hla::fom::employee_server);
  auto const name = owner->getAttributeHandle(employee, fixture_hla::fixture::name);
  auto const efficiency =
      owner->getAttributeHandle(server, fixture_hla::fixture::efficiency);
  REQUIRE(employee.isValid());
  REQUIRE(name.isValid());

  AttributeHandleSet const employeeName{name};
  AttributeHandleSet const serverEfficiency{efficiency};
  AttributeHandleSet const publishedAttributes{name, efficiency};
  REQUIRE_NOTHROW(owner->publishObjectClassAttributes(server, publishedAttributes));
  REQUIRE_NOTHROW(subscriber->subscribeObjectClassAttributes(
      employee,
      employeeName,
      true));

  ObjectInstanceHandle objectInstance;
  REQUIRE_NOTHROW(objectInstance = owner->registerObjectInstance(server));
  REQUIRE(objectInstance.isValid());

  while (subscriber->evokeMultipleCallbacks(0.0, 0.0)) {
  }
  REQUIRE(subscriberReports.discoveryReports.size() == 1U);
  REQUIRE(subscriberReports.discoveryReports.front().objectInstance == objectInstance);
  REQUIRE(subscriberReports.discoveryReports.front().objectClass == employee);

  while (owner->evokeMultipleCallbacks(0.0, 0.0)) {
  }
  REQUIRE(ownerReports.turnUpdatesOnReports.size() == 1U);
  REQUIRE(ownerReports.turnUpdatesOnReports.front().objectInstance == objectInstance);
  REQUIRE(ownerReports.turnUpdatesOnReports.front().attributes == employeeName);
  ownerReports.turnUpdatesOnReports.clear();
  ownerReports.turnUpdatesOffReports.clear();

  // The receiver knows only Employee and the enabled policy therefore limits
  // advisory relevance to that known class.  A later Server-only declaration
  // must not produce an owner-directed advisory for the registered instance.
  REQUIRE_NOTHROW(subscriber->subscribeObjectClassAttributes(
      server,
      serverEfficiency,
      true));
  while (owner->evokeMultipleCallbacks(0.0, 0.0)) {
  }
  REQUIRE(ownerReports.turnUpdatesOnReports.empty());
  REQUIRE(ownerReports.turnUpdatesOffReports.empty());

  REQUIRE_NOTHROW(subscriber->unsubscribeObjectClassAttributes(server, serverEfficiency));
  while (owner->evokeMultipleCallbacks(0.0, 0.0)) {
  }
  REQUIRE(ownerReports.turnUpdatesOnReports.empty());
  REQUIRE(ownerReports.turnUpdatesOffReports.empty());

  REQUIRE_NOTHROW(subscriber->unsubscribeObjectClassAttributes(employee, employeeName));
  while (owner->evokeMultipleCallbacks(0.0, 0.0)) {
  }
  REQUIRE(ownerReports.turnUpdatesOffReports.size() == 1U);
  REQUIRE(ownerReports.turnUpdatesOffReports.front().attributes == employeeName);

  REQUIRE_NOTHROW([&] {
    subscriber->resignFederationExecution(NO_ACTION);
    owner->resignFederationExecution(CANCEL_THEN_DELETE_THEN_DIVEST);
    owner->destroyFederationExecution(federationName);
    owner->disconnect();
    subscriber->disconnect();
  }());
}

}  // namespace
