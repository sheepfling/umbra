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
#error "The TAR/NMR directed-interaction tests require the Umbra source directory."
#endif

namespace {

namespace standard_hla = umbra::detail::hla::wide;
namespace fixture_hla = umbra::test::hla::wide;

using rti1516_2025::FederateHandle;
using rti1516_2025::HLA_EVOKED;
using rti1516_2025::InteractionClassHandle;
using rti1516_2025::InteractionClassHandleSet;
using rti1516_2025::ObjectClassHandle;
using rti1516_2025::ObjectInstanceHandle;
using rti1516_2025::OrderType;
using rti1516_2025::ParameterHandleValueMap;
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
  return L"timestamped-directed-tar-nmr-" +
      std::to_wstring(sequence.fetch_add(1U, std::memory_order_relaxed));
}

std::filesystem::path resourcePath(std::filesystem::path const& relativePath) {
  return std::filesystem::path(UMBRA_SOURCE_DIRECTORY) / "cpp" / "tests" /
      "data" / relativePath;
}

class ReportingFederateAmbassador final
    : public rti1516_2025::NullFederateAmbassador {
 public:
  struct TimeAdvanceGrantReport final {
    std::wstring implementationName;
    std::wstring value;
  };

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

  struct ObjectDiscoveryReport final {
    ObjectInstanceHandle objectInstance;
    ObjectClassHandle objectClass;
    std::wstring objectInstanceName;
    FederateHandle producingFederate;
  };

  void timeAdvanceGrant(rti1516_2025::LogicalTime const& time) override {
    timeAdvanceGrantReports.push_back({time.implementationName(), time.toString()});
    callbackOrder.push_back("grant");
  }

  void receiveDirectedInteraction(
      InteractionClassHandle const& interactionClass,
      ObjectInstanceHandle const& objectInstance,
      ParameterHandleValueMap const& parameterValues,
      VariableLengthData const& userSuppliedTag,
      TransportationTypeHandle const& transportationType,
      FederateHandle const& producingFederate,
      rti1516_2025::LogicalTime const& time,
      OrderType sentOrderType,
      OrderType receivedOrderType,
      rti1516_2025::MessageRetractionHandle const* optionalRetraction) override {
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

  void discoverObjectInstance(
      ObjectInstanceHandle const& objectInstance,
      ObjectClassHandle const& objectClass,
      std::wstring const& objectInstanceName,
      FederateHandle const& producingFederate) override {
    objectDiscoveryReports.push_back({
        objectInstance,
        objectClass,
        objectInstanceName,
        producingFederate,
    });
  }

  std::vector<TimeAdvanceGrantReport> timeAdvanceGrantReports;
  std::vector<DirectedInteractionReport> directedInteractionReports;
  std::vector<ObjectDiscoveryReport> objectDiscoveryReports;
  std::vector<std::string> callbackOrder;
};

std::unique_ptr<RTIambassador> makeRti() {
  RTIambassadorFactory factory;
  return factory.createRTIambassador();
}

}  // namespace

