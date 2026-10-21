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
#error "The queued Next Message Request test requires the Umbra source directory."
#endif

namespace {

namespace standard_hla = umbra::detail::hla::wide;
namespace fixture_hla = umbra::test::hla::wide;

using rti1516_2025::FederateHandle;
using rti1516_2025::HLA_EVOKED;
using rti1516_2025::ObjectClassHandle;
using rti1516_2025::ObjectInstanceHandle;
using rti1516_2025::InteractionClassHandle;
using rti1516_2025::InteractionClassHandleSet;
using rti1516_2025::LogicalTime;
using rti1516_2025::MessageRetractionHandle;
using rti1516_2025::OrderType;
using rti1516_2025::ParameterHandleValueMap;
using rti1516_2025::AttributeHandle;
using rti1516_2025::AttributeHandleSet;
using rti1516_2025::AttributeHandleValueMap;
using rti1516_2025::RECEIVE;
using rti1516_2025::RTIambassador;
using rti1516_2025::RTIambassadorFactory;
using rti1516_2025::TIMESTAMP;
using rti1516_2025::TransportationTypeHandle;
using rti1516_2025::VariableLengthData;

std::wstring nextFederationName() {
  static std::atomic_uint64_t sequence{0U};
  return L"next-message-request-queued-tso-" +
      std::to_wstring(sequence.fetch_add(1U, std::memory_order_relaxed));
}

std::filesystem::path sourcePath(std::filesystem::path const& relativePath) {
  return std::filesystem::path(UMBRA_SOURCE_DIRECTORY) / relativePath;
}

class ReportingFederateAmbassador final
    : public rti1516_2025::NullFederateAmbassador {
 public:
  struct TimestampedInteractionReport final {
    InteractionClassHandle interactionClass;
    ParameterHandleValueMap parameterValues;
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
  };

  struct TimeAdvanceGrantReport final {
    std::wstring timeImplementationName;
    std::wstring value;
  };

  struct ObjectRemovalReport final {
    ObjectInstanceHandle objectInstance;
    FederateHandle producingFederate;
    std::wstring timeImplementationName;
    std::wstring timeValue;
    OrderType sentOrderType = RECEIVE;
    OrderType receivedOrderType = RECEIVE;
    bool retractionValid = false;
  };

  struct DirectedInteractionReport final {
    InteractionClassHandle interactionClass;
    ObjectInstanceHandle target;
    std::wstring timeValue;
    OrderType sentOrderType = RECEIVE;
    OrderType receivedOrderType = RECEIVE;
    bool retractionValid = false;
  };

  void receiveInteraction(
      InteractionClassHandle const& interactionClass,
      ParameterHandleValueMap const& parameterValues,
      VariableLengthData const& userSuppliedTag,
      TransportationTypeHandle const& transportationType,
      FederateHandle const& producingFederate,
      rti1516_2025::RegionHandleSet const* optionalSentRegions,
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
        optionalSentRegions != nullptr,
        time.implementationName(),
        time.toString(),
        sentOrderType,
        receivedOrderType,
        optionalRetraction != nullptr,
        optionalRetraction != nullptr && optionalRetraction->isValid(),
    });
    callbackOrder.push_back("interaction");
  }

  void discoverObjectInstance(
      ObjectInstanceHandle const& objectInstance,
      ObjectClassHandle const&,
      std::wstring const&,
      FederateHandle const&) override {
    discoveredObjectInstances.push_back(objectInstance);
  }

  void reflectAttributeValues(
      ObjectInstanceHandle const& objectInstance,
      AttributeHandleValueMap const& attributeValues,
      VariableLengthData const&,
      TransportationTypeHandle const&,
      FederateHandle const&,
      rti1516_2025::RegionHandleSet const*,
      LogicalTime const& time,
      OrderType sentOrderType,
      OrderType receivedOrderType,
      MessageRetractionHandle const* optionalRetraction) override {
    reflectedObjectInstances.push_back(objectInstance);
    reflectedAttributeCount = attributeValues.size();
    reflectedTimeValue = time.toString();
    reflectedSentOrderType = sentOrderType;
    reflectedReceivedOrderType = receivedOrderType;
    reflectedRetractionValid =
        optionalRetraction != nullptr && optionalRetraction->isValid();
    callbackOrder.push_back("attribute");
  }

  void removeObjectInstance(
      ObjectInstanceHandle const& objectInstance,
      VariableLengthData const&,
      FederateHandle const& producingFederate,
      LogicalTime const& time,
      OrderType sentOrderType,
      OrderType receivedOrderType,
      MessageRetractionHandle const* optionalRetraction) override {
    objectRemovalReports.push_back({
        objectInstance,
        producingFederate,
        time.implementationName(),
        time.toString(),
        sentOrderType,
        receivedOrderType,
        optionalRetraction != nullptr && optionalRetraction->isValid(),
    });
    callbackOrder.push_back("remove");
  }

  void receiveDirectedInteraction(
      InteractionClassHandle const& interactionClass,
      ObjectInstanceHandle const& target,
      ParameterHandleValueMap const&,
      VariableLengthData const&,
      TransportationTypeHandle const&,
      FederateHandle const&,
      LogicalTime const& time,
      OrderType sentOrderType,
      OrderType receivedOrderType,
      MessageRetractionHandle const* optionalRetraction) override {
    directedInteractionReports.push_back({
        interactionClass,
        target,
        time.toString(),
        sentOrderType,
        receivedOrderType,
        optionalRetraction != nullptr && optionalRetraction->isValid(),
    });
    callbackOrder.push_back("directed");
  }

  void timeAdvanceGrant(LogicalTime const& time) override {
    timeAdvanceGrantReports.push_back({time.implementationName(), time.toString()});
    callbackOrder.push_back("grant");
  }

  std::vector<TimestampedInteractionReport> timestampedInteractionReports;
  std::vector<ObjectRemovalReport> objectRemovalReports;
  std::vector<DirectedInteractionReport> directedInteractionReports;
  std::vector<ObjectInstanceHandle> discoveredObjectInstances;
  std::vector<ObjectInstanceHandle> reflectedObjectInstances;
  std::size_t reflectedAttributeCount = 0U;
  std::wstring reflectedTimeValue;
  OrderType reflectedSentOrderType = RECEIVE;
  OrderType reflectedReceivedOrderType = RECEIVE;
  bool reflectedRetractionValid = false;
  std::vector<TimeAdvanceGrantReport> timeAdvanceGrantReports;
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
    "Embedded Next Message Request grants at the next queued TSO timestamp",
    "[integration][development-profile][interaction-management][time-management]"
    "[next-message-request]"
    "[rti.service.next-message-request][rti.service.send-interaction]"
    "[rti.service.time-advance-request][federate.callback.receive-interaction]"
    "[federate.callback.time-advance-grant]") {
  ReportingFederateAmbassador publisherReports;
  ReportingFederateAmbassador receiverReports;
  auto publisher = makeRti();
  auto receiver = makeRti();
  auto const federationName = nextFederationName();
  auto const fomModule = sourcePath(std::filesystem::path("cpp") / "tests" / "data" /
                                    "parameter-handle-provider-fom.xml")
                             .wstring();
  unsigned char const tagBytes[] = {0x4E, 0x4D, 0x52};
  VariableLengthData const tag(tagBytes, sizeof(tagBytes));

  REQUIRE_NOTHROW(publisher->connect(publisherReports, HLA_EVOKED));
  REQUIRE_NOTHROW(receiver->connect(receiverReports, HLA_EVOKED));
  REQUIRE_NOTHROW(
      publisher->createFederationExecution(federationName, fomModule, standard_hla::mom::integer64_time));
  REQUIRE_NOTHROW(publisher->joinFederationExecution(
      L"nmr-publisher", L"publisher", federationName));
  REQUIRE_NOTHROW(receiver->joinFederationExecution(
      L"nmr-receiver", L"subscriber", federationName));

  auto const interactionClass = publisher->getInteractionClassHandle(
      fixture_hla::fom::parameter_fixture_child_interaction);
  auto const identifier = publisher->getParameterHandle(
      interactionClass,
      fixture_hla::fixture::identifier);
  REQUIRE(interactionClass.isValid());
  REQUIRE(identifier.isValid());
  ParameterHandleValueMap parameterValues;
  unsigned char const identifierBytes[] = {0xC1, 0xD2};
  parameterValues.emplace(
      identifier,
      VariableLengthData(identifierBytes, sizeof(identifierBytes)));

  REQUIRE_NOTHROW(publisher->publishInteractionClass(interactionClass));
  REQUIRE_NOTHROW(publisher->changeInteractionOrderType(interactionClass, TIMESTAMP));
  REQUIRE_NOTHROW(receiver->subscribeInteractionClass(interactionClass));
  REQUIRE_NOTHROW(receiver->enableTimeConstrained());
  drainCallbacks(*receiver);
  REQUIRE_NOTHROW(publisher->enableTimeRegulation(
      rti1516_2025::HLAinteger64Interval(5)));
  drainCallbacks(*publisher);

  auto const handle = publisher->sendInteraction(
      interactionClass,
      parameterValues,
      tag,
      rti1516_2025::HLAinteger64Time(7));
  REQUIRE(handle.isValid());
  REQUIRE(receiverReports.timestampedInteractionReports.empty());

  // The queued message at 7 is the NMR target even though the request asks
  // for 10. The request initially waits because the regulator's GALT is 5.
  REQUIRE_NOTHROW(receiver->nextMessageRequest(rti1516_2025::HLAinteger64Time(10)));
  REQUIRE(receiverReports.timeAdvanceGrantReports.empty());
  REQUIRE_NOTHROW(publisher->timeAdvanceRequest(rti1516_2025::HLAinteger64Time(2)));
  drainCallbacks(*publisher);
  drainCallbacks(*receiver);

  REQUIRE(publisherReports.timeAdvanceGrantReports.size() == 1U);
  REQUIRE(receiverReports.timeAdvanceGrantReports.size() == 1U);
  REQUIRE(receiverReports.timestampedInteractionReports.size() == 1U);
  REQUIRE(receiverReports.callbackOrder == std::vector<std::string>{"interaction", "grant"});
  REQUIRE(receiverReports.timeAdvanceGrantReports.front().value == L"7");
  REQUIRE(receiverReports.timestampedInteractionReports.front().timeValue == L"7");
  REQUIRE(receiverReports.timestampedInteractionReports.front().sentOrderType == TIMESTAMP);
  REQUIRE(receiverReports.timestampedInteractionReports.front().receivedOrderType == TIMESTAMP);
  REQUIRE(receiverReports.timestampedInteractionReports.front().retractionValid);

  rti1516_2025::HLAinteger64Time queriedTime;
  REQUIRE_NOTHROW(receiver->queryLogicalTime(queriedTime));
  REQUIRE(queriedTime.getTime() == 7);

  REQUIRE_NOTHROW(receiver->resignFederationExecution(rti1516_2025::NO_ACTION));
  REQUIRE_NOTHROW(publisher->resignFederationExecution(
      rti1516_2025::CANCEL_THEN_DELETE_THEN_DIVEST));
  REQUIRE_NOTHROW(publisher->destroyFederationExecution(federationName));
  REQUIRE_NOTHROW(receiver->disconnect());
  REQUIRE_NOTHROW(publisher->disconnect());
}

