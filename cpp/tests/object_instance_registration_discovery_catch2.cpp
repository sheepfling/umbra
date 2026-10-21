#include <catch2/catch_test_macros.hpp>

#include "hla_test_names.hpp"
#include "internal/fom/hla_names.hpp"

#include <RTI/NullFederateAmbassador.h>
#include <RTI/RTI1516.h>

#include <atomic>
#include <filesystem>
#include <memory>
#include <string>
#include <vector>

#ifndef UMBRA_SOURCE_DIRECTORY
#error "The object-instance registration/discovery test requires the Umbra source directory."
#endif

namespace {

namespace standard_hla = umbra::detail::hla::wide;
namespace fixture_hla = umbra::test::hla::wide;

using rti1516_2025::CANCEL_THEN_DELETE_THEN_DIVEST;
using rti1516_2025::FederateHandle;
using rti1516_2025::HLA_EVOKED;
using rti1516_2025::NO_ACTION;
using rti1516_2025::NullFederateAmbassador;
using rti1516_2025::ObjectClassHandle;
using rti1516_2025::ObjectInstanceHandle;
using rti1516_2025::RTIambassador;
using rti1516_2025::RTIambassadorFactory;
using rti1516_2025::AttributeHandleSet;

class ReportingFederateAmbassador final : public NullFederateAmbassador {
 public:
  struct ObjectDiscoveryReport final {
    ObjectInstanceHandle objectInstance;
    ObjectClassHandle objectClass;
    std::wstring objectInstanceName;
    FederateHandle producingFederate;
  };

  void discoverObjectInstance(
      ObjectInstanceHandle const& objectInstance,
      ObjectClassHandle const& objectClass,
      std::wstring const& objectInstanceName,
      FederateHandle const& producingFederate) override {
    objectDiscoveryReports.push_back({
        objectInstance,
        objectClass,
        objectInstanceName,
        producingFederate,
    });
  }

  std::vector<ObjectDiscoveryReport> objectDiscoveryReports;
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
  static std::atomic_uint64_t sequence{0U};
  return L"umbra-object-instance-registration-discovery-" +
      std::to_wstring(sequence.fetch_add(1U, std::memory_order_relaxed));
}

}  // namespace

