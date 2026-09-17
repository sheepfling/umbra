#include <catch2/catch_test_macros.hpp>

#include "internal/federation/federation_management_coordinator.hpp"
#include "internal/federation/process_federation_service.hpp"
#include "internal/fom/libxml2_fom_composer.hpp"
#include "internal/fom/libxml2_fom_validator.hpp"
#include "internal/time/reference_time_selection.hpp"

#include <umbra/embedded_profile_configuration.hpp>

#include <RTI/NullFederateAmbassador.h>
#include <RTI/RTI1516.h>

#include <filesystem>
#include <memory>
#include <optional>
#include <stdexcept>
#include <thread>
#include <utility>
#include <vector>

#ifndef UMBRA_SOURCE_DIRECTORY
#error "The process additional-FOM test requires the Umbra source directory."
#endif

namespace {

using umbra::detail::EmbeddedFederationRegistry;
using umbra::detail::FederationDefinition;
using umbra::detail::FederationManagementCoordinator;
using umbra::detail::FederationManagementResources;
using umbra::detail::FederationPreparationStatus;
using umbra::detail::LibXml2FomModuleComposer;
using umbra::detail::LibXml2FomValidator;
using umbra::detail::ProcessFederationCreateRequest;
using umbra::detail::ProcessFederationJoinRequest;
using umbra::detail::ProcessFederationJoinResult;
using umbra::detail::ProcessFederationResignRequest;
using umbra::detail::ProcessFederationService;
using umbra::detail::ProcessTransportConnection;
using umbra::detail::ProcessTransportListener;
using umbra::detail::ProcessTransportServiceDispatcher;
using umbra::detail::ProcessTransportSession;
using umbra::detail::ReferenceLogicalTimeSelector;
using umbra::detail::TransportServiceMessage;
using umbra::detail::TransportServiceMessageKind;
using umbra::detail::TransportServiceOperation;
using umbra::detail::TransportServiceStatus;

std::filesystem::path resourcePath(std::filesystem::path const& relative) {
  return std::filesystem::path(UMBRA_SOURCE_DIRECTORY) / "third_party" /
      "ieee1516.2-2025" / "resources" / relative;
}

std::filesystem::path testDataPath(std::filesystem::path const& relative) {
  return std::filesystem::path(UMBRA_SOURCE_DIRECTORY) / "cpp" / "tests" /
      "data" / relative;
}

TransportServiceMessage request(
    TransportServiceOperation operation,
    std::uint64_t requestId,
    std::vector<std::uint8_t> payload) {
  return {
      TransportServiceMessageKind::request,
      operation,
      TransportServiceStatus::ok,
      requestId,
      std::move(payload)};
}

}  // namespace

