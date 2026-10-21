#include <catch2/catch_test_macros.hpp>

#include "internal/federation/federation_registry.hpp"
#include "internal/federation/federation_save_commit_store.hpp"
#include "internal/federation/federation_state_image.hpp"
#include "internal/federation/process_federation_client.hpp"
#include "internal/federation/process_federation_service.hpp"
#include "process_federation_service_test_support.hpp"

#include <RTI/NullFederateAmbassador.h>

#include <algorithm>
#include <array>
#include <atomic>
#include <chrono>
#include <cstdint>
#include <exception>
#include <filesystem>
#include <memory>
#include <mutex>
#include <stdexcept>
#include <string>
#include <string_view>
#include <system_error>
#include <thread>
#include <utility>
#include <vector>

namespace {

using umbra::detail::EmbeddedFederationRegistry;
using umbra::detail::FederationSaveCommitStore;
using umbra::detail::FederationStateImageCodec;
using umbra::detail::FilesystemFederationSaveCommitStore;
using umbra::detail::ProcessFederationClient;
using umbra::detail::ProcessFederationService;
using umbra::detail::ProcessFederationServiceOptions;
using umbra::detail::ProcessTransportListener;
using umbra::detail::ProcessTransportSession;
using umbra::detail::ProcessTransportServiceDispatcher;
using umbra::detail::TransportServiceMessage;
using umbra::detail::TransportServiceOperation;
using umbra::test::process_federation_service_support::composedRestaurantDefinition;
using umbra::detail::CallbackDispatchModel;

class TemporaryDirectory final {
 public:
  explicit TemporaryDirectory(std::filesystem::path path)
      : path_(std::move(path)) {}
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

TemporaryDirectory makeTemporaryDirectory() {
  static std::atomic_uint64_t sequence{0U};
  auto const stamp = std::chrono::high_resolution_clock::now()
                         .time_since_epoch()
                         .count();
  for (std::size_t attempt = 0U; attempt != 128U; ++attempt) {
    auto path = std::filesystem::temp_directory_path() /
        ("umbra-push-ro-restart-" + std::to_string(stamp) + "-" +
         std::to_string(sequence.fetch_add(1U)));
    std::error_code error;
    if (std::filesystem::create_directory(path, error)) {
      return TemporaryDirectory(std::move(path));
    }
    if (error && error != std::errc::file_exists) {
      throw std::filesystem::filesystem_error(
          "Unable to create process-restart test directory", path, error);
    }
  }
  throw std::runtime_error("Unable to reserve a unique restart test directory.");
}

class ProcessServiceInstance final {
 public:
  explicit ProcessServiceInstance(
      std::shared_ptr<FederationSaveCommitStore> saveStore)
      : registry_(nullptr, std::move(saveStore)),
        service_(registry_, composedRestaurantDefinition(), options_) {
    listener_ = ProcessTransportListener::listen({"127.0.0.1", 0U});
    if (!listener_) {
      throw std::runtime_error("Unable to create the process service listener.");
    }
    port_ = listener_->address().port;
    server_ = std::thread([this] { runServer(); });
  }

  ProcessServiceInstance(ProcessServiceInstance const&) = delete;
  ProcessServiceInstance& operator=(ProcessServiceInstance const&) = delete;

  ~ProcessServiceInstance() {
    if (server_.joinable()) {
      listener_->close();
      server_.join();
    }
  }

  [[nodiscard]] std::uint16_t port() const noexcept { return port_; }

  void joinServer() {
    if (server_.joinable()) {
      server_.join();
    }
    if (failure_) {
      std::rethrow_exception(failure_);
    }
  }

  [[nodiscard]] std::vector<std::uint64_t> acknowledgedPushIds() const {
    std::scoped_lock lock(ackMutex_);
    return acknowledgedPushIds_;
  }

