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
#error "The directed-selector connection-loss tests require the Umbra source directory."
#endif

namespace {

namespace standard_hla = umbra::detail::hla::wide;
namespace fixture_hla = umbra::test::hla::wide;

using rti1516_2025::AttributeHandle;
using rti1516_2025::FederateHandle;
using rti1516_2025::HLA_EVOKED;
using rti1516_2025::InteractionClassHandle;
using rti1516_2025::InteractionClassHandleSet;
using rti1516_2025::LogicalTime;
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
  struct TimeReport final {
    std::wstring implementationName;
    std::wstring value;
  };

  struct DirectedReport final {
    InteractionClassHandle interactionClass;
    ObjectInstanceHandle objectInstance;
    ParameterHandleValueMap parameterValues;
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
    std::wstring timeImplementationName;
    std::wstring timeValue;
    OrderType sentOrderType = RECEIVE;
    OrderType receivedOrderType = RECEIVE;
    bool retractionSupplied = false;
    bool retractionValid = false;
  };

  void connectionLost(std::wstring const& description) override {
    faultDescriptions.push_back(description);
  }

  void timeAdvanceGrant(LogicalTime const& time) override {
    grants.push_back({time.implementationName(), time.toString()});
    callbackOrder.push_back("grant");
  }

  void timeRegulationEnabled(LogicalTime const& time) override {
    regulationTimes.push_back({time.implementationName(), time.toString()});
  }

  void timeConstrainedEnabled(LogicalTime const& time) override {
    constrainedTimes.push_back({time.implementationName(), time.toString()});
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
      VariableLengthData const& tag,
      TransportationTypeHandle const& transportationType,
      FederateHandle const& producer,
      LogicalTime const& time,
      OrderType sentOrderType,
      OrderType receivedOrderType,
      rti1516_2025::MessageRetractionHandle const* optionalRetraction) override {
    directed.push_back({
        interactionClass,
        objectInstance,
        parameterValues,
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
    callbackOrder.push_back("directed");
  }

  void removeObjectInstance(
      ObjectInstanceHandle const& objectInstance,
      VariableLengthData const& tag,
      FederateHandle const& producer) override {
    removals.push_back({
        objectInstance,
        tag,
        producer,
        L"",
        L"",
        RECEIVE,
        RECEIVE,
        false,
        false,
    });
    callbackOrder.push_back("remove");
  }

  void removeObjectInstance(
      ObjectInstanceHandle const& objectInstance,
      VariableLengthData const& tag,
      FederateHandle const& producer,
      LogicalTime const& time,
      OrderType sentOrderType,
      OrderType receivedOrderType,
      rti1516_2025::MessageRetractionHandle const* optionalRetraction) override {
    removals.push_back({
        objectInstance,
        tag,
        producer,
        time.implementationName(),
        time.toString(),
        sentOrderType,
        receivedOrderType,
        optionalRetraction != nullptr,
        optionalRetraction != nullptr && optionalRetraction->isValid(),
    });
    callbackOrder.push_back("remove");
  }

  std::vector<std::wstring> faultDescriptions;
  std::vector<TimeReport> grants;
  std::vector<TimeReport> regulationTimes;
  std::vector<TimeReport> constrainedTimes;
  std::vector<DirectedReport> directed;
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
    "Embedded transport loss rechecks a directed ownership selector before automatic cleanup",
    "[integration][development-profile][federation-management][transport]"
    "[interaction-management][directed][ownership-management][time-management]"
    "[timestamped-directed-interaction][connection-lost-tso-cutoff]"
    "[connection-lost-directed-selector-mutation][automatic-resign-delete]"
    "[rti.service.connection-lost][rti.service.send-directed-interaction]"
    "[rti.service.subscribe-object-class-directed-interactions]"
    "[rti.service.time-advance-request][rti.service.set-automatic-resign-directive]"
    "[federate.callback.connection-lost][federate.callback.receive-directed-interaction]"
    "[federate.callback.remove-object-instance][federate.callback.time-advance-grant]") {
  ReportingFederateAmbassador lostReports;
  ReportingFederateAmbassador survivingReports;
  auto lost = makeRti();
  auto surviving = makeRti();
  auto const federationName =
      std::wstring{L"connection-loss-directed-selector-mutation"};
  auto const objectConsumer = (std::filesystem::path(UMBRA_SOURCE_DIRECTORY) /
                               "cpp" / "tests" / "data" /
                               "directed-interaction-object-consumer-fom.xml")
                                  .wstring();
  auto const interactionProvider = (std::filesystem::path(UMBRA_SOURCE_DIRECTORY) /
                                    "cpp" / "tests" / "data" /
                                    "directed-interaction-interaction-provider-fom.xml")
                                       .wstring();
  unsigned char const tagBytes[] = {0x44, 0x53, 0x2D, 0x54, 0x53, 0x4F};
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
      L"directed-selector-lost-owner", L"owner", federationName));
  REQUIRE_NOTHROW(surviving->joinFederationExecution(
      L"directed-selector-surviving-subscriber", L"subscriber", federationName));

