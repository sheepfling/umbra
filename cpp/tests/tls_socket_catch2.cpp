#include "internal/platform/socket_operations.hpp"
#include "internal/platform/tls_socket.hpp"
#include "internal/federation/process_transport.hpp"
#include "support/tls_test_certificate.hpp"

#include <openssl/ssl.h>

#include <catch2/catch_test_macros.hpp>

#include <array>
#include <atomic>
#include <chrono>
#include <exception>
#include <filesystem>
#include <memory>
#include <stdexcept>
#include <string>
#include <thread>
#include <utility>

namespace {

using umbra::detail::platform::NativeSocket;
using umbra::detail::platform::ReceiveExactResult;
using umbra::detail::platform::TlsClientConfiguration;
using umbra::detail::platform::TlsServerConfiguration;
using umbra::detail::platform::TlsSocket;
using umbra::test::tls::TemporaryDirectory;
using umbra::test::tls::writeTestCertificate;

}  // namespace

TEST_CASE(
    "Private TLS socket verifies its peer and negotiates TLS 1.3",
    "[internal][transport][tls][no-normative-mapping]") {
  using namespace umbra::detail::platform;

  initializeSocketRuntime();
  TemporaryDirectory temporaryDirectory;
  auto const certificatePath = temporaryDirectory.path() / "localhost.crt";
  auto const privateKeyPath = temporaryDirectory.path() / "localhost.key";
  writeTestCertificate(certificatePath, privateKeyPath);

  std::uint16_t port = 0U;
  auto const listener = listenSocket("127.0.0.1", 0U, port);
  REQUIRE(port != 0U);

  std::array<std::uint8_t, 4U> serverReceived{};
  std::atomic_int serverProtocolVersion{0};
  std::exception_ptr serverFailure;
  std::thread server([&] {
    NativeSocket accepted = invalidSocket;
    try {
      accepted = acceptSocket(listener);
      auto secure = TlsSocket::accept(
          std::exchange(accepted, invalidSocket),
          TlsServerConfiguration{certificatePath, privateKeyPath});
      std::exception_ptr serverSendFailure;
      std::thread sender([&] {
        try {
          constexpr std::array<std::uint8_t, 4U> response{'p', 'o', 'n', 'g'};
          secure.sendAll(response);
        } catch (...) {
          serverSendFailure = std::current_exception();
        }
      });
      ReceiveExactResult receiveResult = ReceiveExactResult::end_of_stream;
      std::exception_ptr serverReceiveFailure;
      try {
        receiveResult = secure.receiveExact(serverReceived);
      } catch (...) {
        serverReceiveFailure = std::current_exception();
      }
      sender.join();
      if (serverReceiveFailure) {
        std::rethrow_exception(serverReceiveFailure);
      }
      if (serverSendFailure) {
        std::rethrow_exception(serverSendFailure);
      }
      if (receiveResult != ReceiveExactResult::complete) {
        throw std::runtime_error("The TLS test client closed before sending its payload.");
      }
      serverProtocolVersion.store(
          secure.negotiatedProtocolVersion(), std::memory_order_relaxed);
    } catch (...) {
      if (accepted != invalidSocket) {
        closeSocket(accepted);
      }
      serverFailure = std::current_exception();
    }
  });

  std::array<std::uint8_t, 4U> response{};
  int clientProtocolVersion = 0;
  std::exception_ptr clientFailure;
  try {
    auto clientSocket = connectSocket("127.0.0.1", port);
    auto secure = TlsSocket::connect(
        std::exchange(clientSocket, invalidSocket),
        TlsClientConfiguration{certificatePath, "localhost"});
    constexpr std::array<std::uint8_t, 4U> request{'p', 'i', 'n', 'g'};
    std::exception_ptr clientSendFailure;
    std::exception_ptr clientReceiveFailure;
    std::thread receiver([&] {
      try {
        if (secure.receiveExact(response) != ReceiveExactResult::complete) {
          throw std::runtime_error("The TLS test server closed before replying.");
        }
      } catch (...) {
        clientReceiveFailure = std::current_exception();
      }
    });
    try {
      secure.sendAll(request);
    } catch (...) {
      clientSendFailure = std::current_exception();
      secure.close();
    }
    receiver.join();
    if (clientSendFailure) {
      std::rethrow_exception(clientSendFailure);
    }
    if (clientReceiveFailure) {
      std::rethrow_exception(clientReceiveFailure);
    }
    clientProtocolVersion = secure.negotiatedProtocolVersion();
  } catch (...) {
    clientFailure = std::current_exception();
  }
  server.join();
  closeSocket(listener);

  if (clientFailure) {
    std::rethrow_exception(clientFailure);
  }
  if (serverFailure) {
    std::rethrow_exception(serverFailure);
  }

  constexpr std::array<std::uint8_t, 4U> request{'p', 'i', 'n', 'g'};
  constexpr std::array<std::uint8_t, 4U> expectedResponse{'p', 'o', 'n', 'g'};
  CHECK(serverReceived == request);
  CHECK(response == expectedResponse);
  CHECK(clientProtocolVersion == TLS1_3_VERSION);
  CHECK(serverProtocolVersion.load(std::memory_order_relaxed) == TLS1_3_VERSION);
}

