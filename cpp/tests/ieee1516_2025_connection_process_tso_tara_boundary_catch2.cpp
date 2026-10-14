#include <catch2/catch_test_macros.hpp>

#if defined(UMBRA_ENABLE_EMBEDDED_FEDERATION_MANAGEMENT)

#include "internal/federation/federation_registry.hpp"
#include "internal/federation/process_federation_service.hpp"
#include "internal/federation/process_transport.hpp"
#include "internal/federation/process_transport_session.hpp"
#include "process_public_service_fixture.hpp"
#include "process_tara_boundary_test_support.hpp"

#include <RTI/NullFederateAmbassador.h>
#include <RTI/RTI1516.h>
#include <RTI/time/HLAinteger64Interval.h>
#include <RTI/time/HLAinteger64Time.h>

#include <algorithm>
#include <array>
#include <chrono>
#include <condition_variable>
#include <cstdint>
#include <exception>
#include <filesystem>
#include <fstream>
#include <iterator>
#include <mutex>
#include <stdexcept>
#include <string>
#include <thread>
#include <vector>

namespace {

using umbra::detail::EmbeddedFederationRegistry;
using umbra::detail::ProcessFederationService;
using umbra::detail::ProcessFederationServiceOptions;
using umbra::detail::ProcessTransportListener;
using umbra::detail::ProcessTransportSession;
using umbra::detail::TransportServiceMessage;
using umbra::detail::TransportServiceMessageKind;
using umbra::detail::TransportServiceOperation;
using umbra::detail::TransportServiceStatus;

using rti1516_2025::HLA_EVOKED;
using rti1516_2025::NO_ACTION;
using rti1516_2025::NullFederateAmbassador;
using rti1516_2025::RTIambassador;
using rti1516_2025::VariableLengthData;

class ProcessAvailableTaraFederateAmbassador final : public NullFederateAmbassador {
 public:
  void timeAdvanceGrant(rti1516_2025::LogicalTime const& time) override {
    ++timeAdvanceGrantCount;
    timeAdvanceGrantImplementation = time.implementationName();
    auto const* integerTime =
        dynamic_cast<rti1516_2025::HLAinteger64Time const*>(&time);
    if (integerTime != nullptr) {
      timeAdvanceGrantValue = integerTime->getTime();
    }
  }

  std::size_t timeAdvanceGrantCount = 0U;
  std::wstring timeAdvanceGrantImplementation;
  std::int64_t timeAdvanceGrantValue = 0;
};

class ProcessTaraSenderFederateAmbassador final : public NullFederateAmbassador {
 public:
  void timeRegulationEnabled(rti1516_2025::LogicalTime const&) override {
    ++timeRegulationEnabledCount;
  }

  void timeAdvanceGrant(rti1516_2025::LogicalTime const& time) override {
    ++timeAdvanceGrantCount;
    auto const* integerTime =
        dynamic_cast<rti1516_2025::HLAinteger64Time const*>(&time);
    if (integerTime != nullptr) {
      timeAdvanceGrantValue = integerTime->getTime();
    }
  }

  std::size_t timeRegulationEnabledCount = 0U;
  std::size_t timeAdvanceGrantCount = 0U;
  std::int64_t timeAdvanceGrantValue = 0;
};

class ProcessTaraReceiverFederateAmbassador final : public NullFederateAmbassador {
 public:
  void timeConstrainedEnabled(rti1516_2025::LogicalTime const&) override {
    {
      std::lock_guard lock(mutex);
      ++timeConstrainedEnabledCount;
    }
    callbacksChanged.notify_all();
  }

  void receiveInteraction(
      rti1516_2025::InteractionClassHandle const&,
      rti1516_2025::ParameterHandleValueMap const&,
      VariableLengthData const&,
      rti1516_2025::TransportationTypeHandle const&,
      rti1516_2025::FederateHandle const&,
      rti1516_2025::RegionHandleSet const*,
      rti1516_2025::LogicalTime const& time,
      rti1516_2025::OrderType,
      rti1516_2025::OrderType,
      rti1516_2025::MessageRetractionHandle const*) override {
    {
      std::lock_guard lock(mutex);
      callbackOrder.push_back("interaction");
      interactionTime = time.toString();
    }
    callbacksChanged.notify_all();
  }