  auto const objectClass = lost->getObjectClassHandle(
      fixture_hla::fom::directed_fixture_object);
  auto const marker = lost->getAttributeHandle(
      objectClass,
      fixture_hla::fixture::directed_target_marker);
  auto const interactionClass = lost->getInteractionClassHandle(
      fixture_hla::fom::directed_fixture_interaction);
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

  ObjectInstanceHandle target;
  REQUIRE_NOTHROW(target = lost->registerObjectInstance(objectClass));
  REQUIRE(target.isValid());
  auto const targetName = lost->getObjectInstanceName(target);
  drain(*surviving);
  REQUIRE(survivingReports.discoveryCount == 1U);

  REQUIRE_NOTHROW(lost->setAutomaticResignDirective(rti1516_2025::DELETE_OBJECTS));
  REQUIRE(lost->getAutomaticResignDirective() == rti1516_2025::DELETE_OBJECTS);
  REQUIRE_NOTHROW(surviving->enableTimeConstrained());
  drain(*surviving);
  REQUIRE(survivingReports.constrainedTimes.size() == 1U);
  REQUIRE_NOTHROW(lost->enableTimeRegulation(
      rti1516_2025::HLAinteger64Interval(5)));
  drain(*lost);
  REQUIRE(lostReports.regulationTimes.size() == 1U);

  auto const retraction = lost->sendDirectedInteraction(
      interactionClass,
      target,
      ParameterHandleValueMap{},
      tag,
      rti1516_2025::HLAinteger64Time(6));
  REQUIRE(retraction.isValid());
  REQUIRE(survivingReports.directed.empty());
  REQUIRE_NOTHROW(surviving->timeAdvanceRequest(
      rti1516_2025::HLAinteger64Time(6)));
  REQUIRE_NOTHROW(lost->timeAdvanceRequest(
      rti1516_2025::HLAinteger64Time(6)));
  drain(*lost);
  REQUIRE(lostReports.grants.size() == 1U);
  REQUIRE(lostReports.grants.front().value == L"6");
  REQUIRE(survivingReports.grants.empty());

  std::wstring const faultDescription =
      L"directed ownership selector mutation cutoff transport fault";
  REQUIRE(umbra::detail::failEmbeddedTransportConnectionForTesting(
      *lost,
      faultDescription));
  REQUIRE(survivingReports.directed.empty());
  REQUIRE(survivingReports.removals.empty());

  // The universal selector was part of admission, but the callback-time
  // selector is now by-ownership.  The lost owner no longer satisfies it.
  REQUIRE_NOTHROW(surviving->subscribeObjectClassDirectedInteractions(
      objectClass,
      directedClasses,
      false));
  drain(*surviving);
  REQUIRE(survivingReports.directed.empty());
  REQUIRE(survivingReports.removals.empty());
  REQUIRE(survivingReports.grants.size() == 1U);
  REQUIRE(survivingReports.grants.front().value == L"6");

