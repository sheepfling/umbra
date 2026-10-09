#include <catch2/catch_test_macros.hpp>

#include <atomic>
#include <cstddef>
#include <cstdint>
#include <exception>
#include <stdexcept>
#include <string>
#include <thread>

#include <RTI/NullFederateAmbassador.h>
#include <RTI/RTI1516.h>

#include <umbra/embedded_profile_configuration.hpp>

#if defined(UMBRA_ENABLE_EMBEDDED_FEDERATION_MANAGEMENT)
#include "internal/federation/federation_registry.hpp"
#include "internal/federation/process_federation_service.hpp"
#include "internal/federation/process_transport.hpp"
#include "internal/federation/process_transport_session.hpp"
#include "process_public_service_fixture.hpp"
#include "ieee1516_2025_connection_test_support.hpp"
#include "ieee1516_2025_process_fom_test_support.hpp"

namespace {
using rti1516_2025::CallbackModel;
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
using umbra::detail::TransportServiceStatus;
using umbra::test::connection_support_2025::makeRti;
using umbra::test::process_fom_support_2025::composedDirectedProcessDefinition;
using umbra::test::process_fom_support_2025::composedProcessDefinition;

class InteractionTransportationFederateAmbassador final
    : public NullFederateAmbassador {
 public:
  void confirmInteractionTransportationTypeChange(
      rti1516_2025::InteractionClassHandle const &interactionClass,
      rti1516_2025::TransportationTypeHandle const &transportationType) override {
    ++changeCount;
    changedInteractionClass = interactionClass;
    changedTransportationType = transportationType;
  }

  void reportInteractionTransportationType(
      rti1516_2025::FederateHandle const &federate,
      rti1516_2025::InteractionClassHandle const &interactionClass,
      rti1516_2025::TransportationTypeHandle const &transportationType) override {
    ++queryCount;
    queriedFederate = federate;
    queriedInteractionClass = interactionClass;
    queriedTransportationType = transportationType;
  }

  std::size_t changeCount = 0U;
  rti1516_2025::InteractionClassHandle changedInteractionClass;
  rti1516_2025::TransportationTypeHandle changedTransportationType;
  std::size_t queryCount = 0U;
  rti1516_2025::FederateHandle queriedFederate;
  rti1516_2025::InteractionClassHandle queriedInteractionClass;
  rti1516_2025::TransportationTypeHandle queriedTransportationType;
};

}  // namespace