TEST_CASE(
    "Embedded timestamped directed interaction delivers before TAR and NMR grants",
    "[integration][development-profile][interaction-management][directed][time-management]"
    "[timestamped-directed-interaction][tso][time-advance-request]"
    "[next-message-request][rti.service.send-directed-interaction]"
    "[rti.service.retract][rti.service.time-advance-request]"
    "[rti.service.next-message-request]"
    "[federate.callback.receive-directed-interaction]"
    "[federate.callback.time-advance-grant]") {
  ReportingFederateAmbassador publisherReports;
  ReportingFederateAmbassador tarReports;
  ReportingFederateAmbassador nmrReports;
  auto publisher = makeRti();
  auto tar = makeRti();
  auto nmr = makeRti();
  auto const federationName = nextFederationName();
  auto const objectConsumer = resourcePath(
      "directed-interaction-object-consumer-fom.xml").wstring();
  auto const interactionProvider = resourcePath(
      "directed-interaction-interaction-provider-fom.xml").wstring();
  std::vector<unsigned char> const tagBytes{
      0x54U, 0x41U, 0x52U, 0x2DU, 0x4E, 0x4DU, 0x52U};
  VariableLengthData const tag(tagBytes.data(), tagBytes.size());
  InteractionClassHandleSet directedClasses;

  REQUIRE_NOTHROW(publisher->connect(publisherReports, HLA_EVOKED));
  REQUIRE_NOTHROW(tar->connect(tarReports, HLA_EVOKED));
  REQUIRE_NOTHROW(nmr->connect(nmrReports, HLA_EVOKED));
  REQUIRE_NOTHROW(publisher->createFederationExecution(
      federationName,
      std::vector<std::wstring>{objectConsumer, interactionProvider},
      standard_hla::mom::integer64_time));
  FederateHandle publisherHandle;
  REQUIRE_NOTHROW(publisherHandle = publisher->joinFederationExecution(
      L"timestamped-directed-tar-nmr-publisher", L"publisher", federationName));
  REQUIRE_NOTHROW(tar->joinFederationExecution(
      L"timestamped-directed-tar", L"subscriber", federationName));
  REQUIRE_NOTHROW(nmr->joinFederationExecution(
      L"timestamped-directed-nmr", L"subscriber", federationName));

  auto const objectClass = publisher->getObjectClassHandle(
      fixture_hla::fom::directed_fixture_object);
  auto const marker = publisher->getAttributeHandle(
      objectClass,
      fixture_hla::fixture::directed_target_marker);
  auto const interactionClass = publisher->getInteractionClassHandle(
      fixture_hla::fom::directed_fixture_interaction);
  REQUIRE(objectClass.isValid());
  REQUIRE(marker.isValid());
  REQUIRE(interactionClass.isValid());
  directedClasses.insert(interactionClass);

  REQUIRE_NOTHROW(publisher->publishObjectClassAttributes(objectClass, {marker}));
  for (auto* receiver : {tar.get(), nmr.get()}) {
    REQUIRE_NOTHROW(receiver->subscribeObjectClassAttributes(objectClass, {marker}));
  }
  REQUIRE_NOTHROW(publisher->publishObjectClassDirectedInteractions(
      objectClass,
      directedClasses));
  REQUIRE_NOTHROW(publisher->changeInteractionOrderType(interactionClass, TIMESTAMP));
  for (auto* receiver : {tar.get(), nmr.get()}) {
    REQUIRE_NOTHROW(receiver->subscribeObjectClassDirectedInteractions(
        objectClass,
        directedClasses,
        true));
  }

  ObjectInstanceHandle target;
  REQUIRE_NOTHROW(target = publisher->registerObjectInstance(objectClass));
  REQUIRE(target.isValid());
  while (tar->evokeCallback(0.0)) {
  }
  while (nmr->evokeCallback(0.0)) {
  }
  REQUIRE(tarReports.objectDiscoveryReports.size() == 1U);
  REQUIRE(nmrReports.objectDiscoveryReports.size() == 1U);

  REQUIRE_NOTHROW(tar->enableTimeConstrained());
  REQUIRE_NOTHROW(nmr->enableTimeConstrained());
  REQUIRE_FALSE(tar->evokeCallback(0.0));
  REQUIRE_FALSE(nmr->evokeCallback(0.0));
  REQUIRE_NOTHROW(publisher->enableTimeRegulation(
      rti1516_2025::HLAinteger64Interval(5)));
  REQUIRE(publisher->evokeCallback(0.0));
  REQUIRE_FALSE(publisher->evokeCallback(0.0));

  auto const retraction = publisher->sendDirectedInteraction(
      interactionClass,
      target,
      ParameterHandleValueMap{},
      tag,
      rti1516_2025::HLAinteger64Time(7));
  REQUIRE(retraction.isValid());
  REQUIRE(tarReports.directedInteractionReports.empty());
  REQUIRE(nmrReports.directedInteractionReports.empty());

  REQUIRE_NOTHROW(tar->timeAdvanceRequest(
      rti1516_2025::HLAinteger64Time(7)));
  REQUIRE_NOTHROW(nmr->nextMessageRequest(
      rti1516_2025::HLAinteger64Time(10)));
  REQUIRE_NOTHROW(publisher->timeAdvanceRequest(
      rti1516_2025::HLAinteger64Time(2)));
  REQUIRE_FALSE(publisher->evokeCallback(0.0));

  auto requireDirected = [&](auto const& report) {
    REQUIRE(report.interactionClass == interactionClass);
    REQUIRE(report.objectInstance == target);
    REQUIRE(report.parameterValues.empty());
    REQUIRE(variableLengthDataBytes(report.userSuppliedTag) == tagBytes);
    REQUIRE(report.transportationType ==
            publisher->getTransportationTypeHandle(L"HLAreliable"));
    REQUIRE(report.producingFederate == publisherHandle);
    REQUIRE(report.timeImplementationName == L"HLAinteger64Time");
    REQUIRE(report.timeValue == L"7");
    REQUIRE(report.sentOrderType == TIMESTAMP);
    REQUIRE(report.receivedOrderType == TIMESTAMP);
    REQUIRE(report.retractionSupplied);
    REQUIRE(report.retractionValid);
  };

  REQUIRE_FALSE(tar->evokeCallback(0.0));
  REQUIRE_FALSE(nmr->evokeCallback(0.0));
  REQUIRE(publisherReports.timeAdvanceGrantReports.size() == 1U);
  REQUIRE(publisherReports.timeAdvanceGrantReports.front().value == L"2");
  REQUIRE(tarReports.directedInteractionReports.size() == 1U);
  REQUIRE(tarReports.timeAdvanceGrantReports.size() == 1U);
  REQUIRE(tarReports.timeAdvanceGrantReports.front().value == L"7");
  REQUIRE(tarReports.callbackOrder ==
          std::vector<std::string>{"directed", "grant"});
  REQUIRE(nmrReports.directedInteractionReports.size() == 1U);
  REQUIRE(nmrReports.timeAdvanceGrantReports.size() == 1U);
  REQUIRE(nmrReports.timeAdvanceGrantReports.front().value == L"7");
  REQUIRE(nmrReports.callbackOrder ==
          std::vector<std::string>{"directed", "grant"});
  requireDirected(tarReports.directedInteractionReports.front());
  requireDirected(nmrReports.directedInteractionReports.front());
  REQUIRE_THROWS_AS(
      publisher->retract(retraction),
      rti1516_2025::MessageCanNoLongerBeRetracted);

  REQUIRE_NOTHROW(tar->resignFederationExecution(rti1516_2025::NO_ACTION));
  REQUIRE_NOTHROW(nmr->resignFederationExecution(rti1516_2025::NO_ACTION));
  REQUIRE_NOTHROW(publisher->resignFederationExecution(
      rti1516_2025::CANCEL_THEN_DELETE_THEN_DIVEST));
  REQUIRE_NOTHROW(publisher->destroyFederationExecution(federationName));
  REQUIRE_NOTHROW(tar->disconnect());
  REQUIRE_NOTHROW(nmr->disconnect());
  REQUIRE_NOTHROW(publisher->disconnect());
}