  // DELETE_OBJECTS cleanup is independent receive-order work and remains
  // deliverable at the following gate after the directed candidate is stale.
  survivingReports.callbackOrder.clear();
  REQUIRE_NOTHROW(surviving->timeAdvanceRequest(
      rti1516_2025::HLAinteger64Time(7)));
  drain(*surviving);
  REQUIRE(survivingReports.directed.empty());
  REQUIRE(survivingReports.removals.size() == 1U);
  // The source's forced departure removes the only regulator, so this
  // receive-order gate may not produce a second Time Advance Grant.  The
  // cleanup callback itself is the conformance signal for this slice.
  REQUIRE(survivingReports.grants.size() == 1U);
  REQUIRE(survivingReports.callbackOrder == std::vector<std::string>{"remove"});
  auto const& removal = survivingReports.removals.front();
  REQUIRE(removal.objectInstance == target);
  REQUIRE(removal.producer == lostFederate);
  REQUIRE(removal.timeImplementationName.empty());
  REQUIRE(removal.timeValue.empty());
  REQUIRE(removal.sentOrderType == RECEIVE);
  REQUIRE(removal.receivedOrderType == RECEIVE);
  REQUIRE_FALSE(removal.retractionSupplied);
  REQUIRE_FALSE(removal.retractionValid);
  REQUIRE_THROWS_AS(
      surviving->getObjectInstanceHandle(targetName),
      rti1516_2025::ObjectInstanceNotKnown);

  drain(*lost);
  REQUIRE(lostReports.faultDescriptions ==
          std::vector<std::wstring>{faultDescription});
  REQUIRE_THROWS_AS(lost->disconnect(), rti1516_2025::NotConnected);
  REQUIRE_NOTHROW(lost->connect(lostReports, HLA_EVOKED));
  REQUIRE_NOTHROW(lost->disconnect());
  REQUIRE_NOTHROW(surviving->resignFederationExecution(rti1516_2025::NO_ACTION));
  REQUIRE_NOTHROW(surviving->destroyFederationExecution(federationName));
  REQUIRE_NOTHROW(surviving->disconnect());
}

