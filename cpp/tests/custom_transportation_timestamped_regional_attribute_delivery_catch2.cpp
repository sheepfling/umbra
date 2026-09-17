#include <catch2/catch_test_macros.hpp>

#include "internal/fom/hla_names.hpp"
#include "hla_test_names.hpp"

#include <RTI/NullFederateAmbassador.h>
#include <RTI/RTI1516.h>
#include <RTI/encoding/BasicDataElements.h>
#include <RTI/time/HLAinteger64Interval.h>
#include <RTI/time/HLAinteger64Time.h>

#include <atomic>
#include <filesystem>
#include <memory>
#include <string>
#include <vector>

#ifndef UMBRA_SOURCE_DIRECTORY
#error "The custom transportation timestamped-regional-attribute test requires the Umbra source directory."
#endif

namespace {

namespace standard_hla = umbra::detail::hla::wide;
namespace fixture_hla = umbra::test::hla::wide;

using rti1516_2025::AttributeHandle;
using rti1516_2025::AttributeHandleSet;
using rti1516_2025::AttributeHandleSetRegionHandleSetPairVector;
using rti1516_2025::AttributeHandleValueMap;
using rti1516_2025::DimensionHandleSet;
using rti1516_2025::FederateHandle;
using rti1516_2025::HLA_EVOKED;
using rti1516_2025::LogicalTime;
using rti1516_2025::MessageRetractionHandle;
using rti1516_2025::NullFederateAmbassador;
using rti1516_2025::ObjectClassHandle;
using rti1516_2025::ObjectInstanceHandle;
using rti1516_2025::OrderType;
using rti1516_2025::RECEIVE;
using rti1516_2025::RangeBounds;
using rti1516_2025::RegionHandle;
using rti1516_2025::RegionHandleSet;
using rti1516_2025::RTIambassador;
using rti1516_2025::RTIambassadorFactory;
using rti1516_2025::TIMESTAMP;
using rti1516_2025::TransportationTypeHandle;
using rti1516_2025::VariableLengthData;

std::wstring nextFederationName() {
  static std::atomic_uint64_t sequence{0U};
  return L"custom-transportation-timestamped-regional-attribute-delivery-" +
      std::to_wstring(sequence.fetch_add(1U, std::memory_order_relaxed));
}

std::filesystem::path resourcePath(std::filesystem::path const& relativePath) {
  return std::filesystem::path(UMBRA_SOURCE_DIRECTORY) / "cpp" / "tests" /
      "data" / relativePath;
}

std::vector<unsigned char> variableLengthDataBytes(
    VariableLengthData const& value) {
  auto const* bytes = static_cast<unsigned char const*>(value.data());
  if (bytes == nullptr || value.size() == 0U) {
    return {};
  }
  return {bytes, bytes + value.size()};
}

class ReportingFederateAmbassador final : public NullFederateAmbassador {
 public:
  struct AttributeReflectionReport final {
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

  struct TimeAdvanceGrantReport final {
    std::wstring implementationName;
    std::wstring value;
  };

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
      RegionHandleSet const* optionalSentRegions,
      LogicalTime const& time,
      OrderType sentOrderType,
      OrderType receivedOrderType,
      MessageRetractionHandle const* optionalRetraction) override {
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
  }

  void timeConstrainedEnabled(LogicalTime const&) override {
    timeConstrainedEnabledReports.push_back(0);
  }

  void timeRegulationEnabled(LogicalTime const&) override {
    timeRegulationEnabledReports.push_back(0);
  }

  void timeAdvanceGrant(LogicalTime const& time) override {
    timeAdvanceGrantReports.push_back({time.implementationName(), time.toString()});
  }

