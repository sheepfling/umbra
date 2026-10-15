#include <catch2/catch_test_macros.hpp>

#include "internal/federation/federation_registry.hpp"
#include "internal/federation/process_federation_service_host.hpp"
#include "internal/platform/socket_operations.hpp"

#include <RTI/NullFederateAmbassador.h>
#include <RTI/RTI1516.h>

#include <chrono>
#include <cstdint>
#include <future>
#include <memory>
#include <string>
#include <thread>

namespace {

using rti1516_2025::HLA_EVOKED;
using rti1516_2025::NullFederateAmbassador;
using rti1516_2025::RTIambassador;
using rti1516_2025::RTIambassadorFactory;
using rti1516_2025::RtiConfiguration;
using umbra::detail::EmbeddedFederationRegistry;
using umbra::detail::FederationDefinition;
using umbra::detail::ProcessFederationServiceHost;
namespace platform = umbra::detail::platform;

std::unique_ptr<RTIambassador> makeRti() {
  RTIambassadorFactory factory;
  return factory.createRTIambassador();
}

RtiConfiguration configurationFor(std::uint16_t port, wchar_t const* name) {
  return RtiConfiguration::createConfiguration()
      .withConfigurationName(name)
      .withRtiAddress(L"tcp://127.0.0.1:" + std::to_wstring(port));
}

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
    "Process federation service host shutdown interrupts a stalled hello handshake",
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
