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
#error "The timestamped object-deletion retraction test requires the Umbra source directory."
#endif

namespace {

namespace standard_hla = umbra::detail::hla::wide;
namespace fixture_hla = umbra::test::hla::wide;

using rti1516_2025::AttributeHandle;
using rti1516_2025::AttributeHandleSet;
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
  return L"timestamped-object-deletion-retraction-" +
      std::to_wstring(sequence.fetch_add(1U, std::memory_order_relaxed));
}

std::filesystem::path sourcePath(std::filesystem::path const& relativePath) {
  return std::filesystem::path(UMBRA_SOURCE_DIRECTORY) / relativePath;
}

class ReportingFederateAmbassador final
    : public rti1516_2025::NullFederateAmbassador {
 public:
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

  struct ReceiveOrderRemovalReport final {
    ObjectInstanceHandle objectInstance;
    VariableLengthData tag;
    FederateHandle producer;
  };

  struct TimeReport final {
    std::wstring implementationName;
    std::wstring value;
  };

  struct FlushQueueGrantReport final {
    std::wstring value;
    std::wstring optimisticValue;
  };

  struct RequestRetractionReport final {
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

  void removeObjectInstance(
      ObjectInstanceHandle const& objectInstance,
      VariableLengthData const& tag,
      FederateHandle const& producer,
      LogicalTime const& time,
      OrderType sentOrderType,
      OrderType receivedOrderType,
      rti1516_2025::MessageRetractionHandle const* optionalRetraction) override {
    objectRemovalReports.push_back({
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

  void removeObjectInstance(
      ObjectInstanceHandle const& objectInstance,
      VariableLengthData const& tag,
      FederateHandle const& producer) override {
    receiveOrderObjectRemovalReports.push_back({objectInstance, tag, producer});
    callbackOrder.push_back("remove");
  }

  void requestRetraction(
      rti1516_2025::MessageRetractionHandle const& retraction) override {
    requestRetractionReports.push_back({retraction.isValid(), retraction.encode()});
    callbackOrder.push_back("request-retraction");
  }

  void timeAdvanceGrant(LogicalTime const& time) override {
    timeAdvanceGrantReports.push_back({time.implementationName(), time.toString()});
    callbackOrder.push_back("grant");
  }

  void flushQueueGrant(
      LogicalTime const& time,
      LogicalTime const& optimisticTime) override {
    flushQueueGrantReports.push_back({time.toString(), optimisticTime.toString()});
    callbackOrder.push_back("flush-grant");
  }

  std::vector<ObjectInstanceHandle> objectDiscoveryReports;
  std::vector<RemovalReport> objectRemovalReports;
  std::vector<ReceiveOrderRemovalReport> receiveOrderObjectRemovalReports;
  std::vector<RequestRetractionReport> requestRetractionReports;
  std::vector<TimeReport> timeAdvanceGrantReports;
  std::vector<FlushQueueGrantReport> flushQueueGrantReports;
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
    "Embedded Request Retraction reconstitutes a delivered timestamped object deletion",
    "[integration][development-profile][object-management][time-management][tso]"
    "[retraction][multi-federate-callback-ordering]"
    "[rti.service.delete-object-instance][rti.service.retract]"
    "[rti.service.enable-time-constrained][rti.service.enable-time-regulation]"
    "[rti.service.time-advance-request]"
    "[federate.callback.remove-object-instance][federate.callback.request-retraction]"
    "[federate.callback.time-advance-grant]") {
  ReportingFederateAmbassador publisherReports;
  ReportingFederateAmbassador immediateReports;
  ReportingFederateAmbassador constrainedReports;
  auto publisher = makeRti();
  auto immediate = makeRti();
  auto constrained = makeRti();
  auto const federationName = nextFederationName();
  auto const fomModule = sourcePath(
      std::filesystem::path("cpp") / "tests" / "data" /
      "attribute-update-passel-fom.xml").wstring();
  unsigned char const tagBytes[] = {0x52U, 0x45U, 0x54U};
  VariableLengthData const tag(tagBytes, sizeof(tagBytes));

  REQUIRE_NOTHROW(publisher->connect(publisherReports, HLA_EVOKED));
  REQUIRE_NOTHROW(immediate->connect(immediateReports, HLA_EVOKED));
  REQUIRE_NOTHROW(constrained->connect(constrainedReports, HLA_EVOKED));
  REQUIRE_NOTHROW(publisher->createFederationExecution(
      federationName,
      fomModule,
      standard_hla::mom::integer64_time));
  FederateHandle publisherHandle;
  REQUIRE_NOTHROW(publisherHandle = publisher->joinFederationExecution(
      L"object-retraction-publisher", L"publisher", federationName));
  REQUIRE_NOTHROW(immediate->joinFederationExecution(
      L"object-retraction-immediate", L"subscriber", federationName));
  REQUIRE_NOTHROW(constrained->joinFederationExecution(
      L"object-retraction-constrained", L"subscriber", federationName));

  auto const objectClass = publisher->getObjectClassHandle(
      fixture_hla::fom::attribute_fixture_child);
  auto const reliable = publisher->getAttributeHandle(
      objectClass,
      fixture_hla::fixture::reliable_base_a);
  auto const bestEffort = publisher->getAttributeHandle(
      objectClass,
      fixture_hla::fixture::best_effort_base);
  REQUIRE(objectClass.isValid());
  REQUIRE(reliable.isValid());
  REQUIRE(bestEffort.isValid());
  AttributeHandleSet const attributes{reliable, bestEffort};
  REQUIRE_NOTHROW(publisher->publishObjectClassAttributes(objectClass, attributes));
  REQUIRE_NOTHROW(immediate->subscribeObjectClassAttributes(objectClass, attributes));
  REQUIRE_NOTHROW(constrained->subscribeObjectClassAttributes(objectClass, attributes));

  ObjectInstanceHandle objectInstance;
  REQUIRE_NOTHROW(objectInstance = publisher->registerObjectInstance(objectClass));
  drainCallbacks(*immediate);
  drainCallbacks(*constrained);
  REQUIRE(immediateReports.objectDiscoveryReports.size() == 1U);
  REQUIRE(constrainedReports.objectDiscoveryReports.size() == 1U);
  auto const objectInstanceName = publisher->getObjectInstanceName(objectInstance);

  REQUIRE_NOTHROW(constrained->enableTimeConstrained());
  drainCallbacks(*constrained);
  REQUIRE_NOTHROW(publisher->enableTimeRegulation(
      rti1516_2025::HLAinteger64Interval(1)));
  drainCallbacks(*publisher);

  // The nonconstrained recipient receives the timestamped deletion as soon as
  // its evoked callback runs; the constrained recipient remains pending.
  auto const retraction = publisher->deleteObjectInstance(
      objectInstance,
      tag,
      rti1516_2025::HLAinteger64Time(2));
  REQUIRE(retraction.isValid());
  REQUIRE_FALSE(immediate->evokeCallback(0.0));
  REQUIRE(immediateReports.objectRemovalReports.size() == 1U);
  REQUIRE(constrainedReports.objectRemovalReports.empty());
  auto const& removal = immediateReports.objectRemovalReports.front();
  REQUIRE(removal.objectInstance == objectInstance);
  REQUIRE(removal.producer == publisherHandle);
  REQUIRE(variableLengthDataBytes(removal.tag) ==
          std::vector<unsigned char>(tagBytes, tagBytes + sizeof(tagBytes)));
  REQUIRE(removal.timeImplementationName == standard_hla::mom::integer64_time);
  REQUIRE(removal.timeValue == L"2");
  REQUIRE(removal.sentOrderType == TIMESTAMP);
  REQUIRE(removal.receivedOrderType == RECEIVE);
  REQUIRE(removal.retractionSupplied);
  REQUIRE(removal.retractionValid);

  // Retract restores the invocation-time object/name/ownership snapshot before
  // queuing Request Retraction for the recipient whose callback was delivered.
  REQUIRE_NOTHROW(publisher->retract(retraction));
  REQUIRE_FALSE(immediate->evokeCallback(0.0));
  REQUIRE(immediateReports.requestRetractionReports.size() == 1U);
  REQUIRE(immediateReports.requestRetractionReports.front().retractionValid);
  REQUIRE(variableLengthDataBytes(
              immediateReports.requestRetractionReports.front().encodedRetraction) ==
          variableLengthDataBytes(retraction.encode()));
  REQUIRE(immediateReports.callbackOrder ==
          std::vector<std::string>{"remove", "request-retraction"});
  REQUIRE(constrainedReports.requestRetractionReports.empty());
  REQUIRE(publisher->getObjectInstanceName(objectInstance) == objectInstanceName);
  REQUIRE(immediate->getObjectInstanceHandle(objectInstanceName) == objectInstance);
  REQUIRE(constrained->getObjectInstanceHandle(objectInstanceName) == objectInstance);
  REQUIRE(publisher->isAttributeOwnedByFederate(objectInstance, reliable));
  REQUIRE(publisher->isAttributeOwnedByFederate(objectInstance, bestEffort));
  REQUIRE_THROWS_AS(
      publisher->retract(retraction),
      rti1516_2025::MessageCanNoLongerBeRetracted);

  // The still-pending constrained fanout was suppressed by the same retract;
  // its advance reaches the grant boundary without a stale removal callback.
  REQUIRE_NOTHROW(constrained->timeAdvanceRequest(
      rti1516_2025::HLAinteger64Time(2)));
  REQUIRE_NOTHROW(publisher->timeAdvanceRequest(
      rti1516_2025::HLAinteger64Time(2)));
  drainCallbacks(*publisher);
  drainCallbacks(*constrained);
  REQUIRE(constrainedReports.objectRemovalReports.empty());
  REQUIRE(constrainedReports.requestRetractionReports.empty());
  REQUIRE(constrainedReports.timeAdvanceGrantReports.size() == 1U);
  REQUIRE(constrainedReports.callbackOrder == std::vector<std::string>{"grant"});

  REQUIRE_NOTHROW(constrained->resignFederationExecution(rti1516_2025::NO_ACTION));
  REQUIRE_NOTHROW(immediate->resignFederationExecution(rti1516_2025::NO_ACTION));
  REQUIRE_NOTHROW(publisher->resignFederationExecution(
      rti1516_2025::CANCEL_THEN_DELETE_THEN_DIVEST));
  REQUIRE_NOTHROW(publisher->destroyFederationExecution(federationName));
  REQUIRE_NOTHROW(constrained->disconnect());
  REQUIRE_NOTHROW(immediate->disconnect());
  REQUIRE_NOTHROW(publisher->disconnect());
}

TEST_CASE(
    "Embedded early NMR grant eases timestamped object deletion bound",
    "[integration][development-profile][object-management][time-management][tso]"
    "[timestamped-delete-early-grant-tso-bound-easing][2025]"
    "[rti.service.delete-object-instance]"
    "[rti.service.next-message-request][rti.service.time-advance-request]"
    "[federate.callback.remove-object-instance][federate.callback.time-advance-grant]") {
  ReportingFederateAmbassador requesterReports;
  ReportingFederateAmbassador peerReports;
  auto requester = makeRti();
  auto peer = makeRti();
  auto const federationName = nextFederationName();
  auto const fomModule = sourcePath(
      std::filesystem::path("cpp") / "tests" / "data" /
      "attribute-update-passel-fom.xml").wstring();
  unsigned char const earlyTagBytes[] = {0x45U, 0x41U};
  unsigned char const outgoingTagBytes[] = {0x44U, 0x45U};
  VariableLengthData const earlyTag(earlyTagBytes, sizeof(earlyTagBytes));
  VariableLengthData const outgoingTag(outgoingTagBytes, sizeof(outgoingTagBytes));

  REQUIRE_NOTHROW(requester->connect(requesterReports, HLA_EVOKED));
  REQUIRE_NOTHROW(peer->connect(peerReports, HLA_EVOKED));
  REQUIRE_NOTHROW(requester->createFederationExecution(
      federationName, fomModule, standard_hla::mom::integer64_time));
  REQUIRE_NOTHROW(requester->joinFederationExecution(
      L"deletion-early-grant-requester", L"publisher", federationName));
  REQUIRE_NOTHROW(peer->joinFederationExecution(
      L"deletion-early-grant-peer", L"publisher", federationName));

  auto const objectClass = requester->getObjectClassHandle(
      fixture_hla::fom::attribute_fixture_child);
  auto const reliable = requester->getAttributeHandle(
      objectClass, fixture_hla::fixture::reliable_base_a);
  auto const bestEffort = requester->getAttributeHandle(
      objectClass, fixture_hla::fixture::best_effort_base);
  REQUIRE(objectClass.isValid());
  REQUIRE(reliable.isValid());
  REQUIRE(bestEffort.isValid());
  AttributeHandleSet const attributes{reliable, bestEffort};
  REQUIRE_NOTHROW(requester->publishObjectClassAttributes(objectClass, attributes));
  REQUIRE_NOTHROW(peer->publishObjectClassAttributes(objectClass, attributes));
  REQUIRE_NOTHROW(requester->subscribeObjectClassAttributes(objectClass, attributes));
  ObjectInstanceHandle const earlyObject = peer->registerObjectInstance(objectClass);
  ObjectInstanceHandle const outgoingObject = requester->registerObjectInstance(objectClass);

  REQUIRE_NOTHROW(requester->enableTimeConstrained());
  drainCallbacks(*requester);
  REQUIRE_NOTHROW(requester->enableTimeRegulation(
      rti1516_2025::HLAinteger64Interval(1)));
  drainCallbacks(*requester);
  REQUIRE_NOTHROW(peer->enableTimeRegulation(
      rti1516_2025::HLAinteger64Interval(1)));
  drainCallbacks(*peer);

  // The queued deletion at 5 supplies an early NMR target for request 10.
  auto const earlyRetraction = peer->deleteObjectInstance(
      earlyObject, earlyTag, rti1516_2025::HLAinteger64Time(5));
  REQUIRE(earlyRetraction.isValid());
  REQUIRE_NOTHROW(requester->nextMessageRequest(
      rti1516_2025::HLAinteger64Time(10)));
  REQUIRE(requesterReports.timeAdvanceGrantReports.empty());
  REQUIRE_NOTHROW(peer->timeAdvanceRequest(rti1516_2025::HLAinteger64Time(4)));
  drainCallbacks(*peer);
  drainCallbacks(*requester);
  REQUIRE(requesterReports.objectRemovalReports.size() == 1U);
  REQUIRE(requesterReports.objectRemovalReports.front().objectInstance == earlyObject);
  REQUIRE(requesterReports.objectRemovalReports.front().timeValue == L"5");
  REQUIRE(requesterReports.timeAdvanceGrantReports.size() == 1U);
  REQUIRE(requesterReports.timeAdvanceGrantReports.front().value == L"5");

  rti1516_2025::HLAinteger64Time logicalTime;
  REQUIRE_NOTHROW(requester->queryLogicalTime(logicalTime));
  REQUIRE(logicalTime.getTime() == 5);

  // At logical time 5 and lookahead 1, timestamp 7 is legal. It is below the
  // stale request-time bound 10 + 1 and therefore witnesses bound easing.
  auto const outgoingRetraction = requester->deleteObjectInstance(
      outgoingObject, outgoingTag, rti1516_2025::HLAinteger64Time(7));
  REQUIRE(outgoingRetraction.isValid());
  REQUIRE_NOTHROW(requester->queryLogicalTime(logicalTime));
  REQUIRE(logicalTime.getTime() == 5);

  REQUIRE_NOTHROW(requester->resignFederationExecution(rti1516_2025::NO_ACTION));
  REQUIRE_NOTHROW(peer->resignFederationExecution(
      rti1516_2025::CANCEL_THEN_DELETE_THEN_DIVEST));
  REQUIRE_NOTHROW(requester->destroyFederationExecution(federationName));
  REQUIRE_NOTHROW(requester->disconnect());
  REQUIRE_NOTHROW(peer->disconnect());
}

TEST_CASE(
    "Embedded Delete Object Instance follows all Table 2 order-classification rows",
    "[integration][development-profile][2025][object-management][time-management]"
    "[delete-object-instance-order-classification-matrix]"
    "[delete-object-instance-preferred-order][tso][receive-order]"
    "[rti.service.delete-object-instance][rti.service.change-attribute-order-type]"
    "[rti.service.enable-time-regulation][federate.callback.remove-object-instance]") {
  ReportingFederateAmbassador publisherReports;
  ReportingFederateAmbassador receiverReports;
  ReportingFederateAmbassador regulatorReports;
  auto publisher = makeRti();
  auto receiver = makeRti();
  auto regulator = makeRti();
  auto const federationName = nextFederationName() + L"-order-matrix";
  auto const fomModule = sourcePath(
      std::filesystem::path("cpp") / "tests" / "data" /
      "attribute-update-passel-fom.xml").wstring();

  REQUIRE_NOTHROW(publisher->connect(publisherReports, HLA_EVOKED));
  REQUIRE_NOTHROW(receiver->connect(receiverReports, HLA_EVOKED));
  REQUIRE_NOTHROW(regulator->connect(regulatorReports, HLA_EVOKED));
  REQUIRE_NOTHROW(publisher->createFederationExecution(
      federationName,
      fomModule,
      standard_hla::mom::integer64_time));
  FederateHandle publisherHandle;
  REQUIRE_NOTHROW(publisherHandle = publisher->joinFederationExecution(
      L"delete-order-matrix-publisher", L"publisher", federationName));
  REQUIRE_NOTHROW(receiver->joinFederationExecution(
      L"delete-order-matrix-receiver", L"subscriber", federationName));
  REQUIRE_NOTHROW(regulator->joinFederationExecution(
      L"delete-order-matrix-regulator", L"regulator", federationName));

  auto const objectClass = publisher->getObjectClassHandle(
      fixture_hla::fom::attribute_fixture_child);
  auto const reliable = publisher->getAttributeHandle(
      objectClass, fixture_hla::fixture::reliable_base_a);
  auto const bestEffort = publisher->getAttributeHandle(
      objectClass, fixture_hla::fixture::best_effort_base);
  auto const privilegeToDelete = publisher->getAttributeHandle(
      objectClass, umbra::test::hla::wide::standard::privilege_to_delete_object);
  REQUIRE(objectClass.isValid());
  REQUIRE(reliable.isValid());
  REQUIRE(bestEffort.isValid());
  REQUIRE(privilegeToDelete.isValid());
  AttributeHandleSet const observedAttributes{reliable, bestEffort};
  REQUIRE_NOTHROW(publisher->publishObjectClassAttributes(
      objectClass, observedAttributes));
  REQUIRE_NOTHROW(receiver->subscribeObjectClassAttributes(
      objectClass, observedAttributes));

  struct MatrixRow final {
    OrderType preferredOrder;
    bool regulating;
    bool timestamped;
  };
  std::vector<MatrixRow> const rows{
      {RECEIVE, false, false},
      {RECEIVE, false, true},
      {TIMESTAMP, false, false},
      {TIMESTAMP, false, true},
      {RECEIVE, true, false},
      {RECEIVE, true, true},
      {TIMESTAMP, true, false},
      {TIMESTAMP, true, true},
  };
  std::vector<ObjectInstanceHandle> objects;
  objects.reserve(rows.size());
  for (std::size_t index = 0U; index != rows.size(); ++index) {
    objects.push_back(publisher->registerObjectInstance(objectClass));
  }
  drainCallbacks(*receiver);
  REQUIRE(receiverReports.objectDiscoveryReports == objects);

  // A separate regulator gives the pre-regulation timestamped calls a valid
  // time bound without changing the sender's role under test.
  REQUIRE_NOTHROW(regulator->enableTimeRegulation(
      rti1516_2025::HLAinteger64Interval(5)));
  drainCallbacks(*regulator);

  std::size_t timestampedCallbacks = 0U;
  std::size_t receiveOrderCallbacks = 0U;
  bool senderRegulationEnabled = false;
  for (std::size_t index = 0U; index != rows.size(); ++index) {
    auto const& row = rows[index];
    if (row.regulating && !senderRegulationEnabled) {
      REQUIRE_NOTHROW(publisher->enableTimeRegulation(
          rti1516_2025::HLAinteger64Interval(1)));
      drainCallbacks(*publisher);
      senderRegulationEnabled = true;
    }

    auto const object = objects[index];
    REQUIRE_NOTHROW(publisher->changeAttributeOrderType(
        object,
        AttributeHandleSet{privilegeToDelete},
        row.preferredOrder));
    unsigned char const tagBytes[] = {
        0x44U, static_cast<unsigned char>(index), 0x32U};
    VariableLengthData const tag(tagBytes, sizeof(tagBytes));
    OrderType const expectedSentOrder =
        row.regulating && row.timestamped && row.preferredOrder == TIMESTAMP
        ? TIMESTAMP
        : RECEIVE;
    bool const expectedRetraction = expectedSentOrder == TIMESTAMP;

    if (row.timestamped) {
      auto const retraction = publisher->deleteObjectInstance(
          object, tag, rti1516_2025::HLAinteger64Time(6));
      REQUIRE(retraction.isValid() == expectedRetraction);
      ++timestampedCallbacks;
      drainCallbacks(*receiver);
      REQUIRE(receiverReports.objectRemovalReports.size() == timestampedCallbacks);
      auto const& removal = receiverReports.objectRemovalReports.back();
      REQUIRE(removal.objectInstance == object);
      REQUIRE(removal.producer == publisherHandle);
      REQUIRE(variableLengthDataBytes(removal.tag) ==
              std::vector<unsigned char>(tagBytes, tagBytes + sizeof(tagBytes)));
      REQUIRE(removal.timeImplementationName == standard_hla::mom::integer64_time);
      REQUIRE(removal.timeValue == L"6");
      REQUIRE(removal.sentOrderType == expectedSentOrder);
      REQUIRE(removal.receivedOrderType == RECEIVE);
      REQUIRE(removal.retractionSupplied == expectedRetraction);
      REQUIRE(removal.retractionValid == expectedRetraction);
    } else {
      REQUIRE_NOTHROW(publisher->deleteObjectInstance(object, tag));
      ++receiveOrderCallbacks;
      drainCallbacks(*receiver);
      REQUIRE(receiverReports.receiveOrderObjectRemovalReports.size() ==
              receiveOrderCallbacks);
      auto const& removal =
          receiverReports.receiveOrderObjectRemovalReports.back();
      REQUIRE(removal.objectInstance == object);
      REQUIRE(removal.producer == publisherHandle);
      REQUIRE(variableLengthDataBytes(removal.tag) ==
              std::vector<unsigned char>(tagBytes, tagBytes + sizeof(tagBytes)));
    }
  }
  REQUIRE(timestampedCallbacks == 4U);
  REQUIRE(receiveOrderCallbacks == 4U);

  REQUIRE_NOTHROW(publisher->resignFederationExecution(
      rti1516_2025::NO_ACTION));
  REQUIRE_NOTHROW(receiver->resignFederationExecution(
      rti1516_2025::NO_ACTION));
  REQUIRE_NOTHROW(regulator->resignFederationExecution(
      rti1516_2025::NO_ACTION));
  REQUIRE_NOTHROW(publisher->destroyFederationExecution(federationName));
  REQUIRE_NOTHROW(receiver->disconnect());
  REQUIRE_NOTHROW(regulator->disconnect());
  REQUIRE_NOTHROW(publisher->disconnect());
}

TEST_CASE(
    "Embedded timestamped Delete Object Instance follows privilege attribute order",
    "[integration][development-profile][2025][object-management][time-management][tso]"
    "[delete-object-instance-preferred-order]"
    "[rti.service.delete-object-instance]"
    "[federate.callback.remove-object-instance]") {
  ReportingFederateAmbassador publisherReports;
  ReportingFederateAmbassador recipientReports;
  auto publisher = makeRti();
  auto recipient = makeRti();
  auto const federationName = nextFederationName();
  auto const fomModule = sourcePath(
      std::filesystem::path("cpp") / "tests" / "data" /
      "attribute-update-passel-fom.xml").wstring();
  unsigned char const tagBytes[] = {0x50U, 0x52U, 0x45U, 0x46U};
  VariableLengthData const tag(tagBytes, sizeof(tagBytes));

  REQUIRE_NOTHROW(publisher->connect(publisherReports, HLA_EVOKED));
  REQUIRE_NOTHROW(recipient->connect(recipientReports, HLA_EVOKED));
  REQUIRE_NOTHROW(publisher->createFederationExecution(
      federationName,
      fomModule,
      standard_hla::mom::integer64_time));
  FederateHandle publisherHandle;
  REQUIRE_NOTHROW(publisherHandle = publisher->joinFederationExecution(
      L"delete-order-publisher", L"publisher", federationName));
  REQUIRE_NOTHROW(recipient->joinFederationExecution(
      L"delete-order-recipient", L"subscriber", federationName));

  auto const objectClass = publisher->getObjectClassHandle(
      fixture_hla::fom::attribute_fixture_child);
  auto const reliable = publisher->getAttributeHandle(
      objectClass, fixture_hla::fixture::reliable_base_a);
  auto const bestEffort = publisher->getAttributeHandle(
      objectClass, fixture_hla::fixture::best_effort_base);
  auto const privilegeToDelete = publisher->getAttributeHandle(
      objectClass, umbra::test::hla::wide::standard::privilege_to_delete_object);
  REQUIRE(objectClass.isValid());
  REQUIRE(reliable.isValid());
  REQUIRE(bestEffort.isValid());
  REQUIRE(privilegeToDelete.isValid());
  AttributeHandleSet const observedAttributes{reliable, bestEffort};
  REQUIRE_NOTHROW(publisher->publishObjectClassAttributes(
      objectClass, observedAttributes));
  REQUIRE_NOTHROW(recipient->subscribeObjectClassAttributes(
      objectClass, observedAttributes));
  ObjectInstanceHandle objectInstance;
  REQUIRE_NOTHROW(objectInstance = publisher->registerObjectInstance(objectClass));
  drainCallbacks(*recipient);
  REQUIRE(recipientReports.objectDiscoveryReports ==
          std::vector<ObjectInstanceHandle>{objectInstance});

  REQUIRE_NOTHROW(publisher->enableTimeRegulation(
      rti1516_2025::HLAinteger64Interval(1)));
  drainCallbacks(*publisher);
  REQUIRE_NOTHROW(publisher->changeAttributeOrderType(
      objectInstance,
      AttributeHandleSet{privilegeToDelete},
      RECEIVE));

  // The timestamped API invocation uses the live preferred order of its
  // HLAprivilegeToDeleteObject attribute, not an unconditional TIMESTAMP.
  auto const retraction = publisher->deleteObjectInstance(
      objectInstance,
      tag,
      rti1516_2025::HLAinteger64Time(2));
  REQUIRE_FALSE(retraction.isValid());
  drainCallbacks(*recipient);
  REQUIRE(recipientReports.objectRemovalReports.size() == 1U);
  auto const& removal = recipientReports.objectRemovalReports.front();
  REQUIRE(removal.objectInstance == objectInstance);
  REQUIRE(removal.producer == publisherHandle);
  REQUIRE(variableLengthDataBytes(removal.tag) ==
          std::vector<unsigned char>(tagBytes, tagBytes + sizeof(tagBytes)));
  REQUIRE(removal.timeImplementationName == standard_hla::mom::integer64_time);
  REQUIRE(removal.timeValue == L"2");
  REQUIRE(removal.sentOrderType == RECEIVE);
  REQUIRE(removal.receivedOrderType == RECEIVE);
  REQUIRE_FALSE(removal.retractionSupplied);

  REQUIRE_NOTHROW(publisher->resignFederationExecution(
      rti1516_2025::NO_ACTION));
  REQUIRE_NOTHROW(recipient->resignFederationExecution(
      rti1516_2025::NO_ACTION));
  REQUIRE_NOTHROW(publisher->destroyFederationExecution(federationName));
  REQUIRE_NOTHROW(recipient->disconnect());
  REQUIRE_NOTHROW(publisher->disconnect());
}

TEST_CASE(
    "Disable Time Constrained delivers queued timestamped object deletions as receive order",
    "[integration][development-profile][2025][object-management][time-management][tso]"
    "[lane.tcd-transition-tso-to-ro][time-constrained-disable-tso-to-ro]"
    "[rti.service.disable-time-constrained][rti.service.delete-object-instance]"
    "[federate.callback.remove-object-instance][requirement-8.7]") {
  ReportingFederateAmbassador publisherReports;
  ReportingFederateAmbassador receiverReports;
  auto publisher = makeRti();
  auto receiver = makeRti();
  auto const federationName = nextFederationName() + L"-tcd-tso-to-ro";
  auto const fomModule = sourcePath(
      std::filesystem::path("cpp") / "tests" / "data" /
      "attribute-update-passel-fom.xml").wstring();
  unsigned char const tagBytes[] = {0x54U, 0x43U, 0x44U, 0xD1U};
  VariableLengthData const tag(tagBytes, sizeof(tagBytes));

  REQUIRE_NOTHROW(publisher->connect(publisherReports, HLA_EVOKED));
  REQUIRE_NOTHROW(receiver->connect(receiverReports, HLA_EVOKED));
  REQUIRE_NOTHROW(publisher->createFederationExecution(
      federationName,
      fomModule,
      standard_hla::mom::integer64_time));
  FederateHandle publisherHandle;
  REQUIRE_NOTHROW(publisherHandle = publisher->joinFederationExecution(
      L"tcd-delete-publisher", L"publisher", federationName));
  REQUIRE_NOTHROW(receiver->joinFederationExecution(
      L"tcd-delete-receiver", L"subscriber", federationName));

  auto const objectClass = publisher->getObjectClassHandle(
      fixture_hla::fom::attribute_fixture_child);
  auto const reliable = publisher->getAttributeHandle(
      objectClass,
      fixture_hla::fixture::reliable_base_a);
  auto const bestEffort = publisher->getAttributeHandle(
      objectClass,
      fixture_hla::fixture::best_effort_base);
  REQUIRE(objectClass.isValid());
  REQUIRE(reliable.isValid());
  REQUIRE(bestEffort.isValid());
  AttributeHandleSet const attributes{reliable, bestEffort};
  REQUIRE_NOTHROW(publisher->publishObjectClassAttributes(objectClass, attributes));
  REQUIRE_NOTHROW(receiver->subscribeObjectClassAttributes(objectClass, attributes));

  ObjectInstanceHandle objectInstance;
  REQUIRE_NOTHROW(objectInstance = publisher->registerObjectInstance(objectClass));
  drainCallbacks(*receiver);
  REQUIRE(receiverReports.objectDiscoveryReports ==
          std::vector<ObjectInstanceHandle>{objectInstance});

  REQUIRE_NOTHROW(receiver->enableTimeConstrained());
  drainCallbacks(*receiver);
  REQUIRE_NOTHROW(publisher->enableTimeRegulation(
      rti1516_2025::HLAinteger64Interval(1)));
  drainCallbacks(*publisher);

  // A valid designator proves the timestamped deletion was accepted while the
  // receiver was time constrained; no callback is eligible before the role
  // transition because the receiver has not advanced to the timestamp.
  auto const retraction = publisher->deleteObjectInstance(
      objectInstance,
      tag,
      rti1516_2025::HLAinteger64Time(2));
  REQUIRE(retraction.isValid());
  REQUIRE(receiverReports.objectRemovalReports.empty());
  REQUIRE(receiverReports.receiveOrderObjectRemovalReports.empty());
  REQUIRE(receiverReports.timeAdvanceGrantReports.empty());

  REQUIRE_NOTHROW(receiver->disableTimeConstrained());
  drainCallbacks(*receiver);
  REQUIRE(receiverReports.receiveOrderObjectRemovalReports.size() == 1U);
  REQUIRE(receiverReports.objectRemovalReports.empty());
  REQUIRE(receiverReports.timeAdvanceGrantReports.empty());
  REQUIRE(receiverReports.callbackOrder == std::vector<std::string>{"remove"});

  auto const& removal = receiverReports.receiveOrderObjectRemovalReports.front();
  REQUIRE(removal.objectInstance == objectInstance);
  REQUIRE(removal.producer == publisherHandle);
  REQUIRE(variableLengthDataBytes(removal.tag) ==
          std::vector<unsigned char>(tagBytes, tagBytes + sizeof(tagBytes)));

  REQUIRE_NOTHROW(receiver->resignFederationExecution(rti1516_2025::NO_ACTION));
  REQUIRE_NOTHROW(publisher->resignFederationExecution(
      rti1516_2025::CANCEL_THEN_DELETE_THEN_DIVEST));
  REQUIRE_NOTHROW(publisher->destroyFederationExecution(federationName));
  REQUIRE_NOTHROW(receiver->disconnect());
  REQUIRE_NOTHROW(publisher->disconnect());
}

TEST_CASE(
    "Embedded distinct FQR grant uses actual time for timestamped object deletion bound",
    "[integration][development-profile][2025][object-management][time-management][tso]"
    "[fqr-distinct-actual-optimistic-delete-bound]"
    "[rti.service.delete-object-instance][rti.service.flush-queue-request]"
    "[rti.service.time-advance-request][federate.callback.remove-object-instance]"
    "[federate.callback.flush-queue-grant]") {
  ReportingFederateAmbassador requesterReports;
  ReportingFederateAmbassador peerReports;
  auto requester = makeRti();
  auto peer = makeRti();
  auto const federationName = nextFederationName();
  auto const fomModule = sourcePath(
      std::filesystem::path("cpp") / "tests" / "data" /
      "attribute-update-passel-fom.xml").wstring();
  unsigned char const incomingTagBytes[] = {0x46U, 0x51U, 0x52U, 0x44U};
  unsigned char const outgoingTagBytes[] = {0x46U, 0x51U, 0x52U, 0x4FU};
  VariableLengthData const incomingTag(incomingTagBytes, sizeof(incomingTagBytes));
  VariableLengthData const outgoingTag(outgoingTagBytes, sizeof(outgoingTagBytes));

  REQUIRE_NOTHROW(requester->connect(requesterReports, HLA_EVOKED));
  REQUIRE_NOTHROW(peer->connect(peerReports, HLA_EVOKED));
  REQUIRE_NOTHROW(requester->createFederationExecution(
      federationName, fomModule, standard_hla::mom::integer64_time));
  REQUIRE_NOTHROW(requester->joinFederationExecution(
      L"deletion-fqr-bound-requester", L"publisher", federationName));
  REQUIRE_NOTHROW(peer->joinFederationExecution(
      L"deletion-fqr-bound-peer", L"publisher", federationName));

  auto const objectClass = requester->getObjectClassHandle(
      fixture_hla::fom::attribute_fixture_child);
  auto const reliable = requester->getAttributeHandle(
      objectClass, fixture_hla::fixture::reliable_base_a);
  auto const bestEffort = requester->getAttributeHandle(
      objectClass, fixture_hla::fixture::best_effort_base);
  REQUIRE(objectClass.isValid());
  REQUIRE(reliable.isValid());
  REQUIRE(bestEffort.isValid());
  AttributeHandleSet const attributes{reliable, bestEffort};
  REQUIRE_NOTHROW(requester->publishObjectClassAttributes(objectClass, attributes));
  REQUIRE_NOTHROW(peer->publishObjectClassAttributes(objectClass, attributes));
  REQUIRE_NOTHROW(requester->subscribeObjectClassAttributes(objectClass, attributes));
  ObjectInstanceHandle const requesterObject = requester->registerObjectInstance(objectClass);
  ObjectInstanceHandle const peerObject = peer->registerObjectInstance(objectClass);
  drainCallbacks(*requester);
  drainCallbacks(*peer);

  REQUIRE_NOTHROW(requester->enableTimeConstrained());
  drainCallbacks(*requester);
  REQUIRE_NOTHROW(requester->enableTimeRegulation(
      rti1516_2025::HLAinteger64Interval(1)));
  drainCallbacks(*requester);
  REQUIRE_NOTHROW(peer->enableTimeRegulation(rti1516_2025::HLAinteger64Interval(1)));
  drainCallbacks(*peer);

  REQUIRE_NOTHROW(requester->timeAdvanceRequest(rti1516_2025::HLAinteger64Time(4)));
  REQUIRE_NOTHROW(peer->timeAdvanceRequest(rti1516_2025::HLAinteger64Time(4)));
  for (int pass = 0; pass != 8; ++pass) {
    drainCallbacks(*requester);
    drainCallbacks(*peer);
  }
  rti1516_2025::HLAinteger64Time logicalTime;
  REQUIRE_NOTHROW(requester->queryLogicalTime(logicalTime));
  REQUIRE(logicalTime.getTime() == 4);
  REQUIRE_NOTHROW(peer->queryLogicalTime(logicalTime));
  REQUIRE(logicalTime.getTime() == 4);
  requesterReports.callbackOrder.clear();

  auto const incomingRetraction = peer->deleteObjectInstance(
      peerObject, incomingTag, rti1516_2025::HLAinteger64Time(7));
  REQUIRE(incomingRetraction.isValid());
  REQUIRE_NOTHROW(requester->flushQueueRequest(rti1516_2025::HLAinteger64Time(10)));
  drainCallbacks(*requester);
  REQUIRE(requesterReports.objectRemovalReports.size() == 1U);
  REQUIRE(requesterReports.objectRemovalReports.front().objectInstance == peerObject);
  REQUIRE(requesterReports.objectRemovalReports.front().timeValue == L"7");
  REQUIRE(requesterReports.flushQueueGrantReports.size() == 1U);
  REQUIRE(requesterReports.flushQueueGrantReports.front().value == L"5");
  REQUIRE(requesterReports.flushQueueGrantReports.front().optimisticValue == L"7");
  REQUIRE(requesterReports.callbackOrder ==
          std::vector<std::string>{"remove", "flush-grant"});

  REQUIRE_NOTHROW(requester->queryLogicalTime(logicalTime));
  REQUIRE(logicalTime.getTime() == 5);
  auto const outgoingRetraction = requester->deleteObjectInstance(
      requesterObject, outgoingTag, rti1516_2025::HLAinteger64Time(6));
  REQUIRE(outgoingRetraction.isValid());
  REQUIRE_NOTHROW(requester->queryLogicalTime(logicalTime));
  REQUIRE(logicalTime.getTime() == 5);
  REQUIRE_THROWS_AS(
      requester->timeAdvanceRequest(rti1516_2025::HLAinteger64Time(6)),
      rti1516_2025::LogicalTimeAlreadyPassed);

  REQUIRE_NOTHROW(requester->resignFederationExecution(
      rti1516_2025::CANCEL_THEN_DELETE_THEN_DIVEST));
  REQUIRE_NOTHROW(peer->resignFederationExecution(rti1516_2025::NO_ACTION));
  REQUIRE_NOTHROW(requester->destroyFederationExecution(federationName));
  REQUIRE_NOTHROW(requester->disconnect());
  REQUIRE_NOTHROW(peer->disconnect());
}
