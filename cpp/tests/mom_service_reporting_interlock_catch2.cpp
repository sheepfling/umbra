#include <catch2/catch_test_macros.hpp>
#include "hla_test_names.hpp"

#include "internal/fom/hla_names.hpp"

#include <RTI/NullFederateAmbassador.h>
#include <RTI/RTI1516.h>

#include <atomic>
#include <exception>
#include <filesystem>
#include <memory>
#include <stdexcept>
#include <string>
#include <thread>

#if defined(UMBRA_ENABLE_EMBEDDED_FEDERATION_MANAGEMENT)
#include <umbra/embedded_profile_configuration.hpp>

#include "ieee1516_2025_process_fom_test_support.hpp"
#include "internal/federation/federation_registry.hpp"
#include "internal/federation/process_federation_service.hpp"
#include "internal/federation/process_transport.hpp"
#include "internal/federation/process_transport_session.hpp"
#include "process_public_service_fixture.hpp"
#endif

#ifndef UMBRA_SOURCE_DIRECTORY
#error "The MOM service-reporting interlock test requires the Umbra source directory."
#endif

namespace {

namespace standard_hla = umbra::detail::hla::wide;

using rti1516_2025::DimensionHandle;
using rti1516_2025::DimensionHandleSet;
using rti1516_2025::FederateServiceInvocationsAreBeingReportedViaMOM;
using rti1516_2025::HLA_EVOKED;
using rti1516_2025::NO_ACTION;
using rti1516_2025::RangeBounds;
using rti1516_2025::RegionHandle;
using rti1516_2025::RegionHandleSet;
using rti1516_2025::ReportServiceInvocationsAreSubscribed;
using rti1516_2025::RTIambassador;
using rti1516_2025::RTIambassadorFactory;

std::wstring nextFederationName() {
  static std::atomic_uint64_t sequence{0U};
  return L"mom-service-reporting-interlock-" +
      std::to_wstring(sequence.fetch_add(1U, std::memory_order_relaxed));
}

std::filesystem::path resourcePath(std::filesystem::path const& relativePath) {
  return std::filesystem::path(UMBRA_SOURCE_DIRECTORY) /
      "third_party" / "ieee1516.2-2025" / "resources" / relativePath;
}

std::unique_ptr<RTIambassador> makeRti() {
  RTIambassadorFactory factory;
  return factory.createRTIambassador();
}

}  // namespace

