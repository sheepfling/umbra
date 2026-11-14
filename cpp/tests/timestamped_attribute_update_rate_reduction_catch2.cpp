#include <catch2/catch_test_macros.hpp>
#include <catch2/catch_translate_exception.hpp>

#include "internal/fom/hla_names.hpp"
#include "hla_test_names.hpp"

#include <RTI/NullFederateAmbassador.h>
#include <RTI/RTI1516.h>
#include <RTI/time/HLAinteger64Interval.h>
#include <RTI/time/HLAinteger64Time.h>

#include <atomic>
#include <filesystem>
#include <fstream>
#include <memory>
#include <string>
#include <vector>

CATCH_TRANSLATE_EXCEPTION(rti1516_2025::Exception const& error) {
  auto narrow = [](std::wstring const& value) {
    std::string result;
    result.reserve(value.size());
    for (auto const character : value) {
      result.push_back(character < 128 ? static_cast<char>(character) : '?');
    }
    return result;
  };
  return narrow(error.name()) + ": " + narrow(error.what());
}

#ifndef UMBRA_SOURCE_DIRECTORY
#error "The timestamped update-rate reduction test requires the Umbra source directory."
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
  return L"timestamped-attribute-update-rate-reduction-" +
      std::to_wstring(sequence.fetch_add(1U, std::memory_order_relaxed));
}

class ScopedTemporaryFile final {
 public:
  explicit ScopedTemporaryFile(std::filesystem::path path) : path_(std::move(path)) {}

  ScopedTemporaryFile(ScopedTemporaryFile const&) = delete;
  ScopedTemporaryFile& operator=(ScopedTemporaryFile const&) = delete;

  ~ScopedTemporaryFile() {
    std::error_code ignored;
    std::filesystem::remove(path_, ignored);
  }

  [[nodiscard]] std::filesystem::path const& path() const noexcept {
    return path_;
  }

 private:
  std::filesystem::path path_;
};

