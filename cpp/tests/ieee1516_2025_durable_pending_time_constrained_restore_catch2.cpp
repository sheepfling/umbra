#include "ieee1516_2025_federation_management_fixture_support.hpp"

TEST_CASE(
    "Fresh-registry restore completes one saved pending Time Constrained request",
    "[integration][development-profile][federation-management][save-restore]"
    "[durable-save][filesystem][fresh-registry][restore][time-management][time-role]"
    "[restore-pending-time-role-callbacks][pending-application-request-state]"
    "[rti.service.enable-time-constrained][rti.service.request-federation-save]"
    "[rti.service.federate-save-begun][rti.service.federate-save-complete]"
    "[rti.service.request-federation-restore][rti.service.federate-restore-complete]"
    "[federate.callback.time-constrained-enabled][federate.callback.federation-restored]") {
  auto const saveDirectory = temporaryFederationSaveDirectory();
  auto const sourceStore = std::make_shared<umbra::detail::FilesystemFederationSaveCommitStore>(
      saveDirectory.path());
  auto const sourceRegistry = std::make_shared<umbra::detail::EmbeddedFederationRegistry>(
      nullptr,
      sourceStore);
  ScopedEmbeddedFederationRegistry sourceScope(sourceRegistry);

  auto const federationName = nextFederationName();
  auto const fomModule = resourcePath("examples/RestaurantFOMmodule-2025.xml").wstring();
  std::wstring const saveLabel = L"durable-pending-time-constrained";
  ReportingFederateAmbassador sourceReports;
  auto source = makeRti();
  FederateHandle sourceHandle;

  REQUIRE_NOTHROW(source->connect(sourceReports, HLA_EVOKED));
  REQUIRE_NOTHROW(source->createFederationExecution(
      federationName,
      fomModule,
      standard_hla::mom::integer64_time));
  REQUIRE_NOTHROW(sourceHandle = source->joinFederationExecution(
      L"durable-pending-time-constrained-member", L"time-managed", federationName));

  // Keep the HLA_EVOKED callback queued so the durable image contains the
  // pending public Enable Time Constrained request, not its completed role.
  REQUIRE_NOTHROW(source->enableTimeConstrained());
  REQUIRE(sourceReports.timeConstrainedEnabledReports.empty());
  REQUIRE_NOTHROW(source->requestFederationSave(saveLabel));
  REQUIRE_NOTHROW(source->federateSaveBegun());
  REQUIRE_NOTHROW(source->federateSaveComplete());

  REQUIRE_NOTHROW(source->resignFederationExecution(NO_ACTION));
  REQUIRE_NOTHROW(source->destroyFederationExecution(federationName));
  REQUIRE_NOTHROW(source->disconnect());

  // A new store and registry must reload the committed file-backed image;
  // no original callback route or same-store cache is available to reuse.
  auto const restoredStore = std::make_shared<
      umbra::detail::FilesystemFederationSaveCommitStore>(saveDirectory.path());
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
      L"durable-pending-time-constrained-member", L"time-managed", federationName));
  REQUIRE(restoredHandle == sourceHandle);
  REQUIRE(restoredReports.timeConstrainedEnabledReports.empty());

  REQUIRE_NOTHROW(restored->requestFederationRestore(saveLabel));
  REQUIRE_NOTHROW(restored->federateRestoreComplete());
  while (restored->evokeCallback(0.0)) {
  }

  // The exact public surfaces exercised map to §4.27/§4.32 restore behavior
  // and §8.5.5/§8.6.3 Enable Time Constrained service/callback behavior. This
  // covers one pending-role restore through a fresh filesystem store/registry;
  // it does not assert process-restart or general callback restoration.
  REQUIRE(restoredReports.timeConstrainedEnabledReports.size() == 1U);
  REQUIRE(restoredReports.timeConstrainedEnabledReports.front().value == L"0");
  REQUIRE(restoredReports.federationRestoredReportCount == 1U);

  REQUIRE_NOTHROW(restored->resignFederationExecution(NO_ACTION));
  REQUIRE_NOTHROW(restored->destroyFederationExecution(federationName));
  REQUIRE_NOTHROW(restored->disconnect());
}
