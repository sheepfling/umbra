#ifndef UMBRA_SOURCE_DIRECTORY
#error "The 2025 process service-report test requires the Umbra source directory."
#endif

#include <catch2/catch_test_macros.hpp>

#include <RTI/NullFederateAmbassador.h>
#include <RTI/RTI1516.h>
#include <RTI/time/HLAinteger64Interval.h>
#include <RTI/time/HLAinteger64Time.h>

#include <algorithm>
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

class TimeReports final : public NullFederateAmbassador {
 public:
  void timeRegulationEnabled(LogicalTime const&) override { ++regulationEnabled; }
  void timeConstrainedEnabled(LogicalTime const&) override { ++constrainedEnabled; }
  void timeAdvanceGrant(LogicalTime const& time) override {
    ++grants;
    grantedTime = dynamic_cast<HLAinteger64Time const&>(time).getTime();
  }

  std::size_t regulationEnabled = 0;
  std::size_t constrainedEnabled = 0;
  std::size_t grants = 0;
  std::int64_t grantedTime = -1;
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
  std::sort(files.begin(), files.end());
  return files;
}

std::string reportText(std::filesystem::path const& path) {
  std::ifstream file(path, std::ios::binary);
  return {std::istreambuf_iterator<char>(file), std::istreambuf_iterator<char>()};
}

