#include <catch2/catch_test_macros.hpp>

#include "internal/federation/embedded_transport.hpp"
#include "internal/fom/hla_names.hpp"
#include "hla_test_names.hpp"

#include <RTI/NullFederateAmbassador.h>
#include <RTI/RTI1516.h>
#include <RTI/time/HLAinteger64Interval.h>
#include <RTI/time/HLAinteger64Time.h>

#include <filesystem>
#include <memory>
#include <string>
#include <vector>

#ifndef UMBRA_SOURCE_DIRECTORY
#error "The regional-selector connection-loss tests require the Umbra source directory."
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
using rti1516_2025::RTIambassador;
using rti1516_2025::RTIambassadorFactory;
using rti1516_2025::RegionHandle;
using rti1516_2025::RegionHandleSet;
using rti1516_2025::RangeBounds;
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

class ReportingFederateAmbassador final
    : public rti1516_2025::NullFederateAmbassador {
 public:
  struct TimeReport final {
    std::wstring implementationName;
    std::wstring value;
  };

  struct ReflectionReport final {
    ObjectInstanceHandle objectInstance;
    AttributeHandleValueMap attributeValues;
    VariableLengthData tag;
    TransportationTypeHandle transportationType;
    FederateHandle producer;
    std::wstring timeImplementationName;
    std::wstring timeValue;
    OrderType sentOrderType = RECEIVE;
    OrderType receivedOrderType = RECEIVE;
    bool retractionSupplied = false;
    bool retractionValid = false;
  };

  struct RemovalReport final {
    ObjectInstanceHandle objectInstance;
    VariableLengthData tag;
    FederateHandle producer;
  };

  void connectionLost(std::wstring const& description) override {
    faultDescriptions.push_back(description);
  }

  void timeAdvanceGrant(rti1516_2025::LogicalTime const& time) override {
    grants.push_back({time.implementationName(), time.toString()});
    callbackOrder.push_back("grant");
  }

  void timeRegulationEnabled(rti1516_2025::LogicalTime const& time) override {
    regulationTimes.push_back({time.implementationName(), time.toString()});
  }

  void timeConstrainedEnabled(rti1516_2025::LogicalTime const& time) override {
    constrainedTimes.push_back({time.implementationName(), time.toString()});
  }

  void discoverObjectInstance(
      ObjectInstanceHandle const&,
      ObjectClassHandle const&,
      std::wstring const&,
      FederateHandle const&) override {
    ++discoveryCount;
  }

  void reflectAttributeValues(
      ObjectInstanceHandle const& objectInstance,
      AttributeHandleValueMap const& attributeValues,
      VariableLengthData const& tag,
      TransportationTypeHandle const& transportationType,
      FederateHandle const& producer,
      rti1516_2025::RegionHandleSet const*,
      rti1516_2025::LogicalTime const& time,
      OrderType sentOrderType,
      OrderType receivedOrderType,
      rti1516_2025::MessageRetractionHandle const* optionalRetraction) override {
    reflections.push_back({
        objectInstance,
        attributeValues,
        tag,
        transportationType,
        producer,
        time.implementationName(),
        time.toString(),
        sentOrderType,
        receivedOrderType,
        optionalRetraction != nullptr,
        optionalRetraction != nullptr && optionalRetraction->isValid(),
    });
    callbackOrder.push_back("reflect");
  }

  void removeObjectInstance(
      ObjectInstanceHandle const& objectInstance,
      VariableLengthData const& tag,
      FederateHandle const& producer) override {
    removals.push_back({objectInstance, tag, producer});
    callbackOrder.push_back("remove");
  }

  std::vector<std::wstring> faultDescriptions;
  std::vector<TimeReport> grants;
  std::vector<TimeReport> regulationTimes;
  std::vector<TimeReport> constrainedTimes;
  std::vector<ReflectionReport> reflections;
  std::vector<RemovalReport> removals;
  std::vector<std::string> callbackOrder;
  std::size_t discoveryCount = 0U;
};

std::unique_ptr<RTIambassador> makeRti() {
  RTIambassadorFactory factory;
  return factory.createRTIambassador();
}

}  // namespace

