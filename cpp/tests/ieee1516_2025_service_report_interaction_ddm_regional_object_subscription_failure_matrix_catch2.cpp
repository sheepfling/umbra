#include "ieee1516_2025_federation_management_fixture_support.hpp"

namespace {
TEST_CASE(
    "Embedded service reporting delivers failed regional object-class subscriptions through MOM interaction",
    "[integration][development-profile][federation-management][object-management][ddm]"
    "[mom][service-reporting][service-report-interaction][service-failure]"
    "[ddm-regional-object-failure][rti.service.ddm-regional-object-failure-matrix-interaction]") {
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
      L"regional-object-failure-subject", L"subject", federationName));
  REQUIRE_NOTHROW(observer->joinFederationExecution(
      L"regional-object-failure-observer", L"observer", federationName));

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

  auto const soda = subject->getObjectClassHandle(fixture_hla::fom::food_drink_soda);
  auto const flavor = subject->getAttributeHandle(soda, fixture_hla::fixture::flavor);
  auto const sodaFlavor = subject->getDimensionHandle(fixture_hla::fixture::soda_flavor);
  REQUIRE(soda.isValid());
  REQUIRE(flavor.isValid());
  REQUIRE(sodaFlavor.isValid());
  AttributeHandleSet const flavorOnly{flavor};
  auto const region = subject->createRegion(DimensionHandleSet{sodaFlavor});
  REQUIRE_NOTHROW(subject->setRangeBounds(region, sodaFlavor, RangeBounds(2UL, 3UL)));
  REQUIRE_NOTHROW(subject->commitRegionModifications(RegionHandleSet{region}));
  AttributeHandleSetRegionHandleSetPairVector const validPair{{
      flavorOnly,
      RegionHandleSet{region},
  }};
  AttributeHandleSetRegionHandleSetPairVector const invalidPair{{
      flavorOnly,
      RegionHandleSet{RegionHandle{}},
  }};

  // Setup and disabled calls must not create interaction reports.
  REQUIRE_THROWS_AS(
      subject->subscribeObjectClassAttributesWithRegions(ObjectClassHandle{}, {}, true, L"High"),
      rti1516_2025::ObjectClassNotDefined);
  REQUIRE_THROWS_AS(
      subject->unsubscribeObjectClassAttributesWithRegions(ObjectClassHandle{}, {}),
      rti1516_2025::ObjectClassNotDefined);
  REQUIRE_THROWS_AS(
      subject->subscribeObjectClassAttributesWithRegions(soda, invalidPair, true, L"High"),
      rti1516_2025::InvalidRegion);
  REQUIRE_THROWS_AS(
      subject->unsubscribeObjectClassAttributesWithRegions(soda, invalidPair),
      rti1516_2025::InvalidRegion);
  REQUIRE(observerReports.interactionReports.empty());

  REQUIRE_NOTHROW(subject->setServiceReportingSwitch(true));
  REQUIRE_NOTHROW(subject->setSendServiceReportsToFileSwitch(false));

  auto const quoted = [](std::wstring const& value) {
    return std::wstring{L"\""} + value + L"\"";
  };
  auto const pairValue = [](AttributeHandleSetRegionHandleSetPairVector const& pair) {
    if (pair.empty()) {
      return std::wstring{L"[]"};
    }
    auto const& first = pair.front();
    return std::wstring{L"[{\"attributeHandleSet\":[\""} +
           first.first.begin()->toString() +
           L"\"],\"regionHandleSet\":[\"" +
           first.second.begin()->toString() + L"\"]}]";
  };
  auto const emptyPairValue = pairValue({});
  auto const validPairValue = pairValue(validPair);
  auto const invalidPairValue = pairValue(invalidPair);
  auto const associationName =
      L"Collection of attribute designator set and region designator set pairs";
  struct ExpectedArgument final {
    std::int32_t type;
    std::wstring name;
    std::wstring value;
  };
  auto const verifyReport = [&](std::size_t index,
                                std::wstring const& service,
                                std::vector<ExpectedArgument> const& expectedArguments,
                                bool success,
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

  auto const invalidObjectClass = ObjectClassHandle{}.toString();
  auto const sodaValue = soda.toString();
  auto const failedSubscribeClassArguments = std::vector<ExpectedArgument>{
      {36, L"Object class designator", quoted(invalidObjectClass)},
      {4, associationName, emptyPairValue},
      {6, L"Optional passive subscription indicator", L"false"},
      {53, L"Optional update rate designator", quoted(L"High")}};
  auto const failedUnsubscribeClassArguments = std::vector<ExpectedArgument>{
      {36, L"Object class designator", quoted(invalidObjectClass)},
      {4, associationName, emptyPairValue}};
  auto const failedSubscribeRegionArguments = std::vector<ExpectedArgument>{
      {36, L"Object class designator", quoted(sodaValue)},
      {4, associationName, invalidPairValue},
      {6, L"Optional passive subscription indicator", L"false"},
      {53, L"Optional update rate designator", quoted(L"High")}};
  auto const failedUnsubscribeRegionArguments = std::vector<ExpectedArgument>{
      {36, L"Object class designator", quoted(sodaValue)},
      {4, associationName, invalidPairValue}};
  auto const successfulSubscribeArguments = std::vector<ExpectedArgument>{
      {36, L"Object class designator", quoted(sodaValue)},
      {4, associationName, validPairValue},
      {6, L"Optional passive subscription indicator", L"false"},
      {53, L"Optional update rate designator", quoted(L"High")}};
  auto const successfulUnsubscribeArguments = std::vector<ExpectedArgument>{
      {36, L"Object class designator", quoted(sodaValue)},
      {4, associationName, validPairValue}};

  REQUIRE_THROWS_AS(
      subject->subscribeObjectClassAttributesWithRegions(ObjectClassHandle{}, {}, true, L"High"),
      rti1516_2025::ObjectClassNotDefined);
  verifyReport(
      0U,
      L"SubscribeObjectClassAttributesWithRegions",
      failedSubscribeClassArguments,
      false,
      L"ObjectClassNotDefined: Subscribe Object Class Attributes With Regions requires a defined ObjectClassHandle.");
  REQUIRE_THROWS_AS(
      subject->unsubscribeObjectClassAttributesWithRegions(ObjectClassHandle{}, {}),
      rti1516_2025::ObjectClassNotDefined);
  verifyReport(
      1U,
      L"UnsubscribeObjectClassAttributesWithRegions",
      failedUnsubscribeClassArguments,
      false,
      L"ObjectClassNotDefined: Unsubscribe Object Class Attributes With Regions requires a defined ObjectClassHandle.");
  REQUIRE_THROWS_AS(
      subject->subscribeObjectClassAttributesWithRegions(soda, invalidPair, true, L"High"),
      rti1516_2025::InvalidRegion);
  verifyReport(
      2U,
      L"SubscribeObjectClassAttributesWithRegions",
      failedSubscribeRegionArguments,
      false,
      L"InvalidRegion: Subscribe Object Class Attributes With Regions requires defined RegionHandle values.");
  REQUIRE_THROWS_AS(
      subject->unsubscribeObjectClassAttributesWithRegions(soda, invalidPair),
      rti1516_2025::InvalidRegion);
  verifyReport(
      3U,
      L"UnsubscribeObjectClassAttributesWithRegions",
      failedUnsubscribeRegionArguments,
      false,
      L"InvalidRegion: Unsubscribe Object Class Attributes With Regions requires defined RegionHandle values.");
  REQUIRE_NOTHROW(
      subject->subscribeObjectClassAttributesWithRegions(soda, validPair, true, L"High"));
  verifyReport(
      4U,
      L"SubscribeObjectClassAttributesWithRegions",
      successfulSubscribeArguments,
      true,
      L"");
  REQUIRE_NOTHROW(subject->unsubscribeObjectClassAttributesWithRegions(soda, validPair));
  verifyReport(
      5U,
      L"UnsubscribeObjectClassAttributesWithRegions",
      successfulUnsubscribeArguments,
      true,
      L"");
  REQUIRE(observerReports.interactionReports.size() == 6U);

  REQUIRE_NOTHROW(observer->unsubscribeInteractionClass(reportClass));
  REQUIRE_NOTHROW(observer->resignFederationExecution(NO_ACTION));
  REQUIRE_NOTHROW(subject->deleteRegion(region));
  REQUIRE_NOTHROW(subject->resignFederationExecution(NO_ACTION));
  REQUIRE_NOTHROW(subject->destroyFederationExecution(federationName));
  REQUIRE_NOTHROW(observer->disconnect());
  REQUIRE_NOTHROW(subject->disconnect());
}

}  // namespace