TEST_CASE(
    "RTIambassador reports a pending process NMRA failure to the joined federate file",
    "[integration][development-profile][2025][time-management][mom]"
    "[service-report-file][service-reporting][service-failure]"
    "[process-boundary][public-endpoint][next-message-request-available]"
    "[process-service-report-next-message-request-available-pending-failure]"
    "[rti.service.time-advance-request][rti.service.next-message-request-available]"
    "[federate.callback.time-advance-grant]") {
  constexpr wchar_t const* federationName = L"process-nmra-pending-report-execution";
  constexpr wchar_t const* regulatorName = L"process-nmra-pending-regulator";
  constexpr wchar_t const* constrainedName = L"process-nmra-pending-constrained";
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
      auto regulatorConnection = listener->accept(
          nullptr, {"process-nmra-pending-server", 0x92A1U},
          [](std::wstring) {}, [](std::wstring) { return false; });
      ProcessTransportSession regulatorSession(regulatorConnection);
      auto regulatorHandler = service.handlerFor(regulatorSession);
      auto serve = [&](ProcessTransportSession& session, auto const& handler,
                       TransportServiceOperation expected) {
        if (!umbra::test::servePrimaryProcessRequest(
                session, handler, [&](TransportServiceMessage const& request) {
                  if (request.operation != expected) {
                    throw std::runtime_error("Unexpected process NMRA parity operation.");
                  }
                  return handler(request);
                })) {
          throw std::runtime_error("Lost process NMRA parity operation.");
        }
      };
      serve(regulatorSession, regulatorHandler,
            TransportServiceOperation::create_federation_execution);
      serve(regulatorSession, regulatorHandler,
            TransportServiceOperation::join_federation_execution);

      auto constrainedConnection = listener->accept(
          nullptr, {"process-nmra-pending-server", 0x92A2U},
          [](std::wstring) {}, [](std::wstring) { return false; });
      ProcessTransportSession constrainedSession(constrainedConnection);
      auto constrainedHandler = service.handlerFor(constrainedSession);
      serve(constrainedSession, constrainedHandler,
            TransportServiceOperation::join_federation_execution);
      serve(regulatorSession, regulatorHandler,
            TransportServiceOperation::enable_time_regulation);
      serve(regulatorSession, regulatorHandler,
            TransportServiceOperation::report_successful_void_service_invocation);
      serve(constrainedSession, constrainedHandler,
            TransportServiceOperation::enable_time_constrained);
      serve(constrainedSession, constrainedHandler,
            TransportServiceOperation::report_successful_void_service_invocation);
      serve(constrainedSession, constrainedHandler,
            TransportServiceOperation::time_advance_request);
      serve(constrainedSession, constrainedHandler,
            TransportServiceOperation::report_successful_void_service_invocation);
      serve(constrainedSession, constrainedHandler,
            TransportServiceOperation::receive_interaction);
      serve(constrainedSession, constrainedHandler,
            TransportServiceOperation::set_service_reporting_switch);
      serve(constrainedSession, constrainedHandler,
            TransportServiceOperation::set_send_service_reports_to_file_switch);
      serve(constrainedSession, constrainedHandler,
            TransportServiceOperation::next_message_request_available);
      serve(constrainedSession, constrainedHandler,
            TransportServiceOperation::report_failed_service_invocation);
      serve(regulatorSession, regulatorHandler,
            TransportServiceOperation::time_advance_request);
      serve(regulatorSession, regulatorHandler,
            TransportServiceOperation::report_successful_void_service_invocation);
      serve(constrainedSession, constrainedHandler,
            TransportServiceOperation::resign_federation_execution);
      serve(regulatorSession, regulatorHandler,
            TransportServiceOperation::resign_federation_execution);
      service.detach(constrainedSession);
      service.detach(regulatorSession);
      constrainedConnection->close();
      regulatorConnection->close();
    } catch (...) {
      serverError = std::current_exception();
    }
  });

  TimeReports regulatorReports;
  TimeReports constrainedReports;
  auto regulator = makeRti();
  auto constrained = makeRti();
  auto config = [port](std::wstring name) {
    auto configuration = RtiConfiguration::createConfiguration();
    configuration.withConfigurationName(std::move(name));
    configuration.withRtiAddress(L"tcp://127.0.0.1:" + std::to_wstring(port));
    return configuration;
  };
  std::exception_ptr clientError;
  bool regulatorJoined = false;
  bool constrainedJoined = false;
  try {
    REQUIRE(regulator->connect(
                regulatorReports, HLA_EVOKED,
                config(L"process-nmra-pending-regulator-client"))
                .addressUsed);
    REQUIRE_NOTHROW(regulator->createFederationExecution(
        federationName, L"server-owned-fom.xml"));
    REQUIRE(regulator->joinFederationExecution(
                regulatorName, L"regulator", federationName)
                .isValid());
    regulatorJoined = true;
    REQUIRE(constrained->connect(
                constrainedReports, HLA_EVOKED,
                config(L"process-nmra-pending-constrained-client"))
                .addressUsed);
    REQUIRE(constrained->joinFederationExecution(
                constrainedName, L"constrained", federationName)
                .isValid());
    constrainedJoined = true;

    auto const joinedFiles = reportFiles(reportDirectory);
    REQUIRE(joinedFiles.size() == 2U);
    auto const constrainedFile = std::find_if(
        joinedFiles.begin(), joinedFiles.end(), [&](auto const& file) {
          return reportText(file).find("process-nmra-pending-constrained") !=
                 std::string::npos;
        });
    REQUIRE(constrainedFile != joinedFiles.end());
    auto const reportFile = *constrainedFile;
    REQUIRE(reportFile.is_absolute());

    // Leave a positive initial GALT so the constrained role can complete
    // before this test queues its future NMRA request.
    REQUIRE_NOTHROW(regulator->enableTimeRegulation(HLAinteger64Interval(1)));
    REQUIRE_FALSE(regulator->evokeCallback(0.0));
    REQUIRE(regulatorReports.regulationEnabled == 1U);
    REQUIRE_NOTHROW(constrained->enableTimeConstrained());
    REQUIRE_FALSE(constrained->evokeCallback(0.0));
    REQUIRE(constrainedReports.constrainedEnabled == 1U);

    REQUIRE_NOTHROW(constrained->timeAdvanceRequest(HLAinteger64Time(5)));
    REQUIRE(constrainedReports.grants == 0U);
    REQUIRE_FALSE(constrained->evokeCallback(0.0));
    REQUIRE(constrainedReports.grants == 0U);
    REQUIRE_NOTHROW(constrained->setServiceReportingSwitch(true));
    REQUIRE_NOTHROW(constrained->setSendServiceReportsToFileSwitch(true));
    REQUIRE(reportFiles(reportDirectory) == joinedFiles);
    auto const beforeFailure = reportText(reportFile);

    REQUIRE_THROWS_AS(
        constrained->nextMessageRequestAvailable(HLAinteger64Time(6)),
        InTimeAdvancingState);
    auto const expectedFailure =
        R"({"HLAserialNumber":0,"HLAreturnedArgument":[null],"HLAservice":"NextMessageRequestAvailable","HLAsuppliedArguments":[{"HLAargumentType":31,"HLAargumentName":"Logical time","HLAargumentValue":"6"}],"HLAsuccessIndicator":false,"HLAexception":"InTimeAdvancingState: The joined federate already has a time-advance request awaiting a grant."})";
    INFO(reportText(reportFile));
    REQUIRE(reportText(reportFile) == beforeFailure + expectedFailure);
    REQUIRE(reportFiles(reportDirectory) == joinedFiles);
    REQUIRE(constrainedReports.grants == 0U);

    REQUIRE_NOTHROW(regulator->timeAdvanceRequest(HLAinteger64Time(5)));
    REQUIRE_FALSE(regulator->evokeCallback(0.0));
    REQUIRE(regulatorReports.grants == 1U);
    REQUIRE(regulatorReports.grantedTime == 5);
    REQUIRE_FALSE(constrained->evokeCallback(0.0));
    REQUIRE(constrainedReports.grants == 1U);
    REQUIRE(constrainedReports.grantedTime == 5);
    REQUIRE(reportFiles(reportDirectory) == joinedFiles);
    REQUIRE(reportText(reportFile) == beforeFailure + expectedFailure);

    REQUIRE_NOTHROW(constrained->resignFederationExecution(NO_ACTION));
    constrainedJoined = false;
    REQUIRE_NOTHROW(regulator->resignFederationExecution(NO_ACTION));
    regulatorJoined = false;
    constrained->disconnect();
    regulator->disconnect();
  } catch (...) {
    clientError = std::current_exception();
    if (constrainedJoined) {
      try {
        constrained->resignFederationExecution(NO_ACTION);
      } catch (...) {
      }
    }
    if (regulatorJoined) {
      try {
        regulator->resignFederationExecution(NO_ACTION);
      } catch (...) {
      }
    }
    try {
      constrained->disconnect();
    } catch (...) {
    }
    try {
      regulator->disconnect();
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

}  // namespace
#endif
