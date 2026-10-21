#include "ieee1516_2025_public_federation_restore_test_support.hpp"
#include "ieee1516_2025_federation_management_fixture_support.hpp"
#include "internal/federation/federation_state_image.hpp"

#include <cstdint>
#include <set>
#include <string>
#include <vector>

TEST_CASE(
    "Embedded public fresh-registry restore rebinds an explicit synchronization point",
    "[integration][development-profile][federation-management][save-restore]"
    "[durable-save][filesystem][fresh-registry][multi-federate]"
    "[synchronization][restore-member-identity-rebinding]"
    "[rti.service.register-federation-synchronization-point]"
    "[rti.service.synchronization-point-achieved]"
    "[rti.service.request-federation-save][rti.service.federate-save-begun]"
    "[rti.service.federate-save-complete][rti.service.request-federation-restore]"
    "[rti.service.federate-restore-complete]"
    "[federate.callback.announce-synchronization-point]"
    "[federate.callback.federation-synchronized]"
    "[federate.callback.initiate-federate-save]"
    "[federate.callback.initiate-federate-restore]"
    "[federate.callback.federation-restored][requirement:hla-1516.1-2025:clause-4.1.2]"
    "[2025]") {
  using public_federation_restore_test_support::ScopedEmbeddedFederationRegistry;
  using public_federation_restore_test_support::configurationForServiceReportDirectory;
  using public_federation_restore_test_support::temporaryFederationSaveDirectory;
  using public_federation_restore_test_support::temporaryServiceReportDirectory;

  auto const saveDirectory = temporaryFederationSaveDirectory();
  auto const reportDirectory = temporaryServiceReportDirectory();
  auto configuration =
      configurationForServiceReportDirectory(reportDirectory.path());
  configuration.withRtiAddress(L"in-process");
  auto const federationName = nextFederationName();
  auto const fomModule =
      resourcePath("examples/RestaurantFOMmodule-2025.xml").wstring();
  std::wstring const saveLabel = L"explicit-sync-point-identity-rebinding";
  std::wstring const syncLabel = L"explicit-two-member-point";
  std::vector<unsigned char> const tagBytes{0x53U, 0x59U, 0x4EU, 0x43U};
  VariableLengthData const tag(tagBytes.data(), tagBytes.size());
  auto const drain = [](
                          RTIambassador& first,
                          RTIambassador& second) {
    for (int pass = 0; pass != 32; ++pass) {
      static_cast<void>(first.evokeCallback(0.0));
      static_cast<void>(second.evokeCallback(0.0));
    }
  };

  FederateHandle savedFirstHandle;
  FederateHandle savedSecondHandle;
  std::set<std::uint64_t> savedMemberIds;
  {
    auto const sourceStore =
        std::make_shared<umbra::detail::FilesystemFederationSaveCommitStore>(
            saveDirectory.path());
    auto const sourceRegistry =
        std::make_shared<umbra::detail::EmbeddedFederationRegistry>(
            nullptr, sourceStore);
    ScopedEmbeddedFederationRegistry sourceScope(sourceRegistry);
    ReportingFederateAmbassador firstReports;
    ReportingFederateAmbassador secondReports;
    auto first = makeRti();
    auto second = makeRti();

    REQUIRE_NOTHROW(first->connect(firstReports, HLA_EVOKED, configuration));
    REQUIRE_NOTHROW(second->connect(secondReports, HLA_EVOKED, configuration));
    REQUIRE_NOTHROW(first->createFederationExecution(
        federationName, fomModule, standard_hla::mom::integer64_time));
    REQUIRE_NOTHROW(savedFirstHandle = first->joinFederationExecution(
        L"saved-sync-first", L"participant", federationName));
    REQUIRE_NOTHROW(savedSecondHandle = second->joinFederationExecution(
        L"saved-sync-second", L"participant", federationName));

    auto const savedFirstId =
        rti1516_2025::umbra_binding_detail::federateHandleValue(
            savedFirstHandle);
    auto const savedSecondId =
        rti1516_2025::umbra_binding_detail::federateHandleValue(
            savedSecondHandle);
    REQUIRE(savedFirstId.has_value());
    REQUIRE(savedSecondId.has_value());
    savedMemberIds = {*savedFirstId, *savedSecondId};
    REQUIRE(savedMemberIds.size() == 2U);

    FederateHandleSet synchronizationSet;
    synchronizationSet.insert(savedFirstHandle);
    synchronizationSet.insert(savedSecondHandle);
    REQUIRE_NOTHROW(first->registerFederationSynchronizationPoint(
        syncLabel, tag, synchronizationSet));
    drain(*first, *second);
    REQUIRE(firstReports.synchronizationPointRegistrationReports.size() == 1U);
    REQUIRE(firstReports.synchronizationPointRegistrationReports.front().succeeded);
    REQUIRE(firstReports.synchronizationPointAnnouncementReports.size() == 1U);
    REQUIRE(secondReports.synchronizationPointAnnouncementReports.size() == 1U);
    REQUIRE(firstReports.synchronizationPointAnnouncementReports.front().label ==
            syncLabel);
    REQUIRE(secondReports.synchronizationPointAnnouncementReports.front().label ==
            syncLabel);

    REQUIRE_NOTHROW(first->requestFederationSave(saveLabel));
    drain(*first, *second);
    REQUIRE(firstReports.initiateFederateSaveReports ==
            std::vector<std::wstring>{saveLabel});
    REQUIRE(secondReports.initiateFederateSaveReports ==
            std::vector<std::wstring>{saveLabel});
    REQUIRE_NOTHROW(first->federateSaveBegun());
    REQUIRE_NOTHROW(second->federateSaveBegun());
    REQUIRE_NOTHROW(first->federateSaveComplete());
    REQUIRE_NOTHROW(second->federateSaveComplete());
    drain(*first, *second);
    REQUIRE(firstReports.federationSavedReportCount == 1U);
    REQUIRE(secondReports.federationSavedReportCount == 1U);

    auto const durable = sourceStore->load(federationName, saveLabel);
    REQUIRE(durable.has_value());
    auto const image = umbra::detail::FederationStateImageCodec::decode(
        durable->stateImage);
    REQUIRE(image.synchronizationPointCount == 1U);
    REQUIRE(image.synchronizationPoints.size() == 1U);
    auto const& savedPoint = image.synchronizationPoints.front();
    REQUIRE(savedPoint.label == syncLabel);
    REQUIRE(savedPoint.userSuppliedTag ==
            std::string(tagBytes.begin(), tagBytes.end()));
    REQUIRE_FALSE(savedPoint.lateJoinExpansionAllowed);
    REQUIRE(std::set<std::uint64_t>(
                savedPoint.synchronizationSet.begin(),
                savedPoint.synchronizationSet.end()) == savedMemberIds);
    REQUIRE(std::set<std::uint64_t>(
                savedPoint.announcedFederates.begin(),
                savedPoint.announcedFederates.end()) == savedMemberIds);
    REQUIRE(savedPoint.achievedFederates.empty());

    REQUIRE_NOTHROW(second->resignFederationExecution(NO_ACTION));
    REQUIRE_NOTHROW(first->resignFederationExecution(NO_ACTION));
    REQUIRE_NOTHROW(first->destroyFederationExecution(federationName));
    REQUIRE_NOTHROW(second->disconnect());
    REQUIRE_NOTHROW(first->disconnect());
  }

  {
    auto const freshStore =
        std::make_shared<umbra::detail::FilesystemFederationSaveCommitStore>(
            saveDirectory.path());
    auto const freshRegistry =
        std::make_shared<umbra::detail::EmbeddedFederationRegistry>(
            nullptr, freshStore);
    ScopedEmbeddedFederationRegistry freshScope(freshRegistry);
    ReportingFederateAmbassador firstReports;
    ReportingFederateAmbassador secondReports;
    auto first = makeRti();
    auto second = makeRti();

    REQUIRE_NOTHROW(first->connect(firstReports, HLA_EVOKED, configuration));
    REQUIRE_NOTHROW(second->connect(secondReports, HLA_EVOKED, configuration));
    REQUIRE_NOTHROW(first->createFederationExecution(
        federationName, fomModule, standard_hla::mom::integer64_time));
    REQUIRE_NOTHROW(first->joinFederationExecution(
        L"burned-sync-first", L"participant", federationName));
    REQUIRE_NOTHROW(first->resignFederationExecution(NO_ACTION));
    REQUIRE_NOTHROW(second->joinFederationExecution(
        L"burned-sync-second", L"participant", federationName));
    REQUIRE_NOTHROW(second->resignFederationExecution(NO_ACTION));

    FederateHandle restoredFirstHandle;
    FederateHandle restoredSecondHandle;
    REQUIRE_NOTHROW(restoredFirstHandle = first->joinFederationExecution(
        L"restored-sync-first", L"participant", federationName));
    REQUIRE_NOTHROW(restoredSecondHandle = second->joinFederationExecution(
        L"restored-sync-second", L"participant", federationName));
    auto const restoredFirstId =
        rti1516_2025::umbra_binding_detail::federateHandleValue(
            restoredFirstHandle);
    auto const restoredSecondId =
        rti1516_2025::umbra_binding_detail::federateHandleValue(
            restoredSecondHandle);
    REQUIRE(restoredFirstId.has_value());
    REQUIRE(restoredSecondId.has_value());
    REQUIRE_FALSE(savedMemberIds.contains(*restoredFirstId));
    REQUIRE_FALSE(savedMemberIds.contains(*restoredSecondId));

    REQUIRE_NOTHROW(first->requestFederationRestore(saveLabel));
    drain(*first, *second);
    REQUIRE(firstReports.requestFederationRestoreSucceededReports ==
            std::vector<std::wstring>{saveLabel});
    REQUIRE(firstReports.initiateFederateRestoreReports.size() == 1U);
    REQUIRE(secondReports.initiateFederateRestoreReports.size() == 1U);
    REQUIRE_NOTHROW(first->federateRestoreComplete());
    REQUIRE_NOTHROW(second->federateRestoreComplete());
    drain(*first, *second);
    REQUIRE(firstReports.federationRestoredReportCount == 1U);
    REQUIRE(secondReports.federationRestoredReportCount == 1U);

    REQUIRE_NOTHROW(first->synchronizationPointAchieved(syncLabel));
    drain(*first, *second);
    CHECK(firstReports.federationSynchronizedReports.empty());
    CHECK(secondReports.federationSynchronizedReports.empty());
    REQUIRE_NOTHROW(second->synchronizationPointAchieved(syncLabel));
    drain(*first, *second);
    REQUIRE(firstReports.federationSynchronizedReports.size() == 1U);
    REQUIRE(secondReports.federationSynchronizedReports.size() == 1U);
    REQUIRE(firstReports.federationSynchronizedReports.front().label == syncLabel);
    REQUIRE(secondReports.federationSynchronizedReports.front().label == syncLabel);
    REQUIRE(firstReports.federationSynchronizedReports.front().failedToSyncSet.empty());
    REQUIRE(secondReports.federationSynchronizedReports.front().failedToSyncSet.empty());

    REQUIRE_NOTHROW(second->resignFederationExecution(NO_ACTION));
    REQUIRE_NOTHROW(first->resignFederationExecution(NO_ACTION));
    REQUIRE_NOTHROW(first->destroyFederationExecution(federationName));
    REQUIRE_NOTHROW(second->disconnect());
    REQUIRE_NOTHROW(first->disconnect());
  }
}
