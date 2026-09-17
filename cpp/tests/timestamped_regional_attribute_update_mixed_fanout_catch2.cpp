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
#error "The mixed-fanout regional timestamped attribute-update test requires the Umbra source directory."
#endif

namespace {

namespace standard_hla = umbra::detail::hla::wide;
namespace fixture_hla = umbra::test::hla::wide;

using rti1516_2025::AttributeHandle;
using rti1516_2025::AttributeHandleSet;
using rti1516_2025::AttributeHandleSetRegionHandleSetPairVector;
using rti1516_2025::AttributeHandleValueMap;
using rti1516_2025::FederateHandle;
using rti1516_2025::HLA_EVOKED;
using rti1516_2025::HLA_IMMEDIATE;
using rti1516_2025::ObjectClassHandle;
using rti1516_2025::ObjectInstanceHandle;
using rti1516_2025::OrderType;
using rti1516_2025::RECEIVE;
using rti1516_2025::RegionHandleSet;
using rti1516_2025::RangeBounds;
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
  return L"timestamped-regional-attribute-update-mixed-fanout-" +
      std::to_wstring(sequence.fetch_add(1U, std::memory_order_relaxed));
}

std::filesystem::path resourcePath(std::filesystem::path const& relativePath) {
  return std::filesystem::path(UMBRA_SOURCE_DIRECTORY) /
      "third_party" / "ieee1516.2-2025" / "resources" / relativePath;
}