ScopedTemporaryFile lowRateAttributeUpdatePasselModule() {
  auto const source = std::filesystem::path(UMBRA_SOURCE_DIRECTORY) /
      "cpp" / "tests" / "data" / "attribute-update-passel-fom.xml";
  std::ifstream input(source, std::ios::binary);
  REQUIRE(input.good());
  std::string fomText{
      std::istreambuf_iterator<char>(input),
      std::istreambuf_iterator<char>()};
  auto const marker = std::string{"</objects>"};
  auto const insertion = fomText.find(marker);
  REQUIRE(insertion != std::string::npos);
  fomText.insert(
      insertion + marker.size(),
      "\n    <updateRates>\n"
      "        <updateRate>\n"
      "            <name>Low</name>\n"
      "            <rate>0.2</rate>\n"
      "        </updateRate>\n"
      "        <updateRate>\n"
      "            <name>High</name>\n"
      "            <rate>30</rate>\n"
      "        </updateRate>\n"
      "    </updateRates>");

  static std::atomic_uint64_t sequence{0U};
  auto const path = std::filesystem::temp_directory_path() /
      ("umbra-update-rate-attribute-passel-" +
       std::to_string(sequence.fetch_add(1U, std::memory_order_relaxed)) +
       ".xml");
  std::ofstream output(path, std::ios::binary | std::ios::trunc);
  REQUIRE(output.good());
  output.write(fomText.data(), static_cast<std::streamsize>(fomText.size()));
  REQUIRE(output.good());
  return ScopedTemporaryFile(path);
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
    std::wstring timeImplementationName;
    std::wstring value;
  };

  struct TransportationChangeConfirmationReport final {
    ObjectInstanceHandle objectInstance;
    AttributeHandleSet attributes;
    TransportationTypeHandle transportationType;
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

  void confirmAttributeTransportationTypeChange(
      ObjectInstanceHandle const& objectInstance,
      AttributeHandleSet const& changedAttributes,
      TransportationTypeHandle const& transportationType) override {
    transportationChangeConfirmations.push_back(
        {objectInstance, changedAttributes, transportationType});
  }

  std::vector<ObjectInstanceHandle> discoveredObjectInstances;
  std::vector<ReflectionReport> attributeReflectionReports;
  std::vector<TimeAdvanceGrantReport> timeAdvanceGrantReports;
  std::vector<TransportationChangeConfirmationReport> transportationChangeConfirmations;
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
    "Embedded timestamped attribute reduction suppresses excess best-effort but retains reliable",
    "[integration][development-profile][object-management][time-management]"
    "[timestamped-attribute-update][update-rate-reduction][tso]"
    "[rti.service.request-attribute-transportation-type-change]"
    "[rti.service.subscribe-object-class-attributes][rti.service.update-attribute-values]"
    "[federate.callback.confirm-attribute-transportation-type-change]"
    "[federate.callback.reflect-attribute-values][federate.callback.time-advance-grant]") {
  ReportingFederateAmbassador publisherReports;
  ReportingFederateAmbassador receiverReports;
  auto publisher = makeRti();
  auto receiver = makeRti();
  auto const federationName = nextFederationName();
  auto const fomModule = lowRateAttributeUpdatePasselModule();

  REQUIRE_NOTHROW(publisher->connect(publisherReports, HLA_EVOKED));
  REQUIRE_NOTHROW(receiver->connect(receiverReports, HLA_EVOKED));
  REQUIRE_NOTHROW(publisher->createFederationExecution(
      federationName,
      fomModule.path().wstring(),
      standard_hla::mom::integer64_time));
  FederateHandle publisherHandle;
  REQUIRE_NOTHROW(publisherHandle = publisher->joinFederationExecution(
      L"timestamped-update-rate-publisher", L"publisher", federationName));
  REQUIRE_NOTHROW(receiver->joinFederationExecution(
      L"timestamped-update-rate-receiver", L"subscriber", federationName));

  auto const child = publisher->getObjectClassHandle(
      fixture_hla::fom::attribute_fixture_child);
  auto const reliable = publisher->getAttributeHandle(
      child,
      fixture_hla::fixture::reliable_base_a);
  auto const bestEffort = publisher->getAttributeHandle(
      child,
      fixture_hla::fixture::best_effort_base);
  REQUIRE(child.isValid());
  REQUIRE(reliable.isValid());
  REQUIRE(bestEffort.isValid());

  AttributeHandleSet const attributes{reliable, bestEffort};
  AttributeHandleValueMap values;
  unsigned char const reliableBytes[] = {0x71, 0x72};
  unsigned char const bestEffortBytes[] = {0x81, 0x82};
  values.emplace(reliable, VariableLengthData(reliableBytes, sizeof(reliableBytes)));
  values.emplace(bestEffort, VariableLengthData(bestEffortBytes, sizeof(bestEffortBytes)));

  REQUIRE_NOTHROW(publisher->publishObjectClassAttributes(child, attributes));
  REQUIRE_NOTHROW(receiver->subscribeObjectClassAttributes(child, AttributeHandleSet{reliable}));
  // Low is an active FDD-defined rate: it affects delivery eligibility at
  // the reflection boundary while the reliable attribute remains unthrottled.
  REQUIRE_NOTHROW(receiver->subscribeObjectClassAttributes(
      child,
      AttributeHandleSet{bestEffort},
      true,
      L"Low"));
  REQUIRE_NOTHROW(publisher->changeDefaultAttributeOrderType(
      child,
      attributes,
      TIMESTAMP));

  ObjectInstanceHandle objectInstance;
  REQUIRE_NOTHROW(objectInstance = publisher->registerObjectInstance(child));
  drainCallbacks(*receiver);
  REQUIRE(receiverReports.discoveredObjectInstances.size() == 1U);

  REQUIRE_NOTHROW(receiver->enableTimeConstrained());
  drainCallbacks(*receiver);
  REQUIRE_NOTHROW(publisher->enableTimeRegulation(
      rti1516_2025::HLAinteger64Interval(5)));
  drainCallbacks(*publisher);

  unsigned char const tagBytes[] = {0x91};
  VariableLengthData const tag(tagBytes, sizeof(tagBytes));
  auto const first = publisher->updateAttributeValues(
      objectInstance,
      values,
      tag,
      rti1516_2025::HLAinteger64Time(6));
  REQUIRE(first.isValid());
  REQUIRE_NOTHROW(receiver->timeAdvanceRequest(
      rti1516_2025::HLAinteger64Time(6)));
  REQUIRE_NOTHROW(publisher->timeAdvanceRequest(
      rti1516_2025::HLAinteger64Time(2)));
  drainCallbacks(*publisher);
  drainCallbacks(*receiver);
  REQUIRE(receiverReports.attributeReflectionReports.size() == 2U);
  REQUIRE(receiverReports.timeAdvanceGrantReports.size() == 1U);
  REQUIRE(receiverReports.timeAdvanceGrantReports.back().value == L"6");

  auto countAttribute = [&](AttributeHandle const handle) {
    std::size_t count = 0;
    for (auto const& report : receiverReports.attributeReflectionReports) {
      if (report.attributeValues.contains(handle)) {
        ++count;
      }
    }
    return count;
  };
  REQUIRE(countAttribute(reliable) == 1U);
  REQUIRE(countAttribute(bestEffort) == 1U);
  REQUIRE(receiverReports.attributeReflectionReports.front().producingFederate == publisherHandle);

  auto const second = publisher->updateAttributeValues(
      objectInstance,
      values,
      tag,
      rti1516_2025::HLAinteger64Time(7));
  REQUIRE(second.isValid());
  REQUIRE_NOTHROW(receiver->timeAdvanceRequest(
      rti1516_2025::HLAinteger64Time(7)));
  REQUIRE_NOTHROW(publisher->timeAdvanceRequest(
      rti1516_2025::HLAinteger64Time(3)));
  drainCallbacks(*publisher);
  drainCallbacks(*receiver);
  REQUIRE(receiverReports.attributeReflectionReports.size() == 3U);
  REQUIRE(receiverReports.timeAdvanceGrantReports.size() == 2U);
  REQUIRE(receiverReports.timeAdvanceGrantReports.back().value == L"7");
  REQUIRE(countAttribute(reliable) == 2U);
  REQUIRE(countAttribute(bestEffort) == 1U);
  REQUIRE(receiverReports.attributeReflectionReports.back().producingFederate == publisherHandle);

  // A message containing only the throttled passel still consumes its TSO
  // recipient boundary without inducing a user callback.  Advance the
  // producer far enough that GALT permits the receiver to grant through 8;
  // at that point the producer's retraction lower bound has also passed 8.
  AttributeHandleValueMap bestEffortOnly;
  bestEffortOnly.emplace(bestEffort, values.at(bestEffort));
  auto const suppressed = publisher->updateAttributeValues(
      objectInstance,
      bestEffortOnly,
      tag,
      rti1516_2025::HLAinteger64Time(8));
  REQUIRE(suppressed.isValid());
  REQUIRE_NOTHROW(receiver->timeAdvanceRequest(
      rti1516_2025::HLAinteger64Time(8)));
  REQUIRE_NOTHROW(publisher->timeAdvanceRequest(
      rti1516_2025::HLAinteger64Time(4)));
  drainCallbacks(*publisher);
  drainCallbacks(*receiver);
  REQUIRE(receiverReports.attributeReflectionReports.size() == 3U);
  REQUIRE(receiverReports.timeAdvanceGrantReports.size() == 3U);
  REQUIRE(receiverReports.timeAdvanceGrantReports.back().value == L"8");
  REQUIRE_THROWS_AS(
      publisher->retract(suppressed),
      rti1516_2025::MessageCanNoLongerBeRetracted);

  auto const reliableTransportation =
      publisher->getTransportationTypeHandle(L"HLAreliable");
  REQUIRE(reliableTransportation.isValid());
  AttributeHandleSet const changedTransportationAttributes{bestEffort};
  REQUIRE_NOTHROW(publisher->requestAttributeTransportationTypeChange(
      objectInstance,
      changedTransportationAttributes,
      reliableTransportation));
  drainCallbacks(*publisher);
  REQUIRE(publisherReports.transportationChangeConfirmations.size() == 1U);
  auto const& transportChange =
      publisherReports.transportationChangeConfirmations.front();
  REQUIRE(transportChange.objectInstance == objectInstance);
  REQUIRE(transportChange.attributes == changedTransportationAttributes);
  REQUIRE(transportChange.transportationType == reliableTransportation);
  REQUIRE(publisher->getTransportationTypeName(transportChange.transportationType) ==
          L"HLAreliable");

  auto const afterChangeFirst = publisher->updateAttributeValues(
      objectInstance,
      bestEffortOnly,
      tag,
      rti1516_2025::HLAinteger64Time(9));
  REQUIRE(afterChangeFirst.isValid());
  REQUIRE_NOTHROW(receiver->timeAdvanceRequest(rti1516_2025::HLAinteger64Time(9)));
  REQUIRE_NOTHROW(publisher->timeAdvanceRequest(rti1516_2025::HLAinteger64Time(5)));
  drainCallbacks(*publisher);
  drainCallbacks(*receiver);
  REQUIRE(receiverReports.attributeReflectionReports.size() == 4U);
  REQUIRE(receiverReports.timeAdvanceGrantReports.size() == 4U);
  REQUIRE(receiverReports.timeAdvanceGrantReports.back().value == L"9");
  REQUIRE(countAttribute(reliable) == 2U);
  REQUIRE(countAttribute(bestEffort) == 2U);
  auto const& firstChangedReflection =
      receiverReports.attributeReflectionReports.back();
  REQUIRE(firstChangedReflection.objectInstance == objectInstance);
  REQUIRE(firstChangedReflection.attributeValues.size() == 1U);
  REQUIRE(firstChangedReflection.attributeValues.contains(bestEffort));
  REQUIRE(firstChangedReflection.transportationType == reliableTransportation);
  REQUIRE(firstChangedReflection.timeValue == L"9");

  auto const afterChangeSecond = publisher->updateAttributeValues(
      objectInstance,
      bestEffortOnly,
      tag,
      rti1516_2025::HLAinteger64Time(10));
  REQUIRE(afterChangeSecond.isValid());
  REQUIRE_NOTHROW(receiver->timeAdvanceRequest(rti1516_2025::HLAinteger64Time(10)));
  REQUIRE_NOTHROW(publisher->timeAdvanceRequest(rti1516_2025::HLAinteger64Time(6)));
  drainCallbacks(*publisher);
  drainCallbacks(*receiver);
  REQUIRE(receiverReports.attributeReflectionReports.size() == 5U);
  REQUIRE(receiverReports.timeAdvanceGrantReports.size() == 5U);
  REQUIRE(receiverReports.timeAdvanceGrantReports.back().value == L"10");
  REQUIRE(countAttribute(reliable) == 2U);
  REQUIRE(countAttribute(bestEffort) == 3U);
  auto const& secondChangedReflection =
      receiverReports.attributeReflectionReports.back();
  REQUIRE(secondChangedReflection.objectInstance == objectInstance);
  REQUIRE(secondChangedReflection.attributeValues.size() == 1U);
  REQUIRE(secondChangedReflection.attributeValues.contains(bestEffort));
  REQUIRE(secondChangedReflection.transportationType == reliableTransportation);
  REQUIRE(secondChangedReflection.timeValue == L"10");

  REQUIRE_NOTHROW(receiver->resignFederationExecution(rti1516_2025::NO_ACTION));
  REQUIRE_NOTHROW(publisher->resignFederationExecution(
      rti1516_2025::CANCEL_THEN_DELETE_THEN_DIVEST));
  REQUIRE_NOTHROW(publisher->destroyFederationExecution(federationName));
  REQUIRE_NOTHROW(receiver->disconnect());
  REQUIRE_NOTHROW(publisher->disconnect());
}
