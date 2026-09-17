#include <catch2/catch_test_macros.hpp>

#include "hla_test_names.hpp"
#include "internal/fom/hla_names.hpp"

#include <RTI/NullFederateAmbassador.h>
#include <RTI/RTI1516.h>
#include <RTI/encoding/BasicDataElements.h>

#include <algorithm>
#include <atomic>
#include <filesystem>
#include <memory>
#include <string>
#include <vector>

#ifndef UMBRA_SOURCE_DIRECTORY
#error "The custom transportation ordinary-regional-attribute test requires the Umbra source directory."
#endif

namespace {

namespace standard_hla = umbra::detail::hla::wide;
namespace fixture_hla = umbra::test::hla::wide;

using rti1516_2025::AttributeHandle;
using rti1516_2025::AttributeHandleSet;
using rti1516_2025::AttributeHandleSetRegionHandleSetPairVector;
using rti1516_2025::AttributeHandleValueMap;
using rti1516_2025::DimensionHandle;
using rti1516_2025::DimensionHandleSet;
using rti1516_2025::FederateHandle;
using rti1516_2025::HLA_EVOKED;
using rti1516_2025::NullFederateAmbassador;
using rti1516_2025::ObjectClassHandle;
using rti1516_2025::ObjectInstanceHandle;
using rti1516_2025::RangeBounds;
using rti1516_2025::RegionHandle;
using rti1516_2025::RegionHandleSet;
using rti1516_2025::RTIambassador;
using rti1516_2025::RTIambassadorFactory;
using rti1516_2025::TransportationTypeHandle;
using rti1516_2025::VariableLengthData;

std::wstring nextFederationName() {
  static std::atomic_uint64_t sequence{0U};
  return L"custom-transportation-ordinary-regional-attribute-" +
      std::to_wstring(sequence.fetch_add(1U, std::memory_order_relaxed));
}

std::filesystem::path resourcePath(std::filesystem::path const& relativePath) {
  return std::filesystem::path(UMBRA_SOURCE_DIRECTORY) / "cpp" / "tests" /
      "data" / relativePath;
}

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

bool hasBytes(VariableLengthData const& value,
              std::vector<unsigned char> const& expected) {
  if (value.size() != expected.size()) {
    return false;
  }
  auto const* bytes = static_cast<unsigned char const*>(value.data());
  return bytes != nullptr && std::equal(expected.begin(), expected.end(), bytes);
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
      RegionHandleSet const* optionalSentRegions) override {
    attributeReflectionReports.push_back({
        objectInstance,
        attributeValues,
        userSuppliedTag,
        transportationType,
        producingFederate,
        optionalSentRegions != nullptr,
        optionalSentRegions == nullptr ? RegionHandleSet{} : *optionalSentRegions});
  }

  std::vector<ObjectInstanceHandle> objectDiscoveryReports;
  std::vector<AttributeReflectionReport> attributeReflectionReports;
};

}  // namespace

