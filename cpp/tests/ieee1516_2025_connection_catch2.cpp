#include <catch2/catch_test_macros.hpp>

#include <algorithm>
#include <atomic>
#include <array>
#include <chrono>
#include <condition_variable>
#include <cstring>
#include <exception>
#include <filesystem>
#include <fstream>
#include <functional>
#include <future>
#include <iterator>
#include <limits>
#include <memory>
#include <mutex>
#include <optional>
#include <stdexcept>
#include <set>
#include <string>
#include <thread>
#include <vector>
#include <RTI/RTI1516.h>
#include <RTI/NullFederateAmbassador.h>
#include <RTI/auth/HLAnoCredentials.h>
#include <RTI/time/HLAfloat64Time.h>
#include <RTI/time/HLAinteger64Time.h>
#include <RTI/time/HLAinteger64TimeFactory.h>
#include <RTI/time/HLAinteger64Interval.h>
#include <RTI/time/HLAlogicalTimeFactoryFactory.h>
#include <RTI/encoding/BasicDataElements.h>

#include <umbra/embedded_profile_configuration.hpp>

#if defined(UMBRA_ENABLE_EMBEDDED_FEDERATION_MANAGEMENT)
#include "ieee1516_2025_connection_process_test_support.hpp"


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
  if (serverError) {
    std::rethrow_exception(serverError);
  }
  if (clientError) {
    std::rethrow_exception(clientError);
  }
  REQUIRE(result.has_value());
  REQUIRE(result->configurationUsed);
  REQUIRE(result->addressUsed);
  REQUIRE(result->additionalSettingsResult == SETTINGS_IGNORED);
}

