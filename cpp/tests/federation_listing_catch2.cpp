#include <catch2/catch_test_macros.hpp>

#include <RTI/NullFederateAmbassador.h>
#include <RTI/RTI1516.h>
#include <RTI/RtiConfiguration.h>

#include "ieee1516_2025_federation_listing_test_support.hpp"
#include "ieee1516_2025_process_fom_test_support.hpp"
#include "internal/federation/federation_registry.hpp"
#include "internal/federation/process_federation_service.hpp"
#include "internal/federation/process_transport.hpp"
#include "internal/federation/process_transport_session.hpp"
#include "process_public_service_fixture.hpp"

#include <algorithm>
#include <atomic>
#include <filesystem>
#include <future>
#include <memory>
#include <mutex>
#include <string>
#include <thread>
#include <vector>

#ifndef UMBRA_SOURCE_DIRECTORY
#error "The federation-listing tests require the Umbra source directory."
#endif

namespace {

using rti1516_2025::FederationExecutionInformationVector;
using rti1516_2025::FederationExecutionMemberInformationVector;
using rti1516_2025::NullFederateAmbassador;
using rti1516_2025::RTIambassador;
using rti1516_2025::RTIambassadorFactory;
using rti1516_2025::RtiConfiguration;

class FederationListingFederateAmbassador final : public NullFederateAmbassador {
 public:
  void reportFederationExecutions(
      FederationExecutionInformationVector const& report) override {
    federationExecutionReports.push_back(report);
  }

  void reportFederationExecutionMembers(
      std::wstring const& federationName,
      FederationExecutionMemberInformationVector const& report) override {
    federationExecutionMemberReports.push_back({federationName, report});
  }

  void reportFederationExecutionDoesNotExist(
      std::wstring const& federationName) override {
    missingFederationReports.push_back(federationName);
  }

  struct MemberReport final {
    std::wstring federationName;
    FederationExecutionMemberInformationVector members;
  };

