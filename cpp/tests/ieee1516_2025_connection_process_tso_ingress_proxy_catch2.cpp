#include <catch2/catch_test_macros.hpp>

#include <atomic>
#include <chrono>
#include <cstddef>
#include <condition_variable>
#include <cstdint>
#include <cstdlib>
#include <filesystem>
#include <fstream>
#include <future>
#include <iterator>
#include <memory>
#include <mutex>
#include <stdexcept>
#include <string>
#include <system_error>
#include <thread>
#include <utility>
#include <vector>

#include <RTI/NullFederateAmbassador.h>
#include <RTI/RTI1516.h>
#include <RTI/time/HLAinteger64Interval.h>
#include <RTI/time/HLAinteger64Time.h>

#if defined(UMBRA_ENABLE_EMBEDDED_FEDERATION_MANAGEMENT)

#include "internal/federation/federation_registry.hpp"
#include "internal/federation/process_federation_service_host.hpp"
#include "internal/federation/process_transport_proxy_host.hpp"
#include "internal/fom/libxml2_fom_composer.hpp"
#include "internal/fom/fom_validation.hpp"
#include "internal/fom/libxml2_fom_validator.hpp"

namespace {

using rti1516_2025::HLA_IMMEDIATE;
using rti1516_2025::NullFederateAmbassador;
using rti1516_2025::ParameterHandleValueMap;
using rti1516_2025::RTIambassador;
using rti1516_2025::RTIambassadorFactory;
using rti1516_2025::RtiConfiguration;
using rti1516_2025::VariableLengthData;
using umbra::detail::EmbeddedFederationRegistry;
using umbra::detail::FederationDefinition;
using umbra::detail::FomCompositionStatus;
using umbra::detail::FomModuleKind;
using umbra::detail::LibXml2FomModuleComposer;
using umbra::detail::LibXml2FomValidator;
using umbra::detail::PrevalidatedFomModule;
using umbra::detail::ProcessFederationServiceHost;
using umbra::detail::ProcessTransportAddress;
using umbra::detail::ProcessTransportProxyHost;

std::filesystem::path resourcePath(std::filesystem::path const& relative) {
  return std::filesystem::path(UMBRA_SOURCE_DIRECTORY) / "third_party" /
      "ieee1516.2-2025" / "resources" / relative;
}

PrevalidatedFomModule validateModule(
    std::filesystem::path const& source,
    FomModuleKind kind,
    std::wstring designator) {
  LibXml2FomValidator validator;
  auto result = validator.validate({
      source,
      resourcePath("schemas/IEEE1516-DIF-2025.xsd"),
      kind,
      std::move(designator),
      L"IEEE1516-DIF-2025.xsd",
  });
  if (result.status != umbra::detail::FomValidationStatus::valid ||
      !result.module) {
    throw std::runtime_error("The ingress-proxy FOM module did not validate.");
  }
  return *result.module;
}

FederationDefinition composedDefinition() {
  std::vector<PrevalidatedFomModule> modules{
      validateModule(
          resourcePath("mim/HLAstandardMIM-2025.xml"),
          FomModuleKind::mim,
          L"urn:umbra:test:proxy-interaction-mim"),
      validateModule(
          resourcePath("examples/RestaurantFOMmodule-2025.xml"),
          FomModuleKind::fom,
          L"urn:umbra:test:proxy-interaction-restaurant"),
  };
  LibXml2FomModuleComposer composer(
      resourcePath("schemas/IEEE1516-FDD-2025.xsd"));
  auto result = composer.compose(modules);
  if (result.status != FomCompositionStatus::valid || !result.catalog ||
      !result.fdd) {
    throw std::runtime_error("The ingress-proxy FOM did not compose.");
  }
  return {
      std::move(result.modules),
      L"HLAinteger64Time",
      std::move(result.catalog),
      std::move(result.fdd),
  };
}

std::unique_ptr<RTIambassador> makeRti() {
  RTIambassadorFactory factory;
  return factory.createRTIambassador();
}

RtiConfiguration configurationFor(
    ProcessTransportAddress const& address,
    wchar_t const* name) {
  return RtiConfiguration::createConfiguration()
      .withConfigurationName(name)
      .withRtiAddress(
          L"tcp://127.0.0.1:" + std::to_wstring(address.port));
}

class IngressFederateAmbassador final : public NullFederateAmbassador {
 public:
  void receiveInteraction(
      rti1516_2025::InteractionClassHandle const&,
      ParameterHandleValueMap const&,
      VariableLengthData const&,
      rti1516_2025::TransportationTypeHandle const&,
      rti1516_2025::FederateHandle const& producingFederate,
      rti1516_2025::RegionHandleSet const*,
      rti1516_2025::LogicalTime const& time,
      rti1516_2025::OrderType,
      rti1516_2025::OrderType,
      rti1516_2025::MessageRetractionHandle const*) override {
    std::scoped_lock lock(mutex_);
    ++interactionCount_;
    callbackOrder_.push_back('I');
    producingFederate_ = producingFederate;
    if (auto const* integerTime =
            dynamic_cast<rti1516_2025::HLAinteger64Time const*>(&time)) {
      interactionTime_ = integerTime->getTime();
    }
    changed_.notify_all();
  }

