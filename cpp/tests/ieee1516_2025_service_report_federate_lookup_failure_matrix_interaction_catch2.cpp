#include "ieee1516_2025_federation_management_test_support.hpp"
#include "ieee1516_2025_federation_management_fixture_support.hpp"

namespace {
TEST_CASE(
    "Embedded service reporting delivers failed federate lookups through MOM interaction",
    "[integration][development-profile][federation-management][mom][service-reporting]"
    "[service-report-interaction][service-failure]"
    "[rti.service.get-federate-handle][rti.service.get-federate-name]"
    "[rti.service.federate-lookup-failure-matrix-interaction]") {
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
  FederateHandle subjectFederate;
  REQUIRE_NOTHROW(subjectFederate = subject->joinFederationExecution(
                      L"federate-lookup-mom-subject", L"subject", federationName));
  REQUIRE_NOTHROW(observer->joinFederationExecution(
      L"federate-lookup-mom-observer", L"observer", federationName));

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

  // Resolve the valid handle while service reporting is disabled, then make
  // only the matrix below visible on the MOM interaction route.
  REQUIRE(subject->getFederateHandle(L"federate-lookup-mom-subject") == subjectFederate);
  REQUIRE_NOTHROW(subject->setServiceReportingSwitch(true));
  REQUIRE_NOTHROW(subject->setSendServiceReportsToFileSwitch(false));

  auto const quoted = [](std::wstring const& value) {
    return std::wstring{L"\""} + value + L"\"";
  };
  struct ExpectedArgument final {
    std::int32_t type;
    std::wstring name;
    std::wstring value;
  };
  auto const verifyFailure = [&](std::size_t index,
                                 std::wstring const& service,
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
    REQUIRE(decodedServiceType.get() == 6);
    rti1516_2025::HLAboolean decodedSuccess;
    REQUIRE_NOTHROW(decodedSuccess.decode(report.parameterValues.at(reportParameters[2])));
    REQUIRE_FALSE(decodedSuccess.get());

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
      subject->getFederateHandle(L"federate-lookup-mom-missing"),
      rti1516_2025::NameNotFound);
  verifyFailure(
      0U,
      L"GetFederateHandle",
      {{53, L"Federate name", quoted(L"federate-lookup-mom-missing")}},
      L"NameNotFound: The supplied federate name is not active in this federation execution.");

  REQUIRE_THROWS_AS(subject->getFederateName(FederateHandle{}), rti1516_2025::InvalidFederateHandle);
  verifyFailure(
      1U,
      L"GetFederateName",
      {{15, L"Federate handle", quoted(FederateHandle{}.toString())}},
      L"InvalidFederateHandle: Get Federate Name requires a valid FederateHandle.");

  REQUIRE(subject->getFederateHandle(L"federate-lookup-mom-subject") == subjectFederate);
  REQUIRE(subject->getFederateName(subjectFederate) == L"federate-lookup-mom-subject");
  REQUIRE(observerReports.interactionReports.size() == 4U);
  for (std::size_t index = 2U; index < 4U; ++index) {
    rti1516_2025::HLAboolean success;
    REQUIRE_NOTHROW(success.decode(observerReports.interactionReports.at(index).parameterValues.at(
        reportParameters[2])));
    REQUIRE(success.get());
    rti1516_2025::HLAinteger32BE serial;
    REQUIRE_NOTHROW(serial.decode(observerReports.interactionReports.at(index).parameterValues.at(
        reportParameters[6])));
    REQUIRE(serial.get() == static_cast<rti1516_2025::Integer32>(index));
  }

  REQUIRE_NOTHROW(subject->setServiceReportingSwitch(false));
  REQUIRE_NOTHROW(observer->unsubscribeInteractionClass(reportClass));
  REQUIRE_NOTHROW(observer->resignFederationExecution(NO_ACTION));
  REQUIRE_NOTHROW(subject->resignFederationExecution(NO_ACTION));
  REQUIRE_NOTHROW(subject->destroyFederationExecution(federationName));
  REQUIRE_NOTHROW(observer->disconnect());
  REQUIRE_NOTHROW(subject->disconnect());
}
}
