#include <catch2/catch_test_macros.hpp>

#include "internal/federation/process_transport_session.hpp"

#include <cstdint>
#include <exception>
#include <stdexcept>
#include <thread>
#include <vector>

namespace {

using umbra::detail::ProcessTransportConnection;
using umbra::detail::ProcessTransportListener;
using umbra::detail::ProcessTransportServiceDispatcher;
using umbra::detail::ProcessTransportSession;
using umbra::detail::TransportEndpointIdentity;
using umbra::detail::TransportServiceMessage;
using umbra::detail::TransportServiceMessageKind;
using umbra::detail::TransportServiceOperation;
using umbra::detail::TransportServiceStatus;

}  // namespace

TEST_CASE(
    "Private process service dispatch correlates federation operations over framed data",
    "[unit][foundation][transport][process-boundary][service-dispatch][transport-contract]") {
  auto listener = ProcessTransportListener::listen({"127.0.0.1", 0U});
  REQUIRE(listener != nullptr);

  std::vector<TransportServiceOperation> serverOperations;
  std::vector<std::vector<std::uint8_t>> serverPayloads;
  std::exception_ptr serverFailure;
  std::thread server([&] {
    try {
      auto connection = listener->accept(
          nullptr,
          {"service-server", 0x22U},
          [](std::wstring) {},
          [](std::wstring) { return false; });
      ProcessTransportSession session(std::move(connection));
      ProcessTransportServiceDispatcher::Handler handler =
          [&](TransportServiceMessage const& request) {
            serverOperations.push_back(request.operation);
            serverPayloads.push_back(request.payload);
            auto responsePayload = request.payload;
            responsePayload.push_back(0xeeU);
            return TransportServiceMessage{
                TransportServiceMessageKind::response,
                request.operation,
                TransportServiceStatus::ok,
                request.requestId,
                std::move(responsePayload)};
          };
      for (int index = 0; index < 3; ++index) {
        if (!ProcessTransportServiceDispatcher::serveOne(session, handler)) {
          throw std::runtime_error("service dispatcher lost its connection");
        }
      }
      session.connection()->close();
    } catch (...) {
      serverFailure = std::current_exception();
    }
  });

  std::exception_ptr clientFailure;
  try {
    auto connection = ProcessTransportConnection::connectClient(
        nullptr,
        {"127.0.0.1", listener->address().port},
        {"service-client", 0x11U},
        [](std::wstring) {},
        [](std::wstring) { return false; });
    ProcessTransportSession session(std::move(connection));
    std::vector<TransportServiceMessage> requests{
        {TransportServiceMessageKind::request,
         TransportServiceOperation::create_federation_execution,
         TransportServiceStatus::ok,
         1U,
         std::vector<std::uint8_t>{0x43U, 0x52U, 0x45U, 0x41U, 0x54U, 0x45U}},
        {TransportServiceMessageKind::request,
         TransportServiceOperation::join_federation_execution,
         TransportServiceStatus::ok,
         2U,
         std::vector<std::uint8_t>{0x4aU, 0x4fU, 0x49U, 0x4eU}},
        {TransportServiceMessageKind::request,
         TransportServiceOperation::send_interaction,
         TransportServiceStatus::ok,
         3U,
         std::vector<std::uint8_t>{0x49U, 0x4eU, 0x54U, 0x45U, 0x52U, 0x41U, 0x43U, 0x54U, 0x49U, 0x4fU, 0x4eU}}};
    for (auto const& request : requests) {
      TransportServiceMessage response;
      REQUIRE(session.request(request, response));
      REQUIRE(response.kind == TransportServiceMessageKind::response);
      REQUIRE(response.operation == request.operation);
      REQUIRE(response.status == TransportServiceStatus::ok);
      REQUIRE(response.requestId == request.requestId);
      auto expectedPayload = request.payload;
      expectedPayload.push_back(0xeeU);
      REQUIRE(response.payload == expectedPayload);
    }
    session.connection()->close();
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
  REQUIRE(serverOperations == std::vector<TransportServiceOperation>{
      TransportServiceOperation::create_federation_execution,
      TransportServiceOperation::join_federation_execution,
      TransportServiceOperation::send_interaction});
  REQUIRE(serverPayloads.size() == 3U);
}
