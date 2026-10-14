#include <catch2/catch_test_macros.hpp>

#include "internal/federation/federation_registry.hpp"
#include "internal/federation/process_federation_client.hpp"
#include "internal/federation/process_federation_service.hpp"
#include "process_federation_service_test_support.hpp"

#include <atomic>
#include <exception>
#include <filesystem>
#include <memory>
#include <stdexcept>
#include <thread>
#include <utility>

namespace {

using umbra::detail::EmbeddedFederationRegistry;
using umbra::detail::ProcessFederationCreateRequest;
using umbra::detail::ProcessFederationJoinRequest;
using umbra::detail::ProcessFederationJoinResult;
using umbra::detail::ProcessFederationService;
using umbra::detail::ProcessFederationServiceOptions;
using umbra::detail::ProcessTransportConnection;
using umbra::detail::ProcessTransportListener;
using umbra::detail::ProcessTransportServiceDispatcher;
using umbra::detail::ProcessTransportSession;
using umbra::detail::TransportServiceMessage;
using umbra::detail::TransportServiceOperation;
using umbra::detail::TransportServiceStatus;
using umbra::test::process_federation_service_support::composedRestaurantDefinition;
using umbra::test::process_federation_service_support::request;

constexpr wchar_t federationName[] = L"process-report-rollback-execution";
constexpr wchar_t federateName[] = L"process-report-rollback-federate";

}  // namespace

