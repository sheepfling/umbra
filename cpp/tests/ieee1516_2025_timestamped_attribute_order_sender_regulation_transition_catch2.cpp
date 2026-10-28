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
#error "The sender-regulation order test requires the Umbra source directory."
#endif

namespace {

namespace standard_hla = umbra::detail::hla::wide;
namespace fixture_hla = umbra::test::hla::wide;

using rti1516_2025::AttributeHandleSet;
using rti1516_2025::AttributeHandleValueMap;
using rti1516_2025::FederateHandle;
using rti1516_2025::HLA_EVOKED;
using rti1516_2025::LogicalTime;
using rti1516_2025::MessageRetractionHandle;
using rti1516_2025::ObjectClassHandle;
using rti1516_2025::ObjectInstanceHandle;
using rti1516_2025::OrderType;
using rti1516_2025::RECEIVE;
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
  return L"timestamped-attribute-sender-regulation-order-" +
      std::to_wstring(sequence.fetch_add(1U, std::memory_order_relaxed));
}

class OrderReportingFederateAmbassador final
    : public rti1516_2025::NullFederateAmbassador {
 public:
  struct UntimedReflection final {
    ObjectInstanceHandle object;
    AttributeHandleValueMap values;
    VariableLengthData tag;
    TransportationTypeHandle transport;
    FederateHandle producer;
    bool sentRegionsSupplied = false;
  };

  struct Reflection final {
    ObjectInstanceHandle object;
    AttributeHandleValueMap values;
    VariableLengthData tag;
    TransportationTypeHandle transport;
    FederateHandle producer;
    bool sentRegionsSupplied = false;
    std::wstring timeImplementation;
    std::wstring timeValue;
    OrderType sentOrder = RECEIVE;
    OrderType receivedOrder = RECEIVE;
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
    untimedReflections.push_back({
        object,
        values,
        tag,
        transport,
        producer,
        sentRegions != nullptr,
    });
    ++untimedReflectionCount;
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
    reflections.push_back({
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
    callbackOrder.push_back(sentOrder == TIMESTAMP ? "timestamp" : "receive");
  }

  void timeAdvanceGrant(LogicalTime const& time) override {
    grants.push_back(time.toString());
    callbackOrder.push_back("grant");
  }

  std::vector<ObjectInstanceHandle> discoveries;
  std::vector<UntimedReflection> untimedReflections;
  std::vector<Reflection> reflections;
  std::vector<std::wstring> grants;
  std::vector<std::string> callbackOrder;
  int untimedReflectionCount = 0;
};

std::unique_ptr<RTIambassador> makeRti() {
  RTIambassadorFactory factory;
  return factory.createRTIambassador();
}

}  // namespace

TEST_CASE(
    "Embedded timestamped attribute update changes message order when sender becomes time-regulating",
    "[integration][development-profile][object-management][time-management]"
    "[attribute-update-order-classification-matrix]"
    "[timestamped-attribute-sender-regulation-order][tso][receive-order]"
    "[tso-timestamped-attribute-sender-regulation-order]"
    "[time-regulation-enables-tso-sending]"
    "[rti.service.update-attribute-values][rti.service.enable-time-regulation]"
    "[federate.callback.reflect-attribute-values][federate.callback.time-advance-grant]") {
  OrderReportingFederateAmbassador publisherReports;
  OrderReportingFederateAmbassador receiverReports;
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
      L"sender-regulation-order-publisher", L"publisher", federationName));
  REQUIRE_NOTHROW(receiver->joinFederationExecution(
      L"sender-regulation-order-receiver", L"subscriber", federationName));

  auto const child = publisher->getObjectClassHandle(
      fixture_hla::fom::attribute_fixture_child);
  auto const attribute = publisher->getAttributeHandle(
      child, fixture_hla::fixture::reliable_base_a);
  REQUIRE(child.isValid());
  REQUIRE(attribute.isValid());
  AttributeHandleSet const attributes{attribute};
  REQUIRE_NOTHROW(publisher->publishObjectClassAttributes(child, attributes));
  REQUIRE_NOTHROW(receiver->subscribeObjectClassAttributes(child, attributes));
  REQUIRE_NOTHROW(publisher->changeDefaultAttributeOrderType(
      child, attributes, TIMESTAMP));
  ObjectInstanceHandle object;
  REQUIRE_NOTHROW(object = publisher->registerObjectInstance(child));
  REQUIRE_FALSE(receiver->evokeCallback(0.0));
  REQUIRE(receiverReports.discoveries == std::vector<ObjectInstanceHandle>{object});
  REQUIRE_NOTHROW(receiver->enableTimeConstrained());
  REQUIRE_FALSE(receiver->evokeCallback(0.0));
  REQUIRE_NOTHROW(receiver->enableAsynchronousDelivery());
  auto const reliable = publisher->getTransportationTypeHandle(
      standard_hla::mom::reliable);
  REQUIRE(reliable.isValid());

  // Table 2 baseline: RECEIVE preference, non-regulating sender, no timestamp.
  REQUIRE_NOTHROW(publisher->changeAttributeOrderType(
      object, attributes, RECEIVE));
  unsigned char const receiveUntimedBytes[] = {0x52, 0x4F, 0x30};
  unsigned char const receiveUntimedTagBytes[] = {0x30, 0x30};
  AttributeHandleValueMap receiveUntimedValues;
  receiveUntimedValues.emplace(
      attribute,
      VariableLengthData(receiveUntimedBytes, sizeof(receiveUntimedBytes)));
  VariableLengthData const receiveUntimedTag(
      receiveUntimedTagBytes, sizeof(receiveUntimedTagBytes));
  publisher->updateAttributeValues(object, receiveUntimedValues, receiveUntimedTag);
  for (int attempt = 0;
       attempt != 8 && receiverReports.untimedReflections.empty(); ++attempt) {
    receiver->evokeCallback(0.25);
  }
  REQUIRE(receiverReports.untimedReflectionCount == 1);
  REQUIRE(receiverReports.untimedReflections.size() == 1);
  auto const& receiveUntimed = receiverReports.untimedReflections.front();
  REQUIRE(receiveUntimed.object == object);
  REQUIRE(receiveUntimed.values.size() == 1);
  REQUIRE(receiveUntimed.values.contains(attribute));
  REQUIRE(bytesOf(receiveUntimed.values.at(attribute)) ==
          std::vector<unsigned char>(
              receiveUntimedBytes,
              receiveUntimedBytes + sizeof(receiveUntimedBytes)));
  REQUIRE(bytesOf(receiveUntimed.tag) ==
          std::vector<unsigned char>(receiveUntimedTagBytes, receiveUntimedTagBytes + sizeof(receiveUntimedTagBytes)));
  REQUIRE(receiveUntimed.transport == reliable);
  REQUIRE(receiveUntimed.producer == publisherHandle);
  REQUIRE_FALSE(receiveUntimed.sentRegionsSupplied);
  REQUIRE_NOTHROW(publisher->changeAttributeOrderType(
      object, attributes, TIMESTAMP));

  // Table 2 of §8.1.4: preferred TSO plus a supplied timestamp still sends
  // RO while the publisher is not time-regulating; no retraction designator
  // is returned. Table 3 keeps that message RO at a constrained receiver.
  unsigned char const firstBytes[] = {0x41, 0x42};
  unsigned char const firstTagBytes[] = {0x31};
  AttributeHandleValueMap firstValues;
  firstValues.emplace(
      attribute, VariableLengthData(firstBytes, sizeof(firstBytes)));
  VariableLengthData const firstTag(firstTagBytes, sizeof(firstTagBytes));

  // Table 2: TSO preference without regulation or a timestamp still sends RO.
  unsigned char const tsoUntimedBytes[] = {0x54, 0x53, 0x30};
  unsigned char const tsoUntimedTagBytes[] = {0x30, 0x31};
  AttributeHandleValueMap tsoUntimedValues;
  tsoUntimedValues.emplace(
      attribute,
      VariableLengthData(tsoUntimedBytes, sizeof(tsoUntimedBytes)));
  VariableLengthData const tsoUntimedTag(
      tsoUntimedTagBytes, sizeof(tsoUntimedTagBytes));
  publisher->updateAttributeValues(object, tsoUntimedValues, tsoUntimedTag);
  for (int attempt = 0;
       attempt != 8 && receiverReports.untimedReflectionCount != 2; ++attempt) {
    receiver->evokeCallback(0.25);
  }
  REQUIRE(receiverReports.untimedReflectionCount == 2);
  REQUIRE(receiverReports.untimedReflections.size() == 2);
  auto const& tsoUntimed = receiverReports.untimedReflections.back();
  REQUIRE(tsoUntimed.object == object);
  REQUIRE(tsoUntimed.values.size() == 1);
  REQUIRE(tsoUntimed.values.contains(attribute));
  REQUIRE(bytesOf(tsoUntimed.values.at(attribute)) ==
          std::vector<unsigned char>(tsoUntimedBytes, tsoUntimedBytes + sizeof(tsoUntimedBytes)));
  REQUIRE(bytesOf(tsoUntimed.tag) ==
          std::vector<unsigned char>(tsoUntimedTagBytes, tsoUntimedTagBytes + sizeof(tsoUntimedTagBytes)));
  REQUIRE(tsoUntimed.transport == reliable);
  REQUIRE(tsoUntimed.producer == publisherHandle);
  REQUIRE_FALSE(tsoUntimed.sentRegionsSupplied);

  auto const firstRetraction = publisher->updateAttributeValues(
      object, firstValues, firstTag, rti1516_2025::HLAinteger64Time(6));
  REQUIRE_FALSE(firstRetraction.isValid());
  for (int attempt = 0;
       attempt != 8 && receiverReports.reflections.empty(); ++attempt) {
    receiver->evokeCallback(0.25);
  }
  REQUIRE(receiverReports.untimedReflectionCount == 2);
  REQUIRE(receiverReports.reflections.size() == 1);
  REQUIRE(receiverReports.grants.empty());
  REQUIRE(receiverReports.callbackOrder ==
          std::vector<std::string>{"untimed", "untimed", "receive"});
  auto const& first = receiverReports.reflections.front();
  REQUIRE(first.object == object);
  REQUIRE(first.values.size() == 1);
  REQUIRE(first.values.contains(attribute));
  REQUIRE(bytesOf(first.values.at(attribute)) ==
          std::vector<unsigned char>(firstBytes, firstBytes + sizeof(firstBytes)));
  REQUIRE(bytesOf(first.tag) ==
          std::vector<unsigned char>(firstTagBytes, firstTagBytes + sizeof(firstTagBytes)));
  REQUIRE(first.transport == reliable);
  REQUIRE(first.producer == publisherHandle);
  REQUIRE_FALSE(first.sentRegionsSupplied);
  REQUIRE(first.timeImplementation == standard_hla::mom::integer64_time);
  REQUIRE(first.timeValue == L"6");
  REQUIRE(first.sentOrder == RECEIVE);
  REQUIRE(first.receivedOrder == RECEIVE);
  REQUIRE_FALSE(first.retractionSupplied);

  // Table 2: with preferred RECEIVE, the supplied timestamp remains metadata
  // on a receive-order update while the sender is not regulating.
  REQUIRE_NOTHROW(publisher->changeAttributeOrderType(
      object, attributes, RECEIVE));
  unsigned char const receivePreRegBytes[] = {0x52, 0x4F, 0x32};
  unsigned char const receivePreRegTagBytes[] = {0x31, 0x30};
  AttributeHandleValueMap receivePreRegValues;
  receivePreRegValues.emplace(
      attribute, VariableLengthData(receivePreRegBytes, sizeof(receivePreRegBytes)));
  VariableLengthData const receivePreRegTag(
      receivePreRegTagBytes, sizeof(receivePreRegTagBytes));
  auto const receivePreRegRetraction = publisher->updateAttributeValues(
      object,
      receivePreRegValues,
      receivePreRegTag,
      rti1516_2025::HLAinteger64Time(6));
  REQUIRE_FALSE(receivePreRegRetraction.isValid());
  for (int attempt = 0;
       attempt != 8 && receiverReports.reflections.size() != 2; ++attempt) {
    receiver->evokeCallback(0.25);
  }
  REQUIRE(receiverReports.untimedReflectionCount == 2);
  REQUIRE(receiverReports.reflections.size() == 2);
  REQUIRE(receiverReports.callbackOrder == std::vector<std::string>{
      "untimed", "untimed", "receive", "receive"});
  auto const& receivePreReg = receiverReports.reflections.back();
  REQUIRE(receivePreReg.object == object);
  REQUIRE(receivePreReg.values.size() == 1);
  REQUIRE(receivePreReg.values.contains(attribute));
  REQUIRE(bytesOf(receivePreReg.values.at(attribute)) ==
          std::vector<unsigned char>(receivePreRegBytes, receivePreRegBytes + sizeof(receivePreRegBytes)));
  REQUIRE(bytesOf(receivePreReg.tag) ==
          std::vector<unsigned char>(receivePreRegTagBytes, receivePreRegTagBytes + sizeof(receivePreRegTagBytes)));
  REQUIRE(receivePreReg.producer == publisherHandle);
  REQUIRE(receivePreReg.timeImplementation == standard_hla::mom::integer64_time);
  REQUIRE(receivePreReg.timeValue == L"6");
  REQUIRE(receivePreReg.sentOrder == RECEIVE);
  REQUIRE(receivePreReg.receivedOrder == RECEIVE);
  REQUIRE_FALSE(receivePreReg.retractionSupplied);
  REQUIRE_FALSE(receivePreReg.retractionValid);
  REQUIRE_NOTHROW(publisher->changeAttributeOrderType(
      object, attributes, TIMESTAMP));

  // Hold preferred order and timestamp fixed; change only the publisher's
  // time-regulating role. The second message is TSO and cannot arrive before
  // the receiver's time advance.
  REQUIRE_NOTHROW(publisher->enableTimeRegulation(
      rti1516_2025::HLAinteger64Interval(5)));
  while (publisher->evokeCallback(0.0)) {
  }
  // Table 2: preferred TSO with regulation but no timestamp is still RO.
  unsigned char const tsoRegulatingUntimedBytes[] = {0x54, 0x53, 0x31};
  unsigned char const tsoRegulatingUntimedTagBytes[] = {0x32, 0x30};
  AttributeHandleValueMap tsoRegulatingUntimedValues;
  tsoRegulatingUntimedValues.emplace(
      attribute,
      VariableLengthData(
          tsoRegulatingUntimedBytes,
          sizeof(tsoRegulatingUntimedBytes)));
  VariableLengthData const tsoRegulatingUntimedTag(
      tsoRegulatingUntimedTagBytes,
      sizeof(tsoRegulatingUntimedTagBytes));
  publisher->updateAttributeValues(
      object, tsoRegulatingUntimedValues, tsoRegulatingUntimedTag);
  for (int attempt = 0;
       attempt != 8 && receiverReports.untimedReflectionCount != 3; ++attempt) {
    receiver->evokeCallback(0.25);
  }
  REQUIRE(receiverReports.untimedReflectionCount == 3);
  REQUIRE(receiverReports.untimedReflections.size() == 3);
  REQUIRE(receiverReports.reflections.size() == 2);
  auto const& tsoRegulatingUntimed = receiverReports.untimedReflections.back();
  REQUIRE(tsoRegulatingUntimed.object == object);
  REQUIRE(tsoRegulatingUntimed.values.size() == 1);
  REQUIRE(tsoRegulatingUntimed.values.contains(attribute));
  REQUIRE(bytesOf(tsoRegulatingUntimed.values.at(attribute)) ==
          std::vector<unsigned char>(
              tsoRegulatingUntimedBytes,
              tsoRegulatingUntimedBytes + sizeof(tsoRegulatingUntimedBytes)));
  REQUIRE(bytesOf(tsoRegulatingUntimed.tag) ==
          std::vector<unsigned char>(
              tsoRegulatingUntimedTagBytes,
              tsoRegulatingUntimedTagBytes + sizeof(tsoRegulatingUntimedTagBytes)));
  REQUIRE(tsoRegulatingUntimed.transport == reliable);
  REQUIRE(tsoRegulatingUntimed.producer == publisherHandle);
  REQUIRE_FALSE(tsoRegulatingUntimed.sentRegionsSupplied);

  unsigned char const secondBytes[] = {0x54, 0x53, 0x4F};
  unsigned char const secondTagBytes[] = {0x32};
  AttributeHandleValueMap secondValues;
  secondValues.emplace(
      attribute, VariableLengthData(secondBytes, sizeof(secondBytes)));
  VariableLengthData const secondTag(secondTagBytes, sizeof(secondTagBytes));
  auto const secondRetraction = publisher->updateAttributeValues(
      object, secondValues, secondTag, rti1516_2025::HLAinteger64Time(6));
  REQUIRE(secondRetraction.isValid());
  receiver->evokeCallback(0.1);
  REQUIRE(receiverReports.reflections.size() == 2);
  REQUIRE(receiverReports.grants.empty());

  REQUIRE_NOTHROW(receiver->timeAdvanceRequest(
      rti1516_2025::HLAinteger64Time(6)));
  REQUIRE_NOTHROW(publisher->timeAdvanceRequest(
      rti1516_2025::HLAinteger64Time(2)));
  REQUIRE_FALSE(publisher->evokeCallback(0.0));
  for (int attempt = 0;
       attempt != 8 && (receiverReports.reflections.size() != 3 ||
                        receiverReports.grants.empty()); ++attempt) {
    receiver->evokeCallback(0.25);
  }
  REQUIRE(receiverReports.untimedReflectionCount == 3);
  REQUIRE(receiverReports.reflections.size() == 3);
  REQUIRE(receiverReports.grants == std::vector<std::wstring>{L"6"});
  REQUIRE(receiverReports.callbackOrder ==
          std::vector<std::string>{
              "untimed", "untimed", "receive", "receive", "untimed", "timestamp", "grant"});
  auto const& second = receiverReports.reflections.back();
  REQUIRE(second.object == object);
  REQUIRE(second.values.size() == 1);
  REQUIRE(second.values.contains(attribute));
  REQUIRE(bytesOf(second.values.at(attribute)) ==
          std::vector<unsigned char>(secondBytes, secondBytes + sizeof(secondBytes)));
  REQUIRE(bytesOf(second.tag) ==
          std::vector<unsigned char>(secondTagBytes, secondTagBytes + sizeof(secondTagBytes)));
  REQUIRE(second.transport == reliable);
  REQUIRE(second.producer == publisherHandle);
  REQUIRE_FALSE(second.sentRegionsSupplied);
  REQUIRE(second.timeImplementation == standard_hla::mom::integer64_time);
  REQUIRE(second.timeValue == L"6");
  REQUIRE(second.sentOrder == TIMESTAMP);
  REQUIRE(second.receivedOrder == TIMESTAMP);
  REQUIRE(second.retractionSupplied);
  REQUIRE(second.retractionValid);

  for (int attempt = 0;
       attempt != 8 && publisherReports.grants.empty(); ++attempt) {
    publisher->evokeCallback(0.25);
  }
  REQUIRE(publisherReports.grants == std::vector<std::wstring>{L"2"});

  // Table 2: preferred RECEIVE without a timestamp remains RO while regulating.
  REQUIRE_NOTHROW(publisher->changeAttributeOrderType(
      object, attributes, RECEIVE));
  unsigned char const receiveRegulatingUntimedBytes[] = {0x52, 0x4F, 0x33};
  unsigned char const receiveRegulatingUntimedTagBytes[] = {0x33, 0x30};
  AttributeHandleValueMap receiveRegulatingUntimedValues;
  receiveRegulatingUntimedValues.emplace(
      attribute,
      VariableLengthData(
          receiveRegulatingUntimedBytes,
          sizeof(receiveRegulatingUntimedBytes)));
  VariableLengthData const receiveRegulatingUntimedTag(
      receiveRegulatingUntimedTagBytes,
      sizeof(receiveRegulatingUntimedTagBytes));
  publisher->updateAttributeValues(
      object,
      receiveRegulatingUntimedValues,
      receiveRegulatingUntimedTag);
  for (int attempt = 0;
       attempt != 8 && receiverReports.untimedReflectionCount != 4; ++attempt) {
    receiver->evokeCallback(0.25);
  }
  REQUIRE(receiverReports.untimedReflectionCount == 4);
  REQUIRE(receiverReports.untimedReflections.size() == 4);
  REQUIRE(receiverReports.reflections.size() == 3);
  auto const& receiveRegulatingUntimed = receiverReports.untimedReflections.back();
  REQUIRE(receiveRegulatingUntimed.object == object);
  REQUIRE(receiveRegulatingUntimed.values.size() == 1);
  REQUIRE(receiveRegulatingUntimed.values.contains(attribute));
  REQUIRE(bytesOf(receiveRegulatingUntimed.values.at(attribute)) ==
          std::vector<unsigned char>(
              receiveRegulatingUntimedBytes,
              receiveRegulatingUntimedBytes + sizeof(receiveRegulatingUntimedBytes)));
  REQUIRE(bytesOf(receiveRegulatingUntimed.tag) ==
          std::vector<unsigned char>(
              receiveRegulatingUntimedTagBytes,
              receiveRegulatingUntimedTagBytes + sizeof(receiveRegulatingUntimedTagBytes)));
  REQUIRE(receiveRegulatingUntimed.transport == reliable);
  REQUIRE(receiveRegulatingUntimed.producer == publisherHandle);
  REQUIRE_FALSE(receiveRegulatingUntimed.sentRegionsSupplied);

  // A supplied time still remains attached to a RECEIVE-preferred update.
  unsigned char const receiveRegulatingTimedBytes[] = {0x52, 0x4F, 0x34};
  unsigned char const receiveRegulatingTimedTagBytes[] = {0x34, 0x30};
  AttributeHandleValueMap receiveRegulatingTimedValues;
  receiveRegulatingTimedValues.emplace(
      attribute,
      VariableLengthData(
          receiveRegulatingTimedBytes,
          sizeof(receiveRegulatingTimedBytes)));
  VariableLengthData const receiveRegulatingTimedTag(
      receiveRegulatingTimedTagBytes,
      sizeof(receiveRegulatingTimedTagBytes));
  auto const receiveRegulatingTimedRetraction = publisher->updateAttributeValues(
      object,
      receiveRegulatingTimedValues,
      receiveRegulatingTimedTag,
      rti1516_2025::HLAinteger64Time(8));
  REQUIRE_FALSE(receiveRegulatingTimedRetraction.isValid());
  for (int attempt = 0;
       attempt != 8 && receiverReports.reflections.size() != 4; ++attempt) {
    receiver->evokeCallback(0.25);
  }
  REQUIRE(receiverReports.reflections.size() == 4);
  REQUIRE(receiverReports.untimedReflectionCount == 4);
  REQUIRE(receiverReports.grants == std::vector<std::wstring>{L"6"});
  REQUIRE(receiverReports.callbackOrder == std::vector<std::string>{
      "untimed", "untimed", "receive", "receive", "untimed", "timestamp", "grant", "untimed", "receive"});
  auto const& receiveRegulatingTimed = receiverReports.reflections.back();
  REQUIRE(receiveRegulatingTimed.object == object);
  REQUIRE(receiveRegulatingTimed.values.size() == 1);
  REQUIRE(receiveRegulatingTimed.values.contains(attribute));
  REQUIRE(bytesOf(receiveRegulatingTimed.values.at(attribute)) ==
          std::vector<unsigned char>(
              receiveRegulatingTimedBytes,
              receiveRegulatingTimedBytes + sizeof(receiveRegulatingTimedBytes)));
  REQUIRE(bytesOf(receiveRegulatingTimed.tag) ==
          std::vector<unsigned char>(
              receiveRegulatingTimedTagBytes,
              receiveRegulatingTimedTagBytes + sizeof(receiveRegulatingTimedTagBytes)));
  REQUIRE(receiveRegulatingTimed.producer == publisherHandle);
  REQUIRE(receiveRegulatingTimed.timeImplementation == standard_hla::mom::integer64_time);
  REQUIRE(receiveRegulatingTimed.timeValue == L"8");
  REQUIRE(receiveRegulatingTimed.sentOrder == RECEIVE);
  REQUIRE(receiveRegulatingTimed.receivedOrder == RECEIVE);
  REQUIRE_FALSE(receiveRegulatingTimed.retractionSupplied);
  REQUIRE_FALSE(receiveRegulatingTimed.retractionValid);

  receiver->evokeCallback(0.1);
  REQUIRE(receiverReports.untimedReflectionCount == 4);
  REQUIRE(receiverReports.reflections.size() == 4);
  REQUIRE_NOTHROW(receiver->resignFederationExecution(rti1516_2025::NO_ACTION));
  REQUIRE_NOTHROW(publisher->resignFederationExecution(
      rti1516_2025::CANCEL_THEN_DELETE_THEN_DIVEST));
  REQUIRE_NOTHROW(publisher->destroyFederationExecution(federationName));
  REQUIRE_NOTHROW(receiver->disconnect());
  REQUIRE_NOTHROW(publisher->disconnect());
}
