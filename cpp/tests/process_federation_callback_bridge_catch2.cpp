#include <catch2/catch_test_macros.hpp>

#include "internal/federation/process_federation_callback_bridge.hpp"

#include <RTI/NullFederateAmbassador.h>
#include <RTI/time/HLAinteger64Time.h>

#include <atomic>
#include <chrono>
#include <cstdint>
#include <future>
#include <memory>
#include <optional>
#include <set>
#include <string>
#include <thread>
#include <vector>

TEST_CASE(
    "Private process exception-report projections cancel queued work and drain client lifetime before close",
    "[unit][internal][callbacks][process-boundary][process-exception-report-projection-lifetime]") {
  // Internal concurrency evidence only: the synthetic event and projection
  // gate do not claim a normative MOM report mapping.
  class RecordingFederateAmbassador final
      : public rti1516_2025::NullFederateAmbassador {
   public:
    void receiveInteraction(
        rti1516_2025::InteractionClassHandle const&,
        rti1516_2025::ParameterHandleValueMap const&,
        rti1516_2025::VariableLengthData const&,
        rti1516_2025::TransportationTypeHandle const&,
        rti1516_2025::FederateHandle const&,
        rti1516_2025::RegionHandleSet const*) override {
      ++callbacks;
      if (onReceive) {
        onReceive();
      }
    }

    std::size_t callbacks = 0U;
    std::function<void()> onReceive;
  } recipient;

  using Event = umbra::detail::ProcessFederationInteractionEvent;
  auto event = Event{};
  event.receivingFederateId = 0x21U;
  event.interactionClassHandle = 0x22U;
  event.transportationName = "HLAreliable";
  event.rtiOwnedMomInteraction = true;
  event.exceptionReportFederateId = 0x23U;
  auto dispatcher = std::make_shared<umbra::detail::CallbackDispatcher>(
      umbra::detail::CallbackDispatchModel::evoked);
  auto session =
      std::make_shared<rti1516_2025::umbra_binding_detail::CallbackSession>(
          recipient);
  umbra::detail::ProcessFederationCallbackBridge bridge(dispatcher, session);

  SECTION("resignation cancels queued generation while allowing later membership") {
    std::size_t projections = 0U;
    bridge.setExceptionReportProjectionHandler(
        [&projections](Event incoming) -> std::optional<Event> {
          ++projections;
          return incoming;
        });
    bridge.submitReceiveOrder(event);
    bridge.cancelExceptionReportProjections();
    REQUIRE_FALSE(bridge.evokeOne(std::chrono::milliseconds{0}));
    REQUIRE(projections == 0U);
    REQUIRE(recipient.callbacks == 0U);

    bridge.submitReceiveOrder(event);
    REQUIRE_FALSE(bridge.evokeOne(std::chrono::milliseconds{0}));
    REQUIRE(projections == 1U);
    REQUIRE(recipient.callbacks == 1U);
  }

  SECTION("close drains in-flight projection and suppresses its result") {
    std::promise<void> entered;
    auto projectionEntered = entered.get_future();
    std::promise<void> release;
    auto releaseProjection = release.get_future().share();
    std::atomic_bool ownerAlive{true};
    bool projectionKeptOwnerAlive = false;
    bridge.setExceptionReportProjectionHandler(
        [&](Event incoming) -> std::optional<Event> {
          entered.set_value();
          releaseProjection.wait();
          projectionKeptOwnerAlive = ownerAlive.load(std::memory_order_acquire);
          return incoming;
        });
    bridge.submitReceiveOrder(event);
    bridge.submitReceiveOrder(event);
    auto evoker = std::async(std::launch::async, [&] {
      return bridge.evokeOne(std::chrono::milliseconds{0});
    });
    auto const enteredStatus = projectionEntered.wait_for(std::chrono::seconds{2});
    if (enteredStatus != std::future_status::ready) {
      release.set_value();
      REQUIRE(enteredStatus == std::future_status::ready);
    }
    auto closer = std::async(std::launch::async, [&] {
      bridge.close();
      ownerAlive.store(false, std::memory_order_release);
    });

    // A second queued event keeps pendingCount nonzero until close publishes
    // cancellation. Observe that gate, not a scheduling delay, before proving
    // teardown is blocked on the projection's borrowed-client lease.
    auto const deadline = std::chrono::steady_clock::now() + std::chrono::seconds{2};
    while (bridge.pendingCount() != 0U &&
           std::chrono::steady_clock::now() < deadline) {
      std::this_thread::yield();
    }
    auto const cancellationPublished = bridge.pendingCount() == 0U;
    auto const waitingForProjection =
        closer.wait_for(std::chrono::milliseconds{0}) == std::future_status::timeout;
    release.set_value();
    auto const evokerStatus = evoker.wait_for(std::chrono::seconds{2});
    auto const closerStatus = closer.wait_for(std::chrono::seconds{2});
    REQUIRE(cancellationPublished);
    REQUIRE(waitingForProjection);
    REQUIRE(evokerStatus == std::future_status::ready);
    REQUIRE(closerStatus == std::future_status::ready);
    evoker.get();
    closer.get();
    REQUIRE(projectionKeptOwnerAlive);
    REQUIRE(recipient.callbacks == 0U);
    REQUIRE(dispatcher->pendingCount() == 0U);
  }

  SECTION("resignation cancels a projected report still waiting for session entry") {
    std::promise<void> sessionEntered;
    auto sessionHasEntered = sessionEntered.get_future();
    std::promise<void> releaseSession;
    auto releaseSessionGate = releaseSession.get_future().share();
    auto sessionBlocker = std::async(std::launch::async, [&] {
      session->invoke([&](rti1516_2025::FederateAmbassador&) {
        sessionEntered.set_value();
        releaseSessionGate.wait();
      });
    });
    auto const sessionStatus = sessionHasEntered.wait_for(std::chrono::seconds{2});
    if (sessionStatus != std::future_status::ready) {
      releaseSession.set_value();
      REQUIRE(sessionStatus == std::future_status::ready);
    }
    std::promise<void> projected;
    auto reportHasProjected = projected.get_future();
    bridge.setExceptionReportProjectionHandler(
        [&](Event incoming) -> std::optional<Event> {
          projected.set_value();
          return incoming;
        });
    bridge.submitReceiveOrder(event);
    auto evoker = std::async(std::launch::async, [&] {
      return bridge.evokeOne(std::chrono::milliseconds{0});
    });
    auto const projectionStatus = reportHasProjected.wait_for(std::chrono::seconds{2});
    if (projectionStatus == std::future_status::ready) {
      // This drains project(), but must not wait for the user callback gate.
      bridge.cancelExceptionReportProjections();
    }
    releaseSession.set_value();
    auto const blockerStatus = sessionBlocker.wait_for(std::chrono::seconds{2});
    auto const evokerStatus = evoker.wait_for(std::chrono::seconds{2});
    REQUIRE(projectionStatus == std::future_status::ready);
    REQUIRE(blockerStatus == std::future_status::ready);
    REQUIRE(evokerStatus == std::future_status::ready);
    sessionBlocker.get();
    evoker.get();
    REQUIRE(recipient.callbacks == 0U);
  }

  SECTION("user callback can close after projection retires") {
    bridge.setExceptionReportProjectionHandler(
        [](Event incoming) -> std::optional<Event> { return incoming; });
    recipient.onReceive = [&] { bridge.close(); };
    bridge.submitReceiveOrder(event);
    REQUIRE_FALSE(bridge.evokeOne(std::chrono::milliseconds{0}));
    REQUIRE(recipient.callbacks == 1U);
    REQUIRE(bridge.pendingCount() == 0U);
  }
}