TEST_CASE(
    "Embedded MOM service-reporting state excludes report-service subscriptions",
    "[integration][development-profile][federation-management]"
    "[mom][service-reporting][service-reporting-interlock][ddm]"
    "[support-switches]"
    "[rti.service.get-service-reporting-switch]"
    "[rti.service.set-service-reporting-switch]"
    "[rti.service.subscribe-interaction-class]"
    "[rti.service.subscribe-interaction-class-with-regions]"
    "[rti.service.create-region][rti.service.set-range-bounds]"
    "[rti.service.commit-region-modifications]") {
  rti1516_2025::NullFederateAmbassador callbacks;
  auto rti = makeRti();
  auto const federationName = nextFederationName();
  auto const fomModule =
      resourcePath("examples/RestaurantFOMmodule-2025.xml").wstring();

  REQUIRE_NOTHROW(rti->connect(callbacks, HLA_EVOKED));
  REQUIRE_NOTHROW(rti->createFederationExecution(
      federationName,
      fomModule,
      standard_hla::mom::integer64_time));
  REQUIRE_NOTHROW(rti->joinFederationExecution(
      L"mom-service-reporting-interlock-federate",
      L"interlock",
      federationName));

  auto const reportServiceInvocation = rti->getInteractionClassHandle(
      standard_hla::mom::report_service_invocation);
  REQUIRE(reportServiceInvocation.isValid());
  REQUIRE_FALSE(rti->getServiceReportingSwitch());

  // Once enabled, both ordinary active and passive declarations are rejected
  // before they can become report-service subscriptions.
  REQUIRE_NOTHROW(rti->setServiceReportingSwitch(true));
  REQUIRE(rti->getServiceReportingSwitch());
  REQUIRE_THROWS_AS(
      rti->subscribeInteractionClass(reportServiceInvocation),
      FederateServiceInvocationsAreBeingReportedViaMOM);
  REQUIRE_THROWS_AS(
      rti->subscribeInteractionClass(reportServiceInvocation, false),
      FederateServiceInvocationsAreBeingReportedViaMOM);
  REQUIRE(rti->getServiceReportingSwitch());

  // HLAreportServiceInvocation carries the HLAserviceGroup dimension.  A
  // committed one-dimensional region keeps the regional path on the same
  // valid FOM context as the ordinary subscription path.
  auto const serviceGroup = rti->getDimensionHandle(umbra::test::hla::wide::standard::service_group);
  REQUIRE(serviceGroup.isValid());
  RegionHandle region;
  REQUIRE_NOTHROW(region = rti->createRegion(DimensionHandleSet{serviceGroup}));
  REQUIRE(region.isValid());
  REQUIRE_NOTHROW(rti->setRangeBounds(
      region,
      serviceGroup,
      RangeBounds(0UL, 7UL)));
  REQUIRE_NOTHROW(rti->commitRegionModifications(RegionHandleSet{region}));

  REQUIRE_THROWS_AS(
      rti->subscribeInteractionClassWithRegions(
          reportServiceInvocation,
          RegionHandleSet{region}),
      FederateServiceInvocationsAreBeingReportedViaMOM);
  REQUIRE_THROWS_AS(
      rti->subscribeInteractionClassWithRegions(
          reportServiceInvocation,
          RegionHandleSet{region},
          false),
      FederateServiceInvocationsAreBeingReportedViaMOM);
  REQUIRE(rti->getServiceReportingSwitch());

  // With reporting disabled, a passive ordinary subscription is legal but it
  // still counts as a subscription and therefore blocks re-enablement.
  REQUIRE_NOTHROW(rti->setServiceReportingSwitch(false));
  REQUIRE_FALSE(rti->getServiceReportingSwitch());
  REQUIRE_NOTHROW(rti->subscribeInteractionClass(reportServiceInvocation, false));
  REQUIRE_THROWS_AS(
      rti->setServiceReportingSwitch(true),
      ReportServiceInvocationsAreSubscribed);
  REQUIRE_FALSE(rti->getServiceReportingSwitch());
  REQUIRE_NOTHROW(rti->unsubscribeInteractionClass(reportServiceInvocation));
  REQUIRE_NOTHROW(rti->setServiceReportingSwitch(true));
  REQUIRE(rti->getServiceReportingSwitch());

  // The same subscription-count rule applies to a committed regional
  // declaration, for both active and passive selectors.
  REQUIRE_NOTHROW(rti->setServiceReportingSwitch(false));
  REQUIRE_NOTHROW(rti->subscribeInteractionClassWithRegions(
      reportServiceInvocation,
      RegionHandleSet{region}));
  REQUIRE_THROWS_AS(
      rti->setServiceReportingSwitch(true),
      ReportServiceInvocationsAreSubscribed);
  REQUIRE_FALSE(rti->getServiceReportingSwitch());
  REQUIRE_NOTHROW(rti->unsubscribeInteractionClassWithRegions(
      reportServiceInvocation,
      RegionHandleSet{region}));

  REQUIRE_NOTHROW(rti->subscribeInteractionClassWithRegions(
      reportServiceInvocation,
      RegionHandleSet{region},
      false));
  REQUIRE_THROWS_AS(
      rti->setServiceReportingSwitch(true),
      ReportServiceInvocationsAreSubscribed);
  REQUIRE_FALSE(rti->getServiceReportingSwitch());
  REQUIRE_NOTHROW(rti->unsubscribeInteractionClassWithRegions(
      reportServiceInvocation,
      RegionHandleSet{region}));
  REQUIRE_NOTHROW(rti->setServiceReportingSwitch(true));
  REQUIRE(rti->getServiceReportingSwitch());

  REQUIRE_NOTHROW(rti->deleteRegion(region));
  REQUIRE_NOTHROW(rti->resignFederationExecution(NO_ACTION));
  REQUIRE_NOTHROW(rti->destroyFederationExecution(federationName));
  REQUIRE_NOTHROW(rti->disconnect());
}

