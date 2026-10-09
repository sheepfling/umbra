#include "ieee1516_2025_federation_management_test_support.hpp"
#include "ieee1516_2025_federation_management_fixture_support.hpp"

namespace {

TEST_CASE(
    "Embedded service reporting delivers save and restore requests through MOM",
    "[integration][development-profile][federation-management][mom][service-reporting]"
    "[service-report-interaction][federation-save]") {
  ReportingFederateAmbassador subjectReports;
  ReportingFederateAmbassador observerReports;
  auto subject = makeRti();
  auto observer = makeRti();
  auto const federationName = nextFederationName();
  auto const fomModule = resourcePath("examples/RestaurantFOMmodule-2025.xml").wstring();

  REQUIRE_NOTHROW(subject->connect(subjectReports, HLA_EVOKED));
  REQUIRE_NOTHROW(observer->connect(observerReports, rti1516_2025::HLA_IMMEDIATE));
  REQUIRE_NOTHROW(subject->createFederationExecution(
      federationName,
      fomModule,
      standard_hla::mom::integer64_time));
  REQUIRE_NOTHROW(subject->joinFederationExecution(
      L"timestamped-save-mom-subject", L"subject", federationName));
  REQUIRE_NOTHROW(observer->joinFederationExecution(
      L"timestamped-save-mom-observer", L"observer", federationName));

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

  auto const verifyReport = [&](std::size_t index,
                                std::wstring const& service,
                                std::int32_t serial,
                                std::int32_t serviceType) {
    auto const& report = observerReports.interactionReports.at(index);
    REQUIRE(report.interactionClass == reportClass);
    rti1516_2025::HLAunicodeString decodedService;
    REQUIRE_NOTHROW(decodedService.decode(report.parameterValues.at(reportParameters[0])));
    REQUIRE(decodedService.get() == service);
    rti1516_2025::HLAinteger16BE decodedServiceType;
    REQUIRE_NOTHROW(decodedServiceType.decode(report.parameterValues.at(reportParameters[1])));
    REQUIRE(decodedServiceType.get() == serviceType);
    rti1516_2025::HLAboolean decodedSuccess;
    REQUIRE_NOTHROW(decodedSuccess.decode(report.parameterValues.at(reportParameters[2])));
    REQUIRE(decodedSuccess.get());
    rti1516_2025::HLAinteger32BE decodedSerial;
    REQUIRE_NOTHROW(decodedSerial.decode(report.parameterValues.at(reportParameters[6])));
    REQUIRE(decodedSerial.get() == serial);
  };

  REQUIRE_NOTHROW(subject->enableTimeRegulation(rti1516_2025::HLAinteger64Interval(1)));
  REQUIRE(observerReports.interactionReports.size() == 1U);
  verifyReport(0U, L"EnableTimeRegulation", 0, 4);
  REQUIRE_FALSE(subject->evokeCallback(0.0));
  REQUIRE(subjectReports.timeRegulationEnabledReports.size() == 1U);

  REQUIRE_NOTHROW(subject->requestFederationSave(
      L"mom-timestamped-save",
      rti1516_2025::HLAinteger64Time(1)));
  REQUIRE(observerReports.interactionReports.size() == 3U);
  verifyReport(1U, L"RequestFederationSave", 1, 0);
  verifyReport(2U, L"InitiateFederateSave", 2, 0);
  REQUIRE_NOTHROW(subject->abortFederationSave());
  REQUIRE(observerReports.interactionReports.size() == 5U);
  verifyReport(3U, L"AbortFederationSave", 3, 0);
  verifyReport(4U, L"FederationSaved", 4, 0);

  auto const restoreLabel = std::wstring{L"mom-restore-public"};
  REQUIRE_NOTHROW(subject->requestFederationSave(restoreLabel));
  REQUIRE(observerReports.interactionReports.size() == 7U);
  verifyReport(5U, L"RequestFederationSave", 5, 0);
  verifyReport(6U, L"InitiateFederateSave", 6, 0);
  while (subject->evokeCallback(0.0)) {
  }
  REQUIRE_NOTHROW(subject->federateSaveBegun());
  REQUIRE_NOTHROW(observer->federateSaveBegun());
  REQUIRE_NOTHROW(subject->federateSaveComplete());
  REQUIRE_NOTHROW(observer->federateSaveComplete());
  REQUIRE(observerReports.interactionReports.size() == 10U);
  verifyReport(7U, L"FederateSaveBegun", 7, 0);
  verifyReport(8U, L"FederateSaveComplete", 8, 0);
  verifyReport(9U, L"FederationSaved", 9, 0);
  while (subject->evokeCallback(0.0)) {
  }

  REQUIRE_NOTHROW(subject->requestFederationRestore(restoreLabel));
  REQUIRE(observerReports.interactionReports.size() == 14U);
  verifyReport(10U, L"RequestFederationRestore", 10, 0);
  verifyReport(11U, L"ConfirmFederationRestorationRequest", 11, 0);
  verifyReport(12U, L"FederationRestoreBegun", 12, 0);
  verifyReport(13U, L"InitiateFederateRestore", 13, 0);
  while (subject->evokeCallback(0.0)) {
  }
  REQUIRE_NOTHROW(subject->federateRestoreComplete());
  REQUIRE(observerReports.interactionReports.size() == 15U);
  verifyReport(14U, L"FederateRestoreComplete", 14, 0);
  REQUIRE_NOTHROW(observer->federateRestoreComplete());
  while (subject->evokeCallback(0.0)) {
  }

  // The remaining accepted restore controls share the same public
  // federation-management interaction route and must be visible before their
  // status/failure callbacks are delivered.
  REQUIRE_NOTHROW(subject->requestFederationRestore(restoreLabel));
  REQUIRE(observerReports.interactionReports.size() == 19U);
  verifyReport(15U, L"RequestFederationRestore", 15, 0);
  verifyReport(16U, L"ConfirmFederationRestorationRequest", 16, 0);
  verifyReport(17U, L"FederationRestoreBegun", 17, 0);
  verifyReport(18U, L"InitiateFederateRestore", 18, 0);
  while (subject->evokeCallback(0.0)) {
  }
  REQUIRE_NOTHROW(subject->queryFederationRestoreStatus());
  REQUIRE(observerReports.interactionReports.size() == 21U);
  verifyReport(19U, L"QueryFederationRestoreStatus", 19, 0);
  verifyReport(20U, L"FederationRestoreStatusResponse", 20, 0);
  while (subject->evokeCallback(0.0)) {
  }
  REQUIRE_NOTHROW(subject->abortFederationRestore());
  REQUIRE(observerReports.interactionReports.size() == 22U);
  verifyReport(21U, L"AbortFederationRestore", 21, 0);
  while (subject->evokeCallback(0.0)) {
  }

  REQUIRE_NOTHROW(observer->resignFederationExecution(NO_ACTION));
  REQUIRE_NOTHROW(subject->resignFederationExecution(NO_ACTION));
  REQUIRE_NOTHROW(subject->destroyFederationExecution(federationName));
  REQUIRE_NOTHROW(observer->disconnect());
  REQUIRE_NOTHROW(subject->disconnect());
}
}  // namespace