TEST_CASE(
    "Embedded NMR grant below request eases the subsequent TSO timestamp bound",
    "[integration][development-profile][interaction-management][time-management]"
    "[nmr-early-grant-tso-bound-easing][rti.service.next-message-request]"
    "[rti.service.send-interaction][rti.service.time-advance-request]"
    "[federate.callback.receive-interaction][federate.callback.time-advance-grant]") {
  ReportingFederateAmbassador regulatingReports;
  ReportingFederateAmbassador peerReports;
  auto regulating = makeRti();
  auto peer = makeRti();
  auto const federationName = nextFederationName();
  auto const fomModule = sourcePath(std::filesystem::path("cpp") / "tests" / "data" /
                                    "parameter-handle-provider-fom.xml")
                             .wstring();
  unsigned char const firstPayload[] = {0x51U};
  unsigned char const secondPayload[] = {0x52U};
  unsigned char const tagBytes[] = {0x4EU, 0x4DU, 0x52U, 0x2DU, 0x45U};
  VariableLengthData const tag(tagBytes, sizeof(tagBytes));

  REQUIRE_NOTHROW(regulating->connect(regulatingReports, HLA_EVOKED));
  REQUIRE_NOTHROW(peer->connect(peerReports, HLA_EVOKED));
  REQUIRE_NOTHROW(regulating->createFederationExecution(
      federationName, fomModule, standard_hla::mom::integer64_time));
  REQUIRE_NOTHROW(regulating->joinFederationExecution(
      L"nmr-early-grant-regulating", L"publisher", federationName));
  REQUIRE_NOTHROW(peer->joinFederationExecution(
      L"nmr-early-grant-peer", L"publisher", federationName));

  auto const interactionClass = regulating->getInteractionClassHandle(
      fixture_hla::fom::parameter_fixture_child_interaction);
  auto const identifier = regulating->getParameterHandle(
      interactionClass, fixture_hla::fixture::identifier);
  REQUIRE(interactionClass.isValid());
  REQUIRE(identifier.isValid());
  REQUIRE_NOTHROW(regulating->publishInteractionClass(interactionClass));
  REQUIRE_NOTHROW(peer->publishInteractionClass(interactionClass));
  REQUIRE_NOTHROW(regulating->changeInteractionOrderType(interactionClass, TIMESTAMP));
  REQUIRE_NOTHROW(peer->changeInteractionOrderType(interactionClass, TIMESTAMP));
  REQUIRE_NOTHROW(regulating->subscribeInteractionClass(interactionClass));
  REQUIRE_NOTHROW(regulating->enableTimeConstrained());
  drainCallbacks(*regulating);
  REQUIRE_NOTHROW(regulating->enableTimeRegulation(
      rti1516_2025::HLAinteger64Interval(1)));
  drainCallbacks(*regulating);
  REQUIRE_NOTHROW(peer->enableTimeRegulation(
      rti1516_2025::HLAinteger64Interval(1)));
  drainCallbacks(*peer);

  ParameterHandleValueMap firstValues;
  firstValues.emplace(
      identifier, VariableLengthData(firstPayload, sizeof(firstPayload)));
  auto const incoming = peer->sendInteraction(
      interactionClass,
      firstValues,
      tag,
      rti1516_2025::HLAinteger64Time(5));
  REQUIRE(incoming.isValid());

  // While Time Advancing, a TSO send is bounded against request 10 plus
  // lookahead 1. The queued timestamp-5 message permits the NMR grant at 5.
  REQUIRE_NOTHROW(regulating->nextMessageRequest(
      rti1516_2025::HLAinteger64Time(10)));
  REQUIRE(regulatingReports.timeAdvanceGrantReports.empty());
  REQUIRE_NOTHROW(peer->timeAdvanceRequest(rti1516_2025::HLAinteger64Time(4)));
  drainCallbacks(*peer);
  drainCallbacks(*regulating);
  REQUIRE(regulatingReports.timeAdvanceGrantReports.size() == 1U);
  REQUIRE(regulatingReports.timeAdvanceGrantReports.front().value == L"5");
  REQUIRE(regulatingReports.timestampedInteractionReports.size() == 1U);
  REQUIRE(regulatingReports.callbackOrder ==
      std::vector<std::string>{"interaction", "grant"});

  rti1516_2025::HLAinteger64Time logicalTime;
  REQUIRE_NOTHROW(regulating->queryLogicalTime(logicalTime));
  REQUIRE(logicalTime.getTime() == 5);

  ParameterHandleValueMap secondValues;
  secondValues.emplace(
      identifier, VariableLengthData(secondPayload, sizeof(secondPayload)));
  auto const outgoing = regulating->sendInteraction(
      interactionClass,
      secondValues,
      tag,
      rti1516_2025::HLAinteger64Time(7));
  REQUIRE(outgoing.isValid());
  REQUIRE(regulatingReports.timeAdvanceGrantReports.front().value == L"5");
  REQUIRE_NOTHROW(regulating->queryLogicalTime(logicalTime));
  REQUIRE(logicalTime.getTime() == 5);

  REQUIRE_NOTHROW(regulating->resignFederationExecution(rti1516_2025::NO_ACTION));
  REQUIRE_NOTHROW(peer->resignFederationExecution(
      rti1516_2025::CANCEL_THEN_DELETE_THEN_DIVEST));
  REQUIRE_NOTHROW(regulating->destroyFederationExecution(federationName));
  REQUIRE_NOTHROW(regulating->disconnect());
  REQUIRE_NOTHROW(peer->disconnect());
}

