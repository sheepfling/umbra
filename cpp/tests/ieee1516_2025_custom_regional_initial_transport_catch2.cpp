#include "ieee1516_2025_federation_management_test_support.hpp"

TEST_CASE(
    "Embedded regional discovery reflects an initial value with its custom FOM transportation",
    "[integration][development-profile][object-management]"
    "[data-distribution-management][custom-transportation]"
    "[custom-regional-initial-transport][2025]"
    "[rti.service.subscribe-object-class-attributes-with-regions]"
    "[federate.callback.reflect-attribute-values]") {
  ReportingFederateAmbassador ownerReports;
  ReportingFederateAmbassador receiverReports;
  auto owner = makeRti();
  auto receiver = makeRti();
  std::wstring const federationName = L"custom-regional-initial-transport";
  auto const fom = std::filesystem::path(UMBRA_SOURCE_DIRECTORY) / "packages" /
      "hla-rti-cpp-tck" / "fom" / "fom-model-tck.xml";

  REQUIRE_NOTHROW(owner->connect(ownerReports, HLA_EVOKED));
  REQUIRE_NOTHROW(receiver->connect(receiverReports, HLA_EVOKED));
  REQUIRE_NOTHROW(owner->createFederationExecution(
      federationName, std::vector<std::wstring>{fom.wstring()},
      standard_hla::mom::integer64_time));
  FederateHandle ownerHandle;
  REQUIRE_NOTHROW(ownerHandle = owner->joinFederationExecution(
      L"initial-transport-owner", L"test", federationName));
  REQUIRE_NOTHROW(receiver->joinFederationExecution(
      L"initial-transport-receiver", L"test", federationName));

  auto const ownerClass = owner->getObjectClassHandle(
      fixture_hla::fom::tck_transport_regional_object);
  auto const ownerAttribute = owner->getAttributeHandle(
      ownerClass, fixture_hla::fixture::regional_value);
  auto const ownerX = owner->getDimensionHandle(fixture_hla::fixture::tck_dimension_x);
  auto const ownerY = owner->getDimensionHandle(fixture_hla::fixture::tck_dimension_y);
  auto const receiverClass = receiver->getObjectClassHandle(
      fixture_hla::fom::tck_transport_regional_object);
  auto const receiverAttribute = receiver->getAttributeHandle(
      receiverClass, fixture_hla::fixture::regional_value);
  auto const receiverX = receiver->getDimensionHandle(fixture_hla::fixture::tck_dimension_x);
  auto const receiverY = receiver->getDimensionHandle(fixture_hla::fixture::tck_dimension_y);
  auto const receiverCustom = receiver->getTransportationTypeHandle(
      fixture_hla::fixture::tck_best_effort);
  REQUIRE(ownerClass.isValid());
  REQUIRE(ownerAttribute.isValid());
  REQUIRE(ownerX.isValid());
  REQUIRE(ownerY.isValid());
  REQUIRE(receiverClass.isValid());
  REQUIRE(receiverAttribute.isValid());
  REQUIRE(receiverX.isValid());
  REQUIRE(receiverY.isValid());
  REQUIRE(receiverCustom.isValid());

  owner->publishObjectClassAttributes(ownerClass, AttributeHandleSet{ownerAttribute});
  auto const source = owner->createRegion(DimensionHandleSet{ownerX, ownerY});
  owner->setRangeBounds(source, ownerX, RangeBounds(1UL, 5UL));
  owner->setRangeBounds(source, ownerY, RangeBounds(1UL, 5UL));
  owner->commitRegionModifications(RegionHandleSet{source});
  auto const receiving = receiver->createRegion(DimensionHandleSet{receiverX, receiverY});
  receiver->setRangeBounds(receiving, receiverX, RangeBounds(2UL, 6UL));
  receiver->setRangeBounds(receiving, receiverY, RangeBounds(2UL, 6UL));
  receiver->commitRegionModifications(RegionHandleSet{receiving});
  AttributeHandleSetRegionHandleSetPairVector const sourcePair{{
      AttributeHandleSet{ownerAttribute}, RegionHandleSet{source},
  }};
  AttributeHandleSetRegionHandleSetPairVector const receivingPair{{
      AttributeHandleSet{receiverAttribute}, RegionHandleSet{receiving},
  }};

  auto const object = owner->registerObjectInstanceWithRegions(ownerClass, sourcePair);
  REQUIRE(object.isValid());
  std::array<std::uint8_t, 1U> const value{0x35U};
  AttributeHandleValueMap values;
  values.emplace(ownerAttribute, VariableLengthData(value.data(), value.size()));
  owner->updateAttributeValues(object, values, VariableLengthData{});
  REQUIRE(receiverReports.objectDiscoveryReports.empty());

  // The late regional subscription discovers the stored instance and reflects
  // its initial value using the custom transportation declared by the FOM.
  receiver->subscribeObjectClassAttributesWithRegions(receiverClass, receivingPair);
  auto drain = [&] {
    while (receiver->evokeCallback(0.0)) {
    }
  };
  REQUIRE_NOTHROW(drain());
  REQUIRE(receiverReports.objectDiscoveryReports.size() == 1U);
  REQUIRE(receiverReports.attributeReflectionReports.size() == 1U);
  auto const& reflected = receiverReports.attributeReflectionReports.front();
  REQUIRE(reflected.objectInstance == object);
  REQUIRE(reflected.attributeValues.size() == 1U);
  REQUIRE(reflected.attributeValues.contains(receiverAttribute));
  REQUIRE(reflected.transportationType == receiverCustom);
  REQUIRE(reflected.producingFederate == ownerHandle);

  receiver->resignFederationExecution(NO_ACTION);
  owner->resignFederationExecution(CANCEL_THEN_DELETE_THEN_DIVEST);
  owner->destroyFederationExecution(federationName);
  receiver->disconnect();
  owner->disconnect();
}
