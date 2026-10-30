#include <catch2/catch_test_macros.hpp>

#if defined(UMBRA_ENABLE_EMBEDDED_FEDERATION_MANAGEMENT)

#include <algorithm>
#include <array>
#include <chrono>
#include <condition_variable>
#include <cstdint>
#include <mutex>
#include <string>
#include <utility>
#include <vector>

#include <RTI/NullFederateAmbassador.h>
#include <RTI/RTI1516.h>
#include <RTI/time/HLAinteger64Interval.h>
#include <RTI/time/HLAinteger64Time.h>

#include "ieee1516_2025_process_fom_test_support.hpp"
#include "internal/federation/process_federation_service_host.hpp"
#include "internal/handles/federate_handle.hpp"
#include "internal/handles/message_retraction_handle.hpp"

namespace {

using namespace rti1516_2025;

std::vector<std::uint8_t> bytesOf(VariableLengthData const& data) {
  if (data.size() == 0U) {
    return {};
  }
  auto const* begin = static_cast<std::uint8_t const*>(data.data());
  return {begin, begin + data.size()};
}

struct InteractionReport final {
  InteractionClassHandle interaction;
  ParameterHandle parameter;
  std::size_t parameterCount = 0U;
  std::vector<std::uint8_t> value;
  std::vector<std::uint8_t> tag;
  FederateHandle producer;
  bool sentRegionsPresent = false;
  std::size_t sentRegionCount = 0U;
  std::int64_t time = -1;
  OrderType sentOrder = RECEIVE;
  OrderType receivedOrder = TIMESTAMP;
  bool hasRetraction = false;
  MessageRetractionHandle retraction;
};

class RecordingFederate final : public NullFederateAmbassador {
 public:
  void timeRegulationEnabled(LogicalTime const&) override {
    {
      std::scoped_lock lock(mutex_);
      ++regulationEnabled_;
    }
    callbackChanged_.notify_all();
  }

  void receiveInteraction(
      InteractionClassHandle const& interaction,
      ParameterHandleValueMap const& parameters,
      VariableLengthData const& tag,
      TransportationTypeHandle const&,
      FederateHandle const& producer,
      RegionHandleSet const* sentRegions,
      LogicalTime const& time,
      OrderType sentOrder,
      OrderType receivedOrder,
      MessageRetractionHandle const* retraction) override {
    InteractionReport report;
    report.interaction = interaction;
    report.parameterCount = parameters.size();
    if (!parameters.empty()) {
      report.parameter = parameters.begin()->first;
      report.value = bytesOf(parameters.begin()->second);
    }
    report.tag = bytesOf(tag);
    report.producer = producer;
    report.sentRegionsPresent = sentRegions != nullptr;
    report.sentRegionCount = sentRegions == nullptr ? 0U : sentRegions->size();
    if (auto const* integerTime = dynamic_cast<HLAinteger64Time const*>(&time)) {
      report.time = integerTime->getTime();
    }
    report.sentOrder = sentOrder;
    report.receivedOrder = receivedOrder;
    report.hasRetraction = retraction != nullptr;
    if (retraction != nullptr) {
      report.retraction = *retraction;
    }
    {
      std::scoped_lock lock(mutex_);
      interactions_.push_back(std::move(report));
    }
    callbackChanged_.notify_all();
  }

  void requestRetraction(MessageRetractionHandle const& retraction) override {
    {
      std::scoped_lock lock(mutex_);
      retractions_.push_back(retraction);
    }
    callbackChanged_.notify_all();
  }

  [[nodiscard]] unsigned regulationEnabledCount() const {
    std::scoped_lock lock(mutex_);
    return regulationEnabled_;
  }

  [[nodiscard]] std::vector<InteractionReport> interactionReports() const {
    std::scoped_lock lock(mutex_);
    return interactions_;
  }

  [[nodiscard]] std::vector<MessageRetractionHandle> retractionReports() const {
    std::scoped_lock lock(mutex_);
    return retractions_;
  }

  void waitForCallbackChange(std::chrono::milliseconds timeout) {
    std::unique_lock lock(mutex_);
    callbackChanged_.wait_for(lock, timeout);
  }