TEST_CASE(
    "Private process service composes additional FOM modules during Join Federation Execution",
    "[unit][foundation][federation-management][fom][fom-module-management]"
    "[transport][process-boundary][registry-binding]"
    "[rti.service.join-federation-execution][2025]") {
  auto listener = ProcessTransportListener::listen({"127.0.0.1", 0U});
  REQUIRE(listener != nullptr);

  LibXml2FomValidator validator;
  LibXml2FomModuleComposer composer(
      resourcePath("schemas/IEEE1516-FDD-2025.xsd"));
  ReferenceLogicalTimeSelector timeSelector;
  FederationManagementCoordinator coordinator(
      validator,
      composer,
      timeSelector,
      FederationManagementResources{
          resourcePath("mim/HLAstandardMIM-2025.xml"),
          resourcePath("schemas/IEEE1516-DIF-2025.xsd"),
          std::nullopt});
  auto basePreparation = coordinator.prepareCreate(
      {resourcePath("examples/RestaurantFOMmodule-2025.xml").wstring()},
      std::nullopt,
      L"HLAinteger64Time");
  CAPTURE(basePreparation.diagnostics);
  REQUIRE(basePreparation.status == FederationPreparationStatus::applied);
  REQUIRE(basePreparation.definition);

  umbra::detail::ProcessFederationServiceOptions options;
  options.additionalFomPreparation =
      [&coordinator](FederationDefinition const& existing,
                     std::vector<std::wstring> const& additionalModules)
      -> std::optional<FederationDefinition> {
    auto prepared = coordinator.prepareAdditionalModules(existing, additionalModules);
    if (!prepared.accepted()) {
      return std::nullopt;
    }
    return std::move(prepared.definition);
  };

  EmbeddedFederationRegistry registry;
  ProcessFederationService service(
      registry,
      std::move(*basePreparation.definition),
      options);
  std::exception_ptr serverFailure;
  std::thread server([&] {
    try {
      auto connection = listener->accept(
          nullptr,
          {"process-additional-fom", 0xA1U},
          [](std::wstring) {},
          [](std::wstring) { return false; });
      ProcessTransportSession session(std::move(connection));
      auto handler = service.handlerFor(session);
      if (!ProcessTransportServiceDispatcher::serveOne(session, handler) ||
          !ProcessTransportServiceDispatcher::serveOne(session, handler) ||
          !ProcessTransportServiceDispatcher::serveOne(session, handler)) {
        throw std::runtime_error(
            "The process additional-FOM service lost a lifecycle request.");
      }

      auto const definition = registry.definitionFor(L"process-additional-fom-execution");
      if (!definition || definition->fomModules.size() != 3U ||
          !registry.objectClassHandleFor(
              L"process-additional-fom-execution",
              "HLAobjectRoot.UmbraReferenceFixtureClass")) {
        throw std::runtime_error(
            "The process Join did not commit the composed additional FOM.");
      }
      service.detach(session);
      session.connection()->close();
    } catch (...) {
      serverFailure = std::current_exception();
    }
  });

  std::exception_ptr clientFailure;
  std::shared_ptr<ProcessTransportConnection> connection;
  try {
    connection = ProcessTransportConnection::connectClient(
        nullptr,
        {"127.0.0.1", listener->address().port},
        {"process-additional-fom", 0xA2U},
        [](std::wstring) {},
        [](std::wstring) { return false; });
    ProcessTransportSession session(connection);
    TransportServiceMessage response;

    REQUIRE(session.request(
        request(
            TransportServiceOperation::create_federation_execution,
            1U,
            umbra::detail::encodeProcessFederationCreateRequest(
                ProcessFederationCreateRequest{
                    L"process-additional-fom-execution"})),
        response));
    REQUIRE(response.status == TransportServiceStatus::ok);

    auto const extension = testDataPath("reference-data-class-provider-fom.xml").wstring();
    REQUIRE(session.request(
        request(
            TransportServiceOperation::join_federation_execution,
            2U,
            umbra::detail::encodeProcessFederationJoinRequest(
                ProcessFederationJoinRequest{
                    L"process-additional-fom-execution",
                    L"process-additional-fom-type",
                    L"process-additional-fom-federate",
                    {extension}})),
        response));
    REQUIRE(response.status == TransportServiceStatus::ok);
    auto const joinResult = umbra::detail::decodeProcessFederationJoinResult(
        response.payload);
    REQUIRE(joinResult.federateId != 0U);

    REQUIRE(session.request(
        request(
            TransportServiceOperation::resign_federation_execution,
            3U,
            umbra::detail::encodeProcessFederationResignRequest(
                ProcessFederationResignRequest{
                    L"process-additional-fom-execution",
                    joinResult.federateId,
                    rti1516_2025::NO_ACTION})),
        response));
    REQUIRE(response.status == TransportServiceStatus::ok);
    session.connection()->close();
  } catch (...) {
    clientFailure = std::current_exception();
    if (connection) {
      connection->close();
    }
  }

  listener.reset();
  if (server.joinable()) {
    server.join();
  }
  if (clientFailure) {
    std::rethrow_exception(clientFailure);
  }
  REQUIRE_FALSE(serverFailure);
}

