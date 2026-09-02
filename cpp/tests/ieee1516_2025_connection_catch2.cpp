#include <catch2/catch_test_macros.hpp>

#include <atomic>
#include <array>
#include <cstring>
#include <exception>
#include <filesystem>
#include <fstream>
#include <limits>
#include <memory>
#include <optional>
#include <stdexcept>
#include <set>
#include <string>
#include <thread>
#include <vector>

#include <RTI/RTI1516.h>
#include <RTI/NullFederateAmbassador.h>
#include <RTI/time/HLAinteger64Time.h>
#include <RTI/time/HLAinteger64TimeFactory.h>
#include <RTI/time/HLAlogicalTimeFactoryFactory.h>

#include <umbra/embedded_profile_configuration.hpp>

#if defined(UMBRA_ENABLE_EMBEDDED_FEDERATION_MANAGEMENT)
#include "internal/federation/federation_registry.hpp"
#include "internal/federation/process_federation_service.hpp"
#include "internal/federation/process_transport.hpp"
#include "internal/federation/process_transport_session.hpp"
#include "internal/fom/libxml2_fom_composer.hpp"
#include "internal/fom/fom_validation.hpp"
#include "internal/fom/libxml2_fom_validator.hpp"
#include "internal/handles/attribute_handle.hpp"
#include "internal/handles/federate_handle.hpp"
#include "internal/handles/interaction_class_handle.hpp"
#include "internal/handles/object_instance_handle.hpp"
#include "internal/handles/parameter_handle.hpp"
#endif

namespace {

using rti1516_2025::CallbackModel;
using rti1516_2025::ConfigurationResult;
using rti1516_2025::DELETE_OBJECTS;
using rti1516_2025::HLA_EVOKED;
using rti1516_2025::HLA_IMMEDIATE;
using rti1516_2025::HLAnoCredentials;
using rti1516_2025::HLAplainTextPassword;
using rti1516_2025::NullFederateAmbassador;
using rti1516_2025::NO_ACTION;
using rti1516_2025::AttributeHandleValueMap;
using rti1516_2025::ParameterHandleValueMap;
using rti1516_2025::RTIambassador;
using rti1516_2025::RTIambassadorFactory;
using rti1516_2025::RTIinternalError;
using rti1516_2025::RtiConfiguration;
using rti1516_2025::SETTINGS_APPLIED;
using rti1516_2025::SETTINGS_IGNORED;
using rti1516_2025::Unauthorized;
using rti1516_2025::VariableLengthData;

using TestFederateAmbassador = NullFederateAmbassador;

#if defined(UMBRA_ENABLE_EMBEDDED_FEDERATION_MANAGEMENT)
using umbra::detail::EmbeddedFederationRegistry;
using umbra::detail::FederationDefinition;
using umbra::detail::FomCompositionStatus;
using umbra::detail::FomModuleKind;
using umbra::detail::FomValidationStatus;
using umbra::detail::InteractionClassDeclarationStatus;
using umbra::detail::ObjectClassAttributeDeclarationStatus;
using umbra::detail::LibXml2FomModuleComposer;
using umbra::detail::LibXml2FomValidator;
using umbra::detail::PrevalidatedFomModule;
using umbra::detail::ProcessFederationService;
using umbra::detail::ProcessFederationServiceOptions;
using umbra::detail::ProcessTransportConnection;
using umbra::detail::ProcessTransportListener;
using umbra::detail::ProcessTransportServiceDispatcher;
using umbra::detail::ProcessTransportSession;
using umbra::detail::TransportServiceMessage;
using umbra::detail::TransportServiceMessageKind;
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
  if (result.status != FomValidationStatus::valid || !result.module) {
    throw std::runtime_error("The public process FOM did not validate.");
  }
  return *result.module;
}

FederationDefinition composedProcessDefinition() {
  std::vector<PrevalidatedFomModule> modules{
      validatedProcessModule(
          processResourcePath("mim/HLAstandardMIM-2025.xml"),
          FomModuleKind::mim,
          L"urn:umbra:test:public-process-mim"),
      validatedProcessModule(
          processResourcePath("examples/RestaurantFOMmodule-2025.xml"),
          FomModuleKind::fom,
          L"urn:umbra:test:public-process-restaurant"),
  };
  LibXml2FomModuleComposer composer(
      processResourcePath("schemas/IEEE1516-FDD-2025.xsd"));
  auto result = composer.compose(modules);
  if (result.status != FomCompositionStatus::valid || !result.catalog ||
      !result.fdd) {
    throw std::runtime_error("The public process FOM did not compose.");
  }
  return {
      std::move(result.modules),
      L"HLAinteger64Time",
      std::move(result.catalog),
      std::move(result.fdd),
  };
}

FederationDefinition composedDirectedProcessDefinition() {
  auto const objectConsumer =
      std::filesystem::path(UMBRA_SOURCE_DIRECTORY) / "cpp" / "tests" /
      "data" / "directed-interaction-object-consumer-fom.xml";
  auto const interactionProvider =
      std::filesystem::path(UMBRA_SOURCE_DIRECTORY) / "cpp" / "tests" /
      "data" / "directed-interaction-interaction-provider-fom.xml";
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
    throw std::runtime_error("The public directed process FOM did not compose.");
  }
  return {
      std::move(result.modules),
      L"HLAinteger64Time",
      std::move(result.catalog),
      std::move(result.fdd),
  };
}

#endif

std::unique_ptr<RTIambassador> makeRti() {
  RTIambassadorFactory factory;
  return factory.createRTIambassador();
}

std::filesystem::path temporaryServiceReportDirectory() {
  static std::atomic_uint64_t next{0};
  return std::filesystem::temp_directory_path() /
      ("umbra-service-report-configuration-" + std::to_string(++next));
}

void requireIgnoredConfiguration(ConfigurationResult const& result) {
  REQUIRE_FALSE(result.configurationUsed);
  REQUIRE_FALSE(result.addressUsed);
  REQUIRE(result.additionalSettingsResult == SETTINGS_IGNORED);
  REQUIRE(result.message.empty());
}

}  // namespace

#if defined(UMBRA_ENABLE_EMBEDDED_FEDERATION_MANAGEMENT)
TEST_CASE(
    "RTIambassador selects a configured tcp process endpoint through the official address field",
    "[integration][foundation][connection][transport][process-boundary][public-endpoint]") {
  using umbra::detail::ProcessTransportListener;

  auto listener = ProcessTransportListener::listen({"127.0.0.1", 0U});
  REQUIRE(listener);
  auto const port = listener->address().port;
  REQUIRE(port != 0U);

  std::exception_ptr serverError;
  std::thread server([&] {
    try {
      auto connection = listener->accept(
          nullptr,
          {"process-endpoint-test-server", 0x9101U},
          [](std::wstring) {},
          [](std::wstring) { return false; });
      connection->close();
    } catch (...) {
      serverError = std::current_exception();
    }
  });

  TestFederateAmbassador federate;
  auto rti = makeRti();
  auto configuration = RtiConfiguration::createConfiguration()
                           .withConfigurationName(L"process-endpoint-test-client")
                           .withRtiAddress(
                               L"tcp://127.0.0.1:" + std::to_wstring(port));

  std::exception_ptr clientError;
  std::optional<ConfigurationResult> result;
  try {
    result = rti->connect(federate, HLA_EVOKED, configuration);
    rti->disconnect();
  } catch (...) {
    clientError = std::current_exception();
  }
  if (listener) {
    listener.reset();
  }
  if (server.joinable()) {
    server.join();
  }
  if (clientError) {
    std::rethrow_exception(clientError);
  }
  REQUIRE_FALSE(serverError);
  REQUIRE(result.has_value());
  REQUIRE(result->configurationUsed);
  REQUIRE(result->addressUsed);
  REQUIRE(result->additionalSettingsResult == SETTINGS_IGNORED);
}

TEST_CASE(
    "RTIambassador routes public Create, Join, and Resign through a configured process endpoint",
    "[integration][foundation][federation-management][transport][process-boundary][public-endpoint]") {
  using umbra::detail::EmbeddedFederationRegistry;
  using umbra::detail::FederationDefinition;
  using umbra::detail::FomModuleKind;
  using umbra::detail::PrevalidatedFomModule;
  using umbra::detail::ProcessFederationService;
  using umbra::detail::ProcessTransportListener;
  using umbra::detail::ProcessTransportServiceDispatcher;
  using umbra::detail::ProcessTransportSession;

  auto listener = ProcessTransportListener::listen({"127.0.0.1", 0U});
  REQUIRE(listener);
  auto const port = listener->address().port;
  REQUIRE(port != 0U);

  std::exception_ptr serverError;
  std::thread server([&] {
    try {
      EmbeddedFederationRegistry registry;
      FederationDefinition definition;
      definition.fomModules.push_back(PrevalidatedFomModule{
          L"urn:umbra:test:process-public-fom",
          {},
          {},
          FomModuleKind::fom,
          L"IEEE1516-DIF-2025.xsd",
          {},
          umbra::detail::FomStandardEdition::ieee1516_2025,
          umbra::detail::FomSourceCompatibility::strict,
          {}});
      definition.logicalTimeImplementationName = L"HLAinteger64Time";
      ProcessFederationService service(registry, std::move(definition));
      auto connection = listener->accept(
          nullptr,
          {"process-public-endpoint-server", 0x9201U},
          [](std::wstring) {},
          [](std::wstring) { return false; });
      ProcessTransportSession session(connection);
      auto handler = service.handlerFor(session);
      if (!ProcessTransportServiceDispatcher::serveOne(session, handler) ||
          !ProcessTransportServiceDispatcher::serveOne(session, handler) ||
          !ProcessTransportServiceDispatcher::serveOne(session, handler)) {
        throw std::runtime_error(
            "The public process endpoint server did not receive Create and Join.");
      }
      service.detach(session);
      connection->close();
    } catch (...) {
      serverError = std::current_exception();
    }
  });

  TestFederateAmbassador federate;
  auto rti = makeRti();
  auto configuration = RtiConfiguration::createConfiguration()
                           .withConfigurationName(L"process-public-endpoint-client")
                           .withRtiAddress(
                               L"tcp://127.0.0.1:" + std::to_wstring(port));
  std::exception_ptr clientError;
  std::optional<ConfigurationResult> connectionResult;
  std::optional<rti1516_2025::FederateHandle> joinedHandle;
  try {
    connectionResult = rti->connect(federate, HLA_EVOKED, configuration);
    rti->createFederationExecution(
        L"process-public-execution", L"server-owned-fom.xml");
    joinedHandle = rti->joinFederationExecution(
        L"process-public-type", L"process-public-execution");
    rti->resignFederationExecution(NO_ACTION);
    rti->disconnect();
  } catch (...) {
    clientError = std::current_exception();
  }
  if (listener) {
    listener.reset();
  }
  if (server.joinable()) {
    server.join();
  }
  if (clientError) {
    std::rethrow_exception(clientError);
  }
  REQUIRE_FALSE(serverError);
  REQUIRE(connectionResult.has_value());
  REQUIRE(connectionResult->addressUsed);
  REQUIRE(joinedHandle.has_value());
  REQUIRE(joinedHandle->isValid());
}

TEST_CASE(
    "RTIambassador publishes object-class attributes and registers an object through a configured process endpoint",
    "[integration][foundation][declaration-management][object-management][transport][process-boundary][public-endpoint][rti.service.publish-object-class-attributes][rti.service.register-object-instance]") {
  auto listener = ProcessTransportListener::listen({"127.0.0.1", 0U});
  REQUIRE(listener);
  auto const port = listener->address().port;
  REQUIRE(port != 0U);

  constexpr wchar_t const* federationName =
      L"public-process-object-registration-execution";
  constexpr char const* objectName = "HLAobjectRoot.Customer";
  constexpr char const* attributeName = "HLAprivilegeToDeleteObject";
  std::atomic_uint64_t expectedObjectClass{0U};
  std::atomic_uint64_t expectedAttribute{0U};
  std::exception_ptr serverError;
  std::thread server([&] {
    try {
      EmbeddedFederationRegistry registry;
      ProcessFederationService service(
          registry, composedProcessDefinition(), ProcessFederationServiceOptions{});
      auto connection = listener->accept(
          nullptr,
          {"public-process-object-registration-server", 0x9601U},
          [](std::wstring) {},
          [](std::wstring) { return false; });
      ProcessTransportSession session(connection);
      auto handler = service.handlerFor(session);
      auto serveExpected = [&](TransportServiceOperation operation) {
        return ProcessTransportServiceDispatcher::serveOne(
            session,
            [&](TransportServiceMessage const& request) {
              if (request.operation != operation) {
                throw std::runtime_error(
                    "The public process object-registration server received an unexpected operation.");
              }
              return handler(request);
            });
      };

      if (!serveExpected(TransportServiceOperation::create_federation_execution)) {
        throw std::runtime_error(
            "The public process object-registration server lost Create.");
      }
      auto const objectClass = registry.objectClassHandleFor(federationName, objectName);
      auto const attribute = registry.attributeHandleFor(
          federationName, objectName, attributeName);
      if (!objectClass || !attribute) {
        throw std::runtime_error(
            "The public process object-registration server could not resolve its FOM handles.");
      }
      expectedObjectClass.store(*objectClass, std::memory_order_release);
      expectedAttribute.store(*attribute, std::memory_order_release);

      if (!serveExpected(TransportServiceOperation::join_federation_execution) ||
          !serveExpected(TransportServiceOperation::get_object_class_handle) ||
          !serveExpected(TransportServiceOperation::get_attribute_handle) ||
          !serveExpected(TransportServiceOperation::publish_object_class_attributes)) {
        throw std::runtime_error(
            "The public process object-registration server lost Join, lookup, or publication.");
      }
      auto const member = registry.memberByName(
          federationName, L"public-process-object-registration-federate");
      if (!member) {
        throw std::runtime_error(
            "The public process object-registration server could not resolve its federate.");
      }
      auto const published = registry.publishedObjectClassAttributeHandles(
          federationName, member->id, *objectClass);
      if (!published || !published->contains(*attribute)) {
        throw std::runtime_error(
            "The public process object-registration publication did not reach the registry.");
      }

      if (!serveExpected(TransportServiceOperation::register_object_instance) ||
          !serveExpected(TransportServiceOperation::resign_federation_execution)) {
        throw std::runtime_error(
            "The public process object-registration server lost registration or Resign.");
      }
      service.detach(session);
      connection->close();
    } catch (...) {
      serverError = std::current_exception();
    }
  });

  TestFederateAmbassador federate;
  auto rti = makeRti();
  auto configuration = RtiConfiguration::createConfiguration()
                           .withConfigurationName(L"public-process-object-registration-client")
                           .withRtiAddress(
                               L"tcp://127.0.0.1:" + std::to_wstring(port));
  std::exception_ptr clientError;
  bool clientJoined = false;
  std::optional<rti1516_2025::FederateHandle> joinedHandle;
  try {
    auto const connectionResult = rti->connect(federate, HLA_EVOKED, configuration);
    REQUIRE(connectionResult.addressUsed);
    rti->createFederationExecution(
        federationName, L"server-owned-fom.xml");
    joinedHandle = rti->joinFederationExecution(
        L"public-process-object-registration-federate",
        L"public-process-object-registration-type",
        federationName);
    clientJoined = true;

    auto const objectClass = rti->getObjectClassHandle(
        L"HLAobjectRoot.Customer");
    REQUIRE(objectClass.toString() ==
            L"ObjectClassHandle(" +
                std::to_wstring(
                    expectedObjectClass.load(std::memory_order_acquire)) +
                L")");
    auto const attribute = rti->getAttributeHandle(
        objectClass, L"HLAprivilegeToDeleteObject");
    REQUIRE(attribute.isValid());
    rti1516_2025::AttributeHandleSet attributes;
    attributes.insert(attribute);
    rti->publishObjectClassAttributes(objectClass, attributes);
    auto const objectInstance = rti->registerObjectInstance(objectClass);
    REQUIRE(objectInstance.isValid());
    REQUIRE_FALSE(objectInstance.toString().empty());
    rti->resignFederationExecution(NO_ACTION);
    rti->disconnect();
  } catch (...) {
    clientError = std::current_exception();
    if (clientJoined) {
      try {
        rti->resignFederationExecution(NO_ACTION);
      } catch (...) {
      }
    }
    try {
      rti->disconnect();
    } catch (...) {
    }
  }
  if (listener) {
    listener.reset();
  }
  if (server.joinable()) {
    server.join();
  }
  if (clientError) {
    std::rethrow_exception(clientError);
  }
  REQUIRE_FALSE(serverError);
  REQUIRE(joinedHandle.has_value());
  REQUIRE(joinedHandle->isValid());
}

TEST_CASE(
    "RTIambassador rejects malformed tcp process addresses before connecting",
    "[integration][foundation][connection][transport][process-boundary][public-endpoint]") {
  TestFederateAmbassador federate;
  auto rti = makeRti();
  auto malformed = RtiConfiguration::createConfiguration()
                       .withRtiAddress(L"tcp://127.0.0.1:not-a-port");

  REQUIRE_THROWS_AS(
      rti->connect(federate, HLA_EVOKED, malformed),
      rti1516_2025::ConnectionFailed);
  REQUIRE_NOTHROW(rti->connect(federate, HLA_EVOKED));
  REQUIRE_NOTHROW(rti->disconnect());
}

TEST_CASE(
    "RTIambassador routes Local Delete Object Instance through a configured process endpoint",
    "[integration][foundation][object-management][transport][process-boundary][public-endpoint][local-delete-object-instance][rti.service.local-delete-object-instance]") {
  class RecordingFederateAmbassador final : public NullFederateAmbassador {
   public:
    void discoverObjectInstance(
        rti1516_2025::ObjectInstanceHandle const& objectInstance,
        rti1516_2025::ObjectClassHandle const& objectClass,
        std::wstring const& objectInstanceName,
        rti1516_2025::FederateHandle const& producingFederate) override {
      discovered = true;
      discoveredObjectInstance = objectInstance;
      discoveredObjectClass = objectClass;
      discoveredObjectInstanceName = objectInstanceName;
      discoveredProducingFederate = producingFederate;
    }

    bool discovered = false;
    rti1516_2025::ObjectInstanceHandle discoveredObjectInstance;
    rti1516_2025::ObjectClassHandle discoveredObjectClass;
    std::wstring discoveredObjectInstanceName;
    rti1516_2025::FederateHandle discoveredProducingFederate;
  } receiverFederate;

  auto listener = ProcessTransportListener::listen({"127.0.0.1", 0U});
  REQUIRE(listener);
  auto const port = listener->address().port;
  REQUIRE(port != 0U);

  constexpr wchar_t const* federationName =
      L"public-process-local-delete-execution";
  constexpr char const* objectClassName = "HLAobjectRoot.Employee";
  constexpr wchar_t const* objectClassNameWide = L"HLAobjectRoot.Employee";
  constexpr char const* attributeName = "Name";
  constexpr wchar_t const* attributeNameWide = L"Name";
  std::atomic_uint64_t expectedObjectClass{0U};
  std::atomic_uint64_t expectedAttribute{0U};
  std::atomic_uint64_t expectedSenderFederate{0U};
  std::atomic_uint64_t expectedReceiverFederate{0U};
  std::atomic_uint64_t registeredObject{0U};
  std::atomic_bool localDeleteApplied{false};
  std::exception_ptr serverError;
  std::thread server([&] {
    try {
      EmbeddedFederationRegistry registry;
      ProcessFederationService service(
          registry, composedProcessDefinition(), ProcessFederationServiceOptions{});
      auto senderConnection = listener->accept(
          nullptr,
          {"public-process-local-delete-server", 0x9B01U},
          [](std::wstring) {},
          [](std::wstring) { return false; });
      ProcessTransportSession sender(senderConnection);
      auto senderBaseHandler = service.handlerFor(sender);
      auto senderHandler = [&](TransportServiceMessage const& request) {
        auto response = senderBaseHandler(request);
        if (request.operation == TransportServiceOperation::join_federation_execution &&
            response.status == TransportServiceStatus::ok) {
          auto const result =
              umbra::detail::decodeProcessFederationJoinResult(response.payload);
          expectedSenderFederate.store(result.federateId, std::memory_order_release);
        }
        if (request.operation == TransportServiceOperation::register_object_instance &&
            response.status == TransportServiceStatus::ok) {
          auto const result =
              umbra::detail::decodeProcessFederationRegisterObjectInstanceResult(
                  response.payload);
          registeredObject.store(
              result.objectInstanceHandle, std::memory_order_release);
        }
        return response;
      };
      auto serveExpected = [&](ProcessTransportSession& session,
                               auto const& handler,
                               TransportServiceOperation operation,
                               char const* description) {
        if (!ProcessTransportServiceDispatcher::serveOne(
                session,
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
          "The public process local-delete server lost Create.");
      auto const objectClass = registry.objectClassHandleFor(
          federationName, objectClassName);
      auto const attribute = registry.attributeHandleFor(
          federationName, objectClassName, attributeName);
      if (!objectClass || !attribute) {
        throw std::runtime_error(
            "The public process local-delete server could not resolve its FOM handles.");
      }
      expectedObjectClass.store(*objectClass, std::memory_order_release);
      expectedAttribute.store(*attribute, std::memory_order_release);
      serveExpected(
          sender,
          senderHandler,
          TransportServiceOperation::join_federation_execution,
          "The public process local-delete server lost sender Join.");

      auto receiverConnection = listener->accept(
          nullptr,
          {"public-process-local-delete-server", 0x9B02U},
          [](std::wstring) {},
          [](std::wstring) { return false; });
      ProcessTransportSession receiver(receiverConnection);
      auto receiverBaseHandler = service.handlerFor(receiver);
      auto receiverHandler = [&](TransportServiceMessage const& request) {
        auto response = receiverBaseHandler(request);
        if (request.operation == TransportServiceOperation::join_federation_execution &&
            response.status == TransportServiceStatus::ok) {
          auto const result =
              umbra::detail::decodeProcessFederationJoinResult(response.payload);
          expectedReceiverFederate.store(result.federateId, std::memory_order_release);
        }
        if (request.operation ==
                TransportServiceOperation::local_delete_object_instance &&
            response.status == TransportServiceStatus::ok) {
          auto const result =
              umbra::detail::decodeProcessFederationLocalDeleteObjectInstanceResult(
                  response.payload);
          localDeleteApplied.store(
              result.status ==
                  umbra::detail::LocalObjectInstanceDeletionStatus::applied,
              std::memory_order_release);
        }
        return response;
      };
      serveExpected(
          receiver,
          receiverHandler,
          TransportServiceOperation::join_federation_execution,
          "The public process local-delete server lost receiver Join.");
      serveExpected(
          sender,
          senderHandler,
          TransportServiceOperation::get_object_class_handle,
          "The public process local-delete server lost sender object-class lookup.");
      serveExpected(
          sender,
          senderHandler,
          TransportServiceOperation::get_attribute_handle,
          "The public process local-delete server lost sender attribute lookup.");
      serveExpected(
          receiver,
          receiverHandler,
          TransportServiceOperation::get_object_class_handle,
          "The public process local-delete server lost receiver object-class lookup.");
      serveExpected(
          receiver,
          receiverHandler,
          TransportServiceOperation::get_attribute_handle,
          "The public process local-delete server lost receiver attribute lookup.");
      serveExpected(
          receiver,
          receiverHandler,
          TransportServiceOperation::subscribe_object_class_attributes,
          "The public process local-delete server lost receiver Subscribe.");
      serveExpected(
          sender,
          senderHandler,
          TransportServiceOperation::publish_object_class_attributes,
          "The public process local-delete server lost sender Publish.");
      serveExpected(
          sender,
          senderHandler,
          TransportServiceOperation::register_object_instance,
          "The public process local-delete server lost sender Register.");
      serveExpected(
          receiver,
          receiverHandler,
          TransportServiceOperation::receive_interaction,
          "The public process local-delete server lost receiver discovery poll.");
      serveExpected(
          receiver,
          receiverHandler,
          TransportServiceOperation::local_delete_object_instance,
          "The public process local-delete server lost receiver Local Delete.");
      auto const member = registry.memberByName(
          federationName, L"public-process-local-delete-receiver");
      if (!member || expectedReceiverFederate.load(std::memory_order_acquire) == 0U ||
          registeredObject.load(std::memory_order_acquire) == 0U ||
          registry.knownObjectInstanceFor(
              federationName,
              member->id,
              registeredObject.load(std::memory_order_acquire))) {
        throw std::runtime_error(
            "The public process local-delete transition did not clear recipient-local knowledge.");
      }
      serveExpected(
          sender,
          senderHandler,
          TransportServiceOperation::resign_federation_execution,
          "The public process local-delete server lost sender Resign.");
      serveExpected(
          receiver,
          receiverHandler,
          TransportServiceOperation::resign_federation_execution,
          "The public process local-delete server lost receiver Resign.");
      service.detach(sender);
      service.detach(receiver);
      senderConnection->close();
      receiverConnection->close();
    } catch (...) {
      serverError = std::current_exception();
    }
  });

  TestFederateAmbassador senderFederate;
  auto senderRti = makeRti();
  auto receiverRti = makeRti();
  auto senderConfiguration = RtiConfiguration::createConfiguration()
                                 .withConfigurationName(
                                     L"public-process-local-delete-sender")
                                 .withRtiAddress(
                                     L"tcp://127.0.0.1:" + std::to_wstring(port));
  auto receiverConfiguration = RtiConfiguration::createConfiguration()
                                   .withConfigurationName(
                                       L"public-process-local-delete-receiver")
                                   .withRtiAddress(
                                       L"tcp://127.0.0.1:" + std::to_wstring(port));
  std::exception_ptr clientError;
  bool senderJoined = false;
  bool receiverJoined = false;
  try {
    REQUIRE(senderRti->connect(
                senderFederate, HLA_EVOKED, senderConfiguration)
                .addressUsed);
    senderRti->createFederationExecution(federationName, L"server-owned-fom.xml");
    senderRti->joinFederationExecution(
        L"public-process-local-delete-sender",
        L"public-process-local-delete-type",
        federationName);
    senderJoined = true;

    REQUIRE(receiverRti->connect(
                receiverFederate, HLA_EVOKED, receiverConfiguration)
                .addressUsed);
    receiverRti->joinFederationExecution(
        L"public-process-local-delete-receiver",
        L"public-process-local-delete-type",
        federationName);
    receiverJoined = true;

    auto const senderObjectClass =
        senderRti->getObjectClassHandle(objectClassNameWide);
    auto const senderAttribute =
        senderRti->getAttributeHandle(senderObjectClass, attributeNameWide);
    REQUIRE(senderObjectClass.toString() ==
            L"ObjectClassHandle(" +
                std::to_wstring(expectedObjectClass.load(std::memory_order_acquire)) +
                L")");
    REQUIRE(senderAttribute ==
            rti1516_2025::umbra_binding_detail::makeAttributeHandle(
                expectedAttribute.load(std::memory_order_acquire)));

    auto const receiverObjectClass =
        receiverRti->getObjectClassHandle(objectClassNameWide);
    auto const receiverAttribute =
        receiverRti->getAttributeHandle(receiverObjectClass, attributeNameWide);
    rti1516_2025::AttributeHandleSet receiverAttributes;
    receiverAttributes.insert(receiverAttribute);
    receiverRti->subscribeObjectClassAttributes(
        receiverObjectClass, receiverAttributes);

    rti1516_2025::AttributeHandleSet senderAttributes;
    senderAttributes.insert(senderAttribute);
    senderRti->publishObjectClassAttributes(senderObjectClass, senderAttributes);

    auto const objectInstance =
        senderRti->registerObjectInstance(senderObjectClass);
    REQUIRE(objectInstance.isValid());

    for (std::size_t evokeCount = 0U; evokeCount < 4U &&
         !receiverFederate.discovered; ++evokeCount) {
      static_cast<void>(receiverRti->evokeCallback(0.0));
    }
    REQUIRE(receiverFederate.discovered);
    REQUIRE(receiverFederate.discoveredObjectInstance == objectInstance);
    REQUIRE(receiverFederate.discoveredObjectClass == receiverObjectClass);
    REQUIRE(receiverFederate.discoveredProducingFederate.isValid());
    REQUIRE_NOTHROW(
        receiverRti->localDeleteObjectInstance(
            receiverFederate.discoveredObjectInstance));

    senderRti->resignFederationExecution(DELETE_OBJECTS);
    senderJoined = false;
    receiverRti->resignFederationExecution(NO_ACTION);
    receiverJoined = false;
    senderRti->disconnect();
    receiverRti->disconnect();
  } catch (...) {
    clientError = std::current_exception();
    if (senderJoined) {
      try {
        senderRti->resignFederationExecution(DELETE_OBJECTS);
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
  if (listener) {
    listener.reset();
  }
  if (server.joinable()) {
    server.join();
  }
  if (clientError) {
    std::rethrow_exception(clientError);
  }
  REQUIRE_FALSE(serverError);
  REQUIRE(localDeleteApplied.load(std::memory_order_acquire));
  REQUIRE(expectedSenderFederate.load(std::memory_order_acquire) != 0U);
  REQUIRE(expectedReceiverFederate.load(std::memory_order_acquire) != 0U);
  REQUIRE_FALSE(senderJoined);
  REQUIRE_FALSE(receiverJoined);
}
TEST_CASE(
    "RTIambassador reserves a name and registers a named object through a configured process endpoint",
    "[integration][foundation][declaration-management][object-management][callbacks][callback-controls][transport][process-boundary][public-endpoint][rti.service.reserve-object-instance-name][rti.service.register-object-instance][federate.callback.object-instance-name-reservation-succeeded]") {
  class RecordingFederateAmbassador final : public NullFederateAmbassador {
   public:
    void objectInstanceNameReservationSucceeded(
        std::wstring const& objectInstanceName) override {
      succeeded = true;
      name = objectInstanceName;
    }

    bool succeeded = false;
    std::wstring name;
  } federate;

  auto listener = ProcessTransportListener::listen({"127.0.0.1", 0U});
  REQUIRE(listener);
  auto const port = listener->address().port;
  REQUIRE(port != 0U);

  constexpr wchar_t const* federationName =
      L"public-process-named-registration-execution";
  constexpr wchar_t const* requestedName = L"process-named-object";
  constexpr char const* objectName = "HLAobjectRoot.Customer";
  constexpr char const* attributeName = "HLAprivilegeToDeleteObject";
  std::atomic_uint64_t expectedObjectClass{0U};
  std::atomic_uint64_t expectedAttribute{0U};
  std::atomic_uint64_t expectedObjectInstance{0U};
  std::atomic_bool duplicateRegistrationRejected{false};
  std::atomic_bool illegalReservationRejected{false};
  std::atomic_bool reservationAccepted{false};
  std::exception_ptr serverError;
  std::thread server([&] {
    try {
      EmbeddedFederationRegistry registry;
      ProcessFederationService service(
          registry, composedProcessDefinition(), ProcessFederationServiceOptions{});
      auto connection = listener->accept(
          nullptr,
          {"public-process-named-registration-server", 0x9A01U},
          [](std::wstring) {},
          [](std::wstring) { return false; });
      ProcessTransportSession session(connection);
      auto handler = service.handlerFor(session);
      std::size_t registrationCount = 0U;
      std::size_t reservationCount = 0U;
      auto serveExpected = [&](TransportServiceOperation operation) {
        return ProcessTransportServiceDispatcher::serveOne(
            session,
            [&](TransportServiceMessage const& request) {
              if (request.operation != operation) {
                throw std::runtime_error(
                    "The public process named-registration server received an unexpected operation.");
              }
              auto response = handler(request);
              if (operation == TransportServiceOperation::reserve_object_instance_name &&
                  response.status == TransportServiceStatus::ok) {
                auto const result =
                    umbra::detail::decodeProcessFederationReserveObjectInstanceNameResult(
                        response.payload);
                ++reservationCount;
                if (reservationCount == 1U) {
                  reservationAccepted.store(
                      result.succeeded &&
                          result.status ==
                              umbra::detail::ObjectInstanceNameReservationStatus::applied &&
                          result.objectInstanceName == requestedName,
                      std::memory_order_release);
                } else if (
                    result.status ==
                    umbra::detail::ObjectInstanceNameReservationStatus::illegal_name) {
                  illegalReservationRejected.store(
                      true, std::memory_order_release);
                } else {
                  throw std::runtime_error(
                      "The process service did not report illegal reservation input.");
                }
              }
              if (operation == TransportServiceOperation::register_object_instance &&
                  response.status == TransportServiceStatus::ok) {
                auto const result =
                    umbra::detail::decodeProcessFederationRegisterObjectInstanceResult(
                        response.payload);
                ++registrationCount;
                if (registrationCount == 1U) {
                  expectedObjectInstance.store(
                      result.objectInstanceHandle, std::memory_order_release);
                  if (result.status !=
                          umbra::detail::ObjectInstanceRegistrationStatus::applied ||
                      result.objectInstanceName != requestedName) {
                    throw std::runtime_error(
                        "The process service did not preserve the reserved object-instance name.");
                  }
                } else if (
                    result.status ==
                    umbra::detail::ObjectInstanceRegistrationStatus::object_instance_name_in_use) {
                  duplicateRegistrationRejected.store(
                      true, std::memory_order_release);
                } else {
                  throw std::runtime_error(
                      "The process service did not report duplicate named registration as in use.");
                }
              }
              return response;
            });
      };

      if (!serveExpected(TransportServiceOperation::create_federation_execution)) {
        throw std::runtime_error(
            "The public process named-registration server lost Create.");
      }
      auto const objectClass = registry.objectClassHandleFor(federationName, objectName);
      auto const attribute = registry.attributeHandleFor(
          federationName, objectName, attributeName);
      if (!objectClass || !attribute) {
        throw std::runtime_error(
            "The public process named-registration server could not resolve its FOM handles.");
      }
      expectedObjectClass.store(*objectClass, std::memory_order_release);
      expectedAttribute.store(*attribute, std::memory_order_release);
      if (!serveExpected(TransportServiceOperation::join_federation_execution) ||
          !serveExpected(TransportServiceOperation::get_object_class_handle) ||
          !serveExpected(TransportServiceOperation::get_attribute_handle) ||
          !serveExpected(TransportServiceOperation::publish_object_class_attributes) ||
          !serveExpected(TransportServiceOperation::reserve_object_instance_name) ||
          !serveExpected(TransportServiceOperation::register_object_instance) ||
          !serveExpected(TransportServiceOperation::register_object_instance) ||
          !serveExpected(TransportServiceOperation::reserve_object_instance_name) ||
          !serveExpected(TransportServiceOperation::resign_federation_execution)) {
        throw std::runtime_error(
            "The public process named-registration server lost a required operation.");
      }
      service.detach(session);
      connection->close();
    } catch (...) {
      serverError = std::current_exception();
    }
  });

  auto rti = makeRti();
  auto configuration = RtiConfiguration::createConfiguration()
                           .withConfigurationName(L"public-process-named-registration-client")
                           .withRtiAddress(
                               L"tcp://127.0.0.1:" + std::to_wstring(port));
  std::exception_ptr clientError;
  bool clientJoined = false;
  try {
    REQUIRE(rti->connect(federate, HLA_EVOKED, configuration).addressUsed);
    rti->createFederationExecution(federationName, L"server-owned-fom.xml");
    rti->joinFederationExecution(
        L"public-process-named-registration-federate",
        L"public-process-named-registration-type",
        federationName);
    clientJoined = true;

    auto const objectClass = rti->getObjectClassHandle(L"HLAobjectRoot.Customer");
    REQUIRE(objectClass.toString() ==
            L"ObjectClassHandle(" +
                std::to_wstring(expectedObjectClass.load(std::memory_order_acquire)) +
                L")");
    auto const attribute = rti->getAttributeHandle(
        objectClass, L"HLAprivilegeToDeleteObject");
    REQUIRE(attribute == rti1516_2025::umbra_binding_detail::makeAttributeHandle(
                             expectedAttribute.load(std::memory_order_acquire)));
    rti1516_2025::AttributeHandleSet attributes;
    attributes.insert(attribute);
    rti->publishObjectClassAttributes(objectClass, attributes);

    rti->reserveObjectInstanceName(requestedName);
    REQUIRE_FALSE(federate.succeeded);
    static_cast<void>(rti->evokeCallback(0.0));
    REQUIRE(federate.succeeded);
    REQUIRE(federate.name == requestedName);

    auto const objectInstance = rti->registerObjectInstance(objectClass, requestedName);
    REQUIRE(objectInstance.isValid());
    REQUIRE(objectInstance.toString() ==
            L"ObjectInstanceHandle(" +
                std::to_wstring(expectedObjectInstance.load(std::memory_order_acquire)) +
                L")");
    REQUIRE_THROWS_AS(
        rti->registerObjectInstance(objectClass, requestedName),
        rti1516_2025::ObjectInstanceNameInUse);
    REQUIRE_THROWS_AS(
        rti->reserveObjectInstanceName(L"HLA.illegal-process-name"),
        rti1516_2025::IllegalName);
    rti->resignFederationExecution(NO_ACTION);
    clientJoined = false;
    rti->disconnect();
  } catch (...) {
    clientError = std::current_exception();
    if (clientJoined) {
      try {
        rti->resignFederationExecution(NO_ACTION);
      } catch (...) {
      }
    }
    try {
      rti->disconnect();
    } catch (...) {
    }
  }
  if (listener) {
    listener.reset();
  }
  if (server.joinable()) {
    server.join();
  }
  if (clientError) {
    std::rethrow_exception(clientError);
  }
  REQUIRE_FALSE(serverError);
  REQUIRE(reservationAccepted.load(std::memory_order_acquire));
  REQUIRE(expectedObjectInstance.load(std::memory_order_acquire) != 0U);
  REQUIRE(duplicateRegistrationRejected.load(std::memory_order_acquire));
  REQUIRE(illegalReservationRejected.load(std::memory_order_acquire));
  REQUIRE_FALSE(clientJoined);
}

TEST_CASE(
    "RTIambassador routes public Send Interaction through a configured process endpoint",
    "[integration][foundation][interaction-management][transport][process-boundary][public-endpoint]") {
  auto listener = ProcessTransportListener::listen({"127.0.0.1", 0U});
  REQUIRE(listener);
  auto const port = listener->address().port;
  REQUIRE(port != 0U);

  std::atomic_uint64_t interactionClassValue{0U};
  std::atomic_uint64_t parameterValue{0U};
  std::exception_ptr serverError;
  std::thread server([&] {
    try {
      EmbeddedFederationRegistry registry;
      ProcessFederationService service(
          registry, composedProcessDefinition(), ProcessFederationServiceOptions{});
      auto senderConnection = listener->accept(
          nullptr,
          {"public-process-message-server", 0x9301U},
          [](std::wstring) {},
          [](std::wstring) { return false; });
      ProcessTransportSession sender(senderConnection);
      auto senderHandler = service.handlerFor(sender);
      if (!ProcessTransportServiceDispatcher::serveOne(sender, senderHandler)) {
        throw std::runtime_error("The public process message server lost Create.");
      }

      auto const interactionClass = registry.interactionClassHandleFor(
          L"public-process-message-execution",
          "HLAinteractionRoot.CustomerTransactions.FoodServed.MainCourseServed");
      if (!interactionClass) {
        throw std::runtime_error(
            "The public process message server could not resolve its interaction.");
      }
      auto const parameter = registry.parameterHandleFor(
          L"public-process-message-execution",
          "HLAinteractionRoot.CustomerTransactions.FoodServed.MainCourseServed",
          "TimelinessOk");
      if (!parameter) {
        throw std::runtime_error(
            "The public process message server could not resolve its parameter.");
      }
      interactionClassValue.store(*interactionClass, std::memory_order_release);
      parameterValue.store(*parameter, std::memory_order_release);

      if (!ProcessTransportServiceDispatcher::serveOne(sender, senderHandler)) {
        throw std::runtime_error("The public process message server lost Join.");
      }

      auto receiverConnection = listener->accept(
          nullptr,
          {"public-process-message-server", 0x9302U},
          [](std::wstring) {},
          [](std::wstring) { return false; });
      ProcessTransportSession receiver(receiverConnection);
      auto receiverHandler = service.handlerFor(receiver);
      if (!ProcessTransportServiceDispatcher::serveOne(receiver, receiverHandler)) {
        throw std::runtime_error("The public process message server lost receiver Join.");
      }

      auto const senderMember = registry.memberByName(
          L"public-process-message-execution", L"public-process-message-federate");
      auto const receiverMember = registry.memberByName(
          L"public-process-message-execution", L"public-process-message-receiver");
      if (!senderMember || !receiverMember) {
        throw std::runtime_error(
            "The public process message member names were not registered.");
      }
      auto const publicationStatus = registry.setInteractionClassPublication(
          L"public-process-message-execution",
          senderMember->id,
          *interactionClass,
          true);
      if (publicationStatus != InteractionClassDeclarationStatus::applied) {
        throw std::runtime_error(
            "The public process message publication was rejected (" +
            std::to_string(static_cast<int>(publicationStatus)) + ").");
      }
      auto const subscriptionStatus = registry.setInteractionClassSubscription(
          L"public-process-message-execution",
          receiverMember->id,
          *interactionClass,
          true);
      if (subscriptionStatus != InteractionClassDeclarationStatus::applied) {
        throw std::runtime_error(
            "The public process message subscription was rejected (" +
            std::to_string(static_cast<int>(subscriptionStatus)) + ").");
      }

      if (!ProcessTransportServiceDispatcher::serveOne(sender, senderHandler)) {
        throw std::runtime_error("The public process message server lost Send.");
      }

      if (!ProcessTransportServiceDispatcher::serveOne(receiver, receiverHandler)) {
        throw std::runtime_error("The public process message server lost Receive.");
      }

      if (!ProcessTransportServiceDispatcher::serveOne(sender, senderHandler)) {
        throw std::runtime_error("The public process message server lost Resign.");
      }
      service.detach(sender);
      service.detach(receiver);
      senderConnection->close();
      receiverConnection->close();
    } catch (...) {
      serverError = std::current_exception();
    }
  });

  TestFederateAmbassador federate;
  auto rti = makeRti();
  auto configuration = RtiConfiguration::createConfiguration()
                           .withConfigurationName(L"public-process-message-client")
                           .withRtiAddress(
                               L"tcp://127.0.0.1:" + std::to_wstring(port));
  std::exception_ptr clientError;
  std::optional<rti1516_2025::FederateHandle> joinedHandle;
  std::shared_ptr<ProcessTransportConnection> receiverConnection;
  bool clientJoined = false;
  try {
    auto const connectionResult = rti->connect(federate, HLA_EVOKED, configuration);
    REQUIRE(connectionResult.addressUsed);
    rti->createFederationExecution(
        L"public-process-message-execution", L"server-owned-fom.xml");
    joinedHandle = rti->joinFederationExecution(
        L"public-process-message-federate",
        L"public-process-message-type",
        L"public-process-message-execution");
    clientJoined = true;

    receiverConnection = ProcessTransportConnection::connectClient(
        nullptr,
        {"127.0.0.1", port},
        {"public-process-message-receiver", 0x9303U},
        [](std::wstring) {},
        [](std::wstring) { return false; });
    ProcessTransportSession receiver(receiverConnection);
    TransportServiceMessage receiverJoinResponse;
    if (!receiver.request(
            TransportServiceMessage{
                TransportServiceMessageKind::request,
                TransportServiceOperation::join_federation_execution,
                TransportServiceStatus::ok,
                1U,
                umbra::detail::encodeProcessFederationJoinRequest(
                    umbra::detail::ProcessFederationJoinRequest{
                        L"public-process-message-execution",
                        L"public-process-message-receiver",
                        L"public-process-message-receiver"})},
            receiverJoinResponse) ||
        receiverJoinResponse.status != TransportServiceStatus::ok) {
      throw std::runtime_error("The public process receiver Join request failed.");
    }
    auto const receiverJoin =
        umbra::detail::decodeProcessFederationJoinResult(
            receiverJoinResponse.payload);

    auto const interactionClass =
        rti1516_2025::umbra_binding_detail::makeInteractionClassHandle(
            interactionClassValue.load(std::memory_order_acquire));
    auto const parameter =
        rti1516_2025::umbra_binding_detail::makeParameterHandle(
            parameterValue.load(std::memory_order_acquire));
    std::array<std::uint8_t, 2U> encodedParameter{0x01U, 0x00U};
    ParameterHandleValueMap parameterValues;
    parameterValues.emplace(
        parameter,
        VariableLengthData(encodedParameter.data(), encodedParameter.size()));
    std::array<std::uint8_t, 3U> encodedTag{0x50U, 0x55U, 0x42U};
    VariableLengthData userSuppliedTag(encodedTag.data(), encodedTag.size());
    rti->sendInteraction(interactionClass, parameterValues, userSuppliedTag);

    TransportServiceMessage receiverResponse;
    if (!receiver.request(
            TransportServiceMessage{
                TransportServiceMessageKind::request,
                TransportServiceOperation::receive_interaction,
                TransportServiceStatus::ok,
                2U,
                umbra::detail::encodeProcessFederationReceiveInteractionRequest(
                    umbra::detail::ProcessFederationReceiveInteractionRequest{
                        L"public-process-message-execution",
                        receiverJoin.federateId})},
            receiverResponse) ||
        receiverResponse.status != TransportServiceStatus::ok) {
      throw std::runtime_error("The public process receiver Receive request failed.");
    }
    auto const eventResult =
        umbra::detail::decodeProcessFederationReceiveInteractionResult(
            receiverResponse.payload);
    REQUIRE(eventResult.event.has_value());
    REQUIRE(eventResult.event->parameterHandles.size() == 1U);
    REQUIRE(eventResult.event->parameterHandles.front() == parameterValue.load());
    auto const envelope =
        umbra::detail::decodeProcessFederationInteractionEnvelope(
            eventResult.event->payload);
    REQUIRE(envelope.has_value());
    REQUIRE(envelope->parameterValues.size() == 1U);
    REQUIRE(envelope->parameterValues.front().first == parameterValue.load());
    REQUIRE(envelope->parameterValues.front().second ==
            std::vector<std::uint8_t>{0x01U, 0x00U});
    REQUIRE(envelope->userSuppliedTag ==
            std::vector<std::uint8_t>{0x50U, 0x55U, 0x42U});
    rti->resignFederationExecution(NO_ACTION);
    rti->disconnect();
    receiverConnection->close();
  } catch (...) {
    clientError = std::current_exception();
    if (clientJoined) {
      try {
        rti->resignFederationExecution(NO_ACTION);
      } catch (...) {
      }
    }
    try {
      rti->disconnect();
    } catch (...) {
    }
    if (receiverConnection) {
      receiverConnection->close();
    }
  }
  if (listener) {
    listener.reset();
  }
  if (server.joinable()) {
    server.join();
  }
  if (clientError) {
    std::rethrow_exception(clientError);
  }
  REQUIRE_FALSE(serverError);
  REQUIRE(joinedHandle.has_value());
  REQUIRE(joinedHandle->isValid());
}

TEST_CASE(
    "RTIambassador resolves interaction and parameter handles through a configured process endpoint",
    "[integration][foundation][interaction-management][transport][process-boundary][public-endpoint]") {
  auto listener = ProcessTransportListener::listen({"127.0.0.1", 0U});
  REQUIRE(listener);
  auto const port = listener->address().port;
  REQUIRE(port != 0U);

  constexpr char const* federationName = "public-process-lookup-execution";
  constexpr char const* objectName = "HLAobjectRoot.Customer";
  constexpr char const* interactionName =
      "HLAinteractionRoot.CustomerTransactions.FoodServed.MainCourseServed";
  constexpr char const* parameterName = "TimelinessOk";
  std::atomic_uint64_t expectedObjectClass{0U};
  std::atomic_uint64_t expectedInteractionClass{0U};
  std::atomic_uint64_t expectedParameter{0U};
  std::exception_ptr serverError;
  std::thread server([&] {
    try {
      EmbeddedFederationRegistry registry;
      ProcessFederationService service(
          registry, composedProcessDefinition(), ProcessFederationServiceOptions{});
      auto connection = listener->accept(
          nullptr,
          {"public-process-lookup-server", 0x9401U},
          [](std::wstring) {},
          [](std::wstring) { return false; });
      ProcessTransportSession session(connection);
      auto handler = service.handlerFor(session);
      auto serveExpected = [&](TransportServiceOperation operation) {
        return ProcessTransportServiceDispatcher::serveOne(
            session,
            [&](TransportServiceMessage const& request) {
              if (request.operation != operation) {
                throw std::runtime_error(
                    "The public process lookup server received an unexpected operation.");
              }
              return handler(request);
            });
      };

      if (!serveExpected(TransportServiceOperation::create_federation_execution)) {
        throw std::runtime_error("The public process lookup server lost Create.");
      }
      auto const interactionClass = registry.interactionClassHandleFor(
          L"public-process-lookup-execution", interactionName);
      auto const objectClass = registry.objectClassHandleFor(
          L"public-process-lookup-execution", objectName);
      auto const parameter = registry.parameterHandleFor(
          L"public-process-lookup-execution", interactionName, parameterName);
      if (!objectClass || !interactionClass || !parameter) {
        throw std::runtime_error(
            "The public process lookup server could not resolve its FOM handles.");
      }
      expectedObjectClass.store(*objectClass, std::memory_order_release);
      expectedInteractionClass.store(*interactionClass, std::memory_order_release);
      expectedParameter.store(*parameter, std::memory_order_release);

      if (!serveExpected(TransportServiceOperation::join_federation_execution) ||
          !serveExpected(TransportServiceOperation::get_object_class_handle) ||
          !serveExpected(TransportServiceOperation::get_interaction_class_handle) ||
          !serveExpected(TransportServiceOperation::get_parameter_handle) ||
          !serveExpected(TransportServiceOperation::resign_federation_execution)) {
        throw std::runtime_error("The public process lookup server lost a request.");
      }
      service.detach(session);
      connection->close();
    } catch (...) {
      serverError = std::current_exception();
    }
  });

  TestFederateAmbassador federate;
  auto rti = makeRti();
  auto configuration = RtiConfiguration::createConfiguration()
                           .withConfigurationName(L"public-process-lookup-client")
                           .withRtiAddress(
                               L"tcp://127.0.0.1:" + std::to_wstring(port));
  std::exception_ptr clientError;
  bool clientJoined = false;
  std::optional<rti1516_2025::FederateHandle> joinedHandle;
  try {
    auto const connectionResult = rti->connect(federate, HLA_EVOKED, configuration);
    REQUIRE(connectionResult.addressUsed);
    rti->createFederationExecution(
        L"public-process-lookup-execution", L"server-owned-fom.xml");
    joinedHandle = rti->joinFederationExecution(
        L"public-process-lookup-federate",
        L"public-process-lookup-type",
        L"public-process-lookup-execution");
    clientJoined = true;

    auto const objectClass = rti->getObjectClassHandle(
        L"HLAobjectRoot.Customer");
    // The public operation returns the exact server-owned registry identity;
    // comparing official handles catches accidental local-registry fallback.
    REQUIRE(objectClass.toString() ==
            L"ObjectClassHandle(" +
                std::to_wstring(
                    expectedObjectClass.load(std::memory_order_acquire)) +
                L")");
    auto const interactionClass = rti->getInteractionClassHandle(
        L"HLAinteractionRoot.CustomerTransactions.FoodServed.MainCourseServed");
    // The public operation returns the exact server-owned registry identity;
    // comparing official handles catches accidental local-registry fallback.
    REQUIRE(interactionClass ==
            rti1516_2025::umbra_binding_detail::makeInteractionClassHandle(
                expectedInteractionClass.load(std::memory_order_acquire)));
    auto const parameter = rti->getParameterHandle(
        interactionClass, L"TimelinessOk");
    REQUIRE(parameter ==
            rti1516_2025::umbra_binding_detail::makeParameterHandle(
                expectedParameter.load(std::memory_order_acquire)));
    rti->resignFederationExecution(NO_ACTION);
    rti->disconnect();
  } catch (...) {
    clientError = std::current_exception();
    if (clientJoined) {
      try {
        rti->resignFederationExecution(NO_ACTION);
      } catch (...) {
      }
    }
    try {
      rti->disconnect();
    } catch (...) {
    }
  }
  if (listener) {
    listener.reset();
  }
  if (server.joinable()) {
    server.join();
  }
  if (clientError) {
    std::rethrow_exception(clientError);
  }
  REQUIRE_FALSE(serverError);
  REQUIRE(joinedHandle.has_value());
  REQUIRE(joinedHandle->isValid());
}

TEST_CASE(
    "RTIambassador routes a directed interaction through a configured process endpoint",
    "[integration][foundation][federation-management][interaction-management][object-management]"
    "[directed-interaction][directed-routing][transport][process-boundary][public-endpoint]"
    "[process-event.receive-object-instance-discovery][process-event.receive-directed-interaction]") {
  constexpr wchar_t const* federationName =
      L"public-process-directed-interaction-execution";
  constexpr wchar_t const* senderName =
      L"public-process-directed-interaction-sender";
  constexpr wchar_t const* receiverName =
      L"public-process-directed-interaction-receiver";
  constexpr char const* objectClassName =
      "HLAobjectRoot.UmbraDirectedFixtureObject";
  constexpr char const* interactionClassName =
      "HLAinteractionRoot.UmbraDirectedFixtureInteraction";
  constexpr char const* attributeName = "DirectedTargetMarker";

  auto listener = ProcessTransportListener::listen({"127.0.0.1", 0U});
  REQUIRE(listener);
  auto const port = listener->address().port;
  REQUIRE(port != 0U);

  std::atomic_uint64_t expectedObjectClass{0U};
  std::atomic_uint64_t expectedInteractionClass{0U};
  std::exception_ptr serverError;
  std::thread server([&] {
    try {
      EmbeddedFederationRegistry registry;
      ProcessFederationService service(
          registry, composedDirectedProcessDefinition(), ProcessFederationServiceOptions{});

      auto senderConnection = listener->accept(
          nullptr,
          {"public-process-directed-server", 0x9701U},
          [](std::wstring) {},
          [](std::wstring) { return false; });
      ProcessTransportSession sender(senderConnection);
      auto senderHandler = service.handlerFor(sender);
      auto serveExpected = [&](ProcessTransportSession& session,
                               auto const& handler,
                               TransportServiceOperation operation) {
        return ProcessTransportServiceDispatcher::serveOne(
            session,
            [&](TransportServiceMessage const& request) {
              if (request.operation != operation) {
                throw std::runtime_error(
                    "The public directed-interaction server received an unexpected operation.");
              }
              return handler(request);
            });
      };

      if (!serveExpected(
              sender,
              senderHandler,
              TransportServiceOperation::create_federation_execution) ||
          !serveExpected(
              sender,
              senderHandler,
              TransportServiceOperation::join_federation_execution)) {
        throw std::runtime_error(
            "The public directed-interaction server lost Create or sender Join.");
      }

      auto const objectClass =
          registry.objectClassHandleFor(federationName, objectClassName);
      auto const interactionClass =
          registry.interactionClassHandleFor(federationName, interactionClassName);
      auto const attribute =
          registry.attributeHandleFor(federationName, objectClassName, attributeName);
      if (!objectClass || !interactionClass || !attribute) {
        throw std::runtime_error(
            "The public directed-interaction server could not resolve its FOM handles.");
      }
      expectedObjectClass.store(*objectClass, std::memory_order_release);
      expectedInteractionClass.store(*interactionClass, std::memory_order_release);

      auto receiverConnection = listener->accept(
          nullptr,
          {"public-process-directed-server", 0x9702U},
          [](std::wstring) {},
          [](std::wstring) { return false; });
      ProcessTransportSession receiver(receiverConnection);
      auto receiverHandler = service.handlerFor(receiver);

      if (!serveExpected(
              receiver,
              receiverHandler,
              TransportServiceOperation::join_federation_execution)) {
        throw std::runtime_error(
            "The public directed-interaction server lost receiver Join.");
      }
      auto const senderMember = registry.memberByName(federationName, senderName);
      auto const receiverMember = registry.memberByName(federationName, receiverName);
      if (!senderMember || !receiverMember) {
        throw std::runtime_error(
            "The public directed-interaction server could not resolve joined federates.");
      }
      auto const published = registry.setObjectClassAttributePublication(
          federationName,
          senderMember->id,
          *objectClass,
          std::set<std::uint64_t>{*attribute},
          true);
      if (published != ObjectClassAttributeDeclarationStatus::applied) {
        throw std::runtime_error(
            "The public directed-interaction target publication was rejected.");
      }
      auto const subscribed = registry.setObjectClassAttributeSubscription(
          federationName,
          receiverMember->id,
          *objectClass,
          std::set<std::uint64_t>{*attribute},
          true);
      if (subscribed != ObjectClassAttributeDeclarationStatus::applied) {
        throw std::runtime_error(
            "The public directed-interaction target subscription was rejected.");
      }
      auto const receiverDirected = registry.subscribeObjectClassDirectedInteractions(
          federationName,
          receiverMember->id,
          *objectClass,
          std::set<std::uint64_t>{*interactionClass},
          true);
      if (receiverDirected !=
          umbra::detail::DirectedInteractionDeclarationStatus::applied) {
        throw std::runtime_error(
            "The public directed-interaction receiver subscription was rejected.");
      }

      if (!serveExpected(
              sender,
              senderHandler,
              TransportServiceOperation::get_object_class_handle) ||
          !serveExpected(
              sender,
              senderHandler,
              TransportServiceOperation::get_interaction_class_handle) ||
          !serveExpected(
              sender,
              senderHandler,
              TransportServiceOperation::publish_object_class_directed_interactions) ||
          !serveExpected(
              sender,
              senderHandler,
              TransportServiceOperation::register_object_instance)) {
        throw std::runtime_error(
            "The public directed-interaction server lost lookup, declaration, or registration.");
      }

      if (!serveExpected(
              receiver,
              receiverHandler,
              TransportServiceOperation::receive_interaction)) {
        throw std::runtime_error(
            "The public directed-interaction server lost target discovery Receive.");
      }

      if (!serveExpected(
              sender,
              senderHandler,
              TransportServiceOperation::send_directed_interaction)) {
        throw std::runtime_error(
            "The public directed-interaction server lost directed Send.");
      }

      if (!serveExpected(
              receiver,
              receiverHandler,
              TransportServiceOperation::receive_interaction) ||
          !serveExpected(
              sender,
              senderHandler,
              TransportServiceOperation::resign_federation_execution)) {
        throw std::runtime_error(
            "The public directed-interaction server lost directed Receive or Resign.");
      }
      service.detach(sender);
      service.detach(receiver);
      senderConnection->close();
      receiverConnection->close();
    } catch (...) {
      serverError = std::current_exception();
    }
  });

  TestFederateAmbassador senderFederate;
  auto senderRti = makeRti();
  auto configuration = RtiConfiguration::createConfiguration()
                           .withConfigurationName(L"public-process-directed-client")
                           .withRtiAddress(
                               L"tcp://127.0.0.1:" + std::to_wstring(port));
  std::exception_ptr clientError;
  std::shared_ptr<ProcessTransportConnection> receiverConnection;
  std::optional<rti1516_2025::FederateHandle> senderJoined;
  std::optional<rti1516_2025::FederateHandle> receiverJoined;
  bool senderIsJoined = false;
  try {
    auto const connectionResult =
        senderRti->connect(senderFederate, HLA_EVOKED, configuration);
    REQUIRE(connectionResult.addressUsed);
    senderRti->createFederationExecution(
        federationName, L"server-owned-directed-fom.xml");
    senderJoined = senderRti->joinFederationExecution(
        senderName, L"public-process-directed-type", federationName);
    senderIsJoined = true;

    receiverConnection = ProcessTransportConnection::connectClient(
        nullptr,
        {"127.0.0.1", port},
        {"public-process-directed-server", 0x9703U},
        [](std::wstring) {},
        [](std::wstring) { return false; });
    ProcessTransportSession receiver(receiverConnection);
    TransportServiceMessage receiverJoinResponse;
    if (!receiver.request(
            TransportServiceMessage{
                TransportServiceMessageKind::request,
                TransportServiceOperation::join_federation_execution,
                TransportServiceStatus::ok,
                1U,
                umbra::detail::encodeProcessFederationJoinRequest(
                    umbra::detail::ProcessFederationJoinRequest{
                        federationName,
                        L"public-process-directed-type",
                        receiverName})},
            receiverJoinResponse) ||
        receiverJoinResponse.status != TransportServiceStatus::ok) {
      throw std::runtime_error(
          "The public directed-interaction receiver Join request failed.");
    }
    auto const receiverJoin = umbra::detail::decodeProcessFederationJoinResult(
        receiverJoinResponse.payload);
    receiverJoined = rti1516_2025::umbra_binding_detail::makeFederateHandle(
        receiverJoin.federateId);

    auto const objectClass = senderRti->getObjectClassHandle(
        L"HLAobjectRoot.UmbraDirectedFixtureObject");
    auto const interactionClass = senderRti->getInteractionClassHandle(
        L"HLAinteractionRoot.UmbraDirectedFixtureInteraction");
    REQUIRE(objectClass.toString() ==
            L"ObjectClassHandle(" +
                std::to_wstring(expectedObjectClass.load(std::memory_order_acquire)) +
                L")");
    REQUIRE(interactionClass.toString() ==
            L"InteractionClassHandle(" +
                std::to_wstring(expectedInteractionClass.load(std::memory_order_acquire)) +
                L")");
    REQUIRE_NOTHROW(senderRti->publishObjectClassDirectedInteractions(
        objectClass, rti1516_2025::InteractionClassHandleSet{interactionClass}));

    auto const objectInstance = senderRti->registerObjectInstance(objectClass);
    REQUIRE(objectInstance.isValid());
    auto const objectInstanceValue =
        *rti1516_2025::umbra_binding_detail::objectInstanceHandleValue(objectInstance);

    TransportServiceMessage discoveryResponse;
    if (!receiver.request(
            TransportServiceMessage{
                TransportServiceMessageKind::request,
                TransportServiceOperation::receive_interaction,
                TransportServiceStatus::ok,
                2U,
                umbra::detail::encodeProcessFederationReceiveInteractionRequest(
                    umbra::detail::ProcessFederationReceiveInteractionRequest{
                        federationName,
                        receiverJoin.federateId})},
            discoveryResponse) ||
        discoveryResponse.status != TransportServiceStatus::ok) {
      throw std::runtime_error(
          "The public directed-interaction discovery Receive request failed.");
    }
    auto const discoveryResult =
        umbra::detail::decodeProcessFederationReceiveInteractionResult(
            discoveryResponse.payload);
    REQUIRE(discoveryResult.discoveryEvent.has_value());
    REQUIRE(discoveryResult.discoveryEvent->objectInstanceHandle == objectInstanceValue);

    std::array<std::uint8_t, 3U> encodedTag{0x44U, 0x49U, 0x52U};
    VariableLengthData userSuppliedTag(encodedTag.data(), encodedTag.size());
    REQUIRE_NOTHROW(senderRti->sendDirectedInteraction(
        interactionClass,
        objectInstance,
        ParameterHandleValueMap{},
        userSuppliedTag));

    TransportServiceMessage receiveResponse;
    if (!receiver.request(
            TransportServiceMessage{
                TransportServiceMessageKind::request,
                TransportServiceOperation::receive_interaction,
                TransportServiceStatus::ok,
                3U,
                umbra::detail::encodeProcessFederationReceiveInteractionRequest(
                    umbra::detail::ProcessFederationReceiveInteractionRequest{
                        federationName,
                        receiverJoin.federateId})},
            receiveResponse) ||
        receiveResponse.status != TransportServiceStatus::ok) {
      throw std::runtime_error(
          "The public directed-interaction Receive request failed.");
    }
    auto const eventResult =
        umbra::detail::decodeProcessFederationReceiveInteractionResult(
            receiveResponse.payload);
    REQUIRE(eventResult.event.has_value());
    REQUIRE(eventResult.event->objectInstanceHandle.has_value());
    REQUIRE(*eventResult.event->objectInstanceHandle == objectInstanceValue);
    REQUIRE(eventResult.event->interactionClassHandle ==
            rti1516_2025::umbra_binding_detail::interactionClassHandleValue(
                interactionClass));
    auto const envelope = umbra::detail::decodeProcessFederationInteractionEnvelope(
        eventResult.event->payload);
    REQUIRE(envelope.has_value());
    REQUIRE(envelope->parameterValues.empty());
    REQUIRE(envelope->userSuppliedTag ==
            std::vector<std::uint8_t>{0x44U, 0x49U, 0x52U});

    senderRti->resignFederationExecution(DELETE_OBJECTS);
    senderIsJoined = false;
    senderRti->disconnect();
    receiverConnection->close();
  } catch (...) {
    clientError = std::current_exception();
    if (senderIsJoined) {
      try {
        senderRti->resignFederationExecution(NO_ACTION);
      } catch (...) {
      }
    }
    try {
      senderRti->disconnect();
    } catch (...) {
    }
    if (receiverConnection) {
      receiverConnection->close();
    }
  }
  if (listener) {
    listener.reset();
  }
  if (server.joinable()) {
    server.join();
  }
  if (clientError) {
    // If the endpoint thread failed before completing the handshake, surface
    // that deterministic server-side diagnostic instead of masking it behind
    // the client's generic transport exception.
    if (serverError) {
      std::rethrow_exception(serverError);
    }
    std::rethrow_exception(clientError);
  }
  REQUIRE_FALSE(serverError);
  REQUIRE(senderJoined.has_value());
  REQUIRE(senderJoined->isValid());
  REQUIRE(receiverJoined.has_value());
  REQUIRE(receiverJoined->isValid());
}

TEST_CASE(
    "RTIambassador routes ordinary interaction declarations through a configured process endpoint",
    "[integration][foundation][federation-management][interaction-management][declaration-management][transport][process-boundary][public-endpoint]") {
  auto listener = ProcessTransportListener::listen({"127.0.0.1", 0U});
  REQUIRE(listener);
  auto const port = listener->address().port;
  REQUIRE(port != 0U);

  constexpr char const* federationName = "public-process-declaration-execution";
  constexpr char const* interactionName =
      "HLAinteractionRoot.CustomerTransactions.FoodServed.MainCourseServed";
  std::atomic_uint64_t expectedInteractionClass{0U};
  std::exception_ptr serverError;
  std::thread server([&] {
    try {
      EmbeddedFederationRegistry registry;
      ProcessFederationService service(
          registry, composedProcessDefinition(), ProcessFederationServiceOptions{});
      auto connection = listener->accept(
          nullptr,
          {"public-process-declaration-server", 0x9501U},
          [](std::wstring) {},
          [](std::wstring) { return false; });
      ProcessTransportSession session(connection);
      auto handler = service.handlerFor(session);
      auto serveExpected = [&](TransportServiceOperation operation) {
        return ProcessTransportServiceDispatcher::serveOne(
            session,
            [&](TransportServiceMessage const& request) {
              if (request.operation != operation) {
                throw std::runtime_error(
                    "The public process declaration server received an unexpected operation.");
              }
              return handler(request);
            });
      };
      if (!serveExpected(TransportServiceOperation::create_federation_execution)) {
        throw std::runtime_error("The public process declaration server lost Create.");
      }
      auto const interactionClass = registry.interactionClassHandleFor(
          L"public-process-declaration-execution", interactionName);
      if (!interactionClass) {
        throw std::runtime_error(
            "The public process declaration server could not resolve its interaction.");
      }
      expectedInteractionClass.store(*interactionClass, std::memory_order_release);

      if (!serveExpected(TransportServiceOperation::join_federation_execution) ||
          !serveExpected(TransportServiceOperation::get_interaction_class_handle)) {
        throw std::runtime_error("The public process declaration server lost Join or lookup.");
      }
      if (!serveExpected(TransportServiceOperation::publish_interaction_class)) {
        throw std::runtime_error("The public process declaration server lost Publish.");
      }
      auto const member = registry.memberByName(
          L"public-process-declaration-execution",
          L"public-process-declaration-federate");
      if (!member) {
        throw std::runtime_error(
            "The public process declaration server could not resolve its federate.");
      }
      auto const federateId = member->id;
      auto checkForMember = [&](bool published, std::optional<bool> subscription) {
        auto const snapshot = registry.interactionClassDeclarationFor(
            L"public-process-declaration-execution",
            federateId,
            expectedInteractionClass.load(std::memory_order_acquire));
        if (!snapshot || snapshot->published != published ||
            snapshot->subscriptionActive != subscription) {
          throw std::runtime_error(
              "The public process declaration state did not match the request.");
        }
      };
      checkForMember(true, std::nullopt);

      if (!serveExpected(TransportServiceOperation::subscribe_interaction_class)) {
        throw std::runtime_error("The public process declaration server lost active Subscribe.");
      }
      checkForMember(true, true);
      if (!serveExpected(TransportServiceOperation::subscribe_interaction_class)) {
        throw std::runtime_error("The public process declaration server lost passive Subscribe.");
      }
      checkForMember(true, false);
      if (!serveExpected(TransportServiceOperation::unsubscribe_interaction_class)) {
        throw std::runtime_error("The public process declaration server lost Unsubscribe.");
      }
      checkForMember(true, std::nullopt);
      if (!serveExpected(TransportServiceOperation::unpublish_interaction_class)) {
        throw std::runtime_error("The public process declaration server lost Unpublish.");
      }
      checkForMember(false, std::nullopt);
      if (!serveExpected(TransportServiceOperation::resign_federation_execution)) {
        throw std::runtime_error("The public process declaration server lost Resign.");
      }
      service.detach(session);
      connection->close();
    } catch (...) {
      serverError = std::current_exception();
    }
  });

  TestFederateAmbassador federate;
  auto rti = makeRti();
  auto configuration = RtiConfiguration::createConfiguration()
                           .withConfigurationName(L"public-process-declaration-client")
                           .withRtiAddress(
                               L"tcp://127.0.0.1:" + std::to_wstring(port));
  std::exception_ptr clientError;
  bool clientJoined = false;
  std::optional<rti1516_2025::FederateHandle> joinedHandle;
  try {
    auto const connectionResult = rti->connect(federate, HLA_EVOKED, configuration);
    REQUIRE(connectionResult.addressUsed);
    rti->createFederationExecution(
        L"public-process-declaration-execution", L"server-owned-fom.xml");
    joinedHandle = rti->joinFederationExecution(
        L"public-process-declaration-federate",
        L"public-process-declaration-type",
        L"public-process-declaration-execution");
    clientJoined = true;
    auto const interactionClass = rti->getInteractionClassHandle(
        L"HLAinteractionRoot.CustomerTransactions.FoodServed.MainCourseServed");
    REQUIRE(interactionClass ==
            rti1516_2025::umbra_binding_detail::makeInteractionClassHandle(
                expectedInteractionClass.load(std::memory_order_acquire)));
    REQUIRE_NOTHROW(rti->publishInteractionClass(interactionClass));
    REQUIRE_NOTHROW(rti->subscribeInteractionClass(interactionClass, true));
    REQUIRE_NOTHROW(rti->subscribeInteractionClass(interactionClass, false));
    REQUIRE_NOTHROW(rti->unsubscribeInteractionClass(interactionClass));
    REQUIRE_NOTHROW(rti->unpublishInteractionClass(interactionClass));
    rti->resignFederationExecution(NO_ACTION);
    rti->disconnect();
  } catch (...) {
    clientError = std::current_exception();
    if (clientJoined) {
      try {
        rti->resignFederationExecution(NO_ACTION);
      } catch (...) {
      }
    }
    try {
      rti->disconnect();
    } catch (...) {
    }
  }
  if (listener) {
    listener.reset();
  }
  if (server.joinable()) {
    server.join();
  }
  if (clientError) {
    std::rethrow_exception(clientError);
  }
  REQUIRE_FALSE(serverError);
  REQUIRE(joinedHandle.has_value());
  REQUIRE(joinedHandle->isValid());
}

TEST_CASE(
    "RTIambassador receives a process interaction through the official Evoke callback surface",
    "[integration][foundation][interaction-management][callbacks][callback-controls][transport][process-boundary][public-endpoint]") {
  class RecordingFederateAmbassador final : public NullFederateAmbassador {
   public:
    void receiveInteraction(
        rti1516_2025::InteractionClassHandle const& interactionClass,
        rti1516_2025::ParameterHandleValueMap const& parameterValues,
        rti1516_2025::VariableLengthData const& userSuppliedTag,
        rti1516_2025::TransportationTypeHandle const& transportationType,
        rti1516_2025::FederateHandle const& producingFederate,
        rti1516_2025::RegionHandleSet const* optionalSentRegions) override {
      received = true;
      ++receivedCount;
      interactionClassHandle = interactionClass;
      transportationTypeHandle = transportationType;
      producingFederateHandle = producingFederate;
      parameterCount = parameterValues.size();
      hasOptionalSentRegions = optionalSentRegions != nullptr;
      tag.clear();
      if (userSuppliedTag.size() != 0U) {
        auto const* first = static_cast<std::uint8_t const*>(userSuppliedTag.data());
        tag.assign(first, first + userSuppliedTag.size());
      }
      tags.push_back(tag);
    }

    bool received = false;
    std::size_t receivedCount = 0U;
    rti1516_2025::InteractionClassHandle interactionClassHandle;
    rti1516_2025::TransportationTypeHandle transportationTypeHandle;
    rti1516_2025::FederateHandle producingFederateHandle;
    std::size_t parameterCount = 0U;
    bool hasOptionalSentRegions = false;
    std::vector<std::uint8_t> tag;
    std::vector<std::vector<std::uint8_t>> tags;
  };

  auto listener = ProcessTransportListener::listen({"127.0.0.1", 0U});
  REQUIRE(listener);
  auto const port = listener->address().port;
  REQUIRE(port != 0U);

  constexpr char const* federationName = "public-process-evoke-execution";
  constexpr char const* interactionName =
      "HLAinteractionRoot.CustomerTransactions.FoodServed.MainCourseServed";
  constexpr wchar_t const* interactionNameWide =
      L"HLAinteractionRoot.CustomerTransactions.FoodServed.MainCourseServed";
  constexpr char const* parameterName = "TimelinessOk";
  constexpr wchar_t const* parameterNameWide = L"TimelinessOk";
  std::atomic_uint64_t expectedInteractionClass{0U};
  std::atomic_uint64_t expectedParameter{0U};
  std::atomic_uint64_t sendRecipientCount{0U};
  std::exception_ptr serverError;
  std::thread server([&] {
    try {
      EmbeddedFederationRegistry registry;
      ProcessFederationService service(
          registry, composedProcessDefinition(), ProcessFederationServiceOptions{});
      auto senderConnection = listener->accept(
          nullptr,
          {"public-process-evoke-server", 0x9601U},
          [](std::wstring) {},
          [](std::wstring) { return false; });
      ProcessTransportSession sender(senderConnection);
      auto senderHandler = service.handlerFor(sender);
      auto serveExpected = [&](ProcessTransportSession& session,
                               auto const& handler,
                               TransportServiceOperation operation,
                               char const* description) {
        if (!ProcessTransportServiceDispatcher::serveOne(
                session,
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
          "The public process evoke server lost Create.");
      auto const interactionClass = registry.interactionClassHandleFor(
          L"public-process-evoke-execution", interactionName);
      auto const parameter = registry.parameterHandleFor(
          L"public-process-evoke-execution", interactionName, parameterName);
      if (!interactionClass || !parameter) {
        throw std::runtime_error(
            "The public process evoke server could not resolve its FOM handles.");
      }
      expectedInteractionClass.store(*interactionClass, std::memory_order_release);
      expectedParameter.store(*parameter, std::memory_order_release);
      serveExpected(
          sender,
          senderHandler,
          TransportServiceOperation::join_federation_execution,
          "The public process evoke server lost sender Join.");

      auto receiverConnection = listener->accept(
          nullptr,
          {"public-process-evoke-server", 0x9602U},
          [](std::wstring) {},
          [](std::wstring) { return false; });
      ProcessTransportSession receiver(receiverConnection);
      auto receiverHandler = service.handlerFor(receiver);
      serveExpected(
          receiver,
          receiverHandler,
          TransportServiceOperation::join_federation_execution,
          "The public process evoke server lost receiver Join.");
      serveExpected(
          sender,
          senderHandler,
          TransportServiceOperation::get_interaction_class_handle,
          "The public process evoke server lost sender lookup.");
      serveExpected(
          receiver,
          receiverHandler,
          TransportServiceOperation::get_interaction_class_handle,
          "The public process evoke server lost receiver lookup.");
      serveExpected(
          sender,
          senderHandler,
          TransportServiceOperation::get_parameter_handle,
          "The public process evoke server lost parameter lookup.");
      serveExpected(
          sender,
          senderHandler,
          TransportServiceOperation::publish_interaction_class,
          "The public process evoke server lost Publish.");
      serveExpected(
          receiver,
          receiverHandler,
          TransportServiceOperation::subscribe_interaction_class,
          "The public process evoke server lost Subscribe.");
      auto serveSend = [&] {
        if (!ProcessTransportServiceDispatcher::serveOne(
                sender,
                [&](TransportServiceMessage const& request) {
                  if (request.operation != TransportServiceOperation::send_interaction) {
                    throw std::runtime_error(
                        "The public process evoke server lost Send.");
                  }
                  auto response = senderHandler(request);
                  if (response.status == TransportServiceStatus::ok) {
                    sendRecipientCount.fetch_add(
                        umbra::detail::decodeProcessFederationSendInteractionResult(
                            response.payload)
                            .recipientCount,
                        std::memory_order_release);
                  }
                  return response;
                })) {
          throw std::runtime_error("The public process evoke server lost Send.");
        }
      };
      // The client deliberately interleaves its two sends with Evoke calls so
      // the first callback can be observed before the disabled backlog is
      // admitted through Evoke Multiple.
      serveSend();
      serveExpected(
          receiver,
          receiverHandler,
          TransportServiceOperation::receive_interaction,
          "The public process evoke server lost Receive.");
      serveSend();
      for (std::size_t receiveIndex = 1U; receiveIndex < 3U; ++receiveIndex) {
        serveExpected(
            receiver,
            receiverHandler,
            TransportServiceOperation::receive_interaction,
            "The public process evoke server lost Receive.");
      }
      serveExpected(
          sender,
          senderHandler,
          TransportServiceOperation::resign_federation_execution,
          "The public process evoke server lost sender Resign.");
      serveExpected(
          receiver,
          receiverHandler,
          TransportServiceOperation::resign_federation_execution,
          "The public process evoke server lost receiver Resign.");
      service.detach(sender);
      service.detach(receiver);
      senderConnection->close();
      receiverConnection->close();
    } catch (...) {
      serverError = std::current_exception();
    }
  });

  RecordingFederateAmbassador senderFederate;
  RecordingFederateAmbassador receiverFederate;
  auto senderRti = makeRti();
  auto receiverRti = makeRti();
  auto configuration = RtiConfiguration::createConfiguration()
                           .withConfigurationName(L"public-process-evoke-client")
                           .withRtiAddress(
                               L"tcp://127.0.0.1:" + std::to_wstring(port));
  std::exception_ptr clientError;
  bool senderJoined = false;
  bool receiverJoined = false;
  try {
    REQUIRE(senderRti->connect(senderFederate, HLA_EVOKED, configuration).addressUsed);
    senderRti->createFederationExecution(
        L"public-process-evoke-execution", L"server-owned-fom.xml");
    auto const senderHandle = senderRti->joinFederationExecution(
        L"public-process-evoke-federate",
        L"public-process-evoke-type",
        L"public-process-evoke-execution");
    senderJoined = true;

    REQUIRE(receiverRti->connect(receiverFederate, HLA_EVOKED, configuration).addressUsed);
    auto const receiverHandle = receiverRti->joinFederationExecution(
        L"public-process-evoke-receiver",
        L"public-process-evoke-type",
        L"public-process-evoke-execution");
    receiverJoined = true;

    auto const senderInteraction =
        senderRti->getInteractionClassHandle(interactionNameWide);
    auto const receiverInteraction =
        receiverRti->getInteractionClassHandle(interactionNameWide);
    auto const expectedClass =
        rti1516_2025::umbra_binding_detail::makeInteractionClassHandle(
            expectedInteractionClass.load(std::memory_order_acquire));
    REQUIRE(senderInteraction == expectedClass);
    REQUIRE(receiverInteraction == expectedClass);
    auto const parameter = senderRti->getParameterHandle(
        senderInteraction, parameterNameWide);
    REQUIRE(parameter ==
            rti1516_2025::umbra_binding_detail::makeParameterHandle(
                expectedParameter.load(std::memory_order_acquire)));
    REQUIRE_NOTHROW(senderRti->publishInteractionClass(senderInteraction));
    REQUIRE_NOTHROW(receiverRti->subscribeInteractionClass(receiverInteraction, true));

    std::array<std::uint8_t, 2U> encodedParameter{0x01U, 0x00U};
    ParameterHandleValueMap parameterValues;
    parameterValues.emplace(
        parameter,
        VariableLengthData(encodedParameter.data(), encodedParameter.size()));
    std::array<std::uint8_t, 3U> firstEncodedTag{0x45U, 0x56U, 0x4BU};
    VariableLengthData firstUserSuppliedTag(
        firstEncodedTag.data(), firstEncodedTag.size());
    REQUIRE_NOTHROW(senderRti->sendInteraction(
        senderInteraction, parameterValues, firstUserSuppliedTag));

    // The callback is delivered synchronously by Evoke; the boolean reports
    // whether another callback remains pending after this one is drained.
    REQUIRE_FALSE(receiverRti->evokeCallback(0.0));
    REQUIRE(receiverFederate.received);
    REQUIRE(receiverFederate.interactionClassHandle == expectedClass);
    REQUIRE(receiverFederate.parameterCount == 1U);
    REQUIRE(receiverFederate.receivedCount == 1U);
    REQUIRE(receiverFederate.tag ==
            std::vector<std::uint8_t>{0x45U, 0x56U, 0x4BU});
    REQUIRE_FALSE(receiverFederate.hasOptionalSentRegions);
    REQUIRE(receiverFederate.producingFederateHandle == senderHandle);

    REQUIRE_NOTHROW(receiverRti->disableCallbacks());
    std::array<std::uint8_t, 3U> secondEncodedTag{0x4DU, 0x55U, 0x4CU};
    VariableLengthData secondUserSuppliedTag(
        secondEncodedTag.data(), secondEncodedTag.size());
    REQUIRE_NOTHROW(senderRti->sendInteraction(
        senderInteraction, parameterValues, secondUserSuppliedTag));

    // Disabled EVOKED callbacks remain pending.  Evoke Multiple admits the
    // process-boundary event but must not invoke the FederateAmbassador until
    // Enable Callbacks is called.
    REQUIRE(receiverRti->evokeMultipleCallbacks(0.0, 1.0));
    REQUIRE(receiverFederate.receivedCount == 1U);
    REQUIRE(receiverFederate.tags.size() == 1U);
    REQUIRE_NOTHROW(receiverRti->enableCallbacks());
    REQUIRE_FALSE(receiverRti->evokeMultipleCallbacks(0.0, 1.0));
    REQUIRE(receiverFederate.receivedCount == 2U);
    REQUIRE(receiverFederate.tags.size() == 2U);
    REQUIRE(receiverFederate.tags[1] ==
            std::vector<std::uint8_t>{0x4DU, 0x55U, 0x4CU});

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
  if (listener) {
    listener.reset();
  }
  if (server.joinable()) {
    server.join();
  }
  if (clientError) {
    std::rethrow_exception(clientError);
  }
  REQUIRE_FALSE(serverError);
  REQUIRE(sendRecipientCount.load(std::memory_order_acquire) == 2U);
  REQUIRE_FALSE(senderJoined);
  REQUIRE_FALSE(receiverJoined);
}

TEST_CASE(
    "RTIambassador preserves a timestamped process interaction through the official Evoke callback surface",
    "[integration][foundation][interaction-management][time-management][callbacks][transport][process-boundary][public-endpoint]") {
  class RecordingFederateAmbassador final : public NullFederateAmbassador {
   public:
    void receiveInteraction(
        rti1516_2025::InteractionClassHandle const& interactionClass,
        rti1516_2025::ParameterHandleValueMap const& parameterValues,
        rti1516_2025::VariableLengthData const& userSuppliedTag,
        rti1516_2025::TransportationTypeHandle const& transportationType,
        rti1516_2025::FederateHandle const& producingFederate,
        rti1516_2025::RegionHandleSet const* optionalSentRegions,
        rti1516_2025::LogicalTime const& time,
        rti1516_2025::OrderType sentOrder,
        rti1516_2025::OrderType receivedOrder,
        rti1516_2025::MessageRetractionHandle const* optionalRetraction) override {
      received = true;
      interactionClassHandle = interactionClass;
      transportationTypeHandle = transportationType;
      producingFederateHandle = producingFederate;
      parameterCount = parameterValues.size();
      hasOptionalSentRegions = optionalSentRegions != nullptr;
      hasOptionalRetraction = optionalRetraction != nullptr;
      sentOrderType = sentOrder;
      receivedOrderType = receivedOrder;
      timestampImplementationName = time.implementationName();
      auto const encoded = time.encode();
      timestampEncoding.clear();
      if (encoded.size() != 0U) {
        auto const* first = static_cast<std::uint8_t const*>(encoded.data());
        timestampEncoding.assign(first, first + encoded.size());
      }
      tag.clear();
      if (userSuppliedTag.size() != 0U) {
        auto const* first = static_cast<std::uint8_t const*>(userSuppliedTag.data());
        tag.assign(first, first + userSuppliedTag.size());
      }
    }

    bool received = false;
    rti1516_2025::InteractionClassHandle interactionClassHandle;
    rti1516_2025::TransportationTypeHandle transportationTypeHandle;
    rti1516_2025::FederateHandle producingFederateHandle;
    std::size_t parameterCount = 0U;
    bool hasOptionalSentRegions = false;
    bool hasOptionalRetraction = false;
    rti1516_2025::OrderType sentOrderType = rti1516_2025::RECEIVE;
    rti1516_2025::OrderType receivedOrderType = rti1516_2025::RECEIVE;
    std::wstring timestampImplementationName;
    std::vector<std::uint8_t> timestampEncoding;
    std::vector<std::uint8_t> tag;
  };

  auto listener = ProcessTransportListener::listen({"127.0.0.1", 0U});
  REQUIRE(listener);
  auto const port = listener->address().port;
  REQUIRE(port != 0U);

  constexpr char const* federationName = "public-process-timestamped-execution";
  constexpr char const* interactionName =
      "HLAinteractionRoot.CustomerTransactions.FoodServed.MainCourseServed";
  constexpr wchar_t const* interactionNameWide =
      L"HLAinteractionRoot.CustomerTransactions.FoodServed.MainCourseServed";
  constexpr char const* parameterName = "TimelinessOk";
  constexpr wchar_t const* parameterNameWide = L"TimelinessOk";
  std::atomic_uint64_t expectedInteractionClass{0U};
  std::atomic_uint64_t expectedParameter{0U};
  std::atomic_uint64_t sendRecipientCount{0U};
  std::exception_ptr serverError;
  std::thread server([&] {
    try {
      EmbeddedFederationRegistry registry;
      ProcessFederationService service(
          registry, composedProcessDefinition(), ProcessFederationServiceOptions{});
      auto senderConnection = listener->accept(
          nullptr,
          {"public-process-timestamped-server", 0x9701U},
          [](std::wstring) {},
          [](std::wstring) { return false; });
      ProcessTransportSession sender(senderConnection);
      auto senderHandler = service.handlerFor(sender);
      auto serveExpected = [&](ProcessTransportSession& session,
                               auto const& handler,
                               TransportServiceOperation operation,
                               char const* description) {
        if (!ProcessTransportServiceDispatcher::serveOne(
                session,
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
          "The public timestamped process server lost Create.");
      auto const interactionClass = registry.interactionClassHandleFor(
          L"public-process-timestamped-execution", interactionName);
      auto const parameter = registry.parameterHandleFor(
          L"public-process-timestamped-execution", interactionName, parameterName);
      if (!interactionClass || !parameter) {
        throw std::runtime_error(
            "The public timestamped process server could not resolve its FOM handles.");
      }
      expectedInteractionClass.store(*interactionClass, std::memory_order_release);
      expectedParameter.store(*parameter, std::memory_order_release);
      serveExpected(
          sender,
          senderHandler,
          TransportServiceOperation::join_federation_execution,
          "The public timestamped process server lost sender Join.");

      auto receiverConnection = listener->accept(
          nullptr,
          {"public-process-timestamped-server", 0x9702U},
          [](std::wstring) {},
          [](std::wstring) { return false; });
      ProcessTransportSession receiver(receiverConnection);
      auto receiverHandler = service.handlerFor(receiver);
      serveExpected(
          receiver,
          receiverHandler,
          TransportServiceOperation::join_federation_execution,
          "The public timestamped process server lost receiver Join.");
      serveExpected(
          sender,
          senderHandler,
          TransportServiceOperation::get_interaction_class_handle,
          "The public timestamped process server lost sender lookup.");
      serveExpected(
          receiver,
          receiverHandler,
          TransportServiceOperation::get_interaction_class_handle,
          "The public timestamped process server lost receiver lookup.");
      serveExpected(
          sender,
          senderHandler,
          TransportServiceOperation::get_parameter_handle,
          "The public timestamped process server lost parameter lookup.");
      serveExpected(
          sender,
          senderHandler,
          TransportServiceOperation::publish_interaction_class,
          "The public timestamped process server lost Publish.");
      serveExpected(
          receiver,
          receiverHandler,
          TransportServiceOperation::subscribe_interaction_class,
          "The public timestamped process server lost Subscribe.");
      if (!ProcessTransportServiceDispatcher::serveOne(
              sender,
              [&](TransportServiceMessage const& request) {
                if (request.operation != TransportServiceOperation::send_interaction) {
                  throw std::runtime_error(
                      "The public timestamped process server lost Send.");
                }
                auto response = senderHandler(request);
                if (response.status == TransportServiceStatus::ok) {
                  sendRecipientCount.store(
                      umbra::detail::decodeProcessFederationSendInteractionResult(
                          response.payload)
                          .recipientCount,
                      std::memory_order_release);
                }
                return response;
              })) {
        throw std::runtime_error("The public timestamped process server lost Send.");
      }
      serveExpected(
          receiver,
          receiverHandler,
          TransportServiceOperation::receive_interaction,
          "The public timestamped process server lost Receive.");
      serveExpected(
          sender,
          senderHandler,
          TransportServiceOperation::resign_federation_execution,
          "The public timestamped process server lost sender Resign.");
      serveExpected(
          receiver,
          receiverHandler,
          TransportServiceOperation::resign_federation_execution,
          "The public timestamped process server lost receiver Resign.");
      service.detach(sender);
      service.detach(receiver);
      senderConnection->close();
      receiverConnection->close();
    } catch (...) {
      serverError = std::current_exception();
    }
  });

  RecordingFederateAmbassador senderFederate;
  RecordingFederateAmbassador receiverFederate;
  auto senderRti = makeRti();
  auto receiverRti = makeRti();
  auto configuration = RtiConfiguration::createConfiguration()
                           .withConfigurationName(L"public-process-timestamped-client")
                           .withRtiAddress(
                               L"tcp://127.0.0.1:" + std::to_wstring(port));
  std::exception_ptr clientError;
  bool senderJoined = false;
  bool receiverJoined = false;
  try {
    REQUIRE(senderRti->connect(senderFederate, HLA_EVOKED, configuration).addressUsed);
    senderRti->createFederationExecution(
        L"public-process-timestamped-execution", L"server-owned-fom.xml");
    auto const senderHandle = senderRti->joinFederationExecution(
        L"public-process-timestamped-federate",
        L"public-process-timestamped-type",
        L"public-process-timestamped-execution");
    senderJoined = true;

    REQUIRE(receiverRti->connect(receiverFederate, HLA_EVOKED, configuration).addressUsed);
    auto const receiverHandle = receiverRti->joinFederationExecution(
        L"public-process-timestamped-receiver",
        L"public-process-timestamped-type",
        L"public-process-timestamped-execution");
    receiverJoined = true;

    auto const senderInteraction =
        senderRti->getInteractionClassHandle(interactionNameWide);
    auto const receiverInteraction =
        receiverRti->getInteractionClassHandle(interactionNameWide);
    auto const expectedClass =
        rti1516_2025::umbra_binding_detail::makeInteractionClassHandle(
            expectedInteractionClass.load(std::memory_order_acquire));
    REQUIRE(senderInteraction == expectedClass);
    REQUIRE(receiverInteraction == expectedClass);
    auto const parameter = senderRti->getParameterHandle(
        senderInteraction, parameterNameWide);
    REQUIRE(parameter ==
            rti1516_2025::umbra_binding_detail::makeParameterHandle(
                expectedParameter.load(std::memory_order_acquire)));
    REQUIRE_NOTHROW(senderRti->publishInteractionClass(senderInteraction));
    REQUIRE_NOTHROW(receiverRti->subscribeInteractionClass(receiverInteraction, true));

    auto factory = rti1516_2025::HLAlogicalTimeFactoryFactory::makeLogicalTimeFactory(
        L"HLAinteger64Time");
    auto* integerFactory =
        dynamic_cast<rti1516_2025::HLAinteger64TimeFactory*>(factory.get());
    REQUIRE(integerFactory != nullptr);
    auto timestamp = integerFactory->makeLogicalTime(5);
    REQUIRE(timestamp);
    auto const expectedTimestampEncoding = timestamp->encode();

    std::array<std::uint8_t, 2U> encodedParameter{0x01U, 0x00U};
    ParameterHandleValueMap parameterValues;
    parameterValues.emplace(
        parameter,
        VariableLengthData(encodedParameter.data(), encodedParameter.size()));
    std::array<std::uint8_t, 3U> encodedTag{0x54U, 0x53U, 0x4FU};
    VariableLengthData userSuppliedTag(encodedTag.data(), encodedTag.size());
    auto const retraction = senderRti->sendInteraction(
        senderInteraction, parameterValues, userSuppliedTag, *timestamp);
    REQUIRE_FALSE(retraction.isValid());

    REQUIRE_FALSE(receiverRti->evokeCallback(0.0));
    REQUIRE(receiverFederate.received);
    REQUIRE(receiverFederate.interactionClassHandle == expectedClass);
    REQUIRE(receiverFederate.parameterCount == 1U);
    REQUIRE(receiverFederate.tag ==
            std::vector<std::uint8_t>{0x54U, 0x53U, 0x4FU});
    REQUIRE_FALSE(receiverFederate.hasOptionalSentRegions);
    REQUIRE_FALSE(receiverFederate.hasOptionalRetraction);
    REQUIRE(receiverFederate.producingFederateHandle == senderHandle);
    REQUIRE(receiverFederate.timestampImplementationName == L"HLAinteger64Time");
    std::vector<std::uint8_t> expectedTimestampBytes;
    if (expectedTimestampEncoding.size() != 0U) {
      auto const* first = static_cast<std::uint8_t const*>(expectedTimestampEncoding.data());
      expectedTimestampBytes.assign(
          first, first + expectedTimestampEncoding.size());
    }
    REQUIRE(receiverFederate.timestampEncoding == expectedTimestampBytes);
    REQUIRE(receiverFederate.sentOrderType == rti1516_2025::RECEIVE);
    REQUIRE(receiverFederate.receivedOrderType == rti1516_2025::RECEIVE);

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
  if (listener) {
    listener.reset();
  }
  if (server.joinable()) {
    server.join();
  }
  if (clientError) {
    std::rethrow_exception(clientError);
  }
  REQUIRE_FALSE(serverError);
  REQUIRE(sendRecipientCount.load(std::memory_order_acquire) == 1U);
  REQUIRE_FALSE(senderJoined);
  REQUIRE_FALSE(receiverJoined);
}

TEST_CASE(
    "RTIambassador routes public Update Attribute Values through a configured process endpoint",
    "[integration][foundation][object-management][transport][process-boundary][public-endpoint][rti.service.update-attribute-values]") {
  auto listener = ProcessTransportListener::listen({"127.0.0.1", 0U});
  REQUIRE(listener);
  auto const port = listener->address().port;
  REQUIRE(port != 0U);

  constexpr wchar_t const* federationNameWide =
      L"public-process-attribute-update-execution";
  constexpr char const* objectClassName = "HLAobjectRoot.Employee";
  constexpr wchar_t const* objectClassNameWide = L"HLAobjectRoot.Employee";
  constexpr char const* attributeName = "Name";
  constexpr wchar_t const* attributeNameWide = L"Name";
  std::atomic_uint64_t expectedObjectClass{0U};
  std::atomic_uint64_t expectedAttribute{0U};
  std::atomic_uint64_t recipientCount{std::numeric_limits<std::uint32_t>::max()};
  std::exception_ptr serverError;
  std::thread server([&] {
    try {
      EmbeddedFederationRegistry registry;
      ProcessFederationService service(
          registry, composedProcessDefinition(), ProcessFederationServiceOptions{});
      auto connection = listener->accept(
          nullptr,
          {"public-process-attribute-update-server", 0x9801U},
          [](std::wstring) {},
          [](std::wstring) { return false; });
      ProcessTransportSession session(connection);
      auto handler = service.handlerFor(session);
      auto serveExpected = [&](TransportServiceOperation operation,
                               char const* description) {
        if (!ProcessTransportServiceDispatcher::serveOne(
                session,
                [&](TransportServiceMessage const& request) {
                  if (request.operation != operation) {
                    throw std::runtime_error(description);
                  }
                  auto response = handler(request);
                  if (operation == TransportServiceOperation::update_attribute_values &&
                      response.status == TransportServiceStatus::ok) {
                    recipientCount.store(
                        umbra::detail::decodeProcessFederationUpdateAttributeValuesResult(
                            response.payload)
                            .recipientCount,
                        std::memory_order_release);
                  }
                  return response;
                })) {
          throw std::runtime_error(description);
        }
      };

      serveExpected(
          TransportServiceOperation::create_federation_execution,
          "The public process attribute-update server lost Create.");
      auto const objectClass = registry.objectClassHandleFor(
          federationNameWide,
          objectClassName);
      auto const attribute = registry.attributeHandleFor(
          federationNameWide,
          objectClassName,
          attributeName);
      if (!objectClass || !attribute) {
        throw std::runtime_error(
            "The public process attribute-update server could not resolve its FOM handles.");
      }
      expectedObjectClass.store(*objectClass, std::memory_order_release);
      expectedAttribute.store(*attribute, std::memory_order_release);
      serveExpected(
          TransportServiceOperation::join_federation_execution,
          "The public process attribute-update server lost Join.");
      serveExpected(
          TransportServiceOperation::get_object_class_handle,
          "The public process attribute-update server lost object-class lookup.");
      serveExpected(
          TransportServiceOperation::get_attribute_handle,
          "The public process attribute-update server lost attribute lookup.");
      serveExpected(
          TransportServiceOperation::publish_object_class_attributes,
          "The public process attribute-update server lost Publish.");
      serveExpected(
          TransportServiceOperation::register_object_instance,
          "The public process attribute-update server lost Register.");
      serveExpected(
          TransportServiceOperation::update_attribute_values,
          "The public process attribute-update server lost Update.");
      serveExpected(
          TransportServiceOperation::resign_federation_execution,
          "The public process attribute-update server lost Resign.");
      service.detach(session);
      connection->close();
    } catch (...) {
      serverError = std::current_exception();
    }
  });

  TestFederateAmbassador federate;
  auto rti = makeRti();
  auto configuration = RtiConfiguration::createConfiguration()
                           .withConfigurationName(L"public-process-attribute-update-client")
                           .withRtiAddress(
                               L"tcp://127.0.0.1:" + std::to_wstring(port));
  std::exception_ptr clientError;
  bool joined = false;
  try {
    REQUIRE(rti->connect(federate, HLA_EVOKED, configuration).addressUsed);
    rti->createFederationExecution(
        federationNameWide,
        L"server-owned-fom.xml");
    rti->joinFederationExecution(
        L"public-process-attribute-update-federate",
        L"public-process-attribute-update-type",
        federationNameWide);
    joined = true;

    auto const objectClass = rti->getObjectClassHandle(objectClassNameWide);
    auto const attribute = rti->getAttributeHandle(objectClass, attributeNameWide);
    REQUIRE(objectClass.toString() ==
            L"ObjectClassHandle(" +
                std::to_wstring(expectedObjectClass.load(std::memory_order_acquire)) +
                L")");
    REQUIRE(attribute == rti1516_2025::umbra_binding_detail::makeAttributeHandle(
                             expectedAttribute.load(std::memory_order_acquire)));
    rti1516_2025::AttributeHandleSet attributes;
    attributes.insert(attribute);
    rti->publishObjectClassAttributes(objectClass, attributes);
    auto const objectInstance = rti->registerObjectInstance(objectClass);
    REQUIRE(objectInstance.isValid());

    std::array<std::uint8_t, 3U> encodedValue{0x45U, 0x6DU, 0x70U};
    std::array<std::uint8_t, 3U> encodedTag{0x55U, 0x50U, 0x44U};
    AttributeHandleValueMap attributeValues;
    attributeValues.emplace(
        attribute,
        VariableLengthData(encodedValue.data(), encodedValue.size()));
    VariableLengthData userSuppliedTag(encodedTag.data(), encodedTag.size());
    REQUIRE_NOTHROW(
        rti->updateAttributeValues(objectInstance, attributeValues, userSuppliedTag));

    rti->resignFederationExecution(NO_ACTION);
    joined = false;
    rti->disconnect();
  } catch (...) {
    clientError = std::current_exception();
    if (joined) {
      try {
        rti->resignFederationExecution(NO_ACTION);
      } catch (...) {
      }
    }
    try {
      rti->disconnect();
    } catch (...) {
    }
  }
  if (listener) {
    listener.reset();
  }
  if (server.joinable()) {
    server.join();
  }
  if (clientError) {
    std::rethrow_exception(clientError);
  }
  REQUIRE_FALSE(serverError);
  REQUIRE(recipientCount.load(std::memory_order_acquire) == 0U);
  REQUIRE_FALSE(joined);
}

TEST_CASE(
    "RTIambassador delivers a process Update Attribute Values event through the official Reflect callback",
    "[integration][foundation][object-management][callbacks][callback-controls][transport][process-boundary][public-endpoint][rti.service.update-attribute-values][federate.callback.reflect-attribute-values]") {
  class RecordingFederateAmbassador final : public NullFederateAmbassador {
   public:
    void discoverObjectInstance(
        rti1516_2025::ObjectInstanceHandle const& objectInstance,
        rti1516_2025::ObjectClassHandle const& objectClass,
        std::wstring const& objectInstanceName,
        rti1516_2025::FederateHandle const& producingFederate) override {
      discovered = true;
      discoveredObjectInstance = objectInstance;
      discoveredObjectClass = objectClass;
      discoveredObjectInstanceName = objectInstanceName;
      discoveredProducingFederate = producingFederate;
    }

    void reflectAttributeValues(
        rti1516_2025::ObjectInstanceHandle const& objectInstance,
        rti1516_2025::AttributeHandleValueMap const& attributeValues,
        rti1516_2025::VariableLengthData const& userSuppliedTag,
        rti1516_2025::TransportationTypeHandle const& transportationType,
        rti1516_2025::FederateHandle const& producingFederate,
        rti1516_2025::RegionHandleSet const* optionalSentRegions) override {
      received = true;
      this->objectInstance = objectInstance;
      this->transportationType = transportationType;
      this->producingFederate = producingFederate;
      this->attributeCount = attributeValues.size();
      this->hasOptionalSentRegions = optionalSentRegions != nullptr;
      this->tag.clear();
      if (userSuppliedTag.size() != 0U) {
        auto const* first = static_cast<std::uint8_t const*>(userSuppliedTag.data());
        this->tag.assign(first, first + userSuppliedTag.size());
      }
      if (!attributeValues.empty()) {
        auto const& value = attributeValues.begin()->second;
        this->value.clear();
        if (value.size() != 0U) {
          auto const* first = static_cast<std::uint8_t const*>(value.data());
          this->value.assign(first, first + value.size());
        }
      }
    }

    bool discovered = false;
    rti1516_2025::ObjectInstanceHandle discoveredObjectInstance;
    rti1516_2025::ObjectClassHandle discoveredObjectClass;
    std::wstring discoveredObjectInstanceName;
    rti1516_2025::FederateHandle discoveredProducingFederate;
    bool received = false;
    rti1516_2025::ObjectInstanceHandle objectInstance;
    rti1516_2025::TransportationTypeHandle transportationType;
    rti1516_2025::FederateHandle producingFederate;
    std::size_t attributeCount = 0U;
    bool hasOptionalSentRegions = false;
    std::vector<std::uint8_t> value;
    std::vector<std::uint8_t> tag;
  } receiverFederate;

  auto listener = ProcessTransportListener::listen({"127.0.0.1", 0U});
  REQUIRE(listener);
  auto const port = listener->address().port;
  REQUIRE(port != 0U);

  constexpr wchar_t const* federationNameWide =
      L"public-process-attribute-reflect-execution";
  constexpr char const* objectClassName = "HLAobjectRoot.Employee";
  constexpr wchar_t const* objectClassNameWide = L"HLAobjectRoot.Employee";
  constexpr char const* attributeName = "Name";
  constexpr wchar_t const* attributeNameWide = L"Name";
  std::atomic_uint64_t expectedObjectClass{0U};
  std::atomic_uint64_t expectedAttribute{0U};
  std::atomic_uint64_t expectedObjectInstance{0U};
  std::atomic_uint64_t expectedSenderFederate{0U};
  std::atomic_uint64_t expectedReceiverFederate{0U};
  std::atomic_uint64_t recipientCount{std::numeric_limits<std::uint32_t>::max()};
  std::exception_ptr serverError;
  std::thread server([&] {
    try {
      EmbeddedFederationRegistry registry;
      ProcessFederationService service(
          registry, composedProcessDefinition(), ProcessFederationServiceOptions{});
      auto senderConnection = listener->accept(
          nullptr,
          {"public-process-attribute-reflect-server", 0x9901U},
          [](std::wstring) {},
          [](std::wstring) { return false; });
      ProcessTransportSession sender(senderConnection);
      auto baseSenderHandler = service.handlerFor(sender);
      auto senderHandler = [&](TransportServiceMessage const& request) {
        auto response = baseSenderHandler(request);
        if (request.operation == TransportServiceOperation::join_federation_execution &&
            response.status == TransportServiceStatus::ok) {
          auto const join = umbra::detail::decodeProcessFederationJoinResult(
              response.payload);
          expectedSenderFederate.store(join.federateId, std::memory_order_release);
        }
        if (request.operation == TransportServiceOperation::register_object_instance &&
            response.status == TransportServiceStatus::ok) {
          auto const registration =
              umbra::detail::decodeProcessFederationRegisterObjectInstanceResult(
                  response.payload);
          expectedObjectInstance.store(
              registration.objectInstanceHandle, std::memory_order_release);
        }
        if (request.operation == TransportServiceOperation::update_attribute_values &&
            response.status == TransportServiceStatus::ok) {
          recipientCount.store(
              umbra::detail::decodeProcessFederationUpdateAttributeValuesResult(
                  response.payload)
                  .recipientCount,
              std::memory_order_release);
        }
        return response;
      };
      auto serveExpected = [&](ProcessTransportSession& session,
                               auto const& handler,
                               TransportServiceOperation operation,
                               char const* description) {
        if (!ProcessTransportServiceDispatcher::serveOne(
                session,
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
          "The public process reflection server lost Create.");
      auto const objectClass = registry.objectClassHandleFor(
          federationNameWide, objectClassName);
      auto const attribute = registry.attributeHandleFor(
          federationNameWide, objectClassName, attributeName);
      if (!objectClass || !attribute) {
        throw std::runtime_error(
            "The public process reflection server could not resolve its FOM handles.");
      }
      expectedObjectClass.store(*objectClass, std::memory_order_release);
      expectedAttribute.store(*attribute, std::memory_order_release);
      serveExpected(
          sender,
          senderHandler,
          TransportServiceOperation::join_federation_execution,
          "The public process reflection server lost sender Join.");
      auto receiverConnection = listener->accept(
          nullptr,
          {"public-process-attribute-reflect-server", 0x9902U},
          [](std::wstring) {},
          [](std::wstring) { return false; });
      ProcessTransportSession receiver(receiverConnection);
      auto receiverHandler = service.handlerFor(receiver);
      auto receiverJoinHandler = [&](TransportServiceMessage const& request) {
        auto response = receiverHandler(request);
        if (request.operation == TransportServiceOperation::join_federation_execution &&
            response.status == TransportServiceStatus::ok) {
          auto const join = umbra::detail::decodeProcessFederationJoinResult(
              response.payload);
          expectedReceiverFederate.store(join.federateId, std::memory_order_release);
        }
        return response;
      };
      serveExpected(
          receiver,
          receiverJoinHandler,
          TransportServiceOperation::join_federation_execution,
          "The public process reflection server lost receiver Join.");
      auto const senderMember = registry.memberByName(
          federationNameWide, L"public-process-attribute-reflect-federate");
      auto const receiverMember = registry.memberByName(
          federationNameWide, L"public-process-attribute-reflect-receiver");
      if (!senderMember || !receiverMember ||
          registry.setObjectClassAttributePublication(
              federationNameWide,
              senderMember->id,
              *objectClass,
              std::set<std::uint64_t>{*attribute},
              true) !=
              umbra::detail::ObjectClassAttributeDeclarationStatus::applied ||
          registry.setObjectClassAttributeSubscription(
              federationNameWide,
              receiverMember->id,
              *objectClass,
              std::set<std::uint64_t>{*attribute},
              true) !=
              umbra::detail::ObjectClassAttributeDeclarationStatus::applied) {
        throw std::runtime_error(
            "The public process reflection declarations were rejected.");
      }
      serveExpected(
          sender,
          senderHandler,
          TransportServiceOperation::get_object_class_handle,
          "The public process reflection server lost object-class lookup.");
      serveExpected(
          sender,
          senderHandler,
          TransportServiceOperation::get_attribute_handle,
          "The public process reflection server lost attribute lookup.");
      serveExpected(
          sender,
          senderHandler,
          TransportServiceOperation::publish_object_class_attributes,
          "The public process reflection server lost Publish.");
      serveExpected(
          sender,
          senderHandler,
          TransportServiceOperation::register_object_instance,
          "The public process reflection server lost Register.");
      serveExpected(
          sender,
          senderHandler,
          TransportServiceOperation::update_attribute_values,
          "The public process reflection server lost Update.");
      serveExpected(
          receiver,
          receiverHandler,
          TransportServiceOperation::receive_interaction,
          "The public process reflection server lost Receive.");
      serveExpected(
          receiver,
          receiverHandler,
          TransportServiceOperation::receive_interaction,
          "The public process reflection server lost its reflection Receive.");
      serveExpected(
          sender,
          senderHandler,
          TransportServiceOperation::resign_federation_execution,
          "The public process reflection server lost sender Resign.");
      serveExpected(
          receiver,
          receiverHandler,
          TransportServiceOperation::resign_federation_execution,
          "The public process reflection server lost receiver Resign.");
      service.detach(sender);
      service.detach(receiver);
      senderConnection->close();
      receiverConnection->close();
    } catch (...) {
      serverError = std::current_exception();
    }
  });

  TestFederateAmbassador senderFederate;
  auto senderRti = makeRti();
  auto receiverRti = makeRti();
  auto configuration = RtiConfiguration::createConfiguration()
                           .withConfigurationName(L"public-process-attribute-reflect-client")
                           .withRtiAddress(
                               L"tcp://127.0.0.1:" + std::to_wstring(port));
  std::exception_ptr clientError;
  bool senderJoined = false;
  bool receiverJoined = false;
  try {
    REQUIRE(senderRti->connect(senderFederate, HLA_EVOKED, configuration).addressUsed);
    senderRti->createFederationExecution(federationNameWide, L"server-owned-fom.xml");
    senderRti->joinFederationExecution(
        L"public-process-attribute-reflect-federate",
        L"public-process-attribute-reflect-type",
        federationNameWide);
    senderJoined = true;

    REQUIRE(receiverRti->connect(receiverFederate, HLA_EVOKED, configuration).addressUsed);
    receiverRti->joinFederationExecution(
        L"public-process-attribute-reflect-receiver",
        L"public-process-attribute-reflect-type",
        federationNameWide);
    receiverJoined = true;

    auto const objectClass = senderRti->getObjectClassHandle(objectClassNameWide);
    auto const attribute = senderRti->getAttributeHandle(objectClass, attributeNameWide);
    REQUIRE(objectClass.toString() ==
            L"ObjectClassHandle(" +
                std::to_wstring(expectedObjectClass.load(std::memory_order_acquire)) +
                L")");
    REQUIRE(attribute == rti1516_2025::umbra_binding_detail::makeAttributeHandle(
                             expectedAttribute.load(std::memory_order_acquire)));
    rti1516_2025::AttributeHandleSet attributes;
    attributes.insert(attribute);
    senderRti->publishObjectClassAttributes(objectClass, attributes);
    auto const objectInstance = senderRti->registerObjectInstance(objectClass);
    REQUIRE(objectInstance.isValid());

    std::array<std::uint8_t, 3U> encodedValue{0x52U, 0x46U, 0x4CU};
    std::array<std::uint8_t, 3U> encodedTag{0x54U, 0x41U, 0x47U};
    AttributeHandleValueMap attributeValues;
    attributeValues.emplace(
        attribute,
        VariableLengthData(encodedValue.data(), encodedValue.size()));
    VariableLengthData userSuppliedTag(encodedTag.data(), encodedTag.size());
    REQUIRE_NOTHROW(
        senderRti->updateAttributeValues(objectInstance, attributeValues, userSuppliedTag));

    for (std::size_t evokeCount = 0U; evokeCount < 4U &&
         !receiverFederate.received; ++evokeCount) {
      static_cast<void>(receiverRti->evokeCallback(0.0));
    }
    REQUIRE(receiverFederate.discovered);
    REQUIRE(receiverFederate.received);
    REQUIRE(receiverFederate.objectInstance == objectInstance);
    REQUIRE(receiverFederate.attributeCount == 1U);
    REQUIRE(receiverFederate.value ==
            std::vector<std::uint8_t>{0x52U, 0x46U, 0x4CU});
    REQUIRE(receiverFederate.tag ==
            std::vector<std::uint8_t>{0x54U, 0x41U, 0x47U});
    // The process endpoint currently carries the standard transportation name
    // through the private event envelope; the public transportation lookup is
    // a separate support-services slice.  The callback must nevertheless
    // receive a valid official handle.
    REQUIRE(receiverFederate.transportationType.isValid());
    REQUIRE(receiverFederate.producingFederate ==
            rti1516_2025::umbra_binding_detail::makeFederateHandle(
                expectedSenderFederate.load(std::memory_order_acquire)));
    REQUIRE_FALSE(receiverFederate.hasOptionalSentRegions);

    senderRti->resignFederationExecution(DELETE_OBJECTS);
    senderJoined = false;
    receiverRti->resignFederationExecution(NO_ACTION);
    receiverJoined = false;
    senderRti->disconnect();
    receiverRti->disconnect();
  } catch (...) {
    clientError = std::current_exception();
    if (senderJoined) {
      try {
        senderRti->resignFederationExecution(DELETE_OBJECTS);
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
  if (listener) {
    listener.reset();
  }
  if (server.joinable()) {
    server.join();
  }
  if (clientError) {
    std::rethrow_exception(clientError);
  }
  REQUIRE_FALSE(serverError);
  REQUIRE(recipientCount.load(std::memory_order_acquire) == 1U);
  REQUIRE(expectedObjectInstance.load(std::memory_order_acquire) != 0U);
  REQUIRE(expectedReceiverFederate.load(std::memory_order_acquire) != 0U);
  REQUIRE_FALSE(senderJoined);
  REQUIRE_FALSE(receiverJoined);
}

TEST_CASE(
    "RTIambassador projects the 2025 region lifecycle and regional registration through a configured process endpoint",
    "[integration][foundation][data-distribution-management][object-management][transport][process-boundary][public-endpoint][rti.service.get-dimension-handle][rti.service.create-region][rti.service.set-range-bounds][rti.service.commit-region-modifications][rti.service.register-object-instance-with-regions]") {
  using umbra::detail::EmbeddedFederationRegistry;
  using umbra::detail::FederationDefinition;
  using umbra::detail::FomModuleKind;
  using umbra::detail::PrevalidatedFomModule;
  using umbra::detail::ProcessFederationService;
  using umbra::detail::ProcessTransportListener;
  using umbra::detail::ProcessTransportServiceDispatcher;
  using umbra::detail::ProcessTransportSession;

  auto listener = ProcessTransportListener::listen({"127.0.0.1", 0U});
  REQUIRE(listener);
  auto const port = listener->address().port;
  REQUIRE(port != 0U);

  constexpr wchar_t const* federationName =
      L"public-process-regional-registration-execution";
  std::exception_ptr serverError;
  std::thread server([&] {
    try {
      EmbeddedFederationRegistry registry;
      ProcessFederationService service(
          registry, composedProcessDefinition(), ProcessFederationServiceOptions{});
      auto connection = listener->accept(
          nullptr,
          {"public-process-regional-registration-server", 0x9B01U},
          [](std::wstring) {},
          [](std::wstring) { return false; });
      ProcessTransportSession session(connection);
      auto handler = service.handlerFor(session);
      std::array<TransportServiceOperation, 14U> const expected{
          TransportServiceOperation::create_federation_execution,
          TransportServiceOperation::join_federation_execution,
          TransportServiceOperation::get_object_class_handle,
          TransportServiceOperation::get_attribute_handle,
          TransportServiceOperation::get_dimension_handle,
          TransportServiceOperation::get_dimension_upper_bound,
          TransportServiceOperation::create_region,
          TransportServiceOperation::get_dimension_handle_set,
          TransportServiceOperation::set_range_bounds,
          TransportServiceOperation::get_range_bounds,
          TransportServiceOperation::commit_region_modifications,
          TransportServiceOperation::publish_object_class_attributes,
          TransportServiceOperation::register_object_instance_with_regions,
          TransportServiceOperation::resign_federation_execution};
      for (auto const operation : expected) {
        if (!ProcessTransportServiceDispatcher::serveOne(
                session,
                [&](TransportServiceMessage const& request) {
                  if (request.operation != operation) {
                    throw std::runtime_error(
                        "The public process regional-registration server received an unexpected operation.");
                  }
                  return handler(request);
                })) {
          throw std::runtime_error(
              "The public process regional-registration server lost a request.");
        }
      }
      service.detach(session);
      connection->close();
    } catch (...) {
      serverError = std::current_exception();
    }
  });

  TestFederateAmbassador federate;
  auto rti = makeRti();
  auto configuration = RtiConfiguration::createConfiguration()
                           .withConfigurationName(
                               L"public-process-regional-registration-client")
                           .withRtiAddress(
                               L"tcp://127.0.0.1:" + std::to_wstring(port));
  std::exception_ptr clientError;
  bool joined = false;
  try {
    REQUIRE(rti->connect(federate, HLA_EVOKED, configuration).addressUsed);
    rti->createFederationExecution(federationName, L"server-owned-fom.xml");
    rti->joinFederationExecution(
        L"public-process-regional-registration-federate",
        L"public-process-regional-registration-type",
        federationName);
    joined = true;

    auto const soda = rti->getObjectClassHandle(
        L"HLAobjectRoot.Food.Drink.Soda");
    auto const flavor = rti->getAttributeHandle(soda, L"Flavor");
    auto const sodaFlavor = rti->getDimensionHandle(L"SodaFlavor");
    REQUIRE(soda.isValid());
    REQUIRE(flavor.isValid());
    REQUIRE(sodaFlavor.isValid());
    REQUIRE(rti->getDimensionUpperBound(sodaFlavor) == 4UL);

    auto const region = rti->createRegion(
        rti1516_2025::DimensionHandleSet{sodaFlavor});
    REQUIRE(region.isValid());
    REQUIRE(rti->getDimensionHandleSet(region).contains(sodaFlavor));
    REQUIRE_NOTHROW(rti->setRangeBounds(
        region, sodaFlavor, rti1516_2025::RangeBounds(0UL, 1UL)));
    auto const bounds = rti->getRangeBounds(region, sodaFlavor);
    REQUIRE(bounds.getLowerBound() == 0UL);
    REQUIRE(bounds.getUpperBound() == 1UL);
    REQUIRE_NOTHROW(rti->commitRegionModifications(
        rti1516_2025::RegionHandleSet{region}));

    rti1516_2025::AttributeHandleSet const flavorOnly{flavor};
    REQUIRE_NOTHROW(rti->publishObjectClassAttributes(soda, flavorOnly));
    rti1516_2025::AttributeHandleSetRegionHandleSetPairVector const pair{{
        flavorOnly,
        rti1516_2025::RegionHandleSet{region},
    }};
    auto const objectInstance = rti->registerObjectInstanceWithRegions(soda, pair);
    REQUIRE(objectInstance.isValid());

    rti->resignFederationExecution(NO_ACTION);
    joined = false;
    rti->disconnect();
  } catch (...) {
    clientError = std::current_exception();
    if (joined) {
      try {
        rti->resignFederationExecution(NO_ACTION);
      } catch (...) {
      }
    }
    try {
      rti->disconnect();
    } catch (...) {
    }
  }
  if (listener) {
    listener.reset();
  }
  if (server.joinable()) {
    server.join();
  }
  if (clientError) {
    std::rethrow_exception(clientError);
  }
  REQUIRE_FALSE(serverError);
  REQUIRE_FALSE(joined);
}

TEST_CASE(
    "RTIambassador routes a remote regional subscription and scoped update through a configured process endpoint",
    "[integration][foundation][data-distribution-management][object-management][callbacks][transport][process-boundary][public-endpoint][rti.service.subscribe-object-class-attributes-with-regions][rti.service.update-attribute-values][federate.callback.reflect-attribute-values]") {
  class RecordingRegionalFederateAmbassador final : public NullFederateAmbassador {
   public:
    void discoverObjectInstance(
        rti1516_2025::ObjectInstanceHandle const& objectInstance,
        rti1516_2025::ObjectClassHandle const& objectClass,
        std::wstring const& objectInstanceName,
        rti1516_2025::FederateHandle const& producingFederate) override {
      discovered = true;
      discoveredObjectInstance = objectInstance;
      discoveredObjectClass = objectClass;
      discoveredObjectInstanceName = objectInstanceName;
      discoveredProducingFederate = producingFederate;
    }

    void reflectAttributeValues(
        rti1516_2025::ObjectInstanceHandle const& objectInstance,
        rti1516_2025::AttributeHandleValueMap const& attributeValues,
        rti1516_2025::VariableLengthData const& userSuppliedTag,
        rti1516_2025::TransportationTypeHandle const& transportationType,
        rti1516_2025::FederateHandle const& producingFederate,
        rti1516_2025::RegionHandleSet const* optionalSentRegions) override {
      received = true;
      this->objectInstance = objectInstance;
      this->transportationType = transportationType;
      this->producingFederate = producingFederate;
      this->attributeCount = attributeValues.size();
      this->hasOptionalSentRegions = optionalSentRegions != nullptr;
      this->sentRegionCount = optionalSentRegions == nullptr
          ? 0U
          : optionalSentRegions->size();
      this->tag.clear();
      if (userSuppliedTag.size() != 0U) {
        auto const* first = static_cast<std::uint8_t const*>(userSuppliedTag.data());
        this->tag.assign(first, first + userSuppliedTag.size());
      }
      if (!attributeValues.empty()) {
        auto const& value = attributeValues.begin()->second;
        this->value.clear();
        if (value.size() != 0U) {
          auto const* first = static_cast<std::uint8_t const*>(value.data());
          this->value.assign(first, first + value.size());
        }
      }
    }

    bool discovered = false;
    rti1516_2025::ObjectInstanceHandle discoveredObjectInstance;
    rti1516_2025::ObjectClassHandle discoveredObjectClass;
    std::wstring discoveredObjectInstanceName;
    rti1516_2025::FederateHandle discoveredProducingFederate;
    bool received = false;
    rti1516_2025::ObjectInstanceHandle objectInstance;
    rti1516_2025::TransportationTypeHandle transportationType;
    rti1516_2025::FederateHandle producingFederate;
    std::size_t attributeCount = 0U;
    bool hasOptionalSentRegions = false;
    std::size_t sentRegionCount = 0U;
    std::vector<std::uint8_t> value;
    std::vector<std::uint8_t> tag;
  } receiverFederate;

  using umbra::detail::EmbeddedFederationRegistry;
  using umbra::detail::ProcessFederationService;
  using umbra::detail::ProcessTransportConnection;
  using umbra::detail::ProcessTransportListener;
  using umbra::detail::ProcessTransportServiceDispatcher;
  using umbra::detail::ProcessTransportSession;

  auto listener = ProcessTransportListener::listen({"127.0.0.1", 0U});
  REQUIRE(listener);
  auto const port = listener->address().port;
  REQUIRE(port != 0U);

  constexpr wchar_t const* federationName =
      L"public-process-regional-subscription-execution";
  constexpr char const* objectClassName = "HLAobjectRoot.Food.Drink.Soda";
  constexpr wchar_t const* objectClassNameWide =
      L"HLAobjectRoot.Food.Drink.Soda";
  constexpr char const* attributeName = "Flavor";
  constexpr wchar_t const* attributeNameWide = L"Flavor";
  constexpr char const* dimensionName = "SodaFlavor";
  constexpr wchar_t const* dimensionNameWide = L"SodaFlavor";
  std::atomic_uint64_t expectedObjectClass{0U};
  std::atomic_uint64_t expectedAttribute{0U};
  std::atomic_uint64_t expectedDimension{0U};
  std::atomic_uint64_t expectedObjectInstance{0U};
  std::atomic_uint64_t expectedSenderFederate{0U};
  std::atomic_uint64_t recipientCount{std::numeric_limits<std::uint32_t>::max()};
  std::exception_ptr serverError;
  std::thread server([&] {
    try {
      EmbeddedFederationRegistry registry;
      ProcessFederationService service(
          registry, composedProcessDefinition(), ProcessFederationServiceOptions{});
      auto senderConnection = listener->accept(
          nullptr,
          {"public-process-regional-subscription-server", 0x9C01U},
          [](std::wstring) {},
          [](std::wstring) { return false; });
      ProcessTransportSession sender(senderConnection);
      auto senderHandler = service.handlerFor(sender);
      auto receiverConnection = std::shared_ptr<ProcessTransportConnection>{};
      auto serveExpected = [&](ProcessTransportSession& session,
                               auto const& handler,
                               TransportServiceOperation operation,
                               char const* description) {
        if (!ProcessTransportServiceDispatcher::serveOne(
                session,
                [&](TransportServiceMessage const& request) {
                  if (request.operation != operation) {
                    throw std::runtime_error(description);
                  }
                  return handler(request);
                })) {
          throw std::runtime_error(description);
        }
      };

      auto senderRecordingHandler = [&](TransportServiceMessage const& request) {
        auto response = senderHandler(request);
        if (request.operation == TransportServiceOperation::join_federation_execution &&
            response.status == TransportServiceStatus::ok) {
          auto const join = umbra::detail::decodeProcessFederationJoinResult(
              response.payload);
          expectedSenderFederate.store(join.federateId, std::memory_order_release);
        }
        if (request.operation == TransportServiceOperation::register_object_instance_with_regions &&
            response.status == TransportServiceStatus::ok) {
          auto const registration =
              umbra::detail::decodeProcessFederationRegisterObjectInstanceResult(
                  response.payload);
          expectedObjectInstance.store(
              registration.objectInstanceHandle, std::memory_order_release);
        }
        if (request.operation == TransportServiceOperation::update_attribute_values &&
            response.status == TransportServiceStatus::ok) {
          recipientCount.store(
              umbra::detail::decodeProcessFederationUpdateAttributeValuesResult(
                  response.payload)
                  .recipientCount,
              std::memory_order_release);
        }
        return response;
      };

      serveExpected(
          sender,
          senderRecordingHandler,
          TransportServiceOperation::create_federation_execution,
          "The public process regional-subscription server lost Create.");
      auto const objectClass = registry.objectClassHandleFor(
          federationName, objectClassName);
      auto const attribute = registry.attributeHandleFor(
          federationName, objectClassName, attributeName);
      auto const dimension = registry.dimensionHandleFor(
          federationName, dimensionName);
      if (!objectClass || !attribute || !dimension) {
        throw std::runtime_error(
            "The public process regional-subscription server could not resolve its FOM handles.");
      }
      expectedObjectClass.store(*objectClass, std::memory_order_release);
      expectedAttribute.store(*attribute, std::memory_order_release);
      expectedDimension.store(*dimension, std::memory_order_release);
      serveExpected(
          sender,
          senderRecordingHandler,
          TransportServiceOperation::join_federation_execution,
          "The public process regional-subscription server lost sender Join.");

      receiverConnection = listener->accept(
          nullptr,
          {"public-process-regional-subscription-server", 0x9C02U},
          [](std::wstring) {},
          [](std::wstring) { return false; });
      ProcessTransportSession receiver(receiverConnection);
      auto receiverHandler = service.handlerFor(receiver);
      serveExpected(
          receiver,
          receiverHandler,
          TransportServiceOperation::join_federation_execution,
          "The public process regional-subscription server lost receiver Join.");

      for (auto const operation : {
               TransportServiceOperation::get_object_class_handle,
               TransportServiceOperation::get_attribute_handle,
               TransportServiceOperation::get_dimension_handle,
               TransportServiceOperation::create_region,
               TransportServiceOperation::set_range_bounds,
               TransportServiceOperation::commit_region_modifications,
           }) {
        serveExpected(
            sender,
            senderRecordingHandler,
            operation,
            "The public process regional-subscription server lost sender DDM setup.");
      }
      for (auto const operation : {
               TransportServiceOperation::get_object_class_handle,
               TransportServiceOperation::get_attribute_handle,
               TransportServiceOperation::get_dimension_handle,
               TransportServiceOperation::create_region,
               TransportServiceOperation::set_range_bounds,
               TransportServiceOperation::commit_region_modifications,
               TransportServiceOperation::subscribe_object_class_attributes_with_regions,
           }) {
        serveExpected(
            receiver,
            receiverHandler,
            operation,
            "The public process regional-subscription server lost receiver DDM setup.");
      }
      serveExpected(
          sender,
          senderRecordingHandler,
          TransportServiceOperation::publish_object_class_attributes,
          "The public process regional-subscription server lost Publish.");
      serveExpected(
          sender,
          senderRecordingHandler,
          TransportServiceOperation::register_object_instance_with_regions,
          "The public process regional-subscription server lost regional Register.");
      serveExpected(
          sender,
          senderRecordingHandler,
          TransportServiceOperation::update_attribute_values,
          "The public process regional-subscription server lost regional Update.");
      serveExpected(
          receiver,
          receiverHandler,
          TransportServiceOperation::receive_interaction,
          "The public process regional-subscription server lost discovery Receive.");
      serveExpected(
          receiver,
          receiverHandler,
          TransportServiceOperation::receive_interaction,
          "The public process regional-subscription server lost reflection Receive.");
      serveExpected(
          sender,
          senderRecordingHandler,
          TransportServiceOperation::resign_federation_execution,
          "The public process regional-subscription server lost sender Resign.");
      serveExpected(
          receiver,
          receiverHandler,
          TransportServiceOperation::resign_federation_execution,
          "The public process regional-subscription server lost receiver Resign.");
      service.detach(sender);
      service.detach(receiver);
      senderConnection->close();
      receiverConnection->close();
    } catch (...) {
      serverError = std::current_exception();
    }
  });

  TestFederateAmbassador senderFederate;
  auto senderRti = makeRti();
  auto receiverRti = makeRti();
  auto configuration = RtiConfiguration::createConfiguration()
                           .withConfigurationName(
                               L"public-process-regional-subscription-client")
                           .withRtiAddress(
                               L"tcp://127.0.0.1:" + std::to_wstring(port));
  std::exception_ptr clientError;
  bool senderJoined = false;
  bool receiverJoined = false;
  rti1516_2025::RegionHandle senderRegion;
  try {
    REQUIRE(senderRti->connect(senderFederate, HLA_EVOKED, configuration).addressUsed);
    senderRti->createFederationExecution(federationName, L"server-owned-fom.xml");
    senderRti->joinFederationExecution(
        L"public-process-regional-subscription-sender",
        L"public-process-regional-subscription-type",
        federationName);
    senderJoined = true;

    REQUIRE(receiverRti->connect(receiverFederate, HLA_EVOKED, configuration).addressUsed);
    receiverRti->joinFederationExecution(
        L"public-process-regional-subscription-receiver",
        L"public-process-regional-subscription-type",
        federationName);
    receiverJoined = true;

    auto const senderObjectClass = senderRti->getObjectClassHandle(objectClassNameWide);
    auto const senderAttribute = senderRti->getAttributeHandle(
        senderObjectClass, attributeNameWide);
    auto const senderDimension = senderRti->getDimensionHandle(dimensionNameWide);
    REQUIRE(senderObjectClass.toString() ==
            L"ObjectClassHandle(" +
                std::to_wstring(expectedObjectClass.load(std::memory_order_acquire)) +
                L")");
    REQUIRE(senderAttribute == rti1516_2025::umbra_binding_detail::makeAttributeHandle(
                                  expectedAttribute.load(std::memory_order_acquire)));
    REQUIRE(senderDimension.toString() ==
            L"DimensionHandle(" +
                std::to_wstring(expectedDimension.load(std::memory_order_acquire)) +
                L")");
    senderRegion = senderRti->createRegion(
        rti1516_2025::DimensionHandleSet{senderDimension});
    REQUIRE(senderRegion.isValid());
    REQUIRE_NOTHROW(senderRti->setRangeBounds(
        senderRegion, senderDimension, rti1516_2025::RangeBounds(0UL, 1UL)));
    REQUIRE_NOTHROW(senderRti->commitRegionModifications(
        rti1516_2025::RegionHandleSet{senderRegion}));

    auto const receiverObjectClass = receiverRti->getObjectClassHandle(objectClassNameWide);
    auto const receiverAttribute = receiverRti->getAttributeHandle(
        receiverObjectClass, attributeNameWide);
    auto const receiverDimension = receiverRti->getDimensionHandle(dimensionNameWide);
    auto const receiverRegion = receiverRti->createRegion(
        rti1516_2025::DimensionHandleSet{receiverDimension});
    REQUIRE(receiverRegion.isValid());
    REQUIRE_NOTHROW(receiverRti->setRangeBounds(
        receiverRegion, receiverDimension, rti1516_2025::RangeBounds(0UL, 1UL)));
    REQUIRE_NOTHROW(receiverRti->commitRegionModifications(
        rti1516_2025::RegionHandleSet{receiverRegion}));
    rti1516_2025::AttributeHandleSet const receiverAttributes{receiverAttribute};
    rti1516_2025::AttributeHandleSetRegionHandleSetPairVector const receiverPairs{{
        receiverAttributes,
        rti1516_2025::RegionHandleSet{receiverRegion},
    }};
    REQUIRE_NOTHROW(receiverRti->subscribeObjectClassAttributesWithRegions(
        receiverObjectClass,
        receiverPairs));

    rti1516_2025::AttributeHandleSet const senderAttributes{senderAttribute};
    REQUIRE_NOTHROW(senderRti->publishObjectClassAttributes(
        senderObjectClass, senderAttributes));
    auto const objectInstance = senderRti->registerObjectInstanceWithRegions(
        senderObjectClass,
        rti1516_2025::AttributeHandleSetRegionHandleSetPairVector{{
            senderAttributes,
            rti1516_2025::RegionHandleSet{senderRegion},
        }});
    REQUIRE(objectInstance.isValid());

    std::array<std::uint8_t, 3U> encodedValue{0x52U, 0x45U, 0x47U};
    std::array<std::uint8_t, 3U> encodedTag{0x52U, 0x45U, 0x47U};
    rti1516_2025::AttributeHandleValueMap attributeValues;
    attributeValues.emplace(
        senderAttribute,
        rti1516_2025::VariableLengthData(encodedValue.data(), encodedValue.size()));
    rti1516_2025::VariableLengthData userSuppliedTag(
        encodedTag.data(), encodedTag.size());
    REQUIRE_NOTHROW(senderRti->updateAttributeValues(
        objectInstance, attributeValues, userSuppliedTag));

    for (std::size_t evokeCount = 0U; evokeCount < 5U &&
         !receiverFederate.received; ++evokeCount) {
      static_cast<void>(receiverRti->evokeCallback(0.0));
    }
    REQUIRE(receiverFederate.discovered);
    REQUIRE(receiverFederate.received);
    REQUIRE(receiverFederate.discoveredObjectInstance == objectInstance);
    REQUIRE(receiverFederate.objectInstance == objectInstance);
    REQUIRE(receiverFederate.attributeCount == 1U);
    REQUIRE(receiverFederate.value == std::vector<std::uint8_t>{0x52U, 0x45U, 0x47U});
    REQUIRE(receiverFederate.tag == std::vector<std::uint8_t>{0x52U, 0x45U, 0x47U});
    REQUIRE(receiverFederate.hasOptionalSentRegions);
    REQUIRE(receiverFederate.sentRegionCount == 1U);
    REQUIRE(receiverFederate.transportationType.isValid());
    REQUIRE(recipientCount.load(std::memory_order_acquire) == 1U);
    REQUIRE(expectedObjectInstance.load(std::memory_order_acquire) != 0U);
    REQUIRE(expectedSenderFederate.load(std::memory_order_acquire) != 0U);

    senderRti->resignFederationExecution(DELETE_OBJECTS);
    senderJoined = false;
    receiverRti->resignFederationExecution(NO_ACTION);
    receiverJoined = false;
    senderRti->disconnect();
    receiverRti->disconnect();
  } catch (...) {
    clientError = std::current_exception();
    if (senderJoined) {
      try {
        senderRti->resignFederationExecution(DELETE_OBJECTS);
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
  if (listener) {
    listener.reset();
  }
  if (server.joinable()) {
    server.join();
  }
  if (clientError) {
    std::rethrow_exception(clientError);
  }
  REQUIRE_FALSE(serverError);
  REQUIRE_FALSE(senderJoined);
  REQUIRE_FALSE(receiverJoined);
}

TEST_CASE(
    "RTIambassador preserves a timestamped regional update through a configured process endpoint",
    "[integration][foundation][data-distribution-management][object-management][time-management][callbacks][transport][process-boundary][public-endpoint][rti.service.subscribe-object-class-attributes-with-regions][rti.service.register-object-instance-with-regions][rti.service.update-attribute-values][federate.callback.discover-object-instance][federate.callback.reflect-attribute-values]") {
  class RecordingRegionalFederateAmbassador final : public NullFederateAmbassador {
   public:
    void discoverObjectInstance(
        rti1516_2025::ObjectInstanceHandle const& objectInstance,
        rti1516_2025::ObjectClassHandle const& objectClass,
        std::wstring const& objectInstanceName,
        rti1516_2025::FederateHandle const& producingFederate) override {
      discovered = true;
      discoveredObjectInstance = objectInstance;
      discoveredObjectClass = objectClass;
      discoveredObjectInstanceName = objectInstanceName;
      discoveredProducingFederate = producingFederate;
    }

    void reflectAttributeValues(
        rti1516_2025::ObjectInstanceHandle const& objectInstance,
        rti1516_2025::AttributeHandleValueMap const& attributeValues,
        rti1516_2025::VariableLengthData const& userSuppliedTag,
        rti1516_2025::TransportationTypeHandle const& transportationType,
        rti1516_2025::FederateHandle const& producingFederate,
        rti1516_2025::RegionHandleSet const* optionalSentRegions) override {
      ordinaryReceived = true;
      recordAttributeValues(
          objectInstance,
          attributeValues,
          userSuppliedTag,
          transportationType,
          producingFederate,
          optionalSentRegions);
    }

    void reflectAttributeValues(
        rti1516_2025::ObjectInstanceHandle const& objectInstance,
        rti1516_2025::AttributeHandleValueMap const& attributeValues,
        rti1516_2025::VariableLengthData const& userSuppliedTag,
        rti1516_2025::TransportationTypeHandle const& transportationType,
        rti1516_2025::FederateHandle const& producingFederate,
        rti1516_2025::RegionHandleSet const* optionalSentRegions,
        rti1516_2025::LogicalTime const& time,
        rti1516_2025::OrderType sentOrder,
        rti1516_2025::OrderType receivedOrder,
        rti1516_2025::MessageRetractionHandle const* optionalRetraction) override {
      timestampedReceived = true;
      sentOrderType = sentOrder;
      receivedOrderType = receivedOrder;
      hasOptionalRetraction = optionalRetraction != nullptr;
      timestampImplementationName = time.implementationName();
      auto const encoded = time.encode();
      timestampEncoding.clear();
      if (encoded.size() != 0U) {
        auto const* first = static_cast<std::uint8_t const*>(encoded.data());
        timestampEncoding.assign(first, first + encoded.size());
      }
      recordAttributeValues(
          objectInstance,
          attributeValues,
          userSuppliedTag,
          transportationType,
          producingFederate,
          optionalSentRegions);
    }

    bool discovered = false;
    rti1516_2025::ObjectInstanceHandle discoveredObjectInstance;
    rti1516_2025::ObjectClassHandle discoveredObjectClass;
    std::wstring discoveredObjectInstanceName;
    rti1516_2025::FederateHandle discoveredProducingFederate;
    bool ordinaryReceived = false;
    bool timestampedReceived = false;
    rti1516_2025::ObjectInstanceHandle objectInstance;
    rti1516_2025::TransportationTypeHandle transportationType;
    rti1516_2025::FederateHandle producingFederate;
    std::size_t attributeCount = 0U;
    bool hasOptionalSentRegions = false;
    std::size_t sentRegionCount = 0U;
    std::vector<std::uint8_t> value;
    std::vector<std::uint8_t> tag;
    std::wstring timestampImplementationName;
    std::vector<std::uint8_t> timestampEncoding;
    rti1516_2025::OrderType sentOrderType = rti1516_2025::RECEIVE;
    rti1516_2025::OrderType receivedOrderType = rti1516_2025::RECEIVE;
    bool hasOptionalRetraction = false;

   private:
    void recordAttributeValues(
        rti1516_2025::ObjectInstanceHandle const& objectInstance,
        rti1516_2025::AttributeHandleValueMap const& attributeValues,
        rti1516_2025::VariableLengthData const& userSuppliedTag,
        rti1516_2025::TransportationTypeHandle const& transportationType,
        rti1516_2025::FederateHandle const& producingFederate,
        rti1516_2025::RegionHandleSet const* optionalSentRegions) {
      this->objectInstance = objectInstance;
      this->transportationType = transportationType;
      this->producingFederate = producingFederate;
      this->attributeCount = attributeValues.size();
      this->hasOptionalSentRegions = optionalSentRegions != nullptr;
      this->sentRegionCount = optionalSentRegions == nullptr
          ? 0U
          : optionalSentRegions->size();
      this->tag.clear();
      if (userSuppliedTag.size() != 0U) {
        auto const* first = static_cast<std::uint8_t const*>(userSuppliedTag.data());
        this->tag.assign(first, first + userSuppliedTag.size());
      }
      this->value.clear();
      if (!attributeValues.empty()) {
        auto const& value = attributeValues.begin()->second;
        if (value.size() != 0U) {
          auto const* first = static_cast<std::uint8_t const*>(value.data());
          this->value.assign(first, first + value.size());
        }
      }
    }
  } receiverFederate;

  auto listener = ProcessTransportListener::listen({"127.0.0.1", 0U});
  REQUIRE(listener);
  auto const port = listener->address().port;
  REQUIRE(port != 0U);

  constexpr wchar_t const* federationName =
      L"public-process-timestamped-regional-execution";
  constexpr char const* objectClassName = "HLAobjectRoot.Food.Drink.Soda";
  constexpr wchar_t const* objectClassNameWide =
      L"HLAobjectRoot.Food.Drink.Soda";
  constexpr char const* attributeName = "Flavor";
  constexpr wchar_t const* attributeNameWide = L"Flavor";
  constexpr char const* dimensionName = "SodaFlavor";
  constexpr wchar_t const* dimensionNameWide = L"SodaFlavor";
  std::atomic_uint64_t expectedObjectClass{0U};
  std::atomic_uint64_t expectedAttribute{0U};
  std::atomic_uint64_t expectedDimension{0U};
  std::atomic_uint64_t expectedObjectInstance{0U};
  std::atomic_uint64_t expectedSenderFederate{0U};
  std::atomic_uint64_t recipientCount{std::numeric_limits<std::uint32_t>::max()};
  std::atomic_bool timestampPresent{false};
  std::exception_ptr serverError;
  std::thread server([&] {
    try {
      EmbeddedFederationRegistry registry;
      ProcessFederationService service(
          registry, composedProcessDefinition(), ProcessFederationServiceOptions{});
      auto senderConnection = listener->accept(
          nullptr,
          {"public-process-timestamped-regional-server", 0x9D01U},
          [](std::wstring) {},
          [](std::wstring) { return false; });
      ProcessTransportSession sender(senderConnection);
      auto senderHandler = service.handlerFor(sender);
      auto receiverConnection = std::shared_ptr<ProcessTransportConnection>{};
      auto serveExpected = [&](ProcessTransportSession& session,
                               auto const& handler,
                               TransportServiceOperation operation,
                               char const* description) {
        if (!ProcessTransportServiceDispatcher::serveOne(
                session,
                [&](TransportServiceMessage const& request) {
                  if (request.operation != operation) {
                    throw std::runtime_error(description);
                  }
                  return handler(request);
                })) {
          throw std::runtime_error(description);
        }
      };

      auto senderRecordingHandler = [&](TransportServiceMessage const& request) {
        if (request.operation == TransportServiceOperation::update_attribute_values) {
          auto const update =
              umbra::detail::decodeProcessFederationUpdateAttributeValuesRequest(
                  request.payload);
          timestampPresent.store(update.timestamp.has_value(), std::memory_order_release);
        }
        auto response = senderHandler(request);
        if (request.operation == TransportServiceOperation::join_federation_execution &&
            response.status == TransportServiceStatus::ok) {
          auto const join = umbra::detail::decodeProcessFederationJoinResult(
              response.payload);
          expectedSenderFederate.store(join.federateId, std::memory_order_release);
        }
        if (request.operation == TransportServiceOperation::register_object_instance_with_regions &&
            response.status == TransportServiceStatus::ok) {
          auto const registration =
              umbra::detail::decodeProcessFederationRegisterObjectInstanceResult(
                  response.payload);
          expectedObjectInstance.store(
              registration.objectInstanceHandle, std::memory_order_release);
        }
        if (request.operation == TransportServiceOperation::update_attribute_values &&
            response.status == TransportServiceStatus::ok) {
          recipientCount.store(
              umbra::detail::decodeProcessFederationUpdateAttributeValuesResult(
                  response.payload)
                  .recipientCount,
              std::memory_order_release);
        }
        return response;
      };

      serveExpected(
          sender,
          senderRecordingHandler,
          TransportServiceOperation::create_federation_execution,
          "The public timestamped regional server lost Create.");
      auto const objectClass = registry.objectClassHandleFor(
          federationName, objectClassName);
      auto const attribute = registry.attributeHandleFor(
          federationName, objectClassName, attributeName);
      auto const dimension = registry.dimensionHandleFor(
          federationName, dimensionName);
      if (!objectClass || !attribute || !dimension) {
        throw std::runtime_error(
            "The public timestamped regional server could not resolve its FOM handles.");
      }
      expectedObjectClass.store(*objectClass, std::memory_order_release);
      expectedAttribute.store(*attribute, std::memory_order_release);
      expectedDimension.store(*dimension, std::memory_order_release);
      serveExpected(
          sender,
          senderRecordingHandler,
          TransportServiceOperation::join_federation_execution,
          "The public timestamped regional server lost sender Join.");

      receiverConnection = listener->accept(
          nullptr,
          {"public-process-timestamped-regional-server", 0x9D02U},
          [](std::wstring) {},
          [](std::wstring) { return false; });
      ProcessTransportSession receiver(receiverConnection);
      auto receiverHandler = service.handlerFor(receiver);
      serveExpected(
          receiver,
          receiverHandler,
          TransportServiceOperation::join_federation_execution,
          "The public timestamped regional server lost receiver Join.");

      for (auto const operation : {
               TransportServiceOperation::get_object_class_handle,
               TransportServiceOperation::get_attribute_handle,
               TransportServiceOperation::get_dimension_handle,
               TransportServiceOperation::create_region,
               TransportServiceOperation::set_range_bounds,
               TransportServiceOperation::commit_region_modifications,
           }) {
        serveExpected(
            sender,
            senderRecordingHandler,
            operation,
            "The public timestamped regional server lost sender DDM setup.");
      }
      for (auto const operation : {
               TransportServiceOperation::get_object_class_handle,
               TransportServiceOperation::get_attribute_handle,
               TransportServiceOperation::get_dimension_handle,
               TransportServiceOperation::create_region,
               TransportServiceOperation::set_range_bounds,
               TransportServiceOperation::commit_region_modifications,
               TransportServiceOperation::subscribe_object_class_attributes_with_regions,
           }) {
        serveExpected(
            receiver,
            receiverHandler,
            operation,
            "The public timestamped regional server lost receiver DDM setup.");
      }
      serveExpected(
          sender,
          senderRecordingHandler,
          TransportServiceOperation::publish_object_class_attributes,
          "The public timestamped regional server lost Publish.");
      serveExpected(
          sender,
          senderRecordingHandler,
          TransportServiceOperation::register_object_instance_with_regions,
          "The public timestamped regional server lost regional Register.");
      serveExpected(
          sender,
          senderRecordingHandler,
          TransportServiceOperation::update_attribute_values,
          "The public timestamped regional server lost regional Update.");
      serveExpected(
          receiver,
          receiverHandler,
          TransportServiceOperation::receive_interaction,
          "The public timestamped regional server lost discovery Receive.");
      serveExpected(
          receiver,
          receiverHandler,
          TransportServiceOperation::receive_interaction,
          "The public timestamped regional server lost reflection Receive.");
      serveExpected(
          sender,
          senderRecordingHandler,
          TransportServiceOperation::resign_federation_execution,
          "The public timestamped regional server lost sender Resign.");
      serveExpected(
          receiver,
          receiverHandler,
          TransportServiceOperation::resign_federation_execution,
          "The public timestamped regional server lost receiver Resign.");
      service.detach(sender);
      service.detach(receiver);
      senderConnection->close();
      receiverConnection->close();
    } catch (...) {
      serverError = std::current_exception();
    }
  });

  TestFederateAmbassador senderFederate;
  auto senderRti = makeRti();
  auto receiverRti = makeRti();
  auto configuration = RtiConfiguration::createConfiguration()
                           .withConfigurationName(
                               L"public-process-timestamped-regional-client")
                           .withRtiAddress(
                               L"tcp://127.0.0.1:" + std::to_wstring(port));
  std::exception_ptr clientError;
  bool senderJoined = false;
  bool receiverJoined = false;
  try {
    REQUIRE(senderRti->connect(senderFederate, HLA_EVOKED, configuration).addressUsed);
    senderRti->createFederationExecution(federationName, L"server-owned-fom.xml");
    auto const senderHandle = senderRti->joinFederationExecution(
        L"public-process-timestamped-regional-sender",
        L"public-process-timestamped-regional-type",
        federationName);
    senderJoined = true;

    REQUIRE(receiverRti->connect(receiverFederate, HLA_EVOKED, configuration).addressUsed);
    receiverRti->joinFederationExecution(
        L"public-process-timestamped-regional-receiver",
        L"public-process-timestamped-regional-type",
        federationName);
    receiverJoined = true;

    auto const senderObjectClass = senderRti->getObjectClassHandle(objectClassNameWide);
    auto const senderAttribute = senderRti->getAttributeHandle(
        senderObjectClass, attributeNameWide);
    auto const senderDimension = senderRti->getDimensionHandle(dimensionNameWide);
    REQUIRE(senderObjectClass.toString() ==
            L"ObjectClassHandle(" +
                std::to_wstring(expectedObjectClass.load(std::memory_order_acquire)) +
                L")");
    REQUIRE(senderAttribute == rti1516_2025::umbra_binding_detail::makeAttributeHandle(
                                  expectedAttribute.load(std::memory_order_acquire)));
    REQUIRE(senderDimension.toString() ==
            L"DimensionHandle(" +
                std::to_wstring(expectedDimension.load(std::memory_order_acquire)) +
                L")");
    auto const senderRegion = senderRti->createRegion(
        rti1516_2025::DimensionHandleSet{senderDimension});
    REQUIRE(senderRegion.isValid());
    REQUIRE_NOTHROW(senderRti->setRangeBounds(
        senderRegion, senderDimension, rti1516_2025::RangeBounds(0UL, 1UL)));
    REQUIRE_NOTHROW(senderRti->commitRegionModifications(
        rti1516_2025::RegionHandleSet{senderRegion}));

    auto const receiverObjectClass = receiverRti->getObjectClassHandle(objectClassNameWide);
    auto const receiverAttribute = receiverRti->getAttributeHandle(
        receiverObjectClass, attributeNameWide);
    auto const receiverDimension = receiverRti->getDimensionHandle(dimensionNameWide);
    auto const receiverRegion = receiverRti->createRegion(
        rti1516_2025::DimensionHandleSet{receiverDimension});
    REQUIRE(receiverRegion.isValid());
    REQUIRE_NOTHROW(receiverRti->setRangeBounds(
        receiverRegion, receiverDimension, rti1516_2025::RangeBounds(0UL, 1UL)));
    REQUIRE_NOTHROW(receiverRti->commitRegionModifications(
        rti1516_2025::RegionHandleSet{receiverRegion}));
    rti1516_2025::AttributeHandleSet const receiverAttributes{receiverAttribute};
    rti1516_2025::AttributeHandleSetRegionHandleSetPairVector const receiverPairs{{
        receiverAttributes,
        rti1516_2025::RegionHandleSet{receiverRegion},
    }};
    REQUIRE_NOTHROW(receiverRti->subscribeObjectClassAttributesWithRegions(
        receiverObjectClass,
        receiverPairs));

    rti1516_2025::AttributeHandleSet const senderAttributes{senderAttribute};
    REQUIRE_NOTHROW(senderRti->publishObjectClassAttributes(
        senderObjectClass, senderAttributes));
    auto const objectInstance = senderRti->registerObjectInstanceWithRegions(
        senderObjectClass,
        rti1516_2025::AttributeHandleSetRegionHandleSetPairVector{{
            senderAttributes,
            rti1516_2025::RegionHandleSet{senderRegion},
        }});
    REQUIRE(objectInstance.isValid());

    auto factory = rti1516_2025::HLAlogicalTimeFactoryFactory::makeLogicalTimeFactory(
        L"HLAinteger64Time");
    auto* integerFactory =
        dynamic_cast<rti1516_2025::HLAinteger64TimeFactory*>(factory.get());
    REQUIRE(integerFactory != nullptr);
    auto timestamp = integerFactory->makeLogicalTime(5);
    REQUIRE(timestamp);
    auto const expectedTimestampEncoding = timestamp->encode();

    std::array<std::uint8_t, 3U> encodedValue{0x54U, 0x53U, 0x4FU};
    std::array<std::uint8_t, 3U> encodedTag{0x54U, 0x52U, 0x47U};
    rti1516_2025::AttributeHandleValueMap attributeValues;
    attributeValues.emplace(
        senderAttribute,
        rti1516_2025::VariableLengthData(encodedValue.data(), encodedValue.size()));
    rti1516_2025::VariableLengthData userSuppliedTag(
        encodedTag.data(), encodedTag.size());
    auto const retraction = senderRti->updateAttributeValues(
        objectInstance, attributeValues, userSuppliedTag, *timestamp);
    REQUIRE_FALSE(retraction.isValid());

    for (std::size_t evokeCount = 0U; evokeCount < 5U &&
         !receiverFederate.timestampedReceived; ++evokeCount) {
      static_cast<void>(receiverRti->evokeCallback(0.0));
    }
    REQUIRE(receiverFederate.discovered);
    REQUIRE(receiverFederate.timestampedReceived);
    REQUIRE_FALSE(receiverFederate.ordinaryReceived);
    REQUIRE(receiverFederate.discoveredObjectInstance == objectInstance);
    REQUIRE(receiverFederate.objectInstance == objectInstance);
    REQUIRE(receiverFederate.attributeCount == 1U);
    REQUIRE(receiverFederate.value == std::vector<std::uint8_t>{0x54U, 0x53U, 0x4FU});
    REQUIRE(receiverFederate.tag == std::vector<std::uint8_t>{0x54U, 0x52U, 0x47U});
    REQUIRE(receiverFederate.hasOptionalSentRegions);
    REQUIRE(receiverFederate.sentRegionCount == 1U);
    REQUIRE(receiverFederate.transportationType.isValid());
    REQUIRE(receiverFederate.producingFederate == senderHandle);
    REQUIRE(receiverFederate.timestampImplementationName == L"HLAinteger64Time");
    std::vector<std::uint8_t> expectedTimestampBytes;
    if (expectedTimestampEncoding.size() != 0U) {
      auto const* first = static_cast<std::uint8_t const*>(expectedTimestampEncoding.data());
      expectedTimestampBytes.assign(
          first, first + expectedTimestampEncoding.size());
    }
    REQUIRE(receiverFederate.timestampEncoding == expectedTimestampBytes);
    REQUIRE(receiverFederate.sentOrderType == rti1516_2025::RECEIVE);
    REQUIRE(receiverFederate.receivedOrderType == rti1516_2025::RECEIVE);
    REQUIRE_FALSE(receiverFederate.hasOptionalRetraction);
    REQUIRE(timestampPresent.load(std::memory_order_acquire));
    REQUIRE(recipientCount.load(std::memory_order_acquire) == 1U);
    REQUIRE(expectedObjectInstance.load(std::memory_order_acquire) != 0U);
    REQUIRE(expectedSenderFederate.load(std::memory_order_acquire) != 0U);

    senderRti->resignFederationExecution(DELETE_OBJECTS);
    senderJoined = false;
    receiverRti->resignFederationExecution(NO_ACTION);
    receiverJoined = false;
    senderRti->disconnect();
    receiverRti->disconnect();
  } catch (...) {
    clientError = std::current_exception();
    if (senderJoined) {
      try {
        senderRti->resignFederationExecution(DELETE_OBJECTS);
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
  if (listener) {
    listener.reset();
  }
  if (server.joinable()) {
    server.join();
  }
  if (clientError) {
    std::rethrow_exception(clientError);
  }
  REQUIRE_FALSE(serverError);
  REQUIRE_FALSE(senderJoined);
  REQUIRE_FALSE(receiverJoined);
}

TEST_CASE(
    "RTIambassador removes a regional subscription through a configured process endpoint",
    "[integration][foundation][data-distribution-management][object-management][callbacks][transport][process-boundary][public-endpoint][rti.service.subscribe-object-class-attributes-with-regions][rti.service.unsubscribe-object-class-attributes-with-regions][rti.service.register-object-instance-with-regions][rti.service.associate-regions-for-updates][rti.service.unassociate-regions-for-updates][rti.service.update-attribute-values][rti.service.set-attribute-scope-advisory-switch][rti.service.get-attribute-scope-advisory-switch][federate.callback.discover-object-instance][federate.callback.reflect-attribute-values][federate.callback.attributes-in-scope][federate.callback.attributes-out-of-scope]") {
  class RecordingRegionalFederateAmbassador final : public NullFederateAmbassador {
   public:
    void discoverObjectInstance(
        rti1516_2025::ObjectInstanceHandle const& objectInstance,
        rti1516_2025::ObjectClassHandle const& objectClass,
        std::wstring const& objectInstanceName,
        rti1516_2025::FederateHandle const& producingFederate) override {
      discovered = true;
      discoveredObjectInstance = objectInstance;
      discoveredObjectClass = objectClass;
      discoveredObjectInstanceName = objectInstanceName;
      discoveredProducingFederate = producingFederate;
    }

    void reflectAttributeValues(
        rti1516_2025::ObjectInstanceHandle const& objectInstance,
        rti1516_2025::AttributeHandleValueMap const& attributeValues,
        rti1516_2025::VariableLengthData const& userSuppliedTag,
        rti1516_2025::TransportationTypeHandle const& transportationType,
        rti1516_2025::FederateHandle const& producingFederate,
        rti1516_2025::RegionHandleSet const* optionalSentRegions) override {
      reflected = true;
      reflectedObjectInstance = objectInstance;
      reflectedAttributeCount = attributeValues.size();
      reflectedTransportationType = transportationType;
      reflectedProducingFederate = producingFederate;
      reflectedHasOptionalSentRegions = optionalSentRegions != nullptr;
      reflectedTagSize = userSuppliedTag.size();
    }

    void attributesOutOfScope(
        rti1516_2025::ObjectInstanceHandle const& objectInstance,
        rti1516_2025::AttributeHandleSet const& attributes) override {
      outOfScope = true;
      ++outOfScopeCount;
      outOfScopeObjectInstance = objectInstance;
      outOfScopeAttributeCount = attributes.size();
    }

    void attributesInScope(
        rti1516_2025::ObjectInstanceHandle const& objectInstance,
        rti1516_2025::AttributeHandleSet const& attributes) override {
      inScope = true;
      ++inScopeCount;
      inScopeObjectInstance = objectInstance;
      inScopeAttributeCount = attributes.size();
    }

    bool discovered = false;
    rti1516_2025::ObjectInstanceHandle discoveredObjectInstance;
    rti1516_2025::ObjectClassHandle discoveredObjectClass;
    std::wstring discoveredObjectInstanceName;
    rti1516_2025::FederateHandle discoveredProducingFederate;
    bool reflected = false;
    rti1516_2025::ObjectInstanceHandle reflectedObjectInstance;
    std::size_t reflectedAttributeCount = 0U;
    rti1516_2025::TransportationTypeHandle reflectedTransportationType;
    rti1516_2025::FederateHandle reflectedProducingFederate;
    bool reflectedHasOptionalSentRegions = false;
    std::size_t reflectedTagSize = 0U;
    bool outOfScope = false;
    std::size_t outOfScopeCount = 0U;
    rti1516_2025::ObjectInstanceHandle outOfScopeObjectInstance;
    std::size_t outOfScopeAttributeCount = 0U;
    bool inScope = false;
    std::size_t inScopeCount = 0U;
    rti1516_2025::ObjectInstanceHandle inScopeObjectInstance;
    std::size_t inScopeAttributeCount = 0U;
  } receiverFederate;

  auto listener = ProcessTransportListener::listen({"127.0.0.1", 0U});
  REQUIRE(listener);
  auto const port = listener->address().port;
  REQUIRE(port != 0U);

  constexpr wchar_t const* federationName =
      L"public-process-regional-unsubscribe-execution";
  constexpr char const* objectClassName = "HLAobjectRoot.Food.Drink.Soda";
  constexpr wchar_t const* objectClassNameWide =
      L"HLAobjectRoot.Food.Drink.Soda";
  constexpr char const* attributeName = "Flavor";
  constexpr wchar_t const* attributeNameWide = L"Flavor";
  constexpr char const* dimensionName = "SodaFlavor";
  constexpr wchar_t const* dimensionNameWide = L"SodaFlavor";
  std::atomic_uint64_t expectedObjectClass{0U};
  std::atomic_uint64_t expectedAttribute{0U};
  std::atomic_uint64_t expectedDimension{0U};
  std::atomic_uint64_t expectedObjectInstance{0U};
  std::atomic_uint64_t expectedSenderFederate{0U};
  std::atomic_uint64_t recipientCount{std::numeric_limits<std::uint32_t>::max()};
  std::exception_ptr serverError;
  std::thread server([&] {
    try {
      EmbeddedFederationRegistry registry;
      ProcessFederationService service(
          registry, composedProcessDefinition(), ProcessFederationServiceOptions{});
      auto senderConnection = listener->accept(
          nullptr,
          {"public-process-regional-unsubscribe-server", 0x9E01U},
          [](std::wstring) {},
          [](std::wstring) { return false; });
      ProcessTransportSession sender(senderConnection);
      auto senderHandler = service.handlerFor(sender);
      auto receiverConnection = std::shared_ptr<ProcessTransportConnection>{};
      auto serveExpected = [&](ProcessTransportSession& session,
                               auto const& handler,
                               TransportServiceOperation operation,
                               char const* description) {
        if (!ProcessTransportServiceDispatcher::serveOne(
                session,
                [&](TransportServiceMessage const& request) {
                  if (request.operation != operation) {
                    throw std::runtime_error(description);
                  }
                  return handler(request);
                })) {
          throw std::runtime_error(description);
        }
      };

      auto senderRecordingHandler = [&](TransportServiceMessage const& request) {
        auto response = senderHandler(request);
        if (request.operation == TransportServiceOperation::join_federation_execution &&
            response.status == TransportServiceStatus::ok) {
          auto const join = umbra::detail::decodeProcessFederationJoinResult(
              response.payload);
          expectedSenderFederate.store(join.federateId, std::memory_order_release);
        }
        if (request.operation == TransportServiceOperation::register_object_instance_with_regions &&
            response.status == TransportServiceStatus::ok) {
          auto const registration =
              umbra::detail::decodeProcessFederationRegisterObjectInstanceResult(
                  response.payload);
          expectedObjectInstance.store(
              registration.objectInstanceHandle, std::memory_order_release);
        }
        if (request.operation == TransportServiceOperation::update_attribute_values &&
            response.status == TransportServiceStatus::ok) {
          recipientCount.store(
              umbra::detail::decodeProcessFederationUpdateAttributeValuesResult(
                  response.payload)
                  .recipientCount,
              std::memory_order_release);
        }
        return response;
      };

      serveExpected(
          sender,
          senderRecordingHandler,
          TransportServiceOperation::create_federation_execution,
          "The public regional-unsubscribe server lost Create.");
      auto const objectClass = registry.objectClassHandleFor(
          federationName, objectClassName);
      auto const attribute = registry.attributeHandleFor(
          federationName, objectClassName, attributeName);
      auto const dimension = registry.dimensionHandleFor(
          federationName, dimensionName);
      if (!objectClass || !attribute || !dimension) {
        throw std::runtime_error(
            "The public regional-unsubscribe server could not resolve its FOM handles.");
      }
      expectedObjectClass.store(*objectClass, std::memory_order_release);
      expectedAttribute.store(*attribute, std::memory_order_release);
      expectedDimension.store(*dimension, std::memory_order_release);
      serveExpected(
          sender,
          senderRecordingHandler,
          TransportServiceOperation::join_federation_execution,
          "The public regional-unsubscribe server lost sender Join.");

      receiverConnection = listener->accept(
          nullptr,
          {"public-process-regional-unsubscribe-server", 0x9E02U},
          [](std::wstring) {},
          [](std::wstring) { return false; });
      ProcessTransportSession receiver(receiverConnection);
      auto receiverHandler = service.handlerFor(receiver);
      serveExpected(
          receiver,
          receiverHandler,
          TransportServiceOperation::join_federation_execution,
          "The public regional-unsubscribe server lost receiver Join.");
      serveExpected(
          receiver,
          receiverHandler,
          TransportServiceOperation::set_attribute_scope_advisory_switch,
          "The public regional-unsubscribe server lost scope-switch Set.");
      serveExpected(
          receiver,
          receiverHandler,
          TransportServiceOperation::get_attribute_scope_advisory_switch,
          "The public regional-unsubscribe server lost scope-switch Get.");

      for (auto const operation : {
               TransportServiceOperation::get_object_class_handle,
               TransportServiceOperation::get_attribute_handle,
               TransportServiceOperation::get_dimension_handle,
               TransportServiceOperation::create_region,
               TransportServiceOperation::set_range_bounds,
               TransportServiceOperation::commit_region_modifications,
               TransportServiceOperation::create_region,
               TransportServiceOperation::set_range_bounds,
               TransportServiceOperation::commit_region_modifications,
           }) {
        serveExpected(
            sender,
            senderRecordingHandler,
            operation,
            "The public regional-unsubscribe server lost sender DDM setup.");
      }
      for (auto const operation : {
               TransportServiceOperation::get_object_class_handle,
               TransportServiceOperation::get_attribute_handle,
               TransportServiceOperation::get_dimension_handle,
               TransportServiceOperation::create_region,
               TransportServiceOperation::set_range_bounds,
               TransportServiceOperation::commit_region_modifications,
               TransportServiceOperation::subscribe_object_class_attributes_with_regions,
           }) {
        serveExpected(
            receiver,
            receiverHandler,
            operation,
            "The public regional-unsubscribe server lost receiver DDM setup.");
      }
      serveExpected(
          sender,
          senderRecordingHandler,
          TransportServiceOperation::publish_object_class_attributes,
          "The public regional-unsubscribe server lost Publish.");
      serveExpected(
          sender,
          senderRecordingHandler,
          TransportServiceOperation::register_object_instance_with_regions,
          "The public regional-unsubscribe server lost regional Register.");
      serveExpected(
          receiver,
          receiverHandler,
          TransportServiceOperation::receive_interaction,
          "The public regional-unsubscribe server lost discovery Receive.");
      serveExpected(
          receiver,
          receiverHandler,
          TransportServiceOperation::unsubscribe_object_class_attributes_with_regions,
          "The public regional-unsubscribe server lost regional Unsubscribe.");
      serveExpected(
          receiver,
          receiverHandler,
          TransportServiceOperation::receive_interaction,
          "The public regional-unsubscribe server lost scope Receive.");
      serveExpected(
          sender,
          senderRecordingHandler,
          TransportServiceOperation::update_attribute_values,
          "The public regional-unsubscribe server lost regional Update.");
      serveExpected(
          receiver,
          receiverHandler,
          TransportServiceOperation::subscribe_object_class_attributes_with_regions,
          "The public regional-unsubscribe server lost regional re-subscribe.");
      serveExpected(
          receiver,
          receiverHandler,
          TransportServiceOperation::receive_interaction,
          "The public regional-unsubscribe server lost in-scope Receive.");
      serveExpected(
          sender,
          senderRecordingHandler,
          TransportServiceOperation::update_attribute_values,
          "The public regional-unsubscribe server lost re-enabled regional Update.");
      serveExpected(
          receiver,
          receiverHandler,
          TransportServiceOperation::receive_interaction,
          "The public regional-unsubscribe server lost reflected Update Receive.");
      serveExpected(
          sender,
          senderRecordingHandler,
          TransportServiceOperation::associate_regions_for_updates,
          "The public regional-unsubscribe server lost disjoint-region Associate.");
      serveExpected(
          sender,
          senderRecordingHandler,
          TransportServiceOperation::unassociate_regions_for_updates,
          "The public regional-unsubscribe server lost source-region Unassociate.");
      serveExpected(
          receiver,
          receiverHandler,
          TransportServiceOperation::receive_interaction,
          "The public regional-unsubscribe server lost association out-of-scope Receive.");
      serveExpected(
          sender,
          senderRecordingHandler,
          TransportServiceOperation::associate_regions_for_updates,
          "The public regional-unsubscribe server lost source-region Associate.");
      serveExpected(
          receiver,
          receiverHandler,
          TransportServiceOperation::receive_interaction,
          "The public regional-unsubscribe server lost association in-scope Receive.");
      serveExpected(
          sender,
          senderRecordingHandler,
          TransportServiceOperation::resign_federation_execution,
          "The public regional-unsubscribe server lost sender Resign.");
      serveExpected(
          receiver,
          receiverHandler,
          TransportServiceOperation::resign_federation_execution,
          "The public regional-unsubscribe server lost receiver Resign.");
      service.detach(sender);
      service.detach(receiver);
      senderConnection->close();
      receiverConnection->close();
    } catch (...) {
      serverError = std::current_exception();
    }
  });

  TestFederateAmbassador senderFederate;
  auto senderRti = makeRti();
  auto receiverRti = makeRti();
  auto configuration = RtiConfiguration::createConfiguration()
                           .withConfigurationName(
                               L"public-process-regional-unsubscribe-client")
                           .withRtiAddress(
                               L"tcp://127.0.0.1:" + std::to_wstring(port));
  std::exception_ptr clientError;
  bool senderJoined = false;
  bool receiverJoined = false;
  try {
    REQUIRE(senderRti->connect(senderFederate, HLA_EVOKED, configuration).addressUsed);
    senderRti->createFederationExecution(federationName, L"server-owned-fom.xml");
    senderRti->joinFederationExecution(
        L"public-process-regional-unsubscribe-sender",
        L"public-process-regional-unsubscribe-type",
        federationName);
    senderJoined = true;

    REQUIRE(receiverRti->connect(receiverFederate, HLA_EVOKED, configuration).addressUsed);
    receiverRti->joinFederationExecution(
        L"public-process-regional-unsubscribe-receiver",
        L"public-process-regional-unsubscribe-type",
        federationName);
    receiverJoined = true;
    REQUIRE_NOTHROW(receiverRti->setAttributeScopeAdvisorySwitch(true));
    REQUIRE(receiverRti->getAttributeScopeAdvisorySwitch());

    auto const senderObjectClass = senderRti->getObjectClassHandle(objectClassNameWide);
    auto const senderAttribute = senderRti->getAttributeHandle(
        senderObjectClass, attributeNameWide);
    auto const senderDimension = senderRti->getDimensionHandle(dimensionNameWide);
    REQUIRE(senderObjectClass.toString() ==
            L"ObjectClassHandle(" +
                std::to_wstring(expectedObjectClass.load(std::memory_order_acquire)) +
                L")");
    REQUIRE(senderAttribute == rti1516_2025::umbra_binding_detail::makeAttributeHandle(
                                  expectedAttribute.load(std::memory_order_acquire)));
    REQUIRE(senderDimension.toString() ==
            L"DimensionHandle(" +
                std::to_wstring(expectedDimension.load(std::memory_order_acquire)) +
                L")");
    auto const senderRegion = senderRti->createRegion(
        rti1516_2025::DimensionHandleSet{senderDimension});
    REQUIRE(senderRegion.isValid());
    REQUIRE_NOTHROW(senderRti->setRangeBounds(
        senderRegion, senderDimension, rti1516_2025::RangeBounds(0UL, 1UL)));
    REQUIRE_NOTHROW(senderRti->commitRegionModifications(
        rti1516_2025::RegionHandleSet{senderRegion}));

    auto const senderDisjointRegion = senderRti->createRegion(
        rti1516_2025::DimensionHandleSet{senderDimension});
    REQUIRE(senderDisjointRegion.isValid());
    REQUIRE_NOTHROW(senderRti->setRangeBounds(
        senderDisjointRegion,
        senderDimension,
        rti1516_2025::RangeBounds(2UL, 3UL)));
    REQUIRE_NOTHROW(senderRti->commitRegionModifications(
        rti1516_2025::RegionHandleSet{senderDisjointRegion}));

    auto const receiverObjectClass = receiverRti->getObjectClassHandle(objectClassNameWide);
    auto const receiverAttribute = receiverRti->getAttributeHandle(
        receiverObjectClass, attributeNameWide);
    auto const receiverDimension = receiverRti->getDimensionHandle(dimensionNameWide);
    auto const receiverRegion = receiverRti->createRegion(
        rti1516_2025::DimensionHandleSet{receiverDimension});
    REQUIRE(receiverRegion.isValid());
    REQUIRE_NOTHROW(receiverRti->setRangeBounds(
        receiverRegion, receiverDimension, rti1516_2025::RangeBounds(0UL, 1UL)));
    REQUIRE_NOTHROW(receiverRti->commitRegionModifications(
        rti1516_2025::RegionHandleSet{receiverRegion}));
    rti1516_2025::AttributeHandleSet const receiverAttributes{receiverAttribute};
    rti1516_2025::AttributeHandleSetRegionHandleSetPairVector const receiverPairs{{
        receiverAttributes,
        rti1516_2025::RegionHandleSet{receiverRegion},
    }};
    REQUIRE_NOTHROW(receiverRti->subscribeObjectClassAttributesWithRegions(
        receiverObjectClass,
        receiverPairs));

    rti1516_2025::AttributeHandleSet const senderAttributes{senderAttribute};
    REQUIRE_NOTHROW(senderRti->publishObjectClassAttributes(
        senderObjectClass, senderAttributes));
    auto const objectInstance = senderRti->registerObjectInstanceWithRegions(
        senderObjectClass,
        rti1516_2025::AttributeHandleSetRegionHandleSetPairVector{{
            senderAttributes,
            rti1516_2025::RegionHandleSet{senderRegion},
        }});
    REQUIRE(objectInstance.isValid());
    static_cast<void>(receiverRti->evokeCallback(0.0));
    REQUIRE(receiverFederate.discovered);
    REQUIRE(receiverFederate.discoveredObjectInstance == objectInstance);

    REQUIRE_NOTHROW(receiverRti->unsubscribeObjectClassAttributesWithRegions(
        receiverObjectClass,
        receiverPairs));
    static_cast<void>(receiverRti->evokeCallback(0.0));
    REQUIRE(receiverFederate.outOfScope);
    REQUIRE(receiverFederate.outOfScopeCount == 1U);
    REQUIRE(receiverFederate.outOfScopeObjectInstance == objectInstance);
    REQUIRE(receiverFederate.outOfScopeAttributeCount == 1U);

    std::array<std::uint8_t, 3U> encodedValue{0x55U, 0x4EU, 0x53U};
    std::array<std::uint8_t, 3U> encodedTag{0x55U, 0x4EU, 0x53U};
    rti1516_2025::AttributeHandleValueMap attributeValues;
    attributeValues.emplace(
        senderAttribute,
        rti1516_2025::VariableLengthData(encodedValue.data(), encodedValue.size()));
    rti1516_2025::VariableLengthData userSuppliedTag(
        encodedTag.data(), encodedTag.size());
    REQUIRE_NOTHROW(senderRti->updateAttributeValues(
        objectInstance, attributeValues, userSuppliedTag));
    REQUIRE_FALSE(receiverFederate.reflected);
    REQUIRE(recipientCount.load(std::memory_order_acquire) == 0U);
    REQUIRE(expectedObjectInstance.load(std::memory_order_acquire) != 0U);
    REQUIRE(expectedSenderFederate.load(std::memory_order_acquire) != 0U);

    REQUIRE_FALSE(receiverFederate.inScope);
    REQUIRE_NOTHROW(receiverRti->subscribeObjectClassAttributesWithRegions(
        receiverObjectClass,
        receiverPairs));
    static_cast<void>(receiverRti->evokeCallback(0.0));
    REQUIRE(receiverFederate.inScope);
    REQUIRE(receiverFederate.inScopeCount == 1U);
    REQUIRE(receiverFederate.inScopeObjectInstance == objectInstance);
    REQUIRE(receiverFederate.inScopeAttributeCount == 1U);

    std::array<std::uint8_t, 3U> reenabledValue{0x52U, 0x45U, 0x45U};
    rti1516_2025::AttributeHandleValueMap reenabledValues;
    reenabledValues.emplace(
        senderAttribute,
        rti1516_2025::VariableLengthData(
            reenabledValue.data(), reenabledValue.size()));
    REQUIRE_NOTHROW(senderRti->updateAttributeValues(
        objectInstance, reenabledValues, userSuppliedTag));
    static_cast<void>(receiverRti->evokeCallback(0.0));
    REQUIRE(receiverFederate.reflected);
    REQUIRE(receiverFederate.reflectedObjectInstance == objectInstance);
    REQUIRE(receiverFederate.reflectedAttributeCount == 1U);
    REQUIRE(recipientCount.load(std::memory_order_acquire) == 1U);

    REQUIRE_NOTHROW(senderRti->associateRegionsForUpdates(
        objectInstance,
        rti1516_2025::AttributeHandleSetRegionHandleSetPairVector{{
            rti1516_2025::AttributeHandleSet{senderAttribute},
            rti1516_2025::RegionHandleSet{senderDisjointRegion},
        }}));

    REQUIRE_NOTHROW(senderRti->unassociateRegionsForUpdates(
        objectInstance,
        rti1516_2025::AttributeHandleSetRegionHandleSetPairVector{{
            rti1516_2025::AttributeHandleSet{senderAttribute},
            rti1516_2025::RegionHandleSet{senderRegion},
        }}));
    static_cast<void>(receiverRti->evokeCallback(0.0));
    REQUIRE(receiverFederate.outOfScopeCount == 2U);
    REQUIRE(receiverFederate.outOfScopeObjectInstance == objectInstance);
    REQUIRE(receiverFederate.outOfScopeAttributeCount == 1U);

    REQUIRE_NOTHROW(senderRti->associateRegionsForUpdates(
        objectInstance,
        rti1516_2025::AttributeHandleSetRegionHandleSetPairVector{{
            rti1516_2025::AttributeHandleSet{senderAttribute},
            rti1516_2025::RegionHandleSet{senderRegion},
        }}));
    static_cast<void>(receiverRti->evokeCallback(0.0));
    REQUIRE(receiverFederate.inScopeCount == 2U);
    REQUIRE(receiverFederate.inScopeObjectInstance == objectInstance);
    REQUIRE(receiverFederate.inScopeAttributeCount == 1U);

    senderRti->resignFederationExecution(DELETE_OBJECTS);
    senderJoined = false;
    receiverRti->resignFederationExecution(NO_ACTION);
    receiverJoined = false;
    senderRti->disconnect();
    receiverRti->disconnect();
  } catch (...) {
    clientError = std::current_exception();
    if (senderJoined) {
      try {
        senderRti->resignFederationExecution(DELETE_OBJECTS);
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
  if (listener) {
    listener.reset();
  }
  if (server.joinable()) {
    server.join();
  }
  if (clientError) {
    std::rethrow_exception(clientError);
  }
  REQUIRE_FALSE(serverError);
  REQUIRE_FALSE(senderJoined);
  REQUIRE_FALSE(receiverJoined);
}

TEST_CASE(
    "RTIambassador suppresses a disjoint regional update through a configured process endpoint",
    "[integration][foundation][data-distribution-management][object-management][callbacks][transport][process-boundary][public-endpoint][rti.service.subscribe-object-class-attributes-with-regions][rti.service.register-object-instance-with-regions][rti.service.update-attribute-values][federate.callback.discover-object-instance][federate.callback.reflect-attribute-values]") {
  class RecordingRegionalFederateAmbassador final : public NullFederateAmbassador {
   public:
    void discoverObjectInstance(
        rti1516_2025::ObjectInstanceHandle const&,
        rti1516_2025::ObjectClassHandle const&,
        std::wstring const&,
        rti1516_2025::FederateHandle const&) override {
      discovered = true;
    }

    void reflectAttributeValues(
        rti1516_2025::ObjectInstanceHandle const&,
        rti1516_2025::AttributeHandleValueMap const&,
        rti1516_2025::VariableLengthData const&,
        rti1516_2025::TransportationTypeHandle const&,
        rti1516_2025::FederateHandle const&,
        rti1516_2025::RegionHandleSet const*) override {
      reflected = true;
    }

    bool discovered = false;
    bool reflected = false;
  } receiverFederate;

  auto listener = ProcessTransportListener::listen({"127.0.0.1", 0U});
  REQUIRE(listener);
  auto const port = listener->address().port;
  REQUIRE(port != 0U);

  constexpr wchar_t const* federationName =
      L"public-process-regional-disjoint-execution";
  constexpr char const* objectClassName = "HLAobjectRoot.Food.Drink.Soda";
  constexpr wchar_t const* objectClassNameWide =
      L"HLAobjectRoot.Food.Drink.Soda";
  constexpr char const* attributeName = "Flavor";
  constexpr wchar_t const* attributeNameWide = L"Flavor";
  constexpr char const* dimensionName = "SodaFlavor";
  constexpr wchar_t const* dimensionNameWide = L"SodaFlavor";
  std::atomic_uint64_t expectedObjectClass{0U};
  std::atomic_uint64_t expectedAttribute{0U};
  std::atomic_uint64_t expectedDimension{0U};
  std::atomic_uint64_t expectedObjectInstance{0U};
  std::atomic_uint64_t expectedSenderFederate{0U};
  std::atomic_uint64_t recipientCount{std::numeric_limits<std::uint32_t>::max()};
  std::exception_ptr serverError;
  std::thread server([&] {
    try {
      EmbeddedFederationRegistry registry;
      ProcessFederationService service(
          registry, composedProcessDefinition(), ProcessFederationServiceOptions{});
      auto senderConnection = listener->accept(
          nullptr,
          {"public-process-regional-disjoint-server", 0x9F01U},
          [](std::wstring) {},
          [](std::wstring) { return false; });
      ProcessTransportSession sender(senderConnection);
      auto senderHandler = service.handlerFor(sender);
      auto receiverConnection = std::shared_ptr<ProcessTransportConnection>{};
      auto serveExpected = [&](ProcessTransportSession& session,
                               auto const& handler,
                               TransportServiceOperation operation,
                               char const* description) {
        if (!ProcessTransportServiceDispatcher::serveOne(
                session,
                [&](TransportServiceMessage const& request) {
                  if (request.operation != operation) {
                    throw std::runtime_error(description);
                  }
                  return handler(request);
                })) {
          throw std::runtime_error(description);
        }
      };

      auto senderRecordingHandler = [&](TransportServiceMessage const& request) {
        auto response = senderHandler(request);
        if (request.operation == TransportServiceOperation::join_federation_execution &&
            response.status == TransportServiceStatus::ok) {
          auto const join = umbra::detail::decodeProcessFederationJoinResult(
              response.payload);
          expectedSenderFederate.store(join.federateId, std::memory_order_release);
        }
        if (request.operation == TransportServiceOperation::register_object_instance_with_regions &&
            response.status == TransportServiceStatus::ok) {
          auto const registration =
              umbra::detail::decodeProcessFederationRegisterObjectInstanceResult(
                  response.payload);
          expectedObjectInstance.store(
              registration.objectInstanceHandle, std::memory_order_release);
        }
        if (request.operation == TransportServiceOperation::update_attribute_values &&
            response.status == TransportServiceStatus::ok) {
          recipientCount.store(
              umbra::detail::decodeProcessFederationUpdateAttributeValuesResult(
                  response.payload)
                  .recipientCount,
              std::memory_order_release);
        }
        return response;
      };

      serveExpected(
          sender,
          senderRecordingHandler,
          TransportServiceOperation::create_federation_execution,
          "The public regional-disjoint server lost Create.");
      auto const objectClass = registry.objectClassHandleFor(
          federationName, objectClassName);
      auto const attribute = registry.attributeHandleFor(
          federationName, objectClassName, attributeName);
      auto const dimension = registry.dimensionHandleFor(
          federationName, dimensionName);
      if (!objectClass || !attribute || !dimension) {
        throw std::runtime_error(
            "The public regional-disjoint server could not resolve its FOM handles.");
      }
      expectedObjectClass.store(*objectClass, std::memory_order_release);
      expectedAttribute.store(*attribute, std::memory_order_release);
      expectedDimension.store(*dimension, std::memory_order_release);
      serveExpected(
          sender,
          senderRecordingHandler,
          TransportServiceOperation::join_federation_execution,
          "The public regional-disjoint server lost sender Join.");

      receiverConnection = listener->accept(
          nullptr,
          {"public-process-regional-disjoint-server", 0x9F02U},
          [](std::wstring) {},
          [](std::wstring) { return false; });
      ProcessTransportSession receiver(receiverConnection);
      auto receiverHandler = service.handlerFor(receiver);
      serveExpected(
          receiver,
          receiverHandler,
          TransportServiceOperation::join_federation_execution,
          "The public regional-disjoint server lost receiver Join.");

      for (auto const operation : {
               TransportServiceOperation::get_object_class_handle,
               TransportServiceOperation::get_attribute_handle,
               TransportServiceOperation::get_dimension_handle,
               TransportServiceOperation::create_region,
               TransportServiceOperation::set_range_bounds,
               TransportServiceOperation::commit_region_modifications,
           }) {
        serveExpected(
            sender,
            senderRecordingHandler,
            operation,
            "The public regional-disjoint server lost sender DDM setup.");
      }
      for (auto const operation : {
               TransportServiceOperation::get_object_class_handle,
               TransportServiceOperation::get_attribute_handle,
               TransportServiceOperation::get_dimension_handle,
               TransportServiceOperation::create_region,
               TransportServiceOperation::set_range_bounds,
               TransportServiceOperation::commit_region_modifications,
               TransportServiceOperation::subscribe_object_class_attributes_with_regions,
           }) {
        serveExpected(
            receiver,
            receiverHandler,
            operation,
            "The public regional-disjoint server lost receiver DDM setup.");
      }
      serveExpected(
          sender,
          senderRecordingHandler,
          TransportServiceOperation::publish_object_class_attributes,
          "The public regional-disjoint server lost Publish.");
      serveExpected(
          sender,
          senderRecordingHandler,
          TransportServiceOperation::register_object_instance_with_regions,
          "The public regional-disjoint server lost regional Register.");
      serveExpected(
          sender,
          senderRecordingHandler,
          TransportServiceOperation::update_attribute_values,
          "The public regional-disjoint server lost regional Update.");
      serveExpected(
          sender,
          senderRecordingHandler,
          TransportServiceOperation::resign_federation_execution,
          "The public regional-disjoint server lost sender Resign.");
      serveExpected(
          receiver,
          receiverHandler,
          TransportServiceOperation::resign_federation_execution,
          "The public regional-disjoint server lost receiver Resign.");
      service.detach(sender);
      service.detach(receiver);
      senderConnection->close();
      receiverConnection->close();
    } catch (...) {
      serverError = std::current_exception();
    }
  });

  TestFederateAmbassador senderFederate;
  auto senderRti = makeRti();
  auto receiverRti = makeRti();
  auto configuration = RtiConfiguration::createConfiguration()
                           .withConfigurationName(
                               L"public-process-regional-disjoint-client")
                           .withRtiAddress(
                               L"tcp://127.0.0.1:" + std::to_wstring(port));
  std::exception_ptr clientError;
  bool senderJoined = false;
  bool receiverJoined = false;
  try {
    REQUIRE(senderRti->connect(senderFederate, HLA_EVOKED, configuration).addressUsed);
    senderRti->createFederationExecution(federationName, L"server-owned-fom.xml");
    senderRti->joinFederationExecution(
        L"public-process-regional-disjoint-sender",
        L"public-process-regional-disjoint-type",
        federationName);
    senderJoined = true;

    REQUIRE(receiverRti->connect(receiverFederate, HLA_EVOKED, configuration).addressUsed);
    receiverRti->joinFederationExecution(
        L"public-process-regional-disjoint-receiver",
        L"public-process-regional-disjoint-type",
        federationName);
    receiverJoined = true;

    auto const senderObjectClass = senderRti->getObjectClassHandle(objectClassNameWide);
    auto const senderAttribute = senderRti->getAttributeHandle(
        senderObjectClass, attributeNameWide);
    auto const senderDimension = senderRti->getDimensionHandle(dimensionNameWide);
    REQUIRE(senderObjectClass.toString() ==
            L"ObjectClassHandle(" +
                std::to_wstring(expectedObjectClass.load(std::memory_order_acquire)) +
                L")");
    REQUIRE(senderAttribute == rti1516_2025::umbra_binding_detail::makeAttributeHandle(
                                  expectedAttribute.load(std::memory_order_acquire)));
    REQUIRE(senderDimension.toString() ==
            L"DimensionHandle(" +
                std::to_wstring(expectedDimension.load(std::memory_order_acquire)) +
                L")");
    auto const senderRegion = senderRti->createRegion(
        rti1516_2025::DimensionHandleSet{senderDimension});
    REQUIRE(senderRegion.isValid());
    REQUIRE_NOTHROW(senderRti->setRangeBounds(
        senderRegion, senderDimension, rti1516_2025::RangeBounds(0UL, 1UL)));
    REQUIRE_NOTHROW(senderRti->commitRegionModifications(
        rti1516_2025::RegionHandleSet{senderRegion}));

    auto const receiverObjectClass = receiverRti->getObjectClassHandle(objectClassNameWide);
    auto const receiverAttribute = receiverRti->getAttributeHandle(
        receiverObjectClass, attributeNameWide);
    auto const receiverDimension = receiverRti->getDimensionHandle(dimensionNameWide);
    auto const receiverRegion = receiverRti->createRegion(
        rti1516_2025::DimensionHandleSet{receiverDimension});
    REQUIRE(receiverRegion.isValid());
    REQUIRE_NOTHROW(receiverRti->setRangeBounds(
        receiverRegion, receiverDimension, rti1516_2025::RangeBounds(2UL, 3UL)));
    REQUIRE_NOTHROW(receiverRti->commitRegionModifications(
        rti1516_2025::RegionHandleSet{receiverRegion}));
    rti1516_2025::AttributeHandleSet const receiverAttributes{receiverAttribute};
    rti1516_2025::AttributeHandleSetRegionHandleSetPairVector const receiverPairs{{
        receiverAttributes,
        rti1516_2025::RegionHandleSet{receiverRegion},
    }};
    REQUIRE_NOTHROW(receiverRti->subscribeObjectClassAttributesWithRegions(
        receiverObjectClass,
        receiverPairs));

    rti1516_2025::AttributeHandleSet const senderAttributes{senderAttribute};
    REQUIRE_NOTHROW(senderRti->publishObjectClassAttributes(
        senderObjectClass, senderAttributes));
    auto const objectInstance = senderRti->registerObjectInstanceWithRegions(
        senderObjectClass,
        rti1516_2025::AttributeHandleSetRegionHandleSetPairVector{{
            senderAttributes,
            rti1516_2025::RegionHandleSet{senderRegion},
        }});
    REQUIRE(objectInstance.isValid());

    std::array<std::uint8_t, 3U> encodedValue{0x44U, 0x49U, 0x53U};
    std::array<std::uint8_t, 3U> encodedTag{0x44U, 0x44U, 0x4DU};
    rti1516_2025::AttributeHandleValueMap attributeValues;
    attributeValues.emplace(
        senderAttribute,
        rti1516_2025::VariableLengthData(encodedValue.data(), encodedValue.size()));
    rti1516_2025::VariableLengthData userSuppliedTag(
        encodedTag.data(), encodedTag.size());
    REQUIRE_NOTHROW(senderRti->updateAttributeValues(
        objectInstance, attributeValues, userSuppliedTag));
    REQUIRE_FALSE(receiverFederate.discovered);
    REQUIRE_FALSE(receiverFederate.reflected);
    REQUIRE(recipientCount.load(std::memory_order_acquire) == 0U);
    REQUIRE(expectedObjectInstance.load(std::memory_order_acquire) != 0U);
    REQUIRE(expectedSenderFederate.load(std::memory_order_acquire) != 0U);

    senderRti->resignFederationExecution(DELETE_OBJECTS);
    senderJoined = false;
    receiverRti->resignFederationExecution(NO_ACTION);
    receiverJoined = false;
    senderRti->disconnect();
    receiverRti->disconnect();
  } catch (...) {
    clientError = std::current_exception();
    if (senderJoined) {
      try {
        senderRti->resignFederationExecution(DELETE_OBJECTS);
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
  if (listener) {
    listener.reset();
  }
  if (server.joinable()) {
    server.join();
  }
  if (clientError) {
    std::rethrow_exception(clientError);
  }
  REQUIRE_FALSE(serverError);
  REQUIRE_FALSE(senderJoined);
  REQUIRE_FALSE(receiverJoined);
}
#endif

#if defined(UMBRA_ENABLE_EMBEDDED_FEDERATION_MANAGEMENT)
TEST_CASE(
    "RTIambassador preserves an explicit regional Attribute Relevance Advisory update-rate designator across configured process endpoint transitions",
    "[integration][foundation][data-distribution-management][object-management][callbacks][transport][process-boundary][public-endpoint][regional-attribute-relevance][regional-update-rate-designator][2025][rti.service.get-attribute-relevance-advisory-switch][rti.service.set-attribute-relevance-advisory-switch][rti.service.create-region][rti.service.commit-region-modifications][rti.service.subscribe-object-class-attributes-with-regions][rti.service.register-object-instance-with-regions][rti.service.associate-regions-for-updates][rti.service.unassociate-regions-for-updates][federate.callback.discover-object-instance][federate.callback.turn-updates-on-for-object-instance][federate.callback.turn-updates-off-for-object-instance]") {
  auto runScenario = [](CallbackModel callbackModel) {
    class RecordingOwnerAmbassador final : public NullFederateAmbassador {
     public:
      void turnUpdatesOnForObjectInstance(
          rti1516_2025::ObjectInstanceHandle const& objectInstance,
          rti1516_2025::AttributeHandleSet const& attributes) override {
        ++turnedOnCount;
        turnedOnObjectInstance = objectInstance;
        turnedOnAttributes = attributes;
        turnedOnUpdateRateDesignator.reset();
      }

      void turnUpdatesOnForObjectInstance(
          rti1516_2025::ObjectInstanceHandle const& objectInstance,
          rti1516_2025::AttributeHandleSet const& attributes,
          std::wstring const& updateRateDesignator) override {
        ++turnedOnCount;
        turnedOnObjectInstance = objectInstance;
        turnedOnAttributes = attributes;
        turnedOnUpdateRateDesignator = updateRateDesignator;
      }

      void turnUpdatesOffForObjectInstance(
          rti1516_2025::ObjectInstanceHandle const& objectInstance,
          rti1516_2025::AttributeHandleSet const& attributes) override {
        ++turnedOffCount;
        turnedOffObjectInstance = objectInstance;
        turnedOffAttributes = attributes;
      }

      void clear() {
        turnedOnCount = 0U;
        turnedOnObjectInstance = {};
        turnedOnAttributes.clear();
        turnedOnUpdateRateDesignator.reset();
        turnedOffCount = 0U;
        turnedOffObjectInstance = {};
        turnedOffAttributes.clear();
      }

      std::size_t turnedOnCount = 0U;
      rti1516_2025::ObjectInstanceHandle turnedOnObjectInstance;
      rti1516_2025::AttributeHandleSet turnedOnAttributes;
      std::optional<std::wstring> turnedOnUpdateRateDesignator;
      std::size_t turnedOffCount = 0U;
      rti1516_2025::ObjectInstanceHandle turnedOffObjectInstance;
      rti1516_2025::AttributeHandleSet turnedOffAttributes;
    } ownerFederate;

    class RecordingReceiverAmbassador final : public NullFederateAmbassador {
     public:
      void discoverObjectInstance(
          rti1516_2025::ObjectInstanceHandle const& objectInstance,
          rti1516_2025::ObjectClassHandle const& objectClass,
          std::wstring const& objectInstanceName,
          rti1516_2025::FederateHandle const& producingFederate) override {
        discovered = true;
        discoveredObjectInstance = objectInstance;
        discoveredObjectClass = objectClass;
        discoveredObjectInstanceName = objectInstanceName;
        discoveredProducingFederate = producingFederate;
      }

      bool discovered = false;
      rti1516_2025::ObjectInstanceHandle discoveredObjectInstance;
      rti1516_2025::ObjectClassHandle discoveredObjectClass;
      std::wstring discoveredObjectInstanceName;
      rti1516_2025::FederateHandle discoveredProducingFederate;
    } receiverFederate;

    auto listener = ProcessTransportListener::listen({"127.0.0.1", 0U});
    REQUIRE(listener);
    auto const port = listener->address().port;
    REQUIRE(port != 0U);

    constexpr wchar_t const* federationName =
        L"public-process-regional-attribute-relevance-rate-designator-execution";
    constexpr wchar_t const* objectClassName =
        L"HLAobjectRoot.Food.Drink.Soda";
    constexpr wchar_t const* attributeName = L"Flavor";
    constexpr wchar_t const* dimensionName = L"SodaFlavor";
    std::exception_ptr serverError;
    std::thread server([&] {
      try {
        EmbeddedFederationRegistry registry;
        ProcessFederationService service(
            registry,
            composedProcessDefinition(),
            ProcessFederationServiceOptions{true});
        auto ownerConnection = listener->accept(
            nullptr,
            {"public-process-regional-attribute-relevance-rate-designator-owner", 0xAA01U},
            [](std::wstring) {},
            [](std::wstring) { return false; });
        ProcessTransportSession owner(ownerConnection);
        auto ownerHandler = service.handlerFor(owner);
        auto serveExpected = [&](ProcessTransportSession& session,
                                 auto const& handler,
                                 TransportServiceOperation operation,
                                 char const* description) {
          if (!ProcessTransportServiceDispatcher::serveOne(
                  session,
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
            owner,
            ownerHandler,
            TransportServiceOperation::create_federation_execution,
            "The regional rate-designator server lost Create.");
        serveExpected(
            owner,
            ownerHandler,
            TransportServiceOperation::join_federation_execution,
            "The regional rate-designator server lost owner Join.");
        auto receiverConnection = listener->accept(
            nullptr,
            {"public-process-regional-attribute-relevance-rate-designator-receiver", 0xAA02U},
            [](std::wstring) {},
            [](std::wstring) { return false; });
        ProcessTransportSession receiver(receiverConnection);
        auto receiverHandler = service.handlerFor(receiver);
        serveExpected(
            receiver,
            receiverHandler,
            TransportServiceOperation::join_federation_execution,
            "The regional rate-designator server lost receiver Join.");
        serveExpected(
            owner,
            ownerHandler,
            TransportServiceOperation::set_attribute_relevance_advisory_switch,
            "The regional rate-designator server lost advisory-switch Set.");
        serveExpected(
            owner,
            ownerHandler,
            TransportServiceOperation::get_attribute_relevance_advisory_switch,
            "The regional rate-designator server lost advisory-switch Get.");

        for (auto const operation : {
                 TransportServiceOperation::get_object_class_handle,
                 TransportServiceOperation::get_attribute_handle,
                 TransportServiceOperation::get_dimension_handle,
                 TransportServiceOperation::create_region,
                 TransportServiceOperation::set_range_bounds,
                 TransportServiceOperation::commit_region_modifications,
                 TransportServiceOperation::create_region,
                 TransportServiceOperation::set_range_bounds,
                 TransportServiceOperation::commit_region_modifications,
             }) {
          serveExpected(
              owner,
              ownerHandler,
              operation,
              "The regional rate-designator server lost owner DDM setup.");
        }
        for (auto const operation : {
                 TransportServiceOperation::get_object_class_handle,
                 TransportServiceOperation::get_attribute_handle,
                 TransportServiceOperation::get_dimension_handle,
                 TransportServiceOperation::create_region,
                 TransportServiceOperation::set_range_bounds,
                 TransportServiceOperation::commit_region_modifications,
                 TransportServiceOperation::subscribe_object_class_attributes_with_regions,
             }) {
          serveExpected(
              receiver,
              receiverHandler,
              operation,
              "The regional rate-designator server lost receiver DDM setup.");
        }
        serveExpected(
            owner,
            ownerHandler,
            TransportServiceOperation::publish_object_class_attributes,
            "The regional rate-designator server lost Publish.");
        serveExpected(
            owner,
            ownerHandler,
            TransportServiceOperation::register_object_instance_with_regions,
            "The regional rate-designator server lost regional Register.");
        if (callbackModel == HLA_IMMEDIATE) {
          serveExpected(
              receiver,
              receiverHandler,
              TransportServiceOperation::get_object_class_handle,
              "The regional rate-designator server lost immediate discovery polling.");
        } else {
          serveExpected(
              receiver,
              receiverHandler,
              TransportServiceOperation::receive_interaction,
              "The regional rate-designator server lost discovery Receive.");
        }
        serveExpected(
            owner,
            ownerHandler,
            TransportServiceOperation::associate_regions_for_updates,
            "The regional rate-designator server lost disjoint Associate.");
        serveExpected(
            owner,
            ownerHandler,
            TransportServiceOperation::unassociate_regions_for_updates,
            "The regional rate-designator server lost source Unassociate.");
        serveExpected(
            owner,
            ownerHandler,
            TransportServiceOperation::associate_regions_for_updates,
            "The regional rate-designator server lost source Associate.");
        serveExpected(
            owner,
            ownerHandler,
            TransportServiceOperation::resign_federation_execution,
            "The regional rate-designator server lost owner Resign.");
        serveExpected(
            receiver,
            receiverHandler,
            TransportServiceOperation::resign_federation_execution,
            "The regional rate-designator server lost receiver Resign.");
        service.detach(owner);
        service.detach(receiver);
        ownerConnection->close();
        receiverConnection->close();
      } catch (...) {
        serverError = std::current_exception();
      }
    });

    auto ownerRti = makeRti();
    auto receiverRti = makeRti();
    auto pumpOwner = [&] {
      if (callbackModel == HLA_EVOKED) {
        static_cast<void>(ownerRti->evokeCallback(0.0));
      }
    };
    auto configuration = RtiConfiguration::createConfiguration()
                             .withConfigurationName(
                                 L"public-process-regional-attribute-relevance-rate-designator-client")
                             .withRtiAddress(
                                 L"tcp://127.0.0.1:" + std::to_wstring(port));
    std::exception_ptr clientError;
    bool ownerJoined = false;
    bool receiverJoined = false;
    try {
      REQUIRE(ownerRti->connect(ownerFederate, callbackModel, configuration).addressUsed);
      ownerRti->createFederationExecution(federationName, L"server-owned-fom.xml");
      ownerRti->joinFederationExecution(
          L"public-process-regional-attribute-relevance-rate-designator-owner",
          L"public-process-regional-attribute-relevance-rate-designator-type",
          federationName);
      ownerJoined = true;

      REQUIRE(receiverRti->connect(receiverFederate, callbackModel, configuration).addressUsed);
      receiverRti->joinFederationExecution(
          L"public-process-regional-attribute-relevance-rate-designator-receiver",
          L"public-process-regional-attribute-relevance-rate-designator-type",
          federationName);
      receiverJoined = true;
      REQUIRE_NOTHROW(ownerRti->setAttributeRelevanceAdvisorySwitch(true));
      REQUIRE(ownerRti->getAttributeRelevanceAdvisorySwitch());

      auto const ownerObjectClass = ownerRti->getObjectClassHandle(objectClassName);
      auto const ownerAttribute = ownerRti->getAttributeHandle(
          ownerObjectClass, attributeName);
      auto const ownerDimension = ownerRti->getDimensionHandle(dimensionName);
      REQUIRE(ownerObjectClass.isValid());
      REQUIRE(ownerAttribute.isValid());
      REQUIRE(ownerDimension.isValid());
      auto const ownerRegion = ownerRti->createRegion(
          rti1516_2025::DimensionHandleSet{ownerDimension});
      REQUIRE(ownerRegion.isValid());
      REQUIRE_NOTHROW(ownerRti->setRangeBounds(
          ownerRegion, ownerDimension, rti1516_2025::RangeBounds(0UL, 1UL)));
      REQUIRE_NOTHROW(ownerRti->commitRegionModifications(
          rti1516_2025::RegionHandleSet{ownerRegion}));
      auto const disjointOwnerRegion = ownerRti->createRegion(
          rti1516_2025::DimensionHandleSet{ownerDimension});
      REQUIRE(disjointOwnerRegion.isValid());
      REQUIRE_NOTHROW(ownerRti->setRangeBounds(
          disjointOwnerRegion,
          ownerDimension,
          rti1516_2025::RangeBounds(2UL, 3UL)));
      REQUIRE_NOTHROW(ownerRti->commitRegionModifications(
          rti1516_2025::RegionHandleSet{disjointOwnerRegion}));

      auto const receiverObjectClass = receiverRti->getObjectClassHandle(objectClassName);
      auto const receiverAttribute = receiverRti->getAttributeHandle(
          receiverObjectClass, attributeName);
      auto const receiverDimension = receiverRti->getDimensionHandle(dimensionName);
      REQUIRE(receiverObjectClass == ownerObjectClass);
      REQUIRE(receiverAttribute == ownerAttribute);
      REQUIRE(receiverDimension == ownerDimension);
      auto const receiverRegion = receiverRti->createRegion(
          rti1516_2025::DimensionHandleSet{receiverDimension});
      REQUIRE(receiverRegion.isValid());
      REQUIRE_NOTHROW(receiverRti->setRangeBounds(
          receiverRegion, receiverDimension, rti1516_2025::RangeBounds(0UL, 1UL)));
      REQUIRE_NOTHROW(receiverRti->commitRegionModifications(
          rti1516_2025::RegionHandleSet{receiverRegion}));

      rti1516_2025::AttributeHandleSet const ownerAttributes{ownerAttribute};
      rti1516_2025::AttributeHandleSetRegionHandleSetPairVector const ownerPair{{
          ownerAttributes,
          rti1516_2025::RegionHandleSet{ownerRegion},
      }};
      rti1516_2025::AttributeHandleSetRegionHandleSetPairVector const disjointOwnerPair{{
          ownerAttributes,
          rti1516_2025::RegionHandleSet{disjointOwnerRegion},
      }};
      rti1516_2025::AttributeHandleSetRegionHandleSetPairVector const receiverPair{{
          rti1516_2025::AttributeHandleSet{receiverAttribute},
          rti1516_2025::RegionHandleSet{receiverRegion},
      }};
      REQUIRE_NOTHROW(receiverRti->subscribeObjectClassAttributesWithRegions(
          receiverObjectClass, receiverPair, true, L"High"));
      REQUIRE_NOTHROW(ownerRti->publishObjectClassAttributes(
          ownerObjectClass, ownerAttributes));
      auto const objectInstance = ownerRti->registerObjectInstanceWithRegions(
          ownerObjectClass, ownerPair);
      REQUIRE(objectInstance.isValid());
      pumpOwner();
      if (callbackModel == HLA_IMMEDIATE) {
        static_cast<void>(receiverRti->getObjectClassHandle(objectClassName));
      } else {
        static_cast<void>(receiverRti->evokeCallback(0.0));
      }
      REQUIRE(receiverFederate.discovered);
      REQUIRE(receiverFederate.discoveredObjectInstance == objectInstance);
      REQUIRE(receiverFederate.discoveredObjectClass == receiverObjectClass);
      REQUIRE(ownerFederate.turnedOffCount == 0U);
      REQUIRE(ownerFederate.turnedOnCount == 1U);
      REQUIRE(ownerFederate.turnedOnObjectInstance == objectInstance);
      REQUIRE(ownerFederate.turnedOnAttributes == ownerAttributes);
      REQUIRE(ownerFederate.turnedOnUpdateRateDesignator ==
              std::optional<std::wstring>{L"High"});
      ownerFederate.clear();

      REQUIRE_NOTHROW(ownerRti->associateRegionsForUpdates(
          objectInstance,
          rti1516_2025::AttributeHandleSetRegionHandleSetPairVector{{
              ownerAttributes,
              rti1516_2025::RegionHandleSet{disjointOwnerRegion},
          }}));
      REQUIRE(ownerFederate.turnedOnCount == 0U);
      REQUIRE(ownerFederate.turnedOffCount == 0U);
      ownerFederate.clear();

      REQUIRE_NOTHROW(ownerRti->unassociateRegionsForUpdates(
          objectInstance,
          rti1516_2025::AttributeHandleSetRegionHandleSetPairVector{{
              ownerAttributes,
              rti1516_2025::RegionHandleSet{ownerRegion},
          }}));
      pumpOwner();
      REQUIRE(ownerFederate.turnedOnCount == 0U);
      REQUIRE(ownerFederate.turnedOffCount == 1U);
      REQUIRE(ownerFederate.turnedOffObjectInstance == objectInstance);
      REQUIRE(ownerFederate.turnedOffAttributes == ownerAttributes);
      ownerFederate.clear();

      REQUIRE_NOTHROW(ownerRti->associateRegionsForUpdates(
          objectInstance,
          rti1516_2025::AttributeHandleSetRegionHandleSetPairVector{{
              ownerAttributes,
              rti1516_2025::RegionHandleSet{ownerRegion},
          }}));
      pumpOwner();
      REQUIRE(ownerFederate.turnedOffCount == 0U);
      REQUIRE(ownerFederate.turnedOnCount == 1U);
      REQUIRE(ownerFederate.turnedOnObjectInstance == objectInstance);
      REQUIRE(ownerFederate.turnedOnAttributes == ownerAttributes);
      REQUIRE(ownerFederate.turnedOnUpdateRateDesignator ==
              std::optional<std::wstring>{L"High"});
      ownerFederate.clear();

      ownerRti->resignFederationExecution(DELETE_OBJECTS);
      ownerJoined = false;
      receiverRti->resignFederationExecution(NO_ACTION);
      receiverJoined = false;
      ownerRti->disconnect();
      receiverRti->disconnect();
    } catch (...) {
      clientError = std::current_exception();
      if (ownerJoined) {
        try {
          ownerRti->resignFederationExecution(DELETE_OBJECTS);
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
        ownerRti->disconnect();
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
    if (clientError) {
      std::rethrow_exception(clientError);
    }
    REQUIRE_FALSE(serverError);
    REQUIRE_FALSE(ownerJoined);
    REQUIRE_FALSE(receiverJoined);
  };

  SECTION("HLA_EVOKED") {
    runScenario(HLA_EVOKED);
  }
  SECTION("HLA_IMMEDIATE") {
    runScenario(HLA_IMMEDIATE);
  }
}

TEST_CASE(
    "RTIambassador delivers a directed interaction through the official Evoke callback surface",
    "[integration][foundation][federation-management][interaction-management][object-management]"
    "[directed-interaction][timestamped-directed-interaction][timestamped-directed-retraction][retract][directed-routing][callbacks][callback-controls][callback-gating][callback-immediate][transport][process-boundary][public-endpoint]"
    "[process-event.receive-object-instance-discovery][process-event.receive-directed-interaction][process-event.request-retraction][federate.callback.request-retraction]") {
  auto runScenario = [](CallbackModel callbackModel,
                        bool timestamped,
                        bool gateCallbacks,
                        bool retractBeforeReceive,
                        bool retractAfterReceive = false) {
  class RecordingFederateAmbassador final : public NullFederateAmbassador {
   public:
    void discoverObjectInstance(
        rti1516_2025::ObjectInstanceHandle const& objectInstance,
        rti1516_2025::ObjectClassHandle const& objectClass,
        std::wstring const& objectInstanceName,
        rti1516_2025::FederateHandle const& producingFederate) override {
      discovered = true;
      discoveredObjectInstance = objectInstance;
      discoveredObjectClass = objectClass;
      discoveredObjectInstanceName = objectInstanceName;
      discoveredProducingFederate = producingFederate;
    }

    void receiveDirectedInteraction(
        rti1516_2025::InteractionClassHandle const& interactionClass,
        rti1516_2025::ObjectInstanceHandle const& objectInstance,
        rti1516_2025::ParameterHandleValueMap const& parameterValues,
        rti1516_2025::VariableLengthData const& userSuppliedTag,
        rti1516_2025::TransportationTypeHandle const& transportationType,
        rti1516_2025::FederateHandle const& producingFederate) override {
      received = true;
      receivedInteractionClass = interactionClass;
      receivedObjectInstance = objectInstance;
      receivedParameterCount = parameterValues.size();
      receivedTransportationType = transportationType;
      receivedProducingFederate = producingFederate;
      receivedTag.clear();
      if (userSuppliedTag.size() != 0U) {
        auto const* first =
            static_cast<std::uint8_t const*>(userSuppliedTag.data());
        receivedTag.assign(first, first + userSuppliedTag.size());
      }
    }

    void receiveDirectedInteraction(
        rti1516_2025::InteractionClassHandle const& interactionClass,
        rti1516_2025::ObjectInstanceHandle const& objectInstance,
        rti1516_2025::ParameterHandleValueMap const& parameterValues,
        rti1516_2025::VariableLengthData const& userSuppliedTag,
        rti1516_2025::TransportationTypeHandle const& transportationType,
        rti1516_2025::FederateHandle const& producingFederate,
        rti1516_2025::LogicalTime const& time,
        rti1516_2025::OrderType sentOrder,
        rti1516_2025::OrderType receivedOrder,
        rti1516_2025::MessageRetractionHandle const* optionalRetraction) override {
      receiveDirectedInteraction(
          interactionClass,
          objectInstance,
          parameterValues,
          userSuppliedTag,
          transportationType,
          producingFederate);
      receivedTimestampImplementationName = time.implementationName();
      auto const encoded = time.encode();
      receivedTimestampEncoding.clear();
      if (encoded.size() != 0U) {
        auto const* first = static_cast<std::uint8_t const*>(encoded.data());
        receivedTimestampEncoding.assign(first, first + encoded.size());
      }
      receivedSentOrderType = sentOrder;
      receivedReceivedOrderType = receivedOrder;
      receivedHasOptionalRetraction = optionalRetraction != nullptr;
    }

    void requestRetraction(
        rti1516_2025::MessageRetractionHandle const& retraction) override {
      ++requestRetractionCount;
      requestRetractionHandle = retraction;
    }

    bool discovered = false;
    rti1516_2025::ObjectInstanceHandle discoveredObjectInstance;
    rti1516_2025::ObjectClassHandle discoveredObjectClass;
    std::wstring discoveredObjectInstanceName;
    rti1516_2025::FederateHandle discoveredProducingFederate;
    bool received = false;
    rti1516_2025::InteractionClassHandle receivedInteractionClass;
    rti1516_2025::ObjectInstanceHandle receivedObjectInstance;
    std::size_t receivedParameterCount = 0U;
    rti1516_2025::TransportationTypeHandle receivedTransportationType;
    rti1516_2025::FederateHandle receivedProducingFederate;
    std::vector<std::uint8_t> receivedTag;
    std::wstring receivedTimestampImplementationName;
    std::vector<std::uint8_t> receivedTimestampEncoding;
    rti1516_2025::OrderType receivedSentOrderType = rti1516_2025::RECEIVE;
    rti1516_2025::OrderType receivedReceivedOrderType = rti1516_2025::RECEIVE;
    bool receivedHasOptionalRetraction = false;
    std::size_t requestRetractionCount = 0U;
    rti1516_2025::MessageRetractionHandle requestRetractionHandle;
  };

  constexpr wchar_t const* federationName =
      L"public-process-directed-callback-execution";
  constexpr wchar_t const* senderName =
      L"public-process-directed-callback-sender";
  constexpr wchar_t const* receiverName =
      L"public-process-directed-callback-receiver";
  constexpr wchar_t const* objectClassName =
      L"HLAobjectRoot.UmbraDirectedFixtureObject";
  constexpr char const* objectClassNameUtf8 =
      "HLAobjectRoot.UmbraDirectedFixtureObject";
  constexpr wchar_t const* interactionClassName =
      L"HLAinteractionRoot.UmbraDirectedFixtureInteraction";
  constexpr char const* interactionClassNameUtf8 =
      "HLAinteractionRoot.UmbraDirectedFixtureInteraction";
  constexpr wchar_t const* attributeName = L"DirectedTargetMarker";
  constexpr char const* attributeNameUtf8 = "DirectedTargetMarker";

  auto listener = ProcessTransportListener::listen({"127.0.0.1", 0U});
  REQUIRE(listener);
  auto const port = listener->address().port;
  REQUIRE(port != 0U);

  std::atomic_uint64_t expectedObjectClass{0U};
  std::atomic_uint64_t expectedAttribute{0U};
  std::atomic_uint64_t expectedInteractionClass{0U};
  std::exception_ptr serverError;
  std::thread server([&] {
    try {
      EmbeddedFederationRegistry registry;
      ProcessFederationService service(
          registry,
          composedDirectedProcessDefinition(),
          ProcessFederationServiceOptions{callbackModel == HLA_IMMEDIATE});

      auto senderConnection = listener->accept(
          nullptr,
          {"public-process-directed-callback-server", 0x9711U},
          [](std::wstring) {},
          [](std::wstring) { return false; });
      ProcessTransportSession sender(senderConnection);
      auto senderHandler = service.handlerFor(sender);
      auto serveExpected = [&](ProcessTransportSession& session,
                               auto const& handler,
                               TransportServiceOperation operation,
                               char const* description) {
        if (!ProcessTransportServiceDispatcher::serveOne(
                session,
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
          "The public directed callback server lost Create.");
      serveExpected(
          sender,
          senderHandler,
          TransportServiceOperation::join_federation_execution,
          "The public directed callback server lost sender Join.");

      auto const objectClass =
          registry.objectClassHandleFor(federationName, objectClassNameUtf8);
      auto const attribute = registry.attributeHandleFor(
          federationName, objectClassNameUtf8, attributeNameUtf8);
      auto const interactionClass =
          registry.interactionClassHandleFor(federationName, interactionClassNameUtf8);
      if (!objectClass || !attribute || !interactionClass) {
        throw std::runtime_error(
            "The public directed callback server could not resolve its FOM handles.");
      }
      expectedObjectClass.store(*objectClass, std::memory_order_release);
      expectedAttribute.store(*attribute, std::memory_order_release);
      expectedInteractionClass.store(*interactionClass, std::memory_order_release);

      auto receiverConnection = listener->accept(
          nullptr,
          {"public-process-directed-callback-server", 0x9712U},
          [](std::wstring) {},
          [](std::wstring) { return false; });
      ProcessTransportSession receiver(receiverConnection);
      auto receiverHandler = service.handlerFor(receiver);
      serveExpected(
          receiver,
          receiverHandler,
          TransportServiceOperation::join_federation_execution,
          "The public directed callback server lost receiver Join.");

      serveExpected(
          sender,
          senderHandler,
          TransportServiceOperation::get_object_class_handle,
          "The public directed callback server lost sender object lookup.");
      serveExpected(
          receiver,
          receiverHandler,
          TransportServiceOperation::get_object_class_handle,
          "The public directed callback server lost receiver object lookup.");
      serveExpected(
          sender,
          senderHandler,
          TransportServiceOperation::get_attribute_handle,
          "The public directed callback server lost sender attribute lookup.");
      serveExpected(
          receiver,
          receiverHandler,
          TransportServiceOperation::get_attribute_handle,
          "The public directed callback server lost receiver attribute lookup.");
      serveExpected(
          sender,
          senderHandler,
          TransportServiceOperation::get_interaction_class_handle,
          "The public directed callback server lost sender interaction lookup.");
      serveExpected(
          receiver,
          receiverHandler,
          TransportServiceOperation::get_interaction_class_handle,
          "The public directed callback server lost receiver interaction lookup.");
      serveExpected(
          sender,
          senderHandler,
          TransportServiceOperation::publish_object_class_attributes,
          "The public directed callback server lost target publication.");
      serveExpected(
          receiver,
          receiverHandler,
          TransportServiceOperation::subscribe_object_class_attributes,
          "The public directed callback server lost target subscription.");
      serveExpected(
          sender,
          senderHandler,
          TransportServiceOperation::publish_object_class_directed_interactions,
          "The public directed callback server lost directed publication.");
      serveExpected(
          receiver,
          receiverHandler,
          TransportServiceOperation::subscribe_object_class_directed_interactions,
          "The public directed callback server lost directed subscription.");
      serveExpected(
          sender,
          senderHandler,
          TransportServiceOperation::register_object_instance,
          "The public directed callback server lost target registration.");
      if (callbackModel == HLA_IMMEDIATE) {
        serveExpected(
            receiver,
            receiverHandler,
            TransportServiceOperation::get_object_class_handle,
            "The public directed callback server lost immediate discovery polling.");
      } else {
        serveExpected(
            receiver,
            receiverHandler,
            TransportServiceOperation::receive_interaction,
            "The public directed callback server lost discovery polling.");
      }
      serveExpected(
          sender,
          senderHandler,
          TransportServiceOperation::send_directed_interaction,
          "The public directed callback server lost directed Send.");
      if (retractBeforeReceive) {
        serveExpected(
            sender,
            senderHandler,
            TransportServiceOperation::retract,
            "The public directed callback server lost directed Retract.");
      }
      if (callbackModel == HLA_IMMEDIATE) {
        serveExpected(
            receiver,
            receiverHandler,
            TransportServiceOperation::get_object_class_handle,
            "The public directed callback server lost immediate directed polling.");
      } else {
        serveExpected(
            receiver,
            receiverHandler,
            TransportServiceOperation::receive_interaction,
            "The public directed callback server lost directed polling.");
      }
      if (retractAfterReceive) {
        serveExpected(
            sender,
            senderHandler,
            TransportServiceOperation::retract,
            "The public directed callback server lost post-delivery directed Retract.");
        if (callbackModel == HLA_IMMEDIATE) {
          serveExpected(
              receiver,
              receiverHandler,
              TransportServiceOperation::get_object_class_handle,
              "The public directed callback server lost immediate retraction polling.");
        } else {
          serveExpected(
              receiver,
              receiverHandler,
              TransportServiceOperation::receive_interaction,
              "The public directed callback server lost retraction polling.");
        }
      }
      serveExpected(
          sender,
          senderHandler,
          TransportServiceOperation::resign_federation_execution,
          "The public directed callback server lost sender Resign.");
      serveExpected(
          receiver,
          receiverHandler,
          TransportServiceOperation::resign_federation_execution,
          "The public directed callback server lost receiver Resign.");

      service.detach(sender);
      service.detach(receiver);
      senderConnection->close();
      receiverConnection->close();
    } catch (...) {
      serverError = std::current_exception();
    }
  });

  RecordingFederateAmbassador senderFederate;
  RecordingFederateAmbassador receiverFederate;
  auto senderRti = makeRti();
  auto receiverRti = makeRti();
  auto configuration = RtiConfiguration::createConfiguration()
                           .withConfigurationName(L"public-process-directed-callback-client")
                           .withRtiAddress(
                               L"tcp://127.0.0.1:" + std::to_wstring(port));
  std::exception_ptr clientError;
  bool senderJoined = false;
  bool receiverJoined = false;
  try {
    REQUIRE(senderRti->connect(senderFederate, callbackModel, configuration).addressUsed);
    senderRti->createFederationExecution(
        federationName, L"server-owned-directed-callback-fom.xml");
    auto const senderHandle = senderRti->joinFederationExecution(
        senderName, L"public-process-directed-callback-type", federationName);
    senderJoined = true;

    REQUIRE(receiverRti->connect(receiverFederate, callbackModel, configuration).addressUsed);
    auto const receiverHandle = receiverRti->joinFederationExecution(
        receiverName, L"public-process-directed-callback-type", federationName);
    receiverJoined = true;

    auto const senderObjectClass = senderRti->getObjectClassHandle(objectClassName);
    auto const receiverObjectClass = receiverRti->getObjectClassHandle(objectClassName);
    auto const senderAttribute =
        senderRti->getAttributeHandle(senderObjectClass, attributeName);
    auto const receiverAttribute =
        receiverRti->getAttributeHandle(receiverObjectClass, attributeName);
    auto const senderInteraction =
        senderRti->getInteractionClassHandle(interactionClassName);
    auto const receiverInteraction =
        receiverRti->getInteractionClassHandle(interactionClassName);
    REQUIRE(senderObjectClass == receiverObjectClass);
    REQUIRE(senderAttribute == receiverAttribute);
    REQUIRE(senderInteraction == receiverInteraction);
    REQUIRE(senderAttribute ==
            rti1516_2025::umbra_binding_detail::makeAttributeHandle(
                expectedAttribute.load(std::memory_order_acquire)));
    REQUIRE(senderInteraction ==
            rti1516_2025::umbra_binding_detail::makeInteractionClassHandle(
                expectedInteractionClass.load(std::memory_order_acquire)));

    REQUIRE_NOTHROW(senderRti->publishObjectClassAttributes(
        senderObjectClass,
        rti1516_2025::AttributeHandleSet{senderAttribute}));
    REQUIRE_NOTHROW(receiverRti->subscribeObjectClassAttributes(
        receiverObjectClass,
        rti1516_2025::AttributeHandleSet{receiverAttribute},
        true));
    REQUIRE_NOTHROW(senderRti->publishObjectClassDirectedInteractions(
        senderObjectClass,
        rti1516_2025::InteractionClassHandleSet{senderInteraction}));
    REQUIRE_NOTHROW(receiverRti->subscribeObjectClassDirectedInteractions(
        receiverObjectClass,
        rti1516_2025::InteractionClassHandleSet{receiverInteraction},
        true));

    auto const objectInstance = senderRti->registerObjectInstance(senderObjectClass);
    REQUIRE(objectInstance.isValid());
    REQUIRE_FALSE(receiverFederate.discovered);
    REQUIRE_FALSE(receiverFederate.received);

    // Process Receive Interaction has one shared polling fence: registration
    // discovery is admitted before the later directed event. HLA_IMMEDIATE
    // uses the next ordinary service request as the transport polling fence.
    if (callbackModel == HLA_IMMEDIATE) {
      static_cast<void>(receiverRti->getObjectClassHandle(objectClassName));
    } else {
      REQUIRE_FALSE(receiverRti->evokeCallback(0.0));
    }
    REQUIRE(receiverFederate.discovered);
    REQUIRE(receiverFederate.discoveredObjectInstance == objectInstance);
    REQUIRE(receiverFederate.discoveredObjectClass == receiverObjectClass);
    REQUIRE(receiverFederate.discoveredProducingFederate == senderHandle);
    REQUIRE_FALSE(receiverFederate.discoveredObjectInstanceName.empty());
    REQUIRE_FALSE(receiverFederate.received);

    std::array<std::uint8_t, 3U> encodedTag{0x44U, 0x49U, 0x52U};
    VariableLengthData userSuppliedTag(encodedTag.data(), encodedTag.size());
    if (gateCallbacks) {
      REQUIRE_NOTHROW(receiverRti->disableCallbacks());
    }
    std::vector<std::uint8_t> expectedTimestampBytes;
    std::optional<rti1516_2025::MessageRetractionHandle> retraction;
    if (timestamped) {
      auto factory = rti1516_2025::HLAlogicalTimeFactoryFactory::makeLogicalTimeFactory(
          L"HLAinteger64Time");
      auto* integerFactory =
          dynamic_cast<rti1516_2025::HLAinteger64TimeFactory*>(factory.get());
      REQUIRE(integerFactory != nullptr);
      auto timestamp = integerFactory->makeLogicalTime(5);
      REQUIRE(timestamp);
      auto const encodedTimestamp = timestamp->encode();
      if (encodedTimestamp.size() != 0U) {
        auto const* first =
            static_cast<std::uint8_t const*>(encodedTimestamp.data());
        expectedTimestampBytes.assign(
            first, first + encodedTimestamp.size());
      }
      retraction.emplace(senderRti->sendDirectedInteraction(
          senderInteraction,
          objectInstance,
          ParameterHandleValueMap{},
          userSuppliedTag,
          *timestamp));
      REQUIRE(retraction->isValid());
      if (retractBeforeReceive) {
        REQUIRE_NOTHROW(senderRti->retract(*retraction));
      }
    } else {
      REQUIRE_NOTHROW(senderRti->sendDirectedInteraction(
          senderInteraction,
          objectInstance,
          ParameterHandleValueMap{},
          userSuppliedTag));
    }

    if (retractBeforeReceive) {
      REQUIRE(timestamped);
      if (callbackModel == HLA_IMMEDIATE) {
        static_cast<void>(receiverRti->getObjectClassHandle(objectClassName));
      } else {
        REQUIRE_FALSE(receiverRti->evokeCallback(0.0));
      }
      REQUIRE_FALSE(receiverFederate.received);
    } else if (gateCallbacks) {
      // The process event is admitted while callbacks are disabled, but the
      // official FederateAmbassador must not observe it until the switch is
      // re-enabled. HLA_IMMEDIATE drains on Enable Callbacks; HLA_EVOKED
      // retains the event for the next Evoke Multiple call.
      if (callbackModel == HLA_IMMEDIATE) {
        static_cast<void>(receiverRti->getObjectClassHandle(objectClassName));
        REQUIRE_FALSE(receiverFederate.received);
        REQUIRE_NOTHROW(receiverRti->enableCallbacks());
        REQUIRE(receiverFederate.received);
      } else {
        // Evoke Callback performs exactly one process receive poll. Using
        // Evoke Multiple here would intentionally ask the server for a
        // second empty poll, which this one-event fixture does not provide.
        REQUIRE(receiverRti->evokeCallback(0.0));
        REQUIRE_FALSE(receiverFederate.received);
        REQUIRE_NOTHROW(receiverRti->enableCallbacks());
        REQUIRE_FALSE(receiverRti->evokeCallback(0.0));
      }
    } else if (callbackModel == HLA_IMMEDIATE) {
      static_cast<void>(receiverRti->getObjectClassHandle(objectClassName));
    } else {
      REQUIRE_FALSE(receiverRti->evokeCallback(0.0));
    }
    if (!retractBeforeReceive) {
      REQUIRE(receiverFederate.received);
      REQUIRE(receiverFederate.receivedInteractionClass == receiverInteraction);
      REQUIRE(receiverFederate.receivedObjectInstance == objectInstance);
      REQUIRE(receiverFederate.receivedParameterCount == 0U);
      REQUIRE(receiverFederate.receivedProducingFederate == senderHandle);
      REQUIRE(receiverFederate.receivedTag ==
              std::vector<std::uint8_t>{0x44U, 0x49U, 0x52U});
      REQUIRE(receiverFederate.receivedTransportationType.isValid());
      if (timestamped) {
        REQUIRE(receiverFederate.receivedTimestampImplementationName ==
                L"HLAinteger64Time");
        REQUIRE(receiverFederate.receivedTimestampEncoding == expectedTimestampBytes);
        REQUIRE(receiverFederate.receivedSentOrderType == rti1516_2025::RECEIVE);
        REQUIRE(receiverFederate.receivedReceivedOrderType == rti1516_2025::RECEIVE);
        REQUIRE(receiverFederate.receivedHasOptionalRetraction);
      }
    }
    if (retractAfterReceive) {
      REQUIRE(timestamped);
      REQUIRE(retraction.has_value());
      REQUIRE(receiverFederate.received);
      REQUIRE(receiverFederate.requestRetractionCount == 0U);
      REQUIRE_NOTHROW(senderRti->retract(*retraction));
      if (callbackModel == HLA_IMMEDIATE) {
        static_cast<void>(receiverRti->getObjectClassHandle(objectClassName));
      } else {
        REQUIRE_FALSE(receiverRti->evokeCallback(0.0));
      }
      REQUIRE(receiverFederate.requestRetractionCount == 1U);
      REQUIRE(receiverFederate.requestRetractionHandle == *retraction);
    }

    senderRti->resignFederationExecution(DELETE_OBJECTS);
    senderJoined = false;
    receiverRti->resignFederationExecution(NO_ACTION);
    receiverJoined = false;
    senderRti->disconnect();
    receiverRti->disconnect();
  } catch (...) {
    clientError = std::current_exception();
    if (senderJoined) {
      try {
        senderRti->resignFederationExecution(DELETE_OBJECTS);
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
  if (listener) {
    listener.reset();
  }
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

  SECTION("HLA_EVOKED receive-order") {
    runScenario(HLA_EVOKED, false, false, false);
  }
  SECTION("HLA_IMMEDIATE receive-order") {
    runScenario(HLA_IMMEDIATE, false, false, false);
  }
  SECTION("HLA_EVOKED timestamped") {
    runScenario(HLA_EVOKED, true, false, false);
  }
  SECTION("HLA_IMMEDIATE timestamped") {
    runScenario(HLA_IMMEDIATE, true, false, false);
  }
  SECTION("HLA_EVOKED callback-disabled queue") {
    runScenario(HLA_EVOKED, false, true, false);
  }
  SECTION("HLA_IMMEDIATE callback-disabled queue") {
    runScenario(HLA_IMMEDIATE, false, true, false);
  }
  SECTION("HLA_EVOKED timestamped retraction") {
    runScenario(HLA_EVOKED, true, false, true);
  }
  SECTION("HLA_IMMEDIATE timestamped retraction") {
    runScenario(HLA_IMMEDIATE, true, false, true);
  }
  SECTION("HLA_EVOKED timestamped post-delivery Request Retraction") {
    runScenario(HLA_EVOKED, true, false, false, true);
  }
  SECTION("HLA_IMMEDIATE timestamped post-delivery Request Retraction") {
    runScenario(HLA_IMMEDIATE, true, false, false, true);
  }
}
#endif

TEST_CASE("IEEE 1516.1-2025 connection support types have usable value semantics", "[baseline][support-types][unit][foundation]") {
  RtiConfiguration configuration = RtiConfiguration::createConfiguration()
                                     .withConfigurationName(L"embedded")
                                     .withRtiAddress(L"in-process")
                                     .withAdditionalSettings(L"callbacks=evoked");
  REQUIRE(configuration.configurationName() == L"embedded");
  REQUIRE(configuration.rtiAddress() == L"in-process");
  REQUIRE(configuration.additionalSettings() == L"callbacks=evoked");

  ConfigurationResult defaultResult;
  requireIgnoredConfiguration(defaultResult);

  std::array<unsigned char, 3> source{0x01, 0x02, 0x03};
  VariableLengthData value(source.data(), source.size());
  source[0] = 0xFF;
  REQUIRE(value.size() == 3);
  REQUIRE(std::memcmp(value.data(), "\x01\x02\x03", value.size()) == 0);

  VariableLengthData copied(value);
  REQUIRE(copied.size() == value.size());
  REQUIRE(std::memcmp(copied.data(), value.data(), value.size()) == 0);
}

TEST_CASE("RTIambassador Connect exposes all four official C++ overloads", "[integration][connection][federation-management]") {
  TestFederateAmbassador federate;
  HLAnoCredentials credentials;
  RtiConfiguration configuration = RtiConfiguration::createConfiguration()
                                     .withConfigurationName(L"embedded")
                                     .withRtiAddress(L"in-process")
                                     .withAdditionalSettings(L"ignored-by-initial-slice");

  SECTION("base overload") {
    auto rti = makeRti();
    requireIgnoredConfiguration(rti->connect(federate, HLA_IMMEDIATE));
    REQUIRE_THROWS_AS(rti->connect(federate, HLA_IMMEDIATE), rti1516_2025::AlreadyConnected);
    REQUIRE_NOTHROW(rti->disconnect());
  }

  SECTION("configuration overload") {
    auto rti = makeRti();
    requireIgnoredConfiguration(rti->connect(federate, HLA_EVOKED, configuration));
    REQUIRE_NOTHROW(rti->disconnect());
  }

  SECTION("credentials overload") {
    auto rti = makeRti();
    requireIgnoredConfiguration(rti->connect(federate, HLA_EVOKED, credentials));
    REQUIRE_NOTHROW(rti->disconnect());
  }

  SECTION("configuration and credentials overload") {
    auto rti = makeRti();
    requireIgnoredConfiguration(rti->connect(federate, HLA_EVOKED, configuration, credentials));
    REQUIRE_NOTHROW(rti->disconnect());
  }
}

TEST_CASE(
    "Embedded Connect rejects supplied credentials while authorization is disabled",
    "[integration][connection][authorization][credentials][federation-management]") {
  TestFederateAmbassador federate;
  HLAplainTextPassword password(L"test-password");
  RtiConfiguration configuration = RtiConfiguration::createConfiguration()
                                     .withConfigurationName(L"embedded")
                                     .withRtiAddress(L"in-process");

  SECTION("credentials overload") {
    auto rti = makeRti();
    REQUIRE_THROWS_AS(rti->connect(federate, HLA_EVOKED, password), Unauthorized);
    REQUIRE_NOTHROW(rti->connect(federate, HLA_EVOKED));
    REQUIRE_NOTHROW(rti->disconnect());
  }

  SECTION("configuration and credentials overload") {
    auto rti = makeRti();
    REQUIRE_THROWS_AS(
        rti->connect(federate, HLA_EVOKED, configuration, password),
        Unauthorized);
    REQUIRE_NOTHROW(rti->connect(federate, HLA_EVOKED));
    REQUIRE_NOTHROW(rti->disconnect());
  }
}

TEST_CASE(
    "Embedded Connect accepts only its filesystem service-report directory setting",
    "[integration][connection][mom][service-reporting]") {
  TestFederateAmbassador federate;
  auto const directory = temporaryServiceReportDirectory();
  RtiConfiguration configuration = RtiConfiguration::createConfiguration()
                                     .withAdditionalSettings(
                                         L"serviceReportDirectory=" + directory.wstring());
  auto rti = makeRti();

  auto const result = rti->connect(federate, HLA_EVOKED, configuration);
  REQUIRE(result.configurationUsed);
  REQUIRE_FALSE(result.addressUsed);
  REQUIRE(result.additionalSettingsResult == SETTINGS_APPLIED);
  REQUIRE(std::filesystem::is_directory(directory));
  REQUIRE_NOTHROW(rti->disconnect());

  std::error_code ignored;
  std::filesystem::remove_all(directory, ignored);
}

TEST_CASE(
    "Umbra embedded profile configuration exposes a typed service-report directory",
    "[integration][connection][mom][service-report-store][service-reporting][configuration]") {
  auto const directory = temporaryServiceReportDirectory();
  auto configuration = umbra::embedded::makeEmbeddedRtiConfiguration(
      umbra::embedded::ServiceReportConfiguration{directory});
  configuration.withConfigurationName(L"typed-service-report")
      .withRtiAddress(L"in-process");

  REQUIRE(configuration.additionalSettings() ==
          L"serviceReportDirectory=" + directory.wstring());
  REQUIRE(configuration.configurationName() == L"typed-service-report");
  REQUIRE(configuration.rtiAddress() == L"in-process");

  TestFederateAmbassador federate;
  auto rti = makeRti();
  auto const result = rti->connect(federate, HLA_EVOKED, configuration);
  REQUIRE(result.configurationUsed);
  REQUIRE_FALSE(result.addressUsed);
  REQUIRE(result.additionalSettingsResult == SETTINGS_APPLIED);
  REQUIRE(std::filesystem::is_directory(directory));
  REQUIRE_NOTHROW(rti->disconnect());

  std::error_code ignored;
  std::filesystem::remove_all(directory, ignored);
}

TEST_CASE(
    "Embedded Connect fails deterministically for an unusable service-report directory",
    "[integration][connection][mom][service-reporting]") {
  TestFederateAmbassador federate;
  auto const parent = temporaryServiceReportDirectory();
  std::filesystem::create_directories(parent);
  auto const regularFile = parent / "not-a-directory";
  std::ofstream output(regularFile, std::ios::binary | std::ios::trunc);
  REQUIRE(output.good());
  output.close();

  RtiConfiguration configuration = RtiConfiguration::createConfiguration()
                                     .withAdditionalSettings(
                                         L"serviceReportDirectory=" + regularFile.wstring());
  auto rti = makeRti();
  REQUIRE_THROWS_AS(rti->connect(federate, HLA_EVOKED, configuration), RTIinternalError);
  // A rejected configuration is not a partial connection and never falls
  // back to the in-memory test store.
  REQUIRE_NOTHROW(rti->connect(federate, HLA_EVOKED));
  REQUIRE_NOTHROW(rti->disconnect());

  std::error_code ignored;
  std::filesystem::remove_all(parent, ignored);
}

TEST_CASE(
    "Embedded Connect reports AlreadyConnected before validating service-report configuration",
    "[integration][connection][mom][service-report-store][service-reporting]") {
  TestFederateAmbassador firstFederate;
  TestFederateAmbassador secondFederate;
  auto rti = makeRti();
  REQUIRE_NOTHROW(rti->connect(firstFederate, HLA_EVOKED));

  auto const parent = temporaryServiceReportDirectory();
  std::filesystem::create_directories(parent);
  auto const regularFile = parent / "not-a-directory";
  std::ofstream output(regularFile, std::ios::binary | std::ios::trunc);
  REQUIRE(output.good());
  output << "not a directory";
  REQUIRE(output.good());
  output.close();

  auto const invalidConfiguration = RtiConfiguration::createConfiguration()
                                        .withAdditionalSettings(
                                            L"serviceReportDirectory=" + regularFile.wstring());
  // AlreadyConnected is a lifecycle precondition. It must be resolved before
  // the second configuration can validate or mutate the report-store path.
  REQUIRE_THROWS_AS(
      rti->connect(secondFederate, HLA_EVOKED, invalidConfiguration),
      rti1516_2025::AlreadyConnected);
  REQUIRE_NOTHROW(rti->disconnect());

  std::error_code ignored;
  std::filesystem::remove_all(parent, ignored);
}

TEST_CASE("RTIambassador Connect rejects unsupported callback models without connecting", "[integration][connection][federation-management]") {
  TestFederateAmbassador federate;
  auto rti = makeRti();

  REQUIRE_THROWS_AS(
      rti->connect(federate, static_cast<CallbackModel>(-1)),
      rti1516_2025::UnsupportedCallbackModel);
  REQUIRE_NOTHROW(rti->connect(federate, HLA_EVOKED));
  REQUIRE_NOTHROW(rti->disconnect());
}

TEST_CASE("RTIambassador Disconnect rejects an absent connection", "[integration][connection][federation-management]") {
  auto rti = makeRti();
  REQUIRE_THROWS_AS(rti->disconnect(), rti1516_2025::NotConnected);
}

TEST_CASE("RTIambassador callback controls honor both models with an empty embedded queue", "[integration][callbacks][federation-management]") {
  TestFederateAmbassador federate;

  SECTION("immediate callbacks make Evoke services a no-op") {
    auto rti = makeRti();
    REQUIRE_NOTHROW(rti->connect(federate, HLA_IMMEDIATE));
    REQUIRE_FALSE(rti->evokeCallback(0.0));
    REQUIRE_FALSE(rti->evokeMultipleCallbacks(0.0, 0.0));
    REQUIRE_NOTHROW(rti->disableCallbacks());
    REQUIRE_NOTHROW(rti->enableCallbacks());
    REQUIRE_NOTHROW(rti->disconnect());
  }

  SECTION("evoked callbacks report no pending work after controls are toggled") {
    auto rti = makeRti();
    REQUIRE_NOTHROW(rti->connect(federate, HLA_EVOKED));
    REQUIRE_NOTHROW(rti->disableCallbacks());
    REQUIRE_FALSE(rti->evokeCallback(0.0));
    REQUIRE_NOTHROW(rti->enableCallbacks());
    REQUIRE_FALSE(rti->evokeMultipleCallbacks(0.0, 0.0));
    REQUIRE_NOTHROW(rti->disconnect());
  }
}

TEST_CASE(
    "RTIambassador disconnect terminates an unjoined connection",
    "[integration][compliance][rti.service.disconnect][federation-management]") {
  TestFederateAmbassador federate;
  auto rti = makeRti();

  REQUIRE_NOTHROW(rti->connect(federate, HLA_EVOKED));
  REQUIRE_NOTHROW(rti->disconnect());
  REQUIRE_NOTHROW(rti->connect(federate, HLA_EVOKED));
  REQUIRE_NOTHROW(rti->disconnect());
}

#if defined(UMBRA_ENABLE_EMBEDDED_FEDERATION_MANAGEMENT)
TEST_CASE(
    "RTIambassador delivers Attribute Relevance Advisory callbacks through a configured process endpoint under HLA_EVOKED and HLA_IMMEDIATE",
    "[integration][foundation][object-management][data-distribution-management][callbacks][callback-immediate][transport][process-boundary][public-endpoint][2025][rti.service.get-attribute-relevance-advisory-switch][rti.service.set-attribute-relevance-advisory-switch][rti.service.publish-object-class-attributes][rti.service.register-object-instance][rti.service.subscribe-object-class-attributes][rti.service.unsubscribe-object-class-attributes][federate.callback.turn-updates-on-for-object-instance][federate.callback.turn-updates-off-for-object-instance]") {
  auto runScenario = [](CallbackModel callbackModel) {
  class RecordingAttributeRelevanceFederateAmbassador final
      : public NullFederateAmbassador {
   public:
    void discoverObjectInstance(
        rti1516_2025::ObjectInstanceHandle const& objectInstance,
        rti1516_2025::ObjectClassHandle const& objectClass,
        std::wstring const& objectInstanceName,
        rti1516_2025::FederateHandle const& producingFederate) override {
      discovered = true;
      discoveredObjectInstance = objectInstance;
      discoveredObjectClass = objectClass;
      discoveredObjectInstanceName = objectInstanceName;
      discoveredProducingFederate = producingFederate;
    }

    void turnUpdatesOnForObjectInstance(
        rti1516_2025::ObjectInstanceHandle const& objectInstance,
        rti1516_2025::AttributeHandleSet const& attributes) override {
      turnedOn = true;
      turnedOnObjectInstance = objectInstance;
      turnedOnAttributeCount = attributes.size();
      turnedOnAttributes = attributes;
      turnedOnUpdateRateDesignator.reset();
    }

    void turnUpdatesOnForObjectInstance(
        rti1516_2025::ObjectInstanceHandle const& objectInstance,
        rti1516_2025::AttributeHandleSet const& attributes,
        std::wstring const& updateRateDesignator) override {
      turnedOn = true;
      turnedOnObjectInstance = objectInstance;
      turnedOnAttributeCount = attributes.size();
      turnedOnAttributes = attributes;
      turnedOnUpdateRateDesignator = updateRateDesignator;
    }

    void turnUpdatesOffForObjectInstance(
        rti1516_2025::ObjectInstanceHandle const& objectInstance,
        rti1516_2025::AttributeHandleSet const& attributes) override {
      turnedOff = true;
      turnedOffObjectInstance = objectInstance;
      turnedOffAttributeCount = attributes.size();
      turnedOffAttributes = attributes;
    }

    bool discovered = false;
    rti1516_2025::ObjectInstanceHandle discoveredObjectInstance;
    rti1516_2025::ObjectClassHandle discoveredObjectClass;
    std::wstring discoveredObjectInstanceName;
    rti1516_2025::FederateHandle discoveredProducingFederate;
    bool turnedOn = false;
    rti1516_2025::ObjectInstanceHandle turnedOnObjectInstance;
    rti1516_2025::AttributeHandleSet turnedOnAttributes;
    std::size_t turnedOnAttributeCount = 0U;
    std::optional<std::wstring> turnedOnUpdateRateDesignator;
    bool turnedOff = false;
    rti1516_2025::ObjectInstanceHandle turnedOffObjectInstance;
    rti1516_2025::AttributeHandleSet turnedOffAttributes;
    std::size_t turnedOffAttributeCount = 0U;
  } ownerFederate, receiverFederate;

  auto listener = ProcessTransportListener::listen({"127.0.0.1", 0U});
  REQUIRE(listener);
  auto const port = listener->address().port;
  REQUIRE(port != 0U);

  constexpr wchar_t const* federationName =
      L"public-process-attribute-relevance-advisory-execution";
  constexpr char const* objectClassName = "HLAobjectRoot.Employee";
  constexpr wchar_t const* objectClassNameWide = L"HLAobjectRoot.Employee";
  constexpr char const* nameAttributeName = "Name";
  constexpr wchar_t const* nameAttributeNameWide = L"Name";
  constexpr char const* payRateAttributeName = "PayRate";
  constexpr wchar_t const* payRateAttributeNameWide = L"PayRate";
  std::atomic_uint64_t expectedObjectClass{0U};
  std::atomic_uint64_t expectedNameAttribute{0U};
  std::atomic_uint64_t expectedPayRateAttribute{0U};
  std::atomic_uint64_t expectedObjectInstance{0U};
  std::exception_ptr serverError;
  std::thread server([&] {
    try {
      EmbeddedFederationRegistry registry;
      ProcessFederationService service(
          registry,
          composedProcessDefinition(),
          ProcessFederationServiceOptions{true});
      auto ownerConnection = listener->accept(
          nullptr,
          {"public-process-attribute-relevance-advisory-server", 0xA501U},
          [](std::wstring) {},
          [](std::wstring) { return false; });
      ProcessTransportSession owner(ownerConnection);
      auto ownerHandler = service.handlerFor(owner);
      std::shared_ptr<ProcessTransportConnection> receiverConnection;
      auto serveExpected = [&](ProcessTransportSession& session,
                               auto const& handler,
                               TransportServiceOperation operation,
                               char const* description) {
        if (!ProcessTransportServiceDispatcher::serveOne(
                session,
                [&](TransportServiceMessage const& request) {
                  if (request.operation != operation) {
                    throw std::runtime_error(description);
                  }
                  auto response = handler(request);
                  if (operation == TransportServiceOperation::register_object_instance &&
                      response.status == TransportServiceStatus::ok) {
                    auto const registration =
                        umbra::detail::decodeProcessFederationRegisterObjectInstanceResult(
                            response.payload);
                    expectedObjectInstance.store(
                        registration.objectInstanceHandle,
                        std::memory_order_release);
                  }
                  return response;
                })) {
          throw std::runtime_error(description);
        }
      };

      serveExpected(
          owner,
          ownerHandler,
          TransportServiceOperation::create_federation_execution,
          "The public process advisory server lost Create.");
      auto const objectClass = registry.objectClassHandleFor(
          federationName, objectClassName);
      auto const nameAttribute = registry.attributeHandleFor(
          federationName, objectClassName, nameAttributeName);
      auto const payRateAttribute = registry.attributeHandleFor(
          federationName, objectClassName, payRateAttributeName);
      if (!objectClass || !nameAttribute || !payRateAttribute) {
        throw std::runtime_error(
            "The public process advisory server could not resolve its FOM handles.");
      }
      expectedObjectClass.store(*objectClass, std::memory_order_release);
      expectedNameAttribute.store(*nameAttribute, std::memory_order_release);
      expectedPayRateAttribute.store(*payRateAttribute, std::memory_order_release);
      serveExpected(
          owner,
          ownerHandler,
          TransportServiceOperation::join_federation_execution,
          "The public process advisory server lost owner Join.");
      receiverConnection = listener->accept(
          nullptr,
          {"public-process-attribute-relevance-advisory-server", 0xA502U},
          [](std::wstring) {},
          [](std::wstring) { return false; });
      ProcessTransportSession receiver(receiverConnection);
      auto receiverHandler = service.handlerFor(receiver);
      serveExpected(
          receiver,
          receiverHandler,
          TransportServiceOperation::join_federation_execution,
          "The public process advisory server lost receiver Join.");
      serveExpected(
          owner,
          ownerHandler,
          TransportServiceOperation::set_attribute_relevance_advisory_switch,
          "The public process advisory server lost advisory-switch Set.");
      serveExpected(
          owner,
          ownerHandler,
          TransportServiceOperation::get_attribute_relevance_advisory_switch,
          "The public process advisory server lost advisory-switch Get.");
      for (auto const operation : {
               TransportServiceOperation::get_object_class_handle,
               TransportServiceOperation::get_attribute_handle,
               TransportServiceOperation::get_attribute_handle,
           }) {
        serveExpected(
            owner,
            ownerHandler,
            operation,
            "The public process advisory server lost owner handle lookup.");
      }
      serveExpected(
          owner,
          ownerHandler,
          TransportServiceOperation::publish_object_class_attributes,
          "The public process advisory server lost Publish.");
      serveExpected(
          receiver,
          receiverHandler,
          TransportServiceOperation::get_object_class_handle,
          "The public process advisory server lost receiver class lookup.");
      serveExpected(
          receiver,
          receiverHandler,
          TransportServiceOperation::get_attribute_handle,
          "The public process advisory server lost receiver Name lookup.");
      serveExpected(
          receiver,
          receiverHandler,
          TransportServiceOperation::subscribe_object_class_attributes,
          "The public process advisory server lost initial Subscribe.");
      serveExpected(
          owner,
          ownerHandler,
          TransportServiceOperation::register_object_instance,
          "The public process advisory server lost Register.");
      if (callbackModel == HLA_IMMEDIATE) {
        serveExpected(
            receiver,
            receiverHandler,
            TransportServiceOperation::get_object_class_handle,
            "The public process advisory server lost immediate discovery lookup.");
      } else {
        serveExpected(
            receiver,
            receiverHandler,
            TransportServiceOperation::receive_interaction,
            "The public process advisory server lost discovery Receive.");
      }
      serveExpected(
          receiver,
          receiverHandler,
          TransportServiceOperation::get_attribute_handle,
          "The public process advisory server lost receiver PayRate lookup.");
      serveExpected(
          receiver,
          receiverHandler,
          TransportServiceOperation::subscribe_object_class_attributes,
          "The public process advisory server lost PayRate Subscribe.");
      if (callbackModel == HLA_IMMEDIATE) {
        serveExpected(
            owner,
            ownerHandler,
            TransportServiceOperation::get_object_class_handle,
            "The public process advisory server lost immediate Turn Updates On lookup.");
      } else {
        serveExpected(
            owner,
            ownerHandler,
            TransportServiceOperation::receive_interaction,
            "The public process advisory server lost Turn Updates On Receive.");
      }
      serveExpected(
          receiver,
          receiverHandler,
          TransportServiceOperation::unsubscribe_object_class_attributes,
          "The public process advisory server lost PayRate Unsubscribe.");
      if (callbackModel == HLA_IMMEDIATE) {
        serveExpected(
            owner,
            ownerHandler,
            TransportServiceOperation::get_object_class_handle,
            "The public process advisory server lost immediate Turn Updates Off lookup.");
      } else {
        serveExpected(
            owner,
            ownerHandler,
            TransportServiceOperation::receive_interaction,
            "The public process advisory server lost Turn Updates Off Receive.");
      }
      serveExpected(
          owner,
          ownerHandler,
          TransportServiceOperation::resign_federation_execution,
          "The public process advisory server lost owner Resign.");
      serveExpected(
          receiver,
          receiverHandler,
          TransportServiceOperation::resign_federation_execution,
          "The public process advisory server lost receiver Resign.");
      service.detach(owner);
      service.detach(receiver);
      ownerConnection->close();
      receiverConnection->close();
    } catch (...) {
      serverError = std::current_exception();
    }
  });

  auto ownerRti = makeRti();
  auto receiverRti = makeRti();
  auto pumpEvoked = [&](RTIambassador& rti) {
    if (callbackModel == HLA_EVOKED) {
      static_cast<void>(rti.evokeCallback(0.0));
    }
  };
  auto configuration = RtiConfiguration::createConfiguration()
                           .withConfigurationName(
                               L"public-process-attribute-relevance-advisory-client")
                           .withRtiAddress(
                               L"tcp://127.0.0.1:" + std::to_wstring(port));
  std::exception_ptr clientError;
  bool ownerJoined = false;
  bool receiverJoined = false;
  try {
    REQUIRE(ownerRti->connect(ownerFederate, callbackModel, configuration).addressUsed);
    ownerRti->createFederationExecution(federationName, L"server-owned-fom.xml");
    ownerRti->joinFederationExecution(
        L"public-process-attribute-relevance-owner",
        L"public-process-attribute-relevance-type",
        federationName);
    ownerJoined = true;

    REQUIRE(receiverRti->connect(receiverFederate, callbackModel, configuration).addressUsed);
    receiverRti->joinFederationExecution(
        L"public-process-attribute-relevance-receiver",
        L"public-process-attribute-relevance-type",
        federationName);
    receiverJoined = true;

    REQUIRE_NOTHROW(ownerRti->setAttributeRelevanceAdvisorySwitch(true));
    REQUIRE(ownerRti->getAttributeRelevanceAdvisorySwitch());

    auto const ownerObjectClass = ownerRti->getObjectClassHandle(objectClassNameWide);
    auto const ownerNameAttribute = ownerRti->getAttributeHandle(
        ownerObjectClass, nameAttributeNameWide);
    auto const ownerPayRateAttribute = ownerRti->getAttributeHandle(
        ownerObjectClass, payRateAttributeNameWide);
    REQUIRE(ownerObjectClass.toString() ==
            L"ObjectClassHandle(" +
                std::to_wstring(expectedObjectClass.load(std::memory_order_acquire)) +
                L")");
    REQUIRE(ownerNameAttribute ==
            rti1516_2025::umbra_binding_detail::makeAttributeHandle(
                expectedNameAttribute.load(std::memory_order_acquire)));
    REQUIRE(ownerPayRateAttribute ==
            rti1516_2025::umbra_binding_detail::makeAttributeHandle(
                expectedPayRateAttribute.load(std::memory_order_acquire)));
    REQUIRE_NOTHROW(ownerRti->publishObjectClassAttributes(
        ownerObjectClass,
        rti1516_2025::AttributeHandleSet{
            ownerNameAttribute,
            ownerPayRateAttribute}));

    auto const receiverObjectClass =
        receiverRti->getObjectClassHandle(objectClassNameWide);
    auto const receiverNameAttribute = receiverRti->getAttributeHandle(
        receiverObjectClass, nameAttributeNameWide);
    REQUIRE(receiverObjectClass == ownerObjectClass);
    REQUIRE(receiverNameAttribute == ownerNameAttribute);
    REQUIRE_NOTHROW(receiverRti->subscribeObjectClassAttributes(
        receiverObjectClass,
        rti1516_2025::AttributeHandleSet{receiverNameAttribute}));

    auto const objectInstance = ownerRti->registerObjectInstance(ownerObjectClass);
    REQUIRE(objectInstance.isValid());
    REQUIRE(objectInstance.toString() ==
            L"ObjectInstanceHandle(" +
                std::to_wstring(expectedObjectInstance.load(std::memory_order_acquire)) +
                L")");
    if (callbackModel == HLA_IMMEDIATE) {
      static_cast<void>(receiverRti->getObjectClassHandle(objectClassNameWide));
    } else {
      pumpEvoked(*receiverRti);
    }
    REQUIRE(receiverFederate.discovered);
    REQUIRE(receiverFederate.discoveredObjectInstance == objectInstance);
    REQUIRE(receiverFederate.discoveredObjectClass == receiverObjectClass);

    // Registration/discovery can make the receiver's initial declaration
    // relevant immediately. Drain that owner-directed advisory before
    // changing the receiver declaration so the later PayRate transition stays
    // isolated under both callback models.
    if (callbackModel == HLA_EVOKED) {
      pumpEvoked(*ownerRti);
    }
    REQUIRE(ownerFederate.turnedOn);
    REQUIRE(ownerFederate.turnedOnObjectInstance == objectInstance);
    REQUIRE(ownerFederate.turnedOnAttributes.contains(ownerNameAttribute));
    REQUIRE(ownerFederate.turnedOnUpdateRateDesignator == std::nullopt);
    ownerFederate.turnedOn = false;
    ownerFederate.turnedOnObjectInstance = {};
    ownerFederate.turnedOnAttributes.clear();
    ownerFederate.turnedOnAttributeCount = 0U;
    ownerFederate.turnedOnUpdateRateDesignator.reset();

    auto const receiverPayRateAttribute = receiverRti->getAttributeHandle(
        receiverObjectClass, payRateAttributeNameWide);
    REQUIRE(receiverPayRateAttribute == ownerPayRateAttribute);
    REQUIRE_NOTHROW(receiverRti->subscribeObjectClassAttributes(
        receiverObjectClass,
        rti1516_2025::AttributeHandleSet{receiverPayRateAttribute},
        true,
        L"High"));

    if (callbackModel == HLA_IMMEDIATE) {
      static_cast<void>(ownerRti->getObjectClassHandle(objectClassNameWide));
    } else {
      pumpEvoked(*ownerRti);
    }
    REQUIRE(ownerFederate.turnedOn);
    REQUIRE(ownerFederate.turnedOnObjectInstance == objectInstance);
    REQUIRE(ownerFederate.turnedOnAttributeCount == 1U);
    REQUIRE(ownerFederate.turnedOnAttributes.contains(ownerPayRateAttribute));
    REQUIRE(ownerFederate.turnedOnUpdateRateDesignator == std::optional<std::wstring>{L"High"});

    REQUIRE_NOTHROW(receiverRti->unsubscribeObjectClassAttributes(
        receiverObjectClass,
        rti1516_2025::AttributeHandleSet{receiverPayRateAttribute}));
    if (callbackModel == HLA_IMMEDIATE) {
      static_cast<void>(ownerRti->getObjectClassHandle(objectClassNameWide));
    } else {
      pumpEvoked(*ownerRti);
    }
    REQUIRE(ownerFederate.turnedOff);
    REQUIRE(ownerFederate.turnedOffObjectInstance == objectInstance);
    REQUIRE(ownerFederate.turnedOffAttributeCount == 1U);
    REQUIRE(ownerFederate.turnedOffAttributes.contains(ownerPayRateAttribute));

    ownerRti->resignFederationExecution(DELETE_OBJECTS);
    ownerJoined = false;
    receiverRti->resignFederationExecution(NO_ACTION);
    receiverJoined = false;
    ownerRti->disconnect();
    receiverRti->disconnect();
  } catch (...) {
    clientError = std::current_exception();
    if (ownerJoined) {
      try {
        ownerRti->resignFederationExecution(DELETE_OBJECTS);
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
      ownerRti->disconnect();
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
  if (clientError) {
    std::rethrow_exception(clientError);
  }
  REQUIRE_FALSE(serverError);
  REQUIRE_FALSE(ownerJoined);
  REQUIRE_FALSE(receiverJoined);
  };

  SECTION("HLA_EVOKED") {
    runScenario(HLA_EVOKED);
  }
  SECTION("HLA_IMMEDIATE") {
    runScenario(HLA_IMMEDIATE);
  }
}
TEST_CASE(
    "RTIambassador routes Delete Object Instance and removal callback through a configured process endpoint",
    "[integration][foundation][object-management][transport][process-boundary][public-endpoint][delete-object-instance][rti.service.delete-object-instance][federate.callback.remove-object-instance]") {
  class RecordingFederateAmbassador final : public NullFederateAmbassador {
   public:
    void discoverObjectInstance(
        rti1516_2025::ObjectInstanceHandle const& objectInstance,
        rti1516_2025::ObjectClassHandle const& objectClass,
        std::wstring const& objectInstanceName,
        rti1516_2025::FederateHandle const& producingFederate) override {
      discovered = true;
      discoveredObjectInstance = objectInstance;
      discoveredObjectClass = objectClass;
      discoveredObjectInstanceName = objectInstanceName;
      discoveredProducingFederate = producingFederate;
    }

    void removeObjectInstance(
        rti1516_2025::ObjectInstanceHandle const& objectInstance,
        VariableLengthData const& userSuppliedTag,
        rti1516_2025::FederateHandle const& producingFederate) override {
      removed = true;
      removedObjectInstance = objectInstance;
      removedProducingFederate = producingFederate;
      removedTag.clear();
      if (userSuppliedTag.size() != 0U) {
        auto const* data =
            static_cast<std::uint8_t const*>(userSuppliedTag.data());
        removedTag.assign(data, data + userSuppliedTag.size());
      }
    }

    bool discovered = false;
    rti1516_2025::ObjectInstanceHandle discoveredObjectInstance;
    rti1516_2025::ObjectClassHandle discoveredObjectClass;
    std::wstring discoveredObjectInstanceName;
    rti1516_2025::FederateHandle discoveredProducingFederate;
    bool removed = false;
    rti1516_2025::ObjectInstanceHandle removedObjectInstance;
    rti1516_2025::FederateHandle removedProducingFederate;
    std::vector<std::uint8_t> removedTag;
  } receiverFederate;

  auto listener = ProcessTransportListener::listen({"127.0.0.1", 0U});
  REQUIRE(listener);
  auto const port = listener->address().port;
  REQUIRE(port != 0U);

  constexpr wchar_t const* federationName =
      L"public-process-delete-execution";
  constexpr char const* objectClassName = "HLAobjectRoot.Employee";
  constexpr wchar_t const* objectClassNameWide = L"HLAobjectRoot.Employee";
  constexpr char const* attributeName = "Name";
  constexpr wchar_t const* attributeNameWide = L"Name";
  std::atomic_uint64_t expectedObjectClass{0U};
  std::atomic_uint64_t expectedAttribute{0U};
  std::atomic_uint64_t expectedSenderFederate{0U};
  std::atomic_uint64_t expectedReceiverFederate{0U};
  std::atomic_uint64_t registeredObject{0U};
  std::atomic_uint32_t deleteRecipientCount{0U};
  std::atomic_bool producerKnowledgeCleared{false};
  std::atomic_bool receiverKnowledgeCleared{false};
  std::exception_ptr serverError;
  std::thread server([&] {
    try {
      EmbeddedFederationRegistry registry;
      ProcessFederationService service(
          registry, composedProcessDefinition(), ProcessFederationServiceOptions{});
      auto senderConnection = listener->accept(
          nullptr,
          {"public-process-delete-server", 0x9C01U},
          [](std::wstring) {},
          [](std::wstring) { return false; });
      ProcessTransportSession sender(senderConnection);
      auto senderBaseHandler = service.handlerFor(sender);
      auto senderHandler = [&](TransportServiceMessage const& request) {
        auto response = senderBaseHandler(request);
        if (request.operation ==
                TransportServiceOperation::join_federation_execution &&
            response.status == TransportServiceStatus::ok) {
          auto const result =
              umbra::detail::decodeProcessFederationJoinResult(response.payload);
          expectedSenderFederate.store(result.federateId, std::memory_order_release);
        }
        if (request.operation ==
                TransportServiceOperation::register_object_instance &&
            response.status == TransportServiceStatus::ok) {
          auto const result =
              umbra::detail::decodeProcessFederationRegisterObjectInstanceResult(
                  response.payload);
          registeredObject.store(
              result.objectInstanceHandle, std::memory_order_release);
        }
        if (request.operation == TransportServiceOperation::delete_object_instance &&
            response.status == TransportServiceStatus::ok) {
          auto const result =
              umbra::detail::decodeProcessFederationDeleteObjectInstanceResult(
                  response.payload);
          deleteRecipientCount.store(
              result.recipientCount, std::memory_order_release);
        }
        return response;
      };
      auto serveExpected = [&](ProcessTransportSession& session,
                               auto const& handler,
                               TransportServiceOperation operation,
                               char const* description) {
        if (!ProcessTransportServiceDispatcher::serveOne(
                session,
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
          "The public process delete server lost Create.");
      auto const objectClass = registry.objectClassHandleFor(
          federationName, objectClassName);
      auto const attribute = registry.attributeHandleFor(
          federationName, objectClassName, attributeName);
      if (!objectClass || !attribute) {
        throw std::runtime_error(
            "The public process delete server could not resolve its FOM handles.");
      }
      expectedObjectClass.store(*objectClass, std::memory_order_release);
      expectedAttribute.store(*attribute, std::memory_order_release);
      serveExpected(
          sender,
          senderHandler,
          TransportServiceOperation::join_federation_execution,
          "The public process delete server lost sender Join.");

      auto receiverConnection = listener->accept(
          nullptr,
          {"public-process-delete-server", 0x9C02U},
          [](std::wstring) {},
          [](std::wstring) { return false; });
      ProcessTransportSession receiver(receiverConnection);
      auto receiverBaseHandler = service.handlerFor(receiver);
      auto receiverHandler = [&](TransportServiceMessage const& request) {
        auto response = receiverBaseHandler(request);
        if (request.operation ==
                TransportServiceOperation::join_federation_execution &&
            response.status == TransportServiceStatus::ok) {
          auto const result =
              umbra::detail::decodeProcessFederationJoinResult(response.payload);
          expectedReceiverFederate.store(result.federateId, std::memory_order_release);
        }
        return response;
      };
      serveExpected(
          receiver,
          receiverHandler,
          TransportServiceOperation::join_federation_execution,
          "The public process delete server lost receiver Join.");
      serveExpected(
          sender,
          senderHandler,
          TransportServiceOperation::get_object_class_handle,
          "The public process delete server lost sender object-class lookup.");
      serveExpected(
          sender,
          senderHandler,
          TransportServiceOperation::get_attribute_handle,
          "The public process delete server lost sender attribute lookup.");
      serveExpected(
          receiver,
          receiverHandler,
          TransportServiceOperation::get_object_class_handle,
          "The public process delete server lost receiver object-class lookup.");
      serveExpected(
          receiver,
          receiverHandler,
          TransportServiceOperation::get_attribute_handle,
          "The public process delete server lost receiver attribute lookup.");
      serveExpected(
          receiver,
          receiverHandler,
          TransportServiceOperation::subscribe_object_class_attributes,
          "The public process delete server lost receiver Subscribe.");
      serveExpected(
          sender,
          senderHandler,
          TransportServiceOperation::publish_object_class_attributes,
          "The public process delete server lost sender Publish.");
      serveExpected(
          sender,
          senderHandler,
          TransportServiceOperation::register_object_instance,
          "The public process delete server lost sender Register.");
      serveExpected(
          receiver,
          receiverHandler,
          TransportServiceOperation::receive_interaction,
          "The public process delete server lost receiver discovery poll.");
      serveExpected(
          sender,
          senderHandler,
          TransportServiceOperation::delete_object_instance,
          "The public process delete server lost sender Delete.");
      serveExpected(
          receiver,
          receiverHandler,
          TransportServiceOperation::receive_interaction,
          "The public process delete server lost receiver removal poll.");
      auto const receiverMember = registry.memberByName(
          federationName, L"public-process-delete-receiver");
      auto const senderMember = registry.memberByName(
          federationName, L"public-process-delete-sender");
      auto const objectHandle = registeredObject.load(std::memory_order_acquire);
      producerKnowledgeCleared.store(
          senderMember && objectHandle != 0U &&
              !registry.knownObjectInstanceFor(
                  federationName, senderMember->id, objectHandle),
          std::memory_order_release);
      receiverKnowledgeCleared.store(
          receiverMember && objectHandle != 0U &&
              !registry.knownObjectInstanceFor(
                  federationName, receiverMember->id, objectHandle),
          std::memory_order_release);
      serveExpected(
          sender,
          senderHandler,
          TransportServiceOperation::resign_federation_execution,
          "The public process delete server lost sender Resign.");
      serveExpected(
          receiver,
          receiverHandler,
          TransportServiceOperation::resign_federation_execution,
          "The public process delete server lost receiver Resign.");
      service.detach(sender);
      service.detach(receiver);
      senderConnection->close();
      receiverConnection->close();
    } catch (...) {
      serverError = std::current_exception();
    }
  });

  RecordingFederateAmbassador senderFederate;
  auto senderRti = makeRti();
  auto receiverRti = makeRti();
  auto senderConfiguration = RtiConfiguration::createConfiguration()
                                 .withConfigurationName(
                                     L"public-process-delete-sender")
                                 .withRtiAddress(
                                     L"tcp://127.0.0.1:" + std::to_wstring(port));
  auto receiverConfiguration = RtiConfiguration::createConfiguration()
                                   .withConfigurationName(
                                       L"public-process-delete-receiver")
                                   .withRtiAddress(
                                       L"tcp://127.0.0.1:" + std::to_wstring(port));
  std::exception_ptr clientError;
  bool senderJoined = false;
  bool receiverJoined = false;
  try {
    REQUIRE(senderRti->connect(
                senderFederate, HLA_EVOKED, senderConfiguration)
                .addressUsed);
    senderRti->createFederationExecution(federationName, L"server-owned-fom.xml");
    senderRti->joinFederationExecution(
        L"public-process-delete-sender",
        L"public-process-delete-type",
        federationName);
    senderJoined = true;

    REQUIRE(receiverRti->connect(
                receiverFederate, HLA_EVOKED, receiverConfiguration)
                .addressUsed);
    receiverRti->joinFederationExecution(
        L"public-process-delete-receiver",
        L"public-process-delete-type",
        federationName);
    receiverJoined = true;

    auto const senderObjectClass =
        senderRti->getObjectClassHandle(objectClassNameWide);
    auto const senderAttribute =
        senderRti->getAttributeHandle(senderObjectClass, attributeNameWide);
    REQUIRE(senderObjectClass.toString() ==
            L"ObjectClassHandle(" +
                std::to_wstring(expectedObjectClass.load(std::memory_order_acquire)) +
                L")");
    REQUIRE(senderAttribute ==
            rti1516_2025::umbra_binding_detail::makeAttributeHandle(
                expectedAttribute.load(std::memory_order_acquire)));

    auto const receiverObjectClass =
        receiverRti->getObjectClassHandle(objectClassNameWide);
    auto const receiverAttribute =
        receiverRti->getAttributeHandle(receiverObjectClass, attributeNameWide);
    rti1516_2025::AttributeHandleSet receiverAttributes;
    receiverAttributes.insert(receiverAttribute);
    receiverRti->subscribeObjectClassAttributes(
        receiverObjectClass, receiverAttributes);

    rti1516_2025::AttributeHandleSet senderAttributes;
    senderAttributes.insert(senderAttribute);
    senderRti->publishObjectClassAttributes(senderObjectClass, senderAttributes);

    auto const objectInstance =
        senderRti->registerObjectInstance(senderObjectClass);
    REQUIRE(objectInstance.isValid());
    for (std::size_t evokeCount = 0U;
         evokeCount < 4U && !receiverFederate.discovered;
         ++evokeCount) {
      static_cast<void>(receiverRti->evokeCallback(0.0));
    }
    REQUIRE(receiverFederate.discovered);
    REQUIRE(receiverFederate.discoveredObjectInstance == objectInstance);
    REQUIRE(receiverFederate.discoveredObjectClass == receiverObjectClass);
    REQUIRE(receiverFederate.discoveredProducingFederate.isValid());

    std::vector<std::uint8_t> const expectedTag{0x44U, 0x45U, 0x4CU};
    senderRti->deleteObjectInstance(
        objectInstance,
        VariableLengthData(expectedTag.data(), expectedTag.size()));
    for (std::size_t evokeCount = 0U;
         evokeCount < 4U && !receiverFederate.removed;
         ++evokeCount) {
      static_cast<void>(receiverRti->evokeCallback(0.0));
    }
    REQUIRE(receiverFederate.removed);
    REQUIRE(receiverFederate.removedObjectInstance == objectInstance);
    REQUIRE(receiverFederate.removedProducingFederate ==
            receiverFederate.discoveredProducingFederate);
    REQUIRE(receiverFederate.removedTag == expectedTag);
    REQUIRE_FALSE(senderFederate.removed);

    senderRti->resignFederationExecution(DELETE_OBJECTS);
    senderJoined = false;
    receiverRti->resignFederationExecution(NO_ACTION);
    receiverJoined = false;
    senderRti->disconnect();
    receiverRti->disconnect();
  } catch (...) {
    clientError = std::current_exception();
    if (senderJoined) {
      try {
        senderRti->resignFederationExecution(DELETE_OBJECTS);
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
  if (listener) {
    listener.reset();
  }
  if (server.joinable()) {
    server.join();
  }
  if (clientError) {
    std::rethrow_exception(clientError);
  }
  REQUIRE_FALSE(serverError);
  REQUIRE(deleteRecipientCount.load(std::memory_order_acquire) == 1U);
  REQUIRE(producerKnowledgeCleared.load(std::memory_order_acquire));
  REQUIRE(receiverKnowledgeCleared.load(std::memory_order_acquire));
  REQUIRE(expectedSenderFederate.load(std::memory_order_acquire) != 0U);
  REQUIRE(expectedReceiverFederate.load(std::memory_order_acquire) != 0U);
  REQUIRE(registeredObject.load(std::memory_order_acquire) != 0U);
  REQUIRE_FALSE(senderJoined);
  REQUIRE_FALSE(receiverJoined);
}

TEST_CASE(
    "RTIambassador routes timestamped Delete Object Instance and removal callback through a configured process endpoint",
    "[integration][foundation][object-management][time-management][transport][process-boundary][public-endpoint][timestamped-delete-object-instance][rti.service.delete-object-instance][federate.callback.remove-object-instance]") {
  auto runScenario = [](CallbackModel callbackModel) {
  class RecordingFederateAmbassador final : public NullFederateAmbassador {
   public:
    void discoverObjectInstance(
        rti1516_2025::ObjectInstanceHandle const& objectInstance,
        rti1516_2025::ObjectClassHandle const& objectClass,
        std::wstring const& objectInstanceName,
        rti1516_2025::FederateHandle const& producingFederate) override {
      discovered = true;
      discoveredObjectInstance = objectInstance;
      discoveredObjectClass = objectClass;
      discoveredObjectInstanceName = objectInstanceName;
      discoveredProducingFederate = producingFederate;
    }

    void removeObjectInstance(
        rti1516_2025::ObjectInstanceHandle const& objectInstance,
        VariableLengthData const& userSuppliedTag,
        rti1516_2025::FederateHandle const& producingFederate,
        rti1516_2025::LogicalTime const& time,
        rti1516_2025::OrderType sentOrder,
        rti1516_2025::OrderType receivedOrder,
        rti1516_2025::MessageRetractionHandle const* optionalRetraction) override {
      removed = true;
      removedObjectInstance = objectInstance;
      removedProducingFederate = producingFederate;
      removedSentOrder = sentOrder;
      removedReceivedOrder = receivedOrder;
      hasOptionalRetraction = optionalRetraction != nullptr;
      timestampImplementationName = time.implementationName();
      auto const encoded = time.encode();
      timestampEncoding.clear();
      if (encoded.size() != 0U) {
        auto const* first = static_cast<std::uint8_t const*>(encoded.data());
        timestampEncoding.assign(first, first + encoded.size());
      }
      removedTag.clear();
      if (userSuppliedTag.size() != 0U) {
        auto const* first =
            static_cast<std::uint8_t const*>(userSuppliedTag.data());
        removedTag.assign(first, first + userSuppliedTag.size());
      }
    }

    bool discovered = false;
    rti1516_2025::ObjectInstanceHandle discoveredObjectInstance;
    rti1516_2025::ObjectClassHandle discoveredObjectClass;
    std::wstring discoveredObjectInstanceName;
    rti1516_2025::FederateHandle discoveredProducingFederate;
    bool removed = false;
    rti1516_2025::ObjectInstanceHandle removedObjectInstance;
    rti1516_2025::FederateHandle removedProducingFederate;
    rti1516_2025::OrderType removedSentOrder = rti1516_2025::RECEIVE;
    rti1516_2025::OrderType removedReceivedOrder = rti1516_2025::RECEIVE;
    bool hasOptionalRetraction = false;
    std::wstring timestampImplementationName;
    std::vector<std::uint8_t> timestampEncoding;
    std::vector<std::uint8_t> removedTag;
  } receiverFederate;

  auto listener = ProcessTransportListener::listen({"127.0.0.1", 0U});
  REQUIRE(listener);
  auto const port = listener->address().port;
  REQUIRE(port != 0U);

  constexpr wchar_t const* federationName =
      L"public-process-timestamped-delete-execution";
  constexpr char const* objectClassName = "HLAobjectRoot.Employee";
  constexpr wchar_t const* objectClassNameWide = L"HLAobjectRoot.Employee";
  constexpr char const* attributeName = "Name";
  constexpr wchar_t const* attributeNameWide = L"Name";
  std::atomic_uint64_t expectedObjectClass{0U};
  std::atomic_uint64_t expectedAttribute{0U};
  std::atomic_uint64_t expectedSenderFederate{0U};
  std::atomic_uint64_t expectedReceiverFederate{0U};
  std::atomic_uint64_t registeredObject{0U};
  std::atomic_uint32_t deleteRecipientCount{0U};
  std::atomic_uint64_t deleteMessageId{0U};
  std::atomic_bool producerKnowledgeCleared{false};
  std::atomic_bool receiverKnowledgeCleared{false};
  std::exception_ptr serverError;
  std::thread server([&] {
    try {
      EmbeddedFederationRegistry registry;
      ProcessFederationService service(
          registry,
          composedProcessDefinition(),
          ProcessFederationServiceOptions{callbackModel == HLA_IMMEDIATE});
      auto senderConnection = listener->accept(
          nullptr,
          {"public-process-timestamped-delete-server", 0x9D01U},
          [](std::wstring) {},
          [](std::wstring) { return false; });
      ProcessTransportSession sender(senderConnection);
      auto senderBaseHandler = service.handlerFor(sender);
      auto senderHandler = [&](TransportServiceMessage const& request) {
        auto response = senderBaseHandler(request);
        if (request.operation == TransportServiceOperation::join_federation_execution &&
            response.status == TransportServiceStatus::ok) {
          expectedSenderFederate.store(
              umbra::detail::decodeProcessFederationJoinResult(response.payload)
                  .federateId,
              std::memory_order_release);
        }
        if (request.operation == TransportServiceOperation::register_object_instance &&
            response.status == TransportServiceStatus::ok) {
          registeredObject.store(
              umbra::detail::decodeProcessFederationRegisterObjectInstanceResult(
                  response.payload)
                  .objectInstanceHandle,
              std::memory_order_release);
        }
        if (request.operation == TransportServiceOperation::delete_object_instance &&
            response.status == TransportServiceStatus::ok) {
          auto const result =
              umbra::detail::decodeProcessFederationDeleteObjectInstanceResult(
                  response.payload);
          deleteRecipientCount.store(result.recipientCount, std::memory_order_release);
          deleteMessageId.store(result.messageId, std::memory_order_release);
        }
        return response;
      };
      auto serveExpected = [&](ProcessTransportSession& session,
                               auto const& handler,
                               TransportServiceOperation operation,
                               char const* description) {
        if (!ProcessTransportServiceDispatcher::serveOne(
                session,
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
          "The public timestamped process delete server lost Create.");
      auto const objectClass = registry.objectClassHandleFor(
          federationName, objectClassName);
      auto const attribute = registry.attributeHandleFor(
          federationName, objectClassName, attributeName);
      if (!objectClass || !attribute) {
        throw std::runtime_error(
            "The public timestamped process delete server could not resolve its FOM handles.");
      }
      expectedObjectClass.store(*objectClass, std::memory_order_release);
      expectedAttribute.store(*attribute, std::memory_order_release);
      serveExpected(
          sender,
          senderHandler,
          TransportServiceOperation::join_federation_execution,
          "The public timestamped process delete server lost sender Join.");

      auto receiverConnection = listener->accept(
          nullptr,
          {"public-process-timestamped-delete-server", 0x9D02U},
          [](std::wstring) {},
          [](std::wstring) { return false; });
      ProcessTransportSession receiver(receiverConnection);
      auto receiverBaseHandler = service.handlerFor(receiver);
      auto receiverHandler = [&](TransportServiceMessage const& request) {
        auto response = receiverBaseHandler(request);
        if (request.operation == TransportServiceOperation::join_federation_execution &&
            response.status == TransportServiceStatus::ok) {
          expectedReceiverFederate.store(
              umbra::detail::decodeProcessFederationJoinResult(response.payload)
                  .federateId,
              std::memory_order_release);
        }
        return response;
      };
      serveExpected(
          receiver,
          receiverHandler,
          TransportServiceOperation::join_federation_execution,
          "The public timestamped process delete server lost receiver Join.");
      serveExpected(
          sender,
          senderHandler,
          TransportServiceOperation::get_object_class_handle,
          "The public timestamped process delete server lost sender object-class lookup.");
      serveExpected(
          sender,
          senderHandler,
          TransportServiceOperation::get_attribute_handle,
          "The public timestamped process delete server lost sender attribute lookup.");
      serveExpected(
          receiver,
          receiverHandler,
          TransportServiceOperation::get_object_class_handle,
          "The public timestamped process delete server lost receiver object-class lookup.");
      serveExpected(
          receiver,
          receiverHandler,
          TransportServiceOperation::get_attribute_handle,
          "The public timestamped process delete server lost receiver attribute lookup.");
      serveExpected(
          receiver,
          receiverHandler,
          TransportServiceOperation::subscribe_object_class_attributes,
          "The public timestamped process delete server lost receiver Subscribe.");
      serveExpected(
          sender,
          senderHandler,
          TransportServiceOperation::publish_object_class_attributes,
          "The public timestamped process delete server lost sender Publish.");
      serveExpected(
          sender,
          senderHandler,
          TransportServiceOperation::register_object_instance,
          "The public timestamped process delete server lost sender Register.");
      if (callbackModel == HLA_IMMEDIATE) {
        serveExpected(
            receiver,
            receiverHandler,
            TransportServiceOperation::get_object_class_handle,
            "The public timestamped process delete server lost immediate discovery polling.");
      } else {
        serveExpected(
            receiver,
            receiverHandler,
            TransportServiceOperation::receive_interaction,
            "The public timestamped process delete server lost receiver discovery poll.");
      }
      serveExpected(
          sender,
          senderHandler,
          TransportServiceOperation::delete_object_instance,
          "The public timestamped process delete server lost sender Delete.");
      if (callbackModel == HLA_IMMEDIATE) {
        serveExpected(
            receiver,
            receiverHandler,
            TransportServiceOperation::get_object_class_handle,
            "The public timestamped process delete server lost immediate removal polling.");
      } else {
        serveExpected(
            receiver,
            receiverHandler,
            TransportServiceOperation::receive_interaction,
            "The public timestamped process delete server lost receiver removal poll.");
      }

      auto const receiverMember = registry.memberByName(
          federationName, L"public-process-timestamped-delete-receiver");
      auto const senderMember = registry.memberByName(
          federationName, L"public-process-timestamped-delete-sender");
      auto const objectHandle = registeredObject.load(std::memory_order_acquire);
      producerKnowledgeCleared.store(
          senderMember && objectHandle != 0U &&
              !registry.knownObjectInstanceFor(
                  federationName, senderMember->id, objectHandle),
          std::memory_order_release);
      receiverKnowledgeCleared.store(
          receiverMember && objectHandle != 0U &&
              !registry.knownObjectInstanceFor(
                  federationName, receiverMember->id, objectHandle),
          std::memory_order_release);
      serveExpected(
          sender,
          senderHandler,
          TransportServiceOperation::resign_federation_execution,
          "The public timestamped process delete server lost sender Resign.");
      serveExpected(
          receiver,
          receiverHandler,
          TransportServiceOperation::resign_federation_execution,
          "The public timestamped process delete server lost receiver Resign.");
      service.detach(sender);
      service.detach(receiver);
      senderConnection->close();
      receiverConnection->close();
    } catch (...) {
      serverError = std::current_exception();
    }
  });

  RecordingFederateAmbassador senderFederate;
  auto senderRti = makeRti();
  auto receiverRti = makeRti();
  auto senderConfiguration = RtiConfiguration::createConfiguration()
                                 .withConfigurationName(
                                     L"public-process-timestamped-delete-sender")
                                 .withRtiAddress(
                                     L"tcp://127.0.0.1:" + std::to_wstring(port));
  auto receiverConfiguration = RtiConfiguration::createConfiguration()
                                   .withConfigurationName(
                                       L"public-process-timestamped-delete-receiver")
                                   .withRtiAddress(
                                       L"tcp://127.0.0.1:" + std::to_wstring(port));
  std::exception_ptr clientError;
  bool senderJoined = false;
  bool receiverJoined = false;
  try {
    REQUIRE(senderRti->connect(
                senderFederate, callbackModel, senderConfiguration)
                .addressUsed);
    senderRti->createFederationExecution(federationName, L"server-owned-fom.xml");
    senderRti->joinFederationExecution(
        L"public-process-timestamped-delete-sender",
        L"public-process-timestamped-delete-type",
        federationName);
    senderJoined = true;

    REQUIRE(receiverRti->connect(
                receiverFederate, callbackModel, receiverConfiguration)
                .addressUsed);
    receiverRti->joinFederationExecution(
        L"public-process-timestamped-delete-receiver",
        L"public-process-timestamped-delete-type",
        federationName);
    receiverJoined = true;

    auto const senderObjectClass =
        senderRti->getObjectClassHandle(objectClassNameWide);
    auto const senderAttribute =
        senderRti->getAttributeHandle(senderObjectClass, attributeNameWide);
    REQUIRE(senderObjectClass.toString() ==
            L"ObjectClassHandle(" +
                std::to_wstring(expectedObjectClass.load(std::memory_order_acquire)) +
                L")");
    REQUIRE(senderAttribute ==
            rti1516_2025::umbra_binding_detail::makeAttributeHandle(
                expectedAttribute.load(std::memory_order_acquire)));
    auto const receiverObjectClass =
        receiverRti->getObjectClassHandle(objectClassNameWide);
    auto const receiverAttribute =
        receiverRti->getAttributeHandle(receiverObjectClass, attributeNameWide);
    receiverRti->subscribeObjectClassAttributes(
        receiverObjectClass,
        rti1516_2025::AttributeHandleSet{receiverAttribute});
    senderRti->publishObjectClassAttributes(
        senderObjectClass,
        rti1516_2025::AttributeHandleSet{senderAttribute});

    auto const objectInstance = senderRti->registerObjectInstance(senderObjectClass);
    REQUIRE(objectInstance.isValid());
    if (callbackModel == HLA_IMMEDIATE) {
      static_cast<void>(receiverRti->getObjectClassHandle(objectClassNameWide));
    } else {
      for (std::size_t evokeCount = 0U;
           evokeCount < 4U && !receiverFederate.discovered;
           ++evokeCount) {
        static_cast<void>(receiverRti->evokeCallback(0.0));
      }
    }
    REQUIRE(receiverFederate.discovered);
    REQUIRE(receiverFederate.discoveredObjectInstance == objectInstance);

    auto factory = rti1516_2025::HLAlogicalTimeFactoryFactory::makeLogicalTimeFactory(
        L"HLAinteger64Time");
    auto* integerFactory =
        dynamic_cast<rti1516_2025::HLAinteger64TimeFactory*>(factory.get());
    REQUIRE(integerFactory != nullptr);
    auto timestamp = integerFactory->makeLogicalTime(5);
    REQUIRE(timestamp);
    auto const expectedTimestampEncoding = timestamp->encode();
    std::vector<std::uint8_t> const expectedTag{0x54U, 0x44U, 0x4FU};
    auto const retraction = senderRti->deleteObjectInstance(
        objectInstance,
        VariableLengthData(expectedTag.data(), expectedTag.size()),
        *timestamp);
    REQUIRE_FALSE(retraction.isValid());
    if (callbackModel == HLA_IMMEDIATE) {
      static_cast<void>(receiverRti->getObjectClassHandle(objectClassNameWide));
    } else {
      for (std::size_t evokeCount = 0U;
           evokeCount < 4U && !receiverFederate.removed;
           ++evokeCount) {
        static_cast<void>(receiverRti->evokeCallback(0.0));
      }
    }
    REQUIRE(receiverFederate.removed);
    REQUIRE(receiverFederate.removedObjectInstance == objectInstance);
    REQUIRE(receiverFederate.removedProducingFederate ==
            receiverFederate.discoveredProducingFederate);
    REQUIRE(receiverFederate.removedTag == expectedTag);
    REQUIRE(receiverFederate.timestampImplementationName == L"HLAinteger64Time");
    std::vector<std::uint8_t> expectedTimestampBytes;
    if (expectedTimestampEncoding.size() != 0U) {
      auto const* first =
          static_cast<std::uint8_t const*>(expectedTimestampEncoding.data());
      expectedTimestampBytes.assign(
          first, first + expectedTimestampEncoding.size());
    }
    REQUIRE(receiverFederate.timestampEncoding == expectedTimestampBytes);
    REQUIRE(receiverFederate.removedSentOrder == rti1516_2025::RECEIVE);
    REQUIRE(receiverFederate.removedReceivedOrder == rti1516_2025::RECEIVE);
    REQUIRE(receiverFederate.hasOptionalRetraction);
    REQUIRE_FALSE(senderFederate.removed);

    senderRti->resignFederationExecution(DELETE_OBJECTS);
    senderJoined = false;
    receiverRti->resignFederationExecution(NO_ACTION);
    receiverJoined = false;
    senderRti->disconnect();
    receiverRti->disconnect();
  } catch (...) {
    clientError = std::current_exception();
    if (senderJoined) {
      try {
        senderRti->resignFederationExecution(DELETE_OBJECTS);
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
  if (listener) {
    listener.reset();
  }
  if (server.joinable()) {
    server.join();
  }
  if (clientError) {
    std::rethrow_exception(clientError);
  }
  REQUIRE_FALSE(serverError);
  REQUIRE(deleteRecipientCount.load(std::memory_order_acquire) == 1U);
  REQUIRE(deleteMessageId.load(std::memory_order_acquire) != 0U);
  REQUIRE(producerKnowledgeCleared.load(std::memory_order_acquire));
  REQUIRE(receiverKnowledgeCleared.load(std::memory_order_acquire));
  REQUIRE(expectedSenderFederate.load(std::memory_order_acquire) != 0U);
  REQUIRE(expectedReceiverFederate.load(std::memory_order_acquire) != 0U);
  REQUIRE(registeredObject.load(std::memory_order_acquire) != 0U);
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
#endif
#if defined(UMBRA_ENABLE_EMBEDDED_FEDERATION_MANAGEMENT)
TEST_CASE(
    "RTIambassador delivers regional Attribute Relevance Advisory subscription transitions through a configured process endpoint",
    "[integration][foundation][data-distribution-management][object-management][callbacks][transport][process-boundary][public-endpoint][regional-attribute-relevance][regional-subscription-transition][2025][rti.service.get-attribute-relevance-advisory-switch][rti.service.set-attribute-relevance-advisory-switch][rti.service.create-region][rti.service.commit-region-modifications][rti.service.subscribe-object-class-attributes-with-regions][rti.service.register-object-instance-with-regions][rti.service.unsubscribe-object-class-attributes-with-regions][rti.service.subscribe-object-class-attributes-with-regions][federate.callback.turn-updates-on-for-object-instance][federate.callback.turn-updates-off-for-object-instance]") {
  auto runScenario = [](CallbackModel callbackModel) {
    class RecordingOwnerAmbassador final : public NullFederateAmbassador {
     public:
      void turnUpdatesOnForObjectInstance(
          rti1516_2025::ObjectInstanceHandle const& objectInstance,
          rti1516_2025::AttributeHandleSet const& attributes,
          std::wstring const& updateRateDesignator) override {
        ++turnedOnCount;
        turnedOnObjectInstance = objectInstance;
        turnedOnAttributes = attributes;
        turnedOnUpdateRateDesignator = updateRateDesignator;
      }

      void turnUpdatesOffForObjectInstance(
          rti1516_2025::ObjectInstanceHandle const& objectInstance,
          rti1516_2025::AttributeHandleSet const& attributes) override {
        ++turnedOffCount;
        turnedOffObjectInstance = objectInstance;
        turnedOffAttributes = attributes;
      }

      std::size_t turnedOnCount = 0U;
      rti1516_2025::ObjectInstanceHandle turnedOnObjectInstance;
      rti1516_2025::AttributeHandleSet turnedOnAttributes;
      std::optional<std::wstring> turnedOnUpdateRateDesignator;
      std::size_t turnedOffCount = 0U;
      rti1516_2025::ObjectInstanceHandle turnedOffObjectInstance;
      rti1516_2025::AttributeHandleSet turnedOffAttributes;
    } ownerFederate;
    TestFederateAmbassador receiverFederate;

    auto listener = ProcessTransportListener::listen({"127.0.0.1", 0U});
    REQUIRE(listener);
    auto const port = listener->address().port;
    REQUIRE(port != 0U);

    constexpr wchar_t const* federationName =
        L"public-process-regional-attribute-relevance-subscription-execution";
    constexpr wchar_t const* objectClassName =
        L"HLAobjectRoot.Food.Drink.Soda";
    constexpr wchar_t const* attributeName = L"Flavor";
    constexpr wchar_t const* dimensionName = L"SodaFlavor";
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
            {"public-process-regional-attribute-relevance-subscription-server", 0xA601U},
            [](std::wstring) {},
            [](std::wstring) { return false; });
        ProcessTransportSession sender(senderConnection);
        auto senderHandler = service.handlerFor(sender);
        auto serveExpected = [&](ProcessTransportSession& session,
                                 auto const& handler,
                                 TransportServiceOperation operation,
                                 char const* description) {
          if (!ProcessTransportServiceDispatcher::serveOne(
                  session,
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
            "The public regional advisory server lost Create.");
        serveExpected(
            sender,
            senderHandler,
            TransportServiceOperation::join_federation_execution,
            "The public regional advisory server lost owner Join.");
        auto receiverConnection = listener->accept(
            nullptr,
            {"public-process-regional-attribute-relevance-server", 0xA602U},
            [](std::wstring) {},
            [](std::wstring) { return false; });
        ProcessTransportSession receiver(receiverConnection);
        auto receiverHandler = service.handlerFor(receiver);
        serveExpected(
            receiver,
            receiverHandler,
            TransportServiceOperation::join_federation_execution,
            "The public regional advisory server lost receiver Join.");
        serveExpected(
            sender,
            senderHandler,
            TransportServiceOperation::set_attribute_relevance_advisory_switch,
            "The public regional advisory server lost advisory-switch Set.");
        serveExpected(
            sender,
            senderHandler,
            TransportServiceOperation::get_attribute_relevance_advisory_switch,
            "The public regional advisory server lost advisory-switch Get.");

        for (auto const operation : {
                 TransportServiceOperation::get_object_class_handle,
                 TransportServiceOperation::get_attribute_handle,
                 TransportServiceOperation::get_dimension_handle,
                 TransportServiceOperation::create_region,
                 TransportServiceOperation::set_range_bounds,
                 TransportServiceOperation::commit_region_modifications,
             }) {
          serveExpected(
              sender,
              senderHandler,
              operation,
              "The public regional advisory server lost owner DDM setup.");
        }
        for (auto const operation : {
                 TransportServiceOperation::get_object_class_handle,
                 TransportServiceOperation::get_attribute_handle,
                 TransportServiceOperation::get_dimension_handle,
                 TransportServiceOperation::create_region,
                 TransportServiceOperation::set_range_bounds,
                 TransportServiceOperation::commit_region_modifications,
                 TransportServiceOperation::subscribe_object_class_attributes_with_regions,
             }) {
          serveExpected(
              receiver,
              receiverHandler,
              operation,
              "The public regional advisory server lost receiver DDM setup.");
        }
        serveExpected(
            sender,
            senderHandler,
            TransportServiceOperation::publish_object_class_attributes,
            "The public regional advisory server lost Publish.");
        serveExpected(
            sender,
            senderHandler,
            TransportServiceOperation::register_object_instance_with_regions,
            "The public regional advisory server lost regional Register.");
        if (callbackModel == HLA_IMMEDIATE) {
          serveExpected(
              receiver,
              receiverHandler,
              TransportServiceOperation::get_object_class_handle,
              "The public regional advisory server lost immediate discovery polling.");
        } else {
          serveExpected(
              receiver,
              receiverHandler,
              TransportServiceOperation::receive_interaction,
              "The public regional advisory server lost discovery Receive.");
        }
        serveExpected(
            receiver,
            receiverHandler,
            TransportServiceOperation::unsubscribe_object_class_attributes_with_regions,
            "The public regional advisory server lost regional Unsubscribe.");
        if (callbackModel == HLA_IMMEDIATE) {
          serveExpected(
              sender,
              senderHandler,
              TransportServiceOperation::get_object_class_handle,
              "The public regional advisory server lost immediate advisory-off polling.");
        } else {
          serveExpected(
              sender,
              senderHandler,
              TransportServiceOperation::receive_interaction,
              "The public regional advisory server lost advisory-off Receive.");
        }
        serveExpected(
            receiver,
            receiverHandler,
            TransportServiceOperation::subscribe_object_class_attributes_with_regions,
            "The public regional advisory server lost regional Re-subscribe.");
        if (callbackModel == HLA_IMMEDIATE) {
          serveExpected(
              sender,
              senderHandler,
              TransportServiceOperation::get_object_class_handle,
              "The public regional advisory server lost immediate advisory-on polling.");
        } else {
          serveExpected(
              sender,
              senderHandler,
              TransportServiceOperation::receive_interaction,
              "The public regional advisory server lost advisory-on Receive.");
        }
        serveExpected(
            sender,
            senderHandler,
            TransportServiceOperation::resign_federation_execution,
            "The public regional advisory server lost owner Resign.");
        serveExpected(
            receiver,
            receiverHandler,
            TransportServiceOperation::resign_federation_execution,
            "The public regional advisory server lost receiver Resign.");
        service.detach(sender);
        service.detach(receiver);
        senderConnection->close();
        receiverConnection->close();
      } catch (...) {
        serverError = std::current_exception();
      }
    });

    auto ownerRti = makeRti();
    auto receiverRti = makeRti();
    auto configuration = RtiConfiguration::createConfiguration()
                             .withConfigurationName(
                                 L"public-process-regional-attribute-relevance-subscription-client")
                             .withRtiAddress(
                                 L"tcp://127.0.0.1:" + std::to_wstring(port));
    std::exception_ptr clientError;
    bool ownerJoined = false;
    bool receiverJoined = false;
    try {
      REQUIRE(ownerRti->connect(ownerFederate, callbackModel, configuration).addressUsed);
      ownerRti->createFederationExecution(federationName, L"server-owned-fom.xml");
      ownerRti->joinFederationExecution(
          L"public-process-regional-attribute-relevance-subscription-owner",
          L"public-process-regional-attribute-relevance-subscription-type",
          federationName);
      ownerJoined = true;

      REQUIRE(receiverRti->connect(receiverFederate, callbackModel, configuration).addressUsed);
      receiverRti->joinFederationExecution(
          L"public-process-regional-attribute-relevance-receiver",
          L"public-process-regional-attribute-relevance-type",
          federationName);
      receiverJoined = true;
      REQUIRE_NOTHROW(ownerRti->setAttributeRelevanceAdvisorySwitch(true));
      REQUIRE(ownerRti->getAttributeRelevanceAdvisorySwitch());

      auto const ownerObjectClass =
          ownerRti->getObjectClassHandle(objectClassName);
      auto const ownerAttribute =
          ownerRti->getAttributeHandle(ownerObjectClass, attributeName);
      auto const ownerDimension = ownerRti->getDimensionHandle(dimensionName);
      REQUIRE(ownerObjectClass.isValid());
      REQUIRE(ownerAttribute.isValid());
      REQUIRE(ownerDimension.isValid());

      auto const ownerRegion =
          ownerRti->createRegion(rti1516_2025::DimensionHandleSet{ownerDimension});
      REQUIRE(ownerRegion.isValid());
      REQUIRE_NOTHROW(ownerRti->setRangeBounds(
          ownerRegion,
          ownerDimension,
          rti1516_2025::RangeBounds(0UL, 1UL)));
      REQUIRE_NOTHROW(ownerRti->commitRegionModifications(
          rti1516_2025::RegionHandleSet{ownerRegion}));
      auto const receiverObjectClass =
          receiverRti->getObjectClassHandle(objectClassName);
      auto const receiverAttribute =
          receiverRti->getAttributeHandle(receiverObjectClass, attributeName);
      auto const receiverDimension = receiverRti->getDimensionHandle(dimensionName);
      REQUIRE(receiverObjectClass == ownerObjectClass);
      REQUIRE(receiverAttribute == ownerAttribute);
      REQUIRE(receiverDimension == ownerDimension);
      auto const receiverRegion =
          receiverRti->createRegion(rti1516_2025::DimensionHandleSet{receiverDimension});
      REQUIRE(receiverRegion.isValid());
      REQUIRE_NOTHROW(receiverRti->setRangeBounds(
          receiverRegion,
          receiverDimension,
          rti1516_2025::RangeBounds(0UL, 1UL)));
      REQUIRE_NOTHROW(receiverRti->commitRegionModifications(
          rti1516_2025::RegionHandleSet{receiverRegion}));
      rti1516_2025::AttributeHandleSet const ownerAttributes{ownerAttribute};
      rti1516_2025::AttributeHandleSetRegionHandleSetPairVector const receiverPairs{{
          rti1516_2025::AttributeHandleSet{receiverAttribute},
          rti1516_2025::RegionHandleSet{receiverRegion},
      }};
      REQUIRE_NOTHROW(receiverRti->subscribeObjectClassAttributesWithRegions(
          receiverObjectClass,
          receiverPairs,
          true,
          L"High"));
      REQUIRE_NOTHROW(ownerRti->publishObjectClassAttributes(
          ownerObjectClass,
          ownerAttributes));
      auto const objectInstance = ownerRti->registerObjectInstanceWithRegions(
          ownerObjectClass,
          rti1516_2025::AttributeHandleSetRegionHandleSetPairVector{{
              ownerAttributes,
              rti1516_2025::RegionHandleSet{ownerRegion},
          }});
      REQUIRE(objectInstance.isValid());
      if (callbackModel == HLA_IMMEDIATE) {
        static_cast<void>(receiverRti->getObjectClassHandle(objectClassName));
      } else {
        static_cast<void>(receiverRti->evokeCallback(0.0));
      }
      if (callbackModel == HLA_EVOKED) {
        static_cast<void>(ownerRti->evokeCallback(0.0));
      }
      REQUIRE(ownerFederate.turnedOffCount == 0U);
      REQUIRE(ownerFederate.turnedOnCount == 1U);
      REQUIRE(ownerFederate.turnedOnObjectInstance == objectInstance);
      REQUIRE(ownerFederate.turnedOnAttributes == ownerAttributes);
      REQUIRE(ownerFederate.turnedOnUpdateRateDesignator ==
              std::optional<std::wstring>{L"High"});
      // The initial registration/discovery advisory is covered here; clear
      // it before isolating the subsequent subscription transition pair.
      ownerFederate.turnedOnCount = 0U;
      ownerFederate.turnedOnObjectInstance = {};
      ownerFederate.turnedOnAttributes.clear();
      ownerFederate.turnedOnUpdateRateDesignator.reset();
      REQUIRE(ownerFederate.turnedOffCount == 0U);
      REQUIRE(ownerFederate.turnedOnCount == 0U);

      // Removing the regional subscription crosses the edge to Off.
      REQUIRE_NOTHROW(receiverRti->unsubscribeObjectClassAttributesWithRegions(
          receiverObjectClass,
          receiverPairs));
      if (callbackModel == HLA_IMMEDIATE) {
        static_cast<void>(ownerRti->getObjectClassHandle(objectClassName));
      } else {
        static_cast<void>(ownerRti->evokeCallback(0.0));
      }
      REQUIRE(ownerFederate.turnedOffCount == 1U);
      REQUIRE(ownerFederate.turnedOffObjectInstance == objectInstance);
      REQUIRE(ownerFederate.turnedOffAttributes == ownerAttributes);
      REQUIRE(ownerFederate.turnedOnCount == 0U);

      // Restoring the regional subscription crosses the edge to On and retains High.
      REQUIRE_NOTHROW(receiverRti->subscribeObjectClassAttributesWithRegions(
          receiverObjectClass,
          receiverPairs,
          true,
          L"High"));
      if (callbackModel == HLA_IMMEDIATE) {
        static_cast<void>(ownerRti->getObjectClassHandle(objectClassName));
      } else {
        static_cast<void>(ownerRti->evokeCallback(0.0));
      }
      REQUIRE(ownerFederate.turnedOnCount == 1U);
      REQUIRE(ownerFederate.turnedOnObjectInstance == objectInstance);
      REQUIRE(ownerFederate.turnedOnAttributes == ownerAttributes);
      REQUIRE(ownerFederate.turnedOnUpdateRateDesignator ==
              std::optional<std::wstring>{L"High"});

      ownerRti->resignFederationExecution(DELETE_OBJECTS);
      ownerJoined = false;
      receiverRti->resignFederationExecution(NO_ACTION);
      receiverJoined = false;
      ownerRti->disconnect();
      receiverRti->disconnect();
    } catch (...) {
      clientError = std::current_exception();
      if (ownerJoined) {
        try {
          ownerRti->resignFederationExecution(DELETE_OBJECTS);
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
        ownerRti->disconnect();
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
    if (clientError) {
      std::rethrow_exception(clientError);
    }
    REQUIRE_FALSE(serverError);
    REQUIRE_FALSE(ownerJoined);
    REQUIRE_FALSE(receiverJoined);
  };

  SECTION("HLA_EVOKED") {
    runScenario(HLA_EVOKED);
  }
  SECTION("HLA_IMMEDIATE") {
    runScenario(HLA_IMMEDIATE);
  }
}
#endif
#if defined(UMBRA_ENABLE_EMBEDDED_FEDERATION_MANAGEMENT)
TEST_CASE(
    "RTIambassador delivers regional Attribute Relevance Advisory transitions through a configured process endpoint",
    "[integration][foundation][data-distribution-management][object-management][callbacks][transport][process-boundary][public-endpoint][regional-attribute-relevance][2025][rti.service.get-attribute-relevance-advisory-switch][rti.service.set-attribute-relevance-advisory-switch][rti.service.create-region][rti.service.commit-region-modifications][rti.service.subscribe-object-class-attributes-with-regions][rti.service.register-object-instance-with-regions][rti.service.associate-regions-for-updates][rti.service.unassociate-regions-for-updates][federate.callback.turn-updates-on-for-object-instance][federate.callback.turn-updates-off-for-object-instance]") {
  auto runScenario = [](CallbackModel callbackModel) {
    class RecordingOwnerAmbassador final : public NullFederateAmbassador {
     public:
      void turnUpdatesOnForObjectInstance(
          rti1516_2025::ObjectInstanceHandle const& objectInstance,
          rti1516_2025::AttributeHandleSet const& attributes,
          std::wstring const& updateRateDesignator) override {
        ++turnedOnCount;
        turnedOnObjectInstance = objectInstance;
        turnedOnAttributes = attributes;
        turnedOnUpdateRateDesignator = updateRateDesignator;
      }

      void turnUpdatesOffForObjectInstance(
          rti1516_2025::ObjectInstanceHandle const& objectInstance,
          rti1516_2025::AttributeHandleSet const& attributes) override {
        ++turnedOffCount;
        turnedOffObjectInstance = objectInstance;
        turnedOffAttributes = attributes;
      }

      std::size_t turnedOnCount = 0U;
      rti1516_2025::ObjectInstanceHandle turnedOnObjectInstance;
      rti1516_2025::AttributeHandleSet turnedOnAttributes;
      std::optional<std::wstring> turnedOnUpdateRateDesignator;
      std::size_t turnedOffCount = 0U;
      rti1516_2025::ObjectInstanceHandle turnedOffObjectInstance;
      rti1516_2025::AttributeHandleSet turnedOffAttributes;
    } ownerFederate;
    TestFederateAmbassador receiverFederate;

    auto listener = ProcessTransportListener::listen({"127.0.0.1", 0U});
    REQUIRE(listener);
    auto const port = listener->address().port;
    REQUIRE(port != 0U);

    constexpr wchar_t const* federationName =
        L"public-process-regional-attribute-relevance-execution";
    constexpr wchar_t const* objectClassName =
        L"HLAobjectRoot.Food.Drink.Soda";
    constexpr wchar_t const* attributeName = L"Flavor";
    constexpr wchar_t const* dimensionName = L"SodaFlavor";
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
            {"public-process-regional-attribute-relevance-server", 0xA601U},
            [](std::wstring) {},
            [](std::wstring) { return false; });
        ProcessTransportSession sender(senderConnection);
        auto senderHandler = service.handlerFor(sender);
        auto serveExpected = [&](ProcessTransportSession& session,
                                 auto const& handler,
                                 TransportServiceOperation operation,
                                 char const* description) {
          if (!ProcessTransportServiceDispatcher::serveOne(
                  session,
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
            "The public regional advisory server lost Create.");
        serveExpected(
            sender,
            senderHandler,
            TransportServiceOperation::join_federation_execution,
            "The public regional advisory server lost owner Join.");
        auto receiverConnection = listener->accept(
            nullptr,
            {"public-process-regional-attribute-relevance-server", 0xA602U},
            [](std::wstring) {},
            [](std::wstring) { return false; });
        ProcessTransportSession receiver(receiverConnection);
        auto receiverHandler = service.handlerFor(receiver);
        serveExpected(
            receiver,
            receiverHandler,
            TransportServiceOperation::join_federation_execution,
            "The public regional advisory server lost receiver Join.");
        serveExpected(
            sender,
            senderHandler,
            TransportServiceOperation::set_attribute_relevance_advisory_switch,
            "The public regional advisory server lost advisory-switch Set.");
        serveExpected(
            sender,
            senderHandler,
            TransportServiceOperation::get_attribute_relevance_advisory_switch,
            "The public regional advisory server lost advisory-switch Get.");

        for (auto const operation : {
                 TransportServiceOperation::get_object_class_handle,
                 TransportServiceOperation::get_attribute_handle,
                 TransportServiceOperation::get_dimension_handle,
                 TransportServiceOperation::create_region,
                 TransportServiceOperation::set_range_bounds,
                 TransportServiceOperation::commit_region_modifications,
                 TransportServiceOperation::create_region,
                 TransportServiceOperation::set_range_bounds,
                 TransportServiceOperation::commit_region_modifications,
             }) {
          serveExpected(
              sender,
              senderHandler,
              operation,
              "The public regional advisory server lost owner DDM setup.");
        }
        for (auto const operation : {
                 TransportServiceOperation::get_object_class_handle,
                 TransportServiceOperation::get_attribute_handle,
                 TransportServiceOperation::get_dimension_handle,
                 TransportServiceOperation::create_region,
                 TransportServiceOperation::set_range_bounds,
                 TransportServiceOperation::commit_region_modifications,
                 TransportServiceOperation::subscribe_object_class_attributes_with_regions,
             }) {
          serveExpected(
              receiver,
              receiverHandler,
              operation,
              "The public regional advisory server lost receiver DDM setup.");
        }
        serveExpected(
            sender,
            senderHandler,
            TransportServiceOperation::publish_object_class_attributes,
            "The public regional advisory server lost Publish.");
        serveExpected(
            sender,
            senderHandler,
            TransportServiceOperation::register_object_instance_with_regions,
            "The public regional advisory server lost regional Register.");
        if (callbackModel == HLA_IMMEDIATE) {
          serveExpected(
              receiver,
              receiverHandler,
              TransportServiceOperation::get_object_class_handle,
              "The public regional advisory server lost immediate discovery polling.");
        } else {
          serveExpected(
              receiver,
              receiverHandler,
              TransportServiceOperation::receive_interaction,
              "The public regional advisory server lost discovery Receive.");
        }
        serveExpected(
            sender,
            senderHandler,
            TransportServiceOperation::associate_regions_for_updates,
            "The public regional advisory server lost disjoint-region Associate.");
        serveExpected(
            sender,
            senderHandler,
            TransportServiceOperation::unassociate_regions_for_updates,
            "The public regional advisory server lost source-region Unassociate.");
        serveExpected(
            sender,
            senderHandler,
            TransportServiceOperation::associate_regions_for_updates,
            "The public regional advisory server lost source-region Associate.");
        serveExpected(
            sender,
            senderHandler,
            TransportServiceOperation::resign_federation_execution,
            "The public regional advisory server lost owner Resign.");
        serveExpected(
            receiver,
            receiverHandler,
            TransportServiceOperation::resign_federation_execution,
            "The public regional advisory server lost receiver Resign.");
        service.detach(sender);
        service.detach(receiver);
        senderConnection->close();
        receiverConnection->close();
      } catch (...) {
        serverError = std::current_exception();
      }
    });

    auto ownerRti = makeRti();
    auto receiverRti = makeRti();
    auto configuration = RtiConfiguration::createConfiguration()
                             .withConfigurationName(
                                 L"public-process-regional-attribute-relevance-client")
                             .withRtiAddress(
                                 L"tcp://127.0.0.1:" + std::to_wstring(port));
    std::exception_ptr clientError;
    bool ownerJoined = false;
    bool receiverJoined = false;
    try {
      REQUIRE(ownerRti->connect(ownerFederate, callbackModel, configuration).addressUsed);
      ownerRti->createFederationExecution(federationName, L"server-owned-fom.xml");
      ownerRti->joinFederationExecution(
          L"public-process-regional-attribute-relevance-owner",
          L"public-process-regional-attribute-relevance-type",
          federationName);
      ownerJoined = true;

      REQUIRE(receiverRti->connect(receiverFederate, callbackModel, configuration).addressUsed);
      receiverRti->joinFederationExecution(
          L"public-process-regional-attribute-relevance-receiver",
          L"public-process-regional-attribute-relevance-type",
          federationName);
      receiverJoined = true;
      REQUIRE_NOTHROW(ownerRti->setAttributeRelevanceAdvisorySwitch(true));
      REQUIRE(ownerRti->getAttributeRelevanceAdvisorySwitch());

      auto const ownerObjectClass =
          ownerRti->getObjectClassHandle(objectClassName);
      auto const ownerAttribute =
          ownerRti->getAttributeHandle(ownerObjectClass, attributeName);
      auto const ownerDimension = ownerRti->getDimensionHandle(dimensionName);
      REQUIRE(ownerObjectClass.isValid());
      REQUIRE(ownerAttribute.isValid());
      REQUIRE(ownerDimension.isValid());

      auto const ownerRegion =
          ownerRti->createRegion(rti1516_2025::DimensionHandleSet{ownerDimension});
      REQUIRE(ownerRegion.isValid());
      REQUIRE_NOTHROW(ownerRti->setRangeBounds(
          ownerRegion,
          ownerDimension,
          rti1516_2025::RangeBounds(0UL, 1UL)));
      REQUIRE_NOTHROW(ownerRti->commitRegionModifications(
          rti1516_2025::RegionHandleSet{ownerRegion}));
      auto const disjointOwnerRegion =
          ownerRti->createRegion(rti1516_2025::DimensionHandleSet{ownerDimension});
      REQUIRE(disjointOwnerRegion.isValid());
      REQUIRE_NOTHROW(ownerRti->setRangeBounds(
          disjointOwnerRegion,
          ownerDimension,
          rti1516_2025::RangeBounds(2UL, 3UL)));
      REQUIRE_NOTHROW(ownerRti->commitRegionModifications(
          rti1516_2025::RegionHandleSet{disjointOwnerRegion}));

      auto const receiverObjectClass =
          receiverRti->getObjectClassHandle(objectClassName);
      auto const receiverAttribute =
          receiverRti->getAttributeHandle(receiverObjectClass, attributeName);
      auto const receiverDimension = receiverRti->getDimensionHandle(dimensionName);
      REQUIRE(receiverObjectClass == ownerObjectClass);
      REQUIRE(receiverAttribute == ownerAttribute);
      REQUIRE(receiverDimension == ownerDimension);
      auto const receiverRegion =
          receiverRti->createRegion(rti1516_2025::DimensionHandleSet{receiverDimension});
      REQUIRE(receiverRegion.isValid());
      REQUIRE_NOTHROW(receiverRti->setRangeBounds(
          receiverRegion,
          receiverDimension,
          rti1516_2025::RangeBounds(0UL, 1UL)));
      REQUIRE_NOTHROW(receiverRti->commitRegionModifications(
          rti1516_2025::RegionHandleSet{receiverRegion}));

      rti1516_2025::AttributeHandleSet const ownerAttributes{ownerAttribute};
      rti1516_2025::AttributeHandleSetRegionHandleSetPairVector const receiverPairs{{
          rti1516_2025::AttributeHandleSet{receiverAttribute},
          rti1516_2025::RegionHandleSet{receiverRegion},
      }};
      REQUIRE_NOTHROW(receiverRti->subscribeObjectClassAttributesWithRegions(
          receiverObjectClass,
          receiverPairs,
          true,
          L"High"));
      REQUIRE_NOTHROW(ownerRti->publishObjectClassAttributes(
          ownerObjectClass,
          ownerAttributes));
      auto const objectInstance = ownerRti->registerObjectInstanceWithRegions(
          ownerObjectClass,
          rti1516_2025::AttributeHandleSetRegionHandleSetPairVector{{
              ownerAttributes,
              rti1516_2025::RegionHandleSet{ownerRegion},
          }});
      REQUIRE(objectInstance.isValid());
      if (callbackModel == HLA_IMMEDIATE) {
        static_cast<void>(receiverRti->getObjectClassHandle(objectClassName));
      } else {
        static_cast<void>(receiverRti->evokeCallback(0.0));
      }

      if (callbackModel == HLA_EVOKED) {
        static_cast<void>(ownerRti->evokeCallback(0.0));
      }
      REQUIRE(ownerFederate.turnedOffCount == 0U);
      REQUIRE(ownerFederate.turnedOnCount == 1U);
      REQUIRE(ownerFederate.turnedOnObjectInstance == objectInstance);
      REQUIRE(ownerFederate.turnedOnAttributes == ownerAttributes);
      REQUIRE(ownerFederate.turnedOnUpdateRateDesignator ==
              std::optional<std::wstring>{L"High"});
      ownerFederate.turnedOnCount = 0U;
      ownerFederate.turnedOnObjectInstance = {};
      ownerFederate.turnedOnAttributes.clear();
      ownerFederate.turnedOnUpdateRateDesignator.reset();

      // A disjoint association does not change effective relevance while the
      // original overlap remains active.
      REQUIRE_NOTHROW(ownerRti->associateRegionsForUpdates(
          objectInstance,
          rti1516_2025::AttributeHandleSetRegionHandleSetPairVector{{
              ownerAttributes,
              rti1516_2025::RegionHandleSet{disjointOwnerRegion},
          }}));
      REQUIRE(ownerFederate.turnedOffCount == 0U);
      REQUIRE(ownerFederate.turnedOnCount == 0U);

      // Removing the last overlapping association crosses the edge to Off.
      REQUIRE_NOTHROW(ownerRti->unassociateRegionsForUpdates(
          objectInstance,
          rti1516_2025::AttributeHandleSetRegionHandleSetPairVector{{
              ownerAttributes,
              rti1516_2025::RegionHandleSet{ownerRegion},
          }}));
      if (callbackModel == HLA_EVOKED) {
        static_cast<void>(ownerRti->evokeCallback(0.0));
      }
      REQUIRE(ownerFederate.turnedOffCount == 1U);
      REQUIRE(ownerFederate.turnedOffObjectInstance == objectInstance);
      REQUIRE(ownerFederate.turnedOffAttributes == ownerAttributes);
      REQUIRE(ownerFederate.turnedOnCount == 0U);

      // Restoring the overlap crosses the edge to On and retains High.
      REQUIRE_NOTHROW(ownerRti->associateRegionsForUpdates(
          objectInstance,
          rti1516_2025::AttributeHandleSetRegionHandleSetPairVector{{
              ownerAttributes,
              rti1516_2025::RegionHandleSet{ownerRegion},
          }}));
      if (callbackModel == HLA_EVOKED) {
        static_cast<void>(ownerRti->evokeCallback(0.0));
      }
      REQUIRE(ownerFederate.turnedOnCount == 1U);
      REQUIRE(ownerFederate.turnedOnObjectInstance == objectInstance);
      REQUIRE(ownerFederate.turnedOnAttributes == ownerAttributes);
      REQUIRE(ownerFederate.turnedOnUpdateRateDesignator ==
              std::optional<std::wstring>{L"High"});

      ownerRti->resignFederationExecution(DELETE_OBJECTS);
      ownerJoined = false;
      receiverRti->resignFederationExecution(NO_ACTION);
      receiverJoined = false;
      ownerRti->disconnect();
      receiverRti->disconnect();
    } catch (...) {
      clientError = std::current_exception();
      if (ownerJoined) {
        try {
          ownerRti->resignFederationExecution(DELETE_OBJECTS);
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
        ownerRti->disconnect();
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
    if (clientError) {
      std::rethrow_exception(clientError);
    }
    REQUIRE_FALSE(serverError);
    REQUIRE_FALSE(ownerJoined);
    REQUIRE_FALSE(receiverJoined);
  };

  SECTION("HLA_EVOKED") {
    runScenario(HLA_EVOKED);
  }
  SECTION("HLA_IMMEDIATE") {
    runScenario(HLA_IMMEDIATE);
  }
}
#endif

#if defined(UMBRA_ENABLE_EMBEDDED_FEDERATION_MANAGEMENT)
TEST_CASE(
    "RTIambassador delivers an initial regional Attribute Relevance Advisory after discovery through a configured process endpoint",
    "[integration][foundation][data-distribution-management][object-management][callbacks][transport][process-boundary][public-endpoint][regional-attribute-relevance][regional-registration-initial-advisory][2025][rti.service.get-attribute-relevance-advisory-switch][rti.service.set-attribute-relevance-advisory-switch][rti.service.create-region][rti.service.commit-region-modifications][rti.service.subscribe-object-class-attributes-with-regions][rti.service.register-object-instance-with-regions][federate.callback.discover-object-instance][federate.callback.turn-updates-on-for-object-instance]") {
  auto runScenario = [](CallbackModel callbackModel) {
    class RecordingOwnerAmbassador final : public NullFederateAmbassador {
     public:
      void turnUpdatesOnForObjectInstance(
          rti1516_2025::ObjectInstanceHandle const& objectInstance,
          rti1516_2025::AttributeHandleSet const& attributes) override {
        ++turnedOnCount;
        turnedOnObjectInstance = objectInstance;
        turnedOnAttributes = attributes;
        turnedOnUpdateRateDesignator.reset();
      }

      void turnUpdatesOnForObjectInstance(
          rti1516_2025::ObjectInstanceHandle const& objectInstance,
          rti1516_2025::AttributeHandleSet const& attributes,
          std::wstring const& updateRateDesignator) override {
        ++turnedOnCount;
        turnedOnObjectInstance = objectInstance;
        turnedOnAttributes = attributes;
        turnedOnUpdateRateDesignator = updateRateDesignator;
      }

      std::size_t turnedOnCount = 0U;
      rti1516_2025::ObjectInstanceHandle turnedOnObjectInstance;
      rti1516_2025::AttributeHandleSet turnedOnAttributes;
      std::optional<std::wstring> turnedOnUpdateRateDesignator;
    } ownerFederate;
    class RecordingReceiverAmbassador final : public NullFederateAmbassador {
     public:
      void discoverObjectInstance(
          rti1516_2025::ObjectInstanceHandle const& objectInstance,
          rti1516_2025::ObjectClassHandle const& objectClass,
          std::wstring const& objectInstanceName,
          rti1516_2025::FederateHandle const& producingFederate) override {
        discovered = true;
        discoveredObjectInstance = objectInstance;
        discoveredObjectClass = objectClass;
        discoveredObjectInstanceName = objectInstanceName;
        discoveredProducingFederate = producingFederate;
      }

      bool discovered = false;
      rti1516_2025::ObjectInstanceHandle discoveredObjectInstance;
      rti1516_2025::ObjectClassHandle discoveredObjectClass;
      std::wstring discoveredObjectInstanceName;
      rti1516_2025::FederateHandle discoveredProducingFederate;
    } receiverFederate;

    auto listener = ProcessTransportListener::listen({"127.0.0.1", 0U});
    REQUIRE(listener);
    auto const port = listener->address().port;
    REQUIRE(port != 0U);

    constexpr wchar_t const* federationName =
        L"public-process-regional-attribute-relevance-initial-execution";
    constexpr wchar_t const* objectClassName = L"HLAobjectRoot.Food.Drink.Soda";
    constexpr wchar_t const* attributeName = L"Flavor";
    constexpr wchar_t const* dimensionName = L"SodaFlavor";
    std::exception_ptr serverError;
    std::thread server([&] {
      try {
        EmbeddedFederationRegistry registry;
        ProcessFederationService service(
            registry,
            composedProcessDefinition(),
            ProcessFederationServiceOptions{true});
        auto ownerConnection = listener->accept(
            nullptr,
            {"public-process-regional-attribute-relevance-initial-server", 0xA701U},
            [](std::wstring) {},
            [](std::wstring) { return false; });
        ProcessTransportSession owner(ownerConnection);
        auto ownerHandler = service.handlerFor(owner);
        auto serveExpected = [&](ProcessTransportSession& session,
                                 auto const& handler,
                                 TransportServiceOperation operation,
                                 char const* description) {
          if (!ProcessTransportServiceDispatcher::serveOne(
                  session,
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
            owner,
            ownerHandler,
            TransportServiceOperation::create_federation_execution,
            "The initial regional advisory server lost Create.");
        serveExpected(
            owner,
            ownerHandler,
            TransportServiceOperation::join_federation_execution,
            "The initial regional advisory server lost owner Join.");
        auto receiverConnection = listener->accept(
            nullptr,
            {"public-process-regional-attribute-relevance-initial-receiver", 0xA702U},
            [](std::wstring) {},
            [](std::wstring) { return false; });
        ProcessTransportSession receiver(receiverConnection);
        auto receiverHandler = service.handlerFor(receiver);
        serveExpected(
            receiver,
            receiverHandler,
            TransportServiceOperation::join_federation_execution,
            "The initial regional advisory server lost receiver Join.");
        serveExpected(
            owner,
            ownerHandler,
            TransportServiceOperation::set_attribute_relevance_advisory_switch,
            "The initial regional advisory server lost advisory-switch Set.");
        serveExpected(
            owner,
            ownerHandler,
            TransportServiceOperation::get_attribute_relevance_advisory_switch,
            "The initial regional advisory server lost advisory-switch Get.");
        for (auto const operation : {
                 TransportServiceOperation::get_object_class_handle,
                 TransportServiceOperation::get_attribute_handle,
                 TransportServiceOperation::get_dimension_handle,
                 TransportServiceOperation::create_region,
                 TransportServiceOperation::set_range_bounds,
                 TransportServiceOperation::commit_region_modifications,
             }) {
          serveExpected(
              owner,
              ownerHandler,
              operation,
              "The initial regional advisory server lost owner DDM setup.");
        }
        for (auto const operation : {
                 TransportServiceOperation::get_object_class_handle,
                 TransportServiceOperation::get_attribute_handle,
                 TransportServiceOperation::get_dimension_handle,
                 TransportServiceOperation::create_region,
                 TransportServiceOperation::set_range_bounds,
                 TransportServiceOperation::commit_region_modifications,
                 TransportServiceOperation::subscribe_object_class_attributes_with_regions,
             }) {
          serveExpected(
              receiver,
              receiverHandler,
              operation,
              "The initial regional advisory server lost receiver DDM setup.");
        }
        serveExpected(
            owner,
            ownerHandler,
            TransportServiceOperation::publish_object_class_attributes,
            "The initial regional advisory server lost Publish.");
        serveExpected(
            owner,
            ownerHandler,
            TransportServiceOperation::register_object_instance_with_regions,
            "The initial regional advisory server lost regional Register.");
        if (callbackModel == HLA_IMMEDIATE) {
          serveExpected(
              receiver,
              receiverHandler,
              TransportServiceOperation::get_object_class_handle,
              "The initial regional advisory server lost immediate discovery polling.");
        } else {
          serveExpected(
              receiver,
              receiverHandler,
              TransportServiceOperation::receive_interaction,
              "The initial regional advisory server lost discovery Receive.");
        }
        serveExpected(
            owner,
            ownerHandler,
            TransportServiceOperation::resign_federation_execution,
            "The initial regional advisory server lost owner Resign.");
        serveExpected(
            receiver,
            receiverHandler,
            TransportServiceOperation::resign_federation_execution,
            "The initial regional advisory server lost receiver Resign.");
        service.detach(owner);
        service.detach(receiver);
        ownerConnection->close();
        receiverConnection->close();
      } catch (...) {
        serverError = std::current_exception();
      }
    });

    auto ownerRti = makeRti();
    auto receiverRti = makeRti();
    auto configuration = RtiConfiguration::createConfiguration()
                             .withConfigurationName(
                                 L"public-process-regional-attribute-relevance-initial-client")
                             .withRtiAddress(
                                 L"tcp://127.0.0.1:" + std::to_wstring(port));
    std::exception_ptr clientError;
    bool ownerJoined = false;
    bool receiverJoined = false;
    try {
      REQUIRE(ownerRti->connect(ownerFederate, callbackModel, configuration).addressUsed);
      ownerRti->createFederationExecution(federationName, L"server-owned-fom.xml");
      ownerRti->joinFederationExecution(
          L"public-process-regional-attribute-relevance-initial-owner",
          L"public-process-regional-attribute-relevance-initial-owner-type",
          federationName);
      ownerJoined = true;

      REQUIRE(receiverRti->connect(receiverFederate, callbackModel, configuration).addressUsed);
      receiverRti->joinFederationExecution(
          L"public-process-regional-attribute-relevance-initial-receiver",
          L"public-process-regional-attribute-relevance-initial-receiver-type",
          federationName);
      receiverJoined = true;
      REQUIRE_NOTHROW(ownerRti->setAttributeRelevanceAdvisorySwitch(true));
      REQUIRE(ownerRti->getAttributeRelevanceAdvisorySwitch());

      auto const ownerObjectClass = ownerRti->getObjectClassHandle(objectClassName);
      auto const ownerAttribute = ownerRti->getAttributeHandle(
          ownerObjectClass, attributeName);
      auto const ownerDimension = ownerRti->getDimensionHandle(dimensionName);
      REQUIRE(ownerObjectClass.isValid());
      REQUIRE(ownerAttribute.isValid());
      REQUIRE(ownerDimension.isValid());
      auto const ownerRegion = ownerRti->createRegion(
          rti1516_2025::DimensionHandleSet{ownerDimension});
      REQUIRE(ownerRegion.isValid());
      REQUIRE_NOTHROW(ownerRti->setRangeBounds(
          ownerRegion, ownerDimension, rti1516_2025::RangeBounds(0UL, 1UL)));
      REQUIRE_NOTHROW(ownerRti->commitRegionModifications(
          rti1516_2025::RegionHandleSet{ownerRegion}));

      auto const receiverObjectClass = receiverRti->getObjectClassHandle(objectClassName);
      auto const receiverAttribute = receiverRti->getAttributeHandle(
          receiverObjectClass, attributeName);
      auto const receiverDimension = receiverRti->getDimensionHandle(dimensionName);
      REQUIRE(receiverObjectClass == ownerObjectClass);
      REQUIRE(receiverAttribute == ownerAttribute);
      REQUIRE(receiverDimension == ownerDimension);
      auto const receiverRegion = receiverRti->createRegion(
          rti1516_2025::DimensionHandleSet{receiverDimension});
      REQUIRE(receiverRegion.isValid());
      REQUIRE_NOTHROW(receiverRti->setRangeBounds(
          receiverRegion, receiverDimension, rti1516_2025::RangeBounds(0UL, 1UL)));
      REQUIRE_NOTHROW(receiverRti->commitRegionModifications(
          rti1516_2025::RegionHandleSet{receiverRegion}));
      rti1516_2025::AttributeHandleSet const ownerAttributes{ownerAttribute};
      rti1516_2025::AttributeHandleSetRegionHandleSetPairVector const receiverPairs{{
          rti1516_2025::AttributeHandleSet{receiverAttribute},
          rti1516_2025::RegionHandleSet{receiverRegion},
      }};
      REQUIRE_NOTHROW(receiverRti->subscribeObjectClassAttributesWithRegions(
          receiverObjectClass, receiverPairs, true, L"High"));
      REQUIRE_NOTHROW(ownerRti->publishObjectClassAttributes(
          ownerObjectClass, ownerAttributes));
      auto const objectInstance = ownerRti->registerObjectInstanceWithRegions(
          ownerObjectClass,
          rti1516_2025::AttributeHandleSetRegionHandleSetPairVector{{
              ownerAttributes,
              rti1516_2025::RegionHandleSet{ownerRegion},
          }});
      REQUIRE(objectInstance.isValid());
      // In the process profile the registration response carries the
      // owner-directed advisory as an unsolicited event. Immediate callbacks
      // dispatch it during that response; evoked callbacks retain it locally
      // until this explicit Evoke, with no second network poll required.
      if (callbackModel == HLA_EVOKED) {
        static_cast<void>(ownerRti->evokeCallback(0.0));
      }
      REQUIRE(ownerFederate.turnedOnCount == 1U);
      REQUIRE(ownerFederate.turnedOnObjectInstance == objectInstance);
      REQUIRE(ownerFederate.turnedOnAttributes == ownerAttributes);
      REQUIRE(ownerFederate.turnedOnUpdateRateDesignator ==
              std::optional<std::wstring>{L"High"});
      if (callbackModel == HLA_IMMEDIATE) {
        static_cast<void>(receiverRti->getObjectClassHandle(objectClassName));
      } else {
        static_cast<void>(receiverRti->evokeCallback(0.0));
      }
      REQUIRE(receiverFederate.discovered);
      REQUIRE(receiverFederate.discoveredObjectInstance == objectInstance);
      REQUIRE(receiverFederate.discoveredObjectClass == receiverObjectClass);
      REQUIRE(receiverFederate.discoveredProducingFederate.isValid());

      ownerRti->resignFederationExecution(DELETE_OBJECTS);
      ownerJoined = false;
      receiverRti->resignFederationExecution(NO_ACTION);
      receiverJoined = false;
      ownerRti->disconnect();
      receiverRti->disconnect();
    } catch (...) {
      clientError = std::current_exception();
      if (ownerJoined) {
        try {
          ownerRti->resignFederationExecution(DELETE_OBJECTS);
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
        ownerRti->disconnect();
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
    if (clientError) {
      std::rethrow_exception(clientError);
    }
    REQUIRE_FALSE(serverError);
    REQUIRE_FALSE(ownerJoined);
    REQUIRE_FALSE(receiverJoined);
  };

  SECTION("HLA_EVOKED") {
    runScenario(HLA_EVOKED);
  }
  SECTION("HLA_IMMEDIATE") {
    runScenario(HLA_IMMEDIATE);
  }
}

TEST_CASE(
    "RTIambassador reissues Attribute Relevance Advisory callbacks when the active update rate changes through a configured process endpoint",
    "[integration][foundation][object-management][data-distribution-management][callbacks][callback-immediate][transport][process-boundary][public-endpoint][attribute-relevance-advisory][update-rate-reissue][2025][rti.service.get-attribute-relevance-advisory-switch][rti.service.set-attribute-relevance-advisory-switch][rti.service.publish-object-class-attributes][rti.service.register-object-instance][rti.service.subscribe-object-class-attributes][rti.service.unsubscribe-object-class-attributes][federate.callback.discover-object-instance][federate.callback.turn-updates-on-for-object-instance][federate.callback.turn-updates-off-for-object-instance]") {
  auto runScenario = [](CallbackModel callbackModel) {
    class RecordingOwnerAmbassador final : public NullFederateAmbassador {
     public:
      void turnUpdatesOnForObjectInstance(
          rti1516_2025::ObjectInstanceHandle const& objectInstance,
          rti1516_2025::AttributeHandleSet const& attributes) override {
        ++turnedOnCount;
        turnedOnObjectInstance = objectInstance;
        turnedOnAttributes = attributes;
        turnedOnRates.emplace_back(std::nullopt);
      }

      void turnUpdatesOnForObjectInstance(
          rti1516_2025::ObjectInstanceHandle const& objectInstance,
          rti1516_2025::AttributeHandleSet const& attributes,
          std::wstring const& updateRateDesignator) override {
        ++turnedOnCount;
        turnedOnObjectInstance = objectInstance;
        turnedOnAttributes = attributes;
        turnedOnRates.emplace_back(updateRateDesignator);
      }

      void turnUpdatesOffForObjectInstance(
          rti1516_2025::ObjectInstanceHandle const& objectInstance,
          rti1516_2025::AttributeHandleSet const& attributes) override {
        ++turnedOffCount;
        turnedOffObjectInstance = objectInstance;
        turnedOffAttributes = attributes;
      }

      std::size_t turnedOnCount = 0U;
      rti1516_2025::ObjectInstanceHandle turnedOnObjectInstance;
      rti1516_2025::AttributeHandleSet turnedOnAttributes;
      std::vector<std::optional<std::wstring>> turnedOnRates;
      std::size_t turnedOffCount = 0U;
      rti1516_2025::ObjectInstanceHandle turnedOffObjectInstance;
      rti1516_2025::AttributeHandleSet turnedOffAttributes;
    } ownerFederate;

    class RecordingReceiverAmbassador final : public NullFederateAmbassador {
     public:
      void discoverObjectInstance(
          rti1516_2025::ObjectInstanceHandle const& objectInstance,
          rti1516_2025::ObjectClassHandle const& objectClass,
          std::wstring const& objectInstanceName,
          rti1516_2025::FederateHandle const& producingFederate) override {
        discovered = true;
        discoveredObjectInstance = objectInstance;
        discoveredObjectClass = objectClass;
        discoveredObjectInstanceName = objectInstanceName;
        discoveredProducingFederate = producingFederate;
      }

      bool discovered = false;
      rti1516_2025::ObjectInstanceHandle discoveredObjectInstance;
      rti1516_2025::ObjectClassHandle discoveredObjectClass;
      std::wstring discoveredObjectInstanceName;
      rti1516_2025::FederateHandle discoveredProducingFederate;
    } firstReceiverFederate, secondReceiverFederate;

    auto listener = ProcessTransportListener::listen({"127.0.0.1", 0U});
    REQUIRE(listener);
    auto const port = listener->address().port;
    REQUIRE(port != 0U);

    constexpr wchar_t const* federationName =
        L"public-process-attribute-relevance-rate-reissue-execution";
    constexpr char const* objectClassName = "HLAobjectRoot.Employee";
    constexpr wchar_t const* objectClassNameWide = L"HLAobjectRoot.Employee";
    constexpr char const* nameAttributeName = "Name";
    constexpr wchar_t const* nameAttributeNameWide = L"Name";
    constexpr char const* payRateAttributeName = "PayRate";
    constexpr wchar_t const* payRateAttributeNameWide = L"PayRate";
    std::atomic_uint64_t expectedObjectInstance{0U};
    std::exception_ptr serverError;
    std::thread server([&] {
      try {
        EmbeddedFederationRegistry registry;
        ProcessFederationService service(
            registry,
            composedProcessDefinition(),
            ProcessFederationServiceOptions{true});
        auto ownerConnection = listener->accept(
            nullptr,
            {"public-process-attribute-relevance-rate-reissue-server", 0xA801U},
            [](std::wstring) {},
            [](std::wstring) { return false; });
        ProcessTransportSession owner(ownerConnection);
        auto ownerHandler = service.handlerFor(owner);
        auto serveExpected = [&](ProcessTransportSession& session,
                                 auto const& handler,
                                 TransportServiceOperation operation,
                                 char const* description) {
          if (!ProcessTransportServiceDispatcher::serveOne(
                  session,
                  [&](TransportServiceMessage const& request) {
                    if (request.operation != operation) {
                      throw std::runtime_error(description);
                    }
                    auto response = handler(request);
                    if (operation ==
                            TransportServiceOperation::register_object_instance &&
                        response.status == TransportServiceStatus::ok) {
                      auto const registration =
                          umbra::detail::decodeProcessFederationRegisterObjectInstanceResult(
                              response.payload);
                      expectedObjectInstance.store(
                          registration.objectInstanceHandle,
                          std::memory_order_release);
                    }
                    return response;
                  })) {
            throw std::runtime_error(description);
          }
        };
        auto serveCallbackPoll = [&](ProcessTransportSession& session,
                                     auto const& handler,
                                     char const* description) {
          serveExpected(
              session,
              handler,
              callbackModel == HLA_IMMEDIATE
                  ? TransportServiceOperation::get_object_class_handle
                  : TransportServiceOperation::receive_interaction,
              description);
        };

        serveExpected(
            owner,
            ownerHandler,
            TransportServiceOperation::create_federation_execution,
            "The rate-reissue server lost Create.");
        serveExpected(
            owner,
            ownerHandler,
            TransportServiceOperation::join_federation_execution,
            "The rate-reissue server lost owner Join.");

        auto firstConnection = listener->accept(
            nullptr,
            {"public-process-attribute-relevance-rate-reissue-first", 0xA802U},
            [](std::wstring) {},
            [](std::wstring) { return false; });
        ProcessTransportSession firstReceiver(firstConnection);
        auto firstReceiverHandler = service.handlerFor(firstReceiver);
        serveExpected(
            firstReceiver,
            firstReceiverHandler,
            TransportServiceOperation::join_federation_execution,
            "The rate-reissue server lost first receiver Join.");

        auto secondConnection = listener->accept(
            nullptr,
            {"public-process-attribute-relevance-rate-reissue-second", 0xA803U},
            [](std::wstring) {},
            [](std::wstring) { return false; });
        ProcessTransportSession secondReceiver(secondConnection);
        auto secondReceiverHandler = service.handlerFor(secondReceiver);
        serveExpected(
            secondReceiver,
            secondReceiverHandler,
            TransportServiceOperation::join_federation_execution,
            "The rate-reissue server lost second receiver Join.");

        serveExpected(
            owner,
            ownerHandler,
            TransportServiceOperation::set_attribute_relevance_advisory_switch,
            "The rate-reissue server lost advisory-switch Set.");
        serveExpected(
            owner,
            ownerHandler,
            TransportServiceOperation::get_attribute_relevance_advisory_switch,
            "The rate-reissue server lost advisory-switch Get.");
        for (auto const operation : {
                 TransportServiceOperation::get_object_class_handle,
                 TransportServiceOperation::get_attribute_handle,
                 TransportServiceOperation::get_attribute_handle,
             }) {
          serveExpected(
              owner,
              ownerHandler,
              operation,
              "The rate-reissue server lost owner handle lookup.");
        }
        serveExpected(
            owner,
            ownerHandler,
            TransportServiceOperation::publish_object_class_attributes,
            "The rate-reissue server lost Publish.");

        for (auto* receiver : {&firstReceiver, &secondReceiver}) {
          auto const& handler = receiver == &firstReceiver
                                    ? firstReceiverHandler
                                    : secondReceiverHandler;
          serveExpected(
              *receiver,
              handler,
              TransportServiceOperation::get_object_class_handle,
              "The rate-reissue server lost receiver class lookup.");
          serveExpected(
              *receiver,
              handler,
              TransportServiceOperation::get_attribute_handle,
              "The rate-reissue server lost receiver Name lookup.");
          serveExpected(
              *receiver,
              handler,
              TransportServiceOperation::subscribe_object_class_attributes,
              "The rate-reissue server lost initial receiver Subscribe.");
        }

        serveExpected(
            owner,
            ownerHandler,
            TransportServiceOperation::register_object_instance,
            "The rate-reissue server lost Register.");
        serveCallbackPoll(
            firstReceiver,
            firstReceiverHandler,
            "The rate-reissue server lost first discovery poll.");
        serveCallbackPoll(
            secondReceiver,
            secondReceiverHandler,
            "The rate-reissue server lost second discovery poll.");

        serveExpected(
            firstReceiver,
            firstReceiverHandler,
            TransportServiceOperation::get_attribute_handle,
            "The rate-reissue server lost first receiver PayRate lookup.");
        serveExpected(
            firstReceiver,
            firstReceiverHandler,
            TransportServiceOperation::subscribe_object_class_attributes,
            "The rate-reissue server lost first High Subscribe.");
        serveCallbackPoll(
            owner,
            ownerHandler,
            "The rate-reissue server lost first High advisory poll.");

        serveExpected(
            secondReceiver,
            secondReceiverHandler,
            TransportServiceOperation::get_attribute_handle,
            "The rate-reissue server lost second receiver PayRate lookup.");
        serveExpected(
            secondReceiver,
            secondReceiverHandler,
            TransportServiceOperation::subscribe_object_class_attributes,
            "The rate-reissue server lost second Medium Subscribe.");
        serveExpected(
            secondReceiver,
            secondReceiverHandler,
            TransportServiceOperation::subscribe_object_class_attributes,
            "The rate-reissue server lost second Low Subscribe.");

        serveExpected(
            firstReceiver,
            firstReceiverHandler,
            TransportServiceOperation::subscribe_object_class_attributes,
            "The rate-reissue server lost first Medium Subscribe.");
        serveCallbackPoll(
            owner,
            ownerHandler,
            "The rate-reissue server lost Medium advisory poll.");
        serveExpected(
            firstReceiver,
            firstReceiverHandler,
            TransportServiceOperation::subscribe_object_class_attributes,
            "The rate-reissue server lost first High re-subscribe.");
        serveCallbackPoll(
            owner,
            ownerHandler,
            "The rate-reissue server lost High advisory poll.");

        serveExpected(
            firstReceiver,
            firstReceiverHandler,
            TransportServiceOperation::unsubscribe_object_class_attributes,
            "The rate-reissue server lost first PayRate Unsubscribe.");
        serveCallbackPoll(
            owner,
            ownerHandler,
            "The rate-reissue server lost first unsubscribe advisory poll.");
        serveExpected(
            secondReceiver,
            secondReceiverHandler,
            TransportServiceOperation::unsubscribe_object_class_attributes,
            "The rate-reissue server lost second PayRate Unsubscribe.");
        serveCallbackPoll(
            owner,
            ownerHandler,
            "The rate-reissue server lost final advisory-off poll.");

        serveExpected(
            owner,
            ownerHandler,
            TransportServiceOperation::resign_federation_execution,
            "The rate-reissue server lost owner Resign.");
        serveExpected(
            firstReceiver,
            firstReceiverHandler,
            TransportServiceOperation::resign_federation_execution,
            "The rate-reissue server lost first receiver Resign.");
        serveExpected(
            secondReceiver,
            secondReceiverHandler,
            TransportServiceOperation::resign_federation_execution,
            "The rate-reissue server lost second receiver Resign.");
        service.detach(owner);
        service.detach(firstReceiver);
        service.detach(secondReceiver);
        ownerConnection->close();
        firstConnection->close();
        secondConnection->close();
      } catch (...) {
        serverError = std::current_exception();
      }
    });

    auto ownerRti = makeRti();
    auto firstReceiverRti = makeRti();
    auto secondReceiverRti = makeRti();
    auto pumpOwner = [&] {
      if (callbackModel == HLA_IMMEDIATE) {
        static_cast<void>(ownerRti->getObjectClassHandle(objectClassNameWide));
      } else {
        static_cast<void>(ownerRti->evokeCallback(0.0));
      }
    };
    auto pumpReceiver = [](RTIambassador& rti) {
      static_cast<void>(rti.evokeCallback(0.0));
    };
    auto configuration = RtiConfiguration::createConfiguration()
                             .withConfigurationName(
                                 L"public-process-attribute-relevance-rate-reissue-client")
                             .withRtiAddress(
                                 L"tcp://127.0.0.1:" + std::to_wstring(port));
    std::exception_ptr clientError;
    bool ownerJoined = false;
    bool firstReceiverJoined = false;
    bool secondReceiverJoined = false;
    try {
      REQUIRE(ownerRti->connect(ownerFederate, callbackModel, configuration).addressUsed);
      ownerRti->createFederationExecution(federationName, L"server-owned-fom.xml");
      ownerRti->joinFederationExecution(
          L"public-process-attribute-relevance-rate-owner",
          L"public-process-attribute-relevance-rate-owner-type",
          federationName);
      ownerJoined = true;

      REQUIRE(firstReceiverRti->connect(
                  firstReceiverFederate, callbackModel, configuration)
                  .addressUsed);
      firstReceiverRti->joinFederationExecution(
          L"public-process-attribute-relevance-rate-first",
          L"public-process-attribute-relevance-rate-receiver-type",
          federationName);
      firstReceiverJoined = true;

      REQUIRE(secondReceiverRti->connect(
                  secondReceiverFederate, callbackModel, configuration)
                  .addressUsed);
      secondReceiverRti->joinFederationExecution(
          L"public-process-attribute-relevance-rate-second",
          L"public-process-attribute-relevance-rate-receiver-type",
          federationName);
      secondReceiverJoined = true;

      REQUIRE_NOTHROW(ownerRti->setAttributeRelevanceAdvisorySwitch(true));
      REQUIRE(ownerRti->getAttributeRelevanceAdvisorySwitch());

      auto const ownerObjectClass = ownerRti->getObjectClassHandle(objectClassNameWide);
      auto const ownerNameAttribute = ownerRti->getAttributeHandle(
          ownerObjectClass, nameAttributeNameWide);
      auto const ownerPayRateAttribute = ownerRti->getAttributeHandle(
          ownerObjectClass, payRateAttributeNameWide);
      REQUIRE(ownerObjectClass.isValid());
      REQUIRE(ownerNameAttribute.isValid());
      REQUIRE(ownerPayRateAttribute.isValid());
      REQUIRE_NOTHROW(ownerRti->publishObjectClassAttributes(
          ownerObjectClass,
          rti1516_2025::AttributeHandleSet{
              ownerNameAttribute,
              ownerPayRateAttribute}));

      auto const firstReceiverObjectClass =
          firstReceiverRti->getObjectClassHandle(objectClassNameWide);
      auto const firstReceiverNameAttribute = firstReceiverRti->getAttributeHandle(
          firstReceiverObjectClass, nameAttributeNameWide);
      REQUIRE(firstReceiverObjectClass == ownerObjectClass);
      REQUIRE(firstReceiverNameAttribute == ownerNameAttribute);
      REQUIRE_NOTHROW(firstReceiverRti->subscribeObjectClassAttributes(
          firstReceiverObjectClass,
          rti1516_2025::AttributeHandleSet{firstReceiverNameAttribute}));

      auto const secondReceiverObjectClass =
          secondReceiverRti->getObjectClassHandle(objectClassNameWide);
      auto const secondReceiverNameAttribute = secondReceiverRti->getAttributeHandle(
          secondReceiverObjectClass, nameAttributeNameWide);
      REQUIRE(secondReceiverObjectClass == ownerObjectClass);
      REQUIRE(secondReceiverNameAttribute == ownerNameAttribute);
      REQUIRE_NOTHROW(secondReceiverRti->subscribeObjectClassAttributes(
          secondReceiverObjectClass,
          rti1516_2025::AttributeHandleSet{secondReceiverNameAttribute}));

      auto const objectInstance = ownerRti->registerObjectInstance(ownerObjectClass);
      REQUIRE(objectInstance.isValid());
      REQUIRE(objectInstance.toString() ==
              L"ObjectInstanceHandle(" +
                  std::to_wstring(expectedObjectInstance.load(std::memory_order_acquire)) +
                  L")");
      if (callbackModel == HLA_IMMEDIATE) {
        static_cast<void>(firstReceiverRti->getObjectClassHandle(objectClassNameWide));
        static_cast<void>(secondReceiverRti->getObjectClassHandle(objectClassNameWide));
      } else {
        pumpReceiver(*firstReceiverRti);
        pumpReceiver(*secondReceiverRti);
      }
      REQUIRE(firstReceiverFederate.discovered);
      REQUIRE(secondReceiverFederate.discovered);
      REQUIRE(firstReceiverFederate.discoveredObjectInstance == objectInstance);
      REQUIRE(secondReceiverFederate.discoveredObjectInstance == objectInstance);
      REQUIRE(firstReceiverFederate.discoveredObjectClass == ownerObjectClass);
      REQUIRE(secondReceiverFederate.discoveredObjectClass == ownerObjectClass);

      // The registration response carries the initial Name advisory. Evoked
      // dispatch retains it locally; immediate dispatch already delivered it.
      if (callbackModel == HLA_EVOKED) {
        pumpOwner();
      }
      REQUIRE(ownerFederate.turnedOnCount == 1U);
      REQUIRE(ownerFederate.turnedOnObjectInstance == objectInstance);
      REQUIRE(ownerFederate.turnedOnAttributes ==
              rti1516_2025::AttributeHandleSet{ownerNameAttribute});
      REQUIRE(ownerFederate.turnedOnRates.back() ==
              std::nullopt);
      ownerFederate.turnedOnCount = 0U;
      ownerFederate.turnedOnRates.clear();
      ownerFederate.turnedOnAttributes.clear();

      auto const firstReceiverPayRateAttribute = firstReceiverRti->getAttributeHandle(
          firstReceiverObjectClass, payRateAttributeNameWide);
      REQUIRE(firstReceiverPayRateAttribute == ownerPayRateAttribute);
      REQUIRE_NOTHROW(firstReceiverRti->subscribeObjectClassAttributes(
          firstReceiverObjectClass,
          rti1516_2025::AttributeHandleSet{firstReceiverPayRateAttribute},
          true,
          L"High"));
      pumpOwner();
      REQUIRE(ownerFederate.turnedOnCount == 1U);
      REQUIRE(ownerFederate.turnedOnAttributes ==
              rti1516_2025::AttributeHandleSet{ownerPayRateAttribute});
      REQUIRE(ownerFederate.turnedOnRates.back() ==
              std::optional<std::wstring>{L"High"});

      auto const secondReceiverPayRateAttribute = secondReceiverRti->getAttributeHandle(
          secondReceiverObjectClass, payRateAttributeNameWide);
      REQUIRE(secondReceiverPayRateAttribute == ownerPayRateAttribute);
      REQUIRE_NOTHROW(secondReceiverRti->subscribeObjectClassAttributes(
          secondReceiverObjectClass,
          rti1516_2025::AttributeHandleSet{secondReceiverPayRateAttribute},
          true,
          L"Medium"));
      REQUIRE_NOTHROW(secondReceiverRti->subscribeObjectClassAttributes(
          secondReceiverObjectClass,
          rti1516_2025::AttributeHandleSet{secondReceiverPayRateAttribute},
          true,
          L"Low"));
      REQUIRE(ownerFederate.turnedOnCount == 1U);

      REQUIRE_NOTHROW(firstReceiverRti->subscribeObjectClassAttributes(
          firstReceiverObjectClass,
          rti1516_2025::AttributeHandleSet{firstReceiverPayRateAttribute},
          true,
          L"Medium"));
      pumpOwner();
      REQUIRE(ownerFederate.turnedOnCount == 2U);
      REQUIRE(ownerFederate.turnedOnRates.back() ==
              std::optional<std::wstring>{L"Medium"});

      REQUIRE_NOTHROW(firstReceiverRti->subscribeObjectClassAttributes(
          firstReceiverObjectClass,
          rti1516_2025::AttributeHandleSet{firstReceiverPayRateAttribute},
          true,
          L"High"));
      pumpOwner();
      REQUIRE(ownerFederate.turnedOnCount == 3U);
      REQUIRE(ownerFederate.turnedOnRates.back() ==
              std::optional<std::wstring>{L"High"});

      REQUIRE_NOTHROW(firstReceiverRti->unsubscribeObjectClassAttributes(
          firstReceiverObjectClass,
          rti1516_2025::AttributeHandleSet{firstReceiverPayRateAttribute}));
      pumpOwner();
      REQUIRE(ownerFederate.turnedOffCount == 0U);
      // Removing the higher-rate declaration leaves a still-relevant peer;
      // the boundary carries a Low-rate Turn Updates On refresh, but it must
      // not emit Turn Updates Off while that peer remains subscribed.
      REQUIRE(ownerFederate.turnedOnCount == 4U);
      REQUIRE(ownerFederate.turnedOnRates.back() ==
              std::optional<std::wstring>{L"Low"});

      REQUIRE_NOTHROW(secondReceiverRti->unsubscribeObjectClassAttributes(
          secondReceiverObjectClass,
          rti1516_2025::AttributeHandleSet{secondReceiverPayRateAttribute}));
      pumpOwner();
      REQUIRE(ownerFederate.turnedOffCount == 1U);
      REQUIRE(ownerFederate.turnedOffObjectInstance == objectInstance);
      REQUIRE(ownerFederate.turnedOffAttributes ==
              rti1516_2025::AttributeHandleSet{ownerPayRateAttribute});

      ownerRti->resignFederationExecution(DELETE_OBJECTS);
      ownerJoined = false;
      firstReceiverRti->resignFederationExecution(NO_ACTION);
      firstReceiverJoined = false;
      secondReceiverRti->resignFederationExecution(NO_ACTION);
      secondReceiverJoined = false;
      ownerRti->disconnect();
      firstReceiverRti->disconnect();
      secondReceiverRti->disconnect();
    } catch (...) {
      clientError = std::current_exception();
      if (ownerJoined) {
        try {
          ownerRti->resignFederationExecution(DELETE_OBJECTS);
        } catch (...) {
        }
      }
      if (firstReceiverJoined) {
        try {
          firstReceiverRti->resignFederationExecution(NO_ACTION);
        } catch (...) {
        }
      }
      if (secondReceiverJoined) {
        try {
          secondReceiverRti->resignFederationExecution(NO_ACTION);
        } catch (...) {
        }
      }
      try {
        ownerRti->disconnect();
      } catch (...) {
      }
      try {
        firstReceiverRti->disconnect();
      } catch (...) {
      }
      try {
        secondReceiverRti->disconnect();
      } catch (...) {
      }
    }
    if (listener) {
      listener.reset();
    }
    if (server.joinable()) {
      server.join();
    }
    if (clientError) {
      std::rethrow_exception(clientError);
    }
    REQUIRE_FALSE(serverError);
    REQUIRE_FALSE(ownerJoined);
    REQUIRE_FALSE(firstReceiverJoined);
    REQUIRE_FALSE(secondReceiverJoined);
  };

  SECTION("HLA_EVOKED") {
    runScenario(HLA_EVOKED);
  }
  SECTION("HLA_IMMEDIATE") {
    runScenario(HLA_IMMEDIATE);
  }
}
#endif