 private:
  mutable std::mutex mutex_;
  std::condition_variable callbackChanged_;
  unsigned regulationEnabled_ = 0U;
  std::vector<InteractionReport> interactions_;
  std::vector<MessageRetractionHandle> retractions_;
};

template <typename Predicate>
void callbackUntil(
    RTIambassador& rti,
    CallbackModel callbackModel,
    RecordingFederate& reports,
    Predicate ready) {
  for (unsigned attempt = 0U; attempt != 40U && !ready(); ++attempt) {
    if (callbackModel == HLA_EVOKED) {
      static_cast<void>(rti.evokeCallback(0.05));
    } else {
      reports.waitForCallbackChange(std::chrono::milliseconds(50));
    }
  }
  REQUIRE(ready());
}

void drainCallbacks(
    RTIambassador& rti,
    CallbackModel callbackModel,
    RecordingFederate& reports) {
  for (unsigned attempt = 0U; attempt != 4U; ++attempt) {
    if (callbackModel == HLA_EVOKED) {
      static_cast<void>(rti.evokeCallback(0.02));
    } else {
      reports.waitForCallbackChange(std::chrono::milliseconds(20));
    }
  }
}

void verifyPushedRegionalTsoRoRetraction(
    bool inspectDeliveryLedger,
    CallbackModel callbackModel) {
  umbra::detail::EmbeddedFederationRegistry registry;
  umbra::detail::ProcessFederationServiceOptions options;
  options.pushReceiveOrderEvents = true;
  umbra::detail::ProcessFederationServiceHost authority(
      registry,
      umbra::test::process_fom_support_2025::composedProcessDefinition(),
      {"127.0.0.1", 0U},
      {"regional-tso-ro-push", 0xA97BU},
      options);
  authority.start();
  REQUIRE(authority.address().port != 0U);

  RecordingFederate senderReports;
  RecordingFederate receiverReports;
  RTIambassadorFactory factory;
  auto sender = factory.createRTIambassador();
  auto receiver = factory.createRTIambassador();
  auto const configuration = RtiConfiguration::createConfiguration()
      .withRtiAddress(L"tcp://127.0.0.1:" +
                      std::to_wstring(authority.address().port));
  constexpr wchar_t const* federation = L"regional-tso-ro-push-retraction";
  constexpr wchar_t const* className =
      L"HLAinteractionRoot.CustomerTransactions.FoodServed.MainCourseServed";
  constexpr wchar_t const* parameterName = L"TimelinessOk";
  constexpr wchar_t const* dimensionName = L"ServerId";

  REQUIRE(sender->connect(senderReports, callbackModel, configuration).addressUsed);
  REQUIRE_NOTHROW(sender->createFederationExecution(federation, L"server-owned-fom.xml"));
  auto const senderHandle = sender->joinFederationExecution(
      L"regional-push-sender", L"publisher", federation);
  REQUIRE(senderHandle.isValid());
  REQUIRE(receiver->connect(receiverReports, callbackModel, configuration).addressUsed);
  auto const receiverHandle = receiver->joinFederationExecution(
      L"regional-push-receiver", L"subscriber", federation);
  REQUIRE(receiverHandle.isValid());

  auto const senderClass = sender->getInteractionClassHandle(className);
  auto const receiverClass = receiver->getInteractionClassHandle(className);
  auto const senderParameter = sender->getParameterHandle(senderClass, parameterName);
  auto const receiverParameter = receiver->getParameterHandle(receiverClass, parameterName);
  auto const senderDimension = sender->getDimensionHandle(dimensionName);
  auto const receiverDimension = receiver->getDimensionHandle(dimensionName);
  REQUIRE(senderClass == receiverClass);
  REQUIRE(senderParameter == receiverParameter);
  auto const sourceRegion = sender->createRegion(DimensionHandleSet{senderDimension});
  auto const subscriptionRegion = receiver->createRegion(DimensionHandleSet{receiverDimension});
  REQUIRE_NOTHROW(sender->setRangeBounds(sourceRegion, senderDimension, RangeBounds(0UL, 5UL)));
  REQUIRE_NOTHROW(sender->commitRegionModifications(RegionHandleSet{sourceRegion}));
  REQUIRE_NOTHROW(receiver->setRangeBounds(subscriptionRegion, receiverDimension, RangeBounds(0UL, 5UL)));
  REQUIRE_NOTHROW(receiver->commitRegionModifications(RegionHandleSet{subscriptionRegion}));
  REQUIRE_NOTHROW(sender->publishInteractionClass(senderClass));
  REQUIRE_NOTHROW(sender->changeInteractionOrderType(senderClass, TIMESTAMP));
  REQUIRE_NOTHROW(receiver->setConveyRegionDesignatorSetsSwitch(true));
  REQUIRE_NOTHROW(receiver->subscribeInteractionClassWithRegions(
      receiverClass, RegionHandleSet{subscriptionRegion}, true));
  REQUIRE_NOTHROW(sender->enableTimeRegulation(HLAinteger64Interval(1)));
  callbackUntil(
      *sender,
      callbackModel,
      senderReports,
      [&] { return senderReports.regulationEnabledCount() == 1U; });

  std::array<std::uint8_t, 2> const valueBytes{0x31U, 0x32U};
  std::array<std::uint8_t, 2> const preTagBytes{0x50U, 0x31U};
  std::array<std::uint8_t, 2> const postTagBytes{0x50U, 0x32U};
  ParameterHandleValueMap parameters;
  parameters.emplace(senderParameter, VariableLengthData(valueBytes.data(), valueBytes.size()));

  REQUIRE_NOTHROW(receiver->disableCallbacks());
  auto const beforeAdmission = sender->sendInteractionWithRegions(
      senderClass, parameters, RegionHandleSet{sourceRegion},
      VariableLengthData(preTagBytes.data(), preTagBytes.size()), HLAinteger64Time(5));
  REQUIRE(beforeAdmission.isValid());
  // The push frame precedes this response on the receiver's connection.
  REQUIRE(receiver->getInteractionClassHandle(className) == receiverClass);
  if (callbackModel == HLA_EVOKED) {
    bool pending = false;
    for (unsigned attempt = 0U; attempt != 40U && !pending; ++attempt) {
      pending = receiver->evokeCallback(0.05);
    }
    REQUIRE(pending);
  } else {
    // The service-call round-trip above is the event fence in immediate mode.
    REQUIRE(receiverReports.interactionReports().empty());
  }
  if (inspectDeliveryLedger) {
    auto const receiverId =
        umbra_binding_detail::federateHandleValue(receiverHandle);
    auto const messageId =
        umbra_binding_detail::messageRetractionHandleValue(beforeAdmission);
    REQUIRE(receiverId.has_value());
    REQUIRE(messageId.has_value());
    // The push has crossed the service boundary, but the disabled callback
    // has not entered the federate ambassador.
    REQUIRE_FALSE(registry.beginTsoInteractionCallback(
        federation, *receiverId, *messageId, true));
  }
  REQUIRE_NOTHROW(sender->retract(beforeAdmission));
  REQUIRE(receiver->getInteractionClassHandle(className) == receiverClass);
  REQUIRE_NOTHROW(receiver->enableCallbacks());
  drainCallbacks(*receiver, callbackModel, receiverReports);
  REQUIRE(receiverReports.interactionReports().empty());

  auto const afterAdmission = sender->sendInteractionWithRegions(
      senderClass, parameters, RegionHandleSet{sourceRegion},
      VariableLengthData(postTagBytes.data(), postTagBytes.size()), HLAinteger64Time(6));
  REQUIRE(afterAdmission.isValid());
  callbackUntil(
      *receiver,
      callbackModel,
      receiverReports,
      [&] { return receiverReports.interactionReports().size() == 1U; });
  auto const interactionReports = receiverReports.interactionReports();
  REQUIRE(interactionReports.size() == 1U);
  auto const& delivered = interactionReports.front();
  REQUIRE(delivered.interaction == receiverClass);
  REQUIRE(delivered.parameterCount == 1U);
  REQUIRE(delivered.parameter == receiverParameter);
  REQUIRE(delivered.value == std::vector<std::uint8_t>{0x31U, 0x32U});
  REQUIRE(delivered.tag == std::vector<std::uint8_t>{0x50U, 0x32U});
  REQUIRE(delivered.producer == senderHandle);
  REQUIRE(delivered.sentRegionsPresent);
  REQUIRE(delivered.sentRegionCount == 1U);
  REQUIRE(delivered.time == 6);
  REQUIRE(delivered.sentOrder == TIMESTAMP);
  REQUIRE(delivered.receivedOrder == RECEIVE);
  REQUIRE(delivered.hasRetraction);
  REQUIRE(delivered.retraction == afterAdmission);
  // Fence the callback completion before retracting the delivered message.
  REQUIRE(receiver->getInteractionClassHandle(className) == receiverClass);
  if (inspectDeliveryLedger) {
    auto const receiverId =
        umbra_binding_detail::federateHandleValue(receiverHandle);
    auto const messageId =
        umbra_binding_detail::messageRetractionHandleValue(afterAdmission);
    REQUIRE(receiverId.has_value());
    REQUIRE(messageId.has_value());
    // Internal witness only: a callback that already crossed the push
    // boundary must not still be eligible for its first delivery reservation.
    REQUIRE_FALSE(registry.beginTsoInteractionCallback(
        federation, *receiverId, *messageId, true));
  }
  REQUIRE_NOTHROW(sender->retract(afterAdmission));
  auto const hasAfterAdmissionRetraction = [&] {
    auto const retractions = receiverReports.retractionReports();
    return std::find(retractions.begin(), retractions.end(), afterAdmission) !=
        retractions.end();
  };
  callbackUntil(*receiver, callbackModel, receiverReports, hasAfterAdmissionRetraction);
  REQUIRE(receiver->getInteractionClassHandle(className) == receiverClass);
  drainCallbacks(*receiver, callbackModel, receiverReports);
  auto const retractions = receiverReports.retractionReports();
  REQUIRE(std::count(retractions.begin(), retractions.end(), afterAdmission) == 1);
  REQUIRE(receiverReports.interactionReports().size() == 1U);

  REQUIRE_NOTHROW(receiver->unsubscribeInteractionClassWithRegions(
      receiverClass, RegionHandleSet{subscriptionRegion}));
  REQUIRE_NOTHROW(receiver->deleteRegion(subscriptionRegion));
  REQUIRE_NOTHROW(sender->deleteRegion(sourceRegion));
  REQUIRE_NOTHROW(receiver->resignFederationExecution(NO_ACTION));
  REQUIRE_NOTHROW(sender->resignFederationExecution(NO_ACTION));
  REQUIRE_NOTHROW(sender->destroyFederationExecution(federation));
  REQUIRE_NOTHROW(receiver->disconnect());
  REQUIRE_NOTHROW(sender->disconnect());
  authority.stop();
  REQUIRE_NOTHROW(authority.rethrowIfFailed());
}

}  // namespace

TEST_CASE(
    "Process pushed regional sent TSO interaction is received in receive order by a nonconstrained federate",
    "[integration][development-profile][2025][data-distribution-management]"
    "[interaction-management][time-management][retraction][process-boundary]"
    "[public-endpoint][process-regional-interaction-receiver-order-conversion-push]"
    "[rti.service.send-interaction-with-regions][rti.service.retract]"
    "[rti.service.enable-time-regulation][rti.service.disable-callbacks]"
    "[rti.service.enable-callbacks][rti.service.evoke-callback]"
    "[federate.callback.receive-interaction][federate.callback.request-retraction]") {
  SECTION("HLA_EVOKED") {
    verifyPushedRegionalTsoRoRetraction(false, HLA_EVOKED);
  }
  SECTION("HLA_IMMEDIATE") {
    verifyPushedRegionalTsoRoRetraction(false, HLA_IMMEDIATE);
  }
}

TEST_CASE(
    "Private process push reserves regional TSO-to-RO interaction delivery",
    "[unit][internal-only][no-normative-mapping][2025][process-boundary]"
    "[process-pushed-regional-tso-ro-delivery-ledger]") {
  verifyPushedRegionalTsoRoRetraction(true, HLA_EVOKED);
}

#endif
