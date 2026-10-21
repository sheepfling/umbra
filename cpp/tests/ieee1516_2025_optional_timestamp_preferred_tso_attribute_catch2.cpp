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
#error "The optional-timestamp attribute test requires the Umbra source directory."
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
  return L"optional-timestamp-preferred-tso-attribute-" +
      std::to_wstring(sequence.fetch_add(1U, std::memory_order_relaxed));
}

class OrderReportingFederateAmbassador final
    : public rti1516_2025::NullFederateAmbassador {
 public:
  explicit OrderReportingFederateAmbassador(
      bool recordTimeConstrainedCallback = false) noexcept
      : recordTimeConstrainedCallback_(recordTimeConstrainedCallback) {}

  struct Reflection final {
    ObjectInstanceHandle object;
    AttributeHandleValueMap values;
    VariableLengthData tag;
    TransportationTypeHandle transport;
    FederateHandle producer;
    bool sentRegionsSupplied = false;
    std::wstring timeImplementation;
    std::wstring timeValue;
    OrderType sentOrder = rti1516_2025::RECEIVE;
    OrderType receivedOrder = rti1516_2025::RECEIVE;
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
    untimed.push_back({object, values, tag, transport, producer,
                       sentRegions != nullptr});
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
    callbackOrder.push_back("timed");
  }

  void timeAdvanceGrant(LogicalTime const& time) override {
    grants.push_back(time.toString());
    callbackOrder.push_back("grant");
  }

  void timeConstrainedEnabled(LogicalTime const& time) override {
    constrainedEnables.push_back(time.toString());
    if (recordTimeConstrainedCallback_) {
      callbackOrder.push_back("time-constrained-enabled");
    }
  }

  std::vector<ObjectInstanceHandle> discoveries;
  std::vector<Reflection> untimed;
  std::vector<Reflection> timed;
  std::vector<std::wstring> grants;
  std::vector<std::wstring> constrainedEnables;
  std::vector<std::string> callbackOrder;

 private:
  bool recordTimeConstrainedCallback_ = false;
};

std::unique_ptr<RTIambassador> makeRti() {
  RTIambassadorFactory factory;
  return factory.createRTIambassador();
}

}  // namespace

