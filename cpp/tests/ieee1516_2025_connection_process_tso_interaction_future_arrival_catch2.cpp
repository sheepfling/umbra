#include <catch2/catch_test_macros.hpp>

#include <array>
#include <atomic>
#include <chrono>
#include <cstdint>
#include <condition_variable>
#include <exception>
#include <filesystem>
#include <memory>
#include <mutex>
#include <stdexcept>
#include <string>
#include <thread>
#include <utility>
#include <vector>

#include <RTI/RTI1516.h>
#include <RTI/NullFederateAmbassador.h>
#include <RTI/time/HLAinteger64Time.h>
#include <RTI/time/HLAinteger64Interval.h>

#include <umbra/embedded_profile_configuration.hpp>

#if defined(UMBRA_ENABLE_EMBEDDED_FEDERATION_MANAGEMENT)
#include "internal/federation/federation_registry.hpp"
#include "internal/federation/process_federation_service.hpp"
#include "internal/federation/process_transport.hpp"
#include "internal/federation/process_transport_session.hpp"
#include "internal/fom/libxml2_fom_composer.hpp"
#include "internal/fom/fom_validation.hpp"
#include "internal/fom/libxml2_fom_validator.hpp"
#include "internal/handles/interaction_class_handle.hpp"
#include "internal/handles/parameter_handle.hpp"
#include "process_public_service_fixture.hpp"
#include "process_service_report_fixture.hpp"