#if defined(UMBRA_ENABLE_EMBEDDED_FEDERATION_MANAGEMENT)
TEST_CASE(
    "Process MOM subscription prevents enabling the Service Reporting Switch with the official exception",
    "[integration][process-boundary][public-endpoint][2025]"
    "[mom][service-reporting][service-reporting-interlock]"
    "[rti.service.get-interaction-class-handle]"
    "[rti.service.subscribe-interaction-class]"
    "[rti.service.get-service-reporting-switch]"
    "[rti.service.set-service-reporting-switch]"
    "[rti.service.unsubscribe-interaction-class]"
    "[rti.service.resign-federation-execution]") {
  using namespace umbra::detail;
  using umbra::test::process_fom_support_2025::composedProcessDefinition;

  auto listener = ProcessTransportListener::listen({"127.0.0.1", 0U});
  REQUIRE(listener);
  auto const port = listener->address().port;
  REQUIRE(port != 0U);

  constexpr wchar_t const* federationName =
      L"process-mom-service-reporting-interlock-execution";
  constexpr wchar_t const* federateName =
      L"process-mom-service-reporting-interlock-federate";
  std::exception_ptr serverError;
  std::thread server([&] {
    try {
      EmbeddedFederationRegistry registry;
      ProcessFederationService service(
          registry, composedProcessDefinition(), ProcessFederationServiceOptions{});
      auto connection = listener->accept(
          nullptr,
          {"process-mom-service-reporting-interlock-server", 0x9E51U},
          [](std::wstring) {},
          [](std::wstring) { return false; });
      ProcessTransportSession session(connection);
      auto handler = service.handlerFor(session);
      auto serveExpected = [&](TransportServiceOperation operation,
                               char const* description) {
        if (!umbra::test::servePrimaryProcessRequest(
                session,
                handler,
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
          TransportServiceOperation::create_federation_execution,
          "The process MOM service-reporting fixture lost Create.");
      serveExpected(
          TransportServiceOperation::join_federation_execution,
          "The process MOM service-reporting fixture lost Join.");
      serveExpected(
          TransportServiceOperation::get_interaction_class_handle,
          "The process MOM service-reporting fixture lost report-class lookup.");
      serveExpected(
          TransportServiceOperation::subscribe_interaction_class,
          "The process MOM service-reporting fixture lost the report subscription.");
      serveExpected(
          TransportServiceOperation::get_service_reporting_switch,
          "The process MOM service-reporting fixture lost the initial switch query.");
      serveExpected(
          TransportServiceOperation::set_service_reporting_switch,
          "The process MOM service-reporting fixture lost the interlocked switch request.");
      serveExpected(
          TransportServiceOperation::get_service_reporting_switch,
          "The process MOM service-reporting fixture lost the unchanged switch query.");
      serveExpected(
          TransportServiceOperation::resign_federation_execution,
          "The process MOM service-reporting fixture lost Resign.");
      service.detach(session);
      connection->close();
    } catch (...) {
      serverError = std::current_exception();
    }
  });

  rti1516_2025::NullFederateAmbassador callbacks;
  auto rti = makeRti();
  auto configuration = rti1516_2025::RtiConfiguration::createConfiguration()
                           .withConfigurationName(
                               L"process-mom-service-reporting-interlock-client")
                           .withRtiAddress(
                               L"tcp://127.0.0.1:" + std::to_wstring(port));
  std::exception_ptr clientError;
  bool joined = false;
  try {
    REQUIRE(rti->connect(callbacks, HLA_EVOKED, configuration).addressUsed);
    REQUIRE_NOTHROW(rti->createFederationExecution(
        federationName, L"server-owned-fom.xml"));
    REQUIRE(rti->joinFederationExecution(
                federateName, L"process-mom-service-reporting-interlock-type", federationName)
                .isValid());
    joined = true;

    auto const reportInvocation = rti->getInteractionClassHandle(
        umbra::test::hla::wide::standard::service_invocation);
    REQUIRE(reportInvocation.isValid());
    REQUIRE_NOTHROW(rti->subscribeInteractionClass(reportInvocation));
    REQUIRE_FALSE(rti->getServiceReportingSwitch());

    REQUIRE_THROWS_AS(
        rti->setServiceReportingSwitch(true), ReportServiceInvocationsAreSubscribed);
    REQUIRE_FALSE(rti->getServiceReportingSwitch());

    REQUIRE_NOTHROW(rti->resignFederationExecution(NO_ACTION));
    joined = false;
    REQUIRE_NOTHROW(rti->disconnect());
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
  listener.reset();
  if (server.joinable()) {
    server.join();
  }
  if (clientError) {
    std::rethrow_exception(clientError);
  }
  REQUIRE_FALSE(serverError);
  REQUIRE_FALSE(joined);
}
#endif
