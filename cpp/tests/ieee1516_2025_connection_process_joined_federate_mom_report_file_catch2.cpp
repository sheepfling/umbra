#include <catch2/catch_test_macros.hpp>

#include <algorithm>
#include <atomic>
#include <chrono>
#include <condition_variable>
#include <exception>
#include <filesystem>
#include <fstream>
#include <iterator>
#include <memory>
#include <mutex>
#include <optional>
#include <stdexcept>
#include <string>
#include <thread>
#include <vector>

#include <RTI/NullFederateAmbassador.h>
#include <RTI/RTI1516.h>
#include <RTI/auth/HLAnoCredentials.h>
#include <RTI/encoding/BasicDataElements.h>

#include "internal/federation/federation_registry.hpp"
#include "internal/federation/process_federation_service.hpp"
#include "internal/federation/process_transport.hpp"
#include "internal/federation/process_transport_session.hpp"
#include "ieee1516_2025_connection_test_support.hpp"
#include "ieee1516_2025_process_fom_test_support.hpp"
#include "process_public_service_fixture.hpp"

#if defined(UMBRA_ENABLE_EMBEDDED_FEDERATION_MANAGEMENT)
namespace {

using rti1516_2025::HLA_EVOKED;
using rti1516_2025::HLA_IMMEDIATE;
using rti1516_2025::NO_ACTION;
using rti1516_2025::NullFederateAmbassador;
using rti1516_2025::RtiConfiguration;
using TestFederateAmbassador = NullFederateAmbassador;
using umbra::detail::EmbeddedFederationRegistry;
using umbra::detail::ProcessFederationService;
using umbra::detail::ProcessFederationServiceOptions;
using umbra::detail::ProcessTransportListener;
using umbra::detail::ProcessTransportSession;
using umbra::detail::TransportServiceMessage;
using umbra::detail::TransportServiceOperation;
using umbra::test::connection_support_2025::makeRti;
using umbra::test::connection_support_2025::temporaryServiceReportDirectory;
using umbra::test::process_fom_support_2025::composedProcessDefinition;

TEST_CASE(
    "RTIambassador projects the joined-federate MOM report path through a configured process endpoint",
    "[integration][foundation][federation-management][mom][service-report-file][service-reporting]"
    "[joined-federate-mom][transport][process-boundary][public-endpoint][2025]"
    "[process-service-report-connection-initial-record][process-service-report-file-switch-cycle]"
    "[process-service-report-file-join-lifetime]"
    "[rti.service.connect][rti.service.create-federation-execution]"
    "[rti.service.join-federation-execution]"
    "[rti.service.get-object-class-handle][rti.service.get-attribute-handle]"
    "[rti.service.subscribe-object-class-attributes][rti.service.evoke-callback]"
    "[rti.service.get-interaction-class-handle][rti.service.get-parameter-handle]"
    "[rti.service.send-interaction][rti.service.get-send-service-reports-to-file-switch]"
    "[rti.service.request-attribute-value-update][rti.service.resign-federation-execution]"
    "[federate.callback.discover-object-instance][federate.callback.reflect-attribute-values]") {
  class RecordingFederateAmbassador final : public NullFederateAmbassador {
   public:
    struct Discovery final {
      rti1516_2025::ObjectInstanceHandle objectInstance;
      rti1516_2025::ObjectClassHandle objectClass;
      std::wstring objectInstanceName;
      rti1516_2025::FederateHandle producingFederate;
    };
    struct Reflection final {
      rti1516_2025::ObjectInstanceHandle objectInstance;
      rti1516_2025::AttributeHandleValueMap attributeValues;
      rti1516_2025::FederateHandle producingFederate;
    };
    void discoverObjectInstance(
        rti1516_2025::ObjectInstanceHandle const& objectInstance,
        rti1516_2025::ObjectClassHandle const& objectClass,
        std::wstring const& objectInstanceName,
        rti1516_2025::FederateHandle const& producingFederate) override {
      discoveries.push_back({objectInstance, objectClass, objectInstanceName, producingFederate});
    }
    void reflectAttributeValues(
        rti1516_2025::ObjectInstanceHandle const& objectInstance,
        rti1516_2025::AttributeHandleValueMap const& attributeValues,
        rti1516_2025::VariableLengthData const&,
        rti1516_2025::TransportationTypeHandle const&,
        rti1516_2025::FederateHandle const& producingFederate,
        rti1516_2025::RegionHandleSet const*) override {
      reflections.push_back({objectInstance, attributeValues, producingFederate});
      reflectionCount.fetch_add(1U, std::memory_order_release);
      reflectionChanged.notify_all();
      if (discoveries.size() >= 2U && reflections.size() >= 2U) {
        initialMomValuesReady.store(true, std::memory_order_release);
      }
    }
    bool waitForReflectionCount(std::size_t expectedCount) {
      std::unique_lock lock(reflectionMutex);
      return reflectionChanged.wait_for(
          lock,
          std::chrono::milliseconds(100),
          [&] {
            return reflectionCount.load(std::memory_order_acquire) >= expectedCount;
          });
    }
    std::vector<Discovery> discoveries;
    std::vector<Reflection> reflections;
    std::mutex reflectionMutex;
    std::condition_variable reflectionChanged;
    std::atomic_size_t reflectionCount{0U};
    std::atomic_bool initialMomValuesReady{false};
  } observerFederate;

  using umbra::detail::EmbeddedFederationRegistry;
  using umbra::detail::ProcessFederationService;
  using umbra::detail::ProcessFederationServiceOptions;
  using umbra::detail::ProcessTransportListener;
  using umbra::detail::ProcessTransportServiceDispatcher;
  using umbra::detail::ProcessTransportSession;

  auto listener = ProcessTransportListener::listen({"127.0.0.1", 0U});
  REQUIRE(listener);
  auto const port = listener->address().port;
  REQUIRE(port != 0U);
  auto const reportDirectory = temporaryServiceReportDirectory();
  constexpr wchar_t const* federationName =
      L"public-process-joined-federate-mom-report-execution";
  constexpr wchar_t const* subjectName =
      L"public-process-joined-federate-mom-report-subject";
  constexpr wchar_t const* observerName =
      L"public-process-joined-federate-mom-report-observer";

  std::exception_ptr serverError;
  auto narrowExceptionText = [](std::wstring const& value) {
    std::string result;
    result.reserve(value.size());
    for (wchar_t const character : value) {
      result.push_back(character >= 0 && character <= 0x7F
                           ? static_cast<char>(character)
                           : '?');
    }
    return result;
  };
  std::thread server([&] {
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
          {"public-process-joined-federate-mom-report-server", 0x9D01U},
          [](std::wstring) {},
          [](std::wstring) { return false; });
      ProcessTransportSession subjectSession(subjectConnection);
      auto subjectHandler = service.handlerFor(subjectSession);
      serveExpected(subjectSession, subjectHandler,
                    TransportServiceOperation::create_federation_execution,
                    "The process MOM server lost Create.");
      serveExpected(subjectSession, subjectHandler,
                    TransportServiceOperation::join_federation_execution,
                    "The process MOM server lost subject Join.");
      serveExpected(subjectSession, subjectHandler,
                    TransportServiceOperation::get_send_service_reports_to_file_switch,
                    "The process MOM server lost the initial subject file-switch query.");
      auto observerConnection = listener->accept(
          nullptr,
          {"public-process-joined-federate-mom-report-server", 0x9D02U},
          [](std::wstring) {},
          [](std::wstring) { return false; });
      ProcessTransportSession observerSession(observerConnection);
      auto observerHandler = service.handlerFor(observerSession);
      serveExpected(observerSession, observerHandler,
                    TransportServiceOperation::join_federation_execution,
                    "The process MOM server lost observer Join.");
      serveExpected(observerSession, observerHandler,
                    TransportServiceOperation::get_object_class_handle,
                    "The process MOM server lost the MOM class lookup.");
      serveExpected(observerSession, observerHandler,
                    TransportServiceOperation::get_attribute_handle,
                    "The process MOM server lost the MOM report-file lookup.");
      serveExpected(observerSession, observerHandler,
                    TransportServiceOperation::get_attribute_handle,
                    "The process MOM server lost the MOM name lookup.");
      serveExpected(observerSession, observerHandler,
                    TransportServiceOperation::subscribe_object_class_attributes,
                    "The process MOM server lost the MOM subscription.");
      while (!observerFederate.initialMomValuesReady.load(std::memory_order_acquire)) {
        serveExpected(observerSession, observerHandler,
                      TransportServiceOperation::receive_interaction,
                      "The process MOM server lost an initial callback poll.");
      }
      serveExpected(subjectSession, subjectHandler,
                    TransportServiceOperation::get_interaction_class_handle,
                    "The process MOM server lost the HLAsetSwitches lookup.");
      serveExpected(subjectSession, subjectHandler,
                    TransportServiceOperation::get_parameter_handle,
                    "The process MOM server lost the file-switch parameter lookup.");
      auto serveObserverReflectionsUntil = [&](std::size_t expectedCount) {
        std::size_t pollCount = 0U;
        while (observerFederate.reflectionCount.load(std::memory_order_acquire) <
                   expectedCount &&
               pollCount != 32U) {
          serveExpected(observerSession, observerHandler,
                        TransportServiceOperation::receive_interaction,
                        "The process MOM server lost a requested report-path reflection poll.");
          ++pollCount;
          if (observerFederate.reflectionCount.load(std::memory_order_acquire) <
              expectedCount) {
            static_cast<void>(observerFederate.waitForReflectionCount(expectedCount));
          }
        }
        if (observerFederate.reflectionCount.load(std::memory_order_acquire) <
            expectedCount) {
          throw std::runtime_error(
              "The process MOM server did not observe the expected report-path reflection count.");
        }
        serveExpected(observerSession, observerHandler,
                      TransportServiceOperation::receive_interaction,
                      "The process MOM server lost the final empty callback poll.");
      };
      for (std::size_t expectedReflectionCount = 3U;
           expectedReflectionCount <= 5U;
           ++expectedReflectionCount) {
        serveExpected(subjectSession, subjectHandler,
                      TransportServiceOperation::send_interaction,
                      "The process MOM server lost an HLAsetSwitches file-reporting adjustment.");
        serveExpected(subjectSession, subjectHandler,
                      TransportServiceOperation::report_successful_void_service_invocation,
                      "The process MOM server lost the successful HLAsetSwitches invocation report.");
        serveExpected(subjectSession, subjectHandler,
                      TransportServiceOperation::get_send_service_reports_to_file_switch,
                      "The process MOM server lost a subject file-switch readback.");
        serveExpected(observerSession, observerHandler,
                      TransportServiceOperation::request_attribute_value_update,
                      "The process MOM server lost a report-path value request.");
        serveObserverReflectionsUntil(expectedReflectionCount);
      }
      serveExpected(subjectSession, subjectHandler,
                    TransportServiceOperation::resign_federation_execution,
                    "The process MOM server lost the first subject Resign.");
      serveExpected(subjectSession, subjectHandler,
                    TransportServiceOperation::join_federation_execution,
                    "The process MOM server lost the subject rejoin.");
      serveExpected(subjectSession, subjectHandler,
                    TransportServiceOperation::get_send_service_reports_to_file_switch,
                    "The process MOM server lost the rejoined subject file-switch query.");
      std::exception_ptr observerLifecycleServerError;
      std::thread observerLifecycleServer([&] {
        try {
          while (umbra::test::servePrimaryProcessRequest(
              observerSession, observerHandler)) {
          }
        } catch (...) {
          observerLifecycleServerError = std::current_exception();
        }
      });
      try {
        serveExpected(subjectSession, subjectHandler,
                      TransportServiceOperation::resign_federation_execution,
                      "The process MOM server lost the final subject Resign.");
      } catch (...) {
        observerConnection->close();
        if (observerLifecycleServer.joinable()) {
          observerLifecycleServer.join();
        }
        throw;
      }
      if (observerLifecycleServer.joinable()) {
        observerLifecycleServer.join();
      }
      if (observerLifecycleServerError) {
        std::rethrow_exception(observerLifecycleServerError);
      }
      service.detach(subjectSession);
      service.detach(observerSession);
      subjectConnection->close();
      observerConnection->close();
    } catch (...) {
      serverError = std::current_exception();
    }
  });

  TestFederateAmbassador subjectFederate;
  auto subject = makeRti();
  auto observer = makeRti();
  auto configuration = RtiConfiguration::createConfiguration()
                           .withConfigurationName(
                               L"public-process-joined-federate-mom-report-client")
                           .withRtiAddress(
                               L"tcp://127.0.0.1:" + std::to_wstring(port))
                           .withAdditionalSettings(L"ignored-by-initial-slice");
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
                subjectName,
                L"public-process-joined-federate-mom-report-type",
                federationName)
                .isValid());
    subjectJoined = true;
    REQUIRE_FALSE(subject->getSendServiceReportsToFileSwitch());
    REQUIRE(observer->connect(observerFederate, HLA_EVOKED, configuration).addressUsed);
    REQUIRE(observer->joinFederationExecution(
                observerName,
                L"public-process-joined-federate-mom-report-type",
                federationName)
                .isValid());
    observerJoined = true;

    auto const momClass = observer->getObjectClassHandle(
        L"HLAobjectRoot.HLAmanager.HLAfederate");
    auto const reportFileAttribute =
        observer->getAttributeHandle(momClass, L"HLAreportServiceFile");
    auto const federateNameAttribute =
        observer->getAttributeHandle(momClass, L"HLAfederateName");
    REQUIRE(momClass.isValid());
    REQUIRE(reportFileAttribute.isValid());
    REQUIRE(federateNameAttribute.isValid());
    REQUIRE_NOTHROW(observer->subscribeObjectClassAttributes(
        momClass,
        rti1516_2025::AttributeHandleSet{reportFileAttribute, federateNameAttribute}));
    for (std::size_t pass = 0U; pass != 16U &&
         !observerFederate.initialMomValuesReady.load(std::memory_order_acquire);
         ++pass) {
      static_cast<void>(observer->evokeCallback(0.0));
    }
    // The server's receive-order loop is intentionally request/response
    // driven.  Send one final empty poll after the callback has become ready
    // so the server cannot be left blocked in serveOne between the last
    // reflection and the teardown requests.
    static_cast<void>(observer->evokeCallback(0.0));
    REQUIRE(observerFederate.initialMomValuesReady.load(std::memory_order_acquire));
    REQUIRE(observerFederate.discoveries.size() == 2U);
    REQUIRE(observerFederate.reflections.size() == 2U);

    std::optional<RecordingFederateAmbassador::Reflection> subjectReflection;
    for (auto const& reflection : observerFederate.reflections) {
      auto const value = reflection.attributeValues.find(federateNameAttribute);
      if (value == reflection.attributeValues.end()) {
        continue;
      }
      rti1516_2025::HLAunicodeString reflectedName;
      try {
        reflectedName.decode(value->second);
      } catch (...) {
        continue;
      }
      if (reflectedName.get() == subjectName) {
        subjectReflection = reflection;
        break;
      }
    }
    REQUIRE(subjectReflection.has_value());
    REQUIRE(subjectReflection->attributeValues.size() == 2U);
    REQUIRE_FALSE(subjectReflection->producingFederate.isValid());
    auto const discovery = std::find_if(
        observerFederate.discoveries.begin(),
        observerFederate.discoveries.end(),
        [&](RecordingFederateAmbassador::Discovery const& value) {
          return value.objectInstance == subjectReflection->objectInstance;
        });
    REQUIRE(discovery != observerFederate.discoveries.end());
    REQUIRE(discovery->objectClass == momClass);
    REQUIRE_FALSE(discovery->producingFederate.isValid());

    rti1516_2025::HLAunicodeString advertisedPath;
    REQUIRE_NOTHROW(advertisedPath.decode(
        subjectReflection->attributeValues.at(reportFileAttribute)));
    auto const reportPath = std::filesystem::path(advertisedPath.get());
    REQUIRE(reportPath.is_absolute());
    REQUIRE(reportPath.lexically_normal() == reportPath);
    REQUIRE(reportPath.parent_path() ==
            std::filesystem::absolute(reportDirectory).lexically_normal());
    REQUIRE(std::filesystem::exists(reportPath));
    std::ifstream reportStream(reportPath, std::ios::binary);
    REQUIRE(reportStream.good());
    std::string reportText{
        std::istreambuf_iterator<char>(reportStream),
        std::istreambuf_iterator<char>{}};
    reportStream.close();
    REQUIRE(reportText.find("\"CallbackModel\":\"HLA_IMMEDIATE\"") !=
            std::string::npos);
    REQUIRE(reportText.find(
                "\"ConfigurationName\":\"public-process-joined-federate-mom-report-client\"") !=
            std::string::npos);
    REQUIRE(reportText.find(
                "\"RTIaddress\":\"tcp://127.0.0.1:" + std::to_string(port) + "\"") !=
            std::string::npos);
    REQUIRE(reportText.find(
                "\"AdditionalSettings\":\"ignored-by-initial-slice\"") !=
            std::string::npos);
    REQUIRE(reportText.find(
                "\"Credentials\":{\"Type\":\"HLAnoCredentials\",\"Data\":\"\"}") !=
            std::string::npos);

    auto const setSwitches = subject->getInteractionClassHandle(
        L"HLAinteractionRoot.HLAmanager.HLAfederate.HLAadjust.HLAsetSwitches");
    auto const sendServiceReportsToFile = subject->getParameterHandle(
        setSwitches, L"HLAsendServiceReportsToFile");
    REQUIRE(setSwitches.isValid());
    REQUIRE(sendServiceReportsToFile.isValid());
    auto encodeSwitch = [](bool enabled) {
      return rti1516_2025::HLAinteger32BE(enabled ? 1 : 0).encode();
    };
    auto requestAndCheckStableReportPath = [&](
        rti1516_2025::ObjectInstanceHandle expectedObjectInstance,
        std::wstring const& expectedPath) {
      auto const reflectionsBefore = observerFederate.reflectionCount.load(
          std::memory_order_acquire);
      try {
        observer->requestAttributeValueUpdate(
            expectedObjectInstance,
            rti1516_2025::AttributeHandleSet{reportFileAttribute},
            rti1516_2025::VariableLengthData{});
      } catch (rti1516_2025::Exception const& error) {
        throw std::runtime_error(
            narrowExceptionText(error.name()) + ": " +
            narrowExceptionText(error.what()));
      }
      for (std::size_t pass = 0U;
           pass != 64U &&
           observerFederate.reflectionCount.load(std::memory_order_acquire) ==
               reflectionsBefore;
           ++pass) {
        static_cast<void>(observer->evokeCallback(0.0));
      }
      // Match one final empty process poll if the server has not yet observed
      // the callback-count transition from the last requested-value response.
      static_cast<void>(observer->evokeCallback(0.0));
      REQUIRE(observerFederate.reflectionCount.load(std::memory_order_acquire) ==
              reflectionsBefore + 1U);
      auto const& requestedReflection = observerFederate.reflections.back();
      REQUIRE(requestedReflection.objectInstance == expectedObjectInstance);
      REQUIRE(requestedReflection.attributeValues.size() == 1U);
      rti1516_2025::HLAunicodeString requestedPath;
      REQUIRE_NOTHROW(requestedPath.decode(
          requestedReflection.attributeValues.at(reportFileAttribute)));
      REQUIRE(requestedPath.get() == expectedPath);
    };

    rti1516_2025::ParameterHandleValueMap const enabledFileSwitch{
        {sendServiceReportsToFile, encodeSwitch(true)}};
    REQUIRE_NOTHROW(subject->sendInteraction(
        setSwitches, enabledFileSwitch, rti1516_2025::VariableLengthData{}));
    REQUIRE(subject->getSendServiceReportsToFileSwitch());
    requestAndCheckStableReportPath(
        subjectReflection->objectInstance, advertisedPath.get());

    rti1516_2025::ParameterHandleValueMap const disabledFileSwitch{
        {sendServiceReportsToFile, encodeSwitch(false)}};
    REQUIRE_NOTHROW(subject->sendInteraction(
        setSwitches, disabledFileSwitch, rti1516_2025::VariableLengthData{}));
    REQUIRE_FALSE(subject->getSendServiceReportsToFileSwitch());
    requestAndCheckStableReportPath(
        subjectReflection->objectInstance, advertisedPath.get());

    REQUIRE_NOTHROW(subject->sendInteraction(
        setSwitches, enabledFileSwitch, rti1516_2025::VariableLengthData{}));
    REQUIRE(subject->getSendServiceReportsToFileSwitch());
    requestAndCheckStableReportPath(
        subjectReflection->objectInstance, advertisedPath.get());

    auto readFile = [](std::filesystem::path const& path) {
      std::ifstream stream(path, std::ios::binary);
      return std::string{
          std::istreambuf_iterator<char>(stream),
          std::istreambuf_iterator<char>{}};
    };
    subject->resignFederationExecution(NO_ACTION);
    subjectJoined = false;
    auto const originalFileContents = readFile(reportPath);
    REQUIRE_FALSE(originalFileContents.empty());
    auto const rejoinedSubject = subject->joinFederationExecution(
        subjectName,
        L"public-process-joined-federate-mom-report-type",
        federationName);
    if (!rejoinedSubject.isValid()) {
      throw std::runtime_error(
          "The process rejoin returned an invalid public federate handle.");
    }
    subjectJoined = true;
    static_cast<void>(subject->getSendServiceReportsToFileSwitch());

    for (std::size_t pass = 0U;
         pass != 64U &&
         observerFederate.reflectionCount.load(std::memory_order_acquire) < 6U;
         ++pass) {
      static_cast<void>(observer->evokeCallback(0.0));
    }
    static_cast<void>(observer->evokeCallback(0.0));
    REQUIRE(observerFederate.reflectionCount.load(std::memory_order_acquire) == 6U);
    REQUIRE(observerFederate.discoveries.size() == 3U);
    REQUIRE(observerFederate.reflections.size() == 6U);
    auto const& rejoinedReflection = observerFederate.reflections.back();
    REQUIRE(rejoinedReflection.objectInstance != subjectReflection->objectInstance);
    REQUIRE(rejoinedReflection.attributeValues.size() == 2U);
    rti1516_2025::HLAunicodeString rejoinedFederateName;
    REQUIRE_NOTHROW(rejoinedFederateName.decode(
        rejoinedReflection.attributeValues.at(federateNameAttribute)));
    REQUIRE(rejoinedFederateName.get() == subjectName);
    rti1516_2025::HLAunicodeString rejoinedAdvertisedPath;
    REQUIRE_NOTHROW(rejoinedAdvertisedPath.decode(
        rejoinedReflection.attributeValues.at(reportFileAttribute)));
    auto const rejoinedReportPath =
        std::filesystem::path(rejoinedAdvertisedPath.get());
    REQUIRE(rejoinedReportPath.is_absolute());
    REQUIRE(rejoinedReportPath.lexically_normal() == rejoinedReportPath);
    REQUIRE(rejoinedReportPath.parent_path() == reportPath.parent_path());
    REQUIRE(rejoinedReportPath != reportPath);
    REQUIRE(std::filesystem::exists(rejoinedReportPath));
    auto const rejoinedFileContents = readFile(rejoinedReportPath);
    REQUIRE_FALSE(rejoinedFileContents.empty());
    REQUIRE(rejoinedFileContents.find(
                "\"ConfigurationName\":\"public-process-joined-federate-mom-report-client\"") !=
            std::string::npos);
    REQUIRE(readFile(reportPath) == originalFileContents);
    requestAndCheckStableReportPath(
        rejoinedReflection.objectInstance, rejoinedAdvertisedPath.get());

    subject->resignFederationExecution(NO_ACTION);
    subjectJoined = false;
    observer->resignFederationExecution(NO_ACTION);
    observerJoined = false;
    subject->disconnect();
    observer->disconnect();
  } catch (...) {
    clientError = std::current_exception();
    try {
      std::rethrow_exception(clientError);
    } catch (rti1516_2025::Exception const& error) {
      clientError = std::make_exception_ptr(std::runtime_error(
          narrowExceptionText(error.name()) + ": " +
          narrowExceptionText(error.what())));
    } catch (...) {
    }
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

}  // namespace
#endif
