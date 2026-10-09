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

#if defined(UMBRA_ENABLE_EMBEDDED_FEDERATION_MANAGEMENT)
#include "internal/federation/federation_registry.hpp"
#include "internal/federation/process_federation_service.hpp"
#include "internal/federation/process_transport.hpp"
#include "internal/federation/process_transport_session.hpp"
#include "internal/handles/interaction_class_handle.hpp"
#include "internal/handles/parameter_handle.hpp"
#include "ieee1516_2025_connection_test_support.hpp"
#include "ieee1516_2025_process_fom_test_support.hpp"
#include "process_public_service_fixture.hpp"

namespace {
using rti1516_2025::CallbackModel;
using rti1516_2025::HLA_EVOKED;
using rti1516_2025::HLA_IMMEDIATE;
using rti1516_2025::NO_ACTION;
using rti1516_2025::NullFederateAmbassador;
using rti1516_2025::ParameterHandleValueMap;
using rti1516_2025::RTIambassador;
using rti1516_2025::RtiConfiguration;
using rti1516_2025::VariableLengthData;

using umbra::detail::EmbeddedFederationRegistry;
using umbra::detail::ProcessFederationService;
using umbra::detail::ProcessFederationServiceOptions;
using umbra::detail::ProcessTransportListener;
using umbra::detail::ProcessTransportSession;
using umbra::detail::TransportServiceMessage;
using umbra::detail::TransportServiceOperation;
using umbra::detail::TransportServiceStatus;
using umbra::test::connection_support_2025::makeRti;
using umbra::test::process_fom_support_2025::composedProcessDefinition;
}  // namespace

class RegionalInteractionTransportationFederateAmbassador final
    : public NullFederateAmbassador {
 public:
  void confirmInteractionTransportationTypeChange(
      rti1516_2025::InteractionClassHandle const& interactionClass,
      rti1516_2025::TransportationTypeHandle const& transportationType) override {
    ++changeCount;
    changedInteractionClass = interactionClass;
    changedTransportationType = transportationType;
  }

  void receiveInteraction(
      rti1516_2025::InteractionClassHandle const& interactionClass,
      rti1516_2025::ParameterHandleValueMap const& parameterValues,
      rti1516_2025::VariableLengthData const& userSuppliedTag,
      rti1516_2025::TransportationTypeHandle const& transportationType,
      rti1516_2025::FederateHandle const& producingFederate,
      rti1516_2025::RegionHandleSet const* optionalSentRegions) override {
    ++receiveCount;
    receivedInteractionClass = interactionClass;
    receivedTransportationType = transportationType;
    receivedProducingFederate = producingFederate;
    receivedParameterCount = parameterValues.size();
    receivedRegionCount = optionalSentRegions == nullptr
        ? 0U
        : optionalSentRegions->size();
    receivedOptionalRegions = optionalSentRegions != nullptr;
    receivedTag.clear();
    if (userSuppliedTag.size() != 0U) {
      auto const* first = static_cast<std::uint8_t const*>(userSuppliedTag.data());
      receivedTag.assign(first, first + userSuppliedTag.size());
    }
  }

  std::size_t changeCount = 0U;
  rti1516_2025::InteractionClassHandle changedInteractionClass;
  rti1516_2025::TransportationTypeHandle changedTransportationType;
  std::size_t receiveCount = 0U;
  rti1516_2025::InteractionClassHandle receivedInteractionClass;
  rti1516_2025::TransportationTypeHandle receivedTransportationType;
  rti1516_2025::FederateHandle receivedProducingFederate;
  std::size_t receivedParameterCount = 0U;
  std::size_t receivedRegionCount = 0U;
  bool receivedOptionalRegions = false;
  std::vector<std::uint8_t> receivedTag;
};

