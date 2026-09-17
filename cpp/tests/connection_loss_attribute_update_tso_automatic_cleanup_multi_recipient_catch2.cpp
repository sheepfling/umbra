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
#error "The multi-recipient automatic-cleanup tests require the Umbra source directory."
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

class ReportingFederateAmbassador final
    : public rti1516_2025::NullFederateAmbassador {
 public:
  struct TimeAdvanceGrantReport final {
    std::wstring implementationName;
    std::wstring value;
  };

  struct TimeRegulationReport final {
    std::wstring implementationName;
    std::wstring value;
  };

  struct AttributeReflectionReport final {
    ObjectInstanceHandle objectInstance;
    AttributeHandleValueMap attributeValues;
    VariableLengthData userSuppliedTag;
    TransportationTypeHandle transportationType;
    FederateHandle producingFederate;
    std::wstring timeImplementationName;
    std::wstring timeValue;
    OrderType sentOrderType = RECEIVE;
    OrderType receivedOrderType = RECEIVE;
    bool retractionSupplied = false;
    bool retractionValid = false;
  };

  struct ObjectRemovalReport final {
    ObjectInstanceHandle objectInstance;
    VariableLengthData userSuppliedTag;
    FederateHandle producingFederate;
    OrderType sentOrderType = RECEIVE;
    OrderType receivedOrderType = RECEIVE;
  };

  void connectionLost(std::wstring const& faultDescription) override {
    faultDescriptions.push_back(faultDescription);
  }

  void timeAdvanceGrant(rti1516_2025::LogicalTime const& time) override {
    timeAdvanceGrantReports.push_back({time.implementationName(), time.toString()});
    callbackOrder.push_back("grant");
  }

  void timeRegulationEnabled(rti1516_2025::LogicalTime const& time) override {
    timeRegulationEnabledReports.push_back({time.implementationName(), time.toString()});
  }

  void timeConstrainedEnabled(rti1516_2025::LogicalTime const& time) override {
    timeConstrainedEnabledReports.push_back({time.implementationName(), time.toString()});
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
      VariableLengthData const& userSuppliedTag,
      TransportationTypeHandle const& transportationType,
      FederateHandle const& producingFederate,
      rti1516_2025::RegionHandleSet const*,
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
      VariableLengthData const& userSuppliedTag,
      FederateHandle const& producingFederate) override {
    objectRemovalReports.push_back({
        objectInstance,
        userSuppliedTag,
        producingFederate,
    });
    callbackOrder.push_back("remove");
  }

  std::vector<std::wstring> faultDescriptions;
  std::vector<TimeAdvanceGrantReport> timeAdvanceGrantReports;
  std::vector<TimeRegulationReport> timeRegulationEnabledReports;
  std::vector<TimeRegulationReport> timeConstrainedEnabledReports;
  std::vector<AttributeReflectionReport> attributeReflectionReports;
  std::vector<ObjectRemovalReport> objectRemovalReports;
  std::vector<std::string> callbackOrder;
  std::size_t discoveryCount = 0U;
};

std::unique_ptr<RTIambassador> makeRti() {
  RTIambassadorFactory factory;
  return factory.createRTIambassador();
}

void requireCutoffReflection(
    ReportingFederateAmbassador const& reports,
    ObjectInstanceHandle const& objectInstance,
    AttributeHandle const& attribute,
    VariableLengthData const& value,
    VariableLengthData const& tag,
    TransportationTypeHandle const& reliableTransport,
    FederateHandle const& lostFederate) {
  REQUIRE(reports.attributeReflectionReports.size() == 1U);
  REQUIRE(reports.objectRemovalReports.empty());
  REQUIRE(reports.timeAdvanceGrantReports.size() == 1U);
  REQUIRE(reports.callbackOrder == std::vector<std::string>{"reflect", "grant"});
  auto const& reflection = reports.attributeReflectionReports.front();
  REQUIRE(reflection.objectInstance == objectInstance);
  REQUIRE(reflection.attributeValues.size() == 1U);
  REQUIRE(reflection.attributeValues.contains(attribute));
  REQUIRE(variableLengthDataBytes(reflection.attributeValues.at(attribute)) ==
          variableLengthDataBytes(value));
  REQUIRE(variableLengthDataBytes(reflection.userSuppliedTag) ==
          variableLengthDataBytes(tag));
  REQUIRE(reflection.transportationType == reliableTransport);
  REQUIRE(reflection.producingFederate == lostFederate);
  REQUIRE(reflection.timeImplementationName == standard_hla::mom::integer64_time);
  REQUIRE(reflection.timeValue == L"6");
  REQUIRE(reflection.sentOrderType == TIMESTAMP);
  REQUIRE(reflection.receivedOrderType == TIMESTAMP);
  REQUIRE(reflection.retractionSupplied);
  REQUIRE(reflection.retractionValid);
  REQUIRE(reports.timeAdvanceGrantReports.front().implementationName ==
          standard_hla::mom::integer64_time);
  REQUIRE(reports.timeAdvanceGrantReports.front().value == L"6");
}