TEST_CASE(
    "Embedded Next Message Request retargets to a TSO queued after submission",
    "[integration][development-profile][interaction-management][time-management]"
    "[next-message-request][dynamic-future-input]"
    "[rti.service.next-message-request][rti.service.send-interaction]"
    "[rti.service.time-advance-request][federate.callback.receive-interaction]"
    "[federate.callback.time-advance-grant]") {
  ReportingFederateAmbassador publisherReports;
  ReportingFederateAmbassador receiverReports;
  auto publisher = makeRti();
  auto receiver = makeRti();
  auto const federationName = nextFederationName();
  auto const fomModule = sourcePath(std::filesystem::path("cpp") / "tests" / "data" /
                                    "parameter-handle-provider-fom.xml")
                             .wstring();
  unsigned char const tagBytes[] = {0x4EU, 0x4DU, 0x52U, 0x2DU, 0x46U};
  VariableLengthData const tag(tagBytes, sizeof(tagBytes));

  REQUIRE_NOTHROW(publisher->connect(publisherReports, HLA_EVOKED));
  REQUIRE_NOTHROW(receiver->connect(receiverReports, HLA_EVOKED));
  REQUIRE_NOTHROW(publisher->createFederationExecution(
      federationName, fomModule, standard_hla::mom::integer64_time));
  REQUIRE_NOTHROW(publisher->joinFederationExecution(
      L"nmr-future-publisher", L"publisher", federationName));
  REQUIRE_NOTHROW(receiver->joinFederationExecution(
      L"nmr-future-receiver", L"subscriber", federationName));

  auto const interactionClass = publisher->getInteractionClassHandle(
      fixture_hla::fom::parameter_fixture_child_interaction);
  auto const identifier = publisher->getParameterHandle(
      interactionClass,
      fixture_hla::fixture::identifier);
  REQUIRE(interactionClass.isValid());
  REQUIRE(identifier.isValid());
  ParameterHandleValueMap parameterValues;
  unsigned char const identifierBytes[] = {0xF1U, 0xA7U};
  parameterValues.emplace(
      identifier,
      VariableLengthData(identifierBytes, sizeof(identifierBytes)));

  REQUIRE_NOTHROW(publisher->publishInteractionClass(interactionClass));
  REQUIRE_NOTHROW(publisher->changeInteractionOrderType(interactionClass, TIMESTAMP));
  REQUIRE_NOTHROW(receiver->subscribeInteractionClass(interactionClass));
  REQUIRE_NOTHROW(receiver->enableTimeConstrained());
  drainCallbacks(*receiver);
  REQUIRE_NOTHROW(publisher->enableTimeRegulation(
      rti1516_2025::HLAinteger64Interval(5)));
  drainCallbacks(*publisher);

  // Move the regulator so that its GALT is 7 before the NMR is submitted.
  // With no TSO queued, the receiver must remain pending at its supplied 10.
  REQUIRE_NOTHROW(publisher->timeAdvanceRequest(rti1516_2025::HLAinteger64Time(2)));
  drainCallbacks(*publisher);
  REQUIRE(publisherReports.timeAdvanceGrantReports.size() == 1U);

  REQUIRE_NOTHROW(receiver->nextMessageRequest(rti1516_2025::HLAinteger64Time(10)));
  REQUIRE(receiverReports.timeAdvanceGrantReports.empty());
  REQUIRE(receiverReports.timestampedInteractionReports.empty());

  auto const handle = publisher->sendInteraction(
      interactionClass,
      parameterValues,
      tag,
      rti1516_2025::HLAinteger64Time(7));
  REQUIRE(handle.isValid());
  REQUIRE(receiverReports.timeAdvanceGrantReports.empty());
  REQUIRE(receiverReports.timestampedInteractionReports.empty());

  // The later-arriving message becomes the pending NMR target at the current
  // GALT boundary. It is delivered before the grant at 7, not at supplied 10.
  drainCallbacks(*receiver);

  REQUIRE(publisherReports.timeAdvanceGrantReports.size() == 1U);
  REQUIRE(receiverReports.timeAdvanceGrantReports.size() == 1U);
  REQUIRE(receiverReports.timestampedInteractionReports.size() == 1U);
  REQUIRE(receiverReports.callbackOrder == std::vector<std::string>{"interaction", "grant"});
  REQUIRE(receiverReports.timeAdvanceGrantReports.front().value == L"7");
  auto const& received = receiverReports.timestampedInteractionReports.front();
  REQUIRE(received.interactionClass == interactionClass);
  REQUIRE(received.parameterValues.size() == 1U);
  REQUIRE(received.parameterValues.count(identifier) == 1U);
  REQUIRE(received.parameterValues.at(identifier).size() == sizeof(identifierBytes));
  REQUIRE(received.userSuppliedTag.size() == sizeof(tagBytes));
  REQUIRE(received.timeValue == L"7");
  REQUIRE(received.sentOrderType == TIMESTAMP);
  REQUIRE(received.receivedOrderType == TIMESTAMP);
  REQUIRE(received.retractionValid);

  rti1516_2025::HLAinteger64Time queriedTime;
  REQUIRE_NOTHROW(receiver->queryLogicalTime(queriedTime));
  REQUIRE(queriedTime.getTime() == 7);

  REQUIRE_NOTHROW(receiver->resignFederationExecution(rti1516_2025::NO_ACTION));
  REQUIRE_NOTHROW(publisher->resignFederationExecution(
      rti1516_2025::CANCEL_THEN_DELETE_THEN_DIVEST));
  REQUIRE_NOTHROW(publisher->destroyFederationExecution(federationName));
  REQUIRE_NOTHROW(receiver->disconnect());
  REQUIRE_NOTHROW(publisher->disconnect());
}

