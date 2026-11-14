#include <catch2/catch_test_macros.hpp>

#include "internal/encoding/transport_wire.hpp"
#include "internal/federation/federation_registry.hpp"
#include "internal/federation/process_federation_client.hpp"
#include "internal/federation/process_federation_service_host.hpp"
#include "internal/federation/process_federation_service_protocol.hpp"
#include "internal/federation/process_transport.hpp"
#include "internal/federation/process_transport_session.hpp"
#include "internal/federation/transport_protocol.hpp"
#include "internal/federation/transport_service_protocol.hpp"
#include "internal/platform/socket_operations.hpp"
#include "internal/runtime/ambassador_connection_support.hpp"
#include "support/private_rid_test_files.hpp"

#include <RTI/NullFederateAmbassador.h>
#include <RTI/RTI1516.h>

#include <array>
#include <atomic>
#include <chrono>
#include <cstddef>
#include <cstdint>
#include <exception>
#include <future>
#include <memory>
#include <optional>
#include <span>
#include <stdexcept>
#include <string>
#include <thread>
#include <vector>

namespace {

using rti1516_2025::HLA_EVOKED;
using rti1516_2025::NullFederateAmbassador;
using rti1516_2025::RTIambassador;
using rti1516_2025::RTIambassadorFactory;
using rti1516_2025::RtiConfiguration;
using umbra::detail::EmbeddedFederationRegistry;
using umbra::detail::FederationDefinition;
using umbra::detail::FomModuleKind;
using umbra::detail::ProcessFederationService;
using umbra::detail::ProcessFederationServiceHost;
using umbra::detail::ProcessFederationClient;
using umbra::detail::ProcessFederationClientOptions;
using umbra::detail::ProcessFederationCreateRequest;
using umbra::detail::ProcessFederationJoinRequest;
using umbra::detail::ProcessFederationResignRequest;
using umbra::detail::PrevalidatedFomModule;
using umbra::detail::ProcessTransportConnection;
using umbra::detail::ProcessTransportListener;
using umbra::detail::ProcessTransportSession;
using umbra::detail::ProcessTransportSessionState;
using umbra::detail::TransportResumeRequest;
using umbra::detail::TransportResumeStatus;
using umbra::detail::TransportResumeStatusCode;
using umbra::detail::TransportServiceMessage;
using umbra::detail::TransportServiceMessageKind;
using umbra::detail::TransportServiceOperation;
using umbra::detail::TransportServiceStatus;
using umbra::test::ScopedRidFileEnvironment;
using umbra::test::TemporaryRidFiles;
namespace platform = umbra::detail::platform;

class ScopedNativeSocket final {
 public:
  explicit ScopedNativeSocket(platform::NativeSocket socket) noexcept
      : socket_(socket) {}

  ScopedNativeSocket(ScopedNativeSocket const&) = delete;
  ScopedNativeSocket& operator=(ScopedNativeSocket const&) = delete;

  ~ScopedNativeSocket() {
    if (socket_ != platform::invalidSocket) {
      platform::closeSocket(socket_);
    }
  }

  [[nodiscard]] platform::NativeSocket get() const noexcept {
    return socket_;
  }

 private:
  platform::NativeSocket socket_ = platform::invalidSocket;
};

std::vector<std::uint8_t> receiveEncodedTransportFrame(
    platform::NativeSocket socket,
    std::chrono::steady_clock::time_point deadline) {
  std::array<std::uint8_t, umbra::detail::kTransportFrameHeaderSize> header{};
  if (platform::receiveExact(socket, header, deadline) !=
      platform::ReceiveExactResult::complete) {
    throw std::runtime_error("The process transport peer closed before a frame header.");
  }
  auto const packetSize =
      umbra::detail::transport_wire::readBigEndian32(header, 0U);
  if (packetSize < umbra::detail::kTransportFrameHeaderSize ||
      packetSize > umbra::detail::kTransportFrameHeaderSize +
          umbra::detail::kTransportMaximumPayloadBytes) {
    throw std::runtime_error("The process transport peer announced an invalid frame size.");
  }

  std::vector<std::uint8_t> encoded(header.begin(), header.end());
  encoded.resize(packetSize);
  if (packetSize > umbra::detail::kTransportFrameHeaderSize &&
      platform::receiveExact(
          socket,
          std::span<std::uint8_t>(encoded).subspan(
              umbra::detail::kTransportFrameHeaderSize),
          deadline) != platform::ReceiveExactResult::complete) {
    throw std::runtime_error("The process transport peer closed before its frame payload.");
  }
  return encoded;
}

std::vector<std::uint8_t> withInvalidTransportSequenceNumber(
    std::vector<std::uint8_t> const& validEncodedFrame) {
  if (validEncodedFrame.size() < 8U) {
    throw std::invalid_argument(
        "A sequence-number test frame must contain the protocol header.");
  }
  std::vector<std::uint8_t> malformedFrame;
  malformedFrame.reserve(validEncodedFrame.size());
  malformedFrame.insert(
      malformedFrame.end(),
      validEncodedFrame.begin(),
      validEncodedFrame.begin() + 4);
  umbra::detail::transport_wire::appendBigEndian32(
      malformedFrame,
      umbra::detail::kInvalidTransportSequenceNumber);
  malformedFrame.insert(
      malformedFrame.end(),
      validEncodedFrame.begin() + 8,
      validEncodedFrame.end());
  return malformedFrame;
}

std::unique_ptr<RTIambassador> makeRti() {
  RTIambassadorFactory factory;
  return factory.createRTIambassador();
}

RtiConfiguration configurationFor(std::uint16_t port, wchar_t const* name) {
  return RtiConfiguration::createConfiguration()
      .withConfigurationName(name)
      .withRtiAddress(L"tcp://127.0.0.1:" + std::to_wstring(port));
}

FederationDefinition recoveryFederationDefinition() {
  auto module = [](wchar_t const* designator, wchar_t const* source) {
    return PrevalidatedFomModule{
        designator,
        source,
        L"C:/fom/IEEE1516-DIF-2025.xsd",
        FomModuleKind::fom,
        L"IEEE1516-DIF-2025.xsd"};
  };
  return {
      {
          module(L"file:///fom/base.xml", L"C:/fom/base.xml"),
          module(L"file:///fom/extensions.xml", L"C:/fom/extensions.xml"),
      },
      L"HLAinteger64Time",
  };
}

class SegmentConnectionLossAmbassador final : public NullFederateAmbassador {
 public:
  void connectionLost(std::wstring const&) override {
    lost_.store(true, std::memory_order_release);
  }

  [[nodiscard]] bool lost() const noexcept {
    return lost_.load(std::memory_order_acquire);
  }

 private:
  std::atomic_bool lost_{false};
};

}  // namespace

