#include <catch2/catch_test_macros.hpp>
#include "hla_test_names.hpp"

#include "ieee1516_2025_connection_test_support.hpp"
#include "ieee1516_2025_process_fom_test_support.hpp"
#include "internal/federation/federation_registry.hpp"
#include "internal/federation/process_federation_service.hpp"
#include "internal/federation/process_transport.hpp"
#include "internal/federation/process_transport_session.hpp"
#include "process_public_service_fixture.hpp"

#include <RTI/NullFederateAmbassador.h>
#include <RTI/RTI1516.h>

#include <atomic>
#include <chrono>
#include <cstddef>
#include <cstdint>
#include <exception>
#include <filesystem>
#include <map>
#include <memory>
#include <mutex>
#include <set>
#include <stdexcept>
#include <string>
#include <system_error>
#include <thread>
#include <utility>
#include <vector>

#include <umbra/embedded_profile_configuration.hpp>

#if defined(UMBRA_ENABLE_EMBEDDED_FEDERATION_MANAGEMENT)
namespace {

using rti1516_2025::DimensionHandleSet;
using rti1516_2025::FederateHandle;
using rti1516_2025::HLA_EVOKED;
using rti1516_2025::NO_ACTION;
using rti1516_2025::NullFederateAmbassador;
using rti1516_2025::InteractionClassHandle;
using rti1516_2025::RTIambassador;
using rti1516_2025::RTIambassadorFactory;
using rti1516_2025::RtiConfiguration;
using rti1516_2025::RangeBounds;
using rti1516_2025::RegionHandle;
using rti1516_2025::RegionHandleSet;
using rti1516_2025::VariableLengthData;
using umbra::detail::EmbeddedFederationRegistry;
using umbra::detail::ProcessFederationService;
using umbra::detail::ProcessFederationServiceOptions;
using umbra::detail::ProcessTransportListener;
using umbra::detail::ProcessTransportServiceDispatcher;
using umbra::detail::ProcessTransportSession;
using umbra::test::process_fom_support_2025::composedProcessDefinition;
using umbra::test::process_fom_support_2025::processResourcePath;

std::unique_ptr<RTIambassador> makeRti() {
  RTIambassadorFactory factory;
  return factory.createRTIambassador();
}

class ProcessReportRegionObserver final : public NullFederateAmbassador {
 public:
  void receiveInteraction(
      rti1516_2025::InteractionClassHandle const& interaction,
      rti1516_2025::ParameterHandleValueMap const&,
      VariableLengthData const&,
      rti1516_2025::TransportationTypeHandle const&,
      FederateHandle const&,
      RegionHandleSet const* optionalSentRegions) override {
    ++callbackCount;
    receivedInteraction = interaction;
    sentRegionSetPresence.push_back(optionalSentRegions != nullptr);
    sentRegionCounts.push_back(
        optionalSentRegions == nullptr ? 0U : optionalSentRegions->size());
    if (optionalSentRegions == nullptr || queryAmbassador == nullptr) {
      return;
    }
    try {
      for (RegionHandle const& region : *optionalSentRegions) {
        DimensionHandleSet const dimensions =
            queryAmbassador->getDimensionHandleSet(region);
        for (auto const& dimension : dimensions) {
          auto const name = queryAmbassador->getDimensionName(dimension);
          auto const bounds = queryAmbassador->getRangeBounds(region, dimension);
          dimensionNames.insert(name);
          ranges.emplace(
              name,
              std::pair{bounds.getLowerBound(), bounds.getUpperBound()});
        }
      }
    } catch (...) {
      callbackFailure = std::current_exception();
    }
  }

  RTIambassador* queryAmbassador = nullptr;
  std::size_t callbackCount = 0U;
  rti1516_2025::InteractionClassHandle receivedInteraction;
  std::vector<bool> sentRegionSetPresence;
  std::vector<std::size_t> sentRegionCounts;
  std::set<std::wstring> dimensionNames;
  std::map<std::wstring, std::pair<unsigned long, unsigned long>> ranges;
  std::exception_ptr callbackFailure;
};

void setPointRange(
    RTIambassador& rti,
    RegionHandle region,
    rti1516_2025::DimensionHandle dimension,
    unsigned long point) {
  rti.setRangeBounds(region, dimension, RangeBounds(point, point + 1UL));
}

}  // namespace

