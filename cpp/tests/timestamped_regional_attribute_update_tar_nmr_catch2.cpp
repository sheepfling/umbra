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
#error "The regional timestamped attribute-update TAR/NMR test requires the Umbra source directory."
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
  return L"timestamped-regional-attribute-update-tar-nmr-" +
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

  void timeAdvanceGrant(rti1516_2025::LogicalTime const& time) override {
    timeAdvanceGrantReports.push_back({time.implementationName(), time.toString()});
    callbackOrder.push_back("grant");
  }

  std::vector<ObjectInstanceHandle> discoveredObjectInstances;
  std::vector<ReflectionReport> attributeReflectionReports;
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
    "Embedded regional timestamped attribute updates deliver before TAR and NMR grants",
    "[integration][development-profile][object-management][ddm][time-management]"
    "[timestamped-regional-attribute-update][timestamped-regional-attribute-update-tar-nmr][time-axis-independence][tso][time-advance-request][next-message-request]"
    "[rti.service.update-attribute-values][rti.service.time-advance-request]"
    "[rti.service.next-message-request]"
    "[rti.service.register-object-instance-with-regions]"
    "[rti.service.associate-regions-for-updates]"
    "[rti.service.subscribe-object-class-attributes-with-regions]"
    "[federate.callback.reflect-attribute-values][federate.callback.time-advance-grant]") {
  ReportingFederateAmbassador publisherReports;
  ReportingFederateAmbassador tarReports;
  ReportingFederateAmbassador nmrReports;
  auto publisher = makeRti();
  auto tar = makeRti();
  auto nmr = makeRti();
  auto const federationName = nextFederationName();
  auto const fomModule = resourcePath("examples/RestaurantFOMmodule-2025.xml").wstring();
  unsigned char const valueBytes[] = {0x52, 0x45, 0x47, 0x55};
  unsigned char const tagBytes[] = {0x52, 0x45, 0x47, 0x2D, 0x55};
  VariableLengthData const tag(tagBytes, sizeof(tagBytes));

  REQUIRE_NOTHROW(publisher->connect(publisherReports, HLA_EVOKED));
  REQUIRE_NOTHROW(tar->connect(tarReports, HLA_EVOKED));
  REQUIRE_NOTHROW(nmr->connect(nmrReports, HLA_EVOKED));
  REQUIRE_NOTHROW(
      publisher->createFederationExecution(federationName, fomModule, standard_hla::mom::integer64_time));
  FederateHandle publisherHandle;
  REQUIRE_NOTHROW(publisherHandle = publisher->joinFederationExecution(
      L"regional-attribute-tar-nmr-publisher", L"publisher", federationName));
  REQUIRE_NOTHROW(tar->joinFederationExecution(
      L"regional-attribute-tar-receiver", L"subscriber", federationName));
  REQUIRE_NOTHROW(nmr->joinFederationExecution(
      L"regional-attribute-nmr-receiver", L"subscriber", federationName));

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
  auto const [tarRegion, tarPair] = prepareReceiver(tar);
  auto const [nmrRegion, nmrPair] = prepareReceiver(nmr);

  AttributeHandleSetRegionHandleSetPairVector const publisherPair{{
      flavorOnly,
      RegionHandleSet{publisherRegion},
  }};
  ObjectInstanceHandle objectInstance;
  REQUIRE_NOTHROW(objectInstance = publisher->registerObjectInstanceWithRegions(
      soda,
      publisherPair));
  drainCallbacks(*tar);
  drainCallbacks(*nmr);
  REQUIRE(tarReports.discoveredObjectInstances.size() == 1U);
  REQUIRE(nmrReports.discoveredObjectInstances.size() == 1U);

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
  REQUIRE(tarReports.attributeReflectionReports.empty());
  REQUIRE(nmrReports.attributeReflectionReports.empty());

  // Ordinary TAR and NMR both admit the inclusive timestamp-7 frontier for
  // the same committed regional attribute-update payload.
  REQUIRE_NOTHROW(tar->timeAdvanceRequest(rti1516_2025::HLAinteger64Time(7)));
  REQUIRE_NOTHROW(nmr->nextMessageRequest(rti1516_2025::HLAinteger64Time(10)));
  REQUIRE_NOTHROW(publisher->timeAdvanceRequest(rti1516_2025::HLAinteger64Time(2)));
  drainCallbacks(*publisher);
  drainCallbacks(*tar);
  drainCallbacks(*nmr);

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

  REQUIRE(publisherReports.timeAdvanceGrantReports.size() == 1U);
  REQUIRE(publisherReports.timeAdvanceGrantReports.front().value == L"2");
  REQUIRE(tarReports.attributeReflectionReports.size() == 1U);
  REQUIRE(tarReports.timeAdvanceGrantReports.size() == 1U);
  REQUIRE(tarReports.timeAdvanceGrantReports.front().value == L"7");
  REQUIRE(tarReports.callbackOrder == std::vector<std::string>{"reflect", "grant"});
  REQUIRE(nmrReports.attributeReflectionReports.size() == 1U);
  REQUIRE(nmrReports.timeAdvanceGrantReports.size() == 1U);
  REQUIRE(nmrReports.timeAdvanceGrantReports.front().value == L"7");
  REQUIRE(nmrReports.callbackOrder == std::vector<std::string>{"reflect", "grant"});
  requireRegionalReflection(tarReports.attributeReflectionReports.front());
  requireRegionalReflection(nmrReports.attributeReflectionReports.front());

  REQUIRE_NOTHROW(tar->unsubscribeObjectClassAttributesWithRegions(soda, tarPair));
  REQUIRE_NOTHROW(nmr->unsubscribeObjectClassAttributesWithRegions(soda, nmrPair));
  REQUIRE_NOTHROW(publisher->unassociateRegionsForUpdates(objectInstance, publisherPair));
  REQUIRE_NOTHROW(tar->deleteRegion(tarRegion));
  REQUIRE_NOTHROW(nmr->deleteRegion(nmrRegion));
  REQUIRE_NOTHROW(publisher->deleteRegion(publisherRegion));
  REQUIRE_NOTHROW(nmr->resignFederationExecution(rti1516_2025::NO_ACTION));
  REQUIRE_NOTHROW(tar->resignFederationExecution(rti1516_2025::NO_ACTION));
  REQUIRE_NOTHROW(publisher->resignFederationExecution(rti1516_2025::NO_ACTION));
  REQUIRE_NOTHROW(publisher->destroyFederationExecution(federationName));
  REQUIRE_NOTHROW(nmr->disconnect());
  REQUIRE_NOTHROW(tar->disconnect());
  REQUIRE_NOTHROW(publisher->disconnect());
}