void verifyTimestampedAttributeUpdateRetargetsAdvance(bool availableRequest) {
  ReportingFederateAmbassador publisherReports;
  ReportingFederateAmbassador receiverReports;
  auto publisher = makeRti();
  auto receiver = makeRti();
  auto const federationName = nextFederationName() + L"-attribute";
  auto const fomModule = sourcePath(std::filesystem::path("cpp") / "tests" / "data" /
                                    "attribute-update-passel-fom.xml")
                             .wstring();
  unsigned char const valueBytes[] = {0x4EU, 0x4DU, 0x52U, 0x2DU, 0x41U};

  REQUIRE_NOTHROW(publisher->connect(publisherReports, HLA_EVOKED));
  REQUIRE_NOTHROW(receiver->connect(receiverReports, HLA_EVOKED));
  REQUIRE_NOTHROW(publisher->createFederationExecution(
      federationName, fomModule, standard_hla::mom::integer64_time));
  REQUIRE_NOTHROW(publisher->joinFederationExecution(
      L"nmr-attribute-publisher", L"publisher", federationName));
  REQUIRE_NOTHROW(receiver->joinFederationExecution(
      L"nmr-attribute-receiver", L"subscriber", federationName));

  auto const objectClass = publisher->getObjectClassHandle(
      fixture_hla::fom::attribute_fixture_child);
  auto const attribute = publisher->getAttributeHandle(
      objectClass, fixture_hla::fixture::reliable_base_a);
  REQUIRE(objectClass.isValid());
  REQUIRE(attribute.isValid());
  AttributeHandleSet const attributes{attribute};
  AttributeHandleValueMap values;
  values.emplace(attribute, VariableLengthData(valueBytes, sizeof(valueBytes)));
  REQUIRE_NOTHROW(publisher->publishObjectClassAttributes(objectClass, attributes));
  REQUIRE_NOTHROW(receiver->subscribeObjectClassAttributes(objectClass, attributes));
  REQUIRE_NOTHROW(publisher->changeDefaultAttributeOrderType(
      objectClass, attributes, TIMESTAMP));
  auto const objectInstance = publisher->registerObjectInstance(objectClass);
  REQUIRE(objectInstance.isValid());
  drainCallbacks(*receiver);
  REQUIRE(receiverReports.discoveredObjectInstances.size() == 1U);
  REQUIRE_NOTHROW(receiver->enableTimeConstrained());
  drainCallbacks(*receiver);
  REQUIRE_NOTHROW(publisher->enableTimeRegulation(
      rti1516_2025::HLAinteger64Interval(5)));
  drainCallbacks(*publisher);

  REQUIRE_NOTHROW(publisher->timeAdvanceRequest(rti1516_2025::HLAinteger64Time(2)));
  drainCallbacks(*publisher);
  if (availableRequest) {
    REQUIRE_NOTHROW(receiver->nextMessageRequestAvailable(
        rti1516_2025::HLAinteger64Time(10)));
  } else {
    REQUIRE_NOTHROW(receiver->nextMessageRequest(
        rti1516_2025::HLAinteger64Time(10)));
  }
  REQUIRE(receiverReports.timeAdvanceGrantReports.empty());
  REQUIRE(receiverReports.reflectedObjectInstances.empty());

  auto const retraction = publisher->updateAttributeValues(
      objectInstance, values, VariableLengthData(),
      rti1516_2025::HLAinteger64Time(7));
  REQUIRE(retraction.isValid());
  REQUIRE(receiverReports.timeAdvanceGrantReports.empty());
  REQUIRE(receiverReports.reflectedObjectInstances.empty());
  drainCallbacks(*receiver);

  REQUIRE(receiverReports.reflectedObjectInstances ==
          std::vector<ObjectInstanceHandle>{objectInstance});
  REQUIRE(receiverReports.timeAdvanceGrantReports.size() == 1U);
  REQUIRE(receiverReports.callbackOrder ==
          std::vector<std::string>{"attribute", "grant"});
  REQUIRE(receiverReports.reflectedAttributeCount == 1U);
  REQUIRE(receiverReports.reflectedTimeValue == L"7");
  REQUIRE(receiverReports.reflectedSentOrderType == TIMESTAMP);
  REQUIRE(receiverReports.reflectedReceivedOrderType == TIMESTAMP);
  REQUIRE(receiverReports.reflectedRetractionValid);
  REQUIRE(receiverReports.timeAdvanceGrantReports.front().value == L"7");

  REQUIRE_NOTHROW(receiver->resignFederationExecution(rti1516_2025::NO_ACTION));
  REQUIRE_NOTHROW(publisher->resignFederationExecution(
      rti1516_2025::CANCEL_THEN_DELETE_THEN_DIVEST));
  REQUIRE_NOTHROW(publisher->destroyFederationExecution(federationName));
  REQUIRE_NOTHROW(receiver->disconnect());
  REQUIRE_NOTHROW(publisher->disconnect());
}

