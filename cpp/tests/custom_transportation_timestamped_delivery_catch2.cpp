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
#error "The custom transportation timestamped-delivery test requires the Umbra source directory."
#endif

namespace {

namespace standard_hla = umbra::detail::hla::wide;
namespace fixture_hla = umbra::test::hla::wide;

using rti1516_2025::AttributeHandle;
using rti1516_2025::AttributeHandleValueMap;
using rti1516_2025::AttributeHandleSet;
using rti1516_2025::FederateHandle;
using rti1516_2025::HLA_EVOKED;
using rti1516_2025::InteractionClassHandle;
using rti1516_2025::LogicalTime;
using rti1516_2025::MessageRetractionHandle;
using rti1516_2025::NullFederateAmbassador;
using rti1516_2025::ObjectClassHandle;
using rti1516_2025::ObjectInstanceHandle;
using rti1516_2025::OrderType;
using rti1516_2025::ParameterHandleValueMap;
using rti1516_2025::RECEIVE;
using rti1516_2025::RegionHandleSet;
using rti1516_2025::RTIambassador;
using rti1516_2025::RTIambassadorFactory;
using rti1516_2025::TIMESTAMP;
using rti1516_2025::TransportationTypeHandle;
using rti1516_2025::VariableLengthData;

std::wstring nextFederationName() {
  static std::atomic_uint64_t sequence{0U};
  return L"custom-transportation-timestamped-delivery-" +
      std::to_wstring(sequence.fetch_add(1U, std::memory_order_relaxed));
}

std::filesystem::path resourcePath(std::filesystem::path const& relativePath) {
  return std::filesystem::path(UMBRA_SOURCE_DIRECTORY) / "cpp" / "tests" /
      "data" / relativePath;
}

class ReportingFederateAmbassador final : public NullFederateAmbassador {
 public:
  struct TimestampedInteractionReport final {
    InteractionClassHandle interactionClass;
    ParameterHandleValueMap parameterValues;
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

  void receiveInteraction(
      InteractionClassHandle const& interactionClass,
      ParameterHandleValueMap const& parameterValues,
      VariableLengthData const& userSuppliedTag,
      TransportationTypeHandle const& transportationType,
      FederateHandle const& producingFederate,
      RegionHandleSet const*,
      LogicalTime const& time,
      OrderType sentOrderType,
      OrderType receivedOrderType,
      MessageRetractionHandle const* optionalRetraction) override {
    timestampedInteractionReports.push_back({
        interactionClass,
        parameterValues,
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
    callbackOrder.push_back("interaction");
  }

  void reflectAttributeValues(
      ObjectInstanceHandle const& objectInstance,
      AttributeHandleValueMap const& attributeValues,
      VariableLengthData const& userSuppliedTag,
      TransportationTypeHandle const& transportationType,
      FederateHandle const& producingFederate,
      RegionHandleSet const*,
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
        time.implementationName(),
        time.toString(),
        sentOrderType,
        receivedOrderType,
        optionalRetraction != nullptr,
        optionalRetraction != nullptr && optionalRetraction->isValid(),
    });
    callbackOrder.push_back("attribute");
  }

  void timeAdvanceGrant(LogicalTime const& time) override {
    timeAdvanceGrantReports.push_back({time.implementationName(), time.toString()});
    callbackOrder.push_back("grant");
  }

  struct TimeAdvanceGrantReport final {
    std::wstring timeImplementationName;
    std::wstring value;
  };

