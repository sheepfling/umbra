#include <catch2/catch_test_macros.hpp>

#include <array>
#include <atomic>
#include <cstdint>
#include <exception>
#include <memory>
#include <mutex>
#include <stdexcept>
#include <string>
#include <thread>
#include <vector>

#include <RTI/RTI1516.h>
#include <RTI/NullFederateAmbassador.h>
#include <RTI/time/HLAinteger64Interval.h>
#include <RTI/time/HLAinteger64Time.h>

#include <umbra/embedded_profile_configuration.hpp>

#if defined(UMBRA_ENABLE_EMBEDDED_FEDERATION_MANAGEMENT)
#include "ieee1516_2025_process_fom_test_support.hpp"
#include "internal/federation/federation_registry.hpp"
#include "internal/federation/process_federation_service.hpp"
#include "internal/federation/process_transport.hpp"
#include "internal/federation/process_transport_session.hpp"
#include "process_public_service_fixture.hpp"

namespace {

using namespace umbra::detail;
using namespace rti1516_2025;

class InteractionRetractionFederate final : public NullFederateAmbassador {
 public:
  void discoverObjectInstance(
      ObjectInstanceHandle const&, ObjectClassHandle const&,
      std::wstring const&, FederateHandle const&) override {
    ++discoveryCount;
  }

  void reflectAttributeValues(
      ObjectInstanceHandle const&, AttributeHandleValueMap const&,
      VariableLengthData const&, TransportationTypeHandle const&,
      FederateHandle const&, RegionHandleSet const*) override {
    ++attributeReflectionCount;
  }

  void removeObjectInstance(
      ObjectInstanceHandle const&, VariableLengthData const&,
      FederateHandle const&, LogicalTime const&, OrderType, OrderType,
      MessageRetractionHandle const*) override {
    ++objectRemovalCount;
  }

  void receiveInteraction(
      InteractionClassHandle const&, ParameterHandleValueMap const&,
      VariableLengthData const&, TransportationTypeHandle const&,
      FederateHandle const&, RegionHandleSet const*, LogicalTime const&,
      OrderType, OrderType, MessageRetractionHandle const*) override {
    std::scoped_lock lock(mutex);
    ++interactionCount;
  }

  void requestRetraction(MessageRetractionHandle const&) override {
    std::scoped_lock lock(mutex);
    ++requestRetractionCount;
  }

  void timeConstrainedEnabled(LogicalTime const&) override {
    std::scoped_lock lock(mutex);
    ++timeConstrainedEnabledCount;
  }

  void flushQueueGrant(LogicalTime const&, LogicalTime const&) override {
    std::scoped_lock lock(mutex);
    ++flushQueueGrantCount;
  }

  std::mutex mutex;
  unsigned interactionCount = 0;
  unsigned requestRetractionCount = 0;
  unsigned timeConstrainedEnabledCount = 0;
  unsigned flushQueueGrantCount = 0;
  std::atomic<unsigned> discoveryCount{0U};
  std::atomic<unsigned> attributeReflectionCount{0U};
  std::atomic<unsigned> objectRemovalCount{0U};
};

class ProcessFlushQueueFederate final : public NullFederateAmbassador {
 public:
  void timeRegulationEnabled(LogicalTime const&) override {
    ++timeRegulationEnabledCount;
  }

  void timeConstrainedEnabled(LogicalTime const&) override {
    ++timeConstrainedEnabledCount;
  }

  void receiveInteraction(
      InteractionClassHandle const&, ParameterHandleValueMap const&,
      VariableLengthData const&, TransportationTypeHandle const&,
      FederateHandle const&, RegionHandleSet const*, LogicalTime const& time,
      OrderType, OrderType, MessageRetractionHandle const*) override {
    auto const* integerTime = dynamic_cast<HLAinteger64Time const*>(&time);
    if (integerTime == nullptr) {
      throw std::runtime_error("FQR delivered a non-integer process time.");
    }
    interactionTimes.push_back(integerTime->getTime());
    callbackOrder.push_back('I');
  }

  void requestRetraction(MessageRetractionHandle const&) override {
    ++requestRetractionCount;
    callbackOrder.push_back('R');
  }

  void flushQueueGrant(
      LogicalTime const& time,
      LogicalTime const& optimisticTime) override {
    auto const* integerTime = dynamic_cast<HLAinteger64Time const*>(&time);
    auto const* integerOptimisticTime =
        dynamic_cast<HLAinteger64Time const*>(&optimisticTime);
    if (integerTime == nullptr || integerOptimisticTime == nullptr) {
      throw std::runtime_error("FQG delivered a non-integer process time.");
    }
    ++flushQueueGrantCount;
    grantTime = integerTime->getTime();
    optimisticTimeValue = integerOptimisticTime->getTime();
    callbackOrder.push_back('G');
  }

  unsigned timeRegulationEnabledCount = 0U;
  unsigned timeConstrainedEnabledCount = 0U;
  unsigned requestRetractionCount = 0U;
  unsigned flushQueueGrantCount = 0U;
  std::int64_t grantTime = -1;
  std::int64_t optimisticTimeValue = -1;
  std::vector<std::int64_t> interactionTimes;
  std::vector<char> callbackOrder;
};

}  // namespace