namespace {

using rti1516_2025::CallbackModel;
using rti1516_2025::DELETE_OBJECTS;
using rti1516_2025::HLA_EVOKED;
using rti1516_2025::HLA_IMMEDIATE;
using rti1516_2025::NO_ACTION;
using rti1516_2025::NullFederateAmbassador;
using rti1516_2025::ParameterHandleValueMap;
using rti1516_2025::RTIambassador;
using rti1516_2025::RTIambassadorFactory;
using rti1516_2025::RtiConfiguration;
using rti1516_2025::VariableLengthData;
using umbra::detail::EmbeddedFederationRegistry;
using umbra::detail::FederationDefinition;
using umbra::detail::FomCompositionStatus;
using umbra::detail::FomModuleKind;
using umbra::detail::LibXml2FomModuleComposer;
using umbra::detail::LibXml2FomValidator;
using umbra::detail::PrevalidatedFomModule;
using umbra::detail::ProcessFederationService;
using umbra::detail::ProcessFederationServiceOptions;
using umbra::detail::ProcessTransportConnection;
using umbra::detail::ProcessTransportListener;
using umbra::detail::ProcessTransportSession;
using umbra::detail::ProcessTransportServiceDispatcher;
using umbra::detail::TransportServiceMessage;
using umbra::detail::TransportServiceOperation;
using umbra::detail::TransportServiceStatus;

std::filesystem::path processResourcePath(
    std::filesystem::path const& relative) {
  return std::filesystem::path(UMBRA_SOURCE_DIRECTORY) / "third_party" /
      "ieee1516.2-2025" / "resources" / relative;
}

PrevalidatedFomModule validatedProcessModule(
    std::filesystem::path const& source,
    FomModuleKind kind,
    std::wstring designator) {
  LibXml2FomValidator validator;
  auto result = validator.validate({
      source,
      processResourcePath("schemas/IEEE1516-DIF-2025.xsd"),
      kind,
      std::move(designator),
      L"IEEE1516-DIF-2025.xsd",
  });
  if (result.status != umbra::detail::FomValidationStatus::valid ||
      !result.module) {
    throw std::runtime_error("The process-interaction FOM did not validate.");
  }
  return *result.module;
}

FederationDefinition composedProcessDefinition() {
  std::vector<PrevalidatedFomModule> modules{
      validatedProcessModule(
          processResourcePath("mim/HLAstandardMIM-2025.xml"),
          FomModuleKind::mim,
          L"urn:umbra:test:process-interaction-mim"),
      validatedProcessModule(
          processResourcePath("examples/RestaurantFOMmodule-2025.xml"),
          FomModuleKind::fom,
          L"urn:umbra:test:process-interaction-restaurant"),
  };
  LibXml2FomModuleComposer composer(
      processResourcePath("schemas/IEEE1516-FDD-2025.xsd"));
  auto result = composer.compose(modules);
  if (result.status != FomCompositionStatus::valid || !result.catalog ||
      !result.fdd) {
    throw std::runtime_error("The process-interaction FOM did not compose.");
  }
  return {
      std::move(result.modules),
      L"HLAinteger64Time",
      std::move(result.catalog),
      std::move(result.fdd),
  };
}

std::unique_ptr<RTIambassador> makeRti() {
  RTIambassadorFactory factory;
  return factory.createRTIambassador();
}

enum class ProcessNextMessageRequest { nmr, nmra };

void verifyProcessTsoOrdinaryInteraction(
    CallbackModel const callbackModel,
    ProcessNextMessageRequest const requestKind) {
  class RecordingFederateAmbassador final : public NullFederateAmbassador {
   public:
    void receiveInteraction(
        rti1516_2025::InteractionClassHandle const& interactionClass,
        rti1516_2025::ParameterHandleValueMap const& parameterValues,
        VariableLengthData const& userSuppliedTag,
        rti1516_2025::TransportationTypeHandle const& transportationType,
        rti1516_2025::FederateHandle const& producingFederate,
        rti1516_2025::RegionHandleSet const* optionalSentRegions,
        rti1516_2025::LogicalTime const& time,
        rti1516_2025::OrderType sentOrder,
        rti1516_2025::OrderType receivedOrder,
        rti1516_2025::MessageRetractionHandle const* optionalRetraction) override {
      std::scoped_lock lock(callbackMutex);
      ++receivedInteractionCount;
      callbackOrder.push_back('I');
      receivedInteractionClass = interactionClass;
      receivedTransportationType = transportationType;
      receivedProducingFederate = producingFederate;
      receivedParameterCount = parameterValues.size();
      hasOptionalSentRegions = optionalSentRegions != nullptr;
      hasOptionalRetraction = optionalRetraction != nullptr;
      retractionIsValid = optionalRetraction != nullptr &&
          optionalRetraction->isValid();
      sentOrderType = sentOrder;
      receivedOrderType = receivedOrder;
      timestampImplementation = time.implementationName();
      if (auto const* integerTime =
              dynamic_cast<rti1516_2025::HLAinteger64Time const*>(&time)) {
        timestampValue = integerTime->getTime();
      }
      receivedTag.clear();
      if (userSuppliedTag.size() != 0U) {
        auto const* bytes =
            static_cast<std::uint8_t const*>(userSuppliedTag.data());
        receivedTag.assign(bytes, bytes + userSuppliedTag.size());
      }
      receivedParameter = rti1516_2025::ParameterHandle{};
      receivedParameterBytes.clear();
      if (!parameterValues.empty()) {
        auto const& [parameter, value] = *parameterValues.begin();
        receivedParameter = parameter;
        if (value.size() != 0U) {
          auto const* bytes = static_cast<std::uint8_t const*>(value.data());
          receivedParameterBytes.assign(bytes, bytes + value.size());
        }
      }
      callbackChanged.notify_all();
    }

    void timeAdvanceGrant(rti1516_2025::LogicalTime const& time) override {
      std::scoped_lock lock(callbackMutex);
      ++timeAdvanceGrantCount;
      callbackOrder.push_back('G');
      if (auto const* integerTime =
              dynamic_cast<rti1516_2025::HLAinteger64Time const*>(&time)) {
        timeAdvanceGrantValue = integerTime->getTime();
      }
      callbackChanged.notify_all();
    }

    bool waitForDeliveryAndGrant() {
      std::unique_lock lock(callbackMutex);
      return callbackChanged.wait_for(lock, std::chrono::seconds(2), [this] {
        return receivedInteractionCount == 1U && timeAdvanceGrantCount == 1U;
      });
    }

    std::mutex callbackMutex;
    std::condition_variable callbackChanged;
    std::size_t receivedInteractionCount = 0U;
    rti1516_2025::InteractionClassHandle receivedInteractionClass;
    rti1516_2025::TransportationTypeHandle receivedTransportationType;
    rti1516_2025::FederateHandle receivedProducingFederate;
    std::size_t receivedParameterCount = 0U;
    rti1516_2025::ParameterHandle receivedParameter;
    std::vector<std::uint8_t> receivedParameterBytes;
    bool hasOptionalSentRegions = false;
    bool hasOptionalRetraction = false;
    bool retractionIsValid = false;
    rti1516_2025::OrderType sentOrderType = rti1516_2025::RECEIVE;
    rti1516_2025::OrderType receivedOrderType = rti1516_2025::RECEIVE;
    std::wstring timestampImplementation;
    std::int64_t timestampValue = -1;
    std::vector<std::uint8_t> receivedTag;
    std::size_t timeAdvanceGrantCount = 0U;
    std::int64_t timeAdvanceGrantValue = -1;
    std::vector<char> callbackOrder;
  } receiverFederate;
  NullFederateAmbassador senderFederate;
  NullFederateAmbassador secondSenderFederate;

  constexpr wchar_t const* federationName =
      L"process-tso-ordinary-interaction-future-arrival-execution";
  constexpr wchar_t const* senderName =
      L"process-tso-ordinary-interaction-future-arrival-sender";
  constexpr wchar_t const* secondSenderName =
      L"process-tso-ordinary-interaction-future-arrival-second-sender";
  constexpr wchar_t const* receiverName =
      L"process-tso-ordinary-interaction-future-arrival-receiver";
  constexpr wchar_t const* federateType =
      L"process-tso-ordinary-interaction-future-arrival-type";
  constexpr char const* interactionName =
      "HLAinteractionRoot.CustomerTransactions.FoodServed.MainCourseServed";
  constexpr wchar_t const* interactionNameWide =
      L"HLAinteractionRoot.CustomerTransactions.FoodServed.MainCourseServed";
  constexpr char const* parameterName = "TimelinessOk";
  constexpr wchar_t const* parameterNameWide = L"TimelinessOk";

  auto listener = ProcessTransportListener::listen({"127.0.0.1", 0U});
  REQUIRE(listener);
  auto const port = listener->address().port;
  REQUIRE(port != 0U);
  std::atomic_uint64_t expectedInteractionClass{0U};
  std::atomic_uint64_t expectedParameter{0U};
  std::atomic_uint64_t recipientCount{0U};
  std::exception_ptr serverError;
  std::shared_ptr<ProcessTransportConnection> serverSenderConnection;
  std::shared_ptr<ProcessTransportConnection> serverSecondSenderConnection;
  std::shared_ptr<ProcessTransportConnection> serverReceiverConnection;
  std::thread server([&] {
    try {
      EmbeddedFederationRegistry registry;
      ProcessFederationService senderService(
          registry,
          composedProcessDefinition(),
          ProcessFederationServiceOptions{callbackModel == HLA_IMMEDIATE});
      ProcessFederationService receiverService(
          registry,
          composedProcessDefinition(),
          ProcessFederationServiceOptions{callbackModel == HLA_IMMEDIATE});
      auto senderConnection = listener->accept(
          nullptr,
          {"process-tso-ordinary-interaction-server", 0xB401U},
          [](std::wstring) {},
          [](std::wstring) { return false; });
      serverSenderConnection = senderConnection;
      ProcessTransportSession sender(senderConnection);
      auto const senderHandler = senderService.handlerFor(sender);
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
          "The process-interaction server lost Create.");
      auto const interactionClass = registry.interactionClassHandleFor(
          federationName, interactionName);
      auto const parameter = registry.parameterHandleFor(
          federationName, interactionName, parameterName);
      if (!interactionClass || !parameter) {
        throw std::runtime_error(
            "The process-interaction server could not resolve its FOM handles.");
      }
      expectedInteractionClass.store(
          *interactionClass, std::memory_order_release);
      expectedParameter.store(*parameter, std::memory_order_release);
      serveExpected(
          sender,
          senderHandler,
          TransportServiceOperation::join_federation_execution,
          "The process-interaction server lost sender Join.");

      auto secondSenderConnection = listener->accept(
          nullptr,
          {"process-tso-ordinary-interaction-server", 0xB402U},
          [](std::wstring) {},
          [](std::wstring) { return false; });
      serverSecondSenderConnection = secondSenderConnection;
      ProcessTransportSession secondSender(secondSenderConnection);
      auto const secondSenderHandler = senderService.handlerFor(secondSender);
      serveExpected(
          secondSender,
          secondSenderHandler,
          TransportServiceOperation::join_federation_execution,
          "The process-interaction server lost second sender Join.");

      auto receiverConnection = listener->accept(
          nullptr,
          {"process-tso-ordinary-interaction-server", 0xB403U},
          [](std::wstring) {},
          [](std::wstring) { return false; });
      serverReceiverConnection = receiverConnection;
      ProcessTransportSession receiver(receiverConnection);
      auto const receiverHandler = receiverService.handlerFor(receiver);
      serveExpected(
          receiver,
          receiverHandler,
          TransportServiceOperation::join_federation_execution,
          "The process-interaction server lost receiver Join.");

      serveExpected(
          sender,
          senderHandler,
          TransportServiceOperation::get_interaction_class_handle,
          "The process-interaction server lost sender class lookup.");
      serveExpected(
          secondSender,
          secondSenderHandler,
          TransportServiceOperation::get_interaction_class_handle,
          "The process-interaction server lost second sender class lookup.");
      serveExpected(
          receiver,
          receiverHandler,
          TransportServiceOperation::get_interaction_class_handle,
          "The process-interaction server lost receiver class lookup.");
      serveExpected(
          sender,
          senderHandler,
          TransportServiceOperation::get_parameter_handle,
          "The process-interaction server lost sender parameter lookup.");
      serveExpected(
          secondSender,
          secondSenderHandler,
          TransportServiceOperation::get_parameter_handle,
          "The process-interaction server lost second sender parameter lookup.");
      serveExpected(
          receiver,
          receiverHandler,
          TransportServiceOperation::get_parameter_handle,
          "The process-interaction server lost receiver parameter lookup.");
      serveExpected(
          sender,
          senderHandler,
          TransportServiceOperation::publish_interaction_class,
          "The process-interaction server lost Publish.");
      serveExpected(
          secondSender,
          secondSenderHandler,
          TransportServiceOperation::publish_interaction_class,
          "The process-interaction server lost second sender Publish.");
      serveExpected(
          receiver,
          receiverHandler,
          TransportServiceOperation::subscribe_interaction_class,
          "The process-interaction server lost Subscribe.");
      umbra::test::serveExpectedProcessSuccessfulVoidService(
          serveExpected,
          sender,
          senderHandler,
          TransportServiceOperation::enable_time_regulation,
          "The process-interaction server lost Enable Time Regulation.");
      umbra::test::serveExpectedProcessSuccessfulVoidService(
          serveExpected,
          secondSender,
          secondSenderHandler,
          TransportServiceOperation::enable_time_regulation,
          "The process-interaction server lost second sender Enable Time Regulation.");
      umbra::test::serveExpectedProcessSuccessfulVoidService(
          serveExpected,
          receiver,
          receiverHandler,
          TransportServiceOperation::enable_time_constrained,
          "The process-interaction server lost Enable Time Constrained.");
      auto const requestOperation =
          requestKind == ProcessNextMessageRequest::nmr
              ? TransportServiceOperation::next_message_request
              : TransportServiceOperation::next_message_request_available;
      umbra::test::serveExpectedProcessSuccessfulVoidService(
          serveExpected,
          receiver,
          receiverHandler,
          requestOperation,
          "The process-interaction server lost the receiver next-message request.");
      if (callbackModel == HLA_EVOKED) {
        serveExpected(
            receiver,
            receiverHandler,
            TransportServiceOperation::receive_interaction,
            "The process-interaction server lost the pre-send callback poll.");
      }
      auto serveInteractionSend = [&](ProcessTransportSession& sendingSession,
                                      auto const& sendingHandler) {
        if (!umbra::test::servePrimaryProcessRequest(
                sendingSession,
                sendingHandler,
                [&](TransportServiceMessage const& request) {
                  if (request.operation != TransportServiceOperation::send_interaction) {
                    throw std::runtime_error(
                        "The process-interaction server lost Send Interaction.");
                  }
                  auto response = sendingHandler(request);
                  if (response.status != TransportServiceStatus::ok) {
                    throw std::runtime_error(
                        "The process-interaction server rejected Send Interaction.");
                  }
                  auto const result =
                      umbra::detail::decodeProcessFederationSendInteractionResult(
                          response.payload);
                  recipientCount.store(
                      result.recipientCount, std::memory_order_release);
                  return response;
                })) {
          throw std::runtime_error(
              "The process-interaction server lost Send Interaction.");
        }
      };
      serveInteractionSend(sender, senderHandler);
      umbra::test::serveExpectedProcessSuccessfulVoidService(
          serveExpected,
          sender,
          senderHandler,
          TransportServiceOperation::time_advance_request,
          "The process-interaction server lost sender Time Advance Request.");
      serveInteractionSend(secondSender, secondSenderHandler);
      umbra::test::serveExpectedProcessSuccessfulVoidService(
          serveExpected,
          secondSender,
          secondSenderHandler,
          TransportServiceOperation::time_advance_request,
          "The process-interaction server lost second sender Time Advance Request.");
      serveExpected(
          receiver,
          receiverHandler,
          TransportServiceOperation::acknowledge_tso_delivery,
          "The process-interaction server lost TSO acknowledgement.");
      serveExpected(
          sender,
          senderHandler,
          TransportServiceOperation::resign_federation_execution,
          "The process-interaction server lost sender Resign.");
      serveExpected(
          secondSender,
          secondSenderHandler,
          TransportServiceOperation::resign_federation_execution,
          "The process-interaction server lost second sender Resign.");
      serveExpected(
          receiver,
          receiverHandler,
          TransportServiceOperation::resign_federation_execution,
          "The process-interaction server lost receiver Resign.");
      senderService.detach(sender);
      senderService.detach(secondSender);
      receiverService.detach(receiver);
      senderConnection->close();
      secondSenderConnection->close();
      receiverConnection->close();
    } catch (...) {
      serverError = std::current_exception();
      if (serverSenderConnection) {
        serverSenderConnection->close();
      }
      if (serverSecondSenderConnection) {
        serverSecondSenderConnection->close();
      }
      if (serverReceiverConnection) {
        serverReceiverConnection->close();
      }
    }
  });

