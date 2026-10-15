#include <catch2/catch_test_macros.hpp>

#include <array>
#include <atomic>
#include <chrono>
#include <condition_variable>
#include <cstdint>
#include <exception>
#include <memory>
#include <mutex>
#include <stdexcept>
#include <string>
#include <thread>

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

enum class RetractedObjectEvent { attribute_update, object_removal };

class RetractionGateFederate final : public NullFederateAmbassador {
 public:
  void discoverObjectInstance(
      ObjectInstanceHandle const&, ObjectClassHandle const&,
      std::wstring const&, FederateHandle const&) override {
    discoveryCount.fetch_add(1U, std::memory_order_release);
    callbackChanged.notify_all();
  }

  void reflectAttributeValues(
      ObjectInstanceHandle const&, AttributeHandleValueMap const&,
      VariableLengthData const&, TransportationTypeHandle const&,
      FederateHandle const&, RegionHandleSet const*, LogicalTime const&,
      OrderType, OrderType, MessageRetractionHandle const*) override {
    reflected.fetch_add(1U, std::memory_order_release);
  }

  void removeObjectInstance(
      ObjectInstanceHandle const&, VariableLengthData const&,
      FederateHandle const&, LogicalTime const&, OrderType, OrderType,
      MessageRetractionHandle const*) override {
    removed.fetch_add(1U, std::memory_order_release);
  }

  void requestRetraction(MessageRetractionHandle const&) override {
    retractionRequested.fetch_add(1U, std::memory_order_release);
  }

  bool waitForDiscoveries(unsigned const count) {
    std::unique_lock lock(callbackMutex);
    return callbackChanged.wait_for(
        lock, std::chrono::seconds(2), [&] {
          return discoveryCount.load(std::memory_order_acquire) >= count;
        });
  }

  std::mutex callbackMutex;
  std::condition_variable callbackChanged;
  std::atomic_uint discoveryCount{0U};
  std::atomic_uint reflected{0U};
  std::atomic_uint removed{0U};
  std::atomic_uint retractionRequested{0U};
};