TEST_CASE(
    "RTIambassadors handle pulled timestamped interaction retraction around callback entry",
    "[integration][interaction-management][time-management][retraction]"
    "[transport][process-boundary][public-endpoint]"
    "[process-tso-in-transit][process-tso-interaction-retraction-before-callback]"
    "[process-tso-in-transit-interaction-retraction-before-callback]"
    "[2025]") {
  constexpr wchar_t const* federationName =
      L"process-tso-interaction-retract-before-callback";
  constexpr wchar_t const* senderName = L"interaction-retract-sender";
  constexpr wchar_t const* receiverName = L"interaction-retract-receiver";
  constexpr wchar_t const* constrainedReceiverName =
      L"interaction-retract-constrained-receiver";
  constexpr wchar_t const* interactionWide =
      L"HLAinteractionRoot.CustomerTransactions.FoodServed.MainCourseServed";
  constexpr wchar_t const* parameterWide = L"TimelinessOk";
  constexpr char const* interactionNarrow =
      "HLAinteractionRoot.CustomerTransactions.FoodServed.MainCourseServed";
  constexpr char const* parameterNarrow = "TimelinessOk";

  auto listener = ProcessTransportListener::listen({"127.0.0.1", 0U});
  REQUIRE(listener);
  auto const port = listener->address().port;
  REQUIRE(port != 0U);
  std::exception_ptr serverError;
  std::shared_ptr<ProcessTransportConnection> senderConnection;
  std::shared_ptr<ProcessTransportConnection> receiverConnection;
  std::shared_ptr<ProcessTransportConnection> constrainedReceiverConnection;
  std::thread server([&] {
    try {
      EmbeddedFederationRegistry registry;
      ProcessFederationService service(
          registry,
          umbra::test::process_fom_support_2025::composedProcessDefinition(),
          ProcessFederationServiceOptions{false});
      senderConnection = listener->accept(
          nullptr, {"interaction-retract-server", 0x9781U},
          [](std::wstring) {}, [](std::wstring) { return false; });
      ProcessTransportSession sender(senderConnection);
      auto senderHandler = service.handlerFor(sender);
      auto serveExpected = [](ProcessTransportSession& session,
                              auto const& handler,
                              TransportServiceOperation operation,
                              char const* description) {
        if (!umbra::test::servePrimaryProcessRequest(
                session, handler,
                [&](TransportServiceMessage const& request) {
                  if (request.operation != operation) {
                    throw std::runtime_error(description);
                  }
                  return handler(request);
                })) {
          throw std::runtime_error(description);
        }
      };

      serveExpected(sender, senderHandler,
                    TransportServiceOperation::create_federation_execution,
                    "Missing interaction retraction Create.");
      serveExpected(sender, senderHandler,
                    TransportServiceOperation::join_federation_execution,
                    "Missing interaction retraction sender Join.");
      receiverConnection = listener->accept(
          nullptr, {"interaction-retract-server", 0x9782U},
          [](std::wstring) {}, [](std::wstring) { return false; });
      ProcessTransportSession receiver(receiverConnection);
      auto receiverHandler = service.handlerFor(receiver);
      serveExpected(receiver, receiverHandler,
                    TransportServiceOperation::join_federation_execution,
                    "Missing interaction retraction receiver Join.");

      constrainedReceiverConnection = listener->accept(
          nullptr, {"interaction-retract-server", 0x9783U},
          [](std::wstring) {}, [](std::wstring) { return false; });
      ProcessTransportSession constrainedReceiver(constrainedReceiverConnection);
      auto constrainedReceiverHandler = service.handlerFor(constrainedReceiver);
      serveExpected(constrainedReceiver,
                    constrainedReceiverHandler,
                    TransportServiceOperation::join_federation_execution,
                    "Missing interaction retraction constrained receiver Join.");

      for (auto* entry : {&sender, &receiver, &constrainedReceiver}) {
        auto const& handler = entry == &sender
                                  ? senderHandler
                                  : entry == &receiver
                                        ? receiverHandler
                                        : constrainedReceiverHandler;
        serveExpected(*entry, handler,
                      TransportServiceOperation::get_interaction_class_handle,
                      "Missing interaction class lookup.");
        serveExpected(*entry, handler,
                      TransportServiceOperation::get_parameter_handle,
                      "Missing interaction parameter lookup.");
      }
      serveExpected(sender, senderHandler,
                    TransportServiceOperation::publish_interaction_class,
                    "Missing interaction Publish.");
      serveExpected(receiver, receiverHandler,
                    TransportServiceOperation::subscribe_interaction_class,
                    "Missing interaction Subscribe.");
      serveExpected(constrainedReceiver,
                    constrainedReceiverHandler,
                    TransportServiceOperation::subscribe_interaction_class,
                    "Missing constrained interaction Subscribe.");
      serveExpected(sender, senderHandler,
                    TransportServiceOperation::enable_time_regulation,
                    "Missing sender Enable Time Regulation.");
      umbra::test::serveExpectedSuccessfulVoidInvocationReport(
          serveExpected, sender, senderHandler);
      serveExpected(constrainedReceiver,
                    constrainedReceiverHandler,
                    TransportServiceOperation::enable_time_constrained,
                    "Missing constrained receiver Enable Time Constrained.");
      umbra::test::serveExpectedSuccessfulVoidInvocationReport(
          serveExpected, constrainedReceiver, constrainedReceiverHandler);
      serveExpected(sender, senderHandler,
                    TransportServiceOperation::send_interaction,
                    "Missing pre-poll timestamped Send Interaction.");
      serveExpected(sender, senderHandler,
                    TransportServiceOperation::retract,
                    "Missing pre-poll interaction Retract.");
      serveExpected(receiver, receiverHandler,
                    TransportServiceOperation::receive_interaction,
                    "Missing receiver pull after pre-poll retraction.");
      serveExpected(receiver, receiverHandler,
                    TransportServiceOperation::get_interaction_class_handle,
                    "Missing pre-poll retraction-stream fence.");

      serveExpected(sender, senderHandler,
                    TransportServiceOperation::send_interaction,
                    "Missing post-poll timestamped Send Interaction.");
      serveExpected(receiver, receiverHandler,
                    TransportServiceOperation::receive_interaction,
                    "Missing receiver pull before post-poll retraction.");
      serveExpected(sender, senderHandler,
                    TransportServiceOperation::retract,
                    "Missing post-poll interaction Retract.");
      serveExpected(receiver, receiverHandler,
                    TransportServiceOperation::acknowledge_tso_delivery,
                    "Missing suppressed pull TSO acknowledgement.");

      serveExpected(receiver, receiverHandler,
                    TransportServiceOperation::get_interaction_class_handle,
                    "Missing post-poll retraction-stream fence.");
      serveExpected(receiver, receiverHandler,
                    TransportServiceOperation::get_interaction_class_handle,
                    "Missing post-suppression callback fence.");
      serveExpected(sender, senderHandler,
                    TransportServiceOperation::send_interaction,
                    "Missing after-entry timestamped Send Interaction.");
      serveExpected(receiver, receiverHandler,
                    TransportServiceOperation::receive_interaction,
                    "Missing pull of the after-entry interaction.");
      bool deliveryAcknowledged = false;
      bool callbackStartFenceObserved = false;
      for (int attempt = 0; attempt != 2; ++attempt) {
        if (!umbra::test::servePrimaryProcessRequest(
                receiver, receiverHandler,
                [&](TransportServiceMessage const& request) {
                  if (request.operation ==
                      TransportServiceOperation::acknowledge_tso_delivery) {
                    deliveryAcknowledged = true;
                  } else if (request.operation ==
                             TransportServiceOperation::get_interaction_class_handle) {
                    callbackStartFenceObserved = true;
                  } else {
                    throw std::runtime_error(
                        "Unexpected receiver request around callback-start fence.");
                  }
                  return receiverHandler(request);
                })) {
          throw std::runtime_error(
              "Missing receiver callback-start or delivery acknowledgement request.");
        }
      }
      if (!deliveryAcknowledged || !callbackStartFenceObserved) {
        throw std::runtime_error(
            "The receiver did not acknowledge delivery and cross the callback-start fence.");
      }
      serveExpected(sender, senderHandler,
                    TransportServiceOperation::retract,
                    "Missing after-entry interaction Retract.");
      serveExpected(receiver, receiverHandler,
                    TransportServiceOperation::get_interaction_class_handle,
                    "Missing after-entry retraction callback fence.");
      serveExpected(sender, senderHandler,
                    TransportServiceOperation::send_interaction,
                    "Missing mixed-fanout timestamped Send Interaction.");
      serveExpected(receiver, receiverHandler,
                    TransportServiceOperation::receive_interaction,
                    "Missing mixed-fanout pull of the interaction.");
      serveExpected(constrainedReceiver,
                    constrainedReceiverHandler,
                    TransportServiceOperation::flush_queue_request,
                    "Missing constrained mixed-fanout FQR.");
      serveExpected(constrainedReceiver,
                    constrainedReceiverHandler,
                    TransportServiceOperation::resign_federation_execution,
                    "Missing constrained mixed-fanout receiver Resign.");
      serveExpected(sender, senderHandler,
                    TransportServiceOperation::retract,
                    "Missing mixed-fanout interaction Retract.");
      bool mixedDeliveryAcknowledged = false;
      bool mixedCallbackFenceObserved = false;
      for (int attempt = 0; attempt != 2; ++attempt) {
        if (!umbra::test::servePrimaryProcessRequest(
                receiver, receiverHandler,
                [&](TransportServiceMessage const& request) {
                  if (request.operation ==
                      TransportServiceOperation::acknowledge_tso_delivery) {
                    mixedDeliveryAcknowledged = true;
                  } else if (request.operation ==
                             TransportServiceOperation::get_interaction_class_handle) {
                    mixedCallbackFenceObserved = true;
                  } else {
                    throw std::runtime_error(
                        "Unexpected receiver request around mixed-fanout callback fence.");
                  }
                  return receiverHandler(request);
                })) {
          throw std::runtime_error(
              "Missing mixed-fanout receiver callback fence or acknowledgement.");
        }
      }
      if (!mixedDeliveryAcknowledged || !mixedCallbackFenceObserved) {
        throw std::runtime_error(
            "The mixed-fanout receiver did not acknowledge its pulled TSO event.");
      }
      serveExpected(sender, senderHandler,
                    TransportServiceOperation::resign_federation_execution,
                    "Missing sender Resign.");
      serveExpected(receiver, receiverHandler,
                    TransportServiceOperation::resign_federation_execution,
                    "Missing receiver Resign.");
      service.detach(sender);
      service.detach(receiver);
      service.detach(constrainedReceiver);
      senderConnection->close();
      receiverConnection->close();
      constrainedReceiverConnection->close();
    } catch (...) {
      serverError = std::current_exception();
      if (senderConnection) senderConnection->close();
      if (receiverConnection) receiverConnection->close();
      if (constrainedReceiverConnection) constrainedReceiverConnection->close();
    }
  });

  InteractionRetractionFederate senderFederate;
  InteractionRetractionFederate receiverFederate;
  InteractionRetractionFederate constrainedReceiverFederate;
  RTIambassadorFactory factory;
  auto senderRti = factory.createRTIambassador();
  auto receiverRti = factory.createRTIambassador();
  auto constrainedReceiverRti = factory.createRTIambassador();
  auto configuration = RtiConfiguration::createConfiguration()
                           .withConfigurationName(L"interaction-retract-before-callback")
                           .withRtiAddress(
                               L"tcp://127.0.0.1:" + std::to_wstring(port));
  std::exception_ptr clientError;
  bool senderJoined = false;
  bool receiverJoined = false;
  bool constrainedReceiverJoined = false;
  try {
    REQUIRE(senderRti->connect(senderFederate, HLA_IMMEDIATE, configuration)
                .addressUsed);
    REQUIRE_NOTHROW(senderRti->createFederationExecution(
        federationName, L"server-owned-interaction-retraction-fom.xml"));
    REQUIRE(senderRti->joinFederationExecution(
                senderName, L"interaction-retract-type", federationName)
                .isValid());
    senderJoined = true;
    REQUIRE(receiverRti->connect(receiverFederate, HLA_EVOKED, configuration)
                .addressUsed);
    REQUIRE(receiverRti->joinFederationExecution(
                receiverName, L"interaction-retract-type", federationName)
                .isValid());
    receiverJoined = true;
    REQUIRE(constrainedReceiverRti->connect(
                constrainedReceiverFederate, HLA_EVOKED, configuration)
                .addressUsed);
    REQUIRE(constrainedReceiverRti->joinFederationExecution(
                constrainedReceiverName,
                L"interaction-retract-type",
                federationName)
                .isValid());
    constrainedReceiverJoined = true;

    auto const senderClass = senderRti->getInteractionClassHandle(interactionWide);
    auto const senderParameter = senderRti->getParameterHandle(senderClass, parameterWide);
    auto const receiverClass = receiverRti->getInteractionClassHandle(interactionWide);
    auto const receiverParameter = receiverRti->getParameterHandle(receiverClass, parameterWide);
    auto const constrainedReceiverClass =
        constrainedReceiverRti->getInteractionClassHandle(interactionWide);
    auto const constrainedReceiverParameter =
        constrainedReceiverRti->getParameterHandle(
            constrainedReceiverClass, parameterWide);
    REQUIRE(senderClass == receiverClass);
    REQUIRE(senderParameter == receiverParameter);
    REQUIRE(senderClass == constrainedReceiverClass);
    REQUIRE(senderParameter == constrainedReceiverParameter);
    senderRti->publishInteractionClass(senderClass);
    receiverRti->subscribeInteractionClass(receiverClass);
    constrainedReceiverRti->subscribeInteractionClass(constrainedReceiverClass);
    senderRti->enableTimeRegulation(HLAinteger64Interval(1));
    constrainedReceiverRti->enableTimeConstrained();
    static_cast<void>(constrainedReceiverRti->evokeCallback(0.0));
    REQUIRE(constrainedReceiverFederate.timeConstrainedEnabledCount == 1U);
    receiverRti->disableCallbacks();

    std::array<std::uint8_t, 2U> const bytes{0x52U, 0x54U};
    VariableLengthData const value(bytes.data(), bytes.size());
    ParameterHandleValueMap parameters;
    parameters.emplace(senderParameter, value);
    auto const prePollRetraction = senderRti->sendInteraction(
        senderClass, parameters, value, HLAinteger64Time(5));
    REQUIRE(prePollRetraction.isValid());

    // Retract before the pull leaves the original event in the service queue.
    // The subsequent public poll must not expose it as an interaction callback.
    REQUIRE_NOTHROW(senderRti->retract(prePollRetraction));
    static_cast<void>(receiverRti->evokeCallback(0.0));
    static_cast<void>(receiverRti->getInteractionClassHandle(interactionWide));
    {
      std::scoped_lock lock(receiverFederate.mutex);
      REQUIRE(receiverFederate.interactionCount == 0U);
      REQUIRE(receiverFederate.requestRetractionCount == 0U);
    }

    auto const beforeStartRetraction = senderRti->sendInteraction(
        senderClass, parameters, value, HLAinteger64Time(6));
    REQUIRE(beforeStartRetraction.isValid());

    // Evoke polls the process endpoint and admits the interaction to the
    // callback queue while callbacks are disabled. Retraction therefore
    // lands after pull but before the public callback starts.
    REQUIRE(receiverRti->evokeCallback(0.0));
    REQUIRE(receiverFederate.interactionCount == 0U);
    REQUIRE_NOTHROW(senderRti->retract(beforeStartRetraction));
    static_cast<void>(receiverRti->getInteractionClassHandle(interactionWide));
    receiverRti->enableCallbacks();
    static_cast<void>(receiverRti->evokeCallback(0.0));
    // The canceled callback slot is consumed without starting either the
    // original interaction or a Request Retraction callback.
    static_cast<void>(receiverRti->getInteractionClassHandle(interactionWide));
    {
      std::scoped_lock lock(receiverFederate.mutex);
      REQUIRE(receiverFederate.interactionCount == 0U);
      REQUIRE(receiverFederate.requestRetractionCount == 0U);
    }

    auto const afterEntryRetraction = senderRti->sendInteraction(
        senderClass, parameters, value, HLAinteger64Time(7));
    REQUIRE(afterEntryRetraction.isValid());
    for (int attempt = 0; attempt != 4; ++attempt) {
      static_cast<void>(receiverRti->evokeCallback(0.05));
      std::scoped_lock lock(receiverFederate.mutex);
      if (receiverFederate.interactionCount == 1U) {
        break;
      }
    }
    {
      std::scoped_lock lock(receiverFederate.mutex);
      REQUIRE(receiverFederate.interactionCount == 1U);
      REQUIRE(receiverFederate.requestRetractionCount == 0U);
    }
    static_cast<void>(receiverRti->getInteractionClassHandle(interactionWide));
    REQUIRE_NOTHROW(senderRti->retract(afterEntryRetraction));
    static_cast<void>(receiverRti->evokeCallback(0.0));
    static_cast<void>(receiverRti->getInteractionClassHandle(interactionWide));
    {
      std::scoped_lock lock(receiverFederate.mutex);
      REQUIRE(receiverFederate.interactionCount == 1U);
      REQUIRE(receiverFederate.requestRetractionCount == 1U);
    }

    // Mixed fanout: the unconstrained evoked receiver pulls the event while a
    // constrained receiver keeps its TSO copy queued. Retract both before the
    // former callback starts, then force the constrained queue through FQR to
    // prove the canceled message was removed from both delivery lanes. An
    // unmatched Request Retraction callback is permitted, so only bound its
    // count rather than requiring a particular notification policy.
    receiverRti->disableCallbacks();
    unsigned requestRetractionsBeforeMixedFanout = 0U;
    {
      std::scoped_lock lock(receiverFederate.mutex);
      requestRetractionsBeforeMixedFanout =
          receiverFederate.requestRetractionCount;
    }
    auto const mixedFanoutRetraction = senderRti->sendInteraction(
        senderClass, parameters, value, HLAinteger64Time(8));
    REQUIRE(mixedFanoutRetraction.isValid());
    REQUIRE(receiverRti->evokeCallback(0.0));
    {
      std::scoped_lock lock(receiverFederate.mutex);
      REQUIRE(receiverFederate.interactionCount == 1U);
      REQUIRE(receiverFederate.requestRetractionCount >=
              requestRetractionsBeforeMixedFanout);
      REQUIRE(receiverFederate.requestRetractionCount <=
              requestRetractionsBeforeMixedFanout + 1U);
    }
    constrainedReceiverRti->disableCallbacks();
    REQUIRE_NOTHROW(
        constrainedReceiverRti->flushQueueRequest(HLAinteger64Time(8)));
    {
      std::scoped_lock lock(constrainedReceiverFederate.mutex);
      REQUIRE(constrainedReceiverFederate.interactionCount == 0U);
      REQUIRE(constrainedReceiverFederate.flushQueueGrantCount == 0U);
    }
    REQUIRE_NOTHROW(
        constrainedReceiverRti->resignFederationExecution(NO_ACTION));
    constrainedReceiverJoined = false;
    REQUIRE_NOTHROW(senderRti->retract(mixedFanoutRetraction));
    receiverRti->enableCallbacks();
    static_cast<void>(receiverRti->evokeCallback(0.0));
    static_cast<void>(receiverRti->getInteractionClassHandle(interactionWide));
    {
      std::scoped_lock lock(receiverFederate.mutex);
      REQUIRE(receiverFederate.interactionCount == 1U);
      REQUIRE(receiverFederate.requestRetractionCount >=
              requestRetractionsBeforeMixedFanout);
      REQUIRE(receiverFederate.requestRetractionCount <=
              requestRetractionsBeforeMixedFanout + 1U);
    }
    constrainedReceiverRti->enableCallbacks();
    static_cast<void>(constrainedReceiverRti->evokeCallback(0.0));
    {
      std::scoped_lock lock(constrainedReceiverFederate.mutex);
      REQUIRE(constrainedReceiverFederate.interactionCount == 0U);
      REQUIRE(constrainedReceiverFederate.requestRetractionCount == 0U);
    }

    REQUIRE_NOTHROW(senderRti->resignFederationExecution(NO_ACTION));
    senderJoined = false;
    REQUIRE_NOTHROW(receiverRti->resignFederationExecution(NO_ACTION));
    receiverJoined = false;
    REQUIRE_NOTHROW(senderRti->disconnect());
    REQUIRE_NOTHROW(receiverRti->disconnect());
    REQUIRE_NOTHROW(constrainedReceiverRti->disconnect());
  } catch (...) {
    clientError = std::current_exception();
    if (senderJoined) {
      try { senderRti->resignFederationExecution(DELETE_OBJECTS); } catch (...) {}
    }
    if (receiverJoined) {
      try { receiverRti->resignFederationExecution(NO_ACTION); } catch (...) {}
    }
    if (constrainedReceiverJoined) {
      try {
        constrainedReceiverRti->resignFederationExecution(NO_ACTION);
      } catch (...) {
      }
    }
    try { senderRti->disconnect(); } catch (...) {}
    try { receiverRti->disconnect(); } catch (...) {}
    try { constrainedReceiverRti->disconnect(); } catch (...) {}
  }
  listener.reset();
  if (server.joinable()) server.join();
  if (serverError) {
    try {
      std::rethrow_exception(serverError);
    } catch (std::exception const& error) {
      FAIL("Process interaction-retraction fixture failed: " << error.what());
    }
  }
  if (clientError) std::rethrow_exception(clientError);
  REQUIRE_FALSE(senderJoined);
  REQUIRE_FALSE(receiverJoined);
  REQUIRE_FALSE(constrainedReceiverJoined);
}

