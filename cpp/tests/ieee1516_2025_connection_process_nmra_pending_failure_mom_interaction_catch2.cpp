#ifndef UMBRA_SOURCE_DIRECTORY
#error "The 2025 process MOM service-report test requires the Umbra source directory."
#endif

#include <catch2/catch_test_macros.hpp>
#include "hla_test_names.hpp"

#include <RTI/NullFederateAmbassador.h>
#include <RTI/RTI1516.h>
#include <RTI/encoding/BasicDataElements.h>
#include <RTI/encoding/HLAfixedRecord.h>
#include <RTI/encoding/HLAvariableArray.h>
#include <RTI/time/HLAinteger64Interval.h>
#include <RTI/time/HLAinteger64Time.h>

#include <algorithm>
#include <array>
#include <chrono>
#include <cstddef>
#include <cstdint>
#include <exception>
#include <filesystem>
#include <fstream>
#include <iterator>
#include <memory>
#include <mutex>
#include <string>
#include <system_error>
#include <thread>
#include <utility>
#include <vector>

#include "ieee1516_2025_connection_test_support.hpp"
#include "ieee1516_2025_process_fom_test_support.hpp"

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

  std::size_t regulationEnabled = 0U;
  std::size_t constrainedEnabled = 0U;
  std::size_t grants = 0U;
  std::int64_t grantedTime = -1;
};

class ReportObserver final : public NullFederateAmbassador {
 public:
  void receiveInteraction(
      InteractionClassHandle const& interactionClass,
      ParameterHandleValueMap const& parameterValues,
      VariableLengthData const& userSuppliedTag,
      TransportationTypeHandle const& transportationType,
      FederateHandle const& producingFederate,
      RegionHandleSet const* optionalSentRegions) override {
    ++count;
    receivedClass = interactionClass;
    values = parameterValues;
    tag = userSuppliedTag;
    transportation = transportationType;
    producer = producingFederate;
    sentRegions = optionalSentRegions != nullptr;
  }

  std::size_t count = 0U;
  InteractionClassHandle receivedClass;
  ParameterHandleValueMap values;
  VariableLengthData tag;
  TransportationTypeHandle transportation;
  FederateHandle producer;
  bool sentRegions = false;
};

struct RemoveReportDirectory final {
  std::filesystem::path path;
  ~RemoveReportDirectory() {
    std::error_code ignored;
    std::filesystem::remove_all(path, ignored);
  }
};

std::vector<std::pair<std::filesystem::path, std::string>> reportFiles(
    std::filesystem::path const& directory) {
  std::vector<std::pair<std::filesystem::path, std::string>> files;
  for (auto const& entry : std::filesystem::directory_iterator(directory)) {
    if (!entry.is_regular_file()) {
      continue;
    }
    std::ifstream file(entry.path(), std::ios::binary);
    files.emplace_back(entry.path(), std::string{
        std::istreambuf_iterator<char>(file), std::istreambuf_iterator<char>()});
  }
  std::sort(files.begin(), files.end());
  return files;
}

