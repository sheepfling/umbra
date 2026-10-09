#include "ieee1516_2025_federation_management_test_support.hpp"
#include "ieee1516_2025_federation_management_fixture_support.hpp"
#include "ieee1516_2025_federation_listing_test_support.hpp"
#include "ieee1516_2025_public_federation_restore_test_support.hpp"

namespace {
TEST_CASE(
    "Embedded service reporting delivers dimension and region lookup return arguments through MOM interaction",
    "[integration][development-profile][federation-management][support-services][ddm]"
    "[mom][service-reporting][service-report-interaction]"
    "[dimension-region-lookup-service-reports]"
    "[service-report-successful-dimension-region-lookup-matrix]"
    "[rti.service.get-available-dimensions-for-object-class]"
    "[rti.service.get-available-dimensions-for-interaction-class]"
    "[rti.service.get-dimension-handle][rti.service.get-dimension-name]"
    "[rti.service.get-dimension-upper-bound][rti.service.get-dimension-handle-set]"
    "[federate.callback.receive-interaction]") {
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
      L"lookup-success-dimension-subject", L"subject", federationName));
  REQUIRE_NOTHROW(observer->joinFederationExecution(
      L"lookup-success-dimension-observer", L"observer", federationName));

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

  REQUIRE_NOTHROW(subject->setServiceReportingSwitch(false));
  REQUIRE_NOTHROW(subject->setSendServiceReportsToFileSwitch(false));
  REQUIRE_NOTHROW(observer->setServiceReportingSwitch(false));
  REQUIRE_NOTHROW(observer->subscribeInteractionClass(reportClass));

  auto const barQuantity = subject->getDimensionHandle(fixture_hla::fixture::bar_quantity);
  auto const serverId = subject->getDimensionHandle(fixture_hla::fixture::server_id);
  auto const drink = subject->getObjectClassHandle(fixture_hla::fom::food_drink);
  auto const mainCourseServed = subject->getInteractionClassHandle(
      fixture_hla::fom::main_course_served);
  auto const region = subject->createRegion(DimensionHandleSet{barQuantity});
  REQUIRE(barQuantity.isValid());
  REQUIRE(serverId.isValid());
  REQUIRE(drink.isValid());
  REQUIRE(mainCourseServed.isValid());
  REQUIRE(region.isValid());
  REQUIRE_NOTHROW(subject->setRangeBounds(region, barQuantity, RangeBounds(0UL, 10UL)));
  REQUIRE(observerReports.interactionReports.empty());

  REQUIRE_NOTHROW(subject->setServiceReportingSwitch(true));
  REQUIRE_FALSE(subject->getSendServiceReportsToFileSwitch());

  auto const availableObjectDimensions = subject->getAvailableDimensionsForObjectClass(drink);
  auto const availableInteractionDimensions =
      subject->getAvailableDimensionsForInteractionClass(mainCourseServed);
  auto const resolvedDimension =
      subject->getDimensionHandle(fixture_hla::fixture::bar_quantity);
  auto const resolvedDimensionName = subject->getDimensionName(barQuantity);
  auto const upperBound = subject->getDimensionUpperBound(barQuantity);
  auto const resolvedDimensionSet = subject->getDimensionHandleSet(region);
  REQUIRE(availableObjectDimensions == DimensionHandleSet{barQuantity});
  REQUIRE(availableInteractionDimensions == DimensionHandleSet{serverId});
  REQUIRE(resolvedDimension == barQuantity);
  REQUIRE(resolvedDimensionName == fixture_hla::fixture::bar_quantity);
  REQUIRE(upperBound == 25UL);
  REQUIRE(resolvedDimensionSet == DimensionHandleSet{barQuantity});
  REQUIRE(observerReports.interactionReports.size() == 6U);

  auto const quoted = [](std::wstring const& value) {
    return std::wstring{L"\""} + value + L"\"";
  };
  auto const dimensionSetText = [](DimensionHandle const& value) {
    return std::wstring{L"[\""} + value.toString() + L"\"]";
  };
  struct ExpectedArgument final {
    rti1516_2025::Integer32 type;
    std::wstring name;
    std::wstring value;
  };
  auto const decodeArgument = [](rti1516_2025::HLAfixedRecord const& record,
                                 ExpectedArgument const& expected) {
    REQUIRE(dynamic_cast<rti1516_2025::HLAinteger32BE const&>(record.get(0U)).get() ==
            expected.type);
    REQUIRE(dynamic_cast<rti1516_2025::HLAunicodeString const&>(record.get(1U)).get() ==
            expected.name);
    REQUIRE(dynamic_cast<rti1516_2025::HLAunicodeString const&>(record.get(2U)).get() ==
            expected.value);
  };
  auto const decodeReport = [&](std::size_t index,
                                std::wstring const& service,
                                std::vector<ExpectedArgument> const& expectedSupplied,
                                ExpectedArgument const& expectedReturned) {
    auto const& report = observerReports.interactionReports.at(index);
    REQUIRE(report.interactionClass == reportClass);
    REQUIRE(report.parameterValues.size() == 8U);
    REQUIRE(report.userSuppliedTag.size() == 0U);
    REQUIRE(report.transportationType ==
            observer->getTransportationTypeHandle(standard_hla::mom::reliable));
    REQUIRE_FALSE(report.producingFederate.isValid());
    REQUIRE_FALSE(report.sentRegionsSupplied);

    rti1516_2025::HLAunicodeString decodedService;
    REQUIRE_NOTHROW(decodedService.decode(report.parameterValues.at(reportParameters[0])));
    REQUIRE(decodedService.get() == service);
    rti1516_2025::HLAinteger16BE decodedServiceType;
    REQUIRE_NOTHROW(decodedServiceType.decode(report.parameterValues.at(reportParameters[1])));
    REQUIRE(decodedServiceType.get() == 6);
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
    REQUIRE(suppliedArguments.size() == expectedSupplied.size());
    for (std::size_t argumentIndex = 0U; argumentIndex < expectedSupplied.size();
         ++argumentIndex) {
      auto const& supplied = dynamic_cast<rti1516_2025::HLAfixedRecord const&>(
          suppliedArguments.get(argumentIndex));
      decodeArgument(supplied, expectedSupplied[argumentIndex]);
    }

    rti1516_2025::HLAfixedRecord returnedArgument;
    returnedArgument.appendElement(rti1516_2025::HLAinteger32BE{})
        .appendElement(rti1516_2025::HLAunicodeString{})
        .appendElement(rti1516_2025::HLAunicodeString{});
    REQUIRE_NOTHROW(
        returnedArgument.decode(report.parameterValues.at(reportParameters[4])));
    decodeArgument(returnedArgument, expectedReturned);

    rti1516_2025::HLAunicodeString decodedException;
    REQUIRE_NOTHROW(decodedException.decode(report.parameterValues.at(reportParameters[5])));
    REQUIRE(decodedException.get().empty());
    rti1516_2025::HLAinteger32BE decodedSerial;
    REQUIRE_NOTHROW(decodedSerial.decode(report.parameterValues.at(reportParameters[6])));
    REQUIRE(decodedSerial.get() == static_cast<rti1516_2025::Integer32>(index));
  };

  decodeReport(
      0U,
      L"GetAvailableDimensionsForObjectClass",
      {{36, L"Object class handle", quoted(drink.toString())}},
      {11, L"A set of dimension handles", dimensionSetText(barQuantity)});
  decodeReport(
      1U,
      L"GetAvailableDimensionsForInteractionClass",
      {{27, L"Interaction class handle", quoted(mainCourseServed.toString())}},
      {11, L"A set of dimension handles", dimensionSetText(serverId)});
  decodeReport(
      2U,
      L"GetDimensionHandle",
      {{53, L"Dimension name", quoted(fixture_hla::fixture::bar_quantity)}},
      {10, L"Dimension handle", quoted(resolvedDimension.toString())});
  decodeReport(
      3U,
      L"GetDimensionName",
      {{10, L"Dimension handle", quoted(barQuantity.toString())}},
      {53, L"Dimension name", quoted(resolvedDimensionName)});
  decodeReport(
      4U,
      L"GetDimensionUpperBound",
      {{10, L"Dimension handle", quoted(barQuantity.toString())}},
      {35, L"Dimension upper bound", std::to_wstring(upperBound)});
  decodeReport(
      5U,
      L"GetDimensionHandleSet",
      {{42, L"Region handle", quoted(region.toString())}},
      {11, L"A set of dimensions", dimensionSetText(barQuantity)});

  REQUIRE_NOTHROW(subject->setServiceReportingSwitch(false));
  REQUIRE_NOTHROW(observer->unsubscribeInteractionClass(reportClass));
  REQUIRE_NOTHROW(observer->resignFederationExecution(NO_ACTION));
  REQUIRE_NOTHROW(subject->deleteRegion(region));
  REQUIRE_NOTHROW(subject->resignFederationExecution(NO_ACTION));
  REQUIRE_NOTHROW(subject->destroyFederationExecution(federationName));
  REQUIRE_NOTHROW(observer->disconnect());
  REQUIRE_NOTHROW(subject->disconnect());
}

}  // namespace
