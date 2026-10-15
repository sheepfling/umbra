#ifndef UMBRA_SOURCE_DIRECTORY
#error "The 2025 process service-report test requires the Umbra source directory."
#endif

#include <catch2/catch_test_macros.hpp>

#include <RTI/NullFederateAmbassador.h>
#include <RTI/RTI1516.h>
#include <RTI/time/HLAinteger64Time.h>

#include <atomic>
#include <chrono>
#include <cstddef>
#include <cstdint>
#include <exception>
#include <filesystem>
#include <fstream>
#include <iterator>
#include <stdexcept>
#include <string>
#include <thread>
#include <utility>
#include <vector>

#include "ieee1516_2025_connection_test_support.hpp"
#include "ieee1516_2025_process_fom_test_support.hpp"
#include "process_public_service_fixture.hpp"

#if defined(UMBRA_ENABLE_EMBEDDED_FEDERATION_MANAGEMENT)
#include "internal/federation/federation_registry.hpp"
#include "internal/federation/process_federation_service.hpp"
#include "internal/federation/process_transport.hpp"
#include "internal/federation/process_transport_session.hpp"

namespace {

using namespace rti1516_2025;
using namespace umbra::detail;
using umbra::test::connection_support_2025::makeRti;
using umbra::test::connection_support_2025::temporaryServiceReportDirectory;
using umbra::test::process_fom_support_2025::composedProcessDefinition;

class GrantReports final : public NullFederateAmbassador {
 public:
  void timeAdvanceGrant(LogicalTime const& time) override {
    grantedTime.store(dynamic_cast<HLAinteger64Time const&>(time).getTime());
    grants.fetch_add(1U);
  }