  auto senderRti = makeRti();
  auto secondSenderRti = makeRti();
  auto receiverRti = makeRti();
  auto configurationFor = [&](wchar_t const* name) {
    return RtiConfiguration::createConfiguration()
        .withConfigurationName(name)
        .withRtiAddress(L"tcp://127.0.0.1:" + std::to_wstring(port));
  };
  std::exception_ptr clientError;
  bool senderJoined = false;
  bool secondSenderJoined = false;
  bool receiverJoined = false;
  rti1516_2025::FederateHandle senderHandle;
  rti1516_2025::FederateHandle secondSenderHandle;
  rti1516_2025::InteractionClassHandle receiverInteraction;
  rti1516_2025::ParameterHandle receiverParameter;
  try {
    REQUIRE(senderRti->connect(
                senderFederate,
                callbackModel,
                configurationFor(
                    L"process-tso-ordinary-interaction-sender-client"))
                .addressUsed);
    REQUIRE_NOTHROW(senderRti->createFederationExecution(
        federationName, L"server-owned-fom.xml"));
    senderHandle = senderRti->joinFederationExecution(
        senderName, federateType, federationName);
    REQUIRE(senderHandle.isValid());
    senderJoined = true;

    REQUIRE(secondSenderRti->connect(
                secondSenderFederate,
                callbackModel,
                configurationFor(
                    L"process-tso-ordinary-interaction-second-sender-client"))
                .addressUsed);
    secondSenderHandle = secondSenderRti->joinFederationExecution(
        secondSenderName, federateType, federationName);
    REQUIRE(secondSenderHandle.isValid());
    secondSenderJoined = true;

    REQUIRE(receiverRti->connect(
                receiverFederate,
                callbackModel,
                configurationFor(
                    L"process-tso-ordinary-interaction-receiver-client"))
                .addressUsed);
    auto const receiverHandle = receiverRti->joinFederationExecution(
        receiverName, federateType, federationName);
    REQUIRE(receiverHandle.isValid());
    receiverJoined = true;

    auto const senderInteraction =
        senderRti->getInteractionClassHandle(interactionNameWide);
    auto const secondSenderInteraction =
        secondSenderRti->getInteractionClassHandle(interactionNameWide);
    receiverInteraction =
        receiverRti->getInteractionClassHandle(interactionNameWide);
    auto const expectedInteraction =
        rti1516_2025::umbra_binding_detail::makeInteractionClassHandle(
            expectedInteractionClass.load(std::memory_order_acquire));
    REQUIRE(senderInteraction == expectedInteraction);
    REQUIRE(secondSenderInteraction == expectedInteraction);
    REQUIRE(receiverInteraction == expectedInteraction);
    auto const senderParameter =
        senderRti->getParameterHandle(senderInteraction, parameterNameWide);
    auto const secondSenderParameter =
        secondSenderRti->getParameterHandle(
            secondSenderInteraction, parameterNameWide);
    receiverParameter =
        receiverRti->getParameterHandle(receiverInteraction, parameterNameWide);
    auto const expectedParameterHandle =
        rti1516_2025::umbra_binding_detail::makeParameterHandle(
            expectedParameter.load(std::memory_order_acquire));
    REQUIRE(senderParameter == expectedParameterHandle);
    REQUIRE(secondSenderParameter == expectedParameterHandle);
    REQUIRE(receiverParameter == expectedParameterHandle);
    REQUIRE_NOTHROW(senderRti->publishInteractionClass(senderInteraction));
    REQUIRE_NOTHROW(
        secondSenderRti->publishInteractionClass(secondSenderInteraction));
    REQUIRE_NOTHROW(receiverRti->subscribeInteractionClass(
        receiverInteraction, true));
    REQUIRE_NOTHROW(senderRti->enableTimeRegulation(
        rti1516_2025::HLAinteger64Interval(1)));
    REQUIRE_NOTHROW(secondSenderRti->enableTimeRegulation(
        rti1516_2025::HLAinteger64Interval(1)));
    REQUIRE_NOTHROW(receiverRti->enableTimeConstrained());
    if (callbackModel == HLA_EVOKED) {
      static_cast<void>(senderRti->evokeCallback(0.0));
      static_cast<void>(secondSenderRti->evokeCallback(0.0));
      static_cast<void>(receiverRti->evokeCallback(0.0));
      REQUIRE(receiverFederate.callbackOrder.empty());
    }
    REQUIRE(receiverFederate.receivedInteractionCount == 0U);
    REQUIRE(receiverFederate.timeAdvanceGrantCount == 0U);

    if (requestKind == ProcessNextMessageRequest::nmr) {
      REQUIRE_NOTHROW(receiverRti->nextMessageRequest(
          rti1516_2025::HLAinteger64Time(10)));
    } else {
      REQUIRE_NOTHROW(receiverRti->nextMessageRequestAvailable(
          rti1516_2025::HLAinteger64Time(10)));
    }
    if (callbackModel == HLA_EVOKED) {
      REQUIRE_FALSE(receiverRti->evokeCallback(0.0));
    }
    REQUIRE(receiverFederate.receivedInteractionCount == 0U);
    REQUIRE(receiverFederate.timeAdvanceGrantCount == 0U);

    std::array<std::uint8_t, 2U> const parameterBytes{0x51U, 0x52U};
    ParameterHandleValueMap laterParameterValues;
    laterParameterValues.emplace(
        senderParameter,
        VariableLengthData(parameterBytes.data(), parameterBytes.size()));
    ParameterHandleValueMap earlierParameterValues;
    earlierParameterValues.emplace(
        secondSenderParameter,
        VariableLengthData(parameterBytes.data(), parameterBytes.size()));
    std::array<std::uint8_t, 3U> const tagBytes{0x4EU, 0x4DU, 0x52U};
    auto const laterTimestampRetraction = senderRti->sendInteraction(
        senderInteraction,
        laterParameterValues,
        VariableLengthData(tagBytes.data(), tagBytes.size()),
        rti1516_2025::HLAinteger64Time(8));
    REQUIRE(laterTimestampRetraction.isValid());
    REQUIRE(recipientCount.load(std::memory_order_acquire) == 1U);
    REQUIRE_NOTHROW(senderRti->timeAdvanceRequest(
        rti1516_2025::HLAinteger64Time(8)));

    auto const earlierTimestampRetraction = secondSenderRti->sendInteraction(
        secondSenderInteraction,
        earlierParameterValues,
        VariableLengthData(tagBytes.data(), tagBytes.size()),
        rti1516_2025::HLAinteger64Time(5));
    REQUIRE(earlierTimestampRetraction.isValid());
    REQUIRE(recipientCount.load(std::memory_order_acquire) == 1U);
    REQUIRE_NOTHROW(secondSenderRti->timeAdvanceRequest(
        rti1516_2025::HLAinteger64Time(5)));

    if (callbackModel == HLA_EVOKED) {
      REQUIRE(receiverRti->evokeCallback(0.0));
      {
        std::scoped_lock lock(receiverFederate.callbackMutex);
        REQUIRE(receiverFederate.receivedInteractionCount == 1U);
        REQUIRE(receiverFederate.timeAdvanceGrantCount == 0U);
        REQUIRE(receiverFederate.callbackOrder == std::vector<char>{'I'});
      }
      static_cast<void>(receiverRti->evokeCallback(0.0));
    } else {
      REQUIRE(receiverFederate.waitForDeliveryAndGrant());
    }
    {
      std::scoped_lock lock(receiverFederate.callbackMutex);
      REQUIRE(receiverFederate.receivedInteractionCount == 1U);
      REQUIRE(receiverFederate.timeAdvanceGrantCount == 1U);
      REQUIRE(receiverFederate.callbackOrder == std::vector<char>{'I', 'G'});
      REQUIRE(receiverFederate.receivedInteractionClass == expectedInteraction);
      REQUIRE(receiverFederate.receivedProducingFederate == secondSenderHandle);
      REQUIRE(receiverFederate.receivedTransportationType.isValid());
      REQUIRE(receiverFederate.receivedParameterCount == 1U);
      REQUIRE(receiverFederate.receivedParameter == receiverParameter);
      REQUIRE(receiverFederate.receivedParameterBytes ==
              std::vector<std::uint8_t>{0x51U, 0x52U});
      REQUIRE_FALSE(receiverFederate.hasOptionalSentRegions);
      REQUIRE(receiverFederate.hasOptionalRetraction);
      REQUIRE(receiverFederate.retractionIsValid);
      REQUIRE(receiverFederate.timestampImplementation == L"HLAinteger64Time");
      REQUIRE(receiverFederate.timestampValue == 5);
      REQUIRE(receiverFederate.receivedTag ==
              std::vector<std::uint8_t>{0x4EU, 0x4DU, 0x52U});
      REQUIRE(receiverFederate.sentOrderType == rti1516_2025::TIMESTAMP);
      REQUIRE(receiverFederate.receivedOrderType == rti1516_2025::TIMESTAMP);
      REQUIRE(receiverFederate.timeAdvanceGrantValue == 5);
    }

    REQUIRE_NOTHROW(senderRti->resignFederationExecution(NO_ACTION));
    senderJoined = false;
    REQUIRE_NOTHROW(secondSenderRti->resignFederationExecution(NO_ACTION));
    secondSenderJoined = false;
    REQUIRE_NOTHROW(receiverRti->resignFederationExecution(NO_ACTION));
    receiverJoined = false;
    REQUIRE_NOTHROW(senderRti->disconnect());
    REQUIRE_NOTHROW(secondSenderRti->disconnect());
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
        senderRti->resignFederationExecution(DELETE_OBJECTS);
      } catch (...) {
      }
    }
    if (secondSenderJoined) {
      try {
        secondSenderRti->resignFederationExecution(DELETE_OBJECTS);
      } catch (...) {
      }
    }
    try {
      senderRti->disconnect();
    } catch (...) {
    }
    try {
      secondSenderRti->disconnect();
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
  REQUIRE(recipientCount.load(std::memory_order_acquire) == 1U);
  REQUIRE(expectedInteractionClass.load(std::memory_order_acquire) != 0U);
  REQUIRE(expectedParameter.load(std::memory_order_acquire) != 0U);
  REQUIRE_FALSE(senderJoined);
  REQUIRE_FALSE(secondSenderJoined);
  REQUIRE_FALSE(receiverJoined);
}

}  // namespace