class ReportingFederateAmbassador final
    : public rti1516_2025::NullFederateAmbassador {
 public:
  struct TimeAdvanceGrantReport final {
    std::wstring timeImplementationName;
    std::wstring value;
  };

  struct ReflectionReport final {
    ObjectInstanceHandle objectInstance;
    AttributeHandleValueMap attributeValues;
    VariableLengthData userSuppliedTag;
    TransportationTypeHandle transportationType;
    FederateHandle producingFederate;
    bool sentRegionsSupplied = false;
    RegionHandleSet sentRegions;
    std::wstring timeImplementationName;
    std::wstring timeValue;
    OrderType sentOrderType = RECEIVE;
    OrderType receivedOrderType = RECEIVE;
    bool retractionSupplied = false;
    bool retractionValid = false;
  };

  struct RequestRetractionReport final {
    bool retractionValid = false;
    VariableLengthData encodedRetraction;
  };

  void discoverObjectInstance(
      ObjectInstanceHandle const& objectInstance,
      ObjectClassHandle const&,
      std::wstring const&,
      FederateHandle const&) override {
    discoveredObjectInstances.push_back(objectInstance);
  }

  void reflectAttributeValues(
      ObjectInstanceHandle const& objectInstance,
      AttributeHandleValueMap const& attributeValues,
      VariableLengthData const& userSuppliedTag,
      TransportationTypeHandle const& transportationType,
      FederateHandle const& producingFederate,
      RegionHandleSet const* optionalSentRegions,
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
        optionalSentRegions == nullptr ? RegionHandleSet{} : *optionalSentRegions,
        time.implementationName(),
        time.toString(),
        sentOrderType,
        receivedOrderType,
        optionalRetraction != nullptr,
        optionalRetraction != nullptr && optionalRetraction->isValid(),
    });
    callbackOrder.push_back("reflect");
  }

  void requestRetraction(
      rti1516_2025::MessageRetractionHandle const& retraction) override {
    requestRetractionReports.push_back({retraction.isValid(), retraction.encode()});
    callbackOrder.push_back("request-retraction");
  }

  void timeAdvanceGrant(rti1516_2025::LogicalTime const& time) override {
    timeAdvanceGrantReports.push_back({time.implementationName(), time.toString()});
    callbackOrder.push_back("grant");
  }

  std::vector<ObjectInstanceHandle> discoveredObjectInstances;
  std::vector<ReflectionReport> attributeReflectionReports;
  std::vector<RequestRetractionReport> requestRetractionReports;
  std::vector<TimeAdvanceGrantReport> timeAdvanceGrantReports;
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
    "Embedded timestamped regional Update Attribute Values carries recipient-gated regions across mixed fanout",
    "[integration][development-profile][object-management][ddm][time-management]"
    "[timestamped-regional-attribute-update-mixed-fanout][mixed-fanout][multi-federate-callback-ordering]"
    "[rti.service.update-attribute-values][rti.service.retract]"
    "[rti.service.register-object-instance-with-regions]"
    "[rti.service.associate-regions-for-updates]"
    "[rti.service.subscribe-object-class-attributes-with-regions]"
    "[rti.service.time-advance-request][federate.callback.reflect-attribute-values]"
    "[federate.callback.time-advance-grant][federate.callback.request-retraction]") {
  ReportingFederateAmbassador publisherReports;
  ReportingFederateAmbassador receiverReports;
  ReportingFederateAmbassador immediateReports;
  auto publisher = makeRti();
  auto receiver = makeRti();
  auto immediate = makeRti();
  auto const federationName = nextFederationName();
  auto const fomModule = resourcePath("examples/RestaurantFOMmodule-2025.xml").wstring();
  unsigned char const valueBytes[] = {0x5A, 0x25};
  unsigned char const tagBytes[] = {0x72, 0x54, 0x53, 0x4F};
  VariableLengthData const tag(tagBytes, sizeof(tagBytes));

  REQUIRE_NOTHROW(publisher->connect(publisherReports, HLA_EVOKED));
  REQUIRE_NOTHROW(receiver->connect(receiverReports, HLA_EVOKED));
  REQUIRE_NOTHROW(immediate->connect(immediateReports, HLA_IMMEDIATE));
  REQUIRE_NOTHROW(
      publisher->createFederationExecution(federationName, fomModule, standard_hla::mom::integer64_time));
  FederateHandle publisherHandle;
  REQUIRE_NOTHROW(publisherHandle = publisher->joinFederationExecution(
      L"timestamped-regional-attribute-publisher", L"publisher", federationName));
  REQUIRE_NOTHROW(receiver->joinFederationExecution(
      L"timestamped-regional-attribute-receiver", L"subscriber", federationName));
  REQUIRE_NOTHROW(immediate->joinFederationExecution(
      L"timestamped-regional-attribute-immediate", L"subscriber", federationName));
  REQUIRE_FALSE(receiver->getConveyRegionDesignatorSetsSwitch());
  REQUIRE_FALSE(immediate->getConveyRegionDesignatorSetsSwitch());

  auto const soda = publisher->getObjectClassHandle(fixture_hla::fom::food_drink_soda);
  auto const flavor = publisher->getAttributeHandle(soda, fixture_hla::fixture::flavor);
  auto const sodaFlavor = publisher->getDimensionHandle(fixture_hla::fixture::soda_flavor);
  REQUIRE(soda.isValid());
  REQUIRE(flavor.isValid());
  REQUIRE(sodaFlavor.isValid());
  AttributeHandleSet const flavorOnly{flavor};
  REQUIRE_NOTHROW(publisher->publishObjectClassAttributes(soda, flavorOnly));
  REQUIRE_NOTHROW(publisher->changeDefaultAttributeOrderType(soda, flavorOnly, TIMESTAMP));

  auto const publisherRegion = publisher->createRegion(rti1516_2025::DimensionHandleSet{sodaFlavor});
  auto const receiverRegion = receiver->createRegion(rti1516_2025::DimensionHandleSet{sodaFlavor});
  auto const immediateRegion = immediate->createRegion(rti1516_2025::DimensionHandleSet{sodaFlavor});
  REQUIRE_NOTHROW(publisher->setRangeBounds(
      publisherRegion, sodaFlavor, RangeBounds(0UL, 2UL)));
  REQUIRE_NOTHROW(publisher->commitRegionModifications(RegionHandleSet{publisherRegion}));
  REQUIRE_NOTHROW(receiver->setRangeBounds(
      receiverRegion, sodaFlavor, RangeBounds(1UL, 3UL)));
  REQUIRE_NOTHROW(receiver->commitRegionModifications(RegionHandleSet{receiverRegion}));
  REQUIRE_NOTHROW(immediate->setRangeBounds(
      immediateRegion, sodaFlavor, RangeBounds(1UL, 3UL)));
  REQUIRE_NOTHROW(immediate->commitRegionModifications(RegionHandleSet{immediateRegion}));

  AttributeHandleSetRegionHandleSetPairVector const publisherPair{{
      flavorOnly,
      RegionHandleSet{publisherRegion},
  }};
  AttributeHandleSetRegionHandleSetPairVector const receiverPair{{
      flavorOnly,
      RegionHandleSet{receiverRegion},
  }};
  AttributeHandleSetRegionHandleSetPairVector const immediatePair{{
      flavorOnly,
      RegionHandleSet{immediateRegion},
  }};
  ObjectInstanceHandle objectInstance;
  REQUIRE_NOTHROW(objectInstance = publisher->registerObjectInstanceWithRegions(
      soda,
      publisherPair));
  REQUIRE_NOTHROW(receiver->subscribeObjectClassAttributesWithRegions(
      soda,
      receiverPair));
  REQUIRE_NOTHROW(immediate->subscribeObjectClassAttributesWithRegions(
      soda,
      immediatePair));
  drainCallbacks(*receiver);
  REQUIRE(receiverReports.discoveredObjectInstances.size() == 1U);
  REQUIRE(immediateReports.discoveredObjectInstances.size() == 1U);

  REQUIRE_NOTHROW(receiver->enableTimeConstrained());
  drainCallbacks(*receiver);
  REQUIRE_NOTHROW(publisher->enableTimeRegulation(
      rti1516_2025::HLAinteger64Interval(5)));
  drainCallbacks(*publisher);

  AttributeHandleValueMap attributeValues;
  attributeValues.emplace(flavor, VariableLengthData(valueBytes, sizeof(valueBytes)));
  REQUIRE_THROWS_AS(
      publisher->updateAttributeValues(
          objectInstance,
          attributeValues,
          tag,
          rti1516_2025::HLAinteger64Time(4)),
      rti1516_2025::InvalidLogicalTime);

  receiverReports.callbackOrder.clear();
  immediateReports.callbackOrder.clear();
  auto const firstHandle = publisher->updateAttributeValues(
      objectInstance,
      attributeValues,
      tag,
      rti1516_2025::HLAinteger64Time(6));
  REQUIRE(firstHandle.isValid());
  REQUIRE(receiverReports.attributeReflectionReports.empty());
  REQUIRE(immediateReports.attributeReflectionReports.size() == 1U);
  auto const& immediateFirstReport = immediateReports.attributeReflectionReports.back();
  REQUIRE(immediateFirstReport.objectInstance == objectInstance);
  REQUIRE(immediateFirstReport.timeValue == L"6");
  REQUIRE(immediateFirstReport.sentOrderType == TIMESTAMP);
  REQUIRE(immediateFirstReport.receivedOrderType == RECEIVE);
  REQUIRE(immediateFirstReport.retractionSupplied);
  REQUIRE(immediateFirstReport.retractionValid);
  REQUIRE_FALSE(immediateFirstReport.sentRegionsSupplied);
  REQUIRE_NOTHROW(receiver->timeAdvanceRequest(rti1516_2025::HLAinteger64Time(6)));
  REQUIRE_NOTHROW(publisher->retract(firstHandle));
  REQUIRE(immediateReports.requestRetractionReports.size() == 1U);
  REQUIRE(immediateReports.requestRetractionReports.front().retractionValid);
  REQUIRE(variableLengthDataBytes(
              immediateReports.requestRetractionReports.front().encodedRetraction) ==
          variableLengthDataBytes(firstHandle.encode()));
  REQUIRE(immediateReports.callbackOrder ==
          std::vector<std::string>{"reflect", "request-retraction"});
  // Retracting the immediate copy must not leak a callback into the
  // constrained recipient's still-pending queue.
  REQUIRE(receiverReports.callbackOrder.empty());
  REQUIRE_NOTHROW(publisher->timeAdvanceRequest(rti1516_2025::HLAinteger64Time(2)));
  drainCallbacks(*publisher);
  drainCallbacks(*receiver);
  REQUIRE(receiverReports.timeAdvanceGrantReports.size() == 1U);
  REQUIRE(receiverReports.attributeReflectionReports.empty());
  REQUIRE(receiverReports.callbackOrder == std::vector<std::string>{"grant"});
  receiverReports.callbackOrder.clear();
  REQUIRE(immediateReports.attributeReflectionReports.size() == 1U);

  auto const secondHandle = publisher->updateAttributeValues(
      objectInstance,
      attributeValues,
      tag,
      rti1516_2025::HLAinteger64Time(7));
  REQUIRE(secondHandle.isValid());
  REQUIRE_NOTHROW(receiver->timeAdvanceRequest(rti1516_2025::HLAinteger64Time(7)));
  REQUIRE(immediateReports.attributeReflectionReports.size() == 2U);
  auto const& immediateSecondReport = immediateReports.attributeReflectionReports.back();
  REQUIRE(immediateSecondReport.timeValue == L"7");
  REQUIRE(immediateSecondReport.sentOrderType == TIMESTAMP);
  REQUIRE(immediateSecondReport.receivedOrderType == RECEIVE);
  REQUIRE(immediateSecondReport.retractionSupplied);
  REQUIRE(immediateSecondReport.retractionValid);
  REQUIRE_FALSE(immediateSecondReport.sentRegionsSupplied);
  drainCallbacks(*publisher);
  drainCallbacks(*receiver);
  REQUIRE(receiverReports.timeAdvanceGrantReports.size() == 2U);
  REQUIRE(receiverReports.attributeReflectionReports.size() == 1U);
  auto const& suppressedRegionReport = receiverReports.attributeReflectionReports.front();
  REQUIRE(suppressedRegionReport.objectInstance == objectInstance);
  REQUIRE(suppressedRegionReport.producingFederate == publisherHandle);
  REQUIRE(suppressedRegionReport.timeValue == L"7");
  REQUIRE(suppressedRegionReport.sentOrderType == TIMESTAMP);
  REQUIRE(suppressedRegionReport.receivedOrderType == TIMESTAMP);
  REQUIRE(suppressedRegionReport.retractionSupplied);
  REQUIRE(suppressedRegionReport.retractionValid);
  REQUIRE_FALSE(suppressedRegionReport.sentRegionsSupplied);
  REQUIRE_THROWS_AS(
      publisher->retract(secondHandle),
      rti1516_2025::MessageCanNoLongerBeRetracted);

  REQUIRE_NOTHROW(receiver->setConveyRegionDesignatorSetsSwitch(true));
  auto const thirdHandle = publisher->updateAttributeValues(
      objectInstance,
      attributeValues,
      tag,
      rti1516_2025::HLAinteger64Time(8));
  REQUIRE(thirdHandle.isValid());
  REQUIRE_NOTHROW(publisher->timeAdvanceRequest(rti1516_2025::HLAinteger64Time(3)));
  REQUIRE_NOTHROW(receiver->timeAdvanceRequest(rti1516_2025::HLAinteger64Time(8)));
  REQUIRE(immediateReports.attributeReflectionReports.size() == 3U);
  auto const& immediateThirdReport = immediateReports.attributeReflectionReports.back();
  REQUIRE(immediateThirdReport.timeValue == L"8");
  REQUIRE(immediateThirdReport.sentOrderType == TIMESTAMP);
  REQUIRE(immediateThirdReport.receivedOrderType == RECEIVE);
  REQUIRE(immediateThirdReport.retractionSupplied);
  REQUIRE(immediateThirdReport.retractionValid);
  REQUIRE_FALSE(immediateThirdReport.sentRegionsSupplied);
  drainCallbacks(*publisher);
  drainCallbacks(*receiver);
  REQUIRE(receiverReports.timeAdvanceGrantReports.size() == 3U);
  REQUIRE(receiverReports.attributeReflectionReports.size() == 2U);
  auto const& conveyedRegionReport = receiverReports.attributeReflectionReports.back();
  REQUIRE(conveyedRegionReport.objectInstance == objectInstance);
  REQUIRE(conveyedRegionReport.producingFederate == publisherHandle);
  REQUIRE(conveyedRegionReport.timeValue == L"8");
  REQUIRE(conveyedRegionReport.sentOrderType == TIMESTAMP);
  REQUIRE(conveyedRegionReport.receivedOrderType == TIMESTAMP);
  REQUIRE(conveyedRegionReport.retractionSupplied);
  REQUIRE(conveyedRegionReport.retractionValid);
  REQUIRE(conveyedRegionReport.sentRegionsSupplied);
  REQUIRE(conveyedRegionReport.sentRegions.contains(publisherRegion));
  REQUIRE_THROWS_AS(
      publisher->retract(thirdHandle),
      rti1516_2025::MessageCanNoLongerBeRetracted);

  REQUIRE_NOTHROW(receiver->unsubscribeObjectClassAttributesWithRegions(soda, receiverPair));
  REQUIRE_NOTHROW(immediate->unsubscribeObjectClassAttributesWithRegions(soda, immediatePair));
  REQUIRE_NOTHROW(publisher->unassociateRegionsForUpdates(objectInstance, publisherPair));
  REQUIRE_NOTHROW(receiver->deleteRegion(receiverRegion));
  REQUIRE_NOTHROW(immediate->deleteRegion(immediateRegion));
  REQUIRE_NOTHROW(publisher->deleteRegion(publisherRegion));
  REQUIRE_NOTHROW(receiver->resignFederationExecution(rti1516_2025::NO_ACTION));
  REQUIRE_NOTHROW(immediate->resignFederationExecution(rti1516_2025::NO_ACTION));
  REQUIRE_NOTHROW(publisher->resignFederationExecution(
      rti1516_2025::CANCEL_THEN_DELETE_THEN_DIVEST));
  REQUIRE_NOTHROW(publisher->destroyFederationExecution(federationName));
  REQUIRE_NOTHROW(receiver->disconnect());
  REQUIRE_NOTHROW(immediate->disconnect());
  REQUIRE_NOTHROW(publisher->disconnect());
}
