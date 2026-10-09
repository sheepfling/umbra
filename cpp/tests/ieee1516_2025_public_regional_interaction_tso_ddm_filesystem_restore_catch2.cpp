#include "ieee1516_2025_federation_management_test_support.hpp"
#include "ieee1516_2025_federation_management_fixture_support.hpp"
#include "ieee1516_2025_federation_listing_test_support.hpp"
#include "ieee1516_2025_public_federation_restore_test_support.hpp"

namespace {
TEST_CASE(
    "Embedded public restore rehydrates queued timestamped regional interaction and preserves filesystem service-report identity",
    "[integration][development-profile][federation-management][save-restore]"
    "[time-management][ddm][filesystem][service-report-file][service-reporting]"
    "[process-restart-regional-interaction-tso-ddm]"
    "[public-process-restart-regional-interaction-tso-ddm]"
    "[tso-queue-state][tso-payload-state][tso-regional-interaction-state]"
    "[rti.service.create-region][rti.service.set-range-bounds]"
    "[rti.service.commit-region-modifications]"
    "[rti.service.subscribe-interaction-class-with-regions]"
    "[rti.service.send-interaction-with-regions]"
    "[rti.service.request-federation-save][rti.service.time-advance-request]"
    "[rti.service.federate-save-begun][rti.service.federate-save-complete]"
    "[rti.service.request-federation-restore]"
    "[rti.service.federate-restore-complete][rti.service.get-range-bounds]"
    "[rti.service.enable-time-regulation][rti.service.enable-time-constrained]"
    "[federate.callback.receive-interaction][federate.callback.federation-restored]"
    "[federate.callback.initiate-federate-save][federate.callback.federation-saved]") {
  ReportingFederateAmbassador ownerReports;
  ReportingFederateAmbassador receiverReports;
  auto owner = makeRti();
  auto receiver = makeRti();
  auto const federationName = nextFederationName();
  auto const restaurantFom = resourcePath("examples/RestaurantFOMmodule-2025.xml").wstring();
  auto const switchFom =
      (std::filesystem::path(UMBRA_SOURCE_DIRECTORY) / "cpp" / "tests" / "data" /
       "switch-support-enabled-fom.xml")
          .wstring();
  std::vector<std::wstring> const fomModules{restaurantFom, switchFom};
  auto reportDirectory = temporaryServiceReportDirectory();
  auto configuration = configurationForServiceReportDirectory(reportDirectory.path());
  configuration.withRtiAddress(L"in-process");
  unsigned char const parameterBytes[] = {0xC7, 0x19};
  unsigned char const tagBytes[] = {0x52, 0x47, 0x43, 0x48};
  ParameterHandleValueMap parameterValues;
  VariableLengthData const tag(tagBytes, sizeof(tagBytes));

  // Give the owner the public embedded-profile directory.  The receiver uses
  // the default profile directory; this keeps the assertion focused on one
  // joined federate's immutable advertised report file.
  REQUIRE_NOTHROW(owner->connect(ownerReports, HLA_EVOKED, configuration));
  REQUIRE_NOTHROW(receiver->connect(receiverReports, HLA_EVOKED));
  REQUIRE_NOTHROW(
      owner->createFederationExecution(federationName, fomModules, standard_hla::mom::integer64_time));
  FederateHandle ownerHandle;
  REQUIRE_NOTHROW(ownerHandle = owner->joinFederationExecution(
      L"public-regional-restore-owner", L"publisher", federationName));
  REQUIRE_NOTHROW(receiver->joinFederationExecution(
      L"public-regional-restore-receiver", L"subscriber", federationName));
  suppressDeclarationRelevanceAdvisories(*owner);
  suppressDeclarationRelevanceAdvisories(*receiver);

  auto const filesAtJoin = serviceReportFiles(reportDirectory.path());
  REQUIRE(filesAtJoin.size() == 1U);
  auto const reportFile = filesAtJoin.front();
  auto const initialReportText = readTextFile(reportFile);
  REQUIRE(std::filesystem::absolute(reportFile).lexically_normal() ==
          reportFile.lexically_normal());

  auto const interactionClass = owner->getInteractionClassHandle(
      fixture_hla::fom::main_course_served);
  auto const temperatureOk = owner->getParameterHandle(
      interactionClass,
      fixture_hla::fixture::temperature_ok);
  auto const serverId = owner->getDimensionHandle(fixture_hla::fixture::server_id);
  REQUIRE(interactionClass.isValid());
  REQUIRE(temperatureOk.isValid());
  REQUIRE(serverId.isValid());
  parameterValues.emplace(
      temperatureOk,
      VariableLengthData(parameterBytes, sizeof(parameterBytes)));
  REQUIRE_NOTHROW(owner->publishInteractionClass(interactionClass));
  REQUIRE_NOTHROW(owner->changeInteractionOrderType(interactionClass, TIMESTAMP));

  auto const sourceRegion = owner->createRegion(DimensionHandleSet{serverId});
  auto const receiverRegion = receiver->createRegion(DimensionHandleSet{serverId});
  REQUIRE_NOTHROW(owner->setRangeBounds(
      sourceRegion,
      serverId,
      RangeBounds(2UL, 4UL)));
  REQUIRE_NOTHROW(owner->commitRegionModifications(RegionHandleSet{sourceRegion}));
  REQUIRE_NOTHROW(receiver->setRangeBounds(
      receiverRegion,
      serverId,
      RangeBounds(2UL, 4UL)));
  REQUIRE_NOTHROW(receiver->commitRegionModifications(RegionHandleSet{receiverRegion}));
  REQUIRE_NOTHROW(receiver->subscribeInteractionClassWithRegions(
      interactionClass,
      RegionHandleSet{receiverRegion}));
  REQUIRE_NOTHROW(receiver->setConveyRegionDesignatorSetsSwitch(true));

  REQUIRE_NOTHROW(receiver->enableTimeConstrained());
  REQUIRE_FALSE(receiver->evokeCallback(0.0));
  REQUIRE(receiverReports.timeConstrainedEnabledReports.size() == 1U);
  REQUIRE_NOTHROW(owner->enableTimeRegulation(
      rti1516_2025::HLAinteger64Interval(1)));
  REQUIRE_FALSE(owner->evokeCallback(0.0));
  REQUIRE(ownerReports.timeRegulationEnabledReports.size() == 1U);

  // The message is later than the timed-save boundary, so it remains in the
  // receiver's TSO queue when the save image is committed.
  auto const retraction = owner->sendInteractionWithRegions(
      interactionClass,
      parameterValues,
      RegionHandleSet{sourceRegion},
      tag,
      rti1516_2025::HLAinteger64Time(9));
  REQUIRE(retraction.isValid());
  REQUIRE(receiverReports.timestampedInteractionReports.empty());

  std::wstring const saveLabel = L"public-regional-restore-queued";
  REQUIRE_NOTHROW(owner->requestFederationSave(
      saveLabel,
      rti1516_2025::HLAinteger64Time(7)));
  REQUIRE_NOTHROW(owner->timeAdvanceRequest(rti1516_2025::HLAinteger64Time(7)));
  REQUIRE_FALSE(owner->evokeCallback(0.0));
  REQUIRE(ownerReports.timeAdvanceGrantReports.size() == 1U);
  REQUIRE_NOTHROW(receiver->timeAdvanceRequest(rti1516_2025::HLAinteger64Time(7)));
  REQUIRE_FALSE(receiver->evokeCallback(0.0));
  REQUIRE(receiverReports.initiateFederateSaveReports ==
          std::vector<std::wstring>{saveLabel});
  REQUIRE(receiverReports.timestampedInteractionReports.empty());
  REQUIRE_FALSE(owner->evokeCallback(0.0));
  REQUIRE(ownerReports.initiateFederateSaveReports ==
          std::vector<std::wstring>{saveLabel});

  REQUIRE_NOTHROW(owner->federateSaveBegun());
  REQUIRE_NOTHROW(receiver->federateSaveBegun());
  REQUIRE_NOTHROW(owner->federateSaveComplete());
  REQUIRE_NOTHROW(receiver->federateSaveComplete());
  while (owner->evokeCallback(0.0)) {
  }
  while (receiver->evokeCallback(0.0)) {
  }
  REQUIRE(ownerReports.federationSavedReportCount == 1U);
  REQUIRE(receiverReports.federationSavedReportCount == 1U);
  auto const reportAfterSave = readTextFile(reportFile);
  REQUIRE(reportAfterSave.size() > initialReportText.size());
  REQUIRE(reportAfterSave.find("RequestFederationSave") != std::string::npos);
  REQUIRE(reportAfterSave.find("FederateSaveComplete") != std::string::npos);

  // Change the live source to a disjoint range.  Restore must recover the
  // saved [2,4) region and the queued interaction's invocation snapshot,
  // rather than re-reading this post-save live range.
  REQUIRE_NOTHROW(owner->setRangeBounds(
      sourceRegion,
      serverId,
      RangeBounds(9UL, 11UL)));
  REQUIRE_NOTHROW(owner->commitRegionModifications(RegionHandleSet{sourceRegion}));
  auto const mutatedBounds = owner->getRangeBounds(sourceRegion, serverId);
  REQUIRE(mutatedBounds.getLowerBound() == 9UL);
  REQUIRE(mutatedBounds.getUpperBound() == 11UL);

  REQUIRE_NOTHROW(owner->requestFederationRestore(saveLabel));
  while (owner->evokeCallback(0.0)) {
  }
  while (receiver->evokeCallback(0.0)) {
  }
  REQUIRE(ownerReports.requestFederationRestoreSucceededReports ==
          std::vector<std::wstring>{saveLabel});
  REQUIRE(ownerReports.federationRestoreBegunReportCount == 1U);
  REQUIRE(receiverReports.federationRestoreBegunReportCount == 1U);
  REQUIRE(ownerReports.initiateFederateRestoreReports.size() == 1U);
  REQUIRE(receiverReports.initiateFederateRestoreReports.size() == 1U);

  REQUIRE_NOTHROW(owner->federateRestoreComplete());
  REQUIRE_NOTHROW(receiver->federateRestoreComplete());
  while (owner->evokeCallback(0.0)) {
  }
  while (receiver->evokeCallback(0.0)) {
  }
  REQUIRE(ownerReports.federationRestoredReportCount == 1U);
  REQUIRE(receiverReports.federationRestoredReportCount == 1U);
  REQUIRE(receiverReports.timestampedInteractionReports.empty());

  auto const restoredBounds = owner->getRangeBounds(sourceRegion, serverId);
  REQUIRE(restoredBounds.getLowerBound() == 2UL);
  REQUIRE(restoredBounds.getUpperBound() == 4UL);
  auto const filesAfterRestore = serviceReportFiles(reportDirectory.path());
  REQUIRE(filesAfterRestore == std::vector<std::filesystem::path>{reportFile});
  auto const reportAfterRestore = readTextFile(reportFile);
  REQUIRE(reportAfterRestore.size() > reportAfterSave.size());
  REQUIRE(reportAfterRestore.find("RequestFederationRestore") != std::string::npos);
  REQUIRE(reportAfterRestore.find("FederateRestoreComplete") != std::string::npos);

  // Re-establish the GALT boundary and cross the restored queue entry.  The
  // restored callback carries the original source-region designator and
  // timestamp even though the live region was changed before restore.
  REQUIRE_NOTHROW(owner->timeAdvanceRequest(rti1516_2025::HLAinteger64Time(8)));
  REQUIRE_FALSE(owner->evokeCallback(0.0));
  REQUIRE_NOTHROW(receiver->timeAdvanceRequest(rti1516_2025::HLAinteger64Time(9)));
  REQUIRE_FALSE(receiver->evokeCallback(0.0));
  REQUIRE(receiverReports.timestampedInteractionReports.size() == 1U);
  auto const& report = receiverReports.timestampedInteractionReports.front();
  REQUIRE(report.interactionClass == interactionClass);
  REQUIRE(report.parameterValues.size() == 1U);
  REQUIRE(report.parameterValues.contains(temperatureOk));
  REQUIRE(variableLengthDataBytes(report.parameterValues.at(temperatureOk)) ==
          std::vector<unsigned char>(parameterBytes, parameterBytes + sizeof(parameterBytes)));
  REQUIRE(variableLengthDataBytes(report.userSuppliedTag) ==
          std::vector<unsigned char>(tagBytes, tagBytes + sizeof(tagBytes)));
  REQUIRE(report.producingFederate == ownerHandle);
  REQUIRE(report.timeImplementationName == standard_hla::mom::integer64_time);
  REQUIRE(report.timeValue == L"9");
  REQUIRE(report.sentOrderType == TIMESTAMP);
  REQUIRE(report.receivedOrderType == TIMESTAMP);
  REQUIRE(report.sentRegionsSupplied);
  REQUIRE(report.sentRegions == RegionHandleSet{sourceRegion});
  REQUIRE(report.retractionSupplied);
  REQUIRE(report.retractionValid);

  REQUIRE_NOTHROW(receiver->unsubscribeInteractionClassWithRegions(
      interactionClass,
      RegionHandleSet{receiverRegion}));
  REQUIRE_NOTHROW(receiver->deleteRegion(receiverRegion));
  REQUIRE_NOTHROW(owner->deleteRegion(sourceRegion));
  REQUIRE_NOTHROW(receiver->resignFederationExecution(NO_ACTION));
  REQUIRE_NOTHROW(owner->resignFederationExecution(CANCEL_THEN_DELETE_THEN_DIVEST));
  REQUIRE_NOTHROW(owner->destroyFederationExecution(federationName));
  REQUIRE_NOTHROW(receiver->disconnect());
  REQUIRE_NOTHROW(owner->disconnect());
}
} // namespace