  std::atomic<std::size_t> grants{0U};
  std::atomic<std::int64_t> grantedTime{-1};
};

struct RemoveReportDirectory final {
  std::filesystem::path path;
  ~RemoveReportDirectory() {
    std::error_code ignored;
    std::filesystem::remove_all(path, ignored);
  }
};

std::vector<std::filesystem::path> reportFiles(std::filesystem::path const& directory) {
  std::vector<std::filesystem::path> files;
  for (auto const& entry : std::filesystem::directory_iterator(directory)) {
    if (entry.is_regular_file()) {
      files.push_back(entry.path());
    }
  }
  return files;
}

std::string reportText(std::filesystem::path const& path) {
  std::ifstream file(path, std::ios::binary);
  return {std::istreambuf_iterator<char>(file), std::istreambuf_iterator<char>()};
}

TEST_CASE(
    "RTIambassador reports a backward process NMRA failure to the joined federate file",
    "[integration][development-profile][2025][time-management][mom]"
    "[service-report-file][service-reporting][service-failure]"
    "[process-boundary][public-endpoint][next-message-request-available]"
    "[process-service-report-next-message-request-available-backward-failure]"
    "[rti.service.time-advance-request][rti.service.next-message-request-available]"
    "[federate.callback.time-advance-grant]") {
  for (auto const model : {HLA_EVOKED, HLA_IMMEDIATE}) {
    DYNAMIC_SECTION((model == HLA_EVOKED ? "HLA_EVOKED" : "HLA_IMMEDIATE")) {
      constexpr wchar_t const* federationName = L"process-nmra-backward-report-execution";
      constexpr wchar_t const* federateName = L"process-nmra-backward-member";
      auto const reportDirectory = temporaryServiceReportDirectory();
      RemoveReportDirectory const cleanup{reportDirectory};
      auto listener = ProcessTransportListener::listen({"127.0.0.1", 0U});
      REQUIRE(listener);
      auto const port = listener->address().port;
      REQUIRE(port != 0U);

      std::exception_ptr serverError;
      std::thread server([&] {
        try {
          EmbeddedFederationRegistry registry;
          ProcessFederationServiceOptions options;
          options.serviceReportDirectory = reportDirectory;
          ProcessFederationService service(
              registry, composedProcessDefinition(), std::move(options));
          auto connection = listener->accept(
              nullptr, {"process-nmra-backward-server", 0x92A3U},
              [](std::wstring) {}, [](std::wstring) { return false; });
          ProcessTransportSession session(connection);
          auto handler = service.handlerFor(session);
          auto serve = [&](TransportServiceOperation expected) {
            if (!umbra::test::servePrimaryProcessRequest(
                    session, handler, [&](TransportServiceMessage const& request) {
                      if (request.operation != expected) {
                        throw std::runtime_error("Unexpected process NMRA operation.");
                      }
                      return handler(request);
                    })) {
              throw std::runtime_error("Lost process NMRA operation.");
            }
          };
          serve(TransportServiceOperation::create_federation_execution);
          serve(TransportServiceOperation::join_federation_execution);
          serve(TransportServiceOperation::time_advance_request);
          serve(TransportServiceOperation::report_successful_void_service_invocation);
          serve(TransportServiceOperation::set_service_reporting_switch);
          serve(TransportServiceOperation::set_send_service_reports_to_file_switch);
          serve(TransportServiceOperation::next_message_request_available);
          serve(TransportServiceOperation::report_failed_service_invocation);
          serve(TransportServiceOperation::resign_federation_execution);
          service.detach(session);
          connection->close();
        } catch (...) {
          serverError = std::current_exception();
        }
      });

      GrantReports reports;
      auto rti = makeRti();
      auto configuration = RtiConfiguration::createConfiguration();
      configuration.withConfigurationName(L"process-nmra-backward-client");
      configuration.withRtiAddress(L"tcp://127.0.0.1:" + std::to_wstring(port));
      std::exception_ptr clientError;
      bool joined = false;
      try {
        REQUIRE(rti->connect(reports, model, configuration).addressUsed);
        REQUIRE_NOTHROW(rti->createFederationExecution(
            federationName, L"server-owned-fom.xml"));
        REQUIRE(rti->joinFederationExecution(
                    federateName, L"member", federationName)
                    .isValid());
        joined = true;
        auto const files = reportFiles(reportDirectory);
        REQUIRE(files.size() == 1U);
        auto const reportFile = files.front();
        REQUIRE(reportFile.is_absolute());
        REQUIRE(reportText(reportFile).find("process-nmra-backward-member") !=
                std::string::npos);

        REQUIRE_NOTHROW(rti->timeAdvanceRequest(HLAinteger64Time(5)));
        if (model == HLA_EVOKED) {
          REQUIRE_FALSE(rti->evokeCallback(0.0));
        } else {
          auto const deadline = std::chrono::steady_clock::now() +
                                std::chrono::seconds(2);
          while (reports.grants.load() == 0U &&
                 std::chrono::steady_clock::now() < deadline) {
            std::this_thread::sleep_for(std::chrono::milliseconds(1));
          }
        }
        REQUIRE(reports.grants.load() == 1U);
        REQUIRE(reports.grantedTime.load() == 5);
        REQUIRE_NOTHROW(rti->setServiceReportingSwitch(true));
        REQUIRE_NOTHROW(rti->setSendServiceReportsToFileSwitch(true));
        REQUIRE(reportFiles(reportDirectory) == files);
        auto const beforeFailure = reportText(reportFile);

        REQUIRE_THROWS_AS(
            rti->nextMessageRequestAvailable(HLAinteger64Time(4)),
            LogicalTimeAlreadyPassed);
        auto const expectedFailure =
            R"({"HLAserialNumber":0,"HLAreturnedArgument":[null],"HLAservice":"NextMessageRequestAvailable","HLAsuppliedArguments":[{"HLAargumentType":31,"HLAargumentName":"Logical time","HLAargumentValue":"4"}],"HLAsuccessIndicator":false,"HLAexception":"LogicalTimeAlreadyPassed: Next Message Request Available cannot move a joined federate backward in logical time."})";
        INFO(reportText(reportFile));
        REQUIRE(reportText(reportFile) == beforeFailure + expectedFailure);
        REQUIRE(reportFiles(reportDirectory) == files);
        REQUIRE(reports.grants.load() == 1U);
        REQUIRE(reports.grantedTime.load() == 5);

        REQUIRE_NOTHROW(rti->resignFederationExecution(NO_ACTION));
        joined = false;
        rti->disconnect();
      } catch (...) {
        clientError = std::current_exception();
        if (joined) {
          try {
            rti->resignFederationExecution(NO_ACTION);
          } catch (...) {
          }
        }
        try {
          rti->disconnect();
        } catch (...) {
        }
      }
      listener.reset();
      if (server.joinable()) {
        server.join();
      }
      if (clientError) {
        std::rethrow_exception(clientError);
      }
      REQUIRE_FALSE(serverError);
    }
  }
}

}  // namespace
#endif