TEST_CASE(
    "Embedded object-instance registration/discovery honors 2025 publication, promotion, and callback lifecycle",
    "[integration][development-profile][federation-management][object-management][callbacks]"
    "[object-instance-registration-discovery][2025]"
    "[rti.service.register-object-instance]"
    "[rti.service.get-known-object-class-handle]"
    "[rti.service.get-object-instance-handle]"
    "[rti.service.get-object-instance-name]"
    "[rti.service.subscribe-object-class-attributes]"
    "[rti.service.unsubscribe-object-class-attributes]"
    "[rti.service.publish-object-class-attributes]"
    "[rti.service.evoke-multiple-callbacks]"
    "[federate.callback.discover-object-instance]") {
  NullFederateAmbassador unjoinedFederate;
  ReportingFederateAmbassador publisherReports;
  ReportingFederateAmbassador exactReports;
  ReportingFederateAmbassador promotedReports;
  ReportingFederateAmbassador cancelledReports;
  ReportingFederateAmbassador lateReports;
  ReportingFederateAmbassador immediateReports;
  auto unjoined = makeRti();
  auto publisher = makeRti();
  auto exact = makeRti();
  auto promoted = makeRti();
  auto cancelled = makeRti();
  auto late = makeRti();
  auto immediate = makeRti();
  auto const federationName = nextFederationName();
  auto const fomModule = resourcePath("examples/RestaurantFOMmodule-2025.xml").wstring();
  ObjectClassHandle invalidObjectClass;
  ObjectInstanceHandle objectInstance;

  // Connection and membership preconditions precede caller-supplied handles.
  REQUIRE_THROWS_AS(
      unjoined->registerObjectInstance(invalidObjectClass),
      rti1516_2025::NotConnected);
  REQUIRE_NOTHROW(unjoined->connect(unjoinedFederate, HLA_EVOKED));
  REQUIRE_THROWS_AS(
      unjoined->registerObjectInstance(invalidObjectClass),
      rti1516_2025::FederateNotExecutionMember);

  REQUIRE_NOTHROW(publisher->connect(publisherReports, HLA_EVOKED));
  REQUIRE_NOTHROW(exact->connect(exactReports, HLA_EVOKED));
  REQUIRE_NOTHROW(promoted->connect(promotedReports, HLA_EVOKED));
  REQUIRE_NOTHROW(cancelled->connect(cancelledReports, HLA_EVOKED));
  REQUIRE_NOTHROW(late->connect(lateReports, HLA_EVOKED));
  REQUIRE_NOTHROW(immediate->connect(immediateReports, rti1516_2025::HLA_IMMEDIATE));
  REQUIRE_NOTHROW(publisher->createFederationExecution(
      federationName, fomModule, standard_hla::mom::integer64_time));

  FederateHandle publisherHandle;
  REQUIRE_NOTHROW(publisherHandle = publisher->joinFederationExecution(
      L"object-publisher", L"publisher", federationName));
  REQUIRE_NOTHROW(exact->joinFederationExecution(
      L"object-exact", L"exact", federationName));
  REQUIRE_NOTHROW(promoted->joinFederationExecution(
      L"object-promoted", L"promoted", federationName));
  REQUIRE_NOTHROW(cancelled->joinFederationExecution(
      L"object-cancelled", L"cancelled", federationName));
  REQUIRE_NOTHROW(late->joinFederationExecution(
      L"object-late", L"late", federationName));
  REQUIRE_NOTHROW(immediate->joinFederationExecution(
      L"object-immediate", L"immediate", federationName));

  auto const employee = publisher->getObjectClassHandle(fixture_hla::fom::employee);
  auto const server = publisher->getObjectClassHandle(fixture_hla::fom::employee_server);
  auto const name = publisher->getAttributeHandle(employee, fixture_hla::fixture::name);
  auto const efficiency = publisher->getAttributeHandle(server, fixture_hla::fixture::efficiency);
  REQUIRE(employee.isValid());
  REQUIRE(server.isValid());
  REQUIRE(name.isValid());
  REQUIRE(efficiency.isValid());

  REQUIRE_THROWS_AS(
      publisher->registerObjectInstance(invalidObjectClass),
      rti1516_2025::ObjectClassNotDefined);
  REQUIRE_THROWS_AS(
      publisher->registerObjectInstance(server),
      rti1516_2025::ObjectClassNotPublished);

  AttributeHandleSet const nameOnly{name};
  AttributeHandleSet const efficiencyOnly{efficiency};
  REQUIRE_NOTHROW(exact->subscribeObjectClassAttributes(server, efficiencyOnly));
  REQUIRE_NOTHROW(promoted->subscribeObjectClassAttributes(employee, nameOnly, true));
  REQUIRE_NOTHROW(cancelled->subscribeObjectClassAttributes(server, efficiencyOnly));
  REQUIRE_NOTHROW(immediate->subscribeObjectClassAttributes(server, efficiencyOnly));
  REQUIRE_NOTHROW(publisher->publishObjectClassAttributes(
      server, AttributeHandleSet{name, efficiency}));

  REQUIRE_NOTHROW(objectInstance = publisher->registerObjectInstance(server));
  REQUIRE(objectInstance.isValid());
  auto const objectInstanceName = publisher->getObjectInstanceName(objectInstance);
  REQUIRE_FALSE(objectInstanceName.empty());
  REQUIRE(publisher->getObjectInstanceHandle(objectInstanceName) == objectInstance);
  REQUIRE(publisher->getKnownObjectClassHandle(objectInstance) == server);

  // Immediate delivery is synchronous; evoked recipients remain queued.
  REQUIRE(immediateReports.objectDiscoveryReports.size() == 1U);
  REQUIRE(exactReports.objectDiscoveryReports.empty());
  REQUIRE(promotedReports.objectDiscoveryReports.empty());
  REQUIRE(cancelledReports.objectDiscoveryReports.empty());
  REQUIRE(lateReports.objectDiscoveryReports.empty());
  REQUIRE_THROWS_AS(
      exact->getKnownObjectClassHandle(objectInstance),
      rti1516_2025::ObjectInstanceNotKnown);
  REQUIRE_THROWS_AS(
      late->getObjectInstanceHandle(objectInstanceName),
      rti1516_2025::ObjectInstanceNotKnown);

  // A queued callback is re-evaluated after unsubscribe; a later subscription
  // plans its own discovery callback.
  REQUIRE_NOTHROW(cancelled->unsubscribeObjectClassAttributes(server, efficiencyOnly));
  REQUIRE_NOTHROW(late->subscribeObjectClassAttributes(server, efficiencyOnly));
  REQUIRE_FALSE(exact->evokeMultipleCallbacks(0.0, 0.0));
  REQUIRE_FALSE(promoted->evokeMultipleCallbacks(0.0, 0.0));
  REQUIRE_FALSE(cancelled->evokeMultipleCallbacks(0.0, 0.0));
  REQUIRE_FALSE(late->evokeMultipleCallbacks(0.0, 0.0));

  REQUIRE(exactReports.objectDiscoveryReports.size() == 1U);
  REQUIRE(exactReports.objectDiscoveryReports.front().objectInstance == objectInstance);
  REQUIRE(exactReports.objectDiscoveryReports.front().objectClass == server);
  REQUIRE(exactReports.objectDiscoveryReports.front().objectInstanceName == objectInstanceName);
  REQUIRE(exactReports.objectDiscoveryReports.front().producingFederate == publisherHandle);
  REQUIRE(promotedReports.objectDiscoveryReports.size() == 1U);
  REQUIRE(promotedReports.objectDiscoveryReports.front().objectClass == employee);
  REQUIRE(cancelledReports.objectDiscoveryReports.empty());
  REQUIRE(lateReports.objectDiscoveryReports.size() == 1U);
  REQUIRE(lateReports.objectDiscoveryReports.front().objectClass == server);
  REQUIRE(immediateReports.objectDiscoveryReports.size() == 1U);
  REQUIRE(publisherReports.objectDiscoveryReports.empty());

  REQUIRE(exact->getKnownObjectClassHandle(objectInstance) == server);
  REQUIRE(promoted->getKnownObjectClassHandle(objectInstance) == employee);
  REQUIRE(late->getKnownObjectClassHandle(objectInstance) == server);
  REQUIRE(immediate->getKnownObjectClassHandle(objectInstance) == server);
  REQUIRE(exact->getObjectInstanceName(objectInstance) == objectInstanceName);
  REQUIRE(promoted->getObjectInstanceHandle(objectInstanceName) == objectInstance);
  REQUIRE_THROWS_AS(
      cancelled->getKnownObjectClassHandle(objectInstance),
      rti1516_2025::ObjectInstanceNotKnown);

  // Repeating a declaration does not produce a duplicate discovery callback.
  REQUIRE_NOTHROW(exact->subscribeObjectClassAttributes(server, efficiencyOnly));
  REQUIRE_FALSE(exact->evokeMultipleCallbacks(0.0, 0.0));
  REQUIRE(exactReports.objectDiscoveryReports.size() == 1U);

  REQUIRE_NOTHROW(immediate->resignFederationExecution(NO_ACTION));
  REQUIRE_NOTHROW(late->resignFederationExecution(NO_ACTION));
  REQUIRE_NOTHROW(cancelled->resignFederationExecution(NO_ACTION));
  REQUIRE_NOTHROW(promoted->resignFederationExecution(NO_ACTION));
  REQUIRE_NOTHROW(exact->resignFederationExecution(NO_ACTION));
  REQUIRE_NOTHROW(publisher->resignFederationExecution(CANCEL_THEN_DELETE_THEN_DIVEST));
  REQUIRE_NOTHROW(publisher->destroyFederationExecution(federationName));
  REQUIRE_NOTHROW(unjoined->disconnect());
  REQUIRE_NOTHROW(publisher->disconnect());
  REQUIRE_NOTHROW(exact->disconnect());
  REQUIRE_NOTHROW(promoted->disconnect());
  REQUIRE_NOTHROW(cancelled->disconnect());
  REQUIRE_NOTHROW(late->disconnect());
  REQUIRE_NOTHROW(immediate->disconnect());
}