TEST_CASE(
    "RTIambassador carries additional FOM modules through a configured process Join",
    "[integration][development-profile][federation-management][fom][fom-module-management]"
    "[transport][process-boundary][public-endpoint]"
    "[rti.service.join-federation-execution][rti.service.get-object-class-handle]"
    "[rti.service.get-object-class-name][2025]") {
  auto listener = ProcessTransportListener::listen({"127.0.0.1", 0U});
  REQUIRE(listener != nullptr);

  LibXml2FomValidator validator;
  LibXml2FomModuleComposer composer(
      resourcePath("schemas/IEEE1516-FDD-2025.xsd"));
  ReferenceLogicalTimeSelector timeSelector;
  FederationManagementCoordinator coordinator(
      validator,
      composer,
      timeSelector,
      FederationManagementResources{
          resourcePath("mim/HLAstandardMIM-2025.xml"),
          resourcePath("schemas/IEEE1516-DIF-2025.xsd"),
          std::nullopt});
  auto basePreparation = coordinator.prepareCreate(
      {resourcePath("examples/RestaurantFOMmodule-2025.xml").wstring()},
      std::nullopt,
      L"HLAinteger64Time");
  CAPTURE(basePreparation.diagnostics);
  REQUIRE(basePreparation.status == FederationPreparationStatus::applied);
  REQUIRE(basePreparation.definition);
  auto const baseModuleCount = basePreparation.definition->fomModules.size();

  umbra::detail::ProcessFederationServiceOptions options;
  options.createFomPreparation =
      [&coordinator](std::vector<std::wstring> const& fomModules,
                     std::optional<std::wstring> const& mimModule,
                     std::wstring const& logicalTimeImplementationName)
      -> std::optional<FederationDefinition> {
    auto prepared = coordinator.prepareCreate(
        fomModules,
        mimModule,
        logicalTimeImplementationName);
    if (!prepared.accepted()) {
      return std::nullopt;
    }
    return std::move(prepared.definition);
  };
  options.additionalFomPreparation =
      [&coordinator](FederationDefinition const& existing,
                     std::vector<std::wstring> const& additionalModules)
      -> std::optional<FederationDefinition> {
    auto prepared = coordinator.prepareAdditionalModules(existing, additionalModules);
    if (!prepared.accepted()) {
      return std::nullopt;
    }
    return std::move(prepared.definition);
  };

  EmbeddedFederationRegistry registry;
  ProcessFederationService service(
      registry,
      std::move(*basePreparation.definition),
      options);
  std::exception_ptr serverFailure;
  std::thread server([&] {
    try {
      auto connection = listener->accept(
          nullptr,
          {"process-public-additional-fom", 0xB1U},
          [](std::wstring) {},
          [](std::wstring) { return false; });
      ProcessTransportSession session(std::move(connection));
      auto handler = service.handlerFor(session);
      auto serveExpected = [&](TransportServiceOperation operation) {
        return ProcessTransportServiceDispatcher::serveOne(
            session,
            [&](TransportServiceMessage const& incoming) {
              if (incoming.operation != operation) {
                throw std::runtime_error(
                    "The public additional-FOM process service received an unexpected operation.");
              }
              return handler(incoming);
            });
      };
      if (!serveExpected(TransportServiceOperation::create_federation_execution)) {
        throw std::runtime_error(
            "The public additional-FOM process service lost a lifecycle request.");
      }
      if (registry.definitionFor(
              L"process-public-additional-fom-execution")) {
        throw std::runtime_error(
            "The public process Create committed an invalid FOM definition.");
      }
      if (!serveExpected(TransportServiceOperation::create_federation_execution) ||
          !serveExpected(TransportServiceOperation::join_federation_execution)) {
        throw std::runtime_error(
            "The public additional-FOM process service lost a lifecycle request.");
      }
      auto const afterInvalidJoin = registry.definitionFor(
          L"process-public-additional-fom-execution");
      if (!afterInvalidJoin || afterInvalidJoin->fomModules.size() != baseModuleCount) {
        throw std::runtime_error(
            "The public process Join mutated the definition after an invalid FOM.");
      }
      if (!serveExpected(TransportServiceOperation::join_federation_execution) ||
          !serveExpected(TransportServiceOperation::get_object_class_handle) ||
          !serveExpected(TransportServiceOperation::get_object_class_name) ||
          !serveExpected(TransportServiceOperation::resign_federation_execution)) {
        throw std::runtime_error(
            "The public additional-FOM process service lost a lifecycle request.");
      }
      if (!registry.objectClassHandleFor(
              L"process-public-additional-fom-execution",
              "HLAobjectRoot.UmbraReferenceFixtureClass")) {
        throw std::runtime_error(
            "The public process Join did not retain the additional FOM class.");
      }
      service.detach(session);
      session.connection()->close();
    } catch (...) {
      serverFailure = std::current_exception();
    }
  });

  std::exception_ptr clientFailure;
  try {
    rti1516_2025::NullFederateAmbassador federate;
    rti1516_2025::RTIambassadorFactory factory;
    auto rti = factory.createRTIambassador();
    auto configuration = rti1516_2025::RtiConfiguration::createConfiguration()
                             .withConfigurationName(
                                 L"process-public-additional-fom-client")
                             .withRtiAddress(
                                 L"tcp://127.0.0.1:" +
                                 std::to_wstring(listener->address().port));
    auto const connectionResult =
        rti->connect(federate, rti1516_2025::HLA_EVOKED, configuration);
    REQUIRE(connectionResult.addressUsed);
    auto const invalidCreate =
        testDataPath("missing-additional-fom.xml").wstring();
    REQUIRE_THROWS_AS(
        rti->createFederationExecution(
            L"process-public-additional-fom-execution",
            std::vector<std::wstring>{invalidCreate},
            L"HLAinteger64Time"),
        rti1516_2025::RTIinternalError);
    REQUIRE_NOTHROW(rti->createFederationExecution(
        L"process-public-additional-fom-execution",
        std::vector<std::wstring>{
            resourcePath("examples/RestaurantFOMmodule-2025.xml").wstring()},
        L"HLAinteger64Time"));

    auto const invalidExtension =
        testDataPath("missing-additional-fom.xml").wstring();
    REQUIRE_THROWS_AS(
        rti->joinFederationExecution(
            L"process-public-additional-fom-type",
            L"process-public-additional-fom-execution",
            std::vector<std::wstring>{invalidExtension}),
        rti1516_2025::RTIinternalError);

    auto const extension =
        testDataPath("reference-data-class-provider-fom.xml").wstring();
    auto const joined = rti->joinFederationExecution(
        L"process-public-additional-fom-type",
        L"process-public-additional-fom-execution",
        std::vector<std::wstring>{extension});
    REQUIRE(joined.isValid());
    auto const extensionClass = rti->getObjectClassHandle(
        L"HLAobjectRoot.UmbraReferenceFixtureClass");
    REQUIRE(extensionClass.isValid());
    REQUIRE(rti->getObjectClassName(extensionClass) ==
            L"HLAobjectRoot.UmbraReferenceFixtureClass");
    REQUIRE_NOTHROW(rti->resignFederationExecution(
        rti1516_2025::NO_ACTION));
    REQUIRE_NOTHROW(rti->disconnect());
  } catch (...) {
    clientFailure = std::current_exception();
  }

  listener.reset();
  if (server.joinable()) {
    server.join();
  }
  if (clientFailure) {
    std::rethrow_exception(clientFailure);
  }
  REQUIRE_FALSE(serverFailure);
}

