#include <catch2/catch_test_macros.hpp>

#include "hla_test_names.hpp"
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
#error "The mixed-order attribute-passel test requires the Umbra source directory."
#endif

namespace {

namespace standard_hla = umbra::detail::hla::wide;
namespace fixture_hla = umbra::test::hla::wide;

using rti1516_2025::AttributeHandleValueMap;
using rti1516_2025::AttributeHandleSet;
using rti1516_2025::FederateHandle;
using rti1516_2025::HLA_EVOKED;
using rti1516_2025::LogicalTime;
using rti1516_2025::MessageRetractionHandle;
using rti1516_2025::ObjectClassHandle;
using rti1516_2025::ObjectInstanceHandle;
using rti1516_2025::OrderType;
using rti1516_2025::RegionHandleSet;
using rti1516_2025::RTIambassador;
using rti1516_2025::RTIambassadorFactory;
using rti1516_2025::TIMESTAMP;
using rti1516_2025::TransportationTypeHandle;
using rti1516_2025::VariableLengthData;

std::vector<unsigned char> bytesOf(VariableLengthData const& value) {
  auto const* bytes = static_cast<unsigned char const*>(value.data());
  return bytes == nullptr ? std::vector<unsigned char>{}
                          : std::vector<unsigned char>{bytes, bytes + value.size()};
}

std::wstring nextFederationName() {
  static std::atomic_uint64_t sequence{0U};
  return L"mixed-order-attribute-passels-" +
      std::to_wstring(sequence.fetch_add(1U, std::memory_order_relaxed));
}

class PasselFederateAmbassador final : public rti1516_2025::NullFederateAmbassador {
 public:
  struct Reflection final {
    ObjectInstanceHandle object;
    AttributeHandleValueMap values;
    VariableLengthData tag;
    TransportationTypeHandle transport;
    FederateHandle producer;
    bool sentRegionsSupplied = false;
    std::wstring timeImplementation;
    std::wstring timeValue;
    OrderType sentOrder;
    OrderType receivedOrder;
    bool retractionSupplied = false;
    bool retractionValid = false;
  };

  void discoverObjectInstance(
      ObjectInstanceHandle const& object,
      ObjectClassHandle const&,
      std::wstring const&,
      FederateHandle const&) override {
    discoveries.push_back(object);
  }

  void reflectAttributeValues(
      ObjectInstanceHandle const& object,
      AttributeHandleValueMap const& values,
      VariableLengthData const& tag,
      TransportationTypeHandle const& transport,
      FederateHandle const& producer,
      RegionHandleSet const* sentRegions) override {
    untimed.push_back({
        object, values, tag, transport, producer, sentRegions != nullptr});
    callbackOrder.push_back("untimed");
  }

  void reflectAttributeValues(
      ObjectInstanceHandle const& object,
      AttributeHandleValueMap const& values,
      VariableLengthData const& tag,
      TransportationTypeHandle const& transport,
      FederateHandle const& producer,
      RegionHandleSet const* sentRegions,
      LogicalTime const& time,
      OrderType sentOrder,
      OrderType receivedOrder,
      MessageRetractionHandle const* retraction) override {
    timed.push_back({
        object,
        values,
        tag,
        transport,
        producer,
        sentRegions != nullptr,
        time.implementationName(),
        time.toString(),
        sentOrder,
        receivedOrder,
        retraction != nullptr,
        retraction != nullptr && retraction->isValid()});
    callbackOrder.push_back(receivedOrder == TIMESTAMP ? "timestamp" : "receive");
  }

  void timeAdvanceGrant(LogicalTime const& time) override {
    grants.push_back(time.toString());
    callbackOrder.push_back("grant");
  }

  std::vector<ObjectInstanceHandle> discoveries;
  std::vector<Reflection> untimed;
  std::vector<Reflection> timed;
  std::vector<std::wstring> grants;
  std::vector<std::string> callbackOrder;
};

std::unique_ptr<RTIambassador> makeRti() {
  RTIambassadorFactory factory;
  return factory.createRTIambassador();
}

}  // namespace