TEST_CASE(
    "RTIambassador routes public Create, Join, Resign, and Destroy through a configured process endpoint",
    "[integration][foundation][federation-management][transport][time-management][2025][process-boundary][process-boundary-federation-lifecycle][public-endpoint][rti.service.enable-time-regulation][rti.service.disable-time-regulation][rti.service.query-logical-time][rti.service.destroy-federation-execution][federate.callback.time-regulation-enabled]") {
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
      if (!umbra::test::servePrimaryProcessRequest(session, handler) ||
          !umbra::test::servePrimaryProcessRequest(session, handler) ||
          !umbra::test::servePrimaryProcessRequest(session, handler) ||
          !umbra::test::servePrimaryProcessRequest(session, handler) ||
          !umbra::test::servePrimaryProcessRequest(session, handler) ||
          !umbra::test::servePrimaryProcessRequest(session, handler) ||
          !umbra::test::servePrimaryProcessRequest(session, handler) ||
          !umbra::test::servePrimaryProcessRequest(session, handler) ||
          !umbra::test::servePrimaryProcessRequest(session, handler) ||
          !umbra::test::servePrimaryProcessRequest(session, handler)) {
        throw std::runtime_error(
            "The public process endpoint server did not receive Create, Join, Destroy-while-joined, Enable/Disable Time Regulation, Query Logical Time, Resign, Destroy, and missing-Destroy.");
      }
      service.detach(session);
      connection->close();
    } catch (...) {
      serverError = std::current_exception();
    }
  });

  ProcessTimeFederateAmbassador federate;
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
    REQUIRE_THROWS_AS(
        rti->destroyFederationExecution(L"process-public-execution"),
        rti1516_2025::FederatesCurrentlyJoined);
    REQUIRE_NOTHROW(
        rti->enableTimeRegulation(rti1516_2025::HLAinteger64Interval(1)));
    REQUIRE(federate.timeRegulationEnabledCount == 0U);
    REQUIRE_FALSE(rti->evokeCallback(0.0));
    REQUIRE(federate.timeRegulationEnabledCount == 1U);
    REQUIRE(
        federate.timeRegulationEnabledImplementation == L"HLAinteger64Time");
    rti1516_2025::HLAinteger64Time queriedTime;
    REQUIRE_NOTHROW(rti->queryLogicalTime(queriedTime));
    REQUIRE(queriedTime.getTime() == 0);
    REQUIRE_NOTHROW(rti->disableTimeRegulation());
    REQUIRE_THROWS_AS(
        rti->disableTimeRegulation(),
        rti1516_2025::TimeRegulationIsNotEnabled);
    rti->resignFederationExecution(NO_ACTION);
    REQUIRE_NOTHROW(
        rti->destroyFederationExecution(L"process-public-execution"));
    REQUIRE_THROWS_AS(
        rti->destroyFederationExecution(L"process-public-execution"),
        rti1516_2025::FederationExecutionDoesNotExist);
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

  ConfigurationResult configuredResult(
      true,
      false,
      SETTINGS_APPLIED,
      L"configuration and additional settings were applied");
  REQUIRE(configuredResult.configurationUsed);
  REQUIRE_FALSE(configuredResult.addressUsed);
  REQUIRE(configuredResult.additionalSettingsResult == SETTINGS_APPLIED);
  REQUIRE(configuredResult.message == L"configuration and additional settings were applied");

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
    "[integration][connection][authorization][credentials][federation-management]"
    "[connect-credentials-authorization-disabled]") {
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
    "[internal][integration][connection][mom][service-report-store][service-reporting][configuration][typed-service-report-directory-configuration]") {
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

#endif
#if defined(UMBRA_ENABLE_EMBEDDED_FEDERATION_MANAGEMENT)
#endif

#if defined(UMBRA_ENABLE_EMBEDDED_FEDERATION_MANAGEMENT)
TEST_CASE(
    "RTIambassador rejects an incompatible logical time through a configured process endpoint",
    "[integration][foundation][time-management][time-advance][transport][process-boundary][public-endpoint][rti.service.time-advance-request][rti.error.invalid-logical-time]") {
  constexpr wchar_t const* federationName =
      L"process-time-advance-invalid-time-execution";
  auto listener = ProcessTransportListener::listen({"127.0.0.1", 0U});
  REQUIRE(listener);
  auto const port = listener->address().port;
  REQUIRE(port != 0U);

  std::exception_ptr serverError;
  std::thread server([&] {
    try {
      EmbeddedFederationRegistry registry;
      ProcessFederationService service(
          registry, composedProcessDefinition(), ProcessFederationServiceOptions{});
      auto connection = listener->accept(
          nullptr,
          {"process-time-advance-invalid-time-server", 0x9206U},
          [](std::wstring) {},
          [](std::wstring) { return false; });
      ProcessTransportSession session(connection);
      auto handler = service.handlerFor(session);
      auto serveExpected = [&](TransportServiceOperation operation) {
        return umbra::test::servePrimaryProcessRequest(
            session, handler,
            [&](TransportServiceMessage const& request) {
              if (request.operation != operation) {
                throw std::runtime_error(
                    "The process invalid-time server received an unexpected operation.");
              }
              return handler(request);
            });
      };
      if (!serveExpected(TransportServiceOperation::create_federation_execution) ||
          !serveExpected(TransportServiceOperation::join_federation_execution) ||
          !serveExpected(TransportServiceOperation::time_advance_request) ||
          !serveExpected(TransportServiceOperation::resign_federation_execution)) {
        throw std::runtime_error(
            "The process invalid-time server lost a temporal request.");
      }
      service.detach(session);
      connection->close();
    } catch (...) {
      serverError = std::current_exception();
    }
  });

  ProcessTimeFederateAmbassador federate;
  auto rti = makeRti();
  auto configuration = RtiConfiguration::createConfiguration()
                           .withConfigurationName(L"process-time-advance-invalid-time-client")
                           .withRtiAddress(
                               L"tcp://127.0.0.1:" + std::to_wstring(port));
  std::exception_ptr clientError;
  try {
    auto const connectionResult = rti->connect(federate, HLA_EVOKED, configuration);
    REQUIRE(connectionResult.addressUsed);
    REQUIRE_NOTHROW(rti->createFederationExecution(
        federationName, L"server-owned-fom.xml"));
    auto const joined = rti->joinFederationExecution(
        L"process-time-advance-invalid-time-type", federationName);
    REQUIRE(joined.isValid());
    REQUIRE_THROWS_AS(
        rti->timeAdvanceRequest(rti1516_2025::HLAfloat64Time(4.0)),
        rti1516_2025::InvalidLogicalTime);
    REQUIRE(federate.timeAdvanceGrantCount == 0U);
    REQUIRE_NOTHROW(rti->resignFederationExecution(NO_ACTION));
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
}




#endif
#if defined(UMBRA_ENABLE_EMBEDDED_FEDERATION_MANAGEMENT)
#if defined(UMBRA_ENABLE_EMBEDDED_FEDERATION_MANAGEMENT)
#endif
#if defined(UMBRA_ENABLE_EMBEDDED_FEDERATION_MANAGEMENT)






#endif
#if defined(UMBRA_ENABLE_EMBEDDED_FEDERATION_MANAGEMENT)

#endif
#if defined(UMBRA_ENABLE_EMBEDDED_FEDERATION_MANAGEMENT)
#endif
#if defined(UMBRA_ENABLE_EMBEDDED_FEDERATION_MANAGEMENT)
#endif



#if defined(UMBRA_ENABLE_EMBEDDED_FEDERATION_MANAGEMENT)
class TransportationTypeFederateAmbassador final : public NullFederateAmbassador {
 public:
  void confirmAttributeTransportationTypeChange(
      rti1516_2025::ObjectInstanceHandle const& objectInstance,
      rti1516_2025::AttributeHandleSet const& attributes,
      rti1516_2025::TransportationTypeHandle const& transportationType) override {
    ++changeCount;
    changedObjectInstance = objectInstance;
    changedAttributes = attributes;
    changedTransportationType = transportationType;
  }

  void reportAttributeTransportationType(
      rti1516_2025::ObjectInstanceHandle const& objectInstance,
      rti1516_2025::AttributeHandle const& attribute,
      rti1516_2025::TransportationTypeHandle const& transportationType) override {
    ++queryCount;
    queriedObjectInstance = objectInstance;
    queriedAttribute = attribute;
    queriedTransportationType = transportationType;
  }

  std::size_t changeCount = 0U;
  rti1516_2025::ObjectInstanceHandle changedObjectInstance;
  rti1516_2025::AttributeHandleSet changedAttributes;
  rti1516_2025::TransportationTypeHandle changedTransportationType;
  std::size_t queryCount = 0U;
  rti1516_2025::ObjectInstanceHandle queriedObjectInstance;
  rti1516_2025::AttributeHandle queriedAttribute;
  rti1516_2025::TransportationTypeHandle queriedTransportationType;
};

TEST_CASE(
    "RTIambassador routes instance transportation type change and query through a configured process endpoint",
    "[integration][foundation][object-management][transportation][transport][process-boundary][public-endpoint][process-transportation-instance-control][rti.service.request-attribute-transportation-type-change][rti.service.query-attribute-transportation-type]") {
  auto runScenario = [](CallbackModel callbackModel) {
    auto listener = ProcessTransportListener::listen({"127.0.0.1", 0U});
    REQUIRE(listener);
    auto const port = listener->address().port;
    REQUIRE(port != 0U);

    constexpr wchar_t const* federationName =
        L"public-process-transportation-instance-execution";
    constexpr wchar_t const* federateName =
        L"public-process-transportation-instance-federate";
    constexpr char const* objectName = "HLAobjectRoot.Customer";
    constexpr char const* attributeName = "HLAprivilegeToDeleteObject";
    constexpr char const* transportationTypeName = "HLAreliable";
    std::atomic_uint64_t expectedObjectClass{0U};
    std::atomic_uint64_t expectedAttribute{0U};
    std::atomic_uint64_t expectedTransportation{0U};
    std::exception_ptr serverError;
    std::thread server([&] {
      try {
        EmbeddedFederationRegistry registry;
        ProcessFederationService service(
            registry, composedProcessDefinition(), ProcessFederationServiceOptions{});
        auto connection = listener->accept(
            nullptr,
            {"public-process-transportation-instance-server", 0x9603U},
            [](std::wstring) {},
            [](std::wstring) { return false; });
        ProcessTransportSession session(connection);
        auto handler = service.handlerFor(session);
        auto serveExpected = [&](TransportServiceOperation operation) {
          return umbra::test::servePrimaryProcessRequest(
              session, handler,
              [&](TransportServiceMessage const& request) {
                if (request.operation != operation) {
                  throw std::runtime_error(
                      "The public process transportation-instance server received an unexpected operation (expected " +
                      std::to_string(static_cast<unsigned>(operation)) +
                      ", got " +
                      std::to_string(static_cast<unsigned>(request.operation)) + ").");
                }
                return handler(request);
              });
        };

        if (!serveExpected(TransportServiceOperation::create_federation_execution)) {
          throw std::runtime_error(
              "The public process transportation-instance server lost Create.");
        }
        auto const objectClass = registry.objectClassHandleFor(
            federationName, objectName);
        auto const attribute = registry.attributeHandleFor(
            federationName, objectName, attributeName);
        auto const transportation = registry.transportationTypeHandleFor(
            federationName, transportationTypeName);
        if (!objectClass || !attribute || !transportation) {
          throw std::runtime_error(
              "The public process transportation-instance server could not resolve its FOM handles.");
        }
        expectedObjectClass.store(*objectClass, std::memory_order_release);
        expectedAttribute.store(*attribute, std::memory_order_release);
        expectedTransportation.store(*transportation, std::memory_order_release);
        if (!serveExpected(TransportServiceOperation::join_federation_execution) ||
            !serveExpected(TransportServiceOperation::get_object_class_handle) ||
            !serveExpected(TransportServiceOperation::get_attribute_handle) ||
            !serveExpected(TransportServiceOperation::get_transportation_type_handle) ||
            !serveExpected(TransportServiceOperation::publish_object_class_attributes) ||
            !serveExpected(TransportServiceOperation::register_object_instance) ||
            !serveExpected(
                TransportServiceOperation::request_attribute_transportation_type_change) ||
            !serveExpected(TransportServiceOperation::receive_interaction) ||
            !serveExpected(TransportServiceOperation::query_attribute_transportation_type) ||
            !serveExpected(TransportServiceOperation::receive_interaction) ||
            !serveExpected(TransportServiceOperation::resign_federation_execution)) {
          throw std::runtime_error(
              "The public process transportation-instance server lost a declaration, callback, or Resign operation.");
        }
        service.detach(session);
        connection->close();
      } catch (...) {
        serverError = std::current_exception();
      }
    });

    TransportationTypeFederateAmbassador federate;
    auto rti = makeRti();
    auto configuration = RtiConfiguration::createConfiguration()
                             .withConfigurationName(
                                 L"public-process-transportation-instance-client")
                             .withRtiAddress(
                                 L"tcp://127.0.0.1:" + std::to_wstring(port));
    std::exception_ptr clientError;
    bool clientJoined = false;
    try {
      REQUIRE(rti->connect(federate, callbackModel, configuration).addressUsed);
      rti->createFederationExecution(federationName, L"server-owned-fom.xml");
      static_cast<void>(rti->joinFederationExecution(
          federateName,
          L"public-process-transportation-instance-type",
          federationName));
      clientJoined = true;
      auto const objectClass = rti->getObjectClassHandle(L"HLAobjectRoot.Customer");
      REQUIRE(objectClass.toString() ==
              L"ObjectClassHandle(" +
                  std::to_wstring(
                      expectedObjectClass.load(std::memory_order_acquire)) +
                  L")");
      auto const attribute =
          rti->getAttributeHandle(objectClass, L"HLAprivilegeToDeleteObject");
      REQUIRE(attribute.isValid());
      auto const transportation =
          rti->getTransportationTypeHandle(L"HLAreliable");
      REQUIRE(transportation.toString() ==
              L"TransportationTypeHandle(" +
                  std::to_wstring(
                      expectedTransportation.load(std::memory_order_acquire)) +
                  L")");
      rti1516_2025::AttributeHandleSet attributes;
      attributes.insert(attribute);
      rti->publishObjectClassAttributes(objectClass, attributes);
      auto const objectInstance = rti->registerObjectInstance(objectClass);
      REQUIRE(objectInstance.isValid());
      REQUIRE_NOTHROW(rti->requestAttributeTransportationTypeChange(
          objectInstance, attributes, transportation));
      REQUIRE_NOTHROW(rti->evokeCallback(0.0));
      REQUIRE(federate.changeCount == 1U);
      REQUIRE(federate.changedObjectInstance == objectInstance);
      REQUIRE(federate.changedAttributes.size() == 1U);
      REQUIRE(federate.changedAttributes.contains(attribute));
      REQUIRE(federate.changedTransportationType == transportation);
      REQUIRE_NOTHROW(rti->queryAttributeTransportationType(
          objectInstance, attribute));
      REQUIRE_NOTHROW(rti->evokeCallback(0.0));
      REQUIRE(federate.queryCount == 1U);
      REQUIRE(federate.queriedObjectInstance == objectInstance);
      REQUIRE(federate.queriedAttribute == attribute);
      REQUIRE(federate.queriedTransportationType == transportation);
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
    REQUIRE(expectedAttribute.load(std::memory_order_acquire) != 0U);
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
    "RTIambassador consumes process federation-wide HLAsetSwitches Auto Provide",
    "[integration][development-profile][foundation][federation-management][mom]"
    "[process-mom-federation-set-switches][process-boundary][public-endpoint][2025]"
    "[rti.service.connect][rti.service.create-federation-execution]"
    "[rti.service.join-federation-execution][rti.service.get-interaction-class-handle]"
    "[rti.service.get-parameter-handle][rti.service.send-interaction]"
    "[rti.service.resign-federation-execution][rti.service.disconnect]") {
  using umbra::detail::EmbeddedFederationRegistry;
  using umbra::detail::ProcessFederationService;
  using umbra::detail::ProcessFederationServiceOptions;
  using umbra::detail::ProcessTransportListener;
  using umbra::detail::ProcessTransportSession;

  auto listener = ProcessTransportListener::listen({"127.0.0.1", 0U});
  REQUIRE(listener);
  auto const port = listener->address().port;
  REQUIRE(port != 0U);

  constexpr wchar_t const* federationName =
      L"public-process-federation-mom-switches-execution";
  constexpr wchar_t const* federateName =
      L"public-process-federation-mom-switches-federate";
  std::exception_ptr serverError;
  std::thread server([&] {
    try {
      EmbeddedFederationRegistry registry;
      ProcessFederationService service(
          registry, composedProcessDefinition(), ProcessFederationServiceOptions{});
      auto connection = listener->accept(
          nullptr,
          {"public-process-federation-mom-switches-server", 0x9E02U},
          [](std::wstring) {},
          [](std::wstring) { return false; });
      ProcessTransportSession session(connection);
      auto handler = service.handlerFor(session);
      auto serveExpected = [&](TransportServiceOperation operation) {
        return umbra::test::servePrimaryProcessRequest(
            session, handler,
            [&](TransportServiceMessage const& request) {
              if (request.operation != operation) {
                throw std::runtime_error(
                    "The process federation MOM set-switches server received an unexpected operation.");
              }
              return handler(request);
            });
      };
      auto requireAutoProvide = [&](bool expected) {
        auto const member = registry.memberByName(federationName, federateName);
        auto const value = member
            ? registry.autoProvideSwitchFor(federationName, member->id)
            : std::optional<bool>{};
        if (!value || *value != expected) {
          throw std::runtime_error(
              "The process federation did not apply the expected federation-wide HLAsetSwitches value.");
        }
      };
      if (!serveExpected(TransportServiceOperation::create_federation_execution) ||
          !serveExpected(TransportServiceOperation::join_federation_execution)) {
        throw std::runtime_error(
            "The process federation MOM set-switches server did not receive create and join.");
      }
      requireAutoProvide(true);
      if (!serveExpected(TransportServiceOperation::get_interaction_class_handle) ||
          !serveExpected(TransportServiceOperation::get_parameter_handle) ||
          !serveExpected(TransportServiceOperation::send_interaction)) {
        throw std::runtime_error(
            "The process federation MOM set-switches server did not receive the disabling adjustment.");
      }
      requireAutoProvide(false);
      if (!serveExpected(TransportServiceOperation::send_interaction)) {
        throw std::runtime_error(
            "The process federation MOM set-switches server did not receive the enabling adjustment.");
      }
      requireAutoProvide(true);
      if (!serveExpected(TransportServiceOperation::resign_federation_execution)) {
        throw std::runtime_error(
            "The process federation MOM set-switches server did not receive resignation.");
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
                               L"public-process-federation-mom-switches-client")
                           .withRtiAddress(
                               L"tcp://127.0.0.1:" + std::to_wstring(port));
  std::exception_ptr clientError;
  std::optional<ConfigurationResult> connectionResult;
  bool joined = false;
  try {
    connectionResult = rti->connect(federate, HLA_EVOKED, configuration);
    REQUIRE(connectionResult->addressUsed);
    rti->createFederationExecution(federationName, L"server-owned-fom.xml");
    static_cast<void>(rti->joinFederationExecution(
        federateName, L"public-process-federation-mom-switches-type", federationName));
    joined = true;

    auto const setSwitches = rti->getInteractionClassHandle(
        L"HLAinteractionRoot.HLAmanager.HLAfederation.HLAadjust.HLAsetSwitches");
    auto const autoProvide = rti->getParameterHandle(setSwitches, L"HLAautoProvide");
    REQUIRE(setSwitches.isValid());
    REQUIRE(autoProvide.isValid());

    auto encodeSwitch = [](bool const enabled) {
      return rti1516_2025::HLAinteger32BE(enabled ? 1 : 0).encode();
    };
    ParameterHandleValueMap const disabledValues{
        {autoProvide, encodeSwitch(false)}};
    REQUIRE_NOTHROW(rti->sendInteraction(
        setSwitches, disabledValues, VariableLengthData()));

    ParameterHandleValueMap const enabledValues{
        {autoProvide, encodeSwitch(true)}};
    REQUIRE_NOTHROW(rti->sendInteraction(
        setSwitches, enabledValues, VariableLengthData()));

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
  REQUIRE(connectionResult.has_value());
}

TEST_CASE(
    "RTIambassador processes predefined process HLAsetSwitches parameters through a compatible subclass and ignores extensions",
    "[integration][development-profile][foundation][federation-management][mom][switches]"
    "[mom-extension][process-mom-extension-parameter][extension-subclass][transport]"
    "[process-boundary][public-endpoint][2025]"
    "[rti.service.connect][rti.service.create-federation-execution]"
    "[rti.service.join-federation-execution][rti.service.get-interaction-class-handle]"
    "[rti.service.get-parameter-handle][rti.service.get-service-reporting-switch]"
    "[rti.service.get-automatic-resign-directive][rti.service.send-interaction]"
    "[rti.service.resign-federation-execution][rti.service.disconnect]") {
  using umbra::detail::EmbeddedFederationRegistry;
  using umbra::detail::FomCompositionStatus;
  using umbra::detail::FomModuleKind;
  using umbra::detail::LibXml2FomModuleComposer;
  using umbra::detail::ProcessFederationService;
  using umbra::detail::ProcessFederationServiceOptions;
  using umbra::detail::ProcessTransportListener;
  using umbra::detail::ProcessTransportServiceDispatcher;
  using umbra::detail::ProcessTransportSession;
  using umbra::detail::TransportServiceMessage;
  using umbra::detail::TransportServiceOperation;

  auto composeExtensionDefinition = [] {
    std::vector<PrevalidatedFomModule> modules{
        validatedProcessModule(
            processResourcePath("mim/HLAstandardMIM-2025.xml"),
            FomModuleKind::mim,
            L"urn:umbra:test:process-mom-extension-mim"),
        validatedProcessModule(
            processResourcePath("examples/RestaurantFOMmodule-2025.xml"),
            FomModuleKind::fom,
            L"urn:umbra:test:process-mom-extension-restaurant"),
        validatedProcessModule(
            std::filesystem::path(UMBRA_SOURCE_DIRECTORY) / "cpp" / "tests" /
                "data" / "mom-set-switches-extension-fom.xml",
            FomModuleKind::fom,
            L"urn:umbra:test:process-mom-extension-fom"),
    };
    LibXml2FomModuleComposer composer(
        processResourcePath("schemas/IEEE1516-FDD-2025.xsd"));
    auto result = composer.compose(modules);
    if (result.status != FomCompositionStatus::valid || !result.catalog ||
        !result.fdd) {
      throw std::runtime_error(
          "The process MOM extension FOM did not compose.");
    }
    return FederationDefinition{
        std::move(result.modules),
        L"HLAinteger64Time",
        std::move(result.catalog),
        std::move(result.fdd),
    };
  };

  auto listener = ProcessTransportListener::listen({"127.0.0.1", 0U});
  REQUIRE(listener);
  auto const port = listener->address().port;
  REQUIRE(port != 0U);

  constexpr wchar_t const* federationName =
      L"public-process-mom-extension-parameter-execution";
  constexpr wchar_t const* federateName =
      L"public-process-mom-extension-parameter-federate";
  std::exception_ptr serverError;
  std::thread server([&] {
    try {
      EmbeddedFederationRegistry registry;
      ProcessFederationService service(
          registry, composeExtensionDefinition(), ProcessFederationServiceOptions{});
      auto connection = listener->accept(
          nullptr,
          {"public-process-mom-extension-parameter-server", 0x9E21U},
          [](std::wstring) {},
          [](std::wstring) { return false; });
      ProcessTransportSession session(connection);
      auto handler = service.handlerFor(session);
      auto serveExpected = [&](TransportServiceOperation operation) {
        return umbra::test::servePrimaryProcessRequest(
            session, handler,
            [&](TransportServiceMessage const& request) {
              if (request.operation != operation) {
                throw std::runtime_error(
                    "The process MOM extension server received an unexpected operation.");
              }
              return handler(request);
            });
      };
      if (!serveExpected(TransportServiceOperation::create_federation_execution) ||
          !serveExpected(TransportServiceOperation::join_federation_execution) ||
          !serveExpected(TransportServiceOperation::get_interaction_class_handle) ||
          !serveExpected(TransportServiceOperation::get_interaction_class_handle) ||
          !serveExpected(TransportServiceOperation::get_parameter_handle) ||
          !serveExpected(TransportServiceOperation::get_parameter_handle) ||
          !serveExpected(TransportServiceOperation::get_parameter_handle) ||
          !serveExpected(TransportServiceOperation::get_parameter_handle) ||
          !serveExpected(TransportServiceOperation::get_service_reporting_switch) ||
          !serveExpected(TransportServiceOperation::get_automatic_resign_directive) ||
          !serveExpected(TransportServiceOperation::send_interaction) ||
          !serveExpected(TransportServiceOperation::get_service_reporting_switch) ||
          !serveExpected(TransportServiceOperation::get_automatic_resign_directive) ||
          !serveExpected(TransportServiceOperation::send_interaction) ||
          !serveExpected(TransportServiceOperation::get_service_reporting_switch) ||
          !serveExpected(TransportServiceOperation::get_automatic_resign_directive) ||
          !serveExpected(TransportServiceOperation::send_interaction) ||
          !serveExpected(TransportServiceOperation::get_service_reporting_switch) ||
          !serveExpected(TransportServiceOperation::send_interaction) ||
          !serveExpected(TransportServiceOperation::get_service_reporting_switch) ||
          !serveExpected(TransportServiceOperation::send_interaction) ||
          !serveExpected(TransportServiceOperation::get_automatic_resign_directive) ||
          !serveExpected(TransportServiceOperation::send_interaction) ||
          !serveExpected(TransportServiceOperation::get_automatic_resign_directive) ||
          !serveExpected(TransportServiceOperation::resign_federation_execution)) {
        throw std::runtime_error(
            "The process MOM extension server did not receive the complete sequence.");
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
                               L"public-process-mom-extension-parameter-client")
                           .withRtiAddress(
                               L"tcp://127.0.0.1:" + std::to_wstring(port));
  std::exception_ptr clientError;
  std::optional<ConfigurationResult> connectionResult;
  bool joined = false;
  try {
    connectionResult = rti->connect(federate, HLA_EVOKED, configuration);
    REQUIRE(connectionResult->addressUsed);
    rti->createFederationExecution(federationName, L"server-owned-fom.xml");
    static_cast<void>(rti->joinFederationExecution(
        federateName, L"public-process-mom-extension-parameter-type", federationName));
    joined = true;

    auto const setSwitches = rti->getInteractionClassHandle(
        L"HLAinteractionRoot.HLAmanager.HLAfederate.HLAadjust.HLAsetSwitches");
    auto const extendedSetSwitches = rti->getInteractionClassHandle(
        L"HLAinteractionRoot.HLAmanager.HLAfederate.HLAadjust.HLAsetSwitches.UmbraExtendedSetSwitches");
    REQUIRE(setSwitches.isValid());
    REQUIRE(extendedSetSwitches.isValid());
    auto const extensionPayload = rti->getParameterHandle(
        setSwitches, L"UmbraExtensionSwitchPayload");
    auto const subclassPayload = rti->getParameterHandle(
        extendedSetSwitches, L"UmbraExtendedSwitchPayload");
    REQUIRE(extensionPayload.isValid());
    REQUIRE(subclassPayload.isValid());
    auto const serviceReporting = rti->getParameterHandle(
        extendedSetSwitches, L"HLAserviceReporting");
    REQUIRE(serviceReporting.isValid());
    auto const automaticResign = rti->getParameterHandle(
        extendedSetSwitches, L"HLAautomaticResignAction");
    REQUIRE(automaticResign.isValid());

    auto const initialServiceReporting = rti->getServiceReportingSwitch();
    auto const initialResignAction = rti->getAutomaticResignDirective();
    REQUIRE_THROWS_AS(
        rti->sendInteraction(
            setSwitches,
            ParameterHandleValueMap{
                {extensionPayload, VariableLengthData("extension", 9U)},
            },
            VariableLengthData()),
        RTIinternalError);
    REQUIRE(rti->getServiceReportingSwitch() == initialServiceReporting);
    REQUIRE(rti->getAutomaticResignDirective() == initialResignAction);

    REQUIRE_THROWS_AS(
        rti->sendInteraction(
            extendedSetSwitches,
            ParameterHandleValueMap{
                {subclassPayload, VariableLengthData("subclass", 8U)},
            },
            VariableLengthData()),
        RTIinternalError);
    REQUIRE(rti->getServiceReportingSwitch() == initialServiceReporting);
    REQUIRE(rti->getAutomaticResignDirective() == initialResignAction);

    auto encodeSwitch = [](bool const enabled) {
      return rti1516_2025::HLAinteger32BE(enabled ? 1 : 0).encode();
    };
    auto const promotedServiceReporting = !initialServiceReporting;
    auto encodeResignAction = [](rti1516_2025::ResignAction const action) {
      return rti1516_2025::HLAinteger32BE(
                 static_cast<std::int32_t>(action))
          .encode();
    };
    REQUIRE_NOTHROW(rti->sendInteraction(
        extendedSetSwitches,
        ParameterHandleValueMap{
            {serviceReporting, encodeSwitch(promotedServiceReporting)},
            {subclassPayload, VariableLengthData("subclass", 8U)},
        },
        VariableLengthData()));
    REQUIRE(rti->getServiceReportingSwitch() == promotedServiceReporting);
    REQUIRE_NOTHROW(rti->sendInteraction(
        extendedSetSwitches,
        ParameterHandleValueMap{
            {serviceReporting, encodeSwitch(initialServiceReporting)},
        },
        VariableLengthData()));
    REQUIRE(rti->getServiceReportingSwitch() == initialServiceReporting);

    auto const promotedResignAction = encodeResignAction(DELETE_OBJECTS);
    REQUIRE_NOTHROW(rti->sendInteraction(
        extendedSetSwitches,
        ParameterHandleValueMap{
            {automaticResign, promotedResignAction},
            {subclassPayload, VariableLengthData("subclass", 8U)},
        },
        VariableLengthData()));
    REQUIRE(rti->getAutomaticResignDirective() == DELETE_OBJECTS);
    REQUIRE_NOTHROW(rti->sendInteraction(
        extendedSetSwitches,
        ParameterHandleValueMap{
            {automaticResign, encodeResignAction(initialResignAction)},
        },
        VariableLengthData()));
    REQUIRE(rti->getAutomaticResignDirective() == initialResignAction);

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
  REQUIRE(connectionResult.has_value());
}
#endif

#if defined(UMBRA_ENABLE_EMBEDDED_FEDERATION_MANAGEMENT)
TEST_CASE(
    "RTIambassador preserves the process HLAsetSwitches Service Reporting interlock",
    "[integration][development-profile][foundation][federation-management][mom]"
    "[switches][service-reporting][mom-service-reporting-interlock]"
    "[transport][process-boundary][public-endpoint][2025]"
    "[rti.service.connect][rti.service.create-federation-execution]"
    "[rti.service.join-federation-execution][rti.service.get-interaction-class-handle]"
    "[rti.service.subscribe-interaction-class][rti.service.get-service-reporting-switch]"
    "[rti.service.get-parameter-handle][rti.service.send-interaction]"
    "[rti.service.unsubscribe-interaction-class]"
    "[rti.service.resign-federation-execution][rti.service.disconnect]") {
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

  constexpr wchar_t const* federationName =
      L"public-process-mom-service-report-interlock-execution";
  constexpr wchar_t const* federateName =
      L"public-process-mom-service-report-interlock-federate";
  std::exception_ptr serverError;
  std::thread server([&] {
    try {
      EmbeddedFederationRegistry registry;
      ProcessFederationService service(
          registry, composedProcessDefinition(), ProcessFederationServiceOptions{});
      auto connection = listener->accept(
          nullptr,
          {"public-process-mom-service-report-interlock-server", 0x9E02U},
          [](std::wstring) {},
          [](std::wstring) { return false; });
      ProcessTransportSession session(connection);
      auto handler = service.handlerFor(session);
      auto serveExpected = [&](TransportServiceOperation operation) {
        return umbra::test::servePrimaryProcessRequest(
            session, handler,
            [&](TransportServiceMessage const& request) {
              if (request.operation != operation) {
                throw std::runtime_error(
                    "The process MOM service-report interlock server expected operation " +
                    std::to_string(static_cast<unsigned>(operation)) +
                    " but received " +
                    std::to_string(static_cast<unsigned>(request.operation)) + ".");
              }
              return handler(request);
            });
      };
      if (!serveExpected(TransportServiceOperation::create_federation_execution) ||
          !serveExpected(TransportServiceOperation::join_federation_execution) ||
          !serveExpected(TransportServiceOperation::get_interaction_class_handle) ||
          !serveExpected(TransportServiceOperation::subscribe_interaction_class) ||
          !serveExpected(TransportServiceOperation::get_service_reporting_switch) ||
          !serveExpected(TransportServiceOperation::get_interaction_class_handle) ||
          !serveExpected(TransportServiceOperation::get_parameter_handle) ||
          !serveExpected(TransportServiceOperation::send_interaction) ||
          !serveExpected(TransportServiceOperation::get_service_reporting_switch) ||
          !serveExpected(TransportServiceOperation::unsubscribe_interaction_class) ||
          !serveExpected(TransportServiceOperation::send_interaction) ||
          !serveExpected(TransportServiceOperation::get_service_reporting_switch) ||
          !serveExpected(TransportServiceOperation::subscribe_interaction_class) ||
          !serveExpected(TransportServiceOperation::report_failed_service_invocation) ||
          !serveExpected(TransportServiceOperation::get_service_reporting_switch) ||
          !serveExpected(TransportServiceOperation::resign_federation_execution)) {
        throw std::runtime_error(
            "The process MOM service-report interlock server did not receive the complete sequence.");
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
                               L"public-process-mom-service-report-interlock-client")
                           .withRtiAddress(
                               L"tcp://127.0.0.1:" + std::to_wstring(port));
  std::exception_ptr clientError;
  std::optional<ConfigurationResult> connectionResult;
  bool joined = false;
  try {
    connectionResult = rti->connect(federate, HLA_EVOKED, configuration);
    REQUIRE(connectionResult->addressUsed);
    rti->createFederationExecution(federationName, L"server-owned-fom.xml");
    static_cast<void>(rti->joinFederationExecution(
        federateName,
        L"public-process-mom-service-report-interlock-type",
        federationName));
    joined = true;

    auto const reportInvocation = rti->getInteractionClassHandle(
        L"HLAinteractionRoot.HLAmanager.HLAfederate.HLAreport.HLAreportServiceInvocation");
    REQUIRE(reportInvocation.isValid());
    REQUIRE_NOTHROW(rti->subscribeInteractionClass(reportInvocation, true));
    REQUIRE_FALSE(rti->getServiceReportingSwitch());

    auto const setSwitches = rti->getInteractionClassHandle(
        L"HLAinteractionRoot.HLAmanager.HLAfederate.HLAadjust.HLAsetSwitches");
    auto const serviceReporting = rti->getParameterHandle(
        setSwitches, L"HLAserviceReporting");
    REQUIRE(setSwitches.isValid());
    REQUIRE(serviceReporting.isValid());

    auto const encodedSwitch =
        rti1516_2025::HLAinteger32BE(1).encode();
    ParameterHandleValueMap const enabledValues{
        {serviceReporting, encodedSwitch}};
    REQUIRE_THROWS_AS(
        rti->sendInteraction(
            setSwitches, enabledValues, VariableLengthData()),
        RTIinternalError);
    REQUIRE_FALSE(rti->getServiceReportingSwitch());

    REQUIRE_NOTHROW(rti->unsubscribeInteractionClass(reportInvocation));
    REQUIRE_NOTHROW(rti->sendInteraction(
        setSwitches, enabledValues, VariableLengthData()));
    REQUIRE(rti->getServiceReportingSwitch());

    REQUIRE_THROWS_AS(
        rti->subscribeInteractionClass(reportInvocation, true),
        rti1516_2025::FederateServiceInvocationsAreBeingReportedViaMOM);
    REQUIRE(rti->getServiceReportingSwitch());

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
  REQUIRE(connectionResult.has_value());
}

TEST_CASE(
    "RTIambassador reports a rejected process HLAsetSwitches interaction through HLAreportMOMexception",
    "[integration][development-profile][foundation][federation-management][mom]"
    "[switches][service-reporting][mom-exception][process-mom-exception-report]"
    "[transport][process-boundary][public-endpoint][2025]"
    "[rti.service.connect][rti.service.create-federation-execution]"
    "[rti.service.join-federation-execution][rti.service.get-interaction-class-handle]"
    "[rti.service.subscribe-interaction-class][rti.service.get-service-reporting-switch]"
    "[rti.service.get-parameter-handle][rti.service.send-interaction]"
    "[rti.service.unsubscribe-interaction-class]"
    "[rti.service.resign-federation-execution][rti.service.disconnect]"
    "[federate.callback.receive-interaction]") {
  class MomExceptionFederateAmbassador final : public NullFederateAmbassador {
   public:
    void receiveInteraction(
        rti1516_2025::InteractionClassHandle const& interactionClass,
        rti1516_2025::ParameterHandleValueMap const& parameterValues,
        rti1516_2025::VariableLengthData const& userSuppliedTag,
        rti1516_2025::TransportationTypeHandle const& transportationType,
        rti1516_2025::FederateHandle const& producingFederate,
        rti1516_2025::RegionHandleSet const* optionalSentRegions) override {
      ++receivedInteractionCount;
      receivedInteractionClass = interactionClass;
      receivedParameterValues = parameterValues;
      receivedTagSize = userSuppliedTag.size();
      receivedTransportationType = transportationType;
      receivedProducingFederate = producingFederate;
      receivedSentRegions = optionalSentRegions != nullptr;
    }

    std::size_t receivedInteractionCount = 0U;
    rti1516_2025::InteractionClassHandle receivedInteractionClass;
    rti1516_2025::ParameterHandleValueMap receivedParameterValues;
    std::size_t receivedTagSize = 0U;
    rti1516_2025::TransportationTypeHandle receivedTransportationType;
    rti1516_2025::FederateHandle receivedProducingFederate;
    bool receivedSentRegions = false;
  };

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

  constexpr wchar_t const* federationName =
      L"public-process-mom-exception-report-execution";
  constexpr wchar_t const* federateName =
      L"public-process-mom-exception-report-federate";
  std::exception_ptr serverError;
  std::thread server([&] {
    try {
      EmbeddedFederationRegistry registry;
      ProcessFederationService service(
          registry,
          composedProcessDefinition(),
          ProcessFederationServiceOptions{true});
      auto connection = listener->accept(
          nullptr,
          {"public-process-mom-exception-report-server", 0x9E03U},
          [](std::wstring) {},
          [](std::wstring) { return false; });
      ProcessTransportSession session(connection);
      auto handler = service.handlerFor(session);
      auto serveExpected = [&](TransportServiceOperation operation) {
        return umbra::test::servePrimaryProcessRequest(
            session, handler,
            [&](TransportServiceMessage const& request) {
              if (request.operation != operation) {
                throw std::runtime_error(
                    "The process MOM exception-report server received an unexpected operation.");
              }
              return handler(request);
            });
      };
      if (!serveExpected(TransportServiceOperation::create_federation_execution) ||
          !serveExpected(TransportServiceOperation::join_federation_execution) ||
          !serveExpected(TransportServiceOperation::get_interaction_class_handle) ||
          !serveExpected(TransportServiceOperation::get_parameter_handle) ||
          !serveExpected(TransportServiceOperation::get_parameter_handle) ||
          !serveExpected(TransportServiceOperation::get_parameter_handle) ||
          !serveExpected(TransportServiceOperation::subscribe_interaction_class) ||
          !serveExpected(TransportServiceOperation::get_service_reporting_switch) ||
          !serveExpected(TransportServiceOperation::get_interaction_class_handle) ||
          !serveExpected(TransportServiceOperation::subscribe_interaction_class) ||
          !serveExpected(TransportServiceOperation::get_interaction_class_handle) ||
          !serveExpected(TransportServiceOperation::get_parameter_handle) ||
          !serveExpected(TransportServiceOperation::send_interaction) ||
          !serveExpected(TransportServiceOperation::get_transportation_type_handle) ||
          !serveExpected(TransportServiceOperation::get_service_reporting_switch) ||
          !serveExpected(TransportServiceOperation::unsubscribe_interaction_class) ||
          !serveExpected(TransportServiceOperation::send_interaction) ||
          !serveExpected(TransportServiceOperation::get_service_reporting_switch) ||
          !serveExpected(TransportServiceOperation::resign_federation_execution)) {
        throw std::runtime_error(
            "The process MOM exception-report server did not receive the complete sequence.");
      }
      service.detach(session);
      connection->close();
    } catch (...) {
      serverError = std::current_exception();
    }
  });

  MomExceptionFederateAmbassador federate;
  auto rti = makeRti();
  auto configuration = RtiConfiguration::createConfiguration()
                           .withConfigurationName(
                               L"public-process-mom-exception-report-client")
                           .withRtiAddress(
                               L"tcp://127.0.0.1:" + std::to_wstring(port));
  std::exception_ptr clientError;
  std::optional<ConfigurationResult> connectionResult;
  bool joined = false;
  try {
    connectionResult = rti->connect(federate, HLA_EVOKED, configuration);
    REQUIRE(connectionResult->addressUsed);
    rti->createFederationExecution(federationName, L"server-owned-fom.xml");
    static_cast<void>(rti->joinFederationExecution(
        federateName,
        L"public-process-mom-exception-report-type",
        federationName));
    joined = true;

    auto const reportClass = rti->getInteractionClassHandle(
        L"HLAinteractionRoot.HLAmanager.HLAfederate.HLAreport.HLAreportMOMexception");
    REQUIRE(reportClass.isValid());
    auto const serviceParameter = rti->getParameterHandle(
        reportClass, L"HLAservice");
    auto const exceptionParameter = rti->getParameterHandle(
        reportClass, L"HLAexception");
    auto const parameterErrorParameter = rti->getParameterHandle(
        reportClass, L"HLAparameterError");
    REQUIRE(serviceParameter.isValid());
    REQUIRE(exceptionParameter.isValid());
    REQUIRE(parameterErrorParameter.isValid());
    REQUIRE_NOTHROW(rti->subscribeInteractionClass(reportClass, true));
    REQUIRE_FALSE(rti->getServiceReportingSwitch());

    auto const reportInvocation = rti->getInteractionClassHandle(
        L"HLAinteractionRoot.HLAmanager.HLAfederate.HLAreport.HLAreportServiceInvocation");
    REQUIRE(reportInvocation.isValid());
    REQUIRE_NOTHROW(rti->subscribeInteractionClass(reportInvocation, true));

    auto const setSwitches = rti->getInteractionClassHandle(
        L"HLAinteractionRoot.HLAmanager.HLAfederate.HLAadjust.HLAsetSwitches");
    auto const serviceReporting = rti->getParameterHandle(
        setSwitches, L"HLAserviceReporting");
    REQUIRE(setSwitches.isValid());
    REQUIRE(serviceReporting.isValid());

    auto const encodedSwitch = rti1516_2025::HLAinteger32BE(1).encode();
    REQUIRE_THROWS_AS(
        rti->sendInteraction(
            setSwitches,
            ParameterHandleValueMap{{serviceReporting, encodedSwitch}},
            VariableLengthData()),
        RTIinternalError);
    while (rti->evokeCallback(0.0)) {
    }
    REQUIRE(federate.receivedInteractionCount == 1U);
    REQUIRE(federate.receivedInteractionClass == reportClass);
    REQUIRE(federate.receivedParameterValues.size() == 4U);
    REQUIRE(federate.receivedTagSize == 0U);
    REQUIRE(federate.receivedTransportationType ==
            rti->getTransportationTypeHandle(L"HLAreliable"));
    REQUIRE_FALSE(federate.receivedProducingFederate.isValid());
    REQUIRE_FALSE(federate.receivedSentRegions);

    rti1516_2025::HLAunicodeString decodedService;
    REQUIRE_NOTHROW(decodedService.decode(
        federate.receivedParameterValues.at(serviceParameter)));
    REQUIRE(decodedService.get() ==
            L"HLAinteractionRoot.HLAmanager.HLAfederate.HLAadjust.HLAsetSwitches");
    rti1516_2025::HLAunicodeString decodedException;
    REQUIRE_NOTHROW(decodedException.decode(
        federate.receivedParameterValues.at(exceptionParameter)));
    REQUIRE(decodedException.get().find(L"RTIinternalError") !=
            std::wstring::npos);
    rti1516_2025::HLAboolean decodedParameterError;
    REQUIRE_NOTHROW(decodedParameterError.decode(
        federate.receivedParameterValues.at(parameterErrorParameter)));
    REQUIRE_FALSE(decodedParameterError.get());
    REQUIRE_FALSE(rti->getServiceReportingSwitch());

    REQUIRE_NOTHROW(rti->unsubscribeInteractionClass(reportInvocation));
    REQUIRE_NOTHROW(rti->sendInteraction(
        setSwitches,
        ParameterHandleValueMap{{serviceReporting, encodedSwitch}},
        VariableLengthData()));
    REQUIRE(rti->getServiceReportingSwitch());

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
  REQUIRE(connectionResult.has_value());
}

#if defined(UMBRA_ENABLE_EMBEDDED_FEDERATION_MANAGEMENT)
#include <RTI/encoding/HLAfixedRecord.h>
#include <RTI/encoding/HLAvariableArray.h>
#endif


#endif

#endif
#endif