TEST_CASE(
    "Process federation service host serves multiple public RTI clients",
    "[integration][foundation][transport][process-service-host]"
    "[public-endpoint][2025]") {
  EmbeddedFederationRegistry registry;
  ProcessFederationServiceHost host(
      registry,
      FederationDefinition{},
      {"127.0.0.1", 0U},
      {"umbra-process-federation-service-host-test", 0xB810U});
  REQUIRE(host.address().port != 0U);

  host.start();
  NullFederateAmbassador firstFederate;
  NullFederateAmbassador secondFederate;
  auto firstRti = makeRti();
  auto secondRti = makeRti();
  auto const firstConnection = firstRti->connect(
      firstFederate,
      HLA_EVOKED,
      configurationFor(host.address().port, L"process-host-first-client"));
  auto const secondConnection = secondRti->connect(
      secondFederate,
      HLA_EVOKED,
      configurationFor(host.address().port, L"process-host-second-client"));

  REQUIRE(firstConnection.addressUsed);
  REQUIRE(secondConnection.addressUsed);
  REQUIRE_THROWS(firstRti->destroyFederationExecution(L"process-host-absent"));
  REQUIRE_THROWS(secondRti->destroyFederationExecution(L"process-host-absent"));
  REQUIRE_NOTHROW(firstRti->disconnect());
  REQUIRE_NOTHROW(secondRti->disconnect());

  host.stop();
  REQUIRE_FALSE(host.running());
  REQUIRE_NOTHROW(host.rethrowIfFailed());
}

TEST_CASE(
    "Process federation service host aborts a session on a foreign-session heartbeat",
    "[integration][internal][transport][process-service-host]"
    "[process-session-protocol-abort][invalid-session-id]") {
  EmbeddedFederationRegistry registry;
  ProcessFederationServiceHost host(
      registry,
      FederationDefinition{},
      {"127.0.0.1", 0U},
      {"umbra-process-invalid-session-heartbeat-test", 0xB811U});
  host.start();

  auto connection = ProcessTransportConnection::connectClient(
      nullptr,
      {"127.0.0.1", host.address().port},
      {"umbra-process-invalid-session-heartbeat-client", 0U},
      [](std::wstring) {},
      [](std::wstring) { return false; });
  auto const sessionId = connection->sessionId();
  auto const foreignSessionId = sessionId == 1U ? 2U : 1U;

  REQUIRE(sessionId != 0U);
  REQUIRE(connection->sendFrame(umbra::detail::makeTransportHeartbeat(
      foreignSessionId,
      0U,
      umbra::detail::kInvalidTransportSequenceNumber)));

  umbra::detail::TransportFrame response;
  REQUIRE_FALSE(connection->receiveFrame(response));
  REQUIRE_FALSE(connection->open());

  host.stop();
}

TEST_CASE(
    "Process federation service host aborts a session on an invalid sequence number",
    "[integration][internal][transport][process-service-host]"
    "[process-session-protocol-abort][invalid-sequence-number]") {
  EmbeddedFederationRegistry registry;
  ProcessFederationServiceHost host(
      registry,
      FederationDefinition{},
      {"127.0.0.1", 0U},
      {"umbra-process-invalid-sequence-heartbeat-test", 0xB812U});
  host.start();

  ScopedNativeSocket socket(
      platform::connectSocket("127.0.0.1", host.address().port));
  auto const newSessionRequest = umbra::detail::encodeTransportFrame(
      umbra::detail::makeTransportNewSessionRequest());
  platform::sendAll(socket.get(), newSessionRequest);

  auto const deadline = std::chrono::steady_clock::now() +
      std::chrono::seconds(5);
  auto const sessionStatus = umbra::detail::decodeTransportNewSessionStatus(
      umbra::detail::decodeTransportFrame(
          receiveEncodedTransportFrame(socket.get(), deadline)));
  REQUIRE(sessionStatus.status ==
          umbra::detail::TransportNewSessionStatusCode::success);
  REQUIRE(sessionStatus.sessionId != 0U);

  auto const validHeartbeat = umbra::detail::encodeTransportFrame(
      umbra::detail::makeTransportHeartbeat(
          sessionStatus.sessionId,
          0U,
          umbra::detail::kInvalidTransportSequenceNumber));
  platform::sendAll(
      socket.get(),
      withInvalidTransportSequenceNumber(validHeartbeat));

  std::array<std::uint8_t, 1> closeProbe{};
  REQUIRE(platform::receiveExact(socket.get(), closeProbe, deadline) ==
          platform::ReceiveExactResult::end_of_stream);

  host.stop();
}

TEST_CASE(
    "Process federation client aborts its session on an invalid inbound sequence number",
    "[integration][internal][transport][process-session-protocol-abort]"
    "[invalid-sequence-number][federate-side]") {
  platform::initializeSocketRuntime();
  std::uint16_t port = 0U;
  ScopedNativeSocket listener(
      platform::listenSocket("127.0.0.1", 0U, port));
  REQUIRE(port != 0U);

  std::exception_ptr serverFailure;
  std::atomic_bool clientClosedSocket{false};
  std::thread server([&] {
    try {
      ScopedNativeSocket peer(platform::acceptSocket(listener.get()));
      auto const deadline = std::chrono::steady_clock::now() +
          std::chrono::seconds(5);
      auto const request = umbra::detail::decodeTransportFrame(
          receiveEncodedTransportFrame(peer.get(), deadline));
      if (request.kind != umbra::detail::TransportFrameKind::new_session ||
          umbra::detail::decodeTransportNewSessionRequest(request) !=
              umbra::detail::kTransportHla4ProtocolVersion) {
        throw std::runtime_error(
            "The process client did not send the expected New Session request.");
      }

      constexpr std::uint64_t sessionId = 0xB822U;
      auto const newSessionStatus = umbra::detail::encodeTransportFrame(
          umbra::detail::makeTransportNewSessionStatus(
              {sessionId,
               umbra::detail::TransportNewSessionStatusCode::success},
              0U));
      platform::sendAll(peer.get(), newSessionStatus);

      auto const validHeartbeat = umbra::detail::encodeTransportFrame(
          umbra::detail::makeTransportHeartbeat(
              sessionId,
              0U,
              umbra::detail::kInvalidTransportSequenceNumber));
      platform::sendAll(
          peer.get(),
          withInvalidTransportSequenceNumber(validHeartbeat));

      std::array<std::uint8_t, 1> closeProbe{};
      clientClosedSocket.store(
          platform::receiveExact(peer.get(), closeProbe, deadline) ==
          platform::ReceiveExactResult::end_of_stream,
          std::memory_order_release);
    } catch (...) {
      serverFailure = std::current_exception();
    }
  });

  std::promise<std::wstring> failurePromise;
  auto failureFuture = failurePromise.get_future();
  std::atomic_bool failureReported{false};
  std::unique_ptr<ProcessFederationClient> client;
  std::exception_ptr clientFailure;
  try {
    client = std::make_unique<ProcessFederationClient>(
        umbra::detail::ProcessTransportAddress{"127.0.0.1", port},
        umbra::detail::TransportEndpointIdentity{
            "umbra-invalid-sequence-federate-client", 0U},
        std::shared_ptr<umbra::detail::RuntimeInstrumentation>{},
        [&](std::wstring description) {
          if (!failureReported.exchange(true, std::memory_order_acq_rel)) {
            failurePromise.set_value(std::move(description));
          }
        });
  } catch (...) {
    clientFailure = std::current_exception();
  }

  auto const failureReady =
      failureFuture.wait_for(std::chrono::seconds(3)) ==
      std::future_status::ready;
  if (client) {
    client->close();
  }
  if (server.joinable()) {
    server.join();
  }
  if (clientFailure) {
    std::rethrow_exception(clientFailure);
  }
  if (serverFailure) {
    std::rethrow_exception(serverFailure);
  }

  REQUIRE(failureReady);
  REQUIRE_FALSE(failureFuture.get().empty());
  REQUIRE(clientClosedSocket.load(std::memory_order_acquire));
}

