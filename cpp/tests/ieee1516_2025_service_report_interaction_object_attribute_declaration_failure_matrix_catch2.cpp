#include "ieee1516_2025_federation_management_fixture_support.hpp"

namespace {
TEST_CASE(
    "Embedded service reporting delivers failed object-attribute declarations through MOM interaction",
    "[integration][development-profile][federation-management][declaration-management]"
    "[mom][service-reporting][service-report-interaction][service-failure]"
    "[object-attribute-declaration-failure][rti.service.object-attribute-declaration-failure-matrix-interaction]") {
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
      L"object-attribute-declaration-failure-subject", L"subject", federationName));
  REQUIRE_NOTHROW(observer->joinFederationExecution(
      L"object-attribute-declaration-failure-observer", L"observer", federationName));

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
  auto const objectClass = subject->getObjectClassHandle(fixture_hla::fom::employee_server);
  auto const attribute = subject->getAttributeHandle(objectClass, fixture_hla::fixture::efficiency);
  REQUIRE(objectClass.isValid());
  REQUIRE(attribute.isValid());
  REQUIRE_NOTHROW(subject->setServiceReportingSwitch(true));
  REQUIRE_NOTHROW(subject->setSendServiceReportsToFileSwitch(false));

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
    REQUIRE(decodedServiceType.get() == 1);
    rti1516_2025::HLAboolean decodedSuccess;
    REQUIRE_NOTHROW(decodedSuccess.decode(report.parameterValues.at(reportParameters[2])));
    REQUIRE(decodedSuccess.get() == success);
    rti1516_2025::HLAfixedRecord argumentPrototype;
    argumentPrototype.appendElement(rti1516_2025::HLAinteger32BE{})
        .appendElement(rti1516_2025::HLAunicodeString{})
        .appendElement(rti1516_2025::HLAunicodeString{});
    rti1516_2025::HLAvariableArray suppliedArguments{argumentPrototype};
    REQUIRE_NOTHROW(suppliedArguments.decode(report.parameterValues.at(reportParameters[3])));
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

  auto const invalidObject = ObjectClassHandle{}.toString();
  auto const validObject = objectClass.toString();
  auto const invalidAttribute = AttributeHandle{}.toString();
  auto const validAttribute = attribute.toString();
  auto const publishArgs = [&](std::wstring const& objectValue,
                               std::wstring const& attributeSetValue) {
    return std::vector<ExpectedArgument>{
        {36, L"Object class designator", L"\"" + objectValue + L"\""},
        {1, L"Set of attribute designators", attributeSetValue}};
  };
  auto const unpublishArgs = [&](std::wstring const& objectValue,
                                 std::wstring const& attributeSetValue) {
    return std::vector<ExpectedArgument>{
        {36, L"Object class designator", L"\"" + objectValue + L"\""},
        {1, L"Optional set of attribute designators", attributeSetValue}};
  };
  auto const invalidSet = L"[\"" + invalidAttribute + L"\"]";
  auto const validSet = L"[\"" + validAttribute + L"\"]";

  REQUIRE_THROWS_AS(
      subject->publishObjectClassAttributes(ObjectClassHandle{}, AttributeHandleSet{}),
      rti1516_2025::ObjectClassNotDefined);
  verifyReport(
      0U,
      L"PublishObjectClassAttributes",
      publishArgs(invalidObject, L"[]"),
      false,
      L"ObjectClassNotDefined: Publish Object Class Attributes requires a defined ObjectClassHandle.");
  REQUIRE_THROWS_AS(
      subject->publishObjectClassAttributes(objectClass, AttributeHandleSet{AttributeHandle{}}),
      rti1516_2025::AttributeNotDefined);
  verifyReport(
      1U,
      L"PublishObjectClassAttributes",
      publishArgs(validObject, invalidSet),
      false,
      L"AttributeNotDefined: Publish Object Class Attributes requires defined AttributeHandle values.");
  REQUIRE_THROWS_AS(
      subject->unpublishObjectClassAttributes(ObjectClassHandle{}, AttributeHandleSet{}),
      rti1516_2025::ObjectClassNotDefined);
  verifyReport(
      2U,
      L"UnpublishObjectClassAttributes",
      unpublishArgs(invalidObject, L"[]"),
      false,
      L"ObjectClassNotDefined: Unpublish Object Class Attributes requires a defined ObjectClassHandle.");
  REQUIRE_THROWS_AS(
      subject->unpublishObjectClassAttributes(
          objectClass, AttributeHandleSet{AttributeHandle{}}),
      rti1516_2025::AttributeNotDefined);
  verifyReport(
      3U,
      L"UnpublishObjectClassAttributes",
      unpublishArgs(validObject, invalidSet),
      false,
      L"AttributeNotDefined: Unpublish Object Class Attributes requires defined AttributeHandle values.");

  AttributeHandleSet const attributes{attribute};
  REQUIRE_NOTHROW(subject->publishObjectClassAttributes(objectClass, attributes));
  verifyReport(
      4U,
      L"PublishObjectClassAttributes",
      publishArgs(validObject, validSet),
      true,
      L"");
  REQUIRE_NOTHROW(subject->unpublishObjectClassAttributes(objectClass, attributes));
  verifyReport(
      5U,
      L"UnpublishObjectClassAttributes",
      unpublishArgs(validObject, validSet),
      true,
      L"");
  REQUIRE(observerReports.interactionReports.size() == 6U);

  REQUIRE_NOTHROW(observer->unsubscribeInteractionClass(reportClass));
  REQUIRE_NOTHROW(observer->resignFederationExecution(NO_ACTION));
  REQUIRE_NOTHROW(subject->resignFederationExecution(NO_ACTION));
  REQUIRE_NOTHROW(subject->destroyFederationExecution(federationName));
  REQUIRE_NOTHROW(observer->disconnect());
  REQUIRE_NOTHROW(subject->disconnect());
}

}  // namespace