TEST_CASE(
    "RTIambassador routes interaction transportation type change and query through a configured process endpoint",
    "[integration][foundation][interaction-management][transportation][transport][process-boundary][public-endpoint][process-transportation-interaction-control][rti.service.request-interaction-transportation-type-change][rti.service.query-interaction-transportation-type]") {
  auto runScenario = [](CallbackModel callbackModel) {
    auto listener = ProcessTransportListener::listen({"127.0.0.1", 0U});
    REQUIRE(listener);
    auto const port = listener->address().port;
    REQUIRE(port != 0U);

    constexpr wchar_t const *federationName =
        L"public-process-transportation-interaction-execution";
    constexpr wchar_t const *federateName =
        L"public-process-transportation-interaction-federate";
    constexpr char const *interactionName =
        "HLAinteractionRoot.ServerAction.TakeOrder";
    constexpr wchar_t const *interactionNameWide =
        L"HLAinteractionRoot.ServerAction.TakeOrder";
    constexpr char const *transportationTypeName = "HLAbestEffort";
    std::atomic_uint64_t expectedInteractionClass{0U};
    std::atomic_uint64_t expectedTransportation{0U};
    std::exception_ptr serverError;
    std::thread server([&] {
      try {
        EmbeddedFederationRegistry registry;
        ProcessFederationService service(
            registry, composedProcessDefinition(), ProcessFederationServiceOptions{});
        auto connection = listener->accept(
            nullptr,
            {"public-process-transportation-interaction-server", 0x9604U},
            [](std::wstring) {},
            [](std::wstring) { return false; });
        ProcessTransportSession session(connection);
        auto handler = service.handlerFor(session);
        auto serveExpected = [&](TransportServiceOperation operation) {
          return umbra::test::servePrimaryProcessRequest(
              session, handler,
              [&](TransportServiceMessage const &request) {
                if (request.operation != operation) {
                  throw std::runtime_error(
                      "The public process transportation-interaction server received an unexpected operation (expected " +
                      std::to_string(static_cast<unsigned>(operation)) +
                      ", got " +
                      std::to_string(static_cast<unsigned>(request.operation)) +
                      ").");
                }
                return handler(request);
              });
        };

        if (!serveExpected(TransportServiceOperation::create_federation_execution)) {
          throw std::runtime_error(
              "The public process transportation-interaction server lost Create.");
        }
        auto const interactionClass = registry.interactionClassHandleFor(
            federationName, interactionName);
        auto const transportation = registry.transportationTypeHandleFor(
            federationName, transportationTypeName);
        if (!interactionClass || !transportation) {
          throw std::runtime_error(
              "The public process transportation-interaction server could not resolve its FOM handles.");
        }
        expectedInteractionClass.store(
            *interactionClass, std::memory_order_release);
        expectedTransportation.store(
            *transportation, std::memory_order_release);
        if (!serveExpected(TransportServiceOperation::join_federation_execution) ||
            !serveExpected(TransportServiceOperation::get_interaction_class_handle) ||
            !serveExpected(TransportServiceOperation::get_transportation_type_handle) ||
            !serveExpected(TransportServiceOperation::get_federate_handle) ||
            !serveExpected(TransportServiceOperation::publish_interaction_class) ||
            !serveExpected(
                TransportServiceOperation::request_interaction_transportation_type_change) ||
            !serveExpected(TransportServiceOperation::receive_interaction) ||
            !serveExpected(
                TransportServiceOperation::query_interaction_transportation_type) ||
            !serveExpected(TransportServiceOperation::receive_interaction) ||
            !serveExpected(TransportServiceOperation::resign_federation_execution)) {
          throw std::runtime_error(
              "The public process transportation-interaction server lost a declaration, callback, or Resign operation.");
        }
        service.detach(session);
        connection->close();
      } catch (...) {
        serverError = std::current_exception();
      }
    });

    InteractionTransportationFederateAmbassador federate;
    auto rti = makeRti();
    auto configuration = RtiConfiguration::createConfiguration()
                             .withConfigurationName(
                                 L"public-process-transportation-interaction-client")
                             .withRtiAddress(
                                 L"tcp://127.0.0.1:" + std::to_wstring(port));
    std::exception_ptr clientError;
    bool clientJoined = false;
    try {
      REQUIRE(rti->connect(federate, callbackModel, configuration).addressUsed);
      rti->createFederationExecution(federationName, L"server-owned-fom.xml");
      static_cast<void>(rti->joinFederationExecution(
          federateName,
          L"public-process-transportation-interaction-type",
          federationName));
      clientJoined = true;
      auto const interactionClass =
          rti->getInteractionClassHandle(interactionNameWide);
      REQUIRE(interactionClass.toString() ==
              L"InteractionClassHandle(" +
                  std::to_wstring(expectedInteractionClass.load(
                      std::memory_order_acquire)) +
                  L")");
      auto const transportation =
          rti->getTransportationTypeHandle(L"HLAbestEffort");
      REQUIRE(transportation.toString() ==
              L"TransportationTypeHandle(" +
                  std::to_wstring(expectedTransportation.load(
                      std::memory_order_acquire)) +
                  L")");
      auto const queriedFederate = rti->getFederateHandle(federateName);
      REQUIRE(queriedFederate.isValid());
      REQUIRE_NOTHROW(rti->publishInteractionClass(interactionClass));
      REQUIRE_NOTHROW(rti->requestInteractionTransportationTypeChange(
          interactionClass, transportation));
      REQUIRE_NOTHROW(rti->evokeCallback(0.0));
      REQUIRE(federate.changeCount == 1U);
      REQUIRE(federate.changedInteractionClass == interactionClass);
      REQUIRE(federate.changedTransportationType == transportation);
      REQUIRE_NOTHROW(rti->queryInteractionTransportationType(
          queriedFederate, interactionClass));
      REQUIRE_NOTHROW(rti->evokeCallback(0.0));
      REQUIRE(federate.queryCount == 1U);
      REQUIRE(federate.queriedFederate == queriedFederate);
      REQUIRE(federate.queriedInteractionClass == interactionClass);
      REQUIRE(federate.queriedTransportationType == transportation);
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
    REQUIRE(expectedInteractionClass.load(std::memory_order_acquire) != 0U);
  };

  SECTION("HLA_EVOKED") {
    runScenario(HLA_EVOKED);
  }
  SECTION("HLA_IMMEDIATE") {
    runScenario(HLA_IMMEDIATE);
  }
}

TEST_CASE(
    "RTIambassador reports a directed interaction transportation override through a configured process endpoint",
    "[integration][foundation][interaction-management][object-management][transportation][transport]"
    "[directed-interaction][directed-routing][process-boundary][public-endpoint]"
    "[process-directed-interaction-transportation-query][callbacks][callback-immediate][2025]"
    "[rti.service.publish-object-class-directed-interactions][rti.service.request-interaction-transportation-type-change]"
    "[rti.service.query-interaction-transportation-type]"
    "[federate.callback.confirm-interaction-transportation-type-change]"
    "[federate.callback.report-interaction-transportation-type]") {
  auto runScenario = [](CallbackModel callbackModel) {
    constexpr wchar_t const *federationName =
        L"public-process-directed-transportation-query-execution";
    constexpr wchar_t const *federateName =
        L"public-process-directed-transportation-query-federate";
    constexpr char const *objectClassName =
        "HLAobjectRoot.UmbraDirectedFixtureObject";
    constexpr char const *interactionClassName =
        "HLAinteractionRoot.UmbraDirectedFixtureInteraction";
    constexpr char const *transportationTypeName = "HLAbestEffort";

    auto listener = ProcessTransportListener::listen({"127.0.0.1", 0U});
    REQUIRE(listener);
    auto const port = listener->address().port;
    REQUIRE(port != 0U);

    std::atomic_uint64_t expectedObjectClass{0U};
    std::atomic_uint64_t expectedInteractionClass{0U};
    std::atomic_uint64_t expectedTransportation{0U};
    std::exception_ptr serverError;
    std::thread server([&] {
      try {
        EmbeddedFederationRegistry registry;
        ProcessFederationService service(
            registry,
            composedDirectedProcessDefinition(),
            ProcessFederationServiceOptions{});
        auto connection = listener->accept(
            nullptr,
            {"public-process-directed-transportation-query-server", 0x9741U},
            [](std::wstring) {},
            [](std::wstring) { return false; });
        ProcessTransportSession session(connection);
        auto handler = service.handlerFor(session);
        auto serveExpected = [&](TransportServiceOperation operation,
                                 char const *description) {
          if (!umbra::test::servePrimaryProcessRequest(
                  session, handler,
                  [&](TransportServiceMessage const &request) {
                    if (request.operation != operation) {
                      throw std::runtime_error(
                          std::string(description) + " expected operation " +
                          std::to_string(static_cast<unsigned>(operation)) +
                          " but received " +
                          std::to_string(static_cast<unsigned>(request.operation)));
                    }
                    auto response = handler(request);
                    if (response.status != TransportServiceStatus::ok) {
                      throw std::runtime_error(
                          std::string(description) + " handler status " +
                          std::to_string(static_cast<unsigned>(response.status)));
                    }
                    return response;
                  })) {
            throw std::runtime_error(description);
          }
        };

        serveExpected(
            TransportServiceOperation::create_federation_execution,
            "The directed transportation query server lost Create.");
        auto const objectClass = registry.objectClassHandleFor(
            federationName, objectClassName);
        auto const interactionClass = registry.interactionClassHandleFor(
            federationName, interactionClassName);
        auto const transportation = registry.transportationTypeHandleFor(
            federationName, transportationTypeName);
        if (!objectClass || !interactionClass || !transportation) {
          throw std::runtime_error(
              "The directed transportation query server could not resolve its FOM handles.");
        }
        expectedObjectClass.store(*objectClass, std::memory_order_release);
        expectedInteractionClass.store(*interactionClass, std::memory_order_release);
        expectedTransportation.store(*transportation, std::memory_order_release);

        serveExpected(
            TransportServiceOperation::join_federation_execution,
            "The directed transportation query server lost Join.");
        serveExpected(
            TransportServiceOperation::get_object_class_handle,
            "The directed transportation query server lost object lookup.");
        serveExpected(
            TransportServiceOperation::get_interaction_class_handle,
            "The directed transportation query server lost interaction lookup.");
        serveExpected(
            TransportServiceOperation::get_transportation_type_handle,
            "The directed transportation query server lost transportation lookup.");
        serveExpected(
            TransportServiceOperation::get_federate_handle,
            "The directed transportation query server lost federate lookup.");
        serveExpected(
            TransportServiceOperation::publish_object_class_directed_interactions,
            "The directed transportation query server lost directed publication.");
        serveExpected(
            TransportServiceOperation::request_interaction_transportation_type_change,
            "The directed transportation query server lost directed transportation change.");
        serveExpected(
            TransportServiceOperation::receive_interaction,
            "The directed transportation query server lost transportation confirmation.");
        serveExpected(
            TransportServiceOperation::query_interaction_transportation_type,
            "The directed transportation query server lost transportation query.");
        serveExpected(
            TransportServiceOperation::receive_interaction,
            "The directed transportation query server lost transportation report.");
        serveExpected(
            TransportServiceOperation::resign_federation_execution,
            "The directed transportation query server lost Resign.");
        service.detach(session);
        connection->close();
      } catch (...) {
        serverError = std::current_exception();
      }
    });

    InteractionTransportationFederateAmbassador federate;
    auto rti = makeRti();
    auto configuration = RtiConfiguration::createConfiguration()
                             .withConfigurationName(
                                 L"public-process-directed-transportation-query-client")
                             .withRtiAddress(
                                 L"tcp://127.0.0.1:" + std::to_wstring(port));
    std::exception_ptr clientError;
    bool clientJoined = false;
    try {
      REQUIRE(rti->connect(federate, callbackModel, configuration).addressUsed);
      rti->createFederationExecution(
          federationName, L"server-owned-directed-transportation-fom.xml");
      static_cast<void>(rti->joinFederationExecution(
          federateName,
          L"public-process-directed-transportation-query-type",
          federationName));
      clientJoined = true;

      auto const objectClass = rti->getObjectClassHandle(
          L"HLAobjectRoot.UmbraDirectedFixtureObject");
      auto const interactionClass = rti->getInteractionClassHandle(
          L"HLAinteractionRoot.UmbraDirectedFixtureInteraction");
      auto const transportation = rti->getTransportationTypeHandle(
          L"HLAbestEffort");
      REQUIRE(objectClass.toString() ==
              L"ObjectClassHandle(" +
                  std::to_wstring(expectedObjectClass.load(
                      std::memory_order_acquire)) +
                  L")");
      REQUIRE(interactionClass.toString() ==
              L"InteractionClassHandle(" +
                  std::to_wstring(expectedInteractionClass.load(
                      std::memory_order_acquire)) +
                  L")");
      REQUIRE(transportation.toString() ==
              L"TransportationTypeHandle(" +
                  std::to_wstring(expectedTransportation.load(
                      std::memory_order_acquire)) +
                  L")");
      auto const queriedFederate = rti->getFederateHandle(federateName);
      REQUIRE(queriedFederate.isValid());
      REQUIRE_NOTHROW(rti->publishObjectClassDirectedInteractions(
          objectClass, rti1516_2025::InteractionClassHandleSet{interactionClass}));
      REQUIRE_NOTHROW(rti->requestInteractionTransportationTypeChange(
          interactionClass, transportation));
      REQUIRE_NOTHROW(rti->evokeCallback(0.0));
      REQUIRE(federate.changeCount == 1U);
      REQUIRE(federate.changedInteractionClass == interactionClass);
      REQUIRE(federate.changedTransportationType == transportation);

      REQUIRE_NOTHROW(rti->queryInteractionTransportationType(
          queriedFederate, interactionClass));
      REQUIRE_NOTHROW(rti->evokeCallback(0.0));
      REQUIRE(federate.queryCount == 1U);
      REQUIRE(federate.queriedFederate == queriedFederate);
      REQUIRE(federate.queriedInteractionClass == interactionClass);
      REQUIRE(federate.queriedTransportationType == transportation);
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
    listener.reset();
    if (server.joinable()) {
      server.join();
    }
    if (serverError) {
      try {
        std::rethrow_exception(serverError);
      } catch (std::exception const &error) {
        FAIL_CHECK(std::string("directed transportation query server: ") +
                   error.what());
      } catch (...) {
        FAIL_CHECK(
            "directed transportation query server failed with an unknown exception");
      }
    }
    if (clientError) {
      std::rethrow_exception(clientError);
    }
    REQUIRE_FALSE(serverError);
    REQUIRE_FALSE(clientJoined);
    REQUIRE(expectedObjectClass.load(std::memory_order_acquire) != 0U);
  };

  SECTION("HLA_EVOKED") {
    runScenario(HLA_EVOKED);
  }
  SECTION("HLA_IMMEDIATE") {
    runScenario(HLA_IMMEDIATE);
  }
}


#endif  // UMBRA_ENABLE_EMBEDDED_FEDERATION_MANAGEMENT