TEST_CASE(
    "RTIambassador routes a pending process NMRA failure to one MOM observer",
    "[integration][development-profile][2025][time-management][mom]"
    "[service-report-interaction][service-reporting][service-failure]"
    "[process-boundary][public-endpoint][next-message-request-available]"
    "[process-service-report-next-message-request-available-pending-failure-interaction]"
    "[rti.service.time-advance-request][rti.service.next-message-request-available]"
    "[federate.callback.time-advance-grant]") {
  constexpr wchar_t const* federationName = L"process-nmra-pending-mom-execution";
  auto const reportDirectory = temporaryServiceReportDirectory();
  RemoveReportDirectory const cleanup{reportDirectory};
  auto listener = ProcessTransportListener::listen({"127.0.0.1", 0U});
  REQUIRE(listener);
  auto const port = listener->address().port;
  REQUIRE(port != 0U);

  TimeReports regulatorReports;
  TimeReports constrainedReports;
  ReportObserver observerReports;
  auto regulator = makeRti();
  auto constrained = makeRti();
  auto observer = makeRti();
  std::exception_ptr serverError;
  std::mutex serverErrorMutex;
  std::thread server([&] {
    std::vector<std::shared_ptr<ProcessTransportConnection>> connections;
    std::vector<std::unique_ptr<ProcessTransportSession>> sessions;
    std::vector<std::thread> workers;
    auto joinWorkers = [&] {
      for (auto& worker : workers) {
        if (worker.joinable()) {
          worker.join();
        }
      }
    };
    try {
      EmbeddedFederationRegistry registry;
      ProcessFederationServiceOptions options;
      options.serviceReportDirectory = reportDirectory;
      ProcessFederationService service(
          registry, composedProcessDefinition(), std::move(options));
      auto acceptAndServe = [&](char const* name, std::uint64_t identity) {
        auto connection = listener->accept(
            nullptr, {name, identity},
            [](std::wstring) {}, [](std::wstring) { return false; });
        auto session = std::make_unique<ProcessTransportSession>(connection);
        auto* sessionPointer = session.get();
        auto handler = service.handlerFor(*sessionPointer);
        sessions.push_back(std::move(session));
        connections.push_back(connection);
        workers.emplace_back(
            [&, sessionPointer, connection, handler = std::move(handler)]() mutable {
              try {
                while (ProcessTransportServiceDispatcher::serveOne(
                    *sessionPointer, handler)) {
                }
                service.detach(*sessionPointer);
                connection->close();
              } catch (...) {
                {
                  std::scoped_lock lock(serverErrorMutex);
                  if (!serverError) {
                    serverError = std::current_exception();
                  }
                }
                try {
                  service.detach(*sessionPointer);
                } catch (...) {
                }
                connection->close();
              }
            });
      };
      try {
        acceptAndServe("process-nmra-mom-regulator", 0x92B1U);
        acceptAndServe("process-nmra-mom-constrained", 0x92B2U);
        acceptAndServe("process-nmra-mom-observer", 0x92B3U);
        joinWorkers();
      } catch (...) {
        // Keep service and its handlers alive until every worker has stopped.
        for (auto const& connection : connections) {
          connection->close();
        }
        joinWorkers();
        throw;
      }
    } catch (...) {
      std::scoped_lock lock(serverErrorMutex);
      if (!serverError) {
        serverError = std::current_exception();
      }
    }
  });

  auto config = [port](std::wstring name) {
    auto configuration = RtiConfiguration::createConfiguration();
    configuration.withConfigurationName(std::move(name));
    configuration.withRtiAddress(L"tcp://127.0.0.1:" + std::to_wstring(port));
    return configuration;
  };
  auto evokeUntil = [](auto& rti, auto&& completed) {
    auto const deadline =
        std::chrono::steady_clock::now() + std::chrono::seconds(2);
    while (!completed() && std::chrono::steady_clock::now() < deadline) {
      static_cast<void>(rti->evokeCallback(0.01));
    }
  };
  std::exception_ptr clientError;
  try {
    REQUIRE(regulator->connect(
                regulatorReports, HLA_EVOKED, config(L"process-nmra-mom-regulator"))
                .addressUsed);
    REQUIRE_NOTHROW(regulator->createFederationExecution(
        federationName, L"server-owned-fom.xml"));
    auto const regulatorHandle = regulator->joinFederationExecution(
        L"process-nmra-mom-regulator", L"regulator", federationName);
    REQUIRE(regulatorHandle.isValid());

    REQUIRE(constrained->connect(
                constrainedReports, HLA_EVOKED,
                config(L"process-nmra-mom-constrained"))
                .addressUsed);
    auto const constrainedHandle = constrained->joinFederationExecution(
        L"process-nmra-mom-constrained", L"constrained", federationName);
    REQUIRE(constrainedHandle.isValid());

    // Keep initial GALT above the constrained federate's current time so
    // Enable Time Constrained can complete before this test requests NMRA.
    REQUIRE_NOTHROW(regulator->enableTimeRegulation(HLAinteger64Interval(1)));
    evokeUntil(regulator,
               [&] { return regulatorReports.regulationEnabled == 1U; });
    REQUIRE(regulatorReports.regulationEnabled == 1U);
    REQUIRE_NOTHROW(constrained->enableTimeConstrained());
    evokeUntil(constrained,
               [&] { return constrainedReports.constrainedEnabled == 1U; });
    REQUIRE(constrainedReports.constrainedEnabled == 1U);
    REQUIRE_NOTHROW(constrained->timeAdvanceRequest(HLAinteger64Time(5)));
    REQUIRE_FALSE(constrained->evokeCallback(0.0));
    REQUIRE(constrainedReports.grants == 0U);
    REQUIRE_NOTHROW(constrained->setServiceReportingSwitch(true));
    REQUIRE_FALSE(constrained->getSendServiceReportsToFileSwitch());

    REQUIRE(observer->connect(
                observerReports, HLA_EVOKED, config(L"process-nmra-mom-observer"))
                .addressUsed);
    auto const observerHandle = observer->joinFederationExecution(
        L"process-nmra-mom-observer", L"observer", federationName);
    REQUIRE(observerHandle.isValid());
    auto const reportClass = observer->getInteractionClassHandle(
        umbra::test::hla::wide::standard::service_invocation);
    REQUIRE(reportClass.isValid());
    std::array<ParameterHandle, 8U> reportParameters;
    std::array<wchar_t const*, 8U> const parameterNames{
        L"HLAservice", L"HLAserviceType", L"HLAsuccessIndicator",
        L"HLAsuppliedArguments", L"HLAreturnedArgument", L"HLAexception",
        L"HLAserialNumber", umbra::test::hla::wide::standard::federate};
    for (std::size_t index = 0U; index < reportParameters.size(); ++index) {
      reportParameters[index] = observer->getParameterHandle(
          reportClass, parameterNames[index]);
      REQUIRE(reportParameters[index].isValid());
    }
    auto const reliable = observer->getTransportationTypeHandle(umbra::test::hla::wide::standard::reliable_transportation);
    REQUIRE(reliable.isValid());
    REQUIRE_NOTHROW(observer->subscribeInteractionClass(reportClass, true));
    REQUIRE(observerReports.count == 0U);
    auto const filesBeforeFailure = reportFiles(reportDirectory);
    REQUIRE(filesBeforeFailure.size() == 3U);
    REQUIRE(constrainedReports.grants == 0U);

    REQUIRE_THROWS_AS(
        constrained->nextMessageRequestAvailable(HLAinteger64Time(6)),
        InTimeAdvancingState);
    // The synchronous failure report must choose the MOM interaction destination,
    // not append a success or failure record to any joined federate's file.
    REQUIRE(reportFiles(reportDirectory) == filesBeforeFailure);
    REQUIRE(constrainedReports.grants == 0U);
    auto const reportDeadline =
        std::chrono::steady_clock::now() + std::chrono::seconds(3);
    while (observerReports.count == 0U &&
           std::chrono::steady_clock::now() < reportDeadline) {
      static_cast<void>(observer->evokeCallback(0.01));
    }
    REQUIRE(observerReports.count == 1U);
    REQUIRE(observerReports.receivedClass == reportClass);
    REQUIRE(observerReports.values.size() == reportParameters.size());
    REQUIRE(observerReports.tag.size() == 0U);
    REQUIRE(observerReports.transportation == reliable);
    REQUIRE_FALSE(observerReports.producer.isValid());
    REQUIRE_FALSE(observerReports.sentRegions);

    auto const& values = observerReports.values;
    HLAunicodeString serviceName;
    REQUIRE_NOTHROW(serviceName.decode(values.at(reportParameters[0])));
    REQUIRE(serviceName.get() == L"NextMessageRequestAvailable");
    HLAinteger16BE serviceType;
    REQUIRE_NOTHROW(serviceType.decode(values.at(reportParameters[1])));
    REQUIRE(serviceType.get() == 4);
    HLAboolean success;
    REQUIRE_NOTHROW(success.decode(values.at(reportParameters[2])));
    REQUIRE_FALSE(success.get());

    HLAfixedRecord argumentPrototype;
    argumentPrototype.appendElement(HLAinteger32BE{})
        .appendElement(HLAunicodeString{})
        .appendElement(HLAunicodeString{});
    HLAvariableArray suppliedArguments{argumentPrototype};
    REQUIRE_NOTHROW(suppliedArguments.decode(values.at(reportParameters[3])));
    REQUIRE(suppliedArguments.size() == 1U);
    auto const& supplied =
        dynamic_cast<HLAfixedRecord const&>(suppliedArguments.get(0U));
    REQUIRE(dynamic_cast<HLAinteger32BE const&>(supplied.get(0U)).get() == 31);
    REQUIRE(dynamic_cast<HLAunicodeString const&>(supplied.get(1U)).get() ==
            L"Logical time");
    REQUIRE(dynamic_cast<HLAunicodeString const&>(supplied.get(2U)).get() ==
            L"\"6\"");

    HLAfixedRecord returned;
    returned.appendElement(HLAinteger32BE{})
        .appendElement(HLAunicodeString{})
        .appendElement(HLAunicodeString{});
    REQUIRE_NOTHROW(returned.decode(values.at(reportParameters[4])));
    REQUIRE(dynamic_cast<HLAinteger32BE const&>(returned.get(0U)).get() == 34);
    REQUIRE(dynamic_cast<HLAunicodeString const&>(returned.get(1U)).get().empty());
    REQUIRE(dynamic_cast<HLAunicodeString const&>(returned.get(2U)).get() ==
            L"null");
    HLAunicodeString exception;
    REQUIRE_NOTHROW(exception.decode(values.at(reportParameters[5])));
    REQUIRE(exception.get() ==
            L"InTimeAdvancingState: The joined federate already has a "
            L"time-advance request awaiting a grant.");
    HLAinteger32BE serial;
    REQUIRE_NOTHROW(serial.decode(values.at(reportParameters[6])));
    REQUIRE(serial.get() == 0);
    auto const encodedConstrainedHandle = constrainedHandle.encode();
    auto const& reportedHandle = values.at(reportParameters[7]);
    REQUIRE(reportedHandle.size() == encodedConstrainedHandle.size());
    REQUIRE(std::equal(
        static_cast<std::uint8_t const*>(encodedConstrainedHandle.data()),
        static_cast<std::uint8_t const*>(encodedConstrainedHandle.data()) +
            encodedConstrainedHandle.size(),
        static_cast<std::uint8_t const*>(reportedHandle.data())));

    REQUIRE_NOTHROW(regulator->timeAdvanceRequest(HLAinteger64Time(5)));
    evokeUntil(regulator, [&] { return regulatorReports.grants == 1U; });
    REQUIRE(regulatorReports.grants == 1U);
    REQUIRE(regulatorReports.grantedTime == 5);
    evokeUntil(constrained, [&] { return constrainedReports.grants == 1U; });
    REQUIRE(constrainedReports.grants == 1U);
    REQUIRE(constrainedReports.grantedTime == 5);
    for (std::size_t poll = 0U; poll < 3U; ++poll) {
      static_cast<void>(observer->evokeCallback(0.0));
    }
    REQUIRE(observerReports.count == 1U);
    REQUIRE(reportFiles(reportDirectory) == filesBeforeFailure);

    REQUIRE_NOTHROW(constrained->resignFederationExecution(NO_ACTION));
    REQUIRE_NOTHROW(observer->unsubscribeInteractionClass(reportClass));
    REQUIRE_NOTHROW(observer->resignFederationExecution(NO_ACTION));
    REQUIRE_NOTHROW(regulator->resignFederationExecution(NO_ACTION));
    REQUIRE_NOTHROW(constrained->disconnect());
    REQUIRE_NOTHROW(observer->disconnect());
    REQUIRE_NOTHROW(regulator->disconnect());
  } catch (...) {
    clientError = std::current_exception();
    // Failed assertions must close the listener and all sessions before the
    // scripted service thread is joined.
    listener->close();
    observer.reset();
    constrained.reset();
    regulator.reset();
  }

  listener->close();
  server.join();
  if (clientError) {
    std::rethrow_exception(clientError);
  }
  if (serverError) {
    std::rethrow_exception(serverError);
  }
}

}  // namespace
#endif