TEST_CASE(
    "Public C++ clients on one partition lose access while the shared coordinator continues",
    "[integration][internal][2025][connection][process-service-host]"
    "[authoritative-coordinator-ingress-partition]") {
  EmbeddedFederationRegistry registry;
  umbra::detail::ProcessFederationServiceOptions serviceOptions;
  std::atomic_bool defaultTimeWasSelected{false};
  serviceOptions.createFomPreparation = [&defaultTimeWasSelected](
      std::vector<std::wstring> const& fomModules,
      std::optional<std::wstring> const&,
      std::wstring const& logicalTimeImplementationName)
      -> std::optional<FederationDefinition> {
    if (fomModules != std::vector<std::wstring>{L"file:///fom/base.xml"} ||
        (!logicalTimeImplementationName.empty() &&
         logicalTimeImplementationName != L"HLAinteger64Time")) {
      return std::nullopt;
    }
    if (logicalTimeImplementationName.empty()) {
      defaultTimeWasSelected.store(true, std::memory_order_release);
    }
    return recoveryFederationDefinition();
  };
  auto service = std::make_shared<ProcessFederationService>(
      registry, FederationDefinition{}, std::move(serviceOptions));
  ProcessFederationServiceHost isolatedIngress(
      service,
      {"127.0.0.1", 0U},
      {"umbra-segment-test-isolated-ingress", 0xB821U});
  ProcessFederationServiceHost continuingIngress(
      service,
      {"127.0.0.1", 0U},
      {"umbra-segment-test-continuing-ingress", 0xB822U});
  isolatedIngress.start();
  continuingIngress.start();

  SegmentConnectionLossAmbassador firstIsolatedFederate;
  SegmentConnectionLossAmbassador secondIsolatedFederate;
  SegmentConnectionLossAmbassador survivingFederate;
  auto firstIsolatedRti = makeRti();
  auto secondIsolatedRti = makeRti();
  auto survivingRti = makeRti();
  auto connectionConfiguration = [](std::uint16_t port, wchar_t const* name) {
    return configurationFor(port, name)
        .withAdditionalSettings(L"FED_TIMEOUT_RECONNECT=0.2");
  };
  REQUIRE(firstIsolatedRti->connect(
              firstIsolatedFederate,
              HLA_EVOKED,
              connectionConfiguration(
                  isolatedIngress.address().port,
                  L"segment-isolated-first"))
              .addressUsed);
  firstIsolatedRti->createFederationExecution(
      L"authoritative-coordinator-partition",
      L"file:///fom/base.xml");
  REQUIRE(defaultTimeWasSelected.load(std::memory_order_acquire));
  static_cast<void>(firstIsolatedRti->joinFederationExecution(
      L"isolated-first", L"test", L"authoritative-coordinator-partition"));

  REQUIRE(secondIsolatedRti->connect(
              secondIsolatedFederate,
              HLA_EVOKED,
              connectionConfiguration(
                  isolatedIngress.address().port,
                  L"segment-isolated-second"))
              .addressUsed);
  static_cast<void>(secondIsolatedRti->joinFederationExecution(
      L"isolated-second", L"test", L"authoritative-coordinator-partition"));

  REQUIRE(survivingRti->connect(
              survivingFederate,
              HLA_EVOKED,
              connectionConfiguration(
                  continuingIngress.address().port,
                  L"segment-survivor"))
              .addressUsed);
  static_cast<void>(survivingRti->joinFederationExecution(
      L"survivor", L"test", L"authoritative-coordinator-partition"));

  // Stopping one ingress partitions its clients from the single service and
  // registry. The other ingress shares that same authority and remains live.
  isolatedIngress.stop();
  auto const lossDeadline =
      std::chrono::steady_clock::now() + std::chrono::seconds(3);
  while (std::chrono::steady_clock::now() < lossDeadline &&
         (!firstIsolatedFederate.lost() || !secondIsolatedFederate.lost())) {
    try {
      static_cast<void>(firstIsolatedRti->evokeMultipleCallbacks(0.0, 0.02));
    } catch (...) {
    }
    try {
      static_cast<void>(secondIsolatedRti->evokeMultipleCallbacks(0.0, 0.02));
    } catch (...) {
    }
    std::this_thread::sleep_for(std::chrono::milliseconds(1));
  }
  REQUIRE(firstIsolatedFederate.lost());
  REQUIRE(secondIsolatedFederate.lost());

  REQUIRE_THROWS_AS(
      survivingRti->createFederationExecution(
          L"authoritative-coordinator-partition",
          L"file:///fom/base.xml"),
      rti1516_2025::FederationExecutionAlreadyExists);
  SegmentConnectionLossAmbassador lateFederate;
  auto lateRti = makeRti();
  REQUIRE(lateRti->connect(
              lateFederate,
              HLA_EVOKED,
              connectionConfiguration(
                  continuingIngress.address().port,
                  L"segment-late-join"))
              .addressUsed);
  static_cast<void>(lateRti->joinFederationExecution(
      L"late-survivor", L"test", L"authoritative-coordinator-partition"));
  REQUIRE_FALSE(survivingFederate.lost());

  REQUIRE_NOTHROW(survivingRti->resignFederationExecution(rti1516_2025::NO_ACTION));
  REQUIRE_NOTHROW(lateRti->resignFederationExecution(rti1516_2025::NO_ACTION));
  REQUIRE_NOTHROW(survivingRti->destroyFederationExecution(
      L"authoritative-coordinator-partition"));
  REQUIRE_NOTHROW(survivingRti->disconnect());
  REQUIRE_NOTHROW(lateRti->disconnect());
  continuingIngress.stop();
  REQUIRE_NOTHROW(continuingIngress.rethrowIfFailed());
}

