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
#include <stdexcept>
#include <system_error>
#include <vector>

#ifndef UMBRA_SOURCE_DIRECTORY
#error "The federation-teardown update-rate test requires the Umbra source directory."
#endif

namespace {

namespace standard_hla = umbra::detail::hla::wide;
namespace fixture_hla = umbra::test::hla::wide;

using rti1516_2025::AttributeHandle;
using rti1516_2025::AttributeHandleSet;
using rti1516_2025::AttributeHandleValueMap;
using rti1516_2025::CANCEL_THEN_DELETE_THEN_DIVEST;
using rti1516_2025::FederateHandle;
using rti1516_2025::HLA_EVOKED;
using rti1516_2025::NO_ACTION;
using rti1516_2025::NullFederateAmbassador;
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

  static std::atomic_uint64_t counter{0U};
  auto const ticks = std::chrono::steady_clock::now().time_since_epoch().count();
  auto const parent = std::filesystem::temp_directory_path();
  for (std::size_t attempt = 0U; attempt != 1024U; ++attempt) {
    auto const stem = "umbra-federation-teardown-update-rate-" +
        std::to_string(ticks) + "-" +
        std::to_string(++counter) + "-" + std::to_string(attempt);
    auto const path = parent / (stem + ".xml");
    auto const reservationPath = parent / (stem + ".reservation");
    std::error_code error;
    if (!std::filesystem::create_directory(reservationPath, error)) {
      if (error && error != std::errc::file_exists) {
        throw std::filesystem::filesystem_error(
            "Unable to reserve a temporary federation-teardown FOM path",
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
  throw std::runtime_error(
      "Unable to reserve a unique federation-teardown FOM path.");
}

class RecordingFederateAmbassador final : public NullFederateAmbassador {
 public:
  void discoverObjectInstance(
      ObjectInstanceHandle const& objectInstance,
      ObjectClassHandle const&,
      std::wstring const&,
      FederateHandle const&) override {
    objectDiscoveryReports.push_back(objectInstance);
  }

  void reflectAttributeValues(
      ObjectInstanceHandle const&,
      AttributeHandleValueMap const& attributeValues,
      VariableLengthData const&,
      rti1516_2025::TransportationTypeHandle const&,
      FederateHandle const&,
      rti1516_2025::RegionHandleSet const*) override {
    attributeReflectionReports.push_back(attributeValues);
  }

  std::vector<ObjectInstanceHandle> objectDiscoveryReports;
  std::vector<AttributeHandleValueMap> attributeReflectionReports;
};

std::unique_ptr<RTIambassador> makeRti() {
  RTIambassadorFactory factory;
  return factory.createRTIambassador();
}

std::wstring nextFederationName() {
  static std::atomic_uint64_t counter{0U};
  return L"umbra-federation-teardown-update-rate-" +
      std::to_wstring(counter.fetch_add(1U, std::memory_order_relaxed));
}

}  // namespace

TEST_CASE(
    "Embedded federation teardown preserves update-rate history for another live federation",
    "[integration][development-profile][federation-management][object-management]"
    "[update-rate-reduction][receive-order][attribute-update][federation-isolation]"
    "[teardown-isolation][2025]"
    "[rti.service.connect][rti.service.create-federation-execution]"
    "[rti.service.join-federation-execution][rti.service.get-object-class-handle]"
    "[rti.service.get-attribute-handle][rti.service.publish-object-class-attributes]"
    "[rti.service.subscribe-object-class-attributes][rti.service.register-object-instance]"
    "[rti.service.update-attribute-values][rti.service.evoke-callback]"
    "[rti.service.resign-federation-execution][rti.service.destroy-federation-execution]"
    "[rti.service.disconnect][federate.callback.discover-object-instance]"
    "[federate.callback.reflect-attribute-values]") {
  RecordingFederateAmbassador publisherAReports;
  RecordingFederateAmbassador receiverAReports;
  RecordingFederateAmbassador publisherBReports;
  RecordingFederateAmbassador receiverBReports;
  auto publisherA = makeRti();
  auto receiverA = makeRti();
  auto publisherB = makeRti();
  auto receiverB = makeRti();
  // A is a textual prefix of B. Teardown must identify the exact execution,
  // rather than clearing B's live admission history while removing A.
  auto const federationA = nextFederationName() + L"-rate";
  auto const federationB = federationA + L"/child";
  auto const fomModule = lowRateAttributeUpdatePasselModule();

  REQUIRE_NOTHROW(publisherA->connect(publisherAReports, HLA_EVOKED));
  REQUIRE_NOTHROW(receiverA->connect(receiverAReports, HLA_EVOKED));
  REQUIRE_NOTHROW(publisherB->connect(publisherBReports, HLA_EVOKED));
  REQUIRE_NOTHROW(receiverB->connect(receiverBReports, HLA_EVOKED));
  REQUIRE_NOTHROW(publisherA->createFederationExecution(
      federationA,
      fomModule.path().wstring(),
      standard_hla::mom::integer64_time));
  REQUIRE_NOTHROW(publisherB->createFederationExecution(
      federationB,
      fomModule.path().wstring(),
      standard_hla::mom::integer64_time));
  REQUIRE_NOTHROW(publisherA->joinFederationExecution(
      L"update-rate-isolation-publisher-a", L"publisher-a", federationA));
  REQUIRE_NOTHROW(receiverA->joinFederationExecution(
      L"update-rate-isolation-receiver-a", L"receiver-a", federationA));
  REQUIRE_NOTHROW(publisherB->joinFederationExecution(
      L"update-rate-isolation-publisher-b", L"publisher-b", federationB));
  REQUIRE_NOTHROW(receiverB->joinFederationExecution(
      L"update-rate-isolation-receiver-b", L"receiver-b", federationB));

  auto const childA = publisherA->getObjectClassHandle(
      fixture_hla::fom::attribute_fixture_child);
  auto const childB = publisherB->getObjectClassHandle(
      fixture_hla::fom::attribute_fixture_child);
  auto const attributeA = publisherA->getAttributeHandle(
      childA,
      fixture_hla::fixture::best_effort_base);
  auto const attributeB = publisherB->getAttributeHandle(
      childB,
      fixture_hla::fixture::best_effort_base);
  REQUIRE(childA.isValid());
  REQUIRE(childB.isValid());
  REQUIRE(attributeA.isValid());
  REQUIRE(attributeB.isValid());

  REQUIRE_NOTHROW(publisherA->publishObjectClassAttributes(
      childA,
      AttributeHandleSet{attributeA}));
  REQUIRE_NOTHROW(receiverA->subscribeObjectClassAttributes(
      childA,
      AttributeHandleSet{attributeA},
      true,
      L"Low"));
  REQUIRE_NOTHROW(publisherB->publishObjectClassAttributes(
      childB,
      AttributeHandleSet{attributeB}));
  REQUIRE_NOTHROW(receiverB->subscribeObjectClassAttributes(
      childB,
      AttributeHandleSet{attributeB},
      true,
      L"Low"));

  ObjectInstanceHandle objectA;
  ObjectInstanceHandle objectB;
  REQUIRE_NOTHROW(objectA = publisherA->registerObjectInstance(childA));
  REQUIRE_NOTHROW(objectB = publisherB->registerObjectInstance(childB));
  while (receiverA->evokeCallback(0.0)) {
  }
  while (receiverB->evokeCallback(0.0)) {
  }
  REQUIRE(receiverAReports.objectDiscoveryReports.size() == 1U);
  REQUIRE(receiverBReports.objectDiscoveryReports.size() == 1U);

  auto send = [](RTIambassador& publisher,
                 ObjectInstanceHandle const& object,
                 AttributeHandle const& attribute,
                 unsigned char value) {
    AttributeHandleValueMap values;
    values.emplace(attribute, VariableLengthData(&value, sizeof(value)));
    REQUIRE_NOTHROW(publisher.updateAttributeValues(
        object,
        values,
        VariableLengthData{}));
  };
  auto drain = [](RTIambassador& receiver) {
    while (receiver.evokeCallback(0.0)) {
    }
  };

  // Establish independent Low-rate admission history in both executions.
  send(*publisherA, objectA, attributeA, 0x11U);
  send(*publisherB, objectB, attributeB, 0x21U);
  drain(*receiverA);
  drain(*receiverB);
  REQUIRE(receiverAReports.attributeReflectionReports.size() == 1U);
  REQUIRE(receiverBReports.attributeReflectionReports.size() == 1U);

  // The 0.2 Hz interval suppresses each immediate second update.
  send(*publisherA, objectA, attributeA, 0x12U);
  send(*publisherB, objectB, attributeB, 0x22U);
  drain(*receiverA);
  drain(*receiverB);
  REQUIRE(receiverAReports.attributeReflectionReports.size() == 1U);
  REQUIRE(receiverBReports.attributeReflectionReports.size() == 1U);

  // Destroying A must not clear B's live admission history.
  REQUIRE_NOTHROW(receiverA->resignFederationExecution(NO_ACTION));
  REQUIRE_NOTHROW(publisherA->resignFederationExecution(
      CANCEL_THEN_DELETE_THEN_DIVEST));
  REQUIRE_NOTHROW(publisherA->destroyFederationExecution(federationA));

  send(*publisherB, objectB, attributeB, 0x23U);
  drain(*receiverB);
  REQUIRE(receiverBReports.attributeReflectionReports.size() == 1U);

  REQUIRE_NOTHROW(receiverB->resignFederationExecution(NO_ACTION));
  REQUIRE_NOTHROW(publisherB->resignFederationExecution(
      CANCEL_THEN_DELETE_THEN_DIVEST));
  REQUIRE_NOTHROW(publisherB->destroyFederationExecution(federationB));
  REQUIRE_NOTHROW(receiverA->disconnect());
  REQUIRE_NOTHROW(publisherA->disconnect());
  REQUIRE_NOTHROW(receiverB->disconnect());
  REQUIRE_NOTHROW(publisherB->disconnect());
}
