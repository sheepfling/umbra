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
#error "The mixed-fanout default-region test requires the Umbra source directory."
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
  return L"timestamped-default-region-attribute-mixed-fanout-" +
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

}  // namespace

TEST_CASE(
    "Embedded timestamped default-region Update Attribute Values splits immediate delivery and pending retraction",
    "[integration][development-profile][object-management][ddm][time-management]"
    "[default-region][tso][mixed-fanout][multi-federate-callback-ordering][retract]"
    "[rti.service.update-attribute-values][rti.service.retract]"
    "[rti.service.subscribe-object-class-attributes-with-regions]"
    "[rti.service.time-advance-request][federate.callback.reflect-attribute-values]"
    "[federate.callback.time-advance-grant][federate.callback.request-retraction]") {
  ReportingFederateAmbassador publisherReports;
  ReportingFederateAmbassador constrainedReports;
  ReportingFederateAmbassador immediateReports;
  auto publisher = makeRti();
  auto constrained = makeRti();
  auto immediate = makeRti();
  auto const federationName = nextFederationName();
  auto const fomModule = resourcePath("examples/RestaurantFOMmodule-2025.xml").wstring();
  unsigned char const valueBytes[] = {0x44, 0x45, 0x46, 0x41};
  unsigned char const tagBytes[] = {0x44, 0x45, 0x46, 0x2D, 0x54, 0x53, 0x4F};
  VariableLengthData const tag(tagBytes, sizeof(tagBytes));

  REQUIRE_NOTHROW(publisher->connect(publisherReports, HLA_EVOKED));
  REQUIRE_NOTHROW(constrained->connect(constrainedReports, HLA_EVOKED));
  REQUIRE_NOTHROW(immediate->connect(immediateReports, HLA_EVOKED));
  REQUIRE_NOTHROW(publisher->createFederationExecution(
      federationName,
      fomModule,
      standard_hla::mom::integer64_time));
  FederateHandle publisherHandle;
  REQUIRE_NOTHROW(publisherHandle = publisher->joinFederationExecution(
      L"timestamped-default-region-mixed-publisher",
      L"publisher",
      federationName));
  REQUIRE_NOTHROW(constrained->joinFederationExecution(
      L"timestamped-default-region-mixed-constrained",
      L"subscriber",
      federationName));
  REQUIRE_NOTHROW(immediate->joinFederationExecution(
      L"timestamped-default-region-mixed-immediate",
      L"subscriber",
      federationName));

  auto const soda = publisher->getObjectClassHandle(
      fixture_hla::fom::food_drink_soda);
  auto const flavor = publisher->getAttributeHandle(
      soda,
      fixture_hla::fixture::flavor);
  auto const sodaFlavor = publisher->getDimensionHandle(
      fixture_hla::fixture::soda_flavor);
  REQUIRE(soda.isValid());
  REQUIRE(flavor.isValid());
  REQUIRE(sodaFlavor.isValid());
  AttributeHandleSet const flavorOnly{flavor};
  REQUIRE_NOTHROW(publisher->publishObjectClassAttributes(soda, flavorOnly));
  REQUIRE_NOTHROW(publisher->changeDefaultAttributeOrderType(
      soda,
      flavorOnly,
      TIMESTAMP));

  auto const constrainedRegion = constrained->createRegion(
      rti1516_2025::DimensionHandleSet{sodaFlavor});
  auto const immediateRegion = immediate->createRegion(
      rti1516_2025::DimensionHandleSet{sodaFlavor});
  REQUIRE_NOTHROW(constrained->setRangeBounds(
      constrainedRegion,
      sodaFlavor,
      RangeBounds(2UL, 3UL)));
  REQUIRE_NOTHROW(immediate->setRangeBounds(
      immediateRegion,
      sodaFlavor,
      RangeBounds(2UL, 3UL)));
  REQUIRE_NOTHROW(constrained->commitRegionModifications(
      RegionHandleSet{constrainedRegion}));
  REQUIRE_NOTHROW(immediate->commitRegionModifications(
      RegionHandleSet{immediateRegion}));
  AttributeHandleSetRegionHandleSetPairVector const constrainedPair{{
      flavorOnly,
      RegionHandleSet{constrainedRegion},
  }};
  AttributeHandleSetRegionHandleSetPairVector const immediatePair{{
      flavorOnly,
      RegionHandleSet{immediateRegion},
  }};
  REQUIRE_NOTHROW(constrained->subscribeObjectClassAttributesWithRegions(
      soda,
      constrainedPair));
  REQUIRE_NOTHROW(immediate->subscribeObjectClassAttributesWithRegions(
      soda,
      immediatePair));
  REQUIRE_NOTHROW(constrained->setConveyRegionDesignatorSetsSwitch(true));
  REQUIRE_NOTHROW(immediate->setConveyRegionDesignatorSetsSwitch(true));

  // Ordinary registration has no public source RegionHandle.  Its private
  // default realization nevertheless makes the object known to both committed
  // regional subscribers before the timestamped update is accepted.
  ObjectInstanceHandle objectInstance;
  REQUIRE_NOTHROW(objectInstance = publisher->registerObjectInstance(soda));
  while (constrained->evokeCallback(0.0)) {
  }
  while (immediate->evokeCallback(0.0)) {
  }
  REQUIRE(constrainedReports.discoveredObjectInstances.size() == 1U);
  REQUIRE(immediateReports.discoveredObjectInstances.size() == 1U);

  REQUIRE_NOTHROW(constrained->enableTimeConstrained());
  while (constrained->evokeCallback(0.0)) {
  }
  REQUIRE_NOTHROW(publisher->enableTimeRegulation(
      rti1516_2025::HLAinteger64Interval(5)));
  while (publisher->evokeCallback(0.0)) {
  }

  AttributeHandleValueMap values;
  values.emplace(flavor, VariableLengthData(valueBytes, sizeof(valueBytes)));
  constrainedReports.callbackOrder.clear();
  immediateReports.callbackOrder.clear();
  auto const retraction = publisher->updateAttributeValues(
      objectInstance,
      values,
      tag,
      rti1516_2025::HLAinteger64Time(6));
  REQUIRE(retraction.isValid());
  REQUIRE(constrainedReports.attributeReflectionReports.empty());

  // The non-time-constrained recipient has no TSO queue entry, but it still
  // receives the timestamped reflection at its callback boundary.  The private
  // default source region is conveyed as a supplied-empty set.
  while (immediate->evokeCallback(0.0)) {
  }
  REQUIRE(immediateReports.attributeReflectionReports.size() == 1U);
  auto const& immediateReport = immediateReports.attributeReflectionReports.front();
  REQUIRE(immediateReport.objectInstance == objectInstance);
  REQUIRE(immediateReport.attributeValues.size() == 1U);
  REQUIRE(immediateReport.attributeValues.contains(flavor));
  REQUIRE(variableLengthDataBytes(immediateReport.attributeValues.at(flavor)) ==
          std::vector<unsigned char>(valueBytes, valueBytes + sizeof(valueBytes)));
  REQUIRE(immediateReport.producingFederate == publisherHandle);
  REQUIRE(immediateReport.timeImplementationName == standard_hla::mom::integer64_time);
  REQUIRE(immediateReport.timeValue == L"6");
  REQUIRE(immediateReport.sentOrderType == TIMESTAMP);
  REQUIRE(immediateReport.receivedOrderType == RECEIVE);
  REQUIRE(immediateReport.retractionSupplied);
  REQUIRE(immediateReport.retractionValid);
  REQUIRE(immediateReport.sentRegionsSupplied);
  REQUIRE(immediateReport.sentRegions.empty());
  REQUIRE(variableLengthDataBytes(immediateReport.userSuppliedTag) ==
          std::vector<unsigned char>(tagBytes, tagBytes + sizeof(tagBytes)));

  // Retraction before the constrained recipient's matching grant withdraws its
  // queued passel. Request Retraction still goes to the recipient that already
  // received the immediate default-region reflection.
  REQUIRE_NOTHROW(constrained->timeAdvanceRequest(
      rti1516_2025::HLAinteger64Time(6)));
  REQUIRE_NOTHROW(publisher->retract(retraction));
  REQUIRE(constrainedReports.callbackOrder.empty());
  while (immediate->evokeCallback(0.0)) {
  }
  REQUIRE(immediateReports.requestRetractionReports.size() == 1U);
  REQUIRE(immediateReports.requestRetractionReports.front().retractionValid);
  REQUIRE(variableLengthDataBytes(
              immediateReports.requestRetractionReports.front().encodedRetraction) ==
          variableLengthDataBytes(retraction.encode()));
  REQUIRE(immediateReports.callbackOrder ==
          std::vector<std::string>{"reflect", "request-retraction"});

  REQUIRE_NOTHROW(publisher->timeAdvanceRequest(
      rti1516_2025::HLAinteger64Time(2)));
  while (publisher->evokeCallback(0.0)) {
  }
  while (constrained->evokeCallback(0.0)) {
  }
  REQUIRE(constrainedReports.timeAdvanceGrantReports.size() == 1U);
  REQUIRE(constrainedReports.timeAdvanceGrantReports.front().timeImplementationName ==
          standard_hla::mom::integer64_time);
  REQUIRE(constrainedReports.timeAdvanceGrantReports.front().value == L"6");
  REQUIRE(constrainedReports.attributeReflectionReports.empty());
  REQUIRE(constrainedReports.requestRetractionReports.empty());
  REQUIRE(constrainedReports.callbackOrder == std::vector<std::string>{"grant"});

  REQUIRE_NOTHROW(constrained->unsubscribeObjectClassAttributesWithRegions(
      soda,
      constrainedPair));
  REQUIRE_NOTHROW(immediate->unsubscribeObjectClassAttributesWithRegions(
      soda,
      immediatePair));
  REQUIRE_NOTHROW(constrained->deleteRegion(constrainedRegion));
  REQUIRE_NOTHROW(immediate->deleteRegion(immediateRegion));
  REQUIRE_NOTHROW(constrained->resignFederationExecution(
      rti1516_2025::NO_ACTION));
  REQUIRE_NOTHROW(immediate->resignFederationExecution(
      rti1516_2025::NO_ACTION));
  REQUIRE_NOTHROW(publisher->resignFederationExecution(
      rti1516_2025::CANCEL_THEN_DELETE_THEN_DIVEST));
  REQUIRE_NOTHROW(publisher->destroyFederationExecution(federationName));
  REQUIRE_NOTHROW(constrained->disconnect());
  REQUIRE_NOTHROW(immediate->disconnect());
  REQUIRE_NOTHROW(publisher->disconnect());
}