  std::vector<ObjectInstanceHandle> objectDiscoveryReports;
  std::vector<AttributeReflectionReport> attributeReflectionReports;
  std::vector<int> timeConstrainedEnabledReports;
  std::vector<int> timeRegulationEnabledReports;
  std::vector<TimeAdvanceGrantReport> timeAdvanceGrantReports;
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
    "Embedded timestamped regional attribute delivery accepts a declared custom FOM transportation",
    "[integration][development-profile][federation-management]"
    "[custom-transportation][custom-transportation-timestamped-regional-attribute-delivery]"
    "[fom][transportation-type-lookup][transportation][ddm][time-management][tso]"
    "[timestamped-regional-attribute-update][object-management]"
    "[rti.service.get-transportation-type-handle]"
    "[rti.service.get-object-class-handle][rti.service.get-attribute-handle]"
    "[rti.service.publish-object-class-attributes]"
    "[rti.service.change-default-attribute-order-type]"
    "[rti.service.create-region][rti.service.set-range-bounds]"
    "[rti.service.commit-region-modifications]"
    "[rti.service.register-object-instance-with-regions]"
    "[rti.service.subscribe-object-class-attributes-with-regions]"
    "[rti.service.set-convey-region-designator-sets-switch]"
    "[rti.service.enable-time-constrained][rti.service.enable-time-regulation]"
    "[rti.service.update-attribute-values][rti.service.time-advance-request]"
    "[rti.service.unassociate-regions-for-updates]"
    "[rti.service.unsubscribe-object-class-attributes-with-regions]"
    "[rti.service.evoke-callback]"
    "[federate.callback.discover-object-instance]"
    "[federate.callback.reflect-attribute-values][federate.callback.time-advance-grant]") {
  ReportingFederateAmbassador producerReports;
  ReportingFederateAmbassador consumerReports;
  auto producer = makeRti();
  auto consumer = makeRti();
  auto const federationName = nextFederationName();
  std::vector<std::wstring> const fomModules{
      resourcePath("transportation-regional-reference-consumer-fom.xml").wstring(),
      resourcePath("transportation-reference-provider-fom.xml").wstring(),
  };

  REQUIRE_NOTHROW(producer->connect(producerReports, HLA_EVOKED));
  REQUIRE_NOTHROW(consumer->connect(consumerReports, HLA_EVOKED));
  REQUIRE_NOTHROW(producer->createFederationExecution(
      federationName,
      fomModules,
      standard_hla::mom::integer64_time));
  FederateHandle producerHandle;
  REQUIRE_NOTHROW(producerHandle = producer->joinFederationExecution(
      L"timestamped-regional-attribute-producer",
      L"producer",
      federationName));
  REQUIRE_NOTHROW(consumer->joinFederationExecution(
      L"timestamped-regional-attribute-consumer",
      L"consumer",
      federationName));

  auto const custom = producer->getTransportationTypeHandle(
      fixture_hla::fixture::umbra_transportation_fixture);
  auto const objectClass = producer->getObjectClassHandle(
      fixture_hla::fom::transportation_regional_object);
  auto const consumerObjectClass = consumer->getObjectClassHandle(
      fixture_hla::fom::transportation_regional_object);
  auto const value = producer->getAttributeHandle(
      objectClass,
      fixture_hla::fixture::value);
  auto const consumerValue = consumer->getAttributeHandle(
      consumerObjectClass,
      fixture_hla::fixture::value);
  auto const producerX = producer->getDimensionHandle(
      fixture_hla::fixture::umbra_region_x);
  auto const producerY = producer->getDimensionHandle(
      fixture_hla::fixture::umbra_region_y);
  auto const consumerX = consumer->getDimensionHandle(
      fixture_hla::fixture::umbra_region_x);
  auto const consumerY = consumer->getDimensionHandle(
      fixture_hla::fixture::umbra_region_y);
  REQUIRE(custom.isValid());
  REQUIRE(objectClass.isValid());
  REQUIRE(consumerObjectClass.isValid());
  REQUIRE(value.isValid());
  REQUIRE(consumerValue.isValid());
  REQUIRE(producerX.isValid());
  REQUIRE(producerY.isValid());
  REQUIRE(consumerX.isValid());
  REQUIRE(consumerY.isValid());

  AttributeHandleSet const producerAttributes{value};
  AttributeHandleSet const consumerAttributes{consumerValue};
  REQUIRE_NOTHROW(producer->publishObjectClassAttributes(
      objectClass,
      producerAttributes));
  REQUIRE_NOTHROW(producer->changeDefaultAttributeOrderType(
      objectClass,
      producerAttributes,
      TIMESTAMP));

  auto const sourceRegion = producer->createRegion(
      DimensionHandleSet{producerX, producerY});
  auto const receiverRegion = consumer->createRegion(
      DimensionHandleSet{consumerX, consumerY});
  REQUIRE_NOTHROW(producer->setRangeBounds(
      sourceRegion,
      producerX,
      RangeBounds(0UL, 5UL)));
  REQUIRE_NOTHROW(producer->setRangeBounds(
      sourceRegion,
      producerY,
      RangeBounds(0UL, 5UL)));
  REQUIRE_NOTHROW(consumer->setRangeBounds(
      receiverRegion,
      consumerX,
      RangeBounds(0UL, 5UL)));
  REQUIRE_NOTHROW(consumer->setRangeBounds(
      receiverRegion,
      consumerY,
      RangeBounds(0UL, 5UL)));
  REQUIRE_NOTHROW(producer->commitRegionModifications(
      RegionHandleSet{sourceRegion}));
  REQUIRE_NOTHROW(consumer->commitRegionModifications(
      RegionHandleSet{receiverRegion}));

  AttributeHandleSetRegionHandleSetPairVector const sourcePair{{
      producerAttributes,
      RegionHandleSet{sourceRegion},
  }};
  AttributeHandleSetRegionHandleSetPairVector const receiverPair{{
      consumerAttributes,
      RegionHandleSet{receiverRegion},
  }};
  REQUIRE_NOTHROW(consumer->subscribeObjectClassAttributesWithRegions(
      consumerObjectClass,
      receiverPair));
  REQUIRE_NOTHROW(consumer->setConveyRegionDesignatorSetsSwitch(true));
  REQUIRE_NOTHROW(consumer->enableTimeConstrained());
  drainCallbacks(*consumer);
  REQUIRE(consumerReports.timeConstrainedEnabledReports.size() == 1U);
  REQUIRE_NOTHROW(producer->enableTimeRegulation(
      rti1516_2025::HLAinteger64Interval(1)));
  drainCallbacks(*producer);
  REQUIRE(producerReports.timeRegulationEnabledReports.size() == 1U);

  ObjectInstanceHandle objectInstance;
  REQUIRE_NOTHROW(objectInstance = producer->registerObjectInstanceWithRegions(
      objectClass,
      sourcePair));
  REQUIRE(objectInstance.isValid());
  drainCallbacks(*consumer);
  REQUIRE(consumerReports.objectDiscoveryReports.size() == 1U);
  REQUIRE(consumerReports.objectDiscoveryReports.front() == objectInstance);

  unsigned char const tagBytes[] = {0x54, 0x53, 0x4F, 0x2D, 0x52};
  VariableLengthData const tag(tagBytes, sizeof(tagBytes));
  AttributeHandleValueMap values;
  rti1516_2025::HLAunicodeString encodedValue{L"timestamped-regional-value"};
  values.emplace(value, encodedValue.encode());
  auto const retraction = producer->updateAttributeValues(
      objectInstance,
      values,
      tag,
      rti1516_2025::HLAinteger64Time(2));
  REQUIRE(retraction.isValid());
  REQUIRE(consumerReports.attributeReflectionReports.empty());
  REQUIRE_NOTHROW(consumer->timeAdvanceRequest(
      rti1516_2025::HLAinteger64Time(2)));
  REQUIRE_NOTHROW(producer->timeAdvanceRequest(
      rti1516_2025::HLAinteger64Time(2)));
  drainCallbacks(*producer);
  drainCallbacks(*consumer);

  REQUIRE(consumerReports.attributeReflectionReports.size() == 1U);
  auto const& report = consumerReports.attributeReflectionReports.front();
  REQUIRE(report.objectInstance == objectInstance);
  REQUIRE(report.attributeValues.size() == 1U);
  REQUIRE(report.attributeValues.contains(consumerValue));
  REQUIRE(report.transportationType == custom);
  REQUIRE(report.producingFederate == producerHandle);
  REQUIRE(report.timeImplementationName == standard_hla::mom::integer64_time);
  REQUIRE(report.timeValue == L"2");
  REQUIRE(report.sentOrderType == TIMESTAMP);
  REQUIRE(report.receivedOrderType == TIMESTAMP);
  REQUIRE(report.retractionSupplied);
  REQUIRE(report.retractionValid);
  REQUIRE(report.sentRegionsSupplied);
  REQUIRE(report.sentRegions.contains(sourceRegion));
  REQUIRE(variableLengthDataBytes(report.userSuppliedTag) ==
          std::vector<unsigned char>(tagBytes, tagBytes + sizeof(tagBytes)));

  REQUIRE_NOTHROW(consumer->unsubscribeObjectClassAttributesWithRegions(
      consumerObjectClass,
      receiverPair));
  REQUIRE_NOTHROW(producer->unassociateRegionsForUpdates(
      objectInstance,
      sourcePair));
  REQUIRE_NOTHROW(consumer->deleteRegion(receiverRegion));
  REQUIRE_NOTHROW(producer->deleteRegion(sourceRegion));
  REQUIRE_NOTHROW(consumer->resignFederationExecution(rti1516_2025::NO_ACTION));
  REQUIRE_NOTHROW(producer->resignFederationExecution(
      rti1516_2025::CANCEL_THEN_DELETE_THEN_DIVEST));
  REQUIRE_NOTHROW(producer->destroyFederationExecution(federationName));
  REQUIRE_NOTHROW(consumer->disconnect());
  REQUIRE_NOTHROW(producer->disconnect());
}