TEST_CASE(
    "Private process Join rolls back file and session state after setup failures",
    "[internal][integration][development-profile][process-boundary]"
    "[join-rollback][service-report-file][filesystem]") {
  static std::atomic_uint64_t sequence{0U};
  auto const reportDirectory = std::filesystem::temp_directory_path() /
      ("umbra-process-report-rollback-" +
       std::to_string(sequence.fetch_add(1U, std::memory_order_relaxed)));
  std::error_code cleanupError;
  std::filesystem::remove_all(reportDirectory, cleanupError);

  auto listener = ProcessTransportListener::listen({"127.0.0.1", 0U});
  REQUIRE(listener != nullptr);
  auto const port = listener->address().port;
  REQUIRE(port != 0U);

  EmbeddedFederationRegistry registry;
  std::atomic_bool failAfterFirstAllocation{true};
  std::atomic_bool failAfterFirstSessionInstall{true};
  std::atomic_bool observedAllocatedFile{false};
  ProcessFederationServiceOptions options;
  options.serviceReportDirectory = reportDirectory;
  options.afterServiceReportFileAllocatedForTesting =
      [&](std::filesystem::path const& path) {
        if (!failAfterFirstAllocation.exchange(false, std::memory_order_acq_rel)) {
          return;
        }
        observedAllocatedFile.store(
            std::filesystem::is_regular_file(path), std::memory_order_release);
        throw std::runtime_error("injected post-allocation Join setup failure");
      };
  options.afterJoinedFederateSessionInstalledForTesting = [&] {
    if (failAfterFirstSessionInstall.exchange(false, std::memory_order_acq_rel)) {
      throw std::runtime_error("injected post-installation Join setup failure");
    }
  };
  ProcessFederationService service(
      registry, composedRestaurantDefinition(), std::move(options));

  std::exception_ptr serverFailure;
  std::thread server([&] {
    try {
      auto connection = listener->accept(
          nullptr,
          {"process-report-rollback-server", 0xE3U},
          [](std::wstring) {},
          [](std::wstring) { return false; });
      ProcessTransportSession session(std::move(connection));
      auto handler = service.handlerFor(session);
      for (std::size_t requestIndex = 0U; requestIndex < 5U; ++requestIndex) {
        if (!ProcessTransportServiceDispatcher::serveOne(session, handler)) {
          throw std::runtime_error(
              "The process service lost a report-file rollback request.");
        }
      }
      service.detach(session);
      session.connection()->close();
    } catch (...) {
      serverFailure = std::current_exception();
    }
  });

  std::exception_ptr clientFailure;
  std::shared_ptr<ProcessTransportConnection> connection;
  std::filesystem::path committedReport;
  try {
    connection = ProcessTransportConnection::connectClient(
        nullptr,
        {"127.0.0.1", port},
        {"process-report-rollback-client", 0xE4U},
        [](std::wstring) {},
        [](std::wstring) { return false; });
    ProcessTransportSession session(connection);
    TransportServiceMessage response;

    if (!session.request(
            request(
                TransportServiceOperation::create_federation_execution,
                1U,
                umbra::detail::encodeProcessFederationCreateRequest(
                    ProcessFederationCreateRequest{federationName})),
            response) || response.status != TransportServiceStatus::ok) {
      throw std::runtime_error("The rollback fixture could not create its federation.");
    }

    if (!session.request(
            request(
                TransportServiceOperation::join_federation_execution,
                2U,
                umbra::detail::encodeProcessFederationJoinRequest(
                    ProcessFederationJoinRequest{
                        federationName, federateName, federateName})),
            response) || response.status != TransportServiceStatus::internal_error) {
      throw std::runtime_error(
          "The injected post-allocation Join failure did not return an RTI error.");
    }
    if (!observedAllocatedFile.load(std::memory_order_acquire) ||
        registry.memberByName(federationName, federateName) ||
        !std::filesystem::is_empty(reportDirectory)) {
      throw std::runtime_error(
          "The failed process Join did not roll back its member and report file.");
    }

    if (!session.request(
            request(
                TransportServiceOperation::join_federation_execution,
                3U,
                umbra::detail::encodeProcessFederationJoinRequest(
                    ProcessFederationJoinRequest{
                        federationName, federateName, federateName})),
            response) || response.status != TransportServiceStatus::internal_error ||
        registry.memberByName(federationName, federateName) ||
        !std::filesystem::is_empty(reportDirectory)) {
      throw std::runtime_error(
          "The post-installation Join failure retained member, session, or file state.");
    }

    if (!session.request(
            request(
                TransportServiceOperation::join_federation_execution,
                4U,
                umbra::detail::encodeProcessFederationJoinRequest(
                    ProcessFederationJoinRequest{
                        federationName, federateName, federateName})),
            response) || response.status != TransportServiceStatus::ok) {
      throw std::runtime_error(
          "The process Join could not retry after report-file rollback.");
    }
    auto const joined = umbra::detail::decodeProcessFederationJoinResult(
        response.payload);
    committedReport = joined.reportServiceFile;
    if (committedReport.empty() || !std::filesystem::is_regular_file(committedReport)) {
      throw std::runtime_error(
          "The successful retry did not retain its report file.");
    }

    if (!session.request(
            request(
                TransportServiceOperation::resign_federation_execution,
                5U,
                umbra::detail::encodeProcessFederationResignRequest(
                    umbra::detail::ProcessFederationResignRequest{
                        federationName,
                        joined.federateId,
                        rti1516_2025::NO_ACTION})),
            response) || response.status != TransportServiceStatus::ok ||
        !std::filesystem::is_regular_file(committedReport)) {
      throw std::runtime_error(
          "The successful report file did not persist through resignation.");
    }
    connection->close();
  } catch (...) {
    clientFailure = std::current_exception();
    if (connection) {
      connection->close();
    }
  }

  listener.reset();
  if (server.joinable()) {
    server.join();
  }
  if (clientFailure) {
    std::rethrow_exception(clientFailure);
  }
  REQUIRE_FALSE(serverFailure);
  REQUIRE(observedAllocatedFile.load(std::memory_order_acquire));
  REQUIRE_FALSE(registry.memberByName(federationName, federateName));
  REQUIRE(std::filesystem::is_regular_file(committedReport));

  std::filesystem::remove_all(reportDirectory, cleanupError);
  REQUIRE_FALSE(cleanupError);
}