TEST_CASE(
    "Embedded ordinary regional attribute delivery accepts a declared custom FOM transportation",
    "[integration][development-profile][federation-management]"
    "[custom-transportation][custom-transportation-ordinary-regional-attribute]"
    "[fom][transportation-type-lookup][transportation][ddm][regional-attribute-update]"
    "[object-management]"
    "[rti.service.get-transportation-type-handle]"
    "[rti.service.get-object-class-handle][rti.service.get-attribute-handle]"
    "[rti.service.publish-object-class-attributes]"
    "[rti.service.get-dimension-handle][rti.service.create-region]"
    "[rti.service.set-range-bounds][rti.service.commit-region-modifications]"
    "[rti.service.register-object-instance-with-regions]"
    "[rti.service.subscribe-object-class-attributes-with-regions]"
    "[rti.service.set-convey-region-designator-sets-switch]"
    "[rti.service.update-attribute-values]"
    "[rti.service.unassociate-regions-for-updates]"
    "[rti.service.unsubscribe-object-class-attributes-with-regions]"
    "[rti.service.evoke-callback]"
    "[federate.callback.discover-object-instance]"
    "[federate.callback.reflect-attribute-values]") {
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
      L"ordinary-regional-attribute-producer",
      L"producer",
      federationName));
  REQUIRE_NOTHROW(consumer->joinFederationExecution(
      L"ordinary-regional-attribute-consumer",
      L"consumer",
      federationName));

  auto const custom = producer->getTransportationTypeHandle(
      fixture_hla::fixture::umbra_transportation_fixture);
  auto const consumerCustom = consumer->getTransportationTypeHandle(
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
  REQUIRE(custom.isValid());
  REQUIRE(consumerCustom == custom);
  REQUIRE(objectClass.isValid());
  REQUIRE(consumerObjectClass.isValid());
  REQUIRE(value.isValid());
  REQUIRE(consumerValue.isValid());

  AttributeHandleSet const producerAttributes{value};
  AttributeHandleSet const consumerAttributes{consumerValue};
  REQUIRE_NOTHROW(producer->publishObjectClassAttributes(
      objectClass,
      producerAttributes));

  DimensionHandle const producerX = producer->getDimensionHandle(
      fixture_hla::fixture::umbra_region_x);
  DimensionHandle const producerY = producer->getDimensionHandle(
      fixture_hla::fixture::umbra_region_y);
  DimensionHandle const consumerX = consumer->getDimensionHandle(
      fixture_hla::fixture::umbra_region_x);
  DimensionHandle const consumerY = consumer->getDimensionHandle(
      fixture_hla::fixture::umbra_region_y);
  REQUIRE(producerX.isValid());
  REQUIRE(producerY.isValid());
  REQUIRE(consumerX.isValid());
  REQUIRE(consumerY.isValid());

  RegionHandle const sourceRegion = producer->createRegion(
      DimensionHandleSet{producerX, producerY});
  RegionHandle const receiverRegion = consumer->createRegion(
      DimensionHandleSet{consumerX, consumerY});
  REQUIRE(sourceRegion.isValid());
  REQUIRE(receiverRegion.isValid());
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

  ObjectInstanceHandle objectInstance;
  REQUIRE_NOTHROW(objectInstance = producer->registerObjectInstanceWithRegions(
      objectClass,
      sourcePair));
  REQUIRE(objectInstance.isValid());
  drainCallbacks(*consumer);
  REQUIRE(consumerReports.objectDiscoveryReports.size() == 1U);
  REQUIRE(consumerReports.objectDiscoveryReports.front() == objectInstance);

  unsigned char const tagBytes[] = {'R', 'A', 'T'};
  VariableLengthData const tag(tagBytes, sizeof(tagBytes));
  rti1516_2025::HLAunicodeString valueToSend{L"ordinary-regional-value"};
  AttributeHandleValueMap values{{value, valueToSend.encode()}};
  REQUIRE_NOTHROW(producer->updateAttributeValues(
      objectInstance,
      values,
      tag));
  drainCallbacks(*consumer);

  REQUIRE(consumerReports.attributeReflectionReports.size() == 1U);
  auto const& report = consumerReports.attributeReflectionReports.front();
  REQUIRE(report.objectInstance == objectInstance);
  REQUIRE(report.attributeValues.size() == 1U);
  REQUIRE(report.attributeValues.contains(consumerValue));
  rti1516_2025::HLAunicodeString decodedValue;
  REQUIRE_NOTHROW(decodedValue.decode(report.attributeValues.at(consumerValue)));
  REQUIRE(decodedValue.get() == L"ordinary-regional-value");
  REQUIRE(report.transportationType == custom);
  REQUIRE(report.producingFederate == producerHandle);
  REQUIRE(report.sentRegionsSupplied);
  REQUIRE(report.sentRegions.contains(sourceRegion));
  REQUIRE(hasBytes(
      report.userSuppliedTag,
      std::vector<unsigned char>(tagBytes, tagBytes + sizeof(tagBytes))));

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
