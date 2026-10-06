#include "ieee1516_2025_federation_management_fixture_support.hpp"

namespace {
TEST_CASE(
    "Embedded service reporting delivers accepted regional interaction subscriptions through MOM interaction",
    "[integration][development-profile][interaction-management][ddm]"
    "[mom][service-reporting][service-report-interaction]"
    "[regional-interaction-subscription-service-report]"
    "[rti.service.regional-interaction-subscription-service-report-interaction]"
    "[rti.service.subscribe-interaction-class-with-regions]"
    "[rti.service.unsubscribe-interaction-class-with-regions]") {
  ReportingFederateAmbassador subjectReports;
  ReportingFederateAmbassador observerReports;
  auto subject = makeRti();
  auto observer = makeRti();
  auto const federationName = nextFederationName();
  auto const restaurantFom = resourcePath("examples/RestaurantFOMmodule-2025.xml").wstring();
  auto const switchFom =
      (std::filesystem::path(UMBRA_SOURCE_DIRECTORY) / "cpp" / "tests" / "data" /
       "switch-support-enabled-fom.xml")
          .wstring();

  REQUIRE_NOTHROW(subject->connect(subjectReports, HLA_EVOKED));
  REQUIRE_NOTHROW(observer->connect(observerReports, rti1516_2025::HLA_IMMEDIATE));
  REQUIRE_NOTHROW(subject->createFederationExecution(
      federationName,
      std::vector<std::wstring>{restaurantFom, switchFom},
      standard_hla::mom::integer64_time));
  REQUIRE_NOTHROW(subject->joinFederationExecution(
      L"regional-subscription-mom-subject", L"subject", federationName));
  REQUIRE_NOTHROW(observer->joinFederationExecution(
      L"regional-subscription-mom-observer", L"observer", federationName));

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
  REQUIRE_NOTHROW(observer->setServiceReportingSwitch(false));
  REQUIRE_NOTHROW(observer->subscribeInteractionClass(reportClass));
  REQUIRE_NOTHROW(subject->setServiceReportingSwitch(false));
  REQUIRE_NOTHROW(subject->setSendServiceReportsToFileSwitch(false));

  auto const interactionClass = subject->getInteractionClassHandle(
      fixture_hla::fom::main_course_served);
  auto const serverId = subject->getDimensionHandle(fixture_hla::fixture::server_id);
  REQUIRE(interactionClass.isValid());
  REQUIRE(serverId.isValid());
  auto const region = subject->createRegion(DimensionHandleSet{serverId});
  REQUIRE_NOTHROW(subject->setRangeBounds(region, serverId, RangeBounds(0UL, 10UL)));
  REQUIRE_NOTHROW(subject->commitRegionModifications(RegionHandleSet{region}));

  REQUIRE_NOTHROW(subject->setServiceReportingSwitch(true));
  REQUIRE_NOTHROW(subject->setSendServiceReportsToFileSwitch(false));

  struct ExpectedArgument final {
    std::int32_t type;
    std::wstring name;
    std::wstring value;
  };
  auto const quoted = [](std::wstring const& value) {
    return std::wstring{L"\""} + value + L"\"";
  };
  auto const verifyReport = [&](std::size_t index,
                                std::wstring const& service,
                                std::vector<ExpectedArgument> const& expectedArguments) {
    auto const& report = observerReports.interactionReports.at(index);
    REQUIRE(report.interactionClass == reportClass);
    REQUIRE(report.parameterValues.size() == 8U);

    rti1516_2025::HLAunicodeString decodedService;
    REQUIRE_NOTHROW(decodedService.decode(report.parameterValues.at(reportParameters[0])));
    REQUIRE(decodedService.get() == service);
    rti1516_2025::HLAinteger16BE decodedServiceType;
    REQUIRE_NOTHROW(decodedServiceType.decode(report.parameterValues.at(reportParameters[1])));
    REQUIRE(decodedServiceType.get() == 5);
    rti1516_2025::HLAboolean decodedSuccess;
    REQUIRE_NOTHROW(decodedSuccess.decode(report.parameterValues.at(reportParameters[2])));
    REQUIRE(decodedSuccess.get());

    rti1516_2025::HLAfixedRecord argumentPrototype;
    argumentPrototype.appendElement(rti1516_2025::HLAinteger32BE{})
        .appendElement(rti1516_2025::HLAunicodeString{})
        .appendElement(rti1516_2025::HLAunicodeString{});
    rti1516_2025::HLAvariableArray suppliedArguments{argumentPrototype};
    REQUIRE_NOTHROW(
        suppliedArguments.decode(report.parameterValues.at(reportParameters[3])));
    REQUIRE(suppliedArguments.size() == expectedArguments.size());
    for (std::size_t argumentIndex = 0U; argumentIndex < expectedArguments.size();
         ++argumentIndex) {
      auto const& actual = dynamic_cast<rti1516_2025::HLAfixedRecord const&>(
          suppliedArguments.get(argumentIndex));
      REQUIRE(dynamic_cast<rti1516_2025::HLAinteger32BE const&>(actual.get(0U)).get() ==
              expectedArguments[argumentIndex].type);
      REQUIRE(dynamic_cast<rti1516_2025::HLAunicodeString const&>(actual.get(1U)).get() ==
              expectedArguments[argumentIndex].name);
      REQUIRE(dynamic_cast<rti1516_2025::HLAunicodeString const&>(actual.get(2U)).get() ==
              expectedArguments[argumentIndex].value);
    }

    rti1516_2025::HLAfixedRecord nullReturned;
    nullReturned.appendElement(rti1516_2025::HLAinteger32BE{})
        .appendElement(rti1516_2025::HLAunicodeString{})
        .appendElement(rti1516_2025::HLAunicodeString{});
    REQUIRE_NOTHROW(nullReturned.decode(report.parameterValues.at(reportParameters[4])));
    REQUIRE(dynamic_cast<rti1516_2025::HLAinteger32BE const&>(nullReturned.get(0U)).get() ==
            34);
    REQUIRE(dynamic_cast<rti1516_2025::HLAunicodeString const&>(nullReturned.get(1U)).get().empty());
    REQUIRE(dynamic_cast<rti1516_2025::HLAunicodeString const&>(nullReturned.get(2U)).get() ==
            L"null");

    rti1516_2025::HLAunicodeString decodedException;
    REQUIRE_NOTHROW(decodedException.decode(report.parameterValues.at(reportParameters[5])));
    REQUIRE(decodedException.get().empty());
    rti1516_2025::HLAinteger32BE decodedSerial;
    REQUIRE_NOTHROW(decodedSerial.decode(report.parameterValues.at(reportParameters[6])));
    REQUIRE(decodedSerial.get() == static_cast<rti1516_2025::Integer32>(index));
  };

  auto const classValue = interactionClass.toString();
  auto const regionValue = region.toString();
  auto const associationArguments = [&](bool passive) {
    return std::vector<ExpectedArgument>{
        {27, L"Interaction class designator", quoted(classValue)},
        {43, L"Set of region designators", L"[\"" + regionValue + L"\"]"},
        {6, L"Optional passive subscription indicator", passive ? L"true" : L"false"}};
  };
  auto const unsubscribeArguments = std::vector<ExpectedArgument>{
      {27, L"Interaction class designator", quoted(classValue)},
      {43, L"Set of region designators", L"[\"" + regionValue + L"\"]"}};

  // The first accepted declaration is passive, then the same pair is
  // replaced with an active declaration. The public reports preserve the
  // inverse passive indicator and serial order for each accepted transition.
  REQUIRE_NOTHROW(subject->subscribeInteractionClassWithRegions(
      interactionClass,
      RegionHandleSet{region},
      false));
  verifyReport(
      0U,
      L"SubscribeInteractionClassWithRegions",
      associationArguments(true));
  REQUIRE_NOTHROW(subject->subscribeInteractionClassWithRegions(
      interactionClass,
      RegionHandleSet{region},
      true));
  verifyReport(
      1U,
      L"SubscribeInteractionClassWithRegions",
      associationArguments(false));
  REQUIRE_NOTHROW(subject->unsubscribeInteractionClassWithRegions(
      interactionClass,
      RegionHandleSet{region}));
  verifyReport(
      2U,
      L"UnsubscribeInteractionClassWithRegions",
      unsubscribeArguments);
  REQUIRE(observerReports.interactionReports.size() == 3U);

  REQUIRE_NOTHROW(observer->unsubscribeInteractionClass(reportClass));
  REQUIRE_NOTHROW(subject->setServiceReportingSwitch(false));
  REQUIRE_NOTHROW(subject->deleteRegion(region));
  REQUIRE_NOTHROW(observer->resignFederationExecution(NO_ACTION));
  REQUIRE_NOTHROW(subject->resignFederationExecution(NO_ACTION));
  REQUIRE_NOTHROW(subject->destroyFederationExecution(federationName));
  REQUIRE_NOTHROW(observer->disconnect());
  REQUIRE_NOTHROW(subject->disconnect());
}

}  // namespace