TEST_CASE(
    "RTIambassadors discard queued process attribute and removal callbacks when the recipient Resigns before Retract",
    "[integration][object-management][attribute-update][time-management][retraction]"
    "[transport][process-boundary][public-endpoint][process-tso-resign-retraction]"
    "[github-issue-5][2025]") {
  constexpr wchar_t const* federationName =
      L"process-tso-resign-attribute-removal-retraction";
  constexpr wchar_t const* senderName = L"resign-retraction-sender";
  constexpr wchar_t const* receiverName = L"resign-retraction-receiver";
  constexpr wchar_t const* objectClassName =
      L"HLAobjectRoot.Employee.Server";
  constexpr wchar_t const* attributeName = L"HomeNumber";

  auto listener = ProcessTransportListener::listen({"127.0.0.1", 0U});
  REQUIRE(listener);
  auto const port = listener->address().port;
  REQUIRE(port != 0U);
  std::exception_ptr serverError;
  std::thread server([&] {
    try {
      EmbeddedFederationRegistry registry;
      ProcessFederationService service(
          registry,
          umbra::test::process_fom_support_2025::composedProcessDefinition(),
          ProcessFederationServiceOptions{true});
      auto senderConnection = listener->accept(
          nullptr, {"resign-retraction-server", 0x9784U},
          [](std::wstring) {}, [](std::wstring) { return false; });
      auto receiverConnection = listener->accept(
          nullptr, {"resign-retraction-server", 0x9785U},
          [](std::wstring) {}, [](std::wstring) { return false; });
      ProcessTransportSession sender(senderConnection);
      ProcessTransportSession receiver(receiverConnection);
      auto senderHandler = service.handlerFor(sender);
      auto receiverHandler = service.handlerFor(receiver);
      std::array<std::exception_ptr, 2U> workerErrors{};
      std::thread senderWorker([&] {
        try {
          while (ProcessTransportServiceDispatcher::serveOne(
              sender, senderHandler)) {}
          service.detach(sender);
        } catch (...) {
          workerErrors[0] = std::current_exception();
        }
        senderConnection->close();
      });
      std::thread receiverWorker([&] {
        try {
          while (ProcessTransportServiceDispatcher::serveOne(
              receiver, receiverHandler)) {}
          service.detach(receiver);
        } catch (...) {
          workerErrors[1] = std::current_exception();
        }
        receiverConnection->close();
      });
      senderWorker.join();
      receiverWorker.join();
      if (workerErrors[0]) {
        std::rethrow_exception(workerErrors[0]);
      }
      if (workerErrors[1]) {
        std::rethrow_exception(workerErrors[1]);
      }
    } catch (...) {
      serverError = std::current_exception();
    }
  });

  InteractionRetractionFederate senderFederate;
  InteractionRetractionFederate receiverFederate;
  RTIambassadorFactory factory;
  auto senderRti = factory.createRTIambassador();
  auto receiverRti = factory.createRTIambassador();
  auto configuration = RtiConfiguration::createConfiguration()
                           .withConfigurationName(
                               L"process-tso-resign-family-retractions")
                           .withRtiAddress(
                               L"tcp://127.0.0.1:" + std::to_wstring(port));
  std::exception_ptr clientError;
  bool senderJoined = false;
  bool receiverJoined = false;
  try {
    REQUIRE(senderRti->connect(senderFederate, HLA_EVOKED, configuration)
                .addressUsed);
    REQUIRE(receiverRti->connect(receiverFederate, HLA_EVOKED, configuration)
                .addressUsed);
    REQUIRE_NOTHROW(senderRti->createFederationExecution(
        federationName, L"server-owned-resign-retraction-fom.xml"));
    REQUIRE(senderRti->joinFederationExecution(
                senderName, L"resign-retraction-type", federationName)
                .isValid());
    senderJoined = true;
    REQUIRE(receiverRti->joinFederationExecution(
                receiverName, L"resign-retraction-type", federationName)
                .isValid());
    receiverJoined = true;

    auto const senderObjectClass =
        senderRti->getObjectClassHandle(objectClassName);
    auto const receiverObjectClass =
        receiverRti->getObjectClassHandle(objectClassName);
    auto const senderAttribute =
        senderRti->getAttributeHandle(senderObjectClass, attributeName);
    auto const receiverAttribute =
        receiverRti->getAttributeHandle(receiverObjectClass, attributeName);
    REQUIRE(senderObjectClass == receiverObjectClass);
    REQUIRE(senderAttribute == receiverAttribute);
    AttributeHandleSet const attributes{senderAttribute};
    senderRti->publishObjectClassAttributes(senderObjectClass, attributes);
    receiverRti->subscribeObjectClassAttributes(receiverObjectClass, attributes);
    auto const objectInstance =
        senderRti->registerObjectInstance(senderObjectClass);
    REQUIRE(objectInstance.isValid());

    for (unsigned attempt = 0U;
         attempt != 8U && receiverFederate.discoveryCount.load() == 0U;
         ++attempt) {
      static_cast<void>(receiverRti->evokeCallback(0.0));
    }
    REQUIRE(receiverFederate.discoveryCount.load() > 0U);
    REQUIRE_NOTHROW(senderRti->enableTimeRegulation(HLAinteger64Interval(1)));

    std::array<std::uint8_t, 2U> const valueBytes{0x34U, 0x32U};
    std::array<std::uint8_t, 3U> const tagBytes{0x52U, 0x45U, 0x46U};
    AttributeHandleValueMap attributeValues;
    attributeValues.emplace(
        senderAttribute,
        VariableLengthData(valueBytes.data(), valueBytes.size()));
    VariableLengthData const tag(tagBytes.data(), tagBytes.size());

    receiverRti->disableCallbacks();
    auto const updateRetraction = senderRti->updateAttributeValues(
        objectInstance,
        attributeValues,
        tag,
        HLAinteger64Time(5));
    REQUIRE(updateRetraction.isValid());
    static_cast<void>(receiverRti->getObjectClassHandle(objectClassName));
    REQUIRE(receiverRti->evokeMultipleCallbacks(0.0, 0.0));
    REQUIRE(receiverFederate.attributeReflectionCount.load() == 0U);
    REQUIRE_NOTHROW(receiverRti->resignFederationExecution(NO_ACTION));
    receiverJoined = false;
    REQUIRE(receiverFederate.attributeReflectionCount.load() == 0U);
    REQUIRE_NOTHROW(senderRti->retract(updateRetraction));

    REQUIRE(receiverRti->joinFederationExecution(
                receiverName, L"resign-retraction-type", federationName)
                .isValid());
    receiverJoined = true;
    receiverRti->enableCallbacks();
    for (unsigned attempt = 0U; attempt != 4U; ++attempt) {
      static_cast<void>(receiverRti->evokeCallback(0.0));
    }
    REQUIRE(receiverFederate.attributeReflectionCount.load() == 0U);

    receiverRti->subscribeObjectClassAttributes(receiverObjectClass, attributes);
    for (unsigned attempt = 0U; attempt != 4U; ++attempt) {
      static_cast<void>(receiverRti->evokeCallback(0.0));
    }
    receiverRti->disableCallbacks();
    auto const removalRetraction = senderRti->deleteObjectInstance(
        objectInstance, tag, HLAinteger64Time(6));
    REQUIRE(removalRetraction.isValid());
    REQUIRE(receiverRti->evokeCallback(0.0));
    REQUIRE(receiverFederate.objectRemovalCount.load() == 0U);
    REQUIRE_NOTHROW(receiverRti->resignFederationExecution(NO_ACTION));
    receiverJoined = false;
    REQUIRE(receiverFederate.objectRemovalCount.load() == 0U);
    REQUIRE_NOTHROW(senderRti->retract(removalRetraction));

    REQUIRE(receiverRti->joinFederationExecution(
                receiverName, L"resign-retraction-type", federationName)
                .isValid());
    receiverJoined = true;
    receiverRti->enableCallbacks();
    for (unsigned attempt = 0U; attempt != 4U; ++attempt) {
      static_cast<void>(receiverRti->evokeCallback(0.0));
    }
    REQUIRE(receiverFederate.objectRemovalCount.load() == 0U);

    REQUIRE_NOTHROW(receiverRti->resignFederationExecution(NO_ACTION));
    receiverJoined = false;
    REQUIRE_NOTHROW(senderRti->resignFederationExecution(DELETE_OBJECTS));
    senderJoined = false;
    REQUIRE_NOTHROW(senderRti->disconnect());
    REQUIRE_NOTHROW(receiverRti->disconnect());
  } catch (...) {
    clientError = std::current_exception();
    if (receiverJoined) {
      try { receiverRti->resignFederationExecution(NO_ACTION); } catch (...) {}
    }
    if (senderJoined) {
      try { senderRti->resignFederationExecution(DELETE_OBJECTS); } catch (...) {}
    }
    try { senderRti->disconnect(); } catch (...) {}
    try { receiverRti->disconnect(); } catch (...) {}
  }
  listener.reset();
  if (server.joinable()) server.join();
  if (serverError) {
    try {
      std::rethrow_exception(serverError);
    } catch (std::exception const& error) {
      FAIL("Process attribute/removal retraction fixture failed: " << error.what());
    }
  }
  if (clientError) std::rethrow_exception(clientError);
  REQUIRE_FALSE(senderJoined);
  REQUIRE_FALSE(receiverJoined);
}