  std::vector<ObjectInstanceHandle> objectDiscoveryReports;
  std::vector<int> timeConstrainedEnabledReports;
  std::vector<int> timeRegulationEnabledReports;
  std::vector<TimestampedInteractionReport> timestampedInteractionReports;
  std::vector<AttributeReflectionReport> attributeReflectionReports;
  std::vector<TimeAdvanceGrantReport> timeAdvanceGrantReports;
  std::vector<std::string> callbackOrder;
};

std::unique_ptr<RTIambassador> makeRti() {
  RTIambassadorFactory factory;
  return factory.createRTIambassador();
}

}  // namespace

TEST_CASE(
    "Embedded timestamped delivery accepts a declared custom FOM transportation",
    "[integration][development-profile][federation-management]"
    "[custom-transportation][custom-transportation-timestamped-delivery]"
    "[fom][transportation-type-lookup][transportation][time-management][tso]"
    "[interaction-management][object-management]"
    "[rti.service.get-transportation-type-handle]"
    "[rti.service.publish-interaction-class][rti.service.subscribe-interaction-class]"
    "[rti.service.change-interaction-order-type][rti.service.send-interaction]"
    "[rti.service.get-object-class-handle][rti.service.get-attribute-handle]"
    "[rti.service.publish-object-class-attributes][rti.service.subscribe-object-class-attributes]"
    "[rti.service.change-default-attribute-order-type][rti.service.register-object-instance]"
    "[rti.service.enable-time-constrained][rti.service.enable-time-regulation]"
    "[rti.service.update-attribute-values][rti.service.time-advance-request]"
    "[rti.service.evoke-callback]"
    "[federate.callback.receive-interaction][federate.callback.reflect-attribute-values]"
    "[federate.callback.time-advance-grant]") {
  ReportingFederateAmbassador producerReports;
  ReportingFederateAmbassador consumerReports;
  auto producer = makeRti();
  auto consumer = makeRti();
  auto const federationName = nextFederationName();
  auto const testData = std::filesystem::path(UMBRA_SOURCE_DIRECTORY) / "cpp" /
      "tests" / "data";
  std::vector<std::wstring> const fomModules{
      (testData / "transportation-reference-consumer-fom.xml").wstring(),
      (testData / "transportation-reference-provider-fom.xml").wstring(),
  };

  REQUIRE_NOTHROW(producer->connect(producerReports, HLA_EVOKED));
  REQUIRE_NOTHROW(consumer->connect(consumerReports, HLA_EVOKED));
  REQUIRE_NOTHROW(producer->createFederationExecution(
      federationName,
      fomModules,
      standard_hla::mom::integer64_time));
  REQUIRE_NOTHROW(producer->joinFederationExecution(
      L"timestamped-transport-producer",
      L"producer",
      federationName));
  REQUIRE_NOTHROW(consumer->joinFederationExecution(
      L"timestamped-transport-consumer",
      L"consumer",
      federationName));

  auto const custom = producer->getTransportationTypeHandle(
      fixture_hla::fixture::umbra_transportation_fixture);
  REQUIRE(custom.isValid());

  auto const interaction = producer->getInteractionClassHandle(
      fixture_hla::fom::transportation_fixture_interaction);
  auto const consumerInteraction = consumer->getInteractionClassHandle(
      fixture_hla::fom::transportation_fixture_interaction);
  REQUIRE(interaction.isValid());
  REQUIRE(consumerInteraction.isValid());
  REQUIRE_NOTHROW(producer->publishInteractionClass(interaction));
  REQUIRE_NOTHROW(consumer->subscribeInteractionClass(consumerInteraction));
  REQUIRE_NOTHROW(producer->changeInteractionOrderType(interaction, TIMESTAMP));

  auto const objectClass = producer->getObjectClassHandle(
      fixture_hla::fom::transportation_fixture_object);
  auto const consumerObjectClass = consumer->getObjectClassHandle(
      fixture_hla::fom::transportation_fixture_object);
  auto const attribute = producer->getAttributeHandle(
      objectClass,
      fixture_hla::fixture::umbra_transportation_fixture_attribute);
  auto const consumerAttribute = consumer->getAttributeHandle(
      consumerObjectClass,
      fixture_hla::fixture::umbra_transportation_fixture_attribute);
  REQUIRE(objectClass.isValid());
  REQUIRE(consumerObjectClass.isValid());
  REQUIRE(attribute.isValid());
  REQUIRE(consumerAttribute.isValid());
  AttributeHandleSet const attributes{attribute};
  REQUIRE_NOTHROW(producer->publishObjectClassAttributes(objectClass, attributes));
  REQUIRE_NOTHROW(consumer->subscribeObjectClassAttributes(
      consumerObjectClass,
      AttributeHandleSet{consumerAttribute}));
  REQUIRE_NOTHROW(producer->changeDefaultAttributeOrderType(
      objectClass,
      attributes,
      TIMESTAMP));
  ObjectInstanceHandle objectInstance;
  REQUIRE_NOTHROW(objectInstance = producer->registerObjectInstance(objectClass));
  while (consumer->evokeCallback(0.0)) {
  }
  REQUIRE(consumerReports.objectDiscoveryReports.size() == 1U);
  REQUIRE(consumerReports.objectDiscoveryReports.front() == objectInstance);

  REQUIRE_NOTHROW(consumer->enableTimeConstrained());
  static_cast<void>(consumer->evokeCallback(0.0));
  REQUIRE(consumerReports.timeConstrainedEnabledReports.size() == 1U);
  REQUIRE_NOTHROW(producer->enableTimeRegulation(
      rti1516_2025::HLAinteger64Interval(1)));
  while (producerReports.timeRegulationEnabledReports.empty() &&
         producer->evokeCallback(0.0)) {
  }
  REQUIRE(producerReports.timeRegulationEnabledReports.size() == 1U);

  auto const timestamp = rti1516_2025::HLAinteger64Time(2);
  auto const interactionRetraction = producer->sendInteraction(
      interaction,
      ParameterHandleValueMap{},
      VariableLengthData{},
      timestamp);
  REQUIRE(interactionRetraction.isValid());
  rti1516_2025::HLAunicodeString value{L"timestamped-custom-transportation-value"};
  AttributeHandleValueMap values{{attribute, value.encode()}};
  auto const attributeRetraction = producer->updateAttributeValues(
      objectInstance,
      values,
      VariableLengthData{},
      timestamp);
  REQUIRE(attributeRetraction.isValid());

  REQUIRE_NOTHROW(consumer->timeAdvanceRequest(timestamp));
  REQUIRE_NOTHROW(producer->timeAdvanceRequest(timestamp));
  while (producer->evokeCallback(0.0)) {
  }
  while (consumer->evokeCallback(0.0)) {
  }

  REQUIRE(consumerReports.timestampedInteractionReports.size() == 1U);
  auto const& interactionReport = consumerReports.timestampedInteractionReports.front();
  REQUIRE(interactionReport.interactionClass == consumerInteraction);
  REQUIRE(interactionReport.transportationType == custom);
  REQUIRE(interactionReport.timeValue == L"2");
  REQUIRE(interactionReport.sentOrderType == TIMESTAMP);
  REQUIRE(interactionReport.receivedOrderType == TIMESTAMP);

  REQUIRE(consumerReports.attributeReflectionReports.size() == 1U);
  auto const& reflection = consumerReports.attributeReflectionReports.front();
  REQUIRE(reflection.objectInstance == objectInstance);
  REQUIRE(reflection.transportationType == custom);
  REQUIRE(reflection.timeValue == L"2");
  REQUIRE(reflection.sentOrderType == TIMESTAMP);
  REQUIRE(reflection.receivedOrderType == TIMESTAMP);
  REQUIRE(reflection.attributeValues.size() == 1U);
  REQUIRE(reflection.attributeValues.contains(consumerAttribute));

  REQUIRE_NOTHROW(consumer->resignFederationExecution(rti1516_2025::NO_ACTION));
  REQUIRE_NOTHROW(producer->resignFederationExecution(
      rti1516_2025::CANCEL_THEN_DELETE_THEN_DIVEST));
  REQUIRE_NOTHROW(producer->destroyFederationExecution(federationName));
  REQUIRE_NOTHROW(consumer->disconnect());
  REQUIRE_NOTHROW(producer->disconnect());
}
