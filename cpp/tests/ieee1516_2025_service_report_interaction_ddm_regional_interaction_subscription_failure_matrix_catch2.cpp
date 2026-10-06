#include "ieee1516_2025_federation_management_fixture_support.hpp"

namespace {
TEST_CASE(
    "Embedded service reporting delivers regional interaction subscription failures through MOM interaction",
    "[integration][development-profile][interaction-management][ddm][mom][service-reporting]"
    "[service-report-interaction][service-failure]"
    "[ddm-regional-failure][rti.service.ddm-regional-failure-matrix-interaction]") {
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
      L"regional-subscription-matrix-subject", L"subject", federationName));
  REQUIRE_NOTHROW(observer->joinFederationExecution(
      L"regional-subscription-matrix-observer", L"observer", federationName));

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
                                bool success,
                                std::vector<ExpectedArgument> const& expectedArguments,
                                std::wstring const& exception) {
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
    REQUIRE(decodedSuccess.get() == success);

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
    REQUIRE(decodedException.get() == exception);
    rti1516_2025::HLAinteger32BE decodedSerial;
    REQUIRE_NOTHROW(decodedSerial.decode(report.parameterValues.at(reportParameters[6])));
    REQUIRE(decodedSerial.get() == static_cast<rti1516_2025::Integer32>(index));
  };

  REQUIRE_THROWS_AS(
      subject->subscribeInteractionClassWithRegions(InteractionClassHandle{}, {}),
      rti1516_2025::InteractionClassNotDefined);
  verifyReport(
      0U,
      L"SubscribeInteractionClassWithRegions",
      false,
      {{27, L"Interaction class designator", quoted(InteractionClassHandle{}.toString())},
       {43, L"Set of region designators", L"[]"},
       {6, L"Optional passive subscription indicator", L"false"}},
      L"InteractionClassNotDefined: Subscribe Interaction Class With Regions requires a defined InteractionClassHandle.");

  REQUIRE_THROWS_AS(
      subject->unsubscribeInteractionClassWithRegions(InteractionClassHandle{}, {}),
      rti1516_2025::InteractionClassNotDefined);
  verifyReport(
      1U,
      L"UnsubscribeInteractionClassWithRegions",
      false,
      {{27, L"Interaction class designator", quoted(InteractionClassHandle{}.toString())},
       {43, L"Set of region designators", L"[]"}},
      L"InteractionClassNotDefined: Unsubscribe Interaction Class With Regions requires a defined InteractionClassHandle.");

  REQUIRE_NOTHROW(subject->subscribeInteractionClassWithRegions(
      interactionClass,
      RegionHandleSet{region}));
  verifyReport(
      2U,
      L"SubscribeInteractionClassWithRegions",
      true,
      {{27, L"Interaction class designator", quoted(interactionClass.toString())},
       {43, L"Set of region designators", L"[\"" + region.toString() + L"\"]"},
       {6, L"Optional passive subscription indicator", L"false"}},
      L"");

  REQUIRE_THROWS_AS(
      subject->unsubscribeInteractionClassWithRegions(
          interactionClass,
          RegionHandleSet{RegionHandle{}}),
      rti1516_2025::InvalidRegion);
  verifyReport(
      3U,
      L"UnsubscribeInteractionClassWithRegions",
      false,
      {{27, L"Interaction class designator", quoted(interactionClass.toString())},
       {43, L"Set of region designators", L"[\"" + RegionHandle{}.toString() + L"\"]"}},
      L"InvalidRegion: Unsubscribe Interaction Class With Regions requires valid RegionHandle values.");

  REQUIRE_NOTHROW(subject->unsubscribeInteractionClassWithRegions(
      interactionClass,
      RegionHandleSet{region}));
  verifyReport(
      4U,
      L"UnsubscribeInteractionClassWithRegions",
      true,
      {{27, L"Interaction class designator", quoted(interactionClass.toString())},
       {43, L"Set of region designators", L"[\"" + region.toString() + L"\"]"}},
      L"");
  REQUIRE(observerReports.interactionReports.size() == 5U);

  REQUIRE_NOTHROW(subject->setServiceReportingSwitch(false));
  REQUIRE_NOTHROW(observer->unsubscribeInteractionClass(reportClass));
  REQUIRE_NOTHROW(observer->resignFederationExecution(NO_ACTION));
  REQUIRE_NOTHROW(subject->deleteRegion(region));
  REQUIRE_NOTHROW(subject->resignFederationExecution(NO_ACTION));
  REQUIRE_NOTHROW(subject->destroyFederationExecution(federationName));
  REQUIRE_NOTHROW(observer->disconnect());
  REQUIRE_NOTHROW(subject->disconnect());
}
}
