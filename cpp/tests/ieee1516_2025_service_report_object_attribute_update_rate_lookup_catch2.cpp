#include "ieee1516_2025_federation_management_test_support.hpp"
#include "ieee1516_2025_federation_management_fixture_support.hpp"
#include "ieee1516_2025_federation_listing_test_support.hpp"
#include "ieee1516_2025_public_federation_restore_test_support.hpp"

namespace {
TEST_CASE(
    "Embedded service reporting delivers object, attribute, and update-rate lookup return arguments through MOM interaction",
    "[integration][development-profile][federation-management][support-services]"
    "[mom][service-reporting][service-report-interaction]"
    "[federate-object-attribute-update-rate-lookup-service-reports]"
    "[service-report-successful-lookup-matrix]"
    "[rti.service.get-known-object-class-handle][rti.service.get-object-instance-handle]"
    "[rti.service.get-object-instance-name][rti.service.get-attribute-handle]"
    "[rti.service.get-attribute-name][rti.service.get-update-rate-value]"
    "[rti.service.get-update-rate-value-for-attribute]"
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
      L"lookup-success-object-subject", L"subject", federationName));
  REQUIRE_NOTHROW(observer->joinFederationExecution(
      L"lookup-success-object-observer", L"observer", federationName));

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

  auto const serverName = std::wstring{fixture_hla::fom::employee_server};
  auto const objectName = std::wstring{L"lookup-success-object"};
  auto const attributeName = std::wstring{fixture_hla::fixture::efficiency};
  auto const server = subject->getObjectClassHandle(serverName);
  auto const attribute = subject->getAttributeHandle(server, attributeName);
  REQUIRE_NOTHROW(subject->publishObjectClassAttributes(server, AttributeHandleSet{attribute}));
  REQUIRE_NOTHROW(subject->reserveObjectInstanceName(objectName));
  REQUIRE_FALSE(subject->evokeCallback(0.0));
  auto const object = subject->registerObjectInstance(server, objectName);
  REQUIRE(object.isValid());
  REQUIRE(observerReports.interactionReports.empty());

  REQUIRE_NOTHROW(subject->setServiceReportingSwitch(true));
  REQUIRE_FALSE(subject->getSendServiceReportsToFileSwitch());

  auto const knownClass = subject->getKnownObjectClassHandle(object);
  auto const objectByName = subject->getObjectInstanceHandle(objectName);
  auto const resolvedObjectName = subject->getObjectInstanceName(object);
  auto const attributeByClass = subject->getAttributeHandle(server, attributeName);
  auto const resolvedAttributeName = subject->getAttributeName(server, attribute);
  auto const maximumRate = subject->getUpdateRateValue(L"High");
  auto const attributeMaximumRate = subject->getUpdateRateValueForAttribute(object, attribute);
  REQUIRE(knownClass == server);
  REQUIRE(objectByName == object);
  REQUIRE(resolvedObjectName == objectName);
  REQUIRE(attributeByClass == attribute);
  REQUIRE(resolvedAttributeName == attributeName);
  REQUIRE(maximumRate == Catch::Approx(30.0));
  REQUIRE(attributeMaximumRate == Catch::Approx(0.0));
  REQUIRE(observerReports.interactionReports.size() == 7U);

  auto const quoted = [](std::wstring const& value) {
    return std::wstring{L"\""} + value + L"\"";
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
      L"GetKnownObjectClassHandle",
      {{37, L"Object instance handle", quoted(object.toString())}},
      {36, L"Object class handle", quoted(knownClass.toString())});
  decodeReport(
      1U,
      L"GetObjectInstanceHandle",
      {{53, L"Object instance name", quoted(objectName)}},
      {37, L"Object instance handle", quoted(objectByName.toString())});
  decodeReport(
      2U,
      L"GetObjectInstanceName",
      {{37, L"Object instance handle", quoted(object.toString())}},
      {53, L"Object instance name", quoted(resolvedObjectName)});
  decodeReport(
      3U,
      L"GetAttributeHandle",
      {{36, L"Object class handle", quoted(server.toString())},
       {53, L"Class attribute name", quoted(attributeName)}},
      {0, L"Class attribute handle", quoted(attributeByClass.toString())});
  decodeReport(
      4U,
      L"GetAttributeName",
      {{36, L"Object class handle", quoted(server.toString())},
       {0, L"Class attribute handle", quoted(attribute.toString())}},
      {53, L"Class attribute name", quoted(resolvedAttributeName)});
  decodeReport(
      5U,
      L"GetUpdateRateValue",
      {{53, L"Update rate name", quoted(L"High")}},
      {35, L"Maximum update rate value", std::to_wstring(maximumRate)});
  decodeReport(
      6U,
      L"GetUpdateRateValueForAttribute",
      {{37, L"Object instance handle", quoted(object.toString())},
       {0, L"Attribute handle", quoted(attribute.toString())}},
      {35, L"Maximum update rate value", std::to_wstring(attributeMaximumRate)});

  REQUIRE_NOTHROW(observer->unsubscribeInteractionClass(reportClass));
  REQUIRE_NOTHROW(observer->resignFederationExecution(NO_ACTION));
  REQUIRE_NOTHROW(subject->resignFederationExecution(NO_ACTION));
  REQUIRE_NOTHROW(subject->destroyFederationExecution(federationName));
  REQUIRE_NOTHROW(observer->disconnect());
  REQUIRE_NOTHROW(subject->disconnect());
}

}  // namespace
