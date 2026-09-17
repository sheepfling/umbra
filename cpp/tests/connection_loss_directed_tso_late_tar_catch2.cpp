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
#error "The directed late-TAR connection-loss tests require the Umbra source directory."
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
  std::vector<DirectedInteractionReport> directedInteractionReports;
  std::vector<ObjectRemovalReport> objectRemovalReports;
  std::vector<std::string> callbackOrder;
  std::size_t discoveryCount = 0U;
};

std::unique_ptr<RTIambassador> makeRti() {
  RTIambassadorFactory factory;
  return factory.createRTIambassador();
}

}  // namespace

TEST_CASE(
    "Embedded transport loss protects a late cutoff directed interaction from automatic object cleanup",
    "[integration][development-profile][federation-management][transport]"
    "[interaction-management][directed][time-management]"
    "[timestamped-directed-interaction][connection-lost-tso-cutoff][late-tar]"
    "[rti.service.connection-lost][rti.service.send-directed-interaction]"
    "[rti.service.time-advance-request][rti.service.set-automatic-resign-directive]"
    "[federate.callback.connection-lost][federate.callback.receive-directed-interaction]"
    "[federate.callback.remove-object-instance][federate.callback.time-advance-grant]") {
  ReportingFederateAmbassador lostReports;
  ReportingFederateAmbassador survivingReports;
  auto lost = makeRti();
  auto surviving = makeRti();
  auto const federationName = std::wstring{L"connection-loss-directed-tso-late-tar"};
  auto const objectConsumer = (std::filesystem::path(UMBRA_SOURCE_DIRECTORY) /
                               "cpp" / "tests" / "data" /
                               "directed-interaction-object-consumer-fom.xml")
                                  .wstring();
  auto const interactionProvider = (std::filesystem::path(UMBRA_SOURCE_DIRECTORY) /
                                    "cpp" / "tests" / "data" /
                                    "directed-interaction-interaction-provider-fom.xml")
                                       .wstring();
  unsigned char const tagBytes[] = {0x4C, 0x41, 0x54, 0x45, 0x2D, 0x44, 0x49};
  VariableLengthData const tag(tagBytes, sizeof(tagBytes));
  auto const drain = [](RTIambassador& ambassador) {
    while (ambassador.evokeCallback(0.0)) {
    }
  };

  REQUIRE_NOTHROW(lost->connect(lostReports, HLA_EVOKED));
  REQUIRE_NOTHROW(surviving->connect(survivingReports, HLA_EVOKED));
  REQUIRE_NOTHROW(lost->createFederationExecution(
      federationName,
      std::vector<std::wstring>{objectConsumer, interactionProvider},
      standard_hla::mom::integer64_time));
  FederateHandle lostFederate;
  REQUIRE_NOTHROW(lostFederate = lost->joinFederationExecution(
      L"late-cutoff-lost-directed-publisher", L"publisher", federationName));
  REQUIRE_NOTHROW(surviving->joinFederationExecution(
      L"late-cutoff-surviving-directed-subscriber", L"subscriber", federationName));

  auto const objectClass = lost->getObjectClassHandle(
      fixture_hla::fom::directed_fixture_object);
  auto const marker = lost->getAttributeHandle(
      objectClass,
      fixture_hla::fixture::directed_target_marker);
  auto const interactionClass = lost->getInteractionClassHandle(
      fixture_hla::fom::directed_fixture_interaction);
  auto const reliableTransport = lost->getTransportationTypeHandle(
      standard_hla::mom::reliable);
  InteractionClassHandleSet const directedClasses{interactionClass};
  REQUIRE(objectClass.isValid());
  REQUIRE(marker.isValid());
  REQUIRE(interactionClass.isValid());
  REQUIRE_NOTHROW(lost->publishObjectClassAttributes(objectClass, {marker}));
  REQUIRE_NOTHROW(surviving->subscribeObjectClassAttributes(objectClass, {marker}));
  REQUIRE_NOTHROW(lost->publishObjectClassDirectedInteractions(
      objectClass,
      directedClasses));
  REQUIRE_NOTHROW(lost->changeInteractionOrderType(interactionClass, TIMESTAMP));
  REQUIRE_NOTHROW(surviving->subscribeObjectClassDirectedInteractions(
      objectClass,
      directedClasses,
      true));
  REQUIRE_NOTHROW(lost->setAutomaticResignDirective(
      rti1516_2025::DELETE_OBJECTS));

  ObjectInstanceHandle target;
  REQUIRE_NOTHROW(target = lost->registerObjectInstance(objectClass));
  REQUIRE(target.isValid());
  auto const targetName = lost->getObjectInstanceName(target);
  drain(*surviving);
  REQUIRE(survivingReports.discoveryCount == 1U);
  drain(*lost);
  REQUIRE_NOTHROW(surviving->enableTimeConstrained());
  drain(*surviving);
  REQUIRE(survivingReports.timeConstrainedEnabledReports.size() == 1U);
  REQUIRE_NOTHROW(lost->enableTimeRegulation(
      rti1516_2025::HLAinteger64Interval(5)));
  drain(*lost);
  REQUIRE(lostReports.timeRegulationEnabledReports.size() == 1U);

  auto const retraction = lost->sendDirectedInteraction(
      interactionClass,
      target,
      ParameterHandleValueMap{},
      tag,
      rti1516_2025::HLAinteger64Time(6));
  REQUIRE(retraction.isValid());
  REQUIRE(survivingReports.directedInteractionReports.empty());

  REQUIRE_NOTHROW(lost->timeAdvanceRequest(
      rti1516_2025::HLAinteger64Time(6)));
  drain(*lost);
  REQUIRE(lostReports.timeAdvanceGrantReports.size() == 1U);
  REQUIRE(lostReports.timeAdvanceGrantReports.front().value == L"6");
  REQUIRE_NOTHROW(umbra::detail::failEmbeddedTransportConnectionForTesting(
      *lost,
      L"late timestamped directed cutoff transport fault"));
  REQUIRE(survivingReports.directedInteractionReports.empty());
  REQUIRE(survivingReports.objectRemovalReports.empty());

  // The survivor's first TAR is submitted after loss.  The accepted directed
  // payload must precede its grant, while automatic DELETE_OBJECTS cleanup is
  // held until the next receive-order TAR.
  REQUIRE_NOTHROW(surviving->timeAdvanceRequest(
      rti1516_2025::HLAinteger64Time(6)));
  drain(*surviving);
  REQUIRE(survivingReports.directedInteractionReports.size() == 1U);
  REQUIRE(survivingReports.objectRemovalReports.empty());
  REQUIRE(survivingReports.timeAdvanceGrantReports.size() == 1U);
  REQUIRE(survivingReports.callbackOrder ==
          std::vector<std::string>{"directed", "grant"});
  auto const& report = survivingReports.directedInteractionReports.front();
  REQUIRE(report.interactionClass == interactionClass);
  REQUIRE(report.objectInstance == target);
  REQUIRE(report.parameterValues.empty());
  REQUIRE(variableLengthDataBytes(report.userSuppliedTag) ==
          variableLengthDataBytes(tag));
  REQUIRE(report.transportationType == reliableTransport);
  REQUIRE(report.producingFederate == lostFederate);
  REQUIRE(report.timeImplementationName == standard_hla::mom::integer64_time);
  REQUIRE(report.timeValue == L"6");
  REQUIRE(report.sentOrderType == TIMESTAMP);
  REQUIRE(report.receivedOrderType == TIMESTAMP);
  REQUIRE(report.retractionSupplied);
  REQUIRE(report.retractionValid);
  REQUIRE(survivingReports.timeAdvanceGrantReports.front().implementationName ==
          standard_hla::mom::integer64_time);
  REQUIRE(survivingReports.timeAdvanceGrantReports.front().value == L"6");
  REQUIRE(surviving->getObjectInstanceHandle(targetName) == target);

  REQUIRE_NOTHROW(surviving->timeAdvanceRequest(
      rti1516_2025::HLAinteger64Time(7)));
  drain(*surviving);
  REQUIRE(survivingReports.objectRemovalReports.size() == 1U);
  auto const& removal = survivingReports.objectRemovalReports.front();
  REQUIRE(removal.objectInstance == target);
  REQUIRE(variableLengthDataBytes(removal.userSuppliedTag).empty());
  REQUIRE(removal.producingFederate == lostFederate);
  REQUIRE(removal.sentOrderType == RECEIVE);
  REQUIRE(removal.receivedOrderType == RECEIVE);
  REQUIRE_THROWS_AS(
      surviving->getObjectInstanceHandle(targetName),
      rti1516_2025::ObjectInstanceNotKnown);

  drain(*lost);
  REQUIRE(lostReports.faultDescriptions == std::vector<std::wstring>{
      L"late timestamped directed cutoff transport fault"});
  REQUIRE_THROWS_AS(lost->disconnect(), rti1516_2025::NotConnected);
  REQUIRE_NOTHROW(lost->connect(lostReports, HLA_EVOKED));
  REQUIRE_NOTHROW(lost->disconnect());
  REQUIRE_NOTHROW(surviving->resignFederationExecution(rti1516_2025::NO_ACTION));
  REQUIRE_NOTHROW(surviving->destroyFederationExecution(federationName));
  REQUIRE_NOTHROW(surviving->disconnect());
}
