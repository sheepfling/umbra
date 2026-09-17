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
#error "The mixed regional timestamped attribute-update test requires the Umbra source directory."
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
using rti1516_2025::RegionHandle;
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
  return L"timestamped-regional-attribute-update-mixed-advance-" +
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

  struct FlushQueueGrantReport final {
    std::wstring timeImplementationName;
    std::wstring value;
    std::wstring optimisticValue;
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

  void flushQueueGrant(
      rti1516_2025::LogicalTime const& time,
      rti1516_2025::LogicalTime const& optimisticTime) override {
    flushQueueGrantReports.push_back({
        time.implementationName(),
        time.toString(),
        optimisticTime.toString(),
    });
    callbackOrder.push_back("flush-grant");
  }

  void timeAdvanceGrant(rti1516_2025::LogicalTime const& time) override {
    timeAdvanceGrantReports.push_back({time.implementationName(), time.toString()});
    callbackOrder.push_back("grant");
  }

  std::vector<ObjectInstanceHandle> discoveredObjectInstances;
  std::vector<ReflectionReport> attributeReflectionReports;
  std::vector<FlushQueueGrantReport> flushQueueGrantReports;
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
    "Embedded mixed regional timestamped attribute updates deliver before FQR TARA and NMRA grants",
    "[integration][development-profile][object-management][ddm][time-management]"
    "[timestamped-regional-attribute-update]"
    "[timestamped-regional-attribute-update-mixed-advance]"
    "[tso][multi-federate-callback-ordering]"
    "[flush-queue-request]"
    "[time-advance-request-available][next-message-request-available]"
    "[rti.service.update-attribute-values][rti.service.register-object-instance-with-regions]"
    "[rti.service.subscribe-object-class-attributes-with-regions]"
    "[rti.service.flush-queue-request][rti.service.time-advance-request-available]"
    "[rti.service.next-message-request-available][rti.service.time-advance-request]"
    "[federate.callback.reflect-attribute-values][federate.callback.flush-queue-grant]"
    "[federate.callback.time-advance-grant]") {
  ReportingFederateAmbassador publisherReports;
  ReportingFederateAmbassador fqrReports;
  ReportingFederateAmbassador taraReports;
  ReportingFederateAmbassador nmraReports;
  auto publisher = makeRti();
  auto fqr = makeRti();
  auto tara = makeRti();
  auto nmra = makeRti();
  auto const federationName = nextFederationName();
  auto const fomModule = resourcePath("examples/RestaurantFOMmodule-2025.xml").wstring();
  unsigned char const valueBytes[] = {0x4D, 0x49, 0x58, 0x2D, 0x41};
  unsigned char const tagBytes[] = {0x4D, 0x49, 0x58, 0x2D, 0x46};
  VariableLengthData const tag(tagBytes, sizeof(tagBytes));

  REQUIRE_NOTHROW(publisher->connect(publisherReports, HLA_EVOKED));
  REQUIRE_NOTHROW(fqr->connect(fqrReports, HLA_EVOKED));
  REQUIRE_NOTHROW(tara->connect(taraReports, HLA_EVOKED));
  REQUIRE_NOTHROW(nmra->connect(nmraReports, HLA_EVOKED));
  REQUIRE_NOTHROW(
      publisher->createFederationExecution(federationName, fomModule, standard_hla::mom::integer64_time));
  FederateHandle publisherHandle;
  REQUIRE_NOTHROW(publisherHandle = publisher->joinFederationExecution(
      L"mixed-regional-attribute-publisher", L"publisher", federationName));
  REQUIRE_NOTHROW(fqr->joinFederationExecution(
      L"mixed-regional-attribute-fqr", L"subscriber", federationName));
  REQUIRE_NOTHROW(tara->joinFederationExecution(
      L"mixed-regional-attribute-tara", L"subscriber", federationName));
  REQUIRE_NOTHROW(nmra->joinFederationExecution(
      L"mixed-regional-attribute-nmra", L"subscriber", federationName));

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
  REQUIRE_NOTHROW(publisher->setRangeBounds(
      publisherRegion, sodaFlavor, RangeBounds(0UL, 2UL)));
  REQUIRE_NOTHROW(publisher->commitRegionModifications(RegionHandleSet{publisherRegion}));

  auto prepareReceiver = [&](auto& rti) {
    auto const region = rti->createRegion(rti1516_2025::DimensionHandleSet{sodaFlavor});
    REQUIRE_NOTHROW(rti->setRangeBounds(region, sodaFlavor, RangeBounds(1UL, 3UL)));
    REQUIRE_NOTHROW(rti->commitRegionModifications(RegionHandleSet{region}));
    AttributeHandleSetRegionHandleSetPairVector const receiverPair{{
        flavorOnly,
        RegionHandleSet{region},
    }};
    REQUIRE_NOTHROW(rti->subscribeObjectClassAttributesWithRegions(soda, receiverPair));
    REQUIRE_FALSE(rti->getConveyRegionDesignatorSetsSwitch());
    REQUIRE_NOTHROW(rti->setConveyRegionDesignatorSetsSwitch(true));
    REQUIRE_NOTHROW(rti->enableTimeConstrained());
    drainCallbacks(*rti);
    return std::pair{region, receiverPair};
  };
  auto const [fqrRegion, fqrPair] = prepareReceiver(fqr);
  auto const [taraRegion, taraPair] = prepareReceiver(tara);
  auto const [nmraRegion, nmraPair] = prepareReceiver(nmra);

  AttributeHandleSetRegionHandleSetPairVector const publisherPair{{
      flavorOnly,
      RegionHandleSet{publisherRegion},
  }};
  ObjectInstanceHandle objectInstance;
  REQUIRE_NOTHROW(objectInstance = publisher->registerObjectInstanceWithRegions(soda, publisherPair));
  drainCallbacks(*fqr);
  drainCallbacks(*tara);
  drainCallbacks(*nmra);
  REQUIRE(fqrReports.discoveredObjectInstances.size() == 1U);
  REQUIRE(taraReports.discoveredObjectInstances.size() == 1U);
  REQUIRE(nmraReports.discoveredObjectInstances.size() == 1U);

  REQUIRE_NOTHROW(publisher->enableTimeRegulation(
      rti1516_2025::HLAinteger64Interval(5)));
  drainCallbacks(*publisher);

  AttributeHandleValueMap attributeValues;
  attributeValues.emplace(flavor, VariableLengthData(valueBytes, sizeof(valueBytes)));
  auto const retraction = publisher->updateAttributeValues(
      objectInstance,
      attributeValues,
      tag,
      rti1516_2025::HLAinteger64Time(7));
  REQUIRE(retraction.isValid());
  REQUIRE(fqrReports.attributeReflectionReports.empty());
  REQUIRE(taraReports.attributeReflectionReports.empty());
  REQUIRE(nmraReports.attributeReflectionReports.empty());

  // FQR is strict at its requested boundary, TARA is inclusive at the
  // timestamp frontier, and NMRA selects the queued timestamp. The publisher
  // request to 2 plus lookahead 5 establishes GALT 7 for every recipient.
  REQUIRE_NOTHROW(fqr->flushQueueRequest(rti1516_2025::HLAinteger64Time(10)));
  REQUIRE_NOTHROW(tara->timeAdvanceRequestAvailable(
      rti1516_2025::HLAinteger64Time(7)));
  REQUIRE_NOTHROW(nmra->nextMessageRequestAvailable(
      rti1516_2025::HLAinteger64Time(10)));
  REQUIRE_NOTHROW(publisher->timeAdvanceRequest(rti1516_2025::HLAinteger64Time(2)));
  REQUIRE_FALSE(publisher->evokeCallback(0.0));

  auto requireRegionalReflection = [&](auto const& report) {
    REQUIRE(report.objectInstance == objectInstance);
    REQUIRE(report.attributeValues.size() == 1U);
    REQUIRE(report.attributeValues.contains(flavor));
    REQUIRE(variableLengthDataBytes(report.attributeValues.at(flavor)) ==
            std::vector<unsigned char>(valueBytes, valueBytes + sizeof(valueBytes)));
    REQUIRE(variableLengthDataBytes(report.userSuppliedTag) ==
            std::vector<unsigned char>(tagBytes, tagBytes + sizeof(tagBytes)));
    REQUIRE(report.producingFederate == publisherHandle);
    REQUIRE(report.timeImplementationName == standard_hla::mom::integer64_time);
    REQUIRE(report.timeValue == L"7");
    REQUIRE(report.sentOrderType == TIMESTAMP);
    REQUIRE(report.receivedOrderType == TIMESTAMP);
    REQUIRE(report.sentRegionsSupplied);
    REQUIRE(report.sentRegions.contains(publisherRegion));
    REQUIRE(report.retractionSupplied);
    REQUIRE(report.retractionValid);
  };

  REQUIRE_FALSE(fqr->evokeCallback(0.0));
  REQUIRE(publisherReports.timeAdvanceGrantReports.size() == 1U);
  REQUIRE(publisherReports.timeAdvanceGrantReports.front().value == L"2");
  REQUIRE(fqrReports.attributeReflectionReports.size() == 1U);
  REQUIRE(fqrReports.flushQueueGrantReports.size() == 1U);
  REQUIRE(fqrReports.flushQueueGrantReports.front().value == L"7");
  REQUIRE(fqrReports.flushQueueGrantReports.front().optimisticValue == L"7");
  REQUIRE(fqrReports.callbackOrder == std::vector<std::string>{"reflect", "flush-grant"});
  REQUIRE(taraReports.attributeReflectionReports.empty());
  REQUIRE(taraReports.timeAdvanceGrantReports.empty());
  REQUIRE(nmraReports.attributeReflectionReports.empty());
  REQUIRE(nmraReports.timeAdvanceGrantReports.empty());
  REQUIRE_FALSE(tara->evokeCallback(0.0));
  REQUIRE(taraReports.attributeReflectionReports.size() == 1U);
  REQUIRE(taraReports.timeAdvanceGrantReports.size() == 1U);
  REQUIRE(taraReports.timeAdvanceGrantReports.front().value == L"7");
  REQUIRE(taraReports.callbackOrder == std::vector<std::string>{"reflect", "grant"});
  REQUIRE(nmraReports.attributeReflectionReports.empty());
  REQUIRE(nmraReports.timeAdvanceGrantReports.empty());
  REQUIRE_FALSE(nmra->evokeCallback(0.0));
  REQUIRE(nmraReports.attributeReflectionReports.size() == 1U);
  REQUIRE(nmraReports.timeAdvanceGrantReports.size() == 1U);
  REQUIRE(nmraReports.timeAdvanceGrantReports.front().value == L"7");
  REQUIRE(nmraReports.callbackOrder == std::vector<std::string>{"reflect", "grant"});
  requireRegionalReflection(fqrReports.attributeReflectionReports.front());
  requireRegionalReflection(taraReports.attributeReflectionReports.front());
  requireRegionalReflection(nmraReports.attributeReflectionReports.front());

  REQUIRE_NOTHROW(fqr->unsubscribeObjectClassAttributesWithRegions(soda, fqrPair));
  REQUIRE_NOTHROW(tara->unsubscribeObjectClassAttributesWithRegions(soda, taraPair));
  REQUIRE_NOTHROW(nmra->unsubscribeObjectClassAttributesWithRegions(soda, nmraPair));
  REQUIRE_NOTHROW(publisher->unassociateRegionsForUpdates(objectInstance, publisherPair));
  REQUIRE_NOTHROW(publisher->deleteRegion(publisherRegion));
  REQUIRE_NOTHROW(fqr->deleteRegion(fqrRegion));
  REQUIRE_NOTHROW(tara->deleteRegion(taraRegion));
  REQUIRE_NOTHROW(nmra->deleteRegion(nmraRegion));
  REQUIRE_NOTHROW(nmra->resignFederationExecution(rti1516_2025::NO_ACTION));
  REQUIRE_NOTHROW(tara->resignFederationExecution(rti1516_2025::NO_ACTION));
  REQUIRE_NOTHROW(fqr->resignFederationExecution(rti1516_2025::NO_ACTION));
  REQUIRE_NOTHROW(publisher->resignFederationExecution(rti1516_2025::NO_ACTION));
  REQUIRE_NOTHROW(publisher->destroyFederationExecution(federationName));
  REQUIRE_NOTHROW(nmra->disconnect());
  REQUIRE_NOTHROW(tara->disconnect());
  REQUIRE_NOTHROW(fqr->disconnect());
  REQUIRE_NOTHROW(publisher->disconnect());
}
