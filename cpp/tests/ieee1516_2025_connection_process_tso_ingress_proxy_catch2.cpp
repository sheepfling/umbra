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
#include <optional>
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
using rti1516_2025::HLA_EVOKED;
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

template <typename Operation>
auto withRtiExceptionDiagnostics(Operation&& operation) {
  try {
    return std::forward<Operation>(operation)();
  } catch (rti1516_2025::Exception const& error) {
    auto narrow = [](std::wstring const& value) {
      std::string result;
      result.reserve(value.size());
      for (wchar_t character : value) {
        result.push_back(character >= 0 && character <= 0x7f
                ? static_cast<char>(character)
                : '?');
      }
      return result;
    };
    throw std::runtime_error(
        narrow(error.name()) + ": " + narrow(error.what()));
  }
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
    bool requestAvailable,
    std::optional<ProcessTransportAddress> secondSenderAddress = std::nullopt,
    rti1516_2025::CallbackModel callbackModel = HLA_IMMEDIATE) {
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
  REQUIRE(withRtiExceptionDiagnostics([&] {
            return senderOne->connect(
                senderOneFederate,
                callbackModel,
                configurationFor(
                    senderAddress, L"process-tso-proxy-sender-one"));
          })
              .addressUsed);
  REQUIRE_NOTHROW(senderOne->createFederationExecution(
      federationName, L"server-owned-proxy-fom.xml"));
  auto const senderOneHandle = senderOne->joinFederationExecution(
      senderOneName, federateType, federationName);
  REQUIRE(senderOneHandle.isValid());

  REQUIRE(senderTwo->connect(
              senderTwoFederate,
              callbackModel,
              configurationFor(
                  secondSenderAddress.value_or(senderAddress),
                  L"process-tso-proxy-sender-two"))
              .addressUsed);
  auto const senderTwoHandle = senderTwo->joinFederationExecution(
      senderTwoName, federateType, federationName);
  REQUIRE(senderTwoHandle.isValid());

  REQUIRE(receiver->connect(
              receiverFederate,
              callbackModel,
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

  if (callbackModel == HLA_EVOKED) {
    auto drainInitialCallbacks = [](RTIambassador& ambassador) {
      for (unsigned int attempt = 0U; attempt < 8U; ++attempt) {
        if (!ambassador.evokeCallback(0.02)) {
          return;
        }
      }
      throw std::runtime_error(
          "The process TSO ingress setup did not drain its initial callbacks.");
    };
    drainInitialCallbacks(*senderOne);
    drainInitialCallbacks(*senderTwo);
    drainInitialCallbacks(*receiver);
  }

  if (requestAvailable) {
    REQUIRE_NOTHROW(receiver->nextMessageRequestAvailable(
        rti1516_2025::HLAinteger64Time(10)));
  } else {
    REQUIRE_NOTHROW(receiver->nextMessageRequest(
        rti1516_2025::HLAinteger64Time(10)));
  }
  if (callbackModel == HLA_EVOKED) {
    REQUIRE_FALSE(receiver->evokeCallback(0.0));
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
  if (callbackModel == HLA_EVOKED) {
    static_cast<void>(senderOne->evokeCallback(0.1));
    REQUIRE(senderOneFederate.grantCount() == 1U);
  } else {
    REQUIRE(senderOneFederate.waitForGrantCount(1U));
  }
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
  if (callbackModel == HLA_EVOKED) {
    static_cast<void>(senderTwo->evokeCallback(0.1));
    REQUIRE(senderTwoFederate.grantCount() == 1U);
  } else {
    REQUIRE(senderTwoFederate.waitForGrantCount(1U));
  }

  if (callbackModel == HLA_EVOKED) {
    REQUIRE(receiver->evokeCallback(0.1));
    REQUIRE(receiverFederate.callbackOrder() == std::vector<char>{'I'});
    REQUIRE_FALSE(receiver->evokeCallback(0.1));
  } else {
    REQUIRE(receiverFederate.waitForInteractionAndGrant());
  }
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

class IngressDeleteFederateAmbassador final : public NullFederateAmbassador {
 public:
  void discoverObjectInstance(
      rti1516_2025::ObjectInstanceHandle const& object,
      rti1516_2025::ObjectClassHandle const&,
      std::wstring const&,
      rti1516_2025::FederateHandle const& producer) override {
    std::scoped_lock lock(mutex_);
    discovered_ = true;
    object_ = object;
    producer_ = producer;
    changed_.notify_all();
  }

  void removeObjectInstance(
      rti1516_2025::ObjectInstanceHandle const& object,
      VariableLengthData const&,
      rti1516_2025::FederateHandle const& producer,
      rti1516_2025::LogicalTime const& time,
      rti1516_2025::OrderType,
      rti1516_2025::OrderType,
      rti1516_2025::MessageRetractionHandle const*) override {
    std::scoped_lock lock(mutex_);
    removed_ = true;
    object_ = object;
    producer_ = producer;
    if (auto const* integerTime =
            dynamic_cast<rti1516_2025::HLAinteger64Time const*>(&time)) {
      removalTime_ = integerTime->getTime();
    }
    callbackOrder_.push_back('R');
    changed_.notify_all();
  }

  void timeAdvanceGrant(rti1516_2025::LogicalTime const& time) override {
    std::scoped_lock lock(mutex_);
    ++grantCount_;
    if (auto const* integerTime =
            dynamic_cast<rti1516_2025::HLAinteger64Time const*>(&time)) {
      grantTime_ = integerTime->getTime();
    }
    callbackOrder_.push_back('G');
    changed_.notify_all();
  }

  [[nodiscard]] bool waitForDiscovery() {
    std::unique_lock lock(mutex_);
    return changed_.wait_for(lock, std::chrono::seconds(3), [this] {
      return discovered_;
    });
  }

  [[nodiscard]] bool waitForRemovalAndGrant() {
    std::unique_lock lock(mutex_);
    return changed_.wait_for(lock, std::chrono::seconds(3), [this] {
      return removed_ && grantCount_ == 1U;
    });
  }

  [[nodiscard]] bool hasRemovalOrGrant() const {
    std::scoped_lock lock(mutex_);
    return removed_ || grantCount_ != 0U;
  }

  [[nodiscard]] rti1516_2025::ObjectInstanceHandle object() const {
    std::scoped_lock lock(mutex_);
    return object_;
  }

  [[nodiscard]] rti1516_2025::FederateHandle producer() const {
    std::scoped_lock lock(mutex_);
    return producer_;
  }

  [[nodiscard]] std::int64_t removalTime() const {
    std::scoped_lock lock(mutex_);
    return removalTime_;
  }

  [[nodiscard]] std::int64_t grantTime() const {
    std::scoped_lock lock(mutex_);
    return grantTime_;
  }

  [[nodiscard]] std::vector<char> callbackOrder() const {
    std::scoped_lock lock(mutex_);
    return callbackOrder_;
  }

 private:
  mutable std::mutex mutex_;
  std::condition_variable changed_;
  bool discovered_ = false;
  bool removed_ = false;
  std::size_t grantCount_ = 0U;
  rti1516_2025::ObjectInstanceHandle object_;
  rti1516_2025::FederateHandle producer_;
  std::int64_t removalTime_ = -1;
  std::int64_t grantTime_ = -1;
  std::vector<char> callbackOrder_;
};

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

std::string waitForProcessMarker(
    std::filesystem::path const& directory,
    std::string const& markerName,
    std::string const& errorRole) {
  auto const deadline = std::chrono::steady_clock::now() +
      std::chrono::seconds(15);
  auto const markerPath = directory / markerName;
  auto const errorPath = directory / (errorRole + ".error");
  while (std::chrono::steady_clock::now() < deadline) {
    if (std::filesystem::exists(errorPath)) {
      throw std::runtime_error(
          errorRole + " child process failed: " + readMarker(errorPath));
    }
    if (std::filesystem::exists(markerPath)) {
      auto const marker = readMarker(markerPath);
      if (!marker.empty()) {
        return marker;
      }
    }
    std::this_thread::sleep_for(std::chrono::milliseconds(10));
  }
  throw std::runtime_error("Timed out waiting for child marker " + markerName);
}

class ProxyProcess final {
 public:
  ProxyProcess(
      std::filesystem::path executable,
      std::filesystem::path directory,
      std::string role,
      bool roleBeforeDirectory = false)
      : directory_(std::move(directory)), role_(std::move(role)) {
    auto command = "\"" + executable.string() + "\" ";
    if (roleBeforeDirectory) {
      command += role_ + " \"" + directory_.string() + "\"";
    } else {
      command += "\"" + directory_.string() + "\" " + role_;
    }
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
    throw std::runtime_error(
        "A process proxy did not publish a complete valid port in time.");
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

void verifyTimestampedDeleteArrivalThroughProxyProcesses(
    ProcessTransportAddress const& senderAddress,
    ProcessTransportAddress const& receiverAddress,
    wchar_t const* scenarioName,
    bool requestAvailable) {
  std::wstring const federationName =
      L"process-tso-proxy-delete-" + std::wstring(scenarioName);
  IngressDeleteFederateAmbassador senderFederate;
  IngressDeleteFederateAmbassador receiverFederate;
  auto sender = makeRti();
  auto receiver = makeRti();

  REQUIRE(withRtiExceptionDiagnostics([&] {
            return sender->connect(
                senderFederate,
                HLA_IMMEDIATE,
                configurationFor(senderAddress, L"proxy-delete-sender"));
          })
              .addressUsed);
  REQUIRE_NOTHROW(sender->createFederationExecution(
      federationName, L"server-owned-fom.xml"));
  auto const senderHandle = sender->joinFederationExecution(
      federationName + L"-sender", federationName + L"-type", federationName);
  REQUIRE(senderHandle.isValid());

  REQUIRE(withRtiExceptionDiagnostics([&] {
            return receiver->connect(
                receiverFederate,
                HLA_IMMEDIATE,
                configurationFor(receiverAddress, L"proxy-delete-receiver"));
          })
              .addressUsed);
  auto const receiverHandle = receiver->joinFederationExecution(
      federationName + L"-receiver", federationName + L"-type", federationName);
  REQUIRE(receiverHandle.isValid());

  constexpr wchar_t const* objectClassName = L"HLAobjectRoot.Employee";
  constexpr wchar_t const* attributeName = L"Name";
  auto const senderClass = sender->getObjectClassHandle(objectClassName);
  auto const receiverClass = receiver->getObjectClassHandle(objectClassName);
  auto const senderAttribute = sender->getAttributeHandle(senderClass, attributeName);
  auto const receiverAttribute =
      receiver->getAttributeHandle(receiverClass, attributeName);
  REQUIRE(senderClass == receiverClass);
  REQUIRE(senderAttribute == receiverAttribute);
  REQUIRE_NOTHROW(sender->publishObjectClassAttributes(
      senderClass, rti1516_2025::AttributeHandleSet{senderAttribute}));
  REQUIRE_NOTHROW(receiver->subscribeObjectClassAttributes(
      receiverClass, rti1516_2025::AttributeHandleSet{receiverAttribute}));

  auto const object = sender->registerObjectInstance(senderClass);
  REQUIRE(object.isValid());
  REQUIRE(receiverFederate.waitForDiscovery());
  REQUIRE(receiverFederate.object() == object);
  REQUIRE(receiverFederate.producer() == senderHandle);

  REQUIRE_NOTHROW(sender->enableTimeRegulation(
      rti1516_2025::HLAinteger64Interval(1)));
  REQUIRE_NOTHROW(receiver->enableTimeConstrained());
  if (requestAvailable) {
    REQUIRE_NOTHROW(receiver->nextMessageRequestAvailable(
        rti1516_2025::HLAinteger64Time(10)));
  } else {
    REQUIRE_NOTHROW(receiver->nextMessageRequest(
        rti1516_2025::HLAinteger64Time(10)));
  }
  REQUIRE_FALSE(receiverFederate.hasRemovalOrGrant());

  std::uint8_t const tagByte = 0x44U;
  auto const retraction = sender->deleteObjectInstance(
      object,
      VariableLengthData(&tagByte, sizeof(tagByte)),
      rti1516_2025::HLAinteger64Time(5));
  REQUIRE(retraction.isValid());
  REQUIRE_NOTHROW(sender->timeAdvanceRequest(rti1516_2025::HLAinteger64Time(6)));
  REQUIRE(receiverFederate.waitForRemovalAndGrant());
  REQUIRE(receiverFederate.callbackOrder() == std::vector<char>{'R', 'G'});
  REQUIRE(receiverFederate.object() == object);
  REQUIRE(receiverFederate.producer() == senderHandle);
  REQUIRE(receiverFederate.removalTime() == 5);
  REQUIRE(receiverFederate.grantTime() == 5);

  REQUIRE_NOTHROW(sender->resignFederationExecution(
      rti1516_2025::DELETE_OBJECTS));
  REQUIRE_NOTHROW(receiver->resignFederationExecution(rti1516_2025::NO_ACTION));
  REQUIRE_NOTHROW(sender->disconnect());
  REQUIRE_NOTHROW(receiver->disconnect());
  REQUIRE(receiverHandle.isValid());
}

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

#if defined(UMBRA_PROCESS_SERVICE_PROBE_PATH) && \
    defined(UMBRA_PROCESS_TRANSPORT_PROXY_PROBE_PATH)

TEST_CASE(
    "Independent RTI client processes deliver a future timestamped interaction before pending TARA grant through one authoritative coordinator",
    "[integration][foundation][interaction-management][time-management]"
    "[time-advance][transport][process-boundary][public-endpoint]"
    "[process-tso-authoritative-coordinator-tara-future-arrival]"
    "[rti.service.send-interaction][rti.service.time-advance-request-available]"
    "[federate.callback.receive-interaction][federate.callback.time-advance-grant]"
    "[requirement:hla-1516.1-2025:clause-8.1.4]"
    "[requirement:hla-1516.1-2025:clause-8.9]"
    "[requirement:hla-1516.1-2025:clause-8.14.4][2025]") {
  TemporaryDirectory directory;
  std::filesystem::path const serviceProbe(
      UMBRA_PROCESS_SERVICE_PROBE_PATH);
  ProxyProcess authority(
      serviceProbe, directory.path(), "tso-authority", true);
  ProxyProcess sender(
      serviceProbe, directory.path(), "tso-tara-sender", true);
  ProxyProcess receiver(
      serviceProbe, directory.path(), "tso-tara-receiver", true);

  auto const receiverEvidence = waitForProcessMarker(
      directory.path(),
      "tso-tara-receiver.completed",
      "tso-tara-receiver");
  auto const senderEvidence = waitForProcessMarker(
      directory.path(),
      "tso-tara-sender.completed",
      "tso-tara-sender");
  REQUIRE(receiverEvidence == "interaction-before-tara-grant-at-6");
  REQUIRE(senderEvidence == "sent-timestamp-5");

  receiver.stop();
  sender.stop();
  authority.stop();
}

TEST_CASE(
    "Independent RTI client processes deliver a future timestamped attribute update before pending TARA grant through one authoritative coordinator",
    "[integration][foundation][object-management][time-management]"
    "[time-advance][transport][process-boundary][public-endpoint]"
    "[process-tso-authoritative-coordinator-tara-attribute-update-future-arrival]"
    "[rti.service.update-attribute-values][rti.service.time-advance-request-available]"
    "[federate.callback.discover-object-instance]"
    "[federate.callback.reflect-attribute-values]"
    "[federate.callback.time-advance-grant]"
    "[requirement:hla-1516.1-2025:clause-6.10]"
    "[requirement:hla-1516.1-2025:clause-8.1.4]"
    "[requirement:hla-1516.1-2025:clause-8.9]"
    "[requirement:hla-1516.1-2025:clause-8.14.4][2025]") {
  TemporaryDirectory directory;
  std::filesystem::path const serviceProbe(
      UMBRA_PROCESS_SERVICE_PROBE_PATH);
  ProxyProcess authority(
      serviceProbe, directory.path(), "attribute-tso-tara-authority", true);
  ProxyProcess sender(
      serviceProbe, directory.path(), "attribute-tso-tara-sender", true);
  ProxyProcess receiver(
      serviceProbe, directory.path(), "attribute-tso-tara-receiver", true);

  auto const receiverEvidence = waitForProcessMarker(
      directory.path(),
      "attribute-tso-tara-receiver.completed",
      "attribute-tso-tara-receiver");
  auto const senderEvidence = waitForProcessMarker(
      directory.path(),
      "attribute-tso-tara-sender.completed",
      "attribute-tso-tara-sender");
  REQUIRE(receiverEvidence == "attribute-before-tara-grant-at-6");
  REQUIRE(senderEvidence == "sent-timestamp-5");

  receiver.stop();
  sender.stop();
  authority.stop();
}

TEST_CASE(
    "Independent RTI client processes deliver pre-target and equal-target TSO before safe TAR through one authoritative coordinator",
    "[integration][foundation][time-management][time-advance][transport]"
    "[process-boundary][public-endpoint][galt][tso]"
    "[tar-acknowledged-tso-galt-release]"
    "[rti.service.send-interaction][rti.service.time-advance-request]"
    "[federate.callback.receive-interaction][federate.callback.time-advance-grant]"
    "[requirement:hla-1516.1-2025:clause-8.1.6]"
    "[requirement:hla-1516.1-2025:clause-8.8][2025]") {
  TemporaryDirectory directory;
  std::filesystem::path const serviceProbe(
      UMBRA_PROCESS_SERVICE_PROBE_PATH);
  ProxyProcess authority(
      serviceProbe, directory.path(), "tso-authority", true);
  ProxyProcess sender(
      serviceProbe, directory.path(), "tso-galt-release-sender", true);
  static_cast<void>(waitForProcessMarker(
      directory.path(),
      "tso-galt-release-sender.port",
      "tso-galt-release-sender"));
  ProxyProcess receiver(
      serviceProbe, directory.path(), "tso-galt-release-receiver", true);

  auto const receiverEvidence = waitForProcessMarker(
      directory.path(),
      "tso-galt-release-receiver.completed",
      "tso-galt-release-receiver");
  auto const senderEvidence = waitForProcessMarker(
      directory.path(),
      "tso-galt-release-sender.completed",
      "tso-galt-release-sender");
  REQUIRE(receiverEvidence == "timestamps-3-and-4-before-tar-grant-4");
  REQUIRE(senderEvidence == "sent-timestamps-3-and-4");

  receiver.stop();
  sender.stop();
  authority.stop();
}

TEST_CASE(
    "Independent RTI client processes deliver a late TSO before NMRA grant through one authoritative coordinator",
    "[integration][foundation][interaction-management][time-management]"
    "[time-advance][transport][process-boundary][public-endpoint]"
    "[process-tso-authoritative-coordinator-ingress-os-process]"
    "[rti.service.send-interaction][rti.service.next-message-request-available]"
    "[federate.callback.receive-interaction][federate.callback.time-advance-grant]"
    "[requirement:hla-1516.1-2025:clause-8.11.3]"
    "[requirement:hla-1516.1-2025:clause-8.12.3][2025]") {
  TemporaryDirectory directory;
  std::filesystem::path const serviceProbe(
      UMBRA_PROCESS_SERVICE_PROBE_PATH);
  ProxyProcess authority(
      serviceProbe, directory.path(), "tso-authority", true);
  ProxyProcess sender(
      serviceProbe, directory.path(), "tso-sender", true);
  ProxyProcess receiver(
      serviceProbe, directory.path(), "tso-receiver", true);

  auto const receiverEvidence = waitForProcessMarker(
      directory.path(), "tso-receiver.completed", "tso-receiver");
  auto const senderEvidence = waitForProcessMarker(
      directory.path(), "tso-sender.completed", "tso-sender");
  REQUIRE(receiverEvidence == "interaction-before-grant-at-5");
  REQUIRE(senderEvidence == "sent-timestamp-5");

  receiver.stop();
  sender.stop();
  authority.stop();
}

TEST_CASE(
    "Independent RTI client processes deliver a late directed TSO before NMRA grant through one authoritative coordinator",
    "[integration][foundation][interaction-management][object-management]"
    "[time-management][time-advance][transport][process-boundary][public-endpoint]"
    "[process-tso-directed-authoritative-coordinator-os-process]"
    "[rti.service.publish-object-class-directed-interactions]"
    "[rti.service.subscribe-object-class-directed-interactions]"
    "[rti.service.send-directed-interaction][rti.service.next-message-request-available]"
    "[federate.callback.discover-object-instance]"
    "[federate.callback.receive-directed-interaction]"
    "[federate.callback.time-advance-grant]"
    "[requirement:hla-1516.1-2025:clause-6.14.3]"
    "[requirement:hla-1516.1-2025:clause-6.15.1]"
    "[requirement:hla-1516.1-2025:clause-8.11]"
    "[requirement:hla-1516.1-2025:clause-8.11.3][2025]") {
  TemporaryDirectory directory;
  std::filesystem::path const serviceProbe(
      UMBRA_PROCESS_SERVICE_PROBE_PATH);
  ProxyProcess authority(
      serviceProbe, directory.path(), "directed-tso-nmra-authority", true);
  ProxyProcess sender(
      serviceProbe, directory.path(), "directed-tso-nmra-sender", true);
  ProxyProcess receiver(
      serviceProbe, directory.path(), "directed-tso-nmra-receiver", true);

  auto const receiverEvidence = waitForProcessMarker(
      directory.path(),
      "directed-tso-nmra-receiver.completed",
      "directed-tso-nmra-receiver");
  auto const senderEvidence = waitForProcessMarker(
      directory.path(),
      "directed-tso-nmra-sender.completed",
      "directed-tso-nmra-sender");
  REQUIRE(receiverEvidence == "directed-interaction-before-grant-at-5");
  REQUIRE(senderEvidence == "sent-directed-timestamp-5");

  receiver.stop();
  sender.stop();
  authority.stop();
}

TEST_CASE(
    "Independent RTI client processes deliver a timestamped directed interaction before pending TARA grant through one authoritative coordinator",
    "[integration][foundation][interaction-management][object-management]"
    "[time-management][time-advance][transport][process-boundary][public-endpoint]"
    "[process-tso-directed-authoritative-coordinator-tara-future-arrival]"
    "[rti.service.publish-object-class-directed-interactions]"
    "[rti.service.subscribe-object-class-directed-interactions]"
    "[rti.service.send-directed-interaction]"
    "[rti.service.time-advance-request-available]"
    "[federate.callback.discover-object-instance]"
    "[federate.callback.receive-directed-interaction]"
    "[federate.callback.time-advance-grant]"
    "[requirement:hla-1516.1-2025:clause-6.14.3]"
    "[requirement:hla-1516.1-2025:clause-6.15.1]"
    "[requirement:hla-1516.1-2025:clause-8.1.4]"
    "[requirement:hla-1516.1-2025:clause-8.9]"
    "[requirement:hla-1516.1-2025:clause-8.14.4][2025]") {
  TemporaryDirectory directory;
  std::filesystem::path const serviceProbe(
      UMBRA_PROCESS_SERVICE_PROBE_PATH);
  ProxyProcess authority(
      serviceProbe, directory.path(), "directed-tso-tara-authority", true);
  ProxyProcess sender(
      serviceProbe, directory.path(), "directed-tso-tara-sender", true);
  ProxyProcess receiver(
      serviceProbe, directory.path(), "directed-tso-tara-receiver", true);

  auto const receiverEvidence = waitForProcessMarker(
      directory.path(),
      "directed-tso-tara-receiver.completed",
      "directed-tso-tara-receiver");
  auto const senderEvidence = waitForProcessMarker(
      directory.path(),
      "directed-tso-tara-sender.completed",
      "directed-tso-tara-sender");
  REQUIRE(receiverEvidence == "directed-interaction-before-tara-grant-at-6");
  REQUIRE(senderEvidence == "sent-directed-timestamp-5");

  receiver.stop();
  sender.stop();
  authority.stop();
}

TEST_CASE(
    "Independent RTI client processes deliver a late directed TSO before NMR grant through one authoritative coordinator",
    "[integration][foundation][interaction-management][object-management]"
    "[time-management][time-advance][transport][process-boundary][public-endpoint]"
    "[process-tso-directed-authoritative-coordinator-os-process]"
    "[rti.service.publish-object-class-directed-interactions]"
    "[rti.service.subscribe-object-class-directed-interactions]"
    "[rti.service.send-directed-interaction][rti.service.next-message-request]"
    "[federate.callback.discover-object-instance]"
    "[federate.callback.receive-directed-interaction]"
    "[federate.callback.time-advance-grant]"
    "[requirement:hla-1516.1-2025:clause-6.14.3]"
    "[requirement:hla-1516.1-2025:clause-6.15.1]"
    "[requirement:hla-1516.1-2025:clause-8.10.2][2025]") {
  TemporaryDirectory directory;
  std::filesystem::path const serviceProbe(
      UMBRA_PROCESS_SERVICE_PROBE_PATH);
  ProxyProcess authority(
      serviceProbe, directory.path(), "directed-tso-nmr-authority", true);
  ProxyProcess sender(
      serviceProbe, directory.path(), "directed-tso-nmr-sender", true);
  ProxyProcess receiver(
      serviceProbe, directory.path(), "directed-tso-nmr-receiver", true);

  auto const receiverEvidence = waitForProcessMarker(
      directory.path(),
      "directed-tso-nmr-receiver.completed",
      "directed-tso-nmr-receiver");
  auto const senderEvidence = waitForProcessMarker(
      directory.path(),
      "directed-tso-nmr-sender.completed",
      "directed-tso-nmr-sender");
  REQUIRE(receiverEvidence == "directed-interaction-before-grant-at-5");
  REQUIRE(senderEvidence == "sent-directed-timestamp-5");

  receiver.stop();
  sender.stop();
  authority.stop();
}

TEST_CASE(
    "Registry-free ingress processes forward federate time state to one authoritative coordinator",
    "[integration][foundation][time-management][time-advance][transport]"
    "[process-boundary][public-endpoint]"
    "[process-tso-cross-endpoint-arrival-coordination]"
    "[process-tso-authoritative-coordinator-ingress-os-process]"
    "[rti.service.send-interaction][rti.service.next-message-request]"
    "[rti.service.next-message-request-available]"
    "[federate.callback.receive-interaction]"
    "[federate.callback.time-advance-grant][2025]") {
  TemporaryDirectory directory;
  std::filesystem::path const serviceProbe(
      UMBRA_PROCESS_SERVICE_PROBE_PATH);
  ProxyProcess authority(
      serviceProbe, directory.path(), "tso-authority", true);
  writeMarker(
      directory.path() / "authority.port",
      std::to_string(authority.address().port));

  std::filesystem::path const proxyProbe(
      UMBRA_PROCESS_TRANSPORT_PROXY_PROBE_PATH);
  ProxyProcess firstSenderIngress(proxyProbe, directory.path(), "sender");
  ProxyProcess secondSenderIngress(proxyProbe, directory.path(), "sender-two");
  ProxyProcess receiverIngress(proxyProbe, directory.path(), "receiver");
  REQUIRE(firstSenderIngress.address().port != secondSenderIngress.address().port);
  REQUIRE(firstSenderIngress.address().port != receiverIngress.address().port);
  REQUIRE(secondSenderIngress.address().port != receiverIngress.address().port);

  SECTION("pending NMR uses sender membership and TSO progress at the authority") {
    verifyPublicArrivalScenario(
        firstSenderIngress.address(),
        receiverIngress.address(),
        L"authoritative-ingress-nmr",
        false,
        secondSenderIngress.address());
  }
  SECTION("pending NMRA uses sender membership and TSO progress at the authority") {
    verifyPublicArrivalScenario(
        firstSenderIngress.address(),
        receiverIngress.address(),
        L"authoritative-ingress-nmra",
        true,
        secondSenderIngress.address());
  }

  firstSenderIngress.stop();
  secondSenderIngress.stop();
  receiverIngress.stop();
  authority.stop();
}

TEST_CASE(
    "Independent RTI client processes deliver a late TSO attribute update before NMRA grant through one authoritative coordinator",
    "[integration][foundation][object-management][time-management]"
    "[time-advance][transport][process-boundary][public-endpoint]"
    "[timestamped-process-attribute-update][process-tso-attribute-update-nmra-authoritative-coordinator]"
    "[rti.service.update-attribute-values][rti.service.next-message-request-available]"
    "[federate.callback.discover-object-instance][federate.callback.reflect-attribute-values]"
    "[federate.callback.time-advance-grant]"
    "[requirement:hla-1516.1-2025:clause-8.11]"
    "[requirement:hla-1516.1-2025:clause-8.11.3]"
    "[requirement:hla-1516.1-2025:clause-8.12.3][2025]") {
  TemporaryDirectory directory;
  std::filesystem::path const serviceProbe(
      UMBRA_PROCESS_SERVICE_PROBE_PATH);
  ProxyProcess authority(
      serviceProbe, directory.path(), "attribute-tso-authority", true);
  ProxyProcess sender(
      serviceProbe, directory.path(), "attribute-tso-sender", true);
  ProxyProcess receiver(
      serviceProbe, directory.path(), "attribute-tso-receiver", true);

  auto const receiverEvidence = waitForProcessMarker(
      directory.path(), "attribute-tso-receiver.completed", "attribute-tso-receiver");
  auto const senderEvidence = waitForProcessMarker(
      directory.path(), "attribute-tso-sender.completed", "attribute-tso-sender");
  REQUIRE(receiverEvidence == "attribute-before-grant-at-5");
  REQUIRE(senderEvidence == "sent-timestamp-5");

  receiver.stop();
  sender.stop();
  authority.stop();
}

TEST_CASE(
    "Independent RTI client processes deliver a late TSO attribute update before NMR grant through one authoritative coordinator",
    "[integration][foundation][object-management][time-management]"
    "[time-advance][transport][process-boundary][public-endpoint]"
    "[timestamped-process-attribute-update][process-tso-attribute-update-nmr-authoritative-coordinator]"
    "[rti.service.update-attribute-values][rti.service.next-message-request]"
    "[federate.callback.discover-object-instance][federate.callback.reflect-attribute-values]"
    "[federate.callback.time-advance-grant]"
    "[requirement:hla-1516.1-2025:clause-8.10.2][2025]") {
  TemporaryDirectory directory;
  std::filesystem::path const serviceProbe(
      UMBRA_PROCESS_SERVICE_PROBE_PATH);
  ProxyProcess authority(
      serviceProbe, directory.path(), "attribute-tso-nmr-authority", true);
  ProxyProcess sender(
      serviceProbe, directory.path(), "attribute-tso-nmr-sender", true);
  ProxyProcess receiver(
      serviceProbe, directory.path(), "attribute-tso-nmr-receiver", true);

  auto const receiverEvidence = waitForProcessMarker(
      directory.path(),
      "attribute-tso-nmr-receiver.completed",
      "attribute-tso-nmr-receiver");
  auto const senderEvidence = waitForProcessMarker(
      directory.path(),
      "attribute-tso-nmr-sender.completed",
      "attribute-tso-nmr-sender");
  REQUIRE(receiverEvidence == "attribute-before-nmr-grant-at-5");
  REQUIRE(senderEvidence == "sent-timestamp-5");

  receiver.stop();
  sender.stop();
  authority.stop();
}

TEST_CASE(
    "Independent RTI client processes return a timestamped class-request provider response before TAR grant through one authoritative coordinator",
    "[integration][foundation][object-management][time-management][time-advance]"
    "[transport][process-boundary][public-endpoint]"
    "[process-class-request-tso-response-independent-processes]"
    "[rti.service.request-attribute-value-update]"
    "[rti.service.subscribe-object-class-attributes]"
    "[rti.service.enable-time-constrained][rti.service.enable-time-regulation]"
    "[rti.service.time-advance-request][rti.service.update-attribute-values]"
    "[rti.service.retract][rti.service.change-default-attribute-order-type]"
    "[federate.callback.discover-object-instance]"
    "[federate.callback.provide-attribute-value-update]"
    "[federate.callback.reflect-attribute-values][federate.callback.time-advance-grant]"
    "[federate.callback.time-regulation-enabled]"
    "[federate.callback.time-constrained-enabled]"
    "[requirement:hla-1516.1-2025:clause-6.21]"
    "[requirement:hla-1516.1-2025:clause-6.21.5]"
    "[requirement:hla-1516.1-2025:clause-6.22]"
    "[requirement:hla-1516.1-2025:clause-8.1.5]"
    "[requirement:hla-1516.1-2025:clause-8.1.6]"
    "[requirement:hla-1516.1-2025:clause-8.22.3]"
    "[inherited-attribute-default-order][2025]") {
  TemporaryDirectory directory;
  std::filesystem::path const serviceProbe(
      UMBRA_PROCESS_SERVICE_PROBE_PATH);
  ProxyProcess authority(
      serviceProbe, directory.path(), "class-attribute-response-authority", true);
  ProxyProcess provider(
      serviceProbe, directory.path(), "class-attribute-response-provider", true);
  ProxyProcess requester(
      serviceProbe, directory.path(), "class-attribute-response-requester", true);

  auto const requesterEvidence = waitForProcessMarker(
      directory.path(),
      "class-attribute-response-requester.completed",
      "class-attribute-response-requester");
  auto const providerEvidence = waitForProcessMarker(
      directory.path(),
      "class-attribute-response-provider.completed",
      "class-attribute-response-provider");
  REQUIRE(requesterEvidence == "class-response-reflected-before-tar-grant-2");
  REQUIRE(providerEvidence ==
          "two-provider-callbacks-response-before-grant-retract-rejected");

  requester.stop();
  provider.stop();
  authority.stop();
}

TEST_CASE(
    "Process RTIambassadors route queued TSO through receive order after Disable Time Constrained",
    "[integration][foundation][interaction-management][time-management]"
    "[lane.tcd-transition-tso-to-ro][time-constrained-disable-tso-to-ro]"
    "[process-boundary][public-endpoint]"
    "[rti.service.disable-time-constrained][rti.service.send-interaction]"
    "[rti.service.query-galt]"
    "[federate.callback.receive-interaction]"
    "[requirement:hla-1516.1-2025:clause-8.7][2025]") {
  TemporaryDirectory directory;
  std::filesystem::path const serviceProbe(
      UMBRA_PROCESS_SERVICE_PROBE_PATH);
  ProxyProcess authority(
      serviceProbe, directory.path(), "tcd-authority", true);
  ProxyProcess sender(
      serviceProbe, directory.path(), "tcd-sender", true);
  ProxyProcess receiver(
      serviceProbe, directory.path(), "tcd-receiver", true);

  auto const receiverEvidence = waitForProcessMarker(
      directory.path(), "tcd-receiver.completed", "tcd-receiver");
  auto const senderEvidence = waitForProcessMarker(
      directory.path(), "tcd-sender.completed", "tcd-sender");
  REQUIRE(receiverEvidence == "queued-tso-delivered-receive-order");
  REQUIRE(senderEvidence == "queued-tso-delivered-receive-order");

  receiver.stop();
  sender.stop();
  authority.stop();
}

#endif

TEST_CASE(
    "RTIambassadors coordinate future NMR and NMRA messages across service endpoints sharing one registry",
    "[integration][foundation][interaction-management][time-management]"
    "[time-advance][transport][process-boundary][public-endpoint]"
    "[process-tso-shared-registry-endpoint-arrival]"
    "[rti.service.send-interaction][rti.service.next-message-request]"
    "[rti.service.next-message-request-available]"
    "[federate.callback.receive-interaction][federate.callback.time-advance-grant]"
    "[2025]") {
  EmbeddedFederationRegistry registry;
  ProcessFederationServiceHost firstEndpoint(
      registry,
      composedDefinition(),
      {"127.0.0.1", 0U},
      {"process-tso-shared-registry-first-endpoint", 0xB823U});
  ProcessFederationServiceHost secondEndpoint(
      registry,
      composedDefinition(),
      {"127.0.0.1", 0U},
      {"process-tso-shared-registry-second-endpoint", 0xB824U});
  firstEndpoint.start();
  secondEndpoint.start();

  SECTION("NMR future arrival across endpoint services") {
    verifyPublicArrivalScenario(
        firstEndpoint.address(),
        secondEndpoint.address(),
        L"shared-registry-endpoint-nmr",
        false,
        secondEndpoint.address());
  }
  SECTION("NMRA future arrival across endpoint services") {
    verifyPublicArrivalScenario(
        firstEndpoint.address(),
        secondEndpoint.address(),
        L"shared-registry-endpoint-nmra",
        true,
        secondEndpoint.address());
  }

  firstEndpoint.stop();
  secondEndpoint.stop();
  REQUIRE_NOTHROW(firstEndpoint.rethrowIfFailed());
  REQUIRE_NOTHROW(secondEndpoint.rethrowIfFailed());
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

TEST_CASE(
    "RTIambassadors preserve HLA_EVOKED process NMR and NMRA delivery through separate ingress proxy processes",
    "[integration][foundation][time-management][time-advance][transport]"
    "[process-boundary][public-endpoint][process-tso-cross-endpoint-arrival-coordination]"
    "[process-tso-proxy-subprocess-evoked]"
    "[rti.service.send-interaction][rti.service.next-message-request]"
    "[rti.service.next-message-request-available][federate.callback.receive-interaction]"
    "[federate.callback.time-advance-grant][2025]") {
  EmbeddedFederationRegistry registry;
  ProcessFederationServiceHost authority(
      registry,
      composedDefinition(),
      {"127.0.0.1", 0U},
      {"process-tso-subprocess-evoked-authority", 0xB830U});
  authority.start();

  TemporaryDirectory directory;
  writeMarker(
      directory.path() / "authority.port",
      std::to_string(authority.address().port));
  std::filesystem::path const probe(UMBRA_PROCESS_TRANSPORT_PROXY_PROBE_PATH);
  ProxyProcess senderProxy(probe, directory.path(), "sender");
  ProxyProcess receiverProxy(probe, directory.path(), "receiver");
  REQUIRE(senderProxy.address().port != receiverProxy.address().port);

  SECTION("HLA_EVOKED NMR through separate proxy processes") {
    verifyPublicArrivalScenario(
        senderProxy.address(),
        receiverProxy.address(),
        L"subprocess-evoked-nmr",
        false,
        std::nullopt,
        HLA_EVOKED);
  }
  SECTION("HLA_EVOKED NMRA through separate proxy processes") {
    verifyPublicArrivalScenario(
        senderProxy.address(),
        receiverProxy.address(),
        L"subprocess-evoked-nmra",
        true,
        std::nullopt,
        HLA_EVOKED);
  }

  senderProxy.stop();
  receiverProxy.stop();
  authority.stop();
  REQUIRE_NOTHROW(authority.rethrowIfFailed());
}

TEST_CASE(
    "RTIambassadors deliver future timestamped object deletion through separate ingress proxy processes while NMR or NMRA is pending",
    "[integration][foundation][object-management][time-management][time-advance]"
    "[transport][process-boundary][public-endpoint][process-tso-proxy-subprocess-timestamped-delete-arrival]"
    "[rti.service.delete-object-instance][rti.service.next-message-request]"
    "[rti.service.next-message-request-available][rti.service.time-advance-request]"
    "[federate.callback.remove-object-instance][federate.callback.time-advance-grant][2025]") {
  EmbeddedFederationRegistry registry;
  ProcessFederationServiceHost authority(
      registry,
      composedDefinition(),
      {"127.0.0.1", 0U},
      {"process-tso-proxy-delete-authority", 0xB840U});
  authority.start();

  TemporaryDirectory directory;
  writeMarker(
      directory.path() / "authority.port",
      std::to_string(authority.address().port));
  std::filesystem::path const probe(UMBRA_PROCESS_TRANSPORT_PROXY_PROBE_PATH);
  ProxyProcess senderProxy(probe, directory.path(), "sender");
  ProxyProcess receiverProxy(probe, directory.path(), "receiver");
  REQUIRE(senderProxy.address().port != receiverProxy.address().port);

  SECTION("NMR pending during future timestamped deletion") {
    verifyTimestampedDeleteArrivalThroughProxyProcesses(
        senderProxy.address(), receiverProxy.address(), L"nmr", false);
  }
  SECTION("NMRA pending during future timestamped deletion") {
    verifyTimestampedDeleteArrivalThroughProxyProcesses(
        senderProxy.address(), receiverProxy.address(), L"nmra", true);
  }

  senderProxy.stop();
  receiverProxy.stop();
  authority.stop();
  REQUIRE_NOTHROW(authority.rethrowIfFailed());
}

void verifyAuthoritativeDeleteTsoScenario(std::string const& advanceMode) {
  TemporaryDirectory directory;
  std::filesystem::path const serviceProbe(
      UMBRA_PROCESS_SERVICE_PROBE_PATH);
  std::string const prefix = "delete-tso-" + advanceMode;
  ProxyProcess authority(
      serviceProbe, directory.path(), prefix + "-authority", true);
  ProxyProcess sender(
      serviceProbe, directory.path(), prefix + "-sender", true);
  ProxyProcess receiver(
      serviceProbe, directory.path(), prefix + "-receiver", true);

  if (advanceMode == "tara") {
    static_cast<void>(waitForProcessMarker(
        directory.path(),
        prefix + "-receiver.requested",
        prefix + "-receiver"));
    static_cast<void>(waitForProcessMarker(
        directory.path(),
        prefix + "-sender.sent",
        prefix + "-sender"));
  }

  auto const receiverEvidence = waitForProcessMarker(
      directory.path(),
      prefix + "-receiver.completed",
      prefix + "-receiver");
  auto const senderEvidence = waitForProcessMarker(
      directory.path(),
      prefix + "-sender.completed",
      prefix + "-sender");
  std::string const expectedGrantTime = advanceMode == "tara" ? "6" : "5";
  REQUIRE(receiverEvidence ==
      "delete-before-" + advanceMode + "-grant-at-" + expectedGrantTime);
  REQUIRE(senderEvidence == "sent-delete-at-5");

  receiver.stop();
  sender.stop();
  authority.stop();
}

TEST_CASE(
    "Independent client processes coordinate no-TSO NMR, NMRA, and TARA grants through one authoritative coordinator",
    "[integration][foundation][time-management][time-role][time-advance][transport]"
    "[process-boundary][public-endpoint][multi-federate][scheduler]"
    "[process-no-tso-cross-federate-grant-coordinator-os-process]"
    "[rti.service.enable-time-regulation][rti.service.enable-time-constrained]"
    "[rti.service.next-message-request][rti.service.next-message-request-available]"
    "[rti.service.time-advance-request][rti.service.time-advance-request-available]"
    "[federate.callback.time-advance-grant]"
    "[requirement:hla-1516.1-2025:clause-8.10.2]"
    "[requirement:hla-1516.1-2025:clause-8.11]"
    "[requirement:hla-1516.1-2025:clause-8.11.3]"
    "[requirement:hla-1516.1-2025:clause-8.9][2025]") {
  TemporaryDirectory directory;
  std::filesystem::path const serviceProbe(
      UMBRA_PROCESS_SERVICE_PROBE_PATH);
  ProxyProcess authority(
      serviceProbe,
      directory.path(),
      "no-tso-coordinator-authority",
      true);
  ProxyProcess regulator(
      serviceProbe,
      directory.path(),
      "no-tso-coordinator-regulator",
      true);
  ProxyProcess requester(
      serviceProbe,
      directory.path(),
      "no-tso-coordinator-requester",
      true);

  auto const requesterEvidence = waitForProcessMarker(
      directory.path(),
      "no-tso-coordinator-requester.completed",
      "no-tso-coordinator-requester");
  auto const regulatorEvidence = waitForProcessMarker(
      directory.path(),
      "no-tso-coordinator-regulator.completed",
      "no-tso-coordinator-regulator");
  REQUIRE(requesterEvidence == "NMR-5-NMRA-7-TARA-8");
  REQUIRE(regulatorEvidence == "regulator-granted-at-1-2-and-3");

  requester.stop();
  regulator.stop();
  authority.stop();
}

TEST_CASE(
    "Independent client processes deliver a timestamped object deletion before NMR grant through one authoritative coordinator",
    "[integration][foundation][object-management][time-management][time-advance]"
    "[transport][process-boundary][public-endpoint]"
    "[process-tso-delete-object-nmr-authoritative-coordinator-os-process]"
    "[rti.service.delete-object-instance][rti.service.next-message-request]"
    "[rti.service.time-advance-request][federate.callback.discover-object-instance]"
    "[federate.callback.remove-object-instance][federate.callback.time-advance-grant]"
    "[requirement:hla-1516.1-2025:clause-6.16]"
    "[requirement:hla-1516.1-2025:clause-6.17.1]"
    "[requirement:hla-1516.1-2025:clause-8.10.2][2025]") {
  verifyAuthoritativeDeleteTsoScenario("nmr");
}

TEST_CASE(
    "Independent client processes deliver a timestamped object deletion before NMRA grant through one authoritative coordinator",
    "[integration][foundation][object-management][time-management][time-advance]"
    "[transport][process-boundary][public-endpoint]"
    "[process-tso-delete-object-nmra-authoritative-coordinator-os-process]"
    "[rti.service.delete-object-instance][rti.service.next-message-request-available]"
    "[rti.service.time-advance-request][federate.callback.discover-object-instance]"
    "[federate.callback.remove-object-instance][federate.callback.time-advance-grant]"
    "[requirement:hla-1516.1-2025:clause-6.16]"
    "[requirement:hla-1516.1-2025:clause-6.17.1]"
    "[requirement:hla-1516.1-2025:clause-8.11.3][2025]") {
  verifyAuthoritativeDeleteTsoScenario("nmra");
}

TEST_CASE(
    "Independent client processes deliver a timestamped object deletion before TARA grant through one authoritative coordinator",
    "[integration][foundation][object-management][time-management][time-advance]"
    "[transport][process-boundary][public-endpoint]"
    "[process-tso-delete-object-tara-authoritative-coordinator-os-process]"
    "[rti.service.delete-object-instance][rti.service.time-advance-request-available]"
    "[rti.service.time-advance-request][federate.callback.discover-object-instance]"
    "[federate.callback.remove-object-instance][federate.callback.time-advance-grant]"
    "[requirement:hla-1516.1-2025:clause-6.16]"
    "[requirement:hla-1516.1-2025:clause-6.17.1]"
    "[requirement:hla-1516.1-2025:clause-8.9][2025]") {
  verifyAuthoritativeDeleteTsoScenario("tara");
}

#endif

#endif
