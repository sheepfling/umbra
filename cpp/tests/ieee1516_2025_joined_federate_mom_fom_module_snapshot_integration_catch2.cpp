#include "ieee1516_2025_federation_management_fixture_support.hpp"
#include "ieee1516_2025_public_federation_restore_test_support.hpp"

namespace {
TEST_CASE(
    "Embedded joined-federate MOM snapshots retain only FOM modules supplied at Join",
    "[integration][development-profile][federation-management][mom][mom-object-foundation][fom]") {
  using rti1516_2025::umbra_binding_detail::UmbraRtiAmbassador;

  TestFederateAmbassador reports;
  auto rti = std::make_unique<UmbraRtiAmbassador>();
  auto const federationName = nextFederationName();
  auto const baseFom =
      (std::filesystem::path(UMBRA_SOURCE_DIRECTORY) / "cpp" / "tests" / "data" /
       "switch-known-class-enabled-fom.xml")
          .wstring();
  auto const joinedFom =
      (std::filesystem::path(UMBRA_SOURCE_DIRECTORY) / "cpp" / "tests" / "data" /
       "switch-nrg-disabled-fom.xml")
          .wstring();
  auto directory = temporaryServiceReportDirectory();
  RtiConfiguration configuration = configurationForServiceReportDirectory(directory.path());

  REQUIRE_NOTHROW(rti->connect(reports, HLA_EVOKED, configuration));
  REQUIRE_NOTHROW(
      rti->createFederationExecution(federationName, baseFom, standard_hla::mom::integer64_time));
  REQUIRE_NOTHROW(rti->joinFederationExecution(
      L"mom-snapshot-additional-fom",
      L"observer",
      federationName,
      std::vector<std::wstring>{joinedFom}));
  auto const snapshot = rti->joinedFederateMomObjectSnapshotForTesting();
  REQUIRE(snapshot);

  bool foundJoinScopedModuleList = false;
  for (auto const& [attributeHandle, value] : snapshot->initialAttributeValues) {
    static_cast<void>(attributeHandle);
    auto const designators = decodeHlaUnicodeStringList(value);
    if (designators && *designators == std::vector<std::wstring>{joinedFom}) {
      foundJoinScopedModuleList = true;
      break;
    }
  }
  REQUIRE(foundJoinScopedModuleList);

  // The Table 5 initial report must use the same Join-scoped module list as
  // the private MOM object. It must not claim that the base FOM was supplied
  // by this federate at Join.
  auto const files = serviceReportFiles(directory.path());
  REQUIRE(files.size() == 1U);
  auto const encodedJoinedFom =
      umbra::detail::utf8FromWide(umbra::detail::formatMomString(joinedFom));
  REQUIRE(encodedJoinedFom);
  auto const joinScopedModuleList =
      std::string{"\"HLAFOMmoduleDesignatorList\":["} +
      *encodedJoinedFom + "]";
  REQUIRE(readTextFile(files.front()).find(joinScopedModuleList) != std::string::npos);

  REQUIRE_NOTHROW(rti->resignFederationExecution(NO_ACTION));
  REQUIRE_NOTHROW(rti->destroyFederationExecution(federationName));
  REQUIRE_NOTHROW(rti->disconnect());
}

}  // namespace