TEST_CASE(
    "Embedded time-regulating federate may omit or supply timestamp for a TSO-preferred attribute",
    "[integration][development-profile][object-management][time-management]"
    "[optional-timestamp-preferred-tso-attribute][tso][receive-order]"
    "[tso-optional-timestamp-preferred-attribute]"
    "[rti.service.update-attribute-values][rti.service.enable-time-regulation]"
    "[rti.service.enable-time-constrained][federate.callback.reflect-attribute-values]") {
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
      L"optional-timestamp-publisher", L"publisher", federationName));
  REQUIRE_NOTHROW(receiver->joinFederationExecution(
      L"optional-timestamp-receiver", L"subscriber", federationName));

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
  REQUIRE_NOTHROW(publisher->enableTimeRegulation(
      rti1516_2025::HLAinteger64Interval(5)));
  while (publisher->evokeCallback(0.0)) {
  }
  auto const reliable = publisher->getTransportationTypeHandle(
      standard_hla::mom::reliable);
  REQUIRE(reliable.isValid());

  unsigned char const untimedBytes[] = {0x52, 0x4F};
  unsigned char const untimedTagBytes[] = {0x11};
  AttributeHandleValueMap untimedValues;
  untimedValues.emplace(
      attribute, VariableLengthData(untimedBytes, sizeof(untimedBytes)));
  VariableLengthData const untimedTag(untimedTagBytes, sizeof(untimedTagBytes));

  // The preferred order and both time roles are already established here.
  // The official no-time overload is still valid: the timestamp argument is
  // recommended for intent to send TSO, never mandatory for invocation.
  REQUIRE_NOTHROW(publisher->updateAttributeValues(
      object, untimedValues, untimedTag));
  for (int attempt = 0;
       attempt != 8 && receiverReports.untimed.empty(); ++attempt) {
    receiver->evokeCallback(0.25);
  }
  REQUIRE(receiverReports.untimed.size() == 1);
  REQUIRE(receiverReports.timed.empty());
  REQUIRE(receiverReports.grants.empty());
  REQUIRE(receiverReports.callbackOrder == std::vector<std::string>{"untimed"});
  auto const& untimed = receiverReports.untimed.front();
  REQUIRE(untimed.object == object);
  REQUIRE(untimed.values.size() == 1);
  REQUIRE(untimed.values.contains(attribute));
  REQUIRE(bytesOf(untimed.values.at(attribute)) ==
          std::vector<unsigned char>(untimedBytes, untimedBytes + sizeof(untimedBytes)));
  REQUIRE(bytesOf(untimed.tag) ==
          std::vector<unsigned char>(untimedTagBytes,
                                     untimedTagBytes + sizeof(untimedTagBytes)));
  REQUIRE(untimed.transport == reliable);
  REQUIRE(untimed.producer == publisherHandle);
  REQUIRE_FALSE(untimed.sentRegionsSupplied);

  unsigned char const timedBytes[] = {0x54, 0x53, 0x4F};
  unsigned char const timedTagBytes[] = {0x22};
  AttributeHandleValueMap timedValues;
  timedValues.emplace(
      attribute, VariableLengthData(timedBytes, sizeof(timedBytes)));
  VariableLengthData const timedTag(timedTagBytes, sizeof(timedTagBytes));
  auto const retraction = publisher->updateAttributeValues(
      object, timedValues, timedTag, rti1516_2025::HLAinteger64Time(6));
  REQUIRE(retraction.isValid());
  receiver->evokeCallback(0.1);
  REQUIRE(receiverReports.untimed.size() == 1);
  REQUIRE(receiverReports.timed.empty());
  REQUIRE(receiverReports.grants.empty());

  REQUIRE_NOTHROW(receiver->timeAdvanceRequest(
      rti1516_2025::HLAinteger64Time(6)));
  REQUIRE_NOTHROW(publisher->timeAdvanceRequest(
      rti1516_2025::HLAinteger64Time(2)));
  REQUIRE_FALSE(publisher->evokeCallback(0.0));
  for (int attempt = 0;
       attempt != 8 && (receiverReports.timed.empty() ||
                        receiverReports.grants.empty()); ++attempt) {
    receiver->evokeCallback(0.25);
  }
  REQUIRE(receiverReports.untimed.size() == 1);
  REQUIRE(receiverReports.timed.size() == 1);
  REQUIRE(receiverReports.grants == std::vector<std::wstring>{L"6"});
  REQUIRE(receiverReports.callbackOrder ==
          std::vector<std::string>{"untimed", "timed", "grant"});
  auto const& timed = receiverReports.timed.front();
  REQUIRE(timed.object == object);
  REQUIRE(timed.values.size() == 1);
  REQUIRE(timed.values.contains(attribute));
  REQUIRE(bytesOf(timed.values.at(attribute)) ==
          std::vector<unsigned char>(timedBytes, timedBytes + sizeof(timedBytes)));
  REQUIRE(bytesOf(timed.tag) ==
          std::vector<unsigned char>(timedTagBytes,
                                     timedTagBytes + sizeof(timedTagBytes)));
  REQUIRE(timed.transport == reliable);
  REQUIRE(timed.producer == publisherHandle);
  REQUIRE_FALSE(timed.sentRegionsSupplied);
  REQUIRE(timed.timeImplementation == standard_hla::mom::integer64_time);
  REQUIRE(timed.timeValue == L"6");
  REQUIRE(timed.sentOrder == TIMESTAMP);
  REQUIRE(timed.receivedOrder == TIMESTAMP);
  REQUIRE(timed.retractionSupplied);
  REQUIRE(timed.retractionValid);

  receiver->evokeCallback(0.1);
  REQUIRE(receiverReports.untimed.size() == 1);
  REQUIRE(receiverReports.timed.size() == 1);
  REQUIRE_NOTHROW(receiver->resignFederationExecution(rti1516_2025::NO_ACTION));
  REQUIRE_NOTHROW(publisher->resignFederationExecution(
      rti1516_2025::CANCEL_THEN_DELETE_THEN_DIVEST));
  REQUIRE_NOTHROW(publisher->destroyFederationExecution(federationName));
  REQUIRE_NOTHROW(receiver->disconnect());
  REQUIRE_NOTHROW(publisher->disconnect());
}

