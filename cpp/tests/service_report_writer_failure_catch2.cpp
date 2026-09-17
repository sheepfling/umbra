#include <catch2/catch_test_macros.hpp>

#include "internal/observability/service_report_store.hpp"
#include "internal/runtime/umbra_rti_ambassador.hpp"

#include <RTI/NullFederateAmbassador.h>
#include <RTI/RTI1516.h>

#include <atomic>
#include <filesystem>
#include <memory>
#include <stdexcept>
#include <string>
#include <vector>

#ifndef UMBRA_SOURCE_DIRECTORY
#error "The service-report writer failure test requires the Umbra source directory."
#endif

namespace {

using rti1516_2025::HLA_EVOKED;
using rti1516_2025::NO_ACTION;
using rti1516_2025::NullFederateAmbassador;
using rti1516_2025::RTIambassador;
using rti1516_2025::RTIambassadorFactory;
using rti1516_2025::umbra_binding_detail::UmbraRtiAmbassador;

class FailingServiceReportStore final : public umbra::detail::ServiceReportStore {
 public:
  [[nodiscard]] std::unique_ptr<umbra::detail::ServiceReportWriter>
  createForJoinedFederate(
      umbra::detail::JoinedFederateReportDescriptor const& descriptor) override {
    static_cast<void>(descriptor);
    ++createCalls;
    throw std::runtime_error("intentional service-report store failure");
  }

  std::size_t createCalls = 0U;
};

class FailingAppendServiceReportWriter final : public umbra::detail::ServiceReportWriter {
 public:
  explicit FailingAppendServiceReportWriter(std::size_t& appendCalls)
      : appendCalls_(appendCalls) {}

  [[nodiscard]] std::filesystem::path location() const override { return {}; }

  void append(std::wstring_view encodedRecord) override {
    static_cast<void>(encodedRecord);
    ++appendCalls_;
    throw std::runtime_error("intentional service-report append failure");
  }

 private:
  std::size_t& appendCalls_;
};

class FailingAppendServiceReportStore final : public umbra::detail::ServiceReportStore {
 public:
  [[nodiscard]] std::unique_ptr<umbra::detail::ServiceReportWriter>
  createForJoinedFederate(
      umbra::detail::JoinedFederateReportDescriptor const& descriptor) override {
    ++createCalls;
    initialRecords.push_back(descriptor.initialRecord);
    return std::make_unique<FailingAppendServiceReportWriter>(appendCalls);
  }

  std::size_t createCalls = 0U;
  std::size_t appendCalls = 0U;
  std::vector<std::wstring> initialRecords;
};

std::unique_ptr<RTIambassador> makeRti() {
  RTIambassadorFactory factory;
  return factory.createRTIambassador();
}

std::wstring nextFederationName() {
  static std::atomic_uint64_t sequence{0U};
  return L"service-report-writer-failure-" +
      std::to_wstring(sequence.fetch_add(1U, std::memory_order_relaxed));
}

std::filesystem::path resourcePath(std::filesystem::path const& relativePath) {
  return std::filesystem::path(UMBRA_SOURCE_DIRECTORY) / "cpp" / "tests" /
      "data" / relativePath;
}

}  // namespace

