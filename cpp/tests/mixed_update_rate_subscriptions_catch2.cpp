#include <catch2/catch_test_macros.hpp>

#include "internal/fom/hla_names.hpp"
#include "hla_test_names.hpp"

#include <RTI/NullFederateAmbassador.h>
#include <RTI/RTI1516.h>

#include <atomic>
#include <chrono>
#include <filesystem>
#include <fstream>
#include <memory>
#include <string>
#include <thread>
#include <vector>

#ifndef UMBRA_SOURCE_DIRECTORY
#error "The mixed update-rate subscription test requires the Umbra source directory."
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
using rti1516_2025::RTIambassador;
using rti1516_2025::RTIambassadorFactory;
using rti1516_2025::VariableLengthData;

class ScopedTemporaryFile final {
 public:
  ScopedTemporaryFile(
      std::filesystem::path path,
      std::filesystem::path reservationPath)
      : path_(std::move(path)), reservationPath_(std::move(reservationPath)) {}

  ScopedTemporaryFile(ScopedTemporaryFile const&) = delete;
  ScopedTemporaryFile& operator=(ScopedTemporaryFile const&) = delete;

  ~ScopedTemporaryFile() {
    std::error_code ignored;
    std::filesystem::remove(path_, ignored);
    std::filesystem::remove_all(reservationPath_, ignored);
  }

  [[nodiscard]] std::filesystem::path const& path() const noexcept {
    return path_;
  }

 private:
  std::filesystem::path path_;
  std::filesystem::path reservationPath_;
};

