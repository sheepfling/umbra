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
#error "The custom transportation timestamped-directed test requires the Umbra source directory."
#endif

namespace {

namespace standard_hla = umbra::detail::hla::wide;
namespace fixture_hla = umbra::test::hla::wide;

using rti1516_2025::AttributeHandle;
using rti1516_2025::AttributeHandleSet;
using rti1516_2025::FederateHandle;
using rti1516_2025::HLA_EVOKED;
using rti1516_2025::InteractionClassHandle;
using rti1516_2025::InteractionClassHandleSet;
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
  return L"custom-transportation-timestamped-directed-delivery-" +
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
  struct DirectedInteractionReport final {
    InteractionClassHandle interactionClass;
    ObjectInstanceHandle objectInstance;
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

  void receiveDirectedInteraction(
      InteractionClassHandle const& interactionClass,
      ObjectInstanceHandle const& objectInstance,
      ParameterHandleValueMap const& parameterValues,
      VariableLengthData const& userSuppliedTag,
      TransportationTypeHandle const& transportationType,
      FederateHandle const& producingFederate,
      LogicalTime const& time,
      OrderType sentOrderType,
      OrderType receivedOrderType,
      MessageRetractionHandle const* optionalRetraction) override {
    directedInteractionReports.push_back({
        interactionClass,
        objectInstance,
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
    callbackOrder.push_back("directed");
  }

  void timeAdvanceGrant(LogicalTime const& time) override {
    timeAdvanceGrantReports.push_back({time.implementationName(), time.toString()});
    callbackOrder.push_back("grant");
  }

  std::vector<ObjectInstanceHandle> objectDiscoveryReports;
  std::vector<DirectedInteractionReport> directedInteractionReports;
  std::vector<TimeAdvanceGrantReport> timeAdvanceGrantReports;
  std::vector<std::string> callbackOrder;
};

std::unique_ptr<RTIambassador> makeRti() {
  RTIambassadorFactory factory;
  return factory.createRTIambassador();
}

}  // namespace

TEST_CASE(
    "Embedded timestamped directed delivery accepts a declared custom FOM transportation",
    "[integration][development-profile][federation-management]"
    "[custom-transportation][custom-transportation-timestamped-directed-delivery]"
    "[fom][transportation-type-lookup][transportation][directed][time-management][tso]"
    "[timestamped-directed-interaction][interaction-management]"
    "[rti.service.get-transportation-type-handle]"
    "[rti.service.get-object-class-handle][rti.service.get-attribute-handle]"
    "[rti.service.get-interaction-class-handle]"
    "[rti.service.publish-object-class-attributes]"
    "[rti.service.subscribe-object-class-attributes]"
    "[rti.service.publish-object-class-directed-interactions]"
    "[rti.service.subscribe-object-class-directed-interactions]"
    "[rti.service.change-interaction-order-type]"
    "[rti.service.register-object-instance]"
    "[rti.service.enable-time-constrained][rti.service.enable-time-regulation]"
    "[rti.service.send-directed-interaction][rti.service.time-advance-request]"
    "[rti.service.evoke-callback]"
    "[federate.callback.discover-object-instance]"
    "[federate.callback.receive-directed-interaction][federate.callback.time-advance-grant]") {
  ReportingFederateAmbassador producerReports;
  ReportingFederateAmbassador consumerReports;
  auto producer = makeRti();
  auto consumer = makeRti();
  auto const federationName = nextFederationName();
  std::vector<std::wstring> const fomModules{
      resourcePath("directed-interaction-object-consumer-fom.xml").wstring(),
      resourcePath("transportation-directed-interaction-provider-fom.xml").wstring(),
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
      L"custom-timestamped-directed-producer",
      L"producer",
      federationName));
  REQUIRE_NOTHROW(consumer->joinFederationExecution(
      L"custom-timestamped-directed-consumer",
      L"consumer",
      federationName));