TEST_CASE(
    "Private process service rejects an invalid additional FOM without mutating the execution",
    "[unit][foundation][federation-management][fom][fom-module-management]"
    "[transport][process-boundary][error-path][registry-binding]"
    "[rti.service.join-federation-execution][2025]") {
  auto listener = ProcessTransportListener::listen({"127.0.0.1", 0U});
  REQUIRE(listener != nullptr);

  LibXml2FomValidator validator;
  LibXml2FomModuleComposer composer(
      resourcePath("schemas/IEEE1516-FDD-2025.xsd"));
  ReferenceLogicalTimeSelector timeSelector;
  FederationManagementCoordinator coordinator(
      validator,
      composer,
      timeSelector,
      FederationManagementResources{
          resourcePath("mim/HLAstandardMIM-2025.xml"),
          resourcePath("schemas/IEEE1516-DIF-2025.xsd"),
          std::nullopt});
  auto basePreparation = coordinator.prepareCreate(
      {resourcePath("examples/RestaurantFOMmodule-2025.xml").wstring()},
      std::nullopt,
      L"HLAinteger64Time");
  CAPTURE(basePreparation.diagnostics);
  REQUIRE(basePreparation.status == FederationPreparationStatus::applied);
  REQUIRE(basePreparation.definition);
  auto const baseModuleCount = basePreparation.definition->fomModules.size();

  umbra::detail::ProcessFederationServiceOptions options;
  options.additionalFomPreparation =
      [&coordinator](FederationDefinition const& existing,
                     std::vector<std::wstring> const& additionalModules)
      -> std::optional<FederationDefinition> {
    auto prepared = coordinator.prepareAdditionalModules(existing, additionalModules);
    if (!prepared.accepted()) {
      return std::nullopt;
    }
    return std::move(prepared.definition);
  };

  EmbeddedFederationRegistry registry;
  ProcessFederationService service(
      registry,
      std::move(*basePreparation.definition),
      options);
  std::exception_ptr serverFailure;
  std::thread server([&] {
    try {
      auto connection = listener->accept(
          nullptr,
          {"process-invalid-additional-fom", 0xC1U},
          [](std::wstring) {},
          [](std::wstring) { return false; });
      ProcessTransportSession session(std::move(connection));
      auto handler = service.handlerFor(session);
      auto serveExpected = [&](TransportServiceOperation operation) {
        return ProcessTransportServiceDispatcher::serveOne(
            session,
            [&](TransportServiceMessage const& incoming) {
              if (incoming.operation != operation) {
                throw std::runtime_error(
                    "The invalid additional-FOM process service received an unexpected operation.");
              }
              return handler(incoming);
            });
      };
      if (!serveExpected(TransportServiceOperation::create_federation_execution)) {
        throw std::runtime_error(
            "The invalid additional-FOM process service lost Create.");
      }
      auto const afterCreate = registry.definitionFor(
          L"process-invalid-additional-fom-execution");
      if (!afterCreate || afterCreate->fomModules.size() != baseModuleCount) {
        throw std::runtime_error(
            "Create did not retain the validated base FOM definition.");
      }
      if (!serveExpected(TransportServiceOperation::join_federation_execution)) {
        throw std::runtime_error(
            "The invalid additional-FOM process service lost the rejected Join.");
      }
      auto const afterInvalidJoin = registry.definitionFor(
          L"process-invalid-additional-fom-execution");
      if (!afterInvalidJoin || afterInvalidJoin->fomModules.size() != baseModuleCount) {
        throw std::runtime_error(
            "An invalid additional FOM mutated the execution definition.");
      }
      if (!serveExpected(TransportServiceOperation::join_federation_execution) ||
          !serveExpected(TransportServiceOperation::resign_federation_execution)) {
        throw std::runtime_error(
            "The invalid additional-FOM process service lost the recovery lifecycle.");
      }
      auto const afterValidJoin = registry.definitionFor(
          L"process-invalid-additional-fom-execution");
      if (!afterValidJoin || afterValidJoin->fomModules.size() != baseModuleCount + 1U ||
          !registry.objectClassHandleFor(
              L"process-invalid-additional-fom-execution",
              "HLAobjectRoot.UmbraReferenceFixtureClass")) {
        throw std::runtime_error(
            "A valid follow-up Join did not compose the additional FOM after rejection.");
      }
      service.detach(session);
      session.connection()->close();
    } catch (...) {
      serverFailure = std::current_exception();
    }
  });

  std::exception_ptr clientFailure;
  std::shared_ptr<ProcessTransportConnection> connection;
  try {
    connection = ProcessTransportConnection::connectClient(
        nullptr,
        {"127.0.0.1", listener->address().port},
        {"process-invalid-additional-fom", 0xC2U},
        [](std::wstring) {},
        [](std::wstring) { return false; });
    ProcessTransportSession session(connection);
    TransportServiceMessage response;

    REQUIRE(session.request(
        request(
            TransportServiceOperation::create_federation_execution,
            1U,
            umbra::detail::encodeProcessFederationCreateRequest(
                ProcessFederationCreateRequest{
                    L"process-invalid-additional-fom-execution"})),
        response));
    REQUIRE(response.status == TransportServiceStatus::ok);

    auto const invalidExtension =
        testDataPath("missing-additional-fom.xml").wstring();
    REQUIRE(session.request(
        request(
            TransportServiceOperation::join_federation_execution,
            2U,
            umbra::detail::encodeProcessFederationJoinRequest(
                ProcessFederationJoinRequest{
                    L"process-invalid-additional-fom-execution",
                    L"process-invalid-additional-fom-type",
                    L"process-invalid-additional-fom-federate",
                    {invalidExtension}})),
        response));
    REQUIRE(response.status == TransportServiceStatus::rejected);

    auto const validExtension =
        testDataPath("reference-data-class-provider-fom.xml").wstring();
    REQUIRE(session.request(
        request(
            TransportServiceOperation::join_federation_execution,
            3U,
            umbra::detail::encodeProcessFederationJoinRequest(
                ProcessFederationJoinRequest{
                    L"process-invalid-additional-fom-execution",
                    L"process-invalid-additional-fom-type",
                    L"process-invalid-additional-fom-federate",
                    {validExtension}})),
        response));
    REQUIRE(response.status == TransportServiceStatus::ok);
    auto const joinResult = umbra::detail::decodeProcessFederationJoinResult(
        response.payload);
    REQUIRE(joinResult.federateId != 0U);

    REQUIRE(session.request(
        request(
            TransportServiceOperation::resign_federation_execution,
            4U,
            umbra::detail::encodeProcessFederationResignRequest(
                ProcessFederationResignRequest{
                    L"process-invalid-additional-fom-execution",
                    joinResult.federateId,
                    rti1516_2025::NO_ACTION})),
        response));
    REQUIRE(response.status == TransportServiceStatus::ok);
    session.connection()->close();
  } catch (...) {
    clientFailure = std::current_exception();
    if (connection) {
      connection->close();
    }
  }

  listener.reset();
  if (server.joinable()) {
    server.join();
  }
  if (clientFailure) {
    std::rethrow_exception(clientFailure);
  }
  REQUIRE_FALSE(serverFailure);
}