TEST_CASE(
    "RTIambassadors retarget pending process NMR and NMRA to a future timestamped ordinary interaction",
    "[integration][foundation][interaction-management][time-management][time-advance]"
    "[transport][process-boundary][public-endpoint][timestamped-process-interaction]"
    "[process-tso-interaction-nmr-nmra-future-arrival]"
    "[process-tso-immediate-no-follow-up-poll]"
    "[process-time-advance-next-message-queued-focused]"
    "[process-time-advance-earlier-tso-late-arrival]"
    "[process-time-advance-independent-senders]"
    "[process-tso-separate-service-instances]"
    "[rti.service.send-interaction][rti.service.next-message-request]"
    "[rti.service.next-message-request-available]"
    "[federate.callback.receive-interaction][federate.callback.time-advance-grant]"
    "[2025]") {
  SECTION("HLA_EVOKED NMR") {
    verifyProcessTsoOrdinaryInteraction(
        HLA_EVOKED, ProcessNextMessageRequest::nmr);
  }
  SECTION("HLA_IMMEDIATE NMR") {
    verifyProcessTsoOrdinaryInteraction(
        HLA_IMMEDIATE, ProcessNextMessageRequest::nmr);
  }
  SECTION("HLA_EVOKED NMRA") {
    verifyProcessTsoOrdinaryInteraction(
        HLA_EVOKED, ProcessNextMessageRequest::nmra);
  }
  SECTION("HLA_IMMEDIATE NMRA") {
    verifyProcessTsoOrdinaryInteraction(
        HLA_IMMEDIATE, ProcessNextMessageRequest::nmra);
  }
}
#endif
