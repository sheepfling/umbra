#include <catch2/catch_test_macros.hpp>

#include <array>
#include <atomic>
#include <cstdint>
#include <exception>
#include <memory>
#include <stdexcept>
#include <string>
#include <thread>
#include <vector>

#include <RTI/NullFederateAmbassador.h>
#include <RTI/RTI1516.h>
#include <RTI/time/HLAinteger64Interval.h>
#include <RTI/time/HLAinteger64Time.h>

#include "internal/federation/federation_registry.hpp"
#include "internal/federation/process_federation_service.hpp"
#include "internal/federation/process_transport.hpp"
#include "internal/federation/process_transport_session.hpp"
#include "internal/handles/attribute_handle.hpp"
#include "internal/handles/object_class_handle.hpp"
#include "ieee1516_2025_process_fom_test_support.hpp"
#include "process_public_service_fixture.hpp"

#if defined(UMBRA_ENABLE_EMBEDDED_FEDERATION_MANAGEMENT)
namespace {

using rti1516_2025::AttributeHandleValueMap;
using rti1516_2025::HLA_EVOKED;
using rti1516_2025::NO_ACTION;
using rti1516_2025::NullFederateAmbassador;
using rti1516_2025::RtiConfiguration;
using rti1516_2025::VariableLengthData;
using umbra::detail::EmbeddedFederationRegistry;
using umbra::detail::ProcessFederationService;
using umbra::detail::ProcessFederationServiceOptions;
using umbra::detail::ProcessTransportListener;
using umbra::detail::ProcessTransportSession;
using umbra::detail::TransportServiceMessage;
using umbra::detail::TransportServiceOperation;
using umbra::test::process_fom_support_2025::composedProcessDefinition;

std::unique_ptr<rti1516_2025::RTIambassador> makeRti() {
  rti1516_2025::RTIambassadorFactory factory;
  return factory.createRTIambassador();
}

TEST_CASE(
    "RTIambassadors preserve FIFO order when a middle timestamped process attribute is retracted before the callback",
    "[integration][foundation][object-management][time-management][time-advance]"
    "[transport][process-boundary][public-endpoint][multi-federate][tso][retraction]"
    "[timestamped-process-attribute-update][process-tso-attribute-retraction-ordering]"
    "[rti.service.update-attribute-values][rti.service.retract]"
    "[rti.service.enable-time-regulation][rti.service.enable-time-constrained]"
    "[rti.service.time-advance-request][federate.callback.reflect-attribute-values]"
    "[federate.callback.time-advance-grant]") {
  class RecordingFederateAmbassador final : public NullFederateAmbassador {
   public:
    void discoverObjectInstance(
        rti1516_2025::ObjectInstanceHandle const& objectInstance,
        rti1516_2025::ObjectClassHandle const&,
        std::wstring const&,
        rti1516_2025::FederateHandle const&) override {
      discoveredObjectInstance = objectInstance;
      discovered = true;
    }

    void reflectAttributeValues(
        rti1516_2025::ObjectInstanceHandle const& objectInstance,
        rti1516_2025::AttributeHandleValueMap const& attributeValues,
        rti1516_2025::VariableLengthData const&,
        rti1516_2025::TransportationTypeHandle const&,
        rti1516_2025::FederateHandle const& producingFederate,
        rti1516_2025::RegionHandleSet const*,
        rti1516_2025::LogicalTime const& time,
        rti1516_2025::OrderType sentOrder,
        rti1516_2025::OrderType receivedOrder,
        rti1516_2025::MessageRetractionHandle const* optionalRetraction) override {
      ++reflectedCount;
      reflectedObjectInstance = objectInstance;
      reflectedAttributeCounts.push_back(attributeValues.size());
      reflectedProducingFederate = producingFederate;
      reflectedSentOrders.push_back(sentOrder);
      reflectedReceivedOrders.push_back(receivedOrder);
      reflectedHasRetraction.push_back(
          optionalRetraction != nullptr && optionalRetraction->isValid());
      auto const* integerTime =
          dynamic_cast<rti1516_2025::HLAinteger64Time const*>(&time);
      REQUIRE(integerTime != nullptr);
      reflectedTimestampValues.push_back(integerTime->getTime());
      callbackOrder.push_back('R');
    }

    void timeRegulationEnabled(rti1516_2025::LogicalTime const&) override {
      ++timeRegulationEnabledCount;
    }

    void timeConstrainedEnabled(rti1516_2025::LogicalTime const&) override {
      ++timeConstrainedEnabledCount;
    }

    void timeAdvanceGrant(rti1516_2025::LogicalTime const& time) override {
      ++timeAdvanceGrantCount;
      callbackOrder.push_back('G');
      auto const* integerTime =
          dynamic_cast<rti1516_2025::HLAinteger64Time const*>(&time);
      REQUIRE(integerTime != nullptr);
      timeAdvanceGrantValue = integerTime->getTime();
    }

    bool discovered = false;
    rti1516_2025::ObjectInstanceHandle discoveredObjectInstance;
    std::size_t reflectedCount = 0U;
    rti1516_2025::ObjectInstanceHandle reflectedObjectInstance;
    std::vector<std::size_t> reflectedAttributeCounts;
    rti1516_2025::FederateHandle reflectedProducingFederate;
    std::vector<rti1516_2025::OrderType> reflectedSentOrders;
    std::vector<rti1516_2025::OrderType> reflectedReceivedOrders;
    std::vector<bool> reflectedHasRetraction;
    std::vector<std::int64_t> reflectedTimestampValues;
    std::size_t timeRegulationEnabledCount = 0U;
    std::size_t timeConstrainedEnabledCount = 0U;
    std::size_t timeAdvanceGrantCount = 0U;
    std::int64_t timeAdvanceGrantValue = -1;
    std::vector<char> callbackOrder;
  } senderFederate, receiverFederate;

  constexpr wchar_t const* federationName =
      L"process-timestamped-attribute-retraction-ordering-execution";
  constexpr wchar_t const* senderName =
      L"process-timestamped-attribute-retraction-ordering-sender";
  constexpr wchar_t const* receiverName =
      L"process-timestamped-attribute-retraction-ordering-receiver";
  constexpr wchar_t const* objectClassName = L"HLAobjectRoot.Employee";
  constexpr wchar_t const* attributeName = L"Name";

  auto listener = ProcessTransportListener::listen({"127.0.0.1", 0U});
  REQUIRE(listener);
  auto const port = listener->address().port;
  REQUIRE(port != 0U);
  std::atomic_uint64_t expectedObjectClass{0U};
  std::atomic_uint64_t expectedAttribute{0U};
  std::exception_ptr serverError;
  std::thread server([&] {
    try {
      EmbeddedFederationRegistry registry;
      ProcessFederationService service(
          registry, composedProcessDefinition(), ProcessFederationServiceOptions{});
      auto senderConnection = listener->accept(
          nullptr,
          {"process-timestamped-attribute-retraction-ordering-server", 0x9350U},
          [](std::wstring) {},
          [](std::wstring) { return false; });
      ProcessTransportSession sender(senderConnection);
      auto const senderHandler = service.handlerFor(sender);
      auto serveExpected = [&](ProcessTransportSession& session,
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

      serveExpected(
          sender,
          senderHandler,
          TransportServiceOperation::create_federation_execution,
          "The process ordering server lost Create.");
      auto const objectClass = registry.objectClassHandleFor(
          federationName, "HLAobjectRoot.Employee");
      auto const attribute = registry.attributeHandleFor(
          federationName, "HLAobjectRoot.Employee", "Name");
      if (!objectClass || !attribute) {
        throw std::runtime_error(
            "The process ordering server could not resolve its FOM handles.");
      }
      expectedObjectClass.store(*objectClass, std::memory_order_release);
      expectedAttribute.store(*attribute, std::memory_order_release);
      serveExpected(
          sender,
          senderHandler,
          TransportServiceOperation::join_federation_execution,
          "The process ordering server lost sender Join.");

      auto receiverConnection = listener->accept(
          nullptr,
          {"process-timestamped-attribute-retraction-ordering-server", 0x9351U},
          [](std::wstring) {},
          [](std::wstring) { return false; });
      ProcessTransportSession receiver(receiverConnection);
      auto const receiverHandler = service.handlerFor(receiver);
      serveExpected(
          receiver,
          receiverHandler,
          TransportServiceOperation::join_federation_execution,
          "The process ordering server lost receiver Join.");
      serveExpected(
          sender,
          senderHandler,
          TransportServiceOperation::get_object_class_handle,
          "The process ordering server lost sender object lookup.");
      serveExpected(
          receiver,
          receiverHandler,
          TransportServiceOperation::get_object_class_handle,
          "The process ordering server lost receiver object lookup.");
      serveExpected(
          sender,
          senderHandler,
          TransportServiceOperation::get_attribute_handle,
          "The process ordering server lost sender attribute lookup.");
      serveExpected(
          receiver,
          receiverHandler,
          TransportServiceOperation::get_attribute_handle,
          "The process ordering server lost receiver attribute lookup.");
      serveExpected(
          sender,
          senderHandler,
          TransportServiceOperation::publish_object_class_attributes,
          "The process ordering server lost Publish.");
      serveExpected(
          receiver,
          receiverHandler,
          TransportServiceOperation::subscribe_object_class_attributes,
          "The process ordering server lost Subscribe.");
      serveExpected(
          sender,
          senderHandler,
          TransportServiceOperation::enable_time_regulation,
          "The process ordering server lost Enable Time Regulation.");
      serveExpected(
          receiver,
          receiverHandler,
          TransportServiceOperation::enable_time_constrained,
          "The process ordering server lost Enable Time Constrained.");
      serveExpected(
          sender,
          senderHandler,
          TransportServiceOperation::register_object_instance,
          "The process ordering server lost Register.");
      serveExpected(
          receiver,
          receiverHandler,
          TransportServiceOperation::receive_interaction,
          "The process ordering server lost discovery polling.");
      serveExpected(
          sender,
          senderHandler,
          TransportServiceOperation::update_attribute_values,
          "The process ordering server lost timestamp-5 Update.");
      serveExpected(
          sender,
          senderHandler,
          TransportServiceOperation::update_attribute_values,
          "The process ordering server lost timestamp-6 Update.");
      serveExpected(
          sender,
          senderHandler,
          TransportServiceOperation::update_attribute_values,
          "The process ordering server lost timestamp-7 Update.");
      serveExpected(
          sender,
          senderHandler,
          TransportServiceOperation::retract,
          "The process ordering server lost middle Retract.");
      serveExpected(
          receiver,
          receiverHandler,
          TransportServiceOperation::time_advance_request,
          "The process ordering server lost receiver TAR at timestamp 5.");
      serveExpected(
          sender,
          senderHandler,
          TransportServiceOperation::time_advance_request,
          "The process ordering server lost sender intermediate TAR.");
      serveExpected(
          sender,
          senderHandler,
          TransportServiceOperation::time_advance_request,
          "The process ordering server lost sender timestamp-5 TAR.");
      std::size_t receiverPolls = 0U;
      std::size_t receiverAcknowledgements = 0U;
      auto serveReceiverUntilAcknowledgements =
          [&](std::size_t target, char const* description) {
            while (receiverAcknowledgements < target && receiverPolls != 8U) {
              if (!umbra::test::servePrimaryProcessRequest(
                      receiver, receiverHandler,
                      [&](TransportServiceMessage const& request) {
                        if (request.operation ==
                            TransportServiceOperation::receive_interaction) {
                          ++receiverPolls;
                          return receiverHandler(request);
                        }
                        if (request.operation ==
                            TransportServiceOperation::acknowledge_tso_delivery) {
                          ++receiverAcknowledgements;
                          return receiverHandler(request);
                        }
                        throw std::runtime_error(
                            std::string(
                                "The process ordering server received an unexpected receiver callback operation ") +
                            std::to_string(static_cast<unsigned>(request.operation)) +
                            ".");
                      })) {
                throw std::runtime_error(
                    "The process ordering server lost a receiver callback operation.");
              }
            }
            if (receiverAcknowledgements != target) {
              throw std::runtime_error(description);
            }
      };
      serveReceiverUntilAcknowledgements(
          1U,
          "The process ordering server did not receive the timestamp-5 acknowledgement.");
      serveExpected(
          receiver,
          receiverHandler,
          TransportServiceOperation::time_advance_request,
          "The process ordering server lost receiver TAR at timestamp 7.");
      serveExpected(
          sender,
          senderHandler,
          TransportServiceOperation::time_advance_request,
          "The process ordering server lost sender final TAR.");
      serveReceiverUntilAcknowledgements(
          2U,
          "The process ordering server did not receive the timestamp-7 acknowledgement.");
      serveExpected(
          sender,
          senderHandler,
          TransportServiceOperation::resign_federation_execution,
          "The process ordering server lost sender Resign.");
      serveExpected(
          receiver,
          receiverHandler,
          TransportServiceOperation::resign_federation_execution,
          "The process ordering server lost receiver Resign.");
      service.detach(sender);
      service.detach(receiver);
      senderConnection->close();
      receiverConnection->close();
    } catch (...) {
      serverError = std::current_exception();
    }
  });

  auto senderRti = makeRti();
  auto receiverRti = makeRti();
  auto senderConfiguration = RtiConfiguration::createConfiguration()
                                 .withConfigurationName(
                                     L"process-timestamped-attribute-retraction-ordering-sender-client")
                                 .withRtiAddress(
                                     L"tcp://127.0.0.1:" + std::to_wstring(port));
  auto receiverConfiguration = RtiConfiguration::createConfiguration()
                                   .withConfigurationName(
                                       L"process-timestamped-attribute-retraction-ordering-receiver-client")
                                   .withRtiAddress(
                                       L"tcp://127.0.0.1:" + std::to_wstring(port));
  std::exception_ptr clientError;
  bool senderJoined = false;
  bool receiverJoined = false;
  try {
    REQUIRE(senderRti->connect(
                senderFederate, HLA_EVOKED, senderConfiguration)
                .addressUsed);
    REQUIRE_NOTHROW(senderRti->createFederationExecution(
        federationName, L"server-owned-fom.xml"));
    auto const senderHandle = senderRti->joinFederationExecution(
        senderName,
        L"process-timestamped-attribute-retraction-ordering-type",
        federationName);
    REQUIRE(senderHandle.isValid());
    senderJoined = true;
    REQUIRE(receiverRti->connect(
                receiverFederate, HLA_EVOKED, receiverConfiguration)
                .addressUsed);
    auto const receiverHandle = receiverRti->joinFederationExecution(
        receiverName,
        L"process-timestamped-attribute-retraction-ordering-type",
        federationName);
    REQUIRE(receiverHandle.isValid());
    receiverJoined = true;

    auto const senderObjectClass = senderRti->getObjectClassHandle(objectClassName);
    auto const receiverObjectClass = receiverRti->getObjectClassHandle(objectClassName);
    auto const senderAttribute = senderRti->getAttributeHandle(
        senderObjectClass, attributeName);
    auto const receiverAttribute = receiverRti->getAttributeHandle(
        receiverObjectClass, attributeName);
    REQUIRE(senderObjectClass == receiverObjectClass);
    REQUIRE(senderObjectClass ==
            rti1516_2025::umbra_binding_detail::makeObjectClassHandle(
                expectedObjectClass.load(std::memory_order_acquire)));
    REQUIRE(senderAttribute == receiverAttribute);
    REQUIRE(senderAttribute ==
            rti1516_2025::umbra_binding_detail::makeAttributeHandle(
                expectedAttribute.load(std::memory_order_acquire)));
    REQUIRE_NOTHROW(senderRti->publishObjectClassAttributes(
        senderObjectClass, {senderAttribute}));
    REQUIRE_NOTHROW(receiverRti->subscribeObjectClassAttributes(
        receiverObjectClass, {receiverAttribute}, true));

    REQUIRE_NOTHROW(senderRti->enableTimeRegulation(
        rti1516_2025::HLAinteger64Interval(1)));
    REQUIRE(senderFederate.timeRegulationEnabledCount == 0U);
    REQUIRE_FALSE(senderRti->evokeCallback(0.0));
    REQUIRE(senderFederate.timeRegulationEnabledCount == 1U);
    REQUIRE_NOTHROW(receiverRti->enableTimeConstrained());
    REQUIRE(receiverFederate.timeConstrainedEnabledCount == 0U);
    REQUIRE_FALSE(receiverRti->evokeCallback(0.0));
    REQUIRE(receiverFederate.timeConstrainedEnabledCount == 1U);

    auto const objectInstance = senderRti->registerObjectInstance(senderObjectClass);
    REQUIRE(objectInstance.isValid());
    REQUIRE_FALSE(receiverFederate.discovered);
    REQUIRE_FALSE(receiverRti->evokeCallback(0.0));
    REQUIRE(receiverFederate.discovered);
    REQUIRE(receiverFederate.discoveredObjectInstance == objectInstance);

    AttributeHandleValueMap attributeValues;
    std::array<std::uint8_t, 1U> valueBytes{0U};
    attributeValues.emplace(
        senderAttribute,
        VariableLengthData(valueBytes.data(), valueBytes.size()));
    std::array<std::uint8_t, 1U> tagBytes{0U};
    auto sendAt = [&](std::uint8_t value, std::uint8_t tag, std::int64_t time) {
      valueBytes[0] = value;
      tagBytes[0] = tag;
      auto const retraction = senderRti->updateAttributeValues(
          objectInstance,
          attributeValues,
          VariableLengthData(tagBytes.data(), tagBytes.size()),
          rti1516_2025::HLAinteger64Time(time));
      REQUIRE(retraction.isValid());
      return retraction;
    };
    auto const first = sendAt(0x35U, 0xA5U, 5);
    auto const middle = sendAt(0x36U, 0xA6U, 6);
    auto const last = sendAt(0x37U, 0xA7U, 7);
    REQUIRE(first.isValid());
    REQUIRE(middle.isValid());
    REQUIRE(last.isValid());
    REQUIRE_NOTHROW(senderRti->retract(middle));
    REQUIRE(receiverFederate.reflectedCount == 0U);
    REQUIRE_NOTHROW(receiverRti->timeAdvanceRequest(
        rti1516_2025::HLAinteger64Time(5)));
    REQUIRE(receiverFederate.timeAdvanceGrantCount == 0U);
    REQUIRE_NOTHROW(senderRti->timeAdvanceRequest(
        rti1516_2025::HLAinteger64Time(5)));
    REQUIRE(senderFederate.timeAdvanceGrantCount == 0U);
    REQUIRE_FALSE(senderRti->evokeCallback(0.0));
    REQUIRE(senderFederate.timeAdvanceGrantCount == 1U);
    // The sender requested time 5, so its own grant is 5.  The constrained
    // receiver reaches the same timestamp independently; the retracted
    // middle update does not lower the sender's TAR target.
    REQUIRE(senderFederate.timeAdvanceGrantValue == 5);
    REQUIRE_NOTHROW(senderRti->timeAdvanceRequest(
        rti1516_2025::HLAinteger64Time(5)));
    REQUIRE(senderFederate.timeAdvanceGrantCount == 1U);
    REQUIRE_FALSE(senderRti->evokeCallback(0.0));
    REQUIRE(senderFederate.timeAdvanceGrantCount == 2U);
    REQUIRE(senderFederate.timeAdvanceGrantValue == 5);

    // A constrained grant is released at the earliest queued TSO boundary.
    // The first callback fence therefore admits timestamp 5; the later
    // request to 7 admits timestamp 7 after the middle update is retracted.
    for (int pass = 0; pass != 8 &&
         (receiverFederate.reflectedCount < 1U ||
          receiverFederate.timeAdvanceGrantCount < 1U);
         ++pass) {
      static_cast<void>(receiverRti->evokeCallback(0.0));
    }
    REQUIRE(receiverFederate.reflectedCount == 1U);
    REQUIRE(receiverFederate.timeAdvanceGrantCount == 1U);
    REQUIRE(receiverFederate.timeAdvanceGrantValue == 5);
    REQUIRE(receiverFederate.reflectedTimestampValues ==
            std::vector<std::int64_t>{5});
    REQUIRE(receiverFederate.callbackOrder == std::vector<char>{'R', 'G'});

    REQUIRE_NOTHROW(receiverRti->timeAdvanceRequest(
        rti1516_2025::HLAinteger64Time(7)));
    REQUIRE(receiverFederate.timeAdvanceGrantCount == 1U);
    REQUIRE_NOTHROW(senderRti->timeAdvanceRequest(
        rti1516_2025::HLAinteger64Time(7)));
    REQUIRE(senderFederate.timeAdvanceGrantCount == 2U);
    REQUIRE_FALSE(senderRti->evokeCallback(0.0));
    REQUIRE(senderFederate.timeAdvanceGrantCount == 3U);
    REQUIRE(senderFederate.timeAdvanceGrantValue == 7);
    for (int pass = 0; pass != 8 &&
         (receiverFederate.reflectedCount < 2U ||
          receiverFederate.timeAdvanceGrantCount < 2U);
         ++pass) {
      static_cast<void>(receiverRti->evokeCallback(0.0));
    }
    REQUIRE(receiverFederate.reflectedCount == 2U);
    REQUIRE(receiverFederate.timeAdvanceGrantCount == 2U);
    REQUIRE(receiverFederate.timeAdvanceGrantValue == 7);
    REQUIRE(receiverFederate.reflectedTimestampValues ==
            std::vector<std::int64_t>{5, 7});
    REQUIRE(receiverFederate.callbackOrder ==
            std::vector<char>{'R', 'G', 'R', 'G'});
    REQUIRE(receiverFederate.reflectedAttributeCounts ==
            std::vector<std::size_t>{1U, 1U});
    REQUIRE(receiverFederate.reflectedSentOrders ==
            std::vector<rti1516_2025::OrderType>{rti1516_2025::TIMESTAMP,
                                                  rti1516_2025::TIMESTAMP});
    REQUIRE(receiverFederate.reflectedReceivedOrders ==
            std::vector<rti1516_2025::OrderType>{rti1516_2025::TIMESTAMP,
                                                  rti1516_2025::TIMESTAMP});
    REQUIRE(receiverFederate.reflectedHasRetraction ==
            std::vector<bool>{true, true});
    REQUIRE(receiverFederate.reflectedObjectInstance == objectInstance);
    REQUIRE(receiverFederate.reflectedProducingFederate == senderHandle);

    REQUIRE_NOTHROW(senderRti->resignFederationExecution(
        rti1516_2025::UNCONDITIONALLY_DIVEST_ATTRIBUTES));
    senderJoined = false;
    REQUIRE_NOTHROW(receiverRti->resignFederationExecution(NO_ACTION));
    receiverJoined = false;
    REQUIRE_NOTHROW(senderRti->disconnect());
    REQUIRE_NOTHROW(receiverRti->disconnect());
  } catch (...) {
    clientError = std::current_exception();
    if (receiverJoined) {
      try {
        receiverRti->resignFederationExecution(NO_ACTION);
      } catch (...) {
      }
    }
    if (senderJoined) {
      try {
        senderRti->resignFederationExecution(NO_ACTION);
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
  if (listener) {
    listener.reset();
  }
  if (server.joinable()) {
    server.join();
  }
  if (serverError) {
    try {
      std::rethrow_exception(serverError);
    } catch (std::exception const& error) {
      FAIL_CHECK(std::string("process ordering server: ") + error.what());
    } catch (...) {
      FAIL_CHECK("process ordering server failed with an unknown exception");
    }
  }
  if (clientError) {
    std::rethrow_exception(clientError);
  }
  REQUIRE_FALSE(serverError);
}



}  // namespace
#endif