  void timeAdvanceGrant(rti1516_2025::LogicalTime const& time) override {
    std::scoped_lock lock(mutex_);
    ++grantCount_;
    callbackOrder_.push_back('G');
    if (auto const* integerTime =
            dynamic_cast<rti1516_2025::HLAinteger64Time const*>(&time)) {
      grantTime_ = integerTime->getTime();
    }
    changed_.notify_all();
  }

  [[nodiscard]] bool waitForGrantCount(std::size_t expected) {
    std::unique_lock lock(mutex_);
    return changed_.wait_for(lock, std::chrono::seconds(3), [this, expected] {
      return grantCount_ >= expected;
    });
  }

  [[nodiscard]] bool waitForInteractionAndGrant() {
    std::unique_lock lock(mutex_);
    return changed_.wait_for(lock, std::chrono::seconds(3), [this] {
      return interactionCount_ == 1U && grantCount_ == 1U;
    });
  }

  [[nodiscard]] std::size_t interactionCount() const {
    std::scoped_lock lock(mutex_);
    return interactionCount_;
  }

  [[nodiscard]] std::size_t grantCount() const {
    std::scoped_lock lock(mutex_);
    return grantCount_;
  }

  [[nodiscard]] std::int64_t interactionTime() const {
    std::scoped_lock lock(mutex_);
    return interactionTime_;
  }

  [[nodiscard]] std::int64_t grantTime() const {
    std::scoped_lock lock(mutex_);
    return grantTime_;
  }

  [[nodiscard]] rti1516_2025::FederateHandle producingFederate() const {
    std::scoped_lock lock(mutex_);
    return producingFederate_;
  }

  [[nodiscard]] std::vector<char> callbackOrder() const {
    std::scoped_lock lock(mutex_);
    return callbackOrder_;
  }