  auto const custom = producer->getTransportationTypeHandle(
      fixture_hla::fixture::umbra_transportation_fixture);
  auto const objectClass = producer->getObjectClassHandle(
      fixture_hla::fom::directed_fixture_object);
  auto const marker = producer->getAttributeHandle(
      objectClass,
      fixture_hla::fixture::directed_target_marker);
  auto const interactionClass = producer->getInteractionClassHandle(
      fixture_hla::fom::directed_fixture_interaction);
  REQUIRE(custom.isValid());
  REQUIRE(objectClass.isValid());
  REQUIRE(marker.isValid());
  REQUIRE(interactionClass.isValid());
  InteractionClassHandleSet directedClasses{interactionClass};
  REQUIRE_NOTHROW(producer->publishObjectClassAttributes(objectClass, {marker}));
  REQUIRE_NOTHROW(consumer->subscribeObjectClassAttributes(objectClass, {marker}));
  REQUIRE_NOTHROW(producer->publishObjectClassDirectedInteractions(
      objectClass,
      directedClasses));
  REQUIRE_NOTHROW(producer->changeInteractionOrderType(interactionClass, TIMESTAMP));
  REQUIRE_NOTHROW(consumer->subscribeObjectClassDirectedInteractions(
      objectClass,
      directedClasses,
      true));

  ObjectInstanceHandle target;
  REQUIRE_NOTHROW(target = producer->registerObjectInstance(objectClass));
  REQUIRE(target.isValid());
  while (consumer->evokeCallback(0.0)) {
  }
  REQUIRE(consumerReports.objectDiscoveryReports.size() == 1U);
  REQUIRE_NOTHROW(consumer->enableTimeConstrained());
  while (consumer->evokeCallback(0.0)) {
  }
  REQUIRE_NOTHROW(producer->enableTimeRegulation(
      rti1516_2025::HLAinteger64Interval(1)));
  while (producer->evokeCallback(0.0)) {
  }

  unsigned char const tagBytes[] = {0x54, 0x53, 0x4F, 0x2D, 0x44};
  VariableLengthData const tag(tagBytes, sizeof(tagBytes));
  auto const retraction = producer->sendDirectedInteraction(
      interactionClass,
      target,
      ParameterHandleValueMap{},
      tag,
      rti1516_2025::HLAinteger64Time(2));
  REQUIRE(retraction.isValid());
  REQUIRE(consumerReports.directedInteractionReports.empty());
  REQUIRE_NOTHROW(consumer->timeAdvanceRequest(
      rti1516_2025::HLAinteger64Time(2)));
  REQUIRE_NOTHROW(producer->timeAdvanceRequest(
      rti1516_2025::HLAinteger64Time(2)));
  while (producer->evokeCallback(0.0)) {
  }
  while (consumer->evokeCallback(0.0)) {
  }

  REQUIRE(consumerReports.directedInteractionReports.size() == 1U);
  auto const& report = consumerReports.directedInteractionReports.front();
  REQUIRE(report.interactionClass == interactionClass);
  REQUIRE(report.objectInstance == target);
  REQUIRE(report.transportationType == custom);
  REQUIRE(report.producingFederate == producerHandle);
  REQUIRE(report.timeImplementationName == standard_hla::mom::integer64_time);
  REQUIRE(report.timeValue == L"2");
  REQUIRE(report.sentOrderType == TIMESTAMP);
  REQUIRE(report.receivedOrderType == TIMESTAMP);
  REQUIRE(report.retractionSupplied);
  REQUIRE(report.retractionValid);
  REQUIRE(variableLengthDataBytes(report.userSuppliedTag) ==
          std::vector<unsigned char>(tagBytes, tagBytes + sizeof(tagBytes)));

  REQUIRE_NOTHROW(consumer->unsubscribeObjectClassDirectedInteractions(
      objectClass,
      directedClasses));
  REQUIRE_NOTHROW(consumer->unsubscribeObjectClassAttributes(objectClass, {marker}));
  REQUIRE_NOTHROW(consumer->resignFederationExecution(rti1516_2025::NO_ACTION));
  REQUIRE_NOTHROW(producer->resignFederationExecution(
      rti1516_2025::CANCEL_THEN_DELETE_THEN_DIVEST));
  REQUIRE_NOTHROW(producer->destroyFederationExecution(federationName));
  REQUIRE_NOTHROW(consumer->disconnect());
  REQUIRE_NOTHROW(producer->disconnect());
}
