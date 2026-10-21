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

// Internal callback/lease-boundary evidence only; this synthetic bridge case
// has no direct normative requirement mapping.
TEST_CASE(
    "Private process callback bridge acknowledges grouped timestamped attribute passels after the final callback",
    "[unit][internal][callbacks][process-boundary][2025][time-management][ddm][tso][ddm-process-tso-region-callback-snapshot][attribute-callback-group-boundary]") {
  auto runScenario = [](
                       umbra::detail::CallbackDispatchModel model,
                       bool requestRetractionBeforeFinal = false) {
    class RecordingFederateAmbassador final
        : public rti1516_2025::NullFederateAmbassador {
     public:
      void reflectAttributeValues(
          rti1516_2025::ObjectInstanceHandle const&,
          rti1516_2025::AttributeHandleValueMap const&,
          rti1516_2025::VariableLengthData const&,
          rti1516_2025::TransportationTypeHandle const&,
          rti1516_2025::FederateHandle const&,
          rti1516_2025::RegionHandleSet const* regions,
          rti1516_2025::LogicalTime const&,
          rti1516_2025::OrderType,
          rti1516_2025::OrderType,
          rti1516_2025::MessageRetractionHandle const*) override {
        ++callbacks;
        observedRegionCounts.push_back(regions == nullptr ? 0U : regions->size());
      }

      std::size_t callbacks = 0U;
      std::vector<std::size_t> observedRegionCounts;
    } recipient;

    auto dispatcher = std::make_shared<umbra::detail::CallbackDispatcher>(model);
    auto callbackSession =
        std::make_shared<rti1516_2025::umbra_binding_detail::CallbackSession>(
            recipient);
    umbra::detail::ProcessFederationCallbackBridge bridge(
        std::move(dispatcher), std::move(callbackSession));
    std::size_t acknowledgementCount = 0U;
    std::uint64_t acknowledgedMessageId = 0U;
    bridge.setTsoDeliveryCompletionHandler(
        [&](std::uint64_t messageId) {
          ++acknowledgementCount;
          acknowledgedMessageId = messageId;
        });

    auto const encodedTimestamp = rti1516_2025::HLAinteger64Time(5).encode();
    std::vector<std::uint8_t> timestampBytes;
    if (encodedTimestamp.size() != 0U) {
      auto const* data = static_cast<std::uint8_t const*>(encodedTimestamp.data());
      REQUIRE(data != nullptr);
      timestampBytes.assign(data, data + encodedTimestamp.size());
    }
    auto makeEvent = [&](auto position) {
      umbra::detail::ProcessFederationAttributeUpdateEvent event;
      event.producingFederateId = 0x31U;
      event.receivingFederateId = 0x32U;
      event.objectInstanceHandle = 0x33U;
      event.attributeValues.emplace_back(
          0x34U, std::vector<std::uint8_t>{0x35U});
      event.transportationName = "HLAreliable";
      event.sentRegionHandles = std::set<std::uint64_t>{0x37U};
      event.timestamp = umbra::detail::ProcessFederationLogicalTime{
          L"HLAinteger64Time", timestampBytes};
      event.retractionMessageId = 0x36U;
      event.tsoCallbackGroupPosition = position;
      return event;
    };

    using GroupPosition =
        umbra::detail::ProcessFederationTsoAttributeUpdateCallbackGroupPosition;
    bridge.submitAttributeUpdate(makeEvent(GroupPosition::more));
    REQUIRE(acknowledgementCount == 0U);
    if (requestRetractionBeforeFinal) {
      bridge.submitRequestRetraction(0x36U);
      REQUIRE(acknowledgementCount == 0U);
    }
    bridge.submitAttributeUpdate(makeEvent(GroupPosition::final));

    if (model == umbra::detail::CallbackDispatchModel::evoked) {
      REQUIRE(acknowledgedMessageId == 0U);
      REQUIRE(bridge.pendingCount() == 2U);
      static_cast<void>(bridge.evokeOne(std::chrono::milliseconds{0}));
      REQUIRE(recipient.callbacks == (requestRetractionBeforeFinal ? 0U : 1U));
      REQUIRE(acknowledgementCount == 0U);
      static_cast<void>(bridge.evokeOne(std::chrono::milliseconds{0}));
    } else {
      if (requestRetractionBeforeFinal) {
        REQUIRE(recipient.callbacks == 1U);
        REQUIRE(acknowledgementCount == 0U);
      } else {
        REQUIRE(recipient.callbacks == 2U);
        REQUIRE(acknowledgementCount == 1U);
        REQUIRE(acknowledgedMessageId == 0x36U);
      }
    }

    auto const expectedCallbackCount = requestRetractionBeforeFinal
        ? (model == umbra::detail::CallbackDispatchModel::evoked ? 0U : 1U)
        : 2U;
    REQUIRE(recipient.callbacks == expectedCallbackCount);
    if (!requestRetractionBeforeFinal) {
      REQUIRE(recipient.observedRegionCounts ==
              std::vector<std::size_t>{1U, 1U});
    }
    REQUIRE(acknowledgementCount == 1U);
    REQUIRE(acknowledgedMessageId == 0x36U);
  };

  SECTION("HLA_EVOKED") {
    runScenario(umbra::detail::CallbackDispatchModel::evoked);
  }
  SECTION("HLA_IMMEDIATE") {
    runScenario(umbra::detail::CallbackDispatchModel::immediate);
  }
  SECTION("HLA_EVOKED request retraction before the final passel") {
    runScenario(umbra::detail::CallbackDispatchModel::evoked, true);
  }
}

