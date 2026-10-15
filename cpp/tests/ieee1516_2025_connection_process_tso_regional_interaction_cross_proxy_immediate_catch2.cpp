#include <catch2/catch_test_macros.hpp>

#if defined(UMBRA_ENABLE_EMBEDDED_FEDERATION_MANAGEMENT) && \
    defined(UMBRA_PROCESS_TRANSPORT_PROXY_PROBE_PATH)

#include <array>
#include <atomic>
#include <chrono>
#include <condition_variable>
#include <cstdint>
#include <filesystem>
#include <fstream>
#include <memory>
#include <mutex>
#include <stdexcept>
#include <string>
#include <system_error>
#include <thread>
#include <utility>
#include <vector>

#if defined(_WIN32)
#ifndef NOMINMAX
#define NOMINMAX
#endif
#include <windows.h>
#else
#include <cerrno>
#include <csignal>
#include <sys/types.h>
#include <sys/wait.h>
#include <unistd.h>
#endif

#include <RTI/NullFederateAmbassador.h>
#include <RTI/RTI1516.h>
#include <RTI/time/HLAinteger64Interval.h>
#include <RTI/time/HLAinteger64Time.h>

#include "internal/federation/federation_registry.hpp"
#include "internal/federation/process_federation_service_host.hpp"
#include "internal/fom/fom_validation.hpp"
#include "internal/fom/libxml2_fom_composer.hpp"
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
    throw std::runtime_error("The regional cross-proxy FOM did not validate.");
  }
  return *result.module;
}

FederationDefinition composedDefinition() {
  std::vector<PrevalidatedFomModule> modules{
      validateModule(
          resourcePath("mim/HLAstandardMIM-2025.xml"),
          FomModuleKind::mim,
          L"urn:umbra:test:regional-cross-proxy-mim"),
      validateModule(
          resourcePath("examples/RestaurantFOMmodule-2025.xml"),
          FomModuleKind::fom,
          L"urn:umbra:test:regional-cross-proxy-restaurant"),
  };
  LibXml2FomModuleComposer composer(
      resourcePath("schemas/IEEE1516-FDD-2025.xsd"));
  auto result = composer.compose(modules);
  if (result.status != FomCompositionStatus::valid || !result.catalog ||
      !result.fdd) {
    throw std::runtime_error("The regional cross-proxy FOM did not compose.");
  }
  return {
      std::move(result.modules),
      L"HLAinteger64Time",
      std::move(result.catalog),
      std::move(result.fdd),
  };
}

