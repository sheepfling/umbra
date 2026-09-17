#include <catch2/catch_test_macros.hpp>

#include "internal/fom/hla_names.hpp"
#include "hla_test_names.hpp"

#include <RTI/NullFederateAmbassador.h>
#include <RTI/RTI1516.h>
#include <RTI/encoding/BasicDataElements.h>
#include <RTI/encoding/HLAfixedRecord.h>
#include <RTI/encoding/HLAvariableArray.h>
#include <RTI/time/HLAinteger64Interval.h>
#include <RTI/time/HLAinteger64Time.h>

#include <atomic>
#include <cstdint>
#include <filesystem>
#include <memory>
#include <string>
#include <vector>

#ifndef UMBRA_SOURCE_DIRECTORY
#error "The timestamped regional interaction MOM-failure test requires the Umbra source directory."
#endif

namespace {

namespace standard_hla = umbra::detail::hla::wide;
namespace fixture_hla = umbra::test::hla::wide;

using rti1516_2025::CANCEL_THEN_DELETE_THEN_DIVEST;
using rti1516_2025::FederateHandle;
using rti1516_2025::HLA_IMMEDIATE;
using rti1516_2025::InteractionClassHandle;
using rti1516_2025::MessageRetractionHandle;
using rti1516_2025::ParameterHandle;
using rti1516_2025::ParameterHandleValueMap;
using rti1516_2025::RegionHandle;
using rti1516_2025::RegionHandleSet;
using rti1516_2025::TransportationTypeHandle;
using rti1516_2025::VariableLengthData;

std::wstring nextFederationName() {
  static std::atomic_uint64_t sequence{0U};
  return L"timestamped-regional-failure-mom-report-" +
      std::to_wstring(sequence.fetch_add(1U, std::memory_order_relaxed));
}

std::filesystem::path resourcePath(std::filesystem::path const& relativePath) {
  return std::filesystem::path(UMBRA_SOURCE_DIRECTORY) /
      "third_party" / "ieee1516.2-2025" / "resources" / relativePath;
}

std::unique_ptr<rti1516_2025::RTIambassador> makeRti() {
  rti1516_2025::RTIambassadorFactory factory;
  return factory.createRTIambassador();
}

class ReportingFederateAmbassador final
    : public rti1516_2025::NullFederateAmbassador {
 public:
  struct InteractionReport final {
    InteractionClassHandle interactionClass;
    ParameterHandleValueMap parameterValues;
    VariableLengthData userSuppliedTag;
    TransportationTypeHandle transportationType;
    FederateHandle producingFederate;
  };

  void receiveInteraction(
      InteractionClassHandle const& interactionClass,
      ParameterHandleValueMap const& parameterValues,
      VariableLengthData const& userSuppliedTag,
      TransportationTypeHandle const& transportationType,
      FederateHandle const& producingFederate,
      RegionHandleSet const*) override {
    interactionReports.push_back({
        interactionClass,
        parameterValues,
        userSuppliedTag,
        transportationType,
        producingFederate,
    });
  }

  std::vector<InteractionReport> interactionReports;
};

}  // namespace

