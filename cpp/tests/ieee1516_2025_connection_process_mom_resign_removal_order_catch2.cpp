#include <catch2/catch_test_macros.hpp>

#include <algorithm>
#include <exception>
#include <filesystem>
#include <optional>
#include <stdexcept>
#include <string>
#include <system_error>
#include <thread>
#include <utility>
#include <vector>

#include <RTI/NullFederateAmbassador.h>
#include <RTI/RTI1516.h>
#include <RTI/encoding/BasicDataElements.h>

#if defined(UMBRA_ENABLE_EMBEDDED_FEDERATION_MANAGEMENT)
#include "internal/federation/federation_registry.hpp"
#include "internal/federation/process_federation_service.hpp"
#include "internal/federation/process_transport.hpp"
#include "internal/federation/process_transport_session.hpp"
#include "ieee1516_2025_connection_test_support.hpp"
#include "ieee1516_2025_process_fom_test_support.hpp"
#include "process_public_service_fixture.hpp"

namespace {
using rti1516_2025::HLA_EVOKED;
using rti1516_2025::HLA_IMMEDIATE;
using rti1516_2025::NO_ACTION;
using rti1516_2025::NullFederateAmbassador;
using rti1516_2025::RtiConfiguration;
using umbra::test::connection_support_2025::makeRti;
using umbra::test::connection_support_2025::temporaryServiceReportDirectory;
using umbra::test::process_fom_support_2025::composedProcessDefinition;
}  // namespace
TEST_CASE(
    "RTIambassador delivers resigned HLAfederate MOM removal before rejoined-object discovery",
    "[integration][internal][foundation][federation-management][mom]"
    "[transport][process-boundary][public-endpoint][2025][process-mom-resign-removal-order]"
    "[rti.service.connect][rti.service.create-federation-execution]"
    "[rti.service.join-federation-execution][rti.service.resign-federation-execution]"
    "[rti.service.get-object-class-handle][rti.service.get-attribute-handle]"
    "[rti.service.subscribe-object-class-attributes][rti.service.evoke-callback]"
    "[rti.service.disconnect][federate.callback.discover-object-instance]"
    "[federate.callback.reflect-attribute-values][federate.callback.remove-object-instance]") {
  class RecordingFederateAmbassador final : public NullFederateAmbassador {
   public:
    enum class CallbackKind { discovery, reflection, removal };
    struct CallbackEvent final {
      CallbackKind kind;
      rti1516_2025::ObjectInstanceHandle objectInstance;
    };
    struct Reflection final {
      rti1516_2025::ObjectInstanceHandle objectInstance;
      rti1516_2025::AttributeHandleValueMap attributeValues;
    };

    void discoverObjectInstance(
        rti1516_2025::ObjectInstanceHandle const& objectInstance,
        rti1516_2025::ObjectClassHandle const&,
        std::wstring const&,
        rti1516_2025::FederateHandle const&) override {
      callbackEvents.push_back({CallbackKind::discovery, objectInstance});
    }

    void reflectAttributeValues(
        rti1516_2025::ObjectInstanceHandle const& objectInstance,
        rti1516_2025::AttributeHandleValueMap const& attributeValues,
        rti1516_2025::VariableLengthData const&,
        rti1516_2025::TransportationTypeHandle const&,
        rti1516_2025::FederateHandle const&,
        rti1516_2025::RegionHandleSet const*) override {
      callbackEvents.push_back({CallbackKind::reflection, objectInstance});
      reflections.push_back({objectInstance, attributeValues});
    }

    void removeObjectInstance(
        rti1516_2025::ObjectInstanceHandle const& objectInstance,
        rti1516_2025::VariableLengthData const&,
        rti1516_2025::FederateHandle const&) override {
      callbackEvents.push_back({CallbackKind::removal, objectInstance});
    }

    std::vector<CallbackEvent> callbackEvents;
    std::vector<Reflection> reflections;
  } observerFederate;

  using umbra::detail::EmbeddedFederationRegistry;
  using umbra::detail::ProcessFederationService;
  using umbra::detail::ProcessFederationServiceOptions;
using umbra::detail::ProcessTransportListener;
using umbra::detail::ProcessTransportSession;
using umbra::detail::TransportServiceMessage;
using umbra::detail::TransportServiceOperation;

  auto listener = ProcessTransportListener::listen({"127.0.0.1", 0U});
  REQUIRE(listener);
  auto const port = listener->address().port;
  REQUIRE(port != 0U);
  auto const reportDirectory = temporaryServiceReportDirectory();
  constexpr wchar_t const* federationName =
      L"public-process-mom-resign-removal-order-execution";
  constexpr wchar_t const* subjectName = L"public-process-mom-removal-subject";
  constexpr wchar_t const* observerName = L"public-process-mom-removal-observer";

  std::exception_ptr serverError;
  auto server = std::thread([&] {
    try {
      EmbeddedFederationRegistry registry;
      ProcessFederationServiceOptions options;
      options.serviceReportDirectory = reportDirectory;
      ProcessFederationService service(
          registry, composedProcessDefinition(), std::move(options));
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

      auto subjectConnection = listener->accept(
          nullptr,
          {"public-process-mom-removal-server", 0x9D11U},
          [](std::wstring) {},
          [](std::wstring) { return false; });
      ProcessTransportSession subjectSession(subjectConnection);
      auto subjectHandler = service.handlerFor(subjectSession);
      serveExpected(subjectSession, subjectHandler,
                    TransportServiceOperation::create_federation_execution,
                    "The process MOM lifecycle server lost Create.");
      serveExpected(subjectSession, subjectHandler,
                    TransportServiceOperation::join_federation_execution,
                    "The process MOM lifecycle server lost the initial subject Join.");

      auto observerConnection = listener->accept(
          nullptr,
          {"public-process-mom-removal-server", 0x9D12U},
          [](std::wstring) {},
          [](std::wstring) { return false; });
      ProcessTransportSession observerSession(observerConnection);
      auto observerHandler = service.handlerFor(observerSession);
      serveExpected(observerSession, observerHandler,
                    TransportServiceOperation::join_federation_execution,
                    "The process MOM lifecycle server lost observer Join.");
      serveExpected(observerSession, observerHandler,
                    TransportServiceOperation::get_object_class_handle,
                    "The process MOM lifecycle server lost the MOM class lookup.");
      serveExpected(observerSession, observerHandler,
                    TransportServiceOperation::get_attribute_handle,
                    "The process MOM lifecycle server lost the federate-name lookup.");
      serveExpected(observerSession, observerHandler,
                    TransportServiceOperation::subscribe_object_class_attributes,
                    "The process MOM lifecycle server lost the MOM subscription.");

      std::exception_ptr observerServerError;
      std::thread observerServer([&] {
        try {
          while (umbra::test::servePrimaryProcessRequest(
              observerSession, observerHandler)) {
          }
        } catch (...) {
          observerServerError = std::current_exception();
        }
      });
      try {
        serveExpected(subjectSession, subjectHandler,
                      TransportServiceOperation::resign_federation_execution,
                      "The process MOM lifecycle server lost the subject Resign.");
        serveExpected(subjectSession, subjectHandler,
                      TransportServiceOperation::join_federation_execution,
                      "The process MOM lifecycle server lost the subject rejoin.");
        serveExpected(subjectSession, subjectHandler,
                      TransportServiceOperation::resign_federation_execution,
                      "The process MOM lifecycle server lost the final subject Resign.");
      } catch (...) {
        observerConnection->close();
        if (observerServer.joinable()) {
          observerServer.join();
        }
        throw;
      }
      if (observerServer.joinable()) {
        observerServer.join();
      }
      if (observerServerError) {
        std::rethrow_exception(observerServerError);
      }
      service.detach(subjectSession);
      service.detach(observerSession);
      subjectConnection->close();
      observerConnection->close();
    } catch (...) {
      serverError = std::current_exception();
    }
  });

  NullFederateAmbassador subjectFederate;
  auto subject = makeRti();
  auto observer = makeRti();
  auto configuration = RtiConfiguration::createConfiguration()
                           .withConfigurationName(
                               L"public-process-mom-removal-order-client")
                           .withRtiAddress(
                               L"tcp://127.0.0.1:" + std::to_wstring(port));
  constexpr wchar_t const* federateType = L"public-process-mom-removal-order-type";
  std::exception_ptr clientError;
  bool subjectJoined = false;
  bool observerJoined = false;
  try {
    rti1516_2025::HLAnoCredentials credentials;
    REQUIRE(subject->connect(
                subjectFederate, HLA_IMMEDIATE, configuration, credentials)
                .addressUsed);
    subject->createFederationExecution(federationName, L"server-owned-fom.xml");
    REQUIRE(subject->joinFederationExecution(
                subjectName, federateType, federationName)
                .isValid());
    subjectJoined = true;

    REQUIRE(observer->connect(observerFederate, HLA_EVOKED, configuration).addressUsed);
    REQUIRE(observer->joinFederationExecution(
                observerName, federateType, federationName)
                .isValid());
    observerJoined = true;
    auto const momClass = observer->getObjectClassHandle(
        L"HLAobjectRoot.HLAmanager.HLAfederate");
    auto const federateNameAttribute =
        observer->getAttributeHandle(momClass, L"HLAfederateName");
    REQUIRE(momClass.isValid());
    REQUIRE(federateNameAttribute.isValid());
    REQUIRE_NOTHROW(observer->subscribeObjectClassAttributes(
        momClass, rti1516_2025::AttributeHandleSet{federateNameAttribute}));

    auto subjectReflectionForName = [&](
                                           std::optional<rti1516_2025::ObjectInstanceHandle>
                                               excludedObject = std::nullopt) {
      return std::find_if(
          observerFederate.reflections.begin(), observerFederate.reflections.end(),
          [&](RecordingFederateAmbassador::Reflection const& reflection) {
            if (excludedObject && reflection.objectInstance == *excludedObject) {
              return false;
            }
            auto const value = reflection.attributeValues.find(federateNameAttribute);
            if (value == reflection.attributeValues.end()) {
              return false;
            }
            rti1516_2025::HLAunicodeString name;
            try {
              name.decode(value->second);
            } catch (...) {
              return false;
            }
            return name.get() == subjectName;
          });
    };
    for (std::size_t pass = 0U;
         pass != 64U && observerFederate.reflections.size() < 2U;
         ++pass) {
      static_cast<void>(observer->evokeCallback(0.0));
    }
    REQUIRE(observerFederate.reflections.size() >= 2U);
    auto const initialSubjectReflection = subjectReflectionForName();
    REQUIRE(initialSubjectReflection != observerFederate.reflections.end());
    auto const resignedObject = initialSubjectReflection->objectInstance;

    subject->resignFederationExecution(NO_ACTION);
    subjectJoined = false;
    REQUIRE(subject->joinFederationExecution(
                subjectName, federateType, federationName)
                .isValid());
    subjectJoined = true;

    for (std::size_t pass = 0U;
         pass != 128U &&
         std::none_of(
             observerFederate.reflections.begin(), observerFederate.reflections.end(),
             [&](RecordingFederateAmbassador::Reflection const& reflection) {
               if (reflection.objectInstance == resignedObject) {
                 return false;
               }
               auto const value = reflection.attributeValues.find(federateNameAttribute);
               if (value == reflection.attributeValues.end()) {
                 return false;
               }
               rti1516_2025::HLAunicodeString name;
               try {
                 name.decode(value->second);
               } catch (...) {
                 return false;
               }
               return name.get() == subjectName;
             });
         ++pass) {
      static_cast<void>(observer->evokeCallback(0.0));
    }
    auto const rejoinedSubjectReflection = subjectReflectionForName(resignedObject);
    REQUIRE(rejoinedSubjectReflection != observerFederate.reflections.end());

    auto const removal = std::find_if(
        observerFederate.callbackEvents.begin(), observerFederate.callbackEvents.end(),
        [&](RecordingFederateAmbassador::CallbackEvent const& event) {
          return event.kind == RecordingFederateAmbassador::CallbackKind::removal &&
                 event.objectInstance == resignedObject;
        });
    auto const rejoinedDiscovery = std::find_if(
        observerFederate.callbackEvents.begin(), observerFederate.callbackEvents.end(),
        [&](RecordingFederateAmbassador::CallbackEvent const& event) {
          return event.kind == RecordingFederateAmbassador::CallbackKind::discovery &&
                 event.objectInstance == rejoinedSubjectReflection->objectInstance;
        });
    auto const rejoinedReflection = std::find_if(
        observerFederate.callbackEvents.begin(), observerFederate.callbackEvents.end(),
        [&](RecordingFederateAmbassador::CallbackEvent const& event) {
          return event.kind == RecordingFederateAmbassador::CallbackKind::reflection &&
                 event.objectInstance == rejoinedSubjectReflection->objectInstance;
        });
    REQUIRE(removal != observerFederate.callbackEvents.end());
    REQUIRE(rejoinedDiscovery != observerFederate.callbackEvents.end());
    REQUIRE(rejoinedReflection != observerFederate.callbackEvents.end());
    REQUIRE(removal < rejoinedDiscovery);
    REQUIRE(rejoinedDiscovery < rejoinedReflection);

    subject->resignFederationExecution(NO_ACTION);
    subjectJoined = false;
    observer->resignFederationExecution(NO_ACTION);
    observerJoined = false;
    subject->disconnect();
    observer->disconnect();
  } catch (...) {
    clientError = std::current_exception();
    if (subjectJoined) {
      try {
        subject->resignFederationExecution(NO_ACTION);
      } catch (...) {
      }
    }
    if (observerJoined) {
      try {
        observer->resignFederationExecution(NO_ACTION);
      } catch (...) {
      }
    }
    try {
      subject->disconnect();
    } catch (...) {
    }
    try {
      observer->disconnect();
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
  REQUIRE_FALSE(subjectJoined);
  REQUIRE_FALSE(observerJoined);
  std::error_code ignored;
  std::filesystem::remove_all(reportDirectory, ignored);
}
#endif
