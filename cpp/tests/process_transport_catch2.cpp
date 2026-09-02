#include <catch2/catch_test_macros.hpp>

#include "internal/federation/process_transport.hpp"

#include <cstdint>
#include <exception>
#include <stdexcept>
#include <thread>
#include <utility>
#include <vector>

namespace {

using umbra::detail::ProcessTransportAddress;
using umbra::detail::ProcessTransportConnection;
using umbra::detail::ProcessTransportListener;
using umbra::detail::TransportEndpointIdentity;
using umbra::detail::TransportFrame;
using umbra::detail::TransportFrameKind;

}  // namespace

TEST_CASE(
    "Private process transport exchanges framed data after endpoint handshake",
    "[unit][foundation][transport][process-boundary][transport-contract]") {
  auto listener = ProcessTransportListener::listen({"127.0.0.1", 0U});
  REQUIRE(listener != nullptr);
  REQUIRE(listener->address().port != 0U);

  TransportEndpointIdentity const serverIdentity{"server-process", 0x2002U};
  TransportEndpointIdentity const clientIdentity{"client-process", 0x1001U};
  std::exception_ptr serverFailure;
  TransportFrame serverReceived;

  std::thread server([&] {
    try {
      auto connection = listener->accept(
          nullptr,
          serverIdentity,
          [](std::wstring) {},
          [](std::wstring) { return false; });
      TransportFrame request;
      if (!connection->receiveFrame(request)) {
        throw std::runtime_error("server could not receive the request frame");
      }
      serverReceived = request;
      if (!connection->sendFrame({
              TransportFrameKind::data,
              std::vector<std::uint8_t>{0x7aU, 0x7bU, 0x7cU}})) {
        throw std::runtime_error("server could not send the response frame");
      }
      connection->close();
    } catch (...) {
      serverFailure = std::current_exception();
    }
  });

  std::exception_ptr clientFailure;
  TransportFrame clientReceived;
  try {
    auto client = ProcessTransportConnection::connectClient(
        nullptr,
        {"127.0.0.1", listener->address().port},
        clientIdentity,
        [](std::wstring) {},
        [](std::wstring) { return false; });
    REQUIRE(client->open());
    REQUIRE(client->peerIdentity().endpointId == serverIdentity.endpointId);
    REQUIRE(client->peerIdentity().sessionId == serverIdentity.sessionId);
    REQUIRE(client->sendFrame({
        TransportFrameKind::data,
        std::vector<std::uint8_t>{0x01U, 0x02U, 0x03U}}));
    REQUIRE(client->receiveFrame(clientReceived));
    client->close();
  } catch (...) {
    clientFailure = std::current_exception();
  }

  listener.reset();
  if (server.joinable()) {
    server.join();
  }

  if (clientFailure) {
    std::rethrow_exception(clientFailure);
  }
  REQUIRE_FALSE(serverFailure);
  REQUIRE(serverReceived.kind == TransportFrameKind::data);
  REQUIRE(serverReceived.payload == std::vector<std::uint8_t>{0x01U, 0x02U, 0x03U});
  REQUIRE(clientReceived.kind == TransportFrameKind::data);
  REQUIRE(clientReceived.payload == std::vector<std::uint8_t>{0x7aU, 0x7bU, 0x7cU});
}