TEST_CASE(
    "Process federation service host resumes transport history on a replacement socket",
    "[integration][internal][foundation][transport][process-service-host]"
    "[process-connection-fault-recovery-history][resume-host-control]") {
  EmbeddedFederationRegistry registry;
  ProcessFederationServiceHost host(
      registry,
      recoveryFederationDefinition(),
      {"127.0.0.1", 0U},
      {"umbra-process-federation-service-resume-test", 0xB812U});
  host.start();

  auto clientState = std::make_shared<ProcessTransportSessionState>();
  auto const initialConnection = ProcessTransportConnection::connectClient(
      nullptr,
      {"127.0.0.1", host.address().port},
      {"umbra-process-resume-client", 0U},
      [](std::wstring) {},
      [](std::wstring) { return false; },
      {},
      clientState);
  ProcessTransportSession initialSession(initialConnection);
  TransportServiceMessage response;
  REQUIRE(initialSession.request(
      TransportServiceMessage{
          TransportServiceMessageKind::request,
          TransportServiceOperation::create_federation_execution,
          TransportServiceStatus::ok,
          1U,
          umbra::detail::encodeProcessFederationCreateRequest(
              ProcessFederationCreateRequest{L"resume-host-execution"})},
      response));
  REQUIRE(response.status == TransportServiceStatus::ok);
  REQUIRE(initialSession.request(
      TransportServiceMessage{
          TransportServiceMessageKind::request,
          TransportServiceOperation::join_federation_execution,
          TransportServiceStatus::ok,
          2U,
          umbra::detail::encodeProcessFederationJoinRequest(
              ProcessFederationJoinRequest{
                  L"resume-host-execution",
                  L"resume-host-federate",
                  L"resume-host-federate"})},
      response));
  REQUIRE(response.status == TransportServiceStatus::ok);
  auto const joinResult =
      umbra::detail::decodeProcessFederationJoinResult(response.payload);
  REQUIRE(joinResult.federateId != 0U);
  auto const sessionId = initialConnection->sessionId();
  auto const lastReceivedRti =
      initialConnection->messageHistory().lastReceivedSequenceNumber();
  initialConnection->close();

  auto const replacementConnection = ProcessTransportConnection::connectClient(
      nullptr,
      {"127.0.0.1", host.address().port},
      {"umbra-process-resume-client", 0U},
      [](std::wstring) {},
      [](std::wstring) { return false; },
      {},
      clientState);
  REQUIRE(replacementConnection->sessionId() == sessionId);
  ProcessTransportSession replacementSession(replacementConnection);
  REQUIRE(replacementSession.sendResumeRequest(TransportResumeRequest{
      sessionId,
      lastReceivedRti,
      umbra::detail::kInvalidTransportSequenceNumber}));
  TransportResumeStatus resumeStatus;
  REQUIRE(replacementSession.receiveResumeStatus(resumeStatus));
  REQUIRE(resumeStatus.sessionId == sessionId);
  REQUIRE(resumeStatus.status == TransportResumeStatusCode::ok);
  REQUIRE(replacementSession.replayOutgoingAfter(
      resumeStatus.lastReceivedFederateSequenceNumber));

  TransportServiceMessage resumedResponse;
  REQUIRE(replacementSession.request(
      TransportServiceMessage{
          TransportServiceMessageKind::request,
          TransportServiceOperation::resign_federation_execution,
          TransportServiceStatus::ok,
          3U,
          umbra::detail::encodeProcessFederationResignRequest(
              ProcessFederationResignRequest{
                  L"resume-host-execution",
                  joinResult.federateId,
                  rti1516_2025::NO_ACTION})},
      resumedResponse));
  REQUIRE(resumedResponse.status == TransportServiceStatus::ok);
  replacementConnection->close();
  host.stop();
  REQUIRE_NOTHROW(host.rethrowIfFailed());
}

TEST_CASE(
    "Process federation service host purges inactive joined sessions after RTI_TIMEOUT_PURGE",
    "[integration][internal][foundation][transport][process-service-host]"
    "[process-connection-fault-recovery-history][resume-session-purge]") {
  EmbeddedFederationRegistry registry;
  ProcessFederationServiceHost host(
      registry,
      recoveryFederationDefinition(),
      {"127.0.0.1", 0U},
      {"umbra-process-federation-service-purge-test", 0xB814U},
      {},
      umbra::detail::ProcessFederationServiceHostOptions{
          std::chrono::milliseconds(40), std::chrono::milliseconds(160)});
  host.start();

  auto clientState = std::make_shared<ProcessTransportSessionState>();
  auto const connection = ProcessTransportConnection::connectClient(
      nullptr,
      {"127.0.0.1", host.address().port},
      {"umbra-process-purge-client", 0U},
      [](std::wstring) {},
      [](std::wstring) { return false; },
      {},
      clientState);
  ProcessTransportSession session(connection);
  TransportServiceMessage response;
  REQUIRE(session.request(
      TransportServiceMessage{
          TransportServiceMessageKind::request,
          TransportServiceOperation::create_federation_execution,
          TransportServiceStatus::ok,
          1U,
          umbra::detail::encodeProcessFederationCreateRequest(
              ProcessFederationCreateRequest{L"purge-host-execution"})},
      response));
  REQUIRE(response.status == TransportServiceStatus::ok);
  REQUIRE(session.request(
      TransportServiceMessage{
          TransportServiceMessageKind::request,
          TransportServiceOperation::join_federation_execution,
          TransportServiceStatus::ok,
          2U,
          umbra::detail::encodeProcessFederationJoinRequest(
              ProcessFederationJoinRequest{
                  L"purge-host-execution",
                  L"purge-host-federate",
                  L"purge-host-federate"})},
      response));
  REQUIRE(response.status == TransportServiceStatus::ok);
  auto const joinResult =
      umbra::detail::decodeProcessFederationJoinResult(response.payload);
  REQUIRE(registry.memberById(L"purge-host-execution", joinResult.federateId)
              .has_value());

  auto const purgeDeadline =
      std::chrono::steady_clock::now() + std::chrono::seconds(2);
  while (registry.memberById(L"purge-host-execution", joinResult.federateId)
             .has_value() &&
         std::chrono::steady_clock::now() < purgeDeadline) {
    std::this_thread::sleep_for(std::chrono::milliseconds(10));
  }
  REQUIRE_FALSE(registry.memberById(
      L"purge-host-execution", joinResult.federateId).has_value());
  REQUIRE_NOTHROW(host.rethrowIfFailed());
}

