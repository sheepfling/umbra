#include "ieee1516_2025_federation_management_test_support.hpp"
#include "ieee1516_2025_federation_management_fixture_support.hpp"
#include "ieee1516_2025_federation_listing_test_support.hpp"
#include "ieee1516_2025_public_federation_restore_test_support.hpp"

namespace {
TEST_CASE(
    "Embedded service reporting delivers handle normalization return arguments through MOM interaction",
    "[integration][development-profile][federation-management][support-services]"
    "[mom][service-reporting][service-report-interaction]"
    "[handle-normalization-service-reports]"
    "[service-report-successful-handle-normalization-matrix]"
    "[rti.service.normalize-service-group][rti.service.normalize-federate-handle]"
    "[rti.service.normalize-object-class-handle]"
    "[rti.service.normalize-interaction-class-handle]"
    "[rti.service.normalize-object-instance-handle]"
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
      L"handle-normalization-success-subject", L"subject", federationName));
  REQUIRE_NOTHROW(observer->joinFederationExecution(
      L"handle-normalization-success-observer", L"observer", federationName));

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

  auto const server = subject->getObjectClassHandle(fixture_hla::fom::employee_server);
  auto const efficiency = subject->getAttributeHandle(server, fixture_hla::fixture::efficiency);
  REQUIRE_NOTHROW(subject->publishObjectClassAttributes(server, {efficiency}));
  auto const takeOrder = subject->getInteractionClassHandle(
      fixture_hla::fom::server_take_order);
  auto const objectInstance = subject->registerObjectInstance(server);
  auto const ownerFederate = subject->getFederateHandle(
      L"handle-normalization-success-subject");
  auto const serviceGroupValue = subject->normalizeServiceGroup(rti1516_2025::SUPPORT_SERVICES);
  auto const federateValue = subject->normalizeFederateHandle(ownerFederate);
  auto const objectClassValue = subject->normalizeObjectClassHandle(server);
  auto const interactionClassValue = subject->normalizeInteractionClassHandle(takeOrder);
  auto const objectInstanceValue = subject->normalizeObjectInstanceHandle(objectInstance);
  REQUIRE(serviceGroupValue == static_cast<unsigned long>(rti1516_2025::SUPPORT_SERVICES));
  REQUIRE(observerReports.interactionReports.empty());

  REQUIRE_NOTHROW(subject->setServiceReportingSwitch(true));
  REQUIRE_FALSE(subject->getSendServiceReportsToFileSwitch());
  REQUIRE(subject->normalizeServiceGroup(rti1516_2025::SUPPORT_SERVICES) == serviceGroupValue);
  REQUIRE(subject->normalizeFederateHandle(ownerFederate) == federateValue);
  REQUIRE(subject->normalizeObjectClassHandle(server) == objectClassValue);
  REQUIRE(subject->normalizeInteractionClassHandle(takeOrder) == interactionClassValue);
  REQUIRE(subject->normalizeObjectInstanceHandle(objectInstance) == objectInstanceValue);
  REQUIRE(observerReports.interactionReports.size() == 5U);

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
      L"NormalizeServiceGroup",
      {{50, L"Service group indicator", quoted(L"SUPPORT_SERVICES")}},
      {35, L"Normalized value", std::to_wstring(serviceGroupValue)});
  decodeReport(
      1U,
      L"NormalizeFederateHandle",
      {{15,
        L"Federate handle",
        quoted(ownerFederate.toString())}},
      {35, L"Normalized value", std::to_wstring(federateValue)});
  decodeReport(
      2U,
      L"NormalizeObjectClassHandle",
      {{36, L"Object class handle", quoted(server.toString())}},
      {35, L"Normalized value", std::to_wstring(objectClassValue)});
  decodeReport(
      3U,
      L"NormalizeInteractionClassHandle",
      {{27, L"Interaction class handle", quoted(takeOrder.toString())}},
      {35, L"Normalized value", std::to_wstring(interactionClassValue)});
  decodeReport(
      4U,
      L"NormalizeObjectInstanceHandle",
      {{37, L"Object instance handle", quoted(objectInstance.toString())}},
      {35, L"Normalized value", std::to_wstring(objectInstanceValue)});

  REQUIRE_NOTHROW(subject->setServiceReportingSwitch(false));
  REQUIRE_NOTHROW(observer->unsubscribeInteractionClass(reportClass));
  REQUIRE_NOTHROW(observer->resignFederationExecution(NO_ACTION));
  REQUIRE_NOTHROW(subject->deleteObjectInstance(objectInstance, VariableLengthData{}));
  REQUIRE_NOTHROW(subject->resignFederationExecution(NO_ACTION));
  REQUIRE_NOTHROW(subject->destroyFederationExecution(federationName));
  REQUIRE_NOTHROW(observer->disconnect());
  REQUIRE_NOTHROW(subject->disconnect());
}

}  // namespace