 private:
  void serve(
      ProcessTransportSession& session,
      ProcessTransportServiceDispatcher::Handler const& handler,
      std::size_t slot) {
    try {
      while (ProcessTransportServiceDispatcher::serveOne(session, handler)) {
      }
      service_.detach(session);
    } catch (...) {
      failures_[slot] = std::current_exception();
    }
    session.connection()->close();
  }

  void runServer() noexcept {
    try {
      auto firstConnection = listener_->accept(
          nullptr,
          {"push-ro-restart-server-a", 0xD1U},
          [](std::wstring) {},
          [](std::wstring) { return false; });
      auto secondConnection = listener_->accept(
          nullptr,
          {"push-ro-restart-server-b", 0xD2U},
          [](std::wstring) {},
          [](std::wstring) { return false; });
      ProcessTransportSession firstSession(std::move(firstConnection));
      ProcessTransportSession secondSession(std::move(secondConnection));
      auto firstHandler = service_.handlerFor(firstSession);
      auto secondHandler = service_.handlerFor(secondSession);
      auto wrap = [this](ProcessTransportServiceDispatcher::Handler handler) {
        return [this, handler = std::move(handler)](
                   TransportServiceMessage const& request) mutable {
          if (request.operation ==
              TransportServiceOperation::acknowledge_push_attribute_update) {
            auto const acknowledgement =
                umbra::detail::decodeProcessFederationAcknowledgeTsoDeliveryRequest(
                    request.payload);
            std::scoped_lock lock(ackMutex_);
            acknowledgedPushIds_.push_back(acknowledgement.messageId);
          }
          return handler(request);
        };
      };
      firstHandler = wrap(std::move(firstHandler));
      secondHandler = wrap(std::move(secondHandler));
      std::thread firstWorker([&] { serve(firstSession, firstHandler, 0U); });
      std::thread secondWorker([&] { serve(secondSession, secondHandler, 1U); });
      firstWorker.join();
      secondWorker.join();
      if (failures_[0]) {
        std::rethrow_exception(failures_[0]);
      }
      if (failures_[1]) {
        std::rethrow_exception(failures_[1]);
      }
    } catch (...) {
      failure_ = std::current_exception();
    }
  }

  EmbeddedFederationRegistry registry_;
  ProcessFederationServiceOptions options_ = [] {
    ProcessFederationServiceOptions value;
    value.pushReceiveOrderEvents = true;
    return value;
  }();
  ProcessFederationService service_;
  std::unique_ptr<ProcessTransportListener> listener_;
  std::uint16_t port_ = 0U;
  std::thread server_;
  std::array<std::exception_ptr, 2U> failures_{};
  std::exception_ptr failure_;
  mutable std::mutex ackMutex_;
  std::vector<std::uint64_t> acknowledgedPushIds_;
};

class ReflectionCounter final : public rti1516_2025::NullFederateAmbassador {
 public:
  void reflectAttributeValues(
      rti1516_2025::ObjectInstanceHandle const&,
      rti1516_2025::AttributeHandleValueMap const& values,
      rti1516_2025::VariableLengthData const&,
      rti1516_2025::TransportationTypeHandle const&,
      rti1516_2025::FederateHandle const&,
      rti1516_2025::RegionHandleSet const*) override {
    ++count;
    if (!values.empty()) {
      value = values.begin()->second;
    }
  }