TEST_CASE(
    "Private process callback bridge suppresses a queued Attribute Relevance Advisory after switch disable",
    "[unit][foundation][object-management][data-distribution-management][callbacks][callback-suppression][process-boundary][2025][rti.service.set-attribute-relevance-advisory-switch][federate.callback.turn-updates-on-for-object-instance]") {
  class RecordingFederateAmbassador final
      : public rti1516_2025::NullFederateAmbassador {
   public:
    void turnUpdatesOnForObjectInstance(
        rti1516_2025::ObjectInstanceHandle const& objectInstance,
        rti1516_2025::AttributeHandleSet const& attributes,
        std::wstring const& updateRateDesignator) override {
      invoked = true;
      observedObjectInstance = objectInstance;
      observedAttributeCount = attributes.size();
      observedUpdateRateDesignator = updateRateDesignator;
    }

    bool invoked = false;
    rti1516_2025::ObjectInstanceHandle observedObjectInstance;
    std::size_t observedAttributeCount = 0U;
    std::wstring observedUpdateRateDesignator;
  } recipient;

  auto dispatcher = std::make_shared<umbra::detail::CallbackDispatcher>(
      umbra::detail::CallbackDispatchModel::evoked);
  auto callbackSession =
      std::make_shared<rti1516_2025::umbra_binding_detail::CallbackSession>(
          recipient);
  umbra::detail::ProcessFederationCallbackBridge bridge(
      std::move(dispatcher), std::move(callbackSession));
  auto switchState = std::make_shared<std::atomic_bool>(true);
  bridge.setAttributeRelevanceAdvisorySwitchState(switchState);

  auto makeEvent = [] {
    return umbra::detail::ProcessFederationAttributeRelevanceAdvisoryEvent{
        0xA1U,
        0U,
        0xB2U,
        std::set<std::uint64_t>{0xC3U},
        true,
        std::optional<std::string>{"High"}};
  };

  bridge.submitAttributeRelevanceAdvisory(makeEvent());
  REQUIRE(bridge.pendingCount() == 1U);
  switchState->store(false, std::memory_order_release);
  REQUIRE_FALSE(bridge.evokeOne(std::chrono::milliseconds{0}));
  REQUIRE_FALSE(recipient.invoked);
  REQUIRE(bridge.pendingCount() == 0U);

  switchState->store(true, std::memory_order_release);
  bridge.submitAttributeRelevanceAdvisory(makeEvent());
  REQUIRE_FALSE(bridge.evokeOne(std::chrono::milliseconds{0}));
  REQUIRE(recipient.invoked);
  REQUIRE(recipient.observedObjectInstance.toString() ==
          L"ObjectInstanceHandle(178)");
  REQUIRE(recipient.observedAttributeCount == 1U);
  REQUIRE(recipient.observedUpdateRateDesignator == L"High");
}