TEST_CASE(
    "Embedded object discovery keeps its class after a more-specific subscription",
    "[integration][development-profile][declaration-management][object-management]"
    "[callbacks][object-instance-registration-discovery][discovered-class-stability][2025]"
    "[rti.service.subscribe-object-class-attributes]"
    "[rti.service.get-known-object-class-handle]"
    "[federate.callback.discover-object-instance]") {
  ReportingFederateAmbassador publisherReports;
  ReportingFederateAmbassador subscriberReports;
  auto publisher = makeRti();
  auto subscriber = makeRti();
  auto const federationName = nextFederationName();
  auto const fomModule =
      resourcePath("examples/RestaurantFOMmodule-2025.xml").wstring();

  REQUIRE_NOTHROW(publisher->connect(publisherReports, HLA_EVOKED));
  REQUIRE_NOTHROW(subscriber->connect(subscriberReports, HLA_EVOKED));
  REQUIRE_NOTHROW(publisher->createFederationExecution(
      federationName, fomModule, standard_hla::mom::integer64_time));
  REQUIRE_NOTHROW(publisher->joinFederationExecution(
      L"stable-class-publisher", L"publisher", federationName));
  REQUIRE_NOTHROW(subscriber->joinFederationExecution(
      L"stable-class-subscriber", L"subscriber", federationName));

  auto const employee =
      publisher->getObjectClassHandle(fixture_hla::fom::employee);
  auto const server =
      publisher->getObjectClassHandle(fixture_hla::fom::employee_server);
  auto const name =
      publisher->getAttributeHandle(employee, fixture_hla::fixture::name);
  auto const efficiency = publisher->getAttributeHandle(
      server, fixture_hla::fixture::efficiency);
  REQUIRE(employee.isValid());
  REQUIRE(server.isValid());
  REQUIRE(name.isValid());
  REQUIRE(efficiency.isValid());
  REQUIRE(employee != server);

  REQUIRE_NOTHROW(publisher->publishObjectClassAttributes(
      server, AttributeHandleSet{name, efficiency}));
  REQUIRE_NOTHROW(subscriber->subscribeObjectClassAttributes(
      employee, AttributeHandleSet{name}));
  auto const objectInstance = publisher->registerObjectInstance(server);
  REQUIRE(objectInstance.isValid());
  REQUIRE_FALSE(subscriber->evokeMultipleCallbacks(0.0, 0.0));
  REQUIRE(subscriberReports.objectDiscoveryReports.size() == 1U);
  REQUIRE(subscriberReports.objectDiscoveryReports.front().objectInstance ==
          objectInstance);
  REQUIRE(subscriberReports.objectDiscoveryReports.front().objectClass ==
          employee);
  REQUIRE(subscriber->getKnownObjectClassHandle(objectInstance) == employee);

  // A new, more-specific subscription may not revise the existing discovery.
  REQUIRE_NOTHROW(subscriber->subscribeObjectClassAttributes(
      server, AttributeHandleSet{efficiency}));
  REQUIRE_FALSE(subscriber->evokeMultipleCallbacks(0.0, 0.0));
  REQUIRE(subscriberReports.objectDiscoveryReports.size() == 1U);
  REQUIRE(subscriberReports.objectDiscoveryReports.front().objectClass ==
          employee);
  REQUIRE(subscriber->getKnownObjectClassHandle(objectInstance) == employee);

  REQUIRE_NOTHROW(subscriber->resignFederationExecution(NO_ACTION));
  REQUIRE_NOTHROW(publisher->resignFederationExecution(
      CANCEL_THEN_DELETE_THEN_DIVEST));
  REQUIRE_NOTHROW(publisher->destroyFederationExecution(federationName));
  REQUIRE_NOTHROW(subscriber->disconnect());
  REQUIRE_NOTHROW(publisher->disconnect());
}