 private:
  mutable std::mutex mutex_;
  std::condition_variable changed_;
  std::size_t interactionCount_ = 0U;
  std::size_t grantCount_ = 0U;
  std::int64_t interactionTime_ = -1;
  std::int64_t grantTime_ = -1;
  rti1516_2025::FederateHandle producingFederate_;
  std::vector<char> callbackOrder_;
};

void verifyPublicArrivalScenario(
    ProcessTransportAddress const& senderAddress,
    ProcessTransportAddress const& receiverAddress,
    wchar_t const* scenarioName,
    bool requestAvailable) {
  std::wstring const scenario(scenarioName);
  std::wstring const federationName =
      L"process-tso-ingress-proxy-" + scenario;
  std::wstring const federateType = federationName + L"-type";
  std::wstring const senderOneName = federationName + L"-sender-one";
  std::wstring const senderTwoName = federationName + L"-sender-two";
  std::wstring const receiverName = federationName + L"-receiver";
  constexpr wchar_t const* interactionName =
      L"HLAinteractionRoot.CustomerTransactions.FoodServed.MainCourseServed";
  constexpr wchar_t const* parameterName = L"TimelinessOk";

  IngressFederateAmbassador senderOneFederate;
  IngressFederateAmbassador senderTwoFederate;
  IngressFederateAmbassador receiverFederate;
  auto senderOne = makeRti();
  auto senderTwo = makeRti();
  auto receiver = makeRti();
  REQUIRE(senderAddress.port != receiverAddress.port);
  REQUIRE(senderOne->connect(
              senderOneFederate,
              HLA_IMMEDIATE,
              configurationFor(senderAddress, L"process-tso-proxy-sender-one"))
              .addressUsed);
  REQUIRE_NOTHROW(senderOne->createFederationExecution(
      federationName, L"server-owned-proxy-fom.xml"));
  auto const senderOneHandle = senderOne->joinFederationExecution(
      senderOneName, federateType, federationName);
  REQUIRE(senderOneHandle.isValid());

  REQUIRE(senderTwo->connect(
              senderTwoFederate,
              HLA_IMMEDIATE,
              configurationFor(senderAddress, L"process-tso-proxy-sender-two"))
              .addressUsed);
  auto const senderTwoHandle = senderTwo->joinFederationExecution(
      senderTwoName, federateType, federationName);
  REQUIRE(senderTwoHandle.isValid());

  REQUIRE(receiver->connect(
              receiverFederate,
              HLA_IMMEDIATE,
              configurationFor(receiverAddress, L"process-tso-proxy-receiver"))
              .addressUsed);
  auto const receiverHandle = receiver->joinFederationExecution(
      receiverName, federateType, federationName);
  REQUIRE(receiverHandle.isValid());

  auto const senderOneClass = senderOne->getInteractionClassHandle(interactionName);
  auto const senderTwoClass = senderTwo->getInteractionClassHandle(interactionName);
  auto const receiverClass = receiver->getInteractionClassHandle(interactionName);
  auto const senderOneParameter = senderOne->getParameterHandle(
      senderOneClass, parameterName);
  auto const senderTwoParameter = senderTwo->getParameterHandle(
      senderTwoClass, parameterName);
  REQUIRE(senderOneClass == senderTwoClass);
  REQUIRE(senderOneClass == receiverClass);
  REQUIRE(senderOneParameter == senderTwoParameter);
  REQUIRE_NOTHROW(senderOne->publishInteractionClass(senderOneClass));
  REQUIRE_NOTHROW(senderTwo->publishInteractionClass(senderTwoClass));
  REQUIRE_NOTHROW(receiver->subscribeInteractionClass(receiverClass, true));
  REQUIRE_NOTHROW(senderOne->enableTimeRegulation(
      rti1516_2025::HLAinteger64Interval(1)));
  REQUIRE_NOTHROW(senderTwo->enableTimeRegulation(
      rti1516_2025::HLAinteger64Interval(1)));
  REQUIRE_NOTHROW(receiver->enableTimeConstrained());

  if (requestAvailable) {
    REQUIRE_NOTHROW(receiver->nextMessageRequestAvailable(
        rti1516_2025::HLAinteger64Time(10)));
  } else {
    REQUIRE_NOTHROW(receiver->nextMessageRequest(
        rti1516_2025::HLAinteger64Time(10)));
  }
  REQUIRE(receiverFederate.interactionCount() == 0U);
  REQUIRE(receiverFederate.grantCount() == 0U);

  std::uint8_t const payloadOne = 0x41U;
  std::uint8_t const payloadTwo = 0x52U;
  ParameterHandleValueMap senderOneValues;
  senderOneValues.emplace(
      senderOneParameter,
      VariableLengthData(&payloadOne, sizeof(payloadOne)));
  ParameterHandleValueMap senderTwoValues;
  senderTwoValues.emplace(
      senderTwoParameter,
      VariableLengthData(&payloadTwo, sizeof(payloadTwo)));
  std::uint8_t const tagByte = 0x50U;
  auto const laterRetraction = senderOne->sendInteraction(
      senderOneClass,
      senderOneValues,
      VariableLengthData(&tagByte, sizeof(tagByte)),
      rti1516_2025::HLAinteger64Time(8));
  REQUIRE(laterRetraction.isValid());
  REQUIRE_NOTHROW(senderOne->timeAdvanceRequest(
      rti1516_2025::HLAinteger64Time(8)));
  REQUIRE(senderOneFederate.waitForGrantCount(1U));
  REQUIRE(receiverFederate.interactionCount() == 0U);
  REQUIRE(receiverFederate.grantCount() == 0U);

  auto const earlierRetraction = senderTwo->sendInteraction(
      senderTwoClass,
      senderTwoValues,
      VariableLengthData(&tagByte, sizeof(tagByte)),
      rti1516_2025::HLAinteger64Time(5));
  REQUIRE(earlierRetraction.isValid());
  REQUIRE_NOTHROW(senderTwo->timeAdvanceRequest(
      rti1516_2025::HLAinteger64Time(5)));
  REQUIRE(senderTwoFederate.waitForGrantCount(1U));

  REQUIRE(receiverFederate.waitForInteractionAndGrant());
  REQUIRE(receiverFederate.callbackOrder() == std::vector<char>{'I', 'G'});
  REQUIRE(receiverFederate.interactionTime() == 5);
  REQUIRE(receiverFederate.grantTime() == 5);
  REQUIRE(receiverFederate.producingFederate() == senderTwoHandle);

  REQUIRE_NOTHROW(senderOne->resignFederationExecution(rti1516_2025::NO_ACTION));
  REQUIRE_NOTHROW(senderTwo->resignFederationExecution(rti1516_2025::NO_ACTION));
  REQUIRE_NOTHROW(receiver->resignFederationExecution(rti1516_2025::NO_ACTION));
  REQUIRE_NOTHROW(senderOne->disconnect());
  REQUIRE_NOTHROW(senderTwo->disconnect());
  REQUIRE_NOTHROW(receiver->disconnect());
  REQUIRE(senderOneHandle.isValid());
  REQUIRE(receiverHandle.isValid());
}

#if defined(UMBRA_PROCESS_TRANSPORT_PROXY_PROBE_PATH)

class TemporaryDirectory final {
 public:
  TemporaryDirectory() {
    static std::atomic_uint64_t sequence{0U};
    auto const base = std::filesystem::temp_directory_path();
    for (unsigned int attempt = 0U; attempt < 32U; ++attempt) {
      auto const token = std::to_string(
          std::chrono::steady_clock::now().time_since_epoch().count()) + "-" +
          std::to_string(sequence.fetch_add(1U, std::memory_order_relaxed));
      path_ = base / ("umbra-process-tso-proxy-" + token);
      std::error_code error;
      if (std::filesystem::create_directory(path_, error)) {
        return;
      }
      if (error && error != std::errc::file_exists) {
        throw std::filesystem::filesystem_error(
            "Could not create process proxy test directory", path_, error);
      }
    }
    throw std::runtime_error("Could not allocate a unique process proxy directory.");
  }