void verifyRetractedObjectEventSuppressed(RetractedObjectEvent eventKind) {
  constexpr wchar_t const* federationName = L"process-retracted-object-gate";
  constexpr wchar_t const* objectClassName = L"HLAobjectRoot.Employee";
  constexpr wchar_t const* attributeName = L"Name";

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
          nullptr, {"retracted-object-gate-server", 0x9771U},
          [](std::wstring) {}, [](std::wstring) { return false; });
      ProcessTransportSession sender(senderConnection);
      auto senderHandler = service.handlerFor(sender);
      auto serveExpected = [](ProcessTransportSession& session,
                              auto const& handler,
                              TransportServiceOperation operation,
                              char const* description =
                                  "Missing operation in retracted-object gate fixture") {
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
                    TransportServiceOperation::create_federation_execution);
      serveExpected(sender, senderHandler,
                    TransportServiceOperation::join_federation_execution);
      auto receiverConnection = listener->accept(
          nullptr, {"retracted-object-gate-server", 0x9772U},
          [](std::wstring) {}, [](std::wstring) { return false; });
      ProcessTransportSession receiver(receiverConnection);
      auto receiverHandler = service.handlerFor(receiver);
      serveExpected(receiver, receiverHandler,
                    TransportServiceOperation::join_federation_execution);

      serveExpected(sender, senderHandler,
                    TransportServiceOperation::get_object_class_handle);
      serveExpected(receiver, receiverHandler,
                    TransportServiceOperation::get_object_class_handle);
      serveExpected(sender, senderHandler,
                    TransportServiceOperation::get_attribute_handle);
      serveExpected(receiver, receiverHandler,
                    TransportServiceOperation::get_attribute_handle);
      serveExpected(sender, senderHandler,
                    TransportServiceOperation::publish_object_class_attributes);
      serveExpected(receiver, receiverHandler,
                    TransportServiceOperation::subscribe_object_class_attributes);
      serveExpected(sender, senderHandler,
                    TransportServiceOperation::register_object_instance);
      serveExpected(receiver, receiverHandler,
                    TransportServiceOperation::get_object_class_handle);

      serveExpected(sender, senderHandler,
                    TransportServiceOperation::enable_time_regulation);
      umbra::test::serveExpectedSuccessfulVoidInvocationReport(
          serveExpected, sender, senderHandler);
      serveExpected(sender, senderHandler,
                    eventKind == RetractedObjectEvent::attribute_update
                        ? TransportServiceOperation::update_attribute_values
                        : TransportServiceOperation::delete_object_instance);
      // A receiver-side public lookup fences admission of the pushed event
      // while callback delivery remains disabled.
      serveExpected(receiver, receiverHandler,
                    TransportServiceOperation::get_object_class_handle);
      serveExpected(sender, senderHandler, TransportServiceOperation::retract);

      bool sawLookup = false;
      bool sawAcknowledgement = false;
      for (int operation = 0; operation != 2; ++operation) {
        if (!umbra::test::servePrimaryProcessRequest(
                receiver, receiverHandler,
                [&](TransportServiceMessage const& request) {
                  if (request.operation ==
                          TransportServiceOperation::get_object_class_handle &&
                      !sawLookup) {
                    sawLookup = true;
                  } else if (request.operation ==
                                 TransportServiceOperation::acknowledge_tso_delivery &&
                             !sawAcknowledgement) {
                    sawAcknowledgement = true;
                  } else {
                    throw std::runtime_error(
                        "Unexpected receiver operation after object-event Retract");
                  }
                  return receiverHandler(request);
                })) {
          throw std::runtime_error(
              "Missing receiver fence after object-event Retract");
        }
      }
      if (!sawLookup || !sawAcknowledgement) {
        throw std::runtime_error(
            "Missing retracted object-event completion or receiver fence");
      }
      serveExpected(sender, senderHandler,
                    TransportServiceOperation::register_object_instance);
      serveExpected(receiver, receiverHandler,
                    TransportServiceOperation::get_object_class_handle);
      serveExpected(sender, senderHandler,
                    TransportServiceOperation::resign_federation_execution);
      serveExpected(receiver, receiverHandler,
                    TransportServiceOperation::resign_federation_execution);
      service.detach(sender);
      service.detach(receiver);
      senderConnection->close();
      receiverConnection->close();
    } catch (...) {
      serverError = std::current_exception();
    }
  });

  RetractionGateFederate senderFederate;
  RetractionGateFederate receiverFederate;
  RTIambassadorFactory factory;
  auto senderRti = factory.createRTIambassador();
  auto receiverRti = factory.createRTIambassador();
  auto configuration = RtiConfiguration::createConfiguration()
                           .withConfigurationName(L"retracted-object-gate")
                           .withRtiAddress(
                               L"tcp://127.0.0.1:" + std::to_wstring(port));
  std::exception_ptr clientError;
  bool senderJoined = false;
  bool receiverJoined = false;
  try {
    REQUIRE(senderRti->connect(senderFederate, HLA_IMMEDIATE, configuration)
                .addressUsed);
    senderRti->createFederationExecution(
        federationName, L"server-owned-object-gate-fom.xml");
    senderRti->joinFederationExecution(
        L"object-gate-sender", L"object-gate-type", federationName);
    senderJoined = true;
    REQUIRE(receiverRti->connect(receiverFederate, HLA_IMMEDIATE, configuration)
                .addressUsed);
    receiverRti->joinFederationExecution(
        L"object-gate-receiver", L"object-gate-type", federationName);
    receiverJoined = true;

    auto const senderClass = senderRti->getObjectClassHandle(objectClassName);
    auto const receiverClass = receiverRti->getObjectClassHandle(objectClassName);
    auto const senderAttribute =
        senderRti->getAttributeHandle(senderClass, attributeName);
    auto const receiverAttribute =
        receiverRti->getAttributeHandle(receiverClass, attributeName);
    REQUIRE(senderClass == receiverClass);
    REQUIRE(senderAttribute == receiverAttribute);
    senderRti->publishObjectClassAttributes(
        senderClass, AttributeHandleSet{senderAttribute});
    receiverRti->subscribeObjectClassAttributes(
        receiverClass, AttributeHandleSet{receiverAttribute}, true);
    auto const object = senderRti->registerObjectInstance(senderClass);
    REQUIRE(object.isValid());
    static_cast<void>(receiverRti->getObjectClassHandle(objectClassName));
    REQUIRE(receiverFederate.waitForDiscoveries(1U));

    senderRti->enableTimeRegulation(HLAinteger64Interval(1));
    receiverRti->disableCallbacks();
    std::array<std::uint8_t, 1U> const data{0x52U};
    VariableLengthData const tag(data.data(), data.size());
    MessageRetractionHandle retraction;
    if (eventKind == RetractedObjectEvent::attribute_update) {
      AttributeHandleValueMap values;
      values.emplace(senderAttribute, tag);
      retraction = senderRti->updateAttributeValues(
          object, values, tag, HLAinteger64Time(5));
    } else {
      retraction = senderRti->deleteObjectInstance(
          object, tag, HLAinteger64Time(5));
    }
    REQUIRE(retraction.isValid());
    static_cast<void>(receiverRti->getObjectClassHandle(objectClassName));
    REQUIRE(receiverFederate.reflected.load(std::memory_order_acquire) == 0U);
    REQUIRE(receiverFederate.removed.load(std::memory_order_acquire) == 0U);
    senderRti->retract(retraction);
    static_cast<void>(receiverRti->getObjectClassHandle(objectClassName));
    receiverRti->enableCallbacks();
    // A later public registration supplies a positive callback completion
    // fence. The earlier pushed event and its retraction have already crossed
    // the receiver stream, so any stale queued callback would precede this
    // discovery in the immediate dispatcher.
    auto const laterObject = senderRti->registerObjectInstance(senderClass);
    REQUIRE(laterObject.isValid());
    static_cast<void>(receiverRti->getObjectClassHandle(objectClassName));
    REQUIRE(receiverFederate.waitForDiscoveries(2U));
    // Acknowledgment may already have been emitted when the queued callback
    // runs. That must never turn suppression into delivery.
    CHECK(receiverFederate.reflected.load(std::memory_order_acquire) == 0U);
    CHECK(receiverFederate.removed.load(std::memory_order_acquire) == 0U);
    CHECK(receiverFederate.retractionRequested.load(std::memory_order_acquire) == 0U);

    senderRti->resignFederationExecution(DELETE_OBJECTS);
    senderJoined = false;
    receiverRti->resignFederationExecution(NO_ACTION);
    receiverJoined = false;
    senderRti->disconnect();
    receiverRti->disconnect();
  } catch (...) {
    clientError = std::current_exception();
    if (senderJoined) {
      try { senderRti->resignFederationExecution(DELETE_OBJECTS); } catch (...) {}
    }
    if (receiverJoined) {
      try { receiverRti->resignFederationExecution(NO_ACTION); } catch (...) {}
    }
    try { senderRti->disconnect(); } catch (...) {}
    try { receiverRti->disconnect(); } catch (...) {}
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

}  // namespace

TEST_CASE(
    "RTIambassadors suppress retracted pushed object callbacks across Enable Callbacks",
    "[integration][object-management][time-management][retraction][callbacks]"
    "[process-boundary][public-endpoint][process-object-retraction-callback-gate]"
    "[rti.service.update-attribute-values][rti.service.delete-object-instance]"
    "[rti.service.retract][rti.service.disable-callbacks][rti.service.enable-callbacks]"
    "[federate.callback.reflect-attribute-values][federate.callback.remove-object-instance][2025]") {
  SECTION("HLA_IMMEDIATE timestamped attribute update") {
    verifyRetractedObjectEventSuppressed(RetractedObjectEvent::attribute_update);
  }
  SECTION("HLA_IMMEDIATE timestamped object removal") {
    verifyRetractedObjectEventSuppressed(RetractedObjectEvent::object_removal);
  }
}

#endif