TEST_CASE(
    "Configured-process HLAreportServiceInvocation conveys an optional callback region only when enabled",
    "[integration][process-boundary][public-endpoint][2025][mom][service-reporting][ddm]"
    "[process-mom-service-report-region-callback]"
    "[requirement-candidate-content-clauses-11-management-object-model-page-291-l8-2]"
    "[requirement-candidate-content-clauses-11-management-object-model-page-291-l14-4]"
    "[requirement-candidate-content-clauses-11-management-object-model-page-291-l17-5]"
    "[requirement-candidate-content-clauses-11-management-object-model-page-291-l29-9]"
    "[requirement-candidate-content-clauses-09-data-distribution-management-page-225-l18-4]"
    "[rti.service.set-service-reporting-switch][rti.service.set-convey-region-designator-sets-switch]"
    "[rti.service.subscribe-interaction-class-with-regions]"
    "[rti.service.normalize-federate-handle][rti.service.normalize-service-group]"
    "[rti.service.get-dimension-handle-set][rti.service.get-dimension-name]"
    "[rti.service.get-range-bounds][federate.callback.receive-interaction][callback-evoked]") {
  using umbra::test::connection_support_2025::temporaryServiceReportDirectory;

  auto listener = ProcessTransportListener::listen({"127.0.0.1", 0U});
  REQUIRE(listener);
  auto const port = listener->address().port;
  REQUIRE(port != 0U);
  auto const reportDirectory = temporaryServiceReportDirectory();
  constexpr wchar_t const* federationName =
      L"process-mom-service-report-region-execution";
  constexpr wchar_t const* reporterName =
      L"process-mom-service-report-region-reporter";
  constexpr wchar_t const* observerName =
      L"process-mom-service-report-region-observer";

  std::exception_ptr serverError;
  std::mutex serverErrorMutex;
  std::thread server([&] {
    std::vector<std::shared_ptr<umbra::detail::ProcessTransportConnection>>
        connections;
    std::vector<std::unique_ptr<ProcessTransportSession>> sessions;
    std::vector<std::thread> workers;
    try {
      EmbeddedFederationRegistry registry;
      ProcessFederationServiceOptions options;
      options.serviceReportDirectory = reportDirectory;
      ProcessFederationService service(
          registry, composedProcessDefinition(), std::move(options));
      auto acceptAndServe = [&](char const* name, std::uint64_t identity) {
        auto connection = listener->accept(
            nullptr, {name, identity}, [](std::wstring) {},
            [](std::wstring) { return false; });
        auto session = std::make_unique<ProcessTransportSession>(connection);
        auto* sessionPointer = session.get();
        auto handler = service.handlerFor(*sessionPointer);
        sessions.push_back(std::move(session));
        connections.push_back(connection);
        workers.emplace_back(
            [&, sessionPointer, connection, handler = std::move(handler)]() mutable {
              try {
                while (ProcessTransportServiceDispatcher::serveOne(
                    *sessionPointer, handler)) {
                }
                service.detach(*sessionPointer);
                connection->close();
              } catch (...) {
                service.detach(*sessionPointer);
                connection->close();
                std::scoped_lock lock(serverErrorMutex);
                if (!serverError) {
                  serverError = std::current_exception();
                }
              }
            });
      };
      acceptAndServe("process-mom-service-report-region-reporter", 0xA961U);
      acceptAndServe("process-mom-service-report-region-observer", 0xA962U);
      for (auto& worker : workers) {
        worker.join();
      }
    } catch (...) {
      std::scoped_lock lock(serverErrorMutex);
      if (!serverError) {
        serverError = std::current_exception();
      }
    }
  });

  ProcessReportRegionObserver observerCallbacks;
  NullFederateAmbassador reporterCallbacks;
  auto reporter = makeRti();
  auto observer = makeRti();
  observerCallbacks.queryAmbassador = observer.get();
  auto const makeConfiguration = [port](std::wstring name) {
    return RtiConfiguration::createConfiguration()
        .withConfigurationName(std::move(name))
        .withRtiAddress(L"tcp://127.0.0.1:" + std::to_wstring(port));
  };
  std::exception_ptr clientError;
  bool reporterJoined = false;
  bool observerJoined = false;
  InteractionClassHandle expectedReportClass;
  unsigned long expectedNormalizedFederate = 0UL;
  unsigned long expectedNormalizedGroup = 0UL;
  try {
    if (!reporter->connect(
             reporterCallbacks, HLA_EVOKED,
             makeConfiguration(reporterName)).addressUsed ||
        !observer->connect(
             observerCallbacks, HLA_EVOKED,
             makeConfiguration(observerName)).addressUsed) {
      throw std::runtime_error("A process MOM region endpoint did not connect.");
    }
    reporter->createFederationExecution(
        federationName,
        processResourcePath("examples/RestaurantFOMmodule-2025.xml").wstring());
    auto const reporterHandle = reporter->joinFederationExecution(
        reporterName, L"reporter", federationName);
    reporterJoined = true;
    observer->joinFederationExecution(observerName, L"observer", federationName);
    observerJoined = true;

    expectedReportClass = observer->getInteractionClassHandle(
        umbra::test::hla::wide::standard::service_invocation);
    auto const federateDimension = observer->getDimensionHandle(umbra::test::hla::wide::standard::federate);
    auto const serviceGroupDimension = observer->getDimensionHandle(umbra::test::hla::wide::standard::service_group);
    if (!expectedReportClass.isValid() || !federateDimension.isValid() ||
        !serviceGroupDimension.isValid()) {
      throw std::runtime_error("The process MOM region handles were unavailable.");
    }
    expectedNormalizedFederate =
        observer->normalizeFederateHandle(reporterHandle);
    expectedNormalizedGroup =
        observer->normalizeServiceGroup(rti1516_2025::SUPPORT_SERVICES);
    auto const subscriptionRegion = observer->createRegion(
        DimensionHandleSet{federateDimension, serviceGroupDimension});
    setPointRange(
        *observer, subscriptionRegion, federateDimension,
        expectedNormalizedFederate);
    setPointRange(
        *observer, subscriptionRegion, serviceGroupDimension,
        expectedNormalizedGroup);
    observer->commitRegionModifications(RegionHandleSet{subscriptionRegion});
    observer->subscribeInteractionClassWithRegions(
        expectedReportClass, RegionHandleSet{subscriptionRegion});
    observer->setConveyRegionDesignatorSetsSwitch(true);

    // Keep the trigger's report routed to subscribers rather than the file.
    reporter->setSendServiceReportsToFileSwitch(false);
    reporter->setServiceReportingSwitch(true);
    REQUIRE_THROWS_AS(
        reporter->getDimensionName(rti1516_2025::DimensionHandle{}),
        rti1516_2025::InvalidDimensionHandle);
    auto const callbackDeadline =
        std::chrono::steady_clock::now() + std::chrono::seconds(4);
    while (observerCallbacks.callbackCount == 0U &&
           std::chrono::steady_clock::now() < callbackDeadline) {
      static_cast<void>(observer->evokeCallback(0.02));
    }

    if (observerCallbacks.callbackCount != 1U) {
      throw std::runtime_error(
          "The enabled-convey service report callback was not delivered.");
    }
    observer->setConveyRegionDesignatorSetsSwitch(false);
    REQUIRE_THROWS_AS(
        reporter->getDimensionName(rti1516_2025::DimensionHandle{}),
        rti1516_2025::InvalidDimensionHandle);
    auto const disabledCallbackDeadline =
        std::chrono::steady_clock::now() + std::chrono::seconds(4);
    while (observerCallbacks.callbackCount < 2U &&
           std::chrono::steady_clock::now() < disabledCallbackDeadline) {
      static_cast<void>(observer->evokeCallback(0.02));
    }
  } catch (...) {
    clientError = std::current_exception();
  }

  if (reporterJoined) {
    try {
      reporter->resignFederationExecution(NO_ACTION);
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
    reporter->disconnect();
  } catch (...) {
  }
  try {
    observer->disconnect();
  } catch (...) {
  }
  listener.reset();
  if (server.joinable()) {
    server.join();
  }
  std::error_code removeError;
  std::filesystem::remove_all(reportDirectory, removeError);

  if (clientError) {
    std::rethrow_exception(clientError);
  }
  REQUIRE_FALSE(serverError);
  REQUIRE(observerCallbacks.callbackCount == 2U);
  REQUIRE(observerCallbacks.receivedInteraction == expectedReportClass);
  REQUIRE(observerCallbacks.sentRegionSetPresence.size() == 2U);
  REQUIRE(observerCallbacks.sentRegionCounts.size() == 2U);
  CHECK(observerCallbacks.sentRegionSetPresence[0]);
  CHECK(observerCallbacks.sentRegionCounts[0] == 1U);
  CHECK_FALSE(observerCallbacks.sentRegionSetPresence[1]);
  CHECK(observerCallbacks.sentRegionCounts[1] == 0U);
  REQUIRE_FALSE(observerCallbacks.callbackFailure);
  CHECK((observerCallbacks.dimensionNames ==
         std::set<std::wstring>{umbra::test::hla::wide::standard::federate, umbra::test::hla::wide::standard::service_group}));
  REQUIRE(observerCallbacks.ranges.size() == 2U);
  CHECK((observerCallbacks.ranges.at(umbra::test::hla::wide::standard::federate) ==
         std::pair<unsigned long, unsigned long>{
             expectedNormalizedFederate,
             expectedNormalizedFederate + 1UL}));
  CHECK((observerCallbacks.ranges.at(umbra::test::hla::wide::standard::service_group) ==
         std::pair<unsigned long, unsigned long>{
             expectedNormalizedGroup, expectedNormalizedGroup + 1UL}));
}

#endif
