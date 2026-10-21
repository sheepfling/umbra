#include "ieee1516_2025_federation_management_fixture_support.hpp"

TEST_CASE(
    "Fresh-registry restore completes one saved pending Time Regulation request",
    "[integration][development-profile][federation-management][save-restore]"
    "[durable-save][filesystem][fresh-registry][restore][time-management][time-role]"
    "[restore-pending-time-role-callbacks][pending-application-request-state]"
    "[rti.service.enable-time-regulation][rti.service.request-federation-save]"
    "[rti.service.federate-save-begun][rti.service.federate-save-complete]"
    "[rti.service.request-federation-restore][rti.service.federate-restore-complete]"
    "[federate.callback.time-regulation-enabled][federate.callback.federation-restored]") {
  auto const saveDirectory = temporaryFederationSaveDirectory();
  auto const saveStore = std::make_shared<umbra::detail::FilesystemFederationSaveCommitStore>(
      saveDirectory.path());
  auto const sourceRegistry = std::make_shared<umbra::detail::EmbeddedFederationRegistry>(
      nullptr,
      saveStore);
  ScopedEmbeddedFederationRegistry sourceScope(sourceRegistry);

  auto const federationName = nextFederationName();
  auto const fomModule = resourcePath("examples/RestaurantFOMmodule-2025.xml").wstring();
  std::wstring const saveLabel = L"durable-pending-time-regulation";
  ReportingFederateAmbassador sourceReports;
  auto source = makeRti();
  FederateHandle sourceHandle;

  REQUIRE_NOTHROW(source->connect(sourceReports, HLA_EVOKED));
  REQUIRE_NOTHROW(source->createFederationExecution(
      federationName,
      fomModule,
      standard_hla::mom::integer64_time));
  REQUIRE_NOTHROW(sourceHandle = source->joinFederationExecution(
      L"durable-pending-time-regulation-member", L"time-managed", federationName));

  // Keep the HLA_EVOKED callback queued so the save point contains the
  // outstanding public Enable Time Regulation request rather than the
  // completed regulation role.
  REQUIRE_NOTHROW(source->enableTimeRegulation(
      rti1516_2025::HLAinteger64Interval(2)));
  REQUIRE(sourceReports.timeRegulationEnabledReports.empty());
  REQUIRE_NOTHROW(source->requestFederationSave(saveLabel));
  REQUIRE_NOTHROW(source->federateSaveBegun());
  REQUIRE_NOTHROW(source->federateSaveComplete());

  // Retire the source ambassador and registry. The new registry below has no
  // callback queue to reuse; it can only restore the filesystem-backed image.
  REQUIRE_NOTHROW(source->resignFederationExecution(NO_ACTION));
  REQUIRE_NOTHROW(source->destroyFederationExecution(federationName));
  REQUIRE_NOTHROW(source->disconnect());

  // Use a distinct store object pointed at the same directory so restoration
  // must reload the committed file-backed image rather than reuse a store
  // instance's process-local state.
  auto const restoredStore = std::make_shared<umbra::detail::FilesystemFederationSaveCommitStore>(
      saveDirectory.path());
  auto const restoredRegistry = std::make_shared<umbra::detail::EmbeddedFederationRegistry>(
      nullptr,
      restoredStore);
  ScopedEmbeddedFederationRegistry restoredScope(restoredRegistry);
  ReportingFederateAmbassador restoredReports;
  auto restored = makeRti();
  FederateHandle restoredHandle;

  REQUIRE_NOTHROW(restored->connect(restoredReports, HLA_EVOKED));
  REQUIRE_NOTHROW(restored->createFederationExecution(
      federationName,
      fomModule,
      standard_hla::mom::integer64_time));
  REQUIRE_NOTHROW(restoredHandle = restored->joinFederationExecution(
      L"durable-pending-time-regulation-member", L"time-managed", federationName));
  REQUIRE(restoredHandle == sourceHandle);
  REQUIRE(restoredReports.timeRegulationEnabledReports.empty());

  REQUIRE_NOTHROW(restored->requestFederationRestore(saveLabel));
  REQUIRE_NOTHROW(restored->federateRestoreComplete());
  while (restored->evokeCallback(0.0)) {
  }

  // §4.32 requires successful restore to return the federate to its saved
  // state; §8.2/§8.3.1 provide the pending request/callback contract. This
  // checks one rehydrated role request only, not general callback restoration.
  REQUIRE(restoredReports.timeRegulationEnabledReports.size() == 1U);
  REQUIRE(restoredReports.timeRegulationEnabledReports.front().value == L"0");
  REQUIRE(restoredReports.federationRestoredReportCount == 1U);

  REQUIRE_NOTHROW(restored->resignFederationExecution(NO_ACTION));
  REQUIRE_NOTHROW(restored->destroyFederationExecution(federationName));
  REQUIRE_NOTHROW(restored->disconnect());
}
