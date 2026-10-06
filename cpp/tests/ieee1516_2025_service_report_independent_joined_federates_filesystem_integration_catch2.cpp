#include "ieee1516_2025_federation_management_test_support.hpp"
#include "ieee1516_2025_federation_management_fixture_support.hpp"
#include "ieee1516_2025_public_federation_restore_test_support.hpp"

namespace {
TEST_CASE(
    "Embedded simultaneously joined federates receive independent service-report files",
    "[integration][development-profile][federation-management][mom][service-report-file]"
    "[service-reporting][service-report-store][service-report-independent-joined-federates][2025]") {
  TestFederateAmbassador subjectReports;
  TestFederateAmbassador observerReports;
  auto subject = makeRti();
  auto observer = makeRti();
  auto const federationName = nextFederationName();
  auto const fomModule =
      (std::filesystem::path(UMBRA_SOURCE_DIRECTORY) / "cpp" / "tests" / "data" /
       "switch-nrg-disabled-fom.xml")
          .wstring();
  auto directory = public_federation_restore_test_support::temporaryServiceReportDirectory();
  auto configuration =
      public_federation_restore_test_support::configurationForServiceReportDirectory(directory.path());
  configuration.withRtiAddress(L"in-process");

  REQUIRE_NOTHROW(subject->connect(subjectReports, HLA_EVOKED, configuration));
  REQUIRE_NOTHROW(observer->connect(observerReports, HLA_EVOKED, configuration));
  REQUIRE_NOTHROW(
      subject->createFederationExecution(federationName, fomModule, standard_hla::mom::integer64_time));
  REQUIRE_NOTHROW(subject->joinFederationExecution(
      L"independent-report-subject", L"subject", federationName));
  REQUIRE_NOTHROW(observer->joinFederationExecution(
      L"independent-report-observer", L"observer", federationName));

  // The configured directory is shared, but each joined-federate lifetime
  // owns one distinct report file and its own initial record.  This runs
  // through the production factory path rather than the in-memory test seam.
  auto const files = public_federation_restore_test_support::serviceReportFiles(directory.path());
  REQUIRE(files.size() == 2U);
  REQUIRE(files.front() != files.back());
  auto const firstText = public_federation_restore_test_support::readTextFile(files.front());
  auto const secondText = public_federation_restore_test_support::readTextFile(files.back());
  auto const firstHasSubject =
      firstText.find("\"HLAfederateName\":\"independent-report-subject\"") != std::string::npos;
  auto const secondHasSubject =
      secondText.find("\"HLAfederateName\":\"independent-report-subject\"") != std::string::npos;
  auto const firstHasObserver =
      firstText.find("\"HLAfederateName\":\"independent-report-observer\"") != std::string::npos;
  auto const secondHasObserver =
      secondText.find("\"HLAfederateName\":\"independent-report-observer\"") != std::string::npos;
  REQUIRE(firstHasSubject != secondHasSubject);
  REQUIRE(firstHasObserver != secondHasObserver);
  REQUIRE(firstHasSubject != firstHasObserver);
  REQUIRE(secondHasSubject != secondHasObserver);

  REQUIRE_NOTHROW(observer->resignFederationExecution(NO_ACTION));
  REQUIRE_NOTHROW(subject->resignFederationExecution(NO_ACTION));
  REQUIRE_NOTHROW(subject->destroyFederationExecution(federationName));
  REQUIRE_NOTHROW(observer->disconnect());
  REQUIRE_NOTHROW(subject->disconnect());
}
} // namespace
