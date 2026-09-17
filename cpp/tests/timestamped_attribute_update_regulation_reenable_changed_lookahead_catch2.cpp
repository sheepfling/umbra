#include <catch2/catch_test_macros.hpp>

#include "internal/fom/hla_names.hpp"
#include "hla_test_names.hpp"

#include <RTI/NullFederateAmbassador.h>
#include <RTI/RTI1516.h>
#include <RTI/time/HLAinteger64Interval.h>
#include <RTI/time/HLAinteger64Time.h>

#include <atomic>
#include <filesystem>
#include <memory>
#include <string>
#include <vector>

#ifndef UMBRA_SOURCE_DIRECTORY
#error "The changed-lookahead timestamped attribute-update test requires the Umbra source directory."
#endif

namespace {

namespace standard_hla = umbra::detail::hla::wide;
namespace fixture_hla = umbra::test::hla::wide;

using rti1516_2025::AttributeHandle;
using rti1516_2025::AttributeHandleSet;
using rti1516_2025::AttributeHandleValueMap;
using rti1516_2025::FederateHandle;
using rti1516_2025::HLA_EVOKED;
using rti1516_2025::ObjectClassHandle;
using rti1516_2025::ObjectInstanceHandle;
using rti1516_2025::OrderType;
using rti1516_2025::RECEIVE;
using rti1516_2025::RTIambassador;
using rti1516_2025::RTIambassadorFactory;
using rti1516_2025::TIMESTAMP;
using rti1516_2025::TransportationTypeHandle;
using rti1516_2025::VariableLengthData;

std::vector<unsigned char> variableLengthDataBytes(
    VariableLengthData const& value) {
  auto const* bytes = static_cast<unsigned char const*>(value.data());
  if (bytes == nullptr) {
    return {};
  }
  return {bytes, bytes + value.size()};
}

std::wstring nextFederationName() {
  static std::atomic_uint64_t sequence{0U};
  return L"timestamped-attribute-regulation-reenable-changed-lookahead-" +
      std::to_wstring(sequence.fetch_add(1U, std::memory_order_relaxed));
}

std::filesystem::path sourcePath(std::filesystem::path const& relativePath) {
  return std::filesystem::path(UMBRA_SOURCE_DIRECTORY) / relativePath;
}

class ReportingFederateAmbassador final
    : public rti1516_2025::NullFederateAmbassador {
 public:
  struct ReflectionReport final {
    ObjectInstanceHandle objectInstance;
    AttributeHandleValueMap attributeValues;
    VariableLengthData userSuppliedTag;
    TransportationTypeHandle transportationType;
    FederateHandle producingFederate;
    bool sentRegionsSupplied = false;
    std::wstring timeImplementationName;
    std::wstring timeValue;
    OrderType sentOrderType = RECEIVE;
    OrderType receivedOrderType = RECEIVE;
    bool retractionSupplied = false;
    bool retractionValid = false;
  };

  void timeRegulationEnabled(rti1516_2025::LogicalTime const&) override {
    ++timeRegulationEnabledCount;
  }

  void discoverObjectInstance(
      ObjectInstanceHandle const& objectInstance,
      ObjectClassHandle const&,
      std::wstring const&,
      FederateHandle const&) override {
    objectDiscoveryReports.push_back(objectInstance);
  }

  void reflectAttributeValues(
      ObjectInstanceHandle const& objectInstance,
      AttributeHandleValueMap const& attributeValues,
      VariableLengthData const& userSuppliedTag,
      TransportationTypeHandle const& transportationType,
      FederateHandle const& producingFederate,
      rti1516_2025::RegionHandleSet const* optionalSentRegions,
      rti1516_2025::LogicalTime const& time,
      OrderType sentOrderType,
      OrderType receivedOrderType,
      rti1516_2025::MessageRetractionHandle const* optionalRetraction) override {
    attributeReflectionReports.push_back({
        objectInstance,
        attributeValues,
        userSuppliedTag,
        transportationType,
        producingFederate,
        optionalSentRegions != nullptr,
        time.implementationName(),
        time.toString(),
        sentOrderType,
        receivedOrderType,
        optionalRetraction != nullptr,
        optionalRetraction != nullptr && optionalRetraction->isValid(),
    });
    callbackOrder.push_back("reflect");
  }

  void timeAdvanceGrant(rti1516_2025::LogicalTime const&) override {
    ++timeAdvanceGrantCount;
    callbackOrder.push_back("grant");
  }

  std::vector<ObjectInstanceHandle> objectDiscoveryReports;
  std::vector<ReflectionReport> attributeReflectionReports;
  std::size_t timeRegulationEnabledCount = 0U;
  std::size_t timeAdvanceGrantCount = 0U;
  std::vector<std::string> callbackOrder;
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
    "Embedded queued timestamped attribute update survives time-regulation disable and re-enable with changed lookahead",
    "[integration][development-profile][object-management][time-management]"
    "[timestamped-attribute-update][tso][re-enable][regulation-disable][changed-lookahead]"
    "[timestamped-attribute-update-regulation-reenable-changed-lookahead]"
    "[rti.service.update-attribute-values][rti.service.enable-time-regulation]"
    "[rti.service.disable-time-regulation][rti.service.query-lookahead]"
    "[rti.service.time-advance-request][rti.service.enable-time-constrained]"
    "[rti.service.publish-object-class-attributes][rti.service.subscribe-object-class-attributes]"
    "[rti.service.change-default-attribute-order-type]"
    "[federate.callback.reflect-attribute-values][federate.callback.time-advance-grant]"
    "[federate.callback.time-regulation-enabled]") {
  ReportingFederateAmbassador publisherReports;
  ReportingFederateAmbassador receiverReports;
  auto publisher = makeRti();
  auto receiver = makeRti();
  auto const federationName = nextFederationName();
  auto const fomModule = sourcePath(
      std::filesystem::path("cpp") / "tests" / "data" /
      "attribute-update-passel-fom.xml");
  unsigned char const valueBytes[] = {0xB7, 0x2C, 0x91};
  unsigned char const tagBytes[] = {0x43, 0x48, 0x47, 0x41};
  VariableLengthData const tag(tagBytes, sizeof(tagBytes));

  REQUIRE_NOTHROW(publisher->connect(publisherReports, HLA_EVOKED));
  REQUIRE_NOTHROW(receiver->connect(receiverReports, HLA_EVOKED));
  REQUIRE_NOTHROW(publisher->createFederationExecution(
      federationName,
      fomModule.wstring(),
      standard_hla::mom::integer64_time));
  FederateHandle publisherHandle;
  REQUIRE_NOTHROW(publisherHandle = publisher->joinFederationExecution(
      L"changed-lookahead-attribute-publisher", L"publisher", federationName));
  REQUIRE_NOTHROW(receiver->joinFederationExecution(
      L"changed-lookahead-attribute-receiver", L"subscriber", federationName));

  auto const objectClass = publisher->getObjectClassHandle(
      fixture_hla::fom::attribute_fixture_child);
  auto const reliable = publisher->getAttributeHandle(
      objectClass,
      fixture_hla::fixture::reliable_base_a);
  REQUIRE(objectClass.isValid());
  REQUIRE(reliable.isValid());
  AttributeHandleSet const attributes{reliable};
  AttributeHandleValueMap attributeValues;
  attributeValues.emplace(
      reliable,
      VariableLengthData(valueBytes, sizeof(valueBytes)));

  REQUIRE_NOTHROW(publisher->publishObjectClassAttributes(objectClass, attributes));
  REQUIRE_NOTHROW(receiver->subscribeObjectClassAttributes(objectClass, attributes));
  // The fixture defaults to Receive. Select timestamp order so this lifecycle
  // case exercises the TSO update queue rather than an immediate reflection.
  REQUIRE_NOTHROW(publisher->changeDefaultAttributeOrderType(
      objectClass,
      attributes,
      TIMESTAMP));
  ObjectInstanceHandle objectInstance;
  REQUIRE_NOTHROW(objectInstance = publisher->registerObjectInstance(objectClass));
  drainCallbacks(*receiver);
  REQUIRE(receiverReports.objectDiscoveryReports.size() == 1U);

  REQUIRE_NOTHROW(receiver->enableTimeConstrained());
  drainCallbacks(*receiver);
  REQUIRE_NOTHROW(publisher->enableTimeRegulation(
      rti1516_2025::HLAinteger64Interval(1)));
  drainCallbacks(*publisher);
  REQUIRE(publisherReports.timeRegulationEnabledCount == 1U);

  auto const retraction = publisher->updateAttributeValues(
      objectInstance,
      attributeValues,
      tag,
      rti1516_2025::HLAinteger64Time(5));
  REQUIRE(retraction.isValid());
  REQUIRE(receiverReports.attributeReflectionReports.empty());

  // The accepted passel belongs to the joined-federate lifetime. Disabling
  // regulation removes the producer role, while re-enabling with lookahead
  // three must retain the same passel and establish the new GALT boundary.
  REQUIRE_NOTHROW(publisher->disableTimeRegulation());
  REQUIRE(publisherReports.timeRegulationEnabledCount == 1U);
  REQUIRE_NOTHROW(publisher->enableTimeRegulation(
      rti1516_2025::HLAinteger64Interval(3)));
  drainCallbacks(*publisher);
  REQUIRE(publisherReports.timeRegulationEnabledCount == 2U);
  rti1516_2025::HLAinteger64Interval changedLookahead;
  REQUIRE_NOTHROW(publisher->queryLookahead(changedLookahead));
  REQUIRE(changedLookahead.getInterval() == 3);

  // With current time zero and lookahead three, the producer's advance to two
  // raises GALT to the queued timestamp five. Reflection must precede the
  // receiver's matching grant and preserve the original designator.
  receiverReports.callbackOrder.clear();
  REQUIRE_NOTHROW(receiver->timeAdvanceRequest(
      rti1516_2025::HLAinteger64Time(5)));
  REQUIRE(receiverReports.attributeReflectionReports.empty());
  REQUIRE_NOTHROW(publisher->timeAdvanceRequest(
      rti1516_2025::HLAinteger64Time(2)));
  drainCallbacks(*publisher);
  drainCallbacks(*receiver);
  REQUIRE(receiverReports.attributeReflectionReports.size() == 1U);
  REQUIRE(receiverReports.timeAdvanceGrantCount == 1U);
  REQUIRE(receiverReports.callbackOrder ==
          std::vector<std::string>{"reflect", "grant"});

  auto const& reflection = receiverReports.attributeReflectionReports.front();
  REQUIRE(reflection.objectInstance == objectInstance);
  REQUIRE(reflection.attributeValues.size() == 1U);
  REQUIRE(reflection.attributeValues.contains(reliable));
  REQUIRE(variableLengthDataBytes(reflection.attributeValues.at(reliable)) ==
          std::vector<unsigned char>(valueBytes, valueBytes + sizeof(valueBytes)));
  REQUIRE(variableLengthDataBytes(reflection.userSuppliedTag) ==
          std::vector<unsigned char>(tagBytes, tagBytes + sizeof(tagBytes)));
  REQUIRE(reflection.producingFederate == publisherHandle);
  REQUIRE_FALSE(reflection.sentRegionsSupplied);
  REQUIRE(reflection.timeImplementationName == standard_hla::mom::integer64_time);
  REQUIRE(reflection.timeValue == L"5");
  REQUIRE(reflection.sentOrderType == rti1516_2025::TIMESTAMP);
  REQUIRE(reflection.receivedOrderType == rti1516_2025::TIMESTAMP);
  REQUIRE(reflection.retractionSupplied);
  REQUIRE(reflection.retractionValid);
  REQUIRE_THROWS_AS(
      publisher->retract(retraction),
      rti1516_2025::MessageCanNoLongerBeRetracted);

  REQUIRE_NOTHROW(receiver->resignFederationExecution(rti1516_2025::NO_ACTION));
  REQUIRE_NOTHROW(publisher->resignFederationExecution(
      rti1516_2025::CANCEL_THEN_DELETE_THEN_DIVEST));
  REQUIRE_NOTHROW(publisher->destroyFederationExecution(federationName));
  REQUIRE_NOTHROW(receiver->disconnect());
  REQUIRE_NOTHROW(publisher->disconnect());
}