TEST_CASE(
    "Process federation service host consumes RID timeout settings without injected options",
    "[integration][internal][transport][process-service-host]"
    "[table31-timeout-settings-rid][no-normative-mapping]") {
  TemporaryRidFiles files;
  auto const ridFile = files.writePrivateFile(
      "host-timeouts.ini",
      "[rti.default]\n"
      "RTI_TIMEOUT_HEART=0.04\n"
      "RTI_TIMEOUT_PURGE=1.0\n");
  ScopedRidFileEnvironment ridEnvironment(ridFile);

  EmbeddedFederationRegistry registry;
  ProcessFederationServiceHost host(
      registry,
      recoveryFederationDefinition(),
      {"127.0.0.1", 0U},
      {"umbra-process-federation-service-rid-timeout-test", 0xB816U});
  host.start();

  auto clientState = std::make_shared<ProcessTransportSessionState>();
  auto const connection = ProcessTransportConnection::connectClient(
      nullptr,
      {"127.0.0.1", host.address().port},
      {"umbra-process-rid-timeout-client", 0U},
      [](std::wstring) {},
      [](std::wstring) { return false; },
      {},
      clientState);
  ProcessTransportSession session(connection);
  TransportServiceMessage response;
  REQUIRE(session.request(
      TransportServiceMessage{
          TransportServiceMessageKind::request,
          TransportServiceOperation::create_federation_execution,
          TransportServiceStatus::ok,
          1U,
          umbra::detail::encodeProcessFederationCreateRequest(
              ProcessFederationCreateRequest{L"rid-timeout-host-execution"})},
      response));
  REQUIRE(response.status == TransportServiceStatus::ok);
  REQUIRE(session.request(
      TransportServiceMessage{
          TransportServiceMessageKind::request,
          TransportServiceOperation::join_federation_execution,
          TransportServiceStatus::ok,
          2U,
          umbra::detail::encodeProcessFederationJoinRequest(
              ProcessFederationJoinRequest{
                  L"rid-timeout-host-execution",
                  L"rid-timeout-host-federate",
                  L"rid-timeout-host-federate"})},
      response));
  REQUIRE(response.status == TransportServiceStatus::ok);
  auto const joinResult =
      umbra::detail::decodeProcessFederationJoinResult(response.payload);
  REQUIRE(registry.memberById(
              L"rid-timeout-host-execution", joinResult.federateId)
              .has_value());

  auto const sessionId = connection->sessionId();
  auto const lastReceivedRti =
      connection->messageHistory().lastReceivedSequenceNumber();
  std::this_thread::sleep_for(std::chrono::milliseconds(120));
  REQUIRE(registry.memberById(
              L"rid-timeout-host-execution", joinResult.federateId)
              .has_value());

  auto const replacementConnection = ProcessTransportConnection::connectClient(
      nullptr,
      {"127.0.0.1", host.address().port},
      {"umbra-process-rid-timeout-client", 0U},
      [](std::wstring) {},
      [](std::wstring) { return false; },
      {},
      clientState);
  REQUIRE(replacementConnection->sessionId() == sessionId);
  ProcessTransportSession replacementSession(replacementConnection);
  REQUIRE(replacementSession.sendResumeRequest(TransportResumeRequest{
      sessionId,
      lastReceivedRti,
      umbra::detail::kInvalidTransportSequenceNumber}));
  TransportResumeStatus resumeStatus;
  REQUIRE(replacementSession.receiveResumeStatus(resumeStatus));
  REQUIRE(resumeStatus.sessionId == sessionId);
  REQUIRE(resumeStatus.status == TransportResumeStatusCode::ok);
  REQUIRE(replacementSession.replayOutgoingAfter(
      resumeStatus.lastReceivedFederateSequenceNumber));
  REQUIRE(registry.memberById(
              L"rid-timeout-host-execution", joinResult.federateId)
              .has_value());

  auto const purgeDeadline =
      std::chrono::steady_clock::now() + std::chrono::seconds(3);
  while (registry.memberById(
             L"rid-timeout-host-execution", joinResult.federateId)
             .has_value() &&
         std::chrono::steady_clock::now() < purgeDeadline) {
    std::this_thread::sleep_for(std::chrono::milliseconds(10));
  }
  REQUIRE_FALSE(registry.memberById(
      L"rid-timeout-host-execution", joinResult.federateId).has_value());
  REQUIRE_NOTHROW(host.rethrowIfFailed());
  connection->close();
  replacementConnection->close();
  host.stop();
}

TEST_CASE(
    "Process federation client heartbeats preserve an idle joined session",
    "[integration][internal][foundation][transport][process-service-host]"
    "[process-connection-fault-recovery-history][federate-heartbeat]") {
  EmbeddedFederationRegistry registry;
  ProcessFederationServiceHost host(
      registry,
      recoveryFederationDefinition(),
      {"127.0.0.1", 0U},
      {"umbra-process-federation-service-heartbeat-test", 0xB815U},
      {},
      umbra::detail::ProcessFederationServiceHostOptions{
          std::chrono::milliseconds(120), std::chrono::milliseconds(240)});
  host.start();

  ProcessFederationClientOptions clientOptions;
  clientOptions.federateHeartbeatInterval = std::chrono::milliseconds(20);
  ProcessFederationClient client(
      {"127.0.0.1", host.address().port},
      {"umbra-process-heartbeat-client", 0U},
      {},
      {},
      {},
      clientOptions);
  client.createFederationExecution(L"heartbeat-host-execution");
  auto const joined = client.joinFederationExecution(
      L"heartbeat-host-execution",
      L"heartbeat-host-federate",
      L"heartbeat-host-federate");
  REQUIRE(joined.federateId != 0U);

  std::this_thread::sleep_for(std::chrono::milliseconds(500));
  REQUIRE(registry.memberById(
      L"heartbeat-host-execution", joined.federateId).has_value());
  auto const members =
      client.listFederationExecutionMembers(L"heartbeat-host-execution");
  REQUIRE(members.federationExists);
  REQUIRE(members.members.size() == 1U);
  REQUIRE(members.members.front().name == L"heartbeat-host-federate");

  client.resignFederationExecution(
      L"heartbeat-host-execution",
      joined.federateId,
      rti1516_2025::NO_ACTION);
  client.close();
  host.stop();
  REQUIRE_NOTHROW(host.rethrowIfFailed());
}

TEST_CASE(
    "Public C++ FED_INT_HEART configuration keeps an idle process session active",
    "[integration][internal][connection][configuration][process-service-host]"
    "[federate-heartbeat][federate-heartbeat-configuration]") {
  EmbeddedFederationRegistry registry;
  ProcessFederationServiceHost host(
      registry,
      FederationDefinition{},
      {"127.0.0.1", 0U},
      {"umbra-process-public-heartbeat-configuration-test", 0xB816U},
      {},
      umbra::detail::ProcessFederationServiceHostOptions{
          std::chrono::milliseconds(120), std::chrono::milliseconds(240)});
  host.start();

  NullFederateAmbassador federate;
  auto rti = makeRti();
  auto configuration = configurationFor(
                           host.address().port,
                           L"process-public-heartbeat-configuration-client")
                           .withAdditionalSettings(L"FED_INT_HEART=0.02");
  auto const connection = rti->connect(federate, HLA_EVOKED, configuration);
  REQUIRE(connection.addressUsed);
  REQUIRE(connection.additionalSettingsResult == rti1516_2025::SETTINGS_APPLIED);

  std::this_thread::sleep_for(std::chrono::milliseconds(600));
  REQUIRE_THROWS_AS(
      rti->destroyFederationExecution(L"missing-heartbeat-execution"),
      rti1516_2025::FederationExecutionDoesNotExist);

  REQUIRE_NOTHROW(rti->disconnect());
  host.stop();
  REQUIRE_NOTHROW(host.rethrowIfFailed());
}