  TemporaryDirectory(TemporaryDirectory const&) = delete;
  TemporaryDirectory& operator=(TemporaryDirectory const&) = delete;

  ~TemporaryDirectory() {
    std::error_code ignored;
    std::filesystem::remove_all(path_, ignored);
  }

  [[nodiscard]] std::filesystem::path const& path() const noexcept {
    return path_;
  }

 private:
  std::filesystem::path path_;
};

void writeMarker(
    std::filesystem::path const& path,
    std::string const& content) {
  std::ofstream output(path, std::ios::binary | std::ios::trunc);
  if (!output || !(output << content)) {
    throw std::runtime_error("Could not write process proxy test marker.");
  }
}

std::string readMarker(std::filesystem::path const& path) {
  std::ifstream input(path, std::ios::binary);
  if (!input) {
    return {};
  }
  return std::string(
      std::istreambuf_iterator<char>(input), std::istreambuf_iterator<char>());
}

class ProxyProcess final {
 public:
  ProxyProcess(
      std::filesystem::path executable,
      std::filesystem::path directory,
      std::string role)
      : directory_(std::move(directory)), role_(std::move(role)) {
    auto command = "\"" + executable.string() + "\" \"" +
        directory_.string() + "\" " + role_;
#if defined(_WIN32)
    command = std::string{"\""} + command + "\"";
#endif
    process_ = std::async(std::launch::async, [command] {
      return std::system(command.c_str());
    });
    try {
      port_ = waitForPort();
    } catch (...) {
      requestStopNoThrow();
      waitForExitNoThrow();
      throw;
    }
  }

