#include "ieee1516_2025_federation_management_test_support.hpp"
#include "ieee1516_2025_federation_management_fixture_support.hpp"
#include "ieee1516_2025_public_federation_restore_test_support.hpp"

namespace {

namespace standard_hla = umbra::detail::hla::wide;
namespace fixture_hla = umbra::test::hla::wide;

TEST_CASE(
    "Embedded public Request Federation Restore rejects a corrupt filesystem state image without starting restore",
    "[integration][development-profile][federation-management][save-restore]"
    "[durable-save][filesystem][process-restart][federation-registry][restore][failure]"
    "[public-process-restart-corrupt-state-image]"
    "[rti.service.request-federation-save][rti.service.federate-save-begun]"
    "[rti.service.federate-save-complete][rti.service.request-federation-restore]"
    "[rti.service.resign-federation-execution]"
    "[federate.callback.initiate-federate-save][federate.callback.federation-saved]"
    "[federate.callback.request-federation-restore-failed]"
    "[federate.callback.federation-restore-begun]"
    "[federate.callback.initiate-federate-restore]"
    "[federate.callback.federation-restored]") {
  using public_federation_restore_test_support::ScopedEmbeddedFederationRegistry;
  using public_federation_restore_test_support::configurationForServiceReportAndFederationSaveDirectory;
  using public_federation_restore_test_support::temporaryFederationSaveDirectory;
  using public_federation_restore_test_support::temporaryServiceReportDirectory;

  auto const saveDirectory = temporaryFederationSaveDirectory();
  auto const reportDirectory = temporaryServiceReportDirectory();
  auto const saveStore =
      std::make_shared<umbra::detail::FilesystemFederationSaveCommitStore>(
          saveDirectory.path());
  auto const federationName = nextFederationName();
  std::wstring const federateName = L"corrupt-image-restore-federate";
  std::wstring const saveLabel = L"corrupt-image-restore-save";
  std::wstring savedObjectName;
  auto const fom = resourcePath("examples/RestaurantFOMmodule-2025.xml").wstring();

  {
    auto const registry =
        std::make_shared<umbra::detail::EmbeddedFederationRegistry>(nullptr, saveStore);
    ScopedEmbeddedFederationRegistry registryScope(registry);
    ReportingFederateAmbassador reports;
    auto rti = makeRti();
    auto configuration = configurationForServiceReportAndFederationSaveDirectory(
        reportDirectory.path(), saveDirectory.path());
    configuration.withRtiAddress(L"in-process");

    REQUIRE_NOTHROW(rti->connect(reports, HLA_EVOKED, configuration));
    REQUIRE_NOTHROW(rti->createFederationExecution(
        federationName, {fom}, standard_hla::mom::integer64_time));
    REQUIRE_NOTHROW(rti->joinFederationExecution(
        federateName, L"corrupt-image-restore-type", federationName));
    auto const objectClass = rti->getObjectClassHandle(
        fixture_hla::fom::employee_server);
    auto const efficiency = rti->getAttributeHandle(
        objectClass, fixture_hla::fixture::efficiency);
    REQUIRE(objectClass.isValid());
    REQUIRE(efficiency.isValid());
    REQUIRE_NOTHROW(rti->publishObjectClassAttributes(
        objectClass, rti1516_2025::AttributeHandleSet{efficiency}));
    auto const savedObject = rti->registerObjectInstance(objectClass);
    REQUIRE(savedObject.isValid());
    savedObjectName = rti->getObjectInstanceName(savedObject);
    REQUIRE_FALSE(savedObjectName.empty());
    REQUIRE_NOTHROW(rti->requestFederationSave(saveLabel));
    static_cast<void>(rti->evokeMultipleCallbacks(0.0, 0.0));
    REQUIRE(reports.initiateFederateSaveReports ==
            std::vector<std::wstring>{saveLabel});
    REQUIRE_NOTHROW(rti->federateSaveBegun());
    REQUIRE_NOTHROW(rti->federateSaveComplete());
    static_cast<void>(rti->evokeMultipleCallbacks(0.0, 0.0));
    REQUIRE(reports.federationSavedReportCount == 1U);

    REQUIRE_NOTHROW(rti->resignFederationExecution(NO_ACTION));
    REQUIRE_NOTHROW(rti->destroyFederationExecution(federationName));
    REQUIRE_NOTHROW(rti->disconnect());
  }

  std::filesystem::path manifestPath;
  for (auto const& entry : std::filesystem::directory_iterator(saveDirectory.path())) {
    if (entry.is_regular_file() && entry.path().extension() == ".json") {
      REQUIRE(manifestPath.empty());
      manifestPath = entry.path();
    }
  }
  REQUIRE_FALSE(manifestPath.empty());
  std::ifstream manifestInput(manifestPath, std::ios::binary);
  REQUIRE(manifestInput.good());
  std::string manifest{
      std::istreambuf_iterator<char>(manifestInput), std::istreambuf_iterator<char>()};
  REQUIRE_FALSE(manifestInput.bad());
  constexpr std::string_view stateImageMarker = "\"stateImage\": \"";
  auto const stateImagePayload = manifest.find(stateImageMarker);
  REQUIRE(stateImagePayload != std::string::npos);
  auto const firstPayloadByte = stateImagePayload + stateImageMarker.size();
  REQUIRE(firstPayloadByte < manifest.size());
  REQUIRE(manifest[firstPayloadByte] != '"');
  manifest[firstPayloadByte] = '!';
  std::ofstream manifestOutput(manifestPath, std::ios::binary | std::ios::trunc);
  REQUIRE(manifestOutput.good());
  manifestOutput.write(manifest.data(), static_cast<std::streamsize>(manifest.size()));
  manifestOutput.close();
  REQUIRE(manifestOutput.good());

  {
    auto const registry =
        std::make_shared<umbra::detail::EmbeddedFederationRegistry>(nullptr, saveStore);
    ScopedEmbeddedFederationRegistry registryScope(registry);
    ReportingFederateAmbassador reports;
    auto rti = makeRti();
    auto configuration = configurationForServiceReportAndFederationSaveDirectory(
        reportDirectory.path(), saveDirectory.path());
    configuration.withRtiAddress(L"in-process");

    REQUIRE_NOTHROW(rti->connect(reports, HLA_EVOKED, configuration));
    REQUIRE_NOTHROW(rti->createFederationExecution(
        federationName, {fom}, standard_hla::mom::integer64_time));
    REQUIRE_NOTHROW(rti->joinFederationExecution(
        federateName, L"corrupt-image-restore-type", federationName));
    REQUIRE_THROWS_AS(
        rti->getObjectInstanceHandle(savedObjectName),
        rti1516_2025::ObjectInstanceNotKnown);
    REQUIRE_NOTHROW(rti->requestFederationRestore(saveLabel));
    for (int pass = 0; pass != 32; ++pass) {
      if (!rti->evokeMultipleCallbacks(0.0, 0.0)) {
        break;
      }
    }

    REQUIRE(reports.requestFederationRestoreFailedReports ==
            std::vector<std::wstring>{saveLabel});
    REQUIRE(reports.requestFederationRestoreSucceededReports.empty());
    REQUIRE(reports.federationRestoreBegunReportCount == 0U);
    REQUIRE(reports.initiateFederateRestoreReports.empty());
    REQUIRE(reports.federationRestoredReportCount == 0U);
    REQUIRE_THROWS_AS(
        rti->getObjectInstanceHandle(savedObjectName),
        rti1516_2025::ObjectInstanceNotKnown);

    REQUIRE_NOTHROW(rti->resignFederationExecution(NO_ACTION));
    REQUIRE_NOTHROW(rti->destroyFederationExecution(federationName));
    REQUIRE_NOTHROW(rti->disconnect());
  }
}

}  // namespace