TEST_CASE(
    "Embedded Next Message Request retargets to a timestamped attribute update queued after submission",
    "[integration][development-profile][object-management][time-management]"
    "[next-message-request][dynamic-future-input]"
    "[rti.service.next-message-request][rti.service.update-attribute-values]"
    "[rti.service.time-advance-request][federate.callback.reflect-attribute-values]"
    "[federate.callback.time-advance-grant]") {
  verifyTimestampedAttributeUpdateRetargetsAdvance(false);
}

TEST_CASE(
    "Embedded Next Message Request Available retargets to a timestamped attribute update queued after submission",
    "[integration][development-profile][object-management][time-management]"
    "[next-message-request-available][dynamic-future-input]"
    "[rti.service.next-message-request-available][rti.service.update-attribute-values]"
    "[rti.service.time-advance-request][federate.callback.reflect-attribute-values]"
    "[federate.callback.time-advance-grant]") {
  verifyTimestampedAttributeUpdateRetargetsAdvance(true);
}

void verifyTimestampedObjectDeletionRetargetsAdvance(bool availableRequest) {
  ReportingFederateAmbassador publisherReports;
  ReportingFederateAmbassador receiverReports;
  auto publisher = makeRti();
  auto receiver = makeRti();
  auto const federationName = nextFederationName() + L"-deletion";
  auto const fomModule = sourcePath(std::filesystem::path("cpp") / "tests" / "data" /
                                    "attribute-update-passel-fom.xml")
                             .wstring();

  REQUIRE_NOTHROW(publisher->connect(publisherReports, HLA_EVOKED));
  REQUIRE_NOTHROW(receiver->connect(receiverReports, HLA_EVOKED));
  REQUIRE_NOTHROW(publisher->createFederationExecution(
      federationName, fomModule, standard_hla::mom::integer64_time));
  REQUIRE_NOTHROW(publisher->joinFederationExecution(
      L"nmr-deletion-publisher", L"publisher", federationName));
  REQUIRE_NOTHROW(receiver->joinFederationExecution(
      L"nmr-deletion-receiver", L"subscriber", federationName));

  auto const objectClass = publisher->getObjectClassHandle(
      fixture_hla::fom::attribute_fixture_child);
  auto const attribute = publisher->getAttributeHandle(
      objectClass, fixture_hla::fixture::reliable_base_a);
  REQUIRE(objectClass.isValid());
  REQUIRE(attribute.isValid());
  AttributeHandleSet const attributes{attribute};
  REQUIRE_NOTHROW(publisher->publishObjectClassAttributes(objectClass, attributes));
  REQUIRE_NOTHROW(receiver->subscribeObjectClassAttributes(objectClass, attributes));
  auto const objectInstance = publisher->registerObjectInstance(objectClass);
  REQUIRE(objectInstance.isValid());
  drainCallbacks(*receiver);
  REQUIRE(receiverReports.discoveredObjectInstances.size() == 1U);
  REQUIRE_NOTHROW(receiver->enableTimeConstrained());
  drainCallbacks(*receiver);
  REQUIRE_NOTHROW(publisher->enableTimeRegulation(
      rti1516_2025::HLAinteger64Interval(5)));
  drainCallbacks(*publisher);
  REQUIRE_NOTHROW(publisher->timeAdvanceRequest(rti1516_2025::HLAinteger64Time(2)));
  drainCallbacks(*publisher);

  if (availableRequest) {
    REQUIRE_NOTHROW(receiver->nextMessageRequestAvailable(
        rti1516_2025::HLAinteger64Time(10)));
  } else {
    REQUIRE_NOTHROW(receiver->nextMessageRequest(
        rti1516_2025::HLAinteger64Time(10)));
  }
  REQUIRE(receiverReports.timeAdvanceGrantReports.empty());
  REQUIRE(receiverReports.objectRemovalReports.empty());

  auto const retraction = publisher->deleteObjectInstance(
      objectInstance,
      VariableLengthData(),
      rti1516_2025::HLAinteger64Time(7));
  REQUIRE(retraction.isValid());
  REQUIRE(receiverReports.timeAdvanceGrantReports.empty());
  REQUIRE(receiverReports.objectRemovalReports.empty());
  drainCallbacks(*receiver);

  REQUIRE(receiverReports.objectRemovalReports.size() == 1U);
  REQUIRE(receiverReports.timeAdvanceGrantReports.size() == 1U);
  REQUIRE(receiverReports.callbackOrder ==
          std::vector<std::string>{"remove", "grant"});
  auto const& removal = receiverReports.objectRemovalReports.front();
  REQUIRE(removal.objectInstance == objectInstance);
  REQUIRE(removal.timeValue == L"7");
  REQUIRE(removal.sentOrderType == TIMESTAMP);
  REQUIRE(removal.receivedOrderType == TIMESTAMP);
  REQUIRE(removal.retractionValid);
  REQUIRE(receiverReports.timeAdvanceGrantReports.front().value == L"7");

  REQUIRE_NOTHROW(receiver->resignFederationExecution(rti1516_2025::NO_ACTION));
  REQUIRE_NOTHROW(publisher->resignFederationExecution(
      rti1516_2025::CANCEL_THEN_DELETE_THEN_DIVEST));
  REQUIRE_NOTHROW(publisher->destroyFederationExecution(federationName));
  REQUIRE_NOTHROW(receiver->disconnect());
  REQUIRE_NOTHROW(publisher->disconnect());
}