TEST_CASE(
    "Public C++ FED_TIMEOUT_RECONNECT configuration sets the process recovery window",
    "[integration][internal][connection][configuration][process-service-host]"
    "[federate-reconnect-timeout][federate-reconnect-timeout-configuration]") {
  bool defaultsApplied = false;
  bool defaultsFailedToParse = false;
  auto const defaults =
      rti1516_2025::umbra_binding_detail::
          configuredAmbassadorProcessFederationClientOptions(
              nullptr,
              defaultsApplied,
              defaultsFailedToParse);
  REQUIRE_FALSE(defaultsApplied);
  REQUIRE_FALSE(defaultsFailedToParse);
  REQUIRE(defaults.transportRecoveryWindow == std::chrono::seconds(600));

  auto const settings = RtiConfiguration::createConfiguration()
                            .withAdditionalSettings(
                                L"FED_TIMEOUT_RECONNECT=0.02");
  bool settingsApplied = false;
  bool settingsFailedToParse = false;
  auto const clientOptions =
      rti1516_2025::umbra_binding_detail::
          configuredAmbassadorProcessFederationClientOptions(
              &settings,
              settingsApplied,
              settingsFailedToParse);
  REQUIRE(settingsApplied);
  REQUIRE_FALSE(settingsFailedToParse);
  REQUIRE(clientOptions.transportRecoveryWindow ==
          std::chrono::milliseconds(20));

  auto const duplicateSettings = RtiConfiguration::createConfiguration()
                                    .withAdditionalSettings(
                                        L"FED_TIMEOUT_RECONNECT=1;"
                                        L"FED_TIMEOUT_RECONNECT=2");
  bool duplicateSettingsApplied = false;
  bool duplicateSettingsFailedToParse = false;
  auto const duplicateOptions =
      rti1516_2025::umbra_binding_detail::
          configuredAmbassadorProcessFederationClientOptions(
              &duplicateSettings,
              duplicateSettingsApplied,
              duplicateSettingsFailedToParse);
  REQUIRE_FALSE(duplicateSettingsApplied);
  REQUIRE(duplicateSettingsFailedToParse);
  REQUIRE(duplicateOptions.transportRecoveryWindow ==
          std::chrono::seconds(600));

  EmbeddedFederationRegistry registry;
  ProcessFederationServiceHost host(
      registry,
      FederationDefinition{},
      {"127.0.0.1", 0U},
      {"umbra-process-public-reconnect-configuration-test", 0xB817U});
  host.start();

  NullFederateAmbassador federate;
  auto rti = makeRti();
  auto configuration = configurationFor(
                           host.address().port,
                           L"process-public-reconnect-configuration-client")
                           .withAdditionalSettings(
                               L"FED_TIMEOUT_RECONNECT=0.02");
  auto const connection = rti->connect(federate, HLA_EVOKED, configuration);
  REQUIRE(connection.addressUsed);
  REQUIRE(connection.additionalSettingsResult ==
          rti1516_2025::SETTINGS_APPLIED);

  REQUIRE_NOTHROW(rti->disconnect());
  host.stop();
  REQUIRE_NOTHROW(host.rethrowIfFailed());
}

TEST_CASE(
    "Process federation client reports failure after FED_TIMEOUT_RECONNECT expires",
    "[integration][internal][connection][configuration][process-service-host]"
    "[federate-reconnect-timeout][federate-reconnect-timeout-effect]") {
  auto listener = ProcessTransportListener::listen({"127.0.0.1", 0U});
  REQUIRE(listener != nullptr);
  std::promise<std::shared_ptr<ProcessTransportConnection>> acceptedPromise;
  auto acceptedFuture = acceptedPromise.get_future();
  std::thread acceptThread([&] {
    try {
      acceptedPromise.set_value(listener->accept(
          nullptr,
          {"umbra-process-reconnect-timeout-test-host", 0U},
          {},
          {}));
    } catch (...) {
      acceptedPromise.set_exception(std::current_exception());
    }
  });
  std::atomic_uint32_t reportedTransportFailures{0U};
  std::atomic_int64_t outageStartedNanoseconds{0};
  std::atomic_int64_t reportedFailureElapsedMilliseconds{-1};
  ProcessFederationClientOptions clientOptions;
  clientOptions.federateHeartbeatInterval = std::chrono::seconds(10);
  clientOptions.transportRecoveryWindow = std::chrono::milliseconds(250);
  ProcessFederationClient client(
      {"127.0.0.1", listener->address().port},
      {"umbra-process-reconnect-timeout-effect-client", 0U},
      {},
      [&reportedTransportFailures,
       &outageStartedNanoseconds,
       &reportedFailureElapsedMilliseconds](std::wstring) {
        auto const startedAt = std::chrono::steady_clock::time_point(
            std::chrono::nanoseconds(
                outageStartedNanoseconds.load(std::memory_order_acquire)));
        reportedFailureElapsedMilliseconds.store(
            std::chrono::duration_cast<std::chrono::milliseconds>(
                std::chrono::steady_clock::now() - startedAt)
                .count(),
            std::memory_order_release);
        reportedTransportFailures.fetch_add(1U, std::memory_order_relaxed);
      },
      {},
      clientOptions);
  auto serverConnection = acceptedFuture.get();
  acceptThread.join();

  auto const outageStarted = std::chrono::steady_clock::now();
  outageStartedNanoseconds.store(
      std::chrono::duration_cast<std::chrono::nanoseconds>(
          outageStarted.time_since_epoch())
          .count(),
      std::memory_order_release);
  listener->close();
  serverConnection->close();
  auto const callbackDeadline = outageStarted + std::chrono::seconds(3);
  while (reportedTransportFailures.load(std::memory_order_relaxed) == 0U &&
         std::chrono::steady_clock::now() < callbackDeadline) {
    if (reportedTransportFailures.load(std::memory_order_relaxed) == 0U) {
      std::this_thread::sleep_for(std::chrono::milliseconds(1));
    }
  }

  REQUIRE(reportedTransportFailures.load(std::memory_order_relaxed) > 0U);
  auto const recoveryElapsed =
      reportedFailureElapsedMilliseconds.load(std::memory_order_acquire);
  REQUIRE(recoveryElapsed >= 200);
  REQUIRE(recoveryElapsed < 1500);
  client.close();
}

