#include <catch2/catch_test_macros.hpp>

#include "internal/fom/hla_names.hpp"
#include "hla_test_names.hpp"

#include <RTI/NullFederateAmbassador.h>
#include <RTI/RTI1516.h>

#include <atomic>
#include <filesystem>
#include <memory>
#include <string>
#include <vector>

#ifndef UMBRA_SOURCE_DIRECTORY
#error "The object-instance deletion test requires the Umbra source directory."
#endif

namespace {

namespace standard_hla = umbra::detail::hla::wide;
namespace fixture_hla = umbra::test::hla::wide;

using rti1516_2025::AttributeHandle;
using rti1516_2025::AttributeHandleSet;
using rti1516_2025::AttributeHandleValueMap;
using rti1516_2025::FederateHandle;
using rti1516_2025::HLA_EVOKED;
using rti1516_2025::NO_ACTION;
using rti1516_2025::CANCEL_THEN_DELETE_THEN_DIVEST;
using rti1516_2025::ObjectClassHandle;
using rti1516_2025::ObjectInstanceHandle;
using rti1516_2025::RTIambassador;
using rti1516_2025::RTIambassadorFactory;
using rti1516_2025::TransportationTypeHandle;
using rti1516_2025::VariableLengthData;

std::wstring nextFederationName() {
  static std::atomic_uint64_t sequence{0U};
  return L"object-instance-deletion-" +
      std::to_wstring(sequence.fetch_add(1U, std::memory_order_relaxed));
}

std::filesystem::path resourcePath(std::filesystem::path const& relativePath) {
  return std::filesystem::path(UMBRA_SOURCE_DIRECTORY) / "third_party" /
      "ieee1516.2-2025" / "resources" / relativePath;
}

std::vector<unsigned char> bytes(VariableLengthData const& value) {
  auto const* data = static_cast<unsigned char const*>(value.data());
  if (data == nullptr || value.size() == 0U) {
    return {};
  }
  return {data, data + value.size()};
}

class ReportingFederateAmbassador final
    : public rti1516_2025::NullFederateAmbassador {
 public:
  struct ObjectDiscoveryReport final {
    ObjectInstanceHandle objectInstance;
    ObjectClassHandle objectClass;
    std::wstring objectInstanceName;
    FederateHandle producingFederate;
  };

  struct ObjectRemovalReport final {
    ObjectInstanceHandle objectInstance;
    VariableLengthData userSuppliedTag;
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

  void removeObjectInstance(
      ObjectInstanceHandle const& objectInstance,
      VariableLengthData const& userSuppliedTag,
      FederateHandle const& producingFederate) override {
    objectRemovalReports.push_back({
        objectInstance,
        userSuppliedTag,
        producingFederate,
    });
  }

  std::vector<ObjectDiscoveryReport> objectDiscoveryReports;
  std::vector<ObjectRemovalReport> objectRemovalReports;
};

std::unique_ptr<RTIambassador> makeRti() {
  RTIambassadorFactory factory;
  return factory.createRTIambassador();
}

void drainCallbacks(RTIambassador& rti) {
  for (int pass = 0; pass != 128; ++pass) {
    if (!rti.evokeCallback(0.0)) {
      break;
    }
  }
}

}  // namespace

TEST_CASE(
    "Embedded receive-order Delete Object Instance honors 2025 removal lifecycle",
    "[integration][development-profile][object-management][callbacks]"
    "[object-instance-deletion]"
    "[rti.service.delete-object-instance]"
    "[federate.callback.remove-object-instance]") {
  rti1516_2025::NullFederateAmbassador unjoinedFederate;
  ReportingFederateAmbassador publisherReports;
  ReportingFederateAmbassador receiverReports;
  auto unjoined = makeRti();
  auto publisher = makeRti();
  auto receiver = makeRti();
  auto const federationName = nextFederationName();
  auto const fomModule = resourcePath("examples/RestaurantFOMmodule-2025.xml").wstring();
  ObjectInstanceHandle invalidObjectInstance;

  REQUIRE_THROWS_AS(
      publisher->deleteObjectInstance(invalidObjectInstance, VariableLengthData()),
      rti1516_2025::NotConnected);
  REQUIRE_NOTHROW(unjoined->connect(unjoinedFederate, HLA_EVOKED));
  REQUIRE_THROWS_AS(
      unjoined->deleteObjectInstance(invalidObjectInstance, VariableLengthData()),
      rti1516_2025::FederateNotExecutionMember);

  REQUIRE_NOTHROW(publisher->connect(publisherReports, HLA_EVOKED));
  REQUIRE_NOTHROW(receiver->connect(receiverReports, HLA_EVOKED));
  REQUIRE_NOTHROW(publisher->createFederationExecution(
      federationName,
      fomModule,
      standard_hla::mom::integer64_time));
  FederateHandle publisherHandle;
  REQUIRE_NOTHROW(publisherHandle = publisher->joinFederationExecution(
      L"object-deletion-publisher", L"publisher", federationName));
  REQUIRE_NOTHROW(receiver->joinFederationExecution(
      L"object-deletion-receiver", L"receiver", federationName));

  auto const server = publisher->getObjectClassHandle(
      fixture_hla::fom::employee_server);
  auto const efficiency = publisher->getAttributeHandle(
      server,
      fixture_hla::fixture::efficiency);
  REQUIRE(server.isValid());
  REQUIRE(efficiency.isValid());
  AttributeHandleSet const efficiencyOnly{efficiency};
  REQUIRE_NOTHROW(publisher->publishObjectClassAttributes(server, efficiencyOnly));
  REQUIRE_NOTHROW(receiver->subscribeObjectClassAttributes(server, efficiencyOnly));

  ObjectInstanceHandle const objectInstance = publisher->registerObjectInstance(server);
  REQUIRE(receiverReports.objectDiscoveryReports.empty());
  drainCallbacks(*receiver);
  REQUIRE(receiverReports.objectDiscoveryReports.size() == 1U);
  REQUIRE(receiverReports.objectDiscoveryReports.front().objectInstance == objectInstance);
  REQUIRE(receiver->getKnownObjectClassHandle(objectInstance) == server);
  REQUIRE(publisher->getKnownObjectClassHandle(objectInstance) == server);

  unsigned char const tagBytes[] = {'d', 'e', 'l'};
  VariableLengthData const tag(tagBytes, sizeof(tagBytes));
  ObjectInstanceHandle const unknownObject;
  REQUIRE_THROWS_AS(
      publisher->deleteObjectInstance(unknownObject, tag),
      rti1516_2025::ObjectInstanceNotKnown);
  REQUIRE_THROWS_AS(
      receiver->deleteObjectInstance(objectInstance, tag),
      rti1516_2025::DeletePrivilegeNotHeld);

  REQUIRE(receiverReports.objectRemovalReports.empty());
  REQUIRE(publisherReports.objectRemovalReports.empty());
  REQUIRE_NOTHROW(publisher->deleteObjectInstance(objectInstance, tag));

  // The invoker becomes unknown before the public call returns; recipients
  // remain known until their queued Remove Object Instance callback starts.
  REQUIRE_THROWS_AS(
      publisher->getKnownObjectClassHandle(objectInstance),
      rti1516_2025::ObjectInstanceNotKnown);
  REQUIRE(receiver->getKnownObjectClassHandle(objectInstance) == server);
  REQUIRE(receiverReports.objectRemovalReports.empty());
  REQUIRE(publisherReports.objectRemovalReports.empty());

  drainCallbacks(*receiver);
  REQUIRE(receiverReports.objectRemovalReports.size() == 1U);
  auto const& removal = receiverReports.objectRemovalReports.front();
  REQUIRE(removal.objectInstance == objectInstance);
  REQUIRE(bytes(removal.userSuppliedTag) ==
          std::vector<unsigned char>(tagBytes, tagBytes + sizeof(tagBytes)));
  REQUIRE(removal.producingFederate == publisherHandle);
  REQUIRE_THROWS_AS(
      receiver->getKnownObjectClassHandle(objectInstance),
      rti1516_2025::ObjectInstanceNotKnown);

  REQUIRE_NOTHROW(receiver->resignFederationExecution(NO_ACTION));
  REQUIRE_NOTHROW(publisher->resignFederationExecution(
      CANCEL_THEN_DELETE_THEN_DIVEST));
  REQUIRE_NOTHROW(publisher->destroyFederationExecution(federationName));
  REQUIRE_NOTHROW(unjoined->disconnect());
  REQUIRE_NOTHROW(receiver->disconnect());
  REQUIRE_NOTHROW(publisher->disconnect());
}