  std::size_t count = 0U;
  rti1516_2025::VariableLengthData value;
};

bool waitForPendingReflection(ProcessFederationClient& receiver) {
  for (std::size_t attempt = 0U; attempt != 200U; ++attempt) {
    if (receiver.pendingPushedAttributeUpdateCount() != 0U ||
        receiver.pendingCallbackCount() != 0U) {
      return true;
    }
    std::this_thread::sleep_for(std::chrono::milliseconds{2});
  }
  return false;
}

}  // namespace

TEST_CASE(
    "Pushed ordinary receive-order attribute update is acknowledged exactly once after service and client restart",
    "[integration][internal-only][no-normative-mapping][process-boundary]"
    "[save-restore][filesystem][process-restart]"
    "[save-queued-ro-attribute-update-push-restart]") {
  auto const temporary = makeTemporaryDirectory();
  auto const saveStore = std::make_shared<FilesystemFederationSaveCommitStore>(
      temporary.path());
  std::wstring const federation = L"push-ro-client-restart-execution";
  constexpr std::wstring_view senderName = L"push-ro-restart-sender";
  constexpr std::wstring_view receiverName = L"push-ro-restart-receiver";
  std::wstring const saveLabel = L"push-ro-restart-save";
  std::uint64_t senderId = 0U;
  std::uint64_t receiverId = 0U;
  std::uint64_t objectId = 0U;
  std::uint64_t attributeId = 0U;
  std::uint64_t objectInstanceId = 0U;
  std::uint64_t pushDeliveryId = 0U;
  std::vector<std::uint8_t> const expectedValue{0x52U, 0x65U, 0x73U, 0x74U};

  {
    ProcessServiceInstance service(saveStore);
    ProcessFederationClient sender(
        {"127.0.0.1", service.port()}, {"push-ro-restart-client-a", 0xD3U});
    ProcessFederationClient receiver(
        {"127.0.0.1", service.port()}, {"push-ro-restart-client-b", 0xD4U});
    ReflectionCounter callbacks;
    receiver.attachCallbackBridge(callbacks, CallbackDispatchModel::evoked);
    REQUIRE_NOTHROW(sender.createFederationExecution(federation));
    auto const senderJoin = sender.joinFederationExecution(
        federation, std::wstring(senderName), std::wstring(senderName));
    auto const receiverJoin = receiver.joinFederationExecution(
        federation, std::wstring(receiverName), std::wstring(receiverName));
    senderId = senderJoin.federateId;
    receiverId = receiverJoin.federateId;
    REQUIRE(senderId != receiverId);

    auto const objectClass = sender.lookupObjectClassHandle(
        federation, senderId, L"HLAobjectRoot.Employee");
    REQUIRE(objectClass.has_value());
    auto const attribute = sender.lookupAttributeHandle(
        federation, senderId, *objectClass, L"Name");
    REQUIRE(attribute.has_value());
    objectId = *objectClass;
    attributeId = *attribute;
    REQUIRE_NOTHROW(sender.publishObjectClassAttributes(
        federation, senderId, objectId, {attributeId}));
    REQUIRE_NOTHROW(receiver.subscribeObjectClassAttributes(
        federation, receiverId, objectId, {attributeId}, true, ""));
    auto const registration = sender.registerObjectInstance(
        federation, senderId, objectId);
    objectInstanceId = registration.objectInstanceHandle;
    REQUIRE(objectInstanceId != 0U);
    while (receiver.evokeOne(std::chrono::milliseconds{0})) {
    }

    auto const update = sender.updateAttributeValues(
        federation,
        senderId,
        objectInstanceId,
        {{attributeId, expectedValue}},
        {});
    REQUIRE(update.recipientCount == 1U);
    REQUIRE(waitForPendingReflection(receiver));
    receiver.dispatchPendingPushedEvents();
    REQUIRE(receiver.pendingCallbackCount() >= 1U);
    REQUIRE(callbacks.count == 0U);

    REQUIRE(sender.requestFederationSave(federation, senderId, saveLabel).status ==
            umbra::detail::FederationSaveControlStatus::applied);
    REQUIRE(sender.federateSaveBegun(federation, senderId).status ==
            umbra::detail::FederationSaveControlStatus::applied);
    REQUIRE(receiver.federateSaveBegun(federation, receiverId).status ==
            umbra::detail::FederationSaveControlStatus::applied);
    REQUIRE(sender.federateSaveComplete(federation, senderId).status ==
            umbra::detail::FederationSaveControlStatus::applied);
    REQUIRE(receiver.federateSaveComplete(federation, receiverId).status ==
            umbra::detail::FederationSaveControlStatus::applied);

    auto const commit = saveStore->load(federation, saveLabel);
    REQUIRE(commit.has_value());
    auto const image = FederationStateImageCodec::decode(commit->stateImage);
    REQUIRE(image.queuedReceiveOrderAttributeUpdates.size() == 1U);
    pushDeliveryId =
        image.queuedReceiveOrderAttributeUpdates.front().pushDeliveryId;
    REQUIRE(pushDeliveryId != 0U);
    CHECK(image.queuedReceiveOrderAttributeUpdates.front().receivingFederateId ==
          receiverId);
    CHECK(image.queuedReceiveOrderAttributeUpdates.front().values.front().value ==
          std::string(expectedValue.begin(), expectedValue.end()));
  }

  {
    ProcessServiceInstance restartedService(saveStore);
    ProcessFederationClient sender(
        {"127.0.0.1", restartedService.port()},
        {"push-ro-restart-client-a2", 0xD5U});
    ProcessFederationClient receiver(
        {"127.0.0.1", restartedService.port()},
        {"push-ro-restart-client-b2", 0xD6U});
    ReflectionCounter callbacks;
    receiver.attachCallbackBridge(callbacks, CallbackDispatchModel::evoked);
    REQUIRE_NOTHROW(sender.createFederationExecution(federation));
    auto const senderJoin = sender.joinFederationExecution(
        federation, std::wstring(senderName), std::wstring(senderName));
    auto const receiverJoin = receiver.joinFederationExecution(
        federation, std::wstring(receiverName), std::wstring(receiverName));
    REQUIRE(senderJoin.federateId == senderId);
    REQUIRE(receiverJoin.federateId == receiverId);

    auto const objectClass = sender.lookupObjectClassHandle(
        federation, senderId, L"HLAobjectRoot.Employee");
    REQUIRE(objectClass.has_value());
    auto const attribute = sender.lookupAttributeHandle(
        federation, senderId, *objectClass, L"Name");
    REQUIRE(attribute.has_value());
    REQUIRE_NOTHROW(sender.publishObjectClassAttributes(
        federation, senderId, *objectClass, {*attribute}));
    REQUIRE_NOTHROW(receiver.subscribeObjectClassAttributes(
        federation, receiverId, *objectClass, {*attribute}, true, ""));

    REQUIRE(sender.requestFederationRestore(federation, senderId, saveLabel).status ==
            umbra::detail::FederationRestoreControlStatus::applied);
    REQUIRE(sender.federateRestoreComplete(federation, senderId).status ==
            umbra::detail::FederationRestoreControlStatus::applied);
    REQUIRE(receiver.federateRestoreComplete(federation, receiverId).status ==
            umbra::detail::FederationRestoreControlStatus::applied);

    for (std::size_t attempt = 0U; attempt != 200U && callbacks.count == 0U;
         ++attempt) {
      static_cast<void>(receiver.evokeOne(std::chrono::milliseconds{2}));
    }
    while (receiver.evokeOne(std::chrono::milliseconds{0})) {
    }
    REQUIRE(callbacks.count == 1U);
    REQUIRE(callbacks.value.size() == expectedValue.size());
    auto const* delivered =
        static_cast<std::uint8_t const*>(callbacks.value.data());
    REQUIRE(std::vector<std::uint8_t>(delivered, delivered + callbacks.value.size()) ==
            expectedValue);
    REQUIRE(receiver.pendingCallbackCount() == 0U);

    for (std::size_t attempt = 0U;
         attempt != 100U && restartedService.acknowledgedPushIds().empty();
         ++attempt) {
      std::this_thread::sleep_for(std::chrono::milliseconds{2});
    }
    auto const acknowledged = restartedService.acknowledgedPushIds();
    REQUIRE(acknowledged == std::vector<std::uint64_t>{pushDeliveryId});
  }
}