TEST_CASE(
    "Public C++ FED_TIMEOUT_HEART starts recovery when RTI heartbeat replies stop",
    "[integration][internal][connection][configuration][process-service-host]"
    "[federate-heartbeat-timeout][federate-heartbeat-timeout-effect]") {
  bool defaultsApplied = false;
  bool defaultsFailedToParse = false;
  auto const defaults =
      rti1516_2025::umbra_binding_detail::
          configuredAmbassadorProcessFederationClientOptions(
              nullptr,
              defaultsApplied,
              defaultsFailedToParse);
  REQUIRE_FALSE(defaultsApplied);
  REQUIRE_FALSE(defaultsFailedToParse);
  REQUIRE(defaults.federateHeartbeatTimeout == std::chrono::seconds(180));

  std::atomic_uint32_t resumedSessions{0U};
  umbra::detail::ProcessFederationServiceHostOptions hostOptions{
      std::chrono::seconds(5), std::chrono::seconds(10)};
  hostOptions.replyToFederateHeartbeats = false;
  hostOptions.onSessionResumedForTesting = [&resumedSessions] {
    resumedSessions.fetch_add(1U, std::memory_order_relaxed);
  };
  EmbeddedFederationRegistry registry;
  ProcessFederationServiceHost host(
      registry,
      FederationDefinition{},
      {"127.0.0.1", 0U},
      {"umbra-process-public-heartbeat-timeout-effect-test", 0xB819U},
      {},
      std::move(hostOptions));
  host.start();

  NullFederateAmbassador federate;
  auto rti = makeRti();
  auto configuration = configurationFor(
                           host.address().port,
                           L"process-public-heartbeat-timeout-client")
                           .withAdditionalSettings(
                               L"FED_INT_HEART=0.02;FED_TIMEOUT_HEART=0.12;"
                               L"FED_TIMEOUT_RECONNECT=2");
  auto const connection = rti->connect(federate, HLA_EVOKED, configuration);
  REQUIRE(connection.addressUsed);
  REQUIRE(connection.additionalSettingsResult == rti1516_2025::SETTINGS_APPLIED);

  auto const deadline = std::chrono::steady_clock::now() +
      std::chrono::seconds(3);
  while (resumedSessions.load(std::memory_order_relaxed) == 0U &&
         std::chrono::steady_clock::now() < deadline) {
    std::this_thread::sleep_for(std::chrono::milliseconds(10));
  }
  REQUIRE(resumedSessions.load(std::memory_order_relaxed) > 0U);
  REQUIRE_THROWS_AS(
      rti->destroyFederationExecution(L"missing-heartbeat-timeout-execution"),
      rti1516_2025::FederationExecutionDoesNotExist);

  REQUIRE_NOTHROW(rti->disconnect());
  host.stop();
  REQUIRE_NOTHROW(host.rethrowIfFailed());
}

TEST_CASE(
    "Process federation client resumes its joined session after socket loss",
    "[integration][internal][foundation][transport][process-service-host]"
    "[process-connection-fault-recovery-history][resume-client-reconnect]") {
  EmbeddedFederationRegistry registry;
  ProcessFederationServiceHost host(
      registry,
      recoveryFederationDefinition(),
      {"127.0.0.1", 0U},
      {"umbra-process-federation-service-client-resume-test", 0xB816U});
  host.start();

  std::atomic_uint32_t reportedTransportFailures{0U};
  ProcessFederationClient client(
      {"127.0.0.1", host.address().port},
      {"umbra-process-client-resume-client", 0U},
      {},
      [&reportedTransportFailures](std::wstring) {
        reportedTransportFailures.fetch_add(1U, std::memory_order_relaxed);
      },
      {});
  client.createFederationExecution(L"client-resume-host-execution");
  auto const joined = client.joinFederationExecution(
      L"client-resume-host-execution",
      L"client-resume-host-federate",
      L"client-resume-host-federate");
  REQUIRE(joined.federateId != 0U);

  client.disconnectTransportForTesting();
  auto const members = client.listFederationExecutionMembers(
      L"client-resume-host-execution");
  REQUIRE(members.federationExists);
  REQUIRE(members.members.size() == 1U);
  REQUIRE(members.members.front().name == L"client-resume-host-federate");
  REQUIRE(registry.memberById(
      L"client-resume-host-execution", joined.federateId).has_value());
  REQUIRE(reportedTransportFailures.load(std::memory_order_relaxed) == 0U);

  client.resignFederationExecution(
      L"client-resume-host-execution",
      joined.federateId,
      rti1516_2025::NO_ACTION);
  client.close();
  host.stop();
  REQUIRE_NOTHROW(host.rethrowIfFailed());
}