  ProxyProcess(ProxyProcess const&) = delete;
  ProxyProcess& operator=(ProxyProcess const&) = delete;

  ~ProxyProcess() {
    requestStopNoThrow();
    waitForExitNoThrow();
  }

  [[nodiscard]] ProcessTransportAddress address() const noexcept {
    return {"127.0.0.1", port_};
  }

  void stop() {
    requestStop();
    if (process_.wait_for(std::chrono::seconds(5)) != std::future_status::ready) {
      throw std::runtime_error("A process proxy did not stop within five seconds.");
    }
    auto const exitCode = process_.get();
    reaped_ = true;
    if (exitCode != 0) {
      throw std::runtime_error(
          "A process proxy exited unsuccessfully: " +
          readMarker(directory_ / (role_ + ".error")));
    }
    if (!std::filesystem::exists(directory_ / (role_ + ".ok"))) {
      throw std::runtime_error("A process proxy exited without its stop marker.");
    }
  }

 private:
  [[nodiscard]] unsigned short waitForPort() {
    auto const portPath = directory_ / (role_ + ".port");
    auto const errorPath = directory_ / (role_ + ".error");
    auto const deadline = std::chrono::steady_clock::now() +
        std::chrono::seconds(10);
    while (std::chrono::steady_clock::now() < deadline) {
      if (std::filesystem::exists(errorPath)) {
        throw std::runtime_error(
            "A process proxy failed to start: " + readMarker(errorPath));
      }
      if (std::filesystem::exists(portPath)) {
        std::ifstream input(portPath, std::ios::binary);
        unsigned int port = 0U;
        if (input >> port && port > 0U && port <= 65535U) {
          return static_cast<unsigned short>(port);
        }
        throw std::runtime_error("A process proxy published an invalid port.");
      }
      if (process_.wait_for(std::chrono::milliseconds(0)) ==
          std::future_status::ready) {
        auto const exitCode = process_.get();
        reaped_ = true;
        throw std::runtime_error(
            "A process proxy exited before readiness with code " +
            std::to_string(exitCode) + ": " + readMarker(errorPath));
      }
      std::this_thread::sleep_for(std::chrono::milliseconds(10));
    }
    throw std::runtime_error("A process proxy did not publish readiness in time.");
  }

  void requestStop() {
    if (!stopRequested_) {
      writeMarker(directory_ / (role_ + ".stop"), "stop");
      stopRequested_ = true;
    }
  }

  void requestStopNoThrow() noexcept {
    try {
      requestStop();
    } catch (...) {
    }
  }

  void waitForExitNoThrow() noexcept {
    if (!process_.valid() || reaped_) {
      return;
    }
    if (process_.wait_for(std::chrono::seconds(21)) ==
        std::future_status::ready) {
      try {
        static_cast<void>(process_.get());
      } catch (...) {
      }
      reaped_ = true;
    }
  }