TEST_CASE(
    "Embedded timestamped Update Attribute Values keeps mixed-order reliable passels separate",
    "[integration][development-profile][object-management][time-management]"
    "[mixed-order-attribute-passels][passelization][tso]"
    "[tso-mixed-order-attribute-passels]"
    "[rti.service.update-attribute-values][rti.service.time-advance-request]"
    "[federate.callback.reflect-attribute-values][federate.callback.time-advance-grant]") {
  PasselFederateAmbassador publisherReports;
  PasselFederateAmbassador receiverReports;
  auto publisher = makeRti();
  auto receiver = makeRti();
  auto const federationName = nextFederationName();
  auto const fomModule =
      (std::filesystem::path(UMBRA_SOURCE_DIRECTORY) / "cpp" / "tests" /
       "data" / "attribute-update-passel-fom.xml")
          .wstring();

  REQUIRE_NOTHROW(publisher->connect(publisherReports, HLA_EVOKED));
  REQUIRE_NOTHROW(receiver->connect(receiverReports, HLA_EVOKED));
  REQUIRE_NOTHROW(publisher->createFederationExecution(
      federationName, fomModule, standard_hla::mom::integer64_time));
  FederateHandle publisherHandle;
  REQUIRE_NOTHROW(publisherHandle = publisher->joinFederationExecution(
      L"mixed-order-publisher", L"publisher", federationName));
  REQUIRE_NOTHROW(receiver->joinFederationExecution(
      L"mixed-order-receiver", L"subscriber", federationName));

  auto const child = publisher->getObjectClassHandle(
      fixture_hla::fom::attribute_fixture_child);
  auto const timestampAttribute = publisher->getAttributeHandle(
      child, fixture_hla::fixture::reliable_base_a);
  auto const receiveAttribute = publisher->getAttributeHandle(
      child, fixture_hla::fixture::reliable_base_b);
  REQUIRE(child.isValid());
  REQUIRE(timestampAttribute.isValid());
  REQUIRE(receiveAttribute.isValid());
  AttributeHandleSet const attributes{timestampAttribute, receiveAttribute};
  REQUIRE_NOTHROW(publisher->publishObjectClassAttributes(child, attributes));
  REQUIRE_NOTHROW(receiver->subscribeObjectClassAttributes(child, attributes));
  // Both attributes are reliable and initially Receive-order in the FOM.
  // Changing only A makes order type, not transport type, the passel boundary.
  REQUIRE_NOTHROW(publisher->changeDefaultAttributeOrderType(
      child, AttributeHandleSet{timestampAttribute}, TIMESTAMP));

  ObjectInstanceHandle object;
  REQUIRE_NOTHROW(object = publisher->registerObjectInstance(child));
  REQUIRE_FALSE(receiver->evokeCallback(0.0));
  REQUIRE(receiverReports.discoveries == std::vector<ObjectInstanceHandle>{object});
  REQUIRE_NOTHROW(receiver->enableTimeConstrained());
  REQUIRE_FALSE(receiver->evokeCallback(0.0));
  // A time-constrained receiver normally holds RO callbacks outside a time
  // advance. Enable the official asynchronous-delivery switch so the RO
  // passel's immediate eligibility can be tested independently of TAR.
  REQUIRE_NOTHROW(receiver->enableAsynchronousDelivery());
  REQUIRE_NOTHROW(publisher->enableTimeRegulation(
      rti1516_2025::HLAinteger64Interval(5)));
  while (publisher->evokeCallback(0.0)) {
  }

  unsigned char const tagBytes[] = {0x51, 0x25};
  unsigned char const timestampBytes[] = {0xA1, 0xA2};
  unsigned char const receiveBytes[] = {0xB1, 0xB2, 0xB3};
  VariableLengthData const tag(tagBytes, sizeof(tagBytes));
  AttributeHandleValueMap values;
  values.emplace(
      timestampAttribute,
      VariableLengthData(timestampBytes, sizeof(timestampBytes)));
  values.emplace(
      receiveAttribute,
      VariableLengthData(receiveBytes, sizeof(receiveBytes)));
  auto const reliable = publisher->getTransportationTypeHandle(
      standard_hla::mom::reliable);
  REQUIRE(reliable.isValid());

  // One timestamped service invocation has two reliable passels. The RO
  // passel is available before the receiver requests a time advance; the TSO
  // passel remains queued. One merged reflection would violate this boundary.
  auto const retraction = publisher->updateAttributeValues(
      object, values, tag, rti1516_2025::HLAinteger64Time(6));
  REQUIRE(retraction.isValid());
  // Subscription/advisory callbacks can precede the reflection in the
  // evoked queue, and callback dispatch can be asynchronous. Allow a bounded
  // wait for this positive receive-order delivery before checking its shape.
  for (int attempt = 0;
       attempt != 8 && receiverReports.timed.empty(); ++attempt) {
    receiver->evokeCallback(0.25);
  }
  REQUIRE(receiverReports.untimed.empty());
  REQUIRE(receiverReports.timed.size() == 1);
  REQUIRE(receiverReports.grants.empty());
  REQUIRE(receiverReports.callbackOrder == std::vector<std::string>{"receive"});

  auto const& receive = receiverReports.timed.front();
  REQUIRE(receive.object == object);
  REQUIRE(receive.values.size() == 1);
  REQUIRE(receive.values.contains(receiveAttribute));
  REQUIRE_FALSE(receive.values.contains(timestampAttribute));
  REQUIRE(bytesOf(receive.values.at(receiveAttribute)) ==
          std::vector<unsigned char>(receiveBytes, receiveBytes + sizeof(receiveBytes)));
  REQUIRE(bytesOf(receive.tag) ==
          std::vector<unsigned char>(tagBytes, tagBytes + sizeof(tagBytes)));
  REQUIRE(receive.transport == reliable);
  REQUIRE(receive.producer == publisherHandle);
  REQUIRE_FALSE(receive.sentRegionsSupplied);
  REQUIRE(receive.timeImplementation == standard_hla::mom::integer64_time);
  REQUIRE(receive.timeValue == L"6");
  REQUIRE(receive.sentOrder == rti1516_2025::RECEIVE);
  REQUIRE(receive.receivedOrder == rti1516_2025::RECEIVE);
  REQUIRE_FALSE(receive.retractionSupplied);

  REQUIRE_NOTHROW(receiver->timeAdvanceRequest(rti1516_2025::HLAinteger64Time(6)));
  REQUIRE_NOTHROW(publisher->timeAdvanceRequest(rti1516_2025::HLAinteger64Time(2)));
  REQUIRE_FALSE(publisher->evokeCallback(0.0));
  REQUIRE_FALSE(receiver->evokeCallback(0.0));
  REQUIRE(receiverReports.untimed.empty());
  REQUIRE(receiverReports.timed.size() == 2);
  REQUIRE(receiverReports.grants == std::vector<std::wstring>{L"6"});
  REQUIRE(receiverReports.callbackOrder ==
          std::vector<std::string>{"receive", "timestamp", "grant"});

  auto const& timestamp = receiverReports.timed.back();
  REQUIRE(timestamp.object == object);
  REQUIRE(timestamp.values.size() == 1);
  REQUIRE(timestamp.values.contains(timestampAttribute));
  REQUIRE_FALSE(timestamp.values.contains(receiveAttribute));
  REQUIRE(bytesOf(timestamp.values.at(timestampAttribute)) ==
          std::vector<unsigned char>(
              timestampBytes, timestampBytes + sizeof(timestampBytes)));
  REQUIRE(bytesOf(timestamp.tag) ==
          std::vector<unsigned char>(tagBytes, tagBytes + sizeof(tagBytes)));
  REQUIRE(timestamp.transport == reliable);
  REQUIRE(timestamp.producer == publisherHandle);
  REQUIRE_FALSE(timestamp.sentRegionsSupplied);
  REQUIRE(timestamp.timeImplementation == standard_hla::mom::integer64_time);
  REQUIRE(timestamp.timeValue == L"6");
  REQUIRE(timestamp.sentOrder == TIMESTAMP);
  REQUIRE(timestamp.receivedOrder == TIMESTAMP);
  REQUIRE(timestamp.retractionSupplied);
  REQUIRE(timestamp.retractionValid);

  // A final bounded pump rules out a second copy arriving after the grant.
  receiver->evokeCallback(0.1);
  REQUIRE(receiverReports.untimed.empty());
  REQUIRE(receiverReports.timed.size() == 2);

  REQUIRE_NOTHROW(receiver->resignFederationExecution(rti1516_2025::NO_ACTION));
  REQUIRE_NOTHROW(publisher->resignFederationExecution(
      rti1516_2025::CANCEL_THEN_DELETE_THEN_DIVEST));
  REQUIRE_NOTHROW(publisher->destroyFederationExecution(federationName));
  REQUIRE_NOTHROW(receiver->disconnect());
  REQUIRE_NOTHROW(publisher->disconnect());
}