  void timeAdvanceGrant(rti1516_2025::LogicalTime const& time) override {
    {
      std::lock_guard lock(mutex);
      callbackOrder.push_back("grant");
      grantTime = time.toString();
    }
    callbacksChanged.notify_all();
  }

  bool waitForCallbackCount(std::size_t expectedCount) {
    std::unique_lock lock(mutex);
    return callbacksChanged.wait_for(lock, std::chrono::seconds(2), [&] {
      return callbackOrder.size() >= expectedCount;
    });
  }

  std::vector<std::string> callbackOrderSnapshot() {
    std::lock_guard lock(mutex);
    return callbackOrder;
  }

  std::wstring interactionTimeSnapshot() {
    std::lock_guard lock(mutex);
    return interactionTime;
  }

  std::wstring grantTimeSnapshot() {
    std::lock_guard lock(mutex);
    return grantTime;
  }

  std::size_t timeConstrainedEnabledCountSnapshot() {
    std::lock_guard lock(mutex);
    return timeConstrainedEnabledCount;
  }

 private:
  std::mutex mutex;
  std::condition_variable callbacksChanged;
  std::vector<std::string> callbackOrder;
  std::size_t timeConstrainedEnabledCount = 0U;
  std::wstring interactionTime;
  std::wstring grantTime;
};

void runProcessAvailableTaraBaseline() {
  constexpr wchar_t const* federationName =
      L"process-time-advance-available-execution";
  auto const serviceReportDirectory =
      umbra::test::makeProcessTaraBoundaryServiceReportDirectory();
  auto listener = ProcessTransportListener::listen({"127.0.0.1", 0U});
  REQUIRE(listener);
  auto const port = listener->address().port;
  REQUIRE(port != 0U);

  std::exception_ptr serverError;
  std::vector<TransportServiceOperation> serverOperations;
  std::thread server([&] {
    try {
      EmbeddedFederationRegistry registry;
      ProcessFederationServiceOptions serviceOptions{true};
      serviceOptions.serviceReportDirectory = serviceReportDirectory;
      ProcessFederationService service(
          registry,
          umbra::test::makeProcessTaraBoundaryFederationDefinition(),
          std::move(serviceOptions));
      auto connection = listener->accept(
          nullptr,
          {"process-time-advance-available-server", 0x9212U},
          [](std::wstring) {},
          [](std::wstring) { return false; });
      ProcessTransportSession session(connection);
      auto handler = service.handlerFor(session);
      while (umbra::test::servePrimaryProcessRequest(
          session,
          handler,
          [&](TransportServiceMessage const& request) {
            serverOperations.push_back(request.operation);
            return handler(request);
          })) {
        if (serverOperations.back() ==
            TransportServiceOperation::resign_federation_execution) {
          break;
        }
      }
      if (serverOperations.empty() ||
          serverOperations.back() !=
              TransportServiceOperation::resign_federation_execution) {
        throw std::runtime_error(
            "The process time-advance-available server lost the resign request.");
      }
      service.detach(session);
      connection->close();
    } catch (...) {
      serverError = std::current_exception();
    }
  });

  ProcessAvailableTaraFederateAmbassador federate;
  auto rti = umbra::test::makeProcessTaraBoundaryRti();
  auto configuration = rti1516_2025::RtiConfiguration::createConfiguration()
                           .withConfigurationName(
                               L"process-time-advance-available-client")
                           .withRtiAddress(
                               L"tcp://127.0.0.1:" + std::to_wstring(port))
                           .withAdditionalSettings(
                               L"serviceReportDirectory=" +
                               serviceReportDirectory.wstring());
  std::exception_ptr clientError;
  try {
    auto const connectionResult = rti->connect(federate, HLA_EVOKED, configuration);
    REQUIRE(connectionResult.addressUsed);
    REQUIRE_NOTHROW(rti->createFederationExecution(
        federationName, L"server-owned-fom.xml"));
    auto const joined = rti->joinFederationExecution(
        L"process-time-advance-available-type", federationName);
    REQUIRE(joined.isValid());
    REQUIRE_NOTHROW(rti->setServiceReportingSwitch(true));
    REQUIRE_NOTHROW(rti->setSendServiceReportsToFileSwitch(true));

    REQUIRE_NOTHROW(
        rti->timeAdvanceRequestAvailable(rti1516_2025::HLAinteger64Time(5)));
    REQUIRE(federate.timeAdvanceGrantCount == 0U);
    REQUIRE_FALSE(rti->evokeCallback(0.0));
    REQUIRE(federate.timeAdvanceGrantCount == 1U);
    REQUIRE(federate.timeAdvanceGrantImplementation == L"HLAinteger64Time");
    REQUIRE(federate.timeAdvanceGrantValue == 5);
    REQUIRE_NOTHROW(rti->resignFederationExecution(NO_ACTION));
    rti->disconnect();
  } catch (...) {
    clientError = std::current_exception();
  }
  if (clientError && listener) {
    listener->close();
  }
  if (server.joinable()) {
    server.join();
  }
  if (listener) {
    listener.reset();
  }
  if (clientError) {
    std::rethrow_exception(clientError);
  }
  std::vector<TransportServiceOperation> const expectedOperations{
      TransportServiceOperation::create_federation_execution,
      TransportServiceOperation::join_federation_execution,
      TransportServiceOperation::set_service_reporting_switch,
      TransportServiceOperation::set_send_service_reports_to_file_switch,
      TransportServiceOperation::time_advance_request_available,
      TransportServiceOperation::report_successful_void_service_invocation,
      TransportServiceOperation::resign_federation_execution,
  };
  if (serverOperations != expectedOperations) {
    std::string observed;
    for (auto const operation : serverOperations) {
      if (!observed.empty()) {
        observed += ",";
      }
      observed += std::to_string(static_cast<std::uint16_t>(operation));
    }
    throw std::runtime_error(
        "The process time-advance-available server observed unexpected operations: " +
        observed);
  }
  REQUIRE_FALSE(serverError);

  std::string reportContents;
  std::size_t reportFileCount = 0U;
  for (auto const& entry : std::filesystem::directory_iterator(serviceReportDirectory)) {
    if (!entry.is_regular_file()) {
      continue;
    }
    ++reportFileCount;
    std::ifstream reportFile(entry.path(), std::ios::binary);
    reportContents.append(
        std::istreambuf_iterator<char>{reportFile},
        std::istreambuf_iterator<char>{});
  }
  auto countOccurrences = [&](std::string const& needle) {
    std::size_t count = 0U;
    std::size_t offset = 0U;
    while ((offset = reportContents.find(needle, offset)) != std::string::npos) {
      ++count;
      offset += needle.size();
    }
    return count;
  };
  REQUIRE(reportFileCount == 1U);
  INFO(reportContents);
  REQUIRE(countOccurrences(R"("HLAservice":"TimeAdvanceRequestAvailable")") == 1U);
  REQUIRE(countOccurrences(
              R"("HLAservice":"TimeAdvanceRequestAvailable","HLAsuppliedArguments":[{"HLAargumentType":31,"HLAargumentName":"Logical time","HLAargumentValue":"5"}],"HLAsuccessIndicator":true,"HLAexception":null})") == 1U);
}

void runProcessTaraBoundaryCase(
    std::int64_t const interactionTime,
    std::vector<std::string> const& expectedCallbackOrder) {
  auto const federationName = L"process-tara-boundary-execution-" +
      std::to_wstring(interactionTime);
  auto const serviceReportDirectory =
      umbra::test::makeProcessTaraBoundaryServiceReportDirectory();
  constexpr wchar_t const* senderName = L"process-tara-boundary-sender";
  constexpr wchar_t const* receiverName = L"process-tara-boundary-receiver";
  constexpr wchar_t const* interactionName =
      L"HLAinteractionRoot.CustomerTransactions.FoodServed.MainCourseServed";
  constexpr wchar_t const* parameterName = L"TimelinessOk";

  auto listener = ProcessTransportListener::listen({"127.0.0.1", 0U});
  REQUIRE(listener);
  auto const port = listener->address().port;
  REQUIRE(port != 0U);

  std::exception_ptr serverError;
  std::vector<TransportServiceOperation> serverOperations;
  std::mutex serverOperationsMutex;
  std::thread server([&] {
    try {
      EmbeddedFederationRegistry registry;
      ProcessFederationService service(
          registry,
          umbra::test::makeProcessTaraBoundaryFederationDefinition(),
          ProcessFederationServiceOptions{true});
      auto senderConnection = listener->accept(
          nullptr,
          {"process-tara-boundary-server", 0x9217U},
          [](std::wstring) {},
          [](std::wstring) { return false; });
      ProcessTransportSession sender(senderConnection);
      auto senderHandler = service.handlerFor(sender);

      auto serveExpected = [&](ProcessTransportSession& session,
                               auto const& handler,
                               TransportServiceOperation operation,
                               char const* description) {
        if (!umbra::test::servePrimaryProcessRequest(
                session,
                handler,
                [&](TransportServiceMessage const& request) {
                  if (request.operation != operation) {
                    throw std::runtime_error(
                        std::string(description) + " (observed operation " +
                        std::to_string(static_cast<std::uint16_t>(
                            request.operation)) + ").");
                  }
                  return handler(request);
                })) {
          throw std::runtime_error(description);
        }
      };

      serveExpected(
          sender,
          senderHandler,
          TransportServiceOperation::create_federation_execution,
          "The process TARA boundary server lost Create.");
      serveExpected(
          sender,
          senderHandler,
          TransportServiceOperation::join_federation_execution,
          "The process TARA boundary server lost sender Join.");

      auto receiverConnection = listener->accept(
          nullptr,
          {"process-tara-boundary-server", 0x9218U},
          [](std::wstring) {},
          [](std::wstring) { return false; });
      ProcessTransportSession receiver(receiverConnection);
      auto receiverHandler = service.handlerFor(receiver);
      serveExpected(
          receiver,
          receiverHandler,
          TransportServiceOperation::join_federation_execution,
          "The process TARA boundary server lost receiver Join.");

      std::exception_ptr senderWorkerError;
      std::exception_ptr receiverWorkerError;
      auto serveSession = [&](ProcessTransportSession& session,
                              auto const& handler,
                              std::exception_ptr& workerError) {
        try {
          auto lastOperation = TransportServiceOperation::create_federation_execution;
          while (umbra::test::servePrimaryProcessRequest(
              session,
              handler,
              [&](TransportServiceMessage const& request) {
                {
                  std::lock_guard lock(serverOperationsMutex);
                  serverOperations.push_back(request.operation);
                }
                lastOperation = request.operation;
                return handler(request);
              })) {
            if (lastOperation ==
                TransportServiceOperation::resign_federation_execution) {
              break;
            }
          }
        } catch (...) {
          workerError = std::current_exception();
        }
      };
      std::thread senderWorker(
          [&] { serveSession(sender, senderHandler, senderWorkerError); });
      std::thread receiverWorker(
          [&] { serveSession(receiver, receiverHandler, receiverWorkerError); });
      senderWorker.join();
      receiverWorker.join();
      if (senderWorkerError) {
        std::rethrow_exception(senderWorkerError);
      }
      if (receiverWorkerError) {
        std::rethrow_exception(receiverWorkerError);
      }
      service.detach(sender);
      service.detach(receiver);
      senderConnection->close();
      receiverConnection->close();
    } catch (...) {
      serverError = std::current_exception();
    }
  });

  ProcessTaraReceiverFederateAmbassador receiverFederate;
  auto senderRti = umbra::test::makeProcessTaraBoundaryRti();
  auto receiverRti = umbra::test::makeProcessTaraBoundaryRti();
  auto senderConfiguration = rti1516_2025::RtiConfiguration::createConfiguration()
                                 .withConfigurationName(
                                     L"process-tara-boundary-sender-client")
                                 .withRtiAddress(
                                     L"tcp://127.0.0.1:" + std::to_wstring(port))
                                 .withAdditionalSettings(
                                     L"serviceReportDirectory=" +
                                     serviceReportDirectory.wstring());
  auto receiverConfiguration = rti1516_2025::RtiConfiguration::createConfiguration()
                                   .withConfigurationName(
                                       L"process-tara-boundary-receiver-client")
                                   .withRtiAddress(
                                       L"tcp://127.0.0.1:" + std::to_wstring(port))
                                   .withAdditionalSettings(
                                       L"serviceReportDirectory=" +
                                       serviceReportDirectory.wstring());
  ProcessTaraSenderFederateAmbassador senderFederate;
  std::exception_ptr clientError;
  bool senderJoined = false;
  bool receiverJoined = false;
  try {
    REQUIRE(senderRti->connect(
                senderFederate, HLA_EVOKED, senderConfiguration)
                .addressUsed);
    REQUIRE_NOTHROW(senderRti->createFederationExecution(
        federationName, L"server-owned-fom.xml"));
    auto const senderHandle = senderRti->joinFederationExecution(
        senderName, L"process-tara-boundary-type", federationName);
    REQUIRE(senderHandle.isValid());
    senderJoined = true;

    REQUIRE(receiverRti->connect(
                receiverFederate, HLA_EVOKED, receiverConfiguration)
                .addressUsed);
    auto const receiverHandle = receiverRti->joinFederationExecution(
        receiverName, L"process-tara-boundary-type", federationName);
    REQUIRE(receiverHandle.isValid());
    REQUIRE(senderHandle != receiverHandle);
    receiverJoined = true;

    auto const senderInteraction =
        senderRti->getInteractionClassHandle(interactionName);
    auto const receiverInteraction =
        receiverRti->getInteractionClassHandle(interactionName);
    auto const parameter = senderRti->getParameterHandle(
        senderInteraction, parameterName);
    REQUIRE(senderInteraction.isValid());
    REQUIRE(receiverInteraction == senderInteraction);
    REQUIRE(parameter.isValid());
    REQUIRE_NOTHROW(senderRti->publishInteractionClass(senderInteraction));
    REQUIRE_NOTHROW(receiverRti->subscribeInteractionClass(receiverInteraction));
    REQUIRE_NOTHROW(senderRti->enableTimeRegulation(
        rti1516_2025::HLAinteger64Interval(2)));
    static_cast<void>(senderRti->evokeCallback(0.0));
    REQUIRE(senderFederate.timeRegulationEnabledCount == 1U);
    REQUIRE_NOTHROW(receiverRti->enableTimeConstrained());
    static_cast<void>(receiverRti->evokeCallback(0.0));
    REQUIRE(receiverFederate.timeConstrainedEnabledCountSnapshot() == 1U);

    std::array<std::uint8_t, 2U> encodedParameter{0x01U, 0x00U};
    rti1516_2025::ParameterHandleValueMap parameterValues;
    parameterValues.emplace(
        parameter,
        VariableLengthData(encodedParameter.data(), encodedParameter.size()));
    auto const retraction = senderRti->sendInteraction(
        senderInteraction,
        parameterValues,
        VariableLengthData{},
        rti1516_2025::HLAinteger64Time(interactionTime));
    REQUIRE(retraction.isValid());
    REQUIRE_NOTHROW(
        senderRti->timeAdvanceRequest(rti1516_2025::HLAinteger64Time(3)));
    for (int pass = 0; pass != 128 &&
                       senderFederate.timeAdvanceGrantCount == 0U;
         ++pass) {
      static_cast<void>(senderRti->evokeCallback(0.0));
    }
    REQUIRE(senderFederate.timeAdvanceGrantCount == 1U);
    REQUIRE(senderFederate.timeAdvanceGrantValue == 3);

    REQUIRE(receiverFederate.callbackOrderSnapshot().empty());
    REQUIRE_NOTHROW(receiverRti->timeAdvanceRequestAvailable(
        rti1516_2025::HLAinteger64Time(5)));
    for (int pass = 0; pass != 128 &&
                       receiverFederate.callbackOrderSnapshot().size() < 2U;
         ++pass) {
      static_cast<void>(receiverRti->evokeCallback(0.0));
    }
    auto const observedCallbackOrder = receiverFederate.callbackOrderSnapshot();
    REQUIRE(receiverFederate.waitForCallbackCount(2U));
    REQUIRE(observedCallbackOrder == expectedCallbackOrder);
    REQUIRE(receiverFederate.grantTimeSnapshot() ==
            std::to_wstring(std::min<std::int64_t>(interactionTime, 5)));
    REQUIRE(receiverFederate.interactionTimeSnapshot() ==
            std::to_wstring(interactionTime));

    REQUIRE_NOTHROW(receiverRti->resignFederationExecution(NO_ACTION));
    receiverJoined = false;
    REQUIRE_NOTHROW(senderRti->resignFederationExecution(NO_ACTION));
    senderJoined = false;
    REQUIRE_NOTHROW(receiverRti->disconnect());
    REQUIRE_NOTHROW(senderRti->disconnect());
  } catch (...) {
    clientError = std::current_exception();
    if (receiverJoined) {
      try {
        receiverRti->resignFederationExecution(NO_ACTION);
      } catch (...) {
      }
    }
    if (senderJoined) {
      try {
        senderRti->resignFederationExecution(NO_ACTION);
      } catch (...) {
      }
    }
    try {
      receiverRti->disconnect();
    } catch (...) {
    }
    try {
      senderRti->disconnect();
    } catch (...) {
    }
  }
  if (listener) {
    listener.reset();
  }
  if (server.joinable()) {
    server.join();
  }
  if (serverError) {
    try {
      std::rethrow_exception(serverError);
    } catch (std::exception const& error) {
      FAIL("The process TARA boundary server failed: " << error.what());
    } catch (...) {
      FAIL("The process TARA boundary server failed with a non-standard exception.");
    }
  }
  if (clientError) {
    std::rethrow_exception(clientError);
  }
  REQUIRE(std::count(
              serverOperations.begin(),
              serverOperations.end(),
              TransportServiceOperation::send_interaction) == 1);
  REQUIRE(std::count(
              serverOperations.begin(),
              serverOperations.end(),
              TransportServiceOperation::time_advance_request) == 1);
  REQUIRE(std::count(
              serverOperations.begin(),
              serverOperations.end(),
              TransportServiceOperation::time_advance_request_available) == 1);
}

}  // namespace