void requireAutomaticRemoval(
    ReportingFederateAmbassador const& reports,
    ObjectInstanceHandle const& objectInstance,
    FederateHandle const& lostFederate) {
  REQUIRE(reports.objectRemovalReports.size() == 1U);
  auto const& removal = reports.objectRemovalReports.front();
  REQUIRE(removal.objectInstance == objectInstance);
  REQUIRE(variableLengthDataBytes(removal.userSuppliedTag).empty());
  REQUIRE(removal.producingFederate == lostFederate);
  REQUIRE(removal.sentOrderType == RECEIVE);
  REQUIRE(removal.receivedOrderType == RECEIVE);
}

}  // namespace

TEST_CASE(
    "Embedded transport loss drains automatic cleanup independently after cutoff attribute updates",
    "[integration][development-profile][federation-management][transport]"
    "[object-management][time-management][timestamped-attribute-update]"
    "[connection-lost-tso-cutoff][multi-recipient]"
    "[rti.service.connection-lost][rti.service.update-attribute-values]"
    "[rti.service.time-advance-request][rti.service.set-automatic-resign-directive]"
    "[federate.callback.connection-lost][federate.callback.reflect-attribute-values]"
    "[federate.callback.remove-object-instance][federate.callback.time-advance-grant]") {
  ReportingFederateAmbassador lostReports;
  ReportingFederateAmbassador firstReports;
  ReportingFederateAmbassador secondReports;
  auto lost = makeRti();
  auto first = makeRti();
  auto second = makeRti();
  auto const federationName = std::wstring{L"connection-loss-attribute-tso-automatic-cleanup-multi"};
  auto const fomModule = (std::filesystem::path(UMBRA_SOURCE_DIRECTORY) /
                          "cpp" / "tests" / "data" /
                          "attribute-update-passel-fom.xml")
                             .wstring();
  unsigned char const valueBytes[] = {0x4D, 0x55, 0x4C, 0x54, 0x49};
  unsigned char const tagBytes[] = {0x4D, 0x55, 0x4C, 0x54, 0x49, 0x2D, 0x43, 0x55};
  VariableLengthData const value(valueBytes, sizeof(valueBytes));
  VariableLengthData const tag(tagBytes, sizeof(tagBytes));
  auto const drain = [](RTIambassador& ambassador) {
    while (ambassador.evokeCallback(0.0)) {
    }
  };

  REQUIRE_NOTHROW(lost->connect(lostReports, HLA_EVOKED));
  REQUIRE_NOTHROW(first->connect(firstReports, HLA_EVOKED));
  REQUIRE_NOTHROW(second->connect(secondReports, HLA_EVOKED));
  REQUIRE_NOTHROW(lost->createFederationExecution(
      federationName,
      std::vector<std::wstring>{fomModule},
      standard_hla::mom::integer64_time));
  FederateHandle lostFederate;
  REQUIRE_NOTHROW(lostFederate = lost->joinFederationExecution(
      L"multi-automatic-delete-lost-attribute-publisher", L"publisher", federationName));
  REQUIRE_NOTHROW(first->joinFederationExecution(
      L"multi-automatic-delete-first-attribute-subscriber", L"subscriber", federationName));
  REQUIRE_NOTHROW(second->joinFederationExecution(
      L"multi-automatic-delete-second-attribute-subscriber", L"subscriber", federationName));

  auto const objectClass = lost->getObjectClassHandle(
      fixture_hla::fom::attribute_fixture_child);
  auto const attribute = lost->getAttributeHandle(
      objectClass,
      fixture_hla::fixture::reliable_base_a);
  auto const reliableTransport = lost->getTransportationTypeHandle(
      standard_hla::mom::reliable);
  REQUIRE(objectClass.isValid());
  REQUIRE(attribute.isValid());
  REQUIRE(reliableTransport.isValid());
  AttributeHandleSet const attributes{attribute};
  AttributeHandleValueMap attributeValues;
  attributeValues.emplace(attribute, value);

  REQUIRE_NOTHROW(lost->publishObjectClassAttributes(objectClass, attributes));
  REQUIRE_NOTHROW(first->subscribeObjectClassAttributes(objectClass, attributes));
  REQUIRE_NOTHROW(second->subscribeObjectClassAttributes(objectClass, attributes));
  REQUIRE_NOTHROW(lost->setAutomaticResignDirective(
      rti1516_2025::DELETE_OBJECTS));

  ObjectInstanceHandle objectInstance;
  REQUIRE_NOTHROW(objectInstance = lost->registerObjectInstance(objectClass));
  REQUIRE(objectInstance.isValid());
  auto const objectName = lost->getObjectInstanceName(objectInstance);
  drain(*first);
  drain(*second);
  REQUIRE(firstReports.discoveryCount == 1U);
  REQUIRE(secondReports.discoveryCount == 1U);
  drain(*lost);

  REQUIRE_NOTHROW(lost->changeAttributeOrderType(
      objectInstance,
      attributes,
      TIMESTAMP));
  REQUIRE_NOTHROW(first->enableTimeConstrained());
  REQUIRE_NOTHROW(second->enableTimeConstrained());
  drain(*first);
  drain(*second);
  REQUIRE(firstReports.timeConstrainedEnabledReports.size() == 1U);
  REQUIRE(secondReports.timeConstrainedEnabledReports.size() == 1U);
  REQUIRE_NOTHROW(lost->enableTimeRegulation(
      rti1516_2025::HLAinteger64Interval(5)));
  drain(*lost);
  REQUIRE(lostReports.timeRegulationEnabledReports.size() == 1U);

  auto const retraction = lost->updateAttributeValues(
      objectInstance,
      attributeValues,
      tag,
      rti1516_2025::HLAinteger64Time(6));
  REQUIRE(retraction.isValid());
  REQUIRE(firstReports.attributeReflectionReports.empty());
  REQUIRE(secondReports.attributeReflectionReports.empty());

  // Keep only the first survivor's TAR pending at the fault.  The second
  // survivor crosses the same cutoff after the source has been removed.
  REQUIRE_NOTHROW(first->timeAdvanceRequest(
      rti1516_2025::HLAinteger64Time(6)));
  REQUIRE_NOTHROW(lost->timeAdvanceRequest(
      rti1516_2025::HLAinteger64Time(6)));
  drain(*lost);
  REQUIRE(lostReports.timeAdvanceGrantReports.size() == 1U);
  REQUIRE(lostReports.timeAdvanceGrantReports.front().value == L"6");
  REQUIRE(firstReports.timeAdvanceGrantReports.empty());
  REQUIRE(secondReports.timeAdvanceGrantReports.empty());

  std::wstring const faultDescription =
      L"multi-recipient timestamped attribute automatic-delete cutoff transport fault";
  REQUIRE(umbra::detail::failEmbeddedTransportConnectionForTesting(
      *lost,
      faultDescription));

  drain(*first);
  requireCutoffReflection(
      firstReports,
      objectInstance,
      attribute,
      value,
      tag,
      reliableTransport,
      lostFederate);
  REQUIRE(secondReports.attributeReflectionReports.empty());
  REQUIRE(secondReports.objectRemovalReports.empty());
  REQUIRE(secondReports.timeAdvanceGrantReports.empty());

  REQUIRE_NOTHROW(second->timeAdvanceRequest(
      rti1516_2025::HLAinteger64Time(6)));
  drain(*second);
  requireCutoffReflection(
      secondReports,
      objectInstance,
      attribute,
      value,
      tag,
      reliableTransport,
      lostFederate);

  // Each recipient owns a separate automatic-removal gate.  Advancing only
  // the first survivor releases only its removal; the second remains live.
  REQUIRE_NOTHROW(first->timeAdvanceRequest(
      rti1516_2025::HLAinteger64Time(7)));
  drain(*first);
  requireAutomaticRemoval(firstReports, objectInstance, lostFederate);
  REQUIRE(firstReports.timeAdvanceGrantReports.size() == 1U);
  REQUIRE_THROWS_AS(
      first->getObjectInstanceHandle(objectName),
      rti1516_2025::ObjectInstanceNotKnown);
  REQUIRE(secondReports.objectRemovalReports.empty());
  REQUIRE(second->getObjectInstanceHandle(objectName) == objectInstance);

  REQUIRE_NOTHROW(second->timeAdvanceRequest(
      rti1516_2025::HLAinteger64Time(7)));
  drain(*second);
  requireAutomaticRemoval(secondReports, objectInstance, lostFederate);
  REQUIRE(secondReports.timeAdvanceGrantReports.size() == 1U);
  REQUIRE_THROWS_AS(
      second->getObjectInstanceHandle(objectName),
      rti1516_2025::ObjectInstanceNotKnown);

  drain(*lost);
  REQUIRE(lostReports.faultDescriptions ==
          std::vector<std::wstring>{faultDescription});
  REQUIRE_THROWS_AS(lost->disconnect(), rti1516_2025::NotConnected);
  REQUIRE_NOTHROW(lost->connect(lostReports, HLA_EVOKED));
  REQUIRE_NOTHROW(lost->disconnect());
  REQUIRE_NOTHROW(first->resignFederationExecution(rti1516_2025::NO_ACTION));
  REQUIRE_NOTHROW(second->resignFederationExecution(rti1516_2025::NO_ACTION));
  REQUIRE_NOTHROW(first->destroyFederationExecution(federationName));
  REQUIRE_NOTHROW(first->disconnect());
  REQUIRE_NOTHROW(second->disconnect());
}