TEST_CASE(
    "Embedded transport loss releases automatic cleanup after a cutoff directed interaction is suppressed",
    "[integration][development-profile][federation-management][transport]"
    "[interaction-management][directed][time-management]"
    "[timestamped-directed-interaction][connection-lost-tso-cutoff]"
    "[connection-lost-directed-selector-mutation][automatic-resign-delete]"
    "[rti.service.connection-lost][rti.service.send-directed-interaction]"
    "[rti.service.unsubscribe-object-class-directed-interactions]"
    "[rti.service.time-advance-request][rti.service.set-automatic-resign-directive]"
    "[federate.callback.connection-lost][federate.callback.receive-directed-interaction]"
    "[federate.callback.remove-object-instance][federate.callback.time-advance-grant]") {
  ReportingFederateAmbassador lostReports;
  ReportingFederateAmbassador survivingReports;
  auto lost = makeRti();
  auto surviving = makeRti();
  auto const federationName =
      std::wstring{L"connection-loss-directed-selector-suppressed"};
  auto const objectConsumer = (std::filesystem::path(UMBRA_SOURCE_DIRECTORY) /
                               "cpp" / "tests" / "data" /
                               "directed-interaction-object-consumer-fom.xml")
                                  .wstring();
  auto const interactionProvider = (std::filesystem::path(UMBRA_SOURCE_DIRECTORY) /
                                    "cpp" / "tests" / "data" /
                                    "directed-interaction-interaction-provider-fom.xml")
                                       .wstring();
  unsigned char const tagBytes[] = {0x53, 0x55, 0x50, 0x50, 0x52, 0x45, 0x53, 0x53};
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
      L"suppressed-cutoff-lost-directed-publisher", L"publisher", federationName));
  REQUIRE_NOTHROW(surviving->joinFederationExecution(
      L"suppressed-cutoff-surviving-directed-subscriber", L"subscriber", federationName));

  auto const objectClass = lost->getObjectClassHandle(
      fixture_hla::fom::directed_fixture_object);
  auto const marker = lost->getAttributeHandle(
      objectClass,
      fixture_hla::fixture::directed_target_marker);
  auto const interactionClass = lost->getInteractionClassHandle(
      fixture_hla::fom::directed_fixture_interaction);
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
  REQUIRE_NOTHROW(lost->setAutomaticResignDirective(rti1516_2025::DELETE_OBJECTS));

  ObjectInstanceHandle target;
  REQUIRE_NOTHROW(target = lost->registerObjectInstance(objectClass));
  REQUIRE(target.isValid());
  auto const targetName = lost->getObjectInstanceName(target);
  drain(*surviving);
  REQUIRE(survivingReports.discoveryCount == 1U);
  REQUIRE_NOTHROW(surviving->enableTimeConstrained());
  drain(*surviving);
  REQUIRE(survivingReports.constrainedTimes.size() == 1U);
  REQUIRE_NOTHROW(lost->enableTimeRegulation(
      rti1516_2025::HLAinteger64Interval(5)));
  drain(*lost);
  REQUIRE(lostReports.regulationTimes.size() == 1U);

  auto const retraction = lost->sendDirectedInteraction(
      interactionClass,
      target,
      ParameterHandleValueMap{},
      tag,
      rti1516_2025::HLAinteger64Time(6));
  REQUIRE(retraction.isValid());
  REQUIRE(survivingReports.directed.empty());
  REQUIRE_NOTHROW(surviving->timeAdvanceRequest(
      rti1516_2025::HLAinteger64Time(6)));
  REQUIRE_NOTHROW(lost->timeAdvanceRequest(
      rti1516_2025::HLAinteger64Time(6)));
  drain(*lost);
  REQUIRE(lostReports.grants.size() == 1U);
  REQUIRE(lostReports.grants.front().value == L"6");
  REQUIRE(survivingReports.grants.empty());

  std::wstring const faultDescription =
      L"directed interaction suppression cutoff transport fault";
  REQUIRE(umbra::detail::failEmbeddedTransportConnectionForTesting(
      *lost,
      faultDescription));
  REQUIRE(survivingReports.directed.empty());
  REQUIRE(survivingReports.removals.empty());

  // Remove the universal selector before the pending timestamp-6 callback is
  // dispatched.  The accepted interaction must be suppressed, while the
  // independent receive-order DELETE_OBJECTS cleanup remains deliverable.
  REQUIRE_NOTHROW(surviving->unsubscribeObjectClassDirectedInteractions(
      objectClass,
      directedClasses));
  drain(*surviving);
  REQUIRE(survivingReports.directed.empty());
  REQUIRE(survivingReports.removals.empty());
  REQUIRE(survivingReports.grants.size() == 1U);
  REQUIRE(survivingReports.grants.front().value == L"6");

  survivingReports.callbackOrder.clear();
  REQUIRE_NOTHROW(surviving->timeAdvanceRequest(
      rti1516_2025::HLAinteger64Time(7)));
  drain(*surviving);
  REQUIRE(survivingReports.directed.empty());
  REQUIRE(survivingReports.removals.size() == 1U);
  REQUIRE(survivingReports.grants.size() == 1U);
  REQUIRE(survivingReports.callbackOrder == std::vector<std::string>{"remove"});
  auto const& removal = survivingReports.removals.front();
  REQUIRE(removal.objectInstance == target);
  REQUIRE(removal.producer == lostFederate);
  REQUIRE(removal.timeImplementationName.empty());
  REQUIRE(removal.timeValue.empty());
  REQUIRE(removal.sentOrderType == RECEIVE);
  REQUIRE(removal.receivedOrderType == RECEIVE);
  REQUIRE_FALSE(removal.retractionSupplied);
  REQUIRE_FALSE(removal.retractionValid);
  REQUIRE_THROWS_AS(
      surviving->getObjectInstanceHandle(targetName),
      rti1516_2025::ObjectInstanceNotKnown);

  drain(*lost);
  REQUIRE(lostReports.faultDescriptions ==
          std::vector<std::wstring>{faultDescription});
  REQUIRE_THROWS_AS(lost->disconnect(), rti1516_2025::NotConnected);
  REQUIRE_NOTHROW(lost->connect(lostReports, HLA_EVOKED));
  REQUIRE_NOTHROW(lost->disconnect());
  REQUIRE_NOTHROW(surviving->resignFederationExecution(rti1516_2025::NO_ACTION));
  REQUIRE_NOTHROW(surviving->destroyFederationExecution(federationName));
  REQUIRE_NOTHROW(surviving->disconnect());
}
