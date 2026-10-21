#include <catch2/catch_test_macros.hpp>

#include <array>
#include <atomic>
#include <exception>
#include <stdexcept>
#include <string>
#include <thread>
#include <tuple>

#include <RTI/NullFederateAmbassador.h>
#include <RTI/RTI1516.h>
#include <RTI/time/HLAinteger64Interval.h>
#include <RTI/time/HLAinteger64Time.h>

#include <umbra/embedded_profile_configuration.hpp>

#if defined(UMBRA_ENABLE_EMBEDDED_FEDERATION_MANAGEMENT)
#include "ieee1516_2025_connection_test_support.hpp"
#include "ieee1516_2025_process_fom_test_support.hpp"
#include "internal/federation/federation_registry.hpp"
#include "internal/federation/process_federation_service.hpp"
#include "internal/federation/process_transport.hpp"
#include "internal/federation/process_transport_session.hpp"
#include "process_public_service_fixture.hpp"

namespace {

using rti1516_2025::HLA_EVOKED;
using rti1516_2025::HLA_IMMEDIATE;
using rti1516_2025::NO_ACTION;
using rti1516_2025::NullFederateAmbassador;
using rti1516_2025::RtiConfiguration;
using umbra::detail::EmbeddedFederationRegistry;
using umbra::detail::ProcessFederationService;
using umbra::detail::ProcessFederationServiceOptions;
using umbra::detail::ProcessTransportListener;
using umbra::detail::ProcessTransportSession;
using umbra::detail::TransportServiceMessage;
using umbra::detail::TransportServiceOperation;
using umbra::test::connection_support_2025::makeRti;
using umbra::test::process_fom_support_2025::composedProcessDefinition;

class EvokedAsyncDeliveryFederateAmbassador final : public NullFederateAmbassador {
 public:
  void timeConstrainedEnabled(rti1516_2025::LogicalTime const&) override {
    timeConstrainedEnabledCount.fetch_add(1U, std::memory_order_release);
  }

  void timeRegulationEnabled(rti1516_2025::LogicalTime const&) override {
    timeRegulationEnabledCount.fetch_add(1U, std::memory_order_release);
  }

  void timeAdvanceGrant(rti1516_2025::LogicalTime const&) override {
    timeAdvanceGrantCount.fetch_add(1U, std::memory_order_release);
  }

  void receiveInteraction(
      rti1516_2025::InteractionClassHandle const&,
      rti1516_2025::ParameterHandleValueMap const&,
      rti1516_2025::VariableLengthData const&,
      rti1516_2025::TransportationTypeHandle const&,
      rti1516_2025::FederateHandle const&,
      rti1516_2025::RegionHandleSet const*) override {
    receiveOrderInteractionCount.fetch_add(1U, std::memory_order_release);
  }

  std::atomic_uint timeConstrainedEnabledCount{0U};
  std::atomic_uint timeRegulationEnabledCount{0U};
  std::atomic_uint timeAdvanceGrantCount{0U};
  std::atomic_uint receiveOrderInteractionCount{0U};
};

}  // namespace