TEST_CASE(
    "RTIambassadors preserve the bounded FQR arrival policy across the process callback boundary",
    "[integration][interaction-management][time-management][flush-queue-request]"
    "[transport][process-boundary][public-endpoint][process-tso-fqr-arrival-boundary]"
    "[github-issue-6][2025]") {
  constexpr wchar_t const* federationName =
      L"process-fqr-arrival-before-evoked-grant";
  constexpr wchar_t const* senderName = L"process-fqr-arrival-sender";
  constexpr wchar_t const* receiverName = L"process-fqr-arrival-receiver";
  constexpr wchar_t const* interactionWide =
      L"HLAinteractionRoot.CustomerTransactions.FoodServed.MainCourseServed";
  constexpr wchar_t const* parameterWide = L"TimelinessOk";

  auto listener = ProcessTransportListener::listen({"127.0.0.1", 0U});
  REQUIRE(listener);
  auto const port = listener->address().port;
  REQUIRE(port != 0U);
  std::exception_ptr serverError;
  std::thread server([&] {
    try {
      EmbeddedFederationRegistry registry;
      ProcessFederationService service(
          registry,
          umbra::test::process_fom_support_2025::composedProcessDefinition(),
          ProcessFederationServiceOptions{false});
      auto senderConnection = listener->accept(
          nullptr, {"fqr-arrival-server", 0x9786U},
          [](std::wstring) {}, [](std::wstring) { return false; });
      auto receiverConnection = listener->accept(
          nullptr, {"fqr-arrival-server", 0x9787U},
          [](std::wstring) {}, [](std::wstring) { return false; });
      ProcessTransportSession sender(senderConnection);
      ProcessTransportSession receiver(receiverConnection);
      auto senderHandler = service.handlerFor(sender);
      auto receiverHandler = service.handlerFor(receiver);
      std::array<std::exception_ptr, 2U> workerErrors{};
      std::thread senderWorker([&] {
        try {
          while (ProcessTransportServiceDispatcher::serveOne(
              sender, senderHandler)) {}
          service.detach(sender);
        } catch (...) {
          workerErrors[0] = std::current_exception();
        }
        senderConnection->close();
      });
      std::thread receiverWorker([&] {
        try {
          while (ProcessTransportServiceDispatcher::serveOne(
              receiver, receiverHandler)) {}
          service.detach(receiver);
        } catch (...) {
          workerErrors[1] = std::current_exception();
        }
        receiverConnection->close();
      });
      senderWorker.join();
      receiverWorker.join();
      if (workerErrors[0]) {
        std::rethrow_exception(workerErrors[0]);
      }
      if (workerErrors[1]) {
        std::rethrow_exception(workerErrors[1]);
      }
    } catch (...) {
      serverError = std::current_exception();
    }
  });

  ProcessFlushQueueFederate senderFederate;
  ProcessFlushQueueFederate receiverFederate;
  RTIambassadorFactory factory;
  auto senderRti = factory.createRTIambassador();
  auto receiverRti = factory.createRTIambassador();
  auto configuration = RtiConfiguration::createConfiguration()
                           .withConfigurationName(
                               L"process-fqr-arrival-before-evoked-grant")
                           .withRtiAddress(
                               L"tcp://127.0.0.1:" + std::to_wstring(port));
  std::exception_ptr clientError;
  bool senderJoined = false;
  bool receiverJoined = false;
  try {
    REQUIRE(senderRti->connect(senderFederate, HLA_EVOKED, configuration)
                .addressUsed);
    REQUIRE(receiverRti->connect(receiverFederate, HLA_EVOKED, configuration)
                .addressUsed);
    REQUIRE_NOTHROW(senderRti->createFederationExecution(
        federationName, L"server-owned-fqr-arrival-fom.xml"));
    REQUIRE(senderRti->joinFederationExecution(
                senderName, L"fqr-arrival-type", federationName)
                .isValid());
    senderJoined = true;
    REQUIRE(receiverRti->joinFederationExecution(
                receiverName, L"fqr-arrival-type", federationName)
                .isValid());
    receiverJoined = true;

    auto const senderClass =
        senderRti->getInteractionClassHandle(interactionWide);
    auto const receiverClass =
        receiverRti->getInteractionClassHandle(interactionWide);
    auto const senderParameter =
        senderRti->getParameterHandle(senderClass, parameterWide);
    auto const receiverParameter =
        receiverRti->getParameterHandle(receiverClass, parameterWide);
    REQUIRE(senderClass == receiverClass);
    REQUIRE(senderParameter == receiverParameter);
    senderRti->publishInteractionClass(senderClass);
    senderRti->changeInteractionOrderType(senderClass, TIMESTAMP);
    receiverRti->subscribeInteractionClass(receiverClass);
    REQUIRE_NOTHROW(receiverRti->enableTimeConstrained());
    static_cast<void>(receiverRti->evokeCallback(0.0));
    REQUIRE(receiverFederate.timeConstrainedEnabledCount == 1U);
    REQUIRE_NOTHROW(senderRti->enableTimeRegulation(HLAinteger64Interval(5)));
    static_cast<void>(senderRti->evokeCallback(0.0));
    REQUIRE(senderFederate.timeRegulationEnabledCount == 1U);

    std::array<std::uint8_t, 2U> const valueBytes{0x46U, 0x51U};
    std::array<std::uint8_t, 3U> const tagBytes{0x52U, 0x41U, 0x54U};
    VariableLengthData const value(valueBytes.data(), valueBytes.size());
    VariableLengthData const tag(tagBytes.data(), tagBytes.size());
    ParameterHandleValueMap parameters;
    parameters.emplace(senderParameter, value);

    // The process coordinator fixes this request's FQR input set when it
    // accepts FQR(10). These later transport arrivals are implementation
    // policy evidence: unlike the embedded dispatcher, the process route does
    // not add them to the already accepted grant, even before the HLA_EVOKED
    // grant callback starts. The second is retracted before callback entry.
    REQUIRE_NOTHROW(receiverRti->flushQueueRequest(HLAinteger64Time(10)));
    REQUIRE(receiverFederate.flushQueueGrantCount == 0U);
    auto const timestampNine = senderRti->sendInteraction(
        senderClass, parameters, tag, HLAinteger64Time(9));
    auto const timestampTwelve = senderRti->sendInteraction(
        senderClass, parameters, tag, HLAinteger64Time(12));
    REQUIRE(timestampNine.isValid());
    REQUIRE(timestampTwelve.isValid());
    REQUIRE_NOTHROW(senderRti->retract(timestampTwelve));
    REQUIRE(receiverFederate.interactionTimes.empty());
    REQUIRE(receiverFederate.flushQueueGrantCount == 0U);

    for (unsigned attempt = 0U;
         attempt != 8U && receiverFederate.flushQueueGrantCount == 0U;
         ++attempt) {
      static_cast<void>(receiverRti->evokeCallback(0.0));
    }
    REQUIRE(receiverFederate.interactionTimes.empty());
    REQUIRE(receiverFederate.flushQueueGrantCount == 1U);
    REQUIRE(receiverFederate.grantTime == 5);
    REQUIRE(receiverFederate.optimisticTimeValue == 10);
    std::vector<char> const deliveredCallbackOrder = [&] {
      std::vector<char> result;
      for (char const callback : receiverFederate.callbackOrder) {
        if (callback != 'R') {
          result.push_back(callback);
        }
      }
      return result;
    }();
    REQUIRE(deliveredCallbackOrder == std::vector<char>{'G'});

    // The grant is 5, so no later TSO input below 5 may be accepted. This
    // checks the producer-side time boundary without inventing a transport
    // arrival requirement for the earlier request.
    REQUIRE_THROWS_AS(
        senderRti->sendInteraction(
            senderClass, parameters, tag, HLAinteger64Time(4)),
        InvalidLogicalTime);
    REQUIRE(receiverFederate.interactionTimes.empty());
    REQUIRE(receiverFederate.flushQueueGrantCount == 1U);

    REQUIRE_NOTHROW(receiverRti->resignFederationExecution(NO_ACTION));
    receiverJoined = false;
    REQUIRE_NOTHROW(senderRti->resignFederationExecution(NO_ACTION));
    senderJoined = false;
    REQUIRE_NOTHROW(senderRti->disconnect());
    REQUIRE_NOTHROW(receiverRti->disconnect());
  } catch (...) {
    clientError = std::current_exception();
    if (receiverJoined) {
      try { receiverRti->resignFederationExecution(NO_ACTION); } catch (...) {}
    }
    if (senderJoined) {
      try { senderRti->resignFederationExecution(NO_ACTION); } catch (...) {}
    }
    try { senderRti->disconnect(); } catch (...) {}
    try { receiverRti->disconnect(); } catch (...) {}
  }
  listener.reset();
  if (server.joinable()) server.join();
  if (serverError) {
    try {
      std::rethrow_exception(serverError);
    } catch (std::exception const& error) {
      FAIL("Process FQR-arrival fixture failed: " << error.what());
    }
  }
  if (clientError) std::rethrow_exception(clientError);
  REQUIRE_FALSE(senderJoined);
  REQUIRE_FALSE(receiverJoined);
}

#endif
