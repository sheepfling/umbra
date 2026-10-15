#include <catch2/catch_test_macros.hpp>

#include <array>
#include <atomic>
#include <chrono>
#include <cstddef>
#include <cstdint>
#include <exception>
#include <limits>
#include <set>
#include <stdexcept>
#include <string>
#include <thread>
#include <vector>

#include <RTI/NullFederateAmbassador.h>
#include <RTI/RTI1516.h>

#include "internal/federation/federation_registry.hpp"
#include "internal/federation/process_federation_service.hpp"
#include "internal/federation/process_transport.hpp"
#include "internal/federation/process_transport_session.hpp"
#include "internal/handles/attribute_handle.hpp"
#include "ieee1516_2025_connection_test_support.hpp"
#include "ieee1516_2025_process_fom_test_support.hpp"
#include "process_public_service_fixture.hpp"

#if defined(UMBRA_ENABLE_EMBEDDED_FEDERATION_MANAGEMENT)
namespace {

using rti1516_2025::CallbackModel;
using rti1516_2025::DELETE_OBJECTS;
using rti1516_2025::HLA_EVOKED;
using rti1516_2025::HLA_IMMEDIATE;
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
using umbra::detail::TransportServiceStatus;
using umbra::test::connection_support_2025::makeRti;
using umbra::test::process_fom_support_2025::composedProcessDefinition;

TEST_CASE(
    "RTIambassador expands process class Request Attribute Value Update across registered instances",
    "[integration][foundation][object-management][callbacks][callback-controls]"
    "[transport][process-boundary][process-class-request-attribute-value-update]"
    "[public-endpoint][rti.service.request-attribute-value-update]"
    "[federate.callback.discover-object-instance]"
    "[federate.callback.provide-attribute-value-update]") {
  auto runScenario = [](CallbackModel callbackModel) {
    class RecordingFederateAmbassador final : public NullFederateAmbassador {
     public:
      void discoverObjectInstance(
          rti1516_2025::ObjectInstanceHandle const& objectInstance,
          rti1516_2025::ObjectClassHandle const& objectClass,
          std::wstring const& objectInstanceName,
          rti1516_2025::FederateHandle const& producingFederate) override {
        discoveredObjects.push_back(objectInstance);
        discoveredClasses.push_back(objectClass);
        discoveredNames.push_back(objectInstanceName);
        discoveredProducers.push_back(producingFederate);
      }

      void provideAttributeValueUpdate(
          rti1516_2025::ObjectInstanceHandle const& objectInstance,
          rti1516_2025::AttributeHandleSet const& attributes,
          rti1516_2025::VariableLengthData const& userSuppliedTag) override {
        providedObjects.push_back(objectInstance);
        providedAttributeCounts.push_back(attributes.size());
        providedTags.emplace_back();
        auto& tag = providedTags.back();
        if (userSuppliedTag.size() != 0U) {
          auto const* first = static_cast<std::uint8_t const*>(
              userSuppliedTag.data());
          tag.assign(first, first + userSuppliedTag.size());
        }
      }

      std::vector<rti1516_2025::ObjectInstanceHandle> discoveredObjects;
      std::vector<rti1516_2025::ObjectClassHandle> discoveredClasses;
      std::vector<std::wstring> discoveredNames;
      std::vector<rti1516_2025::FederateHandle> discoveredProducers;
      std::vector<rti1516_2025::ObjectInstanceHandle> providedObjects;
      std::vector<std::size_t> providedAttributeCounts;
      std::vector<std::vector<std::uint8_t>> providedTags;
    } providerFederate, requesterFederate;

    auto listener = ProcessTransportListener::listen({"127.0.0.1", 0U});
    REQUIRE(listener);
    auto const port = listener->address().port;
    REQUIRE(port != 0U);

    constexpr wchar_t const* federationNameWide =
        L"public-process-class-attribute-execution";
    constexpr wchar_t const* objectClassNameWide = L"HLAobjectRoot.Employee";
    constexpr char const* objectClassName = "HLAobjectRoot.Employee";
    constexpr wchar_t const* attributeNameWide = L"Name";
    constexpr char const* attributeName = "Name";
    std::atomic_uint64_t expectedObjectClass{0U};
    std::atomic_uint64_t expectedAttribute{0U};
    std::atomic_uint64_t expectedProviderFederate{0U};
    std::atomic_uint64_t expectedRequesterFederate{0U};
    std::atomic_uint64_t recipientCount{
        std::numeric_limits<std::uint32_t>::max()};
    std::exception_ptr serverError;
    std::thread server([&] {
      try {
        EmbeddedFederationRegistry registry;
        ProcessFederationService service(
            registry,
            composedProcessDefinition(),
            ProcessFederationServiceOptions{callbackModel == HLA_IMMEDIATE});
        auto providerConnection = listener->accept(
            nullptr,
            {"public-process-class-attribute-server", 0x9A11U},
            [](std::wstring) {},
            [](std::wstring) { return false; });
        ProcessTransportSession provider(providerConnection);
        auto providerBaseHandler = service.handlerFor(provider);
        auto providerHandler = [&](TransportServiceMessage const& request) {
          auto response = providerBaseHandler(request);
          if (request.operation ==
                  TransportServiceOperation::join_federation_execution &&
              response.status == TransportServiceStatus::ok) {
            auto const join = umbra::detail::decodeProcessFederationJoinResult(
                response.payload);
            expectedProviderFederate.store(
                join.federateId, std::memory_order_release);
          }
          return response;
        };
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
            provider,
            providerHandler,
            TransportServiceOperation::create_federation_execution,
            "The public process class server lost Create.");
        auto const objectClass = registry.objectClassHandleFor(
            federationNameWide, objectClassName);
        auto const attribute = registry.attributeHandleFor(
            federationNameWide, objectClassName, attributeName);
        if (!objectClass || !attribute) {
          throw std::runtime_error(
              "The public process class server could not resolve its FOM handles.");
        }
        expectedObjectClass.store(*objectClass, std::memory_order_release);
        expectedAttribute.store(*attribute, std::memory_order_release);
        serveExpected(
            provider,
            providerHandler,
            TransportServiceOperation::join_federation_execution,
            "The public process class server lost provider Join.");

        auto requesterConnection = listener->accept(
            nullptr,
            {"public-process-class-attribute-server", 0x9A12U},
            [](std::wstring) {},
            [](std::wstring) { return false; });
        ProcessTransportSession requester(requesterConnection);
        auto requesterBaseHandler = service.handlerFor(requester);
        auto requesterHandler = [&](TransportServiceMessage const& request) {
          auto response = requesterBaseHandler(request);
          if (request.operation ==
                  TransportServiceOperation::join_federation_execution &&
              response.status == TransportServiceStatus::ok) {
            auto const join = umbra::detail::decodeProcessFederationJoinResult(
                response.payload);
            expectedRequesterFederate.store(
                join.federateId, std::memory_order_release);
          }
          if (request.operation ==
                  TransportServiceOperation::request_attribute_value_update_class &&
              response.status == TransportServiceStatus::ok) {
            recipientCount.store(
                umbra::detail::decodeProcessFederationRequestAttributeValueUpdateResult(
                    response.payload)
                    .recipientCount,
                std::memory_order_release);
          }
          return response;
        };
        serveExpected(
            requester,
            requesterHandler,
            TransportServiceOperation::join_federation_execution,
            "The public process class server lost requester Join.");

        auto const providerMember = registry.memberByName(
            federationNameWide, L"public-process-class-attribute-provider");
        auto const requesterMember = registry.memberByName(
            federationNameWide, L"public-process-class-attribute-requester");
        if (!providerMember || !requesterMember ||
            registry.setObjectClassAttributePublication(
                federationNameWide,
                providerMember->id,
                *objectClass,
                std::set<std::uint64_t>{*attribute},
                true) !=
                umbra::detail::ObjectClassAttributeDeclarationStatus::applied ||
            registry.setObjectClassAttributeSubscription(
                federationNameWide,
                requesterMember->id,
                *objectClass,
                std::set<std::uint64_t>{*attribute},
                true) !=
                umbra::detail::ObjectClassAttributeDeclarationStatus::applied) {
          throw std::runtime_error(
              "The public process class declarations were rejected.");
        }
        // Once both federates join, unsolicited callbacks may already be
        // queued when Evoke begins. In that case Evoke does not issue a
        // process poll. Serve each session independently instead of scripting
        // alternating polls that might never arrive.
        auto serveJoinedSession = [&](ProcessTransportSession& session,
                                      auto const& handler,
                                      bool isProvider) {
          bool resigned = false;
          while (!resigned) {
            if (!umbra::test::servePrimaryProcessRequest(
                    session,
                    handler,
                    [&](TransportServiceMessage const& request) {
                      auto const operation = request.operation;
                      bool const providerOperation =
                          operation == TransportServiceOperation::publish_object_class_attributes ||
                          operation == TransportServiceOperation::register_object_instance;
                      bool const requesterOperation =
                          operation == TransportServiceOperation::request_attribute_value_update_class;
                      bool const commonOperation =
                          operation == TransportServiceOperation::get_object_class_handle ||
                          operation == TransportServiceOperation::get_attribute_handle ||
                          operation == TransportServiceOperation::receive_interaction ||
                          operation == TransportServiceOperation::resign_federation_execution;
                      if (!commonOperation &&
                          !(isProvider ? providerOperation : requesterOperation)) {
                        throw std::runtime_error(
                            "The public process class server received an unexpected operation.");
                      }
                      auto response = handler(request);
                      if (response.status != TransportServiceStatus::ok) {
                        throw std::runtime_error(
                            "The public process class server rejected an operation.");
                      }
                      resigned = operation ==
                          TransportServiceOperation::resign_federation_execution;
                      return response;
                    })) {
              throw std::runtime_error(
                  "The public process class server lost a joined session.");
            }
          }
        };
        std::exception_ptr providerSessionError;
        std::thread providerWorker([&] {
          try {
            serveJoinedSession(provider, providerHandler, true);
          } catch (...) {
            providerSessionError = std::current_exception();
            providerConnection->close();
            requesterConnection->close();
          }
        });
        std::exception_ptr requesterSessionError;
        try {
          serveJoinedSession(requester, requesterHandler, false);
        } catch (...) {
          requesterSessionError = std::current_exception();
          providerConnection->close();
          requesterConnection->close();
        }
        providerWorker.join();
        if (providerSessionError) {
          std::rethrow_exception(providerSessionError);
        }
        if (requesterSessionError) {
          std::rethrow_exception(requesterSessionError);
        }
        service.detach(provider);
        service.detach(requester);
        providerConnection->close();
        requesterConnection->close();
      } catch (...) {
        serverError = std::current_exception();
      }
    });

    auto providerRti = makeRti();
    auto requesterRti = makeRti();
    // One Evoke need not deliver the callback under test: advisory callbacks
    // can be interleaved, and a process poll can return before the typed event
    // reaches the evoked dispatcher. Wait for the observed callback count.
    auto evokeUntil = [](auto& rti, auto const& recordedEvents,
                         std::size_t targetCount) {
      auto const deadline = std::chrono::steady_clock::now() +
                            std::chrono::seconds(5);
      while (recordedEvents.size() < targetCount) {
        auto const remaining = deadline - std::chrono::steady_clock::now();
        if (remaining <= std::chrono::steady_clock::duration::zero()) {
          break;
        }
        static_cast<void>(rti->evokeCallback(
            std::chrono::duration<double>(remaining).count()));
      }
    };
    auto configuration = RtiConfiguration::createConfiguration()
                             .withConfigurationName(
                                 L"public-process-class-attribute-client")
                             .withRtiAddress(
                                 L"tcp://127.0.0.1:" + std::to_wstring(port));
    std::exception_ptr clientError;
    bool providerJoined = false;
    bool requesterJoined = false;
    std::vector<rti1516_2025::ObjectInstanceHandle> objectInstances;
    try {
      REQUIRE(providerRti->connect(providerFederate, callbackModel, configuration)
                  .addressUsed);
      providerRti->createFederationExecution(
          federationNameWide, L"server-owned-fom.xml");
      providerRti->joinFederationExecution(
          L"public-process-class-attribute-provider",
          L"public-process-class-attribute-type",
          federationNameWide);
      providerJoined = true;

      REQUIRE(requesterRti->connect(requesterFederate, callbackModel, configuration)
                  .addressUsed);
      requesterRti->joinFederationExecution(
          L"public-process-class-attribute-requester",
          L"public-process-class-attribute-type",
          federationNameWide);
      requesterJoined = true;

      auto const providerObjectClass =
          providerRti->getObjectClassHandle(objectClassNameWide);
      auto const providerAttribute =
          providerRti->getAttributeHandle(providerObjectClass, attributeNameWide);
      auto const requesterObjectClass =
          requesterRti->getObjectClassHandle(objectClassNameWide);
      auto const requesterAttribute =
          requesterRti->getAttributeHandle(requesterObjectClass, attributeNameWide);
      REQUIRE(providerObjectClass.toString() ==
              L"ObjectClassHandle(" +
                  std::to_wstring(expectedObjectClass.load(
                      std::memory_order_acquire)) +
                  L")");
      REQUIRE(providerAttribute ==
              rti1516_2025::umbra_binding_detail::makeAttributeHandle(
                  expectedAttribute.load(std::memory_order_acquire)));
      REQUIRE(requesterObjectClass == providerObjectClass);
      REQUIRE(requesterAttribute == providerAttribute);

      rti1516_2025::AttributeHandleSet publishedAttributes;
      publishedAttributes.insert(providerAttribute);
      providerRti->publishObjectClassAttributes(
          providerObjectClass, publishedAttributes);
      for (std::size_t index = 0U; index < 2U; ++index) {
        auto const objectInstance =
            providerRti->registerObjectInstance(providerObjectClass);
        REQUIRE(objectInstance.isValid());
        objectInstances.push_back(objectInstance);
        if (callbackModel == HLA_IMMEDIATE) {
          static_cast<void>(providerRti->getObjectClassHandle(objectClassNameWide));
          static_cast<void>(requesterRti->getObjectClassHandle(objectClassNameWide));
        } else {
          static_cast<void>(providerRti->evokeCallback(0.0));
          evokeUntil(requesterRti, requesterFederate.discoveredObjects,
                     index + 1U);
        }
      }
      REQUIRE(requesterFederate.discoveredObjects.size() == 2U);
      REQUIRE(requesterFederate.discoveredObjects[0] == objectInstances[0]);
      REQUIRE(requesterFederate.discoveredObjects[1] == objectInstances[1]);

      rti1516_2025::AttributeHandleSet requestedAttributes;
      requestedAttributes.insert(requesterAttribute);
      std::array<std::uint8_t, 3U> encodedTag{0x43U, 0x4CU, 0x53U};
      VariableLengthData userSuppliedTag(encodedTag.data(), encodedTag.size());
      REQUIRE_NOTHROW(requesterRti->requestAttributeValueUpdate(
          requesterObjectClass, requestedAttributes, userSuppliedTag));
      if (callbackModel == HLA_IMMEDIATE) {
        static_cast<void>(providerRti->getObjectClassHandle(objectClassNameWide));
        static_cast<void>(providerRti->getObjectClassHandle(objectClassNameWide));
      } else {
        evokeUntil(providerRti, providerFederate.providedObjects, 2U);
      }
      REQUIRE(providerFederate.providedObjects.size() == 2U);
      REQUIRE(providerFederate.providedObjects[0] == objectInstances[0]);
      REQUIRE(providerFederate.providedObjects[1] == objectInstances[1]);
      REQUIRE(providerFederate.providedAttributeCounts ==
              std::vector<std::size_t>{1U, 1U});
      REQUIRE(providerFederate.providedTags ==
              std::vector<std::vector<std::uint8_t>>{
                  {0x43U, 0x4CU, 0x53U}, {0x43U, 0x4CU, 0x53U}});

      providerRti->resignFederationExecution(DELETE_OBJECTS);
      providerJoined = false;
      requesterRti->resignFederationExecution(NO_ACTION);
      requesterJoined = false;
      providerRti->disconnect();
      requesterRti->disconnect();
    } catch (...) {
      clientError = std::current_exception();
      // An assertion may abort before either joined session resigns. Close
      // both sockets so their server workers can exit rather than deadlocking
      // cleanup on a scripted resignation or another outstanding request.
      requesterRti.reset();
      providerRti.reset();
    }
    listener.reset();
    if (server.joinable()) {
      server.join();
    }
    if (clientError) {
      std::rethrow_exception(clientError);
    }
    REQUIRE_FALSE(serverError);
    REQUIRE(recipientCount.load(std::memory_order_acquire) == 2U);
    REQUIRE(expectedProviderFederate.load(std::memory_order_acquire) != 0U);
    REQUIRE(expectedRequesterFederate.load(std::memory_order_acquire) != 0U);
    REQUIRE_FALSE(providerJoined);
    REQUIRE_FALSE(requesterJoined);
  };

  SECTION("HLA_EVOKED") {
    runScenario(HLA_EVOKED);
  }
  SECTION("HLA_IMMEDIATE") {
    runScenario(HLA_IMMEDIATE);
  }
}

}  // namespace
#endif