TEST_CASE(
    "Embedded registered object class stays fixed across subscriber class views",
    "[integration][development-profile][declaration-management][object-management]"
    "[callbacks][registered-class-stability][2025]"
    "[rti.service.register-object-instance]"
    "[rti.service.get-known-object-class-handle]"
    "[rti.service.subscribe-object-class-attributes]"
    "[federate.callback.discover-object-instance]") {
  ReportingFederateAmbassador publisherReports;
  ReportingFederateAmbassador parentReports;
  ReportingFederateAmbassador childReports;
  auto publisher = makeRti();
  auto parentSubscriber = makeRti();
  auto childSubscriber = makeRti();
  auto const federationName = nextFederationName();
  auto const fomModule =
      resourcePath("examples/RestaurantFOMmodule-2025.xml").wstring();

  REQUIRE_NOTHROW(publisher->connect(publisherReports, HLA_EVOKED));
  REQUIRE_NOTHROW(parentSubscriber->connect(parentReports, HLA_EVOKED));
  REQUIRE_NOTHROW(childSubscriber->connect(childReports, HLA_EVOKED));
  REQUIRE_NOTHROW(publisher->createFederationExecution(
      federationName, fomModule, standard_hla::mom::integer64_time));
  REQUIRE_NOTHROW(publisher->joinFederationExecution(
      L"registered-class-publisher", L"publisher", federationName));
  REQUIRE_NOTHROW(parentSubscriber->joinFederationExecution(
      L"registered-class-parent", L"subscriber", federationName));
  REQUIRE_NOTHROW(childSubscriber->joinFederationExecution(
      L"registered-class-child", L"subscriber", federationName));

  auto const employee =
      publisher->getObjectClassHandle(fixture_hla::fom::employee);
  auto const server =
      publisher->getObjectClassHandle(fixture_hla::fom::employee_server);
  auto const name =
      publisher->getAttributeHandle(employee, fixture_hla::fixture::name);
  auto const efficiency = publisher->getAttributeHandle(
      server, fixture_hla::fixture::efficiency);
  REQUIRE(employee.isValid());
  REQUIRE(server.isValid());
  REQUIRE(name.isValid());
  REQUIRE(efficiency.isValid());
  REQUIRE(employee != server);

  REQUIRE_NOTHROW(publisher->publishObjectClassAttributes(
      server, AttributeHandleSet{name, efficiency}));
  REQUIRE_NOTHROW(parentSubscriber->subscribeObjectClassAttributes(
      employee, AttributeHandleSet{name}));
  REQUIRE_NOTHROW(childSubscriber->subscribeObjectClassAttributes(
      server, AttributeHandleSet{efficiency}));
  auto const objectInstance = publisher->registerObjectInstance(server);
  REQUIRE(objectInstance.isValid());
  REQUIRE(publisher->getKnownObjectClassHandle(objectInstance) == server);
  REQUIRE_FALSE(parentSubscriber->evokeMultipleCallbacks(0.0, 0.0));
  REQUIRE_FALSE(childSubscriber->evokeMultipleCallbacks(0.0, 0.0));
  REQUIRE(parentReports.objectDiscoveryReports.size() == 1U);
  REQUIRE(childReports.objectDiscoveryReports.size() == 1U);
  REQUIRE(parentReports.objectDiscoveryReports.front().objectInstance ==
          objectInstance);
  REQUIRE(childReports.objectDiscoveryReports.front().objectInstance ==
          objectInstance);
  REQUIRE(parentReports.objectDiscoveryReports.front().objectClass == employee);
  REQUIRE(childReports.objectDiscoveryReports.front().objectClass == server);
  REQUIRE(parentSubscriber->getKnownObjectClassHandle(objectInstance) == employee);
  REQUIRE(childSubscriber->getKnownObjectClassHandle(objectInstance) == server);
  REQUIRE(publisher->getKnownObjectClassHandle(objectInstance) == server);

  // Subscriber-specific known classes can differ, but neither subscriber
  // declaration changes the registration class observed by its registrant.
  REQUIRE_NOTHROW(parentSubscriber->subscribeObjectClassAttributes(
      server, AttributeHandleSet{efficiency}));
  REQUIRE_FALSE(parentSubscriber->evokeMultipleCallbacks(0.0, 0.0));
  REQUIRE(parentReports.objectDiscoveryReports.size() == 1U);
  REQUIRE(parentSubscriber->getKnownObjectClassHandle(objectInstance) == employee);
  REQUIRE(publisher->getKnownObjectClassHandle(objectInstance) == server);

  REQUIRE_NOTHROW(childSubscriber->unsubscribeObjectClassAttributes(
      server, AttributeHandleSet{efficiency}));
  REQUIRE_NOTHROW(childSubscriber->subscribeObjectClassAttributes(
      employee, AttributeHandleSet{name}));
  REQUIRE_FALSE(childSubscriber->evokeMultipleCallbacks(0.0, 0.0));
  REQUIRE(childReports.objectDiscoveryReports.size() == 1U);
  REQUIRE(childSubscriber->getKnownObjectClassHandle(objectInstance) == server);
  REQUIRE(publisher->getKnownObjectClassHandle(objectInstance) == server);
  REQUIRE(publisherReports.objectDiscoveryReports.empty());

  REQUIRE_NOTHROW(childSubscriber->resignFederationExecution(NO_ACTION));
  REQUIRE_NOTHROW(parentSubscriber->resignFederationExecution(NO_ACTION));
  REQUIRE_NOTHROW(publisher->resignFederationExecution(
      CANCEL_THEN_DELETE_THEN_DIVEST));
  REQUIRE_NOTHROW(publisher->destroyFederationExecution(federationName));
  REQUIRE_NOTHROW(childSubscriber->disconnect());
  REQUIRE_NOTHROW(parentSubscriber->disconnect());
  REQUIRE_NOTHROW(publisher->disconnect());
}

