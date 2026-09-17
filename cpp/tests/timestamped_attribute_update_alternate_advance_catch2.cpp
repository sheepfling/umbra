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
#error "The alternate-advance timestamped attribute-update test requires the Umbra source directory."
#endif

namespace {

namespace standard_hla = umbra::detail::hla::wide;
namespace fixture_hla = umbra::test::hla::wide;

using rti1516_2025::AttributeHandle;
using rti1516_2025::AttributeHandleSet;
using rti1516_2025::AttributeHandleValueMap;
using rti1516_2025::FederateHandle;
using rti1516_2025::HLA_EVOKED;
using rti1516_2025::ObjectClassHandle;
using rti1516_2025::ObjectInstanceHandle;
using rti1516_2025::OrderType;
using rti1516_2025::RECEIVE;
using rti1516_2025::RTIambassador;
using rti1516_2025::RTIambassadorFactory;
using rti1516_2025::TIMESTAMP;
using rti1516_2025::TransportationTypeHandle;
using rti1516_2025::VariableLengthData;

std::wstring nextFederationName() {
  static std::atomic_uint64_t sequence{0U};
  return L"timestamped-attribute-available-alternate-advance-" +
      std::to_wstring(sequence.fetch_add(1U, std::memory_order_relaxed));
}

std::filesystem::path resourcePath(std::filesystem::path const& relativePath) {
  return std::filesystem::path(UMBRA_SOURCE_DIRECTORY) /
      "third_party" / "ieee1516.2-2025" / "resources" / relativePath;
}

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
  struct ReflectionReport final {
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

  struct TimeAdvanceGrantReport final {
    std::wstring implementationName;
    std::wstring value;
  };

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
      VariableLengthData const& userSuppliedTag,
      TransportationTypeHandle const& transportationType,
      FederateHandle const& producingFederate,
      rti1516_2025::RegionHandleSet const*,
      rti1516_2025::LogicalTime const& time,
      OrderType sentOrderType,
      OrderType receivedOrderType,
      rti1516_2025::MessageRetractionHandle const* optionalRetraction) override {
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
    callbackOrder.push_back("reflect");
  }

  void timeAdvanceGrant(rti1516_2025::LogicalTime const& time) override {
    timeAdvanceGrantReports.push_back({time.implementationName(), time.toString()});
    callbackOrder.push_back("grant");
  }

  std::vector<ObjectInstanceHandle> discoveredObjectInstances;
  std::vector<ReflectionReport> attributeReflectionReports;
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

void drainMultipleCallbacks(RTIambassador& rti) {
  for (int pass = 0; pass != 128; ++pass) {
    if (!rti.evokeMultipleCallbacks(0.0, 0.0)) {
      break;
    }
  }
}

}  // namespace