TEST_CASE(
    "Service-report writer creation failure rejects the join without an in-memory fallback",
    "[integration][development-profile][federation-management][mom]"
    "[service-report-store][service-reporting][failure-handling]"
    "[no-memory-fallback][join-rollback][2025]"
    "[rti.service.connect][rti.service.create-federation-execution]"
    "[rti.service.join-federation-execution][rti.service.resign-federation-execution]"
    "[rti.service.destroy-federation-execution][rti.service.disconnect]") {
  NullFederateAmbassador failingReports;
  NullFederateAmbassador peerReports;
  auto failingStore = std::make_unique<FailingServiceReportStore>();
  auto* const observedStore = failingStore.get();
  auto failingRti = std::make_unique<UmbraRtiAmbassador>(
      UmbraRtiAmbassador::ServiceReportStoreTestSeam{},
      std::move(failingStore));
  auto peer = makeRti();
  auto const federationName = nextFederationName();
  auto const fomModule = resourcePath("switch-nrg-disabled-fom.xml").wstring();

  REQUIRE_NOTHROW(failingRti->connect(failingReports, HLA_EVOKED));
  REQUIRE_NOTHROW(peer->connect(peerReports, HLA_EVOKED));
  REQUIRE_NOTHROW(failingRti->createFederationExecution(
      federationName, fomModule, L"HLAinteger64Time"));
  REQUIRE_THROWS_AS(
      failingRti->joinFederationExecution(
          L"failure-subject", L"observer", federationName),
      rti1516_2025::RTIinternalError);
  REQUIRE(observedStore->createCalls == 1U);

  // A successful peer join with the rejected name proves that writer setup
  // rolled the registry membership back instead of leaving a partial joined
  // federate identity behind.
  REQUIRE_NOTHROW(peer->joinFederationExecution(
      L"failure-subject", L"observer", federationName));
  REQUIRE_NOTHROW(peer->resignFederationExecution(NO_ACTION));
  REQUIRE_NOTHROW(failingRti->destroyFederationExecution(federationName));
  REQUIRE_NOTHROW(failingRti->disconnect());
  REQUIRE_NOTHROW(peer->disconnect());
}

TEST_CASE(
    "Service-report append failure surfaces an RTI error without an in-memory fallback",
    "[integration][development-profile][federation-management][mom]"
    "[service-report-store][service-reporting][failure-handling]"
    "[rti.service.set-exception-reporting-switch]") {
  NullFederateAmbassador reports;
  auto failingStore = std::make_unique<FailingAppendServiceReportStore>();
  auto* const observedStore = failingStore.get();
  auto rti = std::make_unique<UmbraRtiAmbassador>(
      UmbraRtiAmbassador::ServiceReportStoreTestSeam{},
      std::move(failingStore));
  auto const federationName = nextFederationName();
  auto const fomModule = resourcePath("switch-nrg-disabled-fom.xml").wstring();

  REQUIRE_NOTHROW(rti->connect(reports, HLA_EVOKED));
  REQUIRE_NOTHROW(rti->createFederationExecution(
      federationName, fomModule, L"HLAinteger64Time"));
  REQUIRE_NOTHROW(rti->joinFederationExecution(
      L"append-failure-subject", L"observer", federationName));
  REQUIRE(observedStore->createCalls == 1U);
  REQUIRE(observedStore->initialRecords.size() == 1U);
  REQUIRE(observedStore->appendCalls == 0U);

  // Enabling the two routing switches does not itself emit a service record;
  // the first selected successful-void report is the exception-switch call.
  REQUIRE_NOTHROW(rti->setServiceReportingSwitch(true));
  REQUIRE_NOTHROW(rti->setSendServiceReportsToFileSwitch(true));
  REQUIRE_THROWS_AS(
      rti->setExceptionReportingSwitch(true),
      rti1516_2025::RTIinternalError);
  REQUIRE(observedStore->appendCalls == 1U);
  REQUIRE(observedStore->createCalls == 1U);
  REQUIRE(rti->getExceptionReportingSwitch());

  // Disable reporting before cleanup so resignation does not select another
  // append. The failed writer is never replaced by a memory sink.
  REQUIRE_NOTHROW(rti->setSendServiceReportsToFileSwitch(false));
  REQUIRE_NOTHROW(rti->setServiceReportingSwitch(false));
  REQUIRE_NOTHROW(rti->resignFederationExecution(NO_ACTION));
  REQUIRE_NOTHROW(rti->destroyFederationExecution(federationName));
  REQUIRE_NOTHROW(rti->disconnect());
}
