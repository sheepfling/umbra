#include "ieee1516_2025_federation_management_test_support.hpp"
#include "ieee1516_2025_federation_management_fixture_support.hpp"
#include "ieee1516_2025_public_federation_restore_test_support.hpp"

namespace {
TEST_CASE(
    "Embedded service reporting suppresses accepted services while the reporting switch is disabled",
    "[integration][development-profile][federation-management][mom][service-report-file]"
    "[service-reporting][service-report-suppressed][support-services]"
    "[rti.service.get-service-reporting-switch]"
    "[rti.service.set-send-service-reports-to-file-switch]"
    "[rti.service.get-dimension-handle]"
    "[rti.service.subscribe-interaction-class]"
    "[federate.callback.receive-interaction]") {
  TestFederateAmbassador subjectReports;
  ReportingFederateAmbassador observerReports;
  auto subject = makeRti();
  auto observer = makeRti();
  auto const federationName = nextFederationName();
  auto const fomModule = resourcePath("examples/RestaurantFOMmodule-2025.xml").wstring();
  auto directory = public_federation_restore_test_support::temporaryServiceReportDirectory();
  auto configuration =
      public_federation_restore_test_support::configurationForServiceReportDirectory(directory.path());
  configuration.withRtiAddress(L"in-process");

  REQUIRE_NOTHROW(subject->connect(subjectReports, HLA_EVOKED, configuration));
  REQUIRE_NOTHROW(observer->connect(observerReports, HLA_EVOKED, configuration));
  REQUIRE_NOTHROW(
      subject->createFederationExecution(federationName, fomModule, standard_hla::mom::integer64_time));
  REQUIRE_NOTHROW(subject->joinFederationExecution(
      L"suppressed-report-subject", L"subject", federationName));
  REQUIRE_FALSE(subject->getServiceReportingSwitch());
  REQUIRE_NOTHROW(subject->setSendServiceReportsToFileSwitch(true));
  REQUIRE(subject->getSendServiceReportsToFileSwitch());
  REQUIRE_NOTHROW(observer->joinFederationExecution(
      L"suppressed-report-observer", L"observer", federationName));

  auto const reportClass = observer->getInteractionClassHandle(
      standard_hla::mom::report_service_invocation);
  REQUIRE(reportClass.isValid());
  REQUIRE_FALSE(observer->getServiceReportingSwitch());

  auto const files = public_federation_restore_test_support::serviceReportFiles(directory.path());
  REQUIRE(files.size() == 2U);
  auto const subjectFile = std::find_if(
      files.begin(),
      files.end(),
      [](std::filesystem::path const& candidate) {
        return candidate.filename().string().find("suppressed-report-subject") !=
               std::string::npos;
      });
  REQUIRE(subjectFile != files.end());
  REQUIRE(std::filesystem::exists(*subjectFile));
  std::error_code initialSizeError;
  auto const initialSize = std::filesystem::file_size(*subjectFile, initialSizeError);
  REQUIRE_FALSE(initialSizeError);
  REQUIRE(initialSize > 0U);

  // The subject's reporting switch is disabled while its file switch is on.
  // The accepted lookup remains a real service invocation, but it must not
  // append to the preallocated file or reach an otherwise eligible observer.
  REQUIRE_NOTHROW(observer->subscribeInteractionClass(reportClass));
  auto const dimension = subject->getDimensionHandle(fixture_hla::fixture::bar_quantity);
  REQUIRE(dimension.isValid());
  std::error_code finalSizeError;
  auto const finalSize = std::filesystem::file_size(*subjectFile, finalSizeError);
  REQUIRE_FALSE(finalSizeError);
  REQUIRE(finalSize == initialSize);
  REQUIRE_FALSE(observer->evokeCallback(0.0));
  REQUIRE(observerReports.interactionReports.empty());

  REQUIRE_NOTHROW(observer->unsubscribeInteractionClass(reportClass));
  REQUIRE_NOTHROW(observer->resignFederationExecution(NO_ACTION));
  REQUIRE_NOTHROW(subject->resignFederationExecution(NO_ACTION));
  REQUIRE_NOTHROW(subject->destroyFederationExecution(federationName));
  REQUIRE_NOTHROW(observer->disconnect());
  REQUIRE_NOTHROW(subject->disconnect());
}
}  // namespace