TEST_CASE(
    "RTIambassador gates process receive-order callbacks on asynchronous delivery under HLA_EVOKED",
    "[integration][time-management][interaction-management][callbacks]"
    "[process-boundary][public-endpoint][process-asynchronous-receive-order-callback-gate]"
    "[callback-evoked][rti.service.enable-time-constrained]"
    "[rti.service.enable-asynchronous-delivery]"
    "[rti.service.disable-asynchronous-delivery]"
    "[federate.callback.receive-interaction][2025]") {
  constexpr wchar_t const* federationName =
      L"public-process-asynchronous-delivery-evoked-execution";
  constexpr wchar_t const* senderName = L"public-process-asynchronous-evoked-sender";
  constexpr wchar_t const* receiverName = L"public-process-asynchronous-evoked-receiver";
  constexpr wchar_t const* interactionName =
      L"HLAinteractionRoot.CustomerTransactions.FoodServed.MainCourseServed";
  constexpr wchar_t const* parameterName = L"TimelinessOk";

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
          composedProcessDefinition(),
          ProcessFederationServiceOptions{true});
      auto senderConnection = listener->accept(
          nullptr,
          {"process-asynchronous-delivery-evoked-server", 0xA520U},
          [](std::wstring) {},
          [](std::wstring) { return false; });
      ProcessTransportSession sender(senderConnection);
      auto senderHandler = service.handlerFor(sender);
      auto serveExpected = [](ProcessTransportSession& session,
                              auto const& handler,
                              TransportServiceOperation operation,
                              char const* description) {
        if (!umbra::test::servePrimaryProcessRequest(
                session,
                handler,
                [&](TransportServiceMessage const& request) {
                  if (request.operation != operation) {
                    throw std::runtime_error(description);
                  }
                  return handler(request);
                })) {
          throw std::runtime_error(description);
        }
      };

      serveExpected(
          sender, senderHandler,
          TransportServiceOperation::create_federation_execution,
          "The HLA_EVOKED process server lost Create.");
      serveExpected(
          sender, senderHandler,
          TransportServiceOperation::join_federation_execution,
          "The HLA_EVOKED process server lost sender Join.");

      auto receiverConnection = listener->accept(
          nullptr,
          {"process-asynchronous-delivery-evoked-server", 0xA521U},
          [](std::wstring) {},
          [](std::wstring) { return false; });
      ProcessTransportSession receiver(receiverConnection);
      auto receiverHandler = service.handlerFor(receiver);
      serveExpected(
          receiver, receiverHandler,
          TransportServiceOperation::join_federation_execution,
          "The HLA_EVOKED process server lost receiver Join.");

      for (auto const [session, handler, operation, description] : {
               std::tuple{
                   &sender, &senderHandler,
                   TransportServiceOperation::get_interaction_class_handle,
                   "The HLA_EVOKED process server lost sender class lookup."},
               std::tuple{
                   &sender, &senderHandler,
                   TransportServiceOperation::get_parameter_handle,
                   "The HLA_EVOKED process server lost sender parameter lookup."},
               std::tuple{
                   &receiver, &receiverHandler,
                   TransportServiceOperation::get_interaction_class_handle,
                   "The HLA_EVOKED process server lost receiver class lookup."},
               std::tuple{
                   &receiver, &receiverHandler,
                   TransportServiceOperation::get_parameter_handle,
                   "The HLA_EVOKED process server lost receiver parameter lookup."},
               std::tuple{
                   &sender, &senderHandler,
                   TransportServiceOperation::publish_interaction_class,
                   "The HLA_EVOKED process server lost interaction publication."},
               std::tuple{
                   &receiver, &receiverHandler,
                   TransportServiceOperation::subscribe_interaction_class,
                   "The HLA_EVOKED process server lost interaction subscription."}}) {
        serveExpected(*session, *handler, operation, description);
      }

      serveExpected(
          sender, senderHandler,
          TransportServiceOperation::enable_time_regulation,
          "The HLA_EVOKED process server lost Enable Time Regulation.");
      umbra::test::serveExpectedSuccessfulVoidInvocationReport(
          serveExpected, sender, senderHandler);
      serveExpected(
          receiver, receiverHandler,
          TransportServiceOperation::enable_time_constrained,
          "The HLA_EVOKED process server lost Enable Time Constrained.");
      umbra::test::serveExpectedSuccessfulVoidInvocationReport(
          serveExpected, receiver, receiverHandler);
      serveExpected(
          receiver, receiverHandler,
          TransportServiceOperation::get_interaction_class_handle,
          "The HLA_EVOKED process server lost the constrained-enable callback fence.");

      serveExpected(
          receiver, receiverHandler,
          TransportServiceOperation::time_advance_request,
          "The HLA_EVOKED process server lost receiver TAR.");
      umbra::test::serveExpectedSuccessfulVoidInvocationReport(
          serveExpected, receiver, receiverHandler);

      for (unsigned index = 0U; index != 2U; ++index) {
        serveExpected(
            sender, senderHandler,
            TransportServiceOperation::send_interaction,
            "The HLA_EVOKED process server lost Send Interaction.");
        serveExpected(
            sender, senderHandler,
            TransportServiceOperation::report_successful_void_service_invocation,
            "The HLA_EVOKED process server lost the Send report.");
        serveExpected(
            receiver, receiverHandler,
            TransportServiceOperation::get_interaction_class_handle,
            "The HLA_EVOKED process server lost the receive-order callback fence.");
        if (index == 0U) {
          serveExpected(
              sender, senderHandler,
              TransportServiceOperation::time_advance_request,
              "The HLA_EVOKED process server lost sender TAR.");
          umbra::test::serveExpectedSuccessfulVoidInvocationReport(
              serveExpected, sender, senderHandler);
          serveExpected(
              receiver, receiverHandler,
              TransportServiceOperation::get_interaction_class_handle,
              "The HLA_EVOKED process server lost the grant callback fence.");
        } else {
          umbra::test::serveExpectedSuccessfulVoidInvocationReport(
              serveExpected, receiver, receiverHandler);
          umbra::test::serveExpectedSuccessfulVoidInvocationReport(
              serveExpected, receiver, receiverHandler);
        }
      }

      serveExpected(
          sender, senderHandler,
          TransportServiceOperation::send_interaction,
          "The HLA_EVOKED process server lost post-disable Send Interaction.");
      serveExpected(
          sender, senderHandler,
          TransportServiceOperation::report_successful_void_service_invocation,
          "The HLA_EVOKED process server lost the post-disable Send report.");
      serveExpected(
          receiver, receiverHandler,
          TransportServiceOperation::get_interaction_class_handle,
          "The HLA_EVOKED process server lost the post-disable callback fence.");
      umbra::test::serveExpectedSuccessfulVoidInvocationReport(
          serveExpected, receiver, receiverHandler);

      serveExpected(
          sender, senderHandler,
          TransportServiceOperation::resign_federation_execution,
          "The HLA_EVOKED process server lost sender Resign.");
      serveExpected(
          receiver, receiverHandler,
          TransportServiceOperation::resign_federation_execution,
          "The HLA_EVOKED process server lost receiver Resign.");
      service.detach(sender);
      service.detach(receiver);
      senderConnection->close();
      receiverConnection->close();
    } catch (...) {
      serverError = std::current_exception();
    }
  });

  EvokedAsyncDeliveryFederateAmbassador senderFederate;
  EvokedAsyncDeliveryFederateAmbassador receiverFederate;
  auto senderRti = makeRti();
  auto receiverRti = makeRti();
  auto const configuration = RtiConfiguration::createConfiguration()
                                 .withConfigurationName(
                                     L"public-process-asynchronous-delivery-evoked")
                                 .withRtiAddress(
                                     L"tcp://127.0.0.1:" + std::to_wstring(port));
  std::exception_ptr clientError;
  bool senderJoined = false;
  bool receiverJoined = false;
  try {
    REQUIRE(senderRti->connect(senderFederate, HLA_IMMEDIATE, configuration)
                .addressUsed);
    senderRti->createFederationExecution(
        federationName, L"server-owned-asynchronous-delivery-evoked-fom.xml");
    senderRti->joinFederationExecution(
        senderName, L"process-asynchronous-evoked-sender-type", federationName);
    senderJoined = true;

    REQUIRE(receiverRti->connect(receiverFederate, HLA_EVOKED, configuration)
                .addressUsed);
    receiverRti->joinFederationExecution(
        receiverName, L"process-asynchronous-evoked-receiver-type", federationName);
    receiverJoined = true;

    auto const senderInteraction =
        senderRti->getInteractionClassHandle(interactionName);
    auto const senderParameter =
        senderRti->getParameterHandle(senderInteraction, parameterName);
    auto const receiverInteraction =
        receiverRti->getInteractionClassHandle(interactionName);
    static_cast<void>(receiverRti->getParameterHandle(
        receiverInteraction, parameterName));
    senderRti->publishInteractionClass(senderInteraction);
    receiverRti->subscribeInteractionClass(receiverInteraction, true);

    senderRti->enableTimeRegulation(rti1516_2025::HLAinteger64Interval(1));
    REQUIRE(senderFederate.timeRegulationEnabledCount.load(
                std::memory_order_acquire) == 1U);
    receiverRti->enableTimeConstrained();
    static_cast<void>(receiverRti->getInteractionClassHandle(interactionName));
    REQUIRE_FALSE(receiverRti->evokeCallback(0.0));
    REQUIRE(receiverFederate.timeConstrainedEnabledCount.load(
                std::memory_order_acquire) == 1U);
    receiverRti->timeAdvanceRequest(rti1516_2025::HLAinteger64Time(10));

    std::array<std::uint8_t, 1U> const value{0x01U};
    std::array<std::uint8_t, 1U> const firstTag{0x41U};
    std::array<std::uint8_t, 1U> const secondTag{0x42U};
    std::array<std::uint8_t, 1U> const thirdTag{0x43U};
    rti1516_2025::ParameterHandleValueMap values;
    values.emplace(
        senderParameter,
        rti1516_2025::VariableLengthData(value.data(), value.size()));
    senderRti->sendInteraction(
        senderInteraction,
        values,
        rti1516_2025::VariableLengthData(firstTag.data(), firstTag.size()));
    static_cast<void>(receiverRti->getInteractionClassHandle(interactionName));
    REQUIRE_FALSE(receiverRti->evokeCallback(0.0));
    CHECK(receiverFederate.receiveOrderInteractionCount.load(
              std::memory_order_acquire) == 1U);

    senderRti->timeAdvanceRequest(rti1516_2025::HLAinteger64Time(10));
    REQUIRE(senderFederate.timeAdvanceGrantCount.load(
                std::memory_order_acquire) == 1U);
    static_cast<void>(receiverRti->getInteractionClassHandle(interactionName));
    REQUIRE_FALSE(receiverRti->evokeCallback(0.0));
    REQUIRE(receiverFederate.timeAdvanceGrantCount.load(
                std::memory_order_acquire) == 1U);

    senderRti->sendInteraction(
        senderInteraction,
        values,
        rti1516_2025::VariableLengthData(secondTag.data(), secondTag.size()));
    static_cast<void>(receiverRti->getInteractionClassHandle(interactionName));
    REQUIRE_FALSE(receiverRti->evokeCallback(0.0));
    CHECK(receiverFederate.receiveOrderInteractionCount.load(
              std::memory_order_acquire) == 1U);

    receiverRti->enableAsynchronousDelivery();
    REQUIRE_FALSE(receiverRti->evokeCallback(0.0));
    CHECK(receiverFederate.receiveOrderInteractionCount.load(
              std::memory_order_acquire) == 2U);
    receiverRti->disableAsynchronousDelivery();

    senderRti->sendInteraction(
        senderInteraction,
        values,
        rti1516_2025::VariableLengthData(thirdTag.data(), thirdTag.size()));
    static_cast<void>(receiverRti->getInteractionClassHandle(interactionName));
    REQUIRE_FALSE(receiverRti->evokeCallback(0.0));
    CHECK(receiverFederate.receiveOrderInteractionCount.load(
              std::memory_order_acquire) == 2U);

    receiverRti->enableAsynchronousDelivery();
    REQUIRE_FALSE(receiverRti->evokeCallback(0.0));
    CHECK(receiverFederate.receiveOrderInteractionCount.load(
              std::memory_order_acquire) == 3U);

    senderRti->resignFederationExecution(NO_ACTION);
    senderJoined = false;
    receiverRti->resignFederationExecution(NO_ACTION);
    receiverJoined = false;
    senderRti->disconnect();
    receiverRti->disconnect();
  } catch (...) {
    clientError = std::current_exception();
    if (senderJoined) {
      try {
        senderRti->resignFederationExecution(NO_ACTION);
      } catch (...) {
      }
    }
    if (receiverJoined) {
      try {
        receiverRti->resignFederationExecution(NO_ACTION);
      } catch (...) {
      }
    }
    try {
      senderRti->disconnect();
    } catch (...) {
    }
    try {
      receiverRti->disconnect();
    } catch (...) {
    }
  }
  listener.reset();
  if (server.joinable()) {
    server.join();
  }
  if (clientError) {
    std::rethrow_exception(clientError);
  }
  REQUIRE_FALSE(serverError);
}

#endif