TEST_CASE(
    "Embedded retract after the receive-order passel cancels only queued timestamped attributes",
    "[integration][development-profile][object-management][time-management]"
    "[mixed-order-attribute-passels][retraction][tso]"
    "[rti.service.update-attribute-values][rti.service.retract]"
    "[rti.service.time-advance-request][federate.callback.reflect-attribute-values]"
    "[federate.callback.time-advance-grant]") {
  PasselFederateAmbassador publisherReports;
  PasselFederateAmbassador receiverReports;
  auto publisher = makeRti();
  auto receiver = makeRti();
  auto const federationName = nextFederationName();
  auto const fomModule =
      (std::filesystem::path(UMBRA_SOURCE_DIRECTORY) / "cpp" / "tests" /
       "data" / "attribute-update-passel-fom.xml")
          .wstring();

  REQUIRE_NOTHROW(publisher->connect(publisherReports, HLA_EVOKED));
  REQUIRE_NOTHROW(receiver->connect(receiverReports, HLA_EVOKED));
  REQUIRE_NOTHROW(publisher->createFederationExecution(
      federationName, fomModule, standard_hla::mom::integer64_time));
  FederateHandle publisherHandle;
  REQUIRE_NOTHROW(publisherHandle = publisher->joinFederationExecution(
      L"mixed-order-retract-publisher", L"publisher", federationName));
  REQUIRE_NOTHROW(receiver->joinFederationExecution(
      L"mixed-order-retract-receiver", L"subscriber", federationName));

  auto const child = publisher->getObjectClassHandle(
      fixture_hla::fom::attribute_fixture_child);
  auto const timestampAttribute = publisher->getAttributeHandle(
      child, fixture_hla::fixture::reliable_base_a);
  auto const receiveAttribute = publisher->getAttributeHandle(
      child, fixture_hla::fixture::reliable_base_b);
  REQUIRE(child.isValid());
  REQUIRE(timestampAttribute.isValid());
  REQUIRE(receiveAttribute.isValid());
  AttributeHandleSet const attributes{timestampAttribute, receiveAttribute};
  REQUIRE_NOTHROW(publisher->publishObjectClassAttributes(child, attributes));
  REQUIRE_NOTHROW(receiver->subscribeObjectClassAttributes(child, attributes));
  REQUIRE_NOTHROW(publisher->changeDefaultAttributeOrderType(
      child, AttributeHandleSet{timestampAttribute}, TIMESTAMP));
  ObjectInstanceHandle object;
  REQUIRE_NOTHROW(object = publisher->registerObjectInstance(child));
  REQUIRE_FALSE(receiver->evokeCallback(0.0));
  REQUIRE(receiverReports.discoveries == std::vector<ObjectInstanceHandle>{object});
  REQUIRE_NOTHROW(receiver->enableTimeConstrained());
  REQUIRE_FALSE(receiver->evokeCallback(0.0));
  REQUIRE_NOTHROW(receiver->enableAsynchronousDelivery());
  REQUIRE_NOTHROW(publisher->enableTimeRegulation(
      rti1516_2025::HLAinteger64Interval(5)));
  while (publisher->evokeCallback(0.0)) {
  }

  unsigned char const tagBytes[] = {0x61, 0x25};
  unsigned char const timestampBytes[] = {0xC1, 0xC2};
  unsigned char const receiveBytes[] = {0xD1, 0xD2, 0xD3};
  VariableLengthData const tag(tagBytes, sizeof(tagBytes));
  AttributeHandleValueMap values;
  values.emplace(timestampAttribute,
      VariableLengthData(timestampBytes, sizeof(timestampBytes)));
  values.emplace(receiveAttribute,
      VariableLengthData(receiveBytes, sizeof(receiveBytes)));
  auto const retraction = publisher->updateAttributeValues(
      object, values, tag, rti1516_2025::HLAinteger64Time(6));
  REQUIRE(retraction.isValid());
  for (int attempt = 0;
       attempt != 8 && receiverReports.timed.empty(); ++attempt) {
    receiver->evokeCallback(0.25);
  }
  REQUIRE(receiverReports.timed.size() == 1);
  auto const& receive = receiverReports.timed.front();
  REQUIRE(receive.values.size() == 1);
  REQUIRE(receive.values.contains(receiveAttribute));
  REQUIRE_FALSE(receive.values.contains(timestampAttribute));
  REQUIRE(receive.receivedOrder == rti1516_2025::RECEIVE);
  REQUIRE_FALSE(receive.retractionSupplied);
  REQUIRE_NOTHROW(publisher->retract(retraction));

  REQUIRE_NOTHROW(receiver->timeAdvanceRequest(rti1516_2025::HLAinteger64Time(6)));
  REQUIRE_NOTHROW(publisher->timeAdvanceRequest(rti1516_2025::HLAinteger64Time(2)));
  REQUIRE_FALSE(publisher->evokeCallback(0.0));
  for (int attempt = 0;
       attempt != 8 && receiverReports.grants.empty(); ++attempt) {
    receiver->evokeCallback(0.25);
  }
  REQUIRE(receiverReports.timed.size() == 1);
  REQUIRE(receiverReports.grants == std::vector<std::wstring>{L"6"});
  REQUIRE(receiverReports.callbackOrder ==
          std::vector<std::string>{"receive", "grant"});
  receiver->evokeCallback(0.1);
  REQUIRE(receiverReports.timed.size() == 1);
  REQUIRE(receiverReports.callbackOrder ==
          std::vector<std::string>{"receive", "grant"});

  REQUIRE_NOTHROW(receiver->resignFederationExecution(rti1516_2025::NO_ACTION));
  REQUIRE_NOTHROW(publisher->resignFederationExecution(
      rti1516_2025::CANCEL_THEN_DELETE_THEN_DIVEST));
  REQUIRE_NOTHROW(publisher->destroyFederationExecution(federationName));
  REQUIRE_NOTHROW(receiver->disconnect());
  REQUIRE_NOTHROW(publisher->disconnect());
}