TEST_CASE(
    "Embedded transport loss rechecks a regional selector before automatic cleanup",
    "[integration][development-profile][federation-management][transport]"
    "[object-management][ddm][time-management]"
    "[timestamped-regional-attribute-update][connection-lost-tso-cutoff]"
    "[connection-lost-regional-selector-mutation][automatic-resign-delete]"
    "[rti.service.connection-lost][rti.service.update-attribute-values]"
    "[rti.service.set-range-bounds][rti.service.commit-region-modifications]"
    "[rti.service.time-advance-request][rti.service.set-automatic-resign-directive]"
    "[rti.service.register-object-instance-with-regions]"
    "[rti.service.subscribe-object-class-attributes-with-regions]"
    "[federate.callback.connection-lost][federate.callback.reflect-attribute-values]"
    "[federate.callback.remove-object-instance][federate.callback.time-advance-grant]") {
  ReportingFederateAmbassador lostReports;
  ReportingFederateAmbassador survivingReports;
  auto lost = makeRti();
  auto surviving = makeRti();
  auto const federationName =
      std::wstring{L"connection-loss-regional-selector-mutation"};
  auto const fomModule = (std::filesystem::path(UMBRA_SOURCE_DIRECTORY) /
                          "third_party" / "ieee1516.2-2025" / "resources" /
                          "examples" / "RestaurantFOMmodule-2025.xml")
                             .wstring();
  unsigned char const valueBytes[] = {0x52, 0x45, 0x47, 0x2D, 0x54, 0x53, 0x4F};
  unsigned char const tagBytes[] = {0x52, 0x45, 0x47, 0x2D, 0x54, 0x53, 0x4F};
  VariableLengthData const value(valueBytes, sizeof(valueBytes));
  VariableLengthData const tag(tagBytes, sizeof(tagBytes));
  auto const drain = [](RTIambassador& ambassador) {
    while (ambassador.evokeCallback(0.0)) {
    }
  };

  REQUIRE_NOTHROW(lost->connect(lostReports, HLA_EVOKED));
  REQUIRE_NOTHROW(surviving->connect(survivingReports, HLA_EVOKED));
  REQUIRE_NOTHROW(lost->createFederationExecution(
      federationName,
      fomModule,
      standard_hla::mom::integer64_time));
  FederateHandle lostFederate;
  REQUIRE_NOTHROW(lostFederate = lost->joinFederationExecution(
      L"regional-selector-lost-publisher", L"publisher", federationName));
  REQUIRE_NOTHROW(surviving->joinFederationExecution(
      L"regional-selector-surviving-subscriber", L"subscriber", federationName));

  auto const objectClass = lost->getObjectClassHandle(
      fixture_hla::fom::food_drink_soda);
  auto const attribute = lost->getAttributeHandle(
      objectClass,
      fixture_hla::fixture::flavor);
  auto const dimension = lost->getDimensionHandle(
      fixture_hla::fixture::soda_flavor);
  auto const reliableTransport = lost->getTransportationTypeHandle(
      standard_hla::mom::reliable);
  REQUIRE(objectClass.isValid());
  REQUIRE(attribute.isValid());
  REQUIRE(dimension.isValid());
  REQUIRE(reliableTransport.isValid());

  AttributeHandleSet const attributes{attribute};
  AttributeHandleValueMap const values{{attribute, value}};
  REQUIRE_NOTHROW(lost->publishObjectClassAttributes(objectClass, attributes));
  REQUIRE_NOTHROW(lost->changeDefaultAttributeOrderType(
      objectClass,
      attributes,
      TIMESTAMP));

  RegionHandle sourceRegion;
  RegionHandle receiverRegion;
  REQUIRE_NOTHROW(sourceRegion = lost->createRegion(
      rti1516_2025::DimensionHandleSet{dimension}));
  REQUIRE_NOTHROW(receiverRegion = surviving->createRegion(
      rti1516_2025::DimensionHandleSet{dimension}));
  REQUIRE_NOTHROW(lost->setRangeBounds(
      sourceRegion,
      dimension,
      RangeBounds(0UL, 2UL)));
  REQUIRE_NOTHROW(lost->commitRegionModifications(
      RegionHandleSet{sourceRegion}));
  REQUIRE_NOTHROW(surviving->setRangeBounds(
      receiverRegion,
      dimension,
      RangeBounds(1UL, 3UL)));
  REQUIRE_NOTHROW(surviving->commitRegionModifications(
      RegionHandleSet{receiverRegion}));

  AttributeHandleSetRegionHandleSetPairVector const sourcePair{{
      attributes,
      RegionHandleSet{sourceRegion},
  }};
  AttributeHandleSetRegionHandleSetPairVector const receiverPair{{
      attributes,
      RegionHandleSet{receiverRegion},
  }};
  REQUIRE_NOTHROW(surviving->subscribeObjectClassAttributesWithRegions(
      objectClass,
      receiverPair));
  REQUIRE_NOTHROW(surviving->setConveyRegionDesignatorSetsSwitch(true));

  ObjectInstanceHandle objectInstance;
  REQUIRE_NOTHROW(objectInstance = lost->registerObjectInstanceWithRegions(
      objectClass,
      sourcePair));
  REQUIRE(objectInstance.isValid());
  auto const objectName = lost->getObjectInstanceName(objectInstance);
  drain(*surviving);
  REQUIRE(survivingReports.discoveryCount == 1U);

  REQUIRE_NOTHROW(lost->setAutomaticResignDirective(
      rti1516_2025::DELETE_OBJECTS));
  REQUIRE_NOTHROW(surviving->enableTimeConstrained());
  drain(*surviving);
  REQUIRE(survivingReports.constrainedTimes.size() == 1U);
  REQUIRE_NOTHROW(lost->enableTimeRegulation(
      rti1516_2025::HLAinteger64Interval(5)));
  drain(*lost);
  REQUIRE(lostReports.regulationTimes.size() == 1U);

  auto const retraction = lost->updateAttributeValues(
      objectInstance,
      values,
      tag,
      rti1516_2025::HLAinteger64Time(6));
  REQUIRE(retraction.isValid());
  REQUIRE(survivingReports.reflections.empty());
  REQUIRE_NOTHROW(surviving->timeAdvanceRequest(
      rti1516_2025::HLAinteger64Time(6)));
  REQUIRE_NOTHROW(lost->timeAdvanceRequest(
      rti1516_2025::HLAinteger64Time(6)));
  drain(*lost);
  REQUIRE(lostReports.grants.size() == 1U);
  REQUIRE(lostReports.grants.front().value == L"6");
  REQUIRE(survivingReports.grants.empty());

  std::wstring const faultDescription =
      L"regional selector mutation cutoff transport fault";
  REQUIRE(umbra::detail::failEmbeddedTransportConnectionForTesting(
      *lost,
      faultDescription));
  REQUIRE(survivingReports.reflections.empty());
  REQUIRE(survivingReports.removals.empty());

  // The update was admitted while the regions overlapped, but callback-time
  // DDM is now disjoint after the receiver commits [3,4).
  REQUIRE_NOTHROW(surviving->setRangeBounds(
      receiverRegion,
      dimension,
      RangeBounds(3UL, 4UL)));
  REQUIRE_NOTHROW(surviving->commitRegionModifications(
      RegionHandleSet{receiverRegion}));
  drain(*surviving);
  REQUIRE(survivingReports.reflections.empty());
  REQUIRE(survivingReports.removals.empty());
  REQUIRE(survivingReports.grants.size() == 1U);
  REQUIRE(survivingReports.grants.front().value == L"6");

  // The independent receive-order automatic removal remains deliverable at
  // the next gate after the stale regional TSO candidate is suppressed.
  survivingReports.callbackOrder.clear();
  REQUIRE_NOTHROW(surviving->timeAdvanceRequest(
      rti1516_2025::HLAinteger64Time(7)));
  drain(*surviving);
  REQUIRE(survivingReports.reflections.empty());
  REQUIRE(survivingReports.removals.size() == 1U);
  REQUIRE(survivingReports.grants.size() == 1U);
  REQUIRE(survivingReports.callbackOrder == std::vector<std::string>{"remove"});
  auto const& removal = survivingReports.removals.front();
  REQUIRE(removal.objectInstance == objectInstance);
  REQUIRE(variableLengthDataBytes(removal.tag).empty());
  REQUIRE(removal.producer == lostFederate);
  REQUIRE_THROWS_AS(
      surviving->getObjectInstanceHandle(objectName),
      rti1516_2025::ObjectInstanceNotKnown);

  drain(*lost);
  REQUIRE(lostReports.faultDescriptions ==
          std::vector<std::wstring>{faultDescription});
  REQUIRE_THROWS_AS(lost->disconnect(), rti1516_2025::NotConnected);
  REQUIRE_NOTHROW(lost->connect(lostReports, HLA_EVOKED));
  REQUIRE_NOTHROW(lost->disconnect());
  REQUIRE_NOTHROW(surviving->unsubscribeObjectClassAttributesWithRegions(
      objectClass,
      receiverPair));
  REQUIRE_NOTHROW(surviving->deleteRegion(receiverRegion));
  REQUIRE_NOTHROW(surviving->resignFederationExecution(rti1516_2025::NO_ACTION));
  REQUIRE_NOTHROW(surviving->destroyFederationExecution(federationName));
  REQUIRE_NOTHROW(surviving->disconnect());
}