TEST_CASE(
    "RTIambassador requests available time advance through a configured process endpoint",
    "[integration][foundation][time-management][time-advance][transport][process-boundary][process-time-advance-available-focused][public-endpoint][rti.service.time-advance-request-available][rti.service-report][federate.callback.time-advance-grant]") {
  runProcessAvailableTaraBaseline();
}

TEST_CASE(
    "RTIambassador delivers a queued TSO at the TARA request boundary before grant through a configured process endpoint",
    "[integration][foundation][interaction-management][time-management][timestamped-interaction][tso][time-advance-request-available][process-tso-tara-queued-equal-boundary][process-boundary][public-endpoint][multi-federate][rti.service.send-interaction][rti.service.time-advance-request-available][federate.callback.receive-interaction][federate.callback.time-advance-grant][2025]") {
  runProcessTaraBoundaryCase(5, {"interaction", "grant"});
}

TEST_CASE(
    "RTIambassador delivers a TSO before the TARA request boundary before grant through a configured process endpoint",
    "[integration][foundation][interaction-management][time-management][timestamped-interaction][tso][time-advance-request-available][process-tso-tara-pregrant-boundary][process-boundary][public-endpoint][multi-federate][rti.service.send-interaction][rti.service.time-advance-request-available][federate.callback.receive-interaction][federate.callback.time-advance-grant][2025]") {
  runProcessTaraBoundaryCase(4, {"interaction", "grant"});
}

#endif
