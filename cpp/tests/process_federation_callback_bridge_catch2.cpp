#include <catch2/catch_test_macros.hpp>

#include "internal/federation/process_federation_callback_bridge.hpp"

#include <RTI/NullFederateAmbassador.h>
#include <RTI/time/HLAinteger64Time.h>

#include <atomic>
#include <chrono>
#include <cstdint>
#include <memory>
#include <optional>
#include <set>
#include <string>
#include <vector>

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