TEST_CASE(
    "Process federation client rejects a resume LRF not covered by retained history",
    "[integration][internal][foundation][transport][process-service-host]"
    "[process-connection-fault-recovery-history][resume-client-history-gap]") {
  auto listener = ProcessTransportListener::listen({"127.0.0.1", 0U});
  REQUIRE(listener != nullptr);

  std::shared_ptr<ProcessTransportSessionState> serverState;
  auto resolveServerState = [&serverState](std::uint64_t sessionId) {
    if (sessionId == 0U) {
      return std::make_shared<ProcessTransportSessionState>();
    }
    if (serverState && serverState->sessionId() == sessionId) {
      return serverState;
    }
    return std::shared_ptr<ProcessTransportSessionState>{};
  };

  std::exception_ptr serverFailure;
  bool receivedAndAcknowledgedFirstRequest = false;
  bool receivedPendingRequest = false;
  bool reportedLrfBeforeRetainedHistory = false;
  std::thread server([&] {
    try {
      auto const acceptConnection = [&] {
        return listener->accept(
            nullptr,
            {"umbra-process-resume-history-gap-service", 0U},
            [](std::wstring) {},
            [](std::wstring) { return false; },
            {},
            {},
            resolveServerState);
      };

      auto initialConnection = acceptConnection();
      serverState = initialConnection->sessionState();
      ProcessTransportSession initialSession(initialConnection);
      TransportServiceMessage firstRequest;
      if (!initialSession.receive(firstRequest) ||
          firstRequest.kind != TransportServiceMessageKind::request ||
          firstRequest.operation !=
              TransportServiceOperation::create_federation_execution) {
        throw std::runtime_error(
            "client did not send its first federation service request");
      }
      receivedAndAcknowledgedFirstRequest = true;
      if (!initialSession.send(TransportServiceMessage{
              TransportServiceMessageKind::response,
              firstRequest.operation,
              TransportServiceStatus::ok,
              firstRequest.requestId,
              {}})) {
        throw std::runtime_error(
            "service could not acknowledge the first client request");
      }

      TransportServiceMessage pendingRequest;
      if (!initialSession.receive(pendingRequest) ||
          pendingRequest.kind != TransportServiceMessageKind::request ||
          pendingRequest.operation !=
              TransportServiceOperation::list_federation_executions) {
        throw std::runtime_error(
            "client did not send its pending federation-list request");
      }
      receivedPendingRequest = true;
      initialConnection->close();

      auto replacementConnection = acceptConnection();
      ProcessTransportSession replacementSession(replacementConnection);
      TransportResumeRequest resumeRequest;
      if (!replacementSession.receiveResumeRequest(resumeRequest)) {
        throw std::runtime_error("client did not send a Resume Request");
      }
      if (resumeRequest.sessionId != replacementConnection->sessionId() ||
          resumeRequest.lastReceivedRtiSequenceNumber != 0U ||
          resumeRequest.oldestAvailableFederateSequenceNumber != 1U) {
        throw std::runtime_error(
            "client Resume Request did not describe its retained request suffix");
      }

      reportedLrfBeforeRetainedHistory = true;
      if (!replacementSession.sendResumeStatus(TransportResumeStatus{
              replacementConnection->sessionId(),
              TransportResumeStatusCode::ok,
              umbra::detail::kInvalidTransportSequenceNumber})) {
        throw std::runtime_error("service could not send the Resume Status");
      }
    } catch (...) {
      serverFailure = std::current_exception();
    }
  });

  ProcessFederationClient client(
      {"127.0.0.1", listener->address().port},
      {"umbra-process-resume-history-gap-client", 0U});
  auto pendingRequest = std::async(std::launch::async, [&client] {
    client.createFederationExecution(L"resume-history-gap-execution");
    static_cast<void>(client.listFederationExecutions());
  });
  auto const requestStatus = pendingRequest.wait_for(std::chrono::seconds(5));
  bool clientRejectedInsufficientHistory = false;
  std::string clientFailureMessage;
  if (requestStatus == std::future_status::ready) {
    try {
      pendingRequest.get();
    } catch (std::exception const& error) {
      clientFailureMessage = error.what();
      clientRejectedInsufficientHistory =
          clientFailureMessage.find(
              "retained federate service history is insufficient") !=
          std::string::npos;
    }
  }

  client.close();
  listener->close();
  if (server.joinable()) {
    server.join();
  }
  if (requestStatus != std::future_status::ready) {
    try {
      pendingRequest.get();
    } catch (...) {
    }
  }

  REQUIRE(requestStatus == std::future_status::ready);
  REQUIRE_FALSE(serverFailure);
  REQUIRE(receivedAndAcknowledgedFirstRequest);
  REQUIRE(receivedPendingRequest);
  REQUIRE(reportedLrfBeforeRetainedHistory);
  INFO("Client observed: " << clientFailureMessage);
  REQUIRE(clientRejectedInsufficientHistory);
}

TEST_CASE(
    "Process federation service host closes a session when resume history is insufficient",
    "[integration][internal][foundation][transport][process-service-host]"
    "[process-connection-fault-recovery-history][resume-host-control]") {
  EmbeddedFederationRegistry registry;
  ProcessFederationServiceHost host(
      registry,
      FederationDefinition{},
      {"127.0.0.1", 0U},
      {"umbra-process-federation-service-resume-reject-test", 0xB813U});
  host.start();

  auto clientState = std::make_shared<ProcessTransportSessionState>();
  auto const connection = ProcessTransportConnection::connectClient(
      nullptr,
      {"127.0.0.1", host.address().port},
      {"umbra-process-resume-reject-client", 0U},
      [](std::wstring) {},
      [](std::wstring) { return false; },
      {},
      clientState);
  ProcessTransportSession session(connection);
  auto const sessionId = connection->sessionId();
  TransportServiceMessage response;
  REQUIRE(session.request(
      TransportServiceMessage{
          TransportServiceMessageKind::request,
          TransportServiceOperation::list_federation_executions,
          TransportServiceStatus::ok,
          1U,
          {}},
      response));
  REQUIRE(response.status == TransportServiceStatus::ok);
  REQUIRE(session.request(
      TransportServiceMessage{
          TransportServiceMessageKind::request,
          TransportServiceOperation::list_federation_executions,
          TransportServiceStatus::ok,
          2U,
          {}},
      response));
  REQUIRE(response.status == TransportServiceStatus::ok);
  connection->close();

  auto const replacementConnection = ProcessTransportConnection::connectClient(
      nullptr,
      {"127.0.0.1", host.address().port},
      {"umbra-process-resume-reject-client", 0U},
      [](std::wstring) {},
      [](std::wstring) { return false; },
      {},
      clientState);
  ProcessTransportSession replacementSession(replacementConnection);
  REQUIRE(replacementConnection->sessionId() == sessionId);
  REQUIRE(replacementSession.sendResumeRequest(TransportResumeRequest{
      sessionId,
      umbra::detail::kInvalidTransportSequenceNumber,
      1U}));

  TransportResumeStatus status;
  REQUIRE(replacementSession.receiveResumeStatus(status));
  REQUIRE(status.sessionId == sessionId);
  REQUIRE(status.status ==
          TransportResumeStatusCode::insufficient_buffered_messages);
  TransportServiceMessage unexpectedServiceTraffic;
  REQUIRE_FALSE(replacementSession.receive(unexpectedServiceTraffic));

  replacementConnection->close();
  host.stop();
  REQUIRE_NOTHROW(host.rethrowIfFailed());
}

TEST_CASE(
    "Process federation service host shutdown interrupts a stalled New Session startup",
    "[integration][internal][foundation][transport][process-service-host]"
    "[process-service-host-lifecycle]") {
  EmbeddedFederationRegistry registry;
  ProcessFederationServiceHost host(
      registry,
      FederationDefinition{},
      {"127.0.0.1", 0U},
      {"umbra-process-federation-service-host-stalled-hello-test", 0xB811U});
  host.start();

  auto const stalledPeer =
      platform::connectSocket("127.0.0.1", host.address().port);
  std::this_thread::sleep_for(std::chrono::milliseconds(50));

  std::promise<void> stopped;
  auto stoppedFuture = stopped.get_future();
  std::thread stopThread([&host, &stopped] {
    host.stop();
    stopped.set_value();
  });
  auto const stoppedBeforePeerClose =
      stoppedFuture.wait_for(std::chrono::seconds(2)) ==
      std::future_status::ready;

  platform::closeSocket(stalledPeer);
  if (!stoppedBeforePeerClose) {
    static_cast<void>(stoppedFuture.wait_for(std::chrono::seconds(2)));
  }
  stopThread.join();

  REQUIRE(stoppedBeforePeerClose);
  REQUIRE_FALSE(host.running());
  REQUIRE_NOTHROW(host.rethrowIfFailed());
}
