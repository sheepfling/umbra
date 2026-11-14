#include <catch2/catch_test_macros.hpp>

#include <array>
#include <atomic>
#include <chrono>
#include <condition_variable>
#include <cstdint>
#include <exception>
#include <filesystem>
#include <memory>
#include <mutex>
#include <optional>
#include <stdexcept>
#include <string>
#include <thread>
#include <utility>
#include <vector>

#include <RTI/RTI1516.h>
#include <RTI/NullFederateAmbassador.h>
#include <RTI/time/HLAinteger64Interval.h>
#include <RTI/time/HLAinteger64Time.h>

#include <umbra/embedded_profile_configuration.hpp>

#if defined(UMBRA_ENABLE_EMBEDDED_FEDERATION_MANAGEMENT)
#include "internal/federation/federation_registry.hpp"
#include "internal/federation/process_federation_service.hpp"
#include "internal/federation/process_transport.hpp"
#include "internal/federation/process_transport_session.hpp"
#include "internal/fom/libxml2_fom_composer.hpp"
#include "internal/fom/fom_validation.hpp"
#include "internal/fom/libxml2_fom_validator.hpp"
#include "process_public_service_fixture.hpp"
#include "process_service_report_fixture.hpp"

namespace {

using rti1516_2025::CallbackModel;
using rti1516_2025::DELETE_OBJECTS;
using rti1516_2025::HLA_EVOKED;
using rti1516_2025::HLA_IMMEDIATE;
using rti1516_2025::NO_ACTION;
using rti1516_2025::NullFederateAmbassador;
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
    throw std::runtime_error("The process directed-interaction FOM did not validate.");
  }
  return *result.module;
}