TEST_CASE(
    "Embedded Next Message Request retargets to a timestamped object deletion queued after submission",
    "[integration][development-profile][object-management][time-management]"
    "[next-message-request][dynamic-future-input]"
    "[rti.service.next-message-request][rti.service.delete-object-instance]"
    "[federate.callback.remove-object-instance][federate.callback.time-advance-grant]") {
  verifyTimestampedObjectDeletionRetargetsAdvance(false);
}

TEST_CASE(
    "Embedded Next Message Request Available retargets to a timestamped object deletion queued after submission",
    "[integration][development-profile][object-management][time-management]"
    "[next-message-request-available][dynamic-future-input]"
    "[rti.service.next-message-request-available][rti.service.delete-object-instance]"
    "[federate.callback.remove-object-instance][federate.callback.time-advance-grant]") {
  verifyTimestampedObjectDeletionRetargetsAdvance(true);
}

void verifyTimestampedDirectedInteractionRetargetsAdvance(bool availableRequest) {
  ReportingFederateAmbassador publisherReports;
  ReportingFederateAmbassador receiverReports;
  auto publisher = makeRti();
  auto receiver = makeRti();
  auto const federationName = nextFederationName() + L"-directed";
  auto const objectConsumer = sourcePath(
      std::filesystem::path("cpp") / "tests" / "data" /
      "directed-interaction-object-consumer-fom.xml");
  auto const interactionProvider = sourcePath(
      std::filesystem::path("cpp") / "tests" / "data" /
      "directed-interaction-interaction-provider-fom.xml");
  auto const fomModules = std::vector<std::wstring>{
      objectConsumer.wstring(), interactionProvider.wstring()};
  unsigned char const tagBytes[] = {0x4EU, 0x4DU, 0x52U, 0x2DU, 0x44U};
  VariableLengthData const tag(tagBytes, sizeof(tagBytes));

  REQUIRE_NOTHROW(publisher->connect(publisherReports, HLA_EVOKED));
  REQUIRE_NOTHROW(receiver->connect(receiverReports, HLA_EVOKED));
  REQUIRE_NOTHROW(publisher->createFederationExecution(
      federationName, fomModules, standard_hla::mom::integer64_time));
  REQUIRE_NOTHROW(publisher->joinFederationExecution(
      L"nmr-directed-publisher", L"publisher", federationName));
  REQUIRE_NOTHROW(receiver->joinFederationExecution(
      L"nmr-directed-receiver", L"subscriber", federationName));

  auto const objectClass = publisher->getObjectClassHandle(
      fixture_hla::fom::directed_fixture_object);
  auto const marker = publisher->getAttributeHandle(
      objectClass, fixture_hla::fixture::directed_target_marker);
  auto const interactionClass = publisher->getInteractionClassHandle(
      fixture_hla::fom::directed_fixture_interaction);
  REQUIRE(objectClass.isValid());
  REQUIRE(marker.isValid());
  REQUIRE(interactionClass.isValid());
  InteractionClassHandleSet const directedClasses{interactionClass};
  REQUIRE_NOTHROW(publisher->publishObjectClassAttributes(objectClass, {marker}));
  REQUIRE_NOTHROW(receiver->subscribeObjectClassAttributes(objectClass, {marker}));
  REQUIRE_NOTHROW(publisher->publishObjectClassDirectedInteractions(
      objectClass, directedClasses));
  REQUIRE_NOTHROW(publisher->changeInteractionOrderType(
      interactionClass, TIMESTAMP));
  REQUIRE_NOTHROW(receiver->subscribeObjectClassDirectedInteractions(
      objectClass, directedClasses, true));
  auto const target = publisher->registerObjectInstance(objectClass);
  REQUIRE(target.isValid());
  drainCallbacks(*receiver);
  REQUIRE(receiverReports.discoveredObjectInstances.size() == 1U);
  REQUIRE_NOTHROW(receiver->enableTimeConstrained());
  drainCallbacks(*receiver);
  REQUIRE_NOTHROW(publisher->enableTimeRegulation(
      rti1516_2025::HLAinteger64Interval(5)));
  drainCallbacks(*publisher);
  REQUIRE_NOTHROW(publisher->timeAdvanceRequest(
      rti1516_2025::HLAinteger64Time(2)));
  drainCallbacks(*publisher);

  if (availableRequest) {
    REQUIRE_NOTHROW(receiver->nextMessageRequestAvailable(
        rti1516_2025::HLAinteger64Time(10)));
  } else {
    REQUIRE_NOTHROW(receiver->nextMessageRequest(
        rti1516_2025::HLAinteger64Time(10)));
  }
  REQUIRE(receiverReports.timeAdvanceGrantReports.empty());
  REQUIRE(receiverReports.directedInteractionReports.empty());
  auto const retraction = publisher->sendDirectedInteraction(
      interactionClass, target, ParameterHandleValueMap{}, tag,
      rti1516_2025::HLAinteger64Time(7));
  REQUIRE(retraction.isValid());
  REQUIRE(receiverReports.timeAdvanceGrantReports.empty());
  REQUIRE(receiverReports.directedInteractionReports.empty());
  drainCallbacks(*receiver);

  REQUIRE(receiverReports.directedInteractionReports.size() == 1U);
  REQUIRE(receiverReports.timeAdvanceGrantReports.size() == 1U);
  REQUIRE(receiverReports.callbackOrder ==
          std::vector<std::string>{"directed", "grant"});
  auto const& delivered = receiverReports.directedInteractionReports.front();
  REQUIRE(delivered.interactionClass == interactionClass);
  REQUIRE(delivered.target == target);
  REQUIRE(delivered.timeValue == L"7");
  REQUIRE(delivered.sentOrderType == TIMESTAMP);
  REQUIRE(delivered.receivedOrderType == TIMESTAMP);
  REQUIRE(delivered.retractionValid);
  REQUIRE(receiverReports.timeAdvanceGrantReports.front().value == L"7");

  REQUIRE_NOTHROW(receiver->resignFederationExecution(rti1516_2025::NO_ACTION));
  REQUIRE_NOTHROW(publisher->resignFederationExecution(
      rti1516_2025::CANCEL_THEN_DELETE_THEN_DIVEST));
  REQUIRE_NOTHROW(publisher->destroyFederationExecution(federationName));
  REQUIRE_NOTHROW(receiver->disconnect());
  REQUIRE_NOTHROW(publisher->disconnect());
}

