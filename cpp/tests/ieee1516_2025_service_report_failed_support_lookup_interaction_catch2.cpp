#include "ieee1516_2025_federation_management_test_support.hpp"
#include "ieee1516_2025_federation_management_fixture_support.hpp"

namespace {
TEST_CASE(
    "Embedded service reporting delivers a failed support lookup through MOM interaction",
    "[integration][development-profile][federation-management][mom][service-reporting]"
    "[service-report-interaction][service-failure]"
    "[service-report-failed-support-lookup-interaction]"
    "[rti.service.get-object-class-handle]"
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
      L"failed-lookup-report-subject", L"subject", federationName));
  REQUIRE_NOTHROW(observer->joinFederationExecution(
      L"failed-lookup-report-observer", L"observer", federationName));

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
  REQUIRE_NOTHROW(subject->setServiceReportingSwitch(true));
  REQUIRE_NOTHROW(subject->setSendServiceReportsToFileSwitch(false));

  REQUIRE_THROWS_AS(
      subject->getObjectClassHandle(fixture_hla::fom::missing_object_for_interaction),
      rti1516_2025::NameNotFound);
  REQUIRE(observerReports.interactionReports.size() == 1U);
  auto const& failure = observerReports.interactionReports.front();
  REQUIRE(failure.interactionClass == reportClass);
  REQUIRE(failure.parameterValues.size() == 8U);

  rti1516_2025::HLAunicodeString failureService;
  REQUIRE_NOTHROW(failureService.decode(failure.parameterValues.at(reportParameters[0])));
  REQUIRE(failureService.get() == L"GetObjectClassHandle");
  rti1516_2025::HLAinteger16BE failureServiceType;
  REQUIRE_NOTHROW(failureServiceType.decode(failure.parameterValues.at(reportParameters[1])));
  REQUIRE(failureServiceType.get() == 6);
  rti1516_2025::HLAboolean failureSuccess;
  REQUIRE_NOTHROW(failureSuccess.decode(failure.parameterValues.at(reportParameters[2])));
  REQUIRE_FALSE(failureSuccess.get());

  rti1516_2025::HLAfixedRecord suppliedPrototype;
  suppliedPrototype.appendElement(rti1516_2025::HLAinteger32BE{})
      .appendElement(rti1516_2025::HLAunicodeString{})
      .appendElement(rti1516_2025::HLAunicodeString{});
  rti1516_2025::HLAvariableArray suppliedArguments{suppliedPrototype};
  REQUIRE_NOTHROW(
      suppliedArguments.decode(failure.parameterValues.at(reportParameters[3])));
  REQUIRE(suppliedArguments.size() == 1U);
  auto const& supplied = dynamic_cast<rti1516_2025::HLAfixedRecord const&>(
      suppliedArguments.get(0U));
  REQUIRE(dynamic_cast<rti1516_2025::HLAinteger32BE const&>(supplied.get(0U)).get() == 53);
  REQUIRE(dynamic_cast<rti1516_2025::HLAunicodeString const&>(supplied.get(1U)).get() ==
          L"Object class name");
  REQUIRE(dynamic_cast<rti1516_2025::HLAunicodeString const&>(supplied.get(2U)).get() ==
          L"\"HLAobjectRoot.MissingForInteractionFailure\"");

  rti1516_2025::HLAfixedRecord nullReturned;
  nullReturned.appendElement(rti1516_2025::HLAinteger32BE{})
      .appendElement(rti1516_2025::HLAunicodeString{})
      .appendElement(rti1516_2025::HLAunicodeString{});
  REQUIRE_NOTHROW(nullReturned.decode(failure.parameterValues.at(reportParameters[4])));
  REQUIRE(dynamic_cast<rti1516_2025::HLAinteger32BE const&>(nullReturned.get(0U)).get() == 34);
  REQUIRE(dynamic_cast<rti1516_2025::HLAunicodeString const&>(nullReturned.get(1U)).get().empty());
  REQUIRE(dynamic_cast<rti1516_2025::HLAunicodeString const&>(nullReturned.get(2U)).get() ==
          L"null");
  rti1516_2025::HLAunicodeString failureException;
  REQUIRE_NOTHROW(failureException.decode(failure.parameterValues.at(reportParameters[5])));
  REQUIRE(failureException.get() ==
          L"NameNotFound: The supplied object class name is not defined in this federation execution.");
  rti1516_2025::HLAinteger32BE failureSerial;
  REQUIRE_NOTHROW(failureSerial.decode(failure.parameterValues.at(reportParameters[6])));
  REQUIRE(failureSerial.get() == 0);

  auto const server = subject->getObjectClassHandle(fixture_hla::fom::employee_server);
  REQUIRE(server.isValid());
  REQUIRE(observerReports.interactionReports.size() == 2U);
  auto const& success = observerReports.interactionReports.back();
  rti1516_2025::HLAunicodeString successService;
  REQUIRE_NOTHROW(successService.decode(success.parameterValues.at(reportParameters[0])));
  REQUIRE(successService.get() == L"GetObjectClassHandle");
  rti1516_2025::HLAboolean successIndicator;
  REQUIRE_NOTHROW(successIndicator.decode(success.parameterValues.at(reportParameters[2])));
  REQUIRE(successIndicator.get());
  rti1516_2025::HLAinteger32BE successSerial;
  REQUIRE_NOTHROW(successSerial.decode(success.parameterValues.at(reportParameters[6])));
  REQUIRE(successSerial.get() == 1);

  REQUIRE_NOTHROW(subject->setServiceReportingSwitch(false));
  REQUIRE_NOTHROW(observer->unsubscribeInteractionClass(reportClass));
  REQUIRE_NOTHROW(observer->resignFederationExecution(NO_ACTION));
  REQUIRE_NOTHROW(subject->resignFederationExecution(NO_ACTION));
  REQUIRE_NOTHROW(subject->destroyFederationExecution(federationName));
  REQUIRE_NOTHROW(observer->disconnect());
  REQUIRE_NOTHROW(subject->disconnect());
}
}