TEST_CASE(
    "Time Constrained transition retains a timestamped attribute update as TSO",
    "[integration][development-profile][object-management][time-management]"
    "[lane.tce-transition-stored-tso-attribute]"
    "[rti.service.enable-time-constrained][rti.service.update-attribute-values]"
    "[rti.service.time-advance-request]"
    "[federate.callback.time-constrained-enabled]"
    "[federate.callback.reflect-attribute-values]"
    "[federate.callback.time-advance-grant]") {
  OrderReportingFederateAmbassador publisherReports;
  OrderReportingFederateAmbassador receiverReports{true};
  auto publisher = makeRti();
  auto receiver = makeRti();
  auto const federationName = nextFederationName() + L"-tce-stored-tso";
  auto const fomModule =
      (std::filesystem::path(UMBRA_SOURCE_DIRECTORY) / "cpp" / "tests" /
       "data" / "attribute-update-passel-fom.xml")
          .wstring();
  unsigned char const valueBytes[] = {0x54, 0x53, 0x4F};
  unsigned char const tagBytes[] = {0x54, 0x43, 0x45};
  AttributeHandleValueMap values;
  VariableLengthData const tag(tagBytes, sizeof(tagBytes));

  REQUIRE_NOTHROW(publisher->connect(publisherReports, HLA_EVOKED));
  REQUIRE_NOTHROW(receiver->connect(receiverReports, HLA_EVOKED));
  REQUIRE_NOTHROW(publisher->createFederationExecution(
      federationName, fomModule, standard_hla::mom::integer64_time));
  REQUIRE_NOTHROW(publisher->joinFederationExecution(
      L"tce-attribute-publisher", L"publisher", federationName));
  REQUIRE_NOTHROW(receiver->joinFederationExecution(
      L"tce-attribute-receiver", L"subscriber", federationName));

  auto const objectClass = publisher->getObjectClassHandle(
      fixture_hla::fom::attribute_fixture_child);
  auto const attribute = publisher->getAttributeHandle(
      objectClass, fixture_hla::fixture::reliable_base_a);
  REQUIRE(objectClass.isValid());
  REQUIRE(attribute.isValid());
  AttributeHandleSet const attributes{attribute};
  REQUIRE_NOTHROW(publisher->publishObjectClassAttributes(objectClass, attributes));
  REQUIRE_NOTHROW(receiver->subscribeObjectClassAttributes(objectClass, attributes));
  REQUIRE_NOTHROW(publisher->changeDefaultAttributeOrderType(
      objectClass, attributes, TIMESTAMP));
  auto const object = publisher->registerObjectInstance(objectClass);
  REQUIRE_FALSE(receiver->evokeCallback(0.0));
  REQUIRE(receiverReports.discoveries == std::vector<ObjectInstanceHandle>{object});

  REQUIRE_NOTHROW(publisher->enableTimeRegulation(
      rti1516_2025::HLAinteger64Interval(1)));
  while (publisher->evokeCallback(0.0)) {
  }
  REQUIRE_NOTHROW(receiver->timeAdvanceRequest(rti1516_2025::HLAinteger64Time(5)));
  REQUIRE_FALSE(receiver->evokeCallback(0.0));
  REQUIRE(receiverReports.grants == std::vector<std::wstring>{L"5"});
  receiverReports.callbackOrder.clear();

  REQUIRE_NOTHROW(receiver->enableTimeConstrained());
  REQUIRE_FALSE(receiver->evokeCallback(0.0));
  REQUIRE(receiverReports.constrainedEnables.empty());

  values.emplace(attribute, VariableLengthData(valueBytes, sizeof(valueBytes)));
  auto const retraction = publisher->updateAttributeValues(
      object, values, tag, rti1516_2025::HLAinteger64Time(6));
  REQUIRE(retraction.isValid());
  REQUIRE_FALSE(receiver->evokeCallback(0.0));
  REQUIRE(receiverReports.untimed.empty());
  REQUIRE(receiverReports.timed.empty());

  REQUIRE_NOTHROW(publisher->timeAdvanceRequest(rti1516_2025::HLAinteger64Time(4)));
  REQUIRE_FALSE(publisher->evokeCallback(0.0));
  REQUIRE_FALSE(receiver->evokeCallback(0.0));
  REQUIRE(receiverReports.constrainedEnables.empty());
  REQUIRE(receiverReports.untimed.empty());
  REQUIRE(receiverReports.timed.empty());

  REQUIRE_NOTHROW(publisher->timeAdvanceRequest(rti1516_2025::HLAinteger64Time(5)));
  REQUIRE_FALSE(publisher->evokeCallback(0.0));
  REQUIRE_FALSE(receiver->evokeCallback(0.0));
  REQUIRE(receiverReports.constrainedEnables == std::vector<std::wstring>{L"5"});
  REQUIRE(receiverReports.untimed.empty());
  REQUIRE(receiverReports.timed.empty());

  REQUIRE_NOTHROW(receiver->timeAdvanceRequest(rti1516_2025::HLAinteger64Time(6)));
  for (int attempt = 0;
       attempt != 8 && (receiverReports.timed.empty() ||
                        receiverReports.grants.size() != 2U);
       ++attempt) {
    receiver->evokeCallback(0.25);
  }
  REQUIRE(receiverReports.untimed.empty());
  REQUIRE(receiverReports.timed.size() == 1U);
  REQUIRE(receiverReports.grants == std::vector<std::wstring>{L"5", L"6"});
  REQUIRE(receiverReports.callbackOrder == std::vector<std::string>{
      "time-constrained-enabled", "timed", "grant"});
  auto const& reflection = receiverReports.timed.front();
  REQUIRE(reflection.object == object);
  REQUIRE(reflection.values.size() == 1U);
  REQUIRE(reflection.values.contains(attribute));
  REQUIRE(bytesOf(reflection.values.at(attribute)) ==
          std::vector<unsigned char>(valueBytes, valueBytes + sizeof(valueBytes)));
  REQUIRE(bytesOf(reflection.tag) ==
          std::vector<unsigned char>(tagBytes, tagBytes + sizeof(tagBytes)));
  REQUIRE(reflection.timeValue == L"6");
  REQUIRE(reflection.sentOrder == TIMESTAMP);
  REQUIRE(reflection.receivedOrder == TIMESTAMP);
  REQUIRE(reflection.retractionSupplied);
  REQUIRE(reflection.retractionValid);

  REQUIRE_NOTHROW(receiver->disableTimeConstrained());
  REQUIRE_NOTHROW(publisher->disableTimeRegulation());
  REQUIRE_NOTHROW(receiver->resignFederationExecution(rti1516_2025::NO_ACTION));
  REQUIRE_NOTHROW(publisher->resignFederationExecution(
      rti1516_2025::CANCEL_THEN_DELETE_THEN_DIVEST));
  REQUIRE_NOTHROW(publisher->destroyFederationExecution(federationName));
  REQUIRE_NOTHROW(receiver->disconnect());
  REQUIRE_NOTHROW(publisher->disconnect());
}