TEST_CASE(
    "Embedded service reporting delivers failed timestamped regional Send Interaction With Regions invocations through MOM interaction",
    "[integration][development-profile][federation-management][interaction-management]"
    "[ddm][time-management][mom][service-reporting][service-report-interaction][service-failure][tso]"
    "[timestamped-regional-interaction-failure]"
    "[rti.service.timestamped-regional-interaction-failure-matrix-interaction]"
    "[rti.service.send-interaction-with-regions][rti.service.change-interaction-order-type]"
    "[rti.service.enable-time-regulation][rti.service.set-service-reporting-switch]"
    "[rti.service.set-send-service-reports-to-file-switch]"
    "[rti.service.subscribe-interaction-class][federate.callback.receive-interaction]") {
  rti1516_2025::NullFederateAmbassador publisherCallbacks;
  ReportingFederateAmbassador observerCallbacks;
  auto publisher = makeRti();
  auto observer = makeRti();
  auto const federationName = nextFederationName();
  auto const fomModule = resourcePath("examples/RestaurantFOMmodule-2025.xml").wstring();
  unsigned char const parameterBytes[] = {0x01, 0x02};
  unsigned char const tagBytes[] = {'t', 's', 'o'};
  VariableLengthData const parameterValue(parameterBytes, sizeof(parameterBytes));
  VariableLengthData const tag(tagBytes, sizeof(tagBytes));

  REQUIRE_NOTHROW(publisher->connect(publisherCallbacks, rti1516_2025::HLA_EVOKED));
  REQUIRE_NOTHROW(publisher->createFederationExecution(
      federationName,
      fomModule,
      standard_hla::mom::integer64_time));
  REQUIRE_NOTHROW(publisher->joinFederationExecution(
      L"timestamped-regional-failure-mom-publisher",
      L"publisher",
      federationName));
  REQUIRE_NOTHROW(observer->connect(observerCallbacks, HLA_IMMEDIATE));
  REQUIRE_NOTHROW(observer->joinFederationExecution(
      L"timestamped-regional-failure-mom-observer",
      L"observer",
      federationName));

  REQUIRE_NOTHROW(publisher->setServiceReportingSwitch(false));
  REQUIRE_NOTHROW(publisher->setSendServiceReportsToFileSwitch(false));
  REQUIRE_NOTHROW(observer->setServiceReportingSwitch(false));
  REQUIRE_NOTHROW(observer->setSendServiceReportsToFileSwitch(false));

  auto const reportClass = observer->getInteractionClassHandle(
      standard_hla::mom::report_service_invocation);
  REQUIRE(reportClass.isValid());
  std::vector<ParameterHandle> reportParameters;
  for (auto const& name : {
           standard_hla::mom::service,
           standard_hla::mom::service_type,
           standard_hla::mom::success_indicator,
           standard_hla::mom::supplied_arguments,
           standard_hla::mom::returned_argument,
           standard_hla::mom::exception,
           standard_hla::mom::serial_number}) {
    auto const parameter = observer->getParameterHandle(reportClass, name);
    REQUIRE(parameter.isValid());
    reportParameters.push_back(parameter);
  }
  REQUIRE_NOTHROW(observer->subscribeInteractionClass(reportClass));

  auto const interactionClass = publisher->getInteractionClassHandle(
      fixture_hla::fom::main_course_served);
  auto const parameter = publisher->getParameterHandle(
      interactionClass,
      fixture_hla::fixture::temperature_ok);
  auto const dimension = publisher->getDimensionHandle(fixture_hla::fixture::server_id);
  REQUIRE(interactionClass.isValid());
  REQUIRE(parameter.isValid());
  REQUIRE(dimension.isValid());
  REQUIRE_NOTHROW(publisher->publishInteractionClass(interactionClass));
  REQUIRE_NOTHROW(publisher->changeInteractionOrderType(
      interactionClass,
      rti1516_2025::TIMESTAMP));
  auto const region = publisher->createRegion(
      rti1516_2025::DimensionHandleSet{dimension});
  REQUIRE_NOTHROW(publisher->setRangeBounds(
      region,
      dimension,
      rti1516_2025::RangeBounds(0UL, 10UL)));
  REQUIRE_NOTHROW(publisher->commitRegionModifications(RegionHandleSet{region}));
  REQUIRE_NOTHROW(publisher->enableTimeRegulation(
      rti1516_2025::HLAinteger64Interval(5)));
  while (publisher->evokeCallback(0.0)) {
  }

  REQUIRE_NOTHROW(publisher->setServiceReportingSwitch(true));
  REQUIRE_FALSE(publisher->getSendServiceReportsToFileSwitch());

  auto const verifyFailure = [&](std::size_t index,
                                 std::wstring const& interactionValue,
                                 std::wstring const& parameterMapValue,
                                 std::wstring const& regionSetValue,
                                 std::wstring const& timestampValue,
                                 std::wstring const& exceptionValue) {
    auto const& report = observerCallbacks.interactionReports.at(index);
    REQUIRE(report.interactionClass == reportClass);
    REQUIRE(report.parameterValues.size() == 7U);
    REQUIRE(report.userSuppliedTag.size() == 0U);
    REQUIRE(report.transportationType ==
            observer->getTransportationTypeHandle(standard_hla::mom::reliable));
    REQUIRE_FALSE(report.producingFederate.isValid());

    rti1516_2025::HLAunicodeString service;
    REQUIRE_NOTHROW(service.decode(report.parameterValues.at(reportParameters[0])));
    REQUIRE(service.get() == L"SendInteractionWithRegions");
    rti1516_2025::HLAinteger16BE serviceType;
    REQUIRE_NOTHROW(serviceType.decode(report.parameterValues.at(reportParameters[1])));
    REQUIRE(serviceType.get() == 2);
    rti1516_2025::HLAboolean success;
    REQUIRE_NOTHROW(success.decode(report.parameterValues.at(reportParameters[2])));
    REQUIRE_FALSE(success.get());

    rti1516_2025::HLAfixedRecord argumentPrototype;
    argumentPrototype.appendElement(rti1516_2025::HLAinteger32BE{})
        .appendElement(rti1516_2025::HLAunicodeString{})
        .appendElement(rti1516_2025::HLAunicodeString{});
    rti1516_2025::HLAvariableArray suppliedArguments{argumentPrototype};
    REQUIRE_NOTHROW(suppliedArguments.decode(
        report.parameterValues.at(reportParameters[3])));
    REQUIRE(suppliedArguments.size() == 5U);
    auto const verifyArgument = [&](std::size_t argumentIndex,
                                    std::int32_t type,
                                    std::wstring const& name,
                                    std::wstring const& value) {
      auto const& supplied = dynamic_cast<rti1516_2025::HLAfixedRecord const&>(
          suppliedArguments.get(argumentIndex));
      REQUIRE(dynamic_cast<rti1516_2025::HLAinteger32BE const&>(supplied.get(0U)).get() == type);
      REQUIRE(dynamic_cast<rti1516_2025::HLAunicodeString const&>(supplied.get(1U)).get() == name);
      REQUIRE(dynamic_cast<rti1516_2025::HLAunicodeString const&>(supplied.get(2U)).get() == value);
    };
    verifyArgument(0U, 27, L"Interaction class designator", interactionValue);
    verifyArgument(
        1U,
        40,
        L"Constrained set of interaction parameter designator and value pairs",
        parameterMapValue);
    verifyArgument(2U, 43, L"Set of region designators", regionSetValue);
    verifyArgument(3U, 63, L"User-supplied tag", L"\"dHNv\"");
    verifyArgument(4U, 31, L"Optional timestamp", timestampValue);

    rti1516_2025::HLAfixedRecord returned;
    returned.appendElement(rti1516_2025::HLAinteger32BE{})
        .appendElement(rti1516_2025::HLAunicodeString{})
        .appendElement(rti1516_2025::HLAunicodeString{});
    REQUIRE_NOTHROW(returned.decode(report.parameterValues.at(reportParameters[4])));
    REQUIRE(dynamic_cast<rti1516_2025::HLAinteger32BE const&>(returned.get(0U)).get() == 34);
    REQUIRE(dynamic_cast<rti1516_2025::HLAunicodeString const&>(returned.get(1U)).get().empty());
    REQUIRE(dynamic_cast<rti1516_2025::HLAunicodeString const&>(returned.get(2U)).get() ==
            L"null");

    rti1516_2025::HLAunicodeString exception;
    REQUIRE_NOTHROW(exception.decode(report.parameterValues.at(reportParameters[5])));
    REQUIRE(exception.get() == exceptionValue);
    rti1516_2025::HLAinteger32BE serial;
    REQUIRE_NOTHROW(serial.decode(report.parameterValues.at(reportParameters[6])));
    REQUIRE(serial.get() == static_cast<rti1516_2025::Integer32>(index));
  };

  auto const invalidInteractionValue =
      L"\"" + InteractionClassHandle{}.toString() + L"\"";
  auto const validInteractionValue =
      L"\"" + interactionClass.toString() + L"\"";
  auto const parameterMapValue =
      L"{\"" + parameter.toString() + L"\":\"AQI=\"}";
  auto const invalidParameterMapValue =
      L"{\"" + ParameterHandle{}.toString() + L"\":\"AQI=\"}";
  auto const regionSetValue =
      L"[\"" + region.toString() + L"\"]";
  auto const invalidRegionSetValue =
      L"[\"" + RegionHandle{}.toString() + L"\"]";

  REQUIRE_THROWS_AS(
      publisher->sendInteractionWithRegions(
          InteractionClassHandle{},
          ParameterHandleValueMap{{parameter, parameterValue}},
          RegionHandleSet{region},
          tag,
          rti1516_2025::HLAinteger64Time(6)),
      rti1516_2025::InteractionClassNotDefined);
  REQUIRE(observerCallbacks.interactionReports.size() == 1U);
  verifyFailure(
      0U,
      invalidInteractionValue,
      parameterMapValue,
      regionSetValue,
      L"\"6\"",
      L"InteractionClassNotDefined: Timestamped Send Interaction With Regions requires a defined InteractionClassHandle.");

  REQUIRE_THROWS_AS(
      publisher->sendInteractionWithRegions(
          interactionClass,
          ParameterHandleValueMap{{ParameterHandle{}, parameterValue}},
          RegionHandleSet{region},
          tag,
          rti1516_2025::HLAinteger64Time(6)),
      rti1516_2025::InteractionParameterNotDefined);
  REQUIRE(observerCallbacks.interactionReports.size() == 2U);
  verifyFailure(
      1U,
      validInteractionValue,
      invalidParameterMapValue,
      regionSetValue,
      L"\"6\"",
      L"InteractionParameterNotDefined: Timestamped Send Interaction With Regions requires defined ParameterHandle values.");

  REQUIRE_THROWS_AS(
      publisher->sendInteractionWithRegions(
          interactionClass,
          ParameterHandleValueMap{{parameter, parameterValue}},
          RegionHandleSet{RegionHandle{}},
          tag,
          rti1516_2025::HLAinteger64Time(6)),
      rti1516_2025::InvalidRegion);
  REQUIRE(observerCallbacks.interactionReports.size() == 3U);
  verifyFailure(
      2U,
      validInteractionValue,
      parameterMapValue,
      invalidRegionSetValue,
      L"\"6\"",
      L"InvalidRegion: Timestamped Send Interaction With Regions requires valid RegionHandle values.");

  REQUIRE_THROWS_AS(
      publisher->sendInteractionWithRegions(
          interactionClass,
          ParameterHandleValueMap{{parameter, parameterValue}},
          RegionHandleSet{region},
          tag,
          rti1516_2025::HLAinteger64Time(4)),
      rti1516_2025::InvalidLogicalTime);
  REQUIRE(observerCallbacks.interactionReports.size() == 4U);
  verifyFailure(
      3U,
      validInteractionValue,
      parameterMapValue,
      regionSetValue,
      L"\"4\"",
      L"InvalidLogicalTime: A timestamped service is earlier than the sender's current logical time plus lookahead.");

  REQUIRE_NOTHROW(publisher->setServiceReportingSwitch(false));
  REQUIRE_NOTHROW(publisher->setSendServiceReportsToFileSwitch(false));
  REQUIRE_NOTHROW(observer->unsubscribeInteractionClass(reportClass));
  REQUIRE_NOTHROW(publisher->deleteRegion(region));
  REQUIRE_NOTHROW(observer->resignFederationExecution(rti1516_2025::NO_ACTION));
  REQUIRE_NOTHROW(publisher->resignFederationExecution(CANCEL_THEN_DELETE_THEN_DIVEST));
  REQUIRE_NOTHROW(publisher->destroyFederationExecution(federationName));
  REQUIRE_NOTHROW(observer->disconnect());
  REQUIRE_NOTHROW(publisher->disconnect());
}