FederationDefinition composedDirectedProcessDefinition() {
  auto const objectConsumer = std::filesystem::path(UMBRA_SOURCE_DIRECTORY) /
      "cpp" / "tests" / "data" / "directed-interaction-object-consumer-fom.xml";
  auto const interactionProvider = std::filesystem::path(UMBRA_SOURCE_DIRECTORY) /
      "cpp" / "tests" / "data" / "directed-interaction-interaction-provider-fom.xml";
  std::vector<PrevalidatedFomModule> modules{
      validatedProcessModule(
          processResourcePath("mim/HLAstandardMIM-2025.xml"),
          FomModuleKind::mim,
          L"urn:umbra:test:directed-process-mim"),
      validatedProcessModule(
          objectConsumer,
          FomModuleKind::fom,
          L"urn:umbra:test:directed-process-object"),
      validatedProcessModule(
          interactionProvider,
          FomModuleKind::fom,
          L"urn:umbra:test:directed-process-interaction"),
  };
  LibXml2FomModuleComposer composer(
      processResourcePath("schemas/IEEE1516-FDD-2025.xsd"));
  auto result = composer.compose(modules);
  if (result.status != FomCompositionStatus::valid || !result.catalog ||
      !result.fdd) {
    throw std::runtime_error("The process directed-interaction FOM did not compose.");
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

enum class NextMessageRequestKind { nmr, nmra };

class DirectedTimeRecordingFederateAmbassador final : public NullFederateAmbassador {
 public:
  void discoverObjectInstance(
      rti1516_2025::ObjectInstanceHandle const& objectInstance,
      rti1516_2025::ObjectClassHandle const& objectClass,
      std::wstring const& objectInstanceName,
      rti1516_2025::FederateHandle const& producingFederate) override {
    {
      std::scoped_lock lock(mutex);
      ++discoveredCount;
      discoveredObjectInstance = objectInstance;
      discoveredObjectClass = objectClass;
      discoveredObjectInstanceName = objectInstanceName;
      discoveredProducingFederate = producingFederate;
    }
    changed.notify_all();
  }

  void receiveDirectedInteraction(
      rti1516_2025::InteractionClassHandle const& interactionClass,
      rti1516_2025::ObjectInstanceHandle const& objectInstance,
      rti1516_2025::ParameterHandleValueMap const& parameterValues,
      VariableLengthData const& userSuppliedTag,
      rti1516_2025::TransportationTypeHandle const& transportationType,
      rti1516_2025::FederateHandle const& producingFederate,
      rti1516_2025::LogicalTime const& time,
      rti1516_2025::OrderType sentOrder,
      rti1516_2025::OrderType receivedOrder,
      rti1516_2025::MessageRetractionHandle const* optionalRetraction) override {
    {
      std::scoped_lock lock(mutex);
      ++receivedDirectedInteractionCount;
      callbackOrder.push_back('D');
      receivedInteractionClass = interactionClass;
      receivedObjectInstance = objectInstance;
      receivedParameterCount = parameterValues.size();
      receivedTransportationType = transportationType;
      receivedProducingFederate = producingFederate;
      receivedSentOrder = sentOrder;
      receivedReceivedOrder = receivedOrder;
      hasOptionalRetraction = optionalRetraction != nullptr;
      optionalRetractionIsValid =
          optionalRetraction != nullptr && optionalRetraction->isValid();
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
    }
    changed.notify_all();
  }

  void timeAdvanceGrant(rti1516_2025::LogicalTime const& time) override {
    {
      std::scoped_lock lock(mutex);
      ++timeAdvanceGrantCount;
      callbackOrder.push_back('G');
      if (auto const* integerTime =
              dynamic_cast<rti1516_2025::HLAinteger64Time const*>(&time)) {
        timeAdvanceGrantValue = integerTime->getTime();
      }
    }
    changed.notify_all();
  }

  bool waitForDiscovery() {
    std::unique_lock lock(mutex);
    return changed.wait_for(lock, std::chrono::seconds(3), [this] {
      return discoveredCount != 0U;
    });
  }

  bool waitForDeliveryAndGrant() {
    std::unique_lock lock(mutex);
    return changed.wait_for(lock, std::chrono::seconds(5), [this] {
      return receivedDirectedInteractionCount == 1U && timeAdvanceGrantCount == 1U;
    });
  }

  std::mutex mutex;
  std::condition_variable changed;
  std::size_t discoveredCount = 0U;
  rti1516_2025::ObjectInstanceHandle discoveredObjectInstance;
  rti1516_2025::ObjectClassHandle discoveredObjectClass;
  std::wstring discoveredObjectInstanceName;
  rti1516_2025::FederateHandle discoveredProducingFederate;
  std::size_t receivedDirectedInteractionCount = 0U;
  rti1516_2025::InteractionClassHandle receivedInteractionClass;
  rti1516_2025::ObjectInstanceHandle receivedObjectInstance;
  std::size_t receivedParameterCount = 0U;
  rti1516_2025::TransportationTypeHandle receivedTransportationType;
  rti1516_2025::FederateHandle receivedProducingFederate;
  rti1516_2025::OrderType receivedSentOrder = rti1516_2025::RECEIVE;
  rti1516_2025::OrderType receivedReceivedOrder = rti1516_2025::RECEIVE;
  bool hasOptionalRetraction = false;
  bool optionalRetractionIsValid = false;
  std::wstring timestampImplementation;
  std::int64_t timestampValue = -1;
  std::vector<std::uint8_t> receivedTag;
  std::size_t timeAdvanceGrantCount = 0U;
  std::int64_t timeAdvanceGrantValue = -1;
  std::vector<char> callbackOrder;
};

void verifyProcessDirectedTsoLateArrival(
    CallbackModel callbackModel,
    NextMessageRequestKind requestKind) {
  constexpr wchar_t const* federationName =
      L"process-tso-directed-interaction-future-arrival-execution";
  constexpr wchar_t const* senderName =
      L"process-tso-directed-interaction-future-arrival-sender";
  constexpr wchar_t const* secondSenderName =
      L"process-tso-directed-interaction-future-arrival-second-sender";
  constexpr wchar_t const* receiverName =
      L"process-tso-directed-interaction-future-arrival-receiver";
  constexpr wchar_t const* federateType =
      L"process-tso-directed-interaction-future-arrival-type";
  constexpr wchar_t const* objectClassName =
      L"HLAobjectRoot.UmbraDirectedFixtureObject";
  constexpr wchar_t const* attributeName = L"DirectedTargetMarker";
  constexpr wchar_t const* interactionName =
      L"HLAinteractionRoot.UmbraDirectedFixtureInteraction";

  DirectedTimeRecordingFederateAmbassador senderFederate;
  DirectedTimeRecordingFederateAmbassador secondSenderFederate;
  DirectedTimeRecordingFederateAmbassador receiverFederate;
  auto listener = ProcessTransportListener::listen({"127.0.0.1", 0U});
  REQUIRE(listener);
  auto const port = listener->address().port;
  REQUIRE(port != 0U);

  std::atomic_uint64_t laterRecipientCount{0U};
  std::atomic_uint64_t earlierRecipientCount{0U};
  std::exception_ptr serverError;
  std::shared_ptr<ProcessTransportConnection> serverSenderConnection;
  std::shared_ptr<ProcessTransportConnection> serverSecondSenderConnection;
  std::shared_ptr<ProcessTransportConnection> serverReceiverConnection;
  std::thread server([&] {
    try {
      EmbeddedFederationRegistry registry;
      ProcessFederationServiceOptions serviceOptions;
      serviceOptions.pushReceiveOrderEvents = callbackModel == HLA_IMMEDIATE;
      serviceOptions.createFomPreparation = [](
          std::vector<std::wstring> const& fomModules,
          std::optional<std::wstring> const&,
          std::wstring const& logicalTimeImplementationName)
          -> std::optional<FederationDefinition> {
        if (fomModules !=
                std::vector<std::wstring>{
                    L"urn:umbra:test:process-tso-directed-future-fom"} ||
            (!logicalTimeImplementationName.empty() &&
             logicalTimeImplementationName != L"HLAinteger64Time")) {
          return std::nullopt;
        }
        return composedDirectedProcessDefinition();
      };
      ProcessFederationService service(
          registry,
          composedDirectedProcessDefinition(),
          std::move(serviceOptions));

      auto senderConnection = listener->accept(
          nullptr,
          {"process-tso-directed-interaction-server", 0xD501U},
          [](std::wstring) {},
          [](std::wstring) { return false; });
      serverSenderConnection = senderConnection;
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
                  auto response = handler(request);
                  if (response.status != TransportServiceStatus::ok) {
                    throw std::runtime_error(description);
                  }
                  return response;
                })) {
          throw std::runtime_error(description);
        }
      };
      auto serveDiscovery = [&](ProcessTransportSession& session,
                                auto const& handler,
                                char const* description) {
        bool targetDiscoveryServed = false;
        for (unsigned int attempt = 0U;
             attempt != 8U && !targetDiscoveryServed;
             ++attempt) {
          if (!umbra::test::servePrimaryProcessRequest(
                  session, handler,
                  [&](TransportServiceMessage const& request) {
                    if (request.operation !=
                        TransportServiceOperation::receive_interaction) {
                      throw std::runtime_error(description);
                    }
                    auto response = handler(request);
                    if (response.status != TransportServiceStatus::ok) {
                      throw std::runtime_error(description);
                    }
                    auto const result =
                        umbra::detail::decodeProcessFederationReceiveInteractionResult(
                            response.payload);
                    targetDiscoveryServed = result.discoveryEvent.has_value() &&
                        !result.discoveryEvent->rtiOwnedMomObject;
                    return response;
                  })) {
            throw std::runtime_error(description);
          }
        }
        if (!targetDiscoveryServed) {
          throw std::runtime_error(description);
        }
      };
      auto serveDirectedSend = [&](ProcessTransportSession& session,
                                   auto const& handler,
                                   std::atomic_uint64_t& resultRecipientCount,
                                   char const* description) {
        if (!umbra::test::servePrimaryProcessRequest(
                session, handler,
                [&](TransportServiceMessage const& request) {
                  if (request.operation !=
                      TransportServiceOperation::send_directed_interaction) {
                    throw std::runtime_error(description);
                  }
                  auto response = handler(request);
                  if (response.status != TransportServiceStatus::ok) {
                    throw std::runtime_error(description);
                  }
                  auto const result =
                      umbra::detail::decodeProcessFederationSendInteractionResult(
                          response.payload);
                  resultRecipientCount.store(
                      result.recipientCount, std::memory_order_release);
                  return response;
                })) {
          throw std::runtime_error(description);
        }
      };

      serveExpected(
          sender,
          senderHandler,
          TransportServiceOperation::create_federation_execution,
          "The directed TSO server lost Create.");
      serveExpected(
          sender,
          senderHandler,
          TransportServiceOperation::join_federation_execution,
          "The directed TSO server lost sender Join.");

      auto secondSenderConnection = listener->accept(
          nullptr,
          {"process-tso-directed-interaction-server", 0xD502U},
          [](std::wstring) {},
          [](std::wstring) { return false; });
      serverSecondSenderConnection = secondSenderConnection;
      ProcessTransportSession secondSender(secondSenderConnection);
      auto const secondSenderHandler = service.handlerFor(secondSender);
      serveExpected(
          secondSender,
          secondSenderHandler,
          TransportServiceOperation::join_federation_execution,
          "The directed TSO server lost second sender Join.");

      auto receiverConnection = listener->accept(
          nullptr,
          {"process-tso-directed-interaction-server", 0xD503U},
          [](std::wstring) {},
          [](std::wstring) { return false; });
      serverReceiverConnection = receiverConnection;
      ProcessTransportSession receiver(receiverConnection);
      auto const receiverHandler = service.handlerFor(receiver);
      serveExpected(
          receiver,
          receiverHandler,
          TransportServiceOperation::join_federation_execution,
          "The directed TSO server lost receiver Join.");

      auto serveLookups = [&](ProcessTransportSession& session,
                              auto const& handler,
                              char const* description) {
        serveExpected(
            session, handler, TransportServiceOperation::get_object_class_handle,
            description);
        serveExpected(
            session, handler, TransportServiceOperation::get_attribute_handle,
            description);
        serveExpected(
            session, handler, TransportServiceOperation::get_interaction_class_handle,
            description);
      };
      serveLookups(sender, senderHandler,
                   "The directed TSO server lost sender FOM lookup.");
      serveLookups(secondSender, secondSenderHandler,
                   "The directed TSO server lost second sender FOM lookup.");
      serveLookups(receiver, receiverHandler,
                   "The directed TSO server lost receiver FOM lookup.");

      serveExpected(
          sender,
          senderHandler,
          TransportServiceOperation::publish_object_class_attributes,
          "The directed TSO server lost target attribute publication.");
      serveExpected(
          secondSender,
          secondSenderHandler,
          TransportServiceOperation::subscribe_object_class_attributes,
          "The directed TSO server lost second sender target subscription.");
      serveExpected(
          receiver,
          receiverHandler,
          TransportServiceOperation::subscribe_object_class_attributes,
          "The directed TSO server lost receiver target subscription.");
      serveExpected(
          sender,
          senderHandler,
          TransportServiceOperation::publish_object_class_directed_interactions,
          "The directed TSO server lost sender directed publication.");
      serveExpected(
          secondSender,
          secondSenderHandler,
          TransportServiceOperation::publish_object_class_directed_interactions,
          "The directed TSO server lost second sender directed publication.");
      serveExpected(
          receiver,
          receiverHandler,
          TransportServiceOperation::subscribe_object_class_directed_interactions,
          "The directed TSO server lost receiver directed subscription.");
      serveExpected(
          sender,
          senderHandler,
          TransportServiceOperation::register_object_instance,
          "The directed TSO server lost target registration.");

      if (callbackModel == HLA_EVOKED) {
        serveDiscovery(
            secondSender, secondSenderHandler,
            "The directed TSO server lost second sender discovery polling.");
        serveDiscovery(
            receiver, receiverHandler,
            "The directed TSO server lost receiver discovery polling.");
      }

      umbra::test::serveExpectedProcessSuccessfulVoidService(
          serveExpected,
          sender,
          senderHandler,
          TransportServiceOperation::enable_time_regulation,
          "The directed TSO server lost sender Enable Time Regulation.");
      umbra::test::serveExpectedProcessSuccessfulVoidService(
          serveExpected,
          secondSender,
          secondSenderHandler,
          TransportServiceOperation::enable_time_regulation,
          "The directed TSO server lost second sender Enable Time Regulation.");
      umbra::test::serveExpectedProcessSuccessfulVoidService(
          serveExpected,
          receiver,
          receiverHandler,
          TransportServiceOperation::enable_time_constrained,
          "The directed TSO server lost receiver Enable Time Constrained.");

      auto const requestOperation =
          requestKind == NextMessageRequestKind::nmr
              ? TransportServiceOperation::next_message_request
              : TransportServiceOperation::next_message_request_available;
      umbra::test::serveExpectedProcessSuccessfulVoidService(
          serveExpected,
          receiver,
          receiverHandler,
          requestOperation,
          "The directed TSO server lost the pending NMR/NMRA request.");
      if (callbackModel == HLA_EVOKED) {
        serveExpected(
            receiver,
            receiverHandler,
            TransportServiceOperation::receive_interaction,
            "The directed TSO server lost the pre-arrival callback poll.");
      }

      serveDirectedSend(
          sender, senderHandler, laterRecipientCount,
          "The directed TSO server lost the timestamp-8 send.");
      umbra::test::serveExpectedProcessSuccessfulVoidService(
          serveExpected,
          sender,
          senderHandler,
          TransportServiceOperation::time_advance_request,
          "The directed TSO server lost the first sender TAR.");
      serveDirectedSend(
          secondSender, secondSenderHandler, earlierRecipientCount,
          "The directed TSO server lost the late timestamp-5 send.");
      umbra::test::serveExpectedProcessSuccessfulVoidService(
          serveExpected,
          secondSender,
          secondSenderHandler,
          TransportServiceOperation::time_advance_request,
          "The directed TSO server lost the second sender TAR.");

      serveExpected(
          receiver,
          receiverHandler,
          TransportServiceOperation::acknowledge_tso_delivery,
          "The directed TSO server lost the TSO acknowledgement.");
      serveExpected(
          sender,
          senderHandler,
          TransportServiceOperation::resign_federation_execution,
          "The directed TSO server lost sender Resign.");
      serveExpected(
          secondSender,
          secondSenderHandler,
          TransportServiceOperation::resign_federation_execution,
          "The directed TSO server lost second sender Resign.");
      serveExpected(
          receiver,
          receiverHandler,
          TransportServiceOperation::resign_federation_execution,
          "The directed TSO server lost receiver Resign.");

      service.detach(sender);
      service.detach(secondSender);
      service.detach(receiver);
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
  rti1516_2025::FederateHandle secondSenderHandle;
  rti1516_2025::ObjectInstanceHandle targetObjectInstance;
  rti1516_2025::InteractionClassHandle expectedInteractionClass;
  try {
    REQUIRE(senderRti->connect(
                senderFederate,
                callbackModel,
                configurationFor(L"process-tso-directed-future-sender-client"))
                .addressUsed);
    REQUIRE_NOTHROW(senderRti->createFederationExecution(
        federationName,
        L"urn:umbra:test:process-tso-directed-future-fom"));
    auto const senderHandle = senderRti->joinFederationExecution(
        senderName, federateType, federationName);
    REQUIRE(senderHandle.isValid());
    senderJoined = true;

    REQUIRE(secondSenderRti->connect(
                secondSenderFederate,
                callbackModel,
                configurationFor(
                    L"process-tso-directed-future-second-sender-client"))
                .addressUsed);
    secondSenderHandle = secondSenderRti->joinFederationExecution(
        secondSenderName, federateType, federationName);
    REQUIRE(secondSenderHandle.isValid());
    secondSenderJoined = true;

    REQUIRE(receiverRti->connect(
                receiverFederate,
                callbackModel,
                configurationFor(L"process-tso-directed-future-receiver-client"))
                .addressUsed);
    auto const receiverHandle = receiverRti->joinFederationExecution(
        receiverName, federateType, federationName);
    REQUIRE(receiverHandle.isValid());
    receiverJoined = true;

    auto const senderObjectClass = senderRti->getObjectClassHandle(objectClassName);
    auto const senderAttribute =
        senderRti->getAttributeHandle(senderObjectClass, attributeName);
    auto const senderInteraction = senderRti->getInteractionClassHandle(interactionName);

    auto const secondSenderObjectClass =
        secondSenderRti->getObjectClassHandle(objectClassName);
    auto const secondSenderAttribute =
        secondSenderRti->getAttributeHandle(secondSenderObjectClass, attributeName);
    auto const secondSenderInteraction =
        secondSenderRti->getInteractionClassHandle(interactionName);

    auto const receiverObjectClass = receiverRti->getObjectClassHandle(objectClassName);
    auto const receiverAttribute =
        receiverRti->getAttributeHandle(receiverObjectClass, attributeName);
    auto const receiverInteraction =
        receiverRti->getInteractionClassHandle(interactionName);

    REQUIRE(senderObjectClass == secondSenderObjectClass);
    REQUIRE(senderObjectClass == receiverObjectClass);
    REQUIRE(senderAttribute == secondSenderAttribute);
    REQUIRE(senderAttribute == receiverAttribute);

    REQUIRE(senderInteraction == secondSenderInteraction);
    REQUIRE(senderInteraction == receiverInteraction);
    expectedInteractionClass = receiverInteraction;

    REQUIRE_NOTHROW(senderRti->publishObjectClassAttributes(
        senderObjectClass, rti1516_2025::AttributeHandleSet{senderAttribute}));
    REQUIRE_NOTHROW(secondSenderRti->subscribeObjectClassAttributes(
        secondSenderObjectClass,
        rti1516_2025::AttributeHandleSet{secondSenderAttribute},
        true));
    REQUIRE_NOTHROW(receiverRti->subscribeObjectClassAttributes(
        receiverObjectClass,
        rti1516_2025::AttributeHandleSet{receiverAttribute},
        true));
    REQUIRE_NOTHROW(senderRti->publishObjectClassDirectedInteractions(
        senderObjectClass,
        rti1516_2025::InteractionClassHandleSet{senderInteraction}));
    REQUIRE_NOTHROW(secondSenderRti->publishObjectClassDirectedInteractions(
        secondSenderObjectClass,
        rti1516_2025::InteractionClassHandleSet{secondSenderInteraction}));
    REQUIRE_NOTHROW(receiverRti->subscribeObjectClassDirectedInteractions(
        receiverObjectClass,
        rti1516_2025::InteractionClassHandleSet{receiverInteraction},
        true));

    targetObjectInstance = senderRti->registerObjectInstance(senderObjectClass);
    REQUIRE(targetObjectInstance.isValid());
    if (callbackModel == HLA_EVOKED) {
      for (unsigned int attempt = 0U; attempt != 8U; ++attempt) {
        {
          std::scoped_lock secondLock(secondSenderFederate.mutex);
          if (secondSenderFederate.discoveredCount != 0U) {
            break;
          }
        }
        static_cast<void>(secondSenderRti->evokeCallback(0.0));
      }
      for (unsigned int attempt = 0U; attempt != 8U; ++attempt) {
        {
          std::scoped_lock receiverLock(receiverFederate.mutex);
          if (receiverFederate.discoveredCount != 0U) {
            break;
          }
        }
        static_cast<void>(receiverRti->evokeCallback(0.0));
      }
      {
        std::scoped_lock secondLock(secondSenderFederate.mutex);
        REQUIRE(secondSenderFederate.discoveredCount != 0U);
      }
      {
        std::scoped_lock receiverLock(receiverFederate.mutex);
        REQUIRE(receiverFederate.discoveredCount != 0U);
      }
    } else {
      REQUIRE(secondSenderFederate.waitForDiscovery());
      REQUIRE(receiverFederate.waitForDiscovery());
    }
    {
      std::scoped_lock secondLock(secondSenderFederate.mutex);
      REQUIRE(secondSenderFederate.discoveredCount == 1U);
      REQUIRE(secondSenderFederate.discoveredObjectInstance == targetObjectInstance);
      REQUIRE_FALSE(secondSenderFederate.discoveredObjectInstanceName.empty());
    }
    {
      std::scoped_lock receiverLock(receiverFederate.mutex);
      REQUIRE(receiverFederate.discoveredCount == 1U);
      REQUIRE(receiverFederate.discoveredObjectInstance == targetObjectInstance);
      REQUIRE(receiverFederate.discoveredObjectClass == receiverObjectClass);
      REQUIRE(receiverFederate.discoveredProducingFederate == senderHandle);
      REQUIRE_FALSE(receiverFederate.discoveredObjectInstanceName.empty());
    }

    REQUIRE_NOTHROW(senderRti->enableTimeRegulation(
        rti1516_2025::HLAinteger64Interval(1)));
    REQUIRE_NOTHROW(secondSenderRti->enableTimeRegulation(
        rti1516_2025::HLAinteger64Interval(1)));
    REQUIRE_NOTHROW(receiverRti->enableTimeConstrained());
    if (callbackModel == HLA_EVOKED) {
      static_cast<void>(senderRti->evokeCallback(0.0));
      static_cast<void>(secondSenderRti->evokeCallback(0.0));
      static_cast<void>(receiverRti->evokeCallback(0.0));
    }

    if (requestKind == NextMessageRequestKind::nmr) {
      REQUIRE_NOTHROW(receiverRti->nextMessageRequest(
          rti1516_2025::HLAinteger64Time(10)));
    } else {
      REQUIRE_NOTHROW(receiverRti->nextMessageRequestAvailable(
          rti1516_2025::HLAinteger64Time(10)));
    }
    if (callbackModel == HLA_EVOKED) {
      REQUIRE_FALSE(receiverRti->evokeCallback(0.0));
    }
    {
      std::scoped_lock receiverLock(receiverFederate.mutex);
      REQUIRE(receiverFederate.receivedDirectedInteractionCount == 0U);
      REQUIRE(receiverFederate.timeAdvanceGrantCount == 0U);
    }

    std::array<std::uint8_t, 3U> const laterTagBytes{0x44U, 0x54U, 0x38U};
    std::array<std::uint8_t, 3U> const earlierTagBytes{0x44U, 0x54U, 0x35U};
    auto const laterRetraction = senderRti->sendDirectedInteraction(
        senderInteraction,
        targetObjectInstance,
        rti1516_2025::ParameterHandleValueMap{},
        VariableLengthData(laterTagBytes.data(), laterTagBytes.size()),
        rti1516_2025::HLAinteger64Time(8));
    REQUIRE(laterRetraction.isValid());
    REQUIRE_NOTHROW(senderRti->timeAdvanceRequest(
        rti1516_2025::HLAinteger64Time(8)));
    REQUIRE(laterRecipientCount.load(std::memory_order_acquire) == 1U);
    {
      std::scoped_lock receiverLock(receiverFederate.mutex);
      REQUIRE(receiverFederate.receivedDirectedInteractionCount == 0U);
      REQUIRE(receiverFederate.timeAdvanceGrantCount == 0U);
    }

    auto const earlierRetraction = secondSenderRti->sendDirectedInteraction(
        secondSenderInteraction,
        targetObjectInstance,
        rti1516_2025::ParameterHandleValueMap{},
        VariableLengthData(earlierTagBytes.data(), earlierTagBytes.size()),
        rti1516_2025::HLAinteger64Time(5));
    REQUIRE(earlierRetraction.isValid());
    REQUIRE_NOTHROW(secondSenderRti->timeAdvanceRequest(
        rti1516_2025::HLAinteger64Time(5)));
    REQUIRE(earlierRecipientCount.load(std::memory_order_acquire) == 1U);

    if (callbackModel == HLA_EVOKED) {
      REQUIRE(receiverRti->evokeCallback(0.0));
      {
        std::scoped_lock receiverLock(receiverFederate.mutex);
        REQUIRE(receiverFederate.receivedDirectedInteractionCount == 1U);
        REQUIRE(receiverFederate.timeAdvanceGrantCount == 0U);
        REQUIRE(receiverFederate.callbackOrder == std::vector<char>{'D'});
      }
      static_cast<void>(receiverRti->evokeCallback(0.0));
    } else {
      REQUIRE(receiverFederate.waitForDeliveryAndGrant());
    }
    {
      std::scoped_lock receiverLock(receiverFederate.mutex);
      REQUIRE(receiverFederate.receivedDirectedInteractionCount == 1U);
      REQUIRE(receiverFederate.timeAdvanceGrantCount == 1U);
      REQUIRE(receiverFederate.callbackOrder == std::vector<char>{'D', 'G'});
      REQUIRE(receiverFederate.receivedInteractionClass == expectedInteractionClass);
      REQUIRE(receiverFederate.receivedObjectInstance == targetObjectInstance);
      REQUIRE(receiverFederate.receivedProducingFederate == secondSenderHandle);
      REQUIRE(receiverFederate.receivedParameterCount == 0U);
      REQUIRE(receiverFederate.receivedTransportationType.isValid());
      REQUIRE(receiverFederate.timestampImplementation == L"HLAinteger64Time");
      REQUIRE(receiverFederate.timestampValue == 5);
      REQUIRE(receiverFederate.receivedTag ==
              std::vector<std::uint8_t>{0x44U, 0x54U, 0x35U});
      REQUIRE(receiverFederate.receivedSentOrder == rti1516_2025::TIMESTAMP);
      REQUIRE(receiverFederate.receivedReceivedOrder == rti1516_2025::TIMESTAMP);
      REQUIRE(receiverFederate.hasOptionalRetraction);
      REQUIRE(receiverFederate.optionalRetractionIsValid);
      REQUIRE(receiverFederate.timeAdvanceGrantValue == 5);
    }

    REQUIRE_NOTHROW(senderRti->resignFederationExecution(DELETE_OBJECTS));
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
    if (serverSenderConnection) {
      serverSenderConnection->close();
    }
    if (serverSecondSenderConnection) {
      serverSecondSenderConnection->close();
    }
    if (serverReceiverConnection) {
      serverReceiverConnection->close();
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
  REQUIRE(laterRecipientCount.load(std::memory_order_acquire) == 1U);
  REQUIRE(earlierRecipientCount.load(std::memory_order_acquire) == 1U);
  REQUIRE_FALSE(senderJoined);
  REQUIRE_FALSE(secondSenderJoined);
  REQUIRE_FALSE(receiverJoined);
}

}  // namespace

TEST_CASE(
    "RTIambassadors deliver a late timestamped directed interaction before pending NMR and NMRA grants",
    "[integration][foundation][interaction-management][object-management][time-management][time-advance]"
    "[transport][process-boundary][public-endpoint][timestamped-process-directed-interaction]"
    "[process-time-advance-directed-interaction-future-arrival]"
    "[rti.service.send-directed-interaction][rti.service.next-message-request]"
    "[rti.service.next-message-request-available]"
    "[federate.callback.discover-object-instance][federate.callback.receive-directed-interaction]"
    "[federate.callback.time-advance-grant][2025]") {
  SECTION("HLA_EVOKED NMR") {
    verifyProcessDirectedTsoLateArrival(
        HLA_EVOKED, NextMessageRequestKind::nmr);
  }
  SECTION("HLA_IMMEDIATE NMR") {
    verifyProcessDirectedTsoLateArrival(
        HLA_IMMEDIATE, NextMessageRequestKind::nmr);
  }
  SECTION("HLA_EVOKED NMRA") {
    verifyProcessDirectedTsoLateArrival(
        HLA_EVOKED, NextMessageRequestKind::nmra);
  }
  SECTION("HLA_IMMEDIATE NMRA") {
    verifyProcessDirectedTsoLateArrival(
        HLA_IMMEDIATE, NextMessageRequestKind::nmra);
  }
}
#endif