TEST_CASE(
    "An object instance has one discovered class per federate",
    "[integration][development-profile][declaration-management][object-management]"
    "[callbacks][per-federate-discovered-class][2025]"
    "[rti.service.register-object-instance]"
    "[rti.service.subscribe-object-class-attributes]"
    "[rti.service.get-known-object-class-handle]"
    "[federate.callback.discover-object-instance]") {
  ReportingFederateAmbassador publisherReports;
  ReportingFederateAmbassador employeeReports;
  ReportingFederateAmbassador serverReports;
  auto publisher = makeRti();
  auto employeeSubscriber = makeRti();
  auto serverSubscriber = makeRti();
  auto const federationName = nextFederationName();
  auto const fomModule =
      resourcePath("examples/RestaurantFOMmodule-2025.xml").wstring();

  REQUIRE_NOTHROW(publisher->connect(publisherReports, HLA_EVOKED));
  REQUIRE_NOTHROW(employeeSubscriber->connect(employeeReports, HLA_EVOKED));
  REQUIRE_NOTHROW(serverSubscriber->connect(serverReports, HLA_EVOKED));
  REQUIRE_NOTHROW(publisher->createFederationExecution(
      federationName, fomModule, standard_hla::mom::integer64_time));
  REQUIRE_NOTHROW(publisher->joinFederationExecution(
      L"per-class-publisher", L"publisher", federationName));
  REQUIRE_NOTHROW(employeeSubscriber->joinFederationExecution(
      L"per-class-employee-subscriber", L"subscriber", federationName));
  REQUIRE_NOTHROW(serverSubscriber->joinFederationExecution(
      L"per-class-server-subscriber", L"subscriber", federationName));

  auto const employee =
      publisher->getObjectClassHandle(fixture_hla::fom::employee);
  auto const server =
      publisher->getObjectClassHandle(fixture_hla::fom::employee_server);
  auto const name =
      publisher->getAttributeHandle(employee, fixture_hla::fixture::name);
  auto const efficiency = publisher->getAttributeHandle(
      server, fixture_hla::fixture::efficiency);
  REQUIRE(employee.isValid());
  REQUIRE(server.isValid());
  REQUIRE(name.isValid());
  REQUIRE(efficiency.isValid());
  REQUIRE(employee != server);

  REQUIRE_NOTHROW(publisher->publishObjectClassAttributes(
      server, AttributeHandleSet{name, efficiency}));
  REQUIRE_NOTHROW(employeeSubscriber->subscribeObjectClassAttributes(
      employee, AttributeHandleSet{name}));
  REQUIRE_NOTHROW(serverSubscriber->subscribeObjectClassAttributes(
      server, AttributeHandleSet{efficiency}));

  auto const objectInstance = publisher->registerObjectInstance(server);
  REQUIRE(objectInstance.isValid());
  REQUIRE_FALSE(employeeSubscriber->evokeMultipleCallbacks(0.0, 0.0));
  REQUIRE_FALSE(serverSubscriber->evokeMultipleCallbacks(0.0, 0.0));
  REQUIRE(employeeReports.objectDiscoveryReports.size() == 1U);
  REQUIRE(serverReports.objectDiscoveryReports.size() == 1U);
  REQUIRE(employeeReports.objectDiscoveryReports.front().objectInstance ==
          objectInstance);
  REQUIRE(serverReports.objectDiscoveryReports.front().objectInstance ==
          objectInstance);
  REQUIRE(employeeReports.objectDiscoveryReports.front().objectClass == employee);
  REQUIRE(serverReports.objectDiscoveryReports.front().objectClass == server);
  REQUIRE(employeeSubscriber->getKnownObjectClassHandle(objectInstance) ==
          employee);
  REQUIRE(serverSubscriber->getKnownObjectClassHandle(objectInstance) ==
          server);

  REQUIRE_NOTHROW(employeeSubscriber->resignFederationExecution(NO_ACTION));
  REQUIRE_NOTHROW(serverSubscriber->resignFederationExecution(NO_ACTION));
  REQUIRE_NOTHROW(publisher->resignFederationExecution(
      CANCEL_THEN_DELETE_THEN_DIVEST));
  REQUIRE_NOTHROW(publisher->destroyFederationExecution(federationName));
  REQUIRE_NOTHROW(employeeSubscriber->disconnect());
  REQUIRE_NOTHROW(serverSubscriber->disconnect());
  REQUIRE_NOTHROW(publisher->disconnect());
}
