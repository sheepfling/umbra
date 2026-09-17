#include <catch2/catch_test_macros.hpp>

#include "internal/fom/hla_names.hpp"

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
#error "The timestamped source-resignation fanout test requires the Umbra source directory."
#endif

namespace {

namespace standard_hla = umbra::detail::hla::wide;

using rti1516_2025::AttributeHandle;
using rti1516_2025::AttributeHandleSet;
using rti1516_2025::AttributeHandleValueMap;
using rti1516_2025::FederateHandle;
using rti1516_2025::HLA_EVOKED;
using rti1516_2025::LogicalTime;
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

std::wstring nextFederationName() {
  static std::atomic_uint64_t sequence{0U};
  return L"timestamped-attribute-source-resignation-fanout-" +
      std::to_wstring(sequence.fetch_add(1U, std::memory_order_relaxed));
}

std::filesystem::path resourcePath(std::filesystem::path const& relativePath) {
  return std::filesystem::path(UMBRA_SOURCE_DIRECTORY) / "cpp" / "tests" /
      "data" / relativePath;
}

class ReportingFederateAmbassador final
    : public rti1516_2025::NullFederateAmbassador {
 public:
  struct AttributeReflectionReport final {
    ObjectInstanceHandle objectInstance;
    AttributeHandleValueMap attributeValues;
    VariableLengthData userSuppliedTag;
    TransportationTypeHandle transportationType;
    FederateHandle producingFederate;
    bool sentRegionsSupplied = false;
    std::wstring timeImplementationName;
    std::wstring timeValue;
    OrderType sentOrderType = RECEIVE;
    OrderType receivedOrderType = RECEIVE;
    bool retractionSupplied = false;
    bool retractionValid = false;
    VariableLengthData encodedRetraction;
  };

  void discoverObjectInstance(
      ObjectInstanceHandle const& objectInstance,
      ObjectClassHandle const&,
      std::wstring const&,
      FederateHandle const&) override {
    objectDiscoveryReports.push_back(objectInstance);
  }

  void timeConstrainedEnabled(LogicalTime const&) override {
    timeConstrainedEnabledReports.push_back(0);
  }

  void timeRegulationEnabled(LogicalTime const&) override {
    timeRegulationEnabledReports.push_back(0);
  }

  void timeAdvanceGrant(LogicalTime const&) override {
    timeAdvanceGrantReports.push_back(0);
    callbackOrder.push_back("grant");
  }

  void reflectAttributeValues(
      ObjectInstanceHandle const& objectInstance,
      AttributeHandleValueMap const& attributeValues,
      VariableLengthData const& userSuppliedTag,
      TransportationTypeHandle const& transportationType,
      FederateHandle const& producingFederate,
      rti1516_2025::RegionHandleSet const* optionalSentRegions,
      LogicalTime const& time,
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
        time.implementationName(),
        time.toString(),
        sentOrderType,
        receivedOrderType,
        optionalRetraction != nullptr,
        optionalRetraction != nullptr && optionalRetraction->isValid(),
        optionalRetraction != nullptr ? optionalRetraction->encode()
                                       : VariableLengthData{},
    });
    callbackOrder.push_back("reflect");
  }

  std::vector<ObjectInstanceHandle> objectDiscoveryReports;
  std::vector<AttributeReflectionReport> attributeReflectionReports;
  std::vector<int> timeConstrainedEnabledReports;
  std::vector<int> timeRegulationEnabledReports;
  std::vector<int> timeAdvanceGrantReports;
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
    "Embedded queued timestamped attribute update survives source resignation for each recipient",
    "[integration][development-profile][object-management][time-management]"
    "[timestamped-attribute-update][tso][resignation][multi-federate-callback-ordering]"
    "[rti.service.update-attribute-values][rti.service.resign-federation-execution]"
    "[rti.service.enable-time-constrained][rti.service.enable-time-regulation]"
    "[rti.service.time-advance-request][rti.service.next-message-request]"
    "[federate.callback.reflect-attribute-values][federate.callback.time-advance-grant]") {
  ReportingFederateAmbassador ownerReports;
  ReportingFederateAmbassador firstReports;
  ReportingFederateAmbassador secondReports;
  ReportingFederateAmbassador clockReports;
  auto owner = makeRti();
  auto first = makeRti();
  auto second = makeRti();
  auto clock = makeRti();
  auto const federationName = nextFederationName();
  auto const fomModule = resourcePath("attribute-update-passel-fom.xml").wstring();
  std::vector<unsigned char> const valueBytes{0xA7U, 0x71U, 0x52U};
  std::vector<unsigned char> const tagBytes{
      0x52U, 0x45U, 0x53U, 0x2DU, 0x46U, 0x41U, 0x4EU};
  VariableLengthData const tag(tagBytes.data(), tagBytes.size());

  REQUIRE_NOTHROW(owner->connect(ownerReports, HLA_EVOKED));
  REQUIRE_NOTHROW(first->connect(firstReports, HLA_EVOKED));
  REQUIRE_NOTHROW(second->connect(secondReports, HLA_EVOKED));
  REQUIRE_NOTHROW(clock->connect(clockReports, HLA_EVOKED));
  REQUIRE_NOTHROW(owner->createFederationExecution(
      federationName,
      fomModule,
      standard_hla::mom::integer64_time));
  FederateHandle ownerHandle;
  REQUIRE_NOTHROW(ownerHandle = owner->joinFederationExecution(
      L"tso-resigning-attribute-owner",
      L"publisher",
      federationName));
  REQUIRE_NOTHROW(first->joinFederationExecution(
      L"tso-resigning-attribute-first",
      L"subscriber",
      federationName));
  REQUIRE_NOTHROW(second->joinFederationExecution(
      L"tso-resigning-attribute-second",
      L"subscriber",
      federationName));
  REQUIRE_NOTHROW(clock->joinFederationExecution(
      L"tso-resigning-attribute-clock",
      L"publisher",
      federationName));

  auto const objectClass = owner->getObjectClassHandle(
      L"HLAobjectRoot.UmbraAttributeFixtureBase.UmbraAttributeFixtureChild");
  auto const ownerAttribute = owner->getAttributeHandle(objectClass, L"ReliableBaseA");
  auto const firstObjectClass = first->getObjectClassHandle(
      L"HLAobjectRoot.UmbraAttributeFixtureBase.UmbraAttributeFixtureChild");
  auto const firstAttribute = first->getAttributeHandle(firstObjectClass, L"ReliableBaseA");
  auto const secondObjectClass = second->getObjectClassHandle(
      L"HLAobjectRoot.UmbraAttributeFixtureBase.UmbraAttributeFixtureChild");
  auto const secondAttribute = second->getAttributeHandle(secondObjectClass, L"ReliableBaseA");
  REQUIRE(objectClass.isValid());
  REQUIRE(ownerAttribute.isValid());
  REQUIRE(firstAttribute.isValid());
  REQUIRE(secondAttribute.isValid());
  AttributeHandleSet const ownerAttributes{ownerAttribute};
  REQUIRE_NOTHROW(owner->publishObjectClassAttributes(objectClass, ownerAttributes));
  REQUIRE_NOTHROW(owner->changeDefaultAttributeOrderType(
      objectClass,
      ownerAttributes,
      TIMESTAMP));
  REQUIRE_NOTHROW(first->subscribeObjectClassAttributes(
      firstObjectClass,
      AttributeHandleSet{firstAttribute}));
  REQUIRE_NOTHROW(second->subscribeObjectClassAttributes(
      secondObjectClass,
      AttributeHandleSet{secondAttribute}));

  ObjectInstanceHandle objectInstance;
  REQUIRE_NOTHROW(objectInstance = owner->registerObjectInstance(objectClass));
  drainCallbacks(*first);
  drainCallbacks(*second);
  REQUIRE(firstReports.objectDiscoveryReports.size() == 1U);
  REQUIRE(secondReports.objectDiscoveryReports.size() == 1U);

  REQUIRE_NOTHROW(first->enableTimeConstrained());
  REQUIRE_NOTHROW(second->enableTimeConstrained());
  drainCallbacks(*first);
  drainCallbacks(*second);
  REQUIRE(firstReports.timeConstrainedEnabledReports.size() == 1U);
  REQUIRE(secondReports.timeConstrainedEnabledReports.size() == 1U);
  REQUIRE_NOTHROW(owner->enableTimeRegulation(
      rti1516_2025::HLAinteger64Interval(5)));
  drainCallbacks(*owner);
  REQUIRE(ownerReports.timeRegulationEnabledReports.size() == 1U);
  REQUIRE_NOTHROW(clock->enableTimeRegulation(
      rti1516_2025::HLAinteger64Interval(5)));
  drainCallbacks(*clock);
  REQUIRE(clockReports.timeRegulationEnabledReports.size() == 1U);

  AttributeHandleValueMap values;
  values.emplace(
      ownerAttribute,
      VariableLengthData(valueBytes.data(), valueBytes.size()));
  auto const retraction = owner->updateAttributeValues(
      objectInstance,
      values,
      tag,
      rti1516_2025::HLAinteger64Time(7));
  REQUIRE(retraction.isValid());
  REQUIRE(firstReports.attributeReflectionReports.empty());
  REQUIRE(secondReports.attributeReflectionReports.empty());

  REQUIRE_NOTHROW(first->timeAdvanceRequest(
      rti1516_2025::HLAinteger64Time(7)));
  REQUIRE_NOTHROW(second->nextMessageRequest(
      rti1516_2025::HLAinteger64Time(10)));
  REQUIRE(firstReports.attributeReflectionReports.empty());
  REQUIRE(secondReports.attributeReflectionReports.empty());

  REQUIRE_NOTHROW(owner->resignFederationExecution(
      rti1516_2025::UNCONDITIONALLY_DIVEST_ATTRIBUTES));
  REQUIRE(firstReports.attributeReflectionReports.empty());
  REQUIRE(secondReports.attributeReflectionReports.empty());

  REQUIRE_NOTHROW(clock->timeAdvanceRequest(
      rti1516_2025::HLAinteger64Time(2)));
  drainCallbacks(*clock);
  REQUIRE(clockReports.timeAdvanceGrantReports.size() == 1U);
  firstReports.callbackOrder.clear();
  drainCallbacks(*first);
  REQUIRE(firstReports.attributeReflectionReports.size() == 1U);
  REQUIRE(firstReports.timeAdvanceGrantReports.size() == 1U);
  REQUIRE(firstReports.callbackOrder == std::vector<std::string>{"reflect", "grant"});
  REQUIRE(secondReports.attributeReflectionReports.empty());
  REQUIRE(secondReports.timeAdvanceGrantReports.empty());

  secondReports.callbackOrder.clear();
  drainCallbacks(*second);
  REQUIRE(secondReports.attributeReflectionReports.size() == 1U);
  REQUIRE(secondReports.timeAdvanceGrantReports.size() == 1U);
  REQUIRE(secondReports.callbackOrder == std::vector<std::string>{"reflect", "grant"});

  auto requireReflection = [&](
      ReportingFederateAmbassador::AttributeReflectionReport const& report,
      AttributeHandle const& expectedAttribute) {
    REQUIRE(report.objectInstance == objectInstance);
    REQUIRE(report.attributeValues.size() == 1U);
    REQUIRE(report.attributeValues.contains(expectedAttribute));
    REQUIRE(variableLengthDataBytes(report.attributeValues.at(expectedAttribute)) ==
            valueBytes);
    REQUIRE(variableLengthDataBytes(report.userSuppliedTag) == tagBytes);
    REQUIRE(report.producingFederate == ownerHandle);
    REQUIRE_FALSE(report.sentRegionsSupplied);
    REQUIRE(report.timeImplementationName == standard_hla::mom::integer64_time);
    REQUIRE(report.timeValue == L"7");
    REQUIRE(report.sentOrderType == TIMESTAMP);
    REQUIRE(report.receivedOrderType == TIMESTAMP);
    REQUIRE(report.retractionSupplied);
    REQUIRE(report.retractionValid);
    REQUIRE(variableLengthDataBytes(report.encodedRetraction) ==
            variableLengthDataBytes(retraction.encode()));
  };
  requireReflection(firstReports.attributeReflectionReports.front(), firstAttribute);
  requireReflection(secondReports.attributeReflectionReports.front(), secondAttribute);

  REQUIRE_THROWS_AS(
      owner->retract(retraction),
      rti1516_2025::FederateNotExecutionMember);

  REQUIRE_NOTHROW(first->resignFederationExecution(rti1516_2025::NO_ACTION));
  REQUIRE_NOTHROW(second->resignFederationExecution(rti1516_2025::NO_ACTION));
  REQUIRE_NOTHROW(clock->resignFederationExecution(rti1516_2025::NO_ACTION));
  REQUIRE_NOTHROW(first->destroyFederationExecution(federationName));
  REQUIRE_NOTHROW(owner->disconnect());
  REQUIRE_NOTHROW(first->disconnect());
  REQUIRE_NOTHROW(second->disconnect());
  REQUIRE_NOTHROW(clock->disconnect());
}
