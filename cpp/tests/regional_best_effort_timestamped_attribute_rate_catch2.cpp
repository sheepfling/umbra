#include <catch2/catch_test_macros.hpp>

#include "internal/fom/hla_names.hpp"
#include "hla_test_names.hpp"

#include <RTI/NullFederateAmbassador.h>
#include <RTI/RTI1516.h>
#include <RTI/time/HLAinteger64Interval.h>
#include <RTI/time/HLAinteger64Time.h>

#include <atomic>
#include <filesystem>
#include <fstream>
#include <memory>
#include <string>
#include <vector>

#ifndef UMBRA_SOURCE_DIRECTORY
#error "The regional best-effort timestamped-rate test requires the Umbra source directory."
#endif

namespace {

namespace standard_hla = umbra::detail::hla::wide;
namespace fixture_hla = umbra::test::hla::wide;

using rti1516_2025::AttributeHandle;
using rti1516_2025::AttributeHandleSet;
using rti1516_2025::AttributeHandleSetRegionHandleSetPairVector;
using rti1516_2025::AttributeHandleValueMap;
using rti1516_2025::DimensionHandle;
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

std::wstring nextFederationName() {
  static std::atomic_uint64_t sequence{0U};
  return L"regional-best-effort-timestamped-attribute-rate-" +
      std::to_wstring(sequence.fetch_add(1U, std::memory_order_relaxed));
}

std::filesystem::path testDataPath(std::filesystem::path const& relativePath) {
  return std::filesystem::path(UMBRA_SOURCE_DIRECTORY) / "cpp" / "tests" /
      "data" / relativePath;
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

ScopedTemporaryFile lowRateRegionalAttributeUpdateModule() {
  auto const source = testDataPath("two-dimensional-regional-interaction-fom.xml");
  std::ifstream input(source, std::ios::binary);
  REQUIRE(input.good());
  std::string fomText{
      std::istreambuf_iterator<char>(input),
      std::istreambuf_iterator<char>()};

  auto const reliableMarker = std::string{
      "                    <transportation>HLAreliable</transportation>"};
  auto const transportationPosition = fomText.find(reliableMarker);
  REQUIRE(transportationPosition != std::string::npos);
  fomText.replace(
      transportationPosition,
      reliableMarker.size(),
      "                    <transportation>HLAbestEffort</transportation>");

  auto const dimensionsMarker = std::string{"</dimensions>"};
  auto const insertion = fomText.rfind(dimensionsMarker);
  REQUIRE(insertion != std::string::npos);
  fomText.insert(
      insertion + dimensionsMarker.size(),
      "\n    <updateRates>\n"
      "        <updateRate>\n"
      "            <name>Low</name>\n"
      "            <rate>0.2</rate>\n"
      "        </updateRate>\n"
      "        <updateRate>\n"
      "            <name>High</name>\n"
      "            <rate>30</rate>\n"
      "        </updateRate>\n"
      "    </updateRates>");

  static std::atomic_uint64_t sequence{0U};
  auto const path = std::filesystem::temp_directory_path() /
      ("umbra-update-rate-regional-attribute-" +
       std::to_string(sequence.fetch_add(1U, std::memory_order_relaxed)) +
       ".xml");
  std::ofstream output(path, std::ios::binary | std::ios::trunc);
  REQUIRE(output.good());
  output.write(fomText.data(), static_cast<std::streamsize>(fomText.size()));
  REQUIRE(output.good());
  return ScopedTemporaryFile(path);
}

class ReportingFederateAmbassador final
    : public rti1516_2025::NullFederateAmbassador {
 public:
  struct ReflectionReport final {
    ObjectInstanceHandle objectInstance;
    AttributeHandleValueMap attributeValues;
    FederateHandle producingFederate;
    std::wstring timeImplementationName;
    std::wstring timeValue;
    OrderType sentOrderType = RECEIVE;
    OrderType receivedOrderType = RECEIVE;
  };

  struct TimeAdvanceGrantReport final {
    std::wstring value;
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
      VariableLengthData const&,
      TransportationTypeHandle const&,
      FederateHandle const& producingFederate,
      RegionHandleSet const*,
      rti1516_2025::LogicalTime const& time,
      OrderType sentOrderType,
      OrderType receivedOrderType,
      rti1516_2025::MessageRetractionHandle const*) override {
    attributeReflectionReports.push_back({
        objectInstance,
        attributeValues,
        producingFederate,
        time.implementationName(),
        time.toString(),
        sentOrderType,
        receivedOrderType,
    });
    callbackOrder.push_back("reflect");
  }

  void timeAdvanceGrant(rti1516_2025::LogicalTime const& time) override {
    timeAdvanceGrantReports.push_back({time.toString()});
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
    "Embedded regional best-effort timestamped attribute updates honor the subscribed rate",
    "[integration][development-profile][object-management][ddm][time-management]"
    "[regional-attribute-update][timestamped-attribute-update][update-rate-reduction]"
    "[rti.service.register-object-instance-with-regions]"
    "[rti.service.subscribe-object-class-attributes-with-regions]"
    "[rti.service.update-attribute-values][rti.service.time-advance-request]"
    "[federate.callback.reflect-attribute-values][federate.callback.time-advance-grant]") {
  ReportingFederateAmbassador publisherReports;
  ReportingFederateAmbassador receiverReports;
  auto publisher = makeRti();
  auto receiver = makeRti();
  auto const federationName = nextFederationName();
  auto const fomModule = lowRateRegionalAttributeUpdateModule();

  REQUIRE_NOTHROW(publisher->connect(publisherReports, HLA_EVOKED));
  REQUIRE_NOTHROW(receiver->connect(receiverReports, HLA_EVOKED));
  REQUIRE_NOTHROW(publisher->createFederationExecution(
      federationName,
      fomModule.path().wstring(),
      standard_hla::mom::integer64_time));
  FederateHandle publisherHandle;
  REQUIRE_NOTHROW(publisherHandle = publisher->joinFederationExecution(
      L"regional-rate-tso-publisher", L"publisher", federationName));
  REQUIRE_NOTHROW(receiver->joinFederationExecution(
      L"regional-rate-tso-subscriber", L"subscriber", federationName));

  auto const publisherClass = publisher->getObjectClassHandle(
      fixture_hla::fom::two_dimensional_regional_object);
  auto const receiverClass = receiver->getObjectClassHandle(
      fixture_hla::fom::two_dimensional_regional_object);
  auto const publisherValue = publisher->getAttributeHandle(
      publisherClass,
      fixture_hla::fixture::value);
  auto const receiverValue = receiver->getAttributeHandle(
      receiverClass,
      fixture_hla::fixture::value);
  auto const publisherX = publisher->getDimensionHandle(
      fixture_hla::fixture::umbra_region_x);
  auto const publisherY = publisher->getDimensionHandle(
      fixture_hla::fixture::umbra_region_y);
  auto const receiverX = receiver->getDimensionHandle(
      fixture_hla::fixture::umbra_region_x);
  auto const receiverY = receiver->getDimensionHandle(
      fixture_hla::fixture::umbra_region_y);
  REQUIRE(publisherClass.isValid());
  REQUIRE(receiverClass.isValid());
  REQUIRE(publisherValue.isValid());
  REQUIRE(receiverValue.isValid());
  REQUIRE(publisherX.isValid());
  REQUIRE(publisherY.isValid());
  REQUIRE(receiverX.isValid());
  REQUIRE(receiverY.isValid());

  AttributeHandleSet const publisherAttributes{publisherValue};
  REQUIRE_NOTHROW(publisher->publishObjectClassAttributes(
      publisherClass,
      publisherAttributes));
  REQUIRE_NOTHROW(publisher->changeDefaultAttributeOrderType(
      publisherClass,
      publisherAttributes,
      TIMESTAMP));

  auto const sourceRegion = publisher->createRegion(
      rti1516_2025::DimensionHandleSet{publisherX, publisherY});
  auto const subscriptionRegion = receiver->createRegion(
      rti1516_2025::DimensionHandleSet{receiverX, receiverY});
  auto setBounds = [](RTIambassador& ambassador,
                      RegionHandle const& region,
                      DimensionHandle const& x,
                      DimensionHandle const& y) {
    REQUIRE_NOTHROW(ambassador.setRangeBounds(region, x, RangeBounds(0UL, 5UL)));
    REQUIRE_NOTHROW(ambassador.setRangeBounds(region, y, RangeBounds(0UL, 5UL)));
    REQUIRE_NOTHROW(ambassador.commitRegionModifications(RegionHandleSet{region}));
  };
  setBounds(*publisher, sourceRegion, publisherX, publisherY);
  setBounds(*receiver, subscriptionRegion, receiverX, receiverY);

  AttributeHandleSetRegionHandleSetPairVector const sourcePair{{
      publisherAttributes,
      RegionHandleSet{sourceRegion},
  }};
  AttributeHandleSetRegionHandleSetPairVector const subscriptionPair{{
      AttributeHandleSet{receiverValue},
      RegionHandleSet{subscriptionRegion},
  }};
  REQUIRE_NOTHROW(receiver->subscribeObjectClassAttributesWithRegions(
      receiverClass,
      subscriptionPair,
      true,
      L"Low"));

  ObjectInstanceHandle objectInstance;
  REQUIRE_NOTHROW(objectInstance = publisher->registerObjectInstanceWithRegions(
      publisherClass,
      sourcePair));
  REQUIRE(objectInstance.isValid());
  drainCallbacks(*receiver);
  REQUIRE(receiverReports.discoveredObjectInstances.size() == 1U);

  REQUIRE_NOTHROW(receiver->enableTimeConstrained());
  drainCallbacks(*receiver);
  REQUIRE_NOTHROW(publisher->enableTimeRegulation(
      rti1516_2025::HLAinteger64Interval(1)));
  drainCallbacks(*publisher);

  auto sendAt = [&](unsigned char value, std::int64_t timestamp) {
    AttributeHandleValueMap values;
    values.emplace(publisherValue, VariableLengthData(&value, sizeof(value)));
    auto const retraction = publisher->updateAttributeValues(
        objectInstance,
        values,
        VariableLengthData{},
        rti1516_2025::HLAinteger64Time(timestamp));
    REQUIRE(retraction.isValid());
    REQUIRE_NOTHROW(receiver->timeAdvanceRequest(
        rti1516_2025::HLAinteger64Time(timestamp)));
    REQUIRE_NOTHROW(publisher->timeAdvanceRequest(
        rti1516_2025::HLAinteger64Time(timestamp - 1)));
    drainCallbacks(*publisher);
    drainCallbacks(*receiver);
  };

  sendAt(0x11, 2);
  REQUIRE(receiverReports.attributeReflectionReports.size() == 1U);
  auto const& firstReflection = receiverReports.attributeReflectionReports.front();
  REQUIRE(firstReflection.objectInstance == objectInstance);
  REQUIRE(firstReflection.producingFederate == publisherHandle);
  REQUIRE(firstReflection.timeValue == L"2");
  REQUIRE(firstReflection.sentOrderType == TIMESTAMP);
  REQUIRE(firstReflection.receivedOrderType == TIMESTAMP);
  REQUIRE(firstReflection.attributeValues.contains(receiverValue));
  REQUIRE(receiverReports.timeAdvanceGrantReports.size() == 1U);

  // The second passel reaches the same regional TSO frontier, but the FDD
  // Low (0.2 Hz) subscription gate suppresses its reflection while still
  // consuming the recipient's grant boundary.
  sendAt(0x22, 3);
  REQUIRE(receiverReports.attributeReflectionReports.size() == 1U);
  REQUIRE(receiverReports.timeAdvanceGrantReports.size() == 2U);

  REQUIRE_NOTHROW(receiver->unsubscribeObjectClassAttributesWithRegions(
      receiverClass,
      subscriptionPair));
  REQUIRE_NOTHROW(publisher->unassociateRegionsForUpdates(
      objectInstance,
      sourcePair));
  REQUIRE_NOTHROW(publisher->deleteRegion(sourceRegion));
  REQUIRE_NOTHROW(receiver->deleteRegion(subscriptionRegion));
  REQUIRE_NOTHROW(receiver->resignFederationExecution(
      rti1516_2025::NO_ACTION));
  REQUIRE_NOTHROW(publisher->resignFederationExecution(
      rti1516_2025::CANCEL_THEN_DELETE_THEN_DIVEST));
  REQUIRE_NOTHROW(publisher->destroyFederationExecution(federationName));
  REQUIRE_NOTHROW(receiver->disconnect());
  REQUIRE_NOTHROW(publisher->disconnect());
}