  std::vector<FederationExecutionInformationVector> federationExecutionReports;
  std::vector<MemberReport> federationExecutionMemberReports;
  std::vector<std::wstring> missingFederationReports;
};

std::unique_ptr<RTIambassador> makeRti() {
  RTIambassadorFactory factory;
  return factory.createRTIambassador();
}

std::filesystem::path resourcePath(std::filesystem::path const& relativePath) {
  return std::filesystem::path(UMBRA_SOURCE_DIRECTORY) /
      "third_party" / "ieee1516.2-2025" / "resources" / relativePath;
}

std::wstring nextFederationName() {
  static std::atomic_uint64_t counter{0};
  return L"umbra-federation-listing-" + std::to_wstring(++counter);
}

void runProcessFederationExecutionListingScenario(
    rti1516_2025::CallbackModel callbackModel) {
  using umbra::detail::EmbeddedFederationRegistry;
  using umbra::detail::ProcessFederationService;
  using umbra::detail::ProcessTransportListener;
  using umbra::detail::ProcessTransportSession;

  auto const firstFederationName = nextFederationName();
  auto const secondFederationName = nextFederationName();
  auto listener = ProcessTransportListener::listen({"127.0.0.1", 0U});
  REQUIRE(listener);
  auto const port = listener->address().port;
  REQUIRE(port != 0U);
  std::exception_ptr serverError;
  std::thread server([&] {
    try {
      EmbeddedFederationRegistry registry;
      auto definition =
          umbra::test::process_fom_support_2025::composedProcessDefinition();
      auto const firstCreated = registry.create(firstFederationName, definition);
      auto const secondCreated = registry.create(secondFederationName, definition);
      if (firstCreated.status != umbra::detail::FederationRegistryStatus::applied ||
          secondCreated.status != umbra::detail::FederationRegistryStatus::applied) {
        throw std::runtime_error(
            "The process execution-list fixture could not seed its federations.");
      }
      ProcessFederationService service(registry, std::move(definition));
      auto connection = listener->accept(
          nullptr,
          {"process-execution-listing-server", 0x9401U},
          [](std::wstring) {},
          [](std::wstring) { return false; });
      ProcessTransportSession session(connection);
      auto handler = service.handlerFor(session);
      while (umbra::test::servePrimaryProcessRequest(session, handler)) {
      }
      service.detach(session);
      connection->close();
    } catch (...) {
      serverError = std::current_exception();
    }
  });

  FederationListingFederateAmbassador reports;
  auto rti = makeRti();
  auto configuration = RtiConfiguration::createConfiguration()
                           .withConfigurationName(L"process-execution-listing")
                           .withRtiAddress(
                               L"tcp://127.0.0.1:" + std::to_wstring(port));
  bool connected = false;
  try {
    REQUIRE_NOTHROW(rti->connect(
        reports, callbackModel, configuration));
    connected = true;
    REQUIRE_NOTHROW(rti->listFederationExecutions());
    if (callbackModel == rti1516_2025::HLA_EVOKED) {
      REQUIRE(reports.federationExecutionReports.empty());
      REQUIRE_FALSE(rti->evokeCallback(0.0));
    }
    REQUIRE(reports.federationExecutionReports.size() == 1U);
    auto const& executions = reports.federationExecutionReports.front();
    REQUIRE(executions.size() == 2U);
    auto const first = std::find_if(
        executions.begin(), executions.end(), [&](auto const& execution) {
          return execution.federationExecutionName == firstFederationName;
        });
    auto const second = std::find_if(
        executions.begin(), executions.end(), [&](auto const& execution) {
          return execution.federationExecutionName == secondFederationName;
        });
    REQUIRE(first != executions.end());
    REQUIRE(first->logicalTimeImplementationName == L"HLAinteger64Time");
    REQUIRE(second != executions.end());
    REQUIRE(second->logicalTimeImplementationName == L"HLAinteger64Time");
    REQUIRE_NOTHROW(rti->disconnect());
    connected = false;
  } catch (...) {
    try {
      if (connected) {
        rti->disconnect();
      }
    } catch (...) {
    }
    listener->close();
    if (server.joinable()) {
      server.join();
    }
    throw;
  }
  if (server.joinable()) {
    server.join();
  }
  if (serverError) {
    std::rethrow_exception(serverError);
  }
}

void runProcessFederationMemberListingScenario(
    rti1516_2025::CallbackModel callbackModel) {
  using umbra::detail::EmbeddedFederationRegistry;
  using umbra::detail::ProcessFederationService;
  using umbra::detail::ProcessTransportListener;
  using umbra::detail::ProcessTransportSession;

  auto const federationName = nextFederationName();
  auto listener = ProcessTransportListener::listen({"127.0.0.1", 0U});
  REQUIRE(listener);
  auto const port = listener->address().port;
  REQUIRE(port != 0U);
  std::exception_ptr serverError;
  std::mutex serverErrorMutex;
  std::promise<void> requestFirstConnectionLoss;
  auto firstConnectionLossRequested = requestFirstConnectionLoss.get_future();
  std::promise<bool> firstConnectionLossCompleted;
  auto firstConnectionLossResult = firstConnectionLossCompleted.get_future();
  std::atomic_bool connectionLossResultSet = false;
  std::thread server([&] {
    try {
      EmbeddedFederationRegistry registry;
      auto definition =
          umbra::test::process_fom_support_2025::composedProcessDefinition();
      auto const created = registry.create(federationName, definition);
      if (created.status != umbra::detail::FederationRegistryStatus::applied) {
        throw std::runtime_error(
            "The member-list process fixture could not seed its federation.");
      }
      ProcessFederationService service(registry, std::move(definition));
      auto firstConnection = listener->accept(
          nullptr,
          {"process-federation-listing-server", 0x9301U},
          [](std::wstring) {},
          [](std::wstring) { return false; });
      auto secondConnection = listener->accept(
          nullptr,
          {"process-federation-listing-server", 0x9302U},
          [](std::wstring) {},
          [](std::wstring) { return false; });
      ProcessTransportSession firstSession(firstConnection);
      ProcessTransportSession secondSession(secondConnection);
      auto firstHandler = service.handlerFor(firstSession);
      auto secondHandler = service.handlerFor(secondSession);
      std::thread firstWorker([&] {
        try {
          while (umbra::test::servePrimaryProcessRequest(
              firstSession, firstHandler)) {
          }
        } catch (...) {
          std::scoped_lock lock(serverErrorMutex);
          if (!serverError) {
            serverError = std::current_exception();
          }
        }
        service.detach(firstSession);
      });
      std::thread secondWorker([&] {
        try {
          while (umbra::test::servePrimaryProcessRequest(
              secondSession, secondHandler)) {
          }
        } catch (...) {
          std::scoped_lock lock(serverErrorMutex);
          if (!serverError) {
            serverError = std::current_exception();
          }
        }
        service.detach(secondSession);
      });
      firstConnectionLossRequested.wait();
      auto const lost = service.connectionLost(
          firstSession, L"focused federation-listing test loss");
      firstConnection->close();
      connectionLossResultSet = true;
      firstConnectionLossCompleted.set_value(lost);
      firstWorker.join();
      secondWorker.join();
      secondConnection->close();
    } catch (...) {
      {
        std::scoped_lock lock(serverErrorMutex);
        if (!serverError) {
          serverError = std::current_exception();
        }
      }
      if (!connectionLossResultSet.exchange(true)) {
        firstConnectionLossCompleted.set_value(false);
      }
    }
  });

  NullFederateAmbassador departedFederate;
  FederationListingFederateAmbassador survivorReports;
  auto departedRti = makeRti();
  auto survivorRti = makeRti();
  auto configuration = RtiConfiguration::createConfiguration()
                           .withConfigurationName(L"process-federation-listing")
                           .withRtiAddress(
                               L"tcp://127.0.0.1:" + std::to_wstring(port));
  bool departedConnected = false;
  bool departedJoined = false;
  bool survivorConnected = false;
  bool survivorJoined = false;
  bool lossTriggered = false;
  try {
    REQUIRE_NOTHROW(
        departedRti->connect(departedFederate, callbackModel, configuration));
    departedConnected = true;
    REQUIRE_NOTHROW(
        survivorRti->connect(survivorReports, callbackModel, configuration));
    survivorConnected = true;
    REQUIRE_NOTHROW(departedRti->joinFederationExecution(
        L"listed-process-member-a", L"observer", federationName));
    departedJoined = true;
    REQUIRE_NOTHROW(survivorRti->joinFederationExecution(
        L"listed-process-member-b", L"observer", federationName));
    survivorJoined = true;

    REQUIRE_NOTHROW(
        survivorRti->listFederationExecutionMembers(federationName));
    if (callbackModel == rti1516_2025::HLA_EVOKED) {
      REQUIRE(survivorReports.federationExecutionMemberReports.empty());
      REQUIRE_FALSE(survivorRti->evokeCallback(0.0));
    }
    REQUIRE(survivorReports.federationExecutionMemberReports.size() == 1U);
    auto const& members =
        survivorReports.federationExecutionMemberReports.front().members;
    REQUIRE(members.size() == 2U);
    REQUIRE(std::any_of(members.begin(), members.end(), [](auto const& member) {
      return member.federateName == L"listed-process-member-a" &&
          member.federateType == L"observer";
    }));
    REQUIRE(std::any_of(members.begin(), members.end(), [](auto const& member) {
      return member.federateName == L"listed-process-member-b" &&
          member.federateType == L"observer";
    }));

    requestFirstConnectionLoss.set_value();
    lossTriggered = true;
    REQUIRE(firstConnectionLossResult.get());
    REQUIRE_NOTHROW(
        survivorRti->listFederationExecutionMembers(federationName));
    if (callbackModel == rti1516_2025::HLA_EVOKED) {
      REQUIRE_FALSE(survivorRti->evokeCallback(0.0));
    }
    REQUIRE(survivorReports.federationExecutionMemberReports.size() == 2U);
    auto const& survivors =
        survivorReports.federationExecutionMemberReports.back().members;
    REQUIRE(survivors.size() == 1U);
    REQUIRE(survivors.front().federateName == L"listed-process-member-b");
    REQUIRE(survivors.front().federateType == L"observer");

    auto const missingFederationName = nextFederationName();
    REQUIRE_NOTHROW(
        survivorRti->listFederationExecutionMembers(missingFederationName));
    if (callbackModel == rti1516_2025::HLA_EVOKED) {
      REQUIRE(survivorReports.missingFederationReports.empty());
      REQUIRE_FALSE(survivorRti->evokeCallback(0.0));
    }
    REQUIRE(survivorReports.missingFederationReports ==
            std::vector<std::wstring>{missingFederationName});
    REQUIRE_NOTHROW(
        survivorRti->resignFederationExecution(rti1516_2025::NO_ACTION));
    survivorJoined = false;
    REQUIRE_NOTHROW(survivorRti->disconnect());
    survivorConnected = false;
  } catch (...) {
    try {
      if (!lossTriggered) {
        requestFirstConnectionLoss.set_value();
      }
      if (survivorJoined) {
        survivorRti->resignFederationExecution(rti1516_2025::NO_ACTION);
      }
      if (survivorConnected) {
        survivorRti->disconnect();
      }
      if (departedJoined) {
        departedRti->resignFederationExecution(rti1516_2025::NO_ACTION);
      }
      if (departedConnected) {
        departedRti->disconnect();
      }
    } catch (...) {
    }
    listener->close();
    if (server.joinable()) {
      server.join();
    }
    throw;
  }
  if (server.joinable()) {
    server.join();
  }
  if (serverError) {
    std::rethrow_exception(serverError);
  }
}

}  // namespace