TEST_CASE(
    "Embedded Next Message Request retargets to a timestamped directed interaction queued after submission",
    "[integration][development-profile][interaction-management][directed][time-management]"
    "[next-message-request][dynamic-future-input]"
    "[rti.service.next-message-request][rti.service.send-directed-interaction]"
    "[federate.callback.receive-directed-interaction][federate.callback.time-advance-grant]") {
  verifyTimestampedDirectedInteractionRetargetsAdvance(false);
}

TEST_CASE(
    "Embedded Next Message Request Available retargets to a timestamped directed interaction queued after submission",
    "[integration][development-profile][interaction-management][directed][time-management]"
    "[next-message-request-available][dynamic-future-input]"
    "[rti.service.next-message-request-available][rti.service.send-directed-interaction]"
    "[federate.callback.receive-directed-interaction][federate.callback.time-advance-grant]") {
  verifyTimestampedDirectedInteractionRetargetsAdvance(true);
}

TEST_CASE(
    "Embedded Next Message Request Available retargets to a TSO queued after submission",
    "[integration][development-profile][interaction-management][time-management]"
    "[next-message-request-available][dynamic-future-input]"
    "[rti.service.next-message-request-available][rti.service.send-interaction]"
    "[rti.service.time-advance-request][federate.callback.receive-interaction]"
    "[federate.callback.time-advance-grant]") {
  ReportingFederateAmbassador publisherReports;
  ReportingFederateAmbassador receiverReports;
  auto publisher = makeRti();
  auto receiver = makeRti();
  auto const federationName = nextFederationName() + L"-available";
  auto const fomModule = sourcePath(std::filesystem::path("cpp") / "tests" / "data" /
                                    "parameter-handle-provider-fom.xml")
                             .wstring();
  unsigned char const tagBytes[] = {0x4EU, 0x4DU, 0x52U, 0x41U};
  VariableLengthData const tag(tagBytes, sizeof(tagBytes));

  REQUIRE_NOTHROW(publisher->connect(publisherReports, HLA_EVOKED));
  REQUIRE_NOTHROW(receiver->connect(receiverReports, HLA_EVOKED));
  REQUIRE_NOTHROW(publisher->createFederationExecution(
      federationName, fomModule, standard_hla::mom::integer64_time));
  REQUIRE_NOTHROW(publisher->joinFederationExecution(
      L"nmra-future-publisher", L"publisher", federationName));
  REQUIRE_NOTHROW(receiver->joinFederationExecution(
      L"nmra-future-receiver", L"subscriber", federationName));

  auto const interactionClass = publisher->getInteractionClassHandle(
      fixture_hla::fom::parameter_fixture_child_interaction);
  auto const identifier = publisher->getParameterHandle(
      interactionClass,
      fixture_hla::fixture::identifier);
  REQUIRE(interactionClass.isValid());
  REQUIRE(identifier.isValid());
  ParameterHandleValueMap parameterValues;
  unsigned char const identifierBytes[] = {0xA1U, 0x7FU};
  parameterValues.emplace(
      identifier,
      VariableLengthData(identifierBytes, sizeof(identifierBytes)));

  REQUIRE_NOTHROW(publisher->publishInteractionClass(interactionClass));
  REQUIRE_NOTHROW(publisher->changeInteractionOrderType(interactionClass, TIMESTAMP));
  REQUIRE_NOTHROW(receiver->subscribeInteractionClass(interactionClass));
  REQUIRE_NOTHROW(receiver->enableTimeConstrained());
  drainCallbacks(*receiver);
  REQUIRE_NOTHROW(publisher->enableTimeRegulation(
      rti1516_2025::HLAinteger64Interval(5)));
  drainCallbacks(*publisher);

  // With no queued input, NMRA(10) remains pending even after the regulator
  // advances to 2 and establishes GALT 7.
  REQUIRE_NOTHROW(publisher->timeAdvanceRequest(rti1516_2025::HLAinteger64Time(2)));
  drainCallbacks(*publisher);
  REQUIRE_NOTHROW(receiver->nextMessageRequestAvailable(
      rti1516_2025::HLAinteger64Time(10)));
  REQUIRE(receiverReports.timeAdvanceGrantReports.empty());
  REQUIRE(receiverReports.timestampedInteractionReports.empty());

  auto const handle = publisher->sendInteraction(
      interactionClass,
      parameterValues,
      tag,
      rti1516_2025::HLAinteger64Time(7));
  REQUIRE(handle.isValid());
  drainCallbacks(*receiver);

  REQUIRE(receiverReports.timeAdvanceGrantReports.size() == 1U);
  REQUIRE(receiverReports.timestampedInteractionReports.size() == 1U);
  REQUIRE(receiverReports.callbackOrder == std::vector<std::string>{"interaction", "grant"});
  REQUIRE(receiverReports.timeAdvanceGrantReports.front().value == L"7");
  REQUIRE(receiverReports.timestampedInteractionReports.front().timeValue == L"7");
  REQUIRE(receiverReports.timestampedInteractionReports.front().sentOrderType == TIMESTAMP);
  REQUIRE(receiverReports.timestampedInteractionReports.front().receivedOrderType == TIMESTAMP);

  rti1516_2025::HLAinteger64Time queriedTime;
  REQUIRE_NOTHROW(receiver->queryLogicalTime(queriedTime));
  REQUIRE(queriedTime.getTime() == 7);

  REQUIRE_NOTHROW(receiver->resignFederationExecution(rti1516_2025::NO_ACTION));
  REQUIRE_NOTHROW(publisher->resignFederationExecution(
      rti1516_2025::CANCEL_THEN_DELETE_THEN_DIVEST));
  REQUIRE_NOTHROW(publisher->destroyFederationExecution(federationName));
  REQUIRE_NOTHROW(receiver->disconnect());
  REQUIRE_NOTHROW(publisher->disconnect());
}