TEST_CASE(
    "RTIambassador carries an explicit MIM through a configured process Create Federation Execution",
    "[integration][development-profile][federation-management][fom][mim]"
    "[transport][process-boundary][public-endpoint]"
    "[rti.service.create-federation-execution-with-mim][2025]") {
  auto listener = ProcessTransportListener::listen({"127.0.0.1", 0U});
  REQUIRE(listener != nullptr);

  LibXml2FomValidator validator;
  LibXml2FomModuleComposer composer(
      resourcePath("schemas/IEEE1516-FDD-2025.xsd"));
  ReferenceLogicalTimeSelector timeSelector;
  FederationManagementCoordinator coordinator(
      validator,
      composer,
      timeSelector,
      FederationManagementResources{
          resourcePath("mim/HLAstandardMIM-2025.xml"),
          resourcePath("schemas/IEEE1516-DIF-2025.xsd"),
          std::nullopt});
  auto basePreparation = coordinator.prepareCreate(
      {resourcePath("examples/RestaurantFOMmodule-2025.xml").wstring()},
      std::nullopt,
      L"HLAinteger64Time");
  CAPTURE(basePreparation.diagnostics);
  REQUIRE(basePreparation.status == FederationPreparationStatus::applied);
  REQUIRE(basePreparation.definition);

  umbra::detail::ProcessFederationServiceOptions options;
  options.createFomPreparation =
      [&coordinator](std::vector<std::wstring> const& fomModules,
                     std::optional<std::wstring> const& mimModule,
                     std::wstring const& logicalTimeImplementationName)
      -> std::optional<FederationDefinition> {
    auto prepared = coordinator.prepareCreate(
        fomModules,
        mimModule,
        logicalTimeImplementationName);
    if (!prepared.accepted()) {
      return std::nullopt;
    }
    return std::move(prepared.definition);
  };

  EmbeddedFederationRegistry registry;
  ProcessFederationService service(
      registry,
      std::move(*basePreparation.definition),
      options);
  std::exception_ptr serverFailure;
  std::thread server([&] {
    try {
      auto connection = listener->accept(
          nullptr,
          {"process-explicit-mim", 0xD1U},
          [](std::wstring) {},
          [](std::wstring) { return false; });
      ProcessTransportSession session(std::move(connection));
      auto handler = service.handlerFor(session);
      auto serveExpected = [&](TransportServiceOperation operation) {
        return ProcessTransportServiceDispatcher::serveOne(
            session,
            [&](TransportServiceMessage const& incoming) {
              if (incoming.operation != operation) {
                throw std::runtime_error(
                    "The explicit-MIM process service received an unexpected operation.");
              }
              return handler(incoming);
            });
      };
      if (!serveExpected(TransportServiceOperation::create_federation_execution)) {
        throw std::runtime_error(
            "The explicit-MIM process service lost Create.");
      }
      auto const definition = registry.definitionFor(
          L"process-explicit-mim-execution");
      if (!definition || definition->fomModules.size() != 2U ||
          definition->logicalTimeImplementationName != L"HLAinteger64Time") {
        throw std::runtime_error(
            "The process Create With MIM did not commit the composed definition.");
      }
      bool hasFom = false;
      bool hasMim = false;
      for (auto const& module : definition->fomModules) {
        hasFom = hasFom || module.kind == umbra::detail::FomModuleKind::fom;
        hasMim = hasMim || module.kind == umbra::detail::FomModuleKind::mim;
      }
      if (!hasFom || !hasMim) {
        throw std::runtime_error(
            "The process Create With MIM committed the wrong module kinds.");
      }
      if (!serveExpected(TransportServiceOperation::join_federation_execution) ||
          !serveExpected(TransportServiceOperation::resign_federation_execution)) {
        throw std::runtime_error(
            "The explicit-MIM process service lost the membership lifecycle.");
      }
      service.detach(session);
      session.connection()->close();
    } catch (...) {
      serverFailure = std::current_exception();
    }
  });

  std::exception_ptr clientFailure;
  try {
    rti1516_2025::NullFederateAmbassador federate;
    rti1516_2025::RTIambassadorFactory factory;
    auto rti = factory.createRTIambassador();
    auto configuration = rti1516_2025::RtiConfiguration::createConfiguration()
                             .withConfigurationName(L"process-explicit-mim-client")
                             .withRtiAddress(
                                 L"tcp://127.0.0.1:" +
                                 std::to_wstring(listener->address().port));
    auto const connectionResult =
        rti->connect(federate, rti1516_2025::HLA_EVOKED, configuration);
    REQUIRE(connectionResult.addressUsed);

    auto const fomModule =
        resourcePath("examples/RestaurantFOMmodule-2025.xml").wstring();
    auto const mimModule =
        resourcePath("mim/HLAstandardMIM-2025.xml").wstring();
    REQUIRE_NOTHROW(rti->createFederationExecutionWithMIM(
        L"process-explicit-mim-execution",
        std::vector<std::wstring>{fomModule},
        mimModule,
        L"HLAinteger64Time"));
    auto const joined = rti->joinFederationExecution(
        L"process-explicit-mim-type",
        L"process-explicit-mim-execution");
    REQUIRE(joined.isValid());
    REQUIRE_NOTHROW(rti->resignFederationExecution(rti1516_2025::NO_ACTION));
    REQUIRE_NOTHROW(rti->disconnect());
  } catch (...) {
    clientFailure = std::current_exception();
  }

  listener.reset();
  if (server.joinable()) {
    server.join();
  }
  if (clientFailure) {
    std::rethrow_exception(clientFailure);
  }
  REQUIRE_FALSE(serverFailure);
}