TEST_CASE(
    "Private process transport uses authenticated TLS 1.3",
    "[internal][transport][tls][no-normative-mapping]") {
  using namespace umbra::detail;
  using namespace umbra::detail::platform;

  initializeSocketRuntime();
  TemporaryDirectory temporaryDirectory;
  auto const certificatePath = temporaryDirectory.path() / "localhost.crt";
  auto const privateKeyPath = temporaryDirectory.path() / "localhost.key";
  writeTestCertificate(certificatePath, privateKeyPath);

  auto const listener = ProcessTransportListener::listen(
      ProcessTransportAddress{
          "127.0.0.1",
          0U,
          ProcessTransportAddress::Security::tls13,
          {}},
      TlsServerConfiguration{certificatePath, privateKeyPath});
  auto clientAddress = listener->address();
  clientAddress.tlsTrustStoreFile = certificatePath;

  std::shared_ptr<ProcessTransportConnection> serverConnection;
  std::exception_ptr serverAcceptFailure;
  std::thread acceptor([&] {
    try {
      serverConnection = listener->accept(
          nullptr,
          TransportEndpointIdentity{"tls-test-authority", 0U},
          {},
          {});
    } catch (...) {
      serverAcceptFailure = std::current_exception();
    }
  });

  std::shared_ptr<ProcessTransportConnection> clientConnection;
  std::exception_ptr clientConnectFailure;
  try {
    clientConnection = ProcessTransportConnection::connectClient(
        nullptr,
        std::move(clientAddress),
        TransportEndpointIdentity{"tls-test-federate", 0U},
        {},
        {},
        {},
        {},
        {},
        std::chrono::steady_clock::now() + std::chrono::seconds(10));
  } catch (...) {
    clientConnectFailure = std::current_exception();
    listener->close();
  }
  acceptor.join();

  if (serverAcceptFailure) {
    std::rethrow_exception(serverAcceptFailure);
  }
  if (clientConnectFailure) {
    std::rethrow_exception(clientConnectFailure);
  }
  REQUIRE(clientConnection != nullptr);
  REQUIRE(serverConnection != nullptr);

  auto const clientFrame = makeTransportHeartbeat(
      clientConnection->sessionId(),
      7U,
      kInvalidTransportSequenceNumber);
  auto const serverFrame = makeTransportHeartbeatResponse(
      serverConnection->sessionId(),
      9U,
      kInvalidTransportSequenceNumber,
      7U);

  TransportFrame frameReceivedByClient;
  TransportFrame frameReceivedByServer;
  bool clientSendSucceeded = false;
  bool serverSendSucceeded = false;
  bool clientReceiveSucceeded = false;
  bool serverReceiveSucceeded = false;
  std::exception_ptr clientIoFailure;
  std::exception_ptr serverReadFailure;
  std::exception_ptr serverWriteFailure;
  std::thread serverReader([&] {
    try {
      serverReceiveSucceeded = serverConnection->receiveFrame(frameReceivedByServer);
    } catch (...) {
      serverReadFailure = std::current_exception();
    }
  });
  std::thread serverWriter([&] {
    try {
      serverSendSucceeded = serverConnection->sendFrame(serverFrame);
    } catch (...) {
      serverWriteFailure = std::current_exception();
    }
  });
  std::thread clientReader([&] {
    try {
      clientReceiveSucceeded = clientConnection->receiveFrame(frameReceivedByClient);
    } catch (...) {
      clientIoFailure = std::current_exception();
    }
  });
  clientSendSucceeded = clientConnection->sendFrame(clientFrame);

  serverReader.join();
  serverWriter.join();
  clientReader.join();
  if (serverReadFailure) {
    std::rethrow_exception(serverReadFailure);
  }
  if (serverWriteFailure) {
    std::rethrow_exception(serverWriteFailure);
  }
  if (clientIoFailure) {
    std::rethrow_exception(clientIoFailure);
  }

  CHECK(clientSendSucceeded);
  CHECK(serverSendSucceeded);
  CHECK(clientReceiveSucceeded);
  CHECK(serverReceiveSucceeded);
  CHECK(frameReceivedByClient.kind == TransportFrameKind::heartbeat_response);
  CHECK(frameReceivedByClient.sessionId == serverConnection->sessionId());
  CHECK(frameReceivedByServer.kind == TransportFrameKind::heartbeat);
  CHECK(frameReceivedByServer.sessionId == clientConnection->sessionId());
}
