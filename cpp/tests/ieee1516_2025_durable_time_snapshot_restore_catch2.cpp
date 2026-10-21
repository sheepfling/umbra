#include "ieee1516_2025_federation_management_fixture_support.hpp"

TEST_CASE(
    "Filesystem restore rehydrates saved logical time and actual lookahead",
    "[integration][development-profile][federation-management][save-restore]"
    "[durable-save][filesystem][fresh-registry][restore][time-management]"
    "[restore-time-window-state]"
    "[rti.service.request-federation-save][rti.service.federate-save-begun]"
    "[rti.service.federate-save-complete][rti.service.request-federation-restore]"
    "[rti.service.federate-restore-complete][rti.service.query-logical-time]"
    "[rti.service.query-lookahead][rti.service.modify-lookahead]"
    "[rti.service.time-advance-request]") {
  auto const saveDirectory = temporaryFederationSaveDirectory();
  auto const saveStore = std::make_shared<umbra::detail::FilesystemFederationSaveCommitStore>(
      saveDirectory.path());
  auto const sourceRegistry = std::make_shared<umbra::detail::EmbeddedFederationRegistry>(
      nullptr,
      saveStore);
  ScopedEmbeddedFederationRegistry sourceScope(sourceRegistry);

  auto const federationName = nextFederationName();
  auto const fomModule = resourcePath("examples/RestaurantFOMmodule-2025.xml").wstring();
  std::wstring const saveLabel = L"durable-time-window-snapshot";
  ReportingFederateAmbassador sourceReports;
  auto source = makeRti();
  FederateHandle sourceHandle;
  rti1516_2025::HLAinteger64Time logicalTime;
  rti1516_2025::HLAinteger64Interval lookahead;

  REQUIRE_NOTHROW(source->connect(sourceReports, HLA_EVOKED));
  REQUIRE_NOTHROW(source->createFederationExecution(
      federationName,
      fomModule,
      standard_hla::mom::integer64_time));
  REQUIRE_NOTHROW(sourceHandle = source->joinFederationExecution(
      L"durable-time-window-member", L"publisher", federationName));
  REQUIRE_NOTHROW(source->enableTimeRegulation(rti1516_2025::HLAinteger64Interval(2)));
  while (source->evokeCallback(0.0)) {
  }

  REQUIRE_NOTHROW(source->timeAdvanceRequest(rti1516_2025::HLAinteger64Time(3)));
  while (source->evokeCallback(0.0)) {
  }
  REQUIRE_NOTHROW(source->queryLogicalTime(logicalTime));
  REQUIRE(logicalTime.getTime() == 3);
  REQUIRE_NOTHROW(source->queryLookahead(lookahead));
  REQUIRE(lookahead.getInterval() == 2);

  REQUIRE_NOTHROW(source->requestFederationSave(saveLabel));
  while (source->evokeCallback(0.0)) {
  }
  REQUIRE_NOTHROW(source->federateSaveBegun());
  REQUIRE_NOTHROW(source->federateSaveComplete());
  while (source->evokeCallback(0.0)) {
  }
  REQUIRE(sourceReports.federationSavedReportCount == 1U);

  // Change both observable values after the durable snapshot. The later
  // fresh-registry restore must report the saved values, not retain this state.
  REQUIRE_NOTHROW(source->modifyLookahead(rti1516_2025::HLAinteger64Interval(5)));
  REQUIRE_NOTHROW(source->timeAdvanceRequest(rti1516_2025::HLAinteger64Time(5)));
  while (source->evokeCallback(0.0)) {
  }
  REQUIRE_NOTHROW(source->queryLogicalTime(logicalTime));
  REQUIRE(logicalTime.getTime() == 5);
  REQUIRE_NOTHROW(source->queryLookahead(lookahead));
  REQUIRE(lookahead.getInterval() == 5);

  REQUIRE_NOTHROW(source->resignFederationExecution(NO_ACTION));
  REQUIRE_NOTHROW(source->destroyFederationExecution(federationName));
  REQUIRE_NOTHROW(source->disconnect());

  auto const restoredSaveStore = std::make_shared<
      umbra::detail::FilesystemFederationSaveCommitStore>(saveDirectory.path());
  auto const freshRegistry = std::make_shared<umbra::detail::EmbeddedFederationRegistry>(
      nullptr,
      restoredSaveStore);
  ScopedEmbeddedFederationRegistry freshScope(freshRegistry);
  ReportingFederateAmbassador restoredReports;
  auto restored = makeRti();
  FederateHandle restoredHandle;

  REQUIRE_NOTHROW(restored->connect(restoredReports, HLA_EVOKED));
  REQUIRE_NOTHROW(restored->createFederationExecution(
      federationName,
      fomModule,
      standard_hla::mom::integer64_time));
  REQUIRE_NOTHROW(restoredHandle = restored->joinFederationExecution(
      L"durable-time-window-member", L"publisher", federationName));
  REQUIRE(restoredHandle == sourceHandle);
  REQUIRE_NOTHROW(restored->requestFederationRestore(saveLabel));
  while (restored->evokeCallback(0.0)) {
  }
  REQUIRE_NOTHROW(restored->federateRestoreComplete());
  while (restored->evokeCallback(0.0)) {
  }
  REQUIRE(restoredReports.federationRestoredReportCount == 1U);

  // The saved joined-federate state is restored through the public 2025 API.
  // This is one durable snapshot slice, not general time-management persistence.
  REQUIRE_NOTHROW(restored->queryLogicalTime(logicalTime));
  REQUIRE(logicalTime.getTime() == 3);
  REQUIRE_NOTHROW(restored->queryLookahead(lookahead));
  REQUIRE(lookahead.getInterval() == 2);

  REQUIRE_NOTHROW(restored->resignFederationExecution(NO_ACTION));
  REQUIRE_NOTHROW(restored->destroyFederationExecution(federationName));
  REQUIRE_NOTHROW(restored->disconnect());
}
