#include <catch2/catch_test_macros.hpp>

#include "internal/federation/process_federation_callback_bridge.hpp"

#include <RTI/NullFederateAmbassador.h>

#include <atomic>
#include <chrono>
#include <memory>
#include <optional>
#include <set>
#include <string>

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