class TemporaryDirectory final {
 public:
  TemporaryDirectory() {
    static std::atomic_uint64_t sequence{0U};
    for (unsigned int attempt = 0U; attempt < 32U; ++attempt) {
      auto const token = std::to_string(
          std::chrono::steady_clock::now().time_since_epoch().count()) + "-" +
          std::to_string(sequence.fetch_add(1U, std::memory_order_relaxed));
      path_ = std::filesystem::temp_directory_path() /
          ("umbra-process-regional-cross-proxy-" + token);
      std::error_code error;
      if (std::filesystem::create_directory(path_, error)) {
        return;
      }
      if (error && error != std::errc::file_exists) {
        throw std::filesystem::filesystem_error(
            "Could not create cross-proxy directory", path_, error);
      }
    }
    throw std::runtime_error("Could not allocate a cross-proxy directory.");
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

void writeMarker(std::filesystem::path const& path, std::string const& value) {
  std::ofstream output(path, std::ios::binary | std::ios::trunc);
  if (!output || !(output << value)) {
    throw std::runtime_error("Could not write a cross-proxy marker.");
  }
}

class ProxyProcess final {
 public:
  ProxyProcess(
      std::filesystem::path const& executable,
      std::filesystem::path const& directory,
      std::string role)
      : directory_(directory), role_(std::move(role)) {
    launch(executable);
    try {
      port_ = waitForPort();
    } catch (...) {
      requestStopNoThrow();
      finishNoThrow();
#if defined(_WIN32)
      CloseHandle(process_);
      process_ = nullptr;
#endif
      throw;
    }
  }

  ProxyProcess(ProxyProcess const&) = delete;
  ProxyProcess& operator=(ProxyProcess const&) = delete;

  ~ProxyProcess() {
    requestStopNoThrow();
    finishNoThrow();
#if defined(_WIN32)
    if (process_ != nullptr) {
      CloseHandle(process_);
    }
#endif
  }

  [[nodiscard]] ProcessTransportAddress address() const noexcept {
    return {"127.0.0.1", port_};
  }

  void stop() {
    requestStop();
    if (!waitForExit(std::chrono::seconds(5))) {
      terminateAndReapNoThrow();
      throw std::runtime_error("A regional ingress process did not stop.");
    }
    if (!exitedSuccessfully_ ||
        !std::filesystem::exists(directory_ / (role_ + ".ok"))) {
      throw std::runtime_error("A regional ingress process exited unsuccessfully.");
    }
  }

 private:
  void launch(std::filesystem::path const& executable) {
#if defined(_WIN32)
    auto const executableName = executable.wstring();
    auto command = L"\"" + executableName + L"\" \"" +
        directory_.wstring() + L"\" " +
        std::wstring(role_.begin(), role_.end());
    STARTUPINFOW startup{};
    startup.cb = sizeof(startup);
    PROCESS_INFORMATION information{};
    if (!CreateProcessW(
            executableName.c_str(), command.data(), nullptr, nullptr, FALSE,
            CREATE_NO_WINDOW, nullptr, nullptr, &startup, &information)) {
      throw std::system_error(
          static_cast<int>(GetLastError()), std::system_category(),
          "Could not launch a regional ingress process");
    }
    process_ = information.hProcess;
    CloseHandle(information.hThread);
#else
    auto const executableName = executable.string();
    auto const directoryName = directory_.string();
    char* const arguments[] = {
        const_cast<char*>(executableName.c_str()),
        const_cast<char*>(directoryName.c_str()),
        const_cast<char*>(role_.c_str()), nullptr};
    auto const child = fork();
    if (child < 0) {
      throw std::system_error(
          errno, std::generic_category(),
          "Could not launch a regional ingress process");
    }
    if (child == 0) {
      execv(arguments[0], arguments);
      _exit(127);
    }
    process_ = child;
#endif
  }

  [[nodiscard]] bool waitForExit(std::chrono::milliseconds timeout) noexcept {
    if (reaped_) {
      return true;
    }
#if defined(_WIN32)
    if (process_ == nullptr) {
      return false;
    }
    if (WaitForSingleObject(process_, static_cast<DWORD>(timeout.count())) !=
        WAIT_OBJECT_0) {
      return false;
    }
    DWORD exitCode = STILL_ACTIVE;
    exitedSuccessfully_ =
        GetExitCodeProcess(process_, &exitCode) != 0 && exitCode == 0;
    reaped_ = true;
    return true;
#else
    if (process_ < 0) {
      return false;
    }
    auto const deadline = std::chrono::steady_clock::now() + timeout;
    do {
      int status = 0;
      auto const result = waitpid(process_, &status, WNOHANG);
      if (result == process_) {
        exitedSuccessfully_ = WIFEXITED(status) && WEXITSTATUS(status) == 0;
        reaped_ = true;
        return true;
      }
      if (result < 0 && errno != EINTR) {
        return false;
      }
      if (std::chrono::steady_clock::now() >= deadline) {
        return false;
      }
      std::this_thread::sleep_for(std::chrono::milliseconds(10));
    } while (true);
#endif
  }

  void terminateAndReapNoThrow() noexcept {
    if (reaped_) {
      return;
    }
#if defined(_WIN32)
    if (process_ != nullptr) {
      TerminateProcess(process_, 1U);
    }
#else
    if (process_ >= 0) {
      kill(process_, SIGKILL);
    }
#endif
    static_cast<void>(waitForExit(std::chrono::seconds(2)));
  }

  void finishNoThrow() noexcept {
    if (!waitForExit(std::chrono::seconds(2))) {
      terminateAndReapNoThrow();
    }
  }

  [[nodiscard]] unsigned short waitForPort() {
    auto const portPath = directory_ / (role_ + ".port");
    auto const errorPath = directory_ / (role_ + ".error");
    auto const deadline = std::chrono::steady_clock::now() +
        std::chrono::seconds(10);
    while (std::chrono::steady_clock::now() < deadline) {
      if (std::filesystem::exists(errorPath)) {
        throw std::runtime_error("A regional ingress process failed to start.");
      }
      if (std::filesystem::exists(portPath)) {
        std::ifstream input(portPath, std::ios::binary);
        unsigned int port = 0U;
        if (input >> port && port > 0U && port <= 65535U) {
          return static_cast<unsigned short>(port);
        }
        throw std::runtime_error("A regional ingress process published an invalid port.");
      }
      if (waitForExit(std::chrono::milliseconds(0))) {
        throw std::runtime_error("A regional ingress process exited before readiness.");
      }
      std::this_thread::sleep_for(std::chrono::milliseconds(10));
    }
    throw std::runtime_error("A regional ingress process did not become ready.");
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

  std::filesystem::path directory_;
  std::string role_;
#if defined(_WIN32)
  HANDLE process_ = nullptr;
#else
  pid_t process_ = -1;
#endif
  unsigned short port_ = 0U;
  bool stopRequested_ = false;
  bool reaped_ = false;
  bool exitedSuccessfully_ = false;
};

struct CallbackEvent final {
  char kind = '?';
  std::int64_t time = -1;
  rti1516_2025::InteractionClassHandle interaction;
  rti1516_2025::FederateHandle producer;
  rti1516_2025::ParameterHandle parameter;
  std::uint8_t payload = 0U;
  rti1516_2025::RegionHandleSet sentRegions;
  rti1516_2025::OrderType receivedOrder = rti1516_2025::RECEIVE;
};

class RecordingFederate final : public NullFederateAmbassador {
 public:
  void receiveInteraction(
      rti1516_2025::InteractionClassHandle const& interaction,
      ParameterHandleValueMap const& values,
      VariableLengthData const&,
      rti1516_2025::TransportationTypeHandle const&,
      rti1516_2025::FederateHandle const& producer,
      rti1516_2025::RegionHandleSet const* sentRegions,
      rti1516_2025::LogicalTime const& time,
      rti1516_2025::OrderType,
      rti1516_2025::OrderType receivedOrder,
      rti1516_2025::MessageRetractionHandle const*) override {
    CallbackEvent event;
    event.kind = 'I';
    event.interaction = interaction;
    if (auto const* integerTime =
            dynamic_cast<rti1516_2025::HLAinteger64Time const*>(&time)) {
      event.time = integerTime->getTime();
    }
    event.producer = producer;
    if (sentRegions != nullptr) {
      event.sentRegions = *sentRegions;
    }
    event.receivedOrder = receivedOrder;
    if (values.size() == 1U && values.begin()->second.size() == 1U) {
      event.parameter = values.begin()->first;
      event.payload = *static_cast<std::uint8_t const*>(
          values.begin()->second.data());
    }
    append(event);
  }

  void timeAdvanceGrant(rti1516_2025::LogicalTime const& time) override {
    CallbackEvent event;
    event.kind = 'G';
    if (auto const* integerTime =
            dynamic_cast<rti1516_2025::HLAinteger64Time const*>(&time)) {
      event.time = integerTime->getTime();
    }
    append(event);
  }

  [[nodiscard]] bool waitForEvents(
      std::size_t count,
      std::chrono::milliseconds timeout = std::chrono::seconds(3)) const {
    std::unique_lock lock(mutex_);
    return changed_.wait_for(lock, timeout, [this, count] {
      return events_.size() >= count;
    });
  }

  [[nodiscard]] std::vector<CallbackEvent> events() const {
    std::scoped_lock lock(mutex_);
    return events_;
  }

 private:
  void append(CallbackEvent event) {
    std::scoped_lock lock(mutex_);
    events_.push_back(std::move(event));
    changed_.notify_all();
  }

  mutable std::mutex mutex_;
  mutable std::condition_variable changed_;
  std::vector<CallbackEvent> events_;
};

std::unique_ptr<RTIambassador> makeRti() {
  RTIambassadorFactory factory;
  return factory.createRTIambassador();
}

RtiConfiguration configurationFor(
    ProcessTransportAddress const& address,
    wchar_t const* name) {
  return RtiConfiguration::createConfiguration()
      .withConfigurationName(name)
      .withRtiAddress(L"tcp://127.0.0.1:" + std::to_wstring(address.port));
}

}  // namespace

TEST_CASE(
    "RTIambassadors retarget pending process NMR across two OS ingress proxies for regional interactions",
    "[integration][foundation][data-distribution-management]"
    "[interaction-management][time-management][time-advance][transport]"
    "[process-boundary][public-endpoint][multi-federate][regional-interaction]"
    "[timestamped-process-regional-interaction]"
    "[process-time-advance-next-message-queued-focused]"
    "[process-tso-cross-endpoint-arrival-coordination]"
    "[rti.service.create-region][rti.service.set-range-bounds]"
    "[rti.service.commit-region-modifications]"
    "[rti.service.subscribe-interaction-class-with-regions]"
    "[rti.service.send-interaction-with-regions]"
    "[rti.service.enable-time-regulation][rti.service.enable-time-constrained]"
    "[rti.service.next-message-request][federate.callback.receive-interaction]"
    "[federate.callback.time-advance-grant][2025]") {
  EmbeddedFederationRegistry registry;
  ProcessFederationServiceHost authority(
      registry,
      composedDefinition(),
      {"127.0.0.1", 0U},
      {"regional-cross-proxy-authority", 0xB840U});
  authority.start();

  TemporaryDirectory directory;
  writeMarker(
      directory.path() / "authority.port",
      std::to_string(authority.address().port));
  std::filesystem::path const probe(UMBRA_PROCESS_TRANSPORT_PROXY_PROBE_PATH);
  ProxyProcess laterIngress(probe, directory.path(), "sender");
  ProxyProcess earlierIngress(probe, directory.path(), "receiver");
  REQUIRE(laterIngress.address().port != earlierIngress.address().port);
  REQUIRE(laterIngress.address().port != authority.address().port);
  REQUIRE(earlierIngress.address().port != authority.address().port);

  constexpr wchar_t const* federationName =
      L"regional-cross-proxy-immediate-nmr";
  constexpr wchar_t const* interactionName =
      L"HLAinteractionRoot.CustomerTransactions.FoodServed.MainCourseServed";
  constexpr wchar_t const* parameterName = L"TimelinessOk";
  constexpr wchar_t const* dimensionName = L"ServerId";
  RecordingFederate laterFederate;
  RecordingFederate earlierFederate;
  RecordingFederate receiverFederate;
  auto later = makeRti();
  auto earlier = makeRti();
  auto receiver = makeRti();
  REQUIRE(later->connect(
              laterFederate,
              HLA_IMMEDIATE,
              configurationFor(laterIngress.address(), L"regional-later"))
              .addressUsed);
  REQUIRE_NOTHROW(later->createFederationExecution(
      federationName, L"server-owned-regional-cross-proxy-fom.xml"));
  auto const laterHandle = later->joinFederationExecution(
      L"regional-cross-proxy-later", L"regional-cross-proxy-type", federationName);
  REQUIRE(laterHandle.isValid());
  REQUIRE(earlier->connect(
              earlierFederate,
              HLA_IMMEDIATE,
              configurationFor(earlierIngress.address(), L"regional-earlier"))
              .addressUsed);
  auto const earlierHandle = earlier->joinFederationExecution(
      L"regional-cross-proxy-earlier", L"regional-cross-proxy-type", federationName);
  REQUIRE(earlierHandle.isValid());
  REQUIRE(receiver->connect(
              receiverFederate,
              HLA_IMMEDIATE,
              configurationFor(authority.address(), L"regional-receiver"))
              .addressUsed);
  auto const receiverHandle = receiver->joinFederationExecution(
      L"regional-cross-proxy-receiver", L"regional-cross-proxy-type", federationName);
  REQUIRE(receiverHandle.isValid());

  auto const laterClass = later->getInteractionClassHandle(interactionName);
  auto const earlierClass = earlier->getInteractionClassHandle(interactionName);
  auto const receiverClass = receiver->getInteractionClassHandle(interactionName);
  REQUIRE(laterClass == earlierClass);
  REQUIRE(laterClass == receiverClass);
  auto const laterParameter = later->getParameterHandle(
      laterClass, parameterName);
  auto const earlierParameter = earlier->getParameterHandle(
      earlierClass, parameterName);
  auto const laterDimension = later->getDimensionHandle(dimensionName);
  auto const earlierDimension = earlier->getDimensionHandle(dimensionName);
  auto const receiverDimension = receiver->getDimensionHandle(dimensionName);
  auto const laterRegion = later->createRegion(
      rti1516_2025::DimensionHandleSet{laterDimension});
  auto const earlierRegion = earlier->createRegion(
      rti1516_2025::DimensionHandleSet{earlierDimension});
  auto const receiverRegion = receiver->createRegion(
      rti1516_2025::DimensionHandleSet{receiverDimension});
  REQUIRE(laterRegion.isValid());
  REQUIRE(earlierRegion.isValid());
  REQUIRE(receiverRegion.isValid());
  REQUIRE_NOTHROW(later->setRangeBounds(
      laterRegion, laterDimension, rti1516_2025::RangeBounds(0UL, 5UL)));
  REQUIRE_NOTHROW(earlier->setRangeBounds(
      earlierRegion, earlierDimension, rti1516_2025::RangeBounds(0UL, 5UL)));
  REQUIRE_NOTHROW(receiver->setRangeBounds(
      receiverRegion, receiverDimension, rti1516_2025::RangeBounds(0UL, 5UL)));
  REQUIRE_NOTHROW(later->commitRegionModifications(
      rti1516_2025::RegionHandleSet{laterRegion}));
  REQUIRE_NOTHROW(earlier->commitRegionModifications(
      rti1516_2025::RegionHandleSet{earlierRegion}));
  REQUIRE_NOTHROW(receiver->commitRegionModifications(
      rti1516_2025::RegionHandleSet{receiverRegion}));
  REQUIRE_NOTHROW(later->publishInteractionClass(laterClass));
  REQUIRE_NOTHROW(earlier->publishInteractionClass(earlierClass));
  REQUIRE_NOTHROW(receiver->setConveyRegionDesignatorSetsSwitch(true));
  REQUIRE_NOTHROW(receiver->subscribeInteractionClassWithRegions(
      receiverClass,
      rti1516_2025::RegionHandleSet{receiverRegion},
      true));
  REQUIRE_NOTHROW(later->enableTimeRegulation(
      rti1516_2025::HLAinteger64Interval(1)));
  REQUIRE_NOTHROW(earlier->enableTimeRegulation(
      rti1516_2025::HLAinteger64Interval(1)));
  REQUIRE_NOTHROW(receiver->enableTimeConstrained());
  REQUIRE_NOTHROW(receiver->nextMessageRequest(
      rti1516_2025::HLAinteger64Time(10)));
  REQUIRE(receiverFederate.events().empty());

  auto sendRegional = [](
                          RTIambassador& sender,
                          rti1516_2025::InteractionClassHandle const& interaction,
                          rti1516_2025::ParameterHandle const& parameter,
                          rti1516_2025::RegionHandle const& region,
                          std::uint8_t payload,
                          std::int64_t timestamp) {
    ParameterHandleValueMap values;
    values.emplace(parameter, VariableLengthData(&payload, sizeof(payload)));
    std::uint8_t const tag = 0x52U;
    return sender.sendInteractionWithRegions(
        interaction,
        values,
        rti1516_2025::RegionHandleSet{region},
        VariableLengthData(&tag, sizeof(tag)),
        rti1516_2025::HLAinteger64Time(timestamp));
  };
  auto const laterRetraction = sendRegional(
      *later, laterClass, laterParameter, laterRegion, 0x08U, 8);
  REQUIRE(laterRetraction.isValid());
  REQUIRE_NOTHROW(later->timeAdvanceRequest(
      rti1516_2025::HLAinteger64Time(8)));
  REQUIRE(laterFederate.waitForEvents(1U));
  REQUIRE(laterFederate.events()[0].kind == 'G');
  REQUIRE(laterFederate.events()[0].time == 8);
  REQUIRE(receiverFederate.events().empty());

  auto const earlierRetraction = sendRegional(
      *earlier, earlierClass, earlierParameter, earlierRegion, 0x05U, 5);
  REQUIRE(earlierRetraction.isValid());
  REQUIRE_NOTHROW(earlier->timeAdvanceRequest(
      rti1516_2025::HLAinteger64Time(5)));
  REQUIRE(earlierFederate.waitForEvents(1U));
  REQUIRE(earlierFederate.events()[0].kind == 'G');
  REQUIRE(earlierFederate.events()[0].time == 5);

  // HLA_IMMEDIATE must deliver on its own callback thread. No receiver
  // evokeCallback or follow-up service call is made after NMR(10).
  REQUIRE(receiverFederate.waitForEvents(2U));
  auto const events = receiverFederate.events();
  REQUIRE(events.size() == 2U);
  REQUIRE(events[0].kind == 'I');
  REQUIRE(events[0].time == 5);
  REQUIRE(events[0].interaction == receiverClass);
  REQUIRE(events[0].payload == 0x05U);
  REQUIRE(events[0].producer == earlierHandle);
  REQUIRE(events[0].parameter == earlierParameter);
  REQUIRE(events[0].sentRegions ==
          rti1516_2025::RegionHandleSet{earlierRegion});
  REQUIRE(events[0].receivedOrder == rti1516_2025::TIMESTAMP);
  REQUIRE(events[1].kind == 'G');
  REQUIRE(events[1].time == 5);
  REQUIRE_FALSE(receiverFederate.waitForEvents(3U, std::chrono::milliseconds(150)));
  REQUIRE(receiverFederate.events().size() == 2U);

  REQUIRE_NOTHROW(later->resignFederationExecution(rti1516_2025::NO_ACTION));
  REQUIRE_NOTHROW(earlier->resignFederationExecution(rti1516_2025::NO_ACTION));
  REQUIRE_NOTHROW(receiver->resignFederationExecution(rti1516_2025::NO_ACTION));
  REQUIRE_NOTHROW(later->disconnect());
  REQUIRE_NOTHROW(earlier->disconnect());
  REQUIRE_NOTHROW(receiver->disconnect());
  laterIngress.stop();
  earlierIngress.stop();
  authority.stop();
  REQUIRE_NOTHROW(authority.rethrowIfFailed());
}

#endif