TEST_CASE(
    "Embedded timestamped attribute updates preserve ordering through available advances",
    "[integration][development-profile][object-management][time-management]"
    "[timestamped-attribute-update][tso][alternate-advance][multi-federate-callback-ordering]"
    "[rti.service.publish-object-class-attributes][rti.service.subscribe-object-class-attributes]"
    "[rti.service.register-object-instance][rti.service.change-attribute-order-type]"
    "[rti.service.update-attribute-values][rti.service.enable-time-regulation]"
    "[rti.service.enable-time-constrained][rti.service.time-advance-request]"
    "[rti.service.time-advance-request-available][rti.service.next-message-request-available]"
    "[federate.callback.reflect-attribute-values][federate.callback.time-advance-grant]") {
  ReportingFederateAmbassador publisherReports;
  ReportingFederateAmbassador taraReports;
  ReportingFederateAmbassador nmraReports;
  auto publisher = makeRti();
  auto tara = makeRti();
  auto nmra = makeRti();
  auto const federationName = nextFederationName();
  auto const fomModule = resourcePath("examples/RestaurantFOMmodule-2025.xml");
  unsigned char const earlyValueBytes[] = {0x41, 0x56, 0x2D, 0x35};
  unsigned char const equalValueBytes[] = {0x41, 0x56, 0x2D, 0x35, 0x45};
  unsigned char const lateValueBytes[] = {0x41, 0x56, 0x2D, 0x37};
  unsigned char const earlyTagBytes[] = {0x54, 0x41};
  unsigned char const equalTagBytes[] = {0x54, 0x45};
  unsigned char const lateTagBytes[] = {0x54, 0x37};
  VariableLengthData const earlyTag(earlyTagBytes, sizeof(earlyTagBytes));
  VariableLengthData const equalTag(equalTagBytes, sizeof(equalTagBytes));
  VariableLengthData const lateTag(lateTagBytes, sizeof(lateTagBytes));

  REQUIRE_NOTHROW(publisher->connect(publisherReports, HLA_EVOKED));
  REQUIRE_NOTHROW(tara->connect(taraReports, HLA_EVOKED));
  REQUIRE_NOTHROW(nmra->connect(nmraReports, HLA_EVOKED));
  REQUIRE_NOTHROW(publisher->createFederationExecution(
      federationName,
      fomModule.wstring(),
      standard_hla::mom::integer64_time));
  FederateHandle publisherHandle;
  REQUIRE_NOTHROW(publisherHandle = publisher->joinFederationExecution(
      L"timestamp-attribute-available-publisher", L"publisher", federationName));
  REQUIRE_NOTHROW(tara->joinFederationExecution(
      L"timestamp-attribute-available-tara", L"tara", federationName));
  REQUIRE_NOTHROW(nmra->joinFederationExecution(
      L"timestamp-attribute-available-nmra", L"nmra", federationName));

  auto const publisherClass = publisher->getObjectClassHandle(
      fixture_hla::fom::employee_server);
  auto const taraClass = tara->getObjectClassHandle(fixture_hla::fom::employee_server);
  auto const nmraClass = nmra->getObjectClassHandle(fixture_hla::fom::employee_server);
  auto const publisherEfficiency = publisher->getAttributeHandle(
      publisherClass,
      fixture_hla::fixture::efficiency);
  auto const taraEfficiency = tara->getAttributeHandle(
      taraClass,
      fixture_hla::fixture::efficiency);
  auto const nmraEfficiency = nmra->getAttributeHandle(
      nmraClass,
      fixture_hla::fixture::efficiency);
  AttributeHandleSet const publisherAttributes{publisherEfficiency};
  REQUIRE(publisherClass.isValid());
  REQUIRE(publisherEfficiency.isValid());
  REQUIRE(taraEfficiency.isValid());
  REQUIRE(nmraEfficiency.isValid());

  REQUIRE_NOTHROW(publisher->publishObjectClassAttributes(
      publisherClass,
      publisherAttributes));
  REQUIRE_NOTHROW(tara->subscribeObjectClassAttributes(
      taraClass,
      AttributeHandleSet{taraEfficiency}));
  REQUIRE_NOTHROW(nmra->subscribeObjectClassAttributes(
      nmraClass,
      AttributeHandleSet{nmraEfficiency}));
  ObjectInstanceHandle objectInstance;
  REQUIRE_NOTHROW(objectInstance = publisher->registerObjectInstance(publisherClass));
  drainCallbacks(*tara);
  drainCallbacks(*nmra);
  REQUIRE(taraReports.discoveredObjectInstances.size() == 1U);
  REQUIRE(nmraReports.discoveredObjectInstances.size() == 1U);
  taraReports.callbackOrder.clear();
  nmraReports.callbackOrder.clear();

  REQUIRE_NOTHROW(publisher->changeAttributeOrderType(
      objectInstance,
      publisherAttributes,
      TIMESTAMP));
  REQUIRE_NOTHROW(tara->enableTimeConstrained());
  drainCallbacks(*tara);
  REQUIRE_NOTHROW(nmra->enableTimeConstrained());
  drainCallbacks(*nmra);
  REQUIRE_NOTHROW(publisher->enableTimeRegulation(
      rti1516_2025::HLAinteger64Interval(1)));
  drainCallbacks(*publisher);

  AttributeHandleValueMap earlyValues;
  earlyValues.emplace(
      publisherEfficiency,
      VariableLengthData(earlyValueBytes, sizeof(earlyValueBytes)));
  AttributeHandleValueMap equalValues;
  equalValues.emplace(
      publisherEfficiency,
      VariableLengthData(equalValueBytes, sizeof(equalValueBytes)));
  AttributeHandleValueMap lateValues;
  lateValues.emplace(
      publisherEfficiency,
      VariableLengthData(lateValueBytes, sizeof(lateValueBytes)));
  auto const lateRetraction = publisher->updateAttributeValues(
      objectInstance,
      lateValues,
      lateTag,
      rti1516_2025::HLAinteger64Time(7));
  auto const earlyRetraction = publisher->updateAttributeValues(
      objectInstance,
      earlyValues,
      earlyTag,
      rti1516_2025::HLAinteger64Time(5));
  auto const equalRetraction = publisher->updateAttributeValues(
      objectInstance,
      equalValues,
      equalTag,
      rti1516_2025::HLAinteger64Time(5));
  REQUIRE(lateRetraction.isValid());
  REQUIRE(earlyRetraction.isValid());
  REQUIRE(equalRetraction.isValid());
  REQUIRE(taraReports.attributeReflectionReports.empty());
  REQUIRE(nmraReports.attributeReflectionReports.empty());

  // TARA admits the exact timestamp-5 boundary while NMRA selects the next
  // queued message below its requested upper limit. Both recipients receive
  // the complete equal-timestamp cohort before their grant.
  REQUIRE_NOTHROW(tara->timeAdvanceRequestAvailable(
      rti1516_2025::HLAinteger64Time(5)));
  REQUIRE_NOTHROW(nmra->nextMessageRequestAvailable(
      rti1516_2025::HLAinteger64Time(10)));
  REQUIRE_NOTHROW(publisher->timeAdvanceRequest(
      rti1516_2025::HLAinteger64Time(4)));
  drainCallbacks(*publisher);
  drainMultipleCallbacks(*tara);
  drainMultipleCallbacks(*nmra);

  REQUIRE(taraReports.attributeReflectionReports.size() == 2U);
  REQUIRE(nmraReports.attributeReflectionReports.size() == 2U);
  REQUIRE(taraReports.timeAdvanceGrantReports.size() == 1U);
  REQUIRE(nmraReports.timeAdvanceGrantReports.size() == 1U);
  REQUIRE(taraReports.timeAdvanceGrantReports.front().value == L"5");
  REQUIRE(nmraReports.timeAdvanceGrantReports.front().value == L"5");
  REQUIRE(taraReports.callbackOrder ==
          std::vector<std::string>{"reflect", "reflect", "grant"});
  REQUIRE(nmraReports.callbackOrder ==
          std::vector<std::string>{"reflect", "reflect", "grant"});
  REQUIRE(taraReports.attributeReflectionReports[0].timeValue == L"5");
  REQUIRE(taraReports.attributeReflectionReports[1].timeValue == L"5");
  REQUIRE(nmraReports.attributeReflectionReports[0].timeValue == L"5");
  REQUIRE(nmraReports.attributeReflectionReports[1].timeValue == L"5");

  auto const earlyBytes = std::vector<unsigned char>(
      earlyValueBytes,
      earlyValueBytes + sizeof(earlyValueBytes));
  auto const equalBytes = std::vector<unsigned char>(
      equalValueBytes,
      equalValueBytes + sizeof(equalValueBytes));
  auto const earlyTagVector = std::vector<unsigned char>(
      earlyTagBytes,
      earlyTagBytes + sizeof(earlyTagBytes));
  auto const equalTagVector = std::vector<unsigned char>(
      equalTagBytes,
      equalTagBytes + sizeof(equalTagBytes));
  auto verifyCohort = [&](auto const& reports, AttributeHandle const& attribute) {
    auto const& first = reports.attributeReflectionReports[0];
    auto const& second = reports.attributeReflectionReports[1];
    auto const firstValue = variableLengthDataBytes(first.attributeValues.at(attribute));
    auto const secondValue = variableLengthDataBytes(second.attributeValues.at(attribute));
    auto const firstTag = variableLengthDataBytes(first.userSuppliedTag);
    auto const secondTag = variableLengthDataBytes(second.userSuppliedTag);
    REQUIRE(((firstValue == earlyBytes && secondValue == equalBytes) ||
             (firstValue == equalBytes && secondValue == earlyBytes)));
    REQUIRE(((firstTag == earlyTagVector && secondTag == equalTagVector) ||
             (firstTag == equalTagVector && secondTag == earlyTagVector)));
    REQUIRE(first.producingFederate == publisherHandle);
    REQUIRE(second.producingFederate == publisherHandle);
    REQUIRE(first.sentOrderType == TIMESTAMP);
    REQUIRE(second.sentOrderType == TIMESTAMP);
    REQUIRE(first.receivedOrderType == TIMESTAMP);
    REQUIRE(second.receivedOrderType == TIMESTAMP);
    REQUIRE(first.retractionValid);
    REQUIRE(second.retractionValid);
  };
  verifyCohort(taraReports, taraEfficiency);
  verifyCohort(nmraReports, nmraEfficiency);

  // The same alternate-advance forms then admit the distinct timestamp-7
  // record, retaining callback-before-grant ordering for each recipient.
  REQUIRE_NOTHROW(tara->timeAdvanceRequestAvailable(
      rti1516_2025::HLAinteger64Time(7)));
  REQUIRE_NOTHROW(nmra->nextMessageRequestAvailable(
      rti1516_2025::HLAinteger64Time(10)));
  REQUIRE_NOTHROW(publisher->timeAdvanceRequest(
      rti1516_2025::HLAinteger64Time(6)));
  drainCallbacks(*publisher);
  drainMultipleCallbacks(*tara);
  drainMultipleCallbacks(*nmra);
  REQUIRE(taraReports.attributeReflectionReports.size() == 3U);
  REQUIRE(nmraReports.attributeReflectionReports.size() == 3U);
  REQUIRE(taraReports.attributeReflectionReports.back().timeValue == L"7");
  REQUIRE(nmraReports.attributeReflectionReports.back().timeValue == L"7");
  REQUIRE(variableLengthDataBytes(
              taraReports.attributeReflectionReports.back().attributeValues.at(taraEfficiency)) ==
          std::vector<unsigned char>(lateValueBytes, lateValueBytes + sizeof(lateValueBytes)));
  REQUIRE(variableLengthDataBytes(
              nmraReports.attributeReflectionReports.back().attributeValues.at(nmraEfficiency)) ==
          std::vector<unsigned char>(lateValueBytes, lateValueBytes + sizeof(lateValueBytes)));
  REQUIRE(variableLengthDataBytes(taraReports.attributeReflectionReports.back().userSuppliedTag) ==
          std::vector<unsigned char>(lateTagBytes, lateTagBytes + sizeof(lateTagBytes)));
  REQUIRE(variableLengthDataBytes(nmraReports.attributeReflectionReports.back().userSuppliedTag) ==
          std::vector<unsigned char>(lateTagBytes, lateTagBytes + sizeof(lateTagBytes)));
  REQUIRE(taraReports.timeAdvanceGrantReports.size() == 2U);
  REQUIRE(nmraReports.timeAdvanceGrantReports.size() == 2U);
  REQUIRE(taraReports.timeAdvanceGrantReports.back().value == L"7");
  REQUIRE(nmraReports.timeAdvanceGrantReports.back().value == L"7");
  REQUIRE(taraReports.callbackOrder ==
          std::vector<std::string>{"reflect", "reflect", "grant", "reflect", "grant"});
  REQUIRE(nmraReports.callbackOrder ==
          std::vector<std::string>{"reflect", "reflect", "grant", "reflect", "grant"});

  REQUIRE_NOTHROW(tara->unsubscribeObjectClassAttributes(
      taraClass,
      AttributeHandleSet{taraEfficiency}));
  REQUIRE_NOTHROW(nmra->unsubscribeObjectClassAttributes(
      nmraClass,
      AttributeHandleSet{nmraEfficiency}));
  REQUIRE_NOTHROW(nmra->resignFederationExecution(rti1516_2025::NO_ACTION));
  REQUIRE_NOTHROW(tara->resignFederationExecution(rti1516_2025::NO_ACTION));
  REQUIRE_NOTHROW(publisher->resignFederationExecution(
      rti1516_2025::CANCEL_THEN_DELETE_THEN_DIVEST));
  REQUIRE_NOTHROW(publisher->destroyFederationExecution(federationName));
  REQUIRE_NOTHROW(nmra->disconnect());
  REQUIRE_NOTHROW(tara->disconnect());
  REQUIRE_NOTHROW(publisher->disconnect());
}