TEST_CASE(
    "RTIambassadors preserve a regional interaction transportation override through a configured process endpoint",
    "[integration][foundation][data-distribution-management][interaction-management][transportation][transport][process-boundary][public-endpoint][regional-interaction][process-transportation-regional-interaction-control][rti.service.request-interaction-transportation-type-change][rti.service.subscribe-interaction-class-with-regions][rti.service.send-interaction-with-regions][federate.callback.confirm-interaction-transportation-type-change][federate.callback.receive-interaction][2025]") {
  auto runScenario = [](CallbackModel callbackModel) {
    constexpr wchar_t const* federationName =
        L"public-process-transportation-regional-interaction-execution";
    constexpr wchar_t const* senderName =
        L"public-process-transportation-regional-interaction-sender";
    constexpr wchar_t const* receiverName =
        L"public-process-transportation-regional-interaction-receiver";
    constexpr wchar_t const* federateType =
        L"public-process-transportation-regional-interaction-type";
    constexpr wchar_t const* interactionNameWide =
        L"HLAinteractionRoot.CustomerTransactions.FoodServed.MainCourseServed";
    constexpr wchar_t const* parameterNameWide = L"TimelinessOk";
    constexpr wchar_t const* dimensionNameWide = L"ServerId";

    auto listener = ProcessTransportListener::listen({"127.0.0.1", 0U});
    REQUIRE(listener);
    auto const port = listener->address().port;
    REQUIRE(port != 0U);

    std::atomic_uint64_t expectedInteractionClass{0U};
    std::atomic_uint64_t expectedParameter{0U};
    std::atomic_uint64_t expectedDimension{0U};
    std::atomic_uint64_t expectedTransportation{0U};
    std::exception_ptr serverError;
    std::thread server([&] {
      try {
        EmbeddedFederationRegistry registry;
        ProcessFederationService service(
            registry, composedProcessDefinition(), ProcessFederationServiceOptions{});
        auto senderConnection = listener->accept(
            nullptr,
            {"public-process-transportation-regional-interaction-server", 0x9605U},
            [](std::wstring) {},
            [](std::wstring) { return false; });
        ProcessTransportSession sender(senderConnection);
        auto senderHandler = service.handlerFor(sender);
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
                    auto response = handler(request);
                    if (response.status != TransportServiceStatus::ok) {
                      throw std::runtime_error(description);
                    }
                    return response;
                  })) {
            throw std::runtime_error(description);
          }
        };

        serveExpected(
            sender,
            senderHandler,
            TransportServiceOperation::create_federation_execution,
            "The regional transportation process server lost Create.");
        auto const interactionClass = registry.interactionClassHandleFor(
            federationName,
            "HLAinteractionRoot.CustomerTransactions.FoodServed.MainCourseServed");
        auto const parameter = registry.parameterHandleFor(
            federationName,
            "HLAinteractionRoot.CustomerTransactions.FoodServed.MainCourseServed",
            "TimelinessOk");
        auto const dimension = registry.dimensionHandleFor(
            federationName, "ServerId");
        auto const transportation = registry.transportationTypeHandleFor(
            federationName, "HLAbestEffort");
        if (!interactionClass || !parameter || !dimension || !transportation) {
          throw std::runtime_error(
              "The regional transportation process server could not resolve its FOM handles.");
        }
        expectedInteractionClass.store(*interactionClass, std::memory_order_release);
        expectedParameter.store(*parameter, std::memory_order_release);
        expectedDimension.store(*dimension, std::memory_order_release);
        expectedTransportation.store(*transportation, std::memory_order_release);
        serveExpected(
            sender,
            senderHandler,
            TransportServiceOperation::join_federation_execution,
            "The regional transportation process server lost sender Join.");

        auto receiverConnection = listener->accept(
            nullptr,
            {"public-process-transportation-regional-interaction-server", 0x9606U},
            [](std::wstring) {},
            [](std::wstring) { return false; });
        ProcessTransportSession receiver(receiverConnection);
        auto receiverHandler = service.handlerFor(receiver);
        serveExpected(
            receiver,
            receiverHandler,
            TransportServiceOperation::join_federation_execution,
            "The regional transportation process server lost receiver Join.");

        serveExpected(
            sender,
            senderHandler,
            TransportServiceOperation::get_interaction_class_handle,
            "The regional transportation process server lost sender class lookup.");
        serveExpected(
            receiver,
            receiverHandler,
            TransportServiceOperation::get_interaction_class_handle,
            "The regional transportation process server lost receiver class lookup.");
        serveExpected(
            sender,
            senderHandler,
            TransportServiceOperation::get_parameter_handle,
            "The regional transportation process server lost sender parameter lookup.");
        serveExpected(
            receiver,
            receiverHandler,
            TransportServiceOperation::get_parameter_handle,
            "The regional transportation process server lost receiver parameter lookup.");
        serveExpected(
            sender,
            senderHandler,
            TransportServiceOperation::get_dimension_handle,
            "The regional transportation process server lost sender dimension lookup.");
        serveExpected(
            receiver,
            receiverHandler,
            TransportServiceOperation::get_dimension_handle,
            "The regional transportation process server lost receiver dimension lookup.");
        serveExpected(
            sender,
            senderHandler,
            TransportServiceOperation::get_transportation_type_handle,
            "The regional transportation process server lost sender transportation lookup.");
        serveExpected(
            sender,
            senderHandler,
            TransportServiceOperation::create_region,
            "The regional transportation process server lost sender region creation.");
        serveExpected(
            receiver,
            receiverHandler,
            TransportServiceOperation::create_region,
            "The regional transportation process server lost receiver region creation.");
        serveExpected(
            sender,
            senderHandler,
            TransportServiceOperation::set_range_bounds,
            "The regional transportation process server lost sender bounds.");
        serveExpected(
            sender,
            senderHandler,
            TransportServiceOperation::commit_region_modifications,
            "The regional transportation process server lost sender region commit.");
        serveExpected(
            receiver,
            receiverHandler,
            TransportServiceOperation::set_range_bounds,
            "The regional transportation process server lost receiver bounds.");
        serveExpected(
            receiver,
            receiverHandler,
            TransportServiceOperation::commit_region_modifications,
            "The regional transportation process server lost receiver region commit.");
        serveExpected(
            sender,
            senderHandler,
            TransportServiceOperation::publish_interaction_class,
            "The regional transportation process server lost Publish.");
        serveExpected(
            sender,
            senderHandler,
            TransportServiceOperation::request_interaction_transportation_type_change,
            "The regional transportation process server lost transportation override.");
        serveExpected(
            sender,
            senderHandler,
            TransportServiceOperation::receive_interaction,
            "The regional transportation process server lost transportation confirmation.");
        serveExpected(
            receiver,
            receiverHandler,
            TransportServiceOperation::set_convey_region_designator_sets_switch,
            "The regional transportation process server lost Convey switch set.");
        serveExpected(
            receiver,
            receiverHandler,
            TransportServiceOperation::get_convey_region_designator_sets_switch,
            "The regional transportation process server lost Convey switch query.");
        serveExpected(
            receiver,
            receiverHandler,
            TransportServiceOperation::subscribe_interaction_class_with_regions,
            "The regional transportation process server lost regional Subscribe.");
        serveExpected(
            sender,
            senderHandler,
            TransportServiceOperation::send_interaction_with_regions,
            "The regional transportation process server lost regional Send.");
        serveExpected(
            sender,
            senderHandler,
            TransportServiceOperation::report_successful_void_service_invocation,
            "The regional transportation process server lost the successful regional Send report.");
        serveExpected(
            receiver,
            receiverHandler,
            TransportServiceOperation::receive_interaction,
            "The regional transportation process server lost regional Receive.");
        serveExpected(
            receiver,
            receiverHandler,
            TransportServiceOperation::unsubscribe_interaction_class_with_regions,
            "The regional transportation process server lost regional Unsubscribe.");
        serveExpected(
            sender,
            senderHandler,
            TransportServiceOperation::resign_federation_execution,
            "The regional transportation process server lost sender Resign.");
        serveExpected(
            receiver,
            receiverHandler,
            TransportServiceOperation::resign_federation_execution,
            "The regional transportation process server lost receiver Resign.");
        service.detach(sender);
        service.detach(receiver);
        senderConnection->close();
        receiverConnection->close();
      } catch (...) {
        serverError = std::current_exception();
      }
    });

    RegionalInteractionTransportationFederateAmbassador senderFederate;
    RegionalInteractionTransportationFederateAmbassador receiverFederate;
    auto senderRti = makeRti();
    auto receiverRti = makeRti();
    auto configurationFor = [&](wchar_t const* name) {
      return RtiConfiguration::createConfiguration()
          .withConfigurationName(name)
          .withRtiAddress(L"tcp://127.0.0.1:" + std::to_wstring(port));
    };
    std::exception_ptr clientError;
    bool senderJoined = false;
    bool receiverJoined = false;
    try {
      REQUIRE(senderRti->connect(
                  senderFederate,
                  callbackModel,
                  configurationFor(L"public-process-transportation-regional-interaction-sender-client"))
                  .addressUsed);
      senderRti->createFederationExecution(
          federationName, L"server-owned-fom.xml");
      auto const senderHandle = senderRti->joinFederationExecution(
          senderName, federateType, federationName);
      REQUIRE(senderHandle.isValid());
      senderJoined = true;

      REQUIRE(receiverRti->connect(
                  receiverFederate,
                  callbackModel,
                  configurationFor(L"public-process-transportation-regional-interaction-receiver-client"))
                  .addressUsed);
      auto const receiverHandle = receiverRti->joinFederationExecution(
          receiverName, federateType, federationName);
      REQUIRE(receiverHandle.isValid());
      receiverJoined = true;

      auto const senderInteraction =
          senderRti->getInteractionClassHandle(interactionNameWide);
      auto const receiverInteraction =
          receiverRti->getInteractionClassHandle(interactionNameWide);
      auto const expectedClass =
          rti1516_2025::umbra_binding_detail::makeInteractionClassHandle(
              expectedInteractionClass.load(std::memory_order_acquire));
      REQUIRE(senderInteraction.toString() == expectedClass.toString());
      REQUIRE(receiverInteraction.toString() == expectedClass.toString());
      auto const senderParameter =
          senderRti->getParameterHandle(senderInteraction, parameterNameWide);
      auto const receiverParameter =
          receiverRti->getParameterHandle(receiverInteraction, parameterNameWide);
      auto const expectedParameterHandle =
          rti1516_2025::umbra_binding_detail::makeParameterHandle(
              expectedParameter.load(std::memory_order_acquire));
      REQUIRE(senderParameter == expectedParameterHandle);
      REQUIRE(receiverParameter == expectedParameterHandle);
      auto const senderDimension =
          senderRti->getDimensionHandle(dimensionNameWide);
      auto const receiverDimension =
          receiverRti->getDimensionHandle(dimensionNameWide);
      auto const expectedDimensionText =
          L"DimensionHandle(" +
          std::to_wstring(expectedDimension.load(std::memory_order_acquire)) +
          L")";
      REQUIRE(senderDimension.toString() == expectedDimensionText);
      REQUIRE(receiverDimension.toString() == expectedDimensionText);
      auto const transportation =
          senderRti->getTransportationTypeHandle(L"HLAbestEffort");
      auto const expectedTransportationText =
          L"TransportationTypeHandle(" +
          std::to_wstring(expectedTransportation.load(std::memory_order_acquire)) +
          L")";
      REQUIRE(transportation.toString() == expectedTransportationText);

      auto const senderRegion = senderRti->createRegion(
          rti1516_2025::DimensionHandleSet{senderDimension});
      auto const receiverRegion = receiverRti->createRegion(
          rti1516_2025::DimensionHandleSet{receiverDimension});
      REQUIRE(senderRegion.isValid());
      REQUIRE(receiverRegion.isValid());
      REQUIRE_NOTHROW(senderRti->setRangeBounds(
          senderRegion, senderDimension, rti1516_2025::RangeBounds(0UL, 5UL)));
      REQUIRE_NOTHROW(senderRti->commitRegionModifications(
          rti1516_2025::RegionHandleSet{senderRegion}));
      REQUIRE_NOTHROW(receiverRti->setRangeBounds(
          receiverRegion,
          receiverDimension,
          rti1516_2025::RangeBounds(0UL, 5UL)));
      REQUIRE_NOTHROW(receiverRti->commitRegionModifications(
          rti1516_2025::RegionHandleSet{receiverRegion}));

      REQUIRE_NOTHROW(senderRti->publishInteractionClass(senderInteraction));
      REQUIRE_NOTHROW(senderRti->requestInteractionTransportationTypeChange(
          senderInteraction, transportation));
      REQUIRE_NOTHROW(senderRti->evokeCallback(0.0));
      REQUIRE(senderFederate.changeCount == 1U);
      REQUIRE(senderFederate.changedInteractionClass == senderInteraction);
      REQUIRE(senderFederate.changedTransportationType == transportation);

      REQUIRE_NOTHROW(receiverRti->setConveyRegionDesignatorSetsSwitch(true));
      REQUIRE(receiverRti->getConveyRegionDesignatorSetsSwitch());
      REQUIRE_NOTHROW(receiverRti->subscribeInteractionClassWithRegions(
          receiverInteraction,
          rti1516_2025::RegionHandleSet{receiverRegion},
          true));
      ParameterHandleValueMap parameterValues;
      std::array<std::uint8_t, 2U> encodedParameter{0x01U, 0x00U};
      parameterValues.emplace(
          senderParameter,
          VariableLengthData(encodedParameter.data(), encodedParameter.size()));
      std::array<std::uint8_t, 1U> tag{0xB5U};
      REQUIRE_NOTHROW(senderRti->sendInteractionWithRegions(
          senderInteraction,
          parameterValues,
          rti1516_2025::RegionHandleSet{senderRegion},
          VariableLengthData(tag.data(), tag.size())));
      REQUIRE_NOTHROW(receiverRti->evokeCallback(0.0));

      REQUIRE(receiverFederate.receiveCount == 1U);
      REQUIRE(receiverFederate.receivedInteractionClass == receiverInteraction);
      REQUIRE(receiverFederate.receivedTransportationType == transportation);
      REQUIRE(receiverFederate.receivedProducingFederate == senderHandle);
      REQUIRE(receiverFederate.receivedParameterCount == 1U);
      REQUIRE(receiverFederate.receivedOptionalRegions);
      REQUIRE(receiverFederate.receivedRegionCount == 1U);
      REQUIRE(receiverFederate.receivedTag == std::vector<std::uint8_t>{0xB5U});

      REQUIRE_NOTHROW(receiverRti->unsubscribeInteractionClassWithRegions(
          receiverInteraction,
          rti1516_2025::RegionHandleSet{receiverRegion}));
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
      if (serverError) {
        std::rethrow_exception(serverError);
      }
      std::rethrow_exception(clientError);
    }
    REQUIRE_FALSE(serverError);
    REQUIRE_FALSE(senderJoined);
    REQUIRE_FALSE(receiverJoined);
  };

  SECTION("HLA_EVOKED") {
    runScenario(HLA_EVOKED);
  }
  SECTION("HLA_IMMEDIATE") {
    runScenario(HLA_IMMEDIATE);
  }
}

#endif  // UMBRA_ENABLE_EMBEDDED_FEDERATION_MANAGEMENT