TEST_CASE(
    "Process federation execution listing reports the authoritative service catalog",
    "[integration][development-profile][federation-management][transport]"
    "[process-boundary][federation-listing][lane.federation-listing]"
    "[rti.service.list-federation-executions]"
    "[2025]") {
  SECTION("HLA_EVOKED") {
    runProcessFederationExecutionListingScenario(rti1516_2025::HLA_EVOKED);
  }
  SECTION("HLA_IMMEDIATE") {
    runProcessFederationExecutionListingScenario(rti1516_2025::HLA_IMMEDIATE);
  }
}

TEST_CASE(
    "Embedded federation-list services dispatch standards reports in both callback models",
    "[integration][development-profile][federation-management][callbacks]"
    "[federation-listing][lane.federation-listing]"
    "[rti.service.list-federation-executions]"
    "[rti.service.list-federation-execution-members][2025]") {
  FederationListingFederateAmbassador evokedReports;
  FederationListingFederateAmbassador immediateReports;
  FederationListingFederateAmbassador disconnectedReports;
  NullFederateAmbassador creatorFederate;
  NullFederateAmbassador memberFederate;
  umbra::test::ieee1516_2025::runFederationListingScenario(
      evokedReports,
      immediateReports,
      disconnectedReports,
      creatorFederate,
      memberFederate,
      makeRti,
      resourcePath,
      nextFederationName);
}

TEST_CASE(
    "Process federation member listing reports the authoritative roster and missing executions",
    "[integration][development-profile][federation-management][transport]"
    "[process-boundary][federation-listing][lane.federation-listing]"
    "[rti.service.list-federation-execution-members]"
    "[2025]") {
  SECTION("HLA_EVOKED") {
    runProcessFederationMemberListingScenario(rti1516_2025::HLA_EVOKED);
  }
  SECTION("HLA_IMMEDIATE") {
    runProcessFederationMemberListingScenario(rti1516_2025::HLA_IMMEDIATE);
  }
}