ScopedTemporaryFile mixedRateAttributeUpdatePasselModule() {
  auto const source = std::filesystem::path(UMBRA_SOURCE_DIRECTORY) /
      "cpp" / "tests" / "data" / "attribute-update-passel-fom.xml";
  std::ifstream input(source, std::ios::binary);
  REQUIRE(input.good());
  std::string fomText{
      std::istreambuf_iterator<char>(input),
      std::istreambuf_iterator<char>()};

  auto const objectMarker = std::string{"</objects>"};
  auto const objectPosition = fomText.find(objectMarker);
  REQUIRE(objectPosition != std::string::npos);
  fomText.insert(
      objectPosition + objectMarker.size(),
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

  auto const childMarker = std::string{
      "                    <attribute>\n"
      "                        <name>UnownedChild</name>"};
  auto const childPosition = fomText.find(childMarker);
  REQUIRE(childPosition != std::string::npos);
  fomText.insert(
      childPosition,
      "                    <attribute>\n"
      "                        <name>BestEffortChild</name>\n"
      "                        <dataType>HLAunicodeString</dataType>\n"
      "                        <updateType>Static</updateType>\n"
      "                        <updateCondition>Fixture update.</updateCondition>\n"
      "                        <valueRequired>false</valueRequired>\n"
      "                        <ownership>DivestAcquire</ownership>\n"
      "                        <sharing>PublishSubscribe</sharing>\n"
      "                        <transportation>HLAbestEffort</transportation>\n"
      "                        <order>Receive</order>\n"
      "                        <semantics>Second best-effort attribute for mixed update-rate routing.</semantics>\n"
      "                    </attribute>\n");

  static std::atomic_uint64_t counter{0U};
  auto const ticks = std::chrono::steady_clock::now().time_since_epoch().count();
  auto const threadId = std::hash<std::thread::id>{}(std::this_thread::get_id());
  auto const parent = std::filesystem::temp_directory_path();
  for (std::size_t attempt = 0U; attempt != 1024U; ++attempt) {
    auto const stem = "umbra-mixed-update-rate-" + std::to_string(ticks) +
        "-" + std::to_string(threadId) + "-" +
        std::to_string(++counter) + "-" + std::to_string(attempt);
    auto const path = parent / (stem + ".xml");
    auto const reservationPath = parent / (stem + ".reservation");
    std::error_code error;
    if (!std::filesystem::create_directory(reservationPath, error)) {
      if (error && error != std::errc::file_exists) {
        throw std::filesystem::filesystem_error(
            "Unable to reserve a temporary mixed-rate FOM path",
            reservationPath,
            error);
      }
      continue;
    }
    std::ofstream output(path, std::ios::binary | std::ios::trunc);
    REQUIRE(output.good());
    output.write(fomText.data(), static_cast<std::streamsize>(fomText.size()));
    REQUIRE(output.good());
    return ScopedTemporaryFile(path, reservationPath);
  }
  throw std::runtime_error("Unable to reserve a unique temporary mixed-rate FOM path.");
}

std::wstring nextFederationName() {
  static std::atomic_uint64_t counter{0U};
  return L"umbra-mixed-update-rate-" +
      std::to_wstring(counter.fetch_add(1U, std::memory_order_relaxed));
}

class TestFederateAmbassador final : public rti1516_2025::NullFederateAmbassador {
 public:
  void discoverObjectInstance(
      ObjectInstanceHandle const& objectInstance,
      ObjectClassHandle const&,
      std::wstring const&,
      FederateHandle const&) override {
    discoveredObjectInstances.push_back(objectInstance);
  }

  void reflectAttributeValues(
      ObjectInstanceHandle const&,
      AttributeHandleValueMap const& attributeValues,
      VariableLengthData const&,
      rti1516_2025::TransportationTypeHandle const&,
      FederateHandle const& producingFederate,
      rti1516_2025::RegionHandleSet const*) override {
    reflections.push_back(attributeValues);
    producingFederates.push_back(producingFederate);
  }

  void reflectAttributeValues(
      ObjectInstanceHandle const&,
      AttributeHandleValueMap const& attributeValues,
      VariableLengthData const&,
      rti1516_2025::TransportationTypeHandle const&,
      FederateHandle const& producingFederate,
      rti1516_2025::RegionHandleSet const*,
      rti1516_2025::LogicalTime const&,
      rti1516_2025::OrderType,
      rti1516_2025::OrderType,
      rti1516_2025::MessageRetractionHandle const*) override {
    reflections.push_back(attributeValues);
    producingFederates.push_back(producingFederate);
  }

  std::vector<ObjectInstanceHandle> discoveredObjectInstances;
  std::vector<AttributeHandleValueMap> reflections;
  std::vector<FederateHandle> producingFederates;
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
    "Embedded mixed update-rate subscriptions gate each attribute independently",
    "[integration][development-profile][federation-management][object-management]"
    "[update-rate-reduction][update-rate-mixed-attribute-gating][receive-order][attribute-update]"
    "[2025]"
    "[rti.service.connect][rti.service.create-federation-execution]"
    "[rti.service.join-federation-execution][rti.service.get-object-class-handle]"
    "[rti.service.get-attribute-handle][rti.service.publish-object-class-attributes]"
    "[rti.service.subscribe-object-class-attributes][rti.service.register-object-instance]"
    "[rti.service.update-attribute-values][rti.service.evoke-callback]"
    "[rti.service.resign-federation-execution][rti.service.destroy-federation-execution]"
    "[rti.service.disconnect][federate.callback.discover-object-instance]"
    "[federate.callback.reflect-attribute-values]") {
  TestFederateAmbassador publisherReports;
  TestFederateAmbassador receiverReports;
  auto publisher = makeRti();
  auto receiver = makeRti();
  auto const federationName = nextFederationName();
  auto const fomModule = mixedRateAttributeUpdatePasselModule();

  REQUIRE_NOTHROW(publisher->connect(publisherReports, HLA_EVOKED));
  REQUIRE_NOTHROW(receiver->connect(receiverReports, HLA_EVOKED));
  REQUIRE_NOTHROW(publisher->createFederationExecution(
      federationName,
      fomModule.path().wstring(),
      standard_hla::mom::integer64_time));
  FederateHandle publisherHandle;
  REQUIRE_NOTHROW(publisherHandle = publisher->joinFederationExecution(
      L"mixed-update-rate-publisher", L"publisher", federationName));
  REQUIRE_NOTHROW(receiver->joinFederationExecution(
      L"mixed-update-rate-receiver", L"subscriber", federationName));

  auto const child = publisher->getObjectClassHandle(
      fixture_hla::fom::attribute_fixture_child);
  auto const reduced = publisher->getAttributeHandle(
      child,
      fixture_hla::fixture::best_effort_base);
  auto const defaultRate = publisher->getAttributeHandle(
      child,
      fixture_hla::fixture::best_effort_child);
  REQUIRE(child.isValid());
  REQUIRE(reduced.isValid());
  REQUIRE(defaultRate.isValid());

  AttributeHandleSet const attributes{reduced, defaultRate};
  AttributeHandleValueMap values;
  unsigned char const reducedBytes[] = {0x41, 0x42};
  unsigned char const defaultBytes[] = {0x51, 0x52};
  values.emplace(reduced, VariableLengthData(reducedBytes, sizeof(reducedBytes)));
  values.emplace(defaultRate, VariableLengthData(defaultBytes, sizeof(defaultBytes)));

  REQUIRE_NOTHROW(publisher->publishObjectClassAttributes(child, attributes));
  REQUIRE_NOTHROW(receiver->subscribeObjectClassAttributes(
      child,
      AttributeHandleSet{reduced},
      true,
      L"Low"));
  REQUIRE_NOTHROW(receiver->subscribeObjectClassAttributes(
      child,
      AttributeHandleSet{defaultRate},
      true,
      L""));

  ObjectInstanceHandle objectInstance;
  REQUIRE_NOTHROW(objectInstance = publisher->registerObjectInstance(child));
  REQUIRE(objectInstance.isValid());
  drainCallbacks(*receiver);
  REQUIRE(receiverReports.discoveredObjectInstances.size() == 1U);

  auto sendAndDrain = [&] {
    REQUIRE_NOTHROW(publisher->updateAttributeValues(
        objectInstance,
        values,
        VariableLengthData{}));
    drainCallbacks(*receiver);
  };

  sendAndDrain();
  REQUIRE(receiverReports.reflections.size() == 1U);
  REQUIRE(receiverReports.producingFederates.front() == publisherHandle);
  REQUIRE(receiverReports.reflections.front().size() == 2U);
  REQUIRE(receiverReports.reflections.front().contains(reduced));
  REQUIRE(receiverReports.reflections.front().contains(defaultRate));
  REQUIRE(receiverReports.reflections.front().at(reduced).size() == sizeof(reducedBytes));

  // The Low attribute is still inside its reduction interval.  HLAdefault on
  // the other attribute remains independently eligible in the same passel.
  sendAndDrain();
  REQUIRE(receiverReports.reflections.size() == 2U);
  auto const& secondReflection = receiverReports.reflections.back();
  REQUIRE(secondReflection.size() == 1U);
  REQUIRE_FALSE(secondReflection.contains(reduced));
  REQUIRE(secondReflection.contains(defaultRate));
  REQUIRE(secondReflection.at(defaultRate).size() == sizeof(defaultBytes));

  REQUIRE_NOTHROW(receiver->resignFederationExecution(
      rti1516_2025::NO_ACTION));
  REQUIRE_NOTHROW(publisher->resignFederationExecution(
      rti1516_2025::CANCEL_THEN_DELETE_THEN_DIVEST));
  REQUIRE_NOTHROW(publisher->destroyFederationExecution(federationName));
  REQUIRE_NOTHROW(receiver->disconnect());
  REQUIRE_NOTHROW(publisher->disconnect());
}