TEST_CASE(
    "Private process callback bridge delivers Provide Attribute Value Update with official handles and tag",
    "[unit][foundation][object-management][callbacks][transport][process-boundary][2025][rti.service.request-attribute-value-update][federate.callback.provide-attribute-value-update]") {
  auto runScenario = [](umbra::detail::CallbackDispatchModel model) {
    class RecordingFederateAmbassador final
        : public rti1516_2025::NullFederateAmbassador {
     public:
      void provideAttributeValueUpdate(
          rti1516_2025::ObjectInstanceHandle const& objectInstance,
          rti1516_2025::AttributeHandleSet const& attributes,
          rti1516_2025::VariableLengthData const& userSuppliedTag) override {
        invoked = true;
        observedObjectInstance = objectInstance;
        observedAttributeCount = attributes.size();
        observedTag.clear();
        if (userSuppliedTag.size() != 0U) {
          auto const* first = static_cast<std::uint8_t const*>(
              userSuppliedTag.data());
          observedTag.assign(first, first + userSuppliedTag.size());
        }
      }

      bool invoked = false;
      rti1516_2025::ObjectInstanceHandle observedObjectInstance;
      std::size_t observedAttributeCount = 0U;
      std::vector<std::uint8_t> observedTag;
    } recipient;

    auto dispatcher = std::make_shared<umbra::detail::CallbackDispatcher>(model);
    auto callbackSession =
        std::make_shared<rti1516_2025::umbra_binding_detail::CallbackSession>(
            recipient);
    umbra::detail::ProcessFederationCallbackBridge bridge(
        std::move(dispatcher), std::move(callbackSession));
    umbra::detail::ProcessFederationAttributeValueUpdateRequestEvent event{
        0x21U,
        0x22U,
        0x23U,
        std::set<std::uint64_t>{0x24U, 0x25U},
        std::vector<std::uint8_t>{0x52U, 0x45U, 0x51U}};

    bridge.submitAttributeValueUpdateRequest(event);
    if (model == umbra::detail::CallbackDispatchModel::evoked) {
      REQUIRE(bridge.pendingCount() == 1U);
      REQUIRE_FALSE(recipient.invoked);
      REQUIRE_FALSE(bridge.evokeOne(std::chrono::milliseconds{0}));
    } else {
      REQUIRE(bridge.pendingCount() == 0U);
    }
    REQUIRE(recipient.invoked);
    REQUIRE(recipient.observedObjectInstance.toString() ==
            L"ObjectInstanceHandle(35)");
    REQUIRE(recipient.observedAttributeCount == 2U);
    REQUIRE(recipient.observedTag ==
            std::vector<std::uint8_t>{0x52U, 0x45U, 0x51U});

    std::uint64_t acknowledgedMessageId = 0U;
    bridge.setTsoDeliveryCompletionHandler(
        [&acknowledgedMessageId](std::uint64_t messageId) {
          acknowledgedMessageId = messageId;
        });
    auto const timestamp = rti1516_2025::HLAinteger64Time(5);
    auto const encodedTimestamp = timestamp.encode();
    std::vector<std::uint8_t> timestampBytes;
    if (encodedTimestamp.size() != 0U) {
      auto const* data = static_cast<std::uint8_t const*>(encodedTimestamp.data());
      REQUIRE(data != nullptr);
      timestampBytes.assign(data, data + encodedTimestamp.size());
    }
    umbra::detail::ProcessFederationAttributeUpdateEvent tsoEvent;
    tsoEvent.producingFederateId = 0x31U;
    tsoEvent.receivingFederateId = 0x32U;
    tsoEvent.objectInstanceHandle = 0x33U;
    tsoEvent.attributeValues.emplace_back(
        0x34U, std::vector<std::uint8_t>{0x35U});
    // The 2025 binding exposes the two mandatory transportation names only;
    // HLAdefault is an update-rate designator, not a transportation type.
    tsoEvent.transportationName = "HLAreliable";
    tsoEvent.timestamp = umbra::detail::ProcessFederationLogicalTime{
        L"HLAinteger64Time", std::move(timestampBytes)};
    tsoEvent.retractionMessageId = 0x36U;
    bridge.submitAttributeUpdate(std::move(tsoEvent));
    if (model == umbra::detail::CallbackDispatchModel::evoked) {
      REQUIRE(bridge.pendingCount() == 1U);
      REQUIRE(acknowledgedMessageId == 0U);
      REQUIRE_FALSE(bridge.evokeOne(std::chrono::milliseconds{0}));
    }
    REQUIRE(acknowledgedMessageId == 0x36U);
  };

  SECTION("HLA_EVOKED") {
    runScenario(umbra::detail::CallbackDispatchModel::evoked);
  }
  SECTION("HLA_IMMEDIATE") {
    runScenario(umbra::detail::CallbackDispatchModel::immediate);
  }
}