  std::filesystem::path directory_;
  std::string role_;
  std::future<int> process_;
  unsigned short port_ = 0U;
  bool stopRequested_ = false;
  bool reaped_ = false;
};

#endif

}  // namespace

TEST_CASE(
    "RTIambassadors coordinate pending process NMR and NMRA through distinct registry-free ingress endpoints",
    "[integration][foundation][time-management][time-advance][transport]"
    "[process-boundary][public-endpoint][process-tso-cross-endpoint-arrival-coordination]"
    "[rti.service.send-interaction][rti.service.next-message-request]"
    "[rti.service.next-message-request-available][federate.callback.receive-interaction]"
    "[federate.callback.time-advance-grant][2025]") {
  EmbeddedFederationRegistry registry;
  ProcessFederationServiceHost authority(
      registry,
      composedDefinition(),
      {"127.0.0.1", 0U},
      {"process-tso-ingress-proxy-authority", 0xB820U});
  authority.start();

  ProcessTransportProxyHost senderIngress(
      {"127.0.0.1", 0U},
      authority.address(),
      {"process-tso-ingress-proxy-sender-ingress", 0xB821U});
  ProcessTransportProxyHost receiverIngress(
      {"127.0.0.1", 0U},
      authority.address(),
      {"process-tso-ingress-proxy-receiver-ingress", 0xB822U});
  senderIngress.start();
  receiverIngress.start();

  SECTION("NMR through separate ingress endpoints") {
    verifyPublicArrivalScenario(
        senderIngress.address(),
        receiverIngress.address(),
        L"in-process-nmr",
        false);
  }
  SECTION("NMRA through separate ingress endpoints") {
    verifyPublicArrivalScenario(
        senderIngress.address(),
        receiverIngress.address(),
        L"in-process-nmra",
        true);
  }

  senderIngress.stop();
  receiverIngress.stop();
  authority.stop();
  REQUIRE_NOTHROW(senderIngress.rethrowIfFailed());
  REQUIRE_NOTHROW(receiverIngress.rethrowIfFailed());
  REQUIRE_NOTHROW(authority.rethrowIfFailed());
}

#if defined(UMBRA_PROCESS_TRANSPORT_PROXY_PROBE_PATH)

TEST_CASE(
    "RTIambassadors preserve pending process NMR and NMRA delivery through separate ingress proxy processes",
    "[integration][foundation][time-management][time-advance][transport]"
    "[process-boundary][public-endpoint][process-tso-cross-endpoint-arrival-coordination]"
    "[rti.service.send-interaction][rti.service.next-message-request]"
    "[rti.service.next-message-request-available][federate.callback.receive-interaction]"
    "[federate.callback.time-advance-grant][2025]") {
  EmbeddedFederationRegistry registry;
  ProcessFederationServiceHost authority(
      registry,
      composedDefinition(),
      {"127.0.0.1", 0U},
      {"process-tso-subprocess-authority", 0xB820U});
  authority.start();

  TemporaryDirectory directory;
  writeMarker(
      directory.path() / "authority.port",
      std::to_string(authority.address().port));
  std::filesystem::path const probe(UMBRA_PROCESS_TRANSPORT_PROXY_PROBE_PATH);
  ProxyProcess senderProxy(probe, directory.path(), "sender");
  ProxyProcess receiverProxy(probe, directory.path(), "receiver");
  REQUIRE(senderProxy.address().port != receiverProxy.address().port);

  SECTION("NMR through separate proxy processes") {
    verifyPublicArrivalScenario(
        senderProxy.address(),
        receiverProxy.address(),
        L"subprocess-nmr",
        false);
  }
  SECTION("NMRA through separate proxy processes") {
    verifyPublicArrivalScenario(
        senderProxy.address(),
        receiverProxy.address(),
        L"subprocess-nmra",
        true);
  }

  senderProxy.stop();
  receiverProxy.stop();
  authority.stop();
  REQUIRE_NOTHROW(authority.rethrowIfFailed());
}

#endif

#endif