// Internal callback-queue evidence only. This exercises the callback-boundary
// conversion mechanism and is not an independent normative requirement claim.
TEST_CASE(
    "Private process callback bridge converts queued TSO interactions to receive order after Disable Time Constrained",
    "[unit][internal][internal-only][no-normative-mapping][callbacks]"
    "[process-boundary][2025][time-constrained-disable-tso-to-ro]"
    "[lane.time-constrained-disable-tso-to-ro][tcd-pending-tso-conversion]") {
  struct CallbackReport final {
    bool directed = false;
    bool timestamped = false;
    std::wstring interactionClass;
    std::wstring objectInstance;
    std::wstring producingFederate;
    std::wstring transportationType;
    std::vector<std::uint8_t> parameterValue;
    std::vector<std::uint8_t> tag;
    std::vector<std::wstring> regions;
  };

  class RecordingFederateAmbassador final
      : public rti1516_2025::NullFederateAmbassador {
   public:
    static std::vector<std::uint8_t> copyBytes(
        rti1516_2025::VariableLengthData const& value) {
      if (value.size() == 0U) {
        return {};
      }
      auto const* begin =
          static_cast<std::uint8_t const*>(value.data());
      return {begin, begin + value.size()};
    }

    void record(
        bool directed,
        bool timestamped,
        rti1516_2025::InteractionClassHandle const& interactionClass,
        rti1516_2025::ObjectInstanceHandle const* objectInstance,
        rti1516_2025::ParameterHandleValueMap const& parameterValues,
        rti1516_2025::VariableLengthData const& userSuppliedTag,
        rti1516_2025::TransportationTypeHandle const& transportationType,
        rti1516_2025::FederateHandle const& producingFederate,
        rti1516_2025::RegionHandleSet const* regions) {
      CallbackReport report;
      report.directed = directed;
      report.timestamped = timestamped;
      report.interactionClass = interactionClass.toString();
      if (objectInstance != nullptr) {
        report.objectInstance = objectInstance->toString();
      }
      report.producingFederate = producingFederate.toString();
      report.transportationType = transportationType.toString();
      if (!parameterValues.empty()) {
        report.parameterValue = copyBytes(parameterValues.begin()->second);
      }
      report.tag = copyBytes(userSuppliedTag);
      if (regions != nullptr) {
        for (auto const& region : *regions) {
          report.regions.push_back(region.toString());
        }
      }
      reports.push_back(std::move(report));
    }

    void receiveInteraction(
        rti1516_2025::InteractionClassHandle const& interactionClass,
        rti1516_2025::ParameterHandleValueMap const& parameterValues,
        rti1516_2025::VariableLengthData const& userSuppliedTag,
        rti1516_2025::TransportationTypeHandle const& transportationType,
        rti1516_2025::FederateHandle const& producingFederate,
        rti1516_2025::RegionHandleSet const* regions) override {
      record(
          false, false, interactionClass, nullptr, parameterValues,
          userSuppliedTag, transportationType, producingFederate, regions);
    }

    void receiveInteraction(
        rti1516_2025::InteractionClassHandle const& interactionClass,
        rti1516_2025::ParameterHandleValueMap const& parameterValues,
        rti1516_2025::VariableLengthData const& userSuppliedTag,
        rti1516_2025::TransportationTypeHandle const& transportationType,
        rti1516_2025::FederateHandle const& producingFederate,
        rti1516_2025::RegionHandleSet const* regions,
        rti1516_2025::LogicalTime const&,
        rti1516_2025::OrderType,
        rti1516_2025::OrderType,
        rti1516_2025::MessageRetractionHandle const*) override {
      record(
          false, true, interactionClass, nullptr, parameterValues,
          userSuppliedTag, transportationType, producingFederate, regions);
    }

    void receiveDirectedInteraction(
        rti1516_2025::InteractionClassHandle const& interactionClass,
        rti1516_2025::ObjectInstanceHandle const& objectInstance,
        rti1516_2025::ParameterHandleValueMap const& parameterValues,
        rti1516_2025::VariableLengthData const& userSuppliedTag,
        rti1516_2025::TransportationTypeHandle const& transportationType,
        rti1516_2025::FederateHandle const& producingFederate) override {
      record(
          true, false, interactionClass, &objectInstance, parameterValues,
          userSuppliedTag, transportationType, producingFederate, nullptr);
    }

    void receiveDirectedInteraction(
        rti1516_2025::InteractionClassHandle const& interactionClass,
        rti1516_2025::ObjectInstanceHandle const& objectInstance,
        rti1516_2025::ParameterHandleValueMap const& parameterValues,
        rti1516_2025::VariableLengthData const& userSuppliedTag,
        rti1516_2025::TransportationTypeHandle const& transportationType,
        rti1516_2025::FederateHandle const& producingFederate,
        rti1516_2025::LogicalTime const&,
        rti1516_2025::OrderType,
        rti1516_2025::OrderType,
        rti1516_2025::MessageRetractionHandle const*) override {
      record(
          true, true, interactionClass, &objectInstance, parameterValues,
          userSuppliedTag, transportationType, producingFederate, nullptr);
    }

    std::vector<CallbackReport> reports;
  } recipient;

  auto dispatcher = std::make_shared<umbra::detail::CallbackDispatcher>(
      umbra::detail::CallbackDispatchModel::evoked);
  auto callbackSession =
      std::make_shared<rti1516_2025::umbra_binding_detail::CallbackSession>(
          recipient);
  umbra::detail::ProcessFederationCallbackBridge bridge(
      std::move(dispatcher), std::move(callbackSession));
  bridge.setTimeConstrainedEnabled(true);

  std::vector<std::uint64_t> acknowledgedMessageIds;
  bridge.setTsoDeliveryCompletionHandler(
      [&acknowledgedMessageIds](std::uint64_t messageId) {
        acknowledgedMessageIds.push_back(messageId);
      });
  std::vector<std::uint64_t> acceptedMessageIds;
  bridge.setTsoCallbackAcceptedHandler(
      [&acceptedMessageIds](std::uint64_t messageId) {
        acceptedMessageIds.push_back(messageId);
      });

  auto makeEvent = [](std::uint8_t marker, bool timestamped, bool directed) {
    umbra::detail::ProcessFederationInteractionEvent event;
    event.producingFederateId = 0x51U;
    event.receivingFederateId = 0x52U;
    event.interactionClassHandle =
        static_cast<std::uint64_t>(0x40U + marker);
    event.parameterHandles = {0x53U};
    event.transportationName = "HLAreliable";
    if (directed) {
      event.objectInstanceHandle = 0x61U;
    } else {
      event.sentRegionHandles = std::set<std::uint64_t>{0x62U};
    }
    umbra::detail::ProcessFederationInteractionEnvelope envelope;
    envelope.parameterValues.emplace_back(
        0x53U, std::vector<std::uint8_t>{marker, 0xEEU});
    envelope.userSuppliedTag = {marker, 0xF0U};
    event.payload = umbra::detail::encodeProcessFederationInteractionEnvelope(
        envelope);
    if (timestamped) {
      auto const encodedTimestamp = rti1516_2025::HLAinteger64Time(5).encode();
      std::vector<std::uint8_t> timestampBytes;
      if (encodedTimestamp.size() != 0U) {
        auto const* begin =
            static_cast<std::uint8_t const*>(encodedTimestamp.data());
        timestampBytes.assign(begin, begin + encodedTimestamp.size());
      }
      event.timestamp = umbra::detail::ProcessFederationLogicalTime{
          L"HLAinteger64Time", std::move(timestampBytes)};
      event.retractionMessageId = 0x70U + marker;
      event.sentOrderType = rti1516_2025::TIMESTAMP;
      event.receivedOrderType = rti1516_2025::TIMESTAMP;
    }
    return event;
  };

  bridge.submitReceiveOrder(makeEvent(0x01U, true, false));
  bridge.submitReceiveOrder(makeEvent(0x02U, false, false));
  bridge.submitReceiveOrder(makeEvent(0x03U, true, true));
  REQUIRE(bridge.pendingCount() == 3U);
  REQUIRE(acceptedMessageIds ==
          std::vector<std::uint64_t>{0x71U, 0x73U});
  REQUIRE(recipient.reports.empty());

  bridge.setTimeConstrainedEnabled(false);

  for (std::size_t expectedCount = 1U; expectedCount <= 3U; ++expectedCount) {
    static_cast<void>(bridge.evokeOne(std::chrono::milliseconds{0}));
    REQUIRE(recipient.reports.size() == expectedCount);
  }
  REQUIRE(recipient.reports.size() == 3U);
  REQUIRE(acknowledgedMessageIds == std::vector<std::uint64_t>{0x71U, 0x73U});

  std::vector<std::uint8_t> observedMarkers;
  for (auto const& report : recipient.reports) {
    REQUIRE_FALSE(report.timestamped);
    REQUIRE(report.parameterValue.size() == 2U);
    observedMarkers.push_back(report.parameterValue.front());
    REQUIRE(report.tag.size() == 2U);
    REQUIRE(report.tag.front() == report.parameterValue.front());
    REQUIRE(report.producingFederate == L"FederateHandle(81)");
    REQUIRE(report.transportationType == L"TransportationTypeHandle(1)");
  }
  REQUIRE(observedMarkers == std::vector<std::uint8_t>{0x01U, 0x02U, 0x03U});
  REQUIRE(recipient.reports[0].interactionClass == L"InteractionClassHandle(65)");
  REQUIRE(recipient.reports[0].regions ==
          std::vector<std::wstring>{L"RegionHandle(98)"});
  REQUIRE_FALSE(recipient.reports[0].directed);
  REQUIRE(recipient.reports[1].interactionClass == L"InteractionClassHandle(66)");
  REQUIRE(recipient.reports[1].regions ==
          std::vector<std::wstring>{L"RegionHandle(98)"});
  REQUIRE_FALSE(recipient.reports[1].directed);
  REQUIRE(recipient.reports[2].interactionClass == L"InteractionClassHandle(67)");
  REQUIRE(recipient.reports[2].directed);
  REQUIRE(recipient.reports[2].objectInstance == L"ObjectInstanceHandle(97)");
}
