#include "ieee1516_2025_federation_management_test_support.hpp"
#include "ieee1516_2025_federation_management_fixture_support.hpp"
#include "ieee1516_2025_federation_listing_test_support.hpp"
#include "ieee1516_2025_public_federation_restore_test_support.hpp"

namespace {
TEST_CASE(
    "Embedded service reporting delivers interaction-class and parameter lookup return arguments through MOM interaction",
    "[integration][development-profile][federation-management][support-services]"
    "[mom][service-reporting][service-report-interaction]"
    "[federation-interaction-parameter-lookup-service-reports]"
    "[service-report-successful-interaction-parameter-lookup-matrix]"
    "[rti.service.get-interaction-class-handle][rti.service.get-interaction-class-name]"
    "[rti.service.get-parameter-handle][rti.service.get-parameter-name]"
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
      L"lookup-success-interaction-subject", L"subject", federationName));
  REQUIRE_NOTHROW(observer->joinFederationExecution(
      L"lookup-success-interaction-observer", L"observer", federationName));

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

  auto const interactionName = std::wstring{fixture_hla::fom::main_course_served};
  auto const parameterName = std::wstring{fixture_hla::fixture::temperature_ok};
  auto const interaction = subject->getInteractionClassHandle(interactionName);
  auto const parameter = subject->getParameterHandle(interaction, parameterName);
  REQUIRE(interaction.isValid());
  REQUIRE(parameter.isValid());
  REQUIRE(observerReports.interactionReports.empty());

  REQUIRE_NOTHROW(subject->setServiceReportingSwitch(true));
  REQUIRE_FALSE(subject->getSendServiceReportsToFileSwitch());

  auto const resolvedInteraction = subject->getInteractionClassHandle(interactionName);
  auto const resolvedInteractionName = subject->getInteractionClassName(interaction);
  auto const resolvedParameter = subject->getParameterHandle(interaction, parameterName);
  auto const resolvedParameterName = subject->getParameterName(interaction, parameter);
  REQUIRE(resolvedInteraction == interaction);
  REQUIRE(resolvedInteractionName == interactionName);
  REQUIRE(resolvedParameter == parameter);
  REQUIRE(resolvedParameterName == parameterName);
  REQUIRE(observerReports.interactionReports.size() == 4U);

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
      L"GetInteractionClassHandle",
      {{53, L"Interaction class name", quoted(interactionName)}},
      {27, L"Interaction class handle", quoted(resolvedInteraction.toString())});
  decodeReport(
      1U,
      L"GetInteractionClassName",
      {{27, L"Interaction class handle", quoted(interaction.toString())}},
      {53, L"Interaction class name", quoted(resolvedInteractionName)});
  decodeReport(
      2U,
      L"GetParameterHandle",
      {{27, L"Interaction class handle", quoted(interaction.toString())},
       {53, L"Parameter name", quoted(parameterName)}},
      {39, L"Parameter handle", quoted(resolvedParameter.toString())});
  decodeReport(
      3U,
      L"GetParameterName",
      {{27, L"Interaction class handle", quoted(interaction.toString())},
       {39, L"Parameter handle", quoted(parameter.toString())}},
      {53, L"Parameter name", quoted(resolvedParameterName)});

  REQUIRE_NOTHROW(observer->unsubscribeInteractionClass(reportClass));
  REQUIRE_NOTHROW(observer->resignFederationExecution(NO_ACTION));
  REQUIRE_NOTHROW(subject->resignFederationExecution(NO_ACTION));
  REQUIRE_NOTHROW(subject->destroyFederationExecution(federationName));
  REQUIRE_NOTHROW(observer->disconnect());
  REQUIRE_NOTHROW(subject->disconnect());
}

}  // namespace
